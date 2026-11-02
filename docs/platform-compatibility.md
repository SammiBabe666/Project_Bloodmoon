# Platform compatibility

All three builds need an **original OP-1** (not the OP-1 Field) on firmware **R246**.

| | op1r loader | Hundekraft | Opus 2 |
|---|---|---|---|
| Firmware mod | op1r with the plugin loader | Hundekraft 3.0+ (3.0.3 SDK) | Opus 2, core 2.6+ |
| File, version | `bloodmoon.op1p` 1.3 | `bloodmoon.knl` 1.4 | `bloodmoon.opmod` 1.5.2 |
| Identity | engine `bloodmoon` | plugin ID `bloodmoon`, no claims | id `noladrift.bloodmoon`, claims sound type `bloodmoon` |
| Presets load | yes (`type: bloodmoon`) | yes (same ID) | probably not: the kit documents a type bridge, but Opus's author says such sounds do not load; Opus presets need the format Opus saves |
| Sound vs v1.3 | the reference | identical code | within 1 dB; up to 0.8 dB less top harmonic on the highest notes |
| CPU | about 8.5 % of the audio block for six voices (simulator, v1.3) | as op1r | 68,207 cycles for six voices in the simulator, 12 % of the 580,525-cycle block; figure 120,000 (provisional) |
| Memory | 32 voice bytes, firmware table | the same | 32 voice bytes x 6, 4.2 KB of shared fast memory, a 42 KB art file written to its folder |
| On a real OP-1 | **plays well** | not tested | 1.5.0 and 1.5.1 switched off for CPU; **1.5.2 not tested** |

## Living with other mods

- Install only one BLOODMOON file. The op1r `.op1p` and the Hundekraft `.knl` share the ID `bloodmoon`. On Opus, the
  `.opmod` and an `.op1p` loaded through Opus's bridge would both claim the sound type, and Opus refuses the second.
- The Opus build runs nothing on the audio thread except its voices. It has no file work or allocation there and logs
  only at start. Its globals are `static` or prefixed `bm_`.
- Opus switches a plugin off when it runs 25 % over its CPU figure in 8 of 64 blocks. Developer mode never does. Measure
  the real figure with developer mode on (orange push on its row in the mods list, "audio: worst N") and put it in
  `platforms/opus/src/noladrift.bloodmoon/cpu.txt`.

Nothing on this page is confirmed on hardware unless it says so.
