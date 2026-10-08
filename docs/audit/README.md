# Technology Audit — esp32.heartrate

Pilot-Audit (2026-10-08) gegen Commit `72acae4`.

| Dokument | Zweck |
|----------|--------|
| [SCOPE.md](SCOPE.md) | Grenzen und Quellen |
| [INVENTORY.md](INVENTORY.md) | Technologie-Inventar |
| [sheets/](sheets/) | L2 Technology Sheets (YAML) |
| [FOLLOW-ON-IDEAS.md](FOLLOW-ON-IDEAS.md) | Folgeideen aus dem Audit |

## Lokal prüfen

```bash
bash tools/audit/audit_validate.sh
bash tools/audit/audit_scan.sh .   # Signal-Scan gegen dieses Repo
```

CI: [`.github/workflows/audit.yml`](../../.github/workflows/audit.yml) — Validate auf PR/Push; Scan als Artifact.
