# Third-party attribution and integration matrix

NLX Credential Suite code is GPLv3 under the firmware repository's top-level `LICENSE`. The Suite does not absorb the applications below into one binary. It uses firmware APIs or launches their existing FAPs so each project remains independently maintainable and retains its license, history, and attribution.

“Undeclared locally” means no standalone `LICENSE` was present in that app subtree during this inventory. It does **not** mean public domain. Such code is handoff-only; no source is copied into NLX Credential Suite until its license is verified.

| Application/component | Coverage | License evidence in this checkout | NLX integration |
|---|---|---|---|
| Flipper/Nuerolynx built-in NFC | ISO14443-A/B, ISO15693, FeliCa, MIFARE families | Firmware top-level GPL-3.0 | Shared public firmware APIs; direct handoff |
| Flipper/Nuerolynx built-in LF RFID | H10301, EM4100, Indala and other registered LF protocols | Firmware top-level GPL-3.0 | Shared protocol dictionary; direct handoff |
| Flipper/Nuerolynx built-in iButton | Registered 1-Wire/contact protocols | Firmware top-level GPL-3.0 | Shared iButton APIs; direct handoff |
| Picopass (`bettse/picopass`) | HID iCLASS/Picopass and PACS/Wiegand tools | No app-root license in subtree; embedded loclass files carry GNU GPL notices and retain them | Passive protocol probe in NLX; independent FAP handoff for deep work |
| MIFARE Classic Editor | MIFARE Classic `.nfc` file inspection/editing | App-local GPL-3.0 `LICENSE` | Independent FAP handoff; recommended for MIFARE Classic |
| MFKey (`noproto/FlipperMfkey` lineage) | MIFARE Classic key recovery | Upstream project identifies GPL-3.0; no app-root license copied into this subtree | Authorized Lab handoff only |
| NFC APDU Runner | Scripted ISO14443-4/ISO7816 APDUs | Undeclared locally; upstream URL retained in `application.fam` | Authorized Lab handoff only |
| NFC Magic | Magic-card operations | Undeclared locally; subtree origin retained in `.gitsubtree` | Authorized Lab handoff only |
| Metroflip | Transit/card parsers | App-local GPL-3.0 `LICENSE` | Independent parser/tool handoff |
| Seader | iCLASS SE/SEOS/DESFire with supported expansion hardware; Wiegand plugin | App-local GPL-3.0 `LICENSE` | Hardware-preflight handoff; code remains separate |
| Seos Compatible | SEOS reader/emulator | App-local AGPL-3.0 `LICENSE` | Authorized Lab, hardware-preflight, separate-process handoff only |
| ISO 15693-3 NFC Writer | ISO15693 writing | App-local GPL-3.0 `LICENSE` | Authorized Lab handoff only |
| iButton Converter | Cyfral/Metakom/Dallas conversion | App-local GPL-3.0 `LICENSE` | Inventoried; separate application |
| Multi Fuzzer (iButton/RFID) | Reader fuzzing | App-local MIT `LICENSE` | Authorized Lab launch handoff; not linked or copied |
| Mifare Fuzzer | MIFARE UID/card emulation testing | Undeclared locally; upstream URL retained | Authorized Lab launch handoff; not linked or copied |
| UL-C Bruteforce / Relay / ULCFKey | Ultralight-C authentication research | Undeclared locally; xero-firmware subtree origin retained | Authorized tooling; not linked or copied |
| T5577 Multiwriter | Multi-key T5577 writing | Undeclared locally; upstream URL retained | Authorized tooling; not linked or copied |
| T5577 Raw Writer | Raw T5577 writing | App-local GPL-3.0 `LICENSE` | Authorized tooling; not linked or copied |
| Flipper Wedge | NFC/RFID UID-to-HID output | App-local MIT `LICENSE` | Compatible reporting/export tool; independent FAP |
| NFC Maker | NDEF file creation | App-local GPL-3.0 `LICENSE` | Independent FAP; unrelated to passive scan path |
| NFC Playlist | Batch NFC emulation | Undeclared locally; upstream URL retained | Authorized tooling; not linked or copied |
| NFC Login | NFC-based HID login | App-local GPL-3.0 `LICENSE` | Independent FAP |
| NFC-Eink | NFC e-ink tag operation | App-local GPL-3.0 `LICENSE` | Independent FAP |
| AmiTool | NTAG215 tooling | App-local GPL-3.0 `LICENSE` | Independent FAP |
| YRM100 UHF RFID | UHF via YRM100 module | Undeclared locally; upstream URL retained | Hardware-required inventory entry; outside Universal Scan |
| Simultaneous UHF RFID Reader | UHF via ThingMagic/YRM modules | Undeclared locally; upstream URL retained | Hardware-required inventory entry; outside Universal Scan |
| NFC/RFID Detector | Detects energized readers, not credentials | Undeclared locally; good-faps subtree origin retained | Passive Audit & Diagnostics handoff; not used as a credential scan engine |

## NLX Credential Risk Review

No application named “Flipper Access Audit” is present in this checkout, so NLX does not
claim or rebrand that third-party project. Instead, the Suite includes its own GPLv3,
rule-based **Credential Risk Review**. It reports an exposure estimate derived from the
technology family, observed encryption/authentication state, read completeness, confidence,
and format ambiguity. It shows the contributing observations, marks weak evidence
provisional, and explicitly states that the result is a screening estimate rather than a
vulnerability verdict.

## Wiegand format attribution

The NLX decoder's Corporate 1000 field/parity layout is based on the public implementation already carried by the separately licensed Picopass/Seader source trees, which in turn attribute the Proxmark3 project. The NLX implementation is independently structured as a compact, table-driven decoder and retains this attribution. Generic layouts are explicitly labeled generic and never presented as proprietary certainty.

## Release rule

Before adding source-level integration for an app marked “Undeclared locally,” locate and preserve the exact upstream license and copyright notice. A launch handoff may remain, because it does not copy that application's code into the NLX binary.
