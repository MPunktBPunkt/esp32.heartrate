# Pflichtenheft `esp32.heartrate` v0.1

Eigenständiger ESP32-Knoten: **BLE Heart-Rate / Fitness-Gateway** mit lokaler WebUI und optionaler ESP-Hub-Anbindung.  
Baut auf dem Hub-Muster von `esp-hub-base` / `esp32.rfmonitor` auf (WiFiManager, Preferences, SSE, OTA, Heartbeat).  
**Kein** Teil von `esp32.toolnode`.

Arbeitsname: `esp32.heartrate`  
Alternativen später: `esp32.blefit`, `esp32.bluetooth` (wenn weitere Profile dazukommen).

---

## 1. Ziel

Der ESP32 verbindet sich **nur auf Benutzerbefehl** mit einem BLE-Pulsgurt (zuerst Garmin H9 / Polar H10 o. ä.), liest Heart Rate Measurement inkl. RR-Intervalle, Battery und RSSI aus und zeigt alles live in einer **ansprechenden WebUI mit Charts**.

ioBroker ist optional über `iobroker.esp-hub`.

### Kernprinzip Verbindung

> Merken ≠ automatisches Verbinden.

- Scan und Connect nur manuell (WebUI).
- Gerät darf gespeichert werden („Remember“).
- **Auto-connect: Default OFF** — sonst blockiert der ESP Fahrradcomputer / Handy.
- **Disconnect: Pflicht** (manuell) + optional Idle-Timeout.

---

## 2. Muss (v0.1)

### Plattform

- ESP32 classic (WROOM / DevKit) — BLE Central, später Classic-Option offen
- WiFiManager (Captive Portal), kein SSID/Passwort im Source
- Preferences / ConfigStore (NVS)
- WebServer :80, SSE, Browser-OTA
- Hub-Heartbeat `POST /api/register` (wie rfmonitor)
- Watchdog
- NimBLE als GATT Central (nicht nur Observer wie rfmonitor)

### Bluetooth

- BLE Scan (manuell starten/stoppen)
- Geräteliste: Name, MAC, RSSI, erkannte Services (soweit aus Ads ableitbar)
- Connect / Disconnect / Reconnect (Reconnect = Connect auf gespeicherte MAC)
- Remember / Forget Device
- Heart Rate Service (`0x180D`) — Measurement Notifications
- Battery Service (`0x180F`) wenn vorhanden
- Device Information optional (Manufacturer, Model)
- RSSI der aktiven Verbindung (periodisch)
- State-Machine (siehe §5)
- Bei Disconnect / Verbindungsverlust: Gurt wieder freigeben für andere Clients

### Datenmodell (intern)

Jedes Measurement:

```
HeartRateSample {
  timestamp_ms
  heart_rate          // BPM
  rr_intervals[]      // ms, 0..n
  sensor_contact      // unknown | detected | not_detected
  energy_expended     // optional, kJ / raw
  battery_percent     // optional
  rssi                // dBm
}
```

Ringbuffer im RAM (kein DB):

| Buffer | Inhalt | Fenster (Soll) |
|--------|--------|----------------|
| `hr_series` | BPM + Timestamp | ≥ 5 min @ ~1 Hz |
| `rr_series` | jedes RR | ≥ 5 min |
| `battery_series` | % | spärlich, ≥ 30 min |
| `rssi_series` | dBm | ≥ 5 min |
| `state_log` | State-Wechsel | letzte ~50 Events |

### WebUI (Muss — hoher Stellenwert)

Siehe §6. Kurz:

- Dashboard mit großem BPM, Status, KPIs
- **Mehrere Live-Charts** für alle empfangenen Serien
- Devices-Seite: Scan, Connect, Remember, Forget, Disconnect
- Config: Name, Hub, Idle-Disconnect, Auto-connect (Default OFF)
- SSE für Live-Updates (kein Polling-Zwang)

### Hub-IOs (wenn verbunden / zuletzt bekannt)

