/* .op1p parsing and relocation (docs/37, format op1p.h). No firmware calls: shared by the boot loader and the host
 * test (harness/op1p_test.c). Every offset in the file is checked before use; a bad file is refused, never loaded. */
#pragma GCC optimize ("Os", "no-tree-loop-distribute-patterns")   /* boot-time code: size */
#include "op1p.h"
#include "op1p_load.h"

static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

uint32_t op1p_crc32(const uint8_t *p, uint32_t n)     /* zlib crc32; bitwise: runs once per plugin at boot */
{
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & -(c & 1));
    }
    return ~c;
}

int op1p_parse(const uint8_t *file, uint32_t len, op1p_view *v)
{
    if (len < sizeof(op1p_trailer) + sizeof(op1p_hdr) || (uintptr_t)file & 3) return OP1P_E_SIZE;
    const uint8_t *t = file + len - sizeof(op1p_trailer);
    uint32_t bin_off = rd32(t + 4), bin_len = rd32(t + 8);
    if (rd32(t) != OP1P_TMAGIC) return OP1P_E_MAGIC;
    if (bin_off & 3 || bin_off > len - sizeof(op1p_trailer) || bin_len > len - sizeof(op1p_trailer) - bin_off ||
        bin_len < sizeof(op1p_hdr))
        return OP1P_E_SIZE;
    if (op1p_crc32(file + bin_off, bin_len) != rd32(t + 12)) return OP1P_E_CRC;
    const op1p_hdr *h = (const op1p_hdr *)(file + bin_off);
    if (h->magic != OP1P_MAGIC) return OP1P_E_MAGIC;
    if (h->format != OP1P_FORMAT || h->api != OP1P_API) return OP1P_E_VERSION;
    if (h->kind > OP1P_TOOL) return OP1P_E_HEADER;
    int nl = 0;
    while (nl < 16 && h->name[nl]) {
        char c = h->name[nl++];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_')) return OP1P_E_HEADER;
    }
    if (nl == 0 || nl > 15) return OP1P_E_HEADER;
    uint32_t rest = bin_len - sizeof(op1p_hdr);
    if (h->image_size < 4 || h->image_size > OP1P_MAX_IMAGE || h->bss_size > OP1P_MAX_IMAGE || h->image_size & 3 ||
        h->image_size > rest || h->n_relocs > (rest - h->image_size) / sizeof(op1p_reloc) ||
        h->n_imports > OP1P_MAX_IMPORTS)
        return OP1P_E_SIZE;
    uint32_t mem = h->image_size + h->bss_size;
    if (h->entry_off > h->image_size - 4 || (h->screen_off != OP1P_NO_SCREEN && h->screen_off > h->image_size - 4) ||
        h->entry_off & 3 || (h->screen_off != OP1P_NO_SCREEN && h->screen_off & 3))
        return OP1P_E_HEADER;
    v->hdr = h;
    v->image = (const uint8_t *)(h + 1);
    v->relocs = (const op1p_reloc *)(v->image + h->image_size);
    const char *s = (const char *)(v->relocs + h->n_relocs), *end = (const char *)file + bin_off + bin_len;
    for (uint32_t i = 0; i < h->n_imports; i++) {            /* names: NUL-terminated, inside the binary part */
        v->import[i] = s;
        int k = 0;
        while (s < end && *s) { s++; if (++k > 31) return OP1P_E_IMPORT; }
        if (s >= end || k == 0) return OP1P_E_IMPORT;
        s++;
    }
    for (uint32_t i = 0; i < h->n_relocs; i++) {
        const op1p_reloc *r = &v->relocs[i];
        uint32_t w = r->type == OP1P_ABS32 ? 4 : 2;
        if ((r->type != OP1P_ABS32 && r->type != OP1P_HI16 && r->type != OP1P_LO16) || r->off > h->image_size - w ||
            r->off & (w - 1))
            return OP1P_E_RELOC;
        if (r->target & OP1P_IMPORT ? (r->target & ~OP1P_IMPORT) >= h->n_imports : r->target > mem)
            return OP1P_E_RELOC;
    }
    v->svg_len = bin_off;                                      /* SVG text + NUL + padding, 0 = no screen */
    if (bin_off) {
        const uint8_t *p = file;
        while (p < file + bin_off && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
        if (p == file + bin_off || *p != '<' || file[bin_off - 1] != 0) v->svg_len = 0;   /* unusable: no screen */
    }
    v->mem_size = mem;
    return 0;
}

int op1p_relocate(const op1p_view *v, uint8_t *dst, uint32_t base, const uint32_t *imp_addr)
{
    const op1p_hdr *h = v->hdr;
    for (uint32_t i = 0; i < h->image_size; i++) dst[i] = v->image[i];
    for (uint32_t i = h->image_size; i < v->mem_size; i++) dst[i] = 0;
    for (uint32_t i = 0; i < h->n_relocs; i++) {
        const op1p_reloc *r = &v->relocs[i];
        uint32_t x = (r->target & OP1P_IMPORT ? imp_addr[r->target & ~OP1P_IMPORT] : base + r->target) + r->addend;
        uint8_t *p = dst + r->off;
        if (r->type == OP1P_ABS32) { p[0] = x; p[1] = x >> 8; p[2] = x >> 16; p[3] = x >> 24; }
        else { uint32_t q = r->type == OP1P_HI16 ? x >> 16 : x; p[0] = q; p[1] = q >> 8; }
    }
    return 0;
}
