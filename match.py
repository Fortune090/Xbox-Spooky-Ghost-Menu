"""Find v1.0 functions in the v1.5 XBE: mask address-like bytes, match the rest uniquely."""
import struct, sys, csv
sys.path.insert(0, '/home/claude/xbox')
import xdis
V10 = xdis.X
V15 = open('/mnt/user-data/uploads/default_1.xbe', 'rb').read()
def sections(d):
    base = struct.unpack_from('<I', d, 0x104)[0]; nsec, secaddr = struct.unpack_from('<II', d, 0x11C); out = []
    for i in range(nsec):
        o = secaddr - base + 56 * i
        fl, va, vsz, raw, rsz, na = struct.unpack_from('<6I', d, o)
        out.append((d[na - base:d.index(b'\0', na - base)].decode(), va, vsz, raw, rsz))
    return out
S10, S15 = sections(V10), sections(V15)
def read(d, secs, va, n):
    for name, sva, vsz, raw, rsz in secs:
        if sva <= va and va + n <= sva + rsz: return d[raw + va - sva: raw + va - sva + n]
def code15():
    blob, base = b'', None
    for name, sva, vsz, raw, rsz in S15:
        if name in ('.text', 'D3D', 'XPP'):
            yield name, sva, V15[raw:raw + rsz]
def mask_of(b):
    m = [True] * len(b); i = 0
    while i < len(b):
        if b[i] in (0xE8, 0xE9) and i + 5 <= len(b): m[i + 1:i + 5] = [False] * 4
        if b[i] == 0x0F and i + 6 <= len(b) and 0x80 <= b[i + 1] <= 0x8F: m[i + 2:i + 6] = [False] * 4
        i += 1
    for i in range(len(b) - 3):
        v = struct.unpack_from('<I', b, i)[0]
        if 0x10000 <= v < 0x600000: m[i:i + 4] = [False] * 4
    return m
def find(va10, n=None, want_unique=True):
    n = n or int(xdis.ROWS[va10]['size'])
    b = read(V10, S10, va10, n); m = mask_of(b)
    # longest unmasked run as the anchor
    best, cur, start = (0, 0), 0, 0
    for i, ok in enumerate(m + [False]):
        if ok:
            if cur == 0: start = i
            cur += 1
            if cur > best[1]: best = (start, cur)
        else: cur = 0
    a0, alen = best
    anchor = b[a0:a0 + alen]
    hits = []
    for name, sva, blob in code15():
        j = blob.find(anchor)
        while j >= 0:
            s = j - a0
            if s >= 0 and s + n <= len(blob) and all(not m[k] or blob[s + k] == b[k] for k in range(n)): hits.append(sva + s)
            j = blob.find(anchor, j + 1)
    return hits, n, sum(m)
def value_at(va10, off_va10, va15):
    """the 32-bit value at the same offset in the v1.5 copy of a matched function"""
    o = off_va10 - va10
    return struct.unpack_from('<I', read(V15, S15, va15 + o, 4), 0)[0]
def call_target15(site15):
    b = read(V15, S15, site15, 5)
    return site15 + 5 + struct.unpack_from('<i', b, 1)[0] if b[0] == 0xE8 else None
