# SCPI-ESP32

A reusable SCPI (Standard Commands for Programmable Instruments) command
engine for microcontrollers using the Arduino framework on ESP32-family
devices.

The goal is to provide a general-purpose SCPI layer that instrument projects
can reuse, rather than embedding the parser and command handling separately
in every instrument application.

> **Project status:** Early development. The architecture, supported command
> set, hardware targets, and transport interfaces are not yet finalized.
> This project does not currently claim full IEEE 488.2 or SCPI compliance.

## Project goals

- Implement a reusable SCPI parser and command engine in C++.
- Keep the generic engine independent of any particular instrument.
- Allow an application to register instrument-specific commands and handlers.
- Separate command processing from communications transports and hardware drivers.
- Make as much of the engine as practical testable on a desktop host, without
  requiring an ESP32 board.
- Document supported behavior and test it against an explicit requirements
  and compliance checklist.

## Architectural direction

The project is intended to have three principal layers:

1. **SCPI core** — command parsing, command-tree lookup, common commands,
   response formatting, error handling, and status-system behavior.
2. **Transport adapters** — interfaces for receiving command text and sending
   responses. Serial and network transports are candidates; the initial set
   has not been selected.
3. **Instrument modules** — application-specific command registration and
   behavior, connected to the hardware drivers for the chosen instrument.

The core should not need to know whether it is controlling an oscilloscope,
function generator, frequency counter, or another instrument.

## Initial demonstration application

The first planned demonstration is a combined oscilloscope and dual-channel
function generator. It is intended to exercise a useful range of instrument
commands while remaining an example application rather than part of the
generic SCPI engine.

Possible command areas include acquisition, channels, timebase, triggering,
measurements, waveform output, and system/status functions. Exact command
names and behavior will be defined during design and checked against the
applicable SCPI requirements.

No specific ESP32 board or hardware configuration has yet been selected.
ESP32 variants differ in their ADC and DAC capabilities, so hardware-specific
drivers will remain separate from the SCPI core.

## Development approach

The intended sequence is:

1. Define the architecture and the initial supported SCPI feature set.
2. Establish a minimal project structure and public interfaces.
3. Implement a small working engine and a dummy instrument.
4. Add host-side unit tests for parsing, command dispatch, errors, and status.
5. Add ESP32 integration, transports, and the demonstration instrument.
6. Expand the documented requirements and verification tests incrementally.

## Repository layout

The following is a proposed direction, not yet a finalized directory structure:

- `src/core/` — generic SCPI engine and supporting classes.
- `src/transport/` — communication transport interfaces and adapters.
- `src/instruments/` — reusable instrument modules and examples.
- `examples/` — example applications.
- `test/` — host-side and embedded tests.
- `docs/` — architecture notes and SCPI feature/compliance checklist.

## Design principles

- **Instrument independence:** the core contains no instrument-specific
  behavior.
- **Modularity:** transports, instrument modules, and hardware drivers can be
  replaced independently.
- **Testability:** parser and command-engine behavior should be testable
  without physical hardware.
- **Explicit scope:** supported commands and deviations should be documented;
  standards compliance should never be assumed merely because a command is
  implemented.
- **Resource awareness:** implementation choices should account for the
  memory and processing constraints of ESP32 microcontrollers.

## Contributions and current work

The immediate work is to agree on the architecture and requirements before
building out a large parser. Issues and design decisions should be documented
as the project develops.
