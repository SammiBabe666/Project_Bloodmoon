/* host test for moon.c: a mock screen that knows exactly the SVG's ids (aborts on any other non-null use), runs the
 * screen with knob sets and prints, per knob, the visible digit ids and the value group's x shift. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "op1_plugin.h"
#define MAXN 512
static char ids[MAXN][16]; static int nids; static int vis[MAXN]; static int32_t tx[MAXN], ty[MAXN];
int16_t host_knobs[8][8]; int host_slot; const uint8_t *host_sample_buffer[8];
int16_t host_fx_knobs[3][8]; int16_t host_fx_meter[3]; void *host_fx_scratch[3];
#define knobs host_knobs[0]
int16_t host_sine[8192]; int32_t host_pitch_inc[128]; uint8_t host_voice_mem[6 * 0x24]; int16_t host_voice_vel[6]; int16_t host_voice_env[128]; int16_t host_accel[3];
void *host_find_node(void *s, const char *id) { (void)s; for (int i = 0; i < nids; i++) if (!strcmp(ids[i], id)) return &ids[i]; return 0; }
static int idx(void *n) { long i = (char (*)[16])n - ids; if (i < 0 || i >= nids) abort(); return (int)i; }
void host_node_set_transform(void *n, const int32_t *m) { int i = idx(n); tx[i] = m[6]; ty[i] = m[7]; }
void host_node_set_visible(void *n, int on) { vis[idx(n)] = on; }
uint8_t *host_node_style_rgba(void *s, const char *id) { static uint8_t c[4]; (void)s; (void)id; return c; }
void host_screen_draw(void *s, int dt) { (void)s; (void)dt; }
void *host_raster(void) { return 0; }
void host_raster_line(void *r, const void *a, const void *b, int32_t w, const uint8_t *c) { (void)r; (void)a; (void)b; (void)w; (void)c; }
void *read_fx_param_safe(int slot, int pos) { (void)slot; (void)pos; return 0; }
extern const op1_screen_t SCREEN_bloodmoon;
int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "r"); char line[4096];
    while (fgets(line, sizeof line, f)) for (char *p = line; (p = strstr(p, " id=\"")); p += 5) {
        sscanf(p + 5, "%15[^\"]", ids[nids]); vis[nids] = 1; nids++; }
    for (int i = 0; i < 8192; i++) host_sine[i] = 0;
    SCREEN_bloodmoon.init(0);
    for (int a = 2; a + 3 < argc; a += 4) {
        for (int k = 0; k < 4; k++) knobs[k] = (int16_t)atoi(argv[a + k]);
        SCREEN_bloodmoon.update(0, 16);
        for (int k = 0; k < 4; k++) {
            char g[8]; snprintf(g, sizeof g, "k%dv", k);
            printf("%d %d tx=%d :", k, knobs[k], tx[idx(host_find_node(0, g))]);
            for (int i = 0; i < nids; i++) if (ids[i][0] == 'k' && ids[i][2] == 'p' && vis[i]) printf(" %s", ids[i]);
            printf("\n");
        }
    }
    return 0;
}
