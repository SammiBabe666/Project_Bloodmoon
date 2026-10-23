/* .op1p parsing and relocation (op1p_load.c) */
#ifndef OP1P_LOAD_H
#define OP1P_LOAD_H
#include <stdint.h>
#include "op1p.h"

#define OP1P_API          1                /* = OP1_PLUGIN_API (op1_plugin.h) */
#define OP1P_MAX_IMAGE    (2048u * 1024)   /* per plugin, code + data and bss each (heap 0 has ~9 MB free, docs/02) */
#define OP1P_MAX_IMPORTS  32

enum { OP1P_E_SIZE = 1, OP1P_E_MAGIC, OP1P_E_CRC, OP1P_E_VERSION, OP1P_E_HEADER, OP1P_E_RELOC, OP1P_E_IMPORT };

typedef struct {
    const op1p_hdr   *hdr;
    const uint8_t    *image;
    const op1p_reloc *relocs;
    const char       *import[OP1P_MAX_IMPORTS];
    uint32_t          mem_size;            /* image + bss: bytes to allocate */
    uint32_t          svg_len;             /* > 0: the file starts with a usable SVG (screen) */
} op1p_view;

uint32_t op1p_crc32(const uint8_t *p, uint32_t n);
int op1p_parse(const uint8_t *file, uint32_t len, op1p_view *v);    /* 0 or OP1P_E_*; file 4-byte aligned */
/* copy + zero bss + relocate for address `base`; imp_addr[i] = address of v->import[i] */
int op1p_relocate(const op1p_view *v, uint8_t *dst, uint32_t base, const uint32_t *imp_addr);
#endif
