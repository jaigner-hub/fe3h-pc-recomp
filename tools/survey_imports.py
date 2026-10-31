"""Phase 0 survey of an extracted ExeFS: module names, SDK version strings, and the SDK imports of the game modules
compared with the functions the TotK host replaces by symbol.

  .venv/bin/python tools/survey_imports.py build/extracted/v262144/exefs [--totk ../totk-pc-recomp]

Prints only names and counts (module paths, SDK middleware tags, symbol names), never code or data. --totk names a
totk-pc-recomp checkout (default refs/totk-pc-recomp); its override set is every quoted `_ZN2nn...` or `_ZN5movie...`
name in runtime/, as runtime/hostfn.c registers overrides by mangled name.
"""
import argparse
import re
import shutil
import struct
import subprocess
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'recomp'))
from nso import Nso  # noqa: E402

MODULES = ('rtld', 'main', 'subsdk0', 'subsdk1', 'subsdk2', 'subsdk3', 'sdk')
SDK_TAG = re.compile(rb'SDK MW\+[ -~]{4,120}')
OVERRIDE = re.compile(r'"(_ZN(?:2nn|5movie)[A-Za-z0-9_]+)"')


def demangle(names):
    """names demangled by c++filt, without parameter lists; the names unchanged when c++filt is missing."""
    if not shutil.which('c++filt'):
        return list(names)
    out = subprocess.run(['c++filt'], input='\n'.join(names), capture_output=True, text=True).stdout.splitlines()
    return [n.split('(', 1)[0] for n in out]


def namespace(name):
    parts = name.split('::')
    return '::'.join(parts[:2]) if len(parts) > 2 else parts[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('exefs', type=Path)
    ap.add_argument('--totk', type=Path, default=ROOT / 'refs' / 'totk-pc-recomp')
    args = ap.parse_args()

    imports = {}
    for name in MODULES:
        path = args.exefs / name
        if not path.is_file():
            continue
        m = Nso(path)
        ro = bytes(m.image[m.rodata[0]:m.rodata[0] + m.rodata[1]])
        length = struct.unpack_from('<I', ro, 4)[0]
        module_path = ro[8:8 + length].decode('ascii', 'replace') if 0 < length < 0x300 else '?'
        tags = sorted({t.decode() for t in SDK_TAG.findall(bytes(m.image))})
        print(f'{name}: {module_path}; needed {m.needed}; {len(m.imports())} imports, {len(m.exports())} exports')
        for tag in tags:
            print(f'    {tag}')
        if name != 'sdk':   # sdk resolves against rtld only; the game modules' imports are what the host sees
            imports[name] = sorted({s[0] for s in m.imports()})

    game = sorted({n for names in imports.values() for n in names if n.startswith(('_ZN2nn', '_ZN5movie'))})
    runtime = args.totk / 'runtime'
    if not runtime.is_dir():
        sys.exit(f'{runtime} not found: run tools/fetch_refs.sh or pass --totk')
    totk = {n for f in runtime.rglob('*.c') for n in OVERRIDE.findall(f.read_text(errors='replace'))}
    covered = [n for n in game if n in totk]
    print(f'\nnn:: and movie:: imports of the game modules: {len(game)}; replaced by the TotK host: {len(covered)}')
    for title, names in (('covered', covered), ('not covered', [n for n in game if n not in totk])):
        counts = Counter(namespace(n) for n in demangle(names))
        print(f'  {title}: ' + ', '.join(f'{k} {v}' for k, v in counts.most_common()))


if __name__ == '__main__':
    main()
