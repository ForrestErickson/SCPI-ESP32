# SCPI-ESP32 Requirements-to-Tests Compliance Matrix

**Status:** Working matrix; detailed requirements and clause mappings remain under review.  
**Project decision:** Eventual SCPI conformance, developed incrementally, with every implemented feature tested.  
**Approved working standards baseline (2026-11-06):** SCPI-99 (1999.0) and its cited IEEE 488.2-1992 reference; IEEE/IEC 60488-2-2004 will be investigated but is not presumed to replace the cited edition.  
**Current implementation status:** No parser or conformance feature is yet marked implemented and tested.

## 1. Purpose and status conventions

This matrix connects each requirement to one or more verification tests. It is intended to grow with the implementation and to prevent unsupported conformance claims.

Status values:

- **Not started** — no implementation or verification yet.
- **In progress** — implementation or test work has begun.
- **Implemented, tests pending** — code exists, but the required tests have not all passed.
- **Implemented and tested** — implementation and listed tests pass.
- **Not applicable** — excluded only with a documented standards-based rationale.

A requirement is not complete merely because a command with the expected name exists. Tests must exercise behavior, including error cases and interactions where applicable.

The standards references below are provisional. The initial research references are SCPI 1999.0 Volume 1 and IEEE 488.2-1992. The project has not yet frozen its target editions, reviewed all four SCPI volumes, or completed clause-by-clause applicability analysis. Clause references must be confirmed before this becomes a formal compliance record.

## 2. Standards and language requirements

