#!/usr/bin/env bash
# Validate in-repo technology audit under docs/audit/ (technology-audit review checks light).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

RED=$'\033[31m'; GRN=$'\033[32m'; YLW=$'\033[33m'; BLD=$'\033[1m'; RST=$'\033[0m'
ERRORS=0
WARNINGS=0

fail() { echo "${RED}✗${RST} $*"; ERRORS=$((ERRORS + 1)); }
warn() { echo "${YLW}!${RST} $*"; WARNINGS=$((WARNINGS + 1)); }
pass() { echo "${GRN}✓${RST} $*"; }
step() { echo; echo "${BLD}══ $* ══${RST}"; }

AUDIT="docs/audit"
VALID_STATUS='candidate|observed|documented|applied|transferred'
VALID_COVERAGE='none|partial|full'
VALID_PRIMARY='A|B|C|D|E|F|G|H|I|J'
EVID_RE='EVID-[A-Z0-9]+-[0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{3}'

step "Required campaign files"
for f in README.md SCOPE.md INVENTORY.md evidence.md; do
  [[ -f "$AUDIT/$f" ]] || fail "missing $AUDIT/$f"
done
pass "core docs present"

step "Campaign YAML / exit criteria"
grep -q '^```yaml' "$AUDIT/README.md" || fail "README.md: missing yaml frontmatter block"
grep -Eq 'id:[[:space:]]*esp32-heartrate' "$AUDIT/README.md" || fail "README.md: missing campaign id"
grep -q 'Exit-Kriterien' "$AUDIT/README.md" || fail "README.md: missing Exit-Kriterien"
pass "campaign header"

step "Inventory header + table"
grep -q 'project_id:' "$AUDIT/INVENTORY.md" || fail "INVENTORY.md: missing project_id"
grep -q '| Slug |' "$AUDIT/INVENTORY.md" || fail "INVENTORY.md: missing overview table"
pass "inventory header"

step "Inventory rows (status / coverage / links)"
# Only the Übersicht table (has Status + Coverage columns), not Coverage-Lücken.
row_count=0
while IFS= read -r line; do
  # Expect: | `slug` | Name | Primary | Tags | Status | Coverage | Sheet | Application |
  slug=$(echo "$line" | awk -F'|' '{gsub(/[` ]/,"",$2); print $2}')
  primary=$(echo "$line" | awk -F'|' '{gsub(/ /,"",$4); print $4}')
  status=$(echo "$line" | awk -F'|' '{gsub(/ /,"",$6); print $6}')
  coverage=$(echo "$line" | awk -F'|' '{gsub(/ /,"",$7); print $7}')
  sheet_cell=$(echo "$line" | awk -F'|' '{print $8}')
  app_cell=$(echo "$line" | awk -F'|' '{print $9}')

  # Skip non-overview tables (e.g. Coverage-Lücken has only 3 columns after slug)
  [[ -n "$status" && -n "$coverage" ]] || continue
  [[ "$status" =~ ^($VALID_STATUS)$ ]] || continue

  row_count=$((row_count + 1))
  [[ "$slug" =~ ^[a-z0-9-]+$ ]] || { fail "bad slug in row: $line"; continue; }
  [[ "$primary" =~ ^($VALID_PRIMARY)$ ]] || fail "$slug: primary '$primary' not in A–J"
  [[ "$coverage" =~ ^($VALID_COVERAGE)$ ]] || fail "$slug: invalid coverage '$coverage'"

  sheet_path=$(echo "$sheet_cell" | sed -n 's/.*](\([^)]*\)).*/\1/p')
  app_path=$(echo "$app_cell" | sed -n 's/.*](\([^)]*\)).*/\1/p')

  if [[ "$status" == "documented" || "$status" == "applied" ]]; then
    [[ -n "$sheet_path" ]] || fail "$slug: status $status but no sheet link"
    [[ -f "$AUDIT/$sheet_path" ]] || fail "$slug: sheet missing: $AUDIT/$sheet_path"
  fi
  if [[ "$status" == "applied" ]]; then
    [[ -n "$app_path" ]] || fail "$slug: status applied but no application link"
    [[ -f "$AUDIT/$app_path" ]] || fail "$slug: application missing: $AUDIT/$app_path"
  fi
  if [[ "$status" == "observed" || "$status" == "candidate" ]]; then
    if [[ -n "$sheet_path" && ! -f "$AUDIT/$sheet_path" ]]; then
      warn "$slug: sheet link points to missing file (allowed as forward ref)"
    fi
  fi
