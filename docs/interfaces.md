# Software Interface Contract

**Status:** Initial implementation contract. The message-framing component, engine entry-point interface, and framed-input adapter are implemented and host-test sources are provided; the parser and dispatcher remain future work.

This document defines the responsibilities and boundaries for the first host-testable SCPI-ESP32 vertical slice. It deliberately avoids choosing a final callback mechanism, class hierarchy, or build system.

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

The initial C++ entry point is `scpi::EngineInterface::processScpiMessage(const std::string&)`. The transport calls it once for each complete, non-empty program message. The full engine will additionally need equivalents of these operations:

- Register a command path and its handler.
- Submit one complete SCPI program message for processing.
- Write a response from a query handler.
- Record or return a defined error when processing fails.

The engine should receive an application-provided output sink or equivalent abstraction. This allows host tests to capture response text without a serial port.

A command handler should receive only the parsed request context and the application/service object it needs. It should not need direct access to Arduino APIs.

## 3. Transport boundary

A transport moves bytes and determines when a complete message is ready. The transport/application boundary supplies complete messages to the engine and carries engine responses back to the controller.

A single serial read or TCP read is not necessarily one complete SCPI message. TCP is a byte stream; message boundaries are established by `transport::MessageFramer` using these project rules:

- LF terminates a message.
- CRLF terminates a message and is treated as one terminator.
- CR alone does not terminate a message; a CR not followed by LF is retained as message data.
- An optional inactivity timeout can terminate a message when no terminator is sent; it is disabled by default.
- Empty messages are not dispatched. Oversized messages are reported and discarded through the next boundary.

The current default maximum message size is 256 bytes.

For the first host tests, a mock transport or direct message feeder is sufficient. The initial physical transport can be selected later.

## 4. HEARTBEAT service boundary

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

The valid positive period range, resolution, and numeric representation still need to be selected. Invalid values must be rejected without silently changing the active configuration.

## 5. Logical LED output and board adapter

The HEARTBEAT service should request a logical state, such as setLedOn(true) or setLedOn(false). It should not write a voltage level.

A board-specific adapter translates that logical state into GPIO operations. It knows the selected GPIO pin, knows whether the LED is active-high or active-low, performs required pin initialization, and maps logical ON/OFF to the correct electrical output.

For an active-high LED, logical ON generally maps to a high output. For an active-low LED, logical ON generally maps to a low output. The adapter owns this translation.

The pin and polarity are board/application configuration. They are not generic SCPI engine settings, and they are not required HEARTBEAT SCPI commands.

A small abstract output interface allows host tests to substitute a fake LED and record logical state changes without physical hardware.

## 6. HEARTBEAT SCPI command module

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

## 7. Application main-loop integration

The application owns the objects and connects the boundaries. Conceptually, its lifecycle is:

1. At the beginning of `setup()`, initialize the LED output sufficiently to command logical ON, then turn it ON.
2. Perform the remaining setup work while the LED remains ON.
3. Turn the LED OFF immediately before `setup()` returns.
4. At the start of the first `loop()` execution, enable HEARTBEAT, beginning a fresh cycle using the currently configured period and duty cycle.
5. On each loop iteration, service/read available transport input and submit complete messages to the SCPI engine, then call the HEARTBEAT service with the current monotonic time, and continue with other application work.

The exact order of the recurring loop operations may be adjusted, but neither the engine nor the HEARTBEAT service should block waiting for I/O or time to pass. A long-running handler elsewhere in the application can still delay heartbeat servicing; the heartbeat therefore indicates that the loop is being serviced, not that every subsystem is healthy.

## 8. Host-test strategy

The first host test build should substitute fakes for hardware and transport:

- Fake response sink captures response text from the SCPI engine.
- Message feeder/mock transport submits complete command messages.
- Fake clock advances time under test control.
- Fake LED output records logical ON/OFF changes.

Tests should verify the setup LED sequence (ON at setup start, OFF at setup completion), HEARTBEAT starting when loop execution begins, command registration and dispatch; state, configured-period, and duty-cycle queries; valid and invalid parameter handling; disabled state immediately producing logical OFF even in the middle of a cycle; enabling starting a fresh cycle with the latest configured period and duty cycle; a period change not altering an active run; duty-cycle changes immediately restarting the active cycle; 0% and 100% duty-cycle behavior; intermediate duty-cycle timing and period boundaries; no early output transition; prompt return without waiting; and timer rollover behavior.

The same HEARTBEAT service logic should be used in host tests and in the ESP32 application; only the clock and LED output adapters should differ.

## 9. Decisions still open

- Whether handlers use function pointers, callbacks, interfaces, or another registration mechanism.
- How response writing and error reporting are exposed to handlers.
- How the parser represents command headers, parameters, and message units internally.
- Maximum command-token and response sizes (the transport message limit is currently 256 bytes).
- Period numeric representation, minimum/maximum values, and resolution.
- Whether period queries report the configured period (the value that will be used on the next enable) or also expose a separate active-period query while enabled.
- The first ESP32 board and its actual onboard LED pin/polarity.
- The first physical transport and host build/test framework.

## 10. First implementation order

1. Implement and host-test the message framer and the engine entry-point/transport bridge. **The code and host test sources are now in place; execution remains to be verified in a working build environment.**
2. Implement the generic parser, command registry, and dispatcher with a small test command.
3. Implement the HEARTBEAT service against a fake clock and fake logical LED output.
4. Register the HEARTBEAT SCPI command module with the engine.
5. Test command-to-service behavior and query responses on the host.
6. Add the selected board's GPIO adapter and integrate the service into the Arduino main loop.

This order tests each boundary independently before combining them on hardware.
