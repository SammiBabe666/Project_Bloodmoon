/* Cycle benchmark of the Opus engine's voices on a Blackfin simulator (GDB's bfin-elf-run), no OP-1 needed.
 * Six voices at the heaviest settings (all four knobs at full, notes across the range, held then released), rendered
 * block by block as Opus's core does; the CYCLES counter is read around each block's six renders and the worst and
 * mean blocks are printed. The engine is linked twice when OLD is given: the release before (old_ names) and now.
 *
 * What this measures: the core's instructions and their stalls as the simulator models them. Not modelled: the
 * OP-1's memory timing (an ordinary-memory cache line costs about 107 cycles a block on the device) and the core's
 * own work around each render, so the device's "audio: worst N" is higher. Use it to compare versions and to find
 * the order of magnitude, not as the cpu.txt figure. Built and run by opus/tests/bench_bfin.sh.
 * Copyright (C) 2026 Nola Drift. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE). */
#include "opus_plugin.h"

/* ---- the simulator's newlib system calls: P0 the call, R0 the address of its arguments ---- */
static int sys(int nr, int *args)
{
    register int r0 __asm__("R0") = (int)args;
    register int p0 __asm__("P0") = nr;
    __asm__ volatile ("excpt 0;" : "+d"(r0) : "a"(p0) : "memory", "R1", "R2", "CC");
    return r0;
}
static void put(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    int a[3] = { 1, (int)s, n };
    sys(5, a);                                   /* write */
}
static void put_num(unsigned v)
{
    char b[12]; int i = 11; b[i] = 0;
    do { unsigned q = 0, r = v;                  /* v / 10 without libgcc */
         while (r >= 10) { r -= 10; q++; }
         b[--i] = (char)('0' + r); v = q; } while (v);
    put(b + i);
}
static unsigned cycles(void) { unsigned c; __asm__ volatile ("%0 = CYCLES;" : "=d"(c)); return c; }

/* ---- the core's names the engine calls, stubbed ---- */
static const struct opus_engine *engs[2];
static int neng;
int opus_engine_add(struct opl_self *self, const struct opus_engine *e) { (void)self; engs[neng++] = e; return 0x1234; }
int opus_art_text(struct opl_self *self, const char *name, const char *svg, unsigned len)
{ (void)self; (void)name; (void)svg; (void)len; return 0; }
static short fast_mem[2][2052] __attribute__((aligned(32)));
static int nfast;
void *opus_fast(struct opl_self *self, unsigned bytes) { (void)self; return bytes <= sizeof fast_mem[0] ? fast_mem[nfast++] : 0; }
void opus_log(struct opl_self *self, const char *t) { (void)self; put(t); put("\n"); }
void bm_screen_init(void *scr) { (void)scr; }
void bm_screen_draw(void *scr, int dt) { (void)scr; (void)dt; }
void bm_page_init(struct opl_self *self) { (void)self; }
void bm_page(void) { }

int opl_init(struct opl_self *self);
#ifdef OLD
int old_opl_init(struct opl_self *self);
#endif

#define VOICES 6
#define BLOCKS 160                               /* 110 held, then released */
static unsigned char voice_mem[VOICES][64] __attribute__((aligned(32)));
static short out[128] __attribute__((aligned(32)));
static unsigned pitch[VOICES][128] __attribute__((aligned(32)));

static void bench(const struct opus_engine *e, const char *label)
{
    static const short k[8] = { 32767, 32767, 32767, 32767, 0, 0, 0, 0 };
    /* phase steps at 88.2 kHz for C2, G2, C3, E4, G5, C7 (f x 2^32 / 88200) */
    static const unsigned step[VOICES] = { 3185000, 4772000, 6370000, 16052000, 38186000, 101920000 };
    unsigned worst = 0, total = 0;
    for (int v = 0; v < VOICES; v++) {
        for (int i = 0; i < 128; i++) pitch[v][i] = step[v] + (unsigned)i;   /* a slow glide: no constant folding */
        e->init(voice_mem[v], v);
        e->note_on(voice_mem[v], k);
    }
    for (int b = 0; b < BLOCKS; b++) {
        if (b == 110) for (int v = 0; v < VOICES; v++) e->release(voice_mem[v], k);
        unsigned t0 = cycles();
        for (int v = 0; v < VOICES; v++) e->render(voice_mem[v], out, k, pitch[v]);
        unsigned t = cycles() - t0;
        worst = t > worst ? t : worst;
        total += t;
    }
    unsigned mean = 0, r = total;                /* total / BLOCKS without libgcc */
    while (r >= BLOCKS) { r -= BLOCKS; mean++; }
    put(label); put(": six voices, worst block "); put_num(worst); put(" cycles, mean "); put_num(mean);
    put(", one voice about "); { unsigned q = 0, w = worst; while (w >= VOICES) { w -= VOICES; q++; } put_num(q); }
    put(" (sound check: last sample "); put_num((unsigned)(out[127] < 0 ? -out[127] : out[127])); put(")\n");
}

int bench_main(void)
{
    static struct opl_self self = { sizeof self, "noladrift.bloodmoon", "/yaffs2/mod/noladrift.bloodmoon", 0 };
#ifdef OLD
    if (old_opl_init(&self)) { put("old opl_init failed\n"); return 1; }
#endif
    if (opl_init(&self)) { put("opl_init failed\n"); return 1; }
#ifdef OLD
    bench(engs[0], "before (" OLD ")");
    bench(engs[1], "now");
#else
    bench(engs[0], "now");
#endif
    return 0;
}

/* The entry (bfin-elf-ld -e bench_start): a stack of our own, then bench_main and the simulator's exit. */
static unsigned char stack[16384] __attribute__((aligned(32)));
unsigned char *const bench_stack_top = stack + sizeof stack - 32;
void bench_c(void) __attribute__((noreturn, used));
void bench_c(void)
{
    int rc = bench_main();
    sys(1, &rc);                                 /* exit */
    for (;;) { }
}
__asm__(".text\n"
        ".global bench_start\n"
        "bench_start:\n"
        "    P0.L = _bench_stack_top; P0.H = _bench_stack_top;\n"
        "    P0 = [P0];\n"
        "    SP = P0; FP = P0;\n"
        "    R0 = SYSCFG; BITSET (R0, 1); SYSCFG = R0; CSYNC;\n"   /* CCEN: the CYCLES counter runs */
        "    call _bench_c;\n");
