# UberGuidoZ integration audit

Audit date: 2026-09-13
Audited source: `UberGuidoZ/Flipper` at commit `41fc08dbc4c53cf2c87dd4f2de3d3a5fe3eb016f`

## Integrated applications

| Nuerolynx application | Upstream application | Upstream version | Capability | Data written |
| --- | --- | --- | --- | --- |
| NLX Logic Analyzer | GPIO Logic Analyzer | 2.01 | Passive eight-channel GPIO sampling and CSV capture | `/ext/gpio_analyzer/gla_*.csv` only when recording is started |
| NLX Meter Monitor | Smart Meter Monitor | 1.11 | Passive 433/868/915 MHz smart-meter activity and burst monitoring | None |

Both applications are built from source as native FAPs. The approved NLX small menu icon and concise NLX titles are applied to the launcher and splash screens. Original version, author, URL, source comments, README material and About-screen attribution remain intact.

The upstream repository is GPL-3.0. These integrations remain covered by the firmware's GPL-3.0 distribution and the corresponding source is included in this repository.

## Inventory decisions

The upstream `Applications/UberGuidoZ` section contains the two original source applications above. The broader repository is a community catalog containing many third-party submodules, data collections, payloads and precompiled FAP mirrors. Those items were evaluated but are not copied wholesale:

- Precompiled RogueMaster/Unleashed FAP collections are not imported. Nuerolynx already carries the maintained Momentum application set, and foreign binaries can be API-incompatible or duplicate existing tools.
- BadUSB payload dumps, credential-exfiltration scripts, reverse shells and destructive payloads are not added to the default firmware package.
- RF jammers and indiscriminate brute-force tooling are not bundled.
- Host-side utilities and desktop analyzers are not Flipper firmware applications and are not placed on the device.
- Third-party submodules keep their own licenses and maintenance histories; they are not represented as UberGuidoZ-authored code.
- Flipper-IRDB and other large SD-card datasets are not overlaid automatically, which avoids replacing user data and avoids applying one license assumption to older mixed-license history. They can be offered later as optional, user-selected asset packs after a separate provenance audit.

The separately published `Flipper-Research` and `Flipper-SGDB` repositories were also checked. At the audit date they contain an MIT license and README material, but no device application source or signal database files to package. `Flipper-IRDB` is a community fork rather than an original application and therefore remains an optional external dataset, not a rebranded NLX component.

## Maintenance

When updating either application:

1. Record the audited upstream commit.
2. Review the upstream diff and license before copying source.
3. Preserve original attribution and version history.
4. Reapply only the NLX launcher icon/title integration.
5. Build both individual FAPs and the full updater package.
6. Confirm the updater contains both FAPs and does not overwrite `/ext` user data.
