# Pflichtenheft `esp32.heartrate` v0.2 — Phase 1

Erweiterung der Live-Firmware zu einer **RR-zentrierten Session-Basis**.  
Baut auf v0.1.7 auf. Kein Big-Bang: nur Fundament.

## Ziel Phase 1

1. RR intern als `rrRaw` (1/1024 s) speichern; ms nur abgeleitet  
2. Paketübergreifende Beat-Timeline (monotonic)  
3. Quality-Labels: Valid / Suspect / Artifact / Invalid  
4. Gap-Handling (Notification-Lücken) für HRV/Zonen/kcal  
5. Status/UI/Hub: Quality- und Gap-Kennzahlen + kurzes Last-Packet-Debug  

**Nicht** in Phase 1: Baseline-Historie, Poincaré/LF-HF, LittleFS-Archive, NTP/Unix-Pflicht, volle Correction-Pipeline, WebSocket.

## Datenfluss

```
H9 → BLE → HrParser (rrRaw) → BeatTimeline → SessionStats (nur Valid/NN) → SSE/UI/Hub
                              ↘ HistoryStore (Charts, ms)
```

## Quality (Initial)

| Klasse | Regel (ms) |
|--------|------------|
| Invalid | &lt; 300 oder &gt; 2000 |
| Artifact | Sprung &gt; 35 % zum Vorgänger (bei vorhandenem Vorgänger) |
| Suspect | Sprung &gt; 20 % oder &gt; 120 ms |
| Valid | sonst → `usedForHrv` |

Rohwerte bleiben unverändert; Label ist separat.

## Gaps

- `maxZoneGapMs = 2000`: bei größerem Notification-Abstand keine Zonen-/kcal-Akkumulation; Timeline neu verankern  
- HRV nur aus Valid-Beats; bei großer Lücke im Fenster Validity → Limited (Folgeversion kann strenger werden)

## API-Erweiterung (`/api/status`)

- `beats`: count, lastRrRaw/Ms, lastQuality, gapMs, longestGapMs, notifGaps, valid/suspect/artifact/invalid counts + ratios  
- `lastPacket`: hr, rrRaw[], rrMs[], quality[]  
- bestehende `session.*` HRV nutzt bevorzugt Valid-NN aus der Timeline  

## UI

- Session-Panel: Valid-%, Longest Gap, Notif-Gaps  
- kleines Debug „Last packet“ (RR raw + ms + quality)  

## Version

Firmware **0.2.15** — Session-Export: lokale HF-Serie (~10 s), Download JSON/CSV, manueller Hub-Push (`/api/session-export`).
