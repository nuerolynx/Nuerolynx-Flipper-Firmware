# Adding a parser or tool adapter

1. Add a stable entry to `parsers/nlx_parser_registry.c`. Choose `Native`, `Adapter`, or `Handoff` according to where protocol ownership remains.
2. Keep passive classification separate from authenticated/deep analysis.
3. Populate `NlxCredentialResult` fields only when observed or defensibly inferred. Set the matching `*_present` and `*_inferred` flags.
4. If several layouts fit, populate `candidates[]`, lower confidence, and expose every credible interpretation.
5. Add any independent application to `handoff/nlx_tool_registry.*`. Mark write/recovery/fuzz/emulation tools `authorized_lab_only` and mark expansion-dependent tools `external_hardware_may_be_required`.
6. Do not copy a third-party codebase when a shared firmware library, read-only adapter, or launch handoff is sufficient.
7. Record the source, license, protocol coverage, and integration type in `THIRD_PARTY_LICENSES.md`.
8. Add host tests for field extraction, valid/invalid parity, ambiguous inputs, and serialization.
9. Add device smoke cases for cancellation, resource release, missing FAPs, and hardware preflight.
10. Build the standalone FAP before the full updater package.

Parser callbacks must not block the GUI thread. Hardware adapters must stop and release their worker before the coordinator advances to another radio.
