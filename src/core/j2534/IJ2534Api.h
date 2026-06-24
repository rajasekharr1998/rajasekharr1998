#pragma once

#include "core/j2534/J2534Types.h"

namespace core::j2534 {

class IJ2534Api {
public:
    virtual ~IJ2534Api() = default;

    virtual long Open(void* name, unsigned long* deviceId) = 0;
    virtual long Close(unsigned long deviceId) = 0;
    virtual long Connect(unsigned long deviceId,
                         unsigned long protocolId,
                         unsigned long flags,
                         unsigned long baud,
                         unsigned long* channelId) = 0;
    virtual long Disconnect(unsigned long channelId) = 0;
    virtual long ReadMsgs(unsigned long channelId,
                          J2534Msg* msgs,
                          unsigned long* numMsgs,
                          unsigned long timeoutMs) = 0;
    virtual long WriteMsgs(unsigned long channelId,
                           const J2534Msg* msgs,
                           unsigned long* numMsgs,
                           unsigned long timeoutMs) = 0;
    virtual long StartPeriodicMsg(unsigned long channelId,
                                  const J2534Msg* msg,
                                  unsigned long* msgId,
                                  unsigned long timeIntervalMs) = 0;
    virtual long StopPeriodicMsg(unsigned long channelId, unsigned long msgId) = 0;
    virtual long StartMsgFilter(unsigned long channelId,
                                unsigned long filterType,
                                const J2534Msg* mask,
                                const J2534Msg* pattern,
                                const J2534Msg* flowControl,
                                unsigned long* filterId) = 0;
    virtual long StopMsgFilter(unsigned long channelId, unsigned long filterId) = 0;
    virtual long Ioctl(unsigned long channelId,
                       unsigned long ioctlId,
                       void* input,
                       void* output) = 0;
};

}  // namespace core::j2534
