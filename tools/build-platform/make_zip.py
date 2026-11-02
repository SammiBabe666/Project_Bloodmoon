#!/usr/bin/env python3
"""Builds the user download, download/bloodmoon-<version>.zip, from the released files:

    python3 tools/build-platform/make_zip.py <version>

Inside bloodmoon-<version>/: opus/bloodmoon.opmod and hundekraft/bloodmoon.knl (from download/), presets/bloodmoon/
(the eight .aif files), INSTALL.md (download/INSTALL.md), LICENSE and NOTICE. Every entry is dated today at 00:00 (or the
day of SOURCE_DATE_EPOCH), so the same files on the same day give the same zip. Run tools/preset-validation/validate_presets.py first. Standard library only."""
import os, sys, time, zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    version = sys.argv[1]
    top = f'bloodmoon-{version}/'
    presets = sorted(f for f in os.listdir(os.path.join(ROOT, 'presets', 'bloodmoon')) if f.endswith('.aif'))
    files = [('hundekraft/bloodmoon.knl', 'download/hundekraft/bloodmoon.knl'), ('INSTALL.md', 'download/INSTALL.md'),
             ('NOTICE', 'NOTICE'), ('LICENSE', 'LICENSE')]
    files += [('presets/bloodmoon/' + f, 'presets/bloodmoon/' + f) for f in presets]
    files += [('opus/bloodmoon.opmod', 'download/opus/bloodmoon.opmod')]
    date = time.gmtime(int(os.environ.get('SOURCE_DATE_EPOCH', time.time())))[:3] + (0, 0, 0)
    out = os.path.join(ROOT, 'download', f'bloodmoon-{version}.zip')
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
        for d in ('', 'hundekraft/', 'presets/', 'presets/bloodmoon/', 'opus/'):
            z.writestr(zipfile.ZipInfo(top + d, date), '')
        for name, src in files:
            info = zipfile.ZipInfo(top + name, date)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            z.writestr(info, open(os.path.join(ROOT, src), 'rb').read())
    print(out, os.path.getsize(out), 'bytes,', len(files), 'files')


if __name__ == '__main__':
    main()
