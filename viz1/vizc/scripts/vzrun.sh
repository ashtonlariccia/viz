#!/usr/bin/env bash
# Compile one .vz file, run it, and print its exit code.
#
#   scripts/vzrun.sh file.vz       compile + run
#   scripts/vzrun.sh -c file.vz    also print the C that vizc emitted
#
# Build output goes to build/scratch/ so it never clutters your source folders.

set -u
cd "$(dirname "$0")/.."

show_c=0
if [ "${1:-}" = "-c" ]; then show_c=1; shift; fi

src="${1:-}"
if [ -z "$src" ]; then
    echo "usage: scripts/vzrun.sh [-c] file.vz   (or: make run F=file.vz)"
    exit 2
fi
if [ ! -f "$src" ]; then echo "no such file: $src"; exit 2; fi

mkdir -p build/scratch
out="build/scratch/$(basename "$src" .vz)"

./vizc "$src" -o "$out" || exit $?

if [ $show_c -eq 1 ]; then
    echo "----- $out.c -----"
    cat "$out.c"
    echo "------------------"
fi

"$out"
echo "exit: $?"
