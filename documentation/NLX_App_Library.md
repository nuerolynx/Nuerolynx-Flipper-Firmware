# Nuerolynx Application Library

Nuerolynx organizes the on-device Applications browser into short category and
subcategory lists. This changes where FAP binaries are packaged on the SD card;
it does not combine apps, remove features, or modify third-party ownership and
licensing.

## Folder layout

```text
Apps/
  Games/
    Arcade/
    Board/
    Puzzle/
    Strategy-Sim/
  GPIO/
    Boards/
    Cameras/
    Debug/
    Diagnostics/
    ESP/
    FlipBoard/
    FlipperHTTP/
    GPS/
    MALVEKE/
    MAYHEM/
    Network/
    NRF24/
    Programming/
    Radio/
    Security/
    Sensors/
    Utilities/
    VGM/
  Infrared/
    Remotes/
    Tools/
  Media/
    Audio/
    Utilities/
    Visual/
  NFC/
    Credentials/
    Lab/
    Utilities/
  Sub-GHz/
    Analyze/
    Automate/
    Communicate/
    Lab/
  Tools/
    Calculate/
    Field/
    Security/
    System/
  Bluetooth/
  RFID/
  USB/
  iButton/
```

The smaller Bluetooth, RFID, USB, and iButton libraries remain flat because a
second level would add navigation without reducing scrolling. Hidden system
applications under `apps/assets` are unchanged.

Self-updating Flip apps remain at the root of their original category. Their
updaters use the historical location, so retaining that path prevents an update
from creating a duplicate entry.

## Data and compatibility

Only FAP placement changes. Application data remains under
`/ext/apps_data/<application-id>` and assets remain under
`/ext/apps_assets/<application-id>`, preserving settings and saved data.

Known Archive, NFC, desktop, and NLX Credential Suite handoffs use the new paths
directly. The loader also resolves a missing legacy `/ext/apps/.../*.fap` path
by its filename, preserving existing shortcuts and saved keybinds after the
reorganization. If a filename is ambiguous, the loader refuses to guess.

New apps added to an organized top-level category should be assigned in
`scripts/fbt/nlx_app_library.py`. Until classified, they are packaged in that
category's `Other` folder so they remain visible without lengthening the root.

## Smart Search

The main menu's **Smart Search** entry searches application display names,
filenames, category paths, and curated functional keywords. Results are ranked
on-device and launch directly. Common natural terms such as `wire gauge`,
`credential`, `logic analyzer`, and `Wi-Fi audit` lead to the corresponding
tools without requiring the user to know their folder.
