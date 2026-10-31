/* BLOODMOON for Opus 2: gothic organ with a haunted bell strike, as an engine of Opus's engine service.
 *   blue  (BLOOD)    : harmonic richness. Down = hollow, odd-harmonic organ; up = denser, darker stack (2nd-4th).
 *   green (ECLIPSE)  : bell strike at the attack: how loud it starts and how long it takes to sink into the organ.
 *                      Down = no bell; up = loud, long, dissonant bell.
 *   white (HAUNTING) : vibrato depth (fades in after the attack) plus a small steady pitch instability.
 *   orange (MOONRISE): vibrato / instability speed, squared curve (slow end gets most of the travel).
 *
 * Voice = fundamental + 2nd/3rd/4th harmonic (all from one phase) + two inharmonic bell partials at about 1.1875x
 * (a minor third, deliberately dissonant) and 2.40625x the pitch. The OS's envelope (T2), velocity and mix go around
 * each voice, as for every Opus engine.
 *
 * The sound code is v1.3's (src/plugins/bloodmoon/bloodmoon.c) line for line. What changed for Opus:
 *   - the voice is the core's (struct opus_engine's voice bytes), not iter's 0x24-byte object, so no vtable word;
 *   - pitch and knobs come in render's arguments, not from firmware addresses; the knobs have the LFO in;
 *   - the sine table is the plugin's own (bm_sine.h), not the firmware's;
 *   - velocity is the OS's alone: v1.3 also multiplied by it in render, so soft notes are a little louder here.
 * Everything is integer / Q15, 32-bit only per sample, no libc.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE). */
#include "bm.h"
#include "bm_sine.h"
#include "bm_art.h"

/* ---- tuning constants (v1.3's) ---- */
#define ORGAN_PEAK        18000      /* organ harmonics sum to at most this (Q15 amplitude) */
#define BELL_A_PEAK       5200       /* bell partial at ~1.1875x, at full strike level */
#define BELL_B_PEAK       3600       /* bell partial at ~2.40625x */
#define OUT_GAIN          23170      /* Q15 output gain, -3 dB from full scale (v1.3: +6 dB crackled the speaker) */
                                     /* worst case 18000 + 5200 + 3600 = 26800 < 32767, so no clipping before the gain */

#define VIB_MAX_AMT       1400       /* white knob full: +-1400/32768 = +-4.3% frequency */
#define WOBBLE_MAX_AMT    350        /* steady instability at white full */
#define VIB_MIN_INC       14609      /* slowest vibrato step a sample */
#define VIB_MAX_INC       876528     /* fastest */
#define VIB_RANGE         (VIB_MAX_INC - VIB_MIN_INC)

#define VIB_DELAY_BLOCKS  40         /* blocks of held note before vibrato starts */
#define VIB_FADE_IN       900        /* depth gained per block while held */
#define VIB_FADE_OUT      4096       /* depth lost per block after note off */

#define BELL_RATE_MIN     66         /* strike decay: fraction of level lost per block, Q15 */
#define BELL_RATE_SPAN    3200       /* added at green = 0 */

#define BLOCK             128        /* samples a render (88.2 kHz) */

#define SVG_PATH          "/yaffs2/mod/noladrift.bloodmoon/bloodmoon.svg"   /* where opus_art_text writes it */

typedef struct {
    unsigned ph;           /* fundamental phase (2^32 = one cycle); harmonics are multiples of it */
    unsigned ph_a;         /* bell partial A phase */
    unsigned ph_b;         /* bell partial B phase */
    unsigned vib_ph;       /* vibrato LFO phase */
    int vib_depth;         /* 0..32767 vibrato fade */
    int bell_lvl;          /* 0..32767 bell strike level, decays per block */
    int blood_s;           /* smoothed BLOOD knob */
    short held;            /* 1 from note_on until release */
    short age;             /* blocks since note_on (saturates) */
} bm_voice_t;

static unsigned bm_seed = 0x2545F491u;       /* gives each note a different vibrato start phase */

static inline int clamp_q15(int x) { return x < 0 ? 0 : (x > 32767 ? 32767 : x); }
static inline short sat16(int x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (short)x); }

static void bm_init(void *voice, int index)
{
    bm_voice_t *v = (bm_voice_t *)voice;
    (void)index;
    v->ph = v->ph_a = v->ph_b = v->vib_ph = 0;
    v->vib_depth = v->bell_lvl = v->blood_s = 0;
    v->held = v->age = 0;
}

static void bm_note_on(void *voice, const short *knobs)
{
    bm_voice_t *v = (bm_voice_t *)voice;
    bm_seed = bm_seed * 1664525u + 1013904223u;
    v->ph = 0; v->ph_a = 0; v->ph_b = 0;       /* all partials start at a sine zero crossing: no click */
    v->vib_ph = bm_seed;
    v->vib_depth = 0;
    v->bell_lvl = clamp_q15(knobs[1]);
    v->blood_s = clamp_q15(knobs[0]);
    v->held = 1;
    v->age = 0;
}

static void bm_release(void *voice, const short *knobs)
{
    (void)knobs;                               /* the OS's envelope lets it go */
    ((bm_voice_t *)voice)->held = 0;
}

