#!/usr/bin/env bash
# Host test of the Hundekraft build's source: stage the plugin folder (tools/build-platform/stage-hundekraft.sh), check
# its manifest, and render it through the OP-1 host harness with sanitizers (the kennel engine is the op1r engine's
# code: same C, same knobs). Packing the .knl needs the Hundekraft SDK and a Blackfin compiler; see ../README.md.
#   platforms/hundekraft/tests/run.sh
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
STAGE=${OUT:-$ROOT/platforms/hundekraft/build}/bloodmoon
"$ROOT/tools/build-platform/stage-hundekraft.sh" "$STAGE"
python3 - "$STAGE/plugin.json" <<'PY'
import json, sys
p = json.load(open(sys.argv[1]))
assert p['name'] == 'bloodmoon' and p['kind'] == 'synth' and p['symbol'] == 'ENGINE_bloodmoon', 'identity'
assert p['defaults'] == [14000, 20000, 7000, 9000], 'defaults must match the OP-1 build'
assert all(0 <= d <= 32767 for d in p['defaults'])
assert p.get('claims') == [], 'claims no exclusive controls'
print('manifest: bloodmoon', p['version'], 'ok')
PY
python3 "$ROOT/platforms/op1/src/harness/audio.py" "$STAGE" -o "$STAGE/../audio_hundekraft.wav" --knobs 32767,32767,32767,32767
echo PASS
