# Hardware requirements

Universal Scan uses only the Flipper Zero's built-in NFC, 125-kHz RFID, and iButton hardware.

Some separately bundled tools need additional hardware:

- **Seader / SEOS:** compatible SAM or serial expansion hardware may be required for the selected operation. The Suite displays a hardware preflight before launch. The specialist tool owns final capability detection because it exclusively controls the expansion/UART resource.
- **UHF RFID applications:** require their named external modules (for example YRM100 or supported ThingMagic modules). UHF is inventoried but is not part of the built-in Universal Scan cycle.
- **Wiegand electrical diagnostics:** require a correctly level-shifted, protected interface. Never connect field wiring directly without verifying voltage, ground reference, and isolation.

Missing hardware must not crash Universal Scan. It is reported by the specialist tool or explained before handoff.
