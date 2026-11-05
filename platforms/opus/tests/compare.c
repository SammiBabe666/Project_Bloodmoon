/* Host test of the Opus 2 port (platforms/opus/src/noladrift.bloodmoon) against v1.3
 * (platforms/op1/src/plugins/bloodmoon), on the computer:
 *   1. the same notes, knobs and pitch through both engines. Since 1.5.2 the port computes at half rate with its LFOs
 *      once a block, so its samples are close to v1.3's, not equal: per knob set, every note's level must be within
 *      1 dB of v1.3's, and at the points it computes the difference (what the port gets wrong) at least 50 dB below
 *      the sound with the LFOs off (HAUNTING 0), 35 dB with them on (the vibrato's timing drifts a little, which moves the phase);
 *      no note may be silent and the output must never reach full scale;
 *   2. the sine: the quarter table in fast memory folds out to v1.3's whole table;
 *   3. the screen (1.6.0: the core's art service): turning the knobs turns, shows and hides the same elements as
 *      v1.3's screen, the moon's and the values' slides land where v1.3's put them, an unchanged frame is a copy of
 *      the kept pixels, and without its art the engine shows the plain page. No fw_* name is linked.
 * Build and run: platforms/opus/tests/run.sh. Writes v13.raw and opus.raw (int16 mono) for listening. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bm.h"

/* ---- v1.3, built with its own host harness names (-DOP1_HOST) in v13.o ---- */
typedef struct { void (*note_on)(void *, const short *); void (*render)(void *, short *, const short *);
                 void (*note_off)(void *); unsigned char press[4]; } v13_engine_t;
extern const v13_engine_t ENGINE_bloodmoon;
short host_sine[8192]; int host_pitch_inc[128]; unsigned char host_voice_mem[6 * 0x24]; short host_voice_vel[6];

/* ---- the core's names the port calls, stubbed ---- */
static const struct opus_engine *eng;
static int texts;
static int art_fail;
int opus_engine_add(struct opl_self *self, const struct opus_engine *e) { (void)self; eng = e; return 0x1234; }
int opus_art_text(struct opl_self *self, const char *name, const char *svg, unsigned len)
{ (void)self; (void)name; if (art_fail) return -1; return (len > 40000 && strncmp(svg, "<?xml", 5) == 0 && svg[len - 2] == '>') ? 0 : -1; }
static short fast_mem[4096];
static unsigned fast_asked;
void *opus_fast(struct opl_self *self, unsigned bytes) { (void)self; fast_asked = bytes; return bytes <= sizeof fast_mem ? fast_mem : 0; }
void opus_log(struct opl_self *self, const char *t) { (void)self; fprintf(stderr, "log: %s\n", t); }
static short knobs_now[8];
int opus_engine_knobs(short par[8]) { memcpy(par, knobs_now, sizeof knobs_now); return 1; }
void opus_page_text(struct opl_self *s, int sl, int x, int y, int z, int c, const char *t)
{ (void)s; (void)sl; (void)x; (void)y; (void)z; (void)c; (void)t; texts++; }
void opus_page_bar(int a, int b, int c, int d, unsigned v, unsigned m, int col)
{ (void)a; (void)b; (void)c; (void)d; (void)v; (void)m; (void)col; }
/* the art service, stubbed: per id its shown state and its last turn and pivot */
#define MAXN 200
static struct el { char id[16]; int shown, turn, cx, cy; } els[MAXN];
static int nel, draws, puts_, keeps, art_ok = 1;
static struct el *el(const char *id)
{
    for (int i = 0; i < nel; i++) if (!strcmp(els[i].id, id)) return &els[i];
    if (nel == MAXN) { fprintf(stderr, "FAIL: too many ids\n"); exit(1); }
    strncpy(els[nel].id, id, 15); els[nel].shown = 1;                 /* the SVG shows every element */
    return &els[nel++];
}
static int known(const char *id)
{
    static const char *const fixed[] = { "moon", "rose", "bellL", "bellR", "stars0", "stars1", "stars2" };
    for (unsigned i = 0; i < 7; i++) if (!strcmp(id, fixed[i])) return 1;
    if (id[0] == 'k' && id[1] >= '0' && id[1] <= '3' && id[2] == 'v' && !id[3]) return 1;
    return id[0] == 'k' && id[1] >= '0' && id[1] <= '3' && id[2] == 'p' && id[3] >= '0' && id[3] <= '2' && id[4] == 'd'
           && id[5] >= '0' && id[5] <= '9' && !id[6] && (id[3] != '0' || id[5] == '1');
}
int opus_art_ready(int art) { return art == 0 && art_ok; }
int opus_art_show(int art, const char *id, int on)
{ if (art || !known(id)) { fprintf(stderr, "FAIL: show of unknown id %s\n", id); exit(1); } el(id)->shown = on; return 1; }
int opus_art_turn(int art, const char *id, int t, int cx, int cy)
{
    if (art || !known(id)) { fprintf(stderr, "FAIL: turn of unknown id %s\n", id); exit(1); }
    struct el *e = el(id); e->turn = t; e->cx = cx; e->cy = cy; return 1;
}
void opus_art_draw(int art) { (void)art; draws++; }
static unsigned short host_frame[1];
unsigned short *opus_page_pixels(int *rows) { *rows = 376; return host_frame; }
int opus_page_keep(unsigned short *out, unsigned cap, int x0, int x1, int y0, int y1)
{ (void)out; (void)x0; (void)x1; (void)y0; (void)y1; keeps++; return cap ? 1000 : -1; }
void opus_page_put(const unsigned short *runs, int used) { (void)runs; (void)used; puts_++; }
static unsigned char pool[1 << 16];
void *opus_alloc(struct opl_self *self, unsigned bytes) { (void)self; return bytes <= sizeof pool ? pool : 0; }
static unsigned host_clock;
unsigned bm_host_cycles(void) { return host_clock += 35 * 400000u; }   /* 35 ms a frame */
static struct el *by_id(const char *id) { for (int i = 0; i < nel; i++) if (!strcmp(els[i].id, id)) return &els[i]; return 0; }
static int shown(const char *id) { struct el *e = by_id(id); return e ? e->shown : 1; }
/* where a turn puts the point (x, y), in units */
static void turned(const struct el *e, double x, double y, double *ox, double *oy)
{
    double a = e->turn * 2 * M_PI / 4096, cx = e->cx / 256.0, cy = e->cy / 256.0;
    *ox = cx + (x - cx) * cos(a) - (y - cy) * sin(a);
    *oy = cy + (x - cx) * sin(a) + (y - cy) * cos(a);
}
/* the knob values on screen: digits shown per position (-1 none), and whether the group slides */
static void value_of(int k, int d[3])
{
    char id[7] = { 'k', (char)('0' + k), 'p', 0, 'd', 0, 0 };
    for (int p = 0; p < 3; p++) {
        d[p] = -1;
        for (int g = 0; g < 10; g++) {
            id[3] = (char)('0' + p); id[5] = (char)('0' + g);
            if (known(id) && shown(id)) { if (d[p] >= 0) { fprintf(stderr, "FAIL: two digits at k%dp%d\n", k, p); exit(1); } d[p] = g; }
        }
    }
}

