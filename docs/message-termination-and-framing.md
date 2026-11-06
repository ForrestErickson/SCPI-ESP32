# Message Termination and Transport Framing Requirements

**Status:** Working requirements with an initial host-testable framing implementation. The implementation is a project transport utility, not a claim of SCPI or IEEE 488.2 conformance.

## 1. Terminator cases to support

The serial receive layer shall be designed to recognize these input cases:

| Input from controller | Proposed handling | Standards status |
|---|---|---|
| LF (ASCII 0x0A) | End the current program message. | SCPI-99 / IEEE 488.2 newline termination. |
| CR-LF (0x0D 0x0A) | Treat the pair as one message terminator, not two. | Commonly accepted compatibility form; confirm exact IEEE details. |
| CR (ASCII 0x0D) alone | End the current message. | Project compatibility extension; not assumed to be a SCPI-99 required terminator. |
| No terminator | Optionally end the message after a configurable period of receive inactivity. | Transport-specific convenience behavior; not a SCPI message terminator and not a standards-conformance substitute. |

The SCPI-99 baseline requires a newline-terminated program message; common instrument documentation describes LF and CR-LF handling. A standalone CR is included here because the project intends to work conveniently with terminal programs and UART controllers that can be configured to send CR. It must be documented as an accepted compatibility extension, not as a claim that CR-alone is required by SCPI.

The SCPI standard also recognizes IEEE-488 EOI as a message-ending mechanism on interfaces that provide it. UART and ordinary TCP byte streams do not provide GPIB EOI. Interface-specific end-of-message mechanisms must therefore remain the responsibility of the transport adapter.

## 2. Timeout-delimited input

For an unterminated UART command, the receiver may use an **inter-character inactivity timeout**:

1. The first received byte starts a candidate message.
2. Each subsequent byte restarts the inactivity interval.
3. If no further byte arrives before the configured interval expires, the accumulated bytes are delivered as one complete message.
4. The timeout is configurable in the UART transport configuration.
5. The timeout feature can be disabled. The initial implementation defaults to disabled; applications must explicitly configure a timeout suitable for their baud rate and sender behavior.
6. A timeout-delimited message is explicitly marked as transport-delimited, so the parser and diagnostics can distinguish it from a message terminated by LF, CR-LF, or CR.

This must be implemented without blocking delays. The receiver should use elapsed-time checks in its normal service loop or an equivalent nonblocking mechanism.

**Important limitation:** an inactivity timeout cannot reliably distinguish a complete command from a pause in the middle of a slowly transmitted command. The timeout must therefore be long enough for the configured baud rate and expected sender behavior. It should be configurable per transport, not embedded in the SCPI parser.

## 3. CR and CR-LF recognition

Because CR may be either a standalone terminator or the first byte of CR-LF, the receiver must not emit two message boundaries for CR-LF.

Implemented state-machine behavior:
- On CR, hold a pending-CR state rather than immediately completing the message.
- If the next byte supplied to the framer is LF, complete exactly one message with the CR-LF termination reason.
- If the next byte is not LF, complete the preceding message as CR-terminated, then process that byte as the beginning of the next message.
- The transport calls `poll()` after draining currently available input. If CR is still pending at that point, `poll()` completes it as CR-only. This avoids a timed wait while allowing CR-LF split across successive byte-feed calls in the same drain operation to be recognized.

LF alone completes immediately. Empty messages caused by extra terminators are not dispatched.

LF alone terminates immediately. Empty messages caused by extra terminators should be handled deterministically and should not dispatch an empty command.

## 4. Separation of responsibilities

The transport adapter owns:
- Receiving bytes from UART, USB serial, or TCP.
- Detecting configured message boundaries.
- Buffering bytes up to a documented maximum.
- Reporting message completion, overflow, and transport errors.
- Applying the optional inactivity timeout.
- Identifying whether completion came from LF, CR-LF, CR, or inactivity timeout.

The generic SCPI parser owns:
- Parsing the complete program message it is given.
- Splitting semicolon-separated program message units.
- Parsing headers and parameters.
- Dispatching commands and producing errors/responses.

The generic parser shall not wait for UART bytes, call a serial API, or implement a transport inactivity timer.

