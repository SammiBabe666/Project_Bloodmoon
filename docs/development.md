# Development

## Layout

| Path | What |
|---|---|
| `platforms/op1/` | The op1r build and the source of truth for engine, screen and art ([README](../platforms/op1/README.md)) |
| `platforms/opus/` | The Opus 2 port, its tests and cycle benchmark ([README](../platforms/opus/README.md)) |
| `platforms/hundekraft/` | The kennel manifest; the code comes from `platforms/op1` ([README](../platforms/hundekraft/README.md)) |
| `presets/bloodmoon/` | The eight presets, for all platforms |
| `shared/preset-specifications/` | The presets' settings (the source the files are made from) |
| `shared/design-notes/` | What makes the sound BLOODMOON, for anyone changing it |
| `tools/preset-validation/` | `validate_presets.py` |
| `tools/build-platform/` | `build-opus.sh`, `stage-hundekraft.sh` |
| `download/` | The released bundle users download; changed by hand only |
| `releases/` | Every released plugin file and the revision that made it |
| `build/`, `platforms/*/build/` | Build output (not in git) |

## Validate everything

On any Linux or macOS machine with a C compiler and Python 3, from the repository root:

```
python3 tools/preset-validation/validate_presets.py --render
platforms/op1/tests/run.sh
platforms/hundekraft/tests/run.sh
git clone https://github.com/LizardMan245/opus /tmp/opus-kit && git -C /tmp/opus-kit checkout d9b0d2e6ddf660d7db758f2be9647585fe100ad8
python3 platforms/opus/make_headers.py && git diff --exit-code -- platforms/opus/src
OPUS_SDK=/tmp/opus-kit/plugin-kit/sdk platforms/opus/tests/run.sh
SANITIZE="-fsanitize=address,undefined -fno-sanitize-recover=all" OPUS_SDK=/tmp/opus-kit/plugin-kit/sdk platforms/opus/tests/run.sh
```

GitHub runs the same on every push (`.github/workflows/tests.yml`).

| Test | Checks |
|---|---|
| `validate_presets.py --render` | Eight sounds; names, files, ranges, duplicates; AIFF-C format; settings equal the spec; install path in the docs; each renders, is not silent, does not clip, levels within 6 dB |
| `platforms/op1/tests/run.sh` | The engine at default, minimum and maximum knobs with sanitizers; screen digits and centring |
| `platforms/hundekraft/tests/run.sh` | The staged kennel folder: manifest identity and defaults, the engine rendered with sanitizers |
| `platforms/opus/tests/run.sh` | The port against v1.3 over 7 knob sets x 8 notes, held and released: level within 1 dB, difference at least 50 dB down (35 with the LFOs on), never silent, never full scale; table, screen nodes and transforms |
| `platforms/opus/tests/bench_bfin.sh` | Cycles a block for six voices on GDB's Blackfin simulator, optionally beside an older revision |

## Build each platform

- **Opus:** `OPUS_KIT=… OP1_BFIN_TOOLCHAIN=… tools/build-platform/build-opus.sh` writes `build/opus/bloodmoon.opmod`.
  Without a local compiler, use the manual **Build Opus plugin** workflow (Actions tab, "Run workflow"). It builds,
  benchmarks, reports whether the file equals the released one, and attaches it to the run without committing.
- **Hundekraft:** `tools/build-platform/stage-hundekraft.sh`, then pack with the Hundekraft 3.0.3 SDK
  ([README](../platforms/hundekraft/README.md#rebuild)).
- **op1r:** `cd platforms/op1/src && python3 make_plugin.py plugins/bloodmoon` (needs op1r's Docker toolchain image).

## Package a release

1. Bump the version: `platforms/opus/src/noladrift.bloodmoon/manifest.txt` and/or
   `platforms/hundekraft/src/plugin.json`.
2. Run every test above and build the changed platform.
3. Test the new file on a real OP-1 and write what was seen in `CHANGELOG.md`.
4. Copy the file into `download/<platform>/`, add a row to `releases/README.md` (version, commit, SHA-256, hardware
   result), and rebuild the zip:

```
python3 tools/build-platform/make_zip.py 1.5.3
```

5. Update the version and links in `README.md` and `download/INSTALL.md`.

Never overwrite a file in `releases/`. A build that has not been tested on hardware is a development build.
