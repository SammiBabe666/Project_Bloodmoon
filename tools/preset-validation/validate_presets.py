#!/usr/bin/env python3
"""Checks the BLOODMOON preset collection against its specification and the OP-1 preset format.

    python3 tools/preset-validation/validate_presets.py [--render]

Checks, for shared/preset-specifications/bloodmoon-presets.json and presets/bloodmoon/*.aif:
  - the specification: eight sounds, unique names and file names, names lower case and at most 15 characters
    (the OP-1 shows that many), file names <name with _>.aif, every knob and envelope value 0..32767, octave -2..2,
    delay null or four values 0..32767;
  - the files: exactly the specified ones, each an AIFF-C (FORM/AIFC) with COMM, APPL and SSND chunks, mono 16-bit
    44.1 kHz, an APPL chunk "op-1" + JSON whose type is the engine's ("bloodmoon"), and whose name, knobs (the four
    engine knobs, the other four 0), envelope, octave and delay equal the specification's;
  - the Opus set (platforms/opus/presets/presets.py check): its sound.json files equal the specification and lie
    inside the OS's factory ranges;
  - the install paths the docs give: the folder is presets/bloodmoon/, copied into the OP-1's synth/ folder.
With --render it also renders every sound's knobs through the OP-1 host harness (platforms/op1/src/harness/audio.py,
needs a C compiler) and checks it is not silent, does not reach full scale, and that the levels lie within 6 dB.

The preview audio inside each file (SSND) is not compared: the OP-1 does not use it to play the sound. The shipped
files' previews were rendered before v1.3's output gain, so they are 3 dB louder than a new render. Standard library
only. Exit status 0 when everything passes, 1 otherwise."""
import json, math, os, struct, subprocess, sys, tempfile, wave

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
SPEC = os.path.join(ROOT, 'shared', 'preset-specifications', 'bloodmoon-presets.json')
PRESETS = os.path.join(ROOT, 'presets', 'bloodmoon')
HARNESS = os.path.join(ROOT, 'platforms', 'op1', 'src', 'harness', 'audio.py')
OPUS_PRESETS = os.path.join(ROOT, 'platforms', 'opus', 'presets', 'presets.py')
COUNT = 8

errors = []


def fail(msg):
    errors.append(msg)
    print('FAIL:', msg)


def q15(v):
    return isinstance(v, int) and not isinstance(v, bool) and 0 <= v <= 32767


def check_spec(spec):
    sounds = spec.get('sounds', [])
    if spec.get('engine_type') != 'bloodmoon':
        fail('spec: engine_type must be "bloodmoon"')
    if len(sounds) != COUNT:
        fail(f'spec: {len(sounds)} sounds, expected {COUNT}')
    names, files = set(), set()
    for s in sounds:
        n = s.get('name', '')
        if not n or n != n.lower() or len(n) > 15:
            fail(f'spec: name {n!r} must be lower case, 1..15 characters')
        if n in names:
            fail(f'spec: duplicate name {n!r}')
        names.add(n)
        f = s.get('file', '')
        if f != n.replace(' ', '_') + '.aif':
            fail(f'spec: {n}: file {f!r}, expected {n.replace(" ", "_")}.aif')
        if f in files:
            fail(f'spec: duplicate file {f!r}')
        files.add(f)
        for group, keys in (('knobs', ('blood', 'eclipse', 'haunting', 'moonrise')),
                            ('envelope', ('attack', 'decay', 'sustain', 'release'))):
            g = s.get(group, {})
            if sorted(g) != sorted(keys):
                fail(f'spec: {n}: {group} must have exactly {", ".join(keys)}')
            for k in keys:
                if not q15(g.get(k)):
                    fail(f'spec: {n}: {group}.{k} = {g.get(k)!r}, must be an integer 0..32767')
        if s.get('octave') not in (-2, -1, 0, 1, 2):
            fail(f'spec: {n}: octave {s.get("octave")!r}, must be -2..2')
        d = s.get('delay')
        if d is not None and not (isinstance(d, list) and len(d) == 4 and all(q15(x) for x in d)):
            fail(f'spec: {n}: delay must be null or four integers 0..32767')
        if not s.get('character'):
            fail(f'spec: {n}: no character description')
    return sounds


def chunks(data):
    if data[:4] != b'FORM' or data[8:12] != b'AIFC':
        raise ValueError('not an AIFF-C file (FORM/AIFC)')
    if struct.unpack('>I', data[4:8])[0] != len(data) - 8:
        raise ValueError('FORM size does not match the file')
    out, i = {}, 12
    while i + 8 <= len(data):
        tag, size = data[i:i + 4], struct.unpack('>I', data[i + 4:i + 8])[0]
        out[tag] = data[i + 8:i + 8 + size]
        i += 8 + size + (size & 1)
    return out


