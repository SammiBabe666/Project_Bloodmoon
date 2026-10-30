# BLOODMOON 1.4 for Hundekraft (.knl)

> **Experimental: not yet tested on a real OP-1.** The stable version is v1.3 (`bloodmoon.op1p` at the top of this repository).

`bloodmoon.knl` is BLOODMOON packed as a Hundekraft 3.0 kennel plugin. The sound, the four knobs and the cathedral screen are the same as v1.3; only the file format changed (v1.3 was an op1r `.op1p`).

- Needs: an original OP-1 running Hundekraft 3.0 or newer (built against the 3.0.3 SDK).
- File: `bloodmoon.knl`, 46,148 bytes, SHA-256 `e8b8a691bdfbdb6322f3876a34399a1eb101859568fadb09ecdf7cd21eb41201`.
- Plugin ID: `bloodmoon` (synth). Author Nola Drift, version 1.4. Claims no exclusive controls.

## Install

1. Open the [Hundekraft OP-1 Workshop](https://hundekraft.github.io/op1-workshop/index.html).
2. Under **Your plugins**, add `bloodmoon.knl`.
3. Connect the OP-1 (already on Hundekraft 3.0) and press **Update OP-1**. Read the review, send, and press T1 on the OP-1 when asked.
4. After the restart, open synth, then Shift + T1, and pick **bloodmoon**.

If you already installed the v1.3 `bloodmoon.op1p`, remove it from Your plugins first so only one bloodmoon is installed. Both use the ID `bloodmoon`, so saved sounds keep pointing at it.

## Controls

| Knob | Name | What it does |
|---|---|---|
| Blue | BLOOD | Harmonic richness: hollow odd-harmonic organ down, dense dark stack up |
| Green | ECLIPSE | Bell strike at the attack: none down, loud long dissonant bell up |
| White | HAUNTING | Vibrato depth (fades in after the attack) plus slight pitch instability |
| Orange | MOONRISE | Vibrato speed, about 0.15 Hz to 9 Hz |

The v1.3 presets (`presets/bloodmoon/`) use the same engine name and knob meanings.

## Rebuild from source

1. Get the matching SDK from the Workshop: `source/hundekraft-3.0.3-source.tar.gz`, unpack, `cd ext/kennel`.
2. Copy this repository's `knl/source/` folder to `synth_sdk/plugins/bloodmoon/`.
3. With a Blackfin `bfin-elf-gcc` 13.3 toolchain on PATH:
   `python3 tools/kennel/pack.py synth_sdk/plugins/bloodmoon -o build/bloodmoon.knl`

The build is reproducible: the same SDK and compiler give the same bytes and hash as above.

## What was checked

- Packed with the 3.0.3 packer with no warnings: .text 2672, .rodata 80, .data 4, .bss 548 bytes, 65 relocations, no core imports, 42 KB SVG embedded.
- Parsed and relocated by the SDK's reference loader (`tools/kennel/reloc.py`) at two load addresses.
- Screen SVG passes the SDK's `svg_check.py`.
- SDK export test passes for bloodmoon.
- **Not yet checked on hardware.** It has not been loaded on a real OP-1 or the emulator.

GPL-3.0-or-later with the OP-1 firmware permission (see `LICENSE` and `NOTICE` at the top of this repository).
