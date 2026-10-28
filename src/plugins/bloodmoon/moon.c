/* BLOODMOON screen: a blood moon behind a cathedral (bloodmoon.svg). One OP-1 knob colour per layer:
 *   red   (MOONRISE) : the red moon rises from behind the cathedral as the knob goes up
 *   green (ECLIPSE)  : the two green tower bells swing; the knob sets how far (still at 0, about +-20 degrees at full)
 *   blue  (BLOOD)    : the blue rose window turns (half a turn over the knob's travel)
 *   white (HAUNTING) : the white stars flicker; none at 0, more and longer the higher the knob
 * Beside the cathedral each knob's name and its value 0..100 in blackletter (tests/make_labels.py builds the panels):
 * one SVG group per digit glyph and position, k<knob>p<pos>d<digit>, shown for the current value, and the knob's group
 * k<knob>v shifted so a one or two digit number stays centred.
 * GUI thread, integers only. Every node lookup is checked, so a missing id only disables that effect.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
 * Screen structure follows the SDK examples (fmbell, brassophone) by pcppcp and nvmsocool. */
#include "op1_plugin.h"

#define ROSE_X      160.00                 /* pivots in SVG units: must match plugins/bloodmoon/bloodmoon.svg (tests/make_art.py prints them) */
#define ROSE_Y      103.41
#define BELL_L_X    126.10
#define BELL_R_X    193.61
#define BELL_Y      83.59
#define MOON_DROP   24                     /* the moon sits this many pixels lower with the red knob at 0 */
#define DIGIT_CELL_Q10 14938               /* digit cell width, 14.588 px (printed by tests/make_labels.py) */

static void *moon_node, *rose_node, *bell_l, *bell_r, *star_node[3];
static uint32_t swing_q, twinkle_q;        /* phases in 1/256 table steps (8192 * 256 = one period) */
static void *val_grp[4], *digit[4][3][10]; /* digit[k][0][1] is the hundreds 1, the other [k][0][d] stay 0 */
static int shown[4];                       /* value on screen per knob, -1 = none yet */

static inline int32_t tsin(int32_t a) { return OP1_SINE_TABLE[a & 8191]; }
static inline int32_t tcos(int32_t a) { return tsin(a + 2048); }

static void moon_init(void *scr)
{
    moon_node = op1_find_node(scr, "moon");
    rose_node = op1_find_node(scr, "rose");
    bell_l = op1_find_node(scr, "bellL");
    bell_r = op1_find_node(scr, "bellR");
    star_node[0] = op1_find_node(scr, "stars0");
    star_node[1] = op1_find_node(scr, "stars1");
    star_node[2] = op1_find_node(scr, "stars2");
    swing_q = 0; twinkle_q = 0;
    char id[7];
    id[0] = 'k';
    for (int k = 0; k < 4; k++) {
        id[1] = (char)('0' + k); id[2] = 'v'; id[3] = 0;
        val_grp[k] = op1_find_node(scr, id);
        id[2] = 'p'; id[4] = 'd'; id[6] = 0;
        for (int p = 0; p < 3; p++)
            for (int d = 0; d < 10; d++) {
                id[3] = (char)('0' + p); id[5] = (char)('0' + d);
                digit[k][p][d] = (p || d == 1) ? op1_find_node(scr, id) : 0;
            }
        shown[k] = -1;
    }
}

static inline void show_digit(void *n, int on) { if (n) op1_node_set_visible(n, on); }

/* the knob's value as 0..100: only the digits of the new value visible, the group centred for its digit count */
static void show_value(int k, int32_t knob)
{
    int v = (int)((knob * 100 + 16383) >> 15);
    int n = v >= 100 ? 3 : v >= 10 ? 2 : 1;
    if (val_grp[k]) {                                            /* every frame, like the moon's transform */
        int32_t m[9] = { 1024, 0, 0, 0, 1024, 0, -(3 - n) * (DIGIT_CELL_Q10 / 2), 0, 1024 };
        op1_node_set_transform(val_grp[k], m);
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

static void moon_update(void *scr, int dt)
{
    int16_t k[4];
    op1_synth_knobs_live(OP1_SCREEN_SLOT & 7, k);
    for (int i = 0; i < 4; i++) k[i] = k[i] < 0 ? 0 : k[i];          /* knobs are 0..32767 */
    if (dt < 1) dt = 1;
    if (dt > 100) dt = 100;                                          /* a stalled frame must not jump the animation */

    /* red / MOONRISE: moon height. The cathedral is filled black and drawn after the moon, so it hides the moon's lower part */
    if (moon_node) {
        int32_t dy_q10 = ((32767 - k[3]) * MOON_DROP) >> 5;          /* Q10 pixels, 32-bit only */
        int32_t m[9] = { 1024, 0, 0, 0, 1024, 0, 0, dy_q10, 1024 };
        op1_node_set_transform(moon_node, m);
    }

    /* blue / BLOOD: rose window angle, 0..half a turn */
    if (rose_node) {
        int32_t a = (k[0] * 4096) >> 15;
        op1_node_rotate(rose_node, OP1_Q10(ROSE_X), OP1_Q10(ROSE_Y), tsin(a), tcos(a));
    }

    /* green / ECLIPSE: bells swing in opposite directions, about 1 swing/s (dt assumed to be ms, as in horn.c) */
    swing_q += (uint32_t)((16800 * dt) >> 3);
    {
        int32_t amp = (k[1] * 455) >> 15;                            /* 0..455 of 8192 = 0..20 degrees */
        int32_t a = (tsin((int32_t)(swing_q >> 8)) * amp) >> 15;
        if (bell_l) op1_node_rotate(bell_l, OP1_Q10(BELL_L_X), OP1_Q10(BELL_Y), tsin(a), tcos(a));
        if (bell_r) op1_node_rotate(bell_r, OP1_Q10(BELL_R_X), OP1_Q10(BELL_Y), tsin(-a), tcos(-a));
    }

    /* white / HAUNTING: three groups of stars flicker at different speeds; visible while sin > threshold,
     * threshold -32768 (always visible) at knob 0 up to about +7200 (visible ~40% of the time) at full */
    twinkle_q += (uint32_t)((17500 * dt) >> 3);
    {
        int32_t thr = -32768 + ((k[2] * 40000) >> 15);
        for (int i = 0; i < 3; i++) if (star_node[i]) {
            int32_t a = (int32_t)(twinkle_q >> 8) * (2 + i) + i * 2731;
            op1_node_set_visible(star_node[i], tsin(a) > thr);
        }
    }

    for (int i = 0; i < 4; i++) show_value(i, k[i]);

    op1_screen_draw(scr, dt);                                        /* the SVG; nothing is drawn over it */
}

const op1_screen_t SCREEN_bloodmoon = { moon_init, moon_update };
