#!/usr/bin/env python3
"""XEX2 reader: parses headers, decrypts (retail or devkit key) and decompresses (basic / LZX) the base image.

usage: xex.py <in.xex> <out.bin>      writes the loaded image (as mapped at the image base) and prints headers
"""
import struct, subprocess, sys, os, hashlib
from Crypto.Cipher import AES

RETAIL_KEY = bytes.fromhex('20B185A59D28FDC340583FBB0896BF91')
DEVKIT_KEY = bytes(16)
HERE = os.path.dirname(os.path.abspath(__file__))

HDR_NAMES = {
    0x000002FF: 'RESOURCE_INFO', 0x000003FF: 'FILE_FORMAT_INFO', 0x000005FF: 'DELTA_PATCH_DESCRIPTOR',
    0x00000405: 'BASE_REFERENCE', 0x000080FF: 'BOUNDING_PATH', 0x00008105: 'DEVICE_ID',
    0x00010001: 'ORIGINAL_BASE_ADDRESS', 0x00010100: 'ENTRY_POINT', 0x00010201: 'IMAGE_BASE_ADDRESS',
    0x000103FF: 'IMPORT_LIBRARIES', 0x00018002: 'CHECKSUM_TIMESTAMP', 0x00018102: 'ENABLED_FOR_CALLCAP',
    0x00018200: 'ENABLED_FOR_FASTCAP', 0x000183FF: 'ORIGINAL_PE_NAME', 0x000200FF: 'STATIC_LIBRARIES',
    0x00020104: 'TLS_INFO', 0x00020200: 'DEFAULT_STACK_SIZE', 0x00020301: 'DEFAULT_FILESYSTEM_CACHE_SIZE',
    0x00020401: 'DEFAULT_HEAP_SIZE', 0x00028002: 'PAGE_HEAP_SIZE_AND_FLAGS', 0x00030000: 'SYSTEM_FLAGS',
    0x00040006: 'EXECUTION_INFO', 0x00040201: 'TITLE_WORKSPACE_SIZE', 0x00040310: 'GAME_RATINGS',
    0x00040404: 'LAN_KEY', 0x000405FF: 'XBOX360_LOGO', 0x000406FF: 'MULTIDISC_MEDIA_IDS',
    0x000407FF: 'ALTERNATE_TITLE_IDS', 0x00040801: 'ADDITIONAL_TITLE_MEMORY', 0x00E10402: 'EXPORTS_BY_NAME',
}

def be32(b, o): return struct.unpack_from('>I', b, o)[0]
def be16(b, o): return struct.unpack_from('>H', b, o)[0]


