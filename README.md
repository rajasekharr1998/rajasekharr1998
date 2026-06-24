# J2534 Pass-Thru Tool (Diagnostic Stack Baseline)

This repository contains a **working, testable C++ baseline** for a professional J2534 diagnostic stack, with no hardware dependency by default through a deterministic mock Pass-Thru API.

## Implemented components

- J2534 abstraction interface (`IJ2534Api`) with all required PassThru API method signatures.
- Thread-safe mock implementation (`J2534MockApi`) that simulates UDS ECU behavior.
- Windows-ready dynamic loader implementation (`J2534DynamicApi`) behind compile guards.
- J2534 error mapper (`mapJ2534Error`) to internal diagnostic errors.
- UDS client with support for:
  - `0x10` Session Control
  - `0x11` ECU Reset
  - `0x27` Security Access
  - `0x22` ReadDataByIdentifier
  - `0x2E` WriteDataByIdentifier
  - `0x3E` Tester Present
  - `0x34` Request Download
  - `0x36` Transfer Data
  - `0x37` Transfer Exit
- Async UDS worker model with queued diagnostic jobs, retry handling, P2/P2* timing policy, and S3 tester-present keepalive.
- ISO-TP segmentation/reassembly helper for single-frame and multi-frame payloads, including flow-control generation and sequence validation.
- NRC decoder for common negative response codes.
- Executable demo app with end-to-end flow.
- Unit-style test executables integrated with `ctest`.

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/j2534_tool
ctest --test-dir build --output-on-failure
```

## Current architecture direction

- `src/core/j2534`: PassThru abstraction, mock, dynamic-loader adapter, error mapping.
- `src/core/diag`: UDS service layer, async worker, timeout/retry policy, S3 keepalive, NRC decoding.
- `src/core/transport`: ISO-TP segmentation/reassembly and flow-control helpers.
- `src/core/common`: shared result/error model.
- `src/tests`: deterministic tests for diagnostic, transport, and async behavior.

## Next engineering tasks

1. Connect `IsoTpEngine` into `UdsClient` as the default transport for real CAN/J2534 payloads.
2. Add timestamped logging with Tx/Rx direction, CAN ID, DLC, raw hex, and response-time calculation.
3. Add GUI (Qt Widgets) for device selection, connect/disconnect, raw console, UDS panels, and log export.
4. Add script-based automation and batch test runner.
5. Replace mock with real vendor DLL on Windows and validate with physical hardware.
