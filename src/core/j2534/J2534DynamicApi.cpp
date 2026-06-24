#include "core/j2534/J2534DynamicApi.h"

#ifdef _WIN32

#include <windows.h>

#include <stdexcept>

namespace core::j2534 {

namespace {
constexpr size_t kMaxData = 4128;
struct NativeMsg {
    unsigned long ProtocolID;
    unsigned long RxStatus;
    unsigned long TxFlags;
    unsigned long Timestamp;
    unsigned long DataSize;
    unsigned long ExtraDataIndex;
    unsigned char Data[kMaxData];
};

NativeMsg toNative(const J2534Msg& msg) {
    NativeMsg n{};
    n.ProtocolID = msg.protocolId;
    n.RxStatus = msg.rxStatus;
    n.TxFlags = msg.txFlags;
    n.Timestamp = msg.timestamp;
    n.DataSize = msg.dataSize;
    n.ExtraDataIndex = msg.extraDataIndex;
    for (size_t i = 0; i < msg.data.size() && i < kMaxData; ++i) {
        n.Data[i] = msg.data[i];
    }
    return n;
}

J2534Msg fromNative(const NativeMsg& msg) {
    J2534Msg n{};
    n.protocolId = msg.ProtocolID;
    n.rxStatus = msg.RxStatus;
    n.txFlags = msg.TxFlags;
    n.timestamp = msg.Timestamp;
    n.dataSize = msg.DataSize;
    n.extraDataIndex = msg.ExtraDataIndex;
    n.data.assign(msg.Data, msg.Data + msg.DataSize);
    return n;
}

void* mustResolve(HMODULE module, const char* name) {
    void* p = reinterpret_cast<void*>(::GetProcAddress(module, name));
    if (!p) throw std::runtime_error(name);
    return p;
}
}  // namespace

J2534DynamicApi::J2534DynamicApi(const std::wstring& dllPath) {
    module_ = ::LoadLibraryW(dllPath.c_str());
    if (!module_) throw std::runtime_error("LoadLibraryW failed");
    loadSymbols();
}

J2534DynamicApi::~J2534DynamicApi() {
    if (module_) {
        ::FreeLibrary(static_cast<HMODULE>(module_));
    }
}

void J2534DynamicApi::loadSymbols() {
    auto mod = static_cast<HMODULE>(module_);
    open_ = reinterpret_cast<OpenFn>(mustResolve(mod, "PassThruOpen"));
    close_ = reinterpret_cast<CloseFn>(mustResolve(mod, "PassThruClose"));
    connect_ = reinterpret_cast<ConnectFn>(mustResolve(mod, "PassThruConnect"));
    disconnect_ = reinterpret_cast<DisconnectFn>(mustResolve(mod, "PassThruDisconnect"));
    readMsgs_ = reinterpret_cast<ReadMsgsFn>(mustResolve(mod, "PassThruReadMsgs"));
    writeMsgs_ = reinterpret_cast<WriteMsgsFn>(mustResolve(mod, "PassThruWriteMsgs"));
    startPeriodic_ = reinterpret_cast<StartPeriodicMsgFn>(mustResolve(mod, "PassThruStartPeriodicMsg"));
    stopPeriodic_ = reinterpret_cast<StopPeriodicMsgFn>(mustResolve(mod, "PassThruStopPeriodicMsg"));
    startFilter_ = reinterpret_cast<StartMsgFilterFn>(mustResolve(mod, "PassThruStartMsgFilter"));
    stopFilter_ = reinterpret_cast<StopMsgFilterFn>(mustResolve(mod, "PassThruStopMsgFilter"));
    ioctl_ = reinterpret_cast<IoctlFn>(mustResolve(mod, "PassThruIoctl"));
}

long J2534DynamicApi::Open(void* name, unsigned long* deviceId) { std::scoped_lock lk(mtx_); return open_(name, deviceId); }
long J2534DynamicApi::Close(unsigned long deviceId) { std::scoped_lock lk(mtx_); return close_(deviceId); }
long J2534DynamicApi::Connect(unsigned long d, unsigned long p, unsigned long f, unsigned long b, unsigned long* c) { std::scoped_lock lk(mtx_); return connect_(d,p,f,b,c); }
long J2534DynamicApi::Disconnect(unsigned long c) { std::scoped_lock lk(mtx_); return disconnect_(c); }

long J2534DynamicApi::ReadMsgs(unsigned long channelId, J2534Msg* msgs, unsigned long* numMsgs, unsigned long timeoutMs) {
    std::scoped_lock lk(mtx_);
    NativeMsg n{};
    auto ret = readMsgs_(channelId, &n, numMsgs, timeoutMs);
    if (ret == 0 && msgs && numMsgs && *numMsgs > 0) msgs[0] = fromNative(n);
    return ret;
}

long J2534DynamicApi::WriteMsgs(unsigned long channelId, const J2534Msg* msgs, unsigned long* numMsgs, unsigned long timeoutMs) {
    std::scoped_lock lk(mtx_);
    NativeMsg n{};
    if (msgs && numMsgs && *numMsgs > 0) n = toNative(msgs[0]);
    return writeMsgs_(channelId, &n, numMsgs, timeoutMs);
}

long J2534DynamicApi::StartPeriodicMsg(unsigned long channelId, const J2534Msg* msg, unsigned long* msgId, unsigned long timeIntervalMs) {
    std::scoped_lock lk(mtx_);
    NativeMsg n{};
    if (msg) n = toNative(*msg);
    return startPeriodic_(channelId, &n, msgId, timeIntervalMs);
}

long J2534DynamicApi::StopPeriodicMsg(unsigned long channelId, unsigned long msgId) { std::scoped_lock lk(mtx_); return stopPeriodic_(channelId, msgId); }

long J2534DynamicApi::StartMsgFilter(unsigned long channelId, unsigned long filterType, const J2534Msg* mask, const J2534Msg* pattern, const J2534Msg* flowControl, unsigned long* filterId) {
    std::scoped_lock lk(mtx_);
    NativeMsg m{}, p{}, f{};
    if (mask) m = toNative(*mask);
    if (pattern) p = toNative(*pattern);
    if (flowControl) f = toNative(*flowControl);
    return startFilter_(channelId, filterType, &m, &p, &f, filterId);
}

long J2534DynamicApi::StopMsgFilter(unsigned long channelId, unsigned long filterId) { std::scoped_lock lk(mtx_); return stopFilter_(channelId, filterId); }
long J2534DynamicApi::Ioctl(unsigned long channelId, unsigned long ioctlId, void* input, void* output) { std::scoped_lock lk(mtx_); return ioctl_(channelId, ioctlId, input, output); }

}  // namespace core::j2534

#endif