| Key | Typ | Unit |
|-----|-----|------|
| `ble_state` | sensor | string |
| `device_name` | sensor | string |
| `device_mac` | sensor | string |
| `connected` | sensor | 0/1 |
| `heart_rate` | sensor | BPM |
| `rr_interval` | sensor | ms (letztes RR) |
| `rr_count` | sensor | Anzahl RRs im letzten Packet |
| `battery` | sensor | % |
| `rssi_ble` | sensor | dBm |
| `sensor_contact` | sensor | string/enum |
| `last_packet_age` | sensor | ms |
| `session_active` | sensor | 0/1 |
| `session_duration` | sensor | s |
| `hr_avg` / `hr_min` / `hr_max` | sensor | BPM |
| `hr_from_rr` | sensor | BPM |
| `hrv_rmssd` / `hrv_sdnn` | sensor | ms |
| `hrv_pnn50` | sensor | % |
| `hr_pct_max` | sensor | % |
| `hr_zone` / `hr_zone_label` | sensor | 1–5 / Zx |
| `calories` | sensor | kcal |

---

## 3. Kann (v0.2+)

- Preferred Device + Auto-connect (explizit einschaltbar, Warnhinweis in UI)
- Trainingsmodus (Dauer, Min/Avg/Max BPM)
- CSV-Export (Live-Fenster oder Training)
- LittleFS Training-History
- Cycling Speed/Cadence, Cycling Power (Profil-Plugin)
- Mehrere gleichzeitige BLE-Connections (Hardware-/Stack-Limits beachten)
- Bluetooth Classic / A2DP (eigenes späteres Projekt oder Rename zu `esp32.bluetooth`)

---

## 4. Nicht-Ziele (v0.1)

- Keine medizinischen Alarme / Diagnosen
- Keine Cloud
- Keine automatische Verbindung nach Boot (außer User schaltet Auto-connect bewusst ein — dann erst ab v0.2)
- Kein BLE-Peripheral-Modus
- Keine große Datenbank auf dem ESP

---

## 5. BLE State-Machine

```
IDLE
  └─[Scan start]→ SCANNING
                    ├─[Scan stop]→ IDLE
                    └─[Connect]→ CONNECTING
                                   ├─fail→ ERROR → IDLE / DISCONNECTED
                                   └─ok→ CONNECTED
                                          └─[Subscribe HR (+BAT)]→ SUBSCRIBING
                                                                    ├─fail→ ERROR
                                                                    └─ok→ READY
                                                                          ├─[Disconnect]→ DISCONNECTING → DISCONNECTED
                                                                          ├─[Link lost]→ LOST → RECONNECTING? (nur wenn Auto-reconnect ON)
                                                                          └─[Idle timeout]→ DISCONNECTING → DISCONNECTED
```

**v0.1 Defaults**

| Option | Default |
|--------|---------|
| Auto-connect | OFF |
| Auto-reconnect nach Link-Loss | OFF (oder ON nur während manueller Session — siehe unten) |
| Idle-Disconnect | ON, z. B. 15 min ohne Measurement **oder** konfigurierbar „nie“ |
| Remembered MAC | leer / zuletzt gespeichert |

**Session-Reconnect (empfohlen v0.1):**  
Während einer vom User gestarteten Session (nach Connect bis Disconnect): bei kurzem Link-Loss automatisch `RECONNECTING` (z. B. max. 3 Versuche / 30 s).  
Nach manuellem Disconnect oder Idle-Timeout: **kein** Reconnect. Gurt ist frei.

---

## 6. WebUI — Design & Seiten

### Designrichtung

Eigenständige „Lab/Instrument“-Ästhetik analog rfmonitor, aber **Herz-/Signal-Thema** (nicht 1:1 kopieren):

- Dunkler Grund, atmosphärische Gradients (kalt-rot / Graphite — **kein** generisches Purple-Glow)
- Display-Font für BPM / Brand (z. B. Syne oder ähnlich), Mono für Metadaten (IBM Plex Mono o. ä.)
- Eine klare Hero-Zone: Brand + aktueller BPM + Connection-State
- Charts als Hauptinhalt, keine Card-Orgie
- Motion sparsam: Pulse-Dot bei READY, sanfte Chart-Nachführung, State-Übergänge
- Mobil tauglich (BPM und Primärchart zuerst)

### Seiten / Tabs

