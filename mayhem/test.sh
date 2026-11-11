#!/usr/bin/env bash
#
# mayhem/test.sh — RUN chiaki-ng's own munit suite (test/*.c, built by mayhem/build.sh with normal flags).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

RUNNER="$SRC/build-test/chiaki-unit"
if [ ! -x "$RUNNER" ]; then
  echo "test runner $RUNNER missing: mayhem/build.sh should have produced it" >&2
  emit_ctrf "munit" 0 1 0
  exit 1
fi

out=$("$RUNNER" --no-fork 2>&1)
rc=$?
printf '%s\n' "$out"

# munit summary: "<ok> of <total> (<pct>%) tests successful, <skipped> (<pct>%) test skipped."
summary=$(printf '%s\n' "$out" | grep -E '^[0-9]+ of [0-9]+ \([0-9.]+%\) tests successful' | tail -n1)
if [ -z "$summary" ]; then
  echo "could not find munit summary line" >&2
  emit_ctrf "munit" 0 1 0
  exit 1
fi
ok=$(printf '%s' "$summary" | sed -E 's/^([0-9]+) of ([0-9]+).*/\1/')
total=$(printf '%s' "$summary" | sed -E 's/^([0-9]+) of ([0-9]+).*/\2/')
skipped=$(printf '%s' "$summary" | sed -E 's/.*successful, ([0-9]+) .*/\1/')
case "$skipped" in ''|*[!0-9]*) skipped=0;; esac
failed=$(( total - ok - skipped ))
[ "$failed" -ge 0 ] || failed=0
[ "$rc" -eq 0 ] || [ "$failed" -gt 0 ] || failed=1
# an empty or tiny run must not pass: the suite has well over 50 cases
if [ "$total" -lt 50 ]; then
  echo "munit ran only $total tests; expected the full suite" >&2
  failed=$(( failed + 1 ))
fi
emit_ctrf "munit" "$ok" "$failed" "$skipped"
