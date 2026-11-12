#!/usr/bin/env python3
"""run_test - NO-REBOOT RULE: this script never restarts the console and never launches a title as a warm-up; a restart
while the game has files open corrupted the game's maps (Nov 2026). It launches once (only from the dashboard), watches,
probes read-only, zips, and leaves the console as it is.

one run that does the whole round: (optionally) sends the emulator, boots Halo 2, watches the boot over
XBDM, recognises the known failure patterns, and, when a hang settles, runs the hang probe by itself.

  python run_test.py --label r47 --xex xefu1_1.xex --boot "HDD:\\Games\\Halo 2\\default.xbe"
  python run_test.py --label r47 --boot              (the path given once before is remembered in run_test_cache.txt)
  python run_test.py --label r47                     (you launch the game yourself)

Every 0.5 s it reads, from memory only (no stop/go, so the game is not disturbed): the emulator's region base and
cpuobj, the Xbox kernel's page counts, its first page-directory entry, the current x86 thread, and the emulator's
frame counter. Verdicts it can reach (written at the top of test_<label>_<time>.txt, with everything else below):

  PLAYING             frames advancing for 20 s after Halo 2's memory setup - go and test by hand
  INIT RUNAWAY        the kernel's page counters grow without end (a broken init loop)
  KERNEL STALL        kernel up, idle thread current, nothing allocated for 15 s (stalls in init / before the XBE)
  STUCK BEFORE SETUP  Halo 2's main thread running, but its 47.5 MB region never allocated and nothing moves
  STUCK AFTER SETUP   region allocated, then nothing moves and no frames for 15 s (the black screen)
  TITLE ENDED         the emulator vanished from memory (the console went back to the dashboard / logo)
  CONSOLE WEDGED      XBDM stopped answering (connection lost, or no answer of any kind) for a minute
  TIMEOUT             none of the above within --minutes

On KERNEL STALL / STUCK BEFORE SETUP / STUCK AFTER SETUP it runs hang_probe.py (same folder) and appends its output.
Once PLAYING, a minute after the first frame it takes a page map (pages.py: every page of the 128 MB by kernel page
type, the free runs below the contiguous cap, the occupants of the 16 MB window below 64 MB) and takes another when
you stop it with Ctrl+C in the game; both go into the zip.
Needs xefu_probe.py and hang_probe.py beside it; spooky_live.py too for --xex / --boot.
"""
import argparse
import datetime
import os
import re
import struct
import sys
import time
import threading
import xefu_probe as xp
import hang_probe

IDLE = 0x8002B2F0
CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'run_test_cache.txt')


def rd32(x, a, le=False):
    try:
        return struct.unpack('<I' if le else '>I', x.getmem(a, 4))[0]
    except (IOError, ValueError, OSError):
        return None


def sample(x):
    w = {'region': rd32(x, 0x8216F564), 'cpu': rd32(x, 0x82166C40), 'frames': rd32(x, 0x8216F64C)}
    cpu = w['cpu']
    alias = rd32(x, cpu + 112) if cpu else None
    if alias and alias >= 0xC0000000:
        w['total'] = rd32(x, alias + 0x30598, True)
        w['free'] = rd32(x, alias + 0x2FF0C, True)
        w['pde800'] = rd32(x, alias + 0xF800, True)
        w['cur'] = rd32(x, alias + (xp.X86_PCR & 0x0FFFFFFF) + 0x28, True)
    state = rd32(x, cpu + 180) if cpu else None
    if state:
        runtime = state - 4096
        q_base, q_lim, q_rd, q_wr = rd32(x, runtime + 60), rd32(x, runtime + 64), rd32(x, runtime + 68), rd32(x, state + 640)
        if q_base and q_lim and q_lim > q_base and q_rd is not None and q_wr is not None:
            w['queue'] = ((q_wr - q_rd) % (q_lim - q_base)) // 64
        w['req'] = rd32(x, state + 616)
    return w


def fmt(v):
    return '%08X' % v if v is not None else '--------'


def find_emulators(sx, say, depth=3):
    """every file whose name contains 'xefu' on every drive XBDM lists, a few levels deep"""
    try:
        c, h, ls = sx.cmd('drivelist')
    except Exception as e:
        say('drivelist: %s' % e)
        return
    drives = [m for m in re.findall(r'drivename="([^"]+)"', h + ' ' + ' '.join(ls))]
    say('drives: %s' % ' '.join(drives))
    todo = [('%s:\\' % d, 0) for d in drives]
    seen = 0
    while todo:
        path, lvl = todo.pop(0)
        try:
            c, h, ls = sx.cmd('dirlist name="%s"' % path)
        except Exception:
            continue
        for l in ls:
            m = re.search(r'name="([^"]+)"', l)
            if not m:
                continue
            name = m.group(1)
            full = path + ('' if path.endswith('\\') else '\\') + name
            if 'xefu' in name.lower():
                say('  %s  %s' % (full, l[l.find('size'):]))
            if 'directory' in l and lvl < depth and name.lower() not in ('content', 'games', 'apps', 'emulators'):
                todo.append((full, lvl + 1))
            seen += 1
            if seen > 4000:
                say('  (stopped after 4000 entries)')
                return



