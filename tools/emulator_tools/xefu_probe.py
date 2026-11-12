#!/usr/bin/env python3
"""xefu_probe - watch Halo 2's 360 emulator (xefu1_1.xex) over XBDM. Read-only: it never writes to the console.

  python xefu_probe.py 192.168.0.178                       which emulator build runs + every thread (hardware
                                                            thread, priority, what it is)
  python xefu_probe.py 192.168.0.178 --x86sample 30 --label zanzibar_dip
                                                            30 s of what the emulated Xbox CPU does: idle loop,
                                                            interrupts, emulator waits (works on any build)
  python xefu_probe.py 192.168.0.178 --measure 30 --label beaver_creek
                                                            30 s of the emulator's GPU thread, frame by frame:
                                                            how long each frame takes and where the time goes;
                                                            prints a line a second, saves everything to a log

Needs one of the timing builds (xefu1_1_T / _A / _B) for the frame split; on the stock emulator, build 0, or a
stock xefu5 / xefu7 copied over xefu1_1.xex it counts frames only. Python 3, nothing else to install.
"""
import argparse
import datetime
import re
import socket
import struct
import sys
import time

EMUS = {(0x0016F315, 0x4537DDE5): ('xefu1_1', 0x8216F64C),   # (check, timestamp): name, its own frame counter
        (0x00177DA9, 0x44F633D3): ('xefu5', 0x8217024C),
        (0x0017EBE9, 0x474B3577): ('xefu7', 0x82191970)}
BLK = 0x8251F000                    # marker + counters (xefu1_1 timing builds)
RING_N = 64
TB_MS = 49875.0                     # 360 timebase ticks per millisecond
ROLES = {0x820CDA50: 'GPU thread (Halo 2 GPU commands -> 360 GPU)', 0x820326D8: 'service thread (x86 requests)',
         0x820326E8: 'async service thread', 0x8203D420: 'emulator worker (low priority)', 0x820E2450: 'audio thread',
         0x820233B0: 'file-write thread', 0x8210EC68: '360 D3D callback thread', 0x820284D0: 'system notification listener',
         0x816A0400: 'main thread (x86 CPU: Halo 2 itself)'}
EXPECT = {'0': {}, 'T': {}, 'A': {0x8210EC68: 4}, 'B': {0x8210EC68: 4, 0x820326D8: 4},
          'T2': {}, 'B2': {0x8210EC68: 4, 0x820326D8: 4}, 'T3': {}, 'B3': {0x8210EC68: 4, 0x820326D8: 4}, 'T4': {}, 'T5': {}, 'F6': {}, 'D7': {}, 'F7': {}, 'F8': {}, 'F9': {}, 'F10': {}, 'F11': {}, 'B11': {0x8210EC68: 4, 0x820326D8: 4}, 'F12': {}, 'F13': {}, 'F14': {}, 'F15': {}, 'F16': {}, 'F17': {}, 'F18': {}, 'F19': {}, 'F20': {}, 'F21': {}, 'F22': {}, 'R22': {}, 'Q22': {}, 'N22': {}, 'L22': {}, 'P22': {}, 'Y22': {}, 'S22': {}, 'Z22': {}, 'T22': {}, 'U22': {}, 'V22': {}, 'W22': {}, 'X22': {}, 'K22': {}, 'F24': {}}
VARIANTS = {'0': 'build 0 (unchanged emulator, rebuilt file)', 'T': 'build T (timing)',
            'A': 'build A (timing + D3D callback thread on 4 + polite service thread)',
            'B': 'build B (A + service thread on 4)',
            'T2': 'build T2 (timing, round 2)', 'B2': 'build B2 (round-2 timing + B thread placement)',
            'T3': 'build T3 (timing, round 3)', 'B3': 'build B3 (round-3 timing + B thread placement)',
            'T4': 'build T4 (round-3 timing + a timer on every call the stream flush and render-target bind make)',
            'T5': 'build T5 (round 4 + timers one level deeper + the stream-buffer search loop counted)',
            'F6': 'build F6 (FIX: unaligned vertex streams realigned on the CPU, not by GPU copy passes) + round-5 timers',
            'D7': 'build D7 (stock rendering + round-5 timers + a capture of what the GPU copy pass writes)',
            'F7': 'build F7 (FIX try 2: CPU realign with each 32-bit word byte-swapped) + round-5 timers',
            'F8': 'build F8 (FIX: CPU copy exactly as the GPU pass writes it, padding included) + round-5 timers',
            'F9': 'build F9 (FIX: F8 copy into its own rotating area, never reused while the GPU may read it) + timers',
            'F10': 'build F10 (FIX: F9 with the copy-block address corrected, plus fall-back guards) + timers',
            'F11': 'build F11 (F10 + timers on the main command loop)',
            'B11': 'build B11 (F11 + B thread placement)',
            'F12': 'build F12 (F11 + timers inside the per-draw state setup and D3D state flush)',
            'F13': 'build F13 (F12 + timers on texture tracking, D3D constant flush and the surface page table)',
            'F14': 'build F14 (F13 + timers on the indirect calls inside the render-target bind)',
            'F15': 'build F15 (F14 + timers inside the GPU texture converters + which textures get converted)',
            'F16': 'build F16 (F15 + texture fingerprints at each conversion + which surfaces get invalidated)',
            'F17': 'build F17 (FIX: textures in use stay cached - no more re-converting them) + F16 measurements',
            'F18': 'build F18 (records what the texture version check compared at every conversion)',
            'F19': 'build F19 (F18 + the shared linked surface\'s memory range + a log of every 2D blit)',
            'F20': 'build F20 (FIX: texture palettes no longer invalidated by nearby render targets) + F19 measurements',
            'F21': 'build F21 (F20 + render targets no longer invalidate texture palettes; skips logged)',
            'F22': 'build F22 (FIX: a palette only counts as changed when its bytes changed) + measurements',
            'R22': 'release build R23 (the fixes only: F10 vertex copy, F20/F22 palette fixes; per-frame timers)',
            'Q22': 'build R28 (R23 + a 1024-entry async request queue, was 64)',
            'N22': 'build D29 (R28 + no AA - measurement only)',
            'L22': 'build L30 (R28 + timers on the emulator\'s async file-system thread)',
            'P22': 'build P31 (R23 + x86 profiler: where Halo 2\'s code spends its time)',
            'Y22': 'build P33 (R23 + x86 and 360-side profiler: which generated 360 code takes the time)',
            'S22': 'build R34 (R23 + fast x86 RET in the emulator\'s runtime code)',
            'Z22': 'build D34 (R34 + no AA - measurement only)',
            'T22': 'build R35 (R34 + fast x87 FISTP in the emulator\'s translator)',
            'U22': 'build D35 (R35 + no AA - measurement only)',
            'V22': 'build R36 (R35 + x86 cmp/jcc fused across movs in the emulator\'s translator)',
            'W22': 'build D36 (R36 + no AA - measurement only)',
            'X22': 'build L37 (R36 + render-target / eDRAM event log - measuring build)',
            'K22': 'build L37 noAA (D36 + render-target / eDRAM event log - measuring build)',
            'F24': 'build F24 (F22 + a timeline of when the 360 GPU starts and finishes each batch of work)'}
CALLEE_NAMES = {0x820D5608: 'visibility-test (query) begin', 0x820D4AD8: 'visibility-test (query) end',
                0x820CA878: 'stream switch', 0x820C3F60: 'render target: flush pending', 0x820C5770: 'render target: switch',
                0x820FF220: 'is the 360 GPU behind?', 0x820C30B0: 'render target: finish', 0x820C2208: 'render target: copy?',
                0x820C9600: 'stream helper', 0x820FBAC8: '360 D3D 0x820FBAC8', 0x820FBE30: '360 D3D 0x820FBE30',
                0x820F66B8: '360 D3D 0x820F66B8', 0x820F8500: '360 D3D 0x820F8500', 0x82105830: '360 D3D 0x82105830',
                0x82105A70: '360 D3D draw', 0x820FB480: '360 D3D set stream', 0x820FB520: '360 D3D 0x820FB520',
                0x820C9330: 'vertex streams for a draw', 0x820FED50: 'stream buffer: fence check',
                0x820FEB00: '360 D3D fence (from the stream buffer)', 0x820CCBA0: 'flush vertex data from the CPU cache',
                0x820D11F8: 'vertex copy (stub into 0x820D0FD8)', 0x820D0FD8: 'vertex convert/copy',
                0x8210E178: '360 D3D draw-state flush', 0x820FF138: '360 D3D kick', 0x820FEDB0: '360 D3D 0x820FEDB0',
                0x820C8D40: 'stream helper 0x820C8D40', 0x00000C01: 'vertex realign on the CPU (the F6 fix)',
                0x00000C02: 'vertex realign on the CPU, byte-swapped (the F7 fix)',
                0x00000C03: 'vertex copy on the CPU (the F8 fix)',
                0x00000C04: 'vertex copy on the CPU, rotating area (the F9 fix)',
                0x00000C05: 'vertex copy on the CPU, rotating area (the F10 fix)',
                0x820CA970: 'draw submit (streams + 360 D3D draw)', 0x820CB400: 'per-draw state setup',
                0x820D2EE0: 'draw setup: shaders/constants', 0x820CADB0: 'draw setup 0x820CADB0',
                0x820C14D8: 'surface page table: unlink', 0x820C1410: 'surface page table: link',
                0x820BFEA8: 'texture memory tracking', 0x8210DBA8: '360 D3D constant flush',
                0x820CCBA0: 'flush data from the CPU cache', 0x82105D10: '360 D3D 0x82105D10 (GPU cache?)',
                0x820D04B8: 'save 360 D3D state', 0x820D05D0: 'restore 360 D3D state', 0x820D0FD8: 'GPU copy pass',
                0x82110CD8: 'XG buffer header', 0x820C7B70: 'texture collector (evicts old textures)',
                0x820C7928: 'destroy a converted texture', 0x820FB818: '360 D3D set fetch constant',
                0x821020B8: '360 D3D set vertex shader', 0x82100648: '360 D3D 0x82100648', 0x821004F0: '360 D3D 0x821004F0'}
THREAD_HW = {}                      # processor -> emulator threads on it (filled by status())


class Xbdm:
    def __init__(self, ip, port=730, timeout=15.0):
        self.s = socket.create_connection((ip, port), timeout)
        self.s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.f = self.s.makefile('rb')
        self.banner = self.f.readline().decode('latin-1').strip()

    def cmd(self, text):
        self.s.sendall(text.encode('latin-1') + b'\r\n')
        line = self.f.readline()
        if not line:
            raise ConnectionError('XBDM closed the connection')
        head = line.decode('latin-1').rstrip('\r\n')
        code = int(head[:3])
        out = []
        if code == 202:
            while True:
                t = self.f.readline()
                if not t:
                    raise ConnectionError('XBDM closed the connection')
                t = t.decode('latin-1').rstrip('\r\n')
                if t == '.':
                    break
                out.append(t)
        return code, head, out

    def getmem(self, addr, n, chunk=0x400):
        out = bytearray()
        while len(out) < n:
            want = min(n - len(out), chunk)
            code, head, ls = self.cmd('getmem addr=0x%08X length=0x%X' % (addr + len(out), want))
            if code // 100 != 2:
                raise IOError('XBDM getmem 0x%08X: %s' % (addr + len(out), head))
            got = bytearray()
            for t in ls:
                t = t.strip()
                for i in range(0, len(t) - 1, 2):
                    h = t[i:i + 2]
                    if h == '??':
                        raise IOError('XBDM getmem 0x%08X: unreadable memory' % (addr + len(out) + len(got)))
                    got.append(int(h, 16))
            if not got:
                raise IOError('XBDM getmem 0x%08X: empty reply' % (addr + len(out)))
            out += got[:want]
        return bytes(out[:n])

    def close(self):
        try:
            self.s.sendall(b'bye\r\n')
        except OSError:
            pass
        self.s.close()


def kv(text):
    return {k.lower(): v.strip('"') for k, v in re.findall(r'(\w+)=("[^"]*"|\S+)', text)}


def num(t):
    return int(t, 0) & 0xFFFFFFFF


def emulator(x):
    code, head, mods = x.cmd('modules')
    for m in mods:
        d = kv(m)
        if d.get('name', '').lower().startswith('xefu') and not d.get('name', '').lower().startswith('xefutitle'):
            return d.get('name'), num(d.get('check', '0')), num(d.get('timestamp', '0'))
    return None, 0, 0


LAYOUT = {}


def variant(x):
    m = x.getmem(BLK, 0x40)
    if m[:4] != b'XFSG':
        return None
    layout, var = struct.unpack_from('>II', m, 4)
    v = chr(var) + (str(layout) if 2 <= layout <= 24 else '')
    LAYOUT[v] = (layout, struct.unpack_from('>I', m, 0x2C)[0], struct.unpack_from('>I', m, 0x30)[0])
    return v


def threads(x):
    code, head, ls = x.cmd('threads')
    out = []
    for t in ls:
        for tok in t.split():
            try:
                tid = num(tok)
            except ValueError:
                continue
            c, h, l2 = x.cmd('threadinfo thread=0x%08X' % tid)
            out.append((tid, kv(h + ' ' + ' '.join(l2))))
    return out


def identify(x):
    name, ck, ts = emulator(x)
    if name is None:
        sys.exit('No xefu emulator is running. Start Halo 2 first.')
    if (ck, ts) not in EMUS:
        sys.exit('%s is running (check 0x%08X, timestamp 0x%08X): not an emulator this tool knows.' % (name, ck, ts))
    emu, frames = EMUS[(ck, ts)]
    v = variant(x) if emu == 'xefu1_1' else None
    return emu, frames, name, v


def status(x, emu, name, v):
    if emu == 'xefu1_1':
        print('emulator: xefu1_1.xex - %s' % (VARIANTS.get(v, 'no build marker') if v else 'no build marker (the R63-based builds carry none here: h2test run names the build by its kernel marks; the R7x counter lines below show which logging build it is)'))
        if v in ('S22', 'Z22', 'T22', 'U22', 'V22', 'W22', 'X22', 'K22'):
            try:
                ok = struct.unpack('>I', x.getmem(0x8251C3F0, 4))[0]
                print('  fast x86 RET: %s' % ('installed' if ok == 1 else 'NOT installed (glue region tail was in use; stock RET path)'))
            except (IOError, ValueError):
                pass
    else:
        print('emulator: stock %s.xex (running as %s)' % (emu, name))
    exp = EXPECT.get(v, {})
    roles = ROLES if emu == 'xefu1_1' else {}
    print('%-10s %-3s %-5s %-10s %s' % ('thread', 'hw', 'prio', 'start', 'what'))
    problems = []
    for tid, info in threads(x):
        st = num(info.get('start', '0'))
        hw = num(info.get('proc', '0'))
        role = roles.get(st)
        if role is None:
            role = 'system / plugin' if tid >> 24 == 0xFB else '?'
            if num(info.get('namelen', '0')) and num(info.get('nameaddr', '0')):
                try:
                    nm = x.getmem(num(info['nameaddr']), min(num(info['namelen']), 64)).decode('latin-1', 'replace')
                    role += ': ' + nm.strip('\0')
                except IOError:
                    pass
        THREAD_HW.setdefault(hw, []).append(role.split(' (')[0])
        if st in exp and exp[st] != hw:
            problems.append('%s is on hardware thread %d, expected %d' % (ROLES[st], hw, exp[st]))
        print('0x%08X %-3d %-5s 0x%08X %s' % (tid, hw, info.get('priority', '?'), st, role))
    for p in problems:
        print('NOTE: ' + p)
    return problems


class Frames:
    """Turns the ring records into per-frame numbers (ms)."""
    FIELDS = ('frame', 'proc', 'game', 'thr', 'swap', 'ring')

    def __init__(self):
        self.last = None          # last record used: tuple of 8 words
        self.rows = []
        self.gaps = 0

    def feed(self, blob):
        recs = {}
        for k in range(RING_N):
            r = struct.unpack_from('>8I', blob, 0x100 + 32 * k)
            if r[7]:
                recs[r[7]] = r
        if not recs:
            return []
        if self.last is None:
            self.last = recs[max(recs)]
            return []
        new = []
        for f in sorted(recs):
            if f <= self.last[7]:
                continue
            r = recs[f]
            if f != self.last[7] + 1:             # frames overwritten before we read them
                self.gaps += f - self.last[7] - 1
                self.last = r
                continue
            d = [(r[j] - self.last[j]) & 0xFFFFFFFF for j in range(6)]
            row = {'n': f, 'frame': d[0] / TB_MS, 'proc': d[1] / TB_MS, 'game': d[2] / TB_MS, 'thr': d[3] / TB_MS,
                   'swap': d[4] / TB_MS, 'ring': d[5] / TB_MS, 'kicks': (r[6] - self.last[6]) & 0xFFFFFFFF}
            row['wait_game'] = max(0.0, row['game'] - row['thr'])
            row['wait_gpu'] = row['thr'] + row['ring'] + row['swap']
            row['busy'] = max(0.0, row['proc'] - row['game'] - row['ring'])
            row['other'] = max(0.0, row['frame'] - row['proc'] - row['swap'])
            new.append(row)
            self.last = r
        self.rows += new
        return new


def summarize(rows):
    if not rows:
        return None
    n = len(rows)
    total = sum(r['frame'] for r in rows)
    avg = lambda k: sum(r[k] for r in rows) / n
    s = {'frames': n, 'fps': 1000.0 * n / total if total else 0.0, 'frame': avg('frame'),
         'busy': avg('busy'), 'wait_game': avg('wait_game'), 'thr': avg('thr'), 'ring': avg('ring'),
         'swap': avg('swap'), 'wait_gpu': avg('wait_gpu'), 'other': avg('other'), 'kicks': avg('kicks'),
         'worst': max(r['frame'] for r in rows), 'over': 100.0 * sum(r['frame'] > 17.5 for r in rows) / n}
    parts = {'the GPU thread\'s own work (turning Halo 2\'s commands into 360 GPU commands)': s['busy'],
             'waiting for the 360 GPU': s['wait_gpu'], 'waiting for Halo 2 (the game\'s CPU side)': s['wait_game']}
    s['limit'] = max(parts, key=parts.get)
    return s


