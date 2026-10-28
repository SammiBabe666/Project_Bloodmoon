import sys, re, subprocess, os
# preview.py in.svg out.png k0 k1 k2 k3 (knobs 0..32767): digits, centring shift and moon height as moon.c sets them
svg = open(sys.argv[1]).read(); kn = [int(x) for x in sys.argv[3:7]]
cell = 14938 / 1024
show, shift = set(), {}
for k, q in enumerate(kn):
    v = (q * 100 + 16383) >> 15; s = str(v)
    show |= {f'k{k}p{3 - len(s) + i}d{c}' for i, c in enumerate(s)}
    shift[k] = -(3 - len(s)) * cell / 2
svg = re.sub(r'<g id="(k\dp\dd\d)">', lambda m: m.group(0) if m.group(1) in show else f'<g id="{m.group(1)}" display="none">', svg)
svg = re.sub(r'<g id="k(\d)v">', lambda m: f'<g id="k{m.group(1)}v" transform="translate({shift[int(m.group(1))]:.3f},0)">', svg)
svg = svg.replace('<g id="moon">', f'<g id="moon" transform="translate(0,{((32767 - kn[3]) * 24 >> 5) / 1024:.3f})">')
svg = svg.split('?>', 1)[-1].replace('width="320px" height="160px"', 'width="960" height="480"')
h = sys.argv[2] + '.html'; open(h, 'w').write(f'<html><body style="margin:0;background:#000">{svg}</body></html>')
subprocess.run(['/opt/pw-browsers/chromium-1194/chrome-linux/chrome', '--headless', '--no-sandbox', '--disable-gpu', '--hide-scrollbars',
                '--window-size=960,700', f'--screenshot={sys.argv[2]}', 'file://' + os.path.abspath(h)], capture_output=True)
from PIL import Image
Image.open(sys.argv[2]).crop((0, 0, 960, 480)).save(sys.argv[2])
