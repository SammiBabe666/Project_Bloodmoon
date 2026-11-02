#!/usr/bin/env python3
"""Adds BLOODMOON's knob panels to bloodmoon.svg: on each side of the cathedral, per knob its name in blackletter
(Pirata One, SIL OFL 1.1) over an ornamental rule, and its value 0..100 in large blackletter numerals.

Glyphs become flat polygons (no curves, no fill-rule reliance): each outer contour is its own filled path in the knob's
colour, each counter (the hole in o, e, 0, 8 ...) a black filled path drawn after it.
Static: name + rule. Dynamic (moon.c): group k<knob>v holds one group per digit glyph and position, k<knob>p<pos>d<digit>
(pos 0 = hundreds, only the 1); moon.c shows the value's digits and shifts k<knob>v left so the number stays centred.

    python3 tests/make_labels.py tests/fonts/PirataOne-Regular.ttf <in.svg> <out.svg>     (prints the digit cell width for moon.c)
"""
import re, sys
from fontTools.ttLib import TTFont
from fontTools.pens.basePen import BasePen

BLUE, GREEN, WHITE, RED, GREY = '#4D9EFF', '#00ED95', '#FFFFFF', '#FF3A5D', '#6F7173'
PANELS = [(0, 'Blood', BLUE, 50, 21), (1, 'Eclipse', GREEN, 50, 95), (2, 'Haunting', WHITE, 270, 21), (3, 'Moonrise', RED, 270, 95)]
NAME_CAP, NUM_CAP = 11.5, 25.0       # cap height in pixels
CELL_GAP = 1.0

class Flat(BasePen):
    """records contours as polygons, curves flattened"""
    def __init__(self, gs, steps=4):
        super().__init__(gs); self.contours, self.cur, self.steps = [], [], steps
    def _moveTo(self, p): self.cur = [p]
    def _lineTo(self, p): self.cur.append(p)
    def _qCurveToOne(self, p1, p2):
        p0 = self.cur[-1]
        for i in range(1, self.steps + 1):
            t = i / self.steps
            self.cur.append(((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
                             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]))
    def _curveToOne(self, p1, p2, p3):
        p0 = self.cur[-1]
        for i in range(1, self.steps + 1):
            t = i / self.steps; u = 1 - t
            self.cur.append((u ** 3 * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t ** 3 * p3[0],
                             u ** 3 * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t ** 3 * p3[1]))
    def _closePath(self):
        if len(self.cur) > 2: self.contours.append(self.cur)
        self.cur = []
    _endPath = _closePath

EPS = 0.5                            # simplification tolerance in screen pixels

def rdp(pts, eps):
    if len(pts) < 3: return pts
    (x0, y0), (x1, y1) = pts[0], pts[-1]
    dx, dy = x1 - x0, y1 - y0; n = (dx * dx + dy * dy) ** 0.5
    best, bi = -1, 0
    for i in range(1, len(pts) - 1):
        x, y = pts[i]
        d = abs(dy * (x - x0) - dx * (y - y0)) / n if n else ((x - x0) ** 2 + (y - y0) ** 2) ** 0.5
        if d > best: best, bi = d, i
    if best <= eps: return [pts[0], pts[-1]]
    return rdp(pts[:bi + 1], eps)[:-1] + rdp(pts[bi:], eps)

def simplify(c, eps):
    """closed polygon: split at the point farthest from the first one, simplify both halves"""
    far = max(range(len(c)), key=lambda i: (c[i][0] - c[0][0]) ** 2 + (c[i][1] - c[0][1]) ** 2)
    a = rdp(c[:far + 1], eps); b = rdp(c[far:] + [c[0]], eps)
    return a[:-1] + b[:-1]

def area(c):
    return sum(c[i][0] * c[(i + 1) % len(c)][1] - c[(i + 1) % len(c)][0] * c[i][1] for i in range(len(c))) / 2

def f(v):
    s = f'{v:.1f}'.rstrip('0').rstrip('.')
    return '0' if s == '-0' else s