| ID | Requirement | Source / basis | Planned verification | Status |
|---|---|---|---|---|
| STD-001 | Select and record the SCPI and IEEE 488.2 working baseline, then confirm final applicability and any relationship to later related editions. | User-approved working baseline: SCPI-99 (1999.0) and its cited IEEE 488.2-1992 reference; IEEE/IEC 60488-2-2004 comparison remains open. | Verify all four SCPI-99 volumes, available errata, and the relevant IEEE documents; document applicability and any differences. | In progress |
| STD-002 | Maintain traceability from each applicable standards requirement to implementation and tests. | Project conformance decision | Matrix review: every applicable requirement has implementation location and test IDs. | Not started |
| MSG-001 | Recognize program messages containing one or more program-message units, with compound units separated by semicolons. | SCPI 1999.0 Vol. 1 §6 and IEEE 488.2 message grammar; detailed IEEE mapping pending. See [program-message and header requirements](program-message-and-header-requirements.md), PM-001. | Test one-unit and multi-unit messages, ordering, malformed separators, and parser recovery. | Not started |
| MSG-002 | Recognize LF, CR-LF, and configurable CR-only input termination; optionally deliver unterminated UART input after a configurable inactivity timeout; keep framing in the transport adapter. | SCPI-99 newline requirement and IEEE 488.2 message framing; CR-only and inactivity timeout are explicitly project compatibility features. See [message termination and framing requirements](message-termination-and-framing.md), MSG-002A–G. | Test each terminator, CR-LF split across receive calls, timeout enabled/disabled and boundary timing, fragmentation, buffer overflow/recovery, and independent response termination. | Not started |
| HDR-001 | Treat instrument-control headers as case-insensitive. | SCPI 1999.0 Vol. 1 §§5.2, 6; detailed requirement PM/HDR-001. | Same registered command succeeds in uppercase, lowercase, and mixed case. | Not started |
| HDR-002 | Accept only the exact short and long form of each registered instrument-control keyword; reject all other partial forms. | SCPI 1999.0 Vol. 1 §6.2.1; detailed requirement HDR-002. | Test exact short/long forms, invalid prefixes, short-form collisions, and registry-time ambiguity detection. | Not started |
| HDR-003 | Enforce the 12-character header limit, including numeric suffix, and parse numeric suffixes/default capability 1 where applicable. | SCPI 1999.0 Vol. 1 §6.2.1; IEEE 488.2 length/suffix grammar still to be checked. See detailed requirement HDR-003. | Boundary tests, suffix parsing, omitted-suffix behavior for multi-capability commands, and single-capability suffix policy. | Not started |
| HDR-004 | Parse hierarchical command paths and colon separators, with unique registered paths and explicit default-node handling. | SCPI 1999.0 Vol. 1 §6.2.2; detailed requirement HDR-004. | Test valid/invalid paths, unknown nodes, ambiguous registrations, and default-node cases. | Not started |
| HDR-005 | Implement SCPI §6.2.4 traversal: first header root-relative; subsequent headers at the same tree level as the preceding header; leading colon resets to root; do not implement enhanced tree walking; default nodes do not alter retained path. | SCPI 1999.0 Vol. 1 §6.2.4; detailed requirement HDR-005. | Reproduce §6.2.4 sibling, root-reset, invalid branch, and default-node examples as regression tests. | Not started |
| HDR-006 | Distinguish common commands beginning with * from instrument-control subsystem commands. | IEEE 488.2 / SCPI 1999.0 | Parse and dispatch common commands independently of the instrument command tree; reject malformed common-command forms. | Not started |
| QRY-001 | Distinguish commands from queries using the query syntax defined by SCPI. | SCPI 1999.0 Vol. 1, query rules | Test command and query forms, misplaced question marks, and query-only/command-only registrations. | Not started |
| QRY-002 | Return query data in the required response form without echoing the command header. | SCPI 1999.0 Vol. 1; response rules and clause mapping pending | Assert exact response bytes, including separators and terminator policy, for representative queries. | Not started |
| QRY-003 | Ordinary setting queries shall not change settings or couplings; account for the documented MEASurement-query exception. | SCPI 1999.0 Vol. 1 §6.2.3; detailed requirement QRY-002. | Snapshot settings/couplings for ordinary queries; test measurement-query effects separately when applicable. | Not started |
| PAR-001 | Parse parameter separators and supported parameter forms according to the selected SCPI subset. | SCPI 1999.0 / IEEE 488.2; data grammar mapping pending | Test zero, one, and multiple parameters; commas; whitespace; missing and excess parameters. | Not started |
| PAR-002 | Validate numeric parameter syntax, range, and representation. | SCPI 1999.0 / IEEE 488.2 data formats; detailed mapping pending | Test integer and decimal forms, signs, exponent forms if supported, boundaries, overflow, NaN/Inf policy, and malformed values. | Not started |
| PAR-003 | Validate character data and enumerated values using the applicable syntax. | SCPI 1999.0 / IEEE 488.2; detailed mapping pending | Test valid values, case behavior, invalid values, quoted forms if supported, and delimiter errors. | Not started |
| PAR-004 | Implement required response-data formatting for supported data types. | SCPI 1999.0 / IEEE 488.2; detailed mapping pending | Golden-output tests for numeric, character, and compound response values. | Not started |
| ERR-001 | Detect and report syntax, header, parameter, and execution errors using the applicable error model. | IEEE 488.2 / SCPI; error model mapping pending | Inject each error class and assert the correct error record/code, ordering, and queue behavior. | Not started |
| ERR-002 | Define and test error-queue behavior, including empty-queue and queue-overflow behavior as required. | IEEE 488.2 / SCPI; exact requirements pending | Query empty queue, add multiple errors, read errors in order, test overflow and clear behavior. | Not started |
| STAT-001 | Implement the applicable status data structures and register behavior. | IEEE 488.2 / SCPI; status model mapping pending | Unit tests for register bit behavior, event latching, enable masks, and clear/reset semantics. | Not started |
| STAT-002 | Implement status-byte and event-status interactions required by the selected standard. | IEEE 488.2 / SCPI; clause mapping pending | Cross-register tests for event propagation, masks, summary bits, and query side effects. | Not started |
| STAT-003 | Implement service-request behavior where required by the selected interface and target. | IEEE 488.2; applicability and physical-interface implications pending | Simulated request generation and enable-mask tests; hardware/interface integration tests if applicable. | Not started |
| SYNC-001 | Implement operation-complete and wait behavior with correct ordering and pending-operation semantics. | IEEE 488.2 common-command requirements | Tests with immediate and delayed operations, ensuring queries and subsequent commands observe correct completion semantics. | Not started |
| BIN-001 | Implement applicable block-data and binary-transfer syntax required for the selected conformance target. | IEEE 488.2 / SCPI data interchange; applicability mapping pending | Golden tests for each supported block form, lengths, truncation, malformed blocks, and binary payload bytes. | Not started |
| DOC-001 | Document supported features, unsupported features, and known deviations accurately. | Project conformance decision and standards discipline | Documentation review against the matrix and actual automated test results. | Not started |

Detailed parser requirements and proposed test cases for PM-001, HDR-001 through HDR-006, and QRY-001 through QRY-003 are maintained in [docs/program-message-and-header-requirements.md](program-message-and-header-requirements.md). All remain Not started until implemented and tested.\n\nDetailed input framing requirements and proposed tests for MSG-002A through MSG-002G are maintained in [docs/message-termination-and-framing.md](message-termination-and-framing.md). CR-only and inactivity-timeout completion are project compatibility behavior, not claims about mandatory SCPI terminators. All framing requirements remain Not started until implemented and tested.

## 3. IEEE 488.2 common-command verification

SCPI 1999.0 Volume 1 section 4.1.1 lists the following IEEE 488.2 common commands as mandatory in its SCPI context. This table records them as planned work; it does not imply implementation or settle the final target-edition applicability. Detailed semantics and clause references must be verified against the selected IEEE 488.2 edition.