TCP requires special care: TCP is a byte stream and does not preserve sender write boundaries. A TCP adapter must use explicit terminators or a separately configured framing policy. An inactivity timeout can be offered for interactive use, but must not be mistaken for a reliable general-purpose TCP message boundary.

## 5. Response termination is a separate setting

Input termination and output response termination are separate concerns.

The transport/application configuration shall eventually define response termination independently (for example LF or CR-LF). The engine must not assume that accepting CR, LF, CR-LF, or timeout-delimited input means it should use the same form for responses.

Response termination policy and exact IEEE 488.2 response-message requirements remain a separate standards-review item.

## 6. Proposed requirement IDs and tests

### MSG-002A — LF input terminator

LF ends exactly one program message and is not passed to the command parser as command data.

Tests: LF after a valid command; LF after a compound message; LF with no preceding data; repeated LF.

### MSG-002B — CR-LF input terminator

CR-LF ends exactly one program message. The receiver shall not create an additional empty message because it processed CR and LF independently.

Tests: CR-LF after a command; split CR and LF across separate receive calls; CR-LF following a compound message; repeated CR-LF.

### MSG-002C — CR-only compatibility terminator

A standalone CR ends exactly one program message when this compatibility option is enabled.

Tests: CR after a command; CR-only option enabled/disabled; CR followed by the first byte of the next command; CR at the end of a receive buffer.

### MSG-002D — Optional inactivity timeout

When enabled, an incomplete-by-terminator byte sequence is delivered as one message after the configured receive-inactivity interval. When disabled, it remains buffered until a configured terminator or another defined transport event occurs.

Tests: timeout after one byte and after a complete command string; byte arrival just before expiry; timeout disabled; new command after timeout; timer wraparound; no blocking; timeout during a deliberately slow transmission.

### MSG-002E — Input buffer bounds and recovery

The transport adapter shall enforce a maximum message size. On overflow, it shall report a defined transport/input error, discard or resynchronize according to a documented policy, and recover for the next valid message.

Tests: maximum accepted length; one byte beyond maximum; overflow followed by LF; overflow followed by a valid subsequent command; repeated overflow.

### MSG-002F — Transport/parser boundary

The parser receives complete messages and has no dependency on UART, TCP, or Arduino serial APIs.

Tests: feed identical message byte sequences through a fake transport and at least two transport adapters when available; host-build parser tests without Arduino/ESP32 headers.

### MSG-002G — Response terminator configuration

Response termination is configured independently from input termination.

Tests: verify configured LF and CR-LF responses; ensure input CR-only compatibility and timeout modes do not silently change output framing.

## 7. Open design decisions

1. Select an application-appropriate UART inactivity timeout; the implementation default is disabled.
2. Decide whether CR-only acceptance should be enabled by default. The implementation makes it configurable and currently defaults it to enabled for terminal compatibility.
3. Confirm the maximum program-message size; the implementation currently defaults to 256 bytes, configurable per framer.
4. Define exact behavior for buffer overflow and recovery.
5. Choose the initial response terminator, independently of accepted input terminators.
6. Determine whether the first transport implementation is UART/USB serial, and define TCP framing separately when TCP is added.
7. Confirm all normative message-termination and response-message requirements against the complete IEEE 488.2-1992 document and compare relevant provisions of IEEE/IEC 60488-2-2004.

## 8. Initial host implementation

The first implementation is `src/transport/message_framer.h`. It is independent of Arduino and ESP32 headers and exposes `feed(byte, now_ms)`, `poll(now_ms)`, and `reset()` operations. Completed messages carry a termination reason (`LF`, `CR-LF`, `CR`, or inactivity timeout); buffer overflow is reported separately.

The initial configurable defaults are a 256-byte maximum message size, inactivity timeout disabled (`0`), and CR compatibility enabled. These are implementation defaults for this first slice, not final project-wide policy. Overflow is reported once and the remainder of that message is discarded through its next terminator or timeout.

A minimal CMake host build and `test/host/message_framer_test.cpp` exercise LF, CR-LF across calls, CR-only, configurable CR handling, timeout behavior, timeout disabled, timer rollover, and overflow recovery. The host build and test pass with C++11. The transport adapter still needs to call `poll()` after draining available input and regularly service timeout checks.

The implementation is an initial tested slice; the remaining open decisions and standards review above are not thereby closed.
