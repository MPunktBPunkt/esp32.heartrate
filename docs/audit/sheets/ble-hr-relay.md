# Technology Sheet: BLE HR-Relay (Peripheral Bridge)

```yaml
slug: ble-hr-relay
title: BLE HR-Relay (Peripheral Bridge)
primary: C
tags: [esp-idf, streaming]
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

Muster: ein Gerät empfängt HR/RR als BLE Central und bietet denselben Heart Rate Service als Peripheral an weitere Consumer an („ein Sensor, mehrere Listener“), ohne den Originalgurt mehrfach zu koppeln.

## Abgrenzung

- **Ist:** Protokoll-Brücke / GATT-Server-Spiegelung für `0x180D`/`0x2A37`.
- **Ist nicht:** proprietäre Fitness-Cloud; Ersatz für Multi-Connect im Sensor selbst.

## Schnittstellen und Abhängigkeiten

| Richtung | Schnittstelle | Bemerkung |
|----------|---------------|-----------|
| oben | Consumer (Phone, Bike Computer) | NOTIFY auf `0x2A37` |
| unten | Quell-Central / Parser | Rohbytes oder normalisierte HR/RR |
| lateral | Connection-Budget des Controllers | Central + N Peripheral-Links |

## Typische Fallstricke

| Claim | Status | Hinweis |
|-------|--------|---------|
| Advertising bleibt nach erstem Peripheral-Connect automatisch connectable | `HYPOTHESIS` | Stack-abhängig; oft expliziter Restart nötig |
| Notify aus dem BLE-Callback ist immer sicher | `REJECTED` | Besser Einspeisung aus App-Loop (Reentrancy/Heap) |
| Beliebig viele Relay-Clients | `REJECTED` | Controller-`MAX_CONNECTIONS` begrenzt |

## Transfer

Nützlich wenn Sensoren single-link sind und mehrere Apps gleichzeitig Puls brauchen. Vermeiden auf Chips ohne genug Connections/RAM.

## Referenzen

- Bluetooth HR Service (wie `ble-hrm-gatt`)
- NimBLE multi-connection / advertising notes

## Offene Fragen

- Byte-identische Weiterleitung vs. Re-Encode: wann ist was spezkonform genug?
- Idle-Policies: Central-Link halten solange Peripheral-Subscriber existieren?
