#include "core/transport/IsoTpEngine.h"

#include <algorithm>

namespace core::transport {

namespace {
constexpr size_t kCanDlc = 8;
constexpr uint8_t kSingleFrame = 0x00;
constexpr uint8_t kFirstFrame = 0x10;
constexpr uint8_t kConsecutiveFrame = 0x20;
constexpr uint8_t kFlowControl = 0x30;

std::vector<uint8_t> padded(std::vector<uint8_t> data) {
    data.resize(kCanDlc, 0x00);
    return data;
}
}  // namespace

IsoTpEngine::IsoTpEngine(IsoTpConfig config) : config_(config) {}

std::vector<CanFrame> IsoTpEngine::segmentForTransmit(const std::vector<uint8_t>& payload) const {
    std::vector<CanFrame> frames;
    if (payload.size() <= 7) {
        std::vector<uint8_t> data;
        data.push_back(static_cast<uint8_t>(kSingleFrame | payload.size()));
        data.insert(data.end(), payload.begin(), payload.end());
        frames.push_back({config_.txCanId, config_.extendedId, padded(data)});
        return frames;
    }

    const auto totalLength = payload.size();
    std::vector<uint8_t> first{
        static_cast<uint8_t>(kFirstFrame | ((totalLength >> 8) & 0x0F)),
        static_cast<uint8_t>(totalLength & 0xFF),
    };
    first.insert(first.end(), payload.begin(), payload.begin() + 6);
    frames.push_back({config_.txCanId, config_.extendedId, padded(first)});

    size_t offset = 6;
    uint8_t seq = 1;
    while (offset < payload.size()) {
        const auto chunkSize = std::min<size_t>(7, payload.size() - offset);
        std::vector<uint8_t> cf{static_cast<uint8_t>(kConsecutiveFrame | (seq & 0x0F))};
        cf.insert(cf.end(), payload.begin() + static_cast<std::ptrdiff_t>(offset),
                  payload.begin() + static_cast<std::ptrdiff_t>(offset + chunkSize));
        frames.push_back({config_.txCanId, config_.extendedId, padded(cf)});
        offset += chunkSize;
        seq = static_cast<uint8_t>((seq + 1) & 0x0F);
    }

    return frames;
}

Result<std::vector<uint8_t>> IsoTpEngine::reassembleReceived(const std::vector<CanFrame>& frames) const {
    if (frames.empty()) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "No ISO-TP frames supplied");
    }

    const auto& first = frames.front();
    if (first.data.empty()) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Empty CAN frame");
    }

    const auto frameType = static_cast<uint8_t>(first.data[0] & 0xF0);
    if (frameType == kSingleFrame) {
        const size_t len = first.data[0] & 0x0F;
        if (len > first.data.size() - 1) {
            return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Single frame length exceeds DLC");
        }
        return Result<std::vector<uint8_t>>::success(
            std::vector<uint8_t>(first.data.begin() + 1, first.data.begin() + 1 + static_cast<std::ptrdiff_t>(len)));
    }

    if (frameType != kFirstFrame || first.data.size() < 8) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Expected single or first frame");
    }

    const size_t totalLength = ((first.data[0] & 0x0F) << 8U) | first.data[1];
    std::vector<uint8_t> payload;
    payload.reserve(totalLength);
    payload.insert(payload.end(), first.data.begin() + 2, first.data.begin() + 8);

    uint8_t expectedSeq = 1;
    for (size_t i = 1; i < frames.size() && payload.size() < totalLength; ++i) {
        const auto& cf = frames[i];
        if (cf.data.empty() || (cf.data[0] & 0xF0) != kConsecutiveFrame) {
            return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Expected consecutive frame");
        }
        const auto seq = static_cast<uint8_t>(cf.data[0] & 0x0F);
        if (seq != expectedSeq) {
            return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Consecutive frame sequence mismatch");
        }
        const auto remaining = totalLength - payload.size();
        const auto copyLen = std::min<size_t>(7, remaining);
        if (cf.data.size() < copyLen + 1) {
            return Result<std::vector<uint8_t>>::failure(DiagErr::ProtocolViolation, "Consecutive frame shorter than expected");
        }
        payload.insert(payload.end(), cf.data.begin() + 1, cf.data.begin() + 1 + static_cast<std::ptrdiff_t>(copyLen));
        expectedSeq = static_cast<uint8_t>((expectedSeq + 1) & 0x0F);
    }

    if (payload.size() != totalLength) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::Timeout, "Incomplete ISO-TP message");
    }

    return Result<std::vector<uint8_t>>::success(payload);
}

CanFrame IsoTpEngine::makeFlowControl(uint8_t flowStatus) const {
    return {config_.txCanId, config_.extendedId, {static_cast<uint8_t>(kFlowControl | (flowStatus & 0x0F)), config_.blockSize, config_.stMinMs, 0, 0, 0, 0, 0}};
}

}  // namespace core::transport
