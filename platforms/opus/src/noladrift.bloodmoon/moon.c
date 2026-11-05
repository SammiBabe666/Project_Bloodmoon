/* BLOODMOON for Opus 2, the T1 screen: a blood moon behind a cathedral (bloodmoon.svg), one knob colour per layer:
 *   orange (MOONRISE): the red moon rises from behind the cathedral as the knob goes up
 *   green (ECLIPSE)  : the two green tower bells swing; the knob sets how far (still at 0, about +-20 degrees at full)
 *   blue  (BLOOD)    : the blue rose window turns (half a turn over the knob's travel)
 *   white (HAUNTING) : the white stars flicker; none at 0, more and longer the higher the knob
 * Beside the cathedral each knob's name and its value 0..100 in blackletter: one SVG group per digit glyph and
 * position, k<knob>p<pos>d<digit>, shown for the current value, centred for its number of digits.
 *
 * 1.6.0: the same picture and the same movements, drawn only with the core's documented art service (api.md, Art
 * and Pixels: opus_art_ready, opus_art_show, opus_art_turn, opus_art_draw, opus_page_keep, opus_page_put), as the
 * kit's outatime and erdtree draw theirs. 1.5.x drew it as an OS screen through firmware #246's own functions
 * (fw_svg_node_find, fw_node_set_matrix, a node's shown byte at +0x50, fw_screen_draw with a hand-written
 * prototype); none of those is used any more, so the plugin imports no fw_* name at all.
 *
 * The art service turns and shows elements by id but does not slide them. Two of the screen's movements are slides:
 *   - the moon rising (up to 24 units): the moon is turned about a pivot MOON_PIVOT_R units to its left, so a small
 *     turn moves it straight up or down (at most 2.9 degrees of turn, 0.6 units of sideways drift at the very bottom);
 *   - a two-digit value centred half a cell to the left (7.3 units): the value's group is turned one step (1/4096 of
 *     a turn) about a pivot DIGIT_PIVOT_R units above it, which slides it 7.29 units left and tilts it 0.09 degrees.
 *     One- and three-digit values need no slide: a single digit is shown at the middle position.
 * The whole picture is drawn (opus_art_draw) only when something on it changed, and its pixels kept: while nothing
 * moves (ECLIPSE at 0 and the stars between flickers) a frame is a copy (opus_page_put), not a new drawing.
 * Display thread, integers only. Every art call is checked by the core, so a missing id only disables that effect.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE). */
#include "bm.h"

/* pivots and places in units x 256 (art service units), from bloodmoon.svg: they must match it */
#define U(v)            ((int)((v) * 256))
#define ROSE_X          U(160.00)
#define ROSE_Y          U(103.41)
#define BELL_L_X        U(126.10)
#define BELL_R_X        U(193.61)
#define BELL_Y          U(83.59)
#define MOON_X          U(159.71)              /* the moon's centre */
#define MOON_Y          U(51.13)
#define MOON_PIVOT_R    480                    /* units: the moon's turn pivot sits this far to its left */
#define MOON_DROP       24                     /* units lower with the orange knob at 0 */
/* turn steps per unit of drop: 4096 / (2 pi MOON_PIVOT_R) = 1.3581, in Q10 */
#define MOON_STEP_Q10   1391
#define DIGIT_PIVOT_R   4755                   /* units: one turn step slides a value 4755 x 2 pi / 4096 = 7.294 */
/* the middle of each knob's two-digit value (bloodmoon.svg's k<k>p1 and k<k>p2 glyphs): its turn's pivot sits
 * DIGIT_PIVOT_R straight above it, so the glyphs slide left and move at most 0.02 units up or down */
static const int DIGIT_X[4] = { U(56.80), U(56.80), U(276.60), U(276.60) };
static const int DIGIT_Y[4] = { U(56.85), U(130.85), U(56.85), U(130.85) };

#define KEPT_WORDS      (24 * 1024)            /* the kept picture: lit pixels as runs (48 KB from the pool) */

static struct opl_self *me;
static int art = -1;                           /* bloodmoon.svg's art number, -1 none */
static int ready = -1;                         /* -1 not asked yet, 0 refused (the plain page), 1 loaded */
static unsigned short *kept;                   /* 0: no room, the picture is drawn every frame */
static int kept_used = -1;

static unsigned swing_q, twinkle_q;            /* phases in 1/256 table steps (8192 * 256 = one period) */
static unsigned last_cycles;

