#!/usr/bin/env bash
# Every host test of BLOODMOON in one command, on any Linux or macOS machine with a C compiler, Python 3 and git:
#   tests/run.sh                  everything
#   tests/run.sh repo engines     only those steps (repo, engines, presets, op1, hundekraft, opus)
# 1. tests/check_repo.py: the released files, the download zip, the Opus manifest and the firmware ties;
# 2. tests/engine_test.c: both engines' behaviour (knobs, pitch, tuning, voices, knob jumps, DC), with sanitizers;
# 3. each platform's own tests and the preset validator (docs/development.md lists what each checks).
# The Opus tests need the Opus plugin kit at the pinned commit: OPUS_SDK=path/to/opus/plugin-kit/sdk, or, unset, the
# kit is cloned once into build/opus-kit. SANITIZE="" turns the sanitizers off (they are on by default).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
KIT_COMMIT=d9b0d2e6ddf660d7db758f2be9647585fe100ad8
OUT=${OUT:-$ROOT/build/tests}
SANITIZE=${SANITIZE--fsanitize=address,undefined -fno-sanitize-recover=all}
mkdir -p "$OUT"
if [ -z "${OPUS_SDK:-}" ] && { [ $# -eq 0 ] || [[ " $* " == *" engines "* || " $* " == *" opus "* ]]; }; then
    KIT="$ROOT/build/opus-kit"
    if [ ! -d "$KIT/.git" ]; then git clone -q https://github.com/LizardMan245/opus "$KIT"; fi
    git -C "$KIT" checkout -q "$KIT_COMMIT" 2>/dev/null || { git -C "$KIT" fetch -q origin && git -C "$KIT" checkout -q "$KIT_COMMIT"; }
    OPUS_SDK="$KIT/plugin-kit/sdk"
fi
export OPUS_SDK="${OPUS_SDK:-}"

STEPS=" ${*:-repo engines presets op1 hundekraft opus} "
want() { [[ "$STEPS" == *" $1 "* ]]; }
step() { printf '\n== %s\n' "$1"; }

if want repo; then
step "repository"
python3 "$ROOT/tests/check_repo.py"
fi

if want engines; then
step "engines"
OP1="$ROOT/platforms/op1/src"
PORT="$ROOT/platforms/opus/src/noladrift.bloodmoon"
W=(-O2 -Wall -Wextra -Werror $SANITIZE)
cc "${W[@]}" -DOP1_HOST -I"$OP1" -c "$OP1/plugins/bloodmoon/bloodmoon.c" -o "$OUT/v13.o"
cc "${W[@]}" -ffreestanding -fno-builtin -I"$OPUS_SDK" -c "$PORT/bloodmoon.c" -o "$OUT/opus.o"
cc "${W[@]}" -I"$OPUS_SDK" -c "$ROOT/tests/engine_stubs.c" -o "$OUT/stubs.o"
cc "${W[@]}" -I"$OPUS_SDK" "$ROOT/tests/engine_test.c" "$OUT/v13.o" "$OUT/opus.o" "$OUT/stubs.o" -lm -o "$OUT/engine_test"
"$OUT/engine_test"
fi

if want presets; then
step "presets"
python3 "$ROOT/tools/preset-validation/validate_presets.py" --render
fi

if want op1; then
step "OP-1 (op1r)"
"$ROOT/platforms/op1/tests/run.sh"
fi

if want hundekraft; then
step "Hundekraft"
"$ROOT/platforms/hundekraft/tests/run.sh"
fi

if want opus; then
step "Opus against v1.3"
SANITIZE="$SANITIZE" "$ROOT/platforms/opus/tests/run.sh"
fi

printf '\nPASS: %s\n' "$(echo $STEPS)"
