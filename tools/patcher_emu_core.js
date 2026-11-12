/* ---- the Xbox 360 emulator (xefu1_1.xex): unpack the original, apply the release's changes, write it back ----
   The original's program is stored LZX-compressed; the release is stored in the format's plain "basic" form (its runs
   of zeros left out). The page holds only what differs - the bytes of the program the release changes, and the header
   fields that follow from them - and checks the result's SHA-1 against the release build's before handing it over. */
function sgBe32(x, o) { return ((x[o] << 24) | (x[o + 1] << 16) | (x[o + 2] << 8) | x[o + 3]) >>> 0; }
function sgHexOf(a) { var s = '', i; for (i = 0; i < a.length; i++) s += (a[i] < 16 ? '0' : '') + a[i].toString(16); return s; }
function sgXexParse(x) {
	if (x.length < 0x200 || x[0] !== 0x58 || x[1] !== 0x45 || x[2] !== 0x58 || x[3] !== 0x32) throw new Error("This isn't an Xbox 360 program file (XEX2). Choose the console's xefu1_1.xex.");
	var h = { peOff: sgBe32(x, 8), secOff: sgBe32(x, 0x10), n: sgBe32(x, 0x14), keys: {} }, i;
	if (h.n > 64 || h.peOff > x.length || h.secOff + 8 > x.length) throw new Error("This XEX's header can't be read.");
	for (i = 0; i < h.n; i++) h.keys[sgBe32(x, 0x18 + 8 * i)] = sgBe32(x, 0x1C + 8 * i);
	h.imageSize = sgBe32(x, h.secOff + 4);
	h.ffi = h.keys[0x3FF];
	if (!h.ffi || h.ffi + 16 > x.length) throw new Error("This XEX has no file-format header.");
	h.ffiSize = sgBe32(x, h.ffi); h.enc = (x[h.ffi + 4] << 8) | x[h.ffi + 5]; h.comp = (x[h.ffi + 6] << 8) | x[h.ffi + 7];
	return h;
}
/* LZX, as Microsoft's cabinet format has it (the method the console's files use): verbatim, aligned-offset and
   uncompressed blocks, 32 KB frames, no state reset between frames. Written after libmspack's decoder (lzxd.c). */
