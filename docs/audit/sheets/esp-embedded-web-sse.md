# Technology Sheet: Embedded Web REST + SSE

```yaml
slug: esp-embedded-web-sse
title: Embedded Web REST + SSE
primary: G
tags: [streaming]
type: pattern
status: active
seen_in: [esp32-heartrate]
maturity:
  technology: R3
  implementation: R0
  field_evidence: R0
  reusability: W2
```

## Kurzfassung

On-device HTTP-Server mit REST für Steuerung/Config und Server-Sent Events für Live-Metriken — UI oft als eingebettete statische Seiten in der Firmware, ohne separates Frontend-Repo.

## Abgrenzung

- **Ist:** Browser-Setup und Live-Telemetry auf dem Node.
- **Ist nicht:** Cloud-SPA, WebSocket-Pflicht, native Mobile-App.

## Schnittstellen und Abhängigkeiten

| Richtung | Schnittstelle | Bemerkung |
|----------|---------------|-----------|
| oben | Browser | REST + `text/event-stream` |
| unten | App-State / Sensorik | Status, History, Commands |
| lateral | WiFi STA/AP | Captive-Portal oft separat |

## Typische Fallstricke

| Claim | Status | Hinweis |
|-------|--------|---------|
| SSE allein reicht ohne Polling-Fallback | `HYPOTHESIS` | Single-client HTTP auf ESP kann SSE stallen |
| Eingebettete UI skaliert wie ein SPA-Build | `REJECTED` | Speicher/Flash-Budget dominiert |
| REST-Mutationen brauchen keinen sofortigen Telemetry-Push | `HYPOTHESIS` | UX oft besser mit sofortigem Heartbeat/Event |

## Transfer

Muster für ESP-Nodes mit Erstsetup und Live-Charts. Voraussetzung: stabiles WiFi oder SoftAP; klare API-Tabelle.

## Referenzen

- HTML Living Standard — Server-Sent Events
- ESP `WebServer` / Async-Varianten (Implementierungsabhängig)

## Offene Fragen

- Wann AsyncWebServer vs. synchrone `WebServer`-Loop?
- Maximale parallele SSE-Clients auf typischem ESP32-Heap?
