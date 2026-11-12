#!/usr/bin/env python3
"""Undo patch_xbe.py: rebuild the stock Halo 2 XBE from a Spooky Ghost one (every patched byte's original is known)."""
import hashlib, struct, sys
sys.path.insert(0, '.')
from patch_xbe import VERSIONS, BASE, NEW_NAME, PROBES

def main(version, src, out):
    cfg = VERSIONS[version]
    x = bytearray(open(src, 'rb').read())
    base, hdr_size, img_size, img_hdr_size, ts, cert, nsec, secaddr = struct.unpack_from('<8I', x, 0x104)
    assert x[:4] == b'XBEH' and base == BASE and ts == cfg['timestamp'] and nsec == 34, (hex(ts), nsec)
    t = secaddr - BASE
    secs = []
    for i in range(nsec):
        o = t + 56 * i
        fl, va, vs, raw, rs, na, rc, hr, tr = struct.unpack_from('<9I', x, o)
        secs.append(dict(va=va, vsize=vs, raw=raw, rsize=rs, name=x[na - BASE:].split(b'\0')[0].decode()))
    sp = secs[-1]
    assert sp['name'] == '.spooky' and sp['va'] == cfg['va'], sp
    newtab = bytes(x[t:t + 56 * 33])
    old = x.find(newtab, 0, 0x2000)
    if old == t: old = x.find(newtab, t + 1, 0x2000)
    assert old > 0 and old != t, 'old table not found'
    def file_off(va):
        for i, s in enumerate(secs[:33]):
            if s['va'] <= va < s['va'] + s['rsize']: return i, s['raw'] + va - s['va']
        raise SystemExit('0x%X not in a section' % va)
    touched = set()
    for site, target, hook in cfg['calls']:
        i, off = file_off(site)
        assert x[off] in (0xE8, 0xE9)
        struct.pack_into('<i', x, off + 1, target - (site + 5)); touched.add(i)
    for site, want, new in cfg.get('bytes', ()):
        i, off = file_off(site); a, b = bytes.fromhex(want), bytes.fromhex(new)
        assert bytes(x[off:off + len(b)]) == b, hex(site)
        x[off:off + len(a)] = a; touched.add(i)
    for site, want, new in cfg.get('triple', ()):             # 6i.42: the triple_buffering build's: put back where present
        i, off = file_off(site); a, b = bytes.fromhex(want), bytes.fromhex(new)
        if bytes(x[off:off + len(b)]) == b: x[off:off + len(a)] = a; touched.add(i)
        else: assert bytes(x[off:off + len(a)]) == a, hex(site)
    for site, want, new in cfg.get('tu_skip', ()):            # the "1.0 TU Skip" build's: put back where present
        i, off = file_off(site); a, b = bytes.fromhex(want), bytes.fromhex(new)
        if bytes(x[off:off + len(b)]) == b: x[off:off + len(a)] = a; touched.add(i)
        else: assert bytes(x[off:off + len(a)]) == a, hex(site)
    for site, want, new in cfg.get('x360', ()):               # v2.0: the 128 MB changes' (--x360): put back where present
        i, off = file_off(site); a, b = bytes.fromhex(want), bytes.fromhex(new)
        if bytes(x[off:off + len(b)]) == b: x[off:off + len(a)] = a; touched.add(i)
        else: assert bytes(x[off:off + len(a)]) == a, hex(site)
    if struct.unpack_from('<I', x, 0x124)[0] == 0x80000001: struct.pack_into('<I', x, 0x124, 0x80000005)   # ... and the header's limit-to-64 MB flag
    for site, want, sym in cfg.get('symrefs', ()):
        i, off = file_off(site); struct.pack_into('<I', x, off, want); touched.add(i)
    for site, want, new in PROBES[version]['bytes']:                 # 6i.6: the Direct3D probes' jumps, where present
        i, off = file_off(site); a, b = bytes.fromhex(want), bytes.fromhex(new)
        if bytes(x[off:off + len(b)]) == b: x[off:off + len(a)] = a; touched.add(i)
        else: assert bytes(x[off:off + len(a)]) == a, hex(site)
    for site, want, sym in PROBES[version]['symrefs']:
        i, off = file_off(site); struct.pack_into('<I', x, off, want); touched.add(i)
    if cfg['title']:
        c = cert - BASE
        x[c + 0xC:c + 0xC + 80] = 'Halo 2'.encode('utf-16le').ljust(80, b'\0')
    for i in touched:
        s = secs[i]
        d = hashlib.sha1(struct.pack('<I', s['rsize']) + bytes(x[s['raw']:s['raw'] + s['rsize']])).digest()
        x[old + 56 * i + 36:old + 56 * i + 56] = d
    # the new table, its name and its refs go back to zero
    need = 56 * (nsec) + len(NEW_NAME) + 4
    x[t:t + need] = b'\0' * need
    struct.pack_into('<I', x, 0x11C, 33)
    struct.pack_into('<I', x, 0x120, BASE + old)
    end_va = max(s['va'] + s['vsize'] for s in secs[:33])
    # v2.0: the stock image size is not always the last section's end (v1.5's is 8 bytes more): the stock values, by version
    struct.pack_into('<I', x, 0x10C, {'v1.0': 0x5754C0, 'v1.5': 0x5851C0}.get(version, end_va - BASE))
    end_raw = max(s['raw'] + s['rsize'] for s in secs[:33])
    assert (end_raw + 0xFFF) & ~0xFFF == sp['raw'], (hex(end_raw), hex(sp['raw']))
    del x[end_raw:]
    open(out, 'wb').write(x)
    print('%s: old table at 0x%X, touched %s, file 0x%X bytes' % (version, old, sorted(secs[i]['name'] for i in touched), len(x)))

if __name__ == '__main__':
    main(*sys.argv[1:])
