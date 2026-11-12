#!/usr/bin/env python3
"""Spooky Ghost - the live log over XBDM (Xbox 360).

Since step 6h.39 every log line Spooky Ghost writes also goes into one 4 KB page of memory with a "SPKYLIVE" header. On a
360 running XBDM (the debug monitor Xbox 360 Neighborhood talks to - on an RGH, usually Dashlaunch's xbdm plugin), this
reads that page over the network while you play and prints the lines as they're written. Nothing to install on the console.

  python spooky_live.py 192.168.1.50              follow the log: the file the 360's emulator keeps for the Xbox's T: drive
                                                   (HDD:\\Compatibility\\Xbox1\\TDATA\\4d530064\\spooky_log.txt) if it's
                                                   there - else the live page in memory (a long search, once);
                                                   everything is also saved to spooky_live_<date-time>.txt
  python spooky_live.py 192.168.1.50 --tail       the log file only (or --tail "<another path>")
  python spooky_live.py 192.168.1.50 --pull       copy what the mod keeps on T: (log, records, ghost seams, trace) into a
                                                   new local folder (or --pull <folder>; --with-spots: the spots too)
  python spooky_live.py 192.168.1.50 --spots <a spots folder>   replace the predicted spots the Monster/Dreadnought
                                                   Viewer reads (T:\\spots) - e.g. after a new scan
  python spooky_live.py 192.168.1.50 --put <local file or folder> "<console path>"   put any file / folder on the console
  python spooky_live.py 192.168.1.50 --deploy <patched default.xbe> [--boot] [--to "<the game's default.xbe>"]
                                                   put a new build in place of the game's XBE - no dashboard, no FTP: sent
                                                   under a new name, the game's own kept (.spooky_original), swapped in
                                                   (the console restarted to the dashboard first if the game holds the
                                                   file); --boot starts it. The game's path is found the first time
                                                   (a default.xbe whose certificate says Halo 2) and remembered
  python spooky_live.py 192.168.1.50 --restore    the game's own default.xbe back
  python spooky_live.py 192.168.1.50 --find-xbe   just find (and remember) the game's default.xbe
                                                    (the usual places first: v1.0 HDD:\\Games\\Halo 2, the v1.5 title update
                                                    HDD:\\Compatibility\\Xbox1\\TDATA\\4d530064\\$u; the drives searched only
                                                    when it isn't there - that takes a while)
  python spooky_live.py 192.168.1.50 --memory     the live page in memory only
  python spooky_live.py 192.168.1.50 --at 0x...    follow a page whose 360 address you already know
  python spooky_live.py 192.168.1.50 --files      list the drives, and look for spooky_log / TDATA\\4D530064 on them
  python spooky_live.py 192.168.1.50 --get "Hdd:\\path\\spooky_log.txt"   copy one file from the console
  python spooky_live.py 192.168.1.50 --delete                           delete the 360's log (the path as --tail's)
  python spooky_live.py 192.168.1.50 --delete "Hdd:\\path\\file"           delete one file on the console
  python spooky_live.py 192.168.1.50 --tail "Hdd:\\path\\spooky_log.txt"  follow a file on the console as it grows
  python spooky_live.py 192.168.1.50 --raw "walkmem"                      one XBDM command, its reply printed as is
  python spooky_live.py 192.168.1.50 --find                                only find the page (and remember it)
  python spooky_live.py 192.168.1.50 --probe                               how this XBDM answers, and how fast
  python spooky_live.py 192.168.1.50 --range C4000000 13000000             search just that range (hex: start, length)
  python spooky_live.py 192.168.1.50 --conns 8                             read over 8 connections at once (default 4)

Start Halo 2 with the mod first (the page appears with the first log line). The search reads each piece of the 360's
physical memory once - through the 0xC0000000 view of all 512 MB, read-write blocks first - and skips the virtual and
image ranges that are only other views of the same memory: 512 MB at most, usually far less (the emulated Xbox's RAM is
most likely in the read-write block at 0xC4000000). The address found is remembered in spooky_live_cache.txt next to
this script and tried first next time (checked before it's used).
Python 3, nothing else needed.
"""
import os, re, socket, struct, sys, time

# where the 360's backwards-compatibility emulator keeps the Xbox's T: drive for Halo 2 (title 4D530064) - found by --files
BC_TDATA = "HDD:\\Compatibility\\Xbox1\\TDATA\\4d530064"
BC_LOG = BC_TDATA + "\\spooky_log.txt"
MAGIC = b"SPKYLIVE"
MAGIC_SWAPPED = b"YKPSEVIL"      # the same, if the 360's emulator keeps the Xbox's memory as big-endian 32-bit words
PAGE = 4096
HEAD = 32