class Font:
    def __init__(self, path):
        self.t = TTFont(path); self.gs = self.t.getGlyphSet(); self.cmap = self.t.getBestCmap()
        self.cap = self.t['OS/2'].sCapHeight; self.hm = self.t['hmtx']
    def glyph(self, ch):
        n = self.cmap[ord(ch)]
        p = Flat(self.gs); self.gs[n].draw(p)
        return p.contours, self.hm[n][0]
    def width(self, text, sc):
        return sum(self.hm[self.cmap[ord(ch)]][0] for ch in text) * sc
    def paths(self, ch, x, base, sc, col):
        """filled outer contours in col, counters in black after them (TrueType: outer contours run clockwise)"""
        cs, _ = self.glyph(ch)
        def d(c):
            pts, last = [], None
            for X, Y in simplify([(x + X * sc, base - Y * sc) for X, Y in c], EPS):
                q = (f(X), f(Y))
                if q != last: pts.append(q); last = q
            if len(pts) > 1 and pts[0] == pts[-1]: pts.pop()
            return 'M' + 'L'.join(f'{a},{b}' for a, b in pts) + 'Z'
        outer = [c for c in cs if area(c) < 0]; holes = [c for c in cs if area(c) >= 0]
        return ([f'<path fill="{col}" d="{d(c)}"/>' for c in outer] + [f'<path fill="#000000" d="{d(c)}"/>' for c in holes])

def text(font, s, cx, base, cap, col):
    sc = cap / font.cap
    x = cx - font.width(s, sc) / 2
    out = []
    for ch in s:
        out += font.paths(ch, x, base, sc, col)
        x += font.hm[font.cmap[ord(ch)]][0] * sc
    return out, cx - font.width(s, sc) / 2, cx + font.width(s, sc) / 2

def diamond(cx, cy, rx, ry, col):
    return f'<path fill="{col}" d="M{f(cx)},{f(cy - ry)}L{f(cx + rx)},{f(cy)}L{f(cx)},{f(cy + ry)}L{f(cx - rx)},{f(cy)}Z"/>'

def panel(font, kn, name, col, cx, top):
    out = []
    nb = top + NAME_CAP
    t, x0, x1 = text(font, name, cx, nb, NAME_CAP, col)
    out += t
    # rule under the name: thin line with a lozenge in the middle and at both ends
    ry = nb + 5.5
    half = 30
    out.append(f'<path fill="none" stroke="{GREY}" stroke-width="1" d="M{f(cx - half)},{f(ry)}L{f(cx - 4)},{f(ry)}M{f(cx + 4)},{f(ry)}L{f(cx + half)},{f(ry)}"/>')
    out.append(diamond(cx, ry, 2.6, 2.6, col))
    out.append(diamond(cx - half - 2, ry, 1.6, 1.6, GREY))
    out.append(diamond(cx + half + 2, ry, 1.6, 1.6, GREY))
    # value digits, 3 cells laid out for "100" centred on cx; moon.c shifts the group for 1 and 2 digits
    sc = NUM_CAP / font.cap
    cell = max(font.hm[font.cmap[ord(str(d))]][0] for d in range(10)) * sc + CELL_GAP
    vb = ry + 6 + NUM_CAP
    left = cx - 1.5 * cell
    out.append(f'<g id="k{kn}v">')
    for p in range(3):
        for dg in range(10):
            if p == 0 and dg != 1:
                continue
            w = font.hm[font.cmap[ord(str(dg))]][0] * sc
            out.append(f'<g id="k{kn}p{p}d{dg}">' + ''.join(font.paths(str(dg), left + p * cell + (cell - w) / 2, vb, sc, col)) + '</g>')
    out.append('</g>')
    box = (min(x0, cx - half - 4) - 2, top - 2, max(x1, cx + half + 4) + 2, vb + 3)
    return out, box, cell

def main(fontp, src, dst):
    font = Font(fontp)
    svg = open(src).read()
    parts, boxes = [], []
    for p in PANELS:
        o, b, cell = panel(font, *p)
        parts += o; boxes.append(b)
    def keep(m):
        x, y = float(m.group(1)), float(m.group(2))
        return '' if any(b[0] <= x <= b[2] and b[1] <= y <= b[3] for b in boxes) else m.group(0)
    svg = re.sub(r'<circle fill="#(?:8A8A8A|FFFFFF)" cx="([\d.]+)" cy="([\d.]+)" r="0\.(?:7|95)"/>\n', keep, svg)
    note = ('<!-- Knob panels (tests/make_labels.py): names and numerals are outlines of Pirata One by Rodrigo Fuenzalida and\n'
            '     Nicolas Massi, SIL Open Font License 1.1. moon.c shows k<knob>p<pos>d<digit> and moves k<knob>v. -->\n')
    svg = svg.replace('</svg>', note + '<g id="panels">\n' + '\n'.join(parts) + '\n</g>\n</svg>')
    open(dst, 'w').write(svg)
    print(f'{dst}: {len(svg)} B, digit cell {cell:.3f} px')

if __name__ == '__main__':
    main(*sys.argv[1:4])
