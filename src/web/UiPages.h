#pragma once
#include <Arduino.h>

const char PAGE_MAIN[] PROGMEM = R"HRUI(<!DOCTYPE html>
<html lang="de"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="Cache-Control" content="no-store">
<title>Heart Rate</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600&family=Syne:wght@600;700;800&display=swap" rel="stylesheet">
<style>
:root{
  --bg:#0c0a0b;--bg2:#141012;--panel:#1a1416;--line:#2e2226;
  --ink:#f3e8ea;--muted:#9a8088;--accent:#e11d48;--accent2:#fb7185;
  --ok:#34d399;--warn:#fbbf24;--bad:#f87171;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:
 radial-gradient(900px 480px at 8% -8%,rgba(225,29,72,.18),transparent 55%),
 radial-gradient(700px 420px at 100% 0%,rgba(80,20,35,.35),transparent 50%),
 var(--bg);
 color:var(--ink);font:14px/1.45 "IBM Plex Mono",ui-monospace,monospace;min-height:100vh}
.shell{max-width:1180px;margin:0 auto;padding:0 16px 48px}
.brand{padding:28px 0 6px}
.brand-mark{font-family:Syne,sans-serif;font-weight:800;font-size:clamp(2rem,5vw,3.1rem);
 letter-spacing:-.03em;line-height:.95}
.brand-mark span{color:var(--accent)}
.brand-sub{color:var(--muted);margin-top:8px;max-width:40rem;font-size:13px}
.topline{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-top:12px}
.chip{border:1px solid var(--line);background:rgba(26,20,22,.9);padding:5px 10px;font-size:11px;color:var(--muted)}
.chip b{color:var(--ink);font-weight:600}
.live{display:inline-flex;align-items:center;gap:8px;margin-left:auto}
.pulse{width:8px;height:8px;border-radius:50%;background:var(--bad)}
.pulse.on{background:var(--ok);box-shadow:0 0 0 0 rgba(52,211,153,.45);animation:ping 1.5s infinite}
@keyframes ping{0%{box-shadow:0 0 0 0 rgba(52,211,153,.45)}70%{box-shadow:0 0 0 8px transparent}100%{box-shadow:0 0 0 0 transparent}}
.beat{display:inline-block;transform-origin:center;color:var(--accent)}
.beat.go{animation:thump .9s ease-in-out infinite}
@keyframes thump{0%,100%{transform:scale(1)}35%{transform:scale(1.18)}55%{transform:scale(.96)}}
nav{display:flex;gap:2px;overflow:auto;border-bottom:1px solid var(--line);margin:18px 0 20px}
.tab{appearance:none;border:0;background:transparent;color:var(--muted);padding:12px 14px;cursor:pointer;
 font:inherit;border-bottom:2px solid transparent;white-space:nowrap}
.tab:hover{color:var(--ink)}.tab.active{color:var(--accent2);border-bottom-color:var(--accent)}
.pane{display:none}.pane.active{display:block}
.hero{display:grid;grid-template-columns:1.1fr 1fr;gap:16px;margin-bottom:16px}
@media(max-width:820px){.hero{grid-template-columns:1fr}}
.bpm-wrap{background:linear-gradient(165deg,rgba(225,29,72,.14),transparent 42%),var(--panel);
 border:1px solid var(--line);padding:22px 20px;min-height:220px;display:flex;flex-direction:column;justify-content:center}
.bpm-num{font-family:Syne,sans-serif;font-weight:800;font-size:clamp(4.2rem,12vw,6.5rem);line-height:.9;letter-spacing:-.04em}
.bpm-unit{color:var(--muted);font-size:13px;letter-spacing:.12em;text-transform:uppercase;margin-top:6px}
.bpm-meta{margin-top:16px;color:var(--muted);font-size:12px;display:grid;gap:4px}
.kpi-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}
.kpi-grid.wide{grid-template-columns:repeat(4,1fr)}
@media(max-width:900px){.kpi-grid.wide{grid-template-columns:repeat(2,1fr)}}
.kpi{background:var(--panel);border:1px solid var(--line);padding:14px}
.kpi .v{font-family:Syne,sans-serif;font-size:1.45rem;font-weight:700;color:var(--accent2);line-height:1}
.kpi .k{margin-top:6px;font-size:10px;color:var(--muted);text-transform:uppercase;letter-spacing:.07em}
.panel{background:var(--panel);border:1px solid var(--line);padding:14px;margin-bottom:12px}
.panel h3{font-family:Syne,sans-serif;font-size:12px;font-weight:700;letter-spacing:.05em;
 text-transform:uppercase;color:var(--muted);margin-bottom:10px;display:flex;justify-content:space-between;gap:8px;align-items:baseline}