static int inc_of(int midi) { return (int)lrint(4294967296.0 * 440.0 * pow(2, (midi - 69) / 12.0) / 44100); }

int main(void)
{
    static struct opl_self self = { sizeof self, "noladrift.bloodmoon", "/yaffs2/mod/noladrift.bloodmoon", 0 };
    for (int i = 0; i < 8192; i++) host_sine[i] = (short)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
    if (opl_init(&self) || !eng || eng->page != bm_art_page || eng->svg || eng->screen_draw || strcmp(eng->type, "bloodmoon")) {
        fprintf(stderr, "FAIL: opl_init / the engine struct\n"); return 1;
    }
    printf("engine \"%s\", type \"%s\", %u voice bytes, T1 page through the art service\n", eng->name, eng->type, eng->voice);
    /* the quarter table, folded, in fast memory, gives v1.3's whole table */
    extern const short *bm_qsine;
    if (bm_qsine != (const short *)fast_mem) { fprintf(stderr, "FAIL: the sine table is not in fast memory\n"); return 1; }
    for (unsigned k = 0; k < 8192; k++)
        if (bm_sin(k) != host_sine[k]) { fprintf(stderr, "FAIL: sine %u: %d, v1.3 %d\n", k, bm_sin(k), host_sine[k]); return 1; }
    printf("sine: the 4 KB quarter table in fast memory matches v1.3's 8192 steps\n");

    /* 1. sound: knob sets, a phrase each, notes held 110 blocks then released 28, the seed shared by both */
    static const short sets[][4] = { { 14000, 20000, 7000, 9000 }, { 32767, 32767, 32767, 32767 }, { 0, 0, 0, 0 },
                                     { 32767, 0, 16000, 3000 }, { 5000, 32767, 32767, 32767 },
                                     { 32767, 32767, 0, 0 }, { 16000, 8000, 0, 32767 } };
    static const int notes[] = { 36, 48, 52, 55, 60, 72, 84, 96 };
    unsigned char *voice = calloc(1, eng->voice);
    FILE *fa = fopen("v13.raw", "wb"), *fb = fopen("opus.raw", "wb");
    int peak = 0, fail = 0; long n = 0;
    for (unsigned s = 0; s < sizeof sets / sizeof sets[0]; s++) {
        short k[8] = { sets[s][0], sets[s][1], sets[s][2], sets[s][3], 0, 0, 0, 0 };
        double sig = 0, err = 0, worst_lvl = 0;
        for (unsigned j = 0; j < sizeof notes / sizeof notes[0]; j++) {
            unsigned pitch[128]; short a[128], b[128];
            double ea = 0, eb = 0;
            for (int i = 0; i < 128; i++) host_pitch_inc[i] = inc_of(notes[j]), pitch[i] = (unsigned)host_pitch_inc[i];
            host_voice_vel[0] = 127 << 8;
            eng->init(voice, 0);
            ENGINE_bloodmoon.note_on(host_voice_mem, k);
            eng->note_on(voice, k);
            for (int blk = 0; blk < 138; blk++) {
                if (blk == 110) { ENGINE_bloodmoon.note_off(host_voice_mem); eng->release(voice, k); }
                ENGINE_bloodmoon.render(host_voice_mem, a, k);
                eng->render(voice, b, k, pitch);
                for (int i = 0; i < 128; i++) {
                    double x = a[i], y = b[i];
                    if (i & 1) { sig += x * x; err += (x - y) * (x - y); }   /* the computed points: see below */
                    ea += x * x; eb += y * y;
                    peak = abs(b[i]) > peak ? abs(b[i]) : peak; n += b[i] != 0;
                }
                fwrite(a, 2, 128, fa); fwrite(b, 2, 128, fb);
            }
            if (eb == 0) { fprintf(stderr, "FAIL: set %u note %d is silent\n", s, notes[j]); fail = 1; }
            double lvl = fabs(10 * log10((eb + 1) / (ea + 1)));
            worst_lvl = lvl > worst_lvl ? lvl : worst_lvl;
        }
        /* the difference at the points the port computes (every second sample); the samples between are the mean of
         * their neighbours, whose error is mostly above 22 kHz (removed when the OS mixes at 44.1 kHz) and shows in
         * the level instead: at most 0.8 dB down for the highest notes, where the 4th harmonic nears 17 kHz */
        double snr = 10 * log10(sig / (err + 1)), need = sets[s][2] ? 35 : 50;
        printf("sound: knobs %5d %5d %5d %5d: level within %.2f dB of v1.3, difference %.1f dB below it\n",
               sets[s][0], sets[s][1], sets[s][2], sets[s][3], worst_lvl, snr);
        if (worst_lvl > 1.0 || snr < need) { fprintf(stderr, "FAIL: set %u is too far from v1.3\n", s); fail = 1; }
    }
    fclose(fa); fclose(fb); free(voice);
    printf("sound: peak %d (%.1f dBFS), %ld non-zero samples\n", peak, 20 * log10(peak / 32768.0), n);
    if (fail || n == 0 || peak >= 32767) { fprintf(stderr, "FAIL: the sound\n"); return 1; }

    /* 3. screen: the art service */
    knobs_now[0] = 0; knobs_now[1] = 0; knobs_now[2] = 0; knobs_now[3] = 0;
    eng->page();
    struct el *moon = by_id("moon"), *rose = by_id("rose");
    if (!moon || !rose || !by_id("bellL") || !by_id("bellR")) { fprintf(stderr, "FAIL: the screen never turns moon/rose/bells\n"); return 1; }
    /* the moon: v1.3 slid it down (32767 - k) * 24 / 32768 units; here it turns about a far pivot */
    double worst_dy = 0, worst_dx = 0, worst_tilt = 0;
    for (int k = 0; k <= 32767; k += 257) {
        knobs_now[3] = (short)k;
        eng->page();
        double x, y, want = (double)(((32767 - k) * 24) >> 5) / 1024;
        turned(moon, 159.71, 51.13, &x, &y);
        worst_dy = fmax(worst_dy, fabs((y - 51.13) - want));
        worst_dx = fmax(worst_dx, fabs(x - 159.71));
        worst_tilt = fmax(worst_tilt, fabs(moon->turn * 360.0 / 4096));
    }
    printf("screen: the moon within %.2f units of v1.3's height, %.2f units sideways, turned at most %.1f degrees\n",
           worst_dy, worst_dx, worst_tilt);
    if (worst_dy > 0.5 || worst_dx > 0.7 || worst_tilt > 3.0) { fprintf(stderr, "FAIL: the moon\n"); return 1; }
    knobs_now[0] = 32767; knobs_now[3] = 32767;
    eng->page();
    printf("screen: rose turn %d of 4096 at full (half a turn)\n", rose->turn);
    if (rose->turn != 2047 || moon->turn != 0) { fprintf(stderr, "FAIL: the rose / the moon at full\n"); return 1; }
    /* the values: 100, 43, 5 on knob 1 */
    static const struct { short knob; int d[3], slide; } vals[] = {
        { 32767, { 1, 0, 0 }, 0 }, { 14090, { -1, 4, 3 }, 1 }, { 1638, { -1, 5, -1 }, 0 }, { 0, { -1, 0, -1 }, 0 } };
    for (unsigned i = 0; i < sizeof vals / sizeof vals[0]; i++) {
        int d[3];
        knobs_now[1] = 0; knobs_now[0] = vals[i].knob;      /* knob 0 (blue): green stays 0, the bells still */
        eng->page();
        value_of(0, d);
        struct el *g = by_id("k0v");
        int slide = g ? g->turn : 0;
        if (d[0] != vals[i].d[0] || d[1] != vals[i].d[1] || d[2] != vals[i].d[2] || slide != vals[i].slide) {
            fprintf(stderr, "FAIL: knob value %d shows %d %d %d slide %d\n", vals[i].knob, d[0], d[1], d[2], slide); return 1;
        }
        if (slide) {                                       /* v1.3 slid a two-digit value half a cell (7.294 units) left */
            double wx = 0, wy = 0;                         /* the glyphs' corners: k0p1 and k0p2 span x 45.9-67.7, y 44.7-69 */
            static const double cx[2] = { 45.9, 67.7 }, cy[2] = { 44.7, 69.0 };
            for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) {
                double x, y; turned(g, cx[a], cy[b], &x, &y);
                wx = fmax(wx, fabs(cx[a] - x - 7.294)); wy = fmax(wy, fabs(y - cy[b]));
            }
            printf("screen: a two-digit value slides 7.294 units left as v1.3's, within %.3f across, %.3f up/down\n", wx, wy);
            if (wx > 0.02 || wy > 0.03) { fprintf(stderr, "FAIL: the value's slide\n"); return 1; }
        }
    }
    printf("screen: values 100, 43, 5 and 0 show the right digits\n");
    /* frames: still knobs, green 0, white 0 -> copies; green up -> a drawing every frame */
    knobs_now[0] = 14000; knobs_now[1] = 0; knobs_now[2] = 0; knobs_now[3] = 9000;
    eng->page();
    int d0 = draws, p0 = puts_;
    for (int f = 0; f < 20; f++) eng->page();
    printf("screen: 20 still frames: %d drawings, %d copies\n", draws - d0, puts_ - p0);
    if (draws != d0 || puts_ - p0 != 20) { fprintf(stderr, "FAIL: a still frame is drawn again\n"); return 1; }
    knobs_now[1] = 32767;
    d0 = draws;
    int bl_min = 0, bl_max = 0;
    for (int f = 0; f < 60; f++) { eng->page(); int t = by_id("bellL")->turn; bl_min = t < bl_min ? t : bl_min; bl_max = t > bl_max ? t : bl_max;
                                    if (by_id("bellR")->turn != -t) { fprintf(stderr, "FAIL: the bells do not swing apart\n"); return 1; } }
    printf("screen: ECLIPSE full: %d drawings in 60 frames, bells swing %d..%d of 4096 (v1.3: +-20 degrees = +-227)\n", draws - d0, bl_min, bl_max);
    if (draws - d0 < 55 || bl_max < 200 || bl_min > -200) { fprintf(stderr, "FAIL: the bells\n"); return 1; }
    knobs_now[1] = 0; knobs_now[2] = 32767;
    int flick = 0, last = -1;
    for (int f = 0; f < 200; f++) { eng->page(); int m = shown("stars0") | shown("stars1") << 1 | shown("stars2") << 2; flick += m != last; last = m; }
    printf("screen: HAUNTING full: the stars changed %d times in 200 frames\n", flick);
    if (flick < 10) { fprintf(stderr, "FAIL: the stars do not flicker\n"); return 1; }
    /* without its art (no flash headroom): the plain page */
    art_fail = 1;
    if (opl_init(&self) || eng->page != bm_page) { fprintf(stderr, "FAIL: no plain page without the art\n"); return 1; }
    d0 = draws; texts = 0;
    eng->page();
    printf("screen: without its art, the plain page (%d texts, %d art drawings)\n", texts, draws - d0);
    if (texts != 9 || draws != d0) { fprintf(stderr, "FAIL: the plain page\n"); return 1; }
    printf("PASS\n");
    return 0;
}
