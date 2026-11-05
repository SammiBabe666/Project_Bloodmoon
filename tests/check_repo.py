#!/usr/bin/env python3
"""Checks of the repository itself, the things no engine test sees. Standard library only:

    python3 tests/check_repo.py

1. releases: every file in releases/README.md's table exists and its SHA-256 matches the table's;
2. download: download/bloodmoon-<version>.zip holds exactly what tools/build-platform/make_zip.py puts in it, byte for
   byte from download/, presets/, LICENSE and NOTICE; the .opmod's own manifest says the zip's version; INSTALL.md and
   releases/README.md name it;
3. manifests: the Opus manifest's id is its folder, its version is x.y.z, cpu.txt gives a cycle count, and CHANGELOG.md
   has a row for the Opus and the Hundekraft versions;
4. firmware ties: every OP-1 firmware function (fw_*) and raw firmware address (0x01xxxxxx, 0xFFxxxxxx) the Opus source
   uses is listed in tests/firmware-ties.txt, and so is every fw_ name the released .opmod links (its "deep" line).
   A new tie fails until it is listed there with a reason: Beta 4 and later cores may move or drop them.
"""
import hashlib, os, re, sys, zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
OPUS_SRC = os.path.join(ROOT, 'platforms', 'opus', 'src', 'noladrift.bloodmoon')
failures = []


def path(*p):
    return os.path.join(ROOT, *p)


def read(*p, mode='r'):
    with open(path(*p), mode) as f:
        return f.read()


def fail(what):
    failures.append(what)
    print('FAIL:', what)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def opmod_manifest(data):
    """The text manifest an .opmod carries after its binary header (op1mod's plugin.py writes it): {key: value}."""
    if data[:4] != b'OPMD':
        return {}
    start = data.find(b'\nopmod ')
    start = data.find(b'opmod ') if start < 0 else start + 1
    end = data.find(b'\0', start)
    out = {}
    for line in data[start:end].decode('ascii', 'replace').splitlines():
        if line.strip() and not line.startswith('#'):
            k, _, v = line.partition(' ')
            out[k] = v.strip()
    return out


def check_releases():
    table = read('releases', 'README.md')
    rows = 0
    for line in table.splitlines():
        cells = [c.strip() for c in line.strip().strip('|').split('|')]
        if len(cells) < 5 or not re.match(r'^`?[0-9a-f]{8}', cells[4]):
            continue
        rows += 1
        link = re.search(r'\]\(([^)]+)\)', cells[1])
        m = re.match(r'^`([0-9a-f]+)…([0-9a-f]+)`$', cells[4])
        if not link or not m:
            fail(f'releases/README.md: row {cells[0]!r} has no file link or SHA-256 "head…tail"')
            continue
        f = os.path.normpath(os.path.join(ROOT, 'releases', link.group(1)))
        if not os.path.isfile(f):
            fail(f'releases/README.md: {cells[0]}: {os.path.relpath(f, ROOT)} does not exist')
            continue
        h = sha256(open(f, 'rb').read())
        if not (h.startswith(m.group(1)) and h.endswith(m.group(2))):
            fail(f'releases/README.md: {cells[0]}: {os.path.relpath(f, ROOT)} is {h[:8]}…{h[-5:]}, '
                 f'the table says {m.group(1)}…{m.group(2)}')
    if rows < 6:
        fail(f'releases/README.md: only {rows} release rows found')
    print(f'releases: {rows} files match their SHA-256 in releases/README.md')


def check_download():
    zips = [f for f in os.listdir(path('download')) if re.match(r'^bloodmoon-.*\.zip$', f)]
    if len(zips) != 1:
        fail(f'download/: {len(zips)} bloodmoon-*.zip files, want exactly one')
        return
    version = re.match(r'^bloodmoon-(.*)\.zip$', zips[0]).group(1)
    top = f'bloodmoon-{version}/'
    want = {'opus/bloodmoon.opmod': 'download/opus/bloodmoon.opmod',
            'hundekraft/bloodmoon.knl': 'download/hundekraft/bloodmoon.knl',
            'INSTALL.md': 'download/INSTALL.md', 'LICENSE': 'LICENSE', 'NOTICE': 'NOTICE'}
    for f in sorted(os.listdir(path('presets', 'bloodmoon'))):
        if f.endswith('.aif'):
            want['presets/bloodmoon/' + f] = 'presets/bloodmoon/' + f
    with zipfile.ZipFile(path('download', zips[0])) as z:
        names = {n[len(top):] for n in z.namelist() if n.startswith(top) and not n.endswith('/')}
        stray = [n for n in z.namelist() if not n.startswith(top)]
        if stray:
            fail(f'{zips[0]}: entries outside {top}: {stray}')
        for n in sorted(names - set(want)):
            fail(f'{zips[0]}: {n} is not one of the files make_zip.py puts in')
        for n, src in sorted(want.items()):
            if n not in names:
                fail(f'{zips[0]}: no {n}')
            elif z.read(top + n) != read(src, mode='rb'):
                fail(f'{zips[0]}: {n} differs from {src} (rebuild it: tools/build-platform/make_zip.py {version})')
    opmod = read('download', 'opus', 'bloodmoon.opmod', mode='rb')
    m = opmod_manifest(opmod)
    if m.get('id') != 'noladrift.bloodmoon' or m.get('version') != version:
        fail(f'download/opus/bloodmoon.opmod: manifest id {m.get("id")!r} version {m.get("version")!r}, '
             f'the zip is {version}')
    if version not in read('download', 'INSTALL.md').splitlines()[0]:
        fail(f'download/INSTALL.md: its first line does not name {version}')
    h = sha256(opmod)
    if not re.search(rf'`{h[:8]}…{h[-5:]}`', read('releases', 'README.md')):
        fail(f'releases/README.md: no row for download/opus/bloodmoon.opmod ({h[:8]}…{h[-5:]})')
    print(f'download: bloodmoon-{version}.zip holds {len(want)} files, each equal to its source; '
          f'the .opmod says {version}')
    return m