HEAD = '%6s %6s %7s %7s %8s %8s %7s %7s %7s %6s %7s' % ('time', 'fps', 'frame', 'busy', 'wait_H2', 'wait_GPU',
                                                        '(behind', 'cmdbuf', 'swap)', 'other', '>17.5ms')


def line(t, s):
    return '%5.0fs %6.1f %7.2f %7.2f %8.2f %8.2f %7.2f %7.2f %7.2f %6.2f %6.0f%%' % (
        t, s['fps'], s['frame'], s['busy'], s['wait_game'], s['wait_gpu'], s['thr'], s['ring'], s['swap'], s['other'],
        s['over'])


def measure(x, emu, frames, v, secs, label, interval):
    stamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    tag = ('%s_%s' % (emu, v)) if v else ('%s_stock' % emu)
    path = 'xefu_probe_%s_%s_%s.txt' % (tag, re.sub(r'[^A-Za-z0-9_-]+', '_', label or 'run'), stamp)
    log = open(path, 'w')
    what = VARIANTS.get(v, '?') if v else 'stock %s.xex' % emu
    log.write('xefu_probe  %s  %s  label=%s\n' % (stamp, what, label))
    t0 = time.time()
    if v in ('T2', 'B2', 'T3', 'B3', 'T4', 'T5', 'F6', 'D7', 'F7', 'F8', 'F9', 'F10', 'F11', 'B11', 'F12', 'F13', 'F14', 'F15', 'F16', 'F17', 'F18', 'F19', 'F20', 'F21', 'F22', 'R22', 'Q22', 'N22', 'L22', 'P22', 'Y22', 'S22', 'Z22', 'T22', 'U22', 'V22', 'W22', 'X22', 'K22', 'F24'):
        log.close()
        return measure2(x, v, secs, path, what, label, stamp, interval)
    if v not in ('T', 'A', 'B'):
        print('This emulator has no timing counters (%s): counting frames only.' % what)
        c0, tt0 = struct.unpack('>I', x.getmem(frames, 4))[0], time.time()
        while time.time() - t0 < secs:
            time.sleep(1.0)
            c1, tt1 = struct.unpack('>I', x.getmem(frames, 4))[0], time.time()
            fps = ((c1 - c0) & 0xFFFFFFFF) / (tt1 - tt0)
            msg = '%5.0fs %6.1f fps' % (tt1 - t0, fps)
            print(msg)
            log.write(msg + '\n')
            c0, tt0 = c1, tt1
        log.close()
        print('saved %s' % path)
        return
    fr = Frames()
    print('ms per frame:  busy = GPU thread turning Halo 2\'s commands into 360 GPU work;  wait_H2 = waiting for '
          'Halo 2\'s next commands;\n               wait_GPU = waiting for the 360 GPU (behind + cmdbuf full + swap);'
          '  other = rest of the GPU thread\'s loop')
    print(HEAD)
    log.write(HEAD + '\n')
    sec_rows, next_print = [], t0 + 1.0
    while time.time() - t0 < secs:
        blob = x.getmem(BLK, 0x100 + 32 * RING_N)
        sec_rows += fr.feed(blob)
        now = time.time()
        if now >= next_print:
            s = summarize(sec_rows)
            if s:
                msg = line(now - t0, s)
                print(msg)
                log.write(msg + '\n')
            else:
                print('%5.0fs  no new frames this second (console slow to answer, or Halo 2 not running)' % (now - t0))
            sec_rows, next_print = [], next_print + 1.0
        time.sleep(interval)
    s = summarize(fr.rows)
    out = []
    if s:
        out.append('')
        out.append('whole run: %d frames, %.1f fps, frame %.2f ms (worst %.2f ms, %.0f%% over 17.5 ms)' % (
            s['frames'], s['fps'], s['frame'], s['worst'], s['over']))
        out.append('  GPU thread busy %.2f ms | waiting for Halo 2 %.2f ms | waiting for the 360 GPU %.2f ms '
                   '(behind %.2f, command buffer full %.2f, swap %.2f) | other %.2f ms | %.1f 360 GPU kicks a frame' % (
                       s['busy'], s['wait_game'], s['wait_gpu'], s['thr'], s['ring'], s['swap'], s['other'], s['kicks']))
        out.append('  biggest share: %s' % s['limit'])
        if fr.gaps:
            out.append('  (%d frames not seen: the console answered slowly)' % fr.gaps)
    print('\n'.join(out))
    log.write('\n'.join(out) + '\n\nframe,ms,busy,wait_H2,behind,cmdbuf,swap,other,kicks\n')
    for r in fr.rows:
        log.write('%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d\n' % (r['n'], r['frame'], r['busy'], r['wait_game'],
                                                                 r['thr'], r['ring'], r['swap'], r['other'], r['kicks']))
    log.close()
    print('saved %s' % path)


# ---------------------------------------------------------------- round 2 (counter layout 2)
class Frames2:
    """Ring records of layout 2 (64 bytes) -> per-frame numbers in ms."""
    def __init__(self):
        self.last = None
        self.rows = []
        self.gaps = 0

    def feed(self, ring, layout=2):
        recs = {}
        for k in range(RING_N):
            r = struct.unpack_from('>16I', ring, 64 * k)
            if r[15]:
                recs[r[15]] = r
        if not recs:
            return []
        if self.last is None:
            self.last = recs[max(recs)]
            return []
        new = []
        for f in sorted(recs):
            if f <= self.last[15]:
                continue
            r, L = recs[f], self.last
            if f != L[15] + 1:                    # frames overwritten before we read them
                self.gaps += f - L[15] - 1
                self.last = r
                continue
            d = [((r[j] - L[j]) & 0xFFFFFFFF) / TB_MS for j in range(15)]
            row = dict(n=f, frame=d[0], proc=d[1], game=d[2], thr=d[3], swap=d[4], ring=d[5], flip=d[6], intr=d[7],
                       rt=d[8], flush=d[10], kicks=(r[11] - L[11]) & 0xFFFFFFFF,
                       binds=(r[12] - L[12]) & 0xFFFFFFFF, flushes=(r[13] - L[13]) & 0xFFFFFFFF,
                       idx=0.0, idxcopies=0, lock=0.0, rtfin=0.0)
            if layout >= 3:
                row['lock'], row['rtfin'] = d[9], d[14]
            else:
                row['idx'], row['idxcopies'] = d[9], (r[14] - L[14]) & 0xFFFFFFFF
            row['vbl'] = round(row['frame'] / 16.683)
            row['wait_game'] = max(0.0, row['game'] - row['thr'])
            row['wait_gpu'] = row['thr'] + row['ring'] + row['swap']
            row['work'] = max(0.0, row['proc'] - row['game'] - row['ring'] - row['flip'] - row['intr'] - row['lock'])
            row['other'] = max(0.0, row['frame'] - row['proc'] - row['swap'])
            new.append(row)
            self.last = r
        self.rows += new
        return new


def pct(vals, q):
    v = sorted(vals)
    return v[min(len(v) - 1, int(q * (len(v) - 1) + 0.5))] if v else 0.0


def summarize2(rows):
    if not rows:
        return None
    n = len(rows)
    total = sum(r['frame'] for r in rows)
    avg = lambda k, rr=rows: sum(r[k] for r in rr) / len(rr)
    s = {k: avg(k) for k in ('frame', 'work', 'flip', 'intr', 'wait_game', 'wait_gpu', 'thr', 'ring', 'swap', 'other',
                             'rt', 'idx', 'flush', 'kicks', 'binds', 'flushes', 'idxcopies', 'lock', 'rtfin')}
    s.update(frames=n, fps=1000.0 * n / total if total else 0.0, worst=max(r['frame'] for r in rows),
             over=100.0 * sum(r['vbl'] > 1 for r in rows) / n,
             w50=pct([r['work'] for r in rows], 0.5), w90=pct([r['work'] for r in rows], 0.9),
             w99=pct([r['work'] for r in rows], 0.99),
             wover=100.0 * sum(r['work'] > 16.0 for r in rows) / n)
    s['by_vbl'] = {}
    for k in sorted(set(r['vbl'] for r in rows)):
        rr = [r for r in rows if r['vbl'] == k]
        s['by_vbl'][k] = (len(rr), avg('work', rr), avg('flip', rr), avg('intr', rr), avg('lock', rr))
    return s


HEAD2 = '%6s %6s %6s %6s %6s %5s %6s %6s %6s %5s %6s %6s %5s %6s' % (
    'time', 'fps', 'frame', 'work', 'vblank', 'irq', 'wt_H2', 'wt_GPU', 'rt', 'binds', 'idx', 'flush', 'flsh#', '2vbl%')


def line2(t, s):
    return '%5.0fs %6.1f %6.2f %6.2f %6.2f %5.2f %6.2f %6.2f %6.2f %5.0f %6.2f %6.2f %5.0f %5.0f%%' % (
        t, s['fps'], s['frame'], s['work'], s['flip'], s['intr'], s['wait_game'], s['wait_gpu'], s['rt'], s['binds'],
        s['idx'], s['flush'], s['flushes'], s['over'])


HEAD3 = '%6s %6s %6s %6s %6s %6s %6s %5s %6s %6s %6s %5s %6s %5s %6s' % (
    'time', 'fps', 'frame', 'work', 'lock', 'rtfin', 'vblank', 'irq', 'wt_H2', 'wt_GPU', 'rt', 'binds', 'flush', 'flsh#',
    '2vbl%')


def line3(t, s):
    return '%5.0fs %6.1f %6.2f %6.2f %6.2f %6.2f %6.2f %5.2f %6.2f %6.2f %6.2f %5.0f %6.2f %5.0f %5.0f%%' % (
        t, s['fps'], s['frame'], s['work'], s['lock'], s['rtfin'], s['flip'], s['intr'], s['wait_game'], s['wait_gpu'],
        s['rt'], s['binds'], s['flush'], s['flushes'], s['over'])


def owners(x):
    """Who held the render-target lock while the GPU thread waited (layout 3 header)."""
    h = x.getmem(BLK, 0x180)
    gpu_pcr = struct.unpack_from('>I', h, 0x38)[0]
    out, total = [], 0
    for b in range(4):
        ow, pr = struct.unpack_from('>II', h, 0x140 + 16 * b)
        tk = struct.unpack_from('>Q', h, 0x148 + 16 * b)[0]
        if ow:
            out.append((ow, pr, tk))
            total += tk
    lines = []
    for ow, pr, tk in sorted(out, key=lambda o: -o[2]):
        if ow == 1:
            who = 'nobody recorded (lock had just been released)'
        elif ow == gpu_pcr:
            who = 'the GPU thread itself'
        elif pr == 0xFF:
            who = 'PCR 0x%08X (processor unknown)' % ow
        else:
            who = 'hardware thread %d (%s)' % (pr, ', '.join(THREAD_HW.get(pr, ['?'])))
        lines.append('    %5.1f%%  %s' % (100.0 * tk / total if total else 0.0, who))
    return lines, h


def measure2(x, v, secs, path, what, label, stamp, interval):
    layout, ring_addr, rec = LAYOUT[v]
    head_fmt, line_fn = (HEAD3, line3) if layout >= 3 else (HEAD2, line2)
    log = open(path, 'w')
    log.write('xefu_probe  %s  %s  label=%s\n' % (stamp, what, label))
    print('ms per frame:  work = the GPU thread\'s own work (turning Halo 2\'s commands into 360 GPU work);'
          '  vblank = waiting for the next vblank (flip stall);\n'
          '               irq = waiting for Halo 2\'s interrupt handler;  wt_H2 = waiting for Halo 2\'s commands;'
          '  wt_GPU = waiting for the 360 GPU;\n'
          '               rt / idx / flush = parts of work: render-target binds (count: binds), inline index copies,'
          ' stream flushes (count: flsh#);  2vbl% = frames shown 2+ vblanks apart')
    if layout >= 3:
        print('               lock = waiting for the render-target lock (inside work before round 3); rtfin = finishing '
              'a render target (includes its lock wait)')
    print(head_fmt)
    log.write(head_fmt + '\n')
    fr = Frames2()
    h0 = x.getmem(BLK, 0x140)
    prof0 = read_profile(x) if layout >= 4 else None
    if layout >= 5:
        SPIN0[0] = struct.unpack_from('>I', x.getmem(BLK + 0x188, 4), 0)[0]
    t0 = time.time()
    sec_rows, next_print = [], t0 + 1.0
    while time.time() - t0 < secs:
        ring = x.getmem(ring_addr, RING_N * rec)
        sec_rows += fr.feed(ring, layout)
        now = time.time()
        if now >= next_print:
            s = summarize2(sec_rows)
            if s:
                msg = line_fn(now - t0, s)
                print(msg)
                log.write(msg + '\n')
            else:
                print('%5.0fs  no new frames this second (console slow to answer, or Halo 2 not running)' % (now - t0))
            sec_rows, next_print = [], next_print + 1.0
        time.sleep(interval)
    s = summarize2(fr.rows)
    out = []
    if s:
        out.append('')
        out.append('whole run: %d frames, %.1f fps, frame %.2f ms (worst %.2f), %.0f%% of frames shown 2+ vblanks apart'
                   % (s['frames'], s['fps'], s['frame'], s['worst'], s['over']))
        out.append('  GPU thread work %.2f ms a frame (median %.2f, 90%% %.2f, 99%% %.2f; %.0f%% of frames over 16.0 ms)'
                   % (s['work'], s['w50'], s['w90'], s['w99'], s['wover']))
        out.append('  waiting: vblank %.2f | Halo 2\'s interrupt handler %.2f | Halo 2\'s commands %.2f | 360 GPU %.2f '
                   '(behind %.2f, command buffer full %.2f, swap %.2f) | other %.2f ms'
                   % (s['flip'], s['intr'], s['wait_game'], s['wait_gpu'], s['thr'], s['ring'], s['swap'], s['other']))
        out.append('  inside the work: render-target binds %.2f ms (%.0f a frame), inline index copies %.2f ms (%.0f), '
                   'stream flushes %.2f ms (%.0f); %.1f 360 GPU kicks a frame'
                   % (s['rt'], s['binds'], s['idx'], s['idxcopies'], s['flush'], s['flushes'], s['kicks']))
        if layout >= 3:
            out.append('  render-target lock: the GPU thread waited %.2f ms a frame for it; finishing render targets took '
                       '%.2f ms a frame (incl. that wait)' % (s['lock'], s['rtfin']))
            lines, h1 = owners(x)
            if lines:
                out.append('  who held the lock while the GPU thread waited:')
                out += lines
            nf = max(1, s['frames'])
            sl = lambda k, h: struct.unpack_from('>Q', h, 0x40 + 16 * k)[0]
            cl = lambda k, h: struct.unpack_from('>I', h, 0x48 + 16 * k)[0]
            run_frames = (struct.unpack_from('>I', h1, 0x24)[0] - struct.unpack_from('>I', h0, 0x24)[0]) or nf
            out.append('  whole-run totals per frame: inline index copies %.2f ms (%.0f), other threads\' Sleep %.2f ms, '
                       'other threads finishing render targets %.2f ms (%.1f)' % (
                           (sl(9, h1) - sl(9, h0)) / TB_MS / run_frames, (cl(9, h1) - cl(9, h0)) / run_frames,
                           (sl(12, h1) - sl(12, h0)) / TB_MS / run_frames, (sl(14, h1) - sl(14, h0)) / TB_MS / run_frames,
                           (cl(14, h1) - cl(14, h0)) / run_frames))
        if prof0 is not None:
            prof1 = read_profile(x)
            run_frames = (struct.unpack_from('>I', x.getmem(BLK, 0x40), 0x24)[0] - struct.unpack_from('>I', h0, 0x24)[0]) or 1
            rows = []
            for (site, callee, t0, c0, m0), (_, _, t1, c1, m1) in zip(prof0, prof1):
                rows.append(((t1 - t0) / TB_MS / run_frames, (c1 - c0) / run_frames, m1 / TB_MS, site, callee))
            rows.sort(reverse=True)
            out.append('  timed calls, biggest first (ms a frame, calls a frame, longest single call ms since the game '
                       'started, call site -> function); a call inside another timed call counts in both:')
            for ms, calls, longest, site, callee in rows[:26]:
                where = {0: 'main loop', 1: 'draw setup', 2: 'D3D flush', 3: 'tex track', 4: 'D3D const',
                         5: 'page table', 6: 'tex conv', 7: 'invalidate', 8: 'tex evict'}.get(site, '%08X' % site)
                if callee & 0xFFFF0000 == 0x000B0000:
                    out.append('    %7.2f ms  %6.1f calls  longest %6.2f  %9s -> indirect, last target %08X' % (
                        ms, calls, longest, where, IND_TARGET.get(callee, 0)))
                else:
                    out.append('    %7.2f ms  %6.1f calls  longest %6.2f  %9s -> %08X %s' % (
                        ms, calls, longest, where, callee, CALLEE_NAMES.get(callee, '')))
            PROFILE_ROWS[:] = rows
            if v == 'F24':
                out += gpu_timeline_report(x) + palette_report(x)
            elif v in ('R22', 'Q22', 'N22', 'L22', 'P22', 'Y22', 'S22', 'Z22', 'T22', 'U22', 'V22', 'W22', 'X22', 'K22'):
                out += palette_report(x)
            elif layout >= 22:
                out += palette_report(x) + decision_report2(x)
            elif layout >= 21:
                out += decision_report2(x) + rtinv_report(x)
            elif layout >= 19:
                out += decision_report2(x) + blit_report(x) + invalidation_report(x)
            elif layout >= 18:
                out += decision_report(x) + invalidation_report(x)
            elif layout >= 16:
                out += fingerprint_report(x) + invalidation_report(x)
            elif layout >= 15:
                out += converter_report(x)
            if layout >= 5:
                spin = (struct.unpack_from('>I', x.getmem(BLK + 0x188, 4), 0)[0] - SPIN0[0]) & 0xFFFFFFFF
                out.append('  free stream-buffer search loop (0x820C9330): %.1f passes a frame' % (spin / run_frames))
        if v == 'D7':
            out += capture_report(x)
        for k, (n, w, fl, ir, lk) in s['by_vbl'].items():
            out.append('  frames shown %d vblank(s) apart: %d, work %.2f ms, lock wait %.2f ms, vblank wait %.2f ms, '
                       'interrupt wait %.2f ms' % (k, n, w, lk, fl, ir))
        if fr.gaps:
            out.append('  (%d frames not seen: the console answered slowly)' % fr.gaps)
    print('\n'.join(out))
    log.write('\n'.join(out) + '\n\nframe,ms,vblanks,work,lock,rtfin,vblank,irq,wait_H2,behind,cmdbuf,swap,other,rt,binds,'
              'idx,idxcopies,flush,flushes,kicks\n')
    for r in fr.rows:
        log.write('%d,%.3f,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.3f,%d,%.3f,%d,%d\n' % (
            r['n'], r['frame'], r['vbl'], r['work'], r['lock'], r['rtfin'], r['flip'], r['intr'], r['wait_game'],
            r['thr'], r['ring'], r['swap'], r['other'], r['rt'], r['binds'], r['idx'], r['idxcopies'], r['flush'],
            r['flushes'], r['kicks']))
    if PROFILE_ROWS:
        log.write('\nprofile (all call sites): ms_per_frame,calls_per_frame,longest_ms,site,callee\n')
        for ms, calls, longest, site, callee in PROFILE_ROWS:
            log.write('%.4f,%.2f,%.3f,%08X,%08X%s\n' % (ms, calls, longest, site, callee,
                      (',target %08X' % IND_TARGET.get(callee, 0)) if callee & 0xFFFF0000 == 0x000B0000 else ''))
    log.close()
    print('saved %s' % path)


