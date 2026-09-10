# Pflichtenheft `esp32.heartrate` v0.3 — HR-Relay (BLE-Peripheral)

Erweiterung der Firmware um eine **zweite BLE-Rolle**: der ESP32 hält den Pulsgurt
als Central und gibt dessen Daten gleichzeitig als eigener **Heart-Rate-Sensor**
(GATT-Peripheral) an andere Geräte weiter.

Baut auf v0.2.16 auf. Zielversion **0.3.0**.

## 1. Ziel und Begründung

Der Polar H9 erlaubt nur **einen** BLE-Link. Solange der ESP verbunden ist, sehen
Handy, Radcomputer und Trainings-Apps den Gurt nicht. Das Relay dreht das um: der
ESP belegt den Gurt einmal und bedient daraus mehrere Verbraucher.

```
Polar H9 ──BLE Central──▶ ESP32-S3 ──BLE Peripheral──▶ Handy / Radcomputer / App
                             │                          (bis zu 2 gleichzeitig)
                             └── WiFi ──▶ WebUI + SSE + ESP-Hub
```

Nebeneffekt für das geplante `esp32.ergo`: dessen Verbindungsbudget von drei
NimBLE-Links wird entlastet, weil der Puls nicht vom Ergo-Knoten selbst geholt
werden muss.

**Damit fällt das v0.1-Nicht-Ziel „Kein BLE-Peripheral-Modus" bewusst weg.** Das
Prinzip „Merken ≠ automatisches Verbinden" bleibt unangetastet: das Relay verbindet
von sich aus nichts, es advertised nur, wenn der Benutzer es eingeschaltet hat.

## 2. Muss

### Plattform

- **Nur ESP32-S3** (`heartrate-s3`). Der D1 Mini bekommt den Code nicht:
  zwei bis drei BLE-Links plus WiFi plus SSE in 520 kB SRAM ohne PSRAM ist die
  falsche Baustelle. Im Env `heartrate` wird das Relay **wegkompiliert**.
- **Ein Binary pro Chip-Familie bleibt die Regel.** Kein separates Relay-Env —
  der Hub bindet Bins über die Familie (`.esp32s3.bin`), eine zweite Produktlinie
  wäre Ballast. Das Relay ist im S3-Bin enthalten und **zur Laufzeit standardmäßig
  aus**.

### GATT-Peripheral

| Service | Characteristic | Eigenschaften | Inhalt |
|---------|----------------|---------------|--------|
| `0x180D` Heart Rate | `0x2A37` Heart Rate Measurement | Notify | Relay-Paket, §4 |
| `0x180D` | `0x2A38` Body Sensor Location | Read | `0x01` (Chest) |
| `0x180F` Battery | `0x2A19` Battery Level | Read + Notify | Batterie des Gurts |
| `0x180A` Device Information | `0x2A29` Manufacturer Name | Read | `MPunktBPunkt` |
| `0x180A` | `0x2A24` Model Number | Read | `esp32.heartrate` |
| `0x180A` | `0x2A26` Firmware Revision | Read | `FW_VERSION` |

`0x180A` ist nicht überall Pflicht, aber manche Radcomputer zeigen ohne Hersteller-
und Modellstring nur eine MAC an. Kostet drei statische Reads.

### Advertising

- Service-UUID `0x180D` in den Advertising-Daten (danach filtern Apps)
- Appearance **`0x0341`** (Heart Rate Sensor — Heart Rate Belt)
- Gerätename in der Scan Response, Default `HR-Relay-XXXXXX` (letzte 3 MAC-Bytes,
  gleiche Ableitung wie beim mDNS-Namen `hr-XXXXXX`)
- Kein Bonding, keine Passkey-Abfrage — offener Sensor wie ein handelsüblicher Gurt
- Advertising läuft nur bei `relayEnabled`, und es läuft **auch dann weiter, wenn
  bereits ein Verbraucher verbunden ist**, solange das Client-Limit nicht erreicht ist

Der GAP-Gerätename wird bei `NimBLEDevice::init()` gesetzt. Eine Namensänderung
greift daher erst nach Neustart — die UI muss das an der Stelle sagen, an der der
Name editierbar ist.

### Verbindungsbudget

`CONFIG_BT_NIMBLE_MAX_CONNECTIONS=3` im S3-Env: ein Link zum Gurt, zwei für
Verbraucher. Mehr wird nicht freigegeben, damit der Central-Link niemals am
Ressourcenlimit scheitert.

### Datenweg

