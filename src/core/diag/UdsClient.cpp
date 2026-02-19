#include "core/diag/UdsClient.h"

#include "core/diag/NrcDecoder.h"
#include "core/j2534/J2534Error.h"

namespace core::diag {

using core::j2534::J2534Msg;

UdsClient::UdsClient(j2534::IJ2534Api& api, unsigned long channelId)
    : api_(api), channelId_(channelId) {}

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
    J2534Msg tx{};
    tx.protocolId = static_cast<uint32_t>(j2534::Protocol::Iso15765);
    tx.data = payload;
    tx.dataSize = static_cast<uint32_t>(payload.size());

    unsigned long txCount = 1;
    const auto wr = api_.WriteMsgs(channelId_, &tx, &txCount, 100);
    if (wr != j2534::STATUS_NOERROR || txCount != 1) {
        return Result<std::vector<uint8_t>>::failure(j2534::mapJ2534Error(wr), "WriteMsgs failed");
    }

    J2534Msg rx{};
    unsigned long rxCount = 1;
    const auto rd = api_.ReadMsgs(channelId_, &rx, &rxCount, 500);
    if (rd != j2534::STATUS_NOERROR || rxCount != 1) {
        return Result<std::vector<uint8_t>>::failure(j2534::mapJ2534Error(rd), "ReadMsgs timeout/no data");
    }

    if (rx.data.size() >= 3 && rx.data[0] == 0x7F) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::NegativeResponse, decodeNrc(rx.data[2]));
    }

    return Result<std::vector<uint8_t>>::success(rx.data);
}

}  // namespace core::diag
