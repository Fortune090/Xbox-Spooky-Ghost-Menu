#!/usr/bin/env python3
"""Build "Spooky Ghost Patcher.html": a one-file app that players open in a browser, give their own original Halo 2
default.xbe, and press a button for the build they want. It does what patch_xbe.py does, in the page: every hooked call
site and patched byte is checked against the original first, the mod's section is appended, the section table extended
and every touched section's digest recomputed. The app holds only the mod (its section, zero runs left out) and the patch
lists - none of the game's code.

    python3 make_patcher_app.py <build tag> <output .html> <release xefu1_1.xex> <original xefu1_1.xex> [<another original> ...]
                                                                  (after ./build.sh)
v2.0: the page also makes the Xbox 360 emulator's xefu1_1.xex (the "360" tick box in Step 1). It is given the release
build and every original it is to accept; it holds only the bytes of the program the release changes and the header
fields that follow, and its result is checked against the release's SHA-1 before it is handed over. The emulator's part
of the page's code is patcher_emu_core.js, beside this script. Needs xex.py (the emulator tools) to read the files.
"""
import base64, hashlib, json, os, struct, sys
import patch_xbe
HERE = os.path.dirname(os.path.abspath(__file__))

MIN_RUN = 64        # zero runs at least this long are left out of the embedded section


def segments(code):
    segs, i, n = [], 0, len(code)
    while i < n:
        if code[i] == 0:
            j = i
            while j < n and code[j] == 0: j += 1
            if j - i >= MIN_RUN: i = j; continue
        j = i
        while j < n:
            if code[j] == 0:
                k = j
                while k < n and code[k] == 0: k += 1
                if k - j >= MIN_RUN: break
                j = k
            else: j += 1
        segs.append([i, base64.b64encode(code[i:j]).decode()])
        i = j
    return segs


def version_data(version, tag):
    cfg = patch_xbe.VERSIONS[version]
    d = 'build_v10' if version == 'v1.0' else 'build_v15'
    code = open(d + '/spooky.bin', 'rb').read()
    syms = {p[2]: int(p[0], 16) for p in (l.split() for l in open(d + '/spooky.map')) if len(p) == 3}
    assert syms['__section_start'] == cfg['va'] and syms['__section_end'] - cfg['va'] == len(code), version
    for _, _, hook in cfg['calls']: assert hook in syms, hook
    # v2.0: the zero-initialised variables are not written to the file (patch_xbe.py, 6i.150): the section's file size stops
    # at __bss_start and the console zero-fills up to its size in memory. The app holds, and appends, the file part only.
    raw = syms.get('__bss_start', cfg['va'] + len(code)) - cfg['va']
    assert 0 < raw <= len(code) and not any(code[raw:]), version
    assert cfg.get('x360'), version                      # v2.0: the 128 MB changes are in every build, v1.5 included
    out = dict(timestamp=cfg['timestamp'], va=cfg['va'], size=len(code), raw=raw, title=cfg['title'] or '',
               calls=[[s, t, syms[h]] for s, t, h in cfg['calls']],
               # 6i.50: triple buffering in every build; v2.0: and the 128 MB changes (patch_xbe.py --x360) - one XBE for
               # the Xbox and the 360's emulator
               bytes=[[s, w, n] for s, w, n in tuple(cfg.get('bytes', ())) + tuple(cfg.get('x360', ())) + tuple(cfg.get('triple', ()))],
               init_flags=[0x80000005, 0x80000001],     # header 0x124: the stock flags, and with "limit to 64 MB" (bit 2) cleared
               tu_skip=[[s, w, n] for s, w, n in cfg.get('tu_skip', ())],
               symrefs=[[s, w, syms[y]] for s, w, y in cfg.get('symrefs', ())],
               segs=segments(code[:raw]))
    # the segments put back together must be the section's file part exactly
    back = bytearray(raw)
    for o, b in out['segs']:
        part = base64.b64decode(b); back[o:o + len(part)] = part
    assert bytes(back) == code[:raw]
    return out


