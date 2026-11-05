/* Behaviour tests of BLOODMOON's engines on the computer, the same checks on both:
 *   v13   platforms/op1/src/plugins/bloodmoon/bloodmoon.c (op1r v1.3; the Hundekraft .knl is the same code)
 *   opus  platforms/opus/src/noladrift.bloodmoon/bloodmoon.c (the Opus 2 port), through its struct opus_engine
 * platforms/opus/tests/compare.c checks that the port sounds like v1.3; this file checks what both must do on their
 * own, over far more knob settings and notes than a listening test reaches:
 *   1. knobs: every knob at 0, 25, 50, 75 and 100 % (625 settings) on a low, a middle and a high note, held and
 *      released: never above the design peak (19,000: no gain or sum gone wrong, no 16-bit wrap), never silent;
 *   2. pitch: every MIDI note 0-127 at the default knobs and at every knob full, and a bend across seven octaves
 *      inside one note: never above the design peak, never silent;
 *   3. tuning: with the vibrato and the bell off, each note's strongest partial is the note itself (36-96);
 *   4. voices: a voice sounds the same played alone as beside five others (no state shared between voices);
 *   5. a fresh voice: one built from leftover bytes plays exactly as one built from zeros;
 *   6. knob jumps: a knob thrown end to end while a note sounds steps no more than the sound's own steepest step
 *      (BLOOD, and GREEN in v1.3: known clicks on low notes, held to today's size; see jump_limit);
 *   7. DC: a held note's mean stays within 1 % of full scale.
 * Build and run: tests/run.sh (needs the Opus kit's sdk folder, OPUS_SDK). Standard C and libm only. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opus_plugin.h"

/* ---- v1.3, built with the op1r host harness names (-DOP1_HOST) ---- */
typedef struct { void (*note_on)(void *, const short *); void (*render)(void *, short *, const short *);
                 void (*note_off)(void *); unsigned char press[4]; } v13_engine_t;
extern const v13_engine_t ENGINE_bloodmoon;
short host_sine[8192]; int host_pitch_inc[128]; unsigned char host_voice_mem[6 * 0x24]; short host_voice_vel[6];

/* ---- the Opus port, through the struct it adds (engine_stubs.c catches it) ---- */
extern const struct opus_engine *test_engine;

#define BLOCK 128
#define VOICES 6
#define VSIZE 64                                  /* bytes a voice here: both engines' fit (0x24 and 32) */
/* The loudest a voice may be: the partials' gains sum to at most 26,800, times the output gain 0.71 (both bloodmoon.c),
 * so 19,000 and a little rounding. Above it, a gain or a sum went wrong; a 16-bit wrap shows here too. */
#define DESIGN_PEAK 19100

/* One engine as the tests drive it: voice i's bytes, a block at a pitch (a phase step a sample, 2^32 a turn). */
typedef struct {
    const char *name;
    void (*init)(int i);                          /* the voice built again: silent, ready for a note */
    void (*note_on)(int i, const short *k);
    void (*release)(int i, const short *k);
    void (*render)(int i, short *out, const short *k, const unsigned *pitch);
} eng_t;

static unsigned char opus_mem[VOICES][VSIZE];
static void v13_init(int i) { (void)i; }          /* v1.3 has no init: its note_on sets the whole voice */
static void v13_on(int i, const short *k) { host_voice_vel[i] = 127 << 8; ENGINE_bloodmoon.note_on(host_voice_mem + 0x24 * i, k); }
static void v13_off(int i, const short *k) { (void)k; ENGINE_bloodmoon.note_off(host_voice_mem + 0x24 * i); }
static void v13_render(int i, short *out, const short *k, const unsigned *pitch)
{
    for (int n = 0; n < BLOCK; n++) host_pitch_inc[n] = (int)pitch[n];
    ENGINE_bloodmoon.render(host_voice_mem + 0x24 * i, out, k);
}
static void op_init(int i) { test_engine->init(opus_mem[i], i); }
static void op_on(int i, const short *k) { test_engine->note_on(opus_mem[i], k); }
static void op_off(int i, const short *k) { test_engine->release(opus_mem[i], k); }
static void op_render(int i, short *out, const short *k, const unsigned *pitch) { test_engine->render(opus_mem[i], out, k, pitch); }

