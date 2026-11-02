#!/usr/bin/env python3
"""BLOODMOON's Opus presets: the eight factory sounds as Opus sounds, one folder each.

    python3 platforms/opus/presets/presets.py sync          writes each folder's sound.json from the shared specification
    python3 platforms/opus/presets/presets.py check         checks the folders match the specification and the OS's ranges
    python3 platforms/opus/presets/presets.py learn F.aif   reads how Opus saves a bloodmoon sound from one saved on an
                                                            OP-1 (Opus 2, BLOODMOON 1.5.2), into opus-format.json
    python3 platforms/opus/presets/presets.py aif           writes each folder's .aif (needs opus-format.json)

Why these are not the op1r/Hundekraft presets (presets/bloodmoon/): those name the engine by "type": "bloodmoon".
Opus keeps an engine's sound as the OS's "iter" with the engine's mark in knob 4 and an "opus" member naming the plugin
(plugin kit, docs/api.md, Engines). The kit does not give that member's layout, nor the mark the core makes from the
plugin's id and the engine's name, and Opus's author said on 2026-11-01 that sounds carrying another firmware's "type"
do not load (Project Erdtree's notes). So the .aif files are written only once both have been read from one real
bloodmoon sound saved on Opus ("learn"). Until then each folder holds sound.json: every value of the sound.

The values come from shared/preset-specifications/bloodmoon-presets.json, the one source for every platform: change
them there, then run "sync". Every value is one of BLOODMOON's four knobs or the OS's own envelope, effect and octave.
The OS's LFO stays off: its parameters' factory ranges are known only for the tremolo default, and a value outside
the factory ranges makes the firmware import the file as a plain sample.

Standard library only. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
"""
import json
import math
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SPEC = ROOT / 'shared' / 'preset-specifications' / 'bloodmoon-presets.json'
FORMAT = HERE / 'opus-format.json'

# The factory presets' ranges (Hundekraft Workshop source, tools/op1r_presets.py, as Project Erdtree measured them):
# the OS imports a file with a value outside them as a plain sample.
RANGE_KNOB = (0, 32767)
RANGE_ADSR = [(64, 16320), (64, 16320), (0, 32767), (64, 16320)]
RANGE_DELAY = [(3392, 11200), (4969, 32767), (0, 16384), (1332, 32767)]
RANGE_OCTAVE = (-1, 1)
NAME_MAX = 15                     # what the OP-1's screen shows; the OS keeps up to 23

POLY, NO_GLIDE = 2048, 64         # adsr[4] play mode (poly), adsr[5] portamento off, as in the OP-1's own files
DELAY_OFF = [7168, 32767, 5520, 2616]
TREMOLO_OFF = [16000, 0, 0, 16000, 0, 0, 0, 0]


def slug(name):
    return name.replace(' ', '-')


def from_spec():
    """The specification's sounds as sound.json contents, in its order."""
    out = []
    for s in json.loads(SPEC.read_text())['sounds']:
        e = s['envelope']
        out.append({
            'name': s['name'],
            'description': s['character'],
            'knobs': dict(s['knobs']),
            'adsr': [e['attack'], e['decay'], e['sustain'], e['release']],
            'fx': {'type': 'delay', 'params': s['delay']} if s['delay'] else None,
            'octave': s['octave'],
        })
    return out


def problems(j):
    n = j['name']
    bad = []
    if not (0 < len(n) <= NAME_MAX and set(n) <= set('abcdefghijklmnopqrstuvwxyz ')):
        bad.append(f'name {n!r}: 1 to {NAME_MAX} lower-case letters and spaces')
    k = [j['knobs'][x] for x in ('blood', 'eclipse', 'haunting', 'moonrise')]
    if any(not RANGE_KNOB[0] <= v <= RANGE_KNOB[1] for v in k):
        bad.append(f'knobs {k}: 0-32767')
    if len(j['adsr']) != 4 or any(not lo <= v <= hi for v, (lo, hi) in zip(j['adsr'], RANGE_ADSR)):
        bad.append(f'adsr {j["adsr"]}: outside the factory ranges {RANGE_ADSR}')
    fx = j['fx']
    if fx is not None and (fx['type'] != 'delay' or len(fx['params']) != 4
                           or any(not lo <= v <= hi for v, (lo, hi) in zip(fx['params'], RANGE_DELAY))):
        bad.append(f'fx {fx}: a delay inside the factory ranges {RANGE_DELAY}')
    if not RANGE_OCTAVE[0] <= j['octave'] <= RANGE_OCTAVE[1]:
        bad.append(f'octave {j["octave"]}: -1 to 1')
    return [f'{slug(n)}: {b}' for b in bad]


def check():
    """The problems with the preset folders, as text (none: an empty list)."""
    want = from_spec()
    bad = [b for j in want for b in problems(j)]
    folders = sorted(p.name for p in HERE.iterdir() if p.is_dir() and not p.name.startswith(('.', '_')))
    if folders != sorted(slug(j['name']) for j in want):
        bad.append(f'folders {folders} differ from the specification\'s sounds')
    for j in want:
        f = HERE / slug(j['name']) / 'sound.json'
        if not f.is_file():
            bad.append(f'{f.relative_to(ROOT)} is missing')
        elif json.loads(f.read_text()) != j:
            bad.append(f'{f.relative_to(ROOT)} differs from the specification (run "presets.py sync")')
        extra = [p.name for p in f.parent.glob('*') if p.name not in ('sound.json', j['name'] + '.aif')] if f.parent.is_dir() else []
        if extra:
            bad.append(f'{f.parent.relative_to(ROOT)} holds {extra}')
    return bad


