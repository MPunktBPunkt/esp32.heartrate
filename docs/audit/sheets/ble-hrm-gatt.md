# Technology Sheet: BLE Heart Rate GATT (Central)

```yaml
slug: ble-hrm-gatt
title: BLE Heart Rate GATT (Central)
primary: C
tags: [esp-idf, streaming]
type: protocol
status: active
seen_in: [esp32-heartrate]
maturity:
  technology: R3
  implementation: R0
  field_evidence: R0
  reusability: W2
```

Nur Ebene **A**. Projektpfade und Sensor-Quirks → Application.

## Kurzfassung

Bluetooth LE Heart Rate Service (`0x180D`) mit Characteristic Heart Rate Measurement (`0x2A37`). Ein Central subscribed auf Notifies und parst Flags, BPM und optionale RR-Intervalle (Auflösung 1/1024 s laut Spec).

## Abgrenzung

- **Ist:** GATT-Profil für Puls-/RR-Daten von Wearables.
- **Ist nicht:** AVRCP/A2DP, medizinische Zertifizierung, HRV-Analyse oberhalb der Roh-RR-Pipeline.

## Schnittstellen und Abhängigkeiten

| Richtung | Schnittstelle | Bemerkung |
|----------|---------------|-----------|
| oben | App / Session-Pipeline | BPM, RR, Battery, RSSI |
| unten | BLE Stack (z. B. NimBLE) | Scan, Connect, GATT Subscribe |
| lateral | optional Peripheral-Relay | gleicher Service nach außen spiegeln |

## Typische Fallstricke

| Claim | Status | Hinweis |
|-------|--------|---------|
| Sensor Contact Bit ist überall gesetzt | `REJECTED` | Viele Gurte (u. a. Polar H9) liefern es nicht |
| Ein Gurt erlaubt beliebig viele gleichzeitige Centrals | `HYPOTHESIS` | Geräteabhängig; oft nur ein Link |
| RR-Einheiten sind immer Millisekunden | `REJECTED` | Spec: 1/1024 s Einheiten |

## Transfer

Geeignet für ESP-/NimBLE-Gateways und RR-Vorverarbeitung. Voraussetzung: Central-Rolle, Heap für Scan+Connection, klare Trennung Parser vs. UI.

## Referenzen

- [Bluetooth Heart Rate Service](https://www.bluetooth.com/specifications/specs/) (GATT HR)
- NimBLE-Arduino / ESP NimBLE Central-Docs

## Offene Fragen

- Welche weiteren Gurte außer Polar H9 verhalten sich bzgl. Flags/RR identisch?
- Wie stabil ist Session-Reconnect geräteübergreifend (Fenster, Backoff)?