PROFILE_ROWS = []
SPIN0 = [0]


LINKED = []


def decision_report2(x):
    """F19: decision records with the linked surface's range (words 10-12) and the blit count (word 14)."""
    cnt = struct.unpack_from('>I', x.getmem(0x8251CBF8, 4), 0)[0]
    raw = x.getmem(0x8251D000, 32 * 64)
    recs = sorted((struct.unpack_from('>16I', raw, 64 * k) for k in range(32)), key=lambda w: w[15])
    recs = [w for w in recs if w[15]]
    out = ['', '  texture conversions: what the version check compared (the last %d of %d)' % (len(recs), cnt)]
    if not recs:
        return out + ['    none']
    out.append('    surface version differed: %d | linked-surface version differed: %d | of %d'
               % (sum(w[6] != w[7] for w in recs), sum(w[8] != w[9] for w in recs), len(recs)))
    links = {}
    for w in recs:
        links.setdefault((w[5], w[10], w[11], w[12]), []).append(w)
    out.append('    linked surfaces involved:')
    for (reg, start, end, flags), l in sorted(links.items(), key=lambda g: -len(g[1])):
        LINKED.append((start, end))
        out.append('      %08X  guest %08X-%08X (%d bytes)  flags %08X  in %d conversions, its version %d..%d'
                   % (reg, start, end, end - start, flags, len(l), min(w[9] for w in l), max(w[9] for w in l)))
    out.append('    frame    address   surface  record   rec84 surf44 rec88 reg44  blits-so-far')
    for w in recs[-12:]:
        out.append('    %-8d %08X  %08X %08X %5d %6d %5d %5d  %d' % (w[0], w[1], w[3], w[4], w[6], w[7], w[8], w[9], w[14]))
    return out


ALL_RAW = []


def gpu_timeline_report(x, snapshots=40, gap=0.1):
    """F24: from samples of 360 D3D's batch counters (submitted / completed by the GPU), work out per batch how long
    the GPU spent on it and how many draws it held, and how long the GPU sat idle. Also dumps raw samples."""
    out = ['', '  360 GPU timeline (%d snapshots):' % snapshots]
    busy = idle = wall = 0.0
    batches = []                     # (gpu_us, draws)
    first_raw = None
    counts = []
    for snap in range(snapshots):
        hdr = x.getmem(0x8251CB70, 12)
        cnt, last_s, last_c = struct.unpack_from('>3I', hdr, 0)
        raw = x.getmem(0x8251D800, 128 * 16)
        counts.append((cnt, last_s, last_c))
        n = min(cnt, 128)
        recs = [struct.unpack_from('>4I', raw, 16 * i) for i in range(n)]
        # the ring keeps moving while it is read: order by the counters themselves (they only ever go up), then
        # keep the longest run whose timestamps also go up
        recs = [r for r in recs if r[1] or r[2]]
        recs.sort(key=lambda r: (r[1], r[2], r[3]))
        runs, cur = [], recs[:1]
        for a_, b_ in zip(recs, recs[1:]):
            if ((b_[0] - a_[0]) & 0xFFFFFFFF) < 0x40000000 and b_[1] - a_[1] < 64:
                cur.append(b_)
            else:
                runs.append(cur); cur = [b_]
        runs.append(cur)
        recs = max(runs, key=len) if recs else []
        ALL_RAW.append(recs)
        if first_raw is None and recs:
            first_raw = recs
        time.sleep(gap)
        if len(recs) < 4:
            continue
        t0 = recs[0][0]
        rel = [((r[0] - t0) & 0xFFFFFFFF) * 1000.0 / TB_MS for r in recs]   # microseconds
        # submit time/draws per batch id, completion time per batch id (jumps of several batches share a time)
        # the counters step by a fixed amount per batch (2 on the console): find it from the data
        diffs = [b_[1] - a_[1] for a_, b_ in zip(recs, recs[1:]) if b_[1] != a_[1]]
        diffs += [b_[2] - a_[2] for a_, b_ in zip(recs, recs[1:]) if b_[2] != a_[2]]
        step = 1
        if diffs:
            import math
            g_ = 0
            for d_ in diffs:
                g_ = math.gcd(g_, d_)
            step = max(1, g_)
        sub_t, sub_d, done_t = {}, {}, {}
        prev_s = prev_c = None
        for (tb, s_, c, d), t in zip(recs, rel):
            if prev_s is None:
                prev_s, prev_c = s_, c
                continue
            for k in range(prev_s + step, s_ + 1, step) if 0 < s_ - prev_s < 64 * step else ():
                sub_t[k], sub_d[k] = t, d
            for k in range(prev_c + step, c + 1, step) if 0 < c - prev_c < 64 * step else ():
                done_t[k] = t
            prev_s, prev_c = s_, c
        ks = sorted(k for k in done_t if k - step in done_t)
        gpu_here = 0.0
        for k in ks:
            start = max(done_t[k - step], sub_t.get(k, -1e18))
            g = done_t[k] - start
            dk = (sub_d[k] - sub_d[k - step]) if (k in sub_d and k - step in sub_d) else -1
            batches.append((max(0.0, g), dk))
            gpu_here += max(0.0, g)
            if k in sub_t and sub_t[k] > done_t[k - step]:
                idle += sub_t[k] - done_t[k - step]
        if ks:
            wall += done_t[ks[-1]] - done_t[ks[0] - step]
            busy += gpu_here
    c0, c1 = counts[0], counts[-1]
    out.append('    samples logged: %d -> %d during the snapshots; batches submitted %d -> %d, completed %d -> %d'
               % (c0[0], c1[0], c0[1], c1[1], c0[2], c1[2]))
    if first_raw:
        t0 = first_raw[0][0]
        out.append('    raw samples (first snapshot; us since its first sample, submitted, completed, draws so far):')
        show = first_raw[:12] + ([None] if len(first_raw) > 24 else []) + first_raw[-12:] if len(first_raw) > 24 else first_raw
        for r in show:
            if r is None:
                out.append('      ...')
            else:
                out.append('      %10.1f  %10d  %10d  %10d' % (((r[0] - t0) & 0xFFFFFFFF) * 1000.0 / TB_MS, r[1], r[2], r[3]))
    raw_dump = ['', '  raw GPU timeline samples, all snapshots (timebase, submitted, completed, draws):']
    for i, rr in enumerate(ALL_RAW):
        raw_dump.append('    snapshot %d' % i)
        raw_dump += ['    %08X %d %d %d' % r for r in rr]
    if not batches or wall <= 0:
        return out + ['    no complete batches could be timed from these samples'] + raw_dump
    known = [b for b in batches if b[1] >= 0]
    draws = sum(b[1] for b in known)
    gpu = sum(b[0] for b in batches)
    out.append('    %d batches timed; GPU busy %.0f%% of the time between their completions, idle with nothing submitted %.0f%%'
               % (len(batches), 100 * busy / wall, 100 * idle / wall))
    withd = [b for b in known if b[1] > 0]
    out.append('    GPU time per batch: average %.0f us; in batches that contain draws: %.1f us per draw (%d draws in %d'
               ' batches)' % (gpu / len(batches), sum(b[0] for b in withd) / max(1, draws), draws, len(withd)))
    out.append('    longest batches (GPU us, draws in the batch):')
    for g, d in sorted(batches, key=lambda b: -b[0])[:12]:
        out.append('      %8.0f us  %s' % (g, ('%4d draws  %.1f us/draw' % (d, g / d)) if d > 0 else
                                          ('   0 draws  (copy/resolve/sync)' if d == 0 else '   ? draws')))
    nod = [b for b in batches if b[1] == 0]
    out.append('    batches with no draws: %d, %.0f us of GPU time (%.0f%% of all timed GPU time)'
               % (len(nod), sum(b[0] for b in nod), 100 * sum(b[0] for b in nod) / max(1, gpu)))
    return out + raw_dump


def palette_report(x):
    """F22: palette version bumps kept (bytes changed / first seen) vs skipped (bytes unchanged)."""
    raw = x.getmem(0x8251CA60, 268)
    nxt, kept, skipped = struct.unpack_from('>3I', raw, 256)
    out = ['', '  texture palettes marked as changed since the game started: %d had really changed (or were new), %d had'
           ' the same bytes and were left alone by the F22 fix' % (kept, skipped)]
    for k in range(min(nxt, 32)):
        surf, h = struct.unpack_from('>II', raw, 8 * k)
        if surf:
            out.append('    palette surface %08X  fingerprint %08X' % (surf, h))
    return out


def rtinv_report(x):
    """F21: palette invalidations by render-target rebinds that the fix skipped (and other RT-overlap invalidations)."""
    other = struct.unpack_from('>I', x.getmem(0x8251CBEC, 4), 0)[0]
    cnt = struct.unpack_from('>I', x.getmem(0x8251CBF0, 4), 0)[0]
    raw = x.getmem(0x8251D800, 64 * 32)
    recs = sorted((struct.unpack_from('>8I', raw, 32 * k) for k in range(64)), key=lambda w: w[7])
    recs = [w for w in recs if w[7]]
    out = ['', '  render-target rebinds: palette invalidations skipped by the F21 fix since the game started: %d; other'
           ' surfaces invalidated by render-target overlap: %d' % (cnt, other)]
    if recs:
        frames = recs[-1][0] - recs[0][0] + 1
        out.append('    recent rate: %.2f palette skips a frame' % (len(recs) / max(1, frames)))
        by = {}
        for w in recs:
            by.setdefault((w[1], w[2], w[3], w[5], w[6]), []).append(w[0])
        for (rt, rs, re_, ps, pe), fl in sorted(by.items(), key=lambda g: -len(g[1]))[:8]:
            out.append('    %3d x  render target %08X covering guest %08X-%08X  over palette %08X-%08X'
                       % (len(fl), rt, rs, re_, ps, pe))
    return out


def blit_report(x):
    """F19: the last 64 2D blits (arguments to 0x820C59B8) and which land in the linked surface's memory."""
    cnt = struct.unpack_from('>I', x.getmem(0x8251CBF0, 4), 0)[0]
    raw = x.getmem(0x8251D800, 64 * 32)
    recs = sorted((struct.unpack_from('>8I', raw, 32 * k) for k in range(64)), key=lambda w: w[7])
    recs = [w for w in recs if w[7]]
    out = ['', '  2D blits since the game started: %d; the last %d (r6..r7 and r9..r10 are the two memory ranges):'
           % (cnt, len(recs))]
    if not recs:
        return out
    frames = recs[-1][0] - recs[0][0] + 1
    out.append('    %.2f blits a frame' % (len(recs) / max(1, frames)))
    def hits(lo, hi):
        lo &= 0x0FFFFFFF; hi &= 0x0FFFFFFF
        return any(lo < e and s < hi for s, e in LINKED)
    into = 0
    by = {}
    for w in recs:
        by.setdefault((w[1], w[2], w[3], w[4], w[5], w[6]), []).append(w[0])
        into += hits(w[3], w[4]) or hits(w[1], w[2])
    out.append('    blits touching a linked surface\'s memory: %d of %d' % (into, len(recs)))
    for (a, b, c, d, e, f), fl in sorted(by.items(), key=lambda g: -len(g[1]))[:8]:
        out.append('    %3d x  r6 %08X r7 %08X  r9 %08X r10 %08X  r4 %08X r5 %08X  %s  frames %d..%d'
                   % (len(fl), a, b, c, d, e, f, 'LINKED' if (hits(c, d) or hits(a, b)) else '      ', fl[0], fl[-1]))
    return out


def decision_report(x):
    """F18: what the render-target bind's version check compared when it decided to convert a texture."""
    cnt = struct.unpack_from('>I', x.getmem(0x8251CBF8, 4), 0)[0]
    raw = x.getmem(0x8251D000, 64 * 64)
    recs = []
    for k in range(64):
        w = struct.unpack_from('>16I', raw, 64 * k)
        if w[15]:
            recs.append(w)
    recs.sort(key=lambda w: w[15])
    out = ['', '  texture conversions: what the version check compared (the last %d of %d)' % (len(recs), cnt)]
    if not recs:
        return out + ['    none']
    tex_ver = sum(1 for w in recs if w[6] != w[7])
    reg_ver = sum(1 for w in recs if w[8] != w[9])
    out.append('    surface version differed (record+84 != surface+44): %d | region version differed (record+88 != '
               'region+44): %d | of %d' % (tex_ver, reg_ver, len(recs)))
    by = {}
    for w in recs:
        by.setdefault(w[1], []).append(w)
    prev_rec = {}
    changed_rec = changed_surf = 0
    for addr, l in by.items():
        for a, b in zip(l, l[1:]):
            changed_rec += a[4] != b[4]
            changed_surf += a[3] != b[3]
    out.append('    between conversions of the same texture: the record object changed %d times, the surface object %d '
               'times' % (changed_rec, changed_surf))
    out.append('    frame    address   surface  record   region   rec84 surf44 rec88 reg44  rec+0    rec+48 pages flags    surf+100')
    for addr, l in sorted(by.items(), key=lambda g: -len(g[1]))[:4]:
        for w in l[:6]:
            out.append('    %-8d %08X  %08X %08X %08X %5d %6d %5d %5d  %08X %6d %5d %08X %08X'
                       % (w[0], w[1], w[3], w[4], w[5], w[6], w[7], w[8], w[9], w[10], w[11], w[12], w[13], w[14]))
    return out


def fingerprint_report(x):
    """F16: the last 128 texture conversions with a fingerprint of the source bytes, grouped by texture."""
    cnt = struct.unpack_from('>I', x.getmem(0x8251CBF8, 4), 0)[0]
    raw = x.getmem(0x8251D000, 128 * 32)
    recs = []
    for k in range(128):
        frame, target, r3, fp, addr, size, rec, seq = struct.unpack_from('>8I', raw, 32 * k)
        if seq:
            recs.append((seq, frame, r3, fp, addr, size))
    recs.sort()
    out = ['', '  texture conversions (the last %d of %d), by texture: how many, and how many had different contents:'
           % (len(recs), cnt)]
    if not recs:
        return out + ['    none']
    frames = recs[-1][1] - recs[0][1] + 1
    by = {}
    for seq, frame, r3, fp, addr, size in recs:
        by.setdefault((addr, size, r3), []).append(fp)
    same = 0
    for (addr, size, r3), fps in sorted(by.items(), key=lambda g: -len(g[1])):
        distinct = len(set(fps))
        same += len(fps) - distinct
        out.append('    guest %08X  %7d bytes  stage slot %d:  %2d conversions in %d frames, %2d different contents  %s'
                   % (addr - 0xD0000000 if addr >= 0xD0000000 else addr, size, r3, len(fps), frames, distinct,
                      ' '.join('%08X' % f for f in fps[:6])))
    out.append('    => %d of %d conversions re-converted contents already converted for that texture'
               % (same, len(recs)))
    return out