static eng_t engines[2] = { { "v13", v13_init, v13_on, v13_off, v13_render },
                            { "opus", op_init, op_on, op_off, op_render } };

static int failures;
static void fail(const eng_t *e, const char *what)
{
    fprintf(stderr, "FAIL: %s: %s\n", e->name, what);
    failures++;
}

static unsigned inc_of(double midi) { return (unsigned)llrint(4294967296.0 * 440.0 * pow(2, (midi - 69) / 12.0) / 44100); }
static void fill(unsigned *p, unsigned inc) { for (int n = 0; n < BLOCK; n++) p[n] = inc; }
static void knobs(short k[8], int a, int b, int c, int d) { memset(k, 0, 16); k[0] = (short)a; k[1] = (short)b; k[2] = (short)c; k[3] = (short)d; }

/* Plays one note on voice 0 from a fresh voice: held blocks, then released ones. Writes up to max samples to out (or
 * none) and returns the peak; *held_energy gets the held part's sum of squares. */
static int play(const eng_t *e, const short *k, unsigned inc, int held, int rel, short *out, long max, double *held_energy)
{
    unsigned pitch[BLOCK]; short b[BLOCK];
    int peak = 0; long at = 0; double en = 0;
    fill(pitch, inc);
    e->init(0); e->note_on(0, k);
    for (int blk = 0; blk < held + rel; blk++) {
        if (blk == held) e->release(0, k);
        e->render(0, b, k, pitch);
        for (int n = 0; n < BLOCK; n++) {
            int a = abs(b[n]); if (a > peak) peak = a;
            if (blk < held) en += (double)b[n] * b[n];
            if (out && at < max) out[at++] = b[n];
        }
    }
    if (held_energy) *held_energy = en;
    return peak;
}

/* 1. every knob at five places */
static void test_knob_grid(const eng_t *e)
{
    static const int at[5] = { 0, 8192, 16384, 24576, 32767 };
    static const int notes[3] = { 24, 60, 108 };
    int peak = 0, runs = 0;
    for (int q = 0; q < 625; q++) {
        short k[8]; knobs(k, at[q % 5], at[q / 5 % 5], at[q / 25 % 5], at[q / 125]);
        for (int j = 0; j < 3; j++) {
            double en;
            int p = play(e, k, inc_of(notes[j]), 48, 16, 0, 0, &en);
            if (p > peak) peak = p;
            runs++;
            if (p > DESIGN_PEAK || en == 0) {
                char m[120];
                snprintf(m, sizeof m, "knobs %d %d %d %d note %d: %s", k[0], k[1], k[2], k[3], notes[j], en == 0 ? "silent" : "above the design peak");
                fail(e, m); return;
            }
        }
    }
    printf("%-4s knobs:   %d notes over 625 knob settings, peak %d (%.1f dBFS), none silent\n", e->name, runs, peak,
           20 * log10(peak / 32768.0));
}

