#include "core/diag/UdsClient.h"

#include "core/diag/NrcDecoder.h"
#include "core/j2534/J2534Error.h"

#include <chrono>

namespace core::diag {

using core::j2534::J2534Msg;

namespace {
constexpr unsigned long kReadTimeoutMs = 500;
constexpr uint8_t kFirstFrame = 0x10;
constexpr uint8_t kConsecutiveFrame = 0x20;
constexpr uint8_t kSingleFrame = 0x00;
}

UdsClient::UdsClient(j2534::IJ2534Api& api,
                     unsigned long channelId,
                     transport::IsoTpConfig transportConfig,
                     logging::TraceLogger* logger)
    : api_(api), channelId_(channelId), isotp_(transportConfig), logger_(logger) {}

Result<std::vector<uint8_t>> UdsClient::sessionControl(uint8_t sessionType) { return sendAndReceive({0x10, sessionType}); }
Result<std::vector<uint8_t>> UdsClient::ecuReset(uint8_t resetType) { return sendAndReceive({0x11, resetType}); }
Result<std::vector<uint8_t>> UdsClient::securityAccess(uint8_t subFn, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> req{0x27, subFn};
    req.insert(req.end(), payload.begin(), payload.end());
    return sendAndReceive(req);
}
Result<std::vector<uint8_t>> UdsClient::readDataByIdentifier(uint16_t did) {
    return sendAndReceive({0x22, static_cast<uint8_t>((did >> 8) & 0xFF), static_cast<uint8_t>(did & 0xFF)});
}
Result<std::vector<uint8_t>> UdsClient::writeDataByIdentifier(uint16_t did, const std::vector<uint8_t>& val) {
    std::vector<uint8_t> req{0x2E, static_cast<uint8_t>((did >> 8) & 0xFF), static_cast<uint8_t>(did & 0xFF)};
    req.insert(req.end(), val.begin(), val.end());
    return sendAndReceive(req);
}
Result<std::vector<uint8_t>> UdsClient::testerPresent() { return sendAndReceive({0x3E, 0x00}); }
Result<std::vector<uint8_t>> UdsClient::requestDownload(const std::vector<uint8_t>& args) {
    std::vector<uint8_t> req{0x34};
    req.insert(req.end(), args.begin(), args.end());
    return sendAndReceive(req);
}
Result<std::vector<uint8_t>> UdsClient::transferData(uint8_t blockCounter, const std::vector<uint8_t>& chunk) {
    std::vector<uint8_t> req{0x36, blockCounter};
    req.insert(req.end(), chunk.begin(), chunk.end());
    return sendAndReceive(req);
}
Result<std::vector<uint8_t>> UdsClient::transferExit() { return sendAndReceive({0x37}); }

Result<std::vector<uint8_t>> UdsClient::sendAndReceive(const std::vector<uint8_t>& payload) {
    std::scoped_lock requestLock(requestMtx_);
    const auto requestStart = std::chrono::steady_clock::now();
    const auto txFrames = isotp_.segmentForTransmit(payload);
    for (const auto& frame : txFrames) {
        J2534Msg tx{};
        tx.protocolId = static_cast<uint32_t>(j2534::Protocol::Iso15765);
        tx.data = frame.data;
        tx.dataSize = static_cast<uint32_t>(tx.data.size());

        unsigned long txCount = 1;
        const auto wr = api_.WriteMsgs(channelId_, &tx, &txCount, 100);
        if (logger_) logger_->logFrame(logging::Direction::Tx, frame);
        if (wr != j2534::STATUS_NOERROR || txCount != 1) {
            return Result<std::vector<uint8_t>>::failure(j2534::mapJ2534Error(wr), "WriteMsgs failed");
        }
    }

    auto rx = readIsoTpResponse();
    if (logger_ && rx.ok()) {
        logger_->logText(logging::Direction::Info,
                         "UDS response time " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - requestStart).count()) + "ms");
    }
    if (!rx.ok()) {
        return rx;
    }

    if (rx.value.size() >= 3 && rx.value[0] == 0x7F) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::NegativeResponse, decodeNrc(rx.value[2]));
    }

    return rx;
}

Result<std::vector<uint8_t>> UdsClient::readIsoTpResponse() {
    std::vector<transport::CanFrame> frames;
    for (int reads = 0; reads < 16; ++reads) {
        J2534Msg rx{};
        unsigned long rxCount = 1;
        const auto rd = api_.ReadMsgs(channelId_, &rx, &rxCount, kReadTimeoutMs);
        if (rd != j2534::STATUS_NOERROR || rxCount != 1) {
            return Result<std::vector<uint8_t>>::failure(j2534::mapJ2534Error(rd), "ReadMsgs timeout/no data");
        }

        transport::CanFrame frame{0, false, rx.data};
        if (logger_) logger_->logFrame(logging::Direction::Rx, frame);
        frames.push_back(frame);

        if (rx.data.empty()) {
            return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Empty ISO-TP response frame");
        }
        const auto type = static_cast<uint8_t>(rx.data[0] & 0xF0);
        if (type == kSingleFrame) {
            return isotp_.reassembleReceived(frames);
        }
        if (type == kFirstFrame) {
            const auto totalLength = ((rx.data[0] & 0x0F) << 8U) | rx.data[1];
            const auto expectedFrames = 1 + ((totalLength > 6 ? totalLength - 6 : 0) + 6) / 7;
            while (frames.size() < expectedFrames) {
                J2534Msg cf{};
                unsigned long cfCount = 1;
                const auto cfStatus = api_.ReadMsgs(channelId_, &cf, &cfCount, kReadTimeoutMs);
                if (cfStatus != j2534::STATUS_NOERROR || cfCount != 1) {
                    return Result<std::vector<uint8_t>>::failure(j2534::mapJ2534Error(cfStatus), "Incomplete ISO-TP response");
                }
                transport::CanFrame cfFrame{0, false, cf.data};
                if (logger_) logger_->logFrame(logging::Direction::Rx, cfFrame);
                frames.push_back(cfFrame);
            }
            return isotp_.reassembleReceived(frames);
        }
        if (type != kConsecutiveFrame) {
            return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Unexpected ISO-TP response frame type");
        }
    }
    return Result<std::vector<uint8_t>>::failure(DiagErr::Timeout, "ISO-TP read limit exceeded");
}

}  // namespace core::diag
