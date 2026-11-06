# Software Interface Contract

**Status:** Initial implementation contract. The message-framing component, engine entry-point interface, framed-input adapter, error queue, response sink, and SCPI command registry are implemented; host test sources are provided, but have not yet been run in a working build environment.

This document defines the responsibilities and boundaries for the first host-testable SCPI-ESP32 vertical slice. It records the initial function-pointer handler mechanism while leaving parser representation, standards details, and some implementation limits open.

## 1. Overall flow

1. At the beginning of application setup, the application turns the status LED logically ON as a visible setup indicator.
2. Setup performs hardware and application initialization.
3. Immediately before setup finishes, the application turns the status LED logically OFF.
4. When the main loop begins running, the application enables and begins servicing HEARTBEAT.
5. The application main loop regularly services both the SCPI input path and the HEARTBEAT service.

The transport receives bytes and identifies complete SCPI program messages according to the framing rules below. `transport::FramedInput` forwards each complete message through the transport-independent `scpi::EngineInterface::processScpiMessage()` entry point. The engine parses the message, resolves the command, validates syntax and parameters, and invokes the registered handler. A handler may update application-service settings or return query data. The engine sends responses through an output/response interface.

The engine does not operate GPIO. The HEARTBEAT service does not parse SCPI. The GPIO adapter does not know SCPI command names.

## 2. Generic SCPI engine boundary

### Responsibilities

The generic engine should accept complete program messages, parse and dispatch commands according to the documented SCPI subset, resolve registered command paths, distinguish commands from queries, make parsed parameters available to handlers, provide a response-writing mechanism, and report syntax, parameter, and command errors through its error mechanism.

The engine should not include Arduino or ESP32 headers; know about GPIO pins, LEDs, or HEARTBEAT behavior; depend on a particular instrument application; or own the transport's byte-level framing.

### Conceptual interface

The initial C++ entry point is `scpi::EngineInterface::processScpiMessage(const std::string&)`. It remains `void`: the transport submits a complete, non-empty program message, and the engine reports SCPI-visible processing errors through an error queue rather than a per-message return value.

The initial support interfaces are:

- `scpi::ErrorQueue` stores `ScpiError` entries in FIFO order. Its default capacity is 16 entries.
- `scpi::ScpiError` keeps an integer error code separate from its explanatory message.
- `scpi::ResponseSink::writeResponse()` accepts a complete response string. The concrete sink owns transport-specific delivery and termination; the engine does not append LF or CRLF itself.

When the error queue is full, existing entries are preserved. Further errors are not stored individually; one `-350, "Queue overflow"` indication is made available after the stored entries have been read. This is the project's initial bounded-queue policy and will be checked against the selected SCPI specification before claiming conformance.

These interfaces are independent of Arduino, ESP32, and any particular transport. Host tests can use a small fake response sink to capture output without a serial port.

The initial `scpi::CommandRegistry` stores registered long-form command paths and associates each with a function-pointer handler and optional application context pointer. Uppercase letters in a registration define the minimum short form (for example, `MEASure:VOLTage` accepts `MEAS:VOLT`); matching is case-insensitive and each path component is resolved independently. `scpi::CommandRequest` carries the parsed command path, query flag, and parameter text. The handler signature is `void`: it receives the application context, request, `ErrorQueue`, and `ResponseSink`. It reports errors by adding entries to the queue and emits query output through the sink. The registry's `dispatch()` return value means only that a matching handler was found and invoked; it is not a success/failure result for command execution. 

The initial `scpi::Parser` separates one command header, an optional query marker, and the remaining parameter text. It trims surrounding whitespace and rejects malformed headers. It intentionally does not parse parameter grammar. The first slice accepts one command unit per message; it supports subsystem commands and the common command `*IDN?`, but not semicolon-separated units or the broader common-command set. Command-tree defaults and full SCPI syntax remain future work; do not treat this subset as a complete SCPI implementation. The function-pointer/context design avoids requiring `std::function` and permits one handler implementation to serve an application object without coupling the engine to that object's type.

