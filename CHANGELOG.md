# BLOODMOON changelog

| Version | Date | In one line | On a real OP-1 |
|---|---|---|---|
| Repository 2026-11 | 2 Nov 2026 | Reorganised into `platforms/`, preset validator, tests for all three platforms in CI. No plugin file changed | (no new files) |
| v1.5.2 | 2 Nov 2026 | Opus build: the voice costs a quarter of 1.5.1's CPU, measured | Not yet tested |
| v1.5.1 | 1 Nov 2026 | Opus build: sine table in fast memory | Failed: switched off after three notes, organ not heard |
| v1.5 | 31 Oct 2026 | A ready-made download zip: the Opus 2 `.opmod` and the Hundekraft `.knl`, nothing to build | Not yet tested |
| v1.4-knl (experimental) | 22 Oct 2026 | Same as v1.3, packed as a Hundekraft 3.0 `.knl` plugin | Not yet tested |
| **v1.3** (stable) | 21 Oct 2026 | Output level set to +3 dB over v1.0 to stop speaker crackle | Plays well |
| v1.2 | 21 Oct 2026 | Knob names and values on the screen, 8 presets, +6 dB | Screen shows; crackled on the speaker |
| v1.1 | 21 Oct 2026 | +6 dB louder, old screen | Not reported separately |
| v1.0 | 20 Oct 2026 | First release | Works |

## Repository reorganisation (2 Nov 2026)

No plugin file or preset changed. `download/` and `releases/` hold the same files as before, and the 1.5.2 zip
rebuilds byte for byte with `tools/build-platform/make_zip.py`.

- **Moved** (history kept): `src/` to `platforms/op1/src/`, `opus/` to `platforms/opus/` (the plugin folder is now
  `platforms/opus/src/noladrift.bloodmoon/`), `knl/` to `platforms/hundekraft/` (manifest in `src/plugin.json`, guide in
  `README.md`), `knl/bloodmoon.knl` to `releases/v1.4-knl/`, and the preview images to `media/`. The presets stay in
  `presets/bloodmoon/`, one set for all three platforms.
- **Removed duplicates:** the Hundekraft folder's copies of `bloodmoon.c`, `moon.c` and `bloodmoon.svg` were
  byte-identical to the OP-1 source. `tools/build-platform/stage-hundekraft.sh` now builds the kennel folder from that
  source. The Opus folder's copy of `bloodmoon.svg` was also removed: `make_headers.py` reads the OP-1 one, and the
  generated art is unchanged.
- **Presets:** the eight sounds' settings are now in `shared/preset-specifications/bloodmoon-presets.json`.
  `make_presets.py` reads them, and `tools/preset-validation/validate_presets.py` checks the specification, the files,
  the format, the ranges, duplicates, the install path and the rendered levels. The audit kept all eight as they
  were.
- **Tests:** `platforms/op1/tests/run.sh` and `platforms/hundekraft/tests/run.sh` are new. GitHub runs all three
  platforms' tests and the preset check on every push. A manual **Build Opus plugin** workflow builds and benchmarks
  the `.opmod` without committing it.
- **Docs:** a README per platform, plus `docs/architecture.md`, `installation.md`, `preset-guide.md`,
  `platform-compatibility.md`, `development.md`, `releases/README.md` (which commit made each file) and
  `shared/design-notes/sound-identity.md`.
- `preview.py` no longer hard-codes a browser path.

## v1.5.2: Opus voice rebuilt for the OP-1's processor

