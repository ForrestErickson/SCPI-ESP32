# Program-Message and Header Requirements

**Status:** Detailed working requirements derived from SCPI 1999.0 Volume 1, *Syntax and Style*. These requirements are not yet a complete conformance specification. IEEE 488.2-1992 remains the approved working reference; exact IEEE clause details and the relationship to IEEE/IEC 60488-2-2004 remain under review.

## 1. Scope of this review

This document covers the parser's first semantic layer:

- Program-message units and compound program messages.
- Instrument-control program headers and common-command headers.
- Keyword case, short/long forms, numeric suffixes, and header length.
- Hierarchical paths and SCPI command-tree traversal.
- Query recognition and the query behavior relevant to parsing/dispatch.

It does not yet settle transport framing, message terminators, the complete IEEE 488.2 message grammar, parameter grammar, response termination, or all required commands. Those are separate work items.

## 2. Normative source map

The primary source is **SCPI 1999.0 Volume 1: Syntax and Style**:

- §6, Program Headers: header types and syntax.
- §6.2.1, Mnemonic Generation Rules: case, exact forms, short-form generation, header length, numeric suffixes.
- §6.2.2, Building the Command Tree: hierarchical organization and naming constraints.
- §6.2.3, Queries: query marker, response behavior, side effects, and command/query availability.
- §6.2.4, Traversal of the Header Tree: compound-message path state and examples.
- §7, Parameters: parameter syntax is based on IEEE 488.2 §7.7 with SCPI-specific restrictions; detailed review is deferred.

