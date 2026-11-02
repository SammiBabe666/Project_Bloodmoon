# Architecture

BLOODMOON is one synthesis design with three plugin builds. The OP-1 (op1r) source is the reference: Hundekraft
compiles the same files, and Opus has its own port that a test holds to the reference.

```
platforms/op1/src/plugins/bloodmoon/      the source of truth
  bloodmoon.c  engine ───────────────┬──► op1r .op1p            (platforms/op1, make_plugin.py)
  moon.c       screen                ├──► Hundekraft .knl       (platforms/hundekraft: plugin.json + these files)
  bloodmoon.svg art ─────────────────┼──► Opus bm_art.h         (platforms/opus/make_headers.py)
                                     └──◄ Opus port's host test (platforms/opus/tests/compare.c)
shared/preset-specifications/bloodmoon-presets.json ──► presets/bloodmoon/*.aif (op1r and Hundekraft; Opus not yet)
```

## The voice

Per note: a fundamental with its 2nd, 3rd and 4th harmonics (an organ), plus two inharmonic bell partials at 1.1875x
and 2.40625x the pitch. All of it is integer Q15 maths, with no floating point and no C library.

| Part | What sets it |
|---|---|
| Harmonic weights | BLOOD (smoothed per block): 2nd and 3rd grow linearly, the 4th with BLOOD squared; normalised so the organ peaks at 18,000 |
| Bell strike level | ECLIPSE at note-on |
| Bell decay rate | ECLIPSE, per block: slow at the top, fast at the bottom |
| Vibrato depth | HAUNTING x a fade that starts after 40 audio blocks and rises over the next 37 |
| Pitch instability | HAUNTING, a second LFO at 3x the vibrato rate |
| LFO rate | MOONRISE squared, 0.15 Hz to 9 Hz |
| Output gain | 0.71 (v1.3), so the built-in speaker does not crackle |

The OS supplies the envelope (T2), velocity, the mix and the effects on every platform.

## Platform differences

| | op1r (`.op1p`) | Hundekraft (`.knl`) | Opus 2 (`.opmod`) |
|---|---|---|---|
| Engine source | `platforms/op1/src/plugins/bloodmoon/` | the same files | `platforms/opus/src/noladrift.bloodmoon/` (port) |
| Voice state | iter's 0x24-byte object | the same | the core's 32 voice bytes |
| Sine table | the firmware's own, at a fixed address | the same | its own 4 KB quarter table, in fast memory |
| Render | every sample of the OS block | the same | every second sample of the 128-sample, 88.2 kHz block, the rest interpolated |
| Velocity | applied in render and by the OS | the same | by the OS only |
| Knobs | the stored knobs | the same | the knobs with the OS's LFO in |
| Screen | op1r screen API | kennel's | Opus engine screen (core 2.6), the same SVG |

## Screen

The SVG has one group per moving part: `moon`, `rose`, `bellL`, `bellR`, `stars0`-`stars2`, and the knob values as
`k<knob>v` with one group per digit glyph, `k<knob>p<position>d<digit>`. `moon.c` finds them once and then only
changes transforms and visibility each frame. It adds no extra drawing.
