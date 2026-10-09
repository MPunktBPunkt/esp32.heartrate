# Project Scope: esp32.heartrate

```yaml
project_id: esp32-heartrate
updated: 2026-10-09
source_repo: https://github.com/MPunktBPunkt/esp32.heartrate
reviewed_commit: 72acae4
```

## Grenzen

| | |
|--|--|
| **In scope** | Firmware unter `src/`, `platformio.ini`, `docs/PFLICHTENHEFT*.md`, `.github/workflows/build.yml`, README-API/Hub-Doku |
| **Nicht in scope** | Internals von [iobroker.esp-hub](https://github.com/MPunktBPunkt/iobroker.esp-hub) (nur HTTP-Schnittstelle); medizinische Validierung; volle HRV-Analyse (Poincaré/LF-HF) |
| **Hardware** | ESP32 D1 Mini (`heartrate`) · ESP32-S3 (`heartrate-s3`, inkl. HR-Relay) |
| **Version** | README-Badge 0.3.1 · `FW_VERSION` in `platformio.ini` = `0.3.2` |

## Systeme

```text
Polar H9 ──BLE Central──▶ ESP32 ──BLE Peripheral──▶ Handy / Radcomputer (Relay, nur S3)
                            │
                            └──WiFi──▶ WebUI/SSE + ESP-Hub (Heartbeat / OTA / Session-Export)
```

## Ziel dieses Audit-Laufs

Desk-Inventar + priorisierte Technology Sheets/Applications für die vier Kernpfade (HR-Central, HR-Relay, Embedded-Web/SSE, Hub). Feld-Abnahme gegen PFLICHTENHEFT-v0.3 bleibt Folgearbeit.
