# esp32.heartrate

![Version](https://img.shields.io/badge/version-0.3.1-blue)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Build](https://github.com/MPunktBPunkt/esp32.heartrate/actions/workflows/build.yml/badge.svg)](https://github.com/MPunktBPunkt/esp32.heartrate/actions/workflows/build.yml)
[![Donate](https://img.shields.io/badge/Donate-PayPal-00457C.svg?logo=paypal)](https://www.paypal.com/donate/?business=martin%40bchmnn.de&currency_code=EUR)

> **BLE Heart-Rate / Fitness-Gateway** für ESP32 — Polar H9 & Co., Live-Charts, Session-Archiv, Export und Anbindung an [iobroker.esp-hub](https://github.com/MPunktBPunkt/iobroker.esp-hub).

---

## Überblick

`esp32.heartrate` ist ein **eigenständiger Gateway-Node**: der ESP verbindet sich per BLE Central (NimBLE) mit einem Heart-Rate-Gurt, zeigt BPM/RR live im Browser und speichert Sessions lokal. Der Gurt bleibt standardmäßig frei für Fahrradcomputer oder Handy — **kein Auto-Connect**, kein Setup-Wizard.

| Das ist es | Das ist es nicht |
|------------|------------------|
| BLE-HR-Gateway mit Web-UI | Ersatz für Polar / Garmin / Strava |
| Live-Charts + Session-Archiv | Medizinisches Messgerät |
| RR-/Quality-Basis für HRV | Volle HRV-Analyse (Poincaré, LF/HF) |
| Hub-Telemetrie + Session-Export | Cloud-Sync ohne Hub |

```
Polar H9 ──BLE Central──▶ ESP32-S3 ──BLE Peripheral──▶ Handy / Radcomputer (Relay, bis 2)
                             │
                             └──WiFi──▶ WebUI/SSE + ESP-Hub
```

Auf dem D1 Mini (`heartrate`) bleibt nur die Central-Rolle — Relay ist wegkompiliert.

Technologie-Audit (Inventar, Sheets, Evidence) lebt im Repo [technology-audit](https://github.com/MPunktBPunkt/technology-audit) unter [`projects/esp32-heartrate/`](https://github.com/MPunktBPunkt/technology-audit/tree/main/projects/esp32-heartrate) — dieses Repo bleibt die Firmware-Quelle und wird nur gelesen.
---

## Features v0.3.1

- **BLE Central (NimBLE):** Scan, Connect, Disconnect, Remember / Forget
- **HR-Relay (nur ESP32-S3):** GATT-Peripheral `0x180D`, treue Weiterleitung von HR/RR an bis zu 2 Verbraucher — Default aus, persistent
- **Config→Hub:** nach Speichern im Web-UI sofortiger Heartbeat (Gerätename ohne Warten auf Intervall)
- **Heart Rate Measurement:** BPM + RR-Intervalle, Battery, RSSI
- **Session-Reconnect** (bis ca. 5 min nach Linkverlust) + konfigurierbarer Idle-Disconnect (ausgesetzt solange Relay-Verbraucher abonniert)
- **Modi:** Rest / Alltag / Training / Recovery inkl. Soft-Auto und Baseline-Guide
- **RR-Pipeline:** `rrRaw` (1/1024 s), Beat-Timeline, Quality-Labels (Valid / Suspect / Artifact / Invalid), Gap-Handling
- **Continuity-KPIs:** Lücken, Reconnects, RSSI min/avg/max, Continuity-%
- **LittleFS-Archiv:** bis 64 Sessions, verdichtete Serie (~10 s)
- **Export:** JSON / CSV lokal + **An Hub senden**
- **Scan-UI:** standardmäßig nur HR-relevante Geräte (Toggle „alle BLE-Geräte“)
- **Web-UI:** Live-Charts (BPM, RR, Beat-to-Beat, Battery, RSSI), SSE, Relay-Badge
- **ESP-Hub:** Heartbeat mit `fwType: heartrate`, Web-OTA

---

## Hardware

| Board | PlatformIO-Env | Hinweis |
|-------|----------------|---------|
| ESP32 Mini D1 | `heartrate` | getestet — kein Relay |
| ESP32-S3 | `heartrate-s3` | getestet — inkl. HR-Relay |

Getestet u. a. mit **Polar H9** (BLE Heart Rate Service `0x180D`). Sensor-Contact-Flags liefert der H9 typischerweise nicht — die UI zeigt dann `n/a` und leitet `strapFit` ab.

---

## Quickstart

```bash
pio run -e heartrate-s3 --target upload
pio device monitor
```

1. Hotspot **`ESP-HR-Setup`** → WLAN + Hub-IP (Port `8093`)
2. Browser: `http://<ESP-IP>/` → **Devices** → Scan → Connect
3. Gerät erscheint im [ESP-Hub](https://github.com/MPunktBPunkt/iobroker.esp-hub)

| | |
|--|--|
| mDNS | `hr-XXXXXX.local` |
| OTA | `POST /ota-upload` (multipart `firmware`) |
| WLAN zurücksetzen | BOOT / GPIO0 ca. 3 s halten |

---

## Libraries

| Library | Autor | Version |
|---------|-------|---------|
| WiFiManager | tzapu | ≥ 2.0.17 |
| ArduinoJson | bblanchon | ≥ 7.2 |
| NimBLE-Arduino | h2zero | ≥ 1.4.3 |

Platform: `espressif32@6.4.0`, Framework Arduino.

---

## Hub-IO-Werte

Heartbeat-Feld `fwType`: **`heartrate`**

Auszug der IOs / Metriken:

| Key / Feld | Bedeutung |
|------------|-----------|
| `session_continuity_pct` | Anteil ununterbrochener Session-Zeit |
| `session_effective_gap_s` | Summe relevanter Lücken |
| `strap_fit` / `strapFit` | abgeleitete Gurt-Passung |
| `relay_enabled` | Relay an/aus (0/1, S3) |
| `relay_clients` | verbundene Relay-Verbraucher |
| BPM, Battery, RSSI | Live-Werte im Heartbeat / Status |

Session-Export vom Gerät → Hub:

| Endpoint (Gerät) | Beschreibung |
|------------------|--------------|
| `GET /api/session/export` | JSON (Summary + Serie) |
| `GET /api/session/export.csv` | CSV |
| `POST /api/session/export/send` | POST an Hub `/api/session-export` |

Im Hub: States `devices.<MAC>.lastSessionExport` / `lastSessionExportAt`.

---

## API (Auswahl)

| Endpoint | Funktion |
|----------|----------|
| `GET /api/status` | Gesamtstatus, Session, Beats, Quality |
| `GET /api/history` | Chart-Historie (BPM, …) |
| `GET /api/ble/devices` | Scan-/Remember-Liste |
| `POST /api/ble/scan/start` `/stop` | Scan steuern |
| `POST /api/ble/connect` `/disconnect` | Verbindung |
| `POST /api/ble/remember` `/forget` | Merken |
| `POST /api/session/mode` | `rest` / `activity` / `training` / `recovery` / `idle` |
| `POST /api/session/baseline` | Guide / Capture / Clear |
| `GET /api/sessions` · `POST …/clear` | Archiv |
| `GET /api/session/export` · `.csv` · `POST …/send` | Export |
| `GET/POST /api/config/get` `/save` | Config |
| `GET/POST /api/relay` | HR-Relay Status / Schalten (S3; Mini → 501) |
| `POST /api/system/restart` | Neustart |
| `/events` | SSE (Live-Updates) |

---

## Build / CI

GitHub Actions baut beide Envs (`heartrate`, `heartrate-s3`) und lädt `firmware.bin` als Artifact hoch.

Lokal:

```bash
pio run -e heartrate
pio run -e heartrate-s3
```

---

## Docs

- [docs/PFLICHTENHEFT.md](docs/PFLICHTENHEFT.md) — Zielbild & Anforderungen
- [docs/PFLICHTENHEFT-v0.2.md](docs/PFLICHTENHEFT-v0.2.md) — RR-/Quality-Phase
- [docs/PFLICHTENHEFT-v0.3-RELAY.md](docs/PFLICHTENHEFT-v0.3-RELAY.md) — HR-Relay (BLE-Peripheral)

---

## Lizenz & Support

GNU General Public License v3.0 © MPunktBPunkt — siehe [LICENSE](LICENSE).

Wenn dir das Projekt hilft, freue ich mich über einen Kaffee:

[![Donate](https://img.shields.io/badge/Donate-PayPal-00457C.svg?logo=paypal)](https://www.paypal.com/donate/?business=martin%40bchmnn.de&currency_code=EUR)
