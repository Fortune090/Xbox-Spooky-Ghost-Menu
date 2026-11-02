#!/usr/bin/env python3
"""Patch Halo 2's XBE (original Xbox, title 4D530064) with the Spooky Ghost section - v1.0 or the v1.5 title update.

    python3 patch_xbe.py <v1.0|v1.5> <original default.xbe> <spooky.bin> <spooky.map> <output default.xbe>

Changes, and nothing else: appends the section ".spooky"; copies the section table byte for byte into unused
zero space in the header area with a 34th entry; points the image header at it and extends the image size;
redirects six calls in .text (every original call target is verified first);
recomputes the digest of every section it touched. v1.0 also gets the dashboard title "Halo 2 - Spooky Ghost".
The RSA signature is not re-signed (a softmodded console does not check it). Refuses any unexpected input.
"""
import hashlib, struct, sys

BASE = 0x10000
VERSIONS = {
    'v1.0': dict(timestamp=0x4158CC50, va=0x586000, title='Halo 2 - Spooky Ghost',
                 # --tu-skip (the "1.0 TU Skip" build): XAPI's startup relaunch into T:\$u (0x2d129f) and Xbox Live's
                 # update.xbe launch (XONLINE 0x3b056f) return at once
                 tu_skip=((0x2d129f, '558bec', '33c0c3'), (0x3b056f, '558bec81ec', '33c0c20400')),
                 bytes=((0xca139, '0f8534010000', '909090909090'), (0xc683f, '7546', '9090'), (0x203757, '7510', '9090'), (0xfda52, '751d', '9090'), (0x1e2ed4, '751e', '9090'), (0xc9869, '750a', '9090'), (0xd5e33, '751c', '9090'), (0xd976e, '7537', '9090'), (0x1ae51d, '750a', '9090'), (0x1ae76a, '750a', '9090'), (0x1ae7af, '7517', '9090'), (0x1e2d59, '7509', '9090'), (0x1fda13, '7552', '9090'), (0x25cc4b, '7511', '9090'), (0xd9921, '7517', '9090'), (0x1393dc, '0f95c0', 'b00090'), (0x165f2c, '0f95c0', 'b00090'),),
                 calls=((0x2c86e, 0x13c780, 'hud_hook'), (0x124c34, 0x409b6c, 'input_hook'),
                        (0x2b6f7, 0x13cd0, 'end_a_hook'), (0x2c9e9, 0x13cd0, 'end_b_hook'),
                        (0x12b6b0, 0x12b450, 'main_hook'), (0x2b6e4, 0x2c790, 'hud_view_hook'),
                        (0x2235f0, 0x223080, 'tp_view_hook'), (0x2badc, 0x3d2c0, 'tp_fp_hook'), (0x2bc93, 0x1917eb, 'hudd_hook'), (0xd86f8, 0xdaaa0, 'rdmg_hook'), (0xd8aa9, 0xdaaa0, 'rdmg_hook'), (0xdb0ef, 0xdaaa0, 'rdmg_hook'), (0x2c721, 0x3d2c0, 'tp_fp_hook'),
                        (0xa2de1, 0x102fb0, 'bc_fire_hook'), (0xff1bd, 0x102fb0, 'bc_fire_hook'),
                        (0xcd96e, 0x106030, 'ia_hook'), (0x10091f, 0x106030, 'ia_hook'), (0x100bce, 0x106030, 'ia_hook'),
                        (0x1025f1, 0x106030, 'ia_hook'), (0x103155, 0x106030, 'ia_hook'),
                        (0xc2dac, 0x16640f, 'fpm_hook'),
                        (0x2b9b2, 0x155760, 'persp_hook'), (0x2c1e8, 0x155760, 'persp_hook'), (0x3d323, 0x155760, 'persp_hook'), (0x3e9d6, 0x155760, 'persp_hook'), (0x1662f2, 0x155760, 'persp_hook'), (0x166cfe, 0x155760, 'persp_hook'), (0x1670cb, 0x155760, 'persp_hook'), (0x16ec24, 0x155760, 'persp_hook'), (0x16ec71, 0x155760, 'persp_hook'),
                        # 6h: the HUD Widescreen Fix
                        (0x22ab17, 0x1396c7, 'hanc_hook'), (0x22b050, 0x1396c7, 'hanc_hook'), (0x22b9e2, 0x1396c7, 'hanc_hook'), (0x20056e, 0x1396c7, 'hanc_hook'), (0x13a5ca, 0x22b8e2, 'hdraw_grid'), (0x13a625, 0x22aff1, 'hdraw_bmp'), (0x13a67c, 0x22aa16, 'hdraw_text'), (0x139693, 0x200462, 'hcentre_hook'), (0x1e936a, 0x22a664, 'hquad_hook'), (0x1e93b1, 0x22a664, 'hquad_hook'), (0x1e9400, 0x22a664, 'hquad_hook'), (0x1e944f, 0x22a664, 'hquad_hook'), (0x1e949e, 0x22a664, 'hquad_hook'), (0x1e94e4, 0x22a664, 'hquad_hook'), (0x1e9532, 0x22a664, 'hquad_hook'), (0x1e9574, 0x22a664, 'hquad_hook'), (0x1e95b6, 0x22a664, 'hquad_hook'), (0x20077f, 0x22a664, 'hquad_hook'), (0x22b397, 0x22a664, 'hquad_hook'), (0x22b55c, 0x22a664, 'hquad_hook'), (0x22b690, 0x22a664, 'hquad_hook'), (0x22b6fc, 0x22a664, 'hquad_hook'), (0x22b74f, 0x22a664, 'hquad_hook'), (0x22b7ce, 0x22a664, 'hquad_hook'), (0x22bca4, 0x22a664, 'hquad_hook'), (0x24eca0, 0x22a664, 'hquad_hook'), (0x24f0b6, 0x22a664, 'hquad_hook'), (0x24f357, 0x22a664, 'hquad_hook'), (0x24f3de, 0x22a664, 'hquad_hook'), (0x24f432, 0x22a664, 'hquad_hook'), (0x24f486, 0x22a664, 'hquad_hook'), (0x24f4fb, 0x22a664, 'hquad_hook'), (0x24f556, 0x22a664, 'hquad_hook'), (0x2b12ae, 0x22a664, 'hquad_hook'), (0x22bbbb, 0x2d140, 'hgrid_hook'), (0x22aca8, 0x1fb10, 'htext_hook'), (0x139685, 0x1a2823, 'htrack_hook'), (0x2550c8, 0x3fb2d0, 'htrkv_hook'), (0x255108, 0x3fb2d0, 'htrkv_hook'), (0x255147, 0x3fb2d0, 'htrkv_hook'), (0x255187, 0x3fb2d0, 'htrkv_hook'),
                        # 6h.11: a possessed vehicle in third person
                        (0x15596b, 0x155810, 'hcam_mode_hook'), (0x155abc, 0x155810, 'hcam_mode_hook'), (0x1557d1, 0x155810, 'hcam_mode_hook'),
                        # 6h.12: Checkpoint Settings; the possessed vehicle's driving view
                        (0x22613d, 0x12b790, 'hsave_hook'), (0x226090, 0x12b790, 'hsave_hook'), (0x138082, 0x138e40, 'hrevert_hook'), (0x18755b, 0x1871e0, 'htarget_hook'),
                        # 6h.23: a moved view (third person / free camera) - the effect draw pass's first-person marker; the object
                        # attachments' first-person tests (draw-time visibility, instances made, their markers' mode)
                        (0x31d6a, 0x16640f, 'fpm_hook'), (0x17430c, 0x155760, 'persp_hook'), (0x178f06, 0x155760, 'persp_hook'),
                        (0x179071, 0x155760, 'persp_hook'), (0x179942, 0x155760, 'persp_hook'),
                        # 6h.25: smooth slow motion - a snapshot before each tick, a blend around the draw
                        (0x137f89, 0x137fe0, 'ip_tick_hook'), (0x12b659, 0x223240, 'ip_draw_hook'), (0x23cbc0, 0x1871e0, 'htarget_hook'), (0x23d7a0, 0x1871e0, 'htarget_hook'))),
    'v1.5': dict(timestamp=0x4600A62B, va=0x596000, title=None, bytes=((0xcc619, '0f8534010000', '909090909090'), (0xc8d1f, '7546', '9090'), (0x20ba17, '7510', '9090'), (0x102a52, '751d', '9090'), (0x1eb184, '751e', '9090'), (0xcbd49, '750a', '9090'), (0xd8383, '751c', '9090'), (0xdbcbe, '7537', '9090'), (0x1b70dd, '750a', '9090'), (0x1b732a, '750a', '9090'), (0x1b736f, '7517', '9090'), (0x1eb009, '7509', '9090'), (0x205cd3, '7552', '9090'), (0x26695b, '7511', '9090'), (0xdbe71, '7517', '9090'), (0x13bebc, '0f95c0', 'b00090'), (0x168e03, '0f95c0', 'b00090'),),
                 calls=((0x2c84e, 0x13f260, 'hud_hook'), (0x127744, 0x413c4c, 'input_hook'),
                        (0x2b6d7, 0x13cb0, 'end_a_hook'), (0x2c9c9, 0x13cb0, 'end_b_hook'),
                        (0x12e1c0, 0x12df60, 'main_hook'), (0x2b6c4, 0x2c770, 'hud_view_hook'),
                        (0x22a000, 0x229a90, 'tp_view_hook'), (0x2babc, 0x3d2b0, 'tp_fp_hook'), (0x2bc73, 0x1977ab, 'hudd_hook'), (0xdac48, 0xdcff0, 'rdmg_hook'), (0xdaff9, 0xdcff0, 'rdmg_hook'), (0xdd63f, 0xdcff0, 'rdmg_hook'), (0x2c701, 0x3d2b0, 'tp_fp_hook'),
                        (0xacec1, 0x108080, 'bc_fire_hook'), (0x104304, 0x108080, 'bc_fire_hook'),
                        (0xcfe4e, 0x10b0f0, 'ia_hook'), (0x1059ef, 0x10b0f0, 'ia_hook'), (0x105c9e, 0x10b0f0, 'ia_hook'),
                        (0x1076c1, 0x10b0f0, 'ia_hook'), (0x108225, 0x10b0f0, 'ia_hook'),
                        (0xc4ddc, 0x1692e0, 'fpm_hook'),
                        (0x2b992, 0x158290, 'persp_hook'), (0x2c1c8, 0x158290, 'persp_hook'), (0x3d313, 0x158290, 'persp_hook'), (0x3e9c6, 0x158290, 'persp_hook'), (0x1691c1, 0x158290, 'persp_hook'), (0x169bcf, 0x158290, 'persp_hook'), (0x169f9c, 0x158290, 'persp_hook'), (0x171a84, 0x158290, 'persp_hook'), (0x171ad1, 0x158290, 'persp_hook'),
                        # 6h: the HUD Widescreen Fix
                        (0x231517, 0x13c1a7, 'hanc_hook'), (0x231a50, 0x13c1a7, 'hanc_hook'), (0x2323e2, 0x13c1a7, 'hanc_hook'), (0x20882e, 0x13c1a7, 'hanc_hook'), (0x13d0aa, 0x2322e2, 'hdraw_grid'), (0x13d105, 0x2319f1, 'hdraw_bmp'), (0x13d15c, 0x231416, 'hdraw_text'), (0x13c173, 0x208722, 'hcentre_hook'), (0x1f161a, 0x231064, 'hquad_hook'), (0x1f1661, 0x231064, 'hquad_hook'), (0x1f16b0, 0x231064, 'hquad_hook'), (0x1f16ff, 0x231064, 'hquad_hook'), (0x1f174e, 0x231064, 'hquad_hook'), (0x1f1794, 0x231064, 'hquad_hook'), (0x1f17e2, 0x231064, 'hquad_hook'), (0x1f1824, 0x231064, 'hquad_hook'), (0x1f1866, 0x231064, 'hquad_hook'), (0x208a3f, 0x231064, 'hquad_hook'), (0x231d97, 0x231064, 'hquad_hook'), (0x231f5c, 0x231064, 'hquad_hook'), (0x232090, 0x231064, 'hquad_hook'), (0x2320fc, 0x231064, 'hquad_hook'), (0x23214f, 0x231064, 'hquad_hook'), (0x2321ce, 0x231064, 'hquad_hook'), (0x2326a4, 0x231064, 'hquad_hook'), (0x25cbd0, 0x231064, 'hquad_hook'), (0x25cfe6, 0x231064, 'hquad_hook'), (0x25d287, 0x231064, 'hquad_hook'), (0x25d30e, 0x231064, 'hquad_hook'), (0x25d362, 0x231064, 'hquad_hook'), (0x25d3b6, 0x231064, 'hquad_hook'), (0x25d42b, 0x231064, 'hquad_hook'), (0x25d486, 0x231064, 'hquad_hook'), (0x2bafbe, 0x231064, 'hquad_hook'), (0x2325bb, 0x2d120, 'hgrid_hook'), (0x2316a8, 0x1faf0, 'htext_hook'), (0x13c165, 0x1ac453, 'htrack_hook'), (0x25ed98, 0x4053b0, 'htrkv_hook'), (0x25edd8, 0x4053b0, 'htrkv_hook'), (0x25ee17, 0x4053b0, 'htrkv_hook'), (0x25ee57, 0x4053b0, 'htrkv_hook'),
                        # 6h.11: a possessed vehicle in third person
                        (0x15849b, 0x158340, 'hcam_mode_hook'), (0x1585ec, 0x158340, 'hcam_mode_hook'), (0x158301, 0x158340, 'hcam_mode_hook'),
                        # 6h.12: Checkpoint Settings; the possessed vehicle's driving view
                        (0x22cb3d, 0x12e2a0, 'hsave_hook'), (0x22ca90, 0x12e2a0, 'hsave_hook'), (0x13ab62, 0x13b920, 'hrevert_hook'), (0x18a34b, 0x189fd0, 'htarget_hook'),
                        # 6h.23: as v1.0's
                        (0x31d5a, 0x1692e0, 'fpm_hook'), (0x1771ac, 0x158290, 'persp_hook'), (0x17bd96, 0x158290, 'persp_hook'),
                        (0x17bf01, 0x158290, 'persp_hook'), (0x17c7d2, 0x158290, 'persp_hook'),
                        # 6h.25: as v1.0's
                        (0x13aa69, 0x13aac0, 'ip_tick_hook'), (0x12e169, 0x229c50, 'ip_draw_hook'), (0x245eb0, 0x189fd0, 'htarget_hook'), (0x246a90, 0x189fd0, 'htarget_hook'))),
}
NEW_NAME = b'.spooky\0'
FILLER = bytes.fromhex('8d9b00000000')          # lea ebx,[ebx+0]: does nothing


