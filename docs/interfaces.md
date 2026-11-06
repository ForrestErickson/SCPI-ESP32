# Initial Software Interface Proposal

**Status:** Proposal for review; not an implementation contract yet.

This document defines the responsibilities and boundaries for the first host-testable SCPI-ESP32 vertical slice. It deliberately avoids choosing a final callback mechanism, class hierarchy, or build system.

## 1. Overall flow

1. A transport receives bytes and identifies complete SCPI program messages according to its framing rules.
2. The application passes each complete message to the SCPI engine.
3. The engine parses the message, resolves the command, validates syntax and parameters, and invokes the registered handler.
4. A handler may update application-service settings or return query data.
5. The engine sends responses through an output/response interface.
6. The application's main loop regularly services both the SCPI input path and the HEARTBEAT service.

The engine does not operate GPIO. The HEARTBEAT service does not parse SCPI. The GPIO adapter does not know SCPI command names.

## 2. Generic SCPI engine boundary

### Responsibilities

The generic engine should accept complete program messages, parse and dispatch commands according to the documented SCPI subset, resolve registered command paths, distinguish commands from queries, make parsed parameters available to handlers, provide a response-writing mechanism, and report syntax, parameter, and command errors through its error mechanism.

The engine should not include Arduino or ESP32 headers; know about GPIO pins, LEDs, or HEARTBEAT behavior; depend on a particular instrument application; or own the transport's byte-level framing.

### Conceptual interface

The first implementation needs equivalents of these operations, regardless of the final C++ form:

- Register a command path and its handler.
- Submit one complete SCPI program message for processing.
- Write a response from a query handler.
- Record or return a defined error when processing fails.

The engine should receive an application-provided output sink or equivalent abstraction. This allows host tests to capture response text without a serial port.

A command handler should receive only the parsed request context and the application/service object it needs. It should not need direct access to Arduino APIs.

## 3. Transport boundary

A transport moves bytes and determines when a complete message is ready. The transport/application boundary supplies complete messages to the engine and carries engine responses back to the controller.

A single serial read or TCP read is not necessarily one complete SCPI message. TCP is a byte stream; message boundaries must be established using the selected SCPI termination/framing rules.

For the first host tests, a mock transport or direct message feeder is sufficient. The initial physical transport can be selected later.

## 4. HEARTBEAT service boundary

### Responsibilities

The HEARTBEAT service owns whether heartbeat operation is enabled, the configured period in seconds per complete cycle, the configured duty cycle as a percentage, and the timing state needed to determine whether the logical LED output should be on or off.

It does not own SCPI parsing or command registration, a GPIO number or electrical polarity, Arduino pin APIs, or blocking delays.

### Conceptual operations

The service needs equivalents of:

- Set/get enabled state.
- Set/get period.
- Set/get duty cycle.
- A service operation such as service(now), called frequently by the application loop, which updates the logical LED output only when needed and returns promptly.

The service should use an injected or supplied monotonic time source. This makes timing deterministic in host tests and avoids tying the service to a particular ESP32 clock API.

The service must not call delay() or otherwise wait for a blink transition. It calculates the desired output from elapsed time and returns. Elapsed-time arithmetic must be safe across timer rollover.

### Timing semantics

- Period is the duration of one complete on/off cycle, in seconds.
- Duty cycle is the fraction of that period for which the LED is logically on.
- At 0% duty cycle, the logical output remains off.
- At 100% duty cycle, the logical output remains on while heartbeat is enabled.
- When heartbeat is disabled, the logical output is off.
- For intermediate duty cycles, the on interval begins at the start of each cycle.

The valid positive period range, resolution, numeric representation, and startup defaults still need to be selected. Invalid values must be rejected without silently changing the active configuration.

## 5. Logical LED output and board adapter

The HEARTBEAT service should request a logical state, such as setLedOn(true) or setLedOn(false). It should not write a voltage level.

A board-specific adapter translates that logical state into GPIO operations. It knows the selected GPIO pin, knows whether the LED is active-high or active-low, performs required pin initialization, and maps logical ON/OFF to the correct electrical output.

For an active-high LED, logical ON generally maps to a high output. For an active-low LED, logical ON generally maps to a low output. The adapter owns this translation.

The pin and polarity are board/application configuration. They are not generic SCPI engine settings, and they are not required HEARTBEAT SCPI commands.

A small abstract output interface allows host tests to substitute a fake LED and record state changes without physical hardware.

## 6. HEARTBEAT SCPI command module

The command module connects the generic engine to the HEARTBEAT service. It registers handlers and translates SCPI parameters into service operations.

The agreed initial command family is:

| Command | Meaning |
|---|---|
| HEARTbeat:STATe ON | Enable heartbeat |
| HEARTbeat:STATe OFF | Disable heartbeat and turn logical LED output off |
| HEARTbeat:STATe? | Query enabled state |
| HEARTbeat:PERiod <seconds> | Set complete blink period in seconds |
| HEARTbeat:PERiod? | Query period |
| HEARTbeat:DUTYcycle <percent> | Set duty cycle from 0 through 100 |
| HEARTbeat:DUTYcycle? | Query duty cycle |

These are project command-name decisions, subject to verification of SCPI spelling, short/long forms, and syntax against the selected SCPI specification and supported subset.

The command module validates values and reports invalid settings through the generic engine's error mechanism. It does not perform GPIO operations or implement the timing algorithm.

## 7. Application main-loop integration

The application owns the objects and connects the boundaries. Conceptually, its loop does the following:

1. Service/read available transport input and submit complete messages to the SCPI engine.
2. Call the HEARTBEAT service with the current monotonic time.
3. Continue with other application work.

The exact order may be adjusted, but neither the engine nor the HEARTBEAT service should block waiting for I/O or time to pass. A long-running handler elsewhere in the application can still delay heartbeat servicing; the heartbeat therefore indicates that the loop is being serviced, not that every subsystem is healthy.

## 8. Host-test strategy

The first host test build should substitute fakes for hardware and transport:

- Fake response sink captures response text from the SCPI engine.
- Message feeder/mock transport submits complete command messages.
- Fake clock advances time under test control.
- Fake LED output records logical ON/OFF changes.

Tests should verify command registration and dispatch; state, period, and duty-cycle queries; valid and invalid parameter handling; disabled state producing logical OFF; 0% and 100% duty-cycle behavior; intermediate duty-cycle timing and period boundaries; no early output transition; prompt return without waiting; and timer rollover behavior.

The same HEARTBEAT service logic should be used in host tests and in the ESP32 application; only the clock and LED output adapters should differ.

## 9. Decisions to make before implementation

- How complete messages and termination characters are represented at the engine boundary.
- Whether handlers use function pointers, callbacks, interfaces, or another registration mechanism.
- How response writing and error reporting are exposed to handlers.
- Maximum command/message and response sizes.
- Period numeric representation, minimum/maximum values, and resolution.
- Startup defaults for enabled state, period, and duty cycle.
- Whether a period or duty-cycle change restarts the current cycle or takes effect relative to the existing cycle.
- The first ESP32 board and its actual onboard LED pin/polarity.
- The first physical transport and host build/test framework.

## 10. First implementation order

1. Implement and host-test the generic engine boundary with a simple test command.
2. Implement the HEARTBEAT service against a fake clock and fake logical LED output.
3. Register the HEARTBEAT SCPI command module with the engine.
4. Test command-to-service behavior and query responses on the host.
5. Add the selected board's GPIO adapter and integrate the service into the Arduino main loop.

This order tests each boundary independently before combining them on hardware.