class Xbdm:
    """XBDM's text protocol on TCP 730: a command per line; replies '2xx- text', multi-line ones ended by a '.' line"""

    def __init__(self, ip, port=730, timeout=20.0):
        self.ip, self.port, self.timeout = ip, port, timeout
        self.memex = None                     # getmemex's reply format, once checked: "blocks", "raw", or "" (not usable)
        self._open()

    def _open(self):
        self.s = socket.create_connection((self.ip, self.port), self.timeout)
        self.f = self.s.makefile("rb")
        self.banner = self.f.readline().decode("latin-1").strip()        # "201- connected"

    def reopen(self):
        try:
            self.s.close()
        except OSError:
            pass
        self._open()

    def close(self):
        try:
            self.s.sendall(b"bye\r\n")
        except OSError:
            pass
        self.s.close()

    def _head(self):
        line = self.f.readline()
        if not line:
            raise ConnectionError("XBDM closed the connection")
        head = line.decode("latin-1").rstrip("\r\n")
        try:
            code = int(head[:3])
        except ValueError:
            raise ConnectionError("unexpected reply from XBDM: %r" % head)
        return code, head

    def lines(self):
        out = []
        while True:
            line = self.f.readline()
            if not line:
                raise ConnectionError("XBDM closed the connection mid-reply")
            t = line.decode("latin-1").rstrip("\r\n")
            if t == ".":
                return out
            out.append(t)

    def cmd(self, text):
        """one command: (code, head line, the lines of a multi-line reply or [])"""
        self.s.sendall(text.encode("latin-1") + b"\r\n")
        code, head = self._head()
        if code == 202 or ("follows" in head and code != 203):
            return code, head, self.lines()
        return code, head, []

    def binary(self, text, expect=None):
        """a command answered with '203- binary response follows', a 4-byte length (little-endian on the original Xbox's
        monitor; taken the other way round if that's implausible - the 360 is big-endian) and the data"""
        self.s.sendall(text.encode("latin-1") + b"\r\n")
        code, head = self._head()
        if code != 203:
            raise IOError("XBDM: %s" % head)
        raw = self._read(4)
        n = struct.unpack("<I", raw)[0]
        if n > (64 << 20) or (expect is not None and n > expect):
            n = struct.unpack(">I", raw)[0]
        return self._read(n)

    def sendfile(self, path, data):
        """put a file on the console: 'sendfile name=... length=...' - XBDM answers '204- send binary data', takes the
        bytes, then answers again"""
        self.s.sendall(('sendfile name="%s" length=0x%X\r\n' % (path, len(data))).encode("latin-1"))
        code, head = self._head()
        if code != 204:
            raise IOError("XBDM sendfile: %s" % head)
        self.s.sendall(data)
        code, head = self._head()
        if code // 100 != 2:
            raise IOError("XBDM sendfile: %s" % head)

    def rename(self, old, new):
        code, head, ls = self.cmd('rename name="%s" newname="%s"' % (old, new))
        if code // 100 != 2:
            raise IOError("XBDM rename: %s" % head)

    def delete(self, path):
        code, head, ls = self.cmd('delete name="%s"' % path)
        if code // 100 != 2:
            raise IOError("XBDM delete: %s" % head)

    def exists(self, path):
        try:
            self.filesize(path)
            return True
        except IOError:
            return False

    def magicboot(self, title=None, directory=None):
        """restart the console into a title (title=None: the dashboard); the connection drops"""
        t = "magicboot"
        if title:
            t += ' title="%s"' % title
            if directory:
                t += ' directory="%s"' % directory
        try:
            self.s.sendall((t + "\r\n").encode("latin-1"))
            self.s.settimeout(3.0)
            code, head = self._head()
            return head
        except (OSError, ConnectionError, socket.timeout):
            return "(the console went away - restarting)"

    def wait_back(self, log=print, timeout=120.0):
        """reconnect after a restart: XBDM comes back once the console is up again"""
        t0 = time.time()
        time.sleep(3.0)
        while time.time() - t0 < timeout:
            try:
                self._open()
                return True
            except (OSError, ConnectionError, socket.timeout):
                time.sleep(2.0)
        return False

    def mkdir(self, path):
        code, head, ls = self.cmd('mkdir name="%s"' % path)
        if code // 100 != 2 and "exist" not in head.lower():
            raise IOError("XBDM mkdir: %s" % head)

    def filesize(self, path):
        code, head, ls = self.cmd('getfileattributes name="%s"' % path)
        t = head + " " + " ".join(ls)
        hi = re.search(r"sizehi=0x([0-9a-fA-F]+)", t)
        lo = re.search(r"sizelo=0x([0-9a-fA-F]+)", t)
        if code // 100 != 2 or not lo:
            raise IOError("XBDM getfileattributes: %s" % head)
        return (int(hi.group(1), 16) << 32 if hi else 0) | int(lo.group(1), 16)

    def _read(self, n):
        b = b""
        while len(b) < n:
            c = self.f.read(n - len(b))
            if not c:
                raise ConnectionError("XBDM closed the connection mid-data")
            b += c
        return b

    def getmem(self, addr, length):
        """memory as bytes, with None for bytes XBDM couldn't read ('??')"""
        code, head, ls = self.cmd("getmem addr=0x%08X length=0x%X" % (addr, length))
        if code // 100 != 2:
            raise IOError("XBDM getmem: %s" % head)
        out = []
        for t in ls:
            t = t.strip()
            for i in range(0, len(t) - 1, 2):
                h = t[i:i + 2]
                out.append(None if h == "??" else int(h, 16))
        return out[:length]

    def getmemex(self, addr, length, fmt):
        """the same memory as binary (half the bytes of getmem's hex, and no hex formatting on the console):
        '203- binary response follows', then blocks each after a 2-byte header - the low 15 bits its length; on your 360's
        XBDM (its probe: "20 80" before 32 bytes) little-endian with bit 15 SET for readable data, fmt "blocks-v" (other
        layouts: "blocks" - bit 15 set for unreadable, "blocks-be", "raw" - no headers). Unreadable bytes: None"""
        self.s.sendall(("getmemex addr=0x%08X length=0x%X\r\n" % (addr, length)).encode("latin-1"))
        code, head = self._head()
        if code != 203:
            raise IOError("XBDM getmemex: %s" % head)
        if fmt == "raw":
            return list(self._read(length))
        out = []
        while len(out) < length:
            h = struct.unpack(">H" if fmt.startswith("blocks-be") else "<H", self._read(2))[0]
            n = h & 0x7FFF
            if n == 0 or n > 0x4000 or len(out) + n > length:
                raise ValueError("getmemex: an unexpected block header 0x%04X" % h)
            data = self._read(n)
            # bit 15: what it means isn't certain (your probe: set on a readable 32-byte block - "readable", or "the last
            # block"); "-any" ignores it - for the search, a misread unreadable block is only bytes that aren't the page
            bad = False if fmt.endswith("-any") else ((h & 0x8000) == 0 if fmt == "blocks-v" else (h & 0x8000) != 0)
            out.extend([None] * n if bad else data)
        return out

    def check_memex(self, log=print, addr=0x82000000, n=0x800):
        """is getmemex usable, and in which format? Compared byte for byte with getmem over the title's own image
        (0x82000000: always readable). Once per connection."""
        if self.memex is not None:
            return self.memex
        ref = self.getmem(addr, n)
        self.memex = ""
        for fmt in ("blocks-any", "blocks-be-any", "raw"):
            try:
                got = self.getmemex(addr, n, fmt)
                if got == ref:
                    self.memex = fmt
                    break
            except (IOError, ValueError, ConnectionError, socket.timeout):
                pass
            self.reopen()                                                    # (a misread reply leaves the stream out of step)
        log("binary memory reads (getmemex): %s" % ("yes - " + self.memex if self.memex else "no - hex reads (getmem), slower"))
        return self.memex

    def read(self, addr, length):
        """memory, the fastest way this XBDM answers correctly"""
        if self.memex:
            try:
                return self.getmemex(addr, length, self.memex)
            except (ValueError, IOError):
                self.reopen()
        return self.getmem(addr, length)

    def getbytes(self, addr, length):
        m = self.getmem(addr, length)
        if len(m) < length or any(b is None for b in m):
            return None
        return bytes(m)

    def regions(self):
        """(base, size, protect, phys) for each range walkmem lists (phys None if it doesn't say)"""
        code, head, ls = self.cmd("walkmem")
        out = []
        for t in ls:
            b = re.search(r"base=0x([0-9a-fA-F]+)", t)
            s = re.search(r"size=0x([0-9a-fA-F]+)", t)
            p = re.search(r"protect=0x([0-9a-fA-F]+)", t)
            ph = re.search(r"phys=0x([0-9a-fA-F]+)", t)
            if b and s:
                out.append((int(b.group(1), 16), int(s.group(1), 16), int(p.group(1), 16) if p else 4, int(ph.group(1), 16) if ph else None))
        return out