/* 2. every MIDI note, and a bend inside a note */
static void test_pitch_range(const eng_t *e)
{
    static const short sets[2][4] = { { 14000, 20000, 7000, 9000 }, { 32767, 32767, 32767, 32767 } };
    int peak = 0;
    for (int s = 0; s < 2; s++)
        for (int m = 0; m < 128; m++) {
            short k[8]; knobs(k, sets[s][0], sets[s][1], sets[s][2], sets[s][3]);
            double en;
            int p = play(e, k, inc_of(m), 40, 8, 0, 0, &en);
            if (p > peak) peak = p;
            if (p > DESIGN_PEAK || en == 0) {
                char msg[80]; snprintf(msg, sizeof msg, "note %d at knob set %d: %s", m, s, en == 0 ? "silent" : "above the design peak");
                fail(e, msg); return;
            }
        }
    /* a bend from MIDI 24 to 108 and back over 2 x 250 blocks, a new pitch each sample */
    short k[8]; knobs(k, 32767, 32767, 32767, 32767);
    unsigned pitch[BLOCK]; short b[BLOCK];
    e->init(0); e->note_on(0, k);
    double en = 0;
    for (int blk = 0; blk < 500; blk++) {
        for (int n = 0; n < BLOCK; n++) {
            double t = (blk * BLOCK + n) / (250.0 * BLOCK);
            pitch[n] = inc_of(24 + 84 * (t < 1 ? t : 2 - t));
        }
        e->render(0, b, k, pitch);
        for (int n = 0; n < BLOCK; n++) {
            int a = abs(b[n]); if (a > peak) peak = a;
            en += (double)b[n] * b[n];
        }
    }
    if (peak > DESIGN_PEAK || en == 0) { fail(e, "the bend goes above the design peak or is silent"); return; }
    printf("%-4s pitch:   MIDI 0-127 at two knob sets and a 7-octave bend, peak %d, none silent\n", e->name, peak);
}

/* 3. tuning: a Goertzel filter's power at f (cycles a sample) */
static double power_at(const short *x, long n, double f)
{
    double w = 2 * cos(2 * M_PI * f), s1 = 0, s2 = 0;
    for (long i = 0; i < n; i++) { double s0 = x[i] / 32768.0 + w * s1 - s2; s2 = s1; s1 = s0; }
    return s1 * s1 + s2 * s2 - w * s1 * s2;
}
static void test_tuning(const eng_t *e)
{
    enum { HELD = 400 };
    static short buf[HELD * BLOCK];
    double worst = 1e9;
    for (int m = 36; m <= 96; m += 5) {
        short k[8]; knobs(k, 0, 0, 0, 0);                  /* hollow organ, no bell, no vibrato */
        unsigned inc = inc_of(m);
        play(e, k, inc, HELD, 0, buf, HELD * BLOCK, 0);
        double f = inc / 4294967296.0;
        /* skip the first 100 blocks (BLOOD's smoothing) */
        const short *x = buf + 100 * BLOCK; long n = (HELD - 100) * BLOCK;
        double on = power_at(x, n, f), lo = power_at(x, n, f / 1.0594630943592953), hi = power_at(x, n, f * 1.0594630943592953);
        double r = 10 * log10(on / (lo > hi ? lo : hi));
        if (r < worst) worst = r;
        if (r < 20) {
            char msg[80]; snprintf(msg, sizeof msg, "note %d: its pitch is only %.1f dB above a semitone off", m, r);
            fail(e, msg); return;
        }
    }
    printf("%-4s tuning:  MIDI 36-96, each note's own pitch at least %.1f dB above a semitone either side\n", e->name, worst);
}

/* 4. voices are independent (HAUNTING 0: the vibrato's per-note seed then does nothing) */
static void test_voices(const eng_t *e)
{
    enum { BLKS = 120 };
    static short alone[BLKS * BLOCK];
    short k[8]; knobs(k, 20000, 32767, 0, 9000);
    unsigned pitch[BLOCK]; short b[BLOCK];
    play(e, k, inc_of(57), 90, BLKS - 90, alone, BLKS * BLOCK, 0);
    /* again, with five more voices playing other notes and knobs between its blocks */
    short ko[8]; knobs(ko, 32767, 32767, 32767, 32767);
    for (int i = 0; i < VOICES; i++) { e->init(i); e->note_on(i, i ? ko : k); }
    long diff = 0;
    for (int blk = 0; blk < BLKS; blk++) {
        if (blk == 90) e->release(0, k);
        for (int i = VOICES - 1; i >= 0; i--) {
            fill(pitch, inc_of(i ? 30 + 11 * i : 57));
            if (i && blk == 30 * i) { e->release(i, ko); e->note_on(i, ko); }
            e->render(i, b, i ? ko : k, pitch);
            if (!i) for (int n = 0; n < BLOCK; n++) diff += b[n] != alone[blk * BLOCK + n];
        }
    }
    if (diff) { char msg[80]; snprintf(msg, sizeof msg, "a voice beside others differs from it alone in %ld samples", diff); fail(e, msg); return; }
    printf("%-4s voices:  one voice beside five others plays exactly as it does alone\n", e->name);
}