## 3. Parser and command registry

The parser recognizes one command unit per message. It supports subsystem command headers and the initial common command `*IDN?`, separates the header from parameter text at the first whitespace, recognizes a trailing query marker, and rejects malformed headers. It does not yet interpret parameter values. The registry resolves subsystem keywords using the mandatory uppercase prefix and optional lowercase suffix shown in the registered long form. The `*IDN` common command is registered as an exact command name. Semicolon-separated command units, command-tree defaults, and detailed parameter grammar remain future work.

## 4. Transport boundary

A transport moves bytes and determines when a complete message is ready. The transport/application boundary supplies complete messages to the engine and carries engine responses back to the controller.

A single serial read or TCP read is not necessarily one complete SCPI message. TCP is a byte stream; message boundaries are established by `transport::MessageFramer` using these project rules:

- LF terminates a message.
- CRLF terminates a message and is treated as one terminator.
- CR alone does not terminate a message; a CR not followed by LF is retained as message data.
- An optional inactivity timeout can terminate a message when no terminator is sent; it is disabled by default.
- Empty messages are not dispatched. Oversized messages are reported and discarded through the next boundary.

The current default maximum message size is 256 bytes.

For the first host tests, a mock transport or direct message feeder is sufficient. The initial physical transport can be selected later.

## 5. HEARTBEAT service boundary

### Responsibilities

The HEARTBEAT service owns whether heartbeat operation is enabled, the configured period in seconds per complete cycle, the configured duty cycle as a percentage, and the timing state needed to determine whether the logical LED output should be on or off.

It does not own SCPI parsing or command registration, a GPIO number or electrical polarity, Arduino pin APIs, or blocking delays.

### Initial operating settings and startup behavior

The agreed initial HEARTBEAT settings are:

- Period: **2 seconds** per complete cycle.
- Duty cycle: **25%** of each cycle.

With these settings, the logical LED is on for 0.5 seconds and off for 1.5 seconds in each 2-second cycle, while heartbeat is enabled.

Startup behavior is also defined:

- At the beginning of `setup()`, the application commands the status LED logically ON. This provides a visible indication that setup has started and is executing.
- The LED remains logically ON during setup.
- Immediately before `setup()` returns, the application commands the status LED logically OFF.
- HEARTBEAT is not started during setup. It is enabled when the application begins executing `loop()`, and is then serviced regularly by the main loop.

The setup indication and HEARTBEAT both use the logical LED output interface. The board adapter remains responsible for translating logical ON/OFF into the correct GPIO voltage for the selected LED wiring.

When the duty cycle is changed while HEARTBEAT is running, the timing cycle restarts immediately using the new duty cycle and the currently active period. The LED is logically ON at the start of that new cycle (unless the new duty cycle is 0%). A period change, by contrast, changes the configured period but does not alter the active cycle; the new period becomes active the next time HEARTBEAT is enabled.

### Conceptual operations

The service needs equivalents of:

- Set/get enabled state.
- Set/get configured period and, as appropriate, active period.
- Set/get duty cycle.
- A service operation such as service(now), called frequently by the application loop, which updates the logical LED output only when needed and returns promptly.

The service should use an injected or supplied monotonic time source. This makes timing deterministic in host tests and avoids tying the service to a particular ESP32 clock API.

The service must not call delay() or otherwise wait for a blink transition. It calculates the desired output from elapsed time and returns. Elapsed-time arithmetic must be safe across timer rollover.

### Timing semantics