def check_manifests():
    text = read('platforms', 'opus', 'src', 'noladrift.bloodmoon', 'manifest.txt')
    m = {}
    for line in text.splitlines():
        if line.strip() and not line.startswith('#'):
            k, _, v = line.partition(' ')
            m[k] = v.strip()
    for k in ('opmod', 'id', 'name', 'author', 'version', 'core', 'adds'):
        if k not in m:
            fail(f'Opus manifest.txt: no {k} line')
    if m.get('id') != os.path.basename(OPUS_SRC):
        fail(f'Opus manifest.txt: id {m.get("id")!r} is not its folder\'s name')
    if not re.match(r'^\d+\.\d+\.\d+$', m.get('version', '')):
        fail(f'Opus manifest.txt: version {m.get("version")!r} is not x.y.z')
    if not re.match(r'^\d+\.\d+$', m.get('core', '')):
        fail(f'Opus manifest.txt: core {m.get("core")!r} is not x.y')
    cpu = re.findall(r'^cycles\s+(\d+)\s*$', read('platforms', 'opus', 'src', 'noladrift.bloodmoon', 'cpu.txt'), re.M)
    if len(cpu) != 1 or not 10000 <= int(cpu[0]) <= 600000:
        fail(f'Opus cpu.txt: want one "cycles N" line with N a plausible block cost, got {cpu}')
    import json
    hk = json.loads(read('platforms', 'hundekraft', 'src', 'plugin.json'))
    log = read('CHANGELOG.md')
    versions = [r.split('|')[1].strip().strip('*') for r in log.splitlines() if r.startswith('| v') or r.startswith('| **v')]
    for name, v in (('Opus', m.get('version', '?')), ('Hundekraft', hk.get('version', '?'))):
        if not any(re.match(rf'^v{re.escape(v)}(\b|[- ])', row) for row in versions):
            fail(f'CHANGELOG.md: no row for the {name} version v{v}')
    print(f'manifests: Opus {m.get("id")} {m.get("version")} (core {m.get("core")}, {cpu[0] if cpu else "?"} cycles), '
          f'Hundekraft {hk.get("version")}, both in CHANGELOG.md')


def strip_c(src):
    """C source without comments and string/char literals (so prose and the art's SVG never count)."""
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', ' ', src, flags=re.S)


def check_firmware_ties(released):
    listed = {}
    for line in read('tests', 'firmware-ties.txt').splitlines():
        if line.strip() and not line.startswith('#'):
            name, _, why = line.partition(' ')
            listed[name.lower() if name.startswith('0x') else name] = why.strip()
    found = {}
    for f in sorted(os.listdir(OPUS_SRC)):
        if not f.endswith(('.c', '.h')):
            continue
        code = strip_c(open(os.path.join(OPUS_SRC, f), encoding='utf-8', errors='replace').read())
        for t in re.findall(r'\bfw_\w+', code):
            found.setdefault(t, set()).add(f)
        for t in re.findall(r'\b0[xX](?:01[0-9A-Fa-f]{6}|[Ff][Ff][0-9A-Fa-f]{6})\b', code):
            found.setdefault(t.lower(), set()).add(f)
    for t in sorted(set(found) - set(listed)):
        fail(f'firmware tie {t} in {", ".join(sorted(found[t]))} is not in tests/firmware-ties.txt: list it, with '
             f'what it is and where it comes from, or use a core service instead')
    deep = (released or {}).get('deep', '').split()
    for t in deep:
        if t != 'none' and t not in listed:
            fail(f'download/opus/bloodmoon.opmod links {t} ("deep" line), which tests/firmware-ties.txt does not list')
    for t in sorted(set(listed) - set(found)):
        print(f'note: {t} is listed in tests/firmware-ties.txt but the Opus source no longer uses it: remove it there')
    print(f'firmware ties: the Opus source uses {len(found)} ({", ".join(sorted(found)) or "none"}), all listed; '
          f'the released .opmod links {len([d for d in deep if d != "none"])}')


def main():
    check_releases()
    released = check_download()
    check_manifests()
    check_firmware_ties(released)
    if failures:
        sys.exit(f'{len(failures)} repository check(s) failed')
    print('PASS')


if __name__ == '__main__':
    main()
