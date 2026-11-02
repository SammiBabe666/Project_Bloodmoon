#!/usr/bin/env bash
# Host test of the Opus 2 port against v1.3 (see compare.c). Needs a C compiler and the Opus plugin kit's sdk folder:
#   OPUS_SDK=path/to/opus/plugin-kit/sdk platforms/opus/tests/run.sh
# SANITIZE="-fsanitize=address,undefined -fno-sanitize-recover=all" adds the sanitizers.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OP1="$ROOT/platforms/op1/src"
PORT="$ROOT/platforms/opus/src/noladrift.bloodmoon"
OUT=${OUT:-$ROOT/platforms/opus/tests/out}
: "${OPUS_SDK:?set OPUS_SDK to the sdk folder of the Opus plugin kit}"
mkdir -p "$OUT"
W=(-O2 -Wall -Wextra -Werror ${SANITIZE:-})
cc "${W[@]}" -DOP1_HOST -I"$OP1" -c "$OP1/plugins/bloodmoon/bloodmoon.c" -o "$OUT/v13.o"
for f in bloodmoon moon; do
    cc "${W[@]}" -ffreestanding -fno-builtin -I"$OPUS_SDK" -c "$PORT/$f.c" -o "$OUT/$f.o"
done
cc "${W[@]}" -I"$OPUS_SDK" -I"$PORT" "$ROOT/platforms/opus/tests/compare.c" "$OUT/v13.o" "$OUT/bloodmoon.o" "$OUT/moon.o" -lm -o "$OUT/compare"
cd "$OUT" && ./compare
