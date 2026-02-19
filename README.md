# J2534 Pass-Thru Diagnostic Tool (Windows, C++)

This document provides a practical, production-oriented blueprint to design, implement, and test a **J2534 Pass-Thru diagnostic application** on **Windows 11**, with a mockable setup for development without physical hardware.

---

## 1) System Architecture (Layered)

```text
+------------------------------------------------------------+
|                        UI Layer                            |
|  Device Select | Channel Config | Raw Console | UDS Panel  |
+-------------------------|----------------------------------+
                          v
+------------------------------------------------------------+
|                Application / Use-Case Layer                |
|  ConnectUseCase, UdsUseCase, FlashUseCase, ScriptRunner    |
+-------------------------|----------------------------------+
                          v
+------------------------------------------------------------+
|                 Diagnostic Service Layer (UDS)             |
| Session, Security, DTC, Read/Write DID, Flashing sequence  |
+-------------------------|----------------------------------+
                          v
+------------------------------------------------------------+
|                Transport/Protocol Handling Layer           |
|  ISO-TP, CAN framing, addressing, flow control, timers     |
+-------------------------|----------------------------------+
                          v
+------------------------------------------------------------+
|                 J2534 Adapter Abstraction Layer            |
|  IJ2534Api + Dynamic DLL Loader + channel operations       |
+-------------------------|----------------------------------+
                          v
+------------------------------------------------------------+
|                  Vendor J2534 DLL / Mock DLL               |
+------------------------------------------------------------+
```

### Why this matters
- Isolates vendor quirks inside one adapter layer.
- Allows simulation via mock DLL (no hardware).
- Keeps UDS logic independent of GUI and transport details.
- Makes automated testing and scaling to multi-channel easier.

---

## 2) Recommended Folder Structure

```text
j2534-tool/
  CMakeLists.txt
  docs/
    architecture.md
    class-diagram.md
    thread-model.md
    error-codes.md
  src/
    app/
      main.cpp
      AppController.cpp
    ui/
      MainWindow.cpp
      widgets/
    core/
      common/
        Result.h
        Error.h
        Time.h
        Logger.h
      j2534/
        IJ2534Api.h
        J2534DynamicApi.cpp
        J2534Types.h
        J2534Error.cpp
      transport/
        CanFrame.h
        IsoTpEngine.cpp
      diag/
        UdsClient.cpp
        UdsServices.cpp
        NrcDecoder.cpp
      automation/
        ScriptRunner.cpp
      flash/
        FlashManager.cpp
      monitor/
        BusLoadMonitor.cpp
  tests/
    unit/
      test_j2534_api.cpp
      test_isotp.cpp
      test_uds.cpp
    integration/
      test_mock_ecu_session.cpp
      test_stress_timeout.cpp
  tools/
    mock-j2534-dll/
    ecu-simulator/
```

---

## 3) Core J2534 Abstraction

## 3.1 Type-safe wrapper and DLL loading

```cpp
// core/j2534/IJ2534Api.h
#pragma once
#include <cstdint>
#include <vector>
#include <string>

struct J2534Msg {
    uint32_t ProtocolID{};
    uint32_t RxStatus{};
    uint32_t TxFlags{};
    uint32_t Timestamp{};
    uint32_t DataSize{};
    uint32_t ExtraDataIndex{};
    std::vector<uint8_t> Data;
};

class IJ2534Api {
public:
    virtual ~IJ2534Api() = default;
    virtual long Open(void* name, unsigned long* deviceId) = 0;
    virtual long Close(unsigned long deviceId) = 0;
    virtual long Connect(unsigned long deviceId, unsigned long protocolId,
                         unsigned long flags, unsigned long baud,
                         unsigned long* channelId) = 0;
    virtual long Disconnect(unsigned long channelId) = 0;
    virtual long ReadMsgs(unsigned long channelId, J2534Msg* msgs,
                          unsigned long* numMsgs, unsigned long timeoutMs) = 0;
    virtual long WriteMsgs(unsigned long channelId, const J2534Msg* msgs,
                           unsigned long* numMsgs, unsigned long timeoutMs) = 0;
    virtual long StartPeriodicMsg(unsigned long channelId, const J2534Msg* msg,
                                  unsigned long* msgId, unsigned long timeIntervalMs) = 0;
    virtual long StopPeriodicMsg(unsigned long channelId, unsigned long msgId) = 0;
    virtual long StartMsgFilter(unsigned long channelId, unsigned long filterType,
                                const J2534Msg* mask, const J2534Msg* pattern,
                                const J2534Msg* flowControl, unsigned long* filterId) = 0;
    virtual long StopMsgFilter(unsigned long channelId, unsigned long filterId) = 0;
    virtual long Ioctl(unsigned long channelId, unsigned long ioctlId,
                       void* input, void* output) = 0;
};
```

