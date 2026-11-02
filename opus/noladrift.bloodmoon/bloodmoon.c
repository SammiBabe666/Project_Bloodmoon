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
 * The sound is v1.3's (src/plugins/bloodmoon/bloodmoon.c): the same partials, gains, bell decay, vibrato and knob
 * curves. What changed for Opus:
 *   - the voice is the core's (struct opus_engine's voice bytes), not iter's 0x24-byte object, so no vtable word;
 *   - pitch and knobs come in render's arguments, not from firmware addresses; the knobs have the LFO in;
 *   - the sine table is the plugin's own (bm_sine.h, a quarter of v1.3's values), in fast memory (bm.h says why);
 *   - velocity is the OS's alone: v1.3 also multiplied by it in render, so soft notes are a little louder here;
 *   - render computes it at half rate with the LFOs once a block (1.5.2, bm_render says how): close to v1.3's
 *     samples, not equal to them (opus/tests/compare.c measures how close).
 * Everything is integer / Q15, 32-bit only per sample, no libc.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE). */
#include "bm.h"
#include "bm_sine.h"
#include "bm_art.h"

const short *bm_qsine = BM_QSINE;          /* opl_init moves it to fast memory when there is room */

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
    unsigned ph;           /* fundamental phase (2^32 = one cycle); the harmonics are computed from its sine and cosine */
    unsigned ph_a;         /* bell partial A phase */
    unsigned ph_b;         /* bell partial B phase */
    unsigned vib_ph;       /* vibrato LFO phase */
    short vib_depth;       /* 0..32767 vibrato fade */
    short bell_lvl;        /* 0..32767 bell strike level, decays per block */
    short blood_s;         /* smoothed BLOOD knob */
    short held;            /* 1 from note_on until release */
    short age;             /* blocks since note_on (saturates) */
    short last;            /* the last sample written: the start of the next block's first interpolation */
    short pad[2];
} bm_voice_t;              /* 32 bytes */

static unsigned bm_seed = 0x2545F491u;       /* gives each note a different vibrato start phase */

static inline int clamp_q15(int x) { return x < 0 ? 0 : (x > 32767 ? 32767 : x); }
static inline short sat16(int x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (short)x); }

/* n / d for 0 < n < 2^30, d > 0, quotient < 2^16, by shift and subtract: no libgcc call in the audio path */
static unsigned bm_div16(unsigned n, unsigned d)
{
    unsigned q = 0;
    for (int b = 15; b >= 0; b--)
        if ((n >> b) >= d) { n -= d << b; q |= 1u << b; }
    return q;
}

static void bm_init(void *voice, int index)
{
    bm_voice_t *v = (bm_voice_t *)voice;
    (void)index;
    v->ph = v->ph_a = v->ph_b = v->vib_ph = 0;
    v->vib_depth = v->bell_lvl = v->blood_s = 0;
    v->held = v->age = v->last = 0;
}

static void bm_note_on(void *voice, const short *knobs)
{
    bm_voice_t *v = (bm_voice_t *)voice;
    bm_seed = bm_seed * 1664525u + 1013904223u;
    v->ph = 0; v->ph_a = 0; v->ph_b = 0;       /* all partials start at a sine zero crossing: no click */
    v->vib_ph = bm_seed;
    v->vib_depth = 0;
    v->bell_lvl = (short)clamp_q15(knobs[1]);
    v->blood_s = (short)clamp_q15(knobs[0]);
    v->held = 1;
    v->age = 0;
    v->last = 0;
}

static void bm_release(void *voice, const short *knobs)
{
    (void)knobs;                               /* the OS's envelope lets it go */
    ((bm_voice_t *)voice)->held = 0;
}

/* The voice's block. 1.5.2: the same sound as v1.3, computed for the Blackfin's cost (1.5.1 was switched off after
 * three notes on an OP-1):
 *   - the sound is computed at half rate (64 points a block, 44.1 kHz, the OS's own rate) and each second sample is
 *     the mean of its neighbours. The highest partial is the 4th harmonic, below 22 kHz for every note up to C8's
 *     fundamental / 4; v1.3 computed every 88.2 kHz sample;
 *   - the vibrato and instability LFOs are read once a block (689 a second; the fastest, 27 Hz, is 25 reads a cycle);
 *   - the 2nd, 3rd and 4th harmonics come from the fundamental's sine and cosine by the multiple-angle formulas
 *     (two table reads instead of four), the same waveform as v1.3's four lookups;
 *   - the gains are 16-bit (16 x 16 multiplies), the output gain is folded into them once a block, and the per-block
 *     division is a shift-and-subtract loop (bm_div16) instead of libgcc's. */
