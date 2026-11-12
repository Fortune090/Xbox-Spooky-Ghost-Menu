#!/usr/bin/env python3
"""boot_trace - watch the 360 emulator start up over XBDM and keep the last thing it was doing when the console hangs.

  python boot_trace.py 192.168.0.178 --label r38b      then start Halo 2 on the 360

Every ~0.3 s it freezes the title for a moment (XBDM stop / go), records where each emulator thread is (PC and
caller), reads a few words (the x86 kernel's page counts, the emulator's pending request, the region base), and
appends a line to the log - flushed at once, so the log survives the hang. If the console stops answering it keeps
trying to reconnect for a minute; if XBDM comes back it also reads the table of translated x86 blocks and maps the
x86 thread's last PCs to Halo 2 / Xbox kernel addresses. Read-only apart from stop / go. Needs xefu_probe.py beside it.
"""
import argparse
import datetime
import socket
import struct
import sys
import time
import xefu_probe as xp

ROLES = dict(xp.ROLES)
ROLES[0x816A0400] = 'x86 (Halo 2 + Xbox kernel)'


def rd32(x, a, le=False):
    try:
        b = x.getmem(a, 4)
    except (IOError, ValueError, OSError):
        return None
    return struct.unpack('<I' if le else '>I', b)[0]


def targets(x):
    out = []
    try:
        for tid, info in xp.threads(x):
            try:
                st = xp.num(info.get('start', '0'))
            except ValueError:
                continue
            out.append((tid, ROLES.get(st, 'thread %08X' % st), st))
    except (IOError, ValueError, OSError):
        pass
    return out


def one_sample(x, tgt):
    rows = []
    x.cmd('stop')
    try:
        for tid, role, st in tgt:
            try:
                c, h, ls = x.cmd('getcontext thread=0x%08X control' % tid)
                d = xp.kv(h + ' ' + ' '.join(ls))
                rows.append((tid, role, xp.num(d.get('iar', '0')), xp.num(d.get('lr', '0'))))
            except (IOError, ValueError):
                rows.append((tid, role, 0, 0))
    finally:
        x.cmd('go')
    return rows


def words(x):
    cpu = rd32(x, 0x82166C40)
    alias = rd32(x, cpu + 112) if cpu else None
    w = {}
    w['region'] = rd32(x, 0x8216F564)
    w['cpuobj'] = cpu
    if cpu:
        req = cpu and rd32(x, cpu + 268)
        w['req'] = rd32(x, req) if req else None            # the x86 side's pending request to the emulator (10..21)
        w['req2'] = rd32(x, req + 4) if req else None
        w['cr3'] = rd32(x, cpu + 200)
    if alias and alias >= 0xC0000000:
        w['pages'] = rd32(x, alias + 0x30598, True)          # x86 kernel: pages total
        w['free'] = rd32(x, alias + 0x2FF0C, True)           # pages free
        w['pde800'] = rd32(x, alias + 0xF800, True)          # PDE for 0x80000000: 4 MB page (..E3) or page table (..067)
        w['pde840'] = rd32(x, alias + 0xF840, True)          # PDE for 0x84000000
        w['irql'] = rd32(x, alias + (xp.X86_PCR & 0x0FFFFFFF) + 0x28, True)
    return w


