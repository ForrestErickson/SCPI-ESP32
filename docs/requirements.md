# SCPI-ESP32 — Project Requirements and Design Brief

**Status:** Initial requirements summary; design decisions remain open  
**Repository:** [ForrestErickson/SCPI-ESP32](https://github.com/ForrestErickson/SCPI-ESP32)

## 1. Purpose

Develop a reusable, general-purpose SCPI (Standard Commands for Programmable
Instruments) command engine in C++ for microcontrollers using the Arduino
framework on ESP32-family devices.

The engine is intended to provide the common SCPI command-processing layer
that can be reused by different instrument applications. It must not be
designed around one particular instrument.

## 2. Fundamental architectural requirement

**The generic SCPI engine must be independent of the instrument it controls.**

The project should separate at least these responsibilities:

- **Generic SCPI core:** command input parsing, command-tree recognition and
  dispatch, common commands, response handling, error reporting, and status
  behavior.
- **Transport layer:** delivery of command text to the engine and responses
  back to the controlling computer or client.
- **Instrument/application modules:** instrument-specific commands and their
  behavior.
- **Hardware drivers:** access to ADCs, DACs, timers, signal-generation
  hardware, and other board-specific peripherals.

An instrument module should be able to register its commands and handlers
with the generic engine. Replacing an oscilloscope/function-generator
application with a frequency counter, for example, should not require
rewriting the generic parser.

## 3. Initial demonstration instrument

The first demonstration application is planned to combine:

- A multichannel oscilloscope using ADC acquisition.
- A dual-channel function generator using two DAC outputs.

This combined instrument is a test application for exercising the engine. It
is **not** to be built into, or assumed by, the generic SCPI core.

Potential command areas include acquisition, channels, timebase, triggering,
measurements, signal sources, waveform transfer, system functions, and
status. Exact command names, parameters, and behavior remain to be designed
and verified.

## 4. Platform and hardware constraints

- Target the Arduino framework on ESP32-family microcontrollers.
- No exact ESP32 board or hardware configuration has been selected yet.
- ESP32 variants differ: some have native DAC peripherals and others do not.
  ADC performance and capabilities also vary.
- Keep hardware-dependent functions behind replaceable drivers or interfaces.
- Do not assume every ESP32 board can support the same oscilloscope or
  waveform-generation performance.

## 5. Language, resource use, and portability

- Implement the engine in C++.
- C++17 is a candidate, subject to compatibility with the selected Arduino
  ESP32 toolchain.
- Avoid unnecessary dynamic allocation in normal command-processing paths.
- Keep the parser/core as independent of ESP32 hardware as practical.
- Provide host-side tests that can run on a desktop without a physical board.

These are design goals to validate, not yet immutable implementation choices.

## 6. Communications transports

The transport interface should be separate from SCPI parsing and command
execution.

Possible transports include:
- USB serial / serial.
- UART.
- Wi-Fi TCP.

The initial transport set and exact interface are open design decisions.
Adding a transport should not require changes to the SCPI parser.

## 7. SCPI behavior and standards discipline

The engine should implement a useful, documented subset of SCPI and relevant
IEEE 488.2 common-command and status/error behavior.

Requirements:
- Identify which requirements come from SCPI and which come from IEEE 488.2.
- Define the supported command and syntax subset explicitly.
- Document unsupported features and implementation deviations.
- Develop a compliance/verification checklist.
- Test behavior rather than claiming compliance based only on the presence
  of familiar command names.

**No claim of full SCPI or IEEE 488.2 compliance is made at this stage.**
The precise required command set and applicable standards editions must be
researched and agreed before compliance claims are made.

## 8. Candidate generic-core responsibilities

The following are candidates for the generic core and need detailed design:

- SCPI message and program-message parsing.
- Command tree and short/long command keyword handling.
- Query versus command dispatch.
- Parameter parsing and response formatting.
- Common commands (for example, identification, reset, clear, operation
  complete, and wait behavior, as applicable to the selected requirements).
- Error queue and standardized error reporting.
- Status registers, event handling, and service-request-related behavior,
  where supported by the selected scope.
- Defined handling of malformed or unsupported commands.

Exact semantics must be checked against the applicable specifications.

## 9. Candidate instrument-module responsibilities

Instrument modules should define and implement their own command families,
parameters, validation, and hardware behavior. For the first demonstration,
possible families include:

- Acquisition and waveform capture.
- Input channels and channel configuration.
- Timebase and trigger configuration.
- Measurements.
- Output sources and waveform generation.
- Instrument-specific system configuration.

These are illustrative areas, not a finalized command specification.

## 10. Quality and testing requirements

- Unit-test parsing and command dispatch on a desktop host.
- Test valid commands, queries, abbreviated keywords, parameters, and
  malformed input.
- Test error-queue and status behavior that the project elects to support.
- Use a dummy/mock instrument to demonstrate command registration and
  dispatch without physical hardware.
- Add ESP32 integration tests when a target board is selected.
- Keep tests and documentation aligned with the implemented feature set.

## 11. Proposed development sequence

1. Review and refine this requirements brief.
2. Research the applicable SCPI and IEEE 488.2 requirements and define the
   initial supported subset.
3. Write an architecture specification and define core/transport/instrument
   interfaces.
4. Establish the initial source, documentation, example, and test layout.
5. Implement a minimal generic engine and dummy instrument.
6. Add host-side unit tests and continuous integration as practical.
7. Select target hardware and implement hardware-independent driver
   interfaces plus the first ESP32 integration.
8. Implement the oscilloscope/function-generator demonstration as an
   instrument module.
9. Expand and verify the supported SCPI feature set incrementally.

## 12. Open decisions

The following are intentionally unresolved:

- Exact ESP32 chip and development board.
- Exact Arduino ESP32 core/toolchain version and C++ language level.
- Applicable SCPI/IEEE 488.2 specification editions and initial compliance
  target.
- Required versus optional command set and status-system scope.
- Transport(s) for the first working release.
- Parser limits, maximum message/response sizes, and memory strategy.
- Whether command registration uses static tables, explicit registration
  calls, or another approach.
- Detailed instrument-module interface and hardware-driver abstractions.
- Oscilloscope acquisition and waveform-output performance requirements.

## 13. Guiding principles

1. Keep the SCPI engine generic and reusable.
2. Keep instrument behavior out of the generic core.
3. Keep transports and hardware drivers replaceable.
4. Prefer testable, resource-conscious implementation choices.
5. Make requirements and limitations explicit.
6. Verify standards behavior before making compliance claims.

---

This document records the current project understanding. It is a starting
point for review and refinement, not a final detailed design specification.