/* 5. a voice built from leftover bytes */
static void test_fresh_voice(const eng_t *e)
{
    enum { BLKS = 80 };
    static short clean[BLKS * BLOCK], dirty[BLKS * BLOCK];
    short k[8]; knobs(k, 14000, 20000, 0, 9000);
    memset(opus_mem[0], 0, VSIZE); memset(host_voice_mem, 0, 0x24);
    play(e, k, inc_of(48), 60, BLKS - 60, clean, BLKS * BLOCK, 0);
    memset(opus_mem[0], 0xA5, VSIZE); memset(host_voice_mem + 4, 0xA5, 0x20);   /* v1.3: the vtable word is the OS's */
    play(e, k, inc_of(48), 60, BLKS - 60, dirty, BLKS * BLOCK, 0);
    if (memcmp(clean, dirty, sizeof clean)) { fail(e, "a voice built from leftover bytes plays differently"); return; }
    printf("%-4s fresh:   a voice built from leftover bytes plays exactly as one from zeros\n", e->name);
}

/* 6. a knob thrown end to end while a note sounds */
static int steepest(const eng_t *e, const short *k, unsigned inc, int blks)
{
    unsigned pitch[BLOCK]; short b[BLOCK]; int prev = 0, worst = 0;
    fill(pitch, inc);
    e->init(0); e->note_on(0, k);
    for (int blk = 0; blk < blks; blk++) {
        e->render(0, b, k, pitch);
        for (int n = 0; n < BLOCK; n++) { int st = abs(b[n] - prev); prev = b[n]; if ((blk || n) && st > worst) worst = st; }
    }
    return worst;
}
/* Known clicks, held to today's size so they cannot get worse (measured 2026-11-05, when this test was written; not
 * fixed yet). Both engines change their gains once a block, not across it: BLOOD's four organ gains (smoothed over
 * about 8 blocks, but each block a step) and, in v1.3, the bell's level as GREEN sets its decay. On a low note, whose
 * own steps are small, a throw of the knob is a step of a few times the sound's steepest at a block's edge: BLOOD from
 * 0 to full on MIDI 24 is 7.5x in v1.3 and 3.7x on Opus; GREEN from full to 0 is 1.8x in v1.3 (Opus's half-rate
 * interpolation smooths that one away). Every other knob and engine: 1.25x, as the retrigger test in compare.c.
 * Ramping the gains across the block would bring all of them to 1.25x; lower a limit when that lands. */
static const double jump_limit[2][4] = { { 8.0, 2.0, 1.25, 1.25 },    /* v13:  BLOOD, GREEN, WHITE, ORANGE */
                                         { 4.0, 1.25, 1.25, 1.25 } }; /* opus */