def invalidation_report(x):
    """F16: surfaces whose 'valid' bit was cleared (path 0 = Halo 2 wrote to their memory, 1 = 0x820C4660)."""
    c0, c1, tot = struct.unpack_from('>3I', x.getmem(0x8251CBE0, 12), 0)
    raw = x.getmem(0x8251CC00, 32 * 32)
    recs = []
    for k in range(32):
        frame, path, surf, start, end, seq = struct.unpack_from('>6I', raw, 32 * k)
        if seq:
            recs.append((seq, frame, path, surf, start, end))
    recs.sort()
    touched = struct.unpack_from('>I', x.getmem(0x8251CBEC, 4), 0)[0]
    out = ['', '  textures kept cached by the F17 fix (extra refreshes since the game started): %d' % touched,
           '  surfaces invalidated since the game started: %d by Halo 2 writing to their memory (path 0), %d by'
           ' 0x820C4660 (path 1); the last %d:' % (c0, c1, len(recs))]
    by = {}
    for seq, frame, path, surf, start, end in recs:
        by.setdefault((path, start, end), []).append(frame)
    for (path, start, end), fl in sorted(by.items(), key=lambda g: -len(g[1])):
        out.append('    path %d  guest %08X-%08X (%7d bytes): %2d times, frames %d..%d'
                   % (path, start, end, end - start, len(fl), fl[0], fl[-1]))
    return out


def converter_report(x):
    """F15: the last 32 calls of the texture converter reached from 0x820C6888 (frame, target, r3-r7)."""
    cnt = struct.unpack_from('>I', x.getmem(0x8251CBF8, 4), 0)[0]
    raw = x.getmem(0x8251CC00, 32 * 32)
    recs = []
    for k in range(32):
        frame, target, r3, r4, r5, r6, r7, seq = struct.unpack_from('>8I', raw, 32 * k)
        if seq:
            recs.append((seq, frame, target, r3, r4, r5, r6, r7))
    recs.sort()
    out = ['', '  texture converter calls (the last %d of %d; r3 = kind, r5 = guest address, r6 = bytes):' % (len(recs), cnt)]
    if not recs:
        return out + ['    none']
    frames = recs[-1][1] - recs[0][1] + 1
    groups = {}
    for seq, frame, target, r3, r4, r5, r6, r7 in recs:
        groups.setdefault((target, r3, r5, r6, r7), []).append(frame)
    for (target, r3, r5, r6, r7), fl in sorted(groups.items(), key=lambda g: -len(g[1])):
        out.append('    %3d calls in %d frames  target %08X  r3 %08X  address %08X  bytes %8d (0x%X)  r7 %08X'
                   % (len(fl), frames, target, r3, r5, r6, r6, r7))
    return out
CAP = 0x8251D000
TRANSFORMS = {  # where source byte k of vertex v should land in the destination vertex, for each candidate
    'plain copy': lambda k: k,
    'bytes of each 32-bit word reversed': lambda k: (k & ~3) + 3 - (k & 3),
    'bytes of each 16-bit half swapped': lambda k: k ^ 1,
    '16-bit halves of each word swapped': lambda k: k ^ 2,
}