def unswap(b):
    """undo a big-endian 32-bit word layout: each group of 4 bytes reversed"""
    return b"".join(b[i:i + 4][::-1] for i in range(0, len(b) - len(b) % 4, 4))


def header_ok(h):
    if not h or len(h) < HEAD or h[:8] != MAGIC:
        return False
    seq, size, self_, ver = struct.unpack_from("<IIII", h, 8)
    return size == PAGE - HEAD and ver >= 1


RAM = 0x20000000                 # the 360's 512 MB of physical memory


def plan(regions, only=None):
    """what to search, in order. The 360's memory map: 0x00000000-0x7FFFFFFF virtual (4 KB / 64 KB pages), 0x80000000-
    0x9FFFFFFF images and code, then the same 512 MB of physical RAM three times over - 0xA0000000 (64 KB pages), 0xC0000000
    (16 MB pages), 0xE0000000 (4 KB pages). Every allocation, wherever it sits virtually, is in physical RAM: so each range
    is searched by its physical bytes, once - a range whose physical memory was already searched through another view is
    skipped. On your 360, walkmem lists 0xC0000000-0xDFFFFFFF whole: that view alone covers everything (512 MB, at most).
    Read-write first, biggest first (the emulated Xbox's 64 MB: most likely in the read-write block at 0xC4000000)."""
    if only:
        lo, n = only
        regions = [(max(b, lo), min(b + s, lo + n) - max(b, lo), p, (ph + max(b, lo) - b) if ph is not None else None)
                   for b, s, p, ph in regions if b < lo + n and b + s > lo]
    writable = lambda p: (p & 0x44) != 0                                      # PAGE_READWRITE / EXECUTE_READWRITE
    ram = [r for r in regions if r[3] is not None and r[3] + r[1] <= RAM]
    other = [r for r in regions if not (r[3] is not None and r[3] + r[1] <= RAM)]
    # the RAM ranges no other range contains (an equal one: the first of them, the physical views first)
    ram.sort(key=lambda r: (r[0] < 0xA0000000, -r[1]))
    keep = []
    for k, (b, s_, p, ph) in enumerate(ram):
        inside = any(ph2 <= ph and ph + s_ <= ph2 + s2 and (s2 > s_ or k2 < k) for k2, (b2, s2, p2, ph2) in enumerate(ram) if k2 != k)
        if not inside:
            keep.append((b, s_, p))
    keep.sort(key=lambda r: (not writable(r[2]), -r[1]))
    other = sorted(((b, s_, p) for b, s_, p, ph in other), key=lambda r: (not writable(r[2]), -r[1]))
    return keep + other


def scan_chunk(m, a):
    """the live log page's header in one read's bytes, at a page start: (address, swapped) or None"""
    for off in range(0, len(m) - HEAD + 1, PAGE):
        c = m[off]
        if c != 0x53 and c != 0x59:                                           # 'S' / 'Y'
            continue
        h = bytes(b if b is not None else 0 for b in m[off:off + HEAD])
        if h[:8] == MAGIC and header_ok(h):
            return a + off, False
        if h[:8] == MAGIC_SWAPPED and header_ok(unswap(h)):
            return a + off, True
    return None


