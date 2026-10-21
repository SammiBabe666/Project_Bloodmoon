#!/usr/bin/env python3
"""Render a plugin (synth or FX) to a WAV file on the PC, to hear it before building firmware.

    python3 harness/audio.py <plugin name or folder> [-o out.wav] [--knobs a,b,c,d]

Synths play a short phrase (velocities 127 / 64 alternating), FX process plucked saw notes followed by
1.5 s of silence (to hear tails). Knobs default to plugin.json "defaults" (0..32767). Compiled with
-fsanitize=address,undefined: out-of-bounds writes and overflows in the plugin are reported.
"""
import argparse, json, os, subprocess, sys, wave

HERE = os.path.dirname(os.path.abspath(__file__))
SDK = os.path.dirname(HERE)


def find(p):
    """name (in plugins/) or folder -> (folder, plugin.json dict)"""
    d = p if os.path.isfile(os.path.join(p, 'plugin.json')) else os.path.join(SDK, 'plugins', p)
    if not os.path.isfile(os.path.join(d, 'plugin.json')):
        for f in os.listdir(os.path.join(SDK, 'plugins')):        # also by plugin "name"
            j = os.path.join(SDK, 'plugins', f, 'plugin.json')
            if os.path.isfile(j) and json.load(open(j))['name'] == p:
                d = os.path.dirname(j); break
        else:
            sys.exit(f'no plugin {p!r} (give a name from plugins/ or a folder with plugin.json)')
    return d, json.load(open(os.path.join(d, 'plugin.json')))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('plugin'); ap.add_argument('-o', '--out'); ap.add_argument('--knobs')
    a = ap.parse_args()
    d, p = find(a.plugin)
    knobs = [int(x) for x in a.knobs.split(',')] if a.knobs else p['defaults']
    exe = os.path.join(HERE, 'build', 'audio_' + p['name'])
    os.makedirs(os.path.dirname(exe), exist_ok=True)
    kind = f'-DENGINE={p["symbol"]}' if p['kind'] == 'synth' else f'-DFX={p["symbol"]}'
    subprocess.run(['cc', '-O1', '-g', '-Wall', '-Wextra', '-Wno-unused-parameter', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=undefined', '-DOP1_HOST', kind, '-I', SDK,
                    os.path.join(HERE, 'audio_harness.c'), *(os.path.join(d, s) for s in p['sources']),
                    '-lm', '-o', exe], check=True)
    r = subprocess.run([exe, *map(str, knobs)], capture_output=True)
    sys.stderr.write(r.stderr.decode())
    out = a.out or os.path.join(HERE, 'out', f'audio_{p["name"]}.wav')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with wave.open(out, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(44100); w.writeframes(r.stdout)
    print(f'{p["kind"]} {p["name"]}, knobs {knobs} -> {out} ({len(r.stdout) / 2 / 44100:.1f} s)')
    if r.returncode:
        sys.exit('FAIL: output silent or the plugin crashed (see above)')


if __name__ == '__main__':
    main()
