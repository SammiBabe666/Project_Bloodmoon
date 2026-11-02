#!/usr/bin/env bash
# Cycle benchmark of the Opus engine on GDB's Blackfin simulator (see bench_bfin.c). Needs the Blackfin compiler the
# Opus kit builds (OP1_BFIN_TOOLCHAIN, its bin/ holding bfin-elf-gcc), bfin-elf-run on PATH and the kit's sdk folder:
#   OPUS_SDK=path/to/opus/plugin-kit/sdk OP1_BFIN_TOOLCHAIN=path platforms/opus/tests/bench_bfin.sh [old-revision]
# With a git revision (say v1.5.1's 801f8a7) the engine of that revision is measured beside the current one.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OUT=${OUT:-$ROOT/platforms/opus/tests/out-bfin}
: "${OPUS_SDK:?set OPUS_SDK to the sdk folder of the Opus plugin kit}"
: "${OP1_BFIN_TOOLCHAIN:?set OP1_BFIN_TOOLCHAIN to the Blackfin toolchain folder}"
CC="$OP1_BFIN_TOOLCHAIN/bin/bfin-elf-gcc"
SRC="$ROOT/platforms/opus/src/noladrift.bloodmoon"
F=(-mcpu=bf524-any -O2 -ffreestanding -fno-builtin -fno-common -Wall -Wextra -Werror -I"$OPUS_SDK")
mkdir -p "$OUT"
OBJS=("$OUT/now.o")
"$CC" "${F[@]}" -I"$SRC" -c "$SRC/bloodmoon.c" -o "$OUT/now.o"
BENCH=()
if [ $# -ge 1 ]; then
    rm -rf "$OUT/old" && mkdir -p "$OUT/old"
    for f in bloodmoon.c bm.h bm_sine.h bm_art.h; do
        git -C "$ROOT" show "$1:platforms/opus/src/noladrift.bloodmoon/$f" > "$OUT/old/$f" 2>/dev/null ||
            git -C "$ROOT" show "$1:opus/noladrift.bloodmoon/$f" > "$OUT/old/$f"   # before the move to platforms/
    done
    "$CC" "${F[@]}" -I"$OUT/old" -Dopl_init=old_opl_init -Dbm_qsine=old_bm_qsine -DBM_QSINE=OLD_BM_QSINE \
        -c "$OUT/old/bloodmoon.c" -o "$OUT/old.o"
    OBJS+=("$OUT/old.o")
    BENCH=(-DOLD="\"$1\"")
fi
"$CC" "${F[@]}" "${BENCH[@]}" -c "$ROOT/platforms/opus/tests/bench_bfin.c" -o "$OUT/bench.o"
"$CC" -mcpu=bf524-any -nostdlib -Wl,-e,bench_start -o "$OUT/bench.elf" "$OUT/bench.o" "${OBJS[@]}" -lgcc
bfin-elf-run "$OUT/bench.elf"