/* what the picture shows now: the art's state, applied and kept; a frame whose state is the same is a copy */
typedef struct {
    int moon, rose, bell;                      /* turns, 1/4096 of a turn */
    int stars;                                 /* bit i: stars<i> shown */
    signed char digit[4][3];                   /* digit shown per knob and position, -1 none */
    signed char slide[4];                      /* 1: the value's group turned one step (two digits) */
} view_t;
static view_t now;

static inline int tsin(int a) { return bm_sin((unsigned)a); }

#ifdef BM_HOST
unsigned bm_host_cycles(void);                 /* the host test's clock */
static inline unsigned cycles(void) { return bm_host_cycles(); }
#else
static inline unsigned cycles(void)            /* the Blackfin's cycle counter, as the kit's outatime reads it */
{
    unsigned c;
    __asm__ volatile ("%0 = CYCLES;" : "=d"(c));
    return c;
}
#endif

static void digit_id(char id[7], int k, int p, int d)
{
    id[0] = 'k'; id[1] = (char)('0' + k); id[2] = 'p'; id[3] = (char)('0' + p); id[4] = 'd'; id[5] = (char)('0' + d);
    id[6] = 0;
}

/* the first frame the art is loaded: every digit hidden, every element at its own place */
static void art_start(void)
{
    char id[7];
    for (int k = 0; k < 4; k++) {
        for (int p = 0; p < 3; p++) {
            for (int d = 0; d < 10; d++)
                if (p || d == 1) { digit_id(id, k, p, d); opus_art_show(art, id, 0); }
            now.digit[k][p] = -1;
        }
        now.slide[k] = 0;
    }
    now.stars = 7;                             /* the SVG shows them */
    now.moon = now.rose = now.bell = 0;
    opus_art_turn(art, "moon", 0, MOON_X - U(MOON_PIVOT_R), MOON_Y);
    opus_art_turn(art, "rose", 0, ROSE_X, ROSE_Y);
    opus_art_turn(art, "bellL", 0, BELL_L_X, BELL_Y);
    opus_art_turn(art, "bellR", 0, BELL_R_X, BELL_Y);
}

/* the knob's value 0..100 as digits: returns 1 when anything changed */
static int set_value(int k, int knob)
{
    int v = (knob * 100 + 16383) >> 15;
    int q = (v * 205) >> 11;                   /* v / 10, exact for 0..1028: no libgcc division */
    int ones = v - q * 10, tens = q >= 10 ? q - 10 : q;
    signed char want[3] = { -1, -1, -1 };
    int slide = 0;
    if (v >= 100)     { want[0] = 1; want[1] = (signed char)tens; want[2] = (signed char)ones; }
    else if (v >= 10) { want[1] = (signed char)tens; want[2] = (signed char)ones; slide = 1; }
    else              { want[1] = (signed char)ones; }        /* one digit: the middle position is its centre */
    int changed = 0;
    char id[7];
    for (int p = 0; p < 3; p++) {
        if (want[p] == now.digit[k][p]) continue;
        if (now.digit[k][p] >= 0) { digit_id(id, k, p, now.digit[k][p]); opus_art_show(art, id, 0); }
        if (want[p] >= 0)         { digit_id(id, k, p, want[p]);         opus_art_show(art, id, 1); }
        now.digit[k][p] = want[p];
        changed = 1;
    }
    if (slide != now.slide[k]) {
        char g[4] = { 'k', (char)('0' + k), 'v', 0 };
        opus_art_turn(art, g, slide, DIGIT_X[k], DIGIT_Y[k] - U(DIGIT_PIVOT_R));
        now.slide[k] = (signed char)slide;
        changed = 1;
    }
    return changed;
}

