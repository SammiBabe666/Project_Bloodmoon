#!/usr/bin/env python3
"""BLOODMOON factory sounds as OP-1 synth preset files (.aif): copy the folder into the OP-1's synth/ folder in disk mode.

An OP-1 synth preset is an AIFF-C file (mono, 16-bit, 44.1 kHz, little-endian 'sowt' samples) with the sound's settings
as JSON in an APPL chunk ("op-1" + JSON), laid out like the OP-1's own files. "type": "bloodmoon" selects the plugin by
name (op1r plugload.c, set_selector); on an OP-1 without the plugin the sound loads as iter (dispatcher.c).
The audio part is a short preview rendered with the PC harness; the OP-1 does not need it for a synth sound.

    python3 platforms/op1/src/tests/make_presets.py [out_dir]   (needs cc for the harness; default out: presets/bloodmoon)

The sounds are in shared/preset-specifications/bloodmoon-presets.json: change them there, run this, then
tools/preset-validation/validate_presets.py.
"""
import json, os, struct, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SDK = os.path.dirname(HERE)

POLY, NO_GLIDE = 2048, 64                   # adsr[4] play mode (poly), adsr[5] portamento off (OP-1's own files)
DELAY_OFF = [7168, 32767, 5520, 2616, 0, 0, 0, 0]
TREMOLO = [16000, 0, 0, 16000, 0, 0, 0, 0]  # the factory tremolo, switched off

SPEC = os.path.join(SDK, '..', '..', '..', 'shared', 'preset-specifications', 'bloodmoon-presets.json')


def load_sounds(spec=SPEC):
    """-> [(name, knobs, attack, decay, sustain, release, octave, delay)] from the shared specification"""
    out = []
    for snd in json.load(open(spec))['sounds']:
        k, e = snd['knobs'], snd['envelope']
        out.append((snd['name'], (k['blood'], k['eclipse'], k['haunting'], k['moonrise']),
                    e['attack'], e['decay'], e['sustain'], e['release'], snd['octave'], snd['delay']))
    return out


def ext80(x):                               # IEEE 754 80-bit extended, for the COMM sample rate
    e = 16383 + 63; m = int(x)
    while m < 1 << 63: m <<= 1; e -= 1
    return struct.pack('>HQ', e, m)

def chunk(tag, data):
    return tag + struct.pack('>I', len(data)) + data + (b'\0' if len(data) & 1 else b'')

def preview(knobs):
    """PC harness render of the sound's knobs (raw engine, phrase on one voice), first second (44160 frames, as the OP-1 writes)"""
    out = os.path.join(SDK, 'harness', 'out', 'preset_preview.wav')
    subprocess.run([sys.executable, os.path.join(SDK, 'harness', 'audio.py'), 'bloodmoon', '-o', out,
                    '--knobs', ','.join(map(str, knobs))], check=True, capture_output=True)
    import wave
    with wave.open(out) as w:
        pcm = w.readframes(w.getnframes())
    pcm = pcm[:44160 * 2]
    return pcm

def aif(sound):
    name, knobs, a, d, s, r, octave, delay = sound
    patch = {
        'adsr': [a, d, s, r, POLY, NO_GLIDE, 4000, 4000],
        'fx_active': delay is not None,
        'fx_params': (delay + [0, 0, 0, 0]) if delay else DELAY_OFF,
        'fx_type': 'delay',
        'knobs': list(knobs) + [0, 0, 0, 0],
        'lfo_active': False,
        'lfo_params': TREMOLO,
        'lfo_type': 'tremolo',
        'name': name,
        'octave': octave,
        'synth_version': 2,
        'type': 'bloodmoon',
    }
    js = json.dumps(patch, separators=(',', ':')).encode()
    appl = b'op-1' + js + b'\n'
    if len(appl) & 1: appl += b' '                              # keep the chunk even (as the OP-1's files)
    pcm = preview(knobs)
    comp = b'sowt' + bytes([41]) + b'Signed integer (little-endian) linear PCM'
    comm = struct.pack('>hIh', 1, len(pcm) // 2, 16) + ext80(44100) + comp
    body = (b'AIFC' + chunk(b'FVER', struct.pack('>I', 0xA2805140)) + chunk(b'COMM', comm) + chunk(b'APPL', appl)
            + chunk(b'SSND', struct.pack('>II', 0, 0) + pcm))
    return b'FORM' + struct.pack('>I', len(body)) + body, patch

def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(SDK, '..', '..', '..', 'presets', 'bloodmoon')
    os.makedirs(out, exist_ok=True)
    for snd in load_sounds():
        data, patch = aif(snd)
        fn = os.path.join(out, snd[0].replace(' ', '_') + '.aif')
        open(fn, 'wb').write(data)
        print(fn, len(data), 'B', patch['knobs'][:4])

if __name__ == '__main__':
    main()