Das Relay ist ein **treuer Weiterleiter, kein Interpret**:

- Ein eingehendes Notify vom Gurt erzeugt genau ein ausgehendes Notify
- Es wird **nicht** nach Quality gefiltert. `Suspect`, `Artifact` und `Invalid`
  gehen unverändert raus. Die Quality-Pipeline (`BeatTimeline`, `SessionStats`)
  bleibt eine reine Auswertungsschicht und darf den Relay-Pfad nicht beeinflussen.
- RR-Intervalle gehen als `rrRaw` durch, also in BLE-Einheiten (1/1024 s). Es wird
  **nicht** über Millisekunden gerechnet — sonst entstehen Rundungsfehler in Daten,
  die der Empfänger für HRV nutzt.
- Sensor-Contact-Flags werden aus dem Quellpaket gespiegelt, nicht erfunden. Der H9
  meldet `contactSupported == false`; dann bleiben Bit 1 und Bit 2 im Relay-Paket 0.

## 3. Nicht-Ziele

- Kein Weiterleiten anderer Profile (kein CSC, kein Cycling Power, kein FTMS) —
  das ist Sache von `esp32.ergo`
- Keine Glättung, Interpolation oder Ausreißerkorrektur im Relay-Pfad
- Kein Relay ohne verbundenen Gurt (kein Abspielen von Archivdaten)
- Kein Bonding / keine Verschlüsselung
- Kein Relay auf dem D1 Mini
- Kein automatisches Einschalten nach Boot in v0.3.0 (siehe §9)

## 4. Paketaufbau `0x2A37`

Aufbau laut Bluetooth-Spezifikation, little endian:

| Offset | Feld | Bedingung |
|--------|------|-----------|
| 0 | Flags (uint8) | immer |
| 1 | Heart Rate (uint8 **oder** uint16) | immer, Format über Bit 0 |
| … | Energy Expended (uint16, kJ) | nur wenn Bit 3 |
| … | RR-Intervalle (n × uint16, 1/1024 s) | nur wenn Bit 4 |

| Flag-Bit | Bedeutung | Quelle im `HrSample` |
|----------|-----------|----------------------|
| 0 | HR-Format: 0 = uint8, 1 = uint16 | `heartRate > 255` |
| 1 | Sensor Contact Detected | `contact == 1` |
| 2 | Sensor Contact Supported | `contactSupported` |
| 3 | Energy Expended Present | `energyPresent` |
| 4 | RR-Interval Present | `rrCount > 0` |
| 5–7 | reserviert, 0 | — |

### MTU-Budget

Die ATT-Standard-MTU ist 23 Byte, nutzbare Payload also **20 Byte**. Der Encoder
muss darauf klemmen und darf nicht davon ausgehen, dass eine größere MTU
verhandelt wurde (das Aushandeln startet die Gegenseite, nicht wir).

| Variante | Fixteil | Platz für RR |
|----------|---------|--------------|
| uint8 HR, keine Energie | 2 Byte | 9 Intervalle |
| uint16 HR, keine Energie | 3 Byte | 8 Intervalle |
| uint16 HR + Energie | 5 Byte | 7 Intervalle |

`HrSample` führt maximal 8 RR-Werte, es passt also praktisch immer alles. Trotzdem:
**HR als uint8 kodieren, wenn der Wert ≤ 255 ist** (spart ein Byte und entspricht
dem, was echte Gurte senden), und überzählige RR-Werte am Ende abschneiden, statt
ein zu langes Paket zu bauen. Ein abgeschnittenes Paket zählt in `relayTruncated`
mit, damit man es in der UI sieht statt es zu erraten.

### Encoder gehört zum Parser

Die Gegenfunktion zum bestehenden `parseHeartRateMeasurement()` kommt in dieselbe
Übersetzungseinheit und wird in `BleTypes.h` deklariert:

```cpp
/** Encode sample into HRM wire format. Returns bytes written, 0 on failure. */
size_t buildHeartRateMeasurement(const HrSample& s, uint8_t* out, size_t cap);
```

Damit sind Parser und Encoder als Paar testbar (Roundtrip: Rohbytes → `HrSample`
→ Rohbytes muss identische Bytes ergeben, solange das Quellpaket keine reservierten
Flags setzt) und die Datei bleibt frei von NimBLE-Abhängigkeiten.

## 5. Verhalten in Grenzfällen