The official hosted copy is linked in [the IVI Foundation SCPI-99 download](https://www.ivifoundation.org/downloads/SCPI/scpi-99.pdf). Page references in that PDF include §6.2.1 at printed pages 6-1 to 6-2 and §6.2.3–§6.2.4 at printed pages 6-6 to 6-7.

## 3. Testable parser requirements

### PM-001 — Program-message composition

A program message may contain one or more program-message units. Multiple units in one message are separated by semicolons. The parser shall recognize the boundary between units without treating a semicolon inside parameter data as a separator unless the applicable data grammar permits it.

**Tests**
- One-unit message dispatches once.
- Two and three valid units separated by semicolons dispatch in order.
- Empty units, leading/trailing separators, repeated separators, and malformed unit boundaries are tested against the applicable IEEE grammar before final expected error classification is frozen.
- A failure in one unit does not corrupt parser state for the next complete message; exact continuation/error semantics require the IEEE message/error review.

### HDR-001 — Case-insensitive headers

Instrument-control headers shall be accepted without distinguishing uppercase and lowercase letters.

**Tests:** send each registered path in uppercase, lowercase, and mixed case; all forms resolve to the same registered command.

### HDR-002 — Exact short and long keyword forms

Each instrument-control keyword has a short and long form. The parser shall accept the exact short form and exact long form, and shall reject other partial forms.

For a single-word mnemonic longer than four characters, the short form is normally its first four characters. If the fourth character is a vowel, the short form is the first three characters. For phrase-derived mnemonics, the long form is formed from the first letter of each word followed by the entire last word; apply the short-form rule to that generated mnemonic.

**Tests:** for every registered keyword, test exact short and long forms; test at least one too-short prefix and one intermediate prefix that is neither accepted form. Include colliding short forms as a registry-definition error.

### HDR-003 — Header length and numeric suffix

The SCPI text states that IEEE 488.2 limits a header to 12 characters, including a numeric suffix. Instrument-control command headers may carry a numeric suffix; the suffix applies to both short and long forms. Where a command supports multiple identical capabilities, an omitted suffix denotes capability 1. A single-capability instrument is not required to accept a suffix.

**Tests:** verify header-length boundaries including suffix digits; verify suffix parsing separately from the mnemonic; verify default capability 1 only for command definitions that declare this behavior. Confirm exact length accounting against IEEE 488.2 before freezing boundary tests.

### HDR-004 — Hierarchical command tree

Instrument-control commands are organized as a hierarchy of keywords separated by colons. A complete command path must be unique in the applicable SCPI command set. At each tree level, a keyword must be distinguishable from other keywords at that level and any relevant default-node interactions.

**Tests:** resolve valid multi-level paths; reject unknown nodes, missing path elements, extra colons, and ambiguous registry definitions. Test any optional/default nodes only after their semantics are explicitly modeled.

### HDR-005 — Compound-message command-tree traversal

For a compound program message:
1. The first command header is referenced to the root.
2. Each subsequent command header is referenced to the same tree level as the preceding command header in that message, as described by SCPI §6.2.4.
3. A leading colon on a subsequent instrument-control header makes that header root-relative.
4. A sibling command may therefore follow a prior leaf command without repeating the common parent path.
5. The parser shall not implement the enhanced tree-walking behavior that SCPI §6.2.4 explicitly says not to implement.
6. Default nodes shall not silently alter the parser's retained header path.

The SCPI examples include the following pattern: `FREQ:STAR 3;STOP 5` addresses sibling nodes under FREQuency, while `:FREQ:STOP 5` explicitly starts again from the root. A command at an unrelated root branch without a leading colon is not interpreted as a fresh root path.

**Tests**
- `FREQ:STAR 3;STOP 5` resolves to `FREQ:STAR` then `FREQ:STOP`.
- `FREQ:STAR 3;:FREQ:STOP 5` resolves both paths from root.
- A non-sibling path without a leading colon is rejected when it is not a child/sibling at the retained level.
- Explicitly rooted paths work after any preceding subsystem path.
- Default-node examples from §6.2.4 are tested to ensure defaults do not change retained traversal state.
- Add the SCPI §6.2.4 examples as parser regression tests, preserving the standard's distinction between shall and may.

### HDR-006 — Common-command header separation

Common commands and queries use the IEEE 488.2 common-command header syntax and are distinct from instrument-control headers in the subsystem command tree. They shall be recognized and dispatched through the common-command path, not as ordinary tree keywords.

**Tests:** parse representative common commands and queries alongside instrument-control commands; verify they do not inherit or modify instrument-control tree path state. Validate exact common-command syntax and permitted forms against IEEE 488.2.

### QRY-001 — Query marker

A query form is created by appending `?` to the final keyword of the command header. Not every command necessarily has both forms: SCPI allows command-only/event forms and query-only forms when documented.

**Tests:** recognize valid query headers; reject misplaced or repeated question marks; verify registry metadata permits command-only, query-only, or both forms.

### QRY-002 — Query response and side effects

For ordinary setting queries, the current setting is placed in the output buffer; the response does not include the command header; query execution does not change instrument settings or couplings. SCPI §6.2.3 explicitly notes an exception for the MEASurement subsystem, where settings may change as a result of measurement. Numeric query results are returned in fundamental units unless otherwise specified or requested.

**Tests:** assert exact response data without a header; snapshot settings/couplings before and after ordinary queries; separately test any measurement query with its documented effects; verify units and response formatting per command definition.

## 4. Project parser design implications

These are implementation choices, not direct quotations from the standard:

- Keep the registered command tree independent of any specific instrument.
- Resolve each keyword against exact short/long aliases, using case-insensitive comparison.
- Detect ambiguous short forms when a command tree is registered, rather than resolving ambiguity unpredictably at runtime.
- Maintain explicit parser path state for compound messages. Do not implement traversal as a simple string-prefix concatenation.
- Keep common-command recognition separate from instrument-control tree traversal.
- Preserve bounded input and token storage; limits must be documented and must not contradict applicable standard limits.
- Separate a complete program message delivered by the transport from the parser's interpretation of units within that message.

## 5. Open questions and deferred requirements

1. Confirm the precise IEEE 488.2-1992 message grammar, whitespace, separators, terminators, and response-message rules.
2. Determine whether the transport API supplies complete program messages or whether the engine itself owns input framing.
3. Confirm exact 12-character header-length accounting and error behavior from IEEE 488.2.
4. Review numeric suffix grammar, default-node rules, and all relevant IEEE syntax productions.
5. Review all four SCPI-99 volumes and identify the required command families applicable to the eventual instrument/conformance target.
6. Compare the relevant clauses of IEEE 488.2-1992 with IEEE/IEC 60488-2-2004 before changing the working baseline.

No implementation or test is claimed complete by this document. Each requirement remains **Not started** until code and its associated automated tests pass.
