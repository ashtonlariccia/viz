#!/usr/bin/env bash
# Runs the viz test suite.
#
#   ./run_tests.sh              every tests/m*/ folder
#   ./run_tests.sh tests/m1     just one folder (or several)
#
# Each .vz test starts with an expectation on its FIRST line:
#   // expect: 42                 compiles, runs, exits with 42
#   // expect: error              vizc must reject it (nonzero exit)
#   // expect: error undeclared   ...and its error output must contain "undeclared"
#
# Contract with vizc:  vizc file.vz -o out   → writes out.c, builds out, exits 0
#                      on a compile error    → message on stderr, exits nonzero

set -u
cd "$(dirname "$0")"

VIZC=./vizc
OUT=tests/out
mkdir -p "$OUT"

if [ ! -x "$VIZC" ]; then
    echo "no ./vizc yet — run 'make' first"
    exit 1
fi

if [ -t 1 ]; then G=$'\e[32m'; R=$'\e[31m'; Y=$'\e[33m'; D=$'\e[2m'; N=$'\e[0m'
else G=; R=; Y=; D=; N=; fi

pass=0; fail=0; skip=0

ok()  { pass=$((pass + 1)); printf '%sPASS%s %s\n' "$G" "$N" "$1"; }
bad() { fail=$((fail + 1)); printf '%sFAIL%s %s  %s(%s)%s\n' "$R" "$N" "$1" "$D" "$2" "$N"; }

if [ $# -gt 0 ]; then dirs=("$@"); else dirs=(tests/m*/); fi

while IFS= read -r f; do
    # name = milestone_test, so tests in different milestones can share names
    name="$(basename "$(dirname "$f")")_$(basename "$f" .vz)"
    expect="$(head -n1 "$f" | sed -n 's#^//[[:space:]]*expect:[[:space:]]*##p')"

    if [ -z "$expect" ]; then
        skip=$((skip + 1)); printf '%sSKIP%s %s  (no "// expect:" first line)\n' "$Y" "$N" "$f"
        continue
    fi

    bin="$OUT/$name"
    log="$OUT/$name.log"
    rm -f "$bin" "$bin.c"
    timeout 10 "$VIZC" "$f" -o "$bin" >"$log" 2>&1
    rc=$?

    if [[ "$expect" == error* ]]; then
        want="${expect#error}"; want="${want# }"
        if [ $rc -eq 0 ]; then
            bad "$f" "compiled, but should have been rejected"
        elif [ $rc -ge 124 ]; then
            bad "$f" "vizc crashed or hung (exit $rc), see $log"
        elif [ -n "$want" ] && ! grep -qi -- "$want" "$log"; then
            bad "$f" "rejected, but error doesn't mention '$want': $(head -n1 "$log")"
        else
            ok "$f"
        fi
        continue
    fi

    if [ $rc -ne 0 ]; then
        bad "$f" "vizc exit $rc: $(head -n1 "$log")"
        continue
    fi

    timeout 5 "$bin"
    got=$?
    if [ "$got" -eq "$expect" ]; then
        ok "$f"
    else
        bad "$f" "expected exit $expect, got $got — C is in $bin.c"
    fi
done < <(find "${dirs[@]}" -name '*.vz' | sort)

echo
printf '%d passed, %d failed' "$pass" "$fail"
[ $skip -gt 0 ] && printf ', %d skipped' "$skip"
echo
[ $fail -eq 0 ]
