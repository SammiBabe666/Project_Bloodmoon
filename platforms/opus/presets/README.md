# Opus presets

The eight BLOODMOON sounds for Opus 2, one folder each. `sound.json` holds every value of the sound: the four knobs
(BLOOD, ECLIPSE, HAUNTING, MOONRISE, 0-32767), the OS's envelope (attack, decay, sustain, release), the delay (or
none) and the octave. They are generated from `shared/preset-specifications/bloodmoon-presets.json`, the same values
as the Hundekraft/op1r presets, and every value lies inside the factory ranges the OP-1 accepts.

**No `.aif` files yet.** Opus saves an engine's sound as the OS's `iter` with a number in knob 4 and an `"opus"`
member naming the plugin. The Opus plugin kit does not give that member's layout or the number, so the files are not
guessed. To make them:

1. On an OP-1 with Opus 2 and BLOODMOON 1.5.2, choose bloodmoon on a synth slot and save the sound.
2. In disk mode, copy that sound's `.aif` from the OP-1's `synth` folder and send it (or add it to the repository).
3. `python3 platforms/opus/presets/presets.py learn "that sound.aif"`, then `python3 platforms/opus/presets/presets.py aif`.

That writes `<name>.aif` into each folder. Copy those into a `bloodmoon` folder in the OP-1's `synth` folder.

`python3 platforms/opus/presets/presets.py check` checks the folders against the specification and the ranges (CI runs
it through `tools/preset-validation/validate_presets.py`).
