# Preset guide

Eight factory sounds, one file each, in [`presets/bloodmoon/`](../presets/bloodmoon/). The same files load with every
platform's plugin. Their settings live in one place,
[`shared/preset-specifications/bloodmoon-presets.json`](../shared/preset-specifications/bloodmoon-presets.json), and
the files are generated from it.

| Preset | BLOOD | ECLIPSE | HAUNTING | MOONRISE | Envelope A / D / S / R | Octave | Delay | Character |
|---|---|---|---|---|---|---|---|---|
| cathedral | 43 | 61 | 21 | 27 | 1088 / 6592 / 26000 / 12000 | 0 | on | The signature sound: dark organ, a bell that sinks into it, restrained vibrato |
| funeral bell | 27 | 100 | 8 | 12 | 64 / 9000 / 6000 / 14000 | 0 | on | A loud, long dissonant bell over a thin organ that settles low |
| crypt drone | 92 | 0 | 27 | 8 | 6000 / 8000 / 32767 / 14976 | -1 | off | No bell; a dense dark stack an octave down, slow swell and drift |
| witch choir | 61 | 27 | 67 | 23 | 3000 / 6592 / 28000 / 12736 | 0 | on | A wavering, vocal organ with a soft attack |
| hollow saint | 0 | 37 | 11 | 18 | 576 / 6592 / 24000 / 9000 | 0 | off | The hollow odd-harmonic organ, restrained bell, dry |
| moon tremor | 55 | 49 | 100 | 92 | 64 / 6592 / 30000 / 6000 | 0 | off | Fast, deep, deliberately unstable pitch |
| sunken pipes | 79 | 12 | 40 | 5 | 2048 / 6592 / 32767 / 14000 | -1 | on | Slow, low, drifting pipes with a long delay |
| blood rite | 100 | 79 | 49 | 37 | 64 / 12352 / 20000 / 12000 | 0 | on | Everything up: thickest stack, forceful bell attack |

Knobs are shown 0-100 as on the screen; the file stores 0-32767. Every pair of presets differs by more than 9 % of the
knob's travel in at least two of the four engine knobs, and they span the engine's range: BLOOD 0 to 100, ECLIPSE 0 to 100, HAUNTING 8 to 100,
MOONRISE 5 to 92. Rendered through the host harness before the envelope, their levels lie within 3.1 dB of each other
(-15.8 to -18.9 dBFS RMS) and peak at 9,900 to 15,420 of 32,767, so nothing clips and no preset jumps out.

The 2026-11 audit kept all eight as they were. They already matched the intended characters, and no test found a
problem. "Hollow saint" keeps its name, because renaming would change the file that users' setups point at.

## Adding a ninth preset, or changing one

1. Edit `shared/preset-specifications/bloodmoon-presets.json`: add or change a sound (lower-case name of at most 15
   characters, file `<name with _>.aif`, knobs and envelope 0-32767, octave -2 to 2, delay null or four values, and a
   one-line character). For a ninth sound, also raise `COUNT` in `tools/preset-validation/validate_presets.py`.
2. Generate the files: `python3 platforms/op1/src/tests/make_presets.py` (writes `presets/bloodmoon/`, needs a C
   compiler for the preview). This rewrites all eight. To keep the shipped files untouched, write to another folder
   and copy only the new one.
3. Validate: `python3 tools/preset-validation/validate_presets.py --render`.
4. Listen: `python3 platforms/op1/src/harness/audio.py bloodmoon --knobs B,E,H,M`.
5. Rebuild the download zip (see [development.md](development.md)) and note the change in `CHANGELOG.md`.

The engine has exactly four parameters. A sound that needs something else is an engine change, with its own tests,
not a preset.

## Format

An OP-1 synth preset is an AIFF-C file (mono, 16-bit, 44.1 kHz, `sowt`) whose `APPL` chunk is `op-1` and the
sound's JSON: `type: "bloodmoon"` (selects the engine), `knobs` (the four engine knobs, then four zeros), `adsr`
(attack, decay, sustain, release, play mode, portamento, two more), `octave`, the effect (`fx_type`, `fx_active`,
`fx_params`) and the LFO (`lfo_type`, `lfo_active`, `lfo_params`, off). The sound data is a one-second preview, which
the OP-1 does not need to play the sound. The shipped previews were rendered before v1.3's output gain and are 3 dB
louder than a fresh render. The settings are unaffected.
