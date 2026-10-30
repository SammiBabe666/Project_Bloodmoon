/* BLOODMOON for Opus 2: what bloodmoon.c (the engine) and moon.c (the screen) share.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE). */
#ifndef BM_H
#define BM_H
#include "opus_plugin.h"

extern const short BM_SINE[8192];          /* sin(2 pi k / 8192), Q15 (bm_sine.h) */

void bm_screen_init(void *scr);            /* moon.c: the OS screen's hooks (struct opus_engine's screen_*) */
void bm_screen_draw(void *scr, int dt);
void bm_page_init(struct opl_self *self);  /* moon.c: the plain page, when the screen's art could not be written */
void bm_page(void);
#endif
