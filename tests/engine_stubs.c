/* What the Opus port's bloodmoon.c calls in the core, for tests/engine_test.c: the engine it adds is kept, fast memory
 * is given, and the screen's art is refused, so the test needs nothing of the screen (moon.c is not linked). The
 * screen's names are weak stand-ins with no prototypes from bm.h, so this file links against main's moon.c names
 * (bm_screen_*, bm_page_init(self)) and opus-beta4's (bm_page_init(self, art), bm_art_page) alike; none is called. */
#include <stdio.h>
#include "opus_plugin.h"

const struct opus_engine *test_engine;
int opus_engine_add(struct opl_self *self, const struct opus_engine *e) { (void)self; test_engine = e; return 0x1234; }
int opus_art_text(struct opl_self *self, const char *name, const char *svg, unsigned len)
{ (void)self; (void)name; (void)svg; (void)len; return -1; }
static short fast_mem[4096];
void *opus_fast(struct opl_self *self, unsigned bytes) { (void)self; return bytes <= sizeof fast_mem ? fast_mem : 0; }
void opus_log(struct opl_self *self, const char *text) { (void)self; (void)text; }

#define WEAK __attribute__((weak))
WEAK void bm_page_init(void) {}
WEAK void bm_page(void) {}
WEAK void bm_art_page(void) {}
WEAK void bm_screen_init(void) {}
WEAK void bm_screen_draw(void) {}
