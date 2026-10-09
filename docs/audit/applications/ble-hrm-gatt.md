# Project Application: ble-hrm-gatt in esp32.heartrate

```yaml
project_id: esp32-heartrate
tech_slug: ble-hrm-gatt
status: active
sheet: ../sheets/ble-hrm-gatt.md
maturity:
  technology: R3
  implementation: R3
  field_evidence: R3
  reusability: W2
```

## Rolle im Projekt

BLE Central verbindet sich nur auf Nutzerbefehl mit einem HR-Gurt, subscribed `0x2A37`, parst BPM/RR und speist Session-/UI-/Hub-Pipeline. Kein Auto-Connect.

## Architektur / Datenfluss

```text
HR-Gurt --GATT Notify 0x180D/0x2A37--> BleCentral
     --> HrParser (BPM, RR 1/1024 s)
     --> SessionSeries / BeatTimeline / HistoryStore
     --> WebUI (/api/*, SSE) + HubClient heartbeat
```

## Relevante Pfade

| Pfad | Rolle |
|------|-------|
| `src/ble/BleCentral.cpp` | Scan, Connect-Task, Subscribe, Reconnect |
| `src/ble/HrParser.cpp` | HRM-Payload-Parser |
| `src/ble/BleTypes.h` | gemeinsame Typen |
| `platformio.ini` | NimBLE deps; Mini: `MAX_CONNECTIONS=1`, Peripheral disabled |

## Konfiguration und Parameter

| Claim | Status |
|-------|--------|
| Default: HR-only Scan, Toggle „alle Geräte“ in UI | `INFERRED` (README + Code-Pfade) |
| Session-Reconnect-Fenster ca. 5 min nach Linkverlust | `FACT` (`kReconnectGiveUpMs` = 5 min; Scan-Burst `kReconnectScanMs` = 10 s) |
| Idle-Disconnect konfigurierbar; bei Relay-Subscribern ausgesetzt | `INFERRED` (Code `#if HR_RELAY`) |
| D1 Mini: nur Central (`PERIPHERAL_DISABLED`) | `FACT` (`platformio.ini`) |

## Ebene C — Host- / Feld-Evidenz

| Claim | Evidence-ID | Status |
|-------|-------------|--------|
| Polar H9 als Testgurt mit Service `0x180D` | `EVID-HRM-2026-10-08-001` | `MEASURED` |
| H9 liefert Sensor-Contact typischerweise nicht; UI `n/a`, `strapFit` abgeleitet | `EVID-HRM-2026-10-08-002` | `MEASURED` |
| Continuity-KPIs vs. PFLICHTENHEFT Abnahme | — | `UNKNOWN` |

## Beobachtungen (Lab / Feld)

| Datum / Artefakt | Beobachtung |
|------------------|-------------|
| README @ `72acae4` | „Getestet u. a. mit Polar H9“; Hardware-Tabelle Mini/S3 |
| `App.cpp` | `strapFit` aus RR-Qualität + Gaps + RSSI, nicht aus Contact-Flag |

## Abweichungen vom „Lehrbuch“

- Connect läuft asynchron auf FreeRTOS-Task, damit HTTP/SSE nicht blockieren.
- Remember/Forget und manuelle Connect-Pflicht statt Sensor-initiated Bonding-Wizard.

## Offene Fragen

- Rohlog/Trace für H9-RR und Reconnect im Repo hinterlegen?
- Abgleich Garmin/andere Gurte?

## Verwandte Applications / Methods

- [ble-hr-relay](ble-hr-relay.md)
- [esp-embedded-web-sse](esp-embedded-web-sse.md)