static void bm_render(void *voice, short *out, const short *par, const unsigned *pitch)
{
    bm_voice_t *v = (bm_voice_t *)voice;

    /* ---- per-block control values ---- */
    int blood_t = clamp_q15(par[0]);
    int d = blood_t - v->blood_s;
    v->blood_s = (short)(v->blood_s + ((d + (d > 0 ? 7 : 0)) >> 3));   /* one-pole smoothing, reaches the target */
    int b = v->blood_s;

    int w1 = 20000;
    int w2 = 1500 + ((b * 14000) >> 15);
    int w3 = 7000 + ((b * 8000) >> 15);
    int w4 = ((((b * b) >> 15)) * 10000) >> 15;                    /* the 4th only arrives late in the travel */
    int norm = (int)bm_div16((unsigned)ORGAN_PEAK << 15, (unsigned)(w1 + w2 + w3 + w4));   /* 9750..20694 */
    int gn = (norm * OUT_GAIN) >> 15;                              /* the output gain, folded in */
    short g1 = (short)((w1 * gn) >> 15), g2 = (short)((w2 * gn) >> 15),
          g3 = (short)((w3 * gn) >> 15), g4 = (short)((w4 * gn) >> 15);

    int inv_g = 32767 - clamp_q15(par[1]);
    int bell_rate = BELL_RATE_MIN + ((((inv_g * inv_g) >> 15) * BELL_RATE_SPAN) >> 15);
    int lvl = v->bell_lvl;
    lvl -= (lvl * bell_rate) >> 15;
    if (lvl < 16) lvl = 0;
    v->bell_lvl = (short)lvl;
    short ba = (short)((((lvl * BELL_A_PEAK) >> 15) * OUT_GAIN) >> 15);
    short bb = (short)((((lvl * BELL_B_PEAK) >> 15) * OUT_GAIN) >> 15);

    int depth = v->vib_depth;
    if (v->held) {
        if (v->age < 32767) v->age++;
        if (v->age >= VIB_DELAY_BLOCKS) { depth += VIB_FADE_IN; if (depth > 32767) depth = 32767; }
    } else {
        depth -= VIB_FADE_OUT; if (depth < 0) depth = 0;
    }
    v->vib_depth = (short)depth;

    int haunt = clamp_q15(par[2]);
    int vib_amt = (((haunt * VIB_MAX_AMT) >> 15) * depth) >> 15;
    int wob_amt = (haunt * WOBBLE_MAX_AMT) >> 15;

    int rate = clamp_q15(par[3]);
    int rate_sq = (rate * rate) >> 15;
    unsigned vib_inc = (unsigned)(VIB_MIN_INC + ((rate_sq * (VIB_RANGE >> 9)) >> 6));   /* 32-bit only */

    /* pitch modulation for the block, from the LFOs at the block's middle: vibrato plus the 3x instability */
    unsigned vib_ph = v->vib_ph + vib_inc * (BLOCK / 2);
    int mod = (bm_sin(vib_ph >> 19) * vib_amt + bm_sin((vib_ph * 3u + 0x55555555u) >> 19) * wob_amt) >> 15;
    v->vib_ph += vib_inc * BLOCK;                                  /* |mod| < 1750 */

    unsigned ph = v->ph, ph_a = v->ph_a, ph_b = v->ph_b;
    int last = v->last;

    /* ---- 64 points, each two samples' step ---- */
    for (int n = 0; n < BLOCK; n += 2) {
        unsigned inc = pitch[n] + pitch[n + 1];
        unsigned eff = inc + (unsigned)((((int)(inc >> 12)) * mod) >> 3);
        ph += eff;
        ph_a += eff + (eff >> 3) + (eff >> 4);                       /* x1.1875 */
        ph_b += (eff << 1) + (eff >> 2) + (eff >> 3) + (eff >> 5);   /* x2.40625 */

        short s = (short)bm_sin(ph >> 19), c = (short)bm_sin((ph >> 19) + 2048);
        short s2 = (short)((s * c) >> 14);                           /* sin 2x = 2 sin x cos x */
        short c2 = (short)(32767 - ((s * s) >> 14));                 /* cos 2x = 1 - 2 sin^2 x */
        short s3 = (short)(s + ((s * c2) >> 14));                    /* sin 3x = sin x (1 + 2 cos 2x) */
        short s4 = (short)((s2 * c2) >> 14);                         /* sin 4x = 2 sin 2x cos 2x */
        int acc = s * g1 + s2 * g2 + s3 * g3 + s4 * g4
                + (short)bm_sin(ph_a >> 19) * ba + (short)bm_sin(ph_b >> 19) * bb;
        int y = sat16(acc >> 15);                                    /* peak 26800 * 0.71 = 19000 per voice */
        out[n] = (short)((last + y) >> 1);
        out[n + 1] = (short)y;
        last = y;
    }
    v->ph = ph; v->ph_a = ph_a; v->ph_b = ph_b; v->last = (short)last;
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
    short *fast = (short *)opus_fast(self, sizeof BM_QSINE);   /* 4 KB of the shared L1; without it, ordinary memory */
    if (fast) {
        for (int i = 0; i < BM_QSINE_LEN; i++) fast[i] = BM_QSINE[i];
        bm_qsine = fast;
    } else {
        opus_log(self, "bloodmoon: no fast memory, the sine table stays in ordinary memory");
    }
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
