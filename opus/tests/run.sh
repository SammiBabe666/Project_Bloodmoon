#!/usr/bin/env bash
# Host test of the Opus 2 port against v1.3 (see compare.c). Needs a C compiler and the Opus plugin kit's sdk folder:
#   OPUS_SDK=path/to/opus/plugin-kit/sdk opus/tests/run.sh
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${OUT:-$ROOT/opus/tests/out}
: "${OPUS_SDK:?set OPUS_SDK to the sdk folder of the Opus plugin kit}"
mkdir -p "$OUT"
W=(-O2 -Wall -Wextra -Werror)
cc "${W[@]}" -DOP1_HOST -I"$ROOT/src" -c "$ROOT/src/plugins/bloodmoon/bloodmoon.c" -o "$OUT/v13.o"
for f in bloodmoon moon; do
    cc "${W[@]}" -ffreestanding -fno-builtin -I"$OPUS_SDK" -c "$ROOT/opus/noladrift.bloodmoon/$f.c" -o "$OUT/$f.o"
done
cc "${W[@]}" -I"$OPUS_SDK" "$ROOT/opus/tests/compare.c" "$OUT/v13.o" "$OUT/bloodmoon.o" "$OUT/moon.o" -lm -o "$OUT/compare"
cd "$OUT" && ./compare