On a real OP-1, 1.5.1 was still switched off for its CPU after three notes, and only the bell was heard. A Blackfin simulator (GDB's, `platforms/opus/tests/bench_bfin.sh`, run by the manual Build Opus plugin workflow) showed why: 1.5.1's six voices cost 247,335 cycles a block, about 41,000 a voice, so three voices went past the core's limit (25 % over the 120,000-cycle figure). 1.5.2's six voices cost 68,207 in the same test:

- the sound is computed at 44.1 kHz, the OS's own rate, and each sample between is the mean of its neighbours;
- the vibrato and instability LFOs are read once a block (689 times a second) instead of every sample;
- the 2nd, 3rd and 4th harmonics come from the fundamental's sine and cosine by the multiple-angle formulas: two table reads instead of four, the same waveform;
- 16-bit multiplies, no branches and no clamp per sample, no library division, and the organ and bell computed in separate passes so the processor never runs out of registers.

The sound stays close to v1.3: every note within 1 dB, and at the computed points the difference is 40 to 69 dB below the sound (`platforms/opus/tests/run.sh`, now also run with sanitizers). The highest notes lose up to 0.8 dB of their top harmonics. The CPU figure stays a provisional 120,000 until it is measured on a device.

The missing organ is not yet explained. The organ and bell read the same table, so the likeliest cause is the slot's T2 envelope: a new engine keeps the previous sound's, and a short one lets only the attack through, where the bell is loudest. Please test with a long sustain.

## v1.5.1: Opus build, first CPU fix (failed on the device)

On a real OP-1, 1.5.0's Opus build drew its screen but made no sound: Opus showed "audio switched off: over its CPU". It had no CPU figure, so Opus held it to a tenth of the audio block (58,052 cycles), and its 16 KB sine table, read from ordinary memory in every sample of six voices, cost far more than that (Opus counts about 107 cycles for each 32-byte line of ordinary memory an audio block touches). 1.5.1:

- copies a 4 KB quarter of the sine table (the same values, folded by symmetry) into the OP-1's fast memory at start;
- skips what is silent: the vibrato LFOs at HAUNTING 0, the 4th harmonic while BLOOD is low, and the bell once its strike has died away;
- carries a provisional CPU figure of 120,000 cycles until it is measured on a device.

The sound is unchanged: the computer test still matches v1.3 to within 2 of 32,767. The `.knl` is unchanged.

## v1.5: download and play

- New: BLOODMOON for **Opus 2** (`bloodmoon.opmod`, core 2.6 or newer). It uses the same sound and screen code as v1.3 on Opus's engine service, and op1r's sound type, so existing presets load. Velocity is now applied once, by the OS, so soft notes are a little louder than on v1.3. The T4 LFO can move the four knobs.
- `download/bloodmoon-1.5.zip` holds the `.opmod`, the v1.4 Hundekraft `.knl`, the 8 presets and one install guide for both. Nobody needs to build anything.
- The build workflows and build guides were removed. The source stays in `opus/`, `knl/` and `src/`. The v1.3 `.op1p` moved to `releases/v1.3/`.

## v1.4-knl: Hundekraft 3.0 port (experimental)

`knl/bloodmoon.knl` is v1.3's sound, knobs and screen packed as a Hundekraft 3.0 kennel plugin (built against the 3.0.3 SDK, bfin-elf-gcc 13.3). Plugin ID `bloodmoon`, so saved sounds and the presets keep working. Install only one of the `.op1p` and the `.knl`.

Checked with the SDK's tools (packer with no warnings, reference loader at two load addresses, `svg_check.py`, export test). It has not been loaded on a real OP-1 or the emulator. See `knl/INSTALL.md`.

## v1.3: level for the speaker

A fixed output gain of 0.71 (`OUT_GAIN = 23170`, Q15) after the voice is summed and scaled by velocity, in `bloodmoon.c`. This is halfway between v1.0 and v1.2's +6 dB, which made the OP-1's built-in speaker crackle (headphones were clean). Nothing else changed from v1.2: same screen, same presets.

| Knobs (one voice, before the envelope) | v1.0 | v1.2 | v1.3 |
|---|---|---|---|
| Defaults (43, 61, 21, 27) | -11.2 dBFS | -5.2 dBFS | -8.2 dBFS |
| All at 100 | -9.5 dBFS | -3.4 dBFS | -6.5 dBFS |
| All at 0 | -14.0 dBFS | -8.0 dBFS | -11.0 dBFS |

The compiled Blackfin code matches the computer version sample for sample in a simulator. The peak per voice is limited to about 19,000 of 32,767, so one voice can't clip.

## v1.2: knob panels and presets

- Each knob's name and live 0 to 100 value are drawn on the sides of the screen in Pirata One (SIL OFL 1.1), converted to flat filled shapes so the OP-1 needs no font support. One SVG group per digit and position; `moon.c` shows the right digits each frame and centres them, using only the calls the screen already used and no division.
- The SVG grew from 14 KB to 42 KB (about 290 shapes), in line with op1r's built-in iter lab screen (46 KB, about 286 shapes).
- 8 presets (OP-1 synth preset `.aif` files with the settings as JSON in an APPL chunk). `"type": "bloodmoon"` selects the plugin by name.
- New tools in `src/tests/`: `make_labels.py`, `make_presets.py`, `screen_test.c` and `preview.py`.

## v1.1: louder

Output at full scale (`>> 15`) instead of half scale (`>> 16`): +6 dB, defaults from -11.2 to -5.2 dBFS per voice.

## v1.0: first release

Organ (fundamental plus 2nd to 4th harmonics) with two inharmonic bell partials at about 1.19x and 2.41x the pitch. Knobs: Blood, Eclipse, Haunting, Moonrise. Screen: a cathedral under a blood moon.
