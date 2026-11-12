#!/usr/bin/env python3
"""hashmaps - SHA-1 of Halo 2's map files on the console (read over FTP), against a stored baseline.

  python hashmaps.py --label r63 [--baseline]      (h2test.py hashmaps ...)

Reads the game folder over Aurora's FTP (default /Hdd1/Games/Halo 2/; --folder to change) and hashes default.xbe,
mainmenu.map, shared.map, single_player_shared.map and every file in maps/. With --baseline it writes
hashmaps_baseline.json beside this script; without, it compares against that file and reports every file whose hash,
size or presence changed. Read-only on the console. shared.map is large (hundreds of MB): a run takes a few minutes
over the LAN. Run it AFTER a test round (with the game exited) to prove the round left the maps untouched.
"""
import re
import argparse, ftplib, hashlib, io, json, os, sys, time

DEFAULT_FOLDER = '/Hdd1/Games/Halo 2'
# 12 Nov: the baseline used to be written beside this script. Every build is now extracted to a new folder, so a
# baseline made from one folder would not be there for the next. It is kept in the user's own folder instead
# (C:\Users\<you>\h2test_hashmaps_baseline.json); one beside the script, from before, is still used if that is all there is.
BASE_HOME = os.path.join(os.path.expanduser('~'), 'h2test_hashmaps_baseline.json')
BASE_OLD = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'hashmaps_baseline.json')
BASE = BASE_HOME
# default.xbe is the mod itself: it differs with every build, by design. It is hashed and shown, never counted as a change.
EXPECTED_TO_DIFFER = ('default.xbe',)


def ftp_open(ip, user, pw, folder):
    f = ftplib.FTP(ip, timeout=120)
    try:
        f.login(user, pw)
    except ftplib.error_perm:
        f = ftplib.FTP(ip, timeout=120)
        f.login('xbox', 'xbox')
    f.cwd(folder)
    return f


def listing(f):
    """names in the current folder: NLST first, LIST (parsed) when NLST gives nothing - Aurora's server differs by version"""
    names = []
    try:
        f.retrlines('NLST', names.append)
    except ftplib.all_errors:
        names = []
    names = [n.rsplit('/', 1)[-1].strip() for n in names if n.strip()]
    # 10 Nov (the probe agent's finding): Aurora's server answers NLST with whole "ls -l" lines - "-rwxrwxrwx   1 root
    # root  5120000 Nov 10 12:02 default.xbe" - and each line was being used as a file name: nothing matched in the game
    # folder, and in the maps folder the "name" was refused (550 ... Error: 123, an invalid name). A line of that shape
    # gives its name: the part after the time.
    LS = re.compile(r'^[-dl][-rwxsStT]{9}\s+\d+\s+\S+\s+\S+\s+\d+\s+\w{3}\s+\d+\s+[\d:]+\s+(.+)$')
    names = [(LS.match(n).group(1).strip() if LS.match(n) else n) for n in names]
    if not names:
        lines = []
        f.retrlines('LIST', lines.append)
        for l in lines:
            parts = l.split(None, 8)
            if len(parts) >= 9:
                names.append(parts[8])
            elif len(parts) >= 4:                      # DOS-style listing: date time size name
                names.append(parts[3] if parts[2] != '<DIR>' else parts[3])
    # 9 Nov: the listing printed 31 names ending in .map and none was hashed - the names carried something after the
    # extension (a carriage return or a space, by the look of it). Stripped, and anything unprintable dropped.
    names = [''.join(ch for ch in n if ch.isprintable()).strip() for n in names]
    return [n for n in names if n and n not in ('.', '..')]


def sha1_of(f, name):
    h = hashlib.sha1()
    size = [0]
    last = [0.0]

    def cb(block):
        h.update(block)
        size[0] += len(block)
        if time.time() - last[0] >= 2.0:
            last[0] = time.time()
            print('\r  %s: %d MB' % (name, size[0] >> 20), end='', flush=True)
    f.retrbinary('RETR ' + name, cb, blocksize=256 * 1024)
    print('\r  %s: %d MB, %s' % (name, size[0] >> 20, h.hexdigest()[:16]))
    return h.hexdigest(), size[0]


