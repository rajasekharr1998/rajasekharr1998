# J2534 Tool Starter (Concrete Next Step)

You asked for the **next practical step** after architecture planning.
This repository now contains a **buildable C++ starter** that executes a mocked J2534 + UDS flow so implementation can move from design to code.

## What is implemented now

- `IJ2534Api` interface (core PassThru API abstraction).
- `J2534MockApi` implementation for no-hardware development.
- Minimal `UdsClient` with:
  - `0x10` Session Control
  - `0x3E` Tester Present
  - `0x22` ReadDataByIdentifier
- Console app wiring: Open -> Connect -> send UDS -> Read -> Disconnect -> Close.
- CMake project that builds on this environment.

## Build

```bash
cmake -S . -B build
cmake --build build
./build/j2534_tool
```

Expected output (mock loopback):

```text
SessionControl response: 10 03
```

## Why this next step matters

- Converts static documentation into executable baseline.
- Gives a contract-first seam (`IJ2534Api`) so vendor DLL loading can be added without touching UDS code.
- Enables immediate unit testing and CI work with deterministic behavior.

## Immediate follow-up tasks

1. Add `J2534DynamicApi` (Windows-only `LoadLibraryW/GetProcAddress`) behind `IJ2534Api`.
2. Introduce error mapping (`J2534 error -> DiagErr`) in a single mapper.
3. Replace mock loopback with ECU simulator behavior (positive + NRC responses).
4. Add ISO-TP transport layer and move UDS onto transport interface.
5. Add unit tests for API wrapper + UDS request/response handling.