def emu_data(release, originals):
    """what the page needs to turn an original xefu1_1.xex into the release one: the release's program where it differs
    from ANY of the originals, its header where it differs from any of theirs, its block layout and its hashes"""
    for d in (HERE, os.path.join(HERE, 'emulator_tools'), os.path.join(HERE, '..', 'emulator_tools'), '/home/claude/h5/emulator/tools'):
        if os.path.exists(os.path.join(d, 'xex.py')): sys.path.insert(0, d); break
    import xex
    r = xex.Xex(release); rraw = open(release, 'rb').read(); rimg = bytes(r.image())
    ffi = r.hdr_data(0x3FF)
    assert struct.unpack_from('>HH', ffi, 4) == (0, 1), 'the release must be unencrypted, in the basic form'
    blocks = [list(struct.unpack_from('>II', ffi, 8 + 8 * i)) for i in range((len(ffi) - 8) // 8)]
    assert sum(a + b for a, b in blocks) == len(rimg) and r.pe_off + sum(a for a, b in blocks) == len(rraw)
    imgs, hdrs, inputs = [], [], {}
    for p in originals:
        s = xex.Xex(p); simg = bytes(s.image()); sraw = open(p, 'rb').read()
        assert s.pe_off == r.pe_off and len(simg) == len(rimg), p
        sha = hashlib.sha1(simg).hexdigest()
        five = [simg[a - 0x82000000] for a in (0x82520BA5, 0x8252DB14, 0x8252DB60, 0x8252DC38, 0x8252DCFE)]
        inputs[sha] = 'the original' if five == [0x3F] * 5 else 'the original with the 128 MB change' if five == [0x7F] * 5 else 'an original'
        imgs.append(simg); hdrs.append(sraw[:r.pe_off])
    def segs(new, olds):
        out, i, n = [], 0, len(new)
        while i < n:
            if any(o[i] != new[i] for o in olds):
                j = i
                while j < n and any(o[j] != new[j] for o in olds): j += 1
                out.append([i, base64.b64encode(new[i:j]).decode()]); i = j
            else: i += 1
        return out
    d = dict(name='R89', inputs=inputs, image_sha1=hashlib.sha1(rimg).hexdigest(), file_sha1=hashlib.sha1(rraw).hexdigest(),
             size=len(rraw), pe_off=r.pe_off, blocks=blocks, img_segs=segs(rimg, imgs), hdr_segs=segs(rraw[:r.pe_off], hdrs))
    # every original, patched the way the page does it, must give the release exactly
    for p in originals:
        s = xex.Xex(p); img = bytearray(s.image()); raw = open(p, 'rb').read()
        for o, b in d['img_segs']: part = base64.b64decode(b); img[o:o + len(part)] = part
        assert bytes(img) == rimg, p
        out = bytearray(raw[:r.pe_off])
        for o, b in d['hdr_segs']: part = base64.b64decode(b); out[o:o + len(part)] = part
        at = 0
        for ds, zs in blocks: out += img[at:at + ds]; assert not any(img[at + ds:at + ds + zs]); at += ds + zs
        assert bytes(out) == rraw, p
    return d


CORE = r'''
"use strict";
/* ---- the patcher (no page needed: the tests run this part alone) ---- */
var SG_BASE = 0x10000, SG_NEW_NAME = [0x2e, 0x73, 0x70, 0x6f, 0x6f, 0x6b, 0x79, 0x00];   /* ".spooky\0" */
function sgU32(x, o) { return (x[o] | x[o + 1] << 8 | x[o + 2] << 16 | x[o + 3] << 24) >>> 0; }
function sgPut32(x, o, v) { x[o] = v & 255; x[o + 1] = v >>> 8 & 255; x[o + 2] = v >>> 16 & 255; x[o + 3] = v >>> 24 & 255; }
function sgHex(v) { return '0x' + (v >>> 0).toString(16).toUpperCase(); }
function sgFromHex(h) { var a = new Uint8Array(h.length / 2), i; for (i = 0; i < a.length; i++) a[i] = parseInt(h.substr(2 * i, 2), 16); return a; }
function sgB64(s) {
	if (typeof atob === 'function') { var b = atob(s), a = new Uint8Array(b.length), i; for (i = 0; i < b.length; i++) a[i] = b.charCodeAt(i); return a; }
	return new Uint8Array(Buffer.from(s, 'base64'));
}
function sgSection(cfg) {                     /* the section's part in the file (cfg.raw bytes; the rest of its cfg.size is zero-filled by the console) */
	var code = new Uint8Array(cfg.raw), i;
	for (i = 0; i < cfg.segs.length; i++) code.set(sgB64(cfg.segs[i][1]), cfg.segs[i][0]);
	return code;
}
/* SHA-1 (the XBE's section digests) */
function sgSha1(data) {
	var len = data.length, padded = ((len + 9 + 63) >> 6) << 6, m = new Uint8Array(padded), w = new Int32Array(80);
	var h0 = 0x67452301, h1 = 0xEFCDAB89 | 0, h2 = 0x98BADCFE | 0, h3 = 0x10325476, h4 = 0xC3D2E1F0 | 0, i, j, t;
	m.set(data); m[len] = 0x80;
	var bits = len * 8;
	m[padded - 1] = bits & 255; m[padded - 2] = bits >>> 8 & 255; m[padded - 3] = bits >>> 16 & 255; m[padded - 4] = bits >>> 24 & 255;
	m[padded - 5] = Math.floor(bits / 4294967296) & 255;
	for (i = 0; i < padded; i += 64) {
		for (j = 0; j < 16; j++) w[j] = m[i + 4 * j] << 24 | m[i + 4 * j + 1] << 16 | m[i + 4 * j + 2] << 8 | m[i + 4 * j + 3];
		for (j = 16; j < 80; j++) { t = w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16]; w[j] = t << 1 | t >>> 31; }
		var a = h0, b = h1, c = h2, d = h3, e = h4, f, k;
		for (j = 0; j < 80; j++) {
			if (j < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
			else if (j < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
			else if (j < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC | 0; }
			else { f = b ^ c ^ d; k = 0xCA62C1D6 | 0; }
			t = ((a << 5 | a >>> 27) + f + e + k + w[j]) | 0;
			e = d; d = c; c = b << 30 | b >>> 2; b = a; a = t;
		}
		h0 = h0 + a | 0; h1 = h1 + b | 0; h2 = h2 + c | 0; h3 = h3 + d | 0; h4 = h4 + e | 0;
	}
	var out = new Uint8Array(20), hs = [h0, h1, h2, h3, h4];
	for (i = 0; i < 5; i++) { out[4 * i] = hs[i] >>> 24 & 255; out[4 * i + 1] = hs[i] >>> 16 & 255; out[4 * i + 2] = hs[i] >>> 8 & 255; out[4 * i + 3] = hs[i] & 255; }
	return out;
}
function sgDigest(size, bytes) { var d = new Uint8Array(4 + bytes.length); sgPut32(d, 0, size); d.set(bytes, 4); return sgSha1(d); }
/* what the file is: { version: 'v1.0' | 'v1.5' } or { error: '...' } */
function sgDetect(x) {
	if (x.length < 0x2000 || x[0] !== 0x58 || x[1] !== 0x42 || x[2] !== 0x45 || x[3] !== 0x48) return { error: "This isn't an XBE file. Choose Halo 2's default.xbe." };
	if (sgU32(x, 0x104) !== SG_BASE || sgU32(x, 0x108) !== 0x2000) return { error: "This XBE isn't laid out like Halo 2's." };
	var cert = sgU32(x, 0x118) - SG_BASE, ts = sgU32(x, 0x114), nsec = sgU32(x, 0x11C), v;
	if (cert < 0 || cert + 12 > x.length || sgU32(x, cert + 8) !== 0x4D530064) return { error: "This XBE isn't Halo 2 (its title ID isn't 4D530064)." };
	for (v in SG.versions) if (SG.versions[v].timestamp === ts) {
		if (nsec !== 33) return { error: 'This Halo 2 ' + v + ' XBE has already been changed (' + nsec + ' sections, the original has 33) - use the unmodified original.' };
		return { version: v };
	}
	return { error: 'This is Halo 2, but not a version Spooky Ghost supports (the v1.0 disc or the v1.5 title update).' };
}
/* the patched XBE (as patch_xbe.py makes it), or an Error saying why not */
function sgPatch(input, version, tuSkip) {
	var cfg = SG.versions[version];
	if (!cfg) throw new Error('Unknown version ' + version);
	var det = sgDetect(input);
	if (det.error) throw new Error(det.error);
	if (det.version !== version) throw new Error('This is a Halo 2 ' + det.version + ' XBE - this build needs ' + version + '.');
	var x = input, cert = sgU32(x, 0x118) - SG_BASE, nsec = sgU32(x, 0x11C), secaddr = sgU32(x, 0x120), i, j, k;
	var code = sgSection(cfg), bytesList = cfg.bytes.slice();
	if (tuSkip) { if (!cfg.tu_skip.length) throw new Error('The TU Skip build is for v1.0 only.'); bytesList = bytesList.concat(cfg.tu_skip); }
	var secs = [];
	for (i = 0; i < nsec; i++) {
		var o = secaddr - SG_BASE + 56 * i, na = sgU32(x, o + 20) - SG_BASE, name = '';
		for (j = na; x[j]; j++) name += String.fromCharCode(x[j]);
		secs.push({ hdr: x.slice(o, o + 56), flags: sgU32(x, o), va: sgU32(x, o + 4), vsize: sgU32(x, o + 8), raw: sgU32(x, o + 12), rsize: sgU32(x, o + 16),
			name: name, nameOff: na, refs: [sgU32(x, o + 28) - SG_BASE, sgU32(x, o + 32) - SG_BASE] });
	}
	var top = 0, dataFlags = null;
	for (i = 0; i < nsec; i++) { top = Math.max(top, secs[i].va + secs[i].vsize); if (secs[i].name === '.data' && dataFlags === null) dataFlags = secs[i].flags; }
	if (top > cfg.va) throw new Error('The address the mod goes at is taken in this XBE.');
	/* free zero space in the header area: after everything the headers use, before the embedded PE header */
	var used = Math.max(0x178, cert + sgU32(x, cert), secaddr - SG_BASE + 56 * nsec);
	for (i = 0; i < nsec; i++) { used = Math.max(used, secs[i].nameOff + secs[i].name.length + 1, secs[i].refs[0] + 2, secs[i].refs[1] + 2); }
	[0x14C, 0x150, 0x154].forEach(function (f) { var a = sgU32(x, f); if (a) used = Math.max(used, a - SG_BASE + 128); });
	used = Math.max(used, sgU32(x, 0x164) - SG_BASE + 16 * sgU32(x, 0x160));
	used = Math.max(used, sgU32(x, 0x170) - SG_BASE + sgU32(x, 0x174));
	var pe = -1;
	for (i = used; i + 1 < 0x2000; i++) if (x[i] === 0x4D && x[i + 1] === 0x5A) { pe = i; break; }
	var limit = pe > 0 ? pe : 0x2000, tableOff = (used + 3) & ~3, need = 56 * (nsec + 1) + SG_NEW_NAME.length + 4;
	if (tableOff + need > limit) throw new Error('No free room in the XBE header.');
	for (i = tableOff; i < tableOff + need; i++) if (x[i]) throw new Error('No free room in the XBE header.');
	function fileOff(va) {
		for (var s = 0; s < secs.length; s++) if (secs[s].va <= va && va < secs[s].va + secs[s].rsize) return [s, secs[s].raw + va - secs[s].va];
		throw new Error(sgHex(va) + ' is not in a section.');
	}
	var wrong = 'This XBE does not match the original Halo 2 ' + version + ' - use an unmodified original. (';
	cfg.calls.forEach(function (c) {
		var f = fileOff(c[0]), s = f[0], off = f[1];
		if (secs[s].name !== '.text' || (x[off] !== 0xE8 && x[off] !== 0xE9)) throw new Error(wrong + sgHex(c[0]) + ' is not a call.)');
		var cur = (c[0] + 5 + (sgU32(x, off + 1) | 0)) >>> 0;
		if (cur !== c[1]) throw new Error(wrong + sgHex(c[0]) + ' calls ' + sgHex(cur) + ', expected ' + sgHex(c[1]) + '.)');
	});
	cfg.symrefs.forEach(function (r) {
		var f = fileOff(r[0]);
		if ((secs[f[0]].name !== '.text' && secs[f[0]].name !== 'D3D') || sgU32(x, f[1]) !== r[1]) throw new Error(wrong + sgHex(r[0]) + ' holds ' + sgHex(sgU32(x, f[1])) + '.)');
	});
	bytesList.forEach(function (b) {
		var f = fileOff(b[0]), want = sgFromHex(b[1]), nb = sgFromHex(b[2]);
		if ((secs[f[0]].name !== '.text' && secs[f[0]].name !== 'XONLINE' && secs[f[0]].name !== 'D3D') || want.length !== nb.length) throw new Error(wrong + sgHex(b[0]) + '.)');
		for (k = 0; k < want.length; k++) if (x[f[1] + k] !== want[k]) throw new Error(wrong + sgHex(b[0]) + ' holds other bytes.)');
	});
	if (sgU32(x, 0x124) !== cfg.init_flags[0]) throw new Error(wrong + 'its start-up flags are ' + sgHex(sgU32(x, 0x124)) + '.)');
	/* the output: the original, its end padded to a page, then the section, padded to a page */
	var rawAddr = (x.length + 0xFFF) & ~0xFFF, total = (rawAddr + code.length + 0xFFF) & ~0xFFF, y = new Uint8Array(total), touched = {};
	y.set(x); y.set(code, rawAddr);
	sgPut32(y, 0x124, cfg.init_flags[1]);                                    /* v2.0: the console's memory is not limited to 64 MB (the 360's 128 MB emulator; nothing changes on an Xbox) */
	cfg.calls.forEach(function (c) { var f = fileOff(c[0]); sgPut32(y, f[1] + 1, (c[2] - (c[0] + 5)) >>> 0); touched[f[0]] = 1; });
	bytesList.forEach(function (b) { var f = fileOff(b[0]); y.set(sgFromHex(b[2]), f[1]); touched[f[0]] = 1; });
	cfg.symrefs.forEach(function (r) { var f = fileOff(r[0]); sgPut32(y, f[1], r[2]); touched[f[0]] = 1; });
	if (cfg.title) {
		for (k = 0; k < 80; k++) y[cert + 0xC + k] = 0;
		for (k = 0; k < cfg.title.length && k < 40; k++) { var cc = cfg.title.charCodeAt(k); y[cert + 0xC + 2 * k] = cc & 255; y[cert + 0xC + 2 * k + 1] = cc >>> 8; }
	}
	/* the new table: the 33 originals byte for byte (touched sections with new digests), then .spooky */
	var nameOff = tableOff + 56 * (nsec + 1), refsOff = nameOff + SG_NEW_NAME.length, table = new Uint8Array(56 * (nsec + 1));
	for (i = 0; i < nsec; i++) {
		var h = secs[i].hdr.slice();
		if (touched[i]) {
			var dg = sgDigest(secs[i].rsize, y.subarray(secs[i].raw, secs[i].raw + secs[i].rsize));
			h.set(dg, 36);
			y.set(dg, secaddr - SG_BASE + 56 * i + 36);                    /* the old table stays valid too */
		}
		table.set(h, 56 * i);
	}
	var nh = new Uint8Array(56);
	[dataFlags, cfg.va, cfg.size, rawAddr, code.length, SG_BASE + nameOff, 0, SG_BASE + refsOff, SG_BASE + refsOff + 2].forEach(function (v, q) { sgPut32(nh, 4 * q, v >>> 0); });
	nh.set(sgDigest(code.length, code), 36);
	table.set(nh, 56 * nsec);
	y.set(table, tableOff);
	y.set(SG_NEW_NAME, nameOff);
	for (k = 0; k < 4; k++) y[refsOff + k] = 0;
	sgPut32(y, 0x11C, nsec + 1);
	sgPut32(y, 0x120, SG_BASE + tableOff);
	sgPut32(y, 0x10C, cfg.va + cfg.size - SG_BASE);
	return y;
}
if (typeof module !== 'undefined') module.exports = { sgDetect: sgDetect, sgPatch: sgPatch, sgSha1: sgSha1, SG: SG };
'''

PAGE = r'''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Spooky Ghost Patcher</title>
<style>
:root {
	color-scheme: light;
	--bg: #f4f2f7; --panel: #ffffff; --text: #1c1a22; --muted: #5d586b; --line: #ddd8e6;
	--accent: #6b3fd1; --accent-text: #ffffff; --ok: #1d7a46; --bad: #b3261e; --soft: #efe9fb;
}
@media (prefers-color-scheme: dark) {
	:root:not([data-theme="light"]) {
		color-scheme: dark;
		--bg: #121016; --panel: #1c1922; --text: #ece8f4; --muted: #a59fb3; --line: #322d3b;
		--accent: #9f7cf0; --accent-text: #120d1d; --ok: #5fcf8f; --bad: #ff8a80; --soft: #251f31;
	}
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--text); font: 15px/1.5 "Segoe UI", system-ui, -apple-system, Roboto, sans-serif; }
main { max-width: 680px; margin: 0 auto; padding: 32px 20px 48px; }
h1 { font-size: 24px; margin: 0 0 4px; letter-spacing: 0.2px; }
.sub { color: var(--muted); margin: 0 0 24px; }
.panel { background: var(--panel); border: 1px solid var(--line); border-radius: 12px; padding: 20px; margin-bottom: 16px; }
.step { font-size: 12px; font-weight: 700; letter-spacing: 1px; text-transform: uppercase; color: var(--accent); margin: 0 0 8px; }
h2 { font-size: 17px; margin: 0 0 12px; }
.pick { display: flex; gap: 12px; align-items: center; flex-wrap: wrap; }
.pick label { background: var(--soft); color: var(--text); border: 1px solid var(--line); border-radius: 8px; padding: 9px 14px; cursor: pointer; font-weight: 600; }
.pick label:hover { border-color: var(--accent); }
.pick input { position: absolute; width: 1px; height: 1px; opacity: 0; }
.pick input:focus-visible + label { outline: 2px solid var(--accent); outline-offset: 2px; }
.file { color: var(--muted); word-break: break-all; }
.files { margin: 10px 0 0; display: grid; gap: 2px; }
.tick { display: inline-flex; align-items: center; gap: 8px; margin: 0 0 12px; cursor: pointer; font-weight: 600; }
.tick input { width: 17px; height: 17px; accent-color: var(--accent); margin: 0; }
.tick span { color: var(--muted); font-weight: 400; }
[hidden] { display: none !important; }
.detect { margin: 12px 0 0; padding: 10px 12px; border-radius: 8px; background: var(--soft); display: none; }
.detect.show { display: block; }
.detect.ok { color: var(--ok); }
.detect.bad { color: var(--bad); }
.builds { display: grid; gap: 10px; }
.build { display: flex; justify-content: space-between; align-items: center; gap: 14px; border: 1px solid var(--line); border-radius: 10px; padding: 12px 14px; }
.build b { display: block; }
.build span { color: var(--muted); font-size: 13.5px; }
button { background: var(--accent); color: var(--accent-text); border: 0; border-radius: 8px; padding: 9px 16px; font: inherit; font-weight: 700; cursor: pointer; white-space: nowrap; }
button:disabled { opacity: 0.35; cursor: not-allowed; }
button:focus-visible { outline: 2px solid var(--text); outline-offset: 2px; }
.status { min-height: 1.5em; margin: 14px 0 0; }
.status.ok { color: var(--ok); }
.status.bad { color: var(--bad); }
.notes { color: var(--muted); font-size: 13.5px; }
.notes p { margin: 6px 0; }
code { font-family: Consolas, "Cascadia Mono", Menlo, monospace; font-size: 0.92em; }
</style>
</head>
<body>
<main>
<h1>Spooky Ghost Patcher</h1>
<p class="sub">Spooky Ghost Menu by Fortune090 &middot; @TAG@</p>

<section class="panel">
<p class="step">Step 1</p>
<h2>Choose your original Halo 2 <code>default.xbe</code></h2>
<label class="tick"><input type="checkbox" id="x360"> Xbox 360 <span>&mdash; also make the emulator, <code>xefu1_1.xex</code></span></label>
<div class="pick">
<input type="file" id="xbe" accept=".xbe">
<label for="xbe">Choose default.xbe&hellip;</label>
<input type="file" id="xex" accept=".xex" hidden>
<label for="xex" id="xexlabel" hidden>Choose xefu1_1.xex&hellip;</label>
</div>
<div class="files">
<span class="file" id="fname">No default.xbe chosen</span>
<span class="file" id="fname360" hidden>No xefu1_1.xex chosen</span>
</div>
<p class="detect" id="detect" role="status"></p>
<p class="detect" id="detect360" role="status"></p>
</section>

<section class="panel">
<p class="step">Step 2</p>
<h2 id="step2">Pick a build &mdash; you'll be asked where to save it</h2>
<div class="builds">
<div class="build"><div><b>Spooky Ghost &mdash; v1.0</b><span>For the original disc XBE (v1.0).</span></div><button id="b10" disabled>Build v1.0</button></div>
<div class="build"><div><b>Spooky Ghost &mdash; 1.0 TU Skip</b><span>v1.0 that ignores an installed title update and Xbox Live's update.</span></div><button id="b10tu" disabled>Build 1.0 TU Skip</button></div>
<div class="build"><div><b>Spooky Ghost &mdash; v1.5 (title update)</b><span>For the title update's XBE (v1.5).</span></div><button id="b15" disabled>Build v1.5</button></div>
</div>
<p class="status" id="status" role="status" aria-live="polite"></p>
</section>

<section class="notes">
<p>Each build opens a Save dialog with the name <code>default.xbe</code> filled in &mdash; choose where it goes (Desktop, Downloads, any folder). A new folder for each build is easiest: if the folder already has a <code>default.xbe</code> &mdash; maybe your original &mdash; Windows asks before replacing it.</p>
<p>With <b>Xbox 360</b> ticked, a build asks for a folder instead and puts both files in it: <code>default.xbe</code> and the emulator, <code>xefu1_1.xex</code> (@EMUNAME@), which replaces the one in the console's <code>Compatibility</code> folder &mdash; keep a copy of the original. Make a new folder for it: Chrome and Edge won't open Desktop, Documents or Downloads themselves, only folders inside them, and nothing in the folder you pick is replaced without asking.</p>
<p>Your files never leave your computer: the patching happens in this page. Every place the mod changes is checked against the original first; a modified or unknown file is refused, and the finished emulator is checked against the release build before it is saved.</p>
<p>The Monster/Dreadnought Viewer also needs the <code>spots</code> folder from the Spooky Ghost package, copied to <code>E:\TDATA\4D530064\spots</code> on the Xbox.</p>
</section>
</main>

<script id="core">
var SG = @DATA@;
var SGE = @EMU@;
@CORE@
</script>
<script>
"use strict";
(function () {
	var input = null, found = null, emuInput = null, emuFound = null;
	var el = function (id) { return document.getElementById(id); };
	var builds = [
		{ id: 'b10', version: 'v1.0', tuSkip: false, label: 'v1.0' },
		{ id: 'b10tu', version: 'v1.0', tuSkip: true, label: '1.0 TU Skip' },
		{ id: 'b15', version: 'v1.5', tuSkip: false, label: 'v1.5 (title update)' }
	];
	function say(text, kind) { var s = el('status'); s.textContent = text; s.className = 'status' + (kind ? ' ' + kind : ''); }
	function want360() { return el('x360').checked; }
	function refresh() {
		builds.forEach(function (b) { el(b.id).disabled = !found || found !== b.version || (want360() && !emuFound); });
		el('step2').innerHTML = want360() ? "Pick a build &mdash; you'll be asked for a folder; both files go in it" : "Pick a build &mdash; you'll be asked where to save it";
	}
	function size(f) { return f.name + ' (' + Math.round(f.size / 1024).toLocaleString() + ' KB)'; }
	el('x360').addEventListener('change', function () {
		var on = want360();
		el('xex').hidden = !on; el('xexlabel').hidden = !on; el('fname360').hidden = !on;
		if (!on) el('detect360').className = 'detect';
		else if (el('detect360').textContent) el('detect360').className = 'detect show ' + (emuFound ? 'ok' : 'bad');
		say(''); refresh();
	});
	el('xbe').addEventListener('change', function (e) {
		var f = e.target.files && e.target.files[0], d = el('detect');
		input = null; found = null; say(''); refresh();
		if (!f) { el('fname').textContent = 'No default.xbe chosen'; d.className = 'detect'; return; }
		el('fname').textContent = size(f);
		f.arrayBuffer().then(function (buf) {
			var x = new Uint8Array(buf), r = sgDetect(x);
			if (r.error) { d.textContent = r.error; d.className = 'detect show bad'; return; }
			input = x; found = r.version;
			d.textContent = r.version === 'v1.0' ? 'Halo 2 v1.0 (the original disc) - ready. Pick v1.0 or 1.0 TU Skip.' : 'Halo 2 v1.5 (the title update) - ready. Pick v1.5.';
			d.className = 'detect show ok';
			refresh();
		}, function () { d.textContent = "That file couldn't be read."; d.className = 'detect show bad'; });
	});
	el('xex').addEventListener('change', function (e) {
		var f = e.target.files && e.target.files[0], d = el('detect360');
		emuInput = null; emuFound = null; say(''); refresh();
		if (!f) { el('fname360').textContent = 'No xefu1_1.xex chosen'; d.textContent = ''; d.className = 'detect'; return; }
		el('fname360').textContent = size(f);
		d.textContent = 'Reading xefu1_1.xex...'; d.className = 'detect show';
		f.arrayBuffer().then(function (buf) {
			setTimeout(function () {                                     /* (unpacking takes a second or two: let the line above show first) */
				var x = new Uint8Array(buf), r = sgEmuDetect(x);
				if (r.error) { d.textContent = r.error; d.className = 'detect show bad'; return; }
				emuInput = x; emuFound = r;
				d.textContent = 'xefu1_1.xex: ' + r.name + ' - ready. It will be made into ' + SGE.name + '.';
				d.className = 'detect show ok';
				refresh();
			}, 30);
		}, function () { d.textContent = "That file couldn't be read."; d.className = 'detect show bad'; });
	});
	function download(bytes, name) {
		var url = URL.createObjectURL(new Blob([bytes], { type: 'application/octet-stream' })), a = document.createElement('a');
		a.href = url; a.download = name; document.body.appendChild(a); a.click(); a.remove();
		setTimeout(function () { URL.revokeObjectURL(url); }, 10000);
	}
	async function has(dir, name) { try { await dir.getFileHandle(name); return true; } catch (e) { return false; } }
	async function put(dir, name, bytes) { var fh = await dir.getFileHandle(name, { create: true }), w = await fh.createWritable(); await w.write(bytes); await w.close(); }
	/* Xbox 360 ticked: one folder, both files */
	async function buildBoth(b) {
		var dir = null, out, emu;
		if (typeof window.showDirectoryPicker === 'function') {
			try { dir = await window.showDirectoryPicker({ id: 'spooky-ghost-folder', mode: 'readwrite' }); }
			catch (err) {
				if (err && err.name === 'AbortError') { say('No folder chosen - nothing saved.'); return; }
				dir = null;                                                  /* not allowed here: downloads instead */
			}
		}
		if (dir) {
			var there = [];
			if (await has(dir, 'default.xbe')) there.push('default.xbe');
			if (await has(dir, 'xefu1_1.xex')) there.push('xefu1_1.xex');
			if (there.length && !window.confirm('The folder "' + dir.name + '" already has ' + there.join(' and ') + '.\n\nReplace ' + (there.length > 1 ? 'them' : 'it') + '? If that is your original, choose Cancel and pick another folder.')) { say('Nothing saved - pick the build again and choose another folder.'); return; }
		}
		say('Patching ' + b.label + ' and the emulator...');
		await new Promise(function (r) { setTimeout(r, 30); });
		try { out = sgPatch(input, b.version, b.tuSkip); emu = sgEmuPatch(emuInput, emuFound); }
		catch (err) { say(err.message || String(err), 'bad'); return; }
		if (dir) {
			try {
				await put(dir, 'default.xbe', out); await put(dir, 'xefu1_1.xex', emu);
				say('Saved default.xbe (Spooky Ghost ' + b.label + ') and xefu1_1.xex (' + SGE.name + ') in the folder "' + dir.name + '".', 'ok');
			} catch (err) { say("Couldn't save there (" + (err.message || err) + ') - try another folder.', 'bad'); }
		} else {
			download(out, 'default.xbe'); download(emu, 'xefu1_1.xex');
			say("This browser can't be given a folder, so default.xbe (Spooky Ghost " + b.label + ') and xefu1_1.xex (' + SGE.name + ') were downloaded - move them where you need them. (Chrome or Edge ask for a folder.)', 'ok');
		}
	}
	builds.forEach(function (b) {
		el(b.id).addEventListener('click', async function () {
			if (!input || found !== b.version) return;
			if (want360()) { if (emuFound) await buildBoth(b); return; }
			var fh = null, out;
			/* the Save dialog (Chrome / Edge): where to put default.xbe. Not the folder picker - Chrome won't hand a page
			   a whole well-known folder (Desktop, Documents, Downloads, your user folder, a drive's root: "contains
			   system files"), but it saves a file into any of them; and Windows asks before replacing a file there */
			if (typeof window.showSaveFilePicker === 'function') {
				try {
					fh = await window.showSaveFilePicker({ id: 'spooky-ghost-output', suggestedName: 'default.xbe',
						types: [{ description: 'Xbox executable (default.xbe)', accept: { 'application/octet-stream': ['.xbe'] } }] });
				} catch (err) {
					if (err && err.name === 'AbortError') { say('No location chosen - nothing saved.'); return; }
					fh = null;                                               /* not allowed here: a download instead */
				}
			}
			say('Patching ' + b.label + '...');
			try { out = sgPatch(input, b.version, b.tuSkip); }
			catch (err) { say(err.message || String(err), 'bad'); return; }
			if (fh) {
				try {
					var w = await fh.createWritable();
					await w.write(out); await w.close();
					say('Saved Spooky Ghost ' + b.label + ' as ' + fh.name + '.', 'ok');
				} catch (err) { say("Couldn't save there (" + (err.message || err) + ') - try another place.', 'bad'); }
			} else {
				download(out, 'default.xbe');
				say("This browser has no Save dialog for pages, so Spooky Ghost " + b.label + " was downloaded as default.xbe - move it where you need it. (Chrome or Edge ask where to save.)", 'ok');
			}
		});
	});
	refresh();
})();
</script>
</body>
</html>
'''


def main(tag, out, release, *originals):
    data = dict(build=tag, versions={'v1.0': version_data('v1.0', tag), 'v1.5': version_data('v1.5', tag)})
    emu = emu_data(release, originals)
    core = CORE + open(os.path.join(HERE, 'patcher_emu_core.js'), encoding='utf-8').read()
    page = (PAGE.replace('@TAG@', tag).replace('@CORE@', core).replace('@DATA@', json.dumps(data, separators=(',', ':')))
            .replace('@EMU@', json.dumps(emu, separators=(',', ':'))).replace('@EMUNAME@', emu['name']))
    open(out, 'w', encoding='utf-8').write(page)
    print('%s: %d bytes' % (out, len(page.encode('utf-8'))))


if __name__ == '__main__':
    if len(sys.argv) < 5: sys.exit(__doc__)
    main(*sys.argv[1:])