def find_page(x, log=print, only=None, chunk=0x10000, conns=4):
    """the 360 address of the live log page, and whether its words are byte-swapped - or (None, False). Read over
    `conns` connections at once (each read is mostly waiting on the console); the address is remembered afterwards."""
    import threading
    all_regions = x.regions()
    if not all_regions:
        raise IOError("XBDM's walkmem listed no memory - is a game running?")
    regions = plan(all_regions, only)
    skipped = sum(r[1] for r in all_regions) - sum(r[1] for r in regions)
    if skipped > 0 and not only:
        log("(%d MB of the %d listed skipped: the same physical memory seen through another view)" % (skipped >> 20, sum(r[1] for r in all_regions) >> 20))
    total = sum(s_ for _, s_, _ in regions)
    log("searching %d MB of the console's memory (%d ranges, read-write first) for the live log page..." % (total >> 20, len(regions)))
    fmt = x.check_memex(log)
    jobs = []
    for base, size, prot in regions:
        a = base - (base % PAGE)
        while a < base + size:
            n = min(chunk, base + size - a)
            jobs.append((a, n))
            a += n
    jobs.reverse()                                                            # (popped from the end: in order)
    lock = threading.Lock()
    state = {"found": None, "done": 0, "next": 2 << 20, "first": False, "err": None, "at": jobs[-1][0] if jobs else 0}
    t0 = time.time()

    def worker(c):
        while True:
            with lock:
                if state["found"] or not jobs or state["err"]:
                    return
                a, n = jobs.pop()
                state["at"] = a
            try:
                m = c.read(a, n)
            except (IOError, ValueError, ConnectionError, socket.timeout) as e:
                try:
                    c.reopen(); c.memex = fmt
                    m = c.read(a, n)
                except Exception as e2:                                       # give up on this one: say so
                    with lock:
                        state["err"] = "a read at 0x%08X failed: %s" % (a, e2)
                    return
            hit = scan_chunk(m, a) if m else None
            if len(m) < n and len(m) >= PAGE:                                 # the rest of a short read, again
                with lock:
                    jobs.append((a + len(m) - len(m) % PAGE, n - (len(m) - len(m) % PAGE)))
            with lock:
                if hit and not state["found"]:
                    state["found"] = hit
                state["done"] += len(m)
                if not state["first"]:
                    state["first"] = True
                    dt = max(time.time() - t0, 0.001)
                    log("  the first %d KB took %.2f s on %d connections: this search, %.0f minutes at most" % (n >> 10, dt, conns, total / (n * conns / dt) / 60))
                if state["done"] >= state["next"]:
                    rate = state["done"] / max(time.time() - t0, 0.001)
                    log("  %d of %d MB, near 0x%08X (%.0f KB/s, %.0f s to go at most)" % (state["done"] >> 20, total >> 20, a, rate / 1024, (total - state["done"]) / rate))
                    state["next"] += 8 << 20

    cs = [x]
    for k in range(conns - 1):
        try:
            c = Xbdm(x.ip, x.port, x.timeout)
            c.memex = fmt
            cs.append(c)
        except (OSError, ConnectionError):
            break
    ts = [threading.Thread(target=worker, args=(c,), daemon=True) for c in cs]
    for t in ts:
        t.start()
    try:
        while any(t.is_alive() for t in ts):
            time.sleep(0.1)
    except KeyboardInterrupt:
        with lock:
            a = state["at"]
            jobs.clear()
        log("stopped near 0x%08X - to go on from there:  --range %08X <length>" % (a, a))
        raise
    finally:
        for c in cs[1:]:
            try:
                c.close()
            except OSError:
                pass
    if state["err"]:
        log("  " + state["err"])
    if state["found"]:
        addr, sw = state["found"]
        log("found it at 0x%08X%s (%.0f s)" % (addr, ", as big-endian words" if sw else "", time.time() - t0))
        return addr, sw
    return None, False


def cache_path():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), "spooky_live_cache.txt")


def cached(ip):
    try:
        for line in open(cache_path()):
            f = line.split()
            if f and f[0] == ip and not f[1].startswith("xbe"):
                return int(f[1], 16), len(f) > 2 and f[2] == "swapped"
    except (OSError, ValueError, IndexError):
        pass
    return None, False


def remember(ip, addr, swapped):
    lines = []
    try:
        lines = [l for l in open(cache_path()) if not l.startswith(ip + " ") or l.startswith(ip + " xbe")]
    except OSError:
        pass
    lines.append("%s 0x%08X%s\n" % (ip, addr, " swapped" if swapped else ""))
    open(cache_path(), "w").writelines(lines)


def cached_xbe(ip, version=None):
    """the game's path remembered for this console - one a version (an entry without a version: any)"""
    try:
        for line in open(cache_path()):
            f = line.rstrip("\n").split(" ", 2)
            if len(f) == 3 and f[0] == ip and f[1].startswith("xbe"):
                v = int(f[1][3:], 16) if len(f[1]) > 3 else None
                if version is None or v is None or v == version:
                    return f[2]
    except (OSError, ValueError):
        pass
    return None


def remember_xbe(ip, path, version=None):
    key = "xbe" if version is None else "xbe%08X" % version
    lines = []
    try:
        lines = [l for l in open(cache_path()) if not l.startswith(ip + " " + key + " ")]
    except OSError:
        pass
    lines.append("%s %s %s\n" % (ip, key, path))
    open(cache_path(), "w").writelines(lines)


def read_page(x, addr, swapped, n=PAGE):
    m = x.read(addr, n)
    b = bytes(m) if len(m) >= n and None not in m else None
    return unswap(b) if (b and swapped) else b


