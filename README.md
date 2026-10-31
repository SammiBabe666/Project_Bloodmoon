# BLOODMOON

**A dark organ with a haunted bell, for the original Teenage Engineering OP-1.**
A custom synth engine plugin by Nola Drift, made for gothic ambient, witch house and ruined cathedrals.

![BLOODMOON screen preview: eight states of the moon-and-cathedral screen](bloodmoon_screen_preview.png)

*Preview drawn on a computer from the plugin's own screen code. The OP-1's display may look slightly different.*

## Download

**[Download BLOODMOON 1.5 (zip)](download/bloodmoon-1.5.zip)**: everything you need, nothing to build. Inside:

| For | File |
|---|---|
| **Opus 2** (LizardMan's plugin manager, core 2.6 or newer) | `opus/bloodmoon.opmod`, which you copy into the OP-1's mods folder |
| **Hundekraft 3.0** or newer | `hundekraft/bloodmoon.knl`, which you add in the Hundekraft Workshop |
| Either one | `presets/bloodmoon/` (8 presets) and `INSTALL.md` (the steps for both) |

Both files play the same sound, knobs and screen as v1.3. Neither has been tried on a real OP-1 yet, so please report
how it goes. The last version tested on hardware is the v1.3 op1r file in [`releases/v1.3/`](releases/v1.3/).
See [CHANGELOG.md](CHANGELOG.md).

🎥 [30-second demo](media/bloodmoon_demo.mp4) · [preset tour](media/bloodmoon_preset_tour.mp4) (rendered on a computer from the plugin's own sound and screen code)

---

## The sound

Every note is an organ: a fundamental with its 2nd, 3rd and 4th harmonics. A bell is struck on top of it, made of two inharmonic partials at about 1.19x and 2.41x the pitch (the lower one close to a minor third), so the bell never quite agrees with the organ. It rings, then sinks into the organ.

A slow vibrato fades in after about a tenth of a second, with a faint wobble underneath, so held notes drift out of tune like old pipes. Use the OP-1's own envelope (T2) to shape the attack and release.

## The four knobs

| Encoder | Name | What it does |
|---|---|---|
| Blue | **BLOOD** | Harmonic richness. Hollow and thin at the bottom, thick and dark at the top. |
| Green | **ECLIPSE** | The bell strike: how loud it hits and how long it rings. Fully down means no bell. Up means a loud, long, dissonant bell. |
| White | **HAUNTING** | Vibrato depth (up to about ±73 cents, fading in after the attack) plus a steady pitch instability (up to about ±18 cents). |
| Orange (drawn red on screen) | **MOONRISE** | Vibrato and wobble speed, from about 0.15 Hz to 9 Hz. Most of the knob's travel is spent in the slow end. |

The knobs start at about 43 %, 61 %, 21 % and 27 % (blue, green, white, orange).

## The screen

A black cathedral under a blood-red moon. Each knob moves the part of the picture drawn in its colour (the moon is drawn red, for the orange knob):

| Knob | On screen |
|---|---|
| MOONRISE | The moon rises from behind the cathedral as you turn it up. |
| BLOOD | The blue rose window turns, half a turn over the knob's travel. |
| ECLIPSE | The two green tower bells swing in opposite directions, up to about ±20 degrees. Still at zero. |
| HAUNTING | The white stars glow steadily at zero and flicker more as you turn it up. |

Each knob's name and its value (0 to 100) are drawn in blackletter on both sides of the cathedral, in the knob's own colour: Blood and Eclipse on the left, Haunting and Moonrise on the right.

## Presets

Eight presets are in `presets/bloodmoon/`. Back up your OP-1's `synth` folder first, then in disk mode copy the whole `bloodmoon` folder into `synth`. On an OP-1 without the plugin, they load as the stock iter engine.

| Preset | Blood | Eclipse | Haunting | Moonrise | Character |
|---|---|---|---|---|---|
| cathedral | 43 | 61 | 21 | 27 | the default sound, soft attack, delay |
| funeral bell | 27 | 100 | 8 | 12 | loud long bell, long release, delay |
| crypt drone | 92 | 0 | 27 | 8 | dark organ, slow attack, octave down |
| witch choir | 61 | 27 | 67 | 23 | heavy slow vibrato, delay |
| hollow saint | 0 | 37 | 11 | 18 | thin, hollow organ |
| moon tremor | 55 | 49 | 100 | 92 | fast, deep warble |
| sunken pipes | 79 | 12 | 40 | 5 | slow drifting pipes, octave down, delay |
| blood rite | 100 | 79 | 49 | 37 | thick organ with a strong bell, delay |

## Install

Unzip [the download](download/bloodmoon-1.5.zip) and follow its `INSTALL.md`. In short:

- **Opus 2:** in disk mode, copy `opus/bloodmoon.opmod` into the **mods** folder and restart. The bloodmoon row in
  Opus's mods list (menu, **T2**) should say "loaded". Then choose it with **shift + T1** on a synth slot.
  If anything goes wrong, hold **Stop** while you switch on to start with no plugins.
- **Hundekraft 3.0:** in the [Hundekraft OP-1 Workshop](https://hundekraft.github.io/op1-workshop/index.html), add
  `hundekraft/bloodmoon.knl` under **Your plugins** and press **Update OP-1**. Then choose it with **shift + T1**.

Back up your OP-1 (tapes and synth presets) in disk mode first, and start with the volume low.

## What's in this repository

| File | What it is |
|---|---|
| `download/` | The ready-made download: `bloodmoon-1.5.zip`, and the same files unzipped. |
| `opus/` | Source of the Opus 2 build (`noladrift.bloodmoon/`), its computer test and notes. |
| `knl/` | Source of the Hundekraft build and its notes. |
| `src/` | The original op1r source (engine, screen code, artwork) and the tools that made the presets and knob panels. |
| `presets/bloodmoon/` | The 8 presets. |
| `releases/` | Older plugin files: v1.0, v1.1-louder, v1.2 and v1.3 (the op1r `.op1p` files). |
| `media/`, `docs/` | Demo videos, release posts and the changelog as a Word document. |
| `CHANGELOG.md` | What changed in each version. |

## How it was checked

These checks are for v1.3, the op1r `.op1p` build. The Opus 2 and Hundekraft builds use the same sound and screen code; their own checks are in [`opus/README.md`](opus/README.md) and [`knl/INSTALL.md`](knl/INSTALL.md).

| Check | Result |
|---|---|
| Sound engine on a computer, with memory-error detectors, at several knob settings | No problems found. At the default knobs the loudest peak is about -8.2 dBFS before the OP-1's envelope (v1.3). |
| The finished `bloodmoon.op1p` against the OP-1 plugin loader's checks and the Workshop's upload checks (checksum, format, size, name, screen-art rules) | Passes |
| The compiled Blackfin code run in a processor simulator, compared with the computer version across 34 knob, pitch and six-voice cases | Identical output, sample for sample. No out-of-bounds or misaligned memory access, no exceptions |
| Processor cost in that simulator | About 1.4 % of the audio time budget per voice, about 8.5 % for six voices at 400 MHz (a rough figure that leaves out memory delays; a pessimistic estimate is about 26 %) |
| Screen code in the simulator against the computer version, 588 knob and timing combinations | Identical |
| **On a real OP-1** | **v1.3 plays well, screen and knob panels show (R246). Presets not yet confirmed.** |

The simulator is not the OP-1's firmware. Cache and memory delays, interrupt timing, the real display and the contents of the firmware's own tables are only settled by running it on the device.

## Known limits

- The presets have not yet been confirmed on a real OP-1.
- The level is set so the OP-1's built-in speaker doesn't crackle (v1.2 was 3 dB louder and did). Six voices at full level on the speaker have not been checked.
- The screen animation assumes the OP-1 passes frame time in milliseconds. If the bells or stars look much too fast or too slow, please report it.
- The artwork is about 42 KB, above the Workshop's 8 KiB "artwork budget" (a guideline, not a limit). It shows fine on the OP-1. It also ends about 8 pixels above the bottom of the screen, and the Workshop's guide advises leaving room there, so check that nothing is cut off.
- Only the first page of four knobs is used.

## Feedback

Please open an issue and tell us your OP-1's firmware version (it should be R246), what you heard or saw, which firmware mod it runs (Opus 2 or Hundekraft) and any message it showed.

## Credits and licence

- BLOODMOON engine, screen code and artwork by **Nola Drift**.
- Built with the op1r plugin SDK by **pcppcp** and contributors ([op1r-src](https://github.com/pcppcp/op1r-src)). The structure follows the SDK's synth template, and the vibrato fade-in idea and screen structure follow the Brassophone plugin by **nvmsocool** and the SDK's example plugins.
- The knob lettering is outlines of Pirata One by Rodrigo Fuenzalida and Nicolas Massi (SIL Open Font License 1.1), credited in `NOTICE`.
- Installed with the [Hundekraft OP-1 Workshop](https://hundekraft.github.io/op1-workshop/index.html).
- OP-1 is a Teenage Engineering product. This project is not affiliated with or endorsed by Teenage Engineering and contains no Teenage Engineering firmware.

Licensed under **GPL-3.0-or-later with the additional OP-1 firmware permission** described in `LICENSE` and `NOTICE`. If you share the plugin files, share their source too: `opus/`, `knl/` and `src/` in this repository are that source.
