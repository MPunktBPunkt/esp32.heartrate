# Project Application: esp-embedded-web-sse in esp32.heartrate

```yaml
project_id: esp32-heartrate
tech_slug: esp-embedded-web-sse
status: active
sheet: ../sheets/esp-embedded-web-sse.md
maturity:
  technology: R3
  implementation: R3
  field_evidence: R2
  reusability: W2
```

## Rolle im Projekt

Eingebettete UI (`UiPages.h`) + REST in `App.cpp` steuern BLE, Config, Sessions, Relay; SSE `/events` liefert Live-Metriken. Polling-Fallback in der UI falls SSE stallt.

## Architektur / Datenfluss

```text
Browser --HTTP--> WebServer routes (App)
       <--SSE-- /events
       --POST--> config → ConfigStore → HubClient.sendNow()
```

## Relevante Pfade

| Pfad | Rolle |
|------|-------|
| `src/web/UiPages.h` | Embedded HTML/JS inkl. EventSource |
| `src/app/App.cpp` | Routes, `handleEvents`, API |
| README § API | Vertragsoberfläche |

## Konfiguration und Parameter

| Claim | Status |
|-------|--------|
| SSE Content-Type `text/event-stream` auf `/events` | `FACT` (App.cpp) |
| UI hat SSE-Stall-Fallback | `FACT` (UiPages.h Kommentar/Code) |
| Hotspot-Name Setup `ESP-HR-Setup` | `INFERRED` (README Quickstart) |

## Ebene C — Host- / Feld-Evidenz

| Claim | Evidence-ID | Status |
|-------|-------------|--------|
| Live-Charts/SSE Teil der getesteten 0.3.1-Features (Doku) | `EVID-WEB-2026-10-08-001` | `INFERRED` |
| SSE bleibt unter Relay + 45‑min Session flüssig (Abnahme 13) | — | `UNKNOWN` |

## Beobachtungen (Lab / Feld)

| Datum / Artefakt | Beobachtung |
|------------------|-------------|
| README API-Tabelle | `/events`, `/api/status`, BLE- und Relay-Routen dokumentiert |

## Abweichungen vom „Lehrbuch“

- Kein separates Frontend-Build; alles in Firmware-Header.
- Config-Save triggert sofortigen Hub-Heartbeat (nicht nur UI-Event).

## Offene Fragen

- Max. parallele Browser-Tabs / SSE-Clients gemessen?

## Verwandte Applications / Methods

- [esp-hub-telemetry](esp-hub-telemetry.md)
- [ble-hrm-gatt](ble-hrm-gatt.md)
