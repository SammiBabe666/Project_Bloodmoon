# BLOODMOON

**A dark organ with a haunted bell, for the original Teenage Engineering OP-1.**
A custom synth engine plugin by Nola Drift, made for gothic ambient, witch house and ruined cathedrals.

![BLOODMOON screen preview: eight states of the moon-and-cathedral screen](bloodmoon_screen_preview.png)

*Preview drawn on a computer from the plugin's own screen code. The OP-1's display may look slightly different.*



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

## Install

You need an **original OP-1** (not the Field), its original unmodified firmware file `op1_246.op1`, and the plugin file `bloodmoon.op1p`.

1. **Get the plugin file.** Download `bloodmoon.op1p` from this repository's **Releases** page. If there is no release yet, [build it yourself](#build-it-yourself-no-coding-needed).
2. **Back up your OP-1** (tapes, synth and drum presets) using the OP-1's disk mode.
3. Open the [Hundekraft OP-1 Workshop](https://hundekraft.github.io/op1-workshop/index.html) and give it your original `op1_246.op1`.
4. Under **Your plugins**, add `bloodmoon.op1p`. The Workshop checks the file. If it shows a compatibility note, read it before you acknowledge it.
5. Build and install as the Workshop's page tells you (this normally means holding COM while switching the OP-1 on, to install firmware). If your OP-1 already runs a Hundekraft 3.0 firmware, you can connect it and use **Update OP-1** instead.
6. On the OP-1, press **T1** on a synth patch and choose **bloodmoon**. Start with the volume low.

## If something goes wrong

- If the OP-1 hangs while starting up, switch it off and on again. The Workshop's firmware leaves a marker file while it loads plugins and removes it once the screen has been running for about 7 to 10 seconds. If the marker is still there at the next start, it skips all plugin files for that boot.
- If it locks up later, or misbehaves in any way: switch it off, hold **COM** while switching it on, choose firmware update, and install a Workshop build without BLOODMOON. If your OP-1 already runs a Hundekraft 3.0 firmware, you can instead use the Workshop to remove the plugin. The Workshop says your tapes, album and presets stay in place.

## Build it yourself (no coding needed)

GitHub builds the plugin file for you on its free servers. You only click buttons.

1. Put `bloodmoon-build-kit.zip` and `.github/workflows/build.yml` from this repository into a GitHub repository of your own.
2. Open the **Actions** tab, choose **Build BLOODMOON plugin**, then **Run workflow**. The first run takes about an hour, almost all of it building the compiler.
3. When the run has a green tick, open it, scroll to **Artifacts** and download **bloodmoon-op1p**.
4. Unzip it. Inside is `bloodmoon.op1p`.

**Important:** never open `bloodmoon.op1p` in a text editor, and don't copy and paste its contents. That damages the file and the Workshop will refuse it. Just drag the file itself where it needs to go.

`HOW_TO_GET_BLOODMOON_OP1P.md` has the same steps in more detail.

## What's in this repository

| File | What it is |
|---|---|
| `bloodmoon-build-kit.zip` | All the source: the engine (`bloodmoon.c`), the screen code (`moon.c`), the artwork (`bloodmoon.svg`), the plugin recipe (`plugin.json`), and the op1r SDK files and compiler recipe needed to build it. |
| `.github/workflows/build.yml` | The "Build BLOODMOON plugin" button on the Actions tab. |
| `HOW_TO_GET_BLOODMOON_OP1P.md` | Step-by-step build guide. |
| `bloodmoon_screen_preview.png` | The screen preview above. |

## How it was checked

| Check | Result |
|---|---|
| Sound engine on a computer, with memory-error detectors, at several knob settings | No problems found. At the default knobs the loudest peak is about -11 dBFS before the OP-1's envelope. |
| The finished `bloodmoon.op1p` against the OP-1 plugin loader's checks and the Workshop's upload checks (checksum, format, size, name, screen-art rules) | Passes |
| The compiled Blackfin code run in a processor simulator, compared with the computer version across 34 knob, pitch and six-voice cases | Identical output, sample for sample. No out-of-bounds or misaligned memory access, no exceptions |
| Processor cost in that simulator | About 1.4 % of the audio time budget per voice, about 8.5 % for six voices at 400 MHz (a rough figure that leaves out memory delays; a pessimistic estimate is about 26 %) |
| Screen code in the simulator against the computer version, 588 knob and timing combinations | Identical |
| **On a real OP-1** | **Not yet confirmed** |

The simulator is not the OP-1's firmware. Cache and memory delays, interrupt timing, the real display and the contents of the firmware's own tables are only settled by running it on the device.

## Known limits

- It has not yet been confirmed on a real OP-1.
- At the default knobs it is about 5 dB quieter than the Brassophone plugin. Turn the OP-1's volume up.
- The screen animation assumes the OP-1 passes frame time in milliseconds. If the bells or stars look much too fast or too slow, please report it.
- The artwork is about 14 KB, above the Workshop's 8 KiB "artwork budget" (a guideline, not a limit). It also ends about 8 pixels above the bottom of the screen, and the Workshop's guide advises leaving room there, so check that nothing is cut off.
- Only the first page of four knobs is used.

## Feedback

Please open an issue and tell us your OP-1's firmware version (it should be R246), what you heard or saw, and any message the Workshop showed.

## Credits and licence

- BLOODMOON engine, screen code and artwork by **Nola Drift**.
- Built with the op1r plugin SDK by **pcppcp** and contributors ([op1r-src](https://github.com/pcppcp/op1r-src)). The structure follows the SDK's synth template, and the vibrato fade-in idea and screen structure follow the Brassophone plugin by **nvmsocool** and the SDK's example plugins.
- Installed with the [Hundekraft OP-1 Workshop](https://hundekraft.github.io/op1-workshop/index.html).
- OP-1 is a Teenage Engineering product. This project is not affiliated with or endorsed by Teenage Engineering and contains no Teenage Engineering firmware.

Licensed under **GPL-3.0-or-later with the additional OP-1 firmware permission** described in `LICENSE` and `NOTICE` (inside the build kit). If you share the plugin file, share its source too. The kit in this repository is that source.