| Situation | Verhalten |
|-----------|-----------|
| Relay aus | Kein Server, kein Advertising, Peripheral-Rolle liegt brach |
| Relay ein, Gurt nicht verbunden | Advertising läuft, Verbraucher können verbinden, es kommen keine Notifies |
| Boot mit persistiertem `relayEnabled` | Advertising startet mit, der Gurt bleibt getrennt (kein Auto-Connect). Verhält sich wie die Zeile darüber |
| Gurt verliert Link während Verbraucher verbunden ist | Verbindung zum Verbraucher **bleibt**. Nach `HR_RELAY_STALE_S` (Default 10 s) ohne frisches Paket wird **einmalig HR = 0** gesendet, danach Stille bis der Gurt zurück ist |
| Idle-Disconnect (Default 900 s) | **Ausgesetzt**, solange mindestens ein Verbraucher `0x2A37` abonniert hat. Sonst würde das Relay sich selbst abschalten, während ein Radcomputer daran hängt |
| Session-Reconnect nach Linkverlust | Unverändert. Das Relay hält seine Verbraucher über die Reconnect-Phase hinweg |
| Verbraucher abonniert nicht (verbindet nur) | Kein Notify, aber Reads auf `0x180F` / `0x180A` funktionieren |
| Client-Limit erreicht | Advertising stoppt, `relayFull` im Status. Nach einem Disconnect startet Advertising automatisch neu |
| Relay wird per UI ausgeschaltet, während Verbraucher hängen | Verbraucher werden aktiv getrennt, dann Advertising stoppen |
| Scan läuft, Relay aktiv | Erlaubt, aber Notify-Jitter ist möglich. Kein Blockieren, nur eine Notiz in der UI |
| Batterie-Level ändert sich | Notify auf `0x2A19` nur bei Wertänderung, nicht im Sekundentakt |

## 6. Threading und Einspeisung

Der Datenfluss wird **nicht** im NimBLE-Callback gesendet. Eingespeist wird im
Hauptloop, dort wo die frischen Samples ohnehin verteilt werden:

```
App::loop()
  └─ ble.loop()
  └─ fresh = ble.takeSampleDirty()
       └─ beats.onPacket(...)          (bestehend)
       └─ session.onSample(...)        (bestehend)
       └─ relay.onSample(...)          NEU
```

**Wichtig:** `takeSampleDirty()` ist ein selbstlöschendes Flag und wird schon von
`App::loop()` konsumiert. Es darf **kein zweiter Konsument** dazukommen und das
Flag wegnehmen — das Relay hängt sich in den bestehenden `if (fresh)`-Block, es
holt sich das Flag nicht selbst.

`HrServer` wird Member von `App`, analog zu `history`, `session`, `archive`. Die
einzigen Änderungen an `BleCentral` sind der Init-Name und die Idle-Disconnect-Sperre.

## 7. Konfiguration

Neue Felder in `ConfigStore` (`kConfigVersion` von 5 auf **6**, Migration: Defaults
setzen, nicht Factory-Reset):

| Feld | Typ | Default | Bedeutung |
|------|-----|---------|-----------|
| `relayEnabled` | `bool` | `false` | Relay an/aus, **persistent** (siehe unten) |
| `relayName` | `String` | `""` → `HR-Relay-XXXXXX` | GAP-Name, Neustart nötig |
| `relayMaxClients` | `uint8_t` | `2` | 1–2, hart geklemmt |
| `relayBattery` | `bool` | `true` | `0x180F` mit anbieten |

Alle vier gehen durch `toJson()` / `fromJson()` wie die bestehenden Felder. Im
`heartrate`-Env (D1 Mini) existieren die Felder, tun aber nichts — so bleibt die
Config zwischen den Boards austauschbar.

### `relayEnabled` überlebt den Neustart

Der Schalter liegt im NVS und wird beim Boot angewendet — gleiches Muster wie
`autoConnect`: per Default aus, aber wenn der Benutzer ihn setzt, bleibt er gesetzt.
Nach einem Stromausfall am Bike kommt das Gerät also mit laufendem Advertising
hoch, **ohne** verbundenen Gurt. Das ist der in §5 beschriebene Zustand: Verbraucher
können koppeln, es kommen nur keine Notifies, bis der Gurt im Devices-Tab verbunden
wird. Kein Sonderfall, keine Extra-Logik.

Ein zweiter Schalter „nach Boot mitstarten" wird **nicht** eingebaut. Ein Toggle,
der einen Toggle steuert, ist eine Einstellung zu viel.

Neu in `BuildFlags.h`:

