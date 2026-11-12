"""R89 = the RELEASE build: R88L's fixes without its logging.
R88L was R63 (the 128 MB emulator) plus, layered up over R64-R88: word patches, 31 bitmap sites, five routines that fix
things, and a render-target event logger (a ring, a surface table, counters, wrappers on 35 of the emulator's calls). R89
is built from the same R63 with the same word patches, the same bitmap sites and the same five fixes - and none of the
logger: its 35 call sites are the emulator's own again, its routines and its data are not in the file.
  kept:    AA64, CLASS65, BOUND69, WIDEN70 (with R78's, R82's, R85's and R87's words), the kernel's allocation cap (R71),
           the bitmap sites (R70), w_slot at 0x820C3F30 (R67/R85/R86), w_promote at 0x820C655C (R79),
           w_depth_match at 0x820C5E44 (R78), w_take at 0x820C62EC (R85)
  gone:    note / note_keep, w_setup, w_1058 (4 sites), w_66b8, w_5770, c_finish, c_2c28 (2), c_2ef8 (2), w_texalloc,
           w_texalloc2 / w_texalloc2b, w_2208 (15), w_1410, w_resolve (3), w_release, w_clear, w_bind, w_texsetup,
           w_slot_r67, aa_fit, and R76's guard w_evict_ok (uncalled since R88L)
usage: make_release_r89.py <stock xefu1_1.xex> <R63_xefu1_1.xex> <out dir>
"""
import os, sys, hashlib, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import make_builds_r22 as m
import make_release_r88l as r88          # (its module code builds the word-patch lists, the kernel's included, from argv)
b = r88.b

AT = b.AT
HOLD = b.DATA['CNT'] + 0x70               # the held clear's 32 bytes, where R88L kept them
DATA = dict(HOLD=HOLD, FRAMEC=b.DATA['FRAMEC'], TABLE=b.DATA['TABLE'], BITMAPS=b.DATA['BITMAPS'],
            PROMOTEQ=b.DATA['PROMOTEQ'], DEPTHFOUND=b.DATA['DEPTHFOUND'])
SITES = [(0x820C655C, 0x820C1AA8, 'w_promote')]
LOG_SITES = [(s, c, w) for s, c, w in b.SITES if w != 'w_promote']      # R88L's logger sites: left as R63 has them

def assemble():
    tmp = '/tmp/xfsg89'
    subprocess.run(['powerpc64-linux-gnu-as', '-a32', '-mbig', '-mppc64', '-o', tmp + '.o', os.path.join(HERE, 'stubs_xefu1_1_r89.S')], check=True)
    cmd = ['powerpc64-linux-gnu-ld', '-melf32ppc', '-e', '0', '-Ttext=0x%08X' % AT]
    for k, v in DATA.items():
        cmd += ['--defsym', '%s=0x%08X' % (k, v)]
    subprocess.run(cmd + ['-o', tmp + '.elf', tmp + '.o'], check=True)
    subprocess.run(['powerpc64-linux-gnu-objcopy', '-O', 'binary', '-j', '.text', tmp + '.elf', tmp + '.bin'], check=True)
    nm = subprocess.run(['powerpc64-linux-gnu-nm', tmp + '.elf'], capture_output=True, text=True).stdout
    syms = {t.split()[2]: int(t.split()[0], 16) for t in nm.splitlines() if len(t.split()) == 3}
    return open(tmp + '.bin', 'rb').read(), syms

def words(syms):
    """R88L's word patches, with the three that branch into the cave re-aimed at this build's routines"""
    out = []
    for a, old, new in b.AA64 + b.CLASS65 + b.BOUND69 + tuple(b.WIDEN70):
        if a == r88.DEPTH_SITE: new = m.bl(a, syms['w_depth_match'])
        elif a == 0x820C62EC: new = m.bl(a, syms['w_take'])
        out.append((a, old, new))
    return out

def build(stock, r63xex):
    img63 = m.Xex(r63xex).image()
    text, syms = assemble()
    im = m.Image(img63)
    assert AT + len(text) <= 0x82140000 and not any(im.b[AT - m.BASE:0x82140000 - m.BASE]), 'code cave not free'
    im.put_bytes(AT, text)
    for site, callee, wrap in SITES:
        w = im.w(site)
        assert m.bl_target(site, w) == callee, '%08X calls %08X' % (site, m.bl_target(site, w))
        im.put(site, w, m.bl(site, syms[wrap]))
    for site, callee, wrap in LOG_SITES:                       # untouched - but they must be what R88L's recipe expected to find
        assert m.bl_target(site, im.w(site)) == callee, '%08X calls %08X' % (site, m.bl_target(site, im.w(site)))
    W = words(syms)
    for a, old, new in W:
        im.put(a, old, new)
    for a, reg, k in b.BMSITES:
        w = im.w(a)
        off = (0x1988, 0x1A08, 0x1A88, 0x1B08)[k]
        assert (w >> 26) == 14 and ((w >> 21) & 31) == reg and (w & 0xFFFF) == off, '%08X: %08X is not addi r%d,rB,0x%X' % (a, w, reg, off)
        im.put(a, w, m.bl(a, syms['bm%d_r%d' % (k, reg)]))
    KCAP = 0x520000 + 0x8001D760 - 0x80010000                  # R71: the kernel's contiguous-allocation cap
    assert im.b[KCAP:KCAP + 4] == bytes.fromhex('df3f0000'), im.b[KCAP:KCAP + 4].hex()
    im.b[KCAP:KCAP + 4] = bytes.fromhex('af7f0000')
    SLOT = 0x820C3F30
    assert im.w(SLOT) == 0x7FFBD12E, hex(im.w(SLOT))            # stwx r31,r27,r26
    im.put(SLOT, 0x7FFBD12E, m.bl(SLOT, syms['w_slot']))
    new = bytes(im.b)
    diff = [m.BASE + i for i in range(0, len(img63), 4) if new[i:i + 4] != img63[i:i + 4]]
    other = [a for a in diff if not (AT <= a < 0x82140000) and a != m.BASE + (KCAP & ~3)]
    assert sorted(other) == sorted([s for s, c, w in SITES] + [a for a, o, n in W] + [a for a, r, k in b.BMSITES] + [SLOT]), ['%08X' % a for a in other]
    return new, text, syms, W

def main():
    stock, r63xex, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(outdir, exist_ok=True)
    new, text, syms, W = build(stock, r63xex)
    out, blocks = m.repack(stock, new)
    path = os.path.join(outdir, 'R89_xefu1_1.xex')
    open(path, 'wb').write(out)
    chk = m.verify(path, new)
    assert all(chk.values()), chk
    print('%s: sha1 %s, %s; vs R63: 1 call site + the slot + %d words + %d bitmap sites + %d B of code at %08X (R88L: %d sites, 4560 B); no logger' % (
        path, hashlib.sha1(out).hexdigest(), chk, len(W), len(b.BMSITES), len(text), AT, len(b.SITES)))

if __name__ == '__main__':
    main()
