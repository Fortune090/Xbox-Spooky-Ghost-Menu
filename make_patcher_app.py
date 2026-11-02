#!/usr/bin/env python3
"""Build "Spooky Ghost Patcher.html": a one-file app that players open in a browser, give their own original Halo 2
default.xbe, and press a button for the build they want. It does what patch_xbe.py does, in the page: every hooked call
site and patched byte is checked against the original first, the mod's section is appended, the section table extended
and every touched section's digest recomputed. The app holds only the mod (its section, zero runs left out) and the patch
lists - none of the game's code.

    python3 make_patcher_app.py <build tag> <output .html>        (after ./build.sh)
"""
import base64, json, sys
import patch_xbe

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
    out = dict(timestamp=cfg['timestamp'], va=cfg['va'], size=len(code), title=cfg['title'] or '',
               calls=[[s, t, syms[h]] for s, t, h in cfg['calls']],
               bytes=[[s, w, n] for s, w, n in cfg.get('bytes', ())],
               tu_skip=[[s, w, n] for s, w, n in cfg.get('tu_skip', ())],
               symrefs=[[s, w, syms[y]] for s, w, y in cfg.get('symrefs', ())],
               segs=segments(code))
    # the segments put back together must be the section exactly
    back = bytearray(len(code))
    for o, b in out['segs']:
        raw = base64.b64decode(b); back[o:o + len(raw)] = raw
    assert bytes(back) == code
    return out


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
function sgSection(cfg) {
	var code = new Uint8Array(cfg.size), i;
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
		if (secs[f[0]].name !== '.text' || sgU32(x, f[1]) !== r[1]) throw new Error(wrong + sgHex(r[0]) + ' holds ' + sgHex(sgU32(x, f[1])) + '.)');
	});
	bytesList.forEach(function (b) {
		var f = fileOff(b[0]), want = sgFromHex(b[1]), nb = sgFromHex(b[2]);
		if ((secs[f[0]].name !== '.text' && secs[f[0]].name !== 'XONLINE') || want.length !== nb.length) throw new Error(wrong + sgHex(b[0]) + '.)');
		for (k = 0; k < want.length; k++) if (x[f[1] + k] !== want[k]) throw new Error(wrong + sgHex(b[0]) + ' holds other bytes.)');
	});
	/* the output: the original, its end padded to a page, then the section, padded to a page */
	var rawAddr = (x.length + 0xFFF) & ~0xFFF, total = (rawAddr + code.length + 0xFFF) & ~0xFFF, y = new Uint8Array(total), touched = {};
	y.set(x); y.set(code, rawAddr);
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
<p class="sub">Spooky Ghost Menu by Fortune090 &middot; build @TAG@</p>

<section class="panel">
<p class="step">Step 1</p>
<h2>Choose your original Halo 2 <code>default.xbe</code></h2>
<div class="pick">
<input type="file" id="xbe" accept=".xbe">
<label for="xbe">Choose default.xbe&hellip;</label>
<span class="file" id="fname">No file chosen</span>
</div>
<p class="detect" id="detect" role="status"></p>
</section>

<section class="panel">
<p class="step">Step 2</p>
<h2>Pick a build &mdash; you'll be asked where to save it</h2>
<div class="builds">
<div class="build"><div><b>Spooky Ghost &mdash; v1.0</b><span>For the original disc XBE (v1.0).</span></div><button id="b10" disabled>Build v1.0</button></div>
<div class="build"><div><b>Spooky Ghost &mdash; 1.0 TU Skip</b><span>v1.0 that ignores an installed title update and Xbox Live's update.</span></div><button id="b10tu" disabled>Build 1.0 TU Skip</button></div>
<div class="build"><div><b>Spooky Ghost &mdash; v1.5 (title update)</b><span>For the title update's XBE (v1.5).</span></div><button id="b15" disabled>Build v1.5</button></div>
</div>
<p class="status" id="status" role="status" aria-live="polite"></p>
</section>

<section class="notes">
<p>Each build is saved as <code>default.xbe</code> in the folder you choose. If that folder already has a <code>default.xbe</code> &mdash; maybe your original &mdash; you'll be asked before it's replaced, so a new empty folder for each build is easiest.</p>
<p>Your XBE never leaves your computer: the patching happens in this page. Every place the mod changes is checked against the original first; a modified or unknown XBE is refused.</p>
<p>The Monster/Dreadnought Viewer also needs the <code>spots</code> folder from the Spooky Ghost package, copied to <code>E:\TDATA\4D530064\spots</code> on the Xbox.</p>
</section>
</main>

