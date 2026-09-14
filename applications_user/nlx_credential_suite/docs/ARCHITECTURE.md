# Architecture

## Design goals

NLX Credential Suite is a coordinator, normalized inspection model, and tool registry—not a monolithic copy of every credential application. It uses firmware services directly for shallow, passive classification and hands deeper work to the existing specialist application that already owns that protocol.

## Runtime flow

1. The GUI scene starts `NlxScanCoordinator` on a dedicated worker thread.
2. `NlxScanSchedule` advances through NFC, Picopass, LF RFID, and an iButton contact check.
3. Only one radio/contact worker owns hardware at a time. Each worker is stopped and freed before the next begins.
4. An HF detection is retained through the LF pass. When both bands respond, two
   normalized `NlxCredentialResult` components are returned as one dual-technology
   inspection; otherwise the single detected component is returned normally.
5. Format candidates are parity-checked, scored, and retained when ambiguous.
6. The GUI receives only status/result events; no scan blocks the GUI thread.
7. Saving creates timestamped text, JSON, and optional raw files using temporary files plus atomic rename. Dual-technology reports contain both components and retain separate raw captures when available.
8. `NlxToolRegistry` launches built-in or separately bundled tools without copying their code into this application.
9. The result screen exposes Save, Emulate, and Write. Save preserves the inspection and
   opens the technology-specific reader for a complete native capture. Active Emulate or
   Write operations require an authorization acknowledgment. A dual result asks which
   detected component to use before any reader handoff.
10. Audit & Diagnostics exposes the separately bundled HF/LF reader-field detector and
    a local, rule-based exposure review. The review consumes only normalized scan data,
    identifies the observations affecting its score, and marks incomplete, low-confidence,
    or ambiguous results provisional.

## Modules

- `core/nlx_credential_result.*`: transport-neutral normalized result and candidate model.
- `core/nlx_format_decoder.*`: table-driven bit-field extraction, parity validation, scoring, and ambiguity handling.
- `core/nlx_report.*`: bounds-checked, JSON-escaped machine and human report serialization.
- `core/nlx_scan_schedule.*`: platform-neutral scan state machine used by device code and host tests.
- `core/nlx_risk_assessment.*`: deterministic exposure scoring with explicit evidence flags.
- `scanner/nlx_scan_coordinator.*`: Flipper NFC/LF/iButton resource ownership and cancellation.
- `parsers/nlx_parser_registry.*`: extension registry for native readers, adapters, and handoffs.
- `handoff/nlx_tool_registry.*`: independently maintained application launch targets and safety metadata.
- `storage/nlx_inspection_store.*`: collision-safe file naming and atomic persistence.
- `scenes/*`: native SceneManager/ViewDispatcher UI.

## Resource and cancellation rules

Cancellation is latched in coordinator state and also signaled through an event flag to wake timed waits. It cannot be lost when a wait consumes a flag. The scene joins the worker before application teardown. NFC, Picopass, LF RFID, and iButton objects are never active simultaneously.

Event-wait return values are checked for the RTOS error bit before signal bits are inspected, so timeout and resource errors cannot be mistaken for credential detections. NFC scanner allocations are explicitly zero-initialized before their state machine starts.

## Safety boundary

Universal Scan is read-only and shallow. Authorized Lab contains explicit handoffs to tools capable of recovery, scripting, writing, or special hardware operation. The Suite itself never automatically writes or emulates a credential.

MFKey/MFKey32, NFC Magic, RFID/MIFARE/iButton fuzzers, APDU Runner, ISO15693 Writer,
Seader, and SEOS are exposed only after the Authorized Lab acknowledgment. The HF/LF
Field Detector remains in passive diagnostics because it detects energized reader fields
rather than transmitting credential data.

The **Read then Emulate** and **Read then Copy/Write** entries select the correct native reader for the last detected technology. The specialist reader performs the required full capture and presents its own deliberate confirmation flow; the shallow normalized result is never treated as sufficient write material.
