/* Host test of the Opus 2 port (opus/noladrift.bloodmoon) against v1.3 (src/plugins/bloodmoon), on the computer:
 *   1. the same notes, knobs and pitch through both engines: the port's samples must match v1.3's to within 2
 *      (v1.3 multiplies by velocity 127 = 32766/32768 in render and rounds twice; the port leaves velocity to the OS);
 *   2. the screen: every node it should move is found, and turning the knobs moves them as v1.3's screen does.
 * Build and run: opus/tests/run.sh. Writes v13.raw and opus.raw (int16 mono) for listening. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opus_plugin.h"

/* ---- v1.3, built with its own host harness names (-DOP1_HOST) in v13.o ---- */
typedef struct { void (*note_on)(void *, const short *); void (*render)(void *, short *, const short *);
                 void (*note_off)(void *); unsigned char press[4]; } v13_engine_t;
extern const v13_engine_t ENGINE_bloodmoon;
short host_sine[8192]; int host_pitch_inc[128]; unsigned char host_voice_mem[6 * 0x24]; short host_voice_vel[6];

/* ---- the core's names the port calls, stubbed ---- */
static const struct opus_engine *eng;
int opus_engine_add(struct opl_self *self, const struct opus_engine *e) { (void)self; eng = e; return 0x1234; }
int opus_art_text(struct opl_self *self, const char *name, const char *svg, unsigned len)
{ (void)self; (void)name; return (len > 40000 && strncmp(svg, "<?xml", 5) == 0 && svg[len - 2] == '>') ? 0 : -1; }
void opus_log(struct opl_self *self, const char *t) { (void)self; fprintf(stderr, "log: %s\n", t); }
static short knobs_now[8];
int opus_engine_knobs(short par[8]) { memcpy(par, knobs_now, sizeof knobs_now); return 1; }
void opus_page_text(struct opl_self *s, int sl, int x, int y, int z, int c, const char *t)
{ (void)s; (void)sl; (void)x; (void)y; (void)z; (void)c; (void)t; }
void opus_page_bar(int a, int b, int c, int d, unsigned v, unsigned m, int col)
{ (void)a; (void)b; (void)c; (void)d; (void)v; (void)m; (void)col; }
int fw_snprintf(char *d, unsigned n, const char *f, ...) { (void)f; if (n) d[0] = 0; return 0; }

/* nodes: one buffer per id, its matrix kept */
#define MAXN 200
static struct node { char id[16]; unsigned char b[0x60]; int m[9]; } nodes[MAXN];
static int nn, draws;
void *fw_svg_node_find(void *doc, const char *id, int warn)
{
    (void)doc;
    if (warn) { fprintf(stderr, "FAIL: a lookup with warn on (%s)\n", id); exit(1); }
    for (int i = 0; i < nn; i++) if (!strcmp(nodes[i].id, id)) return nodes[i].b;
    if (nn == MAXN) return 0;
    strncpy(nodes[nn].id, id, 15); nodes[nn].b[0x50] = 1;
    return nodes[nn++].b;
}
static struct node *by_id(const char *id)
{ for (int i = 0; i < nn; i++) if (!strcmp(nodes[i].id, id)) return &nodes[i]; return 0; }
void fw_node_set_matrix(void *n, const int *m)
{ for (int i = 0; i < nn; i++) if (nodes[i].b == n) memcpy(nodes[i].m, m, sizeof nodes[i].m); }
void fw_screen_draw(void *scr, int dt) { (void)scr; (void)dt; draws++; }

static int inc_of(int midi) { return (int)lrint(4294967296.0 * 440.0 * pow(2, (midi - 69) / 12.0) / 44100); }

