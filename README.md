# J2534 Pass-Thru Tool (Full Starter Codebase)

This repository now contains a **working, testable C++ baseline** for a professional J2534 diagnostic stack, with no hardware dependency by default (mock API included).

## Implemented components

- J2534 abstraction interface (`IJ2534Api`) with all required PassThru API method signatures.
- Thread-safe mock implementation (`J2534MockApi`) that simulates UDS ECU behavior.
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
- NRC decoder for common negative response codes.
- Windows-ready dynamic loader implementation (`J2534DynamicApi`) behind compile guards.
- Executable demo app with end-to-end flow.
- Unit-style test executable added and integrated with `ctest`.

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/j2534_tool
ctest --test-dir build --output-on-failure
```

## Current architecture direction

- `src/core/j2534`: PassThru abstraction, mock, dynamic-loader adapter, error mapping.
- `src/core/diag`: UDS service layer + NRC decoding.
- `src/core/common`: shared result/error model.
- `src/tests`: deterministic tests for core diagnostic behavior.

## Next engineering tasks

1. Add ISO-TP segmentation/reassembly (FF/CF/FC, STmin, BS) as a dedicated transport layer.
2. Add async worker threads (Tx/Rx/diag/logger) and timeout/retry policies (P2/P2*/S3).
3. Add GUI (Qt Widgets) and persistent trace logging with timing metrics.
4. Add script-based automation and batch test runner.
5. Replace mock with real vendor DLL on Windows and validate with physical hardware.
