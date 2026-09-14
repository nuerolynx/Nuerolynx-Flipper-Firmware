# Build and installation

## Prerequisites

- Nuerolynx Firmware source tree based on Momentum dev commit `d3f89dfe2`.
- Repository-managed `fbt` toolchain and initialized external-app content.
- Flipper Zero with a working SD card for saved inspection reports.

## Build the application

```powershell
.\fbt.cmd fap_nlx_credential_suite
```

## Build the updater package

```powershell
$env:DIST_SUFFIX = "Nuerolynx-v1.1-NLX-Credential-Suite"
$env:NLX_RELEASE_VERSION = "v1.1"
.\fbt.cmd --proxy-env NLX_RELEASE_VERSION dist_updater_package
```

`NLX_RELEASE_VERSION` sets the on-device firmware label to `Nuerolynx v1.1` for
an untagged local release build. It does not alter upstream attribution or tags.

Use the resulting `.tgz` with qFlipper or the supported web updater's **Install from file** flow. Installing a normal update package does not format the SD card. Nevertheless, back up `/ext` before any firmware update.

The Suite writes only beneath `/ext/apps_data/nlx_credential_suite/`. Existing NFC, LF RFID, iButton, asset-pack, settings, and database paths are not migrated, renamed, or deleted by this application.

## Safety

Building does not flash a device. Do not run a flash/deploy target and do not publish artifacts or push the branch without the owner's explicit approval.