int main(void)
{
    static struct opl_self self = { sizeof self, "noladrift.bloodmoon", "/yaffs2/mod/noladrift.bloodmoon", 0 };
    for (int i = 0; i < 8192; i++) host_sine[i] = (short)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
    extern const short BM_SINE[8192];
    if (memcmp(host_sine, BM_SINE, sizeof host_sine)) { fprintf(stderr, "FAIL: sine tables differ\n"); return 1; }
    if (opl_init(&self) || !eng || !eng->svg || !eng->screen_draw || strcmp(eng->type, "bloodmoon")) {
        fprintf(stderr, "FAIL: opl_init / the engine struct\n"); return 1;
    }
    printf("engine \"%s\", type \"%s\", %u voice bytes, screen %s\n", eng->name, eng->type, eng->voice, eng->svg);

    /* 1. sound: knob sets, a phrase each, notes held 110 blocks then released 28, the seed shared by both */
    static const short sets[][4] = { { 14000, 20000, 7000, 9000 }, { 32767, 32767, 32767, 32767 }, { 0, 0, 0, 0 },
                                     { 32767, 0, 16000, 3000 }, { 5000, 32767, 32767, 32767 } };
    static const int notes[] = { 36, 48, 52, 55, 60, 72, 84, 96 };
    unsigned char *voice = calloc(1, eng->voice);
    FILE *fa = fopen("v13.raw", "wb"), *fb = fopen("opus.raw", "wb");
    int maxdiff = 0, peak = 0; long n = 0;
    for (unsigned s = 0; s < sizeof sets / sizeof sets[0]; s++) {
        short k[8] = { sets[s][0], sets[s][1], sets[s][2], sets[s][3], 0, 0, 0, 0 };
        for (unsigned j = 0; j < sizeof notes / sizeof notes[0]; j++) {
            unsigned pitch[128]; short a[128], b[128];
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
                    int d = abs(a[i] - b[i]); maxdiff = d > maxdiff ? d : maxdiff;
                    peak = abs(b[i]) > peak ? abs(b[i]) : peak; n += b[i] != 0;
                }
                fwrite(a, 2, 128, fa); fwrite(b, 2, 128, fb);
            }
        }
    }
    fclose(fa); fclose(fb);
    printf("sound: max difference from v1.3 %d (of 32767), peak %d (%.1f dBFS), %ld non-zero samples\n",
           maxdiff, peak, 20 * log10(peak / 32768.0), n);
    if (maxdiff > 2 || n == 0) { fprintf(stderr, "FAIL: the sound differs from v1.3\n"); return 1; }

    /* 2. screen */
    void *doc = (void *)1, *scr = &doc;
    eng->screen_init(scr);
    static const char *const want[] = { "moon", "rose", "bellL", "bellR", "stars0", "stars1", "stars2", "k0v", "k3v",
                                        "k0p2d0", "k0p0d1", "k3p1d9" };
    for (unsigned i = 0; i < sizeof want / sizeof want[0]; i++)
        if (!by_id(want[i])) { fprintf(stderr, "FAIL: screen never looks up %s\n", want[i]); return 1; }
    printf("screen: %d nodes looked up\n", nn);
    knobs_now[0] = 0; knobs_now[3] = 0;
    eng->screen_draw(scr, 35);
    int moon_low = by_id("moon")->m[7], rose0 = by_id("rose")->m[0];
    knobs_now[0] = 32767; knobs_now[1] = 32767; knobs_now[2] = 32767; knobs_now[3] = 32767;
    for (int f = 0; f < 20; f++) eng->screen_draw(scr, 35);
    int moon_high = by_id("moon")->m[7], rose1 = by_id("rose")->m[0];
    printf("screen: moon offset %d -> %d (Q10 px), rose cos %d -> %d (Q10: no turn -> half a turn), %d draws\n", moon_low, moon_high, rose0, rose1, draws);
    if (!(moon_low == (32767 * 24) >> 5 && moon_low > moon_high && moon_high == 0) || rose0 != 1023 || rose1 != -1024 || draws != 21) {
        fprintf(stderr, "FAIL: the screen does not follow the knobs\n"); return 1;
    }
    /* value 100 on every knob: hundreds 1, tens 0, ones 0 shown, other digits hidden */
    if (!by_id("k1p0d1")->b[0x50] || !by_id("k1p1d0")->b[0x50] || !by_id("k1p2d0")->b[0x50] || by_id("k1p2d5")->b[0x50]) {
        fprintf(stderr, "FAIL: the knob value digits\n"); return 1;
    }
    printf("PASS\n");
    return 0;
}