def follow(x, addr, out, poll=0.25, once=False, log=print, swapped=False):
    """print the ring's lines as they're written; returns when the page is gone (game restarted)"""
    last = None
    while True:
        page = read_page(x, addr, swapped)
        if not page or not header_ok(page):
            return False
        seq, size = struct.unpack_from("<II", page, 8)
        data = page[HEAD:HEAD + size]
        if last is None:
            new = min(seq, size)                                    # what's in it now
        else:
            new = seq - last
            if new < 0:                                             # restarted (the mod's count began again)
                new = min(seq, size)
            elif new > size:
                log("[... %d bytes came and went between two reads ...]" % (new - size))
                new = size
        if new:
            start = (seq - new) % size
            chunk = data[start:start + new] if start + new <= size else data[start:] + data[:(start + new) - size]
            text = chunk.decode("latin-1")
            sys.stdout.write(text)
            sys.stdout.flush()
            if out:
                out.write(text)
                out.flush()
        last = seq
        if once:
            return True
        time.sleep(poll)


def tail_file(x, path, out, poll=1.0, log=print, once=False, have=0):
    """follow a file on the console as it grows (the Xbox's T:\\spooky_log.txt, if XBDM can see it): its size each
    second, and only the new part fetched (getfile's offset / size - or, if this XBDM doesn't take them, the whole file)"""
    partial = True
    while True:
        try:
            size = x.filesize(path)
        except (IOError, ConnectionError, socket.timeout) as e:
            log("[%s - retrying]" % e); x.reopen(); time.sleep(poll); continue
        if size < have:                                                # a new log (the game restarted)
            log("--- the file started again ---"); have = 0
            if out:                                                      # (marked in the saved copy too)
                out.write("--- the file started again (the game restarted) ---\n"); out.flush()
        if size > have:
            data = None
            if partial:
                try:
                    data = x.binary('getfile name="%s" offset=0x%X size=0x%X' % (path, have, size - have), expect=size - have)
                    if len(data) != size - have:
                        raise IOError("short")
                except (IOError, ConnectionError, socket.timeout):
                    partial, data = False, None; x.reopen()
            if data is None:
                data = x.binary('getfile name="%s"' % path)[have:size]
            text = data.decode("latin-1").replace("\r\n", "\n")
            sys.stdout.write(text); sys.stdout.flush()
            if out:
                out.write(text); out.flush()
            have = size
        if once:
            return have
        time.sleep(poll)


def pull(x, dest, log=print, spots=False):
    """copy what Spooky Ghost keeps on the Xbox's T: drive (on the 360: HDD:\\Compatibility\\Xbox1\\TDATA\\4d530064 - the
    FTP's /Hdd1/Compatibility/...) into a local folder: the log, the records, the ghost seams, the crash trace - and the
    spots folder (4.4 MB, it doesn't change) only if asked"""
    os.makedirs(dest, exist_ok=True)
    got = []

    def walk(remote, local):
        code, head, ls = x.cmd('dirlist name="%s"' % remote)
        if code // 100 != 2:
            log("  %s: %s" % (remote, head)); return
        for t in ls:
            m = re.search(r'name="([^"]+)"', t)
            if not m:
                continue
            name = m.group(1)
            if "directory" in t:
                if name.lower() == "spots" and not spots:
                    log("  %-60s (skipped - --with-spots copies it too)" % (remote + "\\" + name))
                    continue
                if name.startswith("$") or name.lower() in ("saves", "savemeta"):
                    continue                                   # the title update ($u) and the game's own saves: not the mod's
                os.makedirs(os.path.join(local, name), exist_ok=True)
                walk(remote + "\\" + name, os.path.join(local, name))
            else:
                if not name.lower().startswith(("spooky", "ghost", "crash", "sb_", "monster", "dread")):
                    continue                                   # only the mod's own files
                data = x.binary('getfile name="%s"' % (remote + "\\" + name))
                open(os.path.join(local, name), "wb").write(data)
                got.append((remote + "\\" + name, len(data)))
                log("  %-60s %8d bytes" % (remote + "\\" + name, len(data)))
    walk(BC_TDATA, dest)
    return got


def put_tree(x, local, remote, log=print):
    """put a local file, or a whole folder, on the console (folders made as needed)"""
    if os.path.isdir(local):
        x.mkdir(remote)
        n = 0
        for name in sorted(os.listdir(local)):
            n += put_tree(x, os.path.join(local, name), remote + "\\" + name, log)
        return n
    data = open(local, "rb").read()
    x.sendfile(remote, data)
    log("  %-60s %8d bytes" % (remote, len(data)))
    return 1


TITLE_ID = 0x4D530064                         # Halo 2
XBE_V10, XBE_V15 = 0x4158CC50, 0x4600A62B     # the image timestamps: the v1.0 disc (the TU Skip build too), the v1.5 title update
XBE_NAMES = {XBE_V10: "v1.0", XBE_V15: "v1.5 (the title update)"}
DEFAULT_XBES = ((XBE_V10, ["HDD:\\Games\\Halo 2\\default.xbe"]),
                (XBE_V15, ["HDD:\\Compatibility\\Xbox1\\TDATA\\4d530064\\$u\\default.xbe"]))   # where they usually are


def xbe_title(head):
    """an XBE's title ID from its first bytes (the certificate's address is in the header), or None"""
    if len(head) < 0x178 or head[:4] != b"XBEH":
        return None
    base, cert = struct.unpack_from("<I", head, 0x104)[0], struct.unpack_from("<I", head, 0x118)[0]
    o = cert - base
    if 0 <= o and o + 12 <= len(head):
        return struct.unpack_from("<I", head, o + 8)[0]
    return None


