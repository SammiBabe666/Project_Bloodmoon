/* BLOODMOON for Opus 2: what bloodmoon.c (the engine) and moon.c (the screen) share.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE). */
#ifndef BM_H
#define BM_H
#include "opus_plugin.h"

/* The sine, 8192 steps a turn in Q15, as v1.3's table gave it, from a quarter of it (bm_sine.h, 4 KB). The voices
 * read it in every sample, so opl_init copies it into the OP-1's fast memory (L1, opus_fast): a 32-byte line of
 * ordinary memory costs about 107 cycles each block an audio block touches it, and v1.3's whole 16 KB table read
 * from ordinary memory by six voices cost more than the core's budget for the plugin (1.5.0 was switched off). */
#define BM_QSINE_LEN 2049
extern const short BM_QSINE[BM_QSINE_LEN]; /* in the plugin's image (bm_sine.h) */
extern const short *bm_qsine;              /* the copy in fast memory, or BM_QSINE when there was no room */

static inline int bm_sin(unsigned k)       /* k: 0..8191 (higher bits are ignored) */
{
    unsigned i = k & 2047;
    if (k & 2048) i = 2048 - i;            /* the second and fourth quarters run backwards */
    int s = bm_qsine[i];
    return (k & 4096) ? -s : s;            /* the second half is the first, negated */
}

void bm_page_init(struct opl_self *self, int art);  /* moon.c: art = bloodmoon.svg's art number, or -1 */
void bm_art_page(void);                    /* moon.c: T1, the moon and cathedral through the core's art service */
void bm_page(void);                        /* moon.c: the plain page, when the art could not be written or loaded */
#endif
