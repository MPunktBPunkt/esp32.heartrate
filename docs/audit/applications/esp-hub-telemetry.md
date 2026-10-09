# Project Application: esp-hub-telemetry in esp32.heartrate

```yaml
project_id: esp32-heartrate
tech_slug: esp-hub-telemetry
status: active
sheet: ../sheets/esp-hub-telemetry.md
maturity:
  technology: R2
  implementation: R3
  field_evidence: R2
  reusability: W2
```

## Rolle im Projekt

`HubClient` sendet Heartbeats mit `fwType=heartrate` und Live-/Relay-Metriken, unterstützt Hub-OTA-URL und Session-Export-POST. Hub-Repo bleibt out of scope.

## Architektur / Datenfluss

```text
HubClient.loop → sendHeartbeat (JSON from App)
User → POST /api/session/export/send → Hub /api/session-export
Hub → OTA URL in response → performOta
Config save → sendNow()
```

## Relevante Pfade

| Pfad | Rolle |
|------|-------|
| `src/core/HubClient.cpp` | Heartbeat, OTA, postJson |
| `src/core/ConfigStore.cpp` | hubHost/hubPort, deviceName |
| `platformio.ini` | `HUB_PORT_DEFAULT=8093` |

## Konfiguration und Parameter

| Claim | Status |
|-------|--------|
| Default-Port 8093 | `FACT` (`platformio.ini`) |
| IO-Keys u. a. continuity, strap_fit, relay_* | `INFERRED` (README Hub-IO) |
| Nach Config-Save sofortiger Heartbeat | `FACT` (Commit `75f1dc5` / App-Integration) |

## Ebene C — Host- / Feld-Evidenz

| Claim | Evidence-ID | Status |
|-------|-------------|--------|
| Hub-IO-Keys in README dokumentiert | `EVID-HUB-2026-10-08-001` | `INFERRED` |
| Heartbeat gegen produktiven Hub im LAN verifiziert (dieses Audit) | — | `UNKNOWN` |

## Beobachtungen (Lab / Feld)

| Datum / Artefakt | Beobachtung |
|------------------|-------------|
| README | Hub-IO-Tabelle, fwType `heartrate` |
| Out of scope | iobroker.esp-hub Internals |

## Abweichungen vom „Lehrbuch“

- Session-Export und OTA teilen sich denselben Client; Fehlerpfade über lastOk/lastSuccessMs.

## Offene Fragen

- Sample-JSON Heartbeat + Export als Fixture im Repo?

## Verwandte Applications / Methods

- [esp-embedded-web-sse](esp-embedded-web-sse.md)