def check_file(path, s):
    n = s['name']
    try:
        c = chunks(open(path, 'rb').read())
    except (OSError, ValueError) as e:
        fail(f'{os.path.basename(path)}: {e}')
        return
    for tag in (b'COMM', b'APPL', b'SSND'):
        if tag not in c:
            fail(f'{os.path.basename(path)}: no {tag.decode()} chunk')
            return
    ch, _, bits = struct.unpack('>hIh', c[b'COMM'][:8])
    if (ch, bits) != (1, 16) or c[b'COMM'][8:18] != bytes.fromhex('400eac44000000000000') or c[b'COMM'][18:22] != b'sowt':
        fail(f'{os.path.basename(path)}: COMM must be mono, 16-bit, 44.1 kHz, sowt')
    appl = c[b'APPL']
    if appl[:4] != b'op-1':
        fail(f'{os.path.basename(path)}: APPL chunk is not "op-1"')
        return
    try:
        p = json.loads(appl[4:].decode().strip())
    except ValueError as e:
        fail(f'{os.path.basename(path)}: APPL JSON: {e}')
        return
    k, e = s['knobs'], s['envelope']
    want = {
        'type': 'bloodmoon', 'name': n,
        'knobs': [k['blood'], k['eclipse'], k['haunting'], k['moonrise'], 0, 0, 0, 0],
        'adsr4': [e['attack'], e['decay'], e['sustain'], e['release']],
        'octave': s['octave'], 'fx_active': s['delay'] is not None,
    }
    got = {
        'type': p.get('type'), 'name': p.get('name'), 'knobs': p.get('knobs'),
        'adsr4': (p.get('adsr') or [])[:4], 'octave': p.get('octave'), 'fx_active': p.get('fx_active'),
    }
    for key in want:
        if want[key] != got[key]:
            fail(f'{os.path.basename(path)}: {key} is {got[key]!r}, the specification says {want[key]!r}')
    if s['delay'] is not None and (p.get('fx_type') != 'delay' or p.get('fx_params', [])[:4] != s['delay']):
        fail(f'{os.path.basename(path)}: delay settings differ from the specification')
    if not all(q15(x) for x in p.get('knobs', []) + p.get('adsr', [])[:4]):
        fail(f'{os.path.basename(path)}: a knob or envelope value outside 0..32767')


def render(s):
    k = s['knobs']
    with tempfile.TemporaryDirectory() as t:
        out = os.path.join(t, 'p.wav')
        subprocess.run([sys.executable, HARNESS, 'bloodmoon', '-o', out, '--knobs',
                        f'{k["blood"]},{k["eclipse"]},{k["haunting"]},{k["moonrise"]}'],
                       check=True, capture_output=True)
        with wave.open(out) as w:
            raw = w.readframes(w.getnframes())
    v = struct.unpack(f'<{len(raw) // 2}h', raw)
    peak = max(abs(x) for x in v)
    rms = math.sqrt(sum(x * x for x in v) / len(v))
    return peak, 20 * math.log10(rms / 32768) if rms else -999.0


def main():
    spec = json.load(open(SPEC))
    sounds = check_spec(spec)
    have = sorted(f for f in os.listdir(PRESETS) if not f.startswith('.'))
    want = sorted(s['file'] for s in sounds)
    if have != want:
        fail(f'presets/bloodmoon holds {have}, the specification names {want}')
    for s in sounds:
        path = os.path.join(PRESETS, s['file'])
        if os.path.isfile(path):
            check_file(path, s)
    for doc in ('download/INSTALL.md', 'docs/installation.md'):
        p = os.path.join(ROOT, doc)
        if os.path.isfile(p) and 'presets/bloodmoon' not in open(p).read():
            fail(f'{doc} does not name the presets/bloodmoon folder')
    if '--render' in sys.argv:
        levels = []
        for s in sounds:
            peak, db = render(s)
            print(f'render: {s["name"]:<13} peak {peak:5d}  RMS {db:6.1f} dBFS')
            if db < -60:
                fail(f'render: {s["name"]} is silent')
            if peak >= 32767:
                fail(f'render: {s["name"]} reaches full scale')
            levels.append(db)
        if levels and max(levels) - min(levels) > 6:
            fail(f'render: levels span {max(levels) - min(levels):.1f} dB, more than 6')
    r = subprocess.run([sys.executable, OPUS_PRESETS, 'check'], capture_output=True, text=True)
    print('opus:', (r.stdout or r.stderr).strip())
    if r.returncode:
        fail('the Opus presets (platforms/opus/presets/) do not match the specification')
    print(f'{len(sounds)} sounds, {len(have)} files: ' + ('PASS' if not errors else f'{len(errors)} problem(s)'))
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
