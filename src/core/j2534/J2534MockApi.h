#pragma once

#include "core/j2534/IJ2534Api.h"

#include <mutex>
#include <queue>

namespace core::j2534 {

class J2534MockApi final : public IJ2534Api {
public:
    long Open(void* name, unsigned long* deviceId) override;
    long Close(unsigned long deviceId) override;
    long Connect(unsigned long deviceId,
                 unsigned long protocolId,
                 unsigned long flags,
                 unsigned long baud,
                 unsigned long* channelId) override;
    long Disconnect(unsigned long channelId) override;
    long ReadMsgs(unsigned long channelId,
                  J2534Msg* msgs,
                  unsigned long* numMsgs,
                  unsigned long timeoutMs) override;
    long WriteMsgs(unsigned long channelId,
                   const J2534Msg* msgs,
                   unsigned long* numMsgs,
                   unsigned long timeoutMs) override;
    long StartPeriodicMsg(unsigned long channelId,
                          const J2534Msg* msg,
                          unsigned long* msgId,
                          unsigned long timeIntervalMs) override;
    long StopPeriodicMsg(unsigned long channelId, unsigned long msgId) override;
    long StartMsgFilter(unsigned long channelId,
                        unsigned long filterType,
                        const J2534Msg* mask,
                        const J2534Msg* pattern,
                        const J2534Msg* flowControl,
                        unsigned long* filterId) override;
    long StopMsgFilter(unsigned long channelId, unsigned long filterId) override;
    long Ioctl(unsigned long channelId,
               unsigned long ioctlId,
               void* input,
               void* output) override;

private:
    std::mutex mtx_;
    bool opened_{false};
    bool connected_{false};
    std::queue<J2534Msg> rxQueue_;
};

}  // namespace core::j2534
