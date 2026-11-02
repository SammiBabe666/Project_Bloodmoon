# BLOODMOON 1.5.2: install

A dark organ with a haunted bell for the **original OP-1** (not the Field), firmware 246. There are two plugin files
in this zip. Pick the one for the firmware mod your OP-1 runs. You don't need to build anything.

| Your OP-1 runs | File | Folder in this zip |
|---|---|---|
| **Opus 2** (LizardMan, core 2.6 or newer) | `bloodmoon.opmod` | `opus/` |
| **Hundekraft 3.0** or newer | `bloodmoon.knl` | `hundekraft/` |

Both play the same sound with the same four knobs and the same moon-and-cathedral screen. Install only one of them.

> Neither file has been confirmed on a real OP-1 yet (the earlier Opus builds 1.5.0 and 1.5.1 were switched off for
> their CPU; 1.5.2 fixes that). Back up your OP-1 first, start with the volume low, and please report what you see and
> hear.

## Opus 2: `bloodmoon.opmod`

You need Opus 2 on the OP-1. You can make the update from your own firmware at https://opus-op1.pages.dev/patcher/.

1. Hold **synth + drum + tape + mixer** to open Opus's menu, press **T1** (system) and switch **developer mode** on
   for the first tries.
2. Press **shift + COM**, then **T3** for disk mode, and connect the OP-1 to your computer.
3. Copy `opus/bloodmoon.opmod` into the disk's **mods** folder. Eject the disk and switch the OP-1 off and on.
4. Open Opus's menu, then press **T2** for the mods list. The **bloodmoon** row should say **loaded**. If it gives a
   reason instead, please send it to us.
5. On a synth slot, press **shift + T1** and choose **bloodmoon**.
6. A new engine keeps the slot's previous envelope. On **T2**, turn the sustain up so held notes ring.

To help us set its CPU figure: with developer mode on, play six-note chords with all four knobs up for a minute, then
open the mods list, select **bloodmoon** and press the **orange** encoder. Send us the "audio: worst" number.

If something goes wrong, hold **Stop** while you switch the OP-1 on. It starts with no plugins, so you can delete the
file in disk mode.

## Hundekraft 3.0: `bloodmoon.knl`

1. Open the [Hundekraft OP-1 Workshop](https://hundekraft.github.io/op1-workshop/index.html).
2. Under **Your plugins**, add `hundekraft/bloodmoon.knl`. If an older `bloodmoon.op1p` is there, remove it first.
3. Connect the OP-1 and press **Update OP-1**. Read the review, send it, and press **T1** on the OP-1 when asked.
4. After the restart, open synth, press **shift + T1** and pick **bloodmoon**.

## Presets

In disk mode, copy the `presets/bloodmoon` folder into the OP-1's `synth` folder. Back up `synth` first.

These presets are for **Hundekraft** (and the older op1r plugin). On **Opus** they probably do not load: Opus's
author has said sounds tagged with an engine type this way do not load on Opus. Opus presets need the format Opus
itself saves, which is not documented yet. If you save a bloodmoon sound on Opus and send us the file, we can make
Opus presets from it.

## The knobs

| Encoder | Name | What it does |
|---|---|---|
| Blue | BLOOD | Harmonic richness: a hollow organ at the bottom, a thick dark stack at the top |
| Green | ECLIPSE | The bell strike: none at the bottom, a loud, long, dissonant bell at the top |
| White | HAUNTING | Vibrato depth, fading in after the attack, plus a slight pitch drift |
| Orange | MOONRISE | Vibrato speed, from about 0.15 Hz to 9 Hz |

T2 is the OP-1's own envelope. The source code and the full guide are at https://github.com/SammiBabe666/Project_Bloodmoon.
BLOODMOON is GPL-3.0-or-later with the OP-1 firmware permission. See `LICENSE` and `NOTICE` in the repository.