| Tab | Inhalt |
|-----|--------|
| **Live** | Hero-BPM, State-Pill, KPIs, Chart-Grid |
| **Devices** | Scan-Ergebnisse, Remembered, Connect/Disconnect/Forget |
| **History** | Längere Chart-Ansichten / Zoom auf Ringbuffer (gleiche Daten) |
| **Config** | Gerätename, Hub-IP/Port, Idle-Timeout, Flags, Hinweis Auto-connect |
| **OTA** | Firmware-Upload wie Schwesterprojekte |

### Live — Layout (Soll)

```
┌─────────────────────────────────────────────────────────┐
│  HEART RATE                         ● READY   SSE live  │
│  Garmin H9 · AA:BB:… · connected 00:12:04               │
├──────────────────┬──────────────────────────────────────┤
│                  │  143                                 │
│   BPM Hero       │  BPM                                 │
│   (groß)         │  RR 418 ms · Bat 87% · −51 dBm       │
│                  │  last packet 240 ms ago               │
├──────────────────┴──────────────────────────────────────┤
│  [Disconnect]  [Reconnect]                              │
├─────────────────────────────────────────────────────────┤
│  Heart Rate (BPM)          ─ 5 min live sparkline/area  │
│  ████████████ canvas ████████████████████████████████   │
├───────────────────────┬─────────────────────────────────┤
│  RR Intervals (ms)    │  RR Scatter / Beat-to-Beat      │
│  canvas               │  canvas                         │
├───────────────────────┴─────────────────────────────────┤
│  Battery %            │  BLE RSSI (dBm)                 │
│  canvas               │  canvas                         │
├─────────────────────────────────────────────────────────┤
│  Packet strip: Zeit · BPM · RRs · Contact · Age         │
└─────────────────────────────────────────────────────────┘
```

### Charts (Muss) — alle empfangenen Daten sichtbar

| Chart | Daten | Darstellung |
|-------|-------|-------------|
| **BPM Timeline** | `hr_series` | Area/Line, Y 40–200 (auto-scale optional), Zeitraster 30s/1min |
| **RR Interval Timeline** | `rr_series` | Line oder Step, ms |
| **RR Beat-to-Beat** | letzte N RRs | Scatter oder Stem — macht Variabilität sichtbar |
| **Battery** | `battery_series` | Line (selten aktualisiert) |
| **BLE RSSI** | `rssi_series` | Line, dBm |
| **Optional Spark** | BPM | Mini-Sparkline in der Topline (wie rfmonitor) |

Zusätzlich Textuell (kein Chart, aber „alle Daten“):

- Letztes Raw-Packet: Flags, Sensor Contact, Energy, Anzahl RRs
- State-Log (kompakte Terminal-Zeile)

Charts: **Canvas selbst gezeichnet** (wie rfmonitor) — kein schweres Chart.js nötig in v0.1.  
SSE pusht Samples; Client hält denselben Ringbuffer-Spiegel (~5 min).

### Devices — Layout

```
[ Scan ] [ Stop ]

Remembered
  Garmin H9   AA:BB:…   [Connect] [Forget]

Scan results
  Polar H10   RSSI −51   HR Service   [Connect] [Remember]
  …
```

Bei verbundenem Gerät: Disconnect prominent (rot/secondary), nie versteckt.

### Config — wichtige Flags

- `idle_disconnect_sec` (0 = aus)
- `session_reconnect` (bool, Default ON)
- `auto_connect` (bool, Default OFF) — in v0.1 UI vorhanden aber disabled/"kommt in v0.2" **oder** schon verdrahtet mit Warnbanner:
  > „Auto-connect blockiert andere Geräte (Fahrrad, Handy), solange der ESP verbunden ist.“

---

## 7. API (ESP lokal)

| Endpoint | Methode | Zweck |
|----------|---------|-------|
| `/` | GET | WebUI |
| `/events` | GET | SSE (samples, state, scan) |
| `/api/status` | GET | Snapshot: state, device, last sample, KPIs |
| `/api/history` | GET | Ringbuffer-JSON (`hr`, `rr`, `battery`, `rssi`) |
| `/api/ble/scan/start` | POST | Scan starten |
| `/api/ble/scan/stop` | POST | Scan stoppen |
| `/api/ble/devices` | GET | Scan-Cache + remembered |
| `/api/ble/connect` | POST | `{ mac }` |
| `/api/ble/disconnect` | POST | Disconnect |
| `/api/ble/remember` | POST | `{ mac, name? }` |
| `/api/ble/forget` | POST | `{ mac }` |
| `/api/config` | GET/POST | Einstellungen |
| OTA-Routen | | wie Schwesterprojekte |