def sha1_retry(state, name, tries=3):
    """one file, with the connection opened again if it drops part-way (a big map over a slow link). state: [ftp,
    (ip, user, pw), the folder to be in]. Returns (hash, size), or None after `tries` failures - said, not fatal."""
    for attempt in range(1, tries + 1):
        try:
            return sha1_of(state[0], name)
        except ftplib.all_errors + (OSError, EOFError) as e:
            print('\n  %s: read failed (%s: %s) - try %d of %d' % (name, type(e).__name__, e, attempt, tries))
            try:
                state[0].close()
            except Exception:
                pass
            time.sleep(2)
            try:
                state[0] = ftp_open(state[1][0], state[1][1], state[1][2], state[2])
            except ftplib.all_errors + (OSError, EOFError) as e2:
                print('  could not connect again (%s)' % e2)
    return None


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ip', nargs='?', default='192.168.0.178')
    ap.add_argument('--label', default='')
    ap.add_argument('--ftp', default='xboxftp:xboxftp')
    ap.add_argument('--folder', default=DEFAULT_FOLDER)
    ap.add_argument('--baseline', action='store_true', help='write the baseline instead of comparing')
    a = ap.parse_args(argv)
    user, pw = (a.ftp.split(':', 1) + ['xboxftp'])[:2]
    result = {}
    f = ftp_open(a.ip, user, pw, a.folder)
    top = listing(f)
    print('folder %s: %d entries%s' % (a.folder, len(top), (': ' + ', '.join(top[:12]) + (' ...' if len(top) > 12 else '')) if top else ''))
    if not top:
        sys.exit('nothing listed in %s - wrong folder (--folder "/Hdd1/Games/<your Halo 2 folder>") or the FTP server would not list it; no baseline written' % a.folder)
    wanted = [n for n in top if n.lower() in ('default.xbe', 'mainmenu.map', 'shared.map', 'single_player_shared.map')]
    if 'maps' not in [t.lower() for t in top]:           # pointed at the maps folder itself (--folder .../maps): every map in it
        wanted += [n for n in top if n.lower().endswith('.map') and n not in wanted]
    failed = []
    t_all = time.time()
    state = [f, (a.ip, user, pw), a.folder]
    for n in wanted:
        r = sha1_retry(state, n)
        if r: result[n] = r
        else: failed.append(n)
    if 'maps' in [t.lower() for t in top]:
        sub = [t for t in top if t.lower() == 'maps'][0]
        state[0].cwd(sub)
        state[2] = a.folder.rstrip('/') + '/' + sub
        names = sorted(listing(state[0]))
        print('folder %s: %d entries' % (state[2], len(names)))
        for n in names:
            if n.lower().endswith('.map'):
                r = sha1_retry(state, n)
                if r: result['maps/' + n] = r
                else: failed.append('maps/' + n)
    try:
        state[0].quit()
    except Exception:
        pass
    total = sum(v[1] for v in result.values())
    print('%d files, %d MB in %d s' % (len(result), total >> 20, int(time.time() - t_all)))
    if failed:
        print('COULD NOT BE READ (%d): %s' % (len(failed), ', '.join(failed)))
        if a.baseline:
            sys.exit('no baseline written: %d file(s) could not be read. Run it again (console on the dashboard, nothing else using its FTP).' % len(failed))
    if not result:
        sys.exit('no map files found in %s - nothing hashed, nothing written. The names as the server gave them: %s' % (a.folder, ', '.join(repr(n) for n in top[:8])))
    base_file = BASE if os.path.exists(BASE) else BASE_OLD if os.path.exists(BASE_OLD) else BASE
    if a.baseline or not os.path.exists(base_file):
        if not a.baseline:
            print('no baseline yet - this run becomes it')
        json.dump({'made': time.strftime('%Y-%m-%d %H:%M:%S'), 'folder': a.folder, 'files': result}, open(BASE, 'w'), indent=1)
        print('BASELINE WRITTEN: %s (%d files)' % (BASE, len(result)))
        for n in sorted(result):
            print('  %-40s %10d bytes  %s' % (n, result[n][1], result[n][0]))
        return 0
    base = json.load(open(base_file))
    made = base.get('made', '?') if isinstance(base.get('files'), dict) else '?'
    if isinstance(base.get('files'), dict):
        base = base['files']
    base = {k: tuple(v) for k, v in base.items()}
    result_t = {k: tuple(v) for k, v in result.items()}
    differ = [n for n in sorted(set(base) | set(result_t)) if base.get(n) != result_t.get(n) and n not in failed]
    expected = [n for n in differ if n.lower() in EXPECTED_TO_DIFFER]
    changed = [(n, base.get(n), result_t.get(n)) for n in differ if n.lower() not in EXPECTED_TO_DIFFER]
    BASE_USED = base_file
    fn = 'hashmaps_%s_%s.txt' % (a.label or 'run', time.strftime('%Y%m%d_%H%M%S'))
    lines = ['map check against %s (made %s): %d files read' % (BASE_USED, made, len(result))]
    for n in expected:
        lines.append('  %s differs from the baseline - expected: it is the mod itself' % n)
    if failed:
        lines.append('NOT CHECKED - could not be read (%d): %s' % (len(failed), ', '.join(failed)))
    if changed:
        lines.append('CHANGED (%d):' % len(changed))
        for n, b, r in changed:
            lines.append('  %s: baseline %s -> now %s' % (n, b, r))
    else:
        lines.append('ALL MAPS UNCHANGED - the round left the game files as they were' if not failed else 'every map that could be read is unchanged')
    text = '\n'.join(lines)
    print(text)
    open(fn, 'w').write(text + '\n')
    print('saved', fn)
    return 1 if (changed or failed) else 0


if __name__ == '__main__':
    sys.exit(main())
