#pragma once

#include "core/j2534/IJ2534Api.h"

#include <mutex>
#include <string>

namespace core::j2534 {

class J2534DynamicApi final : public IJ2534Api {
public:
    explicit J2534DynamicApi(const std::wstring& dllPath);
    ~J2534DynamicApi() override;

    long Open(void* name, unsigned long* deviceId) override;
    long Close(unsigned long deviceId) override;
    long Connect(unsigned long deviceId, unsigned long protocolId, unsigned long flags,
                 unsigned long baud, unsigned long* channelId) override;
    long Disconnect(unsigned long channelId) override;
    long ReadMsgs(unsigned long channelId, J2534Msg* msgs,
                  unsigned long* numMsgs, unsigned long timeoutMs) override;
    long WriteMsgs(unsigned long channelId, const J2534Msg* msgs,
                   unsigned long* numMsgs, unsigned long timeoutMs) override;
    long StartPeriodicMsg(unsigned long channelId, const J2534Msg* msg,
                          unsigned long* msgId, unsigned long timeIntervalMs) override;
    long StopPeriodicMsg(unsigned long channelId, unsigned long msgId) override;
    long StartMsgFilter(unsigned long channelId, unsigned long filterType,
                        const J2534Msg* mask, const J2534Msg* pattern,
                        const J2534Msg* flowControl, unsigned long* filterId) override;
    long StopMsgFilter(unsigned long channelId, unsigned long filterId) override;
    long Ioctl(unsigned long channelId, unsigned long ioctlId, void* input, void* output) override;

private:
    void loadSymbols();
    void* module_{nullptr};
    std::mutex mtx_;

    using OpenFn = long(__stdcall*)(void*, unsigned long*);
    using CloseFn = long(__stdcall*)(unsigned long);
    using ConnectFn = long(__stdcall*)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long*);
    using DisconnectFn = long(__stdcall*)(unsigned long);
    using ReadMsgsFn = long(__stdcall*)(unsigned long, void*, unsigned long*, unsigned long);
    using WriteMsgsFn = long(__stdcall*)(unsigned long, void*, unsigned long*, unsigned long);
    using StartPeriodicMsgFn = long(__stdcall*)(unsigned long, void*, unsigned long*, unsigned long);
    using StopPeriodicMsgFn = long(__stdcall*)(unsigned long, unsigned long);
    using StartMsgFilterFn = long(__stdcall*)(unsigned long, unsigned long, void*, void*, void*, unsigned long*);
    using StopMsgFilterFn = long(__stdcall*)(unsigned long, unsigned long);
    using IoctlFn = long(__stdcall*)(unsigned long, unsigned long, void*, void*);

    OpenFn open_{nullptr};
    CloseFn close_{nullptr};
    ConnectFn connect_{nullptr};
    DisconnectFn disconnect_{nullptr};
    ReadMsgsFn readMsgs_{nullptr};
    WriteMsgsFn writeMsgs_{nullptr};
    StartPeriodicMsgFn startPeriodic_{nullptr};
    StopPeriodicMsgFn stopPeriodic_{nullptr};
    StartMsgFilterFn startFilter_{nullptr};
    StopMsgFilterFn stopFilter_{nullptr};
    IoctlFn ioctl_{nullptr};
};

}  // namespace core::j2534
