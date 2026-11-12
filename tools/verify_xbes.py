#!/usr/bin/env python3
"""Verify patched Spooky Ghost XBEs the way every step is checked before packaging.

Run from the source folder (it imports patch_xbe and calls unpatch.py there):
    python3 verify_xbes.py <step label, e.g. 6i.89> <stock dir with default.xbe / default_1.xbe> <out dir> [names...]
names default to v10 v10tu v15 (out/<name>.xbe); pass just v10tu when only the 1.0 TU Skip build is made.
Checks per XBE: every section's SHA-1 digest, the step label in the mod's section, every hooked call site points at
its hook (from build_vXX/spooky.map), every symref, the triple-buffer region and flag, and unpatch.py gives the stock
XBE back byte for byte. Prints one line per XBE; exits 1 on any failure.
"""
import struct, hashlib, sys, subprocess, os
sys.path.insert(0, os.getcwd())
import patch_xbe

def secs(P):
    base = 0x10000; nsec, secaddr = struct.unpack_from('<II', P, 0x11C); out = []
    for i in range(nsec):
        o = secaddr - base + 56 * i; fl, va, vsz, raw, rsz = struct.unpack_from('<5I', P, o); out.append((o, va, vsz, raw, rsz))
    return out

def va2off(P, va):
    for o, sva, vsz, raw, rsz in secs(P):
        if sva <= va < sva + rsz: return raw + va - sva

KINDS = {'v10': ('v1.0', 'default', 'v10'), 'v10tu': ('v1.0', 'default', 'v10'), 'v15': ('v1.5', 'default_1', 'v15')}

def main():
    if len(sys.argv) < 4: print(__doc__); sys.exit(2)
    step, stock, outd = sys.argv[1], sys.argv[2], sys.argv[3]
    names = sys.argv[4:] or ['v10', 'v10tu', 'v15']
    bad_any = False
    for f in names:
        ver, st, b = KINDS[f]
        P = open(os.path.join(outd, f + '.xbe'), 'rb').read(); S = open(os.path.join(stock, st + '.xbe'), 'rb').read()
        ss = secs(P); ok = sum(hashlib.sha1(struct.pack('<I', rsz) + P[raw:raw + rsz]).digest() == P[o + 36:o + 56] for o, va, vsz, raw, rsz in ss)
        last = ss[-1]; label = (('Spooky Ghost Menu ' if step.startswith('v') else 'Spooky Ghost step ') + step).encode() in P[last[3]:last[3] + last[4]]
        mp = {l.split()[2]: int(l.split()[0], 16) for l in open('build_%s/spooky.map' % b) if len(l.split()) == 3}
        cfg = patch_xbe.VERSIONS[ver]
        bad = [hex(site) for site, target, hook in cfg['calls'] if P[va2off(P, site)] not in (0xE8, 0xE9) or site + 5 + struct.unpack_from('<i', P, va2off(P, site) + 1)[0] != mp[hook]]
        badref = [hex(site) for site, want, sym in cfg.get('symrefs', ()) if struct.unpack_from('<I', P, va2off(P, site))[0] != mp[sym]]
        trip = cfg['triple'][0]; toff = va2off(P, trip[0])
        tri_ok = P[toff:toff + len(bytes.fromhex(trip[2]))] == bytes.fromhex(trip[2]) and P[va2off(P, mp['spooky_triple'])] == 1
        subprocess.run(['python3', 'unpatch.py', ver, os.path.join(outd, f + '.xbe'), '/tmp/_un.xbe'], capture_output=True)
        a = open('/tmp/_un.xbe', 'rb').read()
        un_ok = S[:len(a)] == a and not any(S[len(a):])
        good = ok == len(ss) and label and not bad and not badref and tri_ok and un_ok
        bad_any |= not good
        print(f, '%d/%d digests' % (ok, len(ss)), 'label', label, 'calls wrong', bad, 'symrefs wrong', badref, 'triple', tri_ok,
              'unpatch == stock' if un_ok else 'UNPATCH MISMATCH', 'OK' if good else 'FAILED')
    sys.exit(1 if bad_any else 0)

if __name__ == '__main__':
    main()