```cpp
#ifndef HR_RELAY
#define HR_RELAY 0          // im S3-Env auf 1
#endif
#ifndef HR_RELAY_STALE_S
#define HR_RELAY_STALE_S 10
#endif
#ifndef HR_RELAY_MAX_CLIENTS
#define HR_RELAY_MAX_CLIENTS 2
#endif
```

`platformio.ini`, Env `heartrate-s3` — die beiden Rollen-Sperren müssen **raus**:

```ini
build_flags =
  ${env.build_flags}
  -DHR_BOARD_S3=1
  -DHR_HISTORY_SIZE=360
  -DHR_RR_HISTORY_SIZE=720
  -DARDUINO_USB_CDC_ON_BOOT=0
  -DHR_RELAY=1
  -DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=3
  -DCONFIG_NIMBLE_CPP_DEBUG_ASSERT_ENABLED=0
```

Also: `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED` und
`CONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED` im S3-Env entfernen,
im `heartrate`-Env **unverändert stehen lassen**.

## 8. API, Status, UI, Hub

### API

| Endpoint | Methode | Zweck |
|----------|---------|-------|
| `/api/relay` | GET | Relay-Status als JSON |
| `/api/relay` | POST | `{ "enabled": bool, "name"?: string, "maxClients"?: int }` |

POST speichert die Config und schaltet zur Laufzeit um. Ein geänderter `name` wird
gespeichert und in der Antwort mit `"restartRequired": true` quittiert.
Im Mini-Build antworten die Routen mit `501` und `{"error":"relay unsupported"}` —
nicht mit `404`, damit man den Unterschied zu einer alten Firmware sieht.

### Status

`GET /api/status` bekommt einen Block `relay`:

```json
"relay": {
  "supported": true, "enabled": true, "advertising": true,
  "name": "HR-Relay-A1B2C3", "clients": 1, "subscribed": 1, "full": false,
  "notifySent": 1834, "notifyFailed": 0, "truncated": 0,
  "lastNotifyAgoMs": 940, "holdingStrap": true
}
```

`supported: false` und sonst nichts weiter im Mini-Build. Die Felder gehen auch in
den SSE-`state`-Pfad, damit die UI ohne Poll mitbekommt, wenn ein Verbraucher
kommt oder geht.

### UI (`UiPages.h`)

- **Devices-Tab**, eigener Abschnitt unter der Gurt-Sektion: Toggle „Relay", Name,
  Client-Zähler, Hinweis „Neustart nötig" nach Namensänderung
- **Header-Badge** `RELAY ●` — an, wenn Verbraucher verbunden; hohl, wenn nur
  advertised; aus, wenn Relay aus. Das ist die eine Information, die man während
  des Trainings braucht: hört das andere Gerät noch zu.
- Warnhinweis am Toggle, ehrlich formuliert: mit aktivem Relay hält der ESP den
  Gurt und der Idle-Disconnect greift nicht mehr, solange jemand zuhört
- Kein Relay-Abschnitt im Mini-Build (`supported: false` → Sektion ausblenden)

### Hub-IOs

| Key | Typ | Unit |
|-----|-----|------|
| `relay_enabled` | sensor | 0/1 |
| `relay_clients` | sensor | Anzahl |

Mehr nicht — Notify-Zähler sind Debug-Werte für die UI, nicht für den Hub.

## 9. Was v0.3.0 offen lässt

- **Auto-Connect zum Gurt nach Boot.** Das Relay startet persistent mit, aber
  ohne Gurt hat es nichts zu senden. Damit ein Stromausfall am Bike sich
  vollständig selbst heilt, müsste der ESP den gemerkten Gurt selbst verbinden —
  das ist der bestehende `autoConnect`-Punkt aus v0.2 und wird dort entschieden,
  nicht hier.
- Weitergabe an `esp32.ergo` als Central-zu-Central-Kette produktiv erprobt
- Ob `relayMaxClients = 2` in der Praxis reicht

## 10. Dateien

