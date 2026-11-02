#!/usr/bin/env bash
# Assemble the Hundekraft plugin folder: platforms/hundekraft/src/plugin.json (the kennel manifest) with the engine,
# screen code and art from the OP-1 source (platforms/op1/src/plugins/bloodmoon/, the one copy of them).
#   tools/build-platform/stage-hundekraft.sh [out_dir]      (default: platforms/hundekraft/build/bloodmoon)
# Then pack it with the Hundekraft 3.0.3 SDK (see platforms/hundekraft/README.md).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$ROOT/platforms/hundekraft/build/bloodmoon}
ENGINE="$ROOT/platforms/op1/src/plugins/bloodmoon"
rm -rf "$OUT" && mkdir -p "$OUT"
cp "$ROOT/platforms/hundekraft/src/plugin.json" "$OUT/"
for f in $(python3 -c "import json,sys; p=json.load(open('$OUT/plugin.json')); print(' '.join(p['sources'] + [p['screen']['svg']]))"); do
    cp "$ENGINE/$f" "$OUT/$f"
done
echo "staged $OUT"