static void test_knob_jumps(const eng_t *e, int ei)
{
    static const short base[3][4] = { { 14000, 20000, 7000, 9000 }, { 0, 0, 0, 0 }, { 32767, 32767, 32767, 32767 } };
    static const int notes[4] = { 24, 36, 60, 84 };
    static const char *const name[4] = { "BLOOD", "GREEN", "WHITE", "ORANGE" };
    double worst[4] = { 0 };
    for (int s = 0; s < 3; s++)
        for (int knob = 0; knob < 4; knob++)
            for (int j = 0; j < 4; j++) {
                short ka[8], kb[8];
                knobs(ka, base[s][0], base[s][1], base[s][2], base[s][3]);
                memcpy(kb, ka, sizeof kb);
                kb[knob] = (short)(ka[knob] > 16383 ? 0 : 32767);
                unsigned inc = inc_of(notes[j]);
                int natural = steepest(e, ka, inc, 90), nb = steepest(e, kb, inc, 90);
                if (nb > natural) natural = nb;
                unsigned pitch[BLOCK]; short b[BLOCK]; int prev = 0, jump = 0;
                fill(pitch, inc);
                e->init(0); e->note_on(0, ka);
                for (int blk = 0; blk < 90; blk++) {
                    e->render(0, b, blk < 45 ? ka : kb, pitch);
                    for (int n = 0; n < BLOCK; n++) { int st = abs(b[n] - prev); prev = b[n]; if (blk >= 44 && st > jump) jump = st; }
                }
                double r = (double)jump / natural;
                if (r > worst[knob]) worst[knob] = r;
                if (r > jump_limit[ei][knob]) {
                    char msg[120];
                    snprintf(msg, sizeof msg, "%s thrown at set %d, MIDI %d steps %.2fx the sound's own steepest (limit %.2fx)",
                             name[knob], s, notes[j], r, jump_limit[ei][knob]);
                    fail(e, msg); return;
                }
            }
    printf("%-4s jumps:   each knob thrown end to end, 3 settings x 4 notes, worst step vs the sound's own:", e->name);
    for (int knob = 0; knob < 4; knob++)
        printf(" %s %.2fx%s", name[knob], worst[knob], jump_limit[ei][knob] > 1.25 ? " (known)" : "");
    printf("\n");
}

/* 7. DC */
static void test_dc(const eng_t *e)
{
    enum { BLKS = 200 };
    static short buf[BLKS * BLOCK];
    static const short sets[3][4] = { { 14000, 20000, 7000, 9000 }, { 0, 0, 0, 0 }, { 32767, 32767, 32767, 32767 } };
    double worst = 0;
    for (int s = 0; s < 3; s++)
        for (int m = 24; m <= 108; m += 12) {
            short k[8]; knobs(k, sets[s][0], sets[s][1], sets[s][2], sets[s][3]);
            play(e, k, inc_of(m), BLKS, 0, buf, BLKS * BLOCK, 0);
            double sum = 0;
            for (long i = 0; i < BLKS * BLOCK; i++) sum += buf[i];
            double dc = fabs(sum / (BLKS * BLOCK)) / 32768.0;
            if (dc > worst) worst = dc;
        }
    if (worst > 0.01) { fail(e, "a held note's mean is more than 1 % of full scale away from zero"); return; }
    printf("%-4s dc:      held notes' mean at most %.3f %% of full scale\n", e->name, 100 * worst);
}

int opl_init(struct opl_self *self);

int main(void)
{
    static struct opl_self self = { sizeof self, "noladrift.bloodmoon", "/yaffs2/mod/noladrift.bloodmoon", 0 };
    for (int i = 0; i < 8192; i++) host_sine[i] = (short)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
    if (opl_init(&self) || !test_engine || test_engine->voice > VSIZE || !test_engine->init || !test_engine->note_on
        || !test_engine->release || !test_engine->render) {
        fprintf(stderr, "FAIL: opl_init did not add a whole engine\n"); return 1;
    }
    for (int i = 0; i < 2; i++) {
        const eng_t *e = &engines[i];
        test_knob_grid(e); test_pitch_range(e); test_tuning(e); test_voices(e); test_fresh_voice(e);
        test_knob_jumps(e, i); test_dc(e);
    }
    if (failures) { fprintf(stderr, "%d engine test(s) failed\n", failures); return 1; }
    printf("PASS\n");
    return 0;
}