def xbe_version(head):
    """an XBE's image timestamp (Halo 2 v1.0: XBE_V10; the v1.5 title update: XBE_V15), or None"""
    return struct.unpack_from("<I", head, 0x114)[0] if len(head) >= 0x178 and head[:4] == b"XBEH" else None


def xbe_head(x, path):
    """the first 4 KB of a file on the console, b"" if it isn't there"""
    try:
        return x.binary('getfile name="%s" offset=0x0 size=0x1000' % path, expect=0x1000)
    except (IOError, ConnectionError, socket.timeout):
        x.reopen()
        return b""


def find_xbe(x, log=print, version=None):
    """the game's default.xbe on the console. The usual places first - v1.0 (and the TU Skip build): HDD:\\Games\\Halo 2;
    the v1.5 title update: HDD:\\Compatibility\\Xbox1\\TDATA\\4d530064\\$u - checked by their certificate (Halo 2) and,
    when the build being sent says which, their version. Only when it isn't there: every default.xbe under the drives'
    Games / Apps / Xbox1 / Content folders (and a level below), which takes a while."""
    found = []
    for v, paths in DEFAULT_XBES:
        if version is not None and v != version:
            continue
        for p in paths:
            h = xbe_head(x, p)
            if h and xbe_title(h) == TITLE_ID and (version is None or xbe_version(h) == version):
                log("  %s  (where it usually is%s)" % (p, "" if xbe_version(h) is None else " - %s" % XBE_NAMES.get(xbe_version(h), "another version")))
                found.append((p, TITLE_ID))
    if len(found) == 1:
        return found[0][0], found
    if found:
        return None, found
    log("  not where it usually is - searching the drives (this takes a while)")
    code, head, ls = x.cmd("drivelist")
    drives = re.findall(r'drivename="([^"]+)"', "\n".join(ls)) or ["HDD"]
    found = []

    def look(path, depth):
        code, head, ls = x.cmd('dirlist name="%s"' % path)
        if code // 100 != 2:
            return
        for t in ls:
            m = re.search(r'name="([^"]+)"', t)
            if not m:
                continue
            name = m.group(1); full = path.rstrip("\\") + "\\" + name
            if "directory" in t:
                if depth < 2:
                    look(full, depth + 1)
            elif name.lower() == "default.xbe":
                h = xbe_head(x, full)
                tid = xbe_title(h)
                if tid == TITLE_ID and version is not None and xbe_version(h) != version:
                    log("  %s  (Halo 2, but %s)" % (full, XBE_NAMES.get(xbe_version(h), "another version")))
                    continue
                log("  %s%s" % (full, "" if tid is None else "  (title %08X%s)" % (tid, " - Halo 2" if tid == TITLE_ID else "")))
                found.append((full, tid))
    for d in drives:
        for top in ("Games", "Apps", "Xbox1", "Content"):
            look("%s:\\%s" % (d, top), 1)
    halo = [p for p, t in found if t == TITLE_ID]
    return halo[0] if len(halo) == 1 else None, found


def deploy(x, local, remote, boot=False, log=print):
    """the new default.xbe in place of the game's: sent under a new name, the game's first one kept beside it
    (default.xbe.spooky_original - sent back with --restore), then swapped in by renaming. If the game is running and the
    console won't let its file be replaced, the console is restarted to the dashboard first and the swap done then."""
    data = open(local, "rb").read()
    tid = xbe_title(data[:0x2000])
    if tid != TITLE_ID:
        raise IOError("%s isn't a Halo 2 default.xbe (title %s)" % (local, "none" if tid is None else "%08X" % tid))
    h = xbe_head(x, remote)
    if h and xbe_title(h) == TITLE_ID and xbe_version(h) is not None and xbe_version(data[:0x1000]) is not None and xbe_version(h) != xbe_version(data[:0x1000]):
        raise IOError("%s is Halo 2 %s; %s is a %s build - not put over it (give the right file, or --to the other game file)"
                      % (remote, XBE_NAMES.get(xbe_version(h), "?"), local, XBE_NAMES.get(xbe_version(data[:0x1000]), "?")))
    new, orig = remote + ".spooky_new", remote + ".spooky_original"
    t0 = time.time()
    log("sending %s (%.1f MB) as %s ..." % (local, len(data) / 2 ** 20, new))
    x.sendfile(new, data)
    dt = max(time.time() - t0, 0.001)
    log("  sent in %.1f s (%.0f KB/s)" % (dt, len(data) / dt / 1024))

    def swap():
        if not x.exists(orig):
            x.rename(remote, orig)                                            # the game's own, kept once
        elif x.exists(remote):
            x.delete(remote)
        x.rename(new, remote)
    try:
        swap()
    except IOError as e:
        log("  the swap: %s - the game has its file open; restarting the console to the dashboard to do it" % e)
        x.magicboot()
        if not x.wait_back(log):
            raise IOError("the console didn't come back on XBDM within 2 minutes")
        swap()
    log("  in place: %s (the game's own kept as %s)" % (remote, orig))
    if boot:
        d = remote.rsplit("\\", 1)[0]
        log("starting it: %s" % x.magicboot(remote, d))


def restore(x, remote, log=print):
    orig = remote + ".spooky_original"
    if not x.exists(orig):
        raise IOError("no %s on the console" % orig)
    try:
        if x.exists(remote):
            x.delete(remote)
        x.rename(orig, remote)
    except IOError as e:
        log("  %s - restarting the console to the dashboard first" % e)
        x.magicboot()
        if not x.wait_back(log):
            raise
        if x.exists(remote):
            x.delete(remote)
        x.rename(orig, remote)
    log("the game's own default.xbe is back: %s" % remote)


