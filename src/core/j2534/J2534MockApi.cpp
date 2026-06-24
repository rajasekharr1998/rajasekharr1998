#include "core/j2534/J2534MockApi.h"

#include <array>

namespace core::j2534 {

namespace {
constexpr unsigned long kMockDeviceId = 1;
constexpr unsigned long kMockChannelId = 100;

std::vector<uint8_t> buildUdsResponse(const std::vector<uint8_t>& req) {
    if (req.empty()) return {0x7F, 0x00, 0x13};

    const uint8_t sid = req[0];
    switch (sid) {
        case 0x10:
            if (req.size() < 2) return {0x7F, sid, 0x13};
            return {0x50, req[1], 0x00, 0x32, 0x01, 0xF4};
        case 0x11:
            if (req.size() < 2) return {0x7F, sid, 0x13};
            return {0x51, req[1]};
        case 0x27:
            if (req.size() < 2) return {0x7F, sid, 0x13};
            if ((req[1] & 0x01U) == 1U) return {0x67, req[1], 0x12, 0x34, 0x56, 0x78};
            if (req.size() < 6) return {0x7F, sid, 0x13};
            return {0x67, req[1]};
        case 0x22:
            if (req.size() < 3) return {0x7F, sid, 0x13};
            if (req[1] == 0xF1 && req[2] == 0x90) {
                return {0x62, 0xF1, 0x90, '1', 'H', 'G', 'C', 'M', '8', '2', '6', '3', '3', 'A', '0', '0', '4', '3', '5', '2'};
            }
            return {0x7F, sid, 0x31};
        case 0x2E:
            if (req.size() < 4) return {0x7F, sid, 0x13};
            return {0x6E, req[1], req[2]};
        case 0x3E:
            return {0x7E, 0x00};
        case 0x34:
            return {0x74, 0x20, 0x00, 0x10};
        case 0x36:
            if (req.size() < 2) return {0x7F, sid, 0x13};
            return {0x76, req[1]};
        case 0x37:
            return {0x77};
        default:
            return {0x7F, sid, 0x11};
    }
}
}  // namespace

long J2534MockApi::Open(void* /*name*/, unsigned long* deviceId) {
    std::scoped_lock lk(mtx_);
    opened_ = true;
    if (deviceId) *deviceId = kMockDeviceId;
    return STATUS_NOERROR;
}

long J2534MockApi::Close(unsigned long deviceId) {
    std::scoped_lock lk(mtx_);
    if (!opened_ || deviceId != kMockDeviceId) return ERR_INVALID_DEVICE_ID;
    opened_ = false;
    connected_ = false;
    while (!rxQueue_.empty()) rxQueue_.pop();
    return STATUS_NOERROR;
}

long J2534MockApi::Connect(unsigned long deviceId, unsigned long /*protocolId*/, unsigned long /*flags*/, unsigned long /*baud*/, unsigned long* channelId) {
    std::scoped_lock lk(mtx_);
    if (!opened_ || deviceId != kMockDeviceId) return ERR_INVALID_DEVICE_ID;
    connected_ = true;
    if (channelId) *channelId = kMockChannelId;
    return STATUS_NOERROR;
}

long J2534MockApi::Disconnect(unsigned long channelId) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    connected_ = false;
    return STATUS_NOERROR;
}

long J2534MockApi::ReadMsgs(unsigned long channelId, J2534Msg* msgs, unsigned long* numMsgs, unsigned long /*timeoutMs*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    if (!numMsgs || !msgs || *numMsgs == 0 || rxQueue_.empty()) {
        if (numMsgs) *numMsgs = 0;
        return STATUS_NOERROR;
    }

    msgs[0] = rxQueue_.front();
    rxQueue_.pop();
    *numMsgs = 1;
    return STATUS_NOERROR;
}

long J2534MockApi::WriteMsgs(unsigned long channelId, const J2534Msg* msgs, unsigned long* numMsgs, unsigned long /*timeoutMs*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    if (!msgs || !numMsgs || *numMsgs == 0) return ERR_FAILED;

    J2534Msg response{};
    response.protocolId = msgs[0].protocolId;
    response.data = buildUdsResponse(msgs[0].data);
    response.dataSize = static_cast<uint32_t>(response.data.size());
    rxQueue_.push(response);

    *numMsgs = 1;
    return STATUS_NOERROR;
}

long J2534MockApi::StartPeriodicMsg(unsigned long channelId, const J2534Msg* /*msg*/, unsigned long* msgId, unsigned long /*timeIntervalMs*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    if (msgId) *msgId = 200;
    return STATUS_NOERROR;
}

long J2534MockApi::StopPeriodicMsg(unsigned long channelId, unsigned long /*msgId*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    return STATUS_NOERROR;
}

long J2534MockApi::StartMsgFilter(unsigned long channelId, unsigned long /*filterType*/, const J2534Msg* /*mask*/, const J2534Msg* /*pattern*/, const J2534Msg* /*flowControl*/, unsigned long* filterId) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    if (filterId) *filterId = 300;
    return STATUS_NOERROR;
}

long J2534MockApi::StopMsgFilter(unsigned long channelId, unsigned long /*filterId*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    return STATUS_NOERROR;
}

long J2534MockApi::Ioctl(unsigned long channelId, unsigned long /*ioctlId*/, void* /*input*/, void* /*output*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != kMockChannelId) return ERR_INVALID_CHANNEL_ID;
    return STATUS_NOERROR;
}

}  // namespace core::j2534