```cpp
// core/j2534/J2534DynamicApi.cpp (snippet)
#include <windows.h>
#include <mutex>
#include <stdexcept>

class J2534DynamicApi final : public IJ2534Api {
public:
    explicit J2534DynamicApi(const std::wstring& dllPath) {
        module_ = ::LoadLibraryW(dllPath.c_str());
        if (!module_) throw std::runtime_error("LoadLibraryW failed");
        loadSymbols();
    }

    ~J2534DynamicApi() override {
        if (module_) ::FreeLibrary(module_);
    }

    long Open(void* name, unsigned long* deviceId) override {
        std::scoped_lock lk(apiMtx_);
        return PassThruOpen_(name, deviceId);
    }

    // ... implement remaining methods with same lock strategy

private:
    void loadSymbols() {
        PassThruOpen_ = reinterpret_cast<PassThruOpenFn>(::GetProcAddress(module_, "PassThruOpen"));
        // ... resolve all required symbols
        if (!PassThruOpen_) throw std::runtime_error("GetProcAddress PassThruOpen failed");
    }

    using PassThruOpenFn = long(__stdcall*)(void*, unsigned long*);
    HMODULE module_{};
    std::mutex apiMtx_;
    PassThruOpenFn PassThruOpen_{};
};
```

### Key implementation notes
- Resolve and validate **all required exports** at startup.
- Convert raw return codes into strong internal errors.
- Protect driver calls with mutex unless vendor documentation guarantees reentrancy.

---

## 4) Error Handling Framework

Use a central `Result<T>` model:

```cpp
enum class DiagErr {
    Ok,
    Timeout,
    Busy,
    InvalidChannel,
    DeviceNotOpen,
    Unsupported,
    BufferOverflow,
    ProtocolViolation,
    NegativeResponse,
    SecurityDenied,
    DriverError,
};

template<typename T>
struct Result {
    T value{};
    DiagErr err{DiagErr::Ok};
    std::string message;
    bool ok() const { return err == DiagErr::Ok; }
};
```

Map J2534 codes (`ERR_TIMEOUT`, `ERR_INVALID_CHANNEL_ID`, etc.) to `DiagErr` once in one place.

---

## 5) Thread Model (Production-safe)

- **UI thread**: rendering + command dispatch.
- **Tx worker**: outbound queue with retry and timeout policy.
- **Rx worker**: polling `PassThruReadMsgs`, pushes to decode queue.
- **Diag worker**: request/response state machine (UDS services).
- **Logger worker**: async file logging to avoid jitter.

Use lock-free or bounded queues between threads. Apply backpressure when Rx saturates.

---

## 6) Timeout + Retry Strategy

- P2 timeout: service response timeout.
- P2* timeout: extended server processing.
- S3 timer: tester present keepalive.
- Retry policy:
  - transport timeout: retry N times (e.g., 2)
  - NRC 0x78 (response pending): continue waiting until P2* expires
  - security failures: no blind retry

---

## 7) ISO-TP Layer Essentials

### Required support
- Single Frame (SF)
- First Frame (FF)
- Consecutive Frame (CF)
- Flow Control (FC CTS/WAIT/OVFLW)
- STmin and Block Size handling
- 11-bit and 29-bit CAN IDs
- Physical (single ECU) and functional (broadcast) addressing

### Send flow (FF/CF)
1. Send FF with total payload length.
2. Wait FC from ECU.
3. Respect block size + STmin.
4. Send CF sequence frames.

### Receive flow
1. On FF, allocate reassembly buffer.
2. Send FC CTS.
3. Assemble CF frames with sequence validation.
4. Abort on sequence mismatch or timeout.

---

## 8) UDS Diagnostic Service Layer

Implement strongly typed operations:

```cpp
class UdsClient {
public:
    Result<std::vector<uint8_t>> sessionControl(uint8_t sessionType); // 0x10
    Result<std::vector<uint8_t>> ecuReset(uint8_t resetType);         // 0x11
    Result<std::vector<uint8_t>> securityAccess(uint8_t subFn,
                                                const std::vector<uint8_t>& keyOrSeed); //0x27
    Result<std::vector<uint8_t>> readDID(uint16_t did);               // 0x22
    Result<void> writeDID(uint16_t did, const std::vector<uint8_t>& value); // 0x2E
    Result<void> testerPresent();                                     // 0x3E
    Result<void> requestDownload(...);                                // 0x34
    Result<void> transferData(uint8_t blockCounter, const std::vector<uint8_t>& chunk); //0x36
    Result<void> transferExit();                                      // 0x37
};
```

### Negative response decode
- Detect `0x7F <SID> <NRC>`.
- Decode common NRC:
  - `0x10` GeneralReject
  - `0x11` ServiceNotSupported
  - `0x13` IncorrectMessageLengthOrInvalidFormat
  - `0x22` ConditionsNotCorrect
  - `0x31` RequestOutOfRange
  - `0x33` SecurityAccessDenied
  - `0x35` InvalidKey
  - `0x36` ExceedNumberOfAttempts
  - `0x37` RequiredTimeDelayNotExpired
  - `0x78` ResponsePending

### Security access hook
Keep algorithm external:

```cpp
class ISecurityAlgorithm {
public:
    virtual ~ISecurityAlgorithm() = default;
    virtual std::vector<uint8_t> computeKey(uint8_t level,
                                            const std::vector<uint8_t>& seed) = 0;
};
```