void bm_art_page(void)
{
    if (ready < 0) {
        ready = art >= 0 && opus_art_ready(art);
        if (ready) art_start();
    }
    if (!ready) { bm_page(); return; }

    short par[8];
    int k[4];
    opus_engine_knobs(par);
    for (int i = 0; i < 4; i++) k[i] = par[i] < 0 ? 0 : par[i];  /* knobs are 0..32767 */

    const unsigned c = cycles();               /* the time since the last frame, in ms (400 MHz) */
    int dt = last_cycles ? (int)((c - last_cycles) / 400000u) : 35;
    last_cycles = c;
    if (dt < 1) dt = 1;
    if (dt > 100) dt = 100;                    /* a stalled frame must not jump the animation */

    int changed = 0;

    /* orange / MOONRISE: the moon's drop, as a turn about its far pivot (clockwise on the screen: down) */
    {
        int dy_q10 = ((32767 - k[3]) * MOON_DROP) >> 5;          /* Q10 units, 32-bit only */
        int t = (dy_q10 * MOON_STEP_Q10 + (1 << 19)) >> 20;
        if (t != now.moon) { opus_art_turn(art, "moon", t, MOON_X - U(MOON_PIVOT_R), MOON_Y); now.moon = t; changed = 1; }
    }

    /* blue / BLOOD: rose window angle, 0..half a turn */
    {
        int t = (k[0] * 2048) >> 15;
        if (t != now.rose) { opus_art_turn(art, "rose", t, ROSE_X, ROSE_Y); now.rose = t; changed = 1; }
    }

    /* green / ECLIPSE: bells swing in opposite directions, about 1 swing/s */
    swing_q += (unsigned)((16800 * dt) >> 3);
    const int amp = (k[1] * 455) >> 15;                          /* 0..455 of 8192 = 0..20 degrees */
    {
        int a = (tsin((int)(swing_q >> 8)) * amp) >> 15;         /* 1/8192 of a turn */
        int t = a / 2;                                           /* 1/4096 */
        if (t != now.bell) {
            opus_art_turn(art, "bellL", t, BELL_L_X, BELL_Y);
            opus_art_turn(art, "bellR", -t, BELL_R_X, BELL_Y);
            now.bell = t; changed = 1;
        }
    }

    /* white / HAUNTING: three groups of stars flicker at different speeds; visible while sin > threshold,
     * threshold -32768 (always visible) at knob 0 up to about +7200 (visible ~40% of the time) at full */
    twinkle_q += (unsigned)((17500 * dt) >> 3);
    {
        int thr = -32768 + ((k[2] * 40000) >> 15), stars = 0;
        for (int i = 0; i < 3; i++) {
            int a = (int)(twinkle_q >> 8) * (2 + i) + i * 2731;
            if (tsin(a) > thr) stars |= 1 << i;
        }
        if (stars != now.stars) {
            static const char *const ID[3] = { "stars0", "stars1", "stars2" };
            for (int i = 0; i < 3; i++)
                if ((stars ^ now.stars) >> i & 1) opus_art_show(art, ID[i], stars >> i & 1);
            now.stars = stars; changed = 1;
        }
    }

    for (int i = 0; i < 4; i++) changed |= set_value(i, k[i]);

    if (!changed && kept_used >= 0) {          /* the same picture as last frame: put its pixels back */
        opus_page_put(kept, kept_used);
        return;
    }
    opus_art_draw(art);
    kept_used = -1;
    if (kept && !amp) {                        /* swinging bells change every frame: keeping them would be wasted */
        int rows;
        if (opus_page_pixels(&rows))
            kept_used = opus_page_keep(kept, KEPT_WORDS, 0, rows - 1, 0, rows / 2 - 1);
    }
}

/* ---- the plain page: only when the art could not be written or loaded (no flash headroom, refused) ---- */

static void put_int(char *s, int v)            /* 0..100 as text, no C library */
{
    char t[4];
    int n = 0;
    do { int q = (v * 205) >> 11; t[n++] = (char)('0' + v - q * 10); v = q; } while (v && n < 3);
    while (n) *s++ = t[--n];
    *s = 0;
}

void bm_page(void)
{
    static const char *const NAME[4] = { "blood", "eclipse", "haunting", "moonrise" };
    static char value[4][8];
    short par[8];
    opus_engine_knobs(par);
    opus_page_text(me, 0, 10, 10, 13, OPUS_COLOR_TEXT, "bloodmoon");
    for (int i = 0; i < 4; i++) {
        const int x = 10 + 78 * i, y = 52, v = par[i] < 0 ? 0 : par[i];
        put_int(value[i], (v * 100 + 16383) >> 15);
        opus_page_text(me, 1 + i, x, y, 13, OPUS_COLOR_E(i), NAME[i]);
        opus_page_text(me, 5 + i, x, y + 24, 15, OPUS_COLOR_TEXT, value[i]);
        opus_page_bar(x, y + 56, x + 66, y + 60, (unsigned)v, 32767, OPUS_COLOR_E(i));
    }
}

/* opl_init: the art's number (-1 when it could not be written) and the pool for the kept picture */
void bm_page_init(struct opl_self *self, int art_no)
{
    me = self;
    art = art_no;
    if (art >= 0) kept = (unsigned short *)opus_alloc(self, 2 * KEPT_WORDS);
}
