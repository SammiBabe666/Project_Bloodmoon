#!/usr/bin/env bash
# Build the Opus 2 plugin file from platforms/opus/src/noladrift.bloodmoon with the Opus kit's own tool.
#   OPUS_KIT=path/to/opus OP1_BFIN_TOOLCHAIN=path/to/bfin-toolchain tools/build-platform/build-opus.sh [out.opmod]
# OPUS_KIT is a checkout of https://github.com/LizardMan245/opus (the released files were built at d9b0d2e6);
# OP1_BFIN_TOOLCHAIN the compiler its plugin-kit/tools/toolchain/build-bfin-toolchain.sh builds. Default output:
# build/opus/bloodmoon.opmod (kept apart from download/, which holds the released file).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
: "${OPUS_KIT:?set OPUS_KIT to a checkout of LizardMan245/opus}"
: "${OP1_BFIN_TOOLCHAIN:?set OP1_BFIN_TOOLCHAIN to the Blackfin toolchain folder}"
export OP1_BFIN_TOOLCHAIN
OUT=${1:-$ROOT/build/opus/bloodmoon.opmod}
mkdir -p "$(dirname "$OUT")"
python3 "$ROOT/platforms/opus/make_headers.py"
git -C "$ROOT" diff --quiet -- platforms/opus/src || echo "note: the generated headers changed; commit them with the source"
python3 "$OPUS_KIT/plugin-kit/plugin_tool.py" build "$ROOT/platforms/opus/src/noladrift.bloodmoon" --out "$OUT"
python3 "$OPUS_KIT/plugin-kit/plugin_tool.py" show "$OUT"
rm -rf "${OUT%.opmod}.build"
sha256sum "$OUT"