def die(msg):
    sys.exit('patch_xbe: ' + msg)


HUD_HOOKS = ('hanc_hook', 'hdraw_grid', 'hdraw_bmp', 'hdraw_text', 'hcentre_hook', 'hquad_hook', 'hgrid_hook', 'htext_hook', 'htrack_hook', 'htrkv_hook')


def main(version, src, binpath, mappath, out, *opts):
    cfg = VERSIONS.get(version) or die('version must be v1.0 or v1.5')
    if '--tu-skip' in opts:                # v1.0: the title update checks skipped (the "1.0 TU Skip" build)
        if not cfg.get('tu_skip'): die('--tu-skip: only for v1.0')
        cfg = dict(cfg); cfg['bytes'] = tuple(cfg['bytes']) + tuple(cfg['tu_skip'])
    if '--no-hud' in opts:                 # (testing) the HUD Widescreen Fix's call sites left as the game's
        cfg = dict(cfg, calls=tuple(c for c in cfg['calls'] if c[2] not in HUD_HOOKS))
    x = bytearray(open(src, 'rb').read())
    code = open(binpath, 'rb').read()
    syms = {p[2]: int(p[0], 16) for p in (l.split() for l in open(mappath)) if len(p) == 3}
    if x[:4] != b'XBEH': die('not an XBE')
    base, hdr_size, img_size, img_hdr_size, ts, cert, nsec, secaddr = struct.unpack_from('<8I', x, 0x104)
    if base != BASE or hdr_size != 0x2000: die('unexpected image base / header size')
    if struct.unpack_from('<I', x, cert - BASE + 8)[0] != 0x4D530064: die('not Halo 2')
    if ts != cfg['timestamp']: die('image timestamp %08X is not Halo 2 %s (%08X)' % (ts, version, cfg['timestamp']))
    if nsec != 33: die('unexpected section count %d (already patched?)' % nsec)
    if syms.get('__section_start') != cfg['va']: die('spooky.bin was not linked at 0x%X' % cfg['va'])
    vsize = syms['__section_end'] - cfg['va']
    if len(code) != vsize: die('spooky.bin size does not match its map')

    secs = []
    for i in range(nsec):
        o = secaddr - BASE + 56 * i
        fl, va, vs, raw, rs, na, rc, hr, tr = struct.unpack_from('<9I', x, o)
        secs.append(dict(hdr=bytes(x[o:o + 56]), flags=fl, va=va, vsize=vs, raw=raw, rsize=rs, name=x[na - BASE:].split(b'\0')[0].decode(),
                         refs=(hr - BASE, tr - BASE), name_off=na - BASE))
    if max(s['va'] + s['vsize'] for s in secs) > cfg['va']: die('section address not free')
    data_flags = next(s['flags'] for s in secs if s['name'] == '.data')

    # free zero space in the header area: after everything the headers use, before the embedded PE header
    used_end = max([0x178, cert - BASE + struct.unpack_from('<I', x, cert - BASE)[0], secaddr - BASE + 56 * nsec] +
                   [s['name_off'] + len(s['name']) + 1 for s in secs] + [r + 2 for s in secs for r in s['refs']])
    for field in (0x14C, 0x150, 0x154):
        a = struct.unpack_from('<I', x, field)[0]
        if a: used_end = max(used_end, a - BASE + 128)
    nlib, libaddr = struct.unpack_from('<II', x, 0x160); used_end = max(used_end, libaddr - BASE + 16 * nlib)
    logo, logosz = struct.unpack_from('<II', x, 0x170); used_end = max(used_end, logo - BASE + logosz)
    pe = x.find(b'MZ', used_end, 0x2000); limit = pe if pe > 0 else 0x2000
    table_off = (used_end + 3) & ~3
    need = 56 * (nsec + 1) + len(NEW_NAME) + 4
    if table_off + need > limit or any(x[table_off:table_off + need]): die('no free zero space in the header area')

    def file_off(va):
        for i, s in enumerate(secs):
            if s['va'] <= va < s['va'] + s['rsize']: return i, s['raw'] + va - s['va']
        die('0x%X is not in a section' % va)
    touched = set()
    for site, target, hook in cfg['calls']:
        i, off = file_off(site)
        if secs[i]['name'] != '.text' or x[off] not in (0xE8, 0xE9): die('0x%X is not a call or jump in .text' % site)
        cur = site + 5 + struct.unpack_from('<i', x, off + 1)[0]
        if cur != target: die('0x%X calls 0x%X, expected 0x%X' % (site, cur, target))
    # the address operands pointed at the mod's own data (step 6g.1: the HUD Scaling Fix's factor): each must hold the address expected
    for site, want, sym in cfg.get('symrefs', ()):
        i, off = file_off(site)
        if secs[i]['name'] != '.text' or struct.unpack_from('<I', x, off)[0] != want:
            die('0x%X holds 0x%X, expected 0x%X' % (site, struct.unpack_from('<I', x, off)[0], want))
        if sym not in syms: die('no symbol %s in the map' % sym)
    # the byte patches (the skulls' campaign-only tests, step 6e; v1.0's title update checks, 6h.10: XAPI's startup relaunch
    # into T:\$u (0x2d129f) and Xbox Live's update.xbe launch (XONLINE 0x3b056f) return at once): each must find exactly the bytes expected
    for site, want, new in cfg.get('bytes', ()):
        i, off = file_off(site)
        a, b = bytes.fromhex(want), bytes.fromhex(new)
        if secs[i]['name'] not in ('.text', 'XONLINE') or bytes(x[off:off + len(a)]) != a or len(a) != len(b):
            die('0x%X holds %s, expected %s' % (site, bytes(x[off:off + len(a)]).hex(), want))

    # append the section's data, page-aligned in the file
    raw_addr = (len(x) + 0xFFF) & ~0xFFF
    x += b'\0' * (raw_addr - len(x)) + code
    x += b'\0' * (((len(x) + 0xFFF) & ~0xFFF) - len(x))
    for site, target, hook in cfg['calls']:
        i, off = file_off(site); struct.pack_into('<i', x, off + 1, syms[hook] - (site + 5)); touched.add(i)
    for site, want, new in cfg.get('bytes', ()):
        i, off = file_off(site); b = bytes.fromhex(new); x[off:off + len(b)] = b; touched.add(i)
    for site, want, sym in cfg.get('symrefs', ()):
        i, off = file_off(site); struct.pack_into('<I', x, off, syms[sym]); touched.add(i)

    if cfg['title']:
        c = cert - BASE
        x[c + 0xC:c + 0xC + 80] = cfg['title'].encode('utf-16le').ljust(80, b'\0')

    # the new table: the 33 originals byte for byte (touched sections with their new digests), then .spooky
    name_off = table_off + 56 * (nsec + 1); refs_off = name_off + len(NEW_NAME)
    table = bytearray()
    for i, s in enumerate(secs):
        h = bytearray(s['hdr'])
        if i in touched:
            h[36:56] = hashlib.sha1(struct.pack('<I', s['rsize']) + bytes(x[s['raw']:s['raw'] + s['rsize']])).digest()
            struct.pack_into('20s', x, secaddr - BASE + 56 * i + 36, bytes(h[36:56]))      # the old table stays valid too
        table += h
    new = bytearray(56)
    struct.pack_into('<9I', new, 0, data_flags, cfg['va'], vsize, raw_addr, len(code), BASE + name_off, 0, BASE + refs_off, BASE + refs_off + 2)
    new[36:56] = hashlib.sha1(struct.pack('<I', len(code)) + code).digest()
    table += new
    x[table_off:table_off + len(table)] = table
    x[name_off:name_off + len(NEW_NAME)] = NEW_NAME
    x[refs_off:refs_off + 4] = b'\0' * 4
    struct.pack_into('<I', x, 0x11C, nsec + 1)
    struct.pack_into('<I', x, 0x120, BASE + table_off)
    struct.pack_into('<I', x, 0x10C, cfg['va'] + vsize - BASE)
    open(out, 'wb').write(x)
    print('%s patched: .spooky at 0x%X (%d bytes, file 0x%X); table at 0x%X; %s%s'
          % (version, cfg['va'], len(code), raw_addr, table_off,
             ', '.join('0x%X -> %s 0x%X' % (s, h, syms[h]) for s, t, h in cfg['calls']),
             '; title "%s"' % cfg['title'] if cfg['title'] else ''))


if __name__ == '__main__':
    if len(sys.argv) not in (6, 7): sys.exit(__doc__)
    main(*sys.argv[1:])
