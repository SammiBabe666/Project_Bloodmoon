/* BLOODMOON for Opus 2, the T1 screen: a blood moon behind a cathedral (bloodmoon.svg), one knob colour per layer:
 *   orange (MOONRISE): the red moon rises from behind the cathedral as the knob goes up
 *   green (ECLIPSE)  : the two green tower bells swing; the knob sets how far (still at 0, about +-20 degrees at full)
 *   blue  (BLOOD)    : the blue rose window turns (half a turn over the knob's travel)
 *   white (HAUNTING) : the white stars flicker; none at 0, more and longer the higher the knob
 * Beside the cathedral each knob's name and its value 0..100 in blackletter: one SVG group per digit glyph and
 * position, k<knob>p<pos>d<digit>, shown for the current value, and the knob's group k<knob>v shifted so a one or two
 * digit number stays centred.
 *
 * v1.3's moon.c, moved onto Opus's engine screen (struct opus_engine's svg, screen_init, screen_draw: core 2.6). The
 * firmware calls are the kit's named ones (op1fw.h) instead of op1r's fixed addresses: fw_svg_node_find (warn 0, so a
 * missing id never stops the OS), fw_node_set_matrix, the node's shown byte +0x50, and fw_screen_draw. The knobs come
 * from opus_engine_knobs (the LFO in while a note sounds, as v1.3's live knobs).
 * Display thread, integers only. Every node lookup is checked, so a missing id only disables that effect.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE). */
#include "bm.h"

/* The four firmware functions the screen calls, by op1fw.h's names (the core resolves them at load). Declared here
 * instead of including op1fw.h for one reason: op1fw.h gives fw_screen_draw the screen alone, and the OS's draw also
 * takes the frame's dt, as opus_plugin.h says for an engine's screen ("0x01A3B4C8(scr, dt)") and as v1.3 called it. */
void *fw_svg_node_find(void *node, const char *id, int warn);
void fw_node_set_matrix(void *node, const int *m9);
void fw_screen_draw(void *scr, int dt);
int fw_snprintf(char *dst, unsigned int size, const char *fmt, ...);

#define Q10(v)      ((int)((v) * 1024))
#define ROSE_X      160.00                 /* pivots in SVG units: must match bloodmoon.svg */
#define ROSE_Y      103.41
#define BELL_L_X    126.10
#define BELL_R_X    193.61
#define BELL_Y      83.59
#define MOON_DROP   24                     /* the moon sits this many pixels lower with the orange knob at 0 */
#define DIGIT_CELL_Q10 14938               /* digit cell width, 14.588 px */
#define NODE_SHOWN  0x50                   /* an element's shown byte (op1fw.h, fw_svg_node_find) */

static void *moon_node, *rose_node, *bell_l, *bell_r, *star_node[3];
static unsigned swing_q, twinkle_q;        /* phases in 1/256 table steps (8192 * 256 = one period) */
static void *val_grp[4], *digit[4][3][10]; /* digit[k][0][1] is the hundreds 1, the other [k][0][d] stay 0 */
static int shown[4];                       /* value on screen per knob, -1 = none yet */

static inline int tsin(int a) { return BM_SINE[a & 8191]; }
static inline int tcos(int a) { return tsin(a + 2048); }

static void *find(void *scr, const char *id) { return fw_svg_node_find(*(void **)scr, id, 0); }
static inline void set_visible(void *n, int on) { ((volatile unsigned char *)n)[NODE_SHOWN] = (unsigned char)(on != 0); }

/* rotate node around (cx,cy) by sin/cos (Q15): matrix for row vectors [a b 0; c d 0; tx ty 1] (Q10) */
static void node_rotate(void *node, int cx, int cy, int s15, int c15)
{
    int c = c15 >> 5, s = s15 >> 5;                     /* Q15 -> Q10 */
    int m[9] = { c, s, 0, -s, c, 0,
                 cx - ((cx * c - cy * s) >> 10), cy - ((cx * s + cy * c) >> 10), 1024 };
    fw_node_set_matrix(node, m);
}

void bm_screen_init(void *scr)
{
    moon_node = find(scr, "moon");
    rose_node = find(scr, "rose");
    bell_l = find(scr, "bellL");
    bell_r = find(scr, "bellR");
    star_node[0] = find(scr, "stars0");
    star_node[1] = find(scr, "stars1");
    star_node[2] = find(scr, "stars2");
    swing_q = 0; twinkle_q = 0;
    char id[7];
    id[0] = 'k';
    for (int k = 0; k < 4; k++) {
        id[1] = (char)('0' + k); id[2] = 'v'; id[3] = 0;
        val_grp[k] = find(scr, id);
        id[2] = 'p'; id[4] = 'd'; id[6] = 0;
        for (int p = 0; p < 3; p++)
            for (int d = 0; d < 10; d++) {
                id[3] = (char)('0' + p); id[5] = (char)('0' + d);
                digit[k][p][d] = (p || d == 1) ? find(scr, id) : 0;
            }
        shown[k] = -1;
    }
}