def sync():
    for j in from_spec():
        d = HERE / slug(j['name'])
        d.mkdir(exist_ok=True)
        (d / 'sound.json').write_text(json.dumps(j, indent=2) + '\n')


def appl_json(data):
    """The "op-1" APPL chunk's JSON of an OP-1 sound file."""
    if data[:4] != b'FORM' or data[8:12] != b'AIFC':
        raise ValueError('not an AIFF-C file')
    i = 12
    while i + 8 <= len(data):
        tag, size = data[i:i + 4], struct.unpack('>I', data[i + 4:i + 8])[0]
        body = data[i + 8:i + 8 + size]
        if tag == b'APPL' and body[:4] == b'op-1':
            return json.loads(body[4:].decode().strip().rstrip('\0'))
        i += 8 + size + (size & 1)
    raise ValueError('no "op-1" APPL chunk')


def learn(path):
    """Keeps what Opus wrote for BLOODMOON in a saved sound: its "type", knob 4 (the mark) and its "opus" member."""
    s = appl_json(Path(path).read_bytes())
    if 'opus' not in s or len(s.get('knobs', [])) < 5 or s['knobs'][4] < 0x100:
        sys.exit(f'{path} does not look like an Opus engine sound (no "opus" member, or no mark in knob 4)')
    fmt = {'source': Path(path).name, 'type': s.get('type'), 'mark': s['knobs'][4], 'opus': s['opus'],
           'saved': s}
    FORMAT.write_text(json.dumps(fmt, indent=2) + '\n')
    print(f'type {fmt["type"]!r}, mark {fmt["mark"]} (0x{fmt["mark"]:04X}), "opus" member {json.dumps(s["opus"])}')
    print(f'wrote {FORMAT.relative_to(ROOT)}: check that the member holds nothing of that one sound, then run "aif"')


def sound(j, fmt):
    """The sound as Opus keeps it, its type, mark and "opus" member as a real saved bloodmoon sound has them."""
    k = j['knobs']
    fx = j['fx']
    s = {
        'adsr': j['adsr'] + [POLY, NO_GLIDE, 4000, 4000],
        'fx_active': fx is not None,
        'fx_params': (fx['params'] if fx else DELAY_OFF) + [0, 0, 0, 0],
        'fx_type': 'delay',
        'knobs': [k['blood'], k['eclipse'], k['haunting'], k['moonrise'], fmt['mark'], 0, 0, 0],
        'lfo_active': False,
        'lfo_params': TREMOLO_OFF,
        'lfo_type': 'tremolo',
        'name': j['name'],
        'octave': j['octave'],
        'opus': fmt['opus'],
        'synth_version': 2,
        'type': fmt['type'],
    }
    return s


def ext80(x):
    e = int(math.floor(math.log2(x)))
    return struct.pack('>HQ', e + 16383, int(x / 2 ** e * 2 ** 63))


def chunk(tag, body):
    return tag + struct.pack('>I', len(body)) + body + (b'\0' if len(body) & 1 else b'')


def aifc(meta):
    """An OP-1 synth sound: AIFF-C (mono, 16-bit, 44.1 kHz, sowt), "op-1" + JSON in APPL, no audio."""
    comm = struct.pack('>hIh', 1, 0, 16) + ext80(44100) + b'sowt' + b')Signed integer (little-endian) linear PCM'
    js = json.dumps(meta, separators=(',', ':'), sort_keys=True).encode() + b'\n'
    if len(js) & 1:
        js += b' '
    body = (chunk(b'FVER', bytes.fromhex('a2805140')) + chunk(b'COMM', comm) + chunk(b'APPL', b'op-1' + js)
            + chunk(b'SSND', struct.pack('>II', 0, 0)))
    return b'FORM' + struct.pack('>I', len(body) + 4) + b'AIFC' + body


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else ''
    if cmd == 'sync' and len(sys.argv) == 2:
        sync()
        print('wrote', len(from_spec()), 'sound.json files')
    elif cmd == 'learn' and len(sys.argv) == 3:
        learn(sys.argv[2])
    elif cmd in ('check', 'aif') and len(sys.argv) == 2:
        bad = check()
        if bad:
            sys.exit('\n'.join(bad))
        if cmd == 'check':
            print(f'{len(from_spec())} Opus presets ok' + ('' if FORMAT.is_file() else
                  ' (sound.json only: no opus-format.json yet, so no .aif)'))
            return
        if not FORMAT.is_file():
            sys.exit('no opus-format.json yet: save one bloodmoon sound on an OP-1 with Opus, then run '
                     '"presets.py learn <that file>.aif"')
        fmt = json.loads(FORMAT.read_text())
        for j in from_spec():
            (HERE / slug(j['name']) / (j['name'] + '.aif')).write_bytes(aifc(sound(j, fmt)))
        print(f'wrote {len(from_spec())} .aif files')
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
