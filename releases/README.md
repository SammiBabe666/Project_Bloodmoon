# Releases

Every plugin file BLOODMOON has shipped, kept unchanged. The ready-to-install bundle is in
[`download/`](../download/). A file built from the current source is a development build until it has been tested and
copied here or into `download/` by hand.

| Version | File | Platform | Built from | SHA-256 | On a real OP-1 |
|---|---|---|---|---|---|
| 1.5.3 (test) | [`v1.5.3-test/bloodmoon.opmod`](v1.5.3-test/bloodmoon.opmod) | Opus 2, core 2.6+ | this branch (Opus kit `d9b0d2e`, shared toolchain GCC 13.3.0 / binutils 2.38) | `52b92af3…dc303` | Not yet tested (fixes retrigger pops) |
| 1.5.2 | [`download/opus/bloodmoon.opmod`](../download/opus/bloodmoon.opmod) | Opus 2, core 2.6+ | `67ff2f4` (Opus kit `d9b0d2e`, GitHub Actions) | `854d06e7…dd74e` | Not yet tested |
| 1.4 | [`v1.4-knl/bloodmoon.knl`](v1.4-knl/bloodmoon.knl), also `download/hundekraft/` | Hundekraft 3.0+ | `knl/source/` at `1152b7f` (Hundekraft SDK 3.0.3, bfin-elf-gcc 13.3) | `e8b8a691…41201` | Not yet tested |
| 1.3 | [`v1.3/bloodmoon.op1p`](v1.3/bloodmoon.op1p) | op1r loader | `src/` at `3cac835` (op1r `make_plugin.py`) | `8440dc1e…6c606` | Plays well |
| 1.2 | [`v1.2/bloodmoon.op1p`](v1.2/bloodmoon.op1p) | op1r loader | Before this repository (added in `1152b7f`) | `239943f3…98afd` | Screen shows; crackled on the speaker |
| 1.1 | [`v1.1-louder/bloodmoon.op1p`](v1.1-louder/bloodmoon.op1p) | op1r loader | Before this repository (added in `1152b7f`) | `1ca127cc…d5870` | Not reported |
| 1.0 | [`v1.0/bloodmoon.op1p`](v1.0/bloodmoon.op1p) | op1r loader | Before this repository (added in `1152b7f`) | `45c2a18f…1a2be` | Works |

The Opus builds 1.5.0 and 1.5.1 were switched off for their CPU on a real OP-1 and are not kept as files (their
source is in the history: `9dde706` and `248e363`). `download/bloodmoon-1.5.2.zip` bundles the 1.5.2 `.opmod`, the 1.4
`.knl`, the eight presets, `INSTALL.md`, `LICENSE` and `NOTICE`.

Only the `.opmod` has been checked against its source: rebuilt from `platforms/` after the reorganisation (the
Build Opus plugin workflow, 2 Nov 2026), it is byte-identical to the released file. The `.knl` and the
`.op1p` files were not rebuilt here: their toolchains (the Hundekraft SDK, op1r's Docker image) are not in this
repository.
