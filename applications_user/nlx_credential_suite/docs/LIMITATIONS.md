# Known limitations

- Universal Scan is a classifier, not a universal authentication bypass. Encrypted cards may expose only technology, family, and public identifier.
- Picopass/iCLASS shallow detection uses a passive ACTALL response and does not authenticate, recover keys, or read protected credential blocks.
- The initial release does not silently guess proprietary field layouts. Unknown 32/36/40-bit layouts are labeled generic and low/medium confidence.
- A raw Wiegand bitstream is not available from every firmware protocol decoder. Facility/card decoding is performed only when a compatible bitstream or verified native layout exists.
- The optional site/job label is per application session; reports keep the value after saving.
- The five-second goal covers the first NFC/Picopass/LF radio pass under normal conditions. Deep application parsing, authentication, and physical iButton contact can take longer.
- After detecting an HF component, the card must remain positioned through the following
  125-kHz window so a dual-technology LF component can also be identified. The extra
  confirmation pass can add up to approximately 1.75 seconds.
- HF+HF cards using distinguishable protocol branches (for example, MIFARE plus
  Picopass/iCLASS) can be reported as two components. Two ISO14443-A chips competing in
  the same RF field may not both be selected reliably by the Flipper hardware; a missing
  second response is not reported as certain absence.
- UHF, OSDP, and live Wiegand bus capture are not in the built-in scan cycle. They require external hardware and remain independent tools/extension points.
- A specialist application can be launched only if its FAP exists at the registered path. The Suite explains missing installations rather than failing silently.