- Period is the duration of one complete on/off cycle.
- Duty cycle is the fraction of that period for which the LED is logically on.
- At 0% duty cycle, the logical output remains off.
- At 100% duty cycle, the logical output remains on while heartbeat is enabled.
- When heartbeat is disabled, the logical output is off immediately.
- Disabling HEARTBEAT interrupts the current cycle; it does not wait for the cycle to finish. This provides an immediate way to regain control even if a very long period has been configured.
- Enabling HEARTBEAT starts a fresh cycle using the currently configured period and duty cycle. This is when a newly configured period takes effect.
- Changing the period while HEARTBEAT is enabled does not change the period of the cycle already in progress or subsequent cycles in that uninterrupted run; it is applied when HEARTBEAT is next enabled.
- For intermediate duty cycles, the on interval begins at the start of each cycle.

The initial implementation represents periods as unsigned milliseconds, with a 1 ms resolution and a permitted range of 1 ms through 24 hours (86,400,000 ms). The configured period is reported by `periodMilliseconds()`; the active period is held internally for the current uninterrupted run. Invalid values are rejected without changing the previous configuration.

## 6. Logical LED output and board adapter

The HEARTBEAT service should request a logical state, such as setLedOn(true) or setLedOn(false). It should not write a voltage level.

A board-specific adapter translates that logical state into GPIO operations. It knows the selected GPIO pin, knows whether the LED is active-high or active-low, performs required pin initialization, and maps logical ON/OFF to the correct electrical output.

For an active-high LED, logical ON generally maps to a high output. For an active-low LED, logical ON generally maps to a low output. The adapter owns this translation.

The pin and polarity are board/application configuration. They are not generic SCPI engine settings, and they are not required HEARTBEAT SCPI commands.

A small abstract output interface allows host tests to substitute a fake LED and record logical state changes without physical hardware.

## 7. Identification common command

The reusable `scpi::Identification` helper registers the common query `*IDN?`. The application supplies the four response fields: manufacturer, model, serial number, and firmware revision. The handler returns them in that order, separated by commas. For the ESP32 application, the serial-number field will be populated from the device Wi-Fi station MAC address (for example, through the Arduino-ESP32 Wi-Fi API) in six-octet hexadecimal form. This keeps Wi-Fi and board dependencies out of the generic SCPI engine. The firmware revision is supplied by application build/version configuration. Fields must not contain commas because this initial implementation does not implement quoted-field escaping. The `Identification` object must remain alive while the registry may dispatch its handler. `*IDN` without the query marker, or `*IDN?` with parameters, reports error `-108` (Parameter not allowed). This is an initial common-command subset, not a claim of complete IEEE 488.2 support.

## 8. HEARTBEAT SCPI command module

The command module connects the generic engine to the HEARTBEAT service. It registers handlers and translates SCPI parameters into service operations.

The agreed initial command family is:

| Command | Meaning |
|---|---|
| HEARTbeat:STATe ON | Enable heartbeat and begin a fresh cycle using current settings |
| HEARTbeat:STATe OFF | Immediately interrupt heartbeat and turn logical LED output off |
| HEARTbeat:STATe? | Query enabled state |
| HEARTbeat:PERiod <seconds> | Set configured complete blink period in seconds; takes effect on next enable |
| HEARTbeat:PERiod? | Query configured period |
| HEARTbeat:DUTYcycle <percent> | Set duty cycle from 0 through 100; while enabled, immediately restart the cycle using the new duty cycle |
| HEARTbeat:DUTYcycle? | Query duty cycle |

These are project command-name decisions, subject to verification of SCPI spelling, short/long forms, and syntax against the selected SCPI specification and supported subset.

The command module validates values and reports invalid settings through the generic engine's error mechanism. It does not perform GPIO operations or implement the timing algorithm.

## 9. Application main-loop integration

The application owns the objects and connects the boundaries. Conceptually, its lifecycle is:

1. At the beginning of `setup()`, initialize the LED output sufficiently to command logical ON, then turn it ON.
2. Perform the remaining setup work while the LED remains ON.
3. Turn the LED OFF immediately before `setup()` returns.
4. At the start of the first `loop()` execution, enable HEARTBEAT, beginning a fresh cycle using the currently configured period and duty cycle.
5. On each loop iteration, service/read available transport input and submit complete messages to the SCPI engine, then call the HEARTBEAT service with the current monotonic time, and continue with other application work.

