# BLOODMOON changelog

| Version | Date | In one line | On a real OP-1 |
|---|---|---|---|
| v1.5 | 31 Oct 2026 | A ready-made download zip: the Opus 2 `.opmod` and the Hundekraft `.knl`, nothing to build | Not yet tested |
| v1.4-knl (experimental) | 22 Oct 2026 | Same as v1.3, packed as a Hundekraft 3.0 `.knl` plugin | Not yet tested |
| **v1.3** (stable) | 21 Oct 2026 | Output level set to +3 dB over v1.0 to stop speaker crackle | Plays well |
| v1.2 | 21 Oct 2026 | Knob names and values on the screen, 8 presets, +6 dB | Screen shows; crackled on the speaker |
| v1.1 | 21 Oct 2026 | +6 dB louder, old screen | Not reported separately |
| v1.0 | 20 Oct 2026 | First release | Works |

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