def remembered_boot(ip, port):
    """the game's path when --boot is given without one: this script's run_test_cache.txt, else the path spooky_live
    remembered for this console, else a search of the console's drives for Halo 2's default.xbe (read-only; nothing is
    launched by this)"""
    if os.path.exists(CACHE):
        p = open(CACHE).read().strip()
        if p:
            return p
    try:
        import spooky_live as sl
        p = sl.cached_xbe(ip)
        if p:
            return p
        x = sl.Xbdm(ip, port)
        try:
            p, found = sl.find_xbe(x, log=lambda *a, **k: None)
        finally:
            x.close()
        if p:
            sl.remember_xbe(ip, p)
            return p
    except Exception:
        pass
    return ''

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ip', nargs='?', default='192.168.0.178')
    ap.add_argument('--label', default='run')
    ap.add_argument('--xex', metavar='FILE', help='send this xefu1_1.xex to the Compatibility folder over FTP before booting (see --ftp-dir)')
    ap.add_argument('--ftp', default='xboxftp:xboxftp', metavar='USER:PASSWORD', help='FTP login (default xboxftp:xboxftp)')
    ap.add_argument('--ftp-dir', default='/HddX/Compatibility', metavar='DIR', help='where the emulator lives on the console, as the FTP client shows it (default /HddX/Compatibility)')
    ap.add_argument('--boot', nargs='?', const='', metavar='XBE_PATH')
    ap.add_argument('--xbe', metavar='FILE', help="send this default.xbe over FTP in place of the game's (the --boot path, HDD:\\ taken as --xbe-drive) before booting")
    ap.add_argument('--xbe-drive', default='/Hdd1/', metavar='DIR', help='the FTP root of the drive holding the game (default /Hdd1/)')
    ap.add_argument('--minutes', type=float, default=15.0, help='how long to watch at most (default 15; the watch continues while you play)')
    ap.add_argument('--no-reboot', action='store_true', help='(always on now: this script never restarts the console)')
    ap.add_argument('--port', type=int, default=730)
    a = ap.parse_args()
    stamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    fn = 'test_%s_%s.txt' % (a.label, stamp)
    lines = []
    verdict = [None]

    say_lock = threading.Lock()
    quiet = [False]

    def say(t, echo=True):
        """also from the notification listener thread: the log file is rewritten under a lock, and not at all once the
        zip is being made (Windows refuses to read a file another thread has open for writing)"""
        if echo:
            print(t)
        with say_lock:
            lines.append(t)
            if quiet[0]:
                return
            for attempt in range(5):
                try:
                    with open(fn, 'w') as f:
                        f.write('VERDICT: %s\n\n' % (verdict[0] or '(in progress)') + '\n'.join(lines) + '\n')
                    break
                except PermissionError:
                    time.sleep(0.1)

    # --- the mod's log (6i.149q): the 360's spooky_log.txt followed from this run's launch on, in this window, into the zip ---
    modlog_fn = 'spooky_live_%s_%s.txt' % (a.label, stamp)
    modlog_started = [False]

    def start_modlog():
        """one thread, one extra XBDM connection: every second the log's size, only the new part fetched (as spooky_live
        --tail does), each line echoed here with a [mod] prefix and kept in its own file for the zip. Read-only."""
        if modlog_started[0]:
            return
        modlog_started[0] = True

        def run():
            import socket
            import spooky_live as sl
            out = None
            try:
                lx = sl.Xbdm(a.ip, a.port)
            except Exception as e:
                say('[mod] log follower could not connect: %s' % e)
                return
            have = None
            partial = True
            tail = ''
            while True:
                try:
                    size = lx.filesize(sl.BC_LOG)
                except Exception:
                    try:
                        lx.reopen()
                    except Exception:
                        pass
                    time.sleep(2)
                    continue
                if have is None:
                    have = size                                              # only what this run writes (the file is appended to)
                    say('[mod] following %s from %d bytes' % (sl.BC_LOG, size), echo=False)
                if size < have:
                    have = 0
                if size > have:
                    data = None
                    if partial:
                        try:
                            data = lx.binary('getfile name="%s" offset=0x%X size=0x%X' % (sl.BC_LOG, have, size - have), expect=size - have)
                            if len(data) != size - have:
                                raise IOError('short')
                        except Exception:
                            partial, data = False, None
                            try:
                                lx.reopen()
                            except Exception:
                                pass
                    if data is None:
                        try:
                            data = lx.binary('getfile name="%s"' % sl.BC_LOG)[have:size]
                        except Exception:
                            time.sleep(1)
                            continue
                    text = data.decode('latin-1').replace('\r\n', '\n')
                    if out is None:
                        out = open(modlog_fn, 'w', encoding='latin-1')
                    out.write(text)
                    out.flush()
                    tail += text
                    while '\n' in tail:
                        line, tail = tail.split('\n', 1)
                        say('[mod] ' + line)
                    have = size
                time.sleep(1)

        threading.Thread(target=run, daemon=True).start()

    # --- upload / boot ---
    if a.xex is not None or a.boot is not None:
        import spooky_live as sl
        sx = sl.Xbdm(a.ip, a.port)
        xbe_sent = False
        if a.xbe is not None:
            import ftplib, io
            bp = a.boot if a.boot else remembered_boot(a.ip, a.port)
            if not bp:
                sys.exit('--xbe needs the game path and none is remembered and none was found on the console: give --boot "HDD:\\\\Games\\\\Halo 2\\\\default.xbe" once')
            rpath = bp.replace('HDD:\\', a.xbe_drive.rstrip('/') + '/').replace('\\', '/')
            rdir, rname = rpath.rsplit('/', 1)
            xdata = open(a.xbe, 'rb').read()
            user, pw = (a.ftp.split(':', 1) + ['xboxftp'])[:2]

            def ftp_open():
                ff = ftplib.FTP(a.ip, timeout=60)
                try:
                    ff.login(user, pw)
                except ftplib.error_perm:
                    ff = ftplib.FTP(a.ip, timeout=60)
                    ff.login('xbox', 'xbox')
                ff.cwd(rdir)
                return ff

            # No FTP read of the game's folder: every run that read the xbe back over FTP (Aurora's server) ended in the
            # game's "disc dirty or damaged" error at the intro, runs that only wrote it did not - the read leaves the
            # title folder in a state the emulator's disc layer cannot enumerate. "Already there" is decided by a hash of
            # the last xbe this script sent (run_test_cache_xbe.txt), the write is verified by the kernel's file size.
            import hashlib as _hl
            # (11 Nov: a run labelled 6i205 sent the 6i.204 xbe from an older folder and nobody noticed until the log was read.)
            # The mod's own step, read from the file about to be used, is said here - and said loudly when the label names another.
            try:
                import re as _re
                _mv = _re.search(rb'Spooky Ghost Menu (v\d+\.\d+)', xdata)          # v2.0 on: the build is named, not numbered by step
                if _mv:
                    say('this xbe is Spooky Ghost Menu %s' % _mv.group(1).decode())
                _m = None if _mv else _re.search(rb'Spooky Ghost step (6[a-z]\.\d+)', xdata)
                _step = _m.group(1).decode() if _m else None
                if _step:
                    say('this xbe is Spooky Ghost step %s' % _step)
                    _lab = _re.search(r'6([a-z])(\d+)', a.label or '')
                    if _lab and ('6%s.%s' % (_lab.group(1), _lab.group(2))) != _step:
                        say('*** WARNING: the label says %s but this xbe is step %s - is this the right folder? ***' % (a.label, _step))
            except Exception:
                pass
            xhash = _hl.sha1(xdata).hexdigest()
            hcache = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'run_test_cache_xbe.txt')
            prev = open(hcache).read().strip() if os.path.exists(hcache) else ''
            last_err = None
            if prev == xhash + ' ' + rpath:
                say('the console already has this xbe (%s, %.1f MB, sent by this script before) - not sent' % (a.xbe, len(xdata) / 2 ** 20))
                xbe_sent = False
            else:
                for ftp_try in range(3):                                    # Aurora's FTP drops connections now and then
                    try:
                        f = ftp_open()
                        t0 = time.time()
                        f.storbinary('STOR ' + rname, io.BytesIO(xdata))   # written in place, never deleted first
                        f.quit()
                        xbe_sent = True
                        open(hcache, 'w').write(xhash + ' ' + rpath)
                        break
                    except ftplib.all_errors as e:
                        last_err = e
                        say('FTP of the xbe failed (%s) - trying again in 5 s' % e)
                        time.sleep(5)
                else:
                    sys.exit('FTP of the xbe to %s failed three times: %s' % (rpath, last_err))
            if xbe_sent:
                say('sent %s (%.1f MB) over FTP to %s in %.1f s' % (a.xbe, len(xdata) / 2 ** 20, rpath, time.time() - t0))
        if a.xex is not None:
            data = open(a.xex, 'rb').read()
            # The copy by hand that always worked goes through Aurora's FTP server; an XBDM sendfile of the same bytes
            # to the same path is not picked up by the launcher (the previously loaded emulator keeps running). So do
            # exactly what the hand copy does: FTP, same path, same server.
            import ftplib, hashlib, io
            user, pw = (a.ftp.split(':', 1) + ['xboxftp'])[:2]
            remote_dir, remote_name = a.ftp_dir.rstrip('/'), 'xefu1_1.xex'
            try:
                f = ftplib.FTP(a.ip, timeout=30)
                try:
                    f.login(user, pw)
                except ftplib.error_perm:
                    f = ftplib.FTP(a.ip, timeout=30)
                    f.login('xbox', 'xbox')
                f.cwd(remote_dir)
                f.storbinary('STOR ' + remote_name, io.BytesIO(data))
                # the first launch right after a new file lands fails with the "disc dirty or damaged" error (the system
                # reads the file before the console has finished writing it): read it back until it matches, then wait
                back = None
                for attempt in range(10):
                    time.sleep(1.0)
                    buf = io.BytesIO()
                    try:
                        f.retrbinary('RETR ' + remote_name, buf.write)
                    except ftplib.all_errors:
                        continue
                    back = buf.getvalue()
                    if back == data:
                        break
                f.quit()
            except ftplib.all_errors as e:
                sys.exit('FTP to %s failed (%s) - is Aurora\'s FTP on, user/password %s? (--ftp user:password)' % (a.ip, e, a.ftp))
            if back != data:
                sys.exit('the xex read back from the console does not match what was sent (%s bytes back, %d sent) - send again' % (len(back) if back else 'nothing', len(data)))
            say('sent %s (%.1f MB) over FTP to %s/%s, read back identical' % (a.xex, len(data) / 2 ** 20, remote_dir, remote_name))
        if a.xex is not None or (a.xbe is not None and xbe_sent):
            # settle before the launch so the kernel's view of the files is complete; nothing else - this script never
            # launches a title as a warm-up and never reboots the console (see the no-reboot rule in the docstring)
            wait = 5
            say('settling %d s before the launch' % wait)
            if a.xbe is not None and xbe_sent:
                try:
                    mx = xp.Xbdm(a.ip, a.port, timeout=10.0)
                    want = os.path.getsize(a.xbe)
                    for attempt in range(30):
                        c, h, l2 = mx.cmd('getfileattributes name="%s"' % a.boot)
                        m2 = re.search(r'sizelo=0x([0-9A-Fa-f]+)', h + ' '.join(l2 or []))
                        if m2 and int(m2.group(1), 16) == want:
                            say('  the kernel sees the xbe at its full size (%d bytes)' % want)
                            break
                        time.sleep(1.0)
                except Exception as e:
                    say('  (file size check over XBDM failed: %s)' % e)
            time.sleep(wait)
        if a.xex is not None:
            # FORCE THE OVERWRITE: the console keeps the emulator module resident (base 0x82000000) across every restart
            # short of a power-cycle, and the next launch reuses it whatever the file says. So overwrite the resident
            # module itself: compare the regions that differ between builds (the emulator's patched code and the
            # embedded x86 kernel) with the xex just sent and write the differences into memory with XBDM setmem.
            try:
                import xex as xexmod
                image = xexmod.Xex(a.xex).image()
            except Exception as e:
                sys.exit('cannot read %s locally (%s): pip install pycryptodome' % (a.xex, e))
            mx = xp.Xbdm(a.ip, a.port, timeout=10.0)
            c, h, ls = mx.cmd('modules')
            mods = [l for l in ls if 'xefu' in l.lower()]
            if not mods:
                say('the emulator module is not resident now (no xefu entry in the module list) - the file will be loaded fresh')
            else:
                say('resident emulator module (still in memory at the dashboard): %s' % mods[0].strip())
                m = re.search(r'base=0x([0-9a-fA-F]+)', mods[0])
                base = int(m.group(1), 16) if m else 0x82000000
                regions = [(0x000000, 0x0D0000), (0x520000, 0x20C60)]
                written = 0
                for off, n in regions:
                    cur = mx.getmem(base + off, n)
                    want = image[off:off + n]
                    k = 0
                    while k < n:
                        if cur[k:k + 32] != want[k:k + 32]:
                            chunk = want[k:k + 32]
                            c, h, l2 = mx.cmd('setmem addr=0x%08X data=%s' % (base + off + k, chunk.hex()))
                            if c // 100 != 2:
                                say('  setmem at %08X refused: %s' % (base + off + k, h))
                            else:
                                written += 1
                        k += 32
                say('  %d 32-byte chunks of the resident module differed from the xex and were overwritten in memory' % written)
                bad = 0
                for off, n in regions:
                    if mx.getmem(base + off, n) != image[off:off + n]:
                        bad += 1
                say('  verification: %s' % ('the resident module now matches the xex' if not bad else '%d region(s) still differ' % bad))
            mx.close()
        try:
            sx.close()
        except Exception:
            pass

    boot_path = None
    if a.boot is not None:
        boot_path = a.boot
        if not boot_path:
            boot_path = remembered_boot(a.ip, a.port)
        if not boot_path:
            sys.exit('no game path remembered and none found on the console: give --boot "HDD:\\\\Games\\\\Halo 2\\\\default.xbe" once')
        open(CACHE, 'w').write(boot_path)

    want = None
    if a.xex is not None:
        try:
            import xex as xexmod
            im = xexmod.Xex(a.xex).image()
            want = im[0x520000 + 0xDD75:0x520000 + 0xDD7A] + im[0x520000 + 0x20B2E:0x520000 + 0x20B32] + im[0x520000 + 0x20B3C:0x520000 + 0x20B3D] + im[0x520000 + 0x11804:0x520000 + 0x11805] + im[0x520000 + 0x1180F:0x520000 + 0x11810] + im[0x520000 + 0x11807:0x520000 + 0x11808] + im[0x520000 + 0x11811:0x520000 + 0x11812] + im[0x520000 + 0x1187C:0x520000 + 0x1187D] + im[0x14532C:0x145330] + im[0x145388:0x14538C] + im[0x0C1784:0x0C1788]
        except Exception as e:
            say('(could not read the local xex for the build check: %s - pip install pycryptodome)' % e)

    waits = [0, 30]
    for attempt in range(len(waits) if want is not None and boot_path else 1):
        if boot_path:
            import spooky_live as sl
            try:                                                        # never launch over a running game: it has files open
                chk = sl.Xbdm(a.ip, a.port)
                c0, h0, l0 = chk.cmd('xbeinfo running')
                chk.close()
                running = (h0 + ' ' + ' '.join(l0 or [])).strip()
                m0 = re.search(r'name="([^"]+)"', running)
                name = (m0.group(1) if m0 else running).lower()
                if name and not any(k in name for k in ('dash', 'aurora', 'xexmenu', 'fsd', 'freestyle')):
                    sys.exit('a title is running on the console (%s) - exit to the dashboard first; this script never restarts the console' % name)
            except SystemExit:
                raise
            except Exception:
                pass
            if waits[attempt]:
                say('waiting %d s at the dashboard before launching again' % waits[attempt])
                time.sleep(waits[attempt])
            bx = sl.Xbdm(a.ip, a.port)
            say('starting %s: %s' % (boot_path, bx.magicboot(boot_path, boot_path.rsplit('\\', 1)[0])))
            time.sleep(2)
            start_modlog()
        # --- wait for XBDM and the emulator ---
        x = None
        t_end = time.time() + 120
        while time.time() < t_end:
            try:
                x = xp.Xbdm(a.ip, a.port, timeout=5.0)
                break
            except OSError:
                time.sleep(2)
        if x is None:
            verdict[0] = 'CONSOLE WEDGED (XBDM never answered)'
            say(verdict[0])
            return
        say('connected: %s; waiting for the emulator%s' % (x.banner, '' if boot_path else ' (launch Halo 2 now)'))
        while True:
            try:
                name, ck, ts = xp.emulator(x)
            except (IOError, ValueError, OSError):
                name = None
            if name:
                say('emulator running: %s' % name)
                break
            time.sleep(0.5)
        # --- which build is actually running? two marks in the embedded x86 kernel ---
        same = True
        try:
            cpu = alias = None
            mark = b''
            t_m = time.time() + 15
            while time.time() < t_m:
                cpu = rd32(x, 0x82166C40)
                alias = rd32(x, cpu + 112) if cpu and 0xC0000000 <= cpu < 0xE0000000 else None
                if alias and 0xC0000000 <= alias < 0xE0000000:
                    try:
                        mark = x.getmem(alias + 0x1DD75, 5)
                    except (IOError, ValueError, OSError):
                        mark = b''
                    if len(mark) == 5 and mark != bytes(5):
                        break
                time.sleep(0.5)
            if len(mark) != 5:
                say('the x86 kernel never appeared in game RAM (cpuobj %s, alias %s): the emulator did not get through its start-up' % (fmt(cpu), fmt(alias)))
            mark2 = (x.getmem(alias + 0x30B2E, 4) + x.getmem(alias + 0x30B3C, 1) + x.getmem(alias + 0x21804, 1) + x.getmem(alias + 0x2180F, 1) + x.getmem(alias + 0x21807, 1) + x.getmem(alias + 0x21811, 1) + x.getmem(alias + 0x2187C, 1)) if alias else b''
            names = {('bbf03f0000', '6a106a0068558b'): 'stock-layout kernel (R36/R38a)', ('bbb07f0000', '6a106a0068558b'): 'R43-R47 kernel',
                     ('bbf07f0000', '6a106a0068558b'): 'R48 kernel', ('bbf03f0000', '6a0f6a0168558b'): 'R49 kernel', ('bbf03f0000', '6a0f6a01e8558b'): 'R50 kernel',
                     ('bbf03f0000', '6a0f6a01e831e9f7'): 'R51/R52 kernel', ('bbf03f0000', '6a0f6a01e83183f7'): 'R53/R54 kernel', ('bbf03f0000', '6a0f6a01e83183eb'): 'R55 kernel (all user pages low)'}
            # R56+ kernels all share the first 7 mark bytes; the steering variant is in pop_steer's two type compares
            # (0x8002180F cmp edi,7 / 0x8002187A cmp ecx,5): the opcode byte and the immediate of each
            key = (mark.hex(), mark2[:8].hex())
            if names.get(key) == 'R53/R54 kernel' and x.getmem(alias + 0x1F5B8, 4) != bytes.fromhex('dffeffff'):
                steer = x.getmem(alias + 0x2180F, 1) + x.getmem(alias + 0x21811, 1) + x.getmem(alias + 0x2187A, 1) + x.getmem(alias + 0x2187C, 1)
                names[(mark.hex(), mark2.hex())] = {
                    '83078305': 'R56-R61 kernel (XBE + heaps low)',
                    '90908305': 'R62X kernel (XBE above 64 MB, heaps low; bisection, nops)',
                    '83079090': 'R62H kernel (heaps above 64 MB, XBE low; bisection, nops)',
                    '90909090': 'R62 kernel (XBE + heaps above 64 MB; the two compares nop\'d)',
                    '837f837f': 'R63 kernel (XBE + heaps above 64 MB; compares kept, never match)',
                }.get(steer.hex(), 'R56+ kernel, unknown steering bytes %s' % steer.hex())
            elif names.get(key) and (mark.hex(), mark2.hex()) not in names:
                names[(mark.hex(), mark2.hex())] = names[key]
            glue = x.getmem(0x8214532C, 4).hex() + x.getmem(0x82145388, 4).hex() + x.getmem(0x820C1784, 4).hex()
            gname = {'409c007453ab5ca83b4b9660': 'pre-R52 emulator', '6000000053ab5ca83b4b9660': 'R52/R53 emulator', '6000000053ab5caa3b4b9660': 'R54-R58 emulator', '6000000053ab5caa3b4bb680': 'R59 emulator (tables in .data padding)', '6000000053ab5caa3b4b0000': 'R60+ emulator (wide GPU tables in reserved RAM)'}.get(glue, 'unknown emulator')
            say('kernel build marks 0x8001DD75 %s / 0x80030B2E+3C+21804+0F+07+11+187C %s = %s; glue FPREM words %s (%s)' % (
                mark.hex(), mark2.hex(), names.get((mark.hex(), mark2.hex()), 'unknown'), glue, gname))
            mark2 += bytes.fromhex(glue)
            if want is not None:
                same = want == mark + mark2
                say('the xex you sent: %s' % ('SAME marks - the console runs it' if same else 'DIFFERENT marks - the console is still running the previous emulator'))
        except Exception as e:
            say('build mark unreadable: %s' % e)
        if not same and boot_path:
            say('the console is NOT running the xex you sent (resident module not replaced); the run goes on with what runs - this script does not restart the console. To retry: exit to the dashboard yourself and run again')
        break
        t_end = time.time() + 120
        while time.time() < t_end:
            try:
                sl.Xbdm(a.ip, a.port).close()
                break
            except OSError:
                time.sleep(2)
    if want is not None and not same:
        verdict[0] = 'OLD BUILD STILL RUNNING after %d launches - not testing it' % len(waits)
        say(verdict[0])
        return

    # --- the console's debug output (XBDM notification channel): the emulator reports a fatal crash there ---
    import socket as _socket
    dbg_lines = []

    def listen():
        """XBDM notifications on this same connection (notify reconnectport=...): no inbound connection to the PC is
        needed, so the Windows firewall cannot get in the way. The console reports exceptions, module loads and
        debug strings here - including the emulator's fatal crash."""
        try:
            ctl = _socket.create_connection((a.ip, a.port), 10)
            cf = ctl.makefile('rb'); cf.readline()
            ctl.sendall(b'notify reconnectport=0\r\n'); head = cf.readline().decode('latin-1').strip()
            say('  [console] notify: ' + head)
            if not head.startswith('2'):
                srv = _socket.socket(); srv.bind(('', 0)); srv.listen(1); port = srv.getsockname()[1]; srv.settimeout(None)
                ctl.sendall(('notifyat port=%d\r\n' % port).encode()); head = cf.readline().decode('latin-1').strip()
                say('  [console] notifyat: ' + head)
                conn, _ = srv.accept()
                cf = conn.makefile('rb')
            ctl.settimeout(None)
            while True:
                l = cf.readline()
                if not l:
                    break
                t = l.decode('latin-1').rstrip()
                dbg_lines.append(t)
                say('  [console] ' + t)
        except Exception as e:
            say('  [console] notification channel ended: %s' % e)
    threading.Thread(target=listen, daemon=True).start()

    # --- watch ---
    say('time  | region   cpuobj   | pages    free     pde800   current  | frames   queue req')
    t0 = time.time()
    first_free = None
    region_t = None             # when Halo 2's 47.5 MB region was taken
    main_t = None               # when the main thread first became current
    kernel_t = None             # when the kernel's page counts first appeared
    last_change_t = t0
    last_free = None
    last_frames = None
    frames_t = None             # when frames first advanced after the region
    frames_last_t = None        # when frames last advanced
    post_region_alloc = False   # free pages moved again after the region (loading / D3D)
    dead_since = None
    last_ok = [None]            # when a reading last succeeded (seconds into the watch)
    silent_since = [None]       # since when the console has answered NOTHING (not even 'modules')
    hang_label = None
    produced = [fn, modlog_fn]

    playing_said = False
    pages_taken = []

    pages_aborted = [False]

    def take_rtlog(tag):
        """a logging emulator (L37 / R63L): 6 s of its render-target / eDRAM event log, read-only, into the zip"""
        try:
            rx = xp.Xbdm(a.ip, a.port, timeout=10.0)
            if not xp.rtlg_find(rx):
                return
            say('render-target log (%s): reading 6 s of events...' % tag)
            fn2 = xp.rtlog(rx, 6, a.label)
            if fn2:
                produced.append(fn2)
                say('render-target log saved: %s' % fn2)
                try:
                    say('', echo=False)
                    say(open(fn2).read(), echo=False)
                except OSError:
                    pass
        except KeyboardInterrupt:
            print()
            say('render-target log interrupted by Ctrl+C - skipped')
        except Exception as e:
            say('render-target log (%s) failed: %s' % (tag, e))

    def take_pages(tag):
        """the page-frame map (pages.py), read-only, appended to the log and saved beside it. Reads 128 KB over XBDM,
        which takes a few seconds on a busy console; it shows its progress, and a Ctrl+C during it only skips the map."""
        if pages_aborted[0]:
            say('page map (%s) skipped: an earlier one was interrupted' % tag)
            return
        try:
            import pages
            say('taking the page map (%s): reading the kernel\'s 128 KB page table over XBDM, this takes a little while...' % tag)
            px, palias = pages.connect(a.ip, a.port)
            tp = time.time()
            def prog(done, total):
                if done == total or int(done / total * 8) != int((done - 1) / total * 8):
                    print('\r  page table: %3d of %d KB (%.0f s)' % (done >> 10, total >> 10, time.time() - tp), end='', flush=True)
            lines = pages.page_map(px, palias, tag, prog)
            print()
            pn = 'pages_%s_%s_%d.txt' % (a.label, stamp, len(pages_taken) + 1)
            open(pn, 'w').write('\n'.join(lines) + '\n')
            produced.append(pn)
            pages_taken.append(pn)
            say('')
            say('page map (%s) saved: %s' % (tag, pn))
            for l in lines[:6]:
                say('  ' + l)
            say('  (the rest is in the file)')
            say('', echo=False)
            for l in lines[6:]:
                say('  ' + l, echo=False)
            say('')
        except KeyboardInterrupt:
            print()
            pages_aborted[0] = True
            say('page map (%s) interrupted by Ctrl+C - skipped; the run goes on to its verdict and the zip' % tag)
        except Exception as e:
            say('page map (%s) failed: %s' % (tag, e))
    try:
      while time.time() - t0 < a.minutes * 60:
        try:
            w = sample(x)
        except (IOError, ValueError, OSError, ConnectionError) as e:
            say('%5.1f s: console stopped answering (%s)' % (time.time() - t0, e))
            x = None
            for i in range(12):
                time.sleep(5)
                try:
                    x = xp.Xbdm(a.ip, a.port, timeout=5.0)
                    say('XBDM answers again after %d s' % (5 * (i + 1)))
                    break
                except OSError:
                    pass
            if x is None:
                verdict[0] = 'CONSOLE WEDGED (XBDM gone)'
                break
            continue
        now = time.time() - t0
        say('%5.1f | %s %s | %s %s %s %s | %s %5s %s' % (now, fmt(w.get('region')), fmt(w.get('cpu')), fmt(w.get('total')),
            fmt(w.get('free')), fmt(w.get('pde800')), fmt(w.get('cur')), fmt(w.get('frames')),
            w.get('queue') if w.get('queue') is not None else '-', fmt(w.get('req'))), echo=int(now * 2) % 4 == 0)
        if w.get('cpu') is None and w.get('region') is None:
            dead_since = dead_since or now
            if now - dead_since > 6:
                # only call it ended when the module really is gone; reads can fail for a while during loading
                gone = False
                try:
                    c, h, ls = x.cmd('modules')
                    gone = not any('xefu' in l.lower() for l in ls)
                    silent_since[0] = None
                except (IOError, ValueError, OSError):
                    # no reading AND no answer to 'modules': the console itself is not answering. A minute of that is
                    # a verdict (R81L, 9 Nov: a hard crash while loading was watched for three minutes as dashes and
                    # then called "the game was drawing")
                    silent_since[0] = silent_since[0] or dead_since
                    try:
                        x = xp.Xbdm(a.ip, a.port, timeout=5.0)
                    except OSError:
                        pass
                    if now - silent_since[0] >= 60:
                        verdict[0] = 'CONSOLE WEDGED (no answer of any kind since %.1f s; the last reading was at %s)' % (
                            silent_since[0], ('%.1f s, frames %s' % (last_ok[0], fmt(last_frames))) if last_ok[0] is not None else 'never')
                        say('%5.1f s: the console has not answered for %.0f s - stopping the watch. It has NOT been restarted: do that by hand.' % (now, now - silent_since[0]))
                        break
                if gone:
                    verdict[0] = 'TITLE ENDED (the emulator is no longer in memory)'
                    break
                dead_since = now
            time.sleep(0.5)
            continue
        dead_since = None
        silent_since[0] = None
        last_ok[0] = now
        total, free, cur, frames = w.get('total'), w.get('free'), w.get('cur'), w.get('frames')
        if total:
            kernel_t = kernel_t or now
            if total > 0x10000:
                verdict[0] = 'INIT RUNAWAY (page counters %08X / %08X)' % (total, free or 0)
                break
        if free is not None and free != last_free:
            if last_free is not None and last_free - free > 11000 and region_t is None:
                region_t = now
                say('  -> Halo 2\'s region allocated at %.1f s (free %08X -> %08X)' % (now, last_free, free))
            elif region_t is not None and not post_region_alloc:
                post_region_alloc = True
                say('  -> memory moving again after the region at %.1f s (loading)' % now)
            last_free = free
            last_change_t = now
        if cur not in (None, IDLE, 0) and main_t is None and cur >= 0xD0000000:
            main_t = now
            say('  -> an x86 thread in the pool is current at %.1f s (%08X)' % (now, cur))
        if frames is not None and last_frames is not None and frames != last_frames:
            last_change_t = now
            frames_last_t = now
            if region_t is not None:
                frames_t = frames_t or now
                start_modlog()
        last_frames = frames
        # verdicts
        if frames_last_t is not None and now - frames_last_t > 8:
            # frames stopped: frozen - or the title is simply gone (a quit to the dashboard reads as zeros, not as
            # failed reads; 9 Nov: a normal quit was reported as "CRASHED/FROZE WHILE PLAYING: FROZEN")
            ended = False
            try:
                c, h, ls = x.cmd('modules')
                ended = not any('xefu' in l.lower() for l in ls)
            except (IOError, ValueError, OSError):
                pass
            if ended:
                verdict[0] = 'TITLE ENDED after %.0f s (the emulator is no longer in memory; frames last moved at %.1f s): a quit to the dashboard, or a crash back to it - only you can say which' % (now, frames_last_t)
                break
            verdict[0] = 'FROZEN (frames stopped at %08X after %.1f s; region %s, loading %s)' % (
                frames or 0, frames_last_t, 'taken at %.1f s' % region_t if region_t else 'not taken', 'seen' if post_region_alloc else 'never started')
            hang_label = a.label
            break
        if frames_t is not None and now - frames_t > 45 and post_region_alloc and not playing_said:
            playing_said = True
            say('')
            say('PLAYING (frames advancing since %.1f s, memory moving after the region; free pages %08X) - test by hand now; still watching for a crash or freeze (Ctrl+C to stop)' % (frames_t, free or 0))
            say('')
        if playing_said:
            if not pages_taken and now - frames_t > 60:
                take_pages('at PLAYING, %.0f s after the first frame (menu / early loading)' % (now - frames_t))
            time.sleep(0.5)
            continue
        if kernel_t and now - last_change_t > 15:
            if region_t is not None:
                verdict[0] = 'STUCK AFTER SETUP (region taken at %.1f s, nothing since %.1f s)' % (region_t, last_change_t)
                hang_label = a.label
            elif main_t is not None:
                verdict[0] = 'STUCK BEFORE SETUP (main thread since %.1f s, no region, nothing since %.1f s)' % (main_t, last_change_t)
                hang_label = a.label
            else:
                verdict[0] = 'KERNEL STALL (idle thread current, nothing since %.1f s)' % last_change_t
                hang_label = a.label
            break
        time.sleep(0.5)
    except KeyboardInterrupt:
        unanswered = (time.time() - t0) - last_ok[0] if last_ok[0] is not None else None
        if unanswered is not None and unanswered > 10:                              # stopped while the console was not answering: say that, not "was drawing"
            verdict[0] = verdict[0] or 'STOPPED BY HAND - THE CONSOLE HAD NOT ANSWERED FOR %.0f s (last reading at %.1f s, frames %s): a crash or freeze, not a clean stop' % (unanswered, last_ok[0], fmt(last_frames))
        verdict[0] = verdict[0] or ('STOPPED BY HAND while playing (no crash seen)' if playing_said else ('STOPPED BY HAND (the game was drawing)' if frames_t is not None else 'STOPPED BY HAND'))
        if x is not None and frames_t is not None and not (unanswered is not None and unanswered > 10):                       # the game was drawing: capture what it was doing (PLAYING declared or not)
            take_rtlog('at the stop')
            take_pages('at the stop, %.0f s after the first frame (as it was when you pressed Ctrl+C)' % ((time.time() - t0) - frames_t))
    if verdict[0] is None:
        verdict[0] = ('PLAYED for the whole %.0f minutes without a crash or freeze' % a.minutes) if playing_said else 'TIMEOUT after %.0f minutes (last: free %s, frames %s)' % (a.minutes, fmt(last_free), fmt(last_frames))
        if playing_said and x is not None:
            take_pages('at the end of the watch, %.0f s after the first frame' % ((time.time() - t0) - (frames_t or 0)))
    elif playing_said and not verdict[0].startswith('STOPPED BY HAND') and not verdict[0].startswith('TITLE ENDED after'):
        verdict[0] = 'CRASHED/FROZE WHILE PLAYING: ' + verdict[0]
    say('')
    say('VERDICT: ' + verdict[0])
    if hang_label and x is not None:
        say('')
        say('running the hang probe...')
        try:
            pf = hang_probe.main([a.ip, '--label', hang_label, '--port', str(a.port)])
            say('hang probe saved: %s' % pf)
            produced += [pf, pf.replace('.txt', '.bin')]
            try:
                say('', echo=False)
                say(open(pf).read(), echo=False)
            except OSError:
                pass
        except Exception as e:
            say('hang probe failed: %s' % e)
        take_pages('after the hang probe')
    if verdict[0] and not verdict[0].startswith(('PLAYING', 'TITLE ENDED')):
        say('the console has been left exactly as it is (this script never restarts it: a restart while the game has files open corrupts them). Restart it yourself when you are ready.')
    say('saved ' + fn)
    # everything from this run in one zip, ready to send (the log file closed for good first: the listener may still talk)
    import zipfile
    zn = 'test_%s_%s.zip' % (a.label, stamp)
    with say_lock:
        quiet[0] = True
    zipped = []
    with zipfile.ZipFile(zn, 'w', zipfile.ZIP_DEFLATED) as z:
        for f in produced:
            if os.path.exists(f):
                for attempt in range(10):
                    try:
                        z.write(f)
                        zipped.append(f)
                        break
                    except PermissionError:
                        time.sleep(0.2)
    for f in zipped:
        try:
            os.remove(f)
        except OSError:
            pass
    print('send back: %s (the loose files are inside it)' % zn)


if __name__ == '__main__':
    main()
