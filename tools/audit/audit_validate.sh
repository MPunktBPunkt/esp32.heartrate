#!/usr/bin/env bash
# Validate in-repo technology audit under docs/audit/.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

RED=$'\033[31m'; GRN=$'\033[32m'; BLD=$'\033[1m'; RST=$'\033[0m'
fail() { echo "${RED}✗${RST} $*"; exit 1; }
pass() { echo "${GRN}✓${RST} $*"; }
step() { echo; echo "${BLD}══ $* ══${RST}"; }

AUDIT="docs/audit"

step "Required audit files"
for f in README.md SCOPE.md INVENTORY.md; do
  [[ -f "$AUDIT/$f" ]] || fail "missing $AUDIT/$f"
done
pass "core docs"

step "Inventory table"
grep -q '|.*|' "$AUDIT/INVENTORY.md" || fail "$AUDIT/INVENTORY.md: no table rows"
pass "INVENTORY.md"

step "Sheets"
sheet_count=$(find "$AUDIT/sheets" -name '*.yaml' 2>/dev/null | wc -l)
[[ "$sheet_count" -ge 1 ]] || fail "$AUDIT/sheets: need at least one .yaml"
pass "$sheet_count sheets"

step "YAML sanity"
while IFS= read -r y; do
  grep -q '^id:' "$y" || fail "$y: missing id:"
  grep -q '^maturity:' "$y" || fail "$y: missing maturity:"
done < <(find "$AUDIT/sheets" -name '*.yaml' -print)

pass "YAML sanity"
echo
echo "${GRN}${BLD}OK: audit_validate${RST}"
