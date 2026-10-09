# Project Inventory: esp32.heartrate

```yaml
project_id: esp32-heartrate
updated: 2026-10-09
reviewed_commit: 72acae4
pilot_order: [ble-hrm-gatt, ble-hr-relay, esp-embedded-web-sse, esp-hub-telemetry]
```

Desk-Audit-Lauf 2026-10-08, Nacharbeit/Re-Audit 2026-10-09 (Struktur Review-Checks, Claim-Abgleich). Taxonomie: Primary A–J wie in [technology-audit TAXONOMY](https://github.com/MPunktBPunkt/technology-audit/blob/main/docs/TAXONOMY.md).

## Übersicht

| Slug | Name | Primary | Tags | Status | Coverage | Sheet | Application |
|------|------|---------|------|--------|----------|-------|-------------|
| `ble-hrm-gatt` | BLE Heart Rate GATT (Central) | C | `esp-idf`, `streaming` | applied | partial | [SHEET](sheets/ble-hrm-gatt.md) | [app](applications/ble-hrm-gatt.md) |
| `ble-hr-relay` | BLE HR-Relay (Peripheral) | C | `esp-idf`, `streaming` | applied | partial | [SHEET](sheets/ble-hr-relay.md) | [app](applications/ble-hr-relay.md) |
| `esp-embedded-web-sse` | Embedded Web REST + SSE | G | `streaming` | applied | partial | [SHEET](sheets/esp-embedded-web-sse.md) | [app](applications/esp-embedded-web-sse.md) |
| `esp-hub-telemetry` | ESP-Hub Heartbeat / OTA / Export | F | `esp-idf` | applied | partial | [SHEET](sheets/esp-hub-telemetry.md) | [app](applications/esp-hub-telemetry.md) |
| `esp32-arduino-pio` | ESP32 + PlatformIO Arduino | D | `esp-idf` | observed | none | — | — |
| `wifi-manager-captive` | WiFiManager Captive Portal | F | — | observed | none | — | — |
| `nvs-config-store` | NVS ConfigStore | E | — | observed | none | — | — |
| `littlefs-session-archive` | LittleFS Session-Archiv | E | — | observed | none | — | — |
| `rr-quality-pipeline` | RR / BeatTimeline / Quality | D | `streaming` | observed | none | — | — |
| `web-ota` | Web- und Hub-OTA | F | — | observed | none | — | — |
| `mdns-discovery` | mDNS `hr-*.local` | F | — | observed | none | — | — |
| `github-actions-pio` | GitHub Actions PlatformIO-Matrix | H | — | observed | none | — | — |

Legende Status: `candidate` · `observed` · `documented` · `applied` · `transferred`  
Legende Coverage: `none` · `partial` · `full`

## Nach Kategorie

### A — Empfang

—

### B — Audio

—

### C — Protokolle

`ble-hrm-gatt`, `ble-hr-relay`

### D — Embedded

`esp32-arduino-pio`, `rr-quality-pipeline`

### E — Speicher / Formate

`nvs-config-store`, `littlefs-session-archive`

### F — Systemintegration

`esp-hub-telemetry`, `wifi-manager-captive`, `web-ota`, `mdns-discovery`

### G — Steuerung / UI

`esp-embedded-web-sse`

### H — Test / Analyse

`github-actions-pio`

### I — Fahrzeug / Geräte

—

### J — Architektur

noch kein eigener Slug (Session/Producer-Consumer in Applications)

## Coverage-Lücken

| Bereich | Warum offen | Nächster Schritt |
|---------|-------------|------------------|
| `ble-hrm-gatt` | Lab/README Polar H9; keine Evidence-Rohlogs im Repo | Feldnotiz + Continuity-KPIs vs. PFLICHTENHEFT |
| `ble-hr-relay` | Abnahmekriterien 1–15 dokumentiert, nicht als PASS protokolliert | Lab-Lauf nRF Connect + zweiter Consumer |
| `esp-hub-telemetry` | Schnittstelle beschrieben; Hub-Repo out of scope | Sample-Heartbeat gegen laufenden Hub |
| observed-only Slugs | kein Sheet-Priorität im Pilot | Sheets bei Transferbedarf nachziehen |

## Signale ohne Eintrag

- Soft-Auto Session-Modi (Rest/Alltag/Training/Recovery) — vorerst unter `rr-quality-pipeline` / Web-UI belassen
- HistoryStore Ringbuffer — Teil von `rr-quality-pipeline`