static void bm_render(void *voice, short *out, const short *par, const unsigned *pitch)
{
    bm_voice_t *v = (bm_voice_t *)voice;
    const short *sine = BM_SINE;

    /* ---- per-block control values (divisions are fine here, not per sample) ---- */
    int blood_t = clamp_q15(par[0]);
    int d = blood_t - v->blood_s;
    v->blood_s += (d + (d > 0 ? 7 : 0)) >> 3;                      /* one-pole smoothing, reaches the target exactly */
    int b = v->blood_s;

    int w1 = 20000;
    int w2 = 1500 + ((b * 14000) >> 15);
    int w3 = 7000 + ((b * 8000) >> 15);
    int w4 = ((((b * b) >> 15)) * 10000) >> 15;                    /* the 4th only arrives late in the travel */
    int norm = (int)(((int)ORGAN_PEAK << 15) / (w1 + w2 + w3 + w4));
    int g1 = (w1 * norm) >> 15, g2 = (w2 * norm) >> 15, g3 = (w3 * norm) >> 15, g4 = (w4 * norm) >> 15;

    int inv_g = 32767 - clamp_q15(par[1]);
    int bell_rate = BELL_RATE_MIN + ((((inv_g * inv_g) >> 15) * BELL_RATE_SPAN) >> 15);
    v->bell_lvl -= (v->bell_lvl * bell_rate) >> 15;
    if (v->bell_lvl < 16) v->bell_lvl = 0;
    int ba = (v->bell_lvl * BELL_A_PEAK) >> 15;
    int bb = (v->bell_lvl * BELL_B_PEAK) >> 15;

    if (v->held) {
        if (v->age < 32767) v->age++;
        if (v->age >= VIB_DELAY_BLOCKS) {
            v->vib_depth += VIB_FADE_IN;
            if (v->vib_depth > 32767) v->vib_depth = 32767;
        }
    } else {
        v->vib_depth -= VIB_FADE_OUT;
        if (v->vib_depth < 0) v->vib_depth = 0;
    }

    int haunt = clamp_q15(par[2]);
    int vib_amt = (((haunt * VIB_MAX_AMT) >> 15) * v->vib_depth) >> 15;
    int wob_amt = (haunt * WOBBLE_MAX_AMT) >> 15;

    int rate = clamp_q15(par[3]);
    int rate_sq = (rate * rate) >> 15;
    unsigned vib_inc = (unsigned)(VIB_MIN_INC + ((rate_sq * (VIB_RANGE >> 9)) >> 6));   /* 32-bit only */

    unsigned ph = v->ph, ph_a = v->ph_a, ph_b = v->ph_b, vib_ph = v->vib_ph;

    for (int n = 0; n < BLOCK; n++) {
        /* pitch modulation: vibrato LFO plus a faster-moving (3x) instability LFO, both from one phase */
        int l1 = sine[vib_ph >> 19];
        int l2 = sine[(vib_ph * 3u + 0x55555555u) >> 19];
        vib_ph += vib_inc;
        int mod = (l1 * vib_amt + l2 * wob_amt) >> 15;              /* |mod| < 1750, far below the 2^12 limit */
        int inc = (int)pitch[n];
        int eff = inc + (((inc >> 12) * mod) >> 3);

        ph += (unsigned)eff;
        unsigned ue = (unsigned)eff;
        ph_a += ue + (ue >> 3) + (ue >> 4);                           /* x1.1875 */
        ph_b += (ue << 1) + (ue >> 2) + (ue >> 3) + (ue >> 5);        /* x2.40625 */

        int organ = (sine[ph >> 19] * g1 + sine[(ph << 1) >> 19] * g2
                   + sine[(ph * 3u) >> 19] * g3 + sine[(ph << 2) >> 19] * g4) >> 15;
        int bell = (sine[ph_a >> 19] * ba + sine[ph_b >> 19] * bb) >> 15;

        out[n] = sat16(((organ + bell) * OUT_GAIN) >> 15);           /* peak 26800 * 0.71 = 19000 per voice */
    }
    v->ph = ph; v->ph_a = ph_a; v->ph_b = ph_b; v->vib_ph = vib_ph;
}

/* Not const: when the screen's art cannot be written the engine goes without it (svg 0) and shows the plain page. */
static struct opus_engine BLOODMOON = {
    .size = sizeof(struct opus_engine),
    .name = "bloodmoon",
    .knobs = { 14000, 20000, 7000, 9000, 0, 0, 0, 0 },   /* blue 43 %, green 61 %, white 21 %, orange 27 % */
    .voice = sizeof(bm_voice_t),
    .init = bm_init,
    .note_on = bm_note_on,
    .release = bm_release,
    .render = bm_render,
    .type = "bloodmoon",                       /* op1r's sounds and the v1.2/v1.3 presets carry "type": "bloodmoon" */
    .svg = SVG_PATH,
    .screen_init = bm_screen_init,
    .screen_draw = bm_screen_draw,
};

int opl_init(struct opl_self *self)
{
    if (opus_art_text(self, "bloodmoon.svg", BM_SVG, sizeof BM_SVG - 1) < 0) {
        opus_log(self, "bloodmoon: no room for the screen's art, the plain page instead");
        BLOODMOON.svg = 0;
        BLOODMOON.screen_init = 0;
        BLOODMOON.screen_draw = 0;
        BLOODMOON.page = bm_page;
        bm_page_init(self);
    }
    return opus_engine_add(self, &BLOODMOON) < 0 ? -1 : 0;
}