def fmt(v):
    return '%08X' % v if v is not None else '--------'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ip', nargs='?', default='192.168.0.178')
    ap.add_argument('--label', default='')
    ap.add_argument('--interval', type=float, default=0.3)
    ap.add_argument('--port', type=int, default=730)
    ap.add_argument('--xex', metavar='FILE', help='first send this xefu1_1.xex to HDD:\\Compatibility\\xefu1_1.xex (keeps a one-time xefu1_1.xex.r36_original beside it)')
    ap.add_argument('--boot', nargs='?', const='', metavar='XBE_PATH',
                    help="then start Halo 2 over XBDM (magicboot) - the game's default.xbe path, or the one spooky_live.py remembers for this console")
    a = ap.parse_args()
    fn = 'boot_trace_%s_%s.txt' % (a.label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    log = open(fn, 'w')

    def say(t):
        print(t)
        log.write(t + '\n')
        log.flush()

    if a.xex is not None or a.boot is not None:
        import spooky_live as sl
        sx = sl.Xbdm(a.ip, a.port)
        if a.xex is not None:
            data = open(a.xex, 'rb').read()
            remote = 'HDD:\\Compatibility\\xefu1_1.xex'
            keep = remote + '.r36_original'
            if not sx.exists(keep) and sx.exists(remote):
                sx.rename(remote, keep)
                print('kept the console\'s current emulator as %s' % keep)
            try:
                if sx.exists(remote):
                    sx.delete(remote)
                sx.sendfile(remote, data)
            except IOError as e:
                print('%s - the emulator is running; restarting the console to the dashboard first' % e)
                sx.magicboot()
                if not sx.wait_back():
                    sys.exit('the console did not come back on XBDM')
                sx.reopen()
                if sx.exists(remote):
                    sx.delete(remote)
                sx.sendfile(remote, data)
            print('sent %s (%.1f MB) to %s' % (a.xex, len(data) / 2 ** 20, remote))
        if a.boot is not None:
            path = a.boot or sl.cached_xbe(a.ip)
            if not path:
                sys.exit("no game path: give --boot <path to default.xbe> once, or run spooky_live.py --find-xbe")
            print('starting %s: %s' % (path, sx.magicboot(path, path.rsplit('\\', 1)[0])))
            time.sleep(2)
            t_end = time.time() + 120
            while time.time() < t_end:
                try:
                    probe = xp.Xbdm(a.ip, a.port, timeout=5.0)
                    probe.close()
                    break
                except OSError:
                    time.sleep(2)
        try:
            sx.close()
        except Exception:
            pass
    x = xp.Xbdm(a.ip, a.port, timeout=5.0)
    say('connected: %s' % x.banner)
    say('waiting for the emulator to start (launch Halo 2 now)...')
    while True:
        try:
            name, ck, ts = xp.emulator(x)
        except (IOError, ValueError, OSError) as e:
            say('  (%s)' % e)
            name = None
        if name:
            say('emulator running: %s (check %08X, timestamp %08X)' % (name, ck, ts))
            break
        time.sleep(0.5)
    tgt = targets(x)
    say('threads: ' + ', '.join('%08X=%s' % (t, r) for t, r, s in tgt))
    say('columns: time | per thread PC/LR | region cpuobj req cr3 | x86 pages free pde800 pde840 irql')
    n = 0
    last = []
    t0 = time.time()
    while True:
        try:
            if n % 20 == 0:
                nt = targets(x)
                if nt and len(nt) != len(tgt):
                    tgt = nt
                    say('threads now: ' + ', '.join('%08X=%s' % (t, r) for t, r, s in tgt))
            rows = one_sample(x, tgt)
            w = words(x)
        except (IOError, ValueError, OSError, ConnectionError) as e:
            say('%7.2f s: console stopped answering: %s' % (time.time() - t0, e))
            break
        last.append(rows)
        last = last[-40:]
        say('%7.2f s | %s | %s %s %s %s | %s %s %s %s %s' % (
            time.time() - t0,
            ' '.join('%s:%08X/%08X' % (r[1][:8], r[2], r[3]) for r in rows),
            fmt(w.get('region')), fmt(w.get('cpuobj')), fmt(w.get('req')), fmt(w.get('cr3')),
            fmt(w.get('pages')), fmt(w.get('free')), fmt(w.get('pde800')), fmt(w.get('pde840')), fmt(w.get('irql'))))
        n += 1
        time.sleep(a.interval)
    # try to come back
    x2 = None
    for i in range(12):
        time.sleep(5)
        try:
            x2 = xp.Xbdm(a.ip, a.port, timeout=5.0)
            say('XBDM answers again after %d s' % (5 * (i + 1)))
            break
        except OSError as e:
            say('  reconnect %d: %s' % (i + 1, e))
    if x2 is None:
        say('XBDM is gone (the 360 itself is hung). The last lines above are the last known state.')
        say('saved ' + fn)
        return
    # still alive: sample the hung state a few more times and map the x86 thread's PCs
    try:
        tgt = targets(x2)
        for i in range(5):
            rows = one_sample(x2, tgt)
            last.append(rows)
            say('hung sample %d | %s' % (i, ' '.join('%s:%08X/%08X' % (r[1][:8], r[2], r[3]) for r in rows)))
            time.sleep(0.5)
        w = words(x2)
        say('hung words: ' + ' '.join('%s=%s' % (k, fmt(v)) for k, v in w.items()))
        cpu = w.get('cpuobj')
        # translated-block table (object +32 of one of the emulator globals), as xefu_probe --dumpcode finds it
        table = None
        for g in xp.OBJ_GLOBALS:
            o = rd32(x2, g)
            t = rd32(x2, o + 32) if o else None
            if t and t >= 0x82000000 or (t and t >= 0xC0000000):
                table = t
                break
        pcs = sorted({r[2] for rows in last for r in rows if r[1].startswith('x86')})
        if table and pcs:
            say('block table at %08X; mapping %d x86-thread PCs (reads 1 MB, slow)...' % (table, len(pcs)))
            best = {}
            for off in range(0, 0x100000, 0x400):
                try:
                    b = x2.getmem(table + off, 0x400)
                except (IOError, ValueError, OSError):
                    continue
                for i in range(0, 0x400, 16):
                    code, eip = struct.unpack_from('>II', b, i)
                    if not code or not eip:
                        continue
                    for pc in pcs:
                        if code <= pc < code + 0x4000 and (pc not in best or code > best[pc][0]):
                            best[pc] = (code, eip)
            for pc in pcs:
                if pc in best:
                    say('  PC %08X: block %08X = x86 %08X (+%X)' % (pc, best[pc][0], best[pc][1], pc - best[pc][0]))
                else:
                    say('  PC %08X: not in a translated block (%s)' % (pc, 'emulator' if 0x82000000 <= pc < 0x82580000 else 'other'))
    except (IOError, ValueError, OSError, ConnectionError) as e:
        say('lost it again: %s' % e)
    say('saved ' + fn)


if __name__ == '__main__':
    main()