| ID | Common command | Required behavior to specify and test | Planned test ID | Status |
|---|---|---|---|---|
| CC-001 | *CLS | Clear applicable status/event and error state as specified; verify precisely what is and is not cleared. | T-CC-001 | Not started |
| CC-002 | *ESE | Set Standard Event Status Enable register with specified parameter validation. | T-CC-002 | Not started |
| CC-003 | *ESE? | Return the Standard Event Status Enable register in the required format. | T-CC-003 | Not started |
| CC-004 | *ESR? | Return and apply the specified read/clear semantics for the Standard Event Status Register. | T-CC-004 | Not started |
| CC-005 | *IDN? | Return the required identification fields in the required comma-separated response format. | T-CC-005 | Not started |
| CC-006 | *OPC | Set operation-complete event state at the correct point after pending operations complete. | T-CC-006 | Not started |
| CC-007 | *OPC? | Respond according to operation-complete query semantics without responding prematurely. | T-CC-007 | Not started |
| CC-008 | *RST | Reset instrument/application state and applicable status state according to the selected requirements. | T-CC-008 | Not started |
| CC-009 | *SRE | Set Service Request Enable register with specified parameter validation and reserved-bit behavior. | T-CC-009 | Not started |
| CC-010 | *SRE? | Return the Service Request Enable register in the required format. | T-CC-010 | Not started |
| CC-011 | *STB? | Return the status byte with required summary and read-side-effect behavior. | T-CC-011 | Not started |
| CC-012 | *TST? | Perform/report self-test result using the required response and error semantics. | T-CC-012 | Not started |
| CC-013 | *WAI | Ensure subsequent processing waits for pending operations as specified, without incorrectly blocking unrelated servicing. | T-CC-013 | Not started |

**Important:** These commands are interdependent. In particular, *ESE, *ESR?, *SRE, *STB?, *OPC, *OPC?, and *WAI rely on correctly designed status and operation models. They should not be implemented as isolated command-name stubs.

## 4. Project architecture and quality requirements

These are project requirements, not claims that each item is independently mandated by SCPI.

| ID | Requirement | Basis | Planned verification | Status |
|---|---|---|---|---|
| ARC-001 | Keep the generic SCPI engine independent of any specific instrument. | Project requirements | Build and test the core with a dummy instrument; no instrument-specific headers or symbols in the core. | Not started |
| ARC-002 | Keep transport framing and byte I/O separate from parsing and command execution. | Project requirements | Run the same engine tests using a message feeder and more than one transport adapter when available. | Not started |
| ARC-003 | Allow application modules to register command paths and handlers. | Project requirements | Register two independent test modules and verify both dispatch correctly. | Not started |
| ARC-004 | Keep hardware drivers and GPIO operations outside the generic engine. | Project requirements | Host-build the core without Arduino/ESP32 headers; inspect dependency boundaries. | Not started |
| ARC-005 | Bound memory use and avoid unnecessary dynamic allocation in normal command processing. | Project requirements | Static/resource review; stress tests at configured maximum message, token, parameter, and response sizes. | Not started |
| ARC-006 | Build and run core unit tests on a desktop host. | Project requirements | CI or documented host build runs all core tests without physical hardware. | Not started |
| ARC-007 | Define deterministic behavior for malformed and unsupported input. | Project requirements | Negative tests for every parser/dispatcher error class and recovery with a valid following message. | Not started |
| ARC-008 | Keep implementation documentation and tests aligned with actual behavior. | Project requirements | Release checklist verifies each documented supported feature has passing test evidence. | Not started |

## 5. HEARTBEAT application-support verification

HEARTBEAT is a project-defined application-support command family. It is not part of the generic SCPI parser and must not be presented as a standardized SCPI subsystem.

