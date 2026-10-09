#!/usr/bin/env bash
# Collect technology signals from a target repo (evidence for INVENTORY updates).
# Usage: bash tools/audit/audit_scan.sh [/path/to/repo] [output.json]
set -euo pipefail

TARGET="${1:-.}"
OUT="${2:-}"

if [[ ! -d "$TARGET" ]]; then
  echo "not a directory: $TARGET" >&2
  exit 1
fi
TARGET="$(cd "$TARGET" && pwd)"

if ! command -v rg >/dev/null 2>&1; then
  echo "ripgrep (rg) required" >&2
  exit 1
fi

payload=$(TARGET="$TARGET" python3 - <<'PY'
import json, os, subprocess

target = os.environ["TARGET"]
patterns = [
    ("C", "NimBLE", r"NimBLE"),
    ("C", "Heart Rate GATT", r"0x180[Dd]|180[Dd]"),
    ("C", "HR Relay Peripheral", r"HrServer|HR_RELAY|NimBLEServer"),
    ("G", "SSE", r"/events|text/event-stream|EventSource"),
    ("G", "HTTP API", r"/api/|WebServer|server\.on\("),
    ("F", "WiFiManager", r"WiFiManager"),
    ("F", "Hub telemetry", r"HubClient|esp-hub|fwType"),
    ("F", "mDNS", r"ESPmDNS|MDNS\.|\bhr-"),
    ("F", "OTA", r"performOta|/ota-upload"),
    ("E", "LittleFS", r"LittleFS"),
    ("E", "NVS Config", r"ConfigStore|Preferences"),
    ("D", "PlatformIO", r"^\[env:"),
    ("D", "ArduinoJson", r"ArduinoJson"),
    ("D", "RR pipeline", r"BeatTimeline|SessionSeries|rrRaw"),
    ("H", "GitHub Actions", r"runs-on:"),
]
hits = []
for cat, label, pat in patterns:
    search_root = target
    if label == "GitHub Actions":
        wf = os.path.join(target, ".github", "workflows")
        if not os.path.isdir(wf):
            continue
        search_root = wf
    cmd = [
        "rg", "-l", "--glob", "!.git", "--glob", "!docs/audit/**",
        "--glob", "!tools/audit/**",
        "-e", pat, search_root,
    ]
    try:
        out = subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
        out = e.output or ""
    files = [ln.strip() for ln in out.splitlines() if ln.strip()][:20]
    if not files:
        continue
    sample_parts = []
    for f in files:
        sample_parts.append(f[len(target) + 1 :] if f.startswith(target + "/") else f)
    hits.append(
        {
            "category": cat,
            "technology": label,
            "file_count": len(files),
            "sample": ";".join(sample_parts)[:500],
        }
    )
# Filename-based signal if content search missed the manifest
pio = os.path.join(target, "platformio.ini")
if os.path.isfile(pio) and not any(h["technology"] == "PlatformIO" for h in hits):
    hits.append(
        {
            "category": "D",
            "technology": "PlatformIO",
            "file_count": 1,
            "sample": "platformio.ini",
        }
    )
print(json.dumps({"target": target, "signals": hits}, indent=2))
PY
)

if [[ -n "$OUT" ]]; then
  printf '%s\n' "$payload" > "$OUT"
  count=$(python3 -c 'import json,sys; print(len(json.load(open(sys.argv[1]))["signals"]))' "$OUT")
  echo "wrote $OUT ($count signals)"
else
  printf '%s\n' "$payload"
fi
