# BLOODMOON for the OP-1 (op1r plugin loader)

The original BLOODMOON: an op1r plugin (`.op1p`) for the original OP-1 on firmware R246 with op1r's plugin loader.
**v1.3 is the stable release and plays well on a real OP-1.** Its file is
[`releases/v1.3/bloodmoon.op1p`](../../releases/v1.3/bloodmoon.op1p).

This folder is also the **source of truth** for the sound, the screen and the art. The Hundekraft build is made from
these same files, and the Opus port is tested against them.

## Where things are

| Path | What |
|---|---|
| `src/plugins/bloodmoon/bloodmoon.c` | The engine: organ, bell, vibrato, the four knobs |
| `src/plugins/bloodmoon/moon.c` | The T1 screen: moon, rose window, bells, stars, knob values |
| `src/plugins/bloodmoon/bloodmoon.svg` | The screen art (the only copy; Opus and Hundekraft use this one) |
| `src/plugins/bloodmoon/plugin.json` | op1r's manifest: name, symbol, sources, default knobs, screen |
| `src/op1_plugin.h`, `src/PLUGINS.md` | op1r's plugin API and its guide (from the op1r SDK) |
| `src/harness/` | The op1r host harness: renders a plugin on a computer with sanitizers |
| `src/plugload/`, `src/make_plugin.py`, `src/plugins.py` | op1r's loader format and the `.op1p` packer |
| `src/tests/` | Tools that made the presets (`make_presets.py`), the knob lettering (`make_labels.py`), the screen preview (`preview.py`) and the screen test (`screen_test.c`) |
| `tests/run.sh` | The OP-1 build's host tests |

The presets are in [`presets/bloodmoon/`](../../presets/bloodmoon/) at the top of the repository, shared by every
platform. Their settings are in
[`shared/preset-specifications/bloodmoon-presets.json`](../../shared/preset-specifications/bloodmoon-presets.json).

## Install

Back up the OP-1 first. Install `releases/v1.3/bloodmoon.op1p` with the
[Hundekraft OP-1 Workshop](https://hundekraft.github.io/op1-workshop/index.html) (it accepts op1r plugin files), or
put it in `/yaffs2/plugins/` of an op1r firmware with the loader. Then choose **bloodmoon** with **shift + T1** on a
synth slot. Presets: in disk mode, copy `presets/bloodmoon/` into the OP-1's `synth/` folder.

## Build

The `.op1p` is packed by op1r's tool, which compiles in a `bfin-elf-toolchain` Docker image:

```
cd platforms/op1/src
python3 make_plugin.py plugins/bloodmoon        # -> build/plugins/bloodmoon.op1p
```

It needs Docker with that image and `pyelftools`. The image is op1r's, not part of this repository, and it was not
rebuilt for the move into `platforms/`. A new build is a development build; it replaces nothing in `releases/`.

## Validate

```
platforms/op1/tests/run.sh                                     # engine with sanitizers, screen digits
python3 platforms/op1/src/harness/audio.py bloodmoon --knobs 14000,20000,7000,9000   # listen: harness/out/*.wav
python3 tools/preset-validation/validate_presets.py --render   # the presets
```

## Limitations

- Presets not yet confirmed on a real OP-1.
- Six voices at full level on the built-in speaker not checked.
- The screen assumes the OS passes frame time in milliseconds.
- The art is 42 KB, above the Workshop's 8 KiB artwork guideline (it shows fine).