### SSE Events (Vorschlag)

```
event: state
data: {"state":"READY","mac":"...","name":"Garmin H9","since_ms":123456}

event: sample
data: {"t":...,"hr":143,"rr":[418,421],"bat":87,"rssi":-51,"contact":"detected"}

event: scan
data: {"devices":[...]}
```

---

## 8. Software-Struktur

Vorbild: `esp32.rfmonitor` (PlatformIO, modular).

```
esp32.heartrate/
├── platformio.ini
├── docs/PFLICHTENHEFT.md
└── src/
    ├── main.cpp
    ├── app/App.*
    ├── core/ConfigStore.*  HubClient.*  HistoryStore.*
    ├── ble/BleCentral.*    HrProfile.*   DeviceStore.*  BleState.*
    └── web/UiPages.*       (HTML/CSS/JS + Chart-Renderer)
```

| Modul | Verantwortung |
|-------|----------------|
| `BleCentral` | Scan, Connect, Disconnect, RSSI, NimBLE-Lifecycle |
| `HrProfile` | HRM parse (BPM, RR, Flags), Battery read |
| `DeviceStore` | Remembered MAC/Name in NVS |
| `HistoryStore` | Ringbuffer aller Serien |
| `HubClient` | Heartbeat + `ios` |
| `Web` | UI, REST, SSE |
| `App` | Scheduler WiFi↔BLE, Idle-Timeout, Watchdog |

### WiFi / BLE Coexistence

WLAN und BLE teilen sich die Radio-Zeit. Verbindene Session: BLE Central priorisieren, Heartbeat/SSE weiter bedienen. Scan-Fenster kurz und gesteuert (Learnings aus rfmonitor).

---

## 9. Hardware

| Item | Hinweis |
|------|---------|
| ESP32-WROOM-32 / DevKit | empfohlen |
| USB-Netzteil | ausreichend |
| Garmin H9 / Polar H10 | Testgerät |
| Keine Extra-Sensoren | — |

Portal-SSID Vorschlag: `ESP-HR-Setup`  
`fwType`: `heartrate`  
Firmware-Bin: `heartrate.<semver>.esp32.bin`

---

## 10. Versionen

### v0.1 — erster lauffähiger Baustein

- Scan / Connect / Disconnect / Remember / Forget
- HR + RR + Battery + RSSI
- State-Machine + Session-Reconnect
- Idle-Disconnect
- WebUI Live mit **allen Charts**
- Hub-IOs
- Auto-connect **OFF** (Flag vorbereitet)

### v0.2

- Auto-connect (opt-in) + Preferred Device
- Trainingsmodus
- CSV-Export
- History-Tab verfeinern

### v0.3

- Weitere Fitness-Profile (CSC, Power)
- Multi-Device (soweit Stack erlaubt)
- Generisches Profil-Plugin

---

## 11. Abnahmekriterien v0.1

1. Nach Boot: kein automatisches Verbinden mit dem Gurt.
2. Scan listet H9/H10; Connect liefert BPM in der UI < 3 s nach erstem Packet.
3. RR-Intervalle erscheinen im RR-Chart, sofern der Sensor sie sendet.
4. Disconnect in der UI → Gurt innerhalb weniger Sekunden wieder von Handy/Fahrrad koppelbar.
5. Remember überlebt Reboot; Connect weiterhin nur manuell.
6. Charts zeigen BPM, RR, Battery, RSSI ohne Seiten-Reload (SSE).
7. Hub erhält `heart_rate` / `ble_state` / … über Heartbeat-`ios`.
8. Idle-Timeout trennt Verbindung wie konfiguriert.

---

## 12. Offene Punkte

- Finaler Projektname (`heartrate` vs `blefit`)
- Exakte Idle-Default-Zeit (Vorschlag: 900 s)
- Ob Battery als Notify oder periodisches Read
- Ob v0.1 Auto-connect-UI schon schaltbar oder nur Platzhalter
- Chart-Bibliothek vs. Custom-Canvas (Empfehlung: Custom wie rfmonitor)