<script id="core">
var SG = @DATA@;
@CORE@
</script>
<script>
"use strict";
(function () {
	var input = null, found = null;
	var el = function (id) { return document.getElementById(id); };
	var builds = [
		{ id: 'b10', version: 'v1.0', tuSkip: false, label: 'v1.0' },
		{ id: 'b10tu', version: 'v1.0', tuSkip: true, label: '1.0 TU Skip' },
		{ id: 'b15', version: 'v1.5', tuSkip: false, label: 'v1.5 (title update)' }
	];
	function say(text, kind) { var s = el('status'); s.textContent = text; s.className = 'status' + (kind ? ' ' + kind : ''); }
	function refresh() { builds.forEach(function (b) { el(b.id).disabled = !found || found !== b.version; }); }
	el('xbe').addEventListener('change', function (e) {
		var f = e.target.files && e.target.files[0], d = el('detect');
		input = null; found = null; say(''); refresh();
		if (!f) { el('fname').textContent = 'No file chosen'; d.className = 'detect'; return; }
		el('fname').textContent = f.name + ' (' + Math.round(f.size / 1024).toLocaleString() + ' KB)';
		f.arrayBuffer().then(function (buf) {
			var x = new Uint8Array(buf), r = sgDetect(x);
			if (r.error) { d.textContent = r.error; d.className = 'detect show bad'; return; }
			input = x; found = r.version;
			d.textContent = r.version === 'v1.0' ? 'Halo 2 v1.0 (the original disc) - ready. Pick v1.0 or 1.0 TU Skip.' : 'Halo 2 v1.5 (the title update) - ready. Pick v1.5.';
			d.className = 'detect show ok';
			refresh();
		}, function () { d.textContent = "That file couldn't be read."; d.className = 'detect show bad'; });
	});
	function download(bytes, name) {
		var url = URL.createObjectURL(new Blob([bytes], { type: 'application/octet-stream' })), a = document.createElement('a');
		a.href = url; a.download = name; document.body.appendChild(a); a.click(); a.remove();
		setTimeout(function () { URL.revokeObjectURL(url); }, 10000);
	}
	builds.forEach(function (b) {
		el(b.id).addEventListener('click', async function () {
			if (!input || found !== b.version) return;
			var dir = null, out;
			if (typeof window.showDirectoryPicker === 'function') {
				try { dir = await window.showDirectoryPicker({ id: 'spooky-ghost-output', mode: 'readwrite' }); }
				catch (err) {
					if (err && err.name === 'AbortError') { say('No folder chosen - nothing saved.'); return; }
					dir = null;                                              /* not allowed here: a download instead */
				}
			}
			say('Patching ' + b.label + '...');
			try { out = sgPatch(input, b.version, b.tuSkip); }
			catch (err) { say(err.message || String(err), 'bad'); return; }
			if (dir) {
				try {
					var exists = false;
					try { await dir.getFileHandle('default.xbe'); exists = true; } catch (e) { exists = false; }
					if (exists && !window.confirm('The folder "' + dir.name + '" already has a default.xbe - it may be your original.\n\nReplace it with Spooky Ghost ' + b.label + '?')) { say('Not saved - the folder\'s default.xbe was left as it is.'); return; }
					var fh = await dir.getFileHandle('default.xbe', { create: true }), w = await fh.createWritable();
					await w.write(out); await w.close();
					say('Saved Spooky Ghost ' + b.label + ' as default.xbe in "' + dir.name + '".', 'ok');
				} catch (err) { say("Couldn't save there (" + (err.message || err) + ') - try another folder.', 'bad'); }
			} else {
				download(out, 'default.xbe');
				say("This browser can't choose a folder, so Spooky Ghost " + b.label + " was downloaded as default.xbe - move it where you need it. (Chrome or Edge ask for a folder.)", 'ok');
			}
		});
	});
})();
</script>
</body>
</html>
'''


def main(tag, out):
    data = dict(build=tag, versions={'v1.0': version_data('v1.0', tag), 'v1.5': version_data('v1.5', tag)})
    page = PAGE.replace('@TAG@', tag).replace('@CORE@', CORE).replace('@DATA@', json.dumps(data, separators=(',', ':')))
    open(out, 'w', encoding='utf-8').write(page)
    print('%s: %d bytes' % (out, len(page.encode('utf-8'))))


if __name__ == '__main__':
    if len(sys.argv) != 3: sys.exit(__doc__)
    main(*sys.argv[1:])
