#!/usr/bin/env bash
# Host tests of the original OP-1 (op1r) build, on the computer (a C compiler and Python 3):
#   1. the engine through the op1r host harness, with address and undefined-behaviour sanitizers, at the default
#      knobs, every knob at 0 and every knob at full: it must sound and stay below full scale;
#   2. the screen (src/tests/screen_test.c with moon.c): the digits each knob value shows and the value group's shift.
#   platforms/op1/tests/run.sh
# Building the .op1p needs op1r's Blackfin build (src/make_plugin.py, docker); see ../README.md.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
SRC="$ROOT/platforms/op1/src"
OUT=${OUT:-$SRC/harness/out}
mkdir -p "$OUT"
for k in 14000,20000,7000,9000 0,0,0,0 32767,32767,32767,32767; do
    line=$(python3 "$SRC/harness/audio.py" bloodmoon -o "$OUT/op1_test.wav" --knobs "$k" 2>&1 | grep "^peak")
    echo "audio $k: $line"
    peak=$(echo "$line" | sed -n 's/^peak \([0-9]*\).*/\1/p')
    [ -n "$peak" ] && [ "$peak" -gt 1000 ] && [ "$peak" -lt 32767 ] || { echo "FAIL: audio $k"; exit 1; }
done
cc -O1 -Wall -Wextra -Werror -DOP1_HOST -I"$SRC" "$SRC/tests/screen_test.c" "$SRC/plugins/bloodmoon/moon.c" -lm \
    -o "$OUT/screen_test"
got=$("$OUT/screen_test" "$SRC/plugins/bloodmoon/bloodmoon.svg" 0 0 0 0 32767 32767 32767 32767 5000 12000 0 32767)
expect() { echo "$got" | grep -qF -- "$1" || { echo "FAIL: screen: no line \"$1\""; echo "$got"; exit 1; }; }
expect "0 0 tx=-14938 : k0p2d0 k1p2d0 k2p2d0 k3p2d0"
expect "3 32767 tx=0 : k0p0d1 k0p1d0 k0p2d0 k1p0d1 k1p1d0 k1p2d0 k2p0d1 k2p1d0 k2p2d0 k3p0d1 k3p1d0 k3p2d0"
expect "1 12000 tx=-7469 : k0p1d1 k0p2d5 k1p1d3 k1p2d7 k2p2d0 k3p0d1 k3p1d0 k3p2d0"
echo "screen: values 0, 100, 15 and 37 show the right digits, centred"
echo PASS
