"""xmap - carry addresses from the Halo 2 v1.0 XBE to the v1.5 one.
code(va): the same place in the code, by a masked match of the bytes around it (addresses and branch offsets masked).
data(addr): where a global lives, by the places the code names it (each matched, the operand read back; a vote).
slot(addr): a kernel import slot, by its ordinal."""
import struct, collections
def load(p):
    d = open(p, 'rb').read()
    base = struct.unpack_from('<I', d, 0x104)[0]; ns, sa = struct.unpack_from('<II', d, 0x11C)
    secs = []
    for i in range(ns):
        o = sa - base + 56 * i
        fl, va, vs, raw, rs, na = struct.unpack_from('<6I', d, o)
        secs.append(dict(name=d[na - base:d.index(b'\0', na - base)].decode(), va=va, vs=vs, raw=raw, rs=rs, fl=fl))
    return d, secs
D10, S10 = load('/home/claude/w/stock/default.xbe')
D15, S15 = load('/home/claude/w/stock15/default.xbe')
def sec_of(S, va):
    for s in S:
        if s['va'] <= va < s['va'] + s['rs']: return s
def rd(D, S, va, n):
    s = sec_of(S, va)
    if not s or va + n > s['va'] + s['rs']: return None
    return D[s['raw'] + va - s['va']: s['raw'] + va - s['va'] + n]
CODE = lambda S: [s for s in S if s['fl'] & 4]
LO, HI = 0x10000, 0x700000
def mask(b):
    m = bytearray(b'\1' * len(b))
    for i in range(len(b) - 3):
        v = struct.unpack_from('<I', b, i)[0]
        if LO <= v < HI: m[i:i + 4] = b'\0\0\0\0'
    for i in range(len(b) - 4):
        if b[i] in (0xE8, 0xE9): m[i + 1:i + 5] = b'\0\0\0\0'
        if b[i] == 0x0F and 0x80 <= b[i + 1] <= 0x8F and i + 6 <= len(b): m[i + 2:i + 6] = b'\0\0\0\0'
    return m
_blob15 = [(s['va'], D15[s['raw']:s['raw'] + s['rs']]) for s in CODE(S15)]
def find_masked(b, m):
    # anchor: the longest unmasked run
    best = (0, 0); cur = 0; st = 0
    for i in range(len(m) + 1):
        if i < len(m) and m[i]:
            if cur == 0: st = i
            cur += 1
            if cur > best[1]: best = (st, cur)
        else: cur = 0
    a0, al = best
    if al < 4: return []
    anchor = bytes(b[a0:a0 + al]); hits = []
    for va, blob in _blob15:
        j = blob.find(anchor)
        while j >= 0:
            s = j - a0
            if s >= 0 and s + len(b) <= len(blob):
                ok = True
                for k in range(len(b)):
                    if m[k] and blob[s + k] != b[k]: ok = False; break
                if ok: hits.append(va + s)
            j = blob.find(anchor, j + 1)
    return hits
def code(va, extra_mask=()):
    """va in v1.0 code -> (va in v1.5, window used) or (None, why)"""
    s = sec_of(S10, va)
    if not s: return None, 'not in a section'
    last = None
    for before, after in ((16, 32), (32, 48), (48, 80), (80, 120), (120, 200), (200, 320), (8, 24), (0, 32), (32, 0), (0, 64), (64, 0), (0, 128), (128, 0)):
        lo = max(s['va'], va - before); hi = min(s['va'] + s['rs'], va + after)
        b = rd(D10, S10, lo, hi - lo); m = mask(b)
        for (o, n) in extra_mask:
            for k in range(n):
                if 0 <= va + o + k - lo < len(m): m[va + o + k - lo] = 0
        hits = find_masked(b, m)
        if len(hits) == 1: return hits[0] + (va - lo), (before, after)
        last = len(hits)
    return None, 'matches: %s' % last
def data(addr, span=0):
    """a global's address -> (v1.5 address, votes, total refs)"""
    votes = collections.Counter(); total = 0
    pat = struct.pack('<I', addr)
    for s in CODE(S10):
        blob = D10[s['raw']:s['raw'] + s['rs']]; j = blob.find(pat)
        while j >= 0 and total < 40:
            total += 1
            va = s['va'] + j
            r, w = code(va, extra_mask=((0, 4),))
            if r is not None:
                v = rd(D15, S15, r, 4)
                if v: votes[struct.unpack('<I', v)[0]] += 1
            j = blob.find(pat, j + 1)
    if not votes: return None, 0, total
    best, n = votes.most_common(1)[0]
    return best, n, total, dict(votes)
def thunks(D, S):
    # kernel thunk table: header 0x158 xor key (retail 0x5B6D40B6)
    t = struct.unpack_from('<I', D, 0x158)[0] ^ 0x5B6D40B6
    out = {}; a = t
    while True:
        b = rd(D, S, a, 4)
        if not b: break
        v = struct.unpack('<I', b)[0]
        if not v: break
        out[a] = v & 0x7FFFFFFF; a += 4
    return out
T10, T15 = thunks(D10, S10), thunks(D15, S15)
def slot(addr):
    o = T10.get(addr)
    if o is None: return None, 'not a thunk slot'
    c = [a for a, v in T15.items() if v == o]
    return (c[0], o) if len(c) == 1 else (None, 'ordinal %d: %d slots' % (o, len(c)))
def target(site, D=D15, S=S15):
    b = rd(D, S, site, 5)
    return site + 5 + struct.unpack_from('<i', b, 1)[0] if b and b[0] in (0xE8, 0xE9) else None
