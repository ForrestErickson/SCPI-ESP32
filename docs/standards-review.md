# SCPI Standards Review — Initial Findings

**Status:** Working research notes; not a compliance declaration. The project goal of eventual SCPI conformance, developed incrementally with every implemented feature tested, has been approved.

## 1. Reference documents

The principal public syntax/style reference located for this project is **SCPI 1999.0, Volume 1: Syntax and Style**, published May 1999 by the SCPI Consortium. The IVI Foundation hosts a copy:

https://www.ivifoundation.org/downloads/SCPI/scpi-99.pdf

The related reference is **IEEE 488.2-1992**, *Standard Codes, Formats, Protocols, and Common Commands for Use With IEEE Std 488.1-1987*. IEEE's catalogue currently identifies this edition as Inactive-Reserved:

https://standards.ieee.org/ieee/488.2/718/

SCPI 1999.0 is a four-volume standard. Volume 1 alone is not the complete SCPI specification; the Command Reference, Data Interchange Format, and Instrument Classes volumes also form part of the set. We must not treat a review of Volume 1 as a complete standards review.

SCPI 1999.0 references IEEE 488.2-1992. The exact standards editions and compliance target for this project remain to be agreed. The IEEE catalogue status should be considered when selecting and describing the target; it does not by itself establish what later edition, if any, should replace it for our purposes.

## 2. Syntax engine versus instrument command set

SCPI provides rules for command structure, messages, parameters, responses, and standardized instrument command families. An application-specific command invented by an instrument designer is not automatically a command defined by SCPI.

The proposed HEARTBEAT family is an application-specific extension for this project. It can use SCPI-style instrument-control headers and parameter conventions, but it must be documented as a project-defined command family unless a corresponding standardized command is identified in the complete SCPI Command Reference.

The generic engine should separate:
- Parsing and dispatching the SCPI language constructs supported by the engine.
- The registry of command paths and handlers supplied by an instrument/application.
- The definition and implementation of each instrument's command set.

## 3. Verified SCPI 1999.0 findings

The following findings are supported by Volume 1, Syntax and Style:

1. **Case-insensitive headers.** Instruments accept upper- and lowercase characters without distinguishing case.
2. **Exact short or long keyword forms.** A keyword is accepted in its exact short form or exact long form; arbitrary partial abbreviations are not valid.
3. **Short-form generation.** The short form is generally the first four characters of the long form, except that when the fourth character is a vowel and the word is longer than four characters, the short form is the first three characters.
4. **Header length.** IEEE 488.2 limits a header to 12 characters, including any numeric suffix.
5. **Hierarchical command tree.** Instrument-control commands are organized in a tree, with colon-separated header keywords.
6. **Queries.** A query is formed by appending ? to the final keyword. Query responses do not include the command header, and a query should not change instrument settings.
7. **Compound messages.** A program message can contain multiple program message units separated by semicolons. Subsequent command headers are interpreted relative to the preceding header's tree level according to SCPI traversal rules.
8. **Common commands.** IEEE 488.2 common commands begin with * and are distinct from instrument-control subsystem commands.
9. **SCPI command-set requirements.** SCPI 1999.0 lists mandatory IEEE 488.2 common commands and SCPI-required commands. The exact applicable list and their required behavior must be extracted and reviewed before implementation claims are made.
10. **Compliance is broader than syntax.** SCPI conformance includes applicable IEEE 488.2 requirements and required SCPI commands and behaviors, not merely accepting familiar command spellings.

SCPI 1999.0 section 4.1.1 lists these IEEE-mandated common commands:

| Command | Function |
|---|---|
| *CLS | Clear status |
| *ESE | Standard Event Status Enable |
| *ESE? | Query Standard Event Status Enable |
| *ESR? | Query Standard Event Status Register |
| *IDN? | Identification query |
| *OPC | Operation Complete |
| *OPC? | Operation Complete query |
| *RST | Reset |
| *SRE | Service Request Enable |
| *SRE? | Query Service Request Enable |
| *STB? | Read Status Byte |
| *TST? | Self-test query |
| *WAI | Wait-to-Continue |

