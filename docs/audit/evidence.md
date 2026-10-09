# Evidence — esp32-heartrate-2026-pilot

Desk-Audit 2026-10-08 gegen Commit `72acae4`; Claim-Abgleich und Struktur-Nacharbeit 2026-10-09. Rohdaten bleiben im Quellenrepo; hier nur Register.

Basis: `https://github.com/MPunktBPunkt/esp32.heartrate/blob/72acae4/`

| ID | Claim (Kurz) | Status | Quelle |
|----|--------------|--------|--------|
| `EVID-HRM-2026-10-08-001` | Polar H9 als Testgerät mit HR Service `0x180D` | `MEASURED` | [README](https://github.com/MPunktBPunkt/esp32.heartrate/blob/72acae4/README.md) („Getestet u. a. mit Polar H9“) |
| `EVID-HRM-2026-10-08-002` | H9 ohne zuverlässiges Sensor-Contact-Flag; UI `n/a`, `strapFit` abgeleitet | `MEASURED` | README + `src/app/App.cpp` (strapFit-Ableitung) |
| `EVID-RELAY-2026-10-08-001` | ESP32-S3 mit HR-Relay als getestete Konfiguration dokumentiert | `MEASURED` | README Hardware-Tabelle; `platformio.ini` env `heartrate-s3` |
| `EVID-RELAY-2026-10-08-002` | Advertising-Restart nach erstem Peripheral-Connect | `FACT` | Commit `72acae4` · `src/ble/HrServer.cpp` |
| `EVID-WEB-2026-10-08-001` | REST + SSE `/events` sind dokumentierte Lieferbestandteile v0.3.1 | `INFERRED` | README Features/API; `App.cpp` `/events` |
| `EVID-HUB-2026-10-08-001` | Hub-IO-Keys und `fwType=heartrate` dokumentiert | `INFERRED` | README Hub-IO |
| `EVID-CI-2026-10-08-001` | PlatformIO-Matrix-Build für beide Envs existiert | `FACT` | `.github/workflows/build.yml` |

## Offene Claims (noch ohne ID)

| Claim | Status | Hinweis |
|-------|--------|---------|
| PFLICHTENHEFT-v0.3 Abnahmekriterien 1–15 als Lab-PASS | `UNKNOWN` | Spez da; kein PASS-Protokoll im Repo |
| Relay mit Radcomputer im Feld | `UNKNOWN` | Folgearbeit |
| Continuity-KPIs vs. Pflichtenheft | `UNKNOWN` | Folgearbeit |
| Heartbeat gegen live Hub | `UNKNOWN` | Hub out of scope dieses Laufs |

## Wissens-Chronik

| Datum | Ereignis |
|-------|----------|
| 2026-10-08 | Pilot-Desk-Audit; erster Export fälschlich in pidrive, korrigiert nach `docs/audit/` |
| 2026-10-08 | Struktur an technology-audit Review-Checks angeglichen (Status/Coverage, Sheets A, Apps B/C, Evidence) |
| 2026-10-09 | Re-Audit: Validator/Scan nachgezogen; Reconnect-Claim (`kReconnectGiveUpMs`) korrigiert; Scan ohne Self-Hits |