The exact order of the recurring loop operations may be adjusted, but neither the engine nor the HEARTBEAT service should block waiting for I/O or time to pass. A long-running handler elsewhere in the application can still delay heartbeat servicing; the heartbeat therefore indicates that the loop is being serviced, not that every subsystem is healthy.

## 10. Host-test strategy

The first host test build should substitute fakes for hardware and transport:

- Fake response sink captures response text from the SCPI engine.
- Message feeder/mock transport submits complete command messages.
- Fake clock advances time under test control.
- Fake LED output records logical ON/OFF changes.

Tests should verify the setup LED sequence (ON at setup start, OFF at setup completion), HEARTBEAT starting when loop execution begins, command registration and dispatch; state, configured-period, and duty-cycle queries; valid and invalid parameter handling; disabled state immediately producing logical OFF even in the middle of a cycle; enabling starting a fresh cycle with the latest configured period and duty cycle; a period change not altering an active run; duty-cycle changes immediately restarting the active cycle; 0% and 100% duty-cycle behavior; intermediate duty-cycle timing and period boundaries; no early output transition; prompt return without waiting; and timer rollover behavior.

The same HEARTBEAT service logic should be used in host tests and in the ESP32 application; only the clock and LED output adapters should differ.

## 11. Decisions still open

- How SCPI short/long keyword forms and command-tree path rules are resolved before registry dispatch.
- Maximum command-token and response sizes (the transport message limit is currently 256 bytes).
- Whether period queries report the configured period (the value that will be used on the next enable) or also expose a separate active-period query while enabled.
- Confirm the onboard LED polarity on the physical ESP32 DevKit V1; the user confirmed an ESP32-WROOM-32 board with GPIO 2 LED.
- Confirm the physical serial adapter/port and verify the first firmware upload.

## 12. First implementation order

1. Implement and host-test the message framer and the engine entry-point/transport bridge. **The code and host test sources are now in place; execution remains to be verified in a working build environment.**
2. Define and test the SCPI error queue and response-sink interfaces. **The interfaces and error-queue host test source are now in place; execution remains to be verified in a working build environment.**
3. Implement the initial parser and SCPI short/long keyword resolution; add host tests. **The first parser slice, registry abbreviation matching, synchronous engine-to-registry dispatch, and initial `*IDN?` common-command support are now in place; broader SCPI syntax remains future work.**
4. Implement the HEARTBEAT service against a fake clock and fake logical LED output.
5. Register the HEARTBEAT SCPI command module with the engine. **The command adapter and host test source are now in place; execution remains to be verified in a working build environment.**
6. Test command-to-service behavior and query responses on the host. **The tests cover state, period, duty cycle, query formatting, and parameter errors; execution remains to be verified.**
7. Add the selected board's GPIO adapter and integrate the service into the Arduino main loop.

This order tests each boundary independently before combining them on hardware.


## 13. Initial HEARTBEAT service implementation

The first implementation is in `src/services/heartbeat.h`. It has no Arduino or ESP32 dependencies. The application supplies time in unsigned milliseconds and implements `services::LogicalLedOutput`; the service calls that interface only when the logical LED state changes.

The initial numeric contract is:
- Period unit: milliseconds, unsigned 32-bit value.
- Resolution: 1 ms.
- Accepted period range: 1 ms through 86,400,000 ms (24 hours).
- Default period: 2,000 ms.
- Duty cycle: integer percentage from 0 through 100.
- Default duty cycle: 25%.

The active period is captured when HEARTBEAT is enabled. Updating the configured period while enabled does not alter the current run. Disabling and enabling again adopts the newly configured period. A duty-cycle update while enabled restarts the cycle immediately using the active period. The supplied time value makes the implementation deterministic in host tests; elapsed-time subtraction is rollover-safe for the supported period range.