| Datei | Änderung |
|-------|----------|
| `src/ble/HrServer.h` / `.cpp` | **neu** — Peripheral, Advertising, Client-Buchhaltung, Notify |
| `src/ble/BleTypes.h` | Deklaration `buildHeartRateMeasurement()` |
| `src/ble/HrParser.cpp` | Implementierung des Encoders |
| `src/ble/BleCentral.cpp` | `NimBLEDevice::init(<relayName>)`; Idle-Disconnect-Sperre |
| `src/ble/BleCentral.h` | `void setRelayHold(bool)` + `bool relayHold_` |
| `src/core/ConfigStore.h` / `.cpp` | vier Felder, `kConfigVersion` 6, JSON |
| `src/app/App.h` / `.cpp` | `HrServer relay;`, Einspeisung, Routen, Status, SSE, Heartbeat |
| `src/web/UiPages.h` | Relay-Sektion, Header-Badge |
| `platformio.ini` | S3-Env: Flags nach §7, `FW_VERSION` auf `0.3.0` |
| `include/BuildFlags.h` | `FW_VERSION`, `HR_RELAY*` |
| `README.md` | Feature-Liste, Architekturdiagramm, API-Tabelle, Version |
| `docs/PFLICHTENHEFT.md` | Nicht-Ziel „Kein BLE-Peripheral-Modus" als ab v0.3 aufgehoben markieren |

Der gesamte Relay-Code steht in `#if HR_RELAY` … `#endif`, damit der Mini-Build
byte-identisch zum heutigen Verhalten bleibt.

## 11. Risiken

| Risiko | Bewertung |
|--------|-----------|
| Peripheral-Rolle erhöht RAM- und Flash-Bedarf | S3 mit `min_spiffs.csv` hat Luft. Nach dem Build Flash- und RAM-Report gegen v0.2.16 vergleichen und im PR nennen. |
| Funklast: 2–3 BLE-Links + WiFi + SSE | Bekanntes Terrain, hier enger. Learnings gelten verstärkt: GATT-Arbeit vom Loop trennen, Scan nur auf Befehl. |
| Empfänger verlangt Bonding | Ohne Bonding koppeln die meisten Apps und Radcomputer. Falls ein Gerät zickt: erst mit nRF Connect gegenprüfen, nicht sofort Security einbauen. |
| Relay hält den Gurt dauerhaft | Bewusster Bruch mit „Gurt bleibt frei". Deshalb Default aus und Warnhinweis in der UI. |
| Doppelter Konsument von `takeSampleDirty()` | Konkrete Fehlerquelle, siehe §6. Das Relay holt das Flag nicht selbst. |
| Notify aus dem NimBLE-Callback | Nicht tun. Einspeisung ausschließlich aus dem Loop. |

## 12. Abnahmekriterien

1. Im Werkszustand ist das Relay **aus**. Nach Flash oder Factory-Reset kein
   Advertising — mit nRF Connect nachweisen, dass kein `HR-Relay-*` auftaucht.
2. Relay einschalten, Gerät neu starten: das Relay ist **wieder an** und advertised,
   der Gurt bleibt getrennt. Relay ausschalten, neu starten: bleibt aus.
3. Der Mini-Build (`pio run -e heartrate`) kompiliert unverändert, `/api/status`
   meldet `relay.supported == false`, `/api/relay` antwortet `501`.
4. Relay ein + Gurt verbunden: nRF Connect findet `HR-Relay-XXXXXX`, sieht `0x180D`,
   `0x2A37` liefert im Sekundentakt Notifies mit plausiblen BPM.
5. Die Rohbytes eines Relay-Notifies sind **identisch** mit denen, die der H9
   geschickt hat (bei uint8-HR-Quellpaketen) — Roundtrip an aufgezeichneten Bytes
   nachweisen, nicht nur „sieht plausibel aus".
6. Ein zweiter Verbraucher verbindet parallel; beide bekommen dieselben Werte.
7. Bei drei Verbrauchern lehnt das Gerät ab, ohne dass der Gurt-Link abbricht.
8. Gurt mitten im Betrieb ausschalten: Verbraucher bleiben verbunden, nach 10 s
   kommt einmal HR = 0, die Verbindung bricht nicht.
9. Gurt wieder einschalten: Session-Reconnect greift, Notifies laufen weiter,
   ohne dass der Verbraucher neu koppeln muss.
10. Idle-Disconnect auf 60 s stellen, Verbraucher abonniert, Gurt liefert: der ESP
    trennt den Gurt **nicht**. Verbraucher abmelden: Idle-Disconnect greift wieder.
11. Relay per UI aus: Verbraucher werden getrennt, Advertising endet.
12. 45 Minuten Session mit Gurt + zwei Verbrauchern + offener WebUI, ohne Reboot,
    ohne Heap-Verlust. Freien Heap am Anfang und am Ende protokollieren.
13. Web-UI und SSE bleiben im Relay-Betrieb flüssig bedienbar.
14. `relay_enabled` und `relay_clients` erscheinen im ESP-Hub.
15. GitHub Actions baut beide Envs grün.