class Xex:
    def __init__(self, path):
        self.path = path
        self.raw = open(path, 'rb').read()
        r = self.raw
        assert r[:4] == b'XEX2', 'not XEX2'
        self.module_flags, self.pe_off, _, self.sec_off, n = struct.unpack_from('>IIIII', r, 4)
        self.headers = {}
        for i in range(n):
            k, v = struct.unpack_from('>II', r, 0x18 + i * 8)
            self.headers[k] = v
        s = self.sec_off
        self.sec_header_size = be32(r, s)
        self.image_size = be32(r, s + 4)
        self.image_flags = be32(r, s + 0x10C)
        self.load_address = be32(r, s + 0x110)
        self.enc_key = r[s + 0x150:s + 0x160]
        self.region = be32(r, s + 0x178)
        self.media_types = be32(r, s + 0x17C)
        self.page_desc_count = be32(r, s + 0x180)
        self.page_descs = []
        for i in range(self.page_desc_count):
            o = s + 0x184 + i * 24
            v = be32(r, o)
            self.page_descs.append((v >> 4, v & 0xF, r[o + 4:o + 24]))

    def hdr_data(self, key):
        """Return the bytes of an optional header (offset-style keys)."""
        if key not in self.headers:
            return None
        v = self.headers[key]
        lo = key & 0xFF
        if lo in (0, 1):
            return v
        if lo == 0xFF:
            size = be32(self.raw, v)
            return self.raw[v:v + size]
        return self.raw[v:v + lo * 4]

    def describe(self):
        out = []
        out.append('module flags 0x%x  pe_off 0x%x  sec_off 0x%x  image size 0x%x  load 0x%08x  image flags 0x%x' % (
            self.module_flags, self.pe_off, self.sec_off, self.image_size, self.load_address, self.image_flags))
        for k, v in self.headers.items():
            out.append('  hdr %08x %-28s %08x' % (k, HDR_NAMES.get(k, '?'), v))
        ep = self.headers.get(0x00010100)
        if ep: out.append('  entry 0x%08x' % ep)
        sysf = self.headers.get(0x00030000)
        if sysf is not None: out.append('  system flags 0x%08x' % sysf)
        ei = self.hdr_data(0x00040006)
        if ei:
            media_id, ver, base_ver, title_id = struct.unpack_from('>IIII', ei, 0)
            out.append('  execution info: media 0x%08x version 0x%08x base 0x%08x title 0x%08x platform %d type %d disc %d/%d savegame 0x%08x' % (
                media_id, ver, base_ver, title_id, ei[16], ei[17], ei[18], ei[19], be32(ei, 20)))
        name = self.hdr_data(0x000183FF)
        if name: out.append('  original PE name: %r' % name[4:].split(b'\0')[0])
        return '\n'.join(out)

    def session_key(self, key):
        return AES.new(key, AES.MODE_ECB).decrypt(self.enc_key)

    def image(self):
        ffi = self.hdr_data(0x000003FF)
        enc_type, comp_type = struct.unpack_from('>HH', ffi, 4)
        data = self.raw[self.pe_off:]
        if enc_type == 1:
            n = len(data) & ~15
            for name, key in (('retail', RETAIL_KEY), ('devkit', DEVKIT_KEY)):
                sk = self.session_key(key)
                dec = AES.new(sk, AES.MODE_CBC, bytes(16)).decrypt(data[:n]) + data[n:]
                img = self._decompress(dec, ffi, comp_type)
                if img is not None and img[:2] == b'MZ':
                    self.key_used = name
                    return img
            raise RuntimeError('decryption failed (neither key gives an MZ image)')
        self.key_used = 'none'
        return self._decompress(data, ffi, comp_type)

    def _decompress(self, data, ffi, comp_type):
        if comp_type == 0:
            return data[:self.image_size]
        if comp_type == 1:  # basic: (data_size, zero_size) blocks
            nblk = (be32(ffi, 0) - 8) // 8
            out = bytearray()
            p = 0
            for i in range(nblk):
                ds, zs = struct.unpack_from('>II', ffi, 8 + i * 8)
                out += data[p:p + ds]
                out += bytes(zs)
                p += ds
            return bytes(out[:self.image_size]) + bytes(max(0, self.image_size - len(out)))
        if comp_type == 2:  # LZX, chained blocks of 2-byte-length chunks
            window = be32(ffi, 8)
            block_size = be32(ffi, 12)
            p = 0
            deblocked = bytearray()
            while block_size:
                pnext = p + block_size
                next_size = be32(data, p)
                p += 24
                while True:
                    cs = be16(data, p)
                    p += 2
                    if cs == 0:
                        break
                    deblocked += data[p:p + cs]
                    p += cs
                p = pnext
                block_size = next_size
                if p >= len(data):
                    break
            tin = '/tmp/_lzx_in.bin'
            tout = '/tmp/_lzx_out.bin'
            open(tin, 'wb').write(deblocked)
            r = subprocess.run([os.path.join(HERE, 'lzxmem'), tin, tout, str(window), str(self.image_size)],
                               capture_output=True, text=True)
            img = open(tout, 'rb').read()
            if r.returncode != 0:
                return None
            return img
        raise RuntimeError('compression type %d unsupported' % comp_type)


def pe_sections(img):
    """Parse the PE headers of a loaded XEX base image."""
    pe = struct.unpack_from('<I', img, 0x3C)[0]
    assert img[pe:pe + 4] == b'PE\0\0'
    machine, nsec, _, _, _, opt_size, chars = struct.unpack_from('<HHIIIHH', img, pe + 4)
    opt = pe + 24
    secs = []
    so = opt + opt_size
    for i in range(nsec):
        name = img[so:so + 8].split(b'\0')[0].decode('latin1')
        vsize, vaddr, rsize, raddr = struct.unpack_from('<IIII', img, so + 8)
        ch = struct.unpack_from('<I', img, so + 36)[0]
        secs.append((name, vaddr, vsize, raddr, rsize, ch))
        so += 40
    return machine, secs


if __name__ == '__main__':
    x = Xex(sys.argv[1])
    print(x.describe())
    img = x.image()
    print('  key: %s, image %d bytes, sha1 %s' % (x.key_used, len(img), hashlib.sha1(img).hexdigest()))
    m, secs = pe_sections(img)
    print('  PE machine 0x%x' % m)
    for s in secs:
        print('    %-8s va 0x%06x vsize 0x%06x raw 0x%06x rsize 0x%06x ch 0x%08x' % s)
    if len(sys.argv) > 2:
        open(sys.argv[2], 'wb').write(img)
