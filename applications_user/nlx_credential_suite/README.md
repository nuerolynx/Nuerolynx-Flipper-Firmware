# NLX Credential Suite

NLX Credential Suite is the passive, unified physical-access credential inspector for Nuerolynx Firmware v1.1. It provides one battery-aware scan flow for NFC/smart cards, HID iCLASS/Picopass, 125-kHz LF RFID, and contact iButton credentials. It preserves the firmware's existing readers and third-party applications as independently maintained tools.

The default scan path never writes, emulates, fuzzes, or recovers keys. Those actions are available only through explicit handoffs in **Authorized Lab**, after an authorization acknowledgment.

## Current capabilities

- Native 128x64 application shell and approved NLX icon.
- Sequential, cancel-safe **Read Credential** worker that cleanly releases each radio before the next technology starts.
- Dual-technology detection: an HF hit is retained while the same scan checks other HF
  protocol branches and completes its 125-kHz pass. This covers common HF+LF cards and
  separable HF+HF combinations such as MIFARE plus Picopass/iCLASS.
- Shallow NFC family and UID/CSN identification.
- Passive Picopass/iCLASS classification without authentication or key recovery.
- Automatic LF protocol classification using the firmware's complete LF protocol dictionary.
- HID Generic frame extraction feeding the 26/32/34/35/36/37/40-bit decoder when the captured layout is supported.
- Contact iButton identification.
- Table-driven Wiegand decoder with parity scoring, ambiguity reporting, and 26/32/34/35/36/37/40-bit candidates.
- Atomic `.json`, `.txt`, and optional `.raw` inspection records on the SD card. A
  dual-technology card is saved as one combined inspection with both components.
- Saved-report browser and optional per-session site/job label.
- Registry-based handoffs to built-in and bundled specialist tools.
- **Audit & Diagnostics** with a passive HF/LF reader-field detector, live tool
  availability checks, and a transparent rule-based credential exposure review. The
  review marks shallow or ambiguous evidence provisional and never presents its score
  as a vulnerability verdict.
- Authorized Lab handoffs for MFKey/MFKey32, NFC Magic, Seader/SEOS, RFID Fuzzer,
  MIFARE Fuzzer, iButton Fuzzer, APDU Runner, and ISO15693 Writer. Each tool remains a
  separately maintained application with its own confirmation flow and license.
- Result-screen **Save**, **Emulate**, and **Write** actions. Save first preserves the
  normalized inspection and then opens the matching specialist reader for a complete,
  emulatable native capture. Emulate/Write require the Authorized Lab acknowledgment,
  then hand off directly to the matching specialist engine for a full capture and
  deliberate operation.
- Dual-technology Emulate/Write handoffs first ask which detected component to open.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/SUPPORTED_CREDENTIALS.md](docs/SUPPORTED_CREDENTIALS.md), and [docs/BUILD_INSTALL.md](docs/BUILD_INSTALL.md).

## License

New NLX Credential Suite code is distributed under the repository's GPLv3 license. Existing firmware and third-party applications retain their original copyrights and licenses. See [docs/THIRD_PARTY_LICENSES.md](docs/THIRD_PARTY_LICENSES.md).
