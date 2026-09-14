# Test procedure

## Host tests

From the firmware repository root on this Windows workspace:

```bat
applications_user\nlx_credential_suite\tests\host\run_msvc.cmd
```

The test executable compiles the production core sources with MSVC `/W4 /WX` and verifies:

- H10301 26-bit decoding and parity rejection.
- Corporate 1000 35-bit field extraction and interleaved parity.
- 32-bit and 37-bit ambiguity behavior.
- Multiple normalized format candidates.
- JSON escaping, missing-value representation, and truncation failure.
- Human-readable serialization.
- Scan timeout progression, cancellation latching, and terminal states.
- Authorized/hardware handoff metadata and result-to-tool recommendations.
- Deterministic exposure scoring, evidence flags, and provisional-result handling.

## Build smoke test

```powershell
.\fbt.cmd fap_nlx_credential_suite
```

Expected artifact: `build\f7-firmware-C\.extapps\nlx_credential_suite.fap` with successful `SDKCHK`, `APPCHK`, and no NLX compilation warnings.

## Device smoke checklist

1. Launch NLX Credential Suite and verify all ten main-menu items scroll without clipping.
2. Start Universal Scan with no credential; confirm the stage cycles and Back returns promptly.
3. Present one supported NFC card in each stage timing position; verify classification and UID.
4. Present H10301 LF credential; verify facility code, card number, and valid parity.
5. Touch a supported iButton; verify contact classification.
6. Save a result; verify paired `.json` and `.txt` plus `.raw` when bytes were captured.
7. Open the text report from Saved Inspections.
8. Remove a registered external FAP temporarily; confirm the Suite shows “Tool unavailable.”
9. Select Seader/SEOS; confirm the hardware preflight appears.
10. Confirm no normal scan writes, emulates, fuzzes, or recovers a key.
11. Open Audit & Diagnostics, run Credential Risk Review on the last scan, and confirm
    the score, evidence flags, and provisional label match the normalized result.
12. Open Tool Availability and verify MFKey, NFC Magic, Picopass, Seader, detector, and
    fuzzer handoffs are reported ready on a complete updater installation.

Hardware-dependent tests require manual device validation and are not claimed complete until this checklist is run on the target device.