def probe(x, log=print):
    """what this XBDM answers, and how fast: a few reads timed, hex and binary"""
    for cmd in ("dbgname", "systeminfo"):
        try:
            code, head, ls = x.cmd(cmd)
            log("%s: %s %s" % (cmd, head, " | ".join(ls[:4])))
        except (IOError, ConnectionError, socket.timeout) as e:
            log("%s: %s" % (cmd, e)); x.reopen()
    m = x.getmem(0x82000000, 16)
    log("the title's image at 0x82000000 starts: %s" % " ".join("??" if b is None else "%02X" % b for b in m))
    # what getmemex sends, as it comes (its first 48 bytes after the reply line)
    try:
        x.s.sendall(b"getmemex addr=0x82000000 length=0x20\r\n")
        code, head = x._head()
        log("getmemex's reply: %s" % head)
        if code == 203:
            x.s.settimeout(3.0)
            raw = b""
            try:
                while len(raw) < 48:
                    c = x.f.read1(48 - len(raw)) if hasattr(x.f, "read1") else x.f.read(48 - len(raw))
                    if not c:
                        break
                    raw += c
            except socket.timeout:
                pass
            x.s.settimeout(x.timeout)
            log("  its first %d bytes: %s" % (len(raw), " ".join("%02X" % b for b in raw)))
    except (IOError, ConnectionError, socket.timeout) as e:
        log("getmemex: %s" % e)
    x.reopen()
    t0 = time.time()
    for k in range(8):
        x.getmem(0xC4000000 + k * 0x1000, 16)
    log("a 16-byte read (one line): %.0f ms each" % ((time.time() - t0) / 8 * 1000))
    fmt = x.check_memex(log)
    for addr in (0x82000000, 0xC4000000):
        for n in (0x1000, 0x10000):
            t0 = time.time()
            try:
                m = x.getmem(addr, n)
                dt = time.time() - t0
                log("getmem   0x%08X 0x%05X: %.2f s (%.0f KB/s), %d of %d bytes back, %d unreadable" % (addr, n, dt, n / max(dt, 0.001) / 1024, len(m), n, m.count(None)))
            except (IOError, ConnectionError, socket.timeout) as e:
                log("getmem   0x%08X 0x%05X: %s" % (addr, n, e)); x.reopen()
            if fmt:
                t0 = time.time()
                try:
                    m = x.getmemex(addr, n, fmt)
                    dt = time.time() - t0
                    log("getmemex 0x%08X 0x%05X: %.2f s (%.0f KB/s), %d of %d bytes back, %d unreadable" % (addr, n, dt, n / max(dt, 0.001) / 1024, len(m), n, m.count(None)))
                except (IOError, ValueError, ConnectionError, socket.timeout) as e:
                    log("getmemex 0x%08X 0x%05X: %s" % (addr, n, e)); x.reopen()


def files(x, log=print):
    code, head, ls = x.cmd("drivelist")
    drives = re.findall(r'drivename="([^"]+)"', "\n".join(ls))
    log("drives: " + (", ".join(drives) if drives else "(none listed: %s)" % head))
    hits = []

    calls = [0]

    def walk(path, depth):
        if depth > 5 or calls[0] > 600:
            return
        calls[0] += 1
        try:
            code, head, ls = x.cmd('dirlist name="%s"' % path)
        except (IOError, ConnectionError, socket.timeout):
            x.reopen(); return
        for t in ls:
            m = re.search(r'name="([^"]+)"', t)
            if not m:
                continue
            name = m.group(1)
            full = path.rstrip("\\") + "\\" + name
            low = name.lower()
            if "spooky" in low or "4d530064" in low or low == "tdata":
                hits.append(full)
                log("  " + full)
            if "directory" in t and (depth < 3 or any(k in low for k in ("tdata", "udata", "4d530064", "compat", "xbox1", "xbox", "content", "cache", "game"))):
                walk(full, depth + 1)

    for d in drives:
        walk(d + ":\\", 0)
    if not hits:
        log("no spooky_log / TDATA found on the drives XBDM lists - use the live page instead (no --files)")
    return hits