.panel h3 .cap{font-family:"IBM Plex Mono",monospace;font-weight:500;letter-spacing:0;text-transform:none;color:var(--muted);font-size:10px}
canvas.chart{width:100%;height:160px;display:block;background:#090708;border:1px solid var(--line)}
canvas.chart.tall{height:220px}
.scale{display:flex;justify-content:space-between;gap:10px;margin-top:6px;font-size:11px;color:var(--muted)}
.scale b{color:var(--ink);font-weight:600}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
@media(max-width:800px){.grid2{grid-template-columns:1fr}}
.btn{appearance:none;border:1px solid var(--line);background:#24181c;color:var(--ink);
 padding:9px 14px;cursor:pointer;font:inherit;margin:6px 6px 0 0}
.btn:hover{border-color:var(--accent2);color:var(--accent2)}
.btn-a{background:rgba(225,29,72,.14);border-color:rgba(225,29,72,.45);color:var(--accent2)}
.btn-r{border-color:rgba(248,113,113,.45);color:var(--bad)}
.btn:disabled{opacity:.45;cursor:not-allowed}
input,select{width:100%;background:#090708;border:1px solid var(--line);color:var(--ink);padding:8px;font:inherit}
label{display:block;color:var(--muted);font-size:11px;margin:10px 0 4px;text-transform:uppercase;letter-spacing:.05em}
.row{display:flex;gap:10px;flex-wrap:wrap;align-items:end}
.row>div{flex:1;min-width:140px}
.warn{border:1px solid rgba(251,191,36,.35);background:rgba(251,191,36,.08);color:var(--warn);padding:10px;margin-bottom:12px;font-size:12px}
.state{display:inline-flex;align-items:center;gap:6px;padding:4px 8px;border:1px solid var(--line);font-size:11px}
.state.ok{color:var(--ok);border-color:rgba(52,211,153,.4)}
.state.run{color:var(--accent2);border-color:rgba(225,29,72,.4)}
.state.bad{color:var(--bad);border-color:rgba(248,113,113,.4)}
table{width:100%;border-collapse:collapse;font-size:12px}
th,td{padding:8px 6px;border-bottom:1px solid var(--line);text-align:left;vertical-align:middle}
th{color:var(--muted);font-weight:500;text-transform:uppercase;font-size:10px;letter-spacing:.06em}
.mono{color:var(--accent2)}
.meta{color:var(--muted);font-size:12px}
.drop{border:1px dashed var(--line);padding:28px;text-align:center;cursor:pointer;color:var(--muted)}
.drop:hover{border-color:var(--accent);color:var(--ink)}
.actions{margin-top:12px}
</style></head><body>
<div class="shell">
  <header class="brand">
    <div class="brand-mark">Heart <span>Rate</span></div>
    <p class="brand-sub">BLE Fitness Gateway · manueller Connect · Live-Charts für BPM, RR, Battery und RSSI.</p>
    <div class="topline">
      <span class="chip" id="h-ver">v-</span>
      <span class="chip" id="h-board">-</span>
      <span class="chip"><b id="h-name">-</b></span>
      <span class="chip" id="h-state">IDLE</span>
      <span class="chip" id="h-link" title="BLE Reichweite">Reichweite —</span>
      <span class="chip" id="h-time">NTP …</span>
      <div class="live">
        <span class="beat" id="beat">♥</span>
        <span class="pulse" id="sse-dot"></span>
        <span id="sse-lbl" class="meta">Verbinde…</span>
      </div>
    </div>
  </header>
  <nav>
    <button class="tab active" data-tab="live">Live</button>
    <button class="tab" data-tab="devices">Devices</button>
    <button class="tab" data-tab="history">History</button>
    <button class="tab" data-tab="config">Config</button>
    <button class="tab" data-tab="ota">OTA</button>
  </nav>

  <div class="pane active" id="pane-live">
    <div class="warn" id="sess-banner" style="display:none;margin-bottom:12px"></div>
    <div class="hero">
      <div class="bpm-wrap">
        <div class="bpm-num" id="bpm">—</div>
        <div class="bpm-unit">BPM</div>
        <div class="bpm-meta">
          <div id="peer">kein Sensor</div>
          <div id="pkt">last packet —</div>
          <div id="ble-err" class="meta" style="color:var(--warn);margin-top:4px"></div>
        </div>
        <div class="actions">
          <button class="btn btn-r" id="btn-disc" onclick="disconnect()" disabled>Disconnect</button>
          <button class="btn" id="btn-re" onclick="reconnect()" disabled>Reconnect</button>
          <button class="btn" onclick="rememberCurrent()">Remember</button>
        </div>
      </div>
      <div class="kpi-grid">
        <div class="kpi"><div class="v" id="k-rr">—</div><div class="k">RR Interval</div></div>
        <div class="kpi"><div class="v" id="k-bat">—</div><div class="k">Battery</div></div>
        <div class="kpi"><div class="v" id="k-rssi">—</div><div class="k">BLE RSSI</div></div>
        <div class="kpi"><div class="v" id="k-contact">—</div><div class="k">Kontakt / Fit</div></div>
      </div>
    </div>
    <div class="panel" style="margin-bottom:16px">
      <h3>Session &amp; HRV <span class="cap" id="cap-sess">—</span></h3>
      <div class="actions" style="margin:0 0 12px 0">
        <button class="btn" id="m-rest" onclick="setMode('rest')">Rest</button>
        <button class="btn" id="m-act" onclick="setMode('activity')">Alltag</button>
        <button class="btn" id="m-train" onclick="setMode('training')">Training</button>
        <button class="btn" id="m-rec" onclick="setMode('recovery')">Recovery</button>
        <button class="btn" id="m-suggest" style="display:none" onclick="applySuggest()">Vorschlag übernehmen</button>
        <button class="btn btn-a" onclick="exportSessionJson()">Export JSON</button>
        <button class="btn" onclick="exportSessionCsv()">Export CSV</button>
        <button class="btn" onclick="sendSessionExport()">An Hub senden</button>
      </div>
      <p class="meta" id="export-hint" style="margin:0 0 10px 0">Serie lokal ~alle 10 s (bis 2 h). Export/Hub nur auf Knopfdruck — kein Dauerstream.</p>
      <div class="kpi-grid wide">
        <div class="kpi"><div class="v" id="s-dur">—</div><div class="k">Dauer</div></div>
        <div class="kpi"><div class="v" id="s-mode">—</div><div class="k">Modus</div></div>
        <div class="kpi"><div class="v" id="s-avg">—</div><div class="k">Avg BPM</div></div>
        <div class="kpi"><div class="v" id="s-min">—</div><div class="k">Min BPM</div></div>
        <div class="kpi"><div class="v" id="s-max">—</div><div class="k">Max BPM</div></div>
        <div class="kpi"><div class="v" id="s-fromrr">—</div><div class="k">BPM aus RR</div></div>
        <div class="kpi"><div class="v" id="s-rmssd">—</div><div class="k">RMSSD</div></div>
        <div class="kpi"><div class="v" id="s-sdnn">—</div><div class="k">SDNN</div></div>
        <div class="kpi"><div class="v" id="s-pnn50">—</div><div class="k">pNN50</div></div>
        <div class="kpi"><div class="v" id="s-zone">—</div><div class="k">Zone / Intensität</div></div>
        <div class="kpi"><div class="v" id="s-kcal">—</div><div class="k">kcal (Schätzung)</div></div>
        <div class="kpi"><div class="v" id="s-valid">—</div><div class="k">RR Valid %</div></div>
        <div class="kpi"><div class="v" id="s-gap">—</div><div class="k">Longest Gap</div></div>
        <div class="kpi"><div class="v" id="s-cont">—</div><div class="k">Kontinuität</div></div>
        <div class="kpi"><div class="v" id="s-recon">—</div><div class="k">Reconnects</div></div>
        <div class="kpi"><div class="v" id="s-hrvval">—</div><div class="k">HRV Validity</div></div>
        <div class="kpi"><div class="v" id="s-sdsd">—</div><div class="k">SDSD</div></div>
        <div class="kpi"><div class="v" id="s-ztime">—</div><div class="k">Zeit in Zonen</div></div>
      </div>
      <h3 style="margin-top:14px">Recovery HRR</h3>
      <div class="kpi-grid wide">
        <div class="kpi"><div class="v" id="r-start">—</div><div class="k">Start HR</div></div>
        <div class="kpi"><div class="v" id="r-e">—</div><div class="k">Elapsed</div></div>
        <div class="kpi"><div class="v" id="r-60">—</div><div class="k">HR @1 min</div></div>
        <div class="kpi"><div class="v" id="r-120">—</div><div class="k">HR @2 min</div></div>
        <div class="kpi"><div class="v" id="r-300">—</div><div class="k">HR @5 min</div></div>
        <div class="kpi"><div class="v" id="r-hrr1">—</div><div class="k">HRR 1</div></div>
        <div class="kpi"><div class="v" id="r-hrr2">—</div><div class="k">HRR 2</div></div>
        <div class="kpi"><div class="v" id="r-hrr5">—</div><div class="k">HRR 5</div></div>
      </div>
      <h3 style="margin-top:14px">Baseline / Ruhe-HF</h3>
      <div class="actions" style="margin:0 0 12px 0">
        <button class="btn btn-a" id="b-guide" onclick="startGuide()">Geführte Ruhe (4 min)</button>
        <button class="btn" id="b-guide-cancel" onclick="cancelGuide()" style="display:none">Abbrechen</button>
        <button class="btn" onclick="captureBaseline()">Jetzt speichern</button>
        <button class="btn btn-r" onclick="clearBaseline()">Löschen</button>
      </div>
      <div class="warn" id="b-guide-box" style="display:none;margin-bottom:12px"></div>
      <div class="kpi-grid wide">
        <div class="kpi"><div class="v" id="b-stored">—</div><div class="k">Ruhe-HF (NVS)</div></div>
        <div class="kpi"><div class="v" id="b-live">—</div><div class="k">Live Rest-Avg</div></div>
        <div class="kpi"><div class="v" id="b-min">—</div><div class="k">Rest Min</div></div>
        <div class="kpi"><div class="v" id="b-stab">—</div><div class="k">Stabil</div></div>
        <div class="kpi"><div class="v" id="b-pct">—</div><div class="k">%HRR (Karvonen)</div></div>
        <div class="kpi"><div class="v" id="b-dur">—</div><div class="k">Rest-Dauer</div></div>
        <div class="kpi"><div class="v" id="b-gphase">—</div><div class="k">Guide-Phase</div></div>
        <div class="kpi"><div class="v" id="b-gremain">—</div><div class="k">Guide Restzeit</div></div>
      </div>
      <p class="meta" id="s-hint" style="margin-top:10px">Nach Training → Recovery tippen für HRR 1/2/5.</p>
      <div class="meta" id="pkt-dbg" style="margin-top:8px;white-space:pre-wrap;font-size:11px;color:var(--muted)">Last packet —</div>
    </div>
    <div class="panel"><h3>Heart Rate <span class="cap" id="cap-hr">Ringpuffer</span></h3><canvas class="chart tall" id="c-hr" width="1100" height="220"></canvas><div class="scale" id="sc-hr"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
    <div class="grid2">
      <div class="panel"><h3>RR Intervals <span class="cap" id="cap-rr"></span></h3><canvas class="chart" id="c-rr" width="540" height="160"></canvas><div class="scale" id="sc-rr"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
      <div class="panel"><h3>RR Beat-to-Beat <span class="cap" id="cap-rrbt"></span></h3><canvas class="chart" id="c-rrbt" width="540" height="160"></canvas><div class="scale" id="sc-rrbt"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
    </div>
    <div class="grid2">
      <div class="panel"><h3>Battery <span class="cap" id="cap-bat"></span></h3><canvas class="chart" id="c-bat" width="540" height="160"></canvas><div class="scale" id="sc-bat"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
      <div class="panel"><h3>BLE RSSI <span class="cap" id="cap-rssi"></span></h3><canvas class="chart" id="c-rssi" width="540" height="160"></canvas><div class="scale" id="sc-rssi"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
    </div>
  </div>

  <div class="pane" id="pane-devices">
    <div class="warn">Kein Auto-Connect standardmäßig — sonst finden Fahrrad/Handy den Gurt nicht. Nach Gebrauch Disconnect.</div>
    <div class="panel">
      <h3>Remembered</h3>
      <div id="rem-box" class="meta">—</div>
      <div class="actions">
        <button class="btn btn-a" onclick="connectRem()">Connect</button>
        <button class="btn btn-r" onclick="forget()">Forget</button>
      </div>
    </div>
    <div class="panel">
      <h3>Scan</h3>
      <div class="actions">
        <button class="btn btn-a" id="btn-scan" onclick="scanStart()">Scan</button>
        <button class="btn" onclick="scanStop()">Stop</button>
        <label class="meta" style="display:inline-flex;align-items:center;gap:6px;margin-left:8px">
          <input type="checkbox" id="scan-all" onchange="loadDevices()"> alle BLE-Geräte
        </label>
      </div>
      <p class="meta" id="scan-filter-hint" style="margin:8px 0 0">Standard: nur HR-Service, Polar/HR-Namen oder gemerktes Gerät.</p>
      <table style="margin-top:12px">
        <thead><tr><th>Name</th><th>MAC</th><th>RSSI</th><th>HR</th><th></th></tr></thead>
        <tbody id="dev-rows"></tbody>
      </table>
    </div>
  </div>

  <div class="pane" id="pane-history">
    <div class="panel">
      <h3>Gespeicherte Sessions <span class="cap" id="cap-sess-arch">NVS · max 12</span></h3>
      <p class="meta" id="sess-hint">Wird beim Disconnect gespeichert (≥1 min, ≥30 Samples). Mit NTP: lokale Endzeit.</p>
      <div class="actions" style="margin:8px 0">
        <button class="btn" onclick="loadSessions()">Refresh</button>
        <button class="btn btn-r" onclick="clearSessions()">Alle löschen</button>
      </div>
      <table>
        <thead><tr><th>#</th><th>Ende</th><th>Dauer</th><th>Avg</th><th>Min/Max</th><th>RMSSD</th><th>kcal</th><th>Peer</th></tr></thead>
        <tbody id="sess-rows"><tr><td colspan="8" class="meta">—</td></tr></tbody>
      </table>
    </div>
    <div class="panel"><h3>Heart Rate (Ringbuffer)</h3><canvas class="chart tall" id="h-hr" width="1100" height="220"></canvas><div class="scale" id="sc-h-hr"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
    <div class="grid2">
      <div class="panel"><h3>RR</h3><canvas class="chart" id="h-rr" width="540" height="160"></canvas><div class="scale" id="sc-h-rr"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
      <div class="panel"><h3>RSSI</h3><canvas class="chart" id="h-rssi" width="540" height="160"></canvas><div class="scale" id="sc-h-rssi"><span>min <b>—</b></span><span>max <b>—</b></span></div></div>
    </div>
    <button class="btn" onclick="loadHistory()">Refresh Charts</button>
  </div>

  <div class="pane" id="pane-config">
    <div class="warn">Auto-connect blockiert andere Geräte (Fahrrad, Handy), solange der ESP verbunden ist. Default: aus.</div>
    <div class="panel">
      <h3>Einstellungen</h3>
      <div class="row">
        <div><label>Gerätename</label><input id="cfg-name"></div>
        <div><label>Hub Host</label><input id="cfg-host"></div>
        <div><label>Hub Port</label><input id="cfg-port" type="number"></div>
      </div>
      <div class="row">
        <div><label>Idle-Disconnect (s, 0=aus)</label><input id="cfg-idle" type="number"></div>
        <div><label>Heartbeat (s)</label><input id="cfg-hb" type="number"></div>
        <div><label>Watchdog (s)</label><input id="cfg-wd" type="number"></div>
      </div>
      <div class="row">
        <div><label>Hub aktiv</label><select id="cfg-hub"><option value="1">ja</option><option value="0">nein</option></select></div>
        <div><label>Session-Reconnect</label><select id="cfg-src"><option value="1">ja</option><option value="0">nein</option></select></div>
        <div><label>Auto-Connect</label><select id="cfg-ac"><option value="0">nein (empfohlen)</option><option value="1">ja</option></select></div>
      </div>
      <div class="row">
        <div><label>Alter (Jahre, 0=aus)</label><input id="cfg-age" type="number" min="0" max="120"></div>
        <div><label>Gewicht (kg, 0=aus)</label><input id="cfg-weight" type="number" min="0" max="250"></div>
        <div><label>Geschlecht (kcal)</label><select id="cfg-sex"><option value="0">männlich</option><option value="1">weiblich</option></select></div>
      </div>
      <div class="row">
        <div><label>Ruhe-HF (BPM, 0=aus)</label><input id="cfg-rest" type="number" min="0" max="120"></div>
        <div><label>Zonen-Methode</label><select id="cfg-zmode">
          <option value="auto">auto (HRR wenn Ruhe-HF)</option>
          <option value="hrr">Karvonen %HRR</option>
          <option value="hrmax">%HRmax</option>
        </select></div>
        <div><label>NTP</label><select id="cfg-ntp"><option value="1">an</option><option value="0">aus</option></select></div>
      </div>
      <div class="row">
        <div><label>NTP-Server</label><input id="cfg-ntps"></div>
        <div><label>Zeitzone (POSIX TZ)</label><input id="cfg-tz" placeholder="CET-1CEST,M3.5.0,M10.5.0/3"></div>
      </div>
      <p class="meta" style="margin-top:8px">HRmax ≈ 208−0.7×Alter · Zonen: auto/%HRR/%HRmax (Schwellen 60/70/80/90) · kcal Keytel · NTP für Archive.</p>
      <div class="actions">
        <button class="btn btn-a" onclick="saveCfg()">Speichern</button>
        <button class="btn" onclick="restart()">Neustart</button>
      </div>
    </div>
  </div>

  <div class="pane" id="pane-ota">
    <div class="panel">
      <h3>Firmware OTA</h3>
      <div class="drop" id="ota-drop">Firmware .bin hierher ziehen oder klicken</div>
      <input type="file" id="ota-file" accept=".bin" style="display:none">
      <div class="meta" id="ota-msg" style="margin-top:10px"></div>
    </div>
  </div>
</div>
<script>
const MAX_HR=360, MAX_RR=720, MAX_BAT=120, MAX_RSSI=360;
const S={hr:[],rr:[],bat:[],rssi:[],remMac:'',remName:'',state:'IDLE',lastSampleT:0,pts:0};
document.querySelectorAll('.tab').forEach(t=>t.onclick=()=>{
  document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
  document.querySelectorAll('.pane').forEach(x=>x.classList.remove('active'));
  t.classList.add('active');
  document.getElementById('pane-'+t.dataset.tab).classList.add('active');
  if(t.dataset.tab==='history'){loadHistory();loadSessions()}
  if(t.dataset.tab==='devices') loadDevices();
  if(t.dataset.tab==='config') loadCfg();
});
async function jget(u){const r=await fetch(u);return r.json()}
async function jpost(u,b){const r=await fetch(u,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b||{})});return r.json()}
// Ringbuffer: length stays capped on purpose — recording continues (sliding window)
function pushPt(arr,ts,v,max){
  arr.push({ts,v});
  if(arr.length>max) arr.splice(0, arr.length-max);
}
function spanLabel(arr,max){
  if(!arr.length) return 'leer';
  const span=Math.max(0, (arr[arr.length-1].ts||0)-(arr[0].ts||0));
  const full=arr.length>=max;
  return arr.length+'/'+max+(full?' sliding':'')+' · '+span+'s';
}
function setScale(id,min,max,unit){
  const el=document.getElementById(id); if(!el) return;
  const u=unit||'';
  el.innerHTML='<span>min <b>'+min+u+'</b></span><span>max <b>'+max+u+'</b></span>';
}
function niceRange(vals, pad, floor, ceil, minSpan){
  let lo=Math.min(...vals), hi=Math.max(...vals);
  const p=pad!=null?pad:0;
  lo-=p; hi+=p;
  if(minSpan!=null && (hi-lo)<minSpan){
    const mid=(lo+hi)/2;
    lo=mid-minSpan/2; hi=mid+minSpan/2;
  }
  if(floor!=null) lo=Math.max(lo, floor);
  if(ceil!=null) hi=Math.min(hi, ceil);
  if(lo>=hi){lo-=1; hi+=1}
  return {min:lo, max:hi};
}
function drawLine(canvas,pts,color,opts){
  const ctx=canvas.getContext('2d'); const w=canvas.width,h=canvas.height;
  ctx.clearRect(0,0,w,h);
  ctx.fillStyle='#090708'; ctx.fillRect(0,0,w,h);
  if(!pts||pts.length<1){ctx.fillStyle='#9a8088';ctx.font='12px IBM Plex Mono';ctx.fillText('keine Daten',12,24);return null}
  const vals=pts.map(p=>p.v);
  let min, maxV;
  if(opts&&opts.auto){
    const r=niceRange(vals, opts.padY||0, opts.floor, opts.ceil, opts.minSpan||10);
    min=r.min; maxV=r.max;
  } else {
    min=opts&&opts.min!=null?opts.min:Math.min(...vals);
    maxV=opts&&opts.max!=null?opts.max:Math.max(...vals);
    if(min===maxV){min-=1;maxV+=1}
  }
  const padT=12, padB=14, padL=36, padR=10;
  const t0=pts[0].ts, t1=pts[pts.length-1].ts;
  const useTime=(t1-t0)>=2;
  const xAt=(p,i)=> useTime
    ? padL+(w-padL-padR)*((p.ts-t0)/Math.max(t1-t0,1))
    : padL+(w-padL-padR)*i/Math.max(pts.length-1,1);
  const yAt=v=>padT+(h-padT-padB)*(1-(v-min)/(maxV-min));
  // grid + Y labels
  ctx.font='11px IBM Plex Mono';
  for(let i=0;i<4;i++){
    const frac=i/3;
    const y=padT+(h-padT-padB)*frac;
    const val=maxV-(maxV-min)*frac;
    ctx.strokeStyle='#2e2226'; ctx.lineWidth=1;
    ctx.beginPath(); ctx.moveTo(padL,y); ctx.lineTo(w-padR,y); ctx.stroke();
    ctx.fillStyle='#9a8088';
    ctx.fillText(String(Math.round(val)), 4, y+3);
  }
  const lastX=xAt(pts[pts.length-1],pts.length-1);
  ctx.strokeStyle='rgba(225,29,72,.35)'; ctx.beginPath(); ctx.moveTo(lastX,padT); ctx.lineTo(lastX,h-padB); ctx.stroke();
  if(opts&&opts.fill){
    const g=ctx.createLinearGradient(0,0,0,h);
    g.addColorStop(0,color+'55'); g.addColorStop(1,color+'00');
    ctx.beginPath();
    pts.forEach((p,i)=>{const x=xAt(p,i), y=yAt(p.v); i?ctx.lineTo(x,y):ctx.moveTo(x,y)});
    ctx.lineTo(lastX,h-padB); ctx.lineTo(xAt(pts[0],0),h-padB); ctx.closePath(); ctx.fillStyle=g; ctx.fill();
  }
  ctx.beginPath(); ctx.strokeStyle=color; ctx.lineWidth=2;
  pts.forEach((p,i)=>{const x=xAt(p,i), y=yAt(p.v); i?ctx.lineTo(x,y):ctx.moveTo(x,y)});
  ctx.stroke();
  ctx.fillStyle=color; ctx.beginPath(); ctx.arc(lastX,yAt(pts[pts.length-1].v),3,0,Math.PI*2); ctx.fill();
  const dataMin=Math.min(...vals), dataMax=Math.max(...vals);
  return {min:dataMin, max:dataMax, axisMin:min, axisMax:maxV};
}
function drawStems(canvas,pts,color){
  const ctx=canvas.getContext('2d'); const w=canvas.width,h=canvas.height;
  ctx.clearRect(0,0,w,h); ctx.fillStyle='#090708'; ctx.fillRect(0,0,w,h);
  const n=Math.min(pts.length,80); if(!n){ctx.fillStyle='#9a8088';ctx.font='12px IBM Plex Mono';ctx.fillText('keine RR',12,24);return null}
  const slice=pts.slice(pts.length-n);
  const vals=slice.map(p=>p.v);
  const r=niceRange(vals, 8, null, null, 20);
  let min=r.min, max=r.max;
  const padT=12, padB=14, padL=40, padR=8;
  ctx.font='11px IBM Plex Mono';
  for(let i=0;i<4;i++){
    const frac=i/3;
    const y=padT+(h-padT-padB)*frac;
    const val=max-(max-min)*frac;
    ctx.strokeStyle='#2e2226'; ctx.beginPath(); ctx.moveTo(padL,y); ctx.lineTo(w-padR,y); ctx.stroke();
    ctx.fillStyle='#9a8088'; ctx.fillText(String(Math.round(val)), 2, y+3);
  }
  slice.forEach((p,i)=>{
    const x=padL+(w-padL-padR)*i/Math.max(n-1,1);
    const y=padT+(h-padT-padB)*(1-(p.v-min)/(max-min));
    ctx.strokeStyle=color; ctx.beginPath(); ctx.moveTo(x,h-padB); ctx.lineTo(x,y); ctx.stroke();
    ctx.fillStyle=color; ctx.beginPath(); ctx.arc(x,y,2.2,0,Math.PI*2); ctx.fill();
  });
  return {min:Math.min(...vals), max:Math.max(...vals)};
}
function redraw(){
  const hr=drawLine(document.getElementById('c-hr'),S.hr,'#e11d48',{auto:1,padY:4,floor:30,ceil:220,minSpan:12,fill:1});
  const rr=drawLine(document.getElementById('c-rr'),S.rr,'#fb7185',{auto:1,padY:10,minSpan:40});
  const rrbt=drawStems(document.getElementById('c-rrbt'),S.rr,'#fb7185');
  const bat=drawLine(document.getElementById('c-bat'),S.bat,'#34d399',{auto:1,padY:2,floor:0,ceil:100,minSpan:5});
  const rssi=drawLine(document.getElementById('c-rssi'),S.rssi,'#fbbf24',{auto:1,padY:3,floor:-105,ceil:-20,minSpan:10});
  if(hr) setScale('sc-hr', Math.round(hr.min), Math.round(hr.max), ' BPM');
  if(rr) setScale('sc-rr', Math.round(rr.min), Math.round(rr.max), ' ms');
  if(rrbt) setScale('sc-rrbt', Math.round(rrbt.min), Math.round(rrbt.max), ' ms');
  if(bat) setScale('sc-bat', Math.round(bat.min), Math.round(bat.max), ' %');
  if(rssi) setScale('sc-rssi', Math.round(rssi.min), Math.round(rssi.max), ' dBm');
  const set=(id,arr,max)=>{const el=document.getElementById(id); if(el) el.textContent=spanLabel(arr,max)};
  set('cap-hr',S.hr,MAX_HR); set('cap-rr',S.rr,MAX_RR); set('cap-rrbt',S.rr,MAX_RR);
  set('cap-bat',S.bat,MAX_BAT); set('cap-rssi',S.rssi,MAX_RSSI);
}
function contactTxt(c, supported){
  if(supported===false) return 'n/a';
  return c===1?'detected':c===2?'not detected':'unknown';
}
function applyStatus(d){
  document.getElementById('h-ver').textContent='v'+(d.version||'-');
  document.getElementById('h-board').textContent=d.boardLabel||d.board||'-';
  document.getElementById('h-name').textContent=d.name||'-';
  const tEl=document.getElementById('h-time');
  if(tEl){
    if(!d.enableNtp) tEl.textContent='NTP aus';
    else if(d.ntpOk&&d.time) tEl.textContent=d.time;
    else tEl.textContent='NTP …';
  }
  const b=d.ble||{};
  S.state=b.state||'IDLE';
  let st=S.state;
  if(b.reconnecting||(b.session&&(S.state==='LOST'||S.state==='RECONNECTING'))){
    st=S.state+' · recon';
    if(b.reconnectRemainS!=null) st+=' '+b.reconnectRemainS+'s';
  }
  document.getElementById('h-state').textContent=st;
  const link=b.link||{};
  const linkEl=document.getElementById('h-link');
  if(linkEl){
    const q=link.quality||'n/a';
    const now=(link.rssiNow!=null)?link.rssiNow:((b.sample&&b.sample.rssi!=null)?b.sample.rssi:null);
    let t='Reichweite '+q;
    if(now!=null) t+=' '+now+' dBm';
    if(link.rssiMin!=null && link.rssiMax!=null && link.rssiSamples) t+=' · '+link.rssiMin+'…'+link.rssiMax;
    if(link.lossCount) t+=' · loss '+link.lossCount;
    linkEl.textContent=t;
    linkEl.title='RSSI avg '+(link.rssiAvg!=null?link.rssiAvg:'—')+' · Reconnects OK '+(link.reconnectOk||0);
  }
  const ready=S.state==='READY';
  document.getElementById('btn-disc').disabled=!ready && S.state!=='CONNECTING' && S.state!=='RECONNECTING' && S.state!=='LOST';
  document.getElementById('btn-re').disabled=!S.remMac;
  document.getElementById('beat').classList.toggle('go',ready);
  S.remMac=d.rememberedMac||''; S.remName=d.rememberedName||'';
  const errEl=document.getElementById('ble-err');
  if(errEl){
    if(b.error && (S.state==='ERROR'||S.state==='LOST'||b.reconnecting)) errEl.textContent=b.error+(b.reconnectTries?(' · try '+b.reconnectTries):'');
    else errEl.textContent='';
  }
  const age=b.lastPacketAgeMs;
  const stale=ready && age>=0 && age>3000;
  const s=b.sample;
  if(s&&s.hr){
    document.getElementById('bpm').textContent=s.hr;
    document.getElementById('bpm').style.opacity=stale?'0.45':'1';
    document.getElementById('k-rr').textContent=(s.rr&&s.rr.length)?(s.rr[s.rr.length-1]+' ms'):'—';
    document.getElementById('k-bat').textContent=(s.bat>=0)?(s.bat+' %'):'—';
    document.getElementById('k-rssi').textContent=(s.rssi||s.rssi===0)?(s.rssi+' dBm'):'—';
    const fit=d.strapFit||'';
    let ctxt=contactTxt(s.contact, s.contactSupported);
    if(s.contactSupported===false) ctxt='n/a'+(fit?(' · fit '+fit):'');
    else if(fit) ctxt+=' · fit '+fit;
    document.getElementById('k-contact').textContent=ctxt;
    const ts=Math.floor((s.t||Date.now())/1000);
    if(!S.lastSampleT || s.t!==S.lastSampleT){
      S.lastSampleT=s.t;
      S.pts++;
      pushPt(S.hr,ts,s.hr,MAX_HR);
      if(s.rr) s.rr.forEach(v=>pushPt(S.rr,ts,v,MAX_RR));
      if(s.bat>=0) pushPt(S.bat,ts,s.bat,MAX_BAT);
      if(s.rssi||s.rssi===0) pushPt(S.rssi,ts,s.rssi,MAX_RSSI);
      redraw();
    }
  }
  document.getElementById('peer').textContent=(b.name||b.mac)?((b.name||'Sensor')+' · '+b.mac):'kein Sensor';
  let pkt=S.state;
  if(age>=0) pkt='last packet '+age+' ms · '+S.state;
  if(stale) pkt='⚠ STALE '+age+' ms · '+S.state;
  if(b.error) pkt+=' · '+b.error;
  if(S.hr.length) pkt+=' · buf '+spanLabel(S.hr,MAX_HR);
  document.getElementById('pkt').textContent=pkt;
  document.getElementById('pkt').style.color=stale?'var(--warn)':'';
  const sess=d.session||{};
  const fmtDur=s=>{s=+s||0; const m=Math.floor(s/60), r=s%60; return m+':'+String(r).padStart(2,'0')};
  document.getElementById('s-dur').textContent=sess.samples?fmtDur(sess.durationS):'—';
  document.getElementById('s-mode').textContent=sess.mode||'—';
  document.getElementById('s-avg').textContent=sess.hrAvg||'—';
  document.getElementById('s-min').textContent=sess.hrMin||'—';
  document.getElementById('s-max').textContent=sess.hrMax||'—';
  document.getElementById('s-fromrr').textContent=sess.hrFromRr||'—';
  document.getElementById('s-rmssd').textContent=sess.rmssd!=null?(sess.rmssd+' ms'):'—';
  document.getElementById('s-sdnn').textContent=sess.sdnn!=null?(sess.sdnn+' ms'):'—';
  document.getElementById('s-pnn50').textContent=sess.pnn50!=null?(sess.pnn50+' %'):'—';
  document.getElementById('s-sdsd').textContent=sess.sdsd!=null?(sess.sdsd+' ms'):'—';
  document.getElementById('s-hrvval').textContent=sess.hrvValidity||'—';
  const z=sess.zone?('Z'+sess.zone):'—';
  const by=sess.zoneBy||(d.zonesUseHrr?'hrr':'hrmax');
  let pct='';
  if(by==='hrr' && sess.pctHrr!=null) pct=' · '+sess.pctHrr+'% HRR';
  else if(sess.pctMax) pct=' · '+sess.pctMax+'% max';
  document.getElementById('s-zone').textContent=(sess.zone?z+pct:'—');
  document.getElementById('s-kcal').textContent=sess.calories!=null?sess.calories:'—';
  const zt=sess.zoneTimeS||{};
  document.getElementById('s-ztime').textContent=['z1','z2','z3','z4','z5'].map(k=>(zt[k]||0)+'s').join(' · ');
  const bt=d.beats||{};
  document.getElementById('s-valid').textContent=bt.validPct!=null?(bt.validPct+' %'):'—';
  document.getElementById('s-gap').textContent=bt.longestGapMs!=null?(bt.longestGapMs+' ms'):'—';
  const contEl=document.getElementById('s-cont');
  if(contEl){
    let c=bt.continuityPct!=null?(bt.continuityPct+' %'):'—';
    if(bt.totalGapS) c+=' · −'+bt.totalGapS+'s';
    contEl.textContent=c;
  }
  const reconEl=document.getElementById('s-recon');
  if(reconEl){
    const ln=b.link||{};
    reconEl.textContent=(ln.reconnectOk||0)+' / '+(ln.lossCount||0);
  }
  document.getElementById('cap-sess').textContent=sess.active?'live':(sess.samples?'zuletzt':'idle');
  const ban=document.getElementById('sess-banner');
  if(ban){
    const ls=d.lastSession;
    if(!sess.active && ls && ls.id){
      const fmtDur=s=>{s=+s||0; const m=Math.floor(s/60), r=s%60; return m+':'+String(r).padStart(2,'0')};
      const zt=ls.zoneTimeS||{};
      const zsum=['z1','z2','z3','z4','z5'].map((k,i)=>zt[k]?('Z'+(i+1)+' '+zt[k]+'s'):'').filter(Boolean).join(' · ')||'keine Zonen';
      ban.style.display='block';
      ban.innerHTML='Letzte Session <b>#'+ls.id+'</b>'+(ls.endedAt?(' · '+ls.endedAt):'')+
        ' · '+fmtDur(ls.durationS)+' · Avg '+ls.hrAvg+' ('+ls.hrMin+'–'+ls.hrMax+') · '+zsum+
        (ls.calories!=null?(' · '+ls.calories+' kcal'):'')+
        (ls.continuityPct!=null?(' · Kontinuität '+ls.continuityPct+'%'):'')+
        (ls.reconnectCount?(' · Reconnects '+ls.reconnectCount):'')+
        ' <button class="btn" style="margin-left:8px" onclick="document.querySelector(\'[data-tab=history]\').click()">History</button>';
    } else ban.style.display='none';
  }
  const md=(sess.mode||'').toUpperCase();
  const sug=(sess.suggestedMode||'').toUpperCase();
  [['rest','REST'],['act','ACTIVITY'],['train','TRAINING'],['rec','RECOVERY']].forEach(([id,name])=>{
    const el=document.getElementById('m-'+id);
    if(el) el.classList.toggle('btn-a', md===name);
  });
  const sugBtn=document.getElementById('m-suggest');
  if(sugBtn){
    const show=sess.active && sug && sug!==md && sug!=='IDLE';
    sugBtn.style.display=show?'inline-block':'none';
    if(show) sugBtn.textContent='→ '+sug.charAt(0)+sug.slice(1).toLowerCase();
    sugBtn.dataset.mode=(sess.suggestedMode||'').toLowerCase();
  }
  const rec=sess.recovery||{};
  document.getElementById('r-start').textContent=rec.startHr||'—';
  document.getElementById('r-e').textContent=rec.elapsedS!=null?fmtDur(rec.elapsedS):'—';
  document.getElementById('r-60').textContent=rec.hr60>=0?rec.hr60:'—';
  document.getElementById('r-120').textContent=rec.hr120>=0?rec.hr120:'—';
  document.getElementById('r-300').textContent=rec.hr300>=0?rec.hr300:'—';
  document.getElementById('r-hrr1').textContent=rec.hr60>=0?rec.hrr1:'—';
  document.getElementById('r-hrr2').textContent=rec.hr120>=0?rec.hrr2:'—';
  document.getElementById('r-hrr5').textContent=rec.hr300>=0?rec.hrr5:'—';
  const bl=sess.baseline||{};
  document.getElementById('b-stored').textContent=bl.restingHr||d.restingHr||'—';
  document.getElementById('b-live').textContent=bl.liveRestHr||'—';
  document.getElementById('b-min').textContent=bl.restMin||'—';
  document.getElementById('b-stab').textContent=bl.quiet?'ruhig':(bl.stable?'stabil':(bl.ready?'…':'warmup'));
  document.getElementById('b-pct').textContent=sess.pctHrr!=null&&(bl.restingHr||d.restingHr)?(sess.pctHrr+' %'):'—';
  document.getElementById('b-dur').textContent=bl.restDurationS!=null?fmtDur(bl.restDurationS):'—';
  const gu=bl.guide||{};
  document.getElementById('b-gphase').textContent=gu.phase||'—';
  document.getElementById('b-gremain').textContent=gu.active?fmtDur(gu.remainS):'—';
  const gbox=document.getElementById('b-guide-box');
  const gcancel=document.getElementById('b-guide-cancel');
  const gstart=document.getElementById('b-guide');
  if(gbox){
    if(gu.phase==='SETTLE'||gu.phase==='MEASURE'||gu.active){
      gbox.style.display='block';
      gbox.textContent=(gu.message||'Guided Baseline')+' · '+fmtDur(gu.elapsedS||0)+' / '+fmtDur(gu.totalS||240)+' · '+
        (gu.progressPct||0)+'%'+(bl.quiet?' · ruhig':(bl.stable?' · stabil':''));
      if(gcancel) gcancel.style.display='inline-block';
      if(gstart) gstart.disabled=true;
    } else if(gu.phase==='DONE'){
      gbox.style.display='block';
      gbox.textContent=(gu.message||'Fertig')+' · Ruhe-HF '+(bl.restingHr||d.restingHr||'—');
      if(gcancel) gcancel.style.display='none';
      if(gstart) gstart.disabled=false;
    } else if(gu.phase==='FAILED'){
      gbox.style.display='block';
      gbox.textContent=gu.message||'Fehlgeschlagen';
      if(gcancel) gcancel.style.display='none';
      if(gstart) gstart.disabled=false;
    } else {
      gbox.style.display='none';
      if(gcancel) gcancel.style.display='none';
      if(gstart) gstart.disabled=false;
    }
  }
  const ageOk=d.userAge>0, wOk=d.userWeightKg>0, rOk=(bl.restingHr||d.restingHr)>0;
  let hint='Rest: echte Ruhe/HRV · Alltag: Haushalt/Zonen · Training: Workout · Recovery: HRR.';
  if(gu.active) hint='Guided Baseline läuft — still bleiben. Soft-Auto Alltag ist pausiert.';
  else if(sess.autoSwitchedRest) hint='Auto → Rest (ruhig) — Baseline kann sich absenken. '+hint;
  else if(sess.autoSwitched) hint='Auto → Alltag (HF länger über Ruhe+12). '+hint;
  else if(sug && sug!==md && sug!=='IDLE') hint='Vorschlag: '+sug+'. '+hint;
  if(!ageOk) hint+=' Alter in Config für Zone/%HRR.';
  else if(!rOk) hint+=' Ruhe-HF: „Geführte Ruhe“ oder Config.';
  else if(rOk && sess.hrAvg && sess.hrAvg < (bl.restingHr||d.restingHr)-5)
    hint+=' Ruhe-HF wirkt hoch (live Avg '+(sess.hrAvg)+' < '+(bl.restingHr||d.restingHr)+') — Rest/geführte Ruhe empfohlen.';
  else if(!wOk) hint+=' Gewicht in Config für kcal.';
  else hint+=' HRmax≈'+(sess.hrMaxEst||Math.round(208-0.7*d.userAge))+' · Ruhe '+(bl.restingHr||d.restingHr)+' · Zonen '+(sess.zoneBy||(d.zonesUseHrr?'hrr':'hrmax'));
  if(bl.autoSaved) hint+=' · Baseline auto';
  if(bl.refined) hint+=' · Baseline verfeinert';
  if(bt.notifGaps) hint+=' · gaps '+bt.notifGaps;
  if(bt.continuityPct!=null) hint+=' · Kontinuität '+bt.continuityPct+'%';
  if(d.seriesPoints!=null) hint+=' · Serie '+d.seriesPoints+' Pkt/'+(d.seriesIntervalS||'?')+'s';
  document.getElementById('s-hint').textContent=hint;
  const eh=document.getElementById('export-hint');
  if(eh && !eh.dataset.locked){
    eh.textContent='Serie lokal ~alle '+(d.seriesIntervalS||10)+' s · '+
      (d.seriesPoints||0)+' Punkte. Export/Hub nur auf Knopfdruck — kein Dauerstream.';
  }
  const lp=d.lastPacket||{};
  if(lp.rrCount){
    let line='Last packet  HR '+lp.hr+'  RR×'+lp.rrCount+'\n';
    line+='raw: '+(lp.rrRaw||[]).join(', ')+'\n';
    line+='ms:  '+(lp.rrMs||[]).join(', ')+'\n';
    line+='qty: '+(lp.quality||[]).join(', ');
    document.getElementById('pkt-dbg').textContent=line;
  }
}
async function loadHistory(){
  const h=await jget('/api/history');
  S.hr=(h.hr||[]).map(p=>({ts:p.ts,v:p.v}));
  S.rr=(h.rr||[]).map(p=>({ts:p.ts,v:p.v}));
  S.bat=(h.battery||[]).map(p=>({ts:p.ts,v:p.v}));
  S.rssi=(h.rssi||[]).map(p=>({ts:p.ts,v:p.v}));
  redraw();
  const hr=drawLine(document.getElementById('h-hr'),S.hr,'#e11d48',{auto:1,padY:4,floor:30,ceil:220,minSpan:12,fill:1});
  const rr=drawLine(document.getElementById('h-rr'),S.rr,'#fb7185',{auto:1,padY:10,minSpan:40});
  const rssi=drawLine(document.getElementById('h-rssi'),S.rssi,'#fbbf24',{auto:1,padY:3,floor:-105,ceil:-20,minSpan:10});
  if(hr) setScale('sc-h-hr', Math.round(hr.min), Math.round(hr.max), ' BPM');
  if(rr) setScale('sc-h-rr', Math.round(rr.min), Math.round(rr.max), ' ms');
  if(rssi) setScale('sc-h-rssi', Math.round(rssi.min), Math.round(rssi.max), ' dBm');
}
async function loadSessions(){
  const d=await jget('/api/sessions');
  const tb=document.getElementById('sess-rows');
  const fmtDur=s=>{s=+s||0; const m=Math.floor(s/60), r=s%60; return m+':'+String(r).padStart(2,'0')};
  document.getElementById('cap-sess-arch').textContent=(d.count||0)+'/'+(d.max||64)+' · LittleFS';
  const list=d.sessions||[];
  if(!list.length){tb.innerHTML='<tr><td colspan="8" class="meta">noch keine Sessions</td></tr>';return}
  tb.innerHTML='';
  list.forEach(s=>{
    const tr=document.createElement('tr');
    const zt=s.zoneTimeS||{};
    const tip='RMSSD '+s.rmssd+' · kcal '+s.calories+' · Z '+[1,2,3,4,5].map(i=>zt['z'+i]||0).join('/')+(s.hrr1?(' · HRR1 '+s.hrr1):'');
    tr.title=tip;
    tr.style.cursor='pointer';
    tr.innerHTML='<td>'+s.id+'</td><td>'+(s.endedAt||'—')+'</td><td>'+fmtDur(s.durationS)+'</td><td>'+(s.hrAvg||'—')+
      '</td><td>'+(s.hrMin||'—')+'/'+(s.hrMax||'—')+'</td><td>'+(s.rmssd!=null?s.rmssd:'—')+
      '</td><td>'+(s.calories!=null?s.calories:'—')+'</td><td>'+(s.peer||'—')+'</td>';
    tr.onclick=()=>{
      const z=['z1','z2','z3','z4','z5'].map((k,i)=>(zt[k]||0)+'s').join(' · ');
      alert('Session #'+s.id+'\nEnde: '+(s.endedAt||'—')+'\nDauer: '+fmtDur(s.durationS)+
        '\nHR: '+s.hrAvg+' avg · '+s.hrMin+'–'+s.hrMax+
        '\nRMSSD: '+s.rmssd+' · SDNN: '+s.sdnn+
        '\nkcal: '+s.calories+'\nZonen: '+z+
        (s.hrr1||s.hrr2||s.hrr5?('\nHRR: '+s.hrr1+' / '+s.hrr2+' / '+s.hrr5):'')+
        (s.continuityPct!=null?('\nKontinuität: '+s.continuityPct+'%'):'')+
        (s.reconnectCount!=null?('\nReconnects: '+s.reconnectCount):'')+
        (s.rssiMin||s.rssiAvg?('\nRSSI: min '+s.rssiMin+' / avg '+s.rssiAvg):'')+
        '\nPeer: '+(s.peer||'—'));
    };
    tb.appendChild(tr);
  });
}
async function clearSessions(){
  if(!confirm('Alle gespeicherten Sessions löschen?')) return;
  await jpost('/api/sessions/clear',{});
  loadSessions();
}
function isLikelyHrDevice(x, remMac){
  if(x.hrService) return true;
  const mac=(x.mac||'').toLowerCase();
  if(remMac && mac===remMac.toLowerCase()) return true;
  const n=(x.name||'').toLowerCase();
  if(!n) return false;
  return /polar|h[79]\b|heart|hr[- ]?sensor|wahoo|garmin|tickr|coospo|magene|decathlon|bpm|puls/.test(n);
}
async function loadDevices(){
  const d=await jget('/api/ble/devices');
  document.getElementById('rem-box').innerHTML=d.rememberedMac
    ? ('<b>'+(d.rememberedName||'Sensor')+'</b><br><span class="mono">'+d.rememberedMac+'</span>')
    : 'kein Gerät gespeichert';
  const showAll=!!(document.getElementById('scan-all')||{}).checked;
  const rem=d.rememberedMac||'';
  const all=(d.devices||[]).slice().sort((a,b)=>b.rssi-a.rssi);
  const list=showAll?all:all.filter(x=>isLikelyHrDevice(x, rem));
  const hint=document.getElementById('scan-filter-hint');
  if(hint){
    hint.textContent=showAll
      ? ('Alle '+all.length+' Geräte')
      : (list.length+' relevant · '+Math.max(0, all.length-list.length)+' ausgeblendet (Toggle „alle“)');
  }
  const tb=document.getElementById('dev-rows'); tb.innerHTML='';
  if(!list.length){
    const tr=document.createElement('tr');
    tr.innerHTML='<td colspan="5" class="meta">'+(all.length?'keine HR-Kandidaten — „alle BLE-Geräte“ aktivieren':'keine Scan-Treffer — Scan starten')+'</td>';
    tb.appendChild(tr);
  }
  list.forEach(x=>{
    const tr=document.createElement('tr');
    tr.innerHTML='<td>'+(x.name||'—')+'</td><td class="mono">'+x.mac+'</td><td>'+x.rssi+'</td><td>'+(x.hrService?'yes':'?')+
      '</td><td><button class="btn btn-a" data-m="'+x.mac+'" data-n="'+(x.name||'')+'">Connect</button> <button class="btn" data-rm="'+x.mac+'" data-rn="'+(x.name||'')+'">Remember</button></td>';
    tb.appendChild(tr);
  });
  tb.querySelectorAll('[data-m]').forEach(b=>b.onclick=()=>connectMac(b.dataset.m));
  tb.querySelectorAll('[data-rm]').forEach(b=>b.onclick=()=>jpost('/api/ble/remember',{mac:b.dataset.rm,name:b.dataset.rn}).then(loadDevices));
}
function scanStart(){jpost('/api/ble/scan/start',{}).then(()=>{document.getElementById('btn-scan').disabled=true; setTimeout(loadDevices,800); const t=setInterval(loadDevices,1500); setTimeout(()=>{clearInterval(t);document.getElementById('btn-scan').disabled=false},12000)})}
function scanStop(){jpost('/api/ble/scan/stop',{}).then(loadDevices)}
function connectMac(m){jpost('/api/ble/connect',{mac:m})}
function connectRem(){if(S.remMac) jpost('/api/ble/connect',{mac:S.remMac})}
function reconnect(){connectRem()}
function disconnect(){jpost('/api/ble/disconnect',{})}
function setMode(m){jpost('/api/session/mode',{mode:m})}
function exportSessionJson(){window.location='/api/session/export'}
function exportSessionCsv(){window.location='/api/session/export.csv'}
async function sendSessionExport(){
  const hint=document.getElementById('export-hint');
  if(hint){ hint.dataset.locked='1'; hint.textContent='Sende an Hub…'; }
  try{
    const r=await jpost('/api/session/export/send',{});
    if(hint) hint.textContent=r.ok?('Hub OK · '+r.bytes+' Bytes · '+r.seriesPoints+' Punkte'):('Hub Fehler HTTP '+(r.http||'?')+' · '+(r.error||''));
  }catch(e){ if(hint) hint.textContent='Senden fehlgeschlagen'; }
  setTimeout(()=>{const h=document.getElementById('export-hint'); if(h) delete h.dataset.locked;},8000);
}
function applySuggest(){const m=document.getElementById('m-suggest').dataset.mode; if(m) setMode(m)}
function startGuide(){jpost('/api/session/baseline',{action:'guide'}).then(pollStatus)}
function cancelGuide(){jpost('/api/session/baseline',{action:'cancel'}).then(pollStatus)}
function captureBaseline(){jpost('/api/session/baseline',{action:'capture'}).then(pollStatus)}
function clearBaseline(){jpost('/api/session/baseline',{action:'clear'}).then(pollStatus)}
function rememberCurrent(){jpost('/api/ble/remember',{})}
function forget(){jpost('/api/ble/forget',{}).then(loadDevices)}
async function loadCfg(){
  const c=await jget('/api/config/get');
  document.getElementById('cfg-name').value=c.deviceName||'';
  document.getElementById('cfg-host').value=c.hubHost||'';
  document.getElementById('cfg-port').value=c.hubPort||8093;
  document.getElementById('cfg-idle').value=c.idleDisconnectS||0;
  document.getElementById('cfg-hb').value=c.heartbeatIntervalS||30;
  document.getElementById('cfg-wd').value=c.watchdogS||300;
  document.getElementById('cfg-hub').value=c.enableHub?'1':'0';
  document.getElementById('cfg-src').value=c.sessionReconnect?'1':'0';
  document.getElementById('cfg-ac').value=c.autoConnect?'1':'0';
  document.getElementById('cfg-age').value=c.userAge||0;
  document.getElementById('cfg-weight').value=c.userWeightKg||0;
  document.getElementById('cfg-sex').value=c.userFemale?'1':'0';
  document.getElementById('cfg-rest').value=c.restingHr||0;
  document.getElementById('cfg-zmode').value=c.zoneMode||'auto';
  document.getElementById('cfg-ntp').value=c.enableNtp!==false?'1':'0';
  document.getElementById('cfg-ntps').value=c.ntpServer||'pool.ntp.org';
  document.getElementById('cfg-tz').value=c.tz||'CET-1CEST,M3.5.0,M10.5.0/3';
}
async function saveCfg(){
  await jpost('/api/config/save',{
    deviceName:document.getElementById('cfg-name').value,
    hubHost:document.getElementById('cfg-host').value,
    hubPort:+document.getElementById('cfg-port').value,
    idleDisconnectS:+document.getElementById('cfg-idle').value,
    heartbeatIntervalS:+document.getElementById('cfg-hb').value,
    watchdogS:+document.getElementById('cfg-wd').value,
    enableHub:document.getElementById('cfg-hub').value==='1',
    sessionReconnect:document.getElementById('cfg-src').value==='1',
    autoConnect:document.getElementById('cfg-ac').value==='1',
    userAge:+document.getElementById('cfg-age').value,
    userWeightKg:+document.getElementById('cfg-weight').value,
    userFemale:document.getElementById('cfg-sex').value==='1',
    restingHr:+document.getElementById('cfg-rest').value,
    zoneMode:document.getElementById('cfg-zmode').value,
    enableNtp:document.getElementById('cfg-ntp').value==='1',
    ntpServer:document.getElementById('cfg-ntps').value,
    tz:document.getElementById('cfg-tz').value
  });
}
function restart(){jpost('/api/system/restart',{})}
const drop=document.getElementById('ota-drop'), file=document.getElementById('ota-file');
drop.onclick=()=>file.click();
drop.ondragover=e=>{e.preventDefault();drop.style.borderColor='#e11d48'};
drop.ondragleave=()=>drop.style.borderColor='';
drop.ondrop=e=>{e.preventDefault(); if(e.dataTransfer.files[0]) upload(e.dataTransfer.files[0])};
file.onchange=()=>{if(file.files[0]) upload(file.files[0])};
async function upload(f){
  const fd=new FormData(); fd.append('firmware',f);
  document.getElementById('ota-msg').textContent='Upload…';
  const r=await fetch('/ota-upload',{method:'POST',body:fd});
  document.getElementById('ota-msg').textContent=await r.text();
}
const es=new EventSource('/events');
let sseOk=false;
es.onopen=()=>{sseOk=true;document.getElementById('sse-dot').classList.add('on');document.getElementById('sse-lbl').textContent='live'};
es.onerror=()=>{sseOk=false;document.getElementById('sse-dot').classList.remove('on');document.getElementById('sse-lbl').textContent='poll'};
es.onmessage=e=>{try{sseOk=true;document.getElementById('sse-dot').classList.add('on');document.getElementById('sse-lbl').textContent='live';applyStatus(JSON.parse(e.data))}catch(ex){}};
// Fallback: UI must keep moving even if SSE stalls (ESP single-client / connect freeze)
async function pollStatus(){try{applyStatus(await jget('/api/status'))}catch(e){}}
setInterval(()=>{if(!sseOk) pollStatus(); else if(S.state==='READY'){/* light keep-alive */}},2000);
setInterval(pollStatus,5000);
loadHistory();
pollStatus();
</script>
</body></html>)HRUI";
