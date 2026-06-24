# J2534 Pass-Thru Diagnostic Tool

This repository contains a **working, testable C++ baseline** for a professional J2534 diagnostic stack, with no hardware dependency by default through a deterministic mock Pass-Thru API.

## Implemented components

- J2534 abstraction interface (`IJ2534Api`) with all required PassThru API method signatures.
- Thread-safe mock implementation (`J2534MockApi`) that simulates UDS ECU behavior over ISO-TP frames.
- Windows-ready dynamic loader implementation (`J2534DynamicApi`) behind compile guards.
- Vendor DLL validation helper that checks required J2534 exports on Windows.
- J2534 error mapper (`mapJ2534Error`) to internal diagnostic errors.
- ISO-TP-backed UDS client with support for:
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
- Timestamped trace logging with Tx/Rx direction, CAN ID, DLC, hex data, and response-time messages.
- Script/batch automation runner for simple diagnostic command scripts.
- GUI-ready `DiagnosticController` that centralizes connect/disconnect, VIN read, and script execution.
- Optional Qt Widgets shell (`J2534_BUILD_QT_GUI=ON`) with connect/disconnect controls and a log view scaffold.
- Executable demo app with end-to-end flow.
- Unit-style test executables integrated with `ctest`.

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/j2534_tool
ctest --test-dir build --output-on-failure

# Optional Qt GUI build when Qt6 Widgets is installed:
cmake -S . -B build-qt -DJ2534_BUILD_QT_GUI=ON
cmake --build build-qt
```

## Current architecture direction

- `src/core/j2534`: PassThru abstraction, mock, dynamic-loader adapter, vendor validator, error mapping.
- `src/core/diag`: UDS service layer, async worker, timeout/retry policy, S3 keepalive, NRC decoding.
- `src/core/transport`: ISO-TP segmentation/reassembly and flow-control helpers.
- `src/core/logging`: timestamped diagnostic trace logging.
- `src/core/automation`: script/batch diagnostic execution.
- `src/core/ui`: GUI-ready controller facade.
- `src/ui`: optional Qt Widgets shell.
- `src/core/common`: shared result/error model.
- `src/tests`: deterministic tests for diagnostic, transport, async, and automation behavior.

## Script format

The automation runner accepts one command per line:

```text
session 03
read_did F190
tester_present
download
```

## Next engineering tasks

1. Bind the Qt Widgets shell directly to `DiagnosticController` actions and live trace streams.
2. Add persistent log-file selection and log export from the GUI.
3. Add richer automation commands for flashing blocks, assertions, delays, and DTC read/clear.
4. Validate with a real Windows J2534 DLL and hardware interface.
5. Add packaging presets for Win32/x64 release builds.
