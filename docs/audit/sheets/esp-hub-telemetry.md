# Technology Sheet: ESP-Hub Node Telemetry

```yaml
slug: esp-hub-telemetry
title: ESP-Hub Node Telemetry
primary: F
tags: [esp-idf]
type: pattern
status: active
seen_in: [esp32-heartrate]
maturity:
  technology: R2
  implementation: R0
  field_evidence: R0
  reusability: W2
```

## Kurzfassung

LAN-Hub-Muster: Nodes senden periodische HTTP-Heartbeats (`fwType`, Metriken, Status), empfangen optional OTA-URLs und posten Session-/Export-Payloads. Hub bleibt separates System; Audit betrifft die Node-Schnittstelle.

## Abgrenzung

- **Ist:** Node→Hub Telemetrie + Steuerimpulse (OTA, Export).
- **Ist nicht:** Cloud-Sync ohne Hub; vollständige Hub-Implementierung.

## Schnittstellen und Abhängigkeiten

| Richtung | Schnittstelle | Bemerkung |
|----------|---------------|-----------|
| oben | Hub HTTP API | Heartbeat, session-export, OTA-URL |
| unten | Config / Sensor-State | fwType, live metrics, relay flags |
| lateral | Web-UI Config | Host/Port, sofortiger Heartbeat nach Save |

## Typische Fallstricke

| Claim | Status | Hinweis |
|-------|--------|---------|
| Hub ist immer erreichbar | `REJECTED` | Nodes brauchen lastOk/Backoff |
| Config-Änderungen dürfen bis zum nächsten Intervall warten | `HYPOTHESIS` | Oft schlechte UX; besser send-now |
| OTA aus Heartbeat-Response ist spezifiziert überall gleich | `UNKNOWN` | Projekt-/Hub-Version prüfen |

## Transfer

Wiederkehrendes Muster in ESP-Node-Familien. Voraussetzung: stabile Hub-URL im LAN, dokumentierte IO-Keys.

## Referenzen

- Projektspezifisch: iobroker.esp-hub (externes Repo)
- HTTP JSON Heartbeat-Konventionen der jeweiligen Node-Familie

## Offene Fragen

- Einheitliches Schema der IO-Keys über Node-Typen?
- Authentifizierung/Trust im LAN?
