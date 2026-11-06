# Architecture

## Status

Initial architecture proposal for review. This document establishes the major software boundaries and a deliberately small first implementation. It does not claim full SCPI or IEEE 488.2 compliance; the supported standards subset must be researched and documented separately.

## Design goals

- Provide a reusable SCPI command engine for Arduino-framework ESP32-family microcontrollers.
- Keep the generic SCPI engine independent of any particular instrument or its hardware.
- Allow an instrument module to register its own command families and handlers.
- Keep transport, command processing, instrument behavior, and hardware drivers separate.
- Avoid unnecessary dynamic allocation during normal command processing.
- Make the parser and core testable on a desktop host without an ESP32.
- Begin with a small, useful vertical slice before adding complex instrument functions.

## Proposed software layers

### 1. Transport

A transport moves incoming and outgoing bytes. Initial candidates include USB/serial, UART, and Wi-Fi TCP; the first implementation's transport is still to be selected.

The transport must not interpret SCPI command meaning. The design must explicitly settle message framing and termination, including how a transport presents complete SCPI program messages to the engine. TCP in particular must not assume that one network read equals one complete message.

### 2. SCPI engine

The engine is the instrument-independent public façade. It coordinates:

- Input message handling and parsing.
- Command-path lookup and dispatch.
- Parameter parsing and validation support.
- Response formatting and output.
- Common commands, error handling, and status behavior within the documented supported subset.

The engine must not contain GPIO operations, LED behavior, oscilloscope logic, or other instrument-specific functionality.

### 3. Parser and command registry

The parser recognizes SCPI message structure and produces a representation that the dispatcher can use. The registry represents command paths and associates them with handlers and metadata.

The implementation should support the selected SCPI subset, including the applicable rules for long and short keyword forms, queries, parameters, and message units. Exact rules and limits are to be established in the standards/compliance work before claiming conformance.

The implementation strategy (for example, bounded token views, tables, or another representation) remains open. Prefer bounded resource use and avoid unnecessary heap allocation in the normal command path.

### 4. Instrument module

An instrument module owns instrument-specific command families, state, parameter constraints, and handler behavior. It registers its commands with the generic engine through a defined interface.

The **HEARTBEAT** is better treated as a reusable application-support service than as part of the generic SCPI engine. An instrument application may include this service and register its HEARTBEAT command family with the engine. This allows the same heartbeat behavior to be used across different instruments without making it a dependency of the generic parser or dispatcher.

### 5. Hardware configuration and driver boundary

Hardware-specific details belong below the instrument module, not in the generic SCPI engine.

For HEARTBEAT, board configuration must provide at least:

- The GPIO pin connected to the onboard or selected status LED.
- The LED's electrical polarity (active-high or active-low).
- Any board-specific initialization needed to use that pin.

The GPIO selection should be a board/application configuration choice. Whether changing the pin at runtime through SCPI is useful or safe is an open question; it is not required for the first implementation.

### 6. Main-loop integration and nonblocking heartbeat service

The application main loop calls the heartbeat service frequently. The service checks elapsed time, updates the LED output when a transition is due, and returns promptly.

The heartbeat implementation must not use a blocking delay to generate its blink pattern. It must allow the main loop to continue processing SCPI input and other instrument work.

Use rollover-safe elapsed-time calculations. The achievable blink timing depends on how frequently the main loop calls the service; this limitation should be documented. The heartbeat indicates that the loop is being serviced, not that every subsystem or peripheral is healthy.

## HEARTBEAT behavior proposal

The reusable HEARTBEAT service should be usable by different instrument applications. Its first implementation should support:

- Enable/disable of blinking.
- A configurable blink frequency, expressed in hertz.
- A configurable duty cycle, expressed as a percentage from 0 through 100.
- A configurable LED GPIO pin and active polarity supplied by board/application configuration.
- Deterministic handling of invalid settings, with errors reported through the engine's error mechanism.

The exact allowed frequency range, numeric precision, behavior at duty-cycle endpoints, and startup defaults remain to be decided. A disabled heartbeat should leave the LED in a documented state. A 0% duty cycle means continuously off during blinking; 100% means continuously on.

Illustrative command family (proposed names, not yet finalized):

```text
HEARTbeat:STATe ON
HEARTbeat:FREQuency 2
HEARTbeat:DCYCle 25
HEARTbeat:STATe?
HEARTbeat:FREQuency?
HEARTbeat:DCYCle?
```

The pin and polarity are initially configuration properties rather than required SCPI commands. The command spelling, short forms, units, and behavior must be checked against the chosen SCPI rules and project naming conventions.

## Host-side testing

The core parser, registry, dispatcher, and response/error behavior should build and run on a desktop host without Arduino or ESP32 headers.

Use a mock transport and a simple test instrument. Tests should cover successful command dispatch, queries and responses, invalid parameters, unknown commands, malformed messages, and relevant boundary conditions.

HEARTBEAT logic should be designed so its timing/state behavior can be tested with a controllable clock or equivalent test seam, without requiring physical GPIO hardware. A small hardware integration test can then verify the configured LED pin and polarity on a selected ESP32 board.

## First implementation milestone

Build a minimal vertical slice:

1. Compile the generic engine on a desktop host.
2. Feed it a command through a mock transport.
3. Parse and dispatch a HEARTBEAT command registered by a small example application module.
4. Verify parameter validation and query responses.
5. Verify the nonblocking heartbeat state/timing logic with host tests.
6. Integrate the GPIO adapter and main-loop service on a selected ESP32 board.

The first milestone is successful when the host tests pass and the hardware example visibly blinks the configured LED while continuing to process incoming commands.

## Decisions still open

- Applicable SCPI and IEEE 488.2 editions and the exact supported subset.
- First transport and message-framing contract.
- ESP32 chip/board and Arduino framework/toolchain versions.
- Build system and host-test framework.
- Command registration and handler API design.
- Parser limits, message buffering, and memory strategy.
- HEARTBEAT startup defaults, valid frequency range, and numeric precision.
- Whether GPIO selection is compile-time/board configuration only or also runtime configurable.
- Scope of common commands, status registers, and error-queue behavior.

These decisions should be resolved deliberately; the initial design should not imply unsupported standards compliance or premature hardware portability.