The host test source is `test/host/heartbeat_test.cpp`, registered as `heartbeat_test` in CMake/CTest. It covers default settings, on/off timing, deferred period changes, immediate duty changes, 0% and 100% duty, invalid values, immediate disable, rollover, and avoiding redundant LED writes. These tests have been added but have **not yet been compiled or run**.


## 14. Initial HEARTBEAT SCPI command implementation

The command adapter is in `src/scpi/heartbeat_commands.h`. It registers `HEARTbeat:STATe`, `HEARTbeat:PERiod`, and `HEARTbeat:DUTYcycle` with the generic command registry and delegates all timing and LED behavior to `services::Heartbeat`. The application supplies a monotonic millisecond clock through a function pointer and context; this keeps Arduino and ESP32 APIs outside the command module.

The initial command parameter and response rules are:

- `HEARTbeat:STATe ON|OFF` accepts the discrete values ON and OFF, case-insensitively. `HEARTbeat:STATe?` returns `1` when enabled and `0` when disabled.
- `HEARTbeat:PERiod <seconds>` accepts seconds with 1 ms resolution, from 0.001 through 86400 seconds. Values requiring finer than 1 ms resolution are rejected rather than rounded. `HEARTbeat:PERiod?` returns the configured period in seconds, omitting insignificant trailing zeros.
- `HEARTbeat:DUTYcycle <percent>` accepts an integer from 0 through 100. `HEARTbeat:DUTYcycle?` returns that integer.
- Query commands reject parameters with error `-108` (Parameter not allowed).
- A set command with no parameter reports `-109` (Missing parameter).
- Malformed numeric values or unsupported period precision report `-128` (Numeric data error); out-of-range period or duty values report `-222` (Data out of range); an unsupported state token reports `-224` (Illegal parameter value).

The module's host test is `test/host/heartbeat_commands_test.cpp`, registered as `heartbeat_commands_test` in CMake/CTest. It verifies command abbreviation dispatch, query responses, setting changes, period activation behavior, duty-cycle restart behavior, boundary values, and error reporting. The test source has been added but has not yet been compiled or executed in this environment.


## 15. Initial ESP32 Arduino example

A first hardware integration sketch is provided at
`examples/esp32_scpi/esp32_scpi.ino`. It demonstrates the application wiring
without moving ESP32 dependencies into the SCPI core:

- Uses the Arduino-ESP32 `WiFi` API to read the station MAC address and supplies
  it as the serial-number field in `*IDN?`.
- Uses `! Amused Scientist` as the manufacturer placeholder and
  `SCPI-ESP32` as the model.
- Uses GPIO 2 as the default status LED pin and assumes active-high wiring.
  Define `SCPI_STATUS_LED_PIN` or `SCPI_STATUS_LED_ACTIVE_HIGH` before
  compilation to adapt those board settings.
- Turns the LED on at the beginning of `setup()`, turns it off before setup
  returns, and starts HEARTBEAT on the first execution of `loop()`.
- Uses USB/UART serial at 115200 baud as the initial transport, with CR, LF, or CRLF
  message termination, plus a configurable 1,000 ms inactivity timeout for
  unterminated messages, and a 256-byte maximum input message.
- Writes query responses as one line terminated by the serial sink.

The example intentionally does not connect to a Wi-Fi access point. Wi-Fi is
placed in station mode to obtain the station MAC address; no network SCPI
transport is implemented yet.

Install this repository as an Arduino library (for example, clone it into
the Arduino libraries directory as `SCPI-ESP32`), then open
`examples/esp32_scpi/esp32_scpi.ino` from the Arduino IDE. Select a compatible
ESP32 board and its serial port before compiling. The sketch is configured for PlatformIO's `esp32dev` target, and the GitHub
Actions workflow now includes both host tests and an ESP32 cross-compile. A
successful build has not yet been observed. The first hardware test should
verify setup LED behavior, heartbeat cadence, and `*IDN?` using CR, LF, CRLF,
and no terminator (which should dispatch after the 1,000 ms inactivity timeout).