static inline void show_digit(void *n, int on) { if (n) set_visible(n, on); }

/* the knob's value as 0..100: only the digits of the new value visible, the group centred for its digit count */
static void show_value(int k, int knob)
{
    int v = (knob * 100 + 16383) >> 15;
    int n = v >= 100 ? 3 : v >= 10 ? 2 : 1;
    if (val_grp[k]) {                                            /* every frame, like the moon's transform */
        int m[9] = { 1024, 0, 0, 0, 1024, 0, -(3 - n) * (DIGIT_CELL_Q10 / 2), 0, 1024 };
        fw_node_set_matrix(val_grp[k], m);
    }
    if (v == shown[k]) return;
    for (int p = 0; p < 3; p++)
        for (int d = 0; d < 10; d++) show_digit(digit[k][p][d], 0);
    int q = (v * 205) >> 11;                                     /* v / 10, exact for 0..1028: no libgcc division */
    int ones = v - q * 10, tens = q >= 10 ? q - 10 : q;
    show_digit(digit[k][2][ones], 1);
    if (n >= 2) show_digit(digit[k][1][tens], 1);
    if (n == 3) show_digit(digit[k][0][1], 1);
    shown[k] = v;
}

void bm_screen_draw(void *scr, int dt)
{
    short par[8];
    int k[4];
    opus_engine_knobs(par);
    for (int i = 0; i < 4; i++) k[i] = par[i] < 0 ? 0 : par[i];  /* knobs are 0..32767 */
    if (dt < 1) dt = 1;
    if (dt > 100) dt = 100;                                      /* a stalled frame must not jump the animation */

    /* orange / MOONRISE: moon height. The cathedral is filled black and drawn after the moon, so it hides its lower part */
    if (moon_node) {
        int dy_q10 = ((32767 - k[3]) * MOON_DROP) >> 5;          /* Q10 pixels, 32-bit only */
        int m[9] = { 1024, 0, 0, 0, 1024, 0, 0, dy_q10, 1024 };
        fw_node_set_matrix(moon_node, m);
    }

    /* blue / BLOOD: rose window angle, 0..half a turn */
    if (rose_node) {
        int a = (k[0] * 4096) >> 15;
        node_rotate(rose_node, Q10(ROSE_X), Q10(ROSE_Y), tsin(a), tcos(a));
    }

    /* green / ECLIPSE: bells swing in opposite directions, about 1 swing/s */
    swing_q += (unsigned)((16800 * dt) >> 3);
    {
        int amp = (k[1] * 455) >> 15;                            /* 0..455 of 8192 = 0..20 degrees */
        int a = (tsin((int)(swing_q >> 8)) * amp) >> 15;
        if (bell_l) node_rotate(bell_l, Q10(BELL_L_X), Q10(BELL_Y), tsin(a), tcos(a));
        if (bell_r) node_rotate(bell_r, Q10(BELL_R_X), Q10(BELL_Y), tsin(-a), tcos(-a));
    }

    /* white / HAUNTING: three groups of stars flicker at different speeds; visible while sin > threshold,
     * threshold -32768 (always visible) at knob 0 up to about +7200 (visible ~40% of the time) at full */
    twinkle_q += (unsigned)((17500 * dt) >> 3);
    {
        int thr = -32768 + ((k[2] * 40000) >> 15);
        for (int i = 0; i < 3; i++) if (star_node[i]) {
            int a = (int)(twinkle_q >> 8) * (2 + i) + i * 2731;
            set_visible(star_node[i], tsin(a) > thr);
        }
    }

    for (int i = 0; i < 4; i++) show_value(i, k[i]);

    fw_screen_draw(scr, dt);                                     /* the SVG; nothing is drawn over it */
}

/* ---- the plain page: only when the art could not be written to the plugin's folder (no flash headroom) ---- */

static struct opl_self *me;

void bm_page_init(struct opl_self *self) { me = self; }

void bm_page(void)
{
    static const char *const NAME[4] = { "blood", "eclipse", "haunting", "moonrise" };
    static char value[4][8];
    short par[8];
    opus_engine_knobs(par);
    opus_page_text(me, 0, 10, 10, 13, OPUS_COLOR_TEXT, "bloodmoon");
    for (int i = 0; i < 4; i++) {
        const int x = 10 + 78 * i, y = 52, v = par[i] < 0 ? 0 : par[i];
        fw_snprintf(value[i], sizeof value[i], "%i", (v * 100 + 16383) >> 15);
        opus_page_text(me, 1 + i, x, y, 13, OPUS_COLOR_E(i), NAME[i]);
        opus_page_text(me, 5 + i, x, y + 24, 15, OPUS_COLOR_TEXT, value[i]);
        opus_page_bar(x, y + 56, x + 66, y + 60, (unsigned)v, 32767, OPUS_COLOR_E(i));
    }
}
