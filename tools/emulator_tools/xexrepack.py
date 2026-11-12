#!/usr/bin/env python3
"""Rebuild an XEX2 file around a changed memory image, with every checksum the console checks recomputed.

Checksum formulas (from emoose's idaxex, formats/xex.cpp; verified on all 13 stock Compatibility files):
  * page chain:  SecurityInfo.ImageHash (sec+0x114) = SHA1(page[0] || descriptor[0])
                 descriptor[i].digest              = SHA1(page[i+1] || descriptor[i+1])   (last one: unused, 0)
                 where page[i] is descriptor i's run of pages (count * page size; 64 KB pages below
                 0x90000000, 4 KB above) and descriptor = 24 bytes (size/info word + digest).
  * header hash: SecurityInfo.HeaderHash (sec+0x164) = SHA1(headers[sec+8+0x174 : pe_off] || headers[0 : sec+8])
                 i.e. everything after the signed ImageInfo (incl. the page descriptors and all optional
                 headers such as the file-format header) followed by the start of the file.
The RSA signature (over ImageInfo) can't be regenerated, so it no longer matches; RGH consoles don't check it.

The output is unencrypted with "basic" compression (data runs + zero-fill runs), block edges on 4 KB.

usage (library): repack(orig_xex_path, new_image) -> bytes
       (cli)     xexrepack.py <orig.xex> <image.bin> <out.xex>
"""
import hashlib
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'handoff', 'tools'))
from xex import Xex  # noqa: E402

SEC_IMAGEHASH = 0x114
SEC_HEADERHASH = 0x164
SEC_IMAGEINFO_END = 8 + 0x174
SEC_DESCS = 0x184
FFI_KEY = 0x000003FF


def be32(b, o):
    return struct.unpack_from('>I', b, o)[0]


def page_size(load_address):
    return 0x10000 if load_address < 0x90000000 else 0x1000


def header_hash(hdr):
    pe, s = be32(hdr, 8), be32(hdr, 0x10)
    return hashlib.sha1(bytes(hdr[s + SEC_IMAGEINFO_END:pe]) + bytes(hdr[0:s + 8])).digest()


def rechain(hdr, img):
    """Recompute descriptor digests and ImageHash in-place in the header bytearray."""
    s = be32(hdr, 0x10)
    n = be32(hdr, s + 0x180)
    ps = page_size(be32(hdr, s + 0x110))
    runs, off = [], 0
    for i in range(n):
        cnt = be32(hdr, s + SEC_DESCS + 24 * i) >> 4
        runs.append((off, cnt * ps))
        off += cnt * ps
    assert off == len(img), 'descriptors cover 0x%x, image is 0x%x' % (off, len(img))
    for i in range(n - 2, -1, -1):
        o, sz = runs[i + 1]
        nxt = bytes(hdr[s + SEC_DESCS + 24 * (i + 1): s + SEC_DESCS + 24 * (i + 2)])
        hdr[s + SEC_DESCS + 24 * i + 4: s + SEC_DESCS + 24 * i + 24] = hashlib.sha1(img[o:o + sz] + nxt).digest()
    o, sz = runs[0]
    d0 = bytes(hdr[s + SEC_DESCS: s + SEC_DESCS + 24])
    hdr[s + SEC_IMAGEHASH: s + SEC_IMAGEHASH + 20] = hashlib.sha1(img[o:o + sz] + d0).digest()


def zero_runs(img, align=0x1000, min_len=0x10000):
    """Zero runs of at least min_len with both ends on `align` boundaries."""
    runs = []
    n = len(img)
    i = 0
    zero_page = bytes(align)
    while i < n:
        if img[i:i + align] == zero_page[:min(align, n - i)]:
            j = i
            while j < n and img[j:j + align] == zero_page[:min(align, n - j)]:
                j += align
            j = min(j, n)
            if j - i >= min_len:
                runs.append((i, j))
            i = j
        else:
            i += align
    return runs


def plan_blocks(img, max_blocks):
    runs = sorted(zero_runs(img), key=lambda r: r[0] - r[1])[:max_blocks]  # longest first
    runs.sort()
    blocks, pos = [], 0
    for a, b in runs:
        blocks.append((pos, a - pos, b - a))
        pos = b
    if pos < len(img):
        blocks.append((pos, len(img) - pos, 0))
    while len(blocks) > max_blocks:  # fold the tail into the previous block
        (p0, d0, z0), (p1, d1, z1) = blocks[-2], blocks[-1]
        blocks[-2:] = [(p0, d0 + z0 + d1, z1)]
    assert sum(d + z for p, d, z in blocks) == len(img)
    return blocks


def repack(orig_path, img):
    x = Xex(orig_path)
    assert len(img) == x.image_size, 'image size changed'
    hdr = bytearray(x.raw[:x.pe_off])
    ffi = x.headers[FFI_KEY]
    old_size = be32(hdr, ffi)
    max_blocks = (old_size - 8) // 8
    blocks = plan_blocks(img, max_blocks)
    new_size = 8 + 8 * len(blocks)
    struct.pack_into('>IHH', hdr, ffi, new_size, 0, 1)        # unencrypted, basic compression
    for k, (pos, d, z) in enumerate(blocks):
        struct.pack_into('>II', hdr, ffi + 8 + 8 * k, d, z)
    for o in range(ffi + new_size, ffi + old_size):
        hdr[o] = 0
    rechain(hdr, img)
    s = x.sec_off
    hdr[s + SEC_HEADERHASH: s + SEC_HEADERHASH + 20] = header_hash(hdr)
    data = b''.join(img[p:p + d] for p, d, z in blocks)
    out = bytes(hdr) + data
    out += bytes(-len(out) % 0x1000)
    return out, blocks


def verify(path, expect_img=None):
    """Re-read a file: image must decode (and match), header hash and page chain must hold."""
    x = Xex(path)
    img = x.image()
    s = x.sec_off
    res = {'image_ok': expect_img is None or img == expect_img,
           'header_hash_ok': header_hash(bytearray(x.raw[:x.pe_off])) == x.raw[s + SEC_HEADERHASH:s + SEC_HEADERHASH + 20]}
    hdr = bytearray(x.raw[:x.pe_off])
    before = bytes(hdr)
    rechain(hdr, img)
    res['page_chain_ok'] = bytes(hdr) == before
    return res


if __name__ == '__main__':
    orig, imgp, outp = sys.argv[1:4]
    out, blocks = repack(orig, open(imgp, 'rb').read())
    open(outp, 'wb').write(out)
    print('blocks', [(hex(p), hex(d), hex(z)) for p, d, z in blocks], 'size', len(out))
    print(verify(outp, open(imgp, 'rb').read()))