This is the list stated in SCPI 1999.0 Volume 1, section 4.1.1. It is not yet the project's implementation commitment. Some commands have implications for status structures, output queues, synchronization, and pending operations; they cannot safely be implemented as superficial name-only handlers.

SCPI 1999.0 also requires compound command/query program-header parsing for tree-structured commands. It specifies how the parser's current tree level is handled across semicolon-separated units. This behavior needs explicit parser tests.

## 4. HEARTBEAT keyword spelling and abbreviation review

SCPI notation conventionally uses uppercase letters to show the short form and lowercase trailing letters to complete the long form. Applying the SCPI 1999.0 short-form rule to the proposed single-word keyword HEARTBEAT gives:

- Long form: HEARTBEAT
- Short form: HEAR

The fourth character is R, not a vowel, so the first four characters are retained. The mixed-case notation should therefore be written HEARtbeat, not HEARTbeat, if we intend to show the standard short-form portion.

The other proposed keywords are:
- STATe: short form STAT.
- PERiod: short form PER, because the fourth character I is a vowel and the word is longer than four characters.
- DUTYcycle: proposed short form DUTY; this should be checked carefully against the SCPI mnemonic rule and documented consistently.

The command family, using uppercase letters to show the short-form portion, is proposed as follows:

    HEARtbeat:STATe ON
    HEARtbeat:STATe OFF
    HEARtbeat:STATe?
    HEARtbeat:PERiod <seconds>
    HEARtbeat:PERiod?
    HEARtbeat:DUTYcycle <percent>
    HEARtbeat:DUTYcycle?

This is a project-defined subsystem, not a claim that HEARTBEAT is a standardized SCPI subsystem. Before finalizing the names, review the complete SCPI Command Reference for naming collisions or an existing standardized command that already controls the same function.

## 5. Proposed initial parser scope

The following is a proposed staged implementation scope, not a claim of full SCPI compliance.

### First parser milestone

- Case-insensitive command headers.
- Exact long and short keyword matching; reject incomplete or over-abbreviated forms.
- A registered hierarchical command tree.
- Root-relative first command header and SCPI-defined relative traversal for later semicolon-separated commands.
- Command/query distinction using the final question mark.
- One or more program message units separated by semicolons.
- Parameter separation and parsing sufficient for the first application commands.
- Defined handling of whitespace, message termination, malformed headers, unknown commands, and invalid parameters.
- Bounded message and token storage with explicit overflow errors.

### Deliberately not assumed for the first milestone

- Full IEEE 488.2 status reporting, service request, and synchronization behavior.
- Arbitrary numeric expressions.
- Definite- and indefinite-length binary blocks and all waveform/data-transfer facilities.
- Full SCPI Data Interchange Format support.
- Every standardized SCPI command subsystem or instrument class.
- Full compliance with all SCPI 1999.0 and IEEE 488.2 requirements.

These exclusions are temporary staging boundaries, not permanent exclusions from the conformance goal. Each feature added must have tests. The project must not claim full conformance until the applicable requirements have been implemented and verified.

## 6. Decisions still needed

1. Select and document the target SCPI and IEEE 488.2 editions. The project goal is settled: eventual conformance, developed incrementally, with every implemented feature tested.
2. Review the complete SCPI 1999.0 Command Reference to identify the required SCPI command families and any naming collision relevant to HEARTBEAT.
3. Decide whether the initial parser milestone includes all common-command parsing and execution or only the message/header machinery needed to grow toward that support.
4. Define the initial message termination/framing contract independently of the physical transport.
5. Define numeric and character parameter grammar and response formatting for the supported subset.
6. Create a requirement-to-test compliance checklist before making conformance claims.

## 7. Source locations used

- SCPI 1999.0 Volume 1, sections 1.1–1.4: https://www.ivifoundation.org/downloads/SCPI/scpi-99.pdf
- SCPI 1999.0 Volume 1, section 4.1.1: mandatory IEEE common-command list.
- SCPI 1999.0 Volume 1, section 4.2: SCPI-required and optional commands and documentation requirements.
- SCPI 1999.0 Volume 1, sections 5.1 and 6.2.1: notation and mnemonic rules.
- SCPI 1999.0 Volume 1, sections 6.2.3–6.2.4: queries and command-tree traversal.
- IEEE 488.2-1992 catalogue record: https://standards.ieee.org/ieee/488.2/718/