def capture_report(x):
    """D7: what the GPU copy pass wrote vs. its source, and which byte order explains it."""
    raw = x.getmem(CAP, 0x20 + 8 * 256)
    out = ['', '  GPU copy-pass capture (source sample taken when the pass was set up, GPU result 3+ frames later):']
    done = 0
    scores = {t: [0, 0] for t in TRANSFORMS}
    for i in range(8):
        r = raw[0x20 + 256 * i: 0x20 + 256 * (i + 1)]
        frame, src, dst, ss, ds, count, guest, state = struct.unpack_from('>8I', r, 0)
        if state != 2 or not ss or not ds:
            continue
        done += 1
        sb, db, sa = r[0x20:0x60], r[0x60:0xA0], r[0xA0:0xE0]
        out.append('    frame %d  guest 0x%08X  src 0x%08X (src & 3 = %d)  dst 0x%08X  stride %d -> %d  count %d%s' % (
            frame, guest, src, src & 3, dst, ss, ds, count, '' if sb == sa else '  (source changed since!)'))
        out.append('      src ' + sb.hex())
        out.append('      dst ' + db.hex())
        nv = min(64 // ss, 64 // ds)
        nvf = min((64 - ds) // ss + 1, 64 // ds) if 64 >= ds else 0
        t = 'GPU-pass copy (dst-stride bytes from each vertex start)'
        scores.setdefault(t, [0, 0])
        for v_ in range(nvf):
            for j in range(ds):
                scores[t][1] += 1
                scores[t][0] += sb[v_ * ss + j] == db[v_ * ds + j]
        for t, f in TRANSFORMS.items():
            for v_ in range(nv):
                for k in range(ss):
                    j = f(k)
                    if j < ds:
                        scores[t][1] += 1
                        scores[t][0] += sb[v_ * ss + k] == db[v_ * ds + j]
    if not done:
        out.append('    no completed samples (no unaligned vertex streams seen, or the run was too short)')
    else:
        out.append('    byte order match over %d samples:' % done)
        for t, (ok, n) in sorted(scores.items(), key=lambda kv: -kv[1][0] / max(1, kv[1][1])):
            out.append('      %5.1f%%  %s' % (100.0 * ok / max(1, n), t))
    return out


def read_profile(x):
    """Layout 4: per-call-site profile table -> [(site, callee, ticks, calls, longest)]."""
    addr, n = struct.unpack_from('>II', x.getmem(BLK + 0x180, 8), 0)
    if not addr or n > 1024:
        return []
    raw = x.getmem(addr, 32 * n)
    out = []
    for k in range(n):
        site, callee, ticks, calls, longest, last = struct.unpack_from('>IIQIII', raw, 32 * k)
        if callee & 0xFFFF0000 == 0x000B0000:          # indirect-call timer: show the target it last called
            IND_TARGET[callee] = last
        out.append((site, callee, ticks, calls, longest))
    return out


IND_TARGET = {}


PM4_OPS = {  # Xenos command processor type-3 opcodes (per Xenia's xenos.h)
    0x10: 'NOP', 0x21: 'REG_RMW', 0x22: 'DRAW_INDX', 0x23: 'VIZ_QUERY', 0x25: 'SET_STATE', 0x26: 'WAIT_FOR_IDLE',
    0x27: 'IM_LOAD', 0x2B: 'IM_LOAD_IMMEDIATE', 0x2D: 'SET_CONSTANT', 0x2E: 'LOAD_CONSTANT_CONTEXT',
    0x2F: 'LOAD_ALU_CONSTANT', 0x34: 'DRAW_INDX_BIN', 0x35: 'DRAW_INDX_2_BIN', 0x36: 'DRAW_INDX_2',
    0x37: 'INDIRECT_BUFFER_PFD', 0x3B: 'INVALIDATE_STATE', 0x3C: 'WAIT_REG_MEM', 0x3D: 'MEM_WRITE',
    0x3E: 'REG_TO_MEM', 0x3F: 'INDIRECT_BUFFER', 0x44: 'COND_EXEC', 0x45: 'COND_WRITE', 0x46: 'EVENT_WRITE',
    0x48: 'ME_INIT', 0x4A: 'SET_SHADER_BASES', 0x4B: 'SET_BIN_BASE_OFFSET', 0x4F: 'MEM_WRITE_CNTR',
    0x50: 'SET_BIN_MASK', 0x51: 'SET_BIN_SELECT', 0x52: 'WAIT_REG_EQ', 0x53: 'WAIT_REG_GTE', 0x54: 'INTERRUPT',
    0x55: 'SET_CONSTANT2', 0x56: 'SET_SHADER_CONSTANTS', 0x58: 'EVENT_WRITE_SHD', 0x59: 'EVENT_WRITE_CFL',
    0x5B: 'EVENT_WRITE_ZPD', 0x5C: 'WAIT_UNTIL_READ', 0x5D: 'WAIT_IB_PFD_COMPLETE', 0x5E: 'CONTEXT_UPDATE'}


def decode_pm4(words, base=0):
    """Walk Xenos command packets; on a word that is not a packet header, re-sync at the next 32-byte boundary (360 D3D
    starts each new segment there). Returns (type-3 opcode counts, type-0 register counts, nop words, resyncs, words)."""
    ops, regs, nops, resync, i = {}, {}, 0, 0, 0
    while i < len(words):
        h = words[i]
        t = h >> 30
        if t == 3 and ((h >> 8) & 0x7F) in PM4_OPS:
            n = ((h >> 16) & 0x3FFF) + 1
            op = (h >> 8) & 0x7F
            ops[op] = ops.get(op, 0) + 1
            i += 1 + n
        elif t == 0 and h != 0:
            n = ((h >> 16) & 0x3FFF) + 1
            reg = h & 0x7FFF
            regs[reg] = regs.get(reg, 0) + n
            i += 1 + n
        elif t == 2:
            nops += 1
            i += 1
        else:                                    # zero padding, type 1 or garbage: next 32-byte boundary
            resync += 1
            addr = base + 4 * i
            nxt = (addr + 32) & ~31
            i = (nxt - base) // 4
    return ops, regs, nops, resync, min(i, len(words))


def cmdstream(x, samples, label):
    """Use 360 D3D's primary list of submitted segments (device+13556: {.., .., write ptr @+8, end @+12}; entries
    {0x81000000 | words, GPU address}) to read whole command segments exactly, and decode them."""
    dev = struct.unpack_from('>I', x.getmem(0x82171480 + 0xB84, 4), 0)[0]
    prim = struct.unpack_from('>4I', x.getmem(dev + 13556, 16), 0)
    diag = ['  360 D3D device at %08X; segment list fields %08X %08X %08X %08X' % ((dev,) + prim)]
    tot_ops, tot_regs, segs, words_total, bad, nops = {}, {}, 0, 0, 0, 0
    seen = set()
    shown = []
    skips = {'not a segment entry': 0, 'odd address': 0, 'too big': 0, 'read error': 0, 'already seen': 0}
    t_end = time.time() + 300
    for k in range(samples):
        if time.time() > t_end:
            break
        print('\r  sample %d of %d, %d segments read so far' % (k + 1, samples, segs), end='', flush=True)
        try:
            wp = struct.unpack_from('>I', x.getmem(dev + 13564, 4), 0)[0]
            ent = x.getmem(wp - 8 * 8, 8 * 8)            # the 8 most recent entries
        except IOError:
            skips['read error'] += 1
            continue
        if len(diag) < 8:
            diag.append('  sample %d: list write pointer %08X, entries %s' % (
                k, wp, ' '.join('%08X:%08X' % struct.unpack_from('>II', ent, 8 * i) for i in range(8))))
        for i in range(8):
            hdr, phys = struct.unpack_from('>II', ent, 8 * i)
            if hdr >> 24 != 0x81:
                skips['not a segment entry'] += 1
                continue
            n = hdr & 0xFFFFFF
            if phys & 3 or phys >= 0x20000000:
                skips['odd address'] += 1
                continue
            if n == 0 or n > 65536:
                skips['too big'] += 1
                continue
            if phys in seen:
                skips['already seen'] += 1
                continue
            seen.add(phys)
            try:
                data = x.getmem(phys + 0xA0000000, 4 * n)
            except IOError as e:
                skips['read error'] += 1
                if len(diag) < 10:
                    diag.append('  read error: %s' % e)
                continue
            words = list(struct.unpack('>%dI' % n, data))
            ops, regs, nn, rs, used = decode_pm4(words, phys)
            segs += 1
            bad += rs > 0
            nops += nn
            words_total += n
            for o, c in ops.items():
                tot_ops[o] = tot_ops.get(o, 0) + c
            for r, c in regs.items():
                tot_regs[r] = tot_regs.get(r, 0) + c
            if len(shown) < 6 and n <= 4096:
                shown.append((phys, words))
    draws = sum(tot_ops.get(o, 0) for o in (0x22, 0x36, 0x34, 0x35))
    out = ['', 'command stream: %d whole segments read from 360 D3D\'s segment list, %d words, %d draw packets; %d segments'
           ' contained a word that is not a packet header' % (segs, words_total, draws, bad),
           '  skipped list entries: ' + ', '.join('%s %d' % kv for kv in skips.items()),
           '  %.1f command words per draw' % (words_total / max(1, draws))] + diag + [
           '  type-3 packets, most frequent first (count, per draw):']
    for o, c in sorted(tot_ops.items(), key=lambda kv: -kv[1]):
        out.append('    %6d  %6.2f per draw  0x%02X %s' % (c, c / max(1, draws), o, PM4_OPS.get(o, '?')))
    out.append('  type-0 register writes, most written registers first (values written, per draw):')
    for r, c in sorted(tot_regs.items(), key=lambda kv: -kv[1])[:30]:
        out.append('    %6d  %6.2f per draw  register 0x%04X' % (c, c / max(1, draws), r))
    out.append('  type-2 NOP words: %d' % nops)
    out.append('')
    out.append('  some whole segments, packet by packet:')
    for phys, ws in shown:
        out.append('    segment at %08X (%d words):' % (phys, len(ws)))
        i = 0
        while i < len(ws):
            h = ws[i]; t = h >> 30
            if t == 3:
                n = ((h >> 16) & 0x3FFF) + 1
                op = (h >> 8) & 0x7F
                out.append('      %-22s %s' % (PM4_OPS.get(op, 'op 0x%02X' % op),
                                              ' '.join('%08X' % w for w in ws[i + 1:i + 1 + min(n, 6)]) + (' ...' if n > 6 else '')))
                i += 1 + n
            elif t == 0 and h:
                n = ((h >> 16) & 0x3FFF) + 1
                out.append('      regs 0x%04X x%-4d    %s' % (h & 0x7FFF, n, ' '.join('%08X' % w for w in ws[i + 1:i + 1 + min(n, 6)])
                                                          + (' ...' if n > 6 else '')))
                i += 1 + n
            elif t == 2:
                out.append('      NOP'); i += 1
            else:
                out.append('      ?? %08X' % h); i += 1
    text = '\n'.join(out)
    print(text)
    fn = 'xefu_cmdstream_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


def findsurface(x, label):
    """Look for the render-target settings (640x480, 4x anti-aliasing) in the 360 D3D device and the emulator's data."""
    dev = struct.unpack_from('>I', x.getmem(0x82171480 + 0xB84, 4), 0)[0]
    areas = [('360 D3D device', dev, 0x6000), ('emulator GPU globals', 0x82170000, 0x4000),
             ('emulator data', 0x82160000, 0x10000), ('emulator data 2', 0x82500000, 0x30000)]
    out = ['', 'render-target search (360 D3D device at %08X)' % dev]
    for name, base, size in areas:
        print('  reading %s %08X-%08X ...' % (name, base, base + size), flush=True)
        try:
            data = x.getmem(base, size)
        except IOError as e:
            out.append('  %s: %s' % (name, e))
            continue
        ws = struct.unpack('>%dI' % (len(data) // 4), data)
        found = 0
        for i, v in enumerate(ws):
            why = None
            if v in (0x0A020280, 0x0A010280, 0x0A000280):
                why = 'surface info %08X' % v
            elif v == 0x01E00280:
                why = 'window 640x480'
            elif v == 640 and i + 1 < len(ws) and ws[i + 1] == 480:
                why = '640 then 480'
            if why:
                found += 1
                if found > 40:
                    continue
                lo = max(0, i - 8)
                out.append('  %s +0x%04X (%08X): %s' % (name, 4 * i, base + 4 * i, why))
                out.append('      ' + ' '.join(('[%08X]' if j == i else '%08X') % ws[j] for j in range(lo, min(len(ws), i + 9))))
        out.append('  %s: %d hits' % (name, found))
    text = '\n'.join(out)
    print(text)
    fn = 'xefu_surface_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


def sample_threads(x, n, label):
    """Freeze the game briefly (stop / go), many times, and record where Halo 2, the GPU and service threads were."""
    want = {0x816A0400: 'Halo 2 (x86)', 0x820CDA50: 'GPU thread', 0x820326D8: 'service thread'}
    targets = []
    for tid, info in threads(x):
        try:
            st = num(info.get('start', '0'))
        except ValueError:
            continue
        if st in want:
            targets.append((tid, want[st]))
    out = ['', 'thread sampling: %d rounds, threads %s' % (n, ', '.join('%s 0x%08X' % (r, t) for t, r in targets))]
    samples = {t: [] for t, r in targets}
    first_raw = []
    errors = 0
    t0 = time.time()
    for i in range(n):
        print('\r  sample %d of %d' % (i + 1, n), end='', flush=True)
        # freeze the whole game at once (debugger "break all") so every thread shows where it really was
        try:
            x.cmd('stop')
        except (IOError, ValueError):
            errors += 1
            continue
        try:
            for tid, role in targets:
                try:
                    c, h, ls = x.cmd('getcontext thread=0x%08X control int' % tid)
                    d = kv(h + ' ' + ' '.join(ls))
                    if len(first_raw) < 3:
                        first_raw.append('%s: %s | %s' % (role, h, ' '.join(ls))[:300])
                    samples[tid].append((num(d.get('iar', '0')), num(d.get('lr', '0'))))
                except (IOError, ValueError):
                    errors += 1
        finally:
            x.cmd('go')
    print()
    out.append('  %.0f s, %d read errors' % (time.time() - t0, errors))
    out += ['  first reply: ' + r for r in first_raw]

    def region(a):
        if 0x82000000 <= a < 0x82580000:
            return 'emulator code'
        if 0x80000000 <= a < 0x82000000:
            return '360 kernel / system'
        return 'other (translated Halo 2 code / heap)'
    for tid, role in targets:
        ss = samples[tid]
        out.append('')
        out.append('  %s (0x%08X): %d samples' % (role, tid, len(ss)))
        if not ss:
            continue
        reg = {}
        for a, l in ss:
            reg[region(a)] = reg.get(region(a), 0) + 1
        for k, v in sorted(reg.items(), key=lambda kv_: -kv_[1]):
            out.append('    %5.1f%%  %s' % (100.0 * v / len(ss), k))
        buck = {}
        for a, l in ss:
            k = (a & ~0xFF) if region(a) != 'other (translated Halo 2 code / heap)' else (a & ~0xFFFF)
            buck[k] = buck.get(k, 0) + 1
        out.append('    busiest places (address / 256 bytes, translated code / 64 KB):')
        for k, v in sorted(buck.items(), key=lambda kv_: -kv_[1])[:25]:
            out.append('      %5.1f%%  %08X  %s' % (100.0 * v / len(ss), k, region(k)))
    out.append('')
    out.append('  raw samples (thread, PC, caller):')
    for tid, role in targets:
        for a, l in samples[tid]:
            out.append('    %08X %08X %08X' % (tid, a, l))
    text = '\n'.join(out)
    print('\n'.join(out[:200]))
    fn = 'xefu_samples_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


X86_PCR = 0x8002B43C            # x86 kernel's processor block (KPCR) in the emulated Xbox
X86_IDLE_LOOP = 0x8001B69C      # its idle loop: sti / nop / nop / cli / DPC list empty? -> loop (a spin, no HLT)
CPU_OBJ_PTR = 0x82166C40        # emulator's x86 CPU object


def le32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def be32(b, o):
    return struct.unpack_from('>I', b, o)[0]


def x86_sample(x, secs, label):
    """Sample, from memory only, what the emulated Xbox CPU (Halo 2's hardware thread 0) is doing: which x86
    thread runs (the kernel's idle loop is a spin, so idle time looks like work), its IRQL / DPCs, and the
    emulator's own spin points (synchronous service-thread requests, the async request queue, the message ring)."""
    out = []
    obj = be32(x.getmem(CPU_OBJ_PTR, 4), 0)
    fields = x.getmem(obj, 0x200)
    out.append('x86 CPU object 0x%08X' % obj)
    ram = None
    for off in range(0x20, 0x88, 4):
        base = be32(fields, off)
        if not base or base & 0xFFF:
            continue
        try:
            p = x.getmem(base + (X86_PCR & 0x0FFFFFFF), 0x30)
        except (IOError, ValueError):
            continue
        if le32(p, 0x1C) == X86_PCR and le32(p, 0x20) == X86_PCR + 0x28:
            ram = base
            out.append('emulated Xbox memory at 0x%08X (object +0x%X); x86 PCR found and checked' % (base, off))
            break
    if ram is None:
        out.append('could not find the emulated Xbox memory (no x86 PCR at any candidate) - nothing measured')
        print('\n'.join(out))
        return
    # the CPU state page: object +180 = its kernel address (readable); +192 - 4096 = its user-mode address, which
    # becomes readable after adding the user->kernel offset at +124
    state = None
    for cand, how in ((be32(fields, 180), 'object +180'),
                      ((be32(fields, 192) - 4096 + be32(fields, 124)) & 0xFFFFFFFF, 'object +192 - 4096 + [+124]')):
        try:
            st0 = x.getmem(cand + 160, 8)
        except (IOError, ValueError):
            out.append('CPU state page not readable at 0x%08X (%s)' % (cand, how))
            continue
        state = cand
        out.append('x86 CPU state page 0x%08X (%s, EFLAGS %08X), runtime page 0x%08X' %
                   (state, how, be32(st0, 4), state - 4096))
        break
    if state is None:
        out.append('emulator wait points will not be measured (state page unreadable); x86 side only')
    runtime = state - 4096 if state is not None else None
    emu_ok = state is not None
    pcr_va = ram + (X86_PCR & 0x0FFFFFFF)
    rows = []
    fr0 = None
    delta = be32(fields, 124)          # user-mode address + delta = kernel address (the async thread uses it)
    qcodes, qcodes_reads, qcode_err = {}, [], [0]
    qeip, qraw, qblk = {}, [], []
    t0 = time.time()
    errors = emu_errors = 0
    nxt_print = t0 + 1
    while time.time() - t0 < secs:
        try:
            t = time.time()
            p = x.getmem(pcr_va, 0x60)
            fr = be32(x.getmem(0x8216F64C, 4), 0)
        except (IOError, ValueError):
            errors += 1
            continue
        s, r, g = bytes(0x60), bytes(12), bytes(8)
        if emu_ok:
            try:
                # read pointer first, write pointer after: the consumer only moves read toward write, so this order
                # can't make an empty queue look full (round 28-30 read them the other way round - false "full")
                r = x.getmem(runtime + 60, 12)       # async queue: base, limit, read pointer
                s = x.getmem(state + 560, 0x60)      # 564 irq pending, 616 request code, 632/636 request flags, 640 queue
                g = x.getmem(state + 1664, 8)        # message ring: write / read offsets
            except (IOError, ValueError):
                emu_errors += 1
                if emu_errors > 20 and not rows:
                    emu_ok = False
                s, r, g = bytes(0x60), bytes(12), bytes(8)
        if fr0 is None:
            fr0 = (t, fr)
        q_base, q_lim, q_rd = be32(r, 0), be32(r, 4), be32(r, 8)
        q_wr = be32(s, 80)
        q_size = (q_lim - q_base) if q_lim > q_base else 0
        q_used = ((q_wr - q_rd) % q_size) if q_size else 0
        if q_used and len(qcodes_reads) < 400:          # what is waiting in the queue: request code of each entry
            try:
                k = min(q_used // 64, 16, (q_lim - q_rd) // 64)   # one read: up to 16 entries, no wrap
                e = x.getmem((q_rd + delta) & 0xFFFFFFFF, 64 * k)
                for i in range(k):
                    c = be32(e, 64 * i + 40)
                    qcodes[c] = qcodes.get(c, 0) + 1
                    eip = be32(e, 64 * i + 32)
                    qeip[(c, eip)] = qeip.get((c, eip), 0) + 1
                    if len(qraw) < 64:
                        qraw.append(e[64 * i:64 * i + 64])
                    blk = be32(e, 64 * i + 4)                  # the device handler reads its request block here
                    if blk and len(qblk) < 200:
                        try:
                            qblk.append((c, eip, blk, x.getmem((blk + delta) & 0xFFFFFFFF, 32)))
                        except (IOError, ValueError):
                            qblk.append((c, eip, blk, None))
                qcodes_reads.append(k)
            except (IOError, ValueError):
                qcode_err[0] += 1
        rows.append({'t': t - t0, 'frame': fr, 'irql': p[0x24], 'cur': le32(p, 0x28), 'next': le32(p, 0x2C),
                     'idle': le32(p, 0x30), 'dpclist': le32(p, 0x50) != X86_PCR + 0x50, 'dpcact': le32(p, 0x58),
                     'pend': s[4], 'req': be32(s, 56), 'r632': be32(s, 72), 'r636': be32(s, 76),
                     'q_used': q_used, 'q_size': q_size, 'ring': (be32(g, 0) - be32(g, 4)) & 0xFFF,
                     'emu': s != bytes(0x60)})
        if time.time() >= nxt_print:
            nxt_print += 1
            print('\r  %3.0f s  %d samples' % (time.time() - t0, len(rows)), end='', flush=True)
    print()
    n = len(rows)
    if n < 2:
        out.append('too few samples (%d, %d read errors)' % (n, errors))
        print('\n'.join(out))
        return
    dt = rows[-1]['t'] - rows[0]['t']
    frames = (rows[-1]['frame'] - rows[0]['frame']) & 0xFFFFFFFF
    fps = frames / dt if dt > 0 else 0
    ft = 1000.0 / fps if fps else 0
    out.append('%d samples in %.1f s (%.0f a second), %d read errors; %.1f fps (frame %.2f ms)' %
               (n, dt, n / dt if dt else 0, errors, fps, ft))

    def share(cond):
        k = sum(1 for w in rows if cond(w))
        return 100.0 * k / n, ft * k / n
    out.append('')
    out.append('Emulated Xbox CPU (hardware thread 0), share of samples  [~ms per frame]:')
    idle_ptrs = {}
    for w in rows:
        idle_ptrs[w['idle']] = idle_ptrs.get(w['idle'], 0) + 1
    idle = max(idle_ptrs, key=idle_ptrs.get)
    cur = {}
    for w in rows:
        cur[w['cur']] = cur.get(w['cur'], 0) + 1
    for th, k in sorted(cur.items(), key=lambda kv_: -kv_[1])[:12]:
        tag = '  <- the kernel idle thread (spins in its idle loop)' if th == idle and idle else ''
        out.append('  %5.1f%%  [%5.2f]  x86 thread 0x%08X%s' % (100.0 * k / n, ft * k / n, th, tag))
    out.append('  (idle thread pointer from the PCR: 0x%08X)' % idle)
    for name, cond in (('IRQL 0 (normal code)', lambda w: w['irql'] == 0),
                       ('IRQL 1 (APC)', lambda w: w['irql'] == 1),
                       ('IRQL 2 (dispatch / DPC)', lambda w: w['irql'] == 2),
                       ('IRQL > 2 (interrupt handler)', lambda w: w['irql'] > 2),
                       ('a DPC routine running', lambda w: w['dpcact'] != 0),
                       ('DPCs queued', lambda w: w['dpclist']),
                       ('a thread switch pending (NextThread set)', lambda w: w['next'] != 0),
                       ('an x86 interrupt pending (not yet taken)', lambda w: w['pend'] != 0)):
        a, b = share(cond)
        out.append('  %5.1f%%  [%5.2f]  %s' % (a, b, name))
    out.append('')
    emu_rows = [w for w in rows if w['emu']]
    out.append('Emulator spin points on hardware thread 0 (Halo 2\'s thread waits here, it never sleeps): '
               '%d samples with emulator data, %d read errors there' % (len(emu_rows), emu_errors))
    if not emu_rows:
        out.append('  (not measured)')
    rows_all, rows, n = rows, (emu_rows or rows), len(emu_rows or rows)
    if emu_rows:
        a, b = share(lambda w: w['r636'] == 0)
        out.append('  %5.1f%%  [%5.2f]  waiting for the service thread (synchronous request in flight)' % (a, b))
        kinds = {}
        for w in rows:
            if w['r636'] == 0:
                kinds[w['req']] = kinds.get(w['req'], 0) + 1
        names = {4: 'code not translated yet (block lookup miss)', 260: 'block lookup miss (call)', 516: 'return lookup',
                 772: 'indirect branch lookup'}
        for k, c in sorted(kinds.items(), key=lambda kv_: -kv_[1])[:8]:
            out.append('           %5.1f%%  request 0x%X %s' % (100.0 * c / n, k, names.get(k, '')))
        qs = [w['q_used'] for w in rows if w['q_size']]
        if qs:
            size = rows[0]['q_size']
            a, b = share(lambda w: w['q_size'] and w['q_used'] >= w['q_size'] - 64)
            out.append('  %5.1f%%  [%5.2f]  async request queue full (Halo 2 spins until there is room); '
                       'average use %.1f of %d entries' % (a, b, sum(qs) / len(qs) / 64.0, size // 64))
        if qcodes:
            tot = sum(qcodes.values())
            out.append('           what was waiting in the queue (%d entries read in %d samples, %d read errors):' %
                       (tot, len(qcodes_reads), qcode_err[0]))
            for c, k in sorted(qcodes.items(), key=lambda kv_: -kv_[1])[:12]:
                try:
                    h = be32(x.getmem(obj + ((c & 0xFF) + 97) * 12 + 4, 4), 0)
                except (IOError, ValueError):
                    h = 0
                out.append('             %5.1f%%  code 0x%08X (device %d, request %d, handler %08X)' %
                           (100.0 * k / tot, c, c & 0xFF, (c >> 8) & 0xFF, h))
            out.append('           issued from (x86 address of the instruction that posted it):')
            for (c, eip), k in sorted(qeip.items(), key=lambda kv_: -kv_[1])[:16]:
                out.append('             %5.1f%%  code 0x%04X  x86 %08X' % (100.0 * k / tot, c, eip))
            ops = {}
            for c, eip, blk, b in qblk:
                if b is None:
                    key = (c, 'unreadable')
                else:
                    key = (c, 'volume %d, operation %d' % (be32(b, 0), be32(b, 8)))
                ops[key] = ops.get(key, 0) + 1
            if ops:
                out.append('           request blocks (%d read): word 0 = volume, word 2 = operation' % len(qblk))
                for (c, what), k in sorted(ops.items(), key=lambda kv_: -kv_[1])[:16]:
                    out.append('             %5.1f%%  code 0x%04X  %s' % (100.0 * k / len(qblk), c, what))
        rg = [w['ring'] for w in rows]
        a, b = share(lambda w: w['ring'] >= 4096 - 8)
        out.append('  %5.1f%%  [%5.2f]  message ring full; average use %.0f of 4096 bytes' % (a, b, sum(rg) / len(rg)))
    rows, n = rows_all, len(rows_all)
    if qraw or qblk:
        out.append('')
        out.append('queued entries (64 bytes each: x86 regs r3-r10, +32 EIP, +36 r1, +40 code, +44 r11, +48 r12, +52 r13):')
        for e in qraw:
            out.append('  ' + ' '.join('%08X' % be32(e, j) for j in range(0, 64, 4)))
        out.append('request blocks (code, EIP, block pointer, 32 bytes):')
        for c, eip, blk, b in qblk[:120]:
            out.append('  %04X %08X %08X %s' % (c, eip, blk, ' '.join('%08X' % be32(b, j) for j in range(0, 32, 4))
                                                if b else 'unreadable'))
    out.append('')
    out.append('raw: t frame irql cur next idle dpcact dpclist pend req r632 r636 q_used ring')
    for w in rows:
        out.append('  %.3f %d %d %08X %08X %08X %08X %d %d %X %08X %08X %d %d' % (
            w['t'], w['frame'], w['irql'], w['cur'], w['next'], w['idle'], w['dpcact'], w['dpclist'], w['pend'],
            w['req'], w['r632'], w['r636'], w['q_used'], w['ring']))
    text = '\n'.join(out)
    print('\n'.join(out[:out.index('raw: t frame irql cur next idle dpcact dpclist pend req r632 r636 q_used ring')]))
    fn = 'xefu_x86_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


ASYN = 0x8251C400
FS_OPS = {0: 'open/create', 1: 'op 1', 2: 'op 2', 3: 'open + query info', 4: 'set info', 5: 'directory query',
          6: 'op 6 (memory only)', 7: 'volume info', 8: 'volume info 2'}


def read_asyn(x):
    b = x.getmem(ASYN, 0x170)
    if b[:4] != b'ASYN':
        return None
    d = {'ticks': struct.unpack_from('>Q', b, 8)[0], 'calls': be32(b, 16), 'max': be32(b, 20),
         'wticks': struct.unpack_from('>Q', b, 24)[0], 'waits': be32(b, 32), 'wmax': be32(b, 36),
         'buckets': [(struct.unpack_from('>Q', b, 0x40 + 16 * i)[0], be32(b, 0x48 + 16 * i), be32(b, 0x4C + 16 * i))
                     for i in range(17)],
         'vol': [be32(b, 0x150 + 4 * i) for i in range(8)]}
    return d


def asyncstats(x, secs, label):
    """L30 build: how busy the emulator's async (file-system) thread is and what each request costs."""
    out = []
    a = read_asyn(x)
    if a is None:
        print('this build has no async-thread timers (needs build L30)')
        return
    f0 = be32(x.getmem(0x8216F64C, 4), 0)
    t0 = time.time()
    snaps = []
    while time.time() - t0 < secs:
        time.sleep(1.0)
        try:
            b = read_asyn(x)
            fr = be32(x.getmem(0x8216F64C, 4), 0)
        except (IOError, ValueError):
            continue
        snaps.append((time.time(), fr, b))
        p_t, p_f, p = snaps[-2] if len(snaps) > 1 else (t0, f0, a)
        dt = snaps[-1][0] - p_t
        calls = (b['calls'] - p['calls']) & 0xFFFFFFFF
        busy = (b['ticks'] - p['ticks']) / TB_MS
        sleep = (b['wticks'] - p['wticks']) / TB_MS
        frames = (fr - p_f) & 0xFFFFFFFF
        print('  %3.0f s  %5.1f fps  requests %6d/s  async thread busy %5.1f %%  asleep %5.1f %%' % (
            snaps[-1][0] - t0, frames / dt, calls / dt, busy / (dt * 10), sleep / (dt * 10)))
    t1, f1, b = snaps[-1]
    dt = t1 - t0
    frames = (f1 - f0) & 0xFFFFFFFF
    fps = frames / dt
    calls = (b['calls'] - a['calls']) & 0xFFFFFFFF
    busy = (b['ticks'] - a['ticks']) / TB_MS
    sleep = (b['wticks'] - a['wticks']) / TB_MS
    out.append('async (file-system) thread, %.1f s, %.1f fps' % (dt, fps))
    out.append('  requests handled: %d (%.0f a second, %.1f a frame)' % (calls, calls / dt, calls / max(frames, 1)))
    out.append('  handling them: %.1f %% of the time (%.2f ms a frame), %.1f us each on average, longest ever %.0f us'
               % (busy / (dt * 10), busy / max(frames, 1), 1000 * busy / max(calls, 1), 1000 * b['max'] / TB_MS))
    out.append('  asleep (nothing queued): %.1f %% of the time; longest sleep ever %.1f ms; the rest (%.1f %%) is the'
               ' loop itself or being preempted' % (sleep / (dt * 10), b['wmax'] / TB_MS,
                                                     100 - busy / (dt * 10) - sleep / (dt * 10)))
    out.append('  by request (file-system operation from the request block):')
    rows = []
    for i in range(17):
        tk = b['buckets'][i][0] - a['buckets'][i][0]
        n = (b['buckets'][i][1] - a['buckets'][i][1]) & 0xFFFFFFFF
        if n:
            name = 'other request codes' if i == 16 else 'file system op %d %s' % (i, FS_OPS.get(i, ''))
            rows.append((tk, n, b['buckets'][i][2], name))
    for tk, n, mx, name in sorted(rows, reverse=True):
        out.append('    %6.2f ms a frame  %7.1f a frame  %7.1f us each  longest %7.0f us  %s' % (
            tk / TB_MS / max(frames, 1), n / max(frames, 1), 1000 * tk / TB_MS / n, 1000 * mx / TB_MS, name))
    vols = [(b['vol'][i] - a['vol'][i]) & 0xFFFFFFFF for i in range(8)]
    out.append('  by volume: ' + ', '.join('%s %d' % ('vol %d' % i if i < 7 else 'other', v) for i, v in enumerate(vols) if v))
    text = '\n'.join(out)
    print(text)
    fn = 'xefu_async_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


PROFX = 0x8251C400
PRING = 0x8251D000


def x86prof(x, secs, label):
    """P31 build: sample Halo 2's x86 code address ~1000 times a second (from inside the emulator)."""
    h = x.getmem(PROFX, 0x30)
    if h[:4] != b'XPRF':
        print('this build has no x86 profiler (needs build P31)')
        return
    names = ('queued', 'idx', 'notx86', 'flag', 'noctx', 'size', 'runs')
    def hdr(b):
        return {'queued': be32(b, 0x10), 'idx': be32(b, 0x14), 'notx86': be32(b, 0x18), 'flag': be32(b, 0x1C),
                'noctx': be32(b, 0x20), 'runs': be32(b, 0x28)}
    h0 = hdr(h)
    f0 = be32(x.getmem(0x8216F64C, 4), 0)
    t0 = time.time()
    last = h0['idx']
    eips = {}
    lost = 0
    nxt = t0 + 1
    while time.time() - t0 < secs:
        time.sleep(0.2)
        try:
            idx = be32(x.getmem(PROFX + 0x14, 4), 0)
        except (IOError, ValueError):
            continue
        new = (idx - last) & 0xFFFFFFFF
        if new > 1024:
            lost += new - 1024
            last = (idx - 1024) & 0xFFFFFFFF
            new = 1024
        if new:
            a, n = last % 1024, new
            try:
                while n:
                    k = min(n, 1024 - a)
                    b = x.getmem(PRING + 4 * a, 4 * k)
                    for i in range(k):
                        e = be32(b, 4 * i)
                        eips[e] = eips.get(e, 0) + 1
                    n -= k
                    a = (a + k) % 1024
                last = idx
            except (IOError, ValueError):
                pass
        if time.time() >= nxt:
            nxt += 1
            print('\r  %3.0f s  %d samples' % (time.time() - t0, sum(eips.values())), end='', flush=True)
    print()
    t1 = time.time()
    h1 = hdr(x.getmem(PROFX, 0x30))
    f1 = be32(x.getmem(0x8216F64C, 4), 0)
    dbg = x.getmem(PROFX + 0x80, 0x600)
    dt = t1 - t0
    n = sum(eips.values())
    out = ['x86 profiler, %.1f s, %.1f fps' % (dt, ((f1 - f0) & 0xFFFFFFFF) / dt)]
    d = {k: (h1[k] - h0[k]) & 0xFFFFFFFF for k in h1}
    out.append('  DPCs queued %d (%.0f/s), ran %d; samples %d (%.0f/s); not on the x86 thread %d, x86 thread in emulator '
               'kernel code %d, no context %d; lost (ring overrun) %d' % (
                   d['queued'], d['queued'] / dt, d['runs'], d['idx'], d['idx'] / dt, d['notx86'], d['flag'],
                   d['noctx'], lost))
    if not n:
        out.append('  no samples collected')
    else:
        def region(e):
            if 0x80010000 <= e < 0x80031000:
                return 'x86 Xbox kernel (emulator\'s)'
            if 0x00010000 <= e < 0x01000000:
                return 'Halo 2 (XBE code/data range)'
            return 'other'
        reg = {}
        for e, c in eips.items():
            reg[region(e)] = reg.get(region(e), 0) + c
        out.append('  where (%d samples):' % n)
        for k, c in sorted(reg.items(), key=lambda kv_: -kv_[1]):
            out.append('    %5.1f%%  %s' % (100.0 * c / n, k))
        pages = {}
        for e, c in eips.items():
            pages[e & ~0xFFF] = pages.get(e & ~0xFFF, 0) + c
        out.append('  busiest 4 KB pages:')
        for pg, c in sorted(pages.items(), key=lambda kv_: -kv_[1])[:30]:
            out.append('    %5.1f%%  %08X  %s' % (100.0 * c / n, pg, region(pg)))
        buck = {}
        for e, c in eips.items():
            buck[e & ~0x3F] = buck.get(e & ~0x3F, 0) + c
        out.append('  busiest 64-byte blocks:')
        for bk, c in sorted(buck.items(), key=lambda kv_: -kv_[1])[:60]:
            out.append('    %5.2f%%  %08X' % (100.0 * c / n, bk))
    out.append('')
    out.append('first contexts captured (64-bit words from +0; r2 = EIP expected at +0x60):')
    for i in range(4):
        ws = [struct.unpack_from('>Q', dbg, 0x180 * i + 8 * j)[0] for j in range(0x30)]
        if any(ws):
            for j in range(0, 0x30, 4):
                out.append('  %d +%03X: %s' % (i, 8 * j, ' '.join('%016X' % w for w in ws[j:j + 4])))
    out.append('')
    out.append('all samples (EIP count):')
    for e, c in sorted(eips.items()):
        out.append('  %08X %d' % (e, c))
    text = '\n'.join(out)
    print('\n'.join(out[:out.index('all samples (EIP count):')]))
    fn = 'xefu_x86prof_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


# the 30 hottest Halo 2 functions at the Zanzibar dip (P31 profile, decomp function list): (x86 start, size)
HOT_FUNCS = [(0x2DC220, 0xC34), (0x3FAFA0, 0x2D1), (0x016B90, 0x3CC), (0x04D0B0, 0x58A), (0x01B0A0, 0x18D),
             (0x019CA0, 0x4C9), (0x01C8C0, 0x1DC), (0x131E50, 0x71), (0x04DAA0, 0x1BF), (0x3F7440, 0x1BE),
             (0x04CBF0, 0x4B4), (0x0143C0, 0xBD), (0x01AE70, 0x1D8), (0x01C330, 0x167), (0x018A90, 0x2E0),
             (0x03BB80, 0x46), (0x01BF50, 0x179), (0x04E260, 0x966), (0x3F7330, 0xD0), (0x3F6C60, 0x90),
             (0x023CA0, 0x39F), (0x13DA70, 0x252), (0x01C710, 0xD5), (0x01CF50, 0x30), (0x131BB0, 0x69),
             (0x173520, 0x361), (0x3FA3D0, 0x203), (0x041040, 0x441), (0x045CE0, 0x3E5), (0x034100, 0x318)]
OBJ_GLOBALS = range(0x82166C04, 0x82166C44, 4)     # the emulator's objects, allocated at start-up (0x820201A0)


def dumpcode(x, label):
    """Find the emulator's table of translated x86 blocks and dump the 360 code generated for Halo 2's hottest
    functions (no build needed; read-only)."""
    out = []
    cpuobj = be32(x.getmem(0x82166C40, 4), 0)
    delta = be32(x.getmem(cpuobj + 124, 4), 0)
    out.append('x86 CPU object 0x%08X, user->kernel offset 0x%08X' % (cpuobj, delta))
    probes = [a for a, n in HOT_FUNCS[:8]]
    table = None
    for g in OBJ_GLOBALS:
        try:
            o = be32(x.getmem(g, 4), 0)
            t = be32(x.getmem(o + 32, 4), 0) if o else 0
        except (IOError, ValueError):
            continue
        if not t:
            continue
        hits = 0
        for e in probes:
            try:
                b = x.getmem(t + ((e << 2) & 0xFFFF0), 32 * 16)
            except (IOError, ValueError):
                hits = -1
                break
            if any(be32(b, 16 * i + 4) == e for i in range(32)):
                hits += 1
        out.append('  object at [0x%08X] = 0x%08X, +32 = 0x%08X: %s' % (
            g, o, t, 'unreadable' if hits < 0 else '%d of %d hot function starts found' % (hits, len(probes))))
        if hits > 0 and (table is None or hits > table[1]):
            table = (t, hits)
    if table is None:
        out.append('block table not found')
        text = '\n'.join(out)
        print(text)
        open('xefu_code_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S')), 'w').write(text + '\n')
        return
    t = table[0]
    out.append('block table at 0x%08X; reading all of it (1 MB)...' % t)
    print('\n'.join(out))
    entries = []
    for off in range(0, 0x100000, 0x400):
        if off % 0x10000 == 0:
            print('\r  table %3d%%' % (off * 100 // 0x100000), end='', flush=True)
        try:
            b = x.getmem(t + off, 0x400)
        except (IOError, ValueError):
            continue
        for i in range(0, 0x400, 16):
            w = struct.unpack_from('>IIII', b, i)
            if w[0] and w[1]:
                entries.append(w)
    print()
    out.append('  %d translated blocks in the table' % len(entries))
    eips = [e[1] for e in entries]
    out.append('  x86 range of blocks: %08X..%08X; blocks in Halo 2 code (< 0x01000000): %d, x86 kernel: %d' % (
        min(eips), max(eips), sum(1 for e in eips if e < 0x01000000), sum(1 for e in eips if e >= 0x80000000)))
    by_code = sorted(entries, key=lambda e: e[0])
    nxt = {by_code[i][0]: by_code[i + 1][0] for i in range(len(by_code) - 1)}
    want = []
    for e in entries:
        for a, n in HOT_FUNCS:
            if a <= e[1] < a + n:
                want.append(e)
                break
    out.append('  blocks inside the 30 hot functions: %d' % len(want))
    out.append('')
    out.append('all table entries (code, x86 EIP, word 2, word 3):')
    for e in sorted(entries, key=lambda e: e[1]):
        out.append('  %08X %08X %08X %08X' % e)
    out.append('')
    out.append('translated code of the hot blocks (x86 EIP, code address, words up to the next block, max 1 KB):')
    print('  reading %d blocks of translated code...' % len(want))
    codes = []
    for k, e in enumerate(sorted(want, key=lambda e: e[1])):
        size = min(max(nxt.get(e[0], e[0] + 0x400) - e[0], 4), 0x400)
        code = None
        for base in ((e[0] + delta) & 0xFFFFFFFF, e[0]):
            try:
                code = x.getmem(base, size)
                break
            except (IOError, ValueError):
                continue
        out.append('block %08X code %08X size %X words 2-3 %08X %08X' % (e[1], e[0], size, e[2], e[3]))
        if code is None:
            out.append('  unreadable')
            continue
        codes.append((e[1], code))
        for i in range(0, len(code), 32):
            out.append('  ' + ' '.join('%08X' % be32(code, j) for j in range(i, min(i + 32, len(code)), 4)))
    summ = code_patterns(codes)
    out[12:12] = summ
    text = '\n'.join(out)
    fn = 'xefu_code_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('\n'.join(out[:12 + len(summ)]))
    print('saved', fn)


PATTERN_BLOCKS = (0x019CA0, 0x01AE70, 0x01BF50, 0x01C8C0, 0x131E50)


def code_patterns(codes):
    """Count the slow translator patterns in dumped code: x86 flags rebuilt with mfcr/mfxer after subfco. (R36
    removes them where movs sit between cmp and jcc), the precise x87 conversion (mtfsb0/mcrfs, removed by R35),
    and cmp/cmpl into cr7 (the fused fast form)."""
    def kinds(w):
        if (w & 0xFC1FFFFF) == 0x7C000026: return 'mfcr'
        if (w & 0xFC1FFFFF) == 0x7C0102A6: return 'mfxer'
        if (w & 0xFC0007FF) == 0x7C000411: return 'subfco.'
        if (w & 0xFC0007FE) == 0xFC00008C: return 'mtfsb0'
        if (w & 0xFC0007FE) == 0xFC000080: return 'mcrfs'
        if (w & 0xFFE007FE) in (0x7F800000, 0x7F800040): return 'cmp cr7'
        if (w & 0xFFE00000) in (0x2F800000, 0x2B800000): return 'cmpi cr7'
        return None
    names = ('subfco.', 'mfcr', 'mfxer', 'mtfsb0', 'mcrfs', 'cmp cr7', 'cmpi cr7')
    tot = dict.fromkeys(names, 0)
    per = {}
    for eip, code in codes:
        c = dict.fromkeys(names, 0)
        for i in range(0, len(code) - 3, 4):
            k = kinds(be32(code, i))
            if k:
                c[k] += 1
                tot[k] += 1
        per[eip] = c
    res = ['slow-pattern check over %d hot blocks: %s' % (len(codes), ', '.join('%s %d' % (n, tot[n]) for n in names))]
    for b in PATTERN_BLOCKS:
        if b in per:
            res.append('  block %06X: %s' % (b, ', '.join('%s %d' % (n, per[b][n]) for n in names)))
    res.append('')
    return res


LRRING = 0x8251B800


def x86prof2(x, secs, label):
    """P33 build: each sample = x86 EIP + the 360 instruction address (IAR) + LR of Halo 2's thread. Afterwards
    reads the translated-block table and the hot 360 code so the samples can be mapped to generated code."""
    hdr = lambda b: {'queued': be32(b, 0x10), 'idx': be32(b, 0x14), 'notx86': be32(b, 0x18), 'flag': be32(b, 0x1C),
                     'noctx': be32(b, 0x20), 'runs': be32(b, 0x28)}
    h0 = hdr(x.getmem(PROFX, 0x30))
    f0 = be32(x.getmem(0x8216F64C, 4), 0)
    cpuobj = be32(x.getmem(0x82166C40, 4), 0)
    co = x.getmem(cpuobj, 0x200)
    delta, glue_user, glue_k = be32(co, 124), be32(co, 148), (be32(co, 80) - 0x8000) & 0xFFFFFFFF
    t0 = time.time()
    last = h0['idx']
    samples = []
    lost = 0
    nxt = t0 + 1
    while time.time() - t0 < secs:
        time.sleep(0.15)
        try:
            idx = be32(x.getmem(PROFX + 0x14, 4), 0)
        except (IOError, ValueError):
            continue
        new = (idx - last) & 0xFFFFFFFF
        if new > 512:
            lost += new - 512
            last = (idx - 512) & 0xFFFFFFFF
            new = 512
        if new:
            try:
                a, n = last % 512, new
                while n:
                    k = min(n, 512 - a)
                    b = x.getmem(PRING + 8 * a, 8 * k)
                    l = x.getmem(LRRING + 4 * a, 4 * k)
                    for i in range(k):
                        samples.append((be32(b, 8 * i), be32(b, 8 * i + 4), be32(l, 4 * i)))
                    n -= k
                    a = (a + k) % 512
                last = idx
            except (IOError, ValueError):
                pass
        if time.time() >= nxt:
            nxt += 1
            print('\r  %3.0f s  %d samples' % (time.time() - t0, len(samples)), end='', flush=True)
    print()
    t1 = time.time()
    h1 = hdr(x.getmem(PROFX, 0x30))
    f1 = be32(x.getmem(0x8216F64C, 4), 0)
    dbg = x.getmem(PROFX + 0x80, 3 * 0x1C0)
    dt = t1 - t0
    d = {k: (h1[k] - h0[k]) & 0xFFFFFFFF for k in h1}
    out = ['x86 + 360 profiler, %.1f s, %.1f fps; %d samples (%.0f/s), lost %d; not x86 thread %d, flag %d, no ctx %d'
           % (dt, ((f1 - f0) & 0xFFFFFFFF) / dt, len(samples), len(samples) / dt, lost, d['notx86'], d['flag'],
              d['noctx'])]
    out.append('cpu object %08X, user->kernel %08X, runtime glue: user %08X (kernel %08X), size 0x7620' % (
        cpuobj, delta, glue_user, glue_k))
    # where the 360 PC was
    def where(iar):
        if glue_user <= iar < glue_user + 0x7620:
            return 'emulator runtime glue'
        if 0x82000000 <= iar < 0x82580000:
            return 'emulator code (kernel mode)'
        if 0x80000000 <= iar < 0x82000000:
            return '360 kernel'
        return 'translated Halo 2 / x86 code'
    reg = {}
    for e, i, l in samples:
        reg[where(i)] = reg.get(where(i), 0) + 1
    n = max(len(samples), 1)
    out.append('360 PC (IAR) of Halo 2\'s thread:')
    for k, c in sorted(reg.items(), key=lambda kv_: -kv_[1]):
        out.append('  %5.1f%%  %s' % (100.0 * c / n, k))
    glue = {}
    for e, i, l in samples:
        if where(i) == 'emulator runtime glue':
            o = i - glue_user
            glue[o & ~0x3F] = glue.get(o & ~0x3F, 0) + 1
    out.append('busiest runtime glue (offset in the glue, 64-byte blocks; glue source = .data 0x821402C8 + offset):')
    for o, c in sorted(glue.items(), key=lambda kv_: -kv_[1])[:25]:
        out.append('  %5.2f%%  +%04X  (= 0x%08X in the image)' % (100.0 * c / n, o, 0x821402C8 + o))
    buck = {}
    for e, i, l in samples:
        buck[i & ~0x3F] = buck.get(i & ~0x3F, 0) + 1
    hot = sorted(buck.items(), key=lambda kv_: -kv_[1])[:60]
    out.append('busiest 64-byte blocks of 360 code (IAR):')
    for a, c in hot[:40]:
        out.append('  %5.2f%%  %08X  %s' % (100.0 * c / n, a, where(a)))
    print('\n'.join(out))
    # translated-block table, to map IAR -> x86 block
    print('reading the translated-block table (1 MB)...')
    table = []
    try:
        tobj = be32(x.getmem(0x82166C3C, 4), 0)
        t = be32(x.getmem(tobj + 32, 4), 0)
        for off in range(0, 0x100000, 0x400):
            if off % 0x20000 == 0:
                print('\r  %3d%%' % (off * 100 // 0x100000), end='', flush=True)
            try:
                b = x.getmem(t + off, 0x400)
            except (IOError, ValueError):
                continue
            for k in range(0, 0x400, 16):
                w = struct.unpack_from('>IIII', b, k)
                if w[0] and w[1]:
                    table.append(w)
        print()
    except (IOError, ValueError):
        out.append('block table unreadable')
    out.append('')
    out.append('hot 360 code (IAR 64-byte blocks, 128 bytes from the block start, as words):')
    for a, c in hot:
        for base in ((a + delta) & 0xFFFFFFFF, a):
            try:
                code = x.getmem(base, 128)
                out.append('code %08X %d %s' % (a, c, ' '.join('%08X' % be32(code, j) for j in range(0, 128, 4))))
                break
            except (IOError, ValueError):
                continue
    out.append('')
    out.append('contexts (samples 0 / 4096 / 8192; 64-bit words; GPR n at +0x50+8n, LR +0x1A8, IAR +0x1AC, CR +0x1B0):')
    for k in range(3):
        ws = [struct.unpack_from('>Q', dbg, 0x1C0 * k + 8 * j)[0] for j in range(0x38)]
        if any(ws):
            for j in range(0, 0x38, 4):
                out.append('  %d +%03X: %s' % (k, 8 * j, ' '.join('%016X' % w for w in ws[j:j + 4])))
    out.append('')
    out.append('table entries (code, x86 EIP, w2, w3): %d' % len(table))
    for w in sorted(table, key=lambda w: w[0]):
        out.append('  %08X %08X %08X %08X' % w)
    out.append('')
    out.append('samples (x86 EIP, 360 IAR, LR):')
    for e, i, l in samples:
        out.append('  %08X %08X %08X' % (e, i, l))
    text = '\n'.join(out)
    fn = 'xefu_prof2_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


RTLG_CNT = 0x8251C400          # L37 (R36-based) places
RTLG_RING = 0x8251D000
RTLG_STAB = 0x8251B680
RTLG_N = 128                   # ring entries: 128 (version 1), 512 at 0xD7FF4000 (version 2, R66L)
RTLG_PLACES = ((0xD7FF8000, 0xD7FF9000, 0xD7FFA000), (0x8251C400, 0x8251D000, 0x8251B680))   # R63L (game RAM), L37


def rtlg_find(x):
    """which build's log is there: sets RTLG_CNT / RING / STAB to the place whose header reads 'RTLG'; False if none"""
    global RTLG_CNT, RTLG_RING, RTLG_STAB
    for cnt, ring, stab in RTLG_PLACES:
        try:
            if x.getmem(cnt, 4) == b'RTLG':
                RTLG_CNT, RTLG_RING, RTLG_STAB = cnt, ring, stab
                global RTLG_N
                RTLG_N = 128
                if be32(x.getmem(cnt + 4, 4), 0) == 2:            # R66L: version 2 = the 512-entry ring at 0xD7FF4000
                    RTLG_RING, RTLG_N = 0xD7FF4000, 512
                return True
        except (IOError, ValueError, OSError):
            pass
    return False
RT_CODES = {18: 'CLEAR HELD (R85): the colour slot taken by', 19: 'HELD CLEAR GIVEN (R85) at its bind to', 1: 'setup colour', 2: 'setup depth', 3: 'setup result', 4: 'save/restore 0x820C1058',
            5: 'clear? 0x820F66B8', 6: 'switch 0x820C5770', 7: 'save 0x820C2208 (eDRAM -> texture)', 8: 'insert 0x820C1410 (becomes kind)', 9: 'save returned', 10: 'D3D Resolve', 11: 'D3D Resolve returned', 12: 'GUEST CLEAR 0x820D4BB0 (mask in a)', 13: 'guest clear returned', 14: 'BIND 0x820C6210 entry', 15: 'guest clear: records after', 16: 'TEXTURE SETUP samples a render-target-format texture', 17: 'texture setup returned'}


def rt_counts(x):
    b = x.getmem(RTLG_CNT, 0x40)
    if b[:4] != b'RTLG':
        return None
    return {'n': be32(b, 8), 'codes': [be32(b, 0x10 + 4 * i) for i in range(8)], 'finish': be32(b, 0x30),
            'c2c28': be32(b, 0x34), 'c2ef8': be32(b, 0x38)}


def rt_stab(x):
    b = x.getmem(RTLG_STAB, 0x400)
    t = {}
    for i in range(32):
        s = be32(b, 32 * i)
        if s:
            t[s] = [be32(b, 32 * i + 4 * k) for k in range(1, 8)]
    return t


def rt_desc(e):
    w, h = e[5] >> 16, e[5] & 0xFFFF
    return ('%4dx%-4d fmt %08X msaa %s' % (w, h, e[6], str(e[7] >> 16) if e[7] < 0x40000 else '-')) if e[2] else '(none)'   # R67L3 stamps word 6 with the time base


TEXD = 0xD7FFA400


def texlog_lines(x):
    """R63L2: the texture record lookup's misses - the texture's 40-byte descriptor beside the descriptor of the render-target
    record (table kind 0) covering the same range, if any. The field that differs is why a render target is not found as
    a texture."""
    try:
        b = x.getmem(TEXD, 772)
    except (IOError, ValueError, OSError):
        return []
    n = be32(b, 768)
    if n == 0:
        return ['texture lookup log: no misses recorded']
    out = ['texture lookup log: %d misses so far; the last %d (newest last):' % (n, min(n, 8))]
    first = max(0, n - 8)
    for k in range(first, n):
        o = 96 * (k & 7)
        start, end, rt, frame = struct.unpack_from('>4I', b, o)
        td = struct.unpack_from('>10I', b, o + 16)
        rd = struct.unpack_from('>10I', b, o + 56)
        out.append('  miss %d frame %d: range %08X-%08X (%d KB)%s' % (k, frame, start, end, (end - start) >> 10,
                   '  render target record %08X' % rt if rt else '  no render target with that start in table 0'))
        out.append('    texture descriptor: ' + ' '.join('%08X' % v for v in td))
        if rt:
            out.append('    rt record desc:     ' + ' '.join('%08X' % v for v in rd))
            out.append('    differs at words:   ' + (' '.join(str(i) for i in range(10) if td[i] != rd[i]) or 'none'))
    return out


TABLE_BASE = 0xD7FF0000      # R59+: the per-page resource tables (kind 0 render targets at +0, kind 1 textures at +8192; 2048 entries each)


def records_lines(x, guest):
    """every resource record of both tables covering the 64 KB page of a guest address: range, state, slots, flags, the
    40-byte descriptor, host objects. Chain links: record + (page - record[+36] + 66) * 4."""
    page = (guest >> 16) & 0x7FF
    out = ['resource records for guest %08X (64 KB page %d):' % (guest, page)]
    for kind, off in ((0, 0), (1, 8192)):
        try:
            head = be32(x.getmem(TABLE_BASE + off + 4 * page, 4), 0)
        except (IOError, ValueError):
            out.append('  kind %d: table unreadable' % kind)
            continue
        rec, n = head, 0
        while rec and n < 32:
            try:
                b = x.getmem(rec, 232)
            except (IOError, ValueError):
                out.append('  kind %d: record %08X unreadable' % (kind, rec))
                break
            w = lambda o: be32(b, o)
            desc = ' '.join('%08X' % w(52 + 4 * i) for i in range(10))
            host = w(220)
            hflags = ''
            if host:
                try:
                    hflags = ' host-flags %X' % be32(x.getmem(host + 16, 4), 0)
                except (IOError, ValueError):
                    pass
            out.append('  kind %d rec %08X: range %08X-%08X pages %d-%d state %X +4 %X +8 %X +16 %X +20 %X +44(kind) %X +92 %08X +96 %08X +100(tex) %08X +168 %X +176 %X +220(surf) %08X%s +228 %X' % (
                kind, rec, w(28), w(32), w(36), w(40), w(24), w(4), w(8), w(16), w(20), w(44), w(92), w(96), w(100), w(168), w(176), host, hflags, w(228)))
            out.append('      descriptor: ' + desc)
            out.append('      +100..+228: ' + ' '.join('%08X' % w(100 + 4 * i) for i in range(33)))   # R80L2: the host texture header and what follows (+164 the anti-aliasing class)
            start_page = w(36)
            link = rec + (page - start_page + 66) * 4
            try:
                rec = be32(x.getmem(link, 4), 0)
            except (IOError, ValueError):
                break
            n += 1
        if n == 0 and not head:
            out.append('  kind %d: no records' % kind)
    return out


def texdump_lines(x, guest):
    """the host texture of the render-target record (table kind 0) covering a guest address: the D3D texture header at
    record +100 (XGSetTextureHeader / XGOffsetResourceAddress built it), its base address, and sample rows of its pixels
    read straight from 360 memory - what the save's Resolve produced, before any fetch. Rows are read at the record's
    pitch (descriptor word 5), 12 rows spread over the height; the pixel words are counted by value."""
    page = (guest >> 16) & 0x7FF
    out = ['host texture of the render-target record for guest %08X:' % guest]
    try:
        rec = be32(x.getmem(TABLE_BASE + 4 * page, 4), 0)
    except (IOError, ValueError):
        return out + ['  table unreadable']
    found = None
    n = 0
    while rec and n < 32:
        b = x.getmem(rec, 232)
        if be32(b, 28) == guest:
            found = (rec, b)
            break
        rec = be32(x.getmem(rec + (page - be32(b, 36) + 66) * 4, 4), 0)
        n += 1
    if not found:
        return out + ['  no kind-0 record starts at that address']
    rec, b = found
    hdr = x.getmem(rec + 100, 56)
    words = [be32(hdr, 4 * i) for i in range(14)]
    out.append('  record %08X state %X; texture header +100..+152: %s' % (rec, be32(b, 24), ' '.join('%08X' % w for w in words)))
    pitch, wh = be32(b, 52 + 20), be32(b, 52 + 24)
    w, h = wh >> 16, wh & 0xFFFF
    if not (16 <= w <= 4096 and 16 <= h <= 4096 and pitch >= w * 4):
        pitch, w, h = be32(b, 72), be32(b, 76), be32(b, 80)
    out.append('  size from the record: %d x %d pitch %d' % (w, h, pitch))
    # the fetch constant's base address: a header word whose low 20 bits << 12 is a plausible physical address
    cands = []
    for i, wv in enumerate(words[7:], 7):
        base = (wv & 0xFFFFF) << 12
        if 0x10000 <= base < 0x20000000 and base not in [c[1] for c in cands]:
            cands.append((i, base))
    for i, base in cands[:3]:
        for va in (0xA0000000 | base, 0xC0000000 | base, 0xE0000000 | base):
            try:
                first = x.getmem(va, 16)
            except (IOError, ValueError, OSError):
                continue
            out.append('  word %d -> base %08X readable at %08X: first words %s' % (i, base, va, ' '.join('%08X' % be32(first, 4 * k) for k in range(4))))
            counts = {}
            rows = 12
            for r in range(rows):
                y = (h - 1) * r // (rows - 1)
                try:
                    row = x.getmem(va + y * pitch, w * 4, chunk=0x4000)
                except (IOError, ValueError, OSError):
                    out.append('    row %d unreadable' % y)
                    continue
                for k in range(0, w * 4, 4):
                    v = be32(row, k)
                    counts[v] = counts.get(v, 0) + 1
            total = sum(counts.values()) or 1
            top = sorted(counts.items(), key=lambda kv: -kv[1])[:6]
            out.append('    %d pixels sampled over %d rows: %d distinct values; most common: %s' % (
                total, rows, len(counts), ', '.join('%08X x%d (%.0f%%)' % (v, c, 100.0 * c / total) for v, c in top)))
            break
    if not cands:
        out.append('  no plausible base address in the header words')
    return out


def records(x, guest, label):
    lines = records_lines(x, guest)
    print('\n'.join(lines))
    fn = 'xefu_records_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write('\n'.join(lines) + '\n')
    print('saved', fn)
    return fn


def rtlog(x, secs, label):
    """L37 build: what the emulator's render-target code does each frame (eDRAM placement, evictions, saves,
    switches), per surface size and MSAA."""
    if not rtlg_find(x):
        print('this build has no render-target log (needs build L37 / R63L), or nothing has been logged yet')
        return None
    c0 = rt_counts(x)
    try:
        c0['r67'] = be32(x.getmem(RTLG_CNT + 0x48, 4), 0)
        c0['r76'] = be32(x.getmem(RTLG_CNT + 0x4C, 4), 0)
    except (IOError, ValueError, OSError):
        pass
    s0 = rt_stab(x)
    f0 = be32(x.getmem(0x8216F64C, 4), 0)
    t0 = time.time()
    events, lost = [], 0
    # R63L5 and later fill the ring once (128 events) and stop until the count is cleared; older builds wrap. Either way:
    # clear the count, wait, read what landed - a burst of consecutive events rather than a sample full of holes.
    try:
        x.cmd('setmem addr=0x%08X data=00000000' % (RTLG_CNT + 8))
    except (IOError, ValueError, OSError):
        pass
    while time.time() - t0 < secs:
        time.sleep(0.25)
        try:
            n = be32(x.getmem(RTLG_CNT + 8, 4), 0)
        except (IOError, ValueError):
            continue
        print('\r  %3.0f s  %d events in the ring' % (time.time() - t0, min(n, RTLG_N)), end='', flush=True)
        if n >= RTLG_N:
            break
    try:
        n = be32(x.getmem(RTLG_CNT + 8, 4), 0)
        ring = x.getmem(RTLG_RING, 32 * RTLG_N)
    except (IOError, ValueError):
        n, ring = 0, bytes(32 * RTLG_N)
    if n > RTLG_N:
        lost = n - RTLG_N                  # an older (wrapping) build: only the last RTLG_N are there
        first = n - RTLG_N
    else:
        first = 0
    for seq in range(first, n):
        o = 32 * (seq & (RTLG_N - 1))
        events.append((seq,) + struct.unpack_from('>8I', ring, o))
    print()
    dt = time.time() - t0
    c1 = rt_counts(x)
    s1 = rt_stab(x)
    f1 = be32(x.getmem(0x8216F64C, 4), 0)
    frames = max((f1 - f0) & 0xFFFFFFFF, 1)
    out = ['render-target log, %.1f s, %d frames (%.1f fps); %d events kept, %d lost (ring overrun)'
           % (dt, frames, frames / dt, len(events), lost)]
    out.append('  per frame (whole run, exact counters):')
    for code in range(1, 9):
        slot = 0 if code == 8 else code
        d = (c1['codes'][slot] - c0['codes'][slot]) & 0xFFFFFFFF
        out.append('    %7.2f  %s' % (d / frames, RT_CODES[code]))
    try:
        out.append('    %7.2f  R67: pending clears moved to the record taking over a slot (CNT+0x48)' % (((be32(x.getmem(RTLG_CNT + 0x48, 4), 0) - c0.get('r67', 0)) & 0xFFFFFFFF) / frames))
        r76 = be32(x.getmem(RTLG_CNT + 0x4C, 4), 0)
        out.append('    %7.2f  R76: the cache clean-out refused a colour view laid over a depth surface (CNT+0x4C; %d since the game started)' % (((r76 - c0.get('r76', 0)) & 0xFFFFFFFF) / frames, r76))
        try:                                     # R77L: who released the depth's colour view, and when a plain record was made over the depth
            who = {0x820C7C90: 'the cache clean-out 0x820C7B70', 0x820C7F40: 'the flush of every host texture 0x820C7F08 (called from 0x820C4D20)',
                   0x820C4908: '0x820C4904 (a record\'s texture remade)', 0x820C8160: '0x820C815C (0x820C8108)', 0x82135EF8: 'the timing wrapper 0x82135EE0'}
            nowf = be32(x.getmem(0x8216F64C, 4), 0)
            lat = x.getmem(RTLG_CNT + 0x54, 8)
            out.append('  R77: plain records made over exactly the current depth surface\'s memory: %d (the last at frame %d; now frame %d)' % (be32(lat, 0), be32(lat, 4), nowf))
            out.append('  R78: the depth surface\'s lookup refused a colour-format record over its memory (the view): %d since the game started' % be32(x.getmem(RTLG_CNT + 0x5C, 4), 0))
            pr = x.getmem(RTLG_CNT + 0x60, 8)
            out.append('  R79: a texture over the depth surface promoted at its bind: %d by this build (the depth busy), %d by the game\'s own test - since the game started' % (be32(pr, 0), be32(pr, 4)))
            out.append('  R81: a view given its depth surface\'s anti-aliasing class: %d times since the game started' % be32(x.getmem(RTLG_CNT + 0x68, 4), 0))
            out.append('  R85: colour clears held when a record of another size took the colour slot: %d; given to a wider record at its bind: %d - since the game started (R83\'s old counter: %d)' % (be32(x.getmem(RTLG_CNT + 0x94, 4), 0), be32(x.getmem(RTLG_CNT + 0x90, 4), 0), be32(x.getmem(RTLG_CNT + 0x6C, 4), 0)))
            nrel = be32(x.getmem(RTLG_CNT + 0xFC, 4), 0)
            out.append('  R77: releases of a colour view laid over a depth surface: %d%s' % (nrel, ' - the last %d, oldest first:' % min(nrel, 8) if nrel else ''))
            raw = x.getmem(RTLG_CNT + 0x100, 256)
            for q in range(min(nrel, 8)):
                k = (nrel - min(nrel, 8) + q) & 7
                e = [be32(raw, 32 * k + 4 * w) for w in range(8)]
                out.append('    frame %d: record %08X at %08X format %08X state %X slots %X - released by %08X %s; before that %08X %s' % (
                    e[3], e[0], e[1], e[2], e[6], e[7], e[4], '(' + who[e[4]] + ')' if e[4] in who else '', e[5], '(' + who[e[5]] + ')' if e[5] in who else ''))
        except Exception as err:
            out.append('  R77: (could not read the release log: %s)' % err)
    except (IOError, ValueError, OSError):
        pass
    for k, name in (('finish', 'evictions (finish 0x820C30B0 inside the setup)'),
                    ('c2c28', '0x820C2C28 inside the setup'), ('c2ef8', '0x820C2EF8 inside the setup')):
        out.append('    %7.2f  %s' % (((c1[k] - c0[k]) & 0xFFFFFFFF) / frames, name))
    out.append('  surfaces (per frame: as colour target, as depth target, 0x820C1058, switches):')
    rows = []
    for s, v in s1.items():
        p = s0.get(s, [v[0], v[1], v[2], 0, 0, 0, 0])
        d = [((v[k] - p[k]) & 0xFFFFFFFF) / frames for k in range(3, 7)]
        if any(d):
            rows.append((sum(d), s, v, d))
    for tot, s, v, d in sorted(rows, reverse=True):
        out.append('    %08X  %4dx%-4d fmt %08X msaa %d   %6.2f %6.2f %6.2f %6.2f' % (
            s, v[0] >> 16, v[0] & 0xFFFF, v[1], v[2], d[0], d[1], d[2], d[3]))
    res = {}
    for e in events:
        if e[1] >> 24 == 3:
            res[(e[3], e[4])] = res.get((e[3], e[4]), 0) + 1
    out.append('  setup results (return value, evictions during it): ' +
               ', '.join('%X/%d x%d' % (r, ev, n) for (r, ev), n in sorted(res.items())))
    # R67L3: ring word 6 is the time base (50 MHz) instead of msaa|tiles - the version-2 ring on a build that stamps it
    stamped = RTLG_N == 512 and any(e[7] > 0x10000 for e in events)
    if stamped:
        out.append('  time between consecutive events (time base, 50 MHz): the biggest gaps in the capture')
        gaps = []
        for i in range(1, len(events)):
            dt = (events[i][7] - events[i - 1][7]) & 0xFFFFFFFF
            gaps.append((dt, events[i - 1], events[i]))
        for dt, p, q in sorted(gaps, reverse=True)[:12]:
            out.append('    %8.1f us  after %-24s %08X -> before %-24s %08X (frame %d)' % (
                dt / 50.0, RT_CODES.get(p[1] >> 24, str(p[1] >> 24)), p[2], RT_CODES.get(q[1] >> 24, str(q[1] >> 24)), q[2], q[1] & 0xFFFFFF))
        tot = {}
        for dt, p, q in gaps:
            k = RT_CODES.get(q[1] >> 24, str(q[1] >> 24))
            tot[k] = tot.get(k, 0) + dt
        out.append('  time preceding each event kind, summed over the capture (us): ' + ', '.join('%s %.0f' % (k, v / 50.0) for k, v in sorted(tot.items(), key=lambda kv: -kv[1])))
    byf = {}
    for e in events:
        byf.setdefault(e[1] & 0xFFFFFF, []).append(e)
    full = sorted(byf)[:8]
    out.append('  sample frames (events in order):')
    for f in full:
        out.append('   frame %d:' % f)
        for e in byf[f]:
            code = e[1] >> 24
            extra = ''
            if code == 10:
                extra = '  flags %X rect %08X' % (e[3], e[4])
            if code == 11:
                extra = '  result %08X (0 = D3D_OK) tex word0 %08X' % (e[3], e[4])
            if code in (12, 13):
                extra = ('  mask %08X (0xF0 colour, 1 Z, 2 stencil); current colour record %08X, depth record %08X' % (e[3], e[2], e[4])) if code == 12 else ('  after: colour record %08X pending %08X, depth record\'s pending %08X (-1 = no record)' % (e[2], e[3], e[4]))
            if code == 18:
                extra = '  the clear is pending on record %08X (%dx%d) - copied to the holding place' % (e[2], e[3] >> 16, e[3] & 0xFFFF)
            if code == 19:
                extra = '  bits %X' % e[2]
            if code == 15:
                extra = '  after: depth record %08X; handler result %08X' % (e[2], e[3])
            if code == 16:
                extra = '  texture at %08X format %08X (surface = the current colour record, for reference)' % (e[3], e[4])
            if code == 17:
                extra = '  texture record %08X state %X host-flags %X' % (e[3], e[4] & 0xFF, e[4] >> 8)
            if code == 14:
                extra = '  used %X written/changed %X (bit 0 colour, bit 1 depth); current depth record %08X' % (e[3], e[4], e[2])
            if code in (1, 2, 4, 7, 8, 9):                                        # a = record state (+24), b = host flags (+16) | arg << 8 (7: the caller; 8: the kind; 9: the result)
                extra = '  state %X host-flags %X%s' % (e[3], e[4] & 0xFF, (' arg %X' % (e[4] >> 8)) if code == 4 else ((' from %08X' % (e[4] >> 8)) if code == 7 else ((' kind %d' % (e[4] >> 8)) if code == 8 else ((' result %X' % (e[4] >> 8)) if code == 9 else ''))))
            if stamped:
                prev = byf[f][byf[f].index(e) - 1] if byf[f].index(e) > 0 else None
                extra += '  +%.0f us' % (((e[7] - prev[7]) & 0xFFFFFFFF) / 50.0) if prev else ''
            out.append('     %-24s %08X %-40s a %08X b %08X %s guest %08X%s' % (
                RT_CODES.get(code, str(code)), e[2], rt_desc(e), e[3], e[4], ('tb %08X' % e[7]) if stamped else ('tiles %d' % (e[7] & 0xFFFF)), e[8], extra))
    out.append('')
    out.append('all events (seq, code|frame, surface, a, b, w|h, fmt, msaa|tiles, guest):')
    for e in events:
        out.append('  %d %08X %08X %08X %08X %08X %08X %08X %08X' % e)
    # R63L8: every surface the log knows, with all resource records of its memory (both tables), so the records a
    # working target has (the game's back buffer) can be compared with the mod's picture target's
    # The record walk and the texture-content dumps (records_lines / texdump_lines) read megabytes over XBDM - minutes for a
    # 1280x960 surface - and were for the 6i.149 'what does the texture hold' question, long answered. Off unless --rtdeep.
    if RT_DEEP[0]:
        recs = ['', 'resource records of the surfaces in the captured events:']
        seen = set()
        for e in events:
            g = e[8]                          # the ring event's guest address (+28 of the surface record)
            if g and g not in seen and g < 0x08000000:
                seen.add(g)
                print('\r  reading the records and texture of surface %08X ...' % g, end='', flush=True)
                recs += records_lines(x, g)
                wdt, hgt = e[5] >> 16, e[5] & 0xFFFF
                if (wdt, hgt) not in ((640, 480), (160, 120), (64, 64), (128, 128), (512, 256)):
                    recs += texdump_lines(x, g)
        print()
    else:
        recs = ['', '(resource records and texture contents not read: add --rtdeep to the xefu command for them)']
    out[-1:-1] = [''] + texlog_lines(x) + recs + ['']
    text = '\n'.join(out)
    fn = 'xefu_rtlog_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('\n'.join(out[:60]))
    print('saved', fn)
    return fn


RT_DEEP = [False]


MEM_FLAGS = [(0x80000000, '16MB pages'), (0x20000000, '64KB pages (large)'), (0x40000000, 'physical / no 4KB'),
             (0x00000200, 'nocache'), (0x00000400, 'write-combine')]


def mem128(x, label):
    """Build R38 (128 MB Xbox): where the emulator put its memory and what the Xbox kernel thinks it has. Read-only."""
    out = []
    def rd(a, n=4):
        try:
            return x.getmem(a, n)
        except (IOError, ValueError) as e:
            return None
    def w(a):
        v = rd(a)
        return be32(v, 0) if v else None
    r38 = w(0x8202F1D0) == 0x3C801500
    out.append('emulator: %s' % ('R38 (region 336 MB, game RAM 128 MB)' if r38 else
                                 'not R38 (region word %s)' % ('%08X' % w(0x8202F1D0) if w(0x8202F1D0) is not None else 'unreadable')))
    base = w(0x8216F564)
    out.append('bitmap region: VA %s = 360 physical %s, %d MB -> ends at physical %s' % (
        '%08X' % base if base else '?', '%08X' % (base & 0x1FFFFFFF) if base else '?', 336 if r38 else 304,
        '%08X' % ((base & 0x1FFFFFFF) + (336 if r38 else 304) * 1048576) if base else '?'))
    out.append('16 MB block (vertex copies): VA %s;  GPU thread heap (%d MB): VA %s' % (
        '%08X' % w(0x8251862C) if w(0x8251862C) else '?', 80 if r38 else 112, '%08X' % w(0x82519640) if w(0x82519640) else '?'))
    cpu = w(0x82166C40)
    if cpu:
        f = {o: w(cpu + o) for o in (32, 100, 112, 116, 120, 124, 128)}
        out.append('cpuobj %08X: 96 MB block +32 = %08X, 32 MB block +100 = %08X' % (cpu, f[32] or 0, f[100] or 0))
        out.append('  game RAM: kernel VA +112 = %08X, end +116 = %08X (%d MB), physical +120 = %08X, +128 = %08X' % (
            f[112] or 0, f[116] or 0, ((f[116] or 0) - (f[112] or 0)) >> 20, f[120] or 0, f[128] or 0))
        if f[112] and f[116] and f[112] + 0x8000000 == f[116] and f[120] == 0x10000000:
            out.append('  -> 128 MB at 360 physical 0x10000000-0x18000000, as planned')
    bm_hi, bm_lo = w(0x8202EE8C), w(0x8202EE94)
    bm_addr = (((bm_hi & 0xFFFF) << 16) + struct.unpack('<h', struct.pack('<H', bm_lo & 0xFFFF))[0]) & 0xFFFFFFFF if bm_hi and bm_lo else (0x8216F2E8)
    out.append('region bitmap at %08X' % bm_addr)
    bm = rd(bm_addr, 672 if r38 else 608)
    if bm:
        free = sum(bin(c).count('1') for c in bm)
        units = len(bm) * 8
        runs, run, best = [], 0, 0
        for i in range(units):
            if (bm[i >> 3] >> (7 - (i & 7))) & 1:
                run += 1
                best = max(best, run)
            else:
                run = 0
        out.append('region bitmap: %d of %d 64 KB units free (%d MB), largest free run %d MB' % (free, units, free >> 4, best >> 4))
        used = []
        i = 0
        while i < units:
            if not (bm[i >> 3] >> (7 - (i & 7))) & 1:
                j = i
                while j < units and not (bm[j >> 3] >> (7 - (j & 7))) & 1:
                    j += 1
                if base:
                    used.append('%08X-%08X (%d MB)' % ((base & 0x1FFFFFFF) + i * 65536, (base & 0x1FFFFFFF) + j * 65536, (j - i) >> 4))
                i = j
            else:
                i += 1
        out.append('  used: ' + ', '.join(used))
    # the x86 kernel's view, through the game RAM's kernel alias (x86 0x80000000 = alias + 0)
    alias = (w(cpu + 112) if cpu else None) or 0xD0000000
    def x86(a):
        v = rd(alias + (a - 0x80000000))
        return struct.unpack('<I', v)[0] if v else None
    total, freep = x86(0x80030598), x86(0x8002FF0C)
    out.append('Xbox kernel: %s pages total (%s MB), %s pages free (%s MB); page tables start at page %s' % (
        total, (total or 0) >> 8, freep, (freep or 0) >> 8,
        '%X' % ((x86(0x8000F800) or 0) >> 12)))
    hi = rd(alias + 0x4000000, 16)
    out.append('game RAM at 64 MB (360 %08X): %s' % (alias + 0x4000000, hi.hex() if hi else 'unreadable'))
    hi = rd(alias + 0x7FE0000, 16)
    out.append('game RAM at 127.875 MB (the page tables, 360 %08X): %s' % (alias + 0x7FE0000, hi.hex() if hi else 'unreadable'))
    text = '\n'.join(out)
    print(text)
    fn = 'xefu_mem128_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


def memmap(x, label):
    """How the 360 maps the emulator's address space (XBDM walkmem) - page sizes behind the Xbox memory."""
    out = []
    code, head, lines = x.cmd('walkmem')
    out.append('walkmem: %s (%d regions)' % (head, len(lines)))
    regs = []
    for t in lines:
        d = kv(t)
        if 'base' not in d:
            continue
        try:
            b, sz = num(d.get('base', '0')), num(d.get('size', '0'))
        except ValueError:
            continue
        pr = num(d.get('protect', '0')) if 'protect' in d else 0
        regs.append((b, sz, pr, t))
    # largest regions first, then everything
    tot = {}
    for b, sz, pr, t in regs:
        tot[pr] = tot.get(pr, 0) + sz
    out.append('total size by protect value:')
    for pr, sz in sorted(tot.items(), key=lambda kv_: -kv_[1]):
        out.append('  protect %08X  %8.1f MB  %s' % (pr, sz / 1048576.0,
                   ', '.join(n for f, n in MEM_FLAGS if pr & f)))
    out.append('regions:')
    for b, sz, pr, t in sorted(regs):
        out.append('  %08X-%08X %8.1f MB  %s' % (b, b + sz, sz / 1048576.0, t))
    out.append('readability of addresses the x86 code uses:')
    for a in (0x00010000, 0x00100000, 0x00400000, 0x01000000, 0x03000000, 0x80000000, 0x80010000, 0x83000000,
              0xB7000000, 0xBA000000, 0xD0000000, 0xD0010000, 0xF0000000):
        try:
            v = x.getmem(a, 16)
            out.append('  %08X: %s' % (a, v.hex()))
        except (IOError, ValueError) as e:
            out.append('  %08X: unreadable (%s)' % (a, e))
    text = '\n'.join(out)
    print(text[:6000])
    fn = 'xefu_memmap_%s_%s.txt' % (label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ip', nargs='?', default='192.168.0.178')
    ap.add_argument('--measure', type=float, metavar='SECONDS')
    ap.add_argument('--label', default='')
    ap.add_argument('--interval', type=float, default=0.4, help='seconds between reads (default 0.4)')
    ap.add_argument('--port', type=int, default=730, help=argparse.SUPPRESS)
    ap.add_argument('--cmdstream', type=int, metavar='SAMPLES', help='sample the 360 GPU command stream SAMPLES times')
    ap.add_argument('--findsurface', action='store_true', help='find the render-target settings in memory')
    ap.add_argument('--sample', type=int, metavar='N', help='sample where Halo 2, GPU and service threads are, N times')
    ap.add_argument('--dumpcode', action='store_true',
                    help='dump the 360 code the emulator generated for Halo 2\'s hottest functions (any build)')
    ap.add_argument('--x86prof', type=float, metavar='SECONDS',
                    help='build P31: where Halo 2\'s x86 code spends its time (~1000 samples a second)')
    ap.add_argument('--asyncstats', type=float, metavar='SECONDS',
                    help='build L30: how busy the async file-system thread is and what each request costs')
    ap.add_argument('--x86sample', type=float, metavar='SECONDS',
                    help='sample, from memory, what the emulated Xbox CPU does (idle loop, IRQL, emulator waits)')
    ap.add_argument('--rtdeep', action='store_true', help='with --rtlog: also walk the resource records and dump the texture contents of every surface seen (slow: megabytes over XBDM)')
    ap.add_argument('--rtlog', type=float, metavar='SECONDS',
                    help='build L37: what the render-target code does each frame (eDRAM, evictions, saves)')
    ap.add_argument('--mem128', action='store_true', help='build R38: the emulator\'s memory layout and the Xbox kernel\'s page counts')
    ap.add_argument('--records', type=lambda v: int(v, 16), action='append', help='R59+: dump the resource records covering this guest address (hex), e.g. --records 03C84000')
    ap.add_argument('--texdump', type=lambda v: int(v, 16), action='append', help='R59+: read the host texture of the render-target record at this guest address (hex): header, base, sampled pixel values')
    ap.add_argument('--memmap', action='store_true',
                    help='list how the 360 maps memory (page sizes), read-only')
    a = ap.parse_args()
    RT_DEEP[0] = bool(getattr(a, 'rtdeep', False))
    x = Xbdm(a.ip, a.port)
    try:
        emu, frames, name, v = identify(x)
        problems = status(x, emu, name, v)
        if a.records:
            for g in a.records:
                records(x, g, a.label)
        if a.texdump:
            for g in a.texdump:
                print('\n'.join(texdump_lines(x, g)))
        if a.mem128:
            print()
            mem128(x, a.label)
        elif a.memmap:
            print()
            memmap(x, a.label)
        elif a.rtlog:
            print()
            rtlog(x, a.rtlog, a.label)
        elif a.dumpcode:
            print()
            dumpcode(x, a.label)
        elif a.x86prof:
            print()
            if be32(x.getmem(PROFX + 0x24, 4), 0) == 512:
                x86prof2(x, a.x86prof, a.label)
            else:
                x86prof(x, a.x86prof, a.label)
        elif a.asyncstats:
            print()
            asyncstats(x, a.asyncstats, a.label)
        elif a.x86sample:
            print()
            x86_sample(x, a.x86sample, a.label)
        elif a.sample:
            print()
            sample_threads(x, a.sample, a.label)
        elif a.findsurface:
            print()
            findsurface(x, a.label)
        elif a.cmdstream:
            print()
            cmdstream(x, a.cmdstream, a.label)
        elif a.measure:
            print()
            measure(x, emu, frames, v, a.measure, a.label, a.interval)
        elif problems:
            print('The thread placement above is not what this build sets up.')
    finally:
        x.close()


if __name__ == '__main__':
    main()
