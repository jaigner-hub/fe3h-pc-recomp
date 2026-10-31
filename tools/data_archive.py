"""Read the game's KTGL data archive, DATA0.bin (index) and DATA1.bin (data), straight from the base game file.

  .venv/bin/python tools/data_archive.py summary                 counts, sizes and leading magics of the entries
  .venv/bin/python tools/data_archive.py extract N [N ...]       write entries, decompressed, to build/data/<N>.bin
  .venv/bin/python tools/data_archive.py scan TEXT [TEXT ...]    which entries contain these byte strings

Paths come from config.local.toml (input.base, input.keys); --game and --keys override them. Output stays under
build/. Base game only: entries the update replaces are not seen (romfs_reader.py).

DATA0.bin is an array of 32-byte entries {u64 offset into DATA1.bin, u64 size, u64 stored size, u64 compressed}; an
entry of size 0 is unused. A compressed entry is chunked zlib (inflate()).
"""
import argparse
import re
import struct
import sys
import tomllib
import zlib
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from extract_exefs import require  # noqa: E402
from romfs_reader import RomFS  # noqa: E402

ENTRY = struct.Struct('<4Q')
OUT = ROOT / 'build' / 'data'


def inflate(buf, size):
    """The size bytes of a compressed entry. Layout: u32 chunk size, u32 chunk count, u32 total size, then one u32
    stored length per chunk; each chunk starts 0x80-aligned. A chunk is a u32 length and a zlib stream, or, when its
    stored length is not that length plus 4, the chunk's bytes as they are."""
    _, count, total = struct.unpack_from('<III', buf, 0)
    require(total == size, f'compressed entry says {total} bytes, index says {size}')
    pos, out = (12 + 4 * count + 0x7F) & ~0x7F, bytearray()
    for stored in struct.unpack_from(f'<{count}I', buf, 12):
        n = struct.unpack_from('<I', buf, pos)[0]
        if n + 4 == stored and buf[pos + 4] == 0x78:   # 0x78: zlib header
            out += zlib.decompress(buf[pos + 4:pos + 4 + n])
        else:
            out += buf[pos:pos + stored]
        pos = (pos + stored + 0x7F) & ~0x7F
    require(len(out) == size, f'decompressed {len(out)} bytes, expected {size}')
    return bytes(out)


class Archive:
    def __init__(self, game, keys):
        self.romfs = RomFS(game, keys)
        index = self.romfs.read_file('DATA0.bin')
        require(len(index) % ENTRY.size == 0, 'DATA0.bin is not a whole number of entries')
        self.entries = [ENTRY.unpack_from(index, i) for i in range(0, len(index), ENTRY.size)]
        self._data, self._data_size = self.romfs.files()['DATA1.bin']
        for i, (offset, size, stored, _) in enumerate(self.entries):
            require(not size or offset + stored <= self._data_size, f'entry {i} lies outside DATA1.bin')

    def stored(self, i, limit=None):
        """Entry i as stored in DATA1.bin, or its first limit bytes."""
        offset, size, stored, compressed = self.entries[i]
        return self.romfs.read(self._data + offset, min(stored if compressed else size, limit or 1 << 62))

    def get(self, i):
        """Entry i, decompressed."""
        _, size, _, compressed = self.entries[i]
        data = self.stored(i)
        return inflate(data, size) if compressed else data

    def head(self, i, n=16):
        """First n bytes of entry i, decompressing only its first chunk."""
        _, size, _, compressed = self.entries[i]
        if not compressed:
            return self.stored(i, n)
        top = self.stored(i, 16)
        count, first = struct.unpack_from('<I', top, 4)[0], struct.unpack_from('<I', top, 12)[0]
        start = (12 + 4 * count + 0x7F) & ~0x7F
        chunk = self.stored(i, start + first)[start:]
        length = struct.unpack_from('<I', chunk, 0)[0]
        if length + 4 == first and chunk[4] == 0x78:
            return zlib.decompressobj().decompress(chunk[4:4 + length], n)
        return chunk[:n]

    def used(self):
        return [i for i, e in enumerate(self.entries) if e[1]]


def magic(data):
    m = data[:4]
    return m.decode('ascii') if len(m) == 4 and all(32 <= c < 127 for c in m) else m.hex()


def summary(archive, args):
    used = archive.used()
    compressed = [i for i in used if archive.entries[i][3]]
    size = sum(archive.entries[i][1] for i in used)
    print(f'{len(archive.entries)} entries, {len(used)} used, {len(compressed)} compressed; '
          f'{size / 1e9:.2f} GB unpacked')
    counts, sizes = Counter(), Counter()
    for i in used:
        k = magic(archive.head(i, 4))
        counts[k] += 1
        sizes[k] += archive.entries[i][1]
    for k, n in counts.most_common(args.top):
        print(f'{n:7} {sizes[k] / 1e6:10.1f} MB  {k}')


def extract(archive, args):
    out = args.out.resolve()
    require(out.is_relative_to(ROOT / 'build'), f'{out} is outside build/; game data stays in ignored folders')
    out.mkdir(parents=True, exist_ok=True)
    for i in args.entries:
        require(0 <= i < len(archive.entries) and archive.entries[i][1], f'entry {i} is unused or out of range')
        data = archive.get(i)
        (out / f'{i}.bin').write_bytes(data)
        print(f'{i}: {len(data):,} bytes, {magic(data)} -> {out / f"{i}.bin"}')


def scan(archive, args):
    patterns = [p.encode('latin-1') for p in args.text]
    found = Counter()
    for i in archive.used():
        data = archive.get(i)
        hits = {p: data.count(p) for p in patterns if p in data}
        if hits:
            found.update(hits.keys())
            print(f'{i}\t{archive.entries[i][1]}\t{magic(data)}\t' +
                  ' '.join(f'{p.decode("latin-1")!r}:{n}' for p, n in hits.items()), flush=True)
    print('entries containing: ' + ', '.join(f'{p.decode("latin-1")!r} {found[p]}' for p in patterns),
          file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--config', type=Path, default=ROOT / 'config.local.toml')
    ap.add_argument('--game', type=Path, help='base game XCI or NSP (default: input.base)')
    ap.add_argument('--keys', type=Path, help='prod.keys (default: input.keys)')
    sub = ap.add_subparsers(dest='command', required=True)
    p = sub.add_parser('summary')
    p.add_argument('--top', type=int, default=30, help='magics to list')
    p = sub.add_parser('extract')
    p.add_argument('entries', type=int, nargs='+')
    p.add_argument('--out', type=Path, default=OUT)
    p = sub.add_parser('scan')
    p.add_argument('text', nargs='+', help='byte strings (latin-1)')
    args = ap.parse_args()

    config = {} if args.game and args.keys else tomllib.loads(args.config.read_text(encoding='utf-8'))
    game = args.game or Path(config['input']['base'])
    keys = args.keys or Path(config['input']['keys'])
    archive = Archive(game, keys)
    {'summary': summary, 'extract': extract, 'scan': scan}[args.command](archive, args)


if __name__ == '__main__':
    try:
        main()
    except ValueError as error:
        sys.exit(f'error: {error}')
