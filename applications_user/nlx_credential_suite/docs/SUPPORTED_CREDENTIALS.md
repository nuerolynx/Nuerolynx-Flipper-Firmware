# Supported credential matrix

“Classify” means the Suite can identify the family during Universal Scan. “Identifier” means it can retrieve a UID/CSN/serial or native LF payload without authentication. “Deep tool” names the existing tool used for further authorized work.

| Family | Classify | Identifier | Credential fields | Deep tool |
|---|---:|---:|---:|---|
| MIFARE Classic Mini/1K/4K | Yes | Yes | When exposed by a compatible parser | Built-in NFC / MFC Editor |
| MIFARE Ultralight and NTAG | Yes | Yes | Parser-dependent | Built-in NFC |
| MIFARE Plus | Yes | Yes | Security-level dependent | Built-in NFC |
| MIFARE DESFire | Yes | Yes | Application/authentication dependent | Built-in NFC / APDU Runner |
| ISO14443-A | Yes | Yes | Protocol-dependent | Built-in NFC |
| ISO14443-B | Yes | Yes | Protocol-dependent | Built-in NFC |
| ISO15693 / SLIX | Yes | Yes | Protocol-dependent | Built-in NFC |
| FeliCa | Where supported by firmware | Yes | Parser-dependent | Built-in NFC |
| HID Prox H10301 | Yes | Yes | 26-bit facility/card, parity | Built-in LF RFID |
| EM4100 | Yes | Yes | Serial only | Built-in LF RFID |
| Indala | Yes | Yes | Protocol payload | Built-in LF RFID |
| Other firmware LF protocols | Yes | Yes | Protocol-dependent | Built-in LF RFID |
| HID iCLASS/Picopass | Passive presence classification | Not in shallow scan | Specialist tool-dependent | Picopass / Seader |
| Supported iButton/1-Wire | Contact required | Yes | Protocol-dependent | Built-in iButton |

## Bit-format candidates

The normalized decoder currently evaluates:

- HID H10301 26-bit with verified even/odd parity.
- Two explicitly generic 32-bit layouts (8/24 and 12/20); both are shown when ambiguous.
- HID H10306 34-bit with verified even/odd parity.
- HID Corporate 1000 35-bit standard layout with three interleaved parity checks.
- Generic 36-bit 18/16 layout, labeled generic because multiple incompatible 36-bit formats exist.
- HID H10302 and H10304 37-bit with verified parity; competing valid interpretations remain visible.
- Two explicitly generic 40-bit layouts (8/32 and 16/24); both remain visible when ambiguous.

Bit length alone is never treated as proof of a field layout. Generic candidates receive lower confidence. Invalid parity reduces, rather than increases, confidence.