def main(argv):
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print(__doc__)
        return 0
    # 10 Nov: `h2test.py live --tail 40` failed with "getaddrinfo failed" - the first argument was taken as the console's
    # address whatever it was. An argument that starts with "-" is an option: the address is then the usual 192.168.0.178
    # (as for every other h2test tool).
    if argv[1].startswith("-"):
        argv = [argv[0], "192.168.0.178"] + argv[1:]
    ip, port = argv[1], 730
    if ":" in ip:                                                             # (ip:port - the tests' pretend XBDM)
        ip, port = ip.rsplit(":", 1)[0], int(ip.rsplit(":", 1)[1])
    x = Xbdm(ip, port)
    print("connected to %s: %s" % (ip, x.banner))
    try:
        if "--raw" in argv:
            code, head, ls = x.cmd(argv[argv.index("--raw") + 1])
            print(head)
            for t in ls:
                print(t)
            return 0
        if "--files" in argv:
            files(x)
            return 0
        if "--probe" in argv:
            probe(x)
            return 0
        if "--find-xbe" in argv:
            xbe, found = find_xbe(x)
            print("Halo 2's default.xbe: %s" % (xbe or ("more than one - give the one to use with --to" if found else "not found - give it with --to")))
            if xbe:
                remember_xbe(ip, xbe, xbe_version(xbe_head(x, xbe)))
            return 0
        if "--deploy" in argv or "--restore" in argv:
            version = None
            if "--deploy" in argv:
                version = xbe_version(open(argv[argv.index("--deploy") + 1], "rb").read(0x1000))   # the build being sent says which game file
            remote = argv[argv.index("--to") + 1] if "--to" in argv else cached_xbe(ip, version)
            if remote and "--to" not in argv:
                h = xbe_head(x, remote)
                if not (h and xbe_title(h) == TITLE_ID and (version is None or xbe_version(h) == version)):
                    print("the remembered %s isn't Halo 2%s any more - looking again" % (remote, "" if version is None else " " + XBE_NAMES.get(version, "")))
                    remote = None
            if not remote:
                print("looking for Halo 2's%s default.xbe on the console:" % ("" if version is None else " " + XBE_NAMES.get(version, "")))
                remote, found = find_xbe(x, version=version)
                if not remote:
                    print("%s - give its path:  --to \"HDD:\\Games\\Halo 2\\default.xbe\"" % ("more than one" if found else "not found"))
                    return 1
            remember_xbe(ip, remote, version)
            if "--restore" in argv:
                restore(x, remote)
            else:
                deploy(x, argv[argv.index("--deploy") + 1], remote, boot="--boot" in argv)
            return 0
        if "--boot" in argv:
            remote = argv[argv.index("--to") + 1] if "--to" in argv else cached_xbe(ip)
            print("starting %s: %s" % (remote, x.magicboot(remote, remote.rsplit("\\", 1)[0])) if remote else "no game path yet - --find-xbe or --to")
            return 0
        if "--pull" in argv:
            i = argv.index("--pull")
            dest = argv[i + 1] if i + 1 < len(argv) and not argv[i + 1].startswith("--") else "spooky_t_drive_%s" % time.strftime("%Y%m%d-%H%M%S")
            print("copying %s into %s:" % (BC_TDATA, dest))
            got = pull(x, dest, spots="--with-spots" in argv)
            print("%d files" % len(got))
            return 0
        if "--put" in argv:
            i = argv.index("--put")
            local, remote = argv[i + 1], argv[i + 2]
            print("putting %s on the console as %s:" % (local, remote))
            print("%d files" % put_tree(x, local, remote))
            return 0
        if "--spots" in argv:
            i = argv.index("--spots")
            local = argv[i + 1]
            if not os.path.isfile(os.path.join(local, "index.bin")):
                print("%s has no index.bin - give the spots folder from the Spooky Ghost package" % local)
                return 1
            print("putting the predicted spots (%s) where the Monster/Dreadnought Viewer reads them - T:\\spots on the 360:" % local)
            print("%d files" % put_tree(x, local, BC_TDATA + "\\spots"))
            return 0
        if "--tail" in argv:
            i = argv.index("--tail")
            path = argv[i + 1] if i + 1 < len(argv) and not argv[i + 1].startswith("--") else BC_LOG
            out = open("spooky_live_%s.txt" % time.strftime("%Y%m%d-%H%M%S"), "w", encoding="latin-1")
            print("--- following %s on the console (Ctrl+C to stop) ---" % path)
            tail_file(x, path, out)
            return 0
        if "--delete" in argv:                                                # 6i.78: delete a file (the 360's log by default)
            i = argv.index("--delete")
            path = argv[i + 1] if i + 1 < len(argv) and not argv[i + 1].startswith("--") else BC_LOG
            x.delete(path)
            print("deleted %s" % path)
            return 0
        if "--get" in argv:
            path = argv[argv.index("--get") + 1]
            data = x.binary('getfile name="%s"' % path)
            name = path.split("\\")[-1] or "file.bin"
            open(name, "wb").write(data)
            print("saved %s (%d bytes)" % (name, len(data)))
            return 0
        only = None
        if "--range" in argv:
            i = argv.index("--range")
            only = (int(argv[i + 1], 16), int(argv[i + 2], 16))
        chunk = int(argv[argv.index("--chunk") + 1], 16) if "--chunk" in argv else 0x10000
        conns = int(argv[argv.index("--conns") + 1]) if "--conns" in argv else 4
        if "--at" in argv:
            addr, swapped = int(argv[argv.index("--at") + 1], 16), "--swapped" in argv
        else:
            addr, swapped = cached(ip)
        if "--find" in argv:
            addr, swapped = find_page(x, only=only, chunk=chunk, conns=conns)
            print("not found" if addr is None else "the page: 0x%08X%s" % (addr, " (big-endian words)" if swapped else ""))
            if addr is not None:
                remember(ip, addr, swapped)
            return 0
        out = open("spooky_live_%s.txt" % time.strftime("%Y%m%d-%H%M%S"), "w", encoding="latin-1")
        if "--at" not in argv and "--range" not in argv and "--memory" not in argv:
            # the log file on the console's drive, if the emulator keeps it where your 360 does: just follow it
            try:
                x.filesize(BC_LOG)
                print("--- following %s (Ctrl+C to stop) ---" % BC_LOG)
                tail_file(x, BC_LOG, out)
                return 0
            except (IOError, ConnectionError, socket.timeout):
                x.reopen()
                print("(no %s - looking for the live log page in memory instead)" % BC_LOG)
        while True:
            if addr is not None and not header_ok(read_page(x, addr, swapped, HEAD)):
                addr = None
            if addr is None:
                addr, swapped = find_page(x, only=only, chunk=chunk, conns=conns)
                if addr is None:
                    print("not found - is Halo 2 running with Spooky Ghost 6h.39 or later? (retrying in 10 s)")
                    time.sleep(10)
                    continue
                remember(ip, addr, swapped)
            print("--- following the live log at 0x%08X (Ctrl+C to stop) ---" % addr)
            follow(x, addr, out, swapped=swapped)
            print("--- the page is gone (the game restarted?) - looking again ---")
            addr = None
    except KeyboardInterrupt:
        print()
        return 0
    finally:
        x.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv))
