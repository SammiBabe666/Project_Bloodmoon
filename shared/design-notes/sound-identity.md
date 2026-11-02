# What makes it BLOODMOON

Keep these when changing the engine on any platform. They are what the presets and the screen assume.

1. **An organ underneath.** A fundamental with only the 2nd, 3rd and 4th harmonics, from one phase. BLOOD moves from
   hollow (fundamental and 3rd strongest, odd-sounding) to a thick, dark stack, and the 4th harmonic arrives only late
   in its travel. The organ's peak stays fixed (18,000 of 32,767) whatever BLOOD does, so the knob changes colour, not
   loudness.
2. **A bell that disagrees.** Two inharmonic partials at 1.1875x (near a minor third) and 2.40625x the pitch, struck
   at note-on and decaying into the organ. ECLIPSE sets both how loud the strike is and how long it rings. At zero
   there is no bell at all.
3. **Slow, late vibrato.** It fades in only after the note has been held, and HAUNTING sets its depth (up to about
   ±4 % of frequency) together with a small steady instability at 3x the rate. MOONRISE sets the speed on a squared
   curve, so most of the knob is slow.
4. **Restraint.** Integer maths, no filters, no noise, no effects of its own. The OP-1's envelope, delay and LFO do
   the rest. The output gain of 0.71 keeps the built-in speaker clean.
5. **The picture is the controls.** A black cathedral, a blood-red moon (MOONRISE), a blue rose window (BLOOD), green
   bells (ECLIPSE), white stars (HAUNTING), and each knob's value in blackletter in its colour.

A platform port may compute these differently for its processor, as Opus does at half rate. It must keep the
partials, the knob curves and the levels, and be tested against `platforms/op1/src/plugins/bloodmoon/bloodmoon.c`.
