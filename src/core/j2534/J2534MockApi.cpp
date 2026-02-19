#include "core/j2534/J2534MockApi.h"

namespace core::j2534 {

namespace {
constexpr long STATUS_NOERROR = 0;
constexpr long ERR_FAILED = 0x07;
constexpr long ERR_INVALID_CHANNEL_ID = 0x02;
}

long J2534MockApi::Open(void* /*name*/, unsigned long* deviceId) {
    std::scoped_lock lk(mtx_);
    opened_ = true;
    if (deviceId) {
        *deviceId = 1;
    }
    return STATUS_NOERROR;
}

long J2534MockApi::Close(unsigned long /*deviceId*/) {
    std::scoped_lock lk(mtx_);
    opened_ = false;
    connected_ = false;
    return STATUS_NOERROR;
}

long J2534MockApi::Connect(unsigned long /*deviceId*/,
                           unsigned long /*protocolId*/,
                           unsigned long /*flags*/,
                           unsigned long /*baud*/,
                           unsigned long* channelId) {
    std::scoped_lock lk(mtx_);
    if (!opened_) return ERR_FAILED;
    connected_ = true;
    if (channelId) {
        *channelId = 100;
    }
    return STATUS_NOERROR;
}

long J2534MockApi::Disconnect(unsigned long channelId) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != 100) return ERR_INVALID_CHANNEL_ID;
    connected_ = false;
    return STATUS_NOERROR;
}

long J2534MockApi::ReadMsgs(unsigned long channelId,
                            J2534Msg* msgs,
                            unsigned long* numMsgs,
                            unsigned long /*timeoutMs*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != 100) return ERR_INVALID_CHANNEL_ID;
    if (!numMsgs || !msgs || *numMsgs == 0 || rxQueue_.empty()) {
        if (numMsgs) *numMsgs = 0;
        return STATUS_NOERROR;
    }

    msgs[0] = rxQueue_.front();
    rxQueue_.pop();
    *numMsgs = 1;
    return STATUS_NOERROR;
}

long J2534MockApi::WriteMsgs(unsigned long channelId,
                             const J2534Msg* msgs,
                             unsigned long* numMsgs,
                             unsigned long /*timeoutMs*/) {
    std::scoped_lock lk(mtx_);
    if (!connected_ || channelId != 100) return ERR_INVALID_CHANNEL_ID;
    if (!msgs || !numMsgs || *numMsgs == 0) return STATUS_NOERROR;

    // loopback behavior to simulate ECU echo in early development.
    rxQueue_.push(msgs[0]);
    *numMsgs = 1;
    return STATUS_NOERROR;
}

long J2534MockApi::StartPeriodicMsg(unsigned long /*channelId*/,
                                    const J2534Msg* /*msg*/,
                                    unsigned long* msgId,
                                    unsigned long /*timeIntervalMs*/) {
    if (msgId) *msgId = 200;
    return STATUS_NOERROR;
}

long J2534MockApi::StopPeriodicMsg(unsigned long /*channelId*/, unsigned long /*msgId*/) {
    return STATUS_NOERROR;
}

long J2534MockApi::StartMsgFilter(unsigned long /*channelId*/,
                                  unsigned long /*filterType*/,
                                  const J2534Msg* /*mask*/,
                                  const J2534Msg* /*pattern*/,
                                  const J2534Msg* /*flowControl*/,
                                  unsigned long* filterId) {
    if (filterId) *filterId = 300;
    return STATUS_NOERROR;
}

long J2534MockApi::StopMsgFilter(unsigned long /*channelId*/, unsigned long /*filterId*/) {
    return STATUS_NOERROR;
}

long J2534MockApi::Ioctl(unsigned long /*channelId*/,
                         unsigned long /*ioctlId*/,
                         void* /*input*/,
                         void* /*output*/) {
    return STATUS_NOERROR;
}

}  // namespace core::j2534