These notes are a working review. The official standards remain the controlling references, and interpretations should be checked against the complete applicable documents.


## 8. Approved project conformance strategy

**Decision:** The project will pursue eventual SCPI conformance, developed incrementally, with every implemented feature tested.

Consequences:

- Build the implementation in small, reviewable stages rather than attempting the entire standards set at once.
- Maintain a traceable checklist of applicable SCPI and IEEE 488.2 requirements.
- Add tests with each feature, including positive, negative, boundary, and interaction cases where applicable.
- Record implementation status for each requirement: not started, in progress, implemented and tested, or not applicable with rationale.
- Document temporary omissions and known deviations explicitly.
- Do not claim full SCPI conformance until all applicable requirements for the selected standards editions have been implemented and verified.
- Revisit the target editions before freezing the compliance checklist; SCPI 1999.0 Volume 1 and IEEE 488.2-1992 are the initial research references, not yet a final edition-selection decision.

The first practical deliverable following this decision should be a requirement-to-test compliance matrix, followed by a staged implementation plan that starts with message parsing and command-tree behavior and grows toward the required common commands, status model, and other applicable SCPI features.


## 9. Standards catalogue review — 2026-11-06

A current review of the standards-maintainer and standards-publisher catalogues adds the following information:

1. **SCPI publication source.** The IVI Foundation identifies itself as the maintainer of the SCPI specifications and its current specification-download page lists **SCPI-99** as the available SCPI document. The SCPI overview says SCPI is built on IEEE 488.2 and requires IEEE 488.2 conformance, while remaining a hardware-independent software language.
2. **SCPI-99 remains the available published SCPI reference.** The current IVI download page lists SCPI-99; no later SCPI edition was identified in that catalogue review. This is evidence about the currently published IVI listing, not proof that no unpublished maintenance material or errata exists.
3. **IEEE 488.2-1992 status.** IEEE's catalogue page for IEEE 488.2-1992 identifies it as an Inactive-Reserved Standard.
4. **Related international publication.** IEEE's catalogue lists **IEEE/IEC 60488-2-2004**, *Standard Digital Interface for Programmable Instrumentation — Part 2: Codes, formats, protocols and common commands*, as an Active Standard. It covers codes, formats, protocols, and common commands for programmable instrumentation. It is a closely related international publication and a candidate reference for the IEEE 488.2 foundation, but its exact normative relationship and technical equivalence to the edition cited by SCPI-99 must be confirmed from the complete documents before we declare it a replacement or substitute.

Catalogue sources reviewed:
- IVI Foundation, SCPI overview: https://www.ivifoundation.org/About-IVI/scpi.html
- IVI Foundation, current specification downloads (SCPI-99 listing): https://www.ivifoundation.org/specifications/default.html
- IEEE, IEEE 488.2-1992 catalogue status: https://standards.ieee.org/ieee/488.2/718/
- IEEE, IEEE/IEC 60488-2-2004 catalogue status and scope: https://standards.ieee.org/ieee/60488-2/3632/

### Recommended baseline for user decision

The best-supported working baseline at this stage is:

- **SCPI:** SCPI-99 (1999.0), with review of all four volumes and any available official errata.
- **IEEE 488.2 foundation:** retain IEEE 488.2-1992 as the edition explicitly referenced by SCPI-99, and investigate IEEE/IEC 60488-2-2004 as the current active international publication of the related codes/formats/protocols/common-command standard.

**User approval:** This working baseline was approved for the SCPI-ESP32 project on 2026-11-06.

The approved working references are SCPI-99 (1999.0) and the IEEE 488.2-1992 edition cited by SCPI-99. The project will also investigate IEEE/IEC 60488-2-2004. Approval of the working baseline does not assume that the 2004 publication is technically equivalent or that it replaces the 1992 reference. Compare their scope, forewords, normative references, and relevant technical clauses; record any differences that affect the project before making a final determination about that relationship.

For the project, the practical next action is to review the complete SCPI-99 set and the available IEEE/IEC 60488-2-2004 text, then produce a clause-level applicability map. Until that work is complete, the existing compliance matrix remains provisional.