| ID | Requirement | Basis | Planned verification | Status |
|---|---|---|---|---|
| HB-001 | At the beginning of setup, command the logical LED ON; keep it ON through setup; turn it OFF immediately before setup returns. | Approved project behavior | Fake LED records setup sequence and order. | Not started |
| HB-002 | Start HEARTBEAT at the beginning of the first main-loop execution, not during setup. | Approved project behavior | Lifecycle test verifies enable event occurs on first loop entry. | Not started |
| HB-003 | Default period is 2 seconds per complete cycle. | Approved project behavior | Query initial configured period; timing test verifies cycle length. | Not started |
| HB-004 | Default duty cycle is 25 percent. | Approved project behavior | Query initial duty cycle; timing test verifies 0.5 s ON and 1.5 s OFF for the default cycle. | Not started |
| HB-005 | HEARTBEAT operation is nonblocking and serviced regularly by the application loop. | Approved project behavior | Fake-clock tests verify prompt return and correct output transitions without delay calls. | Not started |
| HB-006 | GPIO pin and active polarity are supplied by board/application configuration. | Approved project architecture | Adapter tests verify active-high and active-low mapping independently. | Not started |
| HB-007 | STATe OFF immediately interrupts the active cycle and logically turns the LED OFF. | Approved project behavior | Turn OFF mid-ON and mid-OFF intervals; assert immediate logical OFF and disabled state. | Not started |
| HB-008 | STATe ON begins a fresh cycle using the currently configured period and duty cycle. | Approved project behavior | Change settings while disabled; enable; verify timing starts from the enable instant with current settings. | Not started |
| HB-009 | Changing PERiod while enabled changes the configured value but does not alter the current uninterrupted run; it becomes active on the next enable. | Approved project behavior | Change period mid-cycle; verify current run remains governed by its prior period; OFF/ON; verify new period. | Not started |
| HB-010 | Changing DUTYcycle while enabled immediately restarts timing using the new duty cycle and currently active period. | Approved project behavior | Change duty at known time; assert new cycle begins immediately and uses new duty. | Not started |
| HB-011 | Duty cycle 0 percent means logically OFF; 100 percent means logically ON while enabled. | Approved project behavior | Boundary tests at 0 and 100 percent, including enable/disable transitions. | Not started |
| HB-012 | Period and duty-cycle settings reject invalid values without silently changing valid configuration. | Approved project behavior; numeric limits still open | Test zero/negative/too-large/non-finite/malformed values after valid settings; verify error and unchanged configuration. | Not started |
| HB-013 | Timing arithmetic remains correct across monotonic clock rollover. | Approved project architecture | Fake clock begins near maximum counter value and crosses rollover during ON/OFF transitions. | Not started |
| HB-014 | HEARTBEAT commands are registered by the application module and dispatched by the generic engine. | Approved project architecture | Host integration tests for each command and query through the engine. | Not started |

## 6. Test-suite organization proposal

The test identifiers above are logical IDs. The final test framework and filenames remain open. A practical organization is:

| Test group | Coverage |
|---|---|
| T-MSG | Message framing, termination, compound program messages |
| T-HDR | Case folding, short/long keywords, hierarchy, tree traversal |
| T-QRY | Query recognition, response formatting, no state mutation |
| T-PAR | Parameter grammar, conversion, ranges, malformed parameters |
| T-ERR | Error classification, error queue, recovery |
| T-STAT | Status/event registers, enable masks, status-byte behavior |
| T-SYNC | Operation-complete and wait semantics |
| T-BIN | Block data and binary transfer formats |
| T-CC | Individual common-command behavior and interactions |
| T-ARC | Host portability, registration, dependency boundaries, bounded resources |
| T-HB | HEARTBEAT lifecycle, commands, timing, and GPIO adapter behavior |

Each test should have a stable name, an expected result, and a trace back to one or more requirement IDs. Where a standards example or exact normative behavior is used, record the standard edition and clause in the test source or test documentation.

## 7. Staged implementation plan

The sequence below is a planning aid. It does not replace the full requirement matrix.

1. **Standards baseline:** select target editions; acquire/review the complete relevant standards documents; confirm applicability and clause references.
2. **Message and parser foundation:** framing contract, message units, header grammar, case-insensitive exact short/long keyword matching, hierarchy, and command-tree traversal.
3. **Command execution foundation:** registry, command/query dispatch, parameter grammar, response output, and deterministic errors.
4. **Common-command foundation:** implement the common-command set with the status and operation models needed for correct semantics.
5. **Status and error system:** error queue, event registers, enable masks, status byte, and service-request integration as applicable.
6. **Data formats:** required numeric, character, response, and block-data formats.
7. **Application demonstration:** HEARTBEAT module and service, fully host-tested, followed by board integration.
8. **Conformance audit:** execute the complete applicable checklist, document any remaining gaps, and only then assess whether a conformance claim is justified.

The precise ordering of common commands, status features, and data formats may be refined after the standards clause review. Dependencies must be respected; for example, status-related common commands should follow the underlying status model.

## 8. Open items before this can become a formal conformance matrix

- Confirm the exact target editions of SCPI and IEEE 488.2.
- Review all relevant volumes of SCPI 1999.0, not Volume 1 alone.
- Verify every normative requirement and clause citation against the official complete documents.
- Determine which instrument-class requirements apply to the generic engine versus a particular instrument implementation.
- Determine the applicability of IEEE interface-dependent service-request behavior to the selected transports.
- Define the required parameter, response, and binary data formats.
- Resolve numeric resource limits and error-queue capacity as permitted by the selected standards.
- Add source-code paths and actual automated test names as implementation proceeds.

Until those items are resolved, this is an initial traceability and planning matrix, not evidence of compliance.