done < <(grep -E '^\| `[a-z0-9-]+`' "$AUDIT/INVENTORY.md" || true)

[[ "$row_count" -ge 4 ]] || fail "INVENTORY.md: expected ≥4 tech rows, got $row_count"
pass "$row_count inventory rows checked"

step "Sheets (Ebene A markers)"
sheet_count=0
while IFS= read -r y; do
  sheet_count=$((sheet_count + 1))
  grep -q '^```yaml' "$y" || fail "$y: missing yaml frontmatter"
  grep -q '^slug:' "$y" || fail "$y: missing slug:"
  grep -q '^primary:' "$y" || fail "$y: missing primary:"
  grep -q 'maturity:' "$y" || fail "$y: missing maturity:"
  grep -q '## Offene Fragen' "$y" || warn "$y: no Offene Fragen section"
done < <(find "$AUDIT/sheets" -name '*.md' -print 2>/dev/null || true)
[[ "$sheet_count" -ge 1 ]] || fail "docs/audit/sheets: need at least one .md"
pass "$sheet_count sheets"

step "Applications"
app_count=0
while IFS= read -r y; do
  app_count=$((app_count + 1))
  grep -q 'tech_slug:' "$y" || fail "$y: missing tech_slug"
  grep -q 'sheet:' "$y" || fail "$y: missing sheet:"
  grep -q '## Ebene C' "$y" || warn "$y: no Ebene C section"
  rel=$(grep -E '^sheet:' "$y" | head -1 | awk '{print $2}')
  if [[ -n "$rel" ]]; then
    dir=$(dirname "$y")
    [[ -f "$dir/$rel" ]] || fail "$y: sheet path missing: $dir/$rel"
  fi
done < <(find "$AUDIT/applications" -name '*.md' -print 2>/dev/null || true)
[[ "$app_count" -ge 1 ]] || fail "docs/audit/applications: need at least one .md"
pass "$app_count applications"

step "Evidence register"
grep -qE "$EVID_RE" "$AUDIT/evidence.md" || fail "evidence.md: no EVID-* IDs"
bad_ids=$(grep -oE 'EVID-[A-Za-z0-9_-]+' "$AUDIT/evidence.md" | grep -Ev "^${EVID_RE}$" || true)
if [[ -n "$bad_ids" ]]; then
  fail "evidence.md: IDs with bad format: $bad_ids"
fi
for st in FACT MEASURED INFERRED HYPOTHESIS UNKNOWN REJECTED; do
  :
done
# require status column values from known set in table rows
while IFS= read -r st; do
  st=${st//\`/}
  [[ "$st" =~ ^(FACT|MEASURED|INFERRED|HYPOTHESIS|UNKNOWN|REJECTED)$ ]] \
    || fail "evidence.md: unexpected status '$st'"
done < <(grep -E '^\| `EVID-' "$AUDIT/evidence.md" | awk -F'|' '{gsub(/ /,"",$4); print $4}')
pass "evidence IDs"

step "Signal scan script present"
[[ -x "$ROOT/tools/audit/audit_scan.sh" || -f "$ROOT/tools/audit/audit_scan.sh" ]] \
  || fail "tools/audit/audit_scan.sh missing"
pass "scan tool"

echo
if [[ "$ERRORS" -gt 0 ]]; then
  echo "${RED}${BLD}FAIL: audit_validate ($ERRORS errors, $WARNINGS warnings)${RST}"
  exit 1
fi
echo "${GRN}${BLD}OK: audit_validate${RST} (${WARNINGS} warnings)"
