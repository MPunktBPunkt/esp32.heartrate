# Project Application: ble-hr-relay in esp32.heartrate

```yaml
project_id: esp32-heartrate
tech_slug: ble-hr-relay
status: active
sheet: ../sheets/ble-hr-relay.md
maturity:
  technology: R2
  implementation: R3
  field_evidence: R2
  reusability: W2
```

## Rolle im Projekt

Nur `heartrate-s3` (`HR_RELAY=1`): GATT-Server spiegelt empfangene HR/RR an bis zu zwei Consumer. Default aus, persistent in Config. Mini-Build: API `501` / `relay.supported == false`.

## Architektur / Datenfluss

```text
Gurt --> BleCentral (parse)
     --> HrServer (0x180D / NOTIFY 0x2A37) --> ≤2 BLE Clients
Parallel: WebUI /api/relay · Hub heartbeat relay_* 
```

## Relevante Pfade

| Pfad | Rolle |
|------|-------|
| `src/ble/HrServer.cpp` | Peripheral, Advertising-Restart nach Connect |
| `src/ble/BleCentral.cpp` | Idle-Disconnect-Sperre bei Subscribern |
| `docs/PFLICHTENHEFT-v0.3-RELAY.md` | Spez + Abnahmekriterien 1–15 |
| `platformio.ini` env `heartrate-s3` | `MAX_CONNECTIONS=3`, `HR_RELAY=1` |

## Konfiguration und Parameter

| Claim | Status |
|-------|--------|
| Werkszustand Relay aus; persistent über Reboot | `INFERRED` (PFLICHTENHEFT + ConfigStore) |
| Max 2 Relay-Clients (`HR_RELAY_MAX_CLIENTS`) | `INFERRED` (App.cpp clamp) |
| Commit `72acae4`: Advertising nach erstem Client neu starten | `FACT` (Git-History / HrServer-Kommentare) |
| Notify-Einspeisung nicht aus NimBLE-Callback | `FACT` (PFLICHTENHEFT-Verbot + Architektur) |

## Ebene C — Host- / Feld-Evidenz

| Claim | Evidence-ID | Status |
|-------|-------------|--------|
| S3-Env inkl. Relay „getestet“ (Projekt-Doku) | `EVID-RELAY-2026-10-08-001` | `MEASURED` |
| Abnahmekriterien 1–15 (nRF Connect, 2 Consumer, Idle-Sperre, …) als PASS protokolliert | — | `UNKNOWN` |
| Radcomputer als zweiter Consumer im Feld | — | `UNKNOWN` |

## Beobachtungen (Lab / Feld)

| Datum / Artefakt | Beobachtung |
|------------------|-------------|
| `72acae4` | fix(relay): restart advertising after first client connects |
| PFLICHTENHEFT §12 | 15 Abnahmekriterien spezifiziert, ohne Evidence-IDs im Repo |

## Abweichungen vom „Lehrbuch“

- Sensor-Contact-Flags werden gespiegelt, nicht erfunden.
- Advertising-Race explizit behandelt (Stack stoppt connectable Adv nach Accept).

## Offene Fragen

- Lab-Protokoll zu Kriterien 4–10 mit Evidence-IDs anlegen?
- Byte-identischer Roundtrip (Kriterium 5) nachgewiesen?

## Verwandte Applications / Methods

- [ble-hrm-gatt](ble-hrm-gatt.md)