function sgLzx(src, windowBits, outLen) {
	var out = new Uint8Array(outLen), ip = 0, bitBuf = 0, bitsLeft = 0;          /* bitBuf: up to 32 bits, the next bit on top (kept as a double below 2^32) */
	var slots = windowBits === 21 ? 50 : windowBits === 20 ? 42 : windowBits << 1, mainN = 256 + (slots << 3);
	var extra = new Uint8Array(52), base = new Uint32Array(52), i, j;
	for (i = 0, j = 0; i < 51; i += 2) { extra[i] = j; extra[i + 1] = j; if (i !== 0 && j < 17) j++; }
	for (i = 0, j = 0; i < 51; i++) { base[i] = j; j += 1 << extra[i]; }
	function need(n) { while (bitsLeft < n) { var b0 = ip < src.length ? src[ip] : 0, b1 = ip + 1 < src.length ? src[ip + 1] : 0; ip += 2; bitBuf = bitBuf * 65536 + ((b1 << 8) | b0); bitsLeft += 16; } }
	function bits(n) {
		if (n === 0) return 0;
		need(n);
		var sh = bitsLeft - n, p = Math.pow(2, sh), v = Math.floor(bitBuf / p);
		bitBuf -= v * p; bitsLeft = sh;
		return v;
	}
	function table(lens, n) {                                                 /* canonical codes, shortest first; null if no symbol has a length */
		var count = new Int32Array(18), first = new Int32Array(18), start = new Int32Array(18), syms = new Int32Array(n), code = 0, k, s, any = 0;
		for (s = 0; s < n; s++) if (lens[s]) { count[lens[s]]++; any = 1; }
		if (!any) return null;
		var at = 0;
		for (k = 1; k <= 16; k++) { first[k] = code; start[k] = at; code = (code + count[k]) << 1; at += count[k]; }
		var fill = new Int32Array(18);
		for (s = 0; s < n; s++) if (lens[s]) { syms[start[lens[s]] + fill[lens[s]]++] = s; }
		return { count: count, first: first, start: start, syms: syms };
	}
	function sym(t) {
		if (!t) throw new Error('LZX: a symbol from an empty tree');
		var code = 0, k;
		for (k = 1; k <= 16; k++) {
			code = (code << 1) | bits(1);
			var d = code - t.first[k];
			if (d < t.count[k]) return t.syms[t.start[k] + d];
		}
		throw new Error('LZX: no such code');
	}
	var mainLen = new Uint8Array(mainN), lenLen = new Uint8Array(250), alignLen = new Uint8Array(8), preLen = new Uint8Array(20);
	var mainT = null, lenT = null, alignT = null;
	function readLens(lens, firstS, lastS) {
		var x, y, z, pre;
		for (x = 0; x < 20; x++) preLen[x] = bits(4);
		pre = table(preLen, 20);
		for (x = firstS; x < lastS;) {
			z = sym(pre);
			if (z === 17) { y = bits(4) + 4; while (y-- && x < lastS) lens[x++] = 0; }
			else if (z === 18) { y = bits(5) + 20; while (y-- && x < lastS) lens[x++] = 0; }
			else if (z === 19) { y = bits(1) + 4; z = sym(pre); z = lens[x] - z; if (z < 0) z += 17; while (y-- && x < lastS) lens[x++] = z; }
			else { z = lens[x] - z; if (z < 0) z += 17; lens[x++] = z; }
		}
	}
	var R0 = 1, R1 = 1, R2 = 1, pos = 0, blockType = 0, blockLen = 0, blockRem = 0, headerRead = 0, intelSize = 0, intelStarted = 0, intelPos = 0, frame = 0;
	while (pos < outLen) {
		if (!headerRead) { var hi = 0, lo = 0; if (bits(1)) { hi = bits(16); lo = bits(16); } intelSize = hi * 65536 + lo; headerRead = 1; }
		var frameStart = pos, frameSize = Math.min(32768, outLen - pos), todo = frameSize;
		while (todo > 0) {
			if (blockRem === 0) {
				if (blockType === 3 && (blockLen & 1)) ip++;                  /* an odd-sized uncompressed block: one byte of padding */
				blockType = bits(3);
				var bi = bits(16), bj = bits(8);
				blockRem = blockLen = bi * 256 + bj;
				if (blockType === 2 || blockType === 1) {
					if (blockType === 2) { for (i = 0; i < 8; i++) alignLen[i] = bits(3); alignT = table(alignLen, 8); }
					readLens(mainLen, 0, 256); readLens(mainLen, 256, mainN);
					mainT = table(mainLen, mainN);
					if (mainLen[0xE8] !== 0) intelStarted = 1;
					readLens(lenLen, 0, 249);
					lenT = table(lenLen, 249);
				} else if (blockType === 3) {
					intelStarted = 1;
					if (bitsLeft === 0) need(16);
					bitsLeft = 0; bitBuf = 0;                                 /* (the 1-16 bits to the next 16-bit boundary are dropped) */
					R0 = (src[ip] | src[ip + 1] << 8 | src[ip + 2] << 16 | src[ip + 3] << 24) >>> 0;
					R1 = (src[ip + 4] | src[ip + 5] << 8 | src[ip + 6] << 16 | src[ip + 7] << 24) >>> 0;
					R2 = (src[ip + 8] | src[ip + 9] << 8 | src[ip + 10] << 16 | src[ip + 11] << 24) >>> 0;
					ip += 12;
				} else throw new Error('LZX: block type ' + blockType);
			}
			var run = Math.min(blockRem, todo);
			todo -= run; blockRem -= run;
			if (blockType === 3) {
				if (ip + run > src.length) throw new Error('LZX: the data ends early');
				out.set(src.subarray(ip, ip + run), pos); ip += run; pos += run; run = 0;
			} else {
				while (run > 0) {
					var me = sym(mainT);
					if (me < 256) { out[pos++] = me; run--; continue; }
					me -= 256;
					var mlen = me & 7, moff = me >> 3, ex, vb;
					if (mlen === 7) mlen += sym(lenT);
					mlen += 2;
					if (moff === 0) moff = R0;
					else if (moff === 1) { moff = R1; R1 = R0; R0 = moff; }
					else if (moff === 2) { moff = R2; R2 = R0; R0 = moff; }
					else {
						if (blockType === 1) {
							if (moff === 3) moff = 1;
							else { ex = moff >= 36 ? 17 : extra[moff]; vb = bits(ex); moff = base[moff] - 2 + vb; }
						} else {
							ex = moff >= 36 ? 17 : extra[moff]; moff = base[moff] - 2;
							if (ex > 3) { vb = bits(ex - 3); moff += vb * 8; moff += sym(alignT); }
							else if (ex === 3) moff += sym(alignT);
							else if (ex > 0) moff += bits(ex);
							else moff = 1;
						}
						R2 = R1; R1 = R0; R0 = moff;
					}
					if (pos + mlen > outLen) throw new Error('LZX: a match runs past the end');
					var from = pos - moff, k2;
					for (k2 = 0; k2 < mlen; k2++, from++) out[pos++] = from < 0 ? 0 : out[from];
					run -= mlen;
				}
				if (run < 0) { if (-run > blockRem) throw new Error('LZX: a match runs past its block'); blockRem -= -run; todo += run; }
			}
		}
		if (pos - frameStart !== frameSize) throw new Error('LZX: a frame of the wrong size');
		if (bitsLeft > 0) need(16);
		if (bitsLeft & 15) bits(bitsLeft & 15);
		if (intelStarted && intelSize && frame <= 32768 && frameSize > 10) {       /* the x86 call translation, undone (not used by the console's own files) */
			var d = frameStart, dend = frameStart + frameSize - 10, cur = intelPos;
			while (d < dend) {
				if (out[d++] !== 0xE8) { cur++; continue; }
				var abs = out[d] | out[d + 1] << 8 | out[d + 2] << 16 | out[d + 3] << 24;
				if (abs >= -cur && abs < intelSize) { var rel = abs >= 0 ? abs - cur : abs + intelSize; out[d] = rel & 255; out[d + 1] = rel >>> 8 & 255; out[d + 2] = rel >>> 16 & 255; out[d + 3] = rel >>> 24 & 255; }
				d += 4; cur += 5;
			}
		}
		if (intelSize) intelPos += frameSize;
		frame++;
	}
	return out;
}
/* the program inside an XEX: unpacked */
function sgXexImage(x, h) {
	var data = x.subarray(h.peOff), out, p = 0, i;
	if (h.enc !== 0) throw new Error("This xefu1_1.xex is an encrypted copy - the patcher needs the unencrypted one.");
	if (h.comp === 1) {
		out = new Uint8Array(h.imageSize); var at = 0, nb = (h.ffiSize - 8) >> 3;
		for (i = 0; i < nb; i++) { var ds = sgBe32(x, h.ffi + 8 + 8 * i), zs = sgBe32(x, h.ffi + 12 + 8 * i); if (at + ds > h.imageSize || p + ds > data.length) throw new Error("This XEX's blocks can't be read."); out.set(data.subarray(p, p + ds), at); at += ds + zs; p += ds; }
		return out;
	}
	if (h.comp !== 2) throw new Error("This XEX is stored in a way the patcher doesn't read (type " + h.comp + ').');
	var win = sgBe32(x, h.ffi + 8), blockSize = sgBe32(x, h.ffi + 12), wb = 0, t, chunks = [], total = 0;
	for (t = win; t > 1; t >>>= 1) wb++;
	if (wb < 15 || wb > 21) throw new Error("This XEX's compression window can't be used.");
	while (blockSize) {
		if (p + 24 > data.length) throw new Error("This XEX's compressed data ends early.");
		var pnext = p + blockSize, nextSize = sgBe32(data, p);
		p += 24;
		for (;;) {
			var cs = (data[p] << 8) | data[p + 1]; p += 2;
			if (!cs) break;
			if (p + cs > data.length) throw new Error("This XEX's compressed data ends early.");
			chunks.push(data.subarray(p, p + cs)); total += cs; p += cs;
		}
		p = pnext; blockSize = nextSize;
		if (p >= data.length) break;
	}
	var stream = new Uint8Array(total), q = 0;
	for (i = 0; i < chunks.length; i++) { stream.set(chunks[i], q); q += chunks[i].length; }
	return sgLzx(stream, wb, h.imageSize);
}
/* which original this is: { name, image, header } or { error } */
function sgEmuDetect(x) {
	var h, img, sha;
	try { h = sgXexParse(x); } catch (e) { return { error: e.message || String(e) }; }
	try { img = sgXexImage(x, h); } catch (e2) { var m = e2.message || String(e2); return { error: m.indexOf('LZX') === 0 ? "This xefu1_1.xex can't be unpacked (" + m + ') - it is damaged or not a complete copy.' : m }; }
	sha = sgHexOf(sgSha1(img));
	if (sha === SGE.image_sha1) return { error: 'This xefu1_1.xex is already the Spooky Ghost one (' + SGE.name + ') - choose the original.' };
	if (!SGE.inputs[sha]) return { error: "This isn't an xefu1_1.xex the patcher knows - choose the console's original (its program's SHA-1 is " + sha + ').' };
	return { name: SGE.inputs[sha], image: img, header: h };
}
/* the release's xefu1_1.xex from an original, or an Error saying why not */
function sgEmuPatch(x, known) {                    /* known: what sgEmuDetect gave for this file, if it has been run already */
	var det = known && known.image ? known : sgEmuDetect(x), i, k;
	if (det.error) throw new Error(det.error);
	var img = new Uint8Array(det.image);                                     /* a copy: the detected one stays as it was */
	for (i = 0; i < SGE.img_segs.length; i++) img.set(sgB64(SGE.img_segs[i][1]), SGE.img_segs[i][0]);
	if (sgHexOf(sgSha1(img)) !== SGE.image_sha1) throw new Error("The patched program doesn't match the release - nothing written.");
	if (det.header.peOff !== SGE.pe_off) throw new Error("This XEX's header isn't the size expected.");
	var out = new Uint8Array(SGE.size), blocks = SGE.blocks, at = 0, p = SGE.pe_off;
	out.set(x.subarray(0, SGE.pe_off));
	for (i = 0; i < SGE.hdr_segs.length; i++) out.set(sgB64(SGE.hdr_segs[i][1]), SGE.hdr_segs[i][0]);
	for (i = 0; i < blocks.length; i++) {
		out.set(img.subarray(at, at + blocks[i][0]), p);
		for (k = at + blocks[i][0]; k < at + blocks[i][0] + blocks[i][1] && k < img.length; k++) if (img[k]) throw new Error("The patched program doesn't fit the release's layout - nothing written.");
		p += blocks[i][0]; at += blocks[i][0] + blocks[i][1];
	}
	if (sgHexOf(sgSha1(out)) !== SGE.file_sha1) throw new Error("The finished file doesn't match the release - nothing written.");
	return out;
}
if (typeof module !== 'undefined') { module.exports.sgEmuDetect = sgEmuDetect; module.exports.sgEmuPatch = sgEmuPatch; module.exports.sgLzx = sgLzx; module.exports.SGE = SGE; }