---

## 9) GUI Requirements Implementation

Recommended stack: **Qt Widgets** (fast for desktop tools).

Main panels:
- Device dropdown: enumerate installed J2534 DLLs.
- Connect/Disconnect buttons.
- Channel config (protocol, baud, IDs, addressing mode).
- Raw console with Tx/Rx filters.
- UDS panel with one-click service forms.
- Log view with millisecond timestamps + export button.
- Hex + ASCII dual display.

---

## 10) Logging/Trace Standard

Each frame log line should include:
- Timestamp (ms precision)
- Direction (`Tx`/`Rx`)
- Channel
- Protocol
- CAN ID
- DLC
- Raw bytes
- Parsed UDS summary
- Response time (if paired)

Example:

```text
2025-02-14 10:15:03.245 | Rx | CH1 | CAN | ID:7E8 | DLC:8 | 03 7F 22 31 00 00 00 00 | NRC:RequestOutOfRange | RTT:12ms
```

---

## 11) Advanced Features Blueprint

- **Script automation**: JSON/YAML test scripts (connect, send UDS, assert responses).
- **Batch diagnostics**: run scripts over ECU list/config matrix.
- **DTC support**: UDS 0x19, clear 0x14.
- **Flashing framework**: pluggable file parsers (S19/HEX/BIN), chunk manager, checksum verify, rollback rules.
- **Performance**: message throughput, latency histogram, jitter.
- **Bus load monitor**: rolling percentage based on frame timing and bitrate.
- **Multi-channel**: one `ChannelContext` per active channel with independent workers.

---

## 12) Testing Strategy (No Hardware)

## 12.1 Unit tests
- J2534 API wrapper method mapping and error mapping.
- ISO-TP frame segmentation/reassembly.
- UDS positive and negative response handling.

## 12.2 Mock J2534 DLL
Create a fake DLL exporting all PassThru functions and deterministic behaviors:
- normal response sequence
- delayed response/timeouts
- invalid frames
- busy/full-buffer simulation

## 12.3 ECU simulator
A socket/CAN-like simulator that emulates UDS states:
- session transitions
- security seed/key checks
- flashing states and block counters

## 12.4 Stress and fault tests
- 24h message soak.
- invalid frame injection.
- channel disconnect mid-transfer.
- timeout boundary tests.
- error-code conformance checks.

---

## 13) Build & Deployment Checklist

- Prefer CMake presets for x86 and x64.
- Build **Win32 (x86)** explicitly for legacy J2534 vendors.
- Runtime:
  - dynamic runtime eases patching.
  - static runtime reduces dependency drift.
- Validate dependencies:
  - `dumpbin /dependents your_app.exe`
  - `dumpbin /exports vendor.dll`
- Include Microsoft VC++ redistributable policy.
- Driver compatibility matrix per vendor version.

---

## 14) Common Failure Cases & Fixes

- **ERR_INVALID_CHANNEL_ID**: stale channel after reconnect; recreate channel context.
- **ERR_TIMEOUT** on multi-frame: missing FC or wrong STmin; verify ISO-TP timing.
- **No response to functional request**: ECU may suppress response in functional mode.
- **Frequent NRC 0x33/0x35**: wrong security level or seed/key algorithm mismatch.
- **Flash abort on 0x73** (wrong block sequence): increment block counter correctly with wrap.
- **UI freezes**: blocking calls on UI thread; move all transport to workers.

---

## 15) Minimal Working Example Flow

1. Load selected J2534 DLL.
2. `PassThruOpen` -> `PassThruConnect` (CAN/ISO15765).
3. Configure filters and CAN ID pair.
4. Send UDS `0x10 0x03` (extended session).
5. Maintain Tester Present every S3 interval.
6. Read DID (`0x22 F1 90`) and parse VIN.
7. Disconnect and close.

---

## 16) Production-Grade Scalability Recommendations

- Plugin architecture for protocol/ECU profiles.
- Central telemetry stream (JSON logs + metrics).
- Config profiles per OEM and ECU family.
- Signed scripting packages for secure automation.
- CI pipeline with mock DLL + simulator regression suite.
- Add DoIP transport adapter (same UDS service API).

---

## 17) DoIP Integration Strategy

- Keep `IUdsTransport` interface generic.
- Implement `CanIsoTpTransport` and `DoipTransport` behind it.
- Reuse same UDS service layer unchanged.
- Add vehicle discovery + routing activation in DoIP adapter.

---

## 18) Suggested Next Implementation Steps

1. Implement `IJ2534Api` + dynamic loader + error mapper.
2. Build ISO-TP engine with deterministic unit tests.
3. Build UDS client + NRC decoder + security hook.
4. Add Qt GUI shell with raw console + log panel.
5. Add mock J2534 DLL and ECU simulator.
6. Add flashing sequence manager with resume support.
7. Finalize CI tests and packaging.

If you want, the next iteration can provide a ready-to-build **CMake project skeleton** with concrete source files for these interfaces.
