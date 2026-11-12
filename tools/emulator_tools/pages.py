#!/usr/bin/env python3
"""pages - what every page of the 128 MB is used for, read from the x86 kernel's page frame database while the game runs.

  python pages.py --label r63            (h2test.py pages --label r63)

Read-only (XBDM getmem through the game RAM alias, no stop/go). The kernel's page table for 0x80000000.. doubles as its
page frame database (PFN table, 32768 entries at 0x87FB0000 in the 128 MB kernel). Entry formats:
  busy page        bit 16 set, BusyType in bits 28-31, PTE index in bits 18-27, lock count in the low word
  identity PTE     pfn << 12 | flags: the kernel's own fixed pages (image, PFN table) AND every page of a contiguous
                   allocation (MmAllocateContiguousMemoryEx: Halo 2's region, D3D's buffers); flags 0x6B = write-combined
                   (PWT), 0x7B = uncached (PCD), 0x63 = cached; bit 9 (0x200) marks an allocation's last page
  free             packed flink / blink words (16 bits each)
BusyType is the Xbox kernel's PageType: 1 stack, 2 virtual page table, 3 system page table, 4 pool, 5 virtual memory
(the heaps), 6 system memory, 7 image (the XBE), 8 FS cache, 9 contiguous (D3D, Halo 2's region), 10 debugger.
The per-type page counters live at 0x8002FEE0 (12 dwords).

The report: the per-type counters, every type's pages below / above 64 MB and inside the 16 MB window 0x2FE5-0x3FDF
(the only space below 64 MB outside Halo 2's 47.5 MB region, i.e. where contiguous GPU-visible memory can come from),
the free runs below the contiguous cap 0x3FDF (largest first), and the exact occupant list of the window.
"""
import argparse
import datetime
import struct
import xefu_probe as xp

TYPES = {0: 'unknown', 1: 'stack', 2: 'virtual page table', 3: 'system page table', 4: 'pool', 5: 'virtual memory (heaps)',
         6: 'system memory', 7: 'image (XBE)', 8: 'FS cache', 9: 'contiguous (D3D / region)', 10: 'debugger', 11: 'free (counter 11)'}
WIN_LO, WIN_HI, CAP = 0x2FE5, 0x3FDF, 0x3FDF      # the 16 MB window below 64 MB outside Halo 2's region; contiguous cap
REGION_LO, REGION_HI = 0x61, 0x2FE4


def fixed_name(flags):
    cache = 'uncached' if flags & 0x10 else ('write-combined' if flags & 0x8 else 'cached')
    return 'identity PTE flags %03X (%s%s)' % (flags, cache, ', last page of its allocation' if flags & 0x200 else '')


def classify(i, e):
    """-> ('busy', type) | ('fixed', flags) | ('free', None) for PFN i with table entry e"""
    t = e >> 28
    if e & 0x10000 and 1 <= t <= 10:
        return 'busy', t
    if e & 1 and ((e >> 12) & 0xFFFFF) == i:
        return 'fixed', e & 0xFFF
    return 'free', None


def read_table(x, alias, progress=None):
    """the whole PFN table (128 KB for 128 MB). Tries 16 KB XBDM reads first and falls back to the usual 1 KB ones;
    progress(done_bytes, total_bytes) is called after every read so a slow console is visibly still answering."""
    pde = struct.unpack('<I', x.getmem(alias + 0xF800, 4))[0]            # PDE for 0x80000000
    tab = (pde & 0xFFFFF000) | 0x80000000
    total = struct.unpack('<I', x.getmem(alias + (0x80030598 - 0x80000000), 4))[0]
    n = 0x8000 if total > 0x4000 else 0x4000
    pt = bytearray()
    base = alias + (tab - 0x80000000)
    chunk = 0x4000
    while len(pt) < n * 4:
        try:
            pt += x.getmem(base + len(pt), min(chunk, n * 4 - len(pt)), chunk=chunk)
        except (IOError, ValueError) as e:
            if chunk == 0x400:
                raise
            chunk = 0x400                                              # XBDM did not like the big read: 1 KB pieces
            continue
        if progress:
            progress(len(pt), n * 4)
    return tab, n, [struct.unpack_from('<I', pt, 4 * i)[0] for i in range(n)]


def page_map(x, alias, tag='', progress=None):
    """the full report as a list of lines; progress(done, total) reports the table read (see read_table)"""
    out = []
    say = out.append
    le = lambda a: struct.unpack('<I', x.getmem(alias + (a - 0x80000000), 4))[0]
    total, free = le(0x80030598), le(0x8002FF0C)
    counters = struct.unpack('<12I', x.getmem(alias + (0x8002FEE0 - 0x80000000), 48))
    say('page map%s: kernel %d pages (%d MB), %d free (%.1f MB)' % (' - ' + tag if tag else '', total, total >> 8, free, free / 256.0))
    say('kernel counters by type (0x8002FEE0): ' + ', '.join('%s %d (%.1f MB)' % (TYPES[t], c, c / 256.0) for t, c in enumerate(counters) if c))
    tab, n, ents = read_table(x, alias, progress)
    cls = [classify(i, ents[i]) for i in range(n)]
    # per type: below 64 MB / in the window / above 64 MB
    say('from the table at %08X (%d entries): pages per type - below 64 MB | of those inside the window %04X-%04X | above 64 MB' % (tab, n, WIN_LO, WIN_HI))
    rows = {}
    for i, (k, v) in enumerate(cls):
        key = ('busy', v) if k == 'busy' else (k, None)
        r = rows.setdefault(key, [0, 0, 0])
        if i < 0x4000:
            r[0] += 1
            if WIN_LO <= i <= WIN_HI:
                r[1] += 1
        else:
            r[2] += 1
    for key in sorted(rows, key=lambda kk: (kk[0] != 'busy', kk[1] or 0)):
        r = rows[key]
        name = TYPES[key[1]] if key[0] == 'busy' else ('contiguous / fixed (identity PTEs)' if key[0] == 'fixed' else 'free')
        say('  %-28s %6d (%6.1f MB) | %6d (%6.1f MB) | %6d (%6.1f MB)' % (name, r[0], r[0] / 256.0, r[1], r[1] / 256.0, r[2], r[2] / 256.0))
    # free runs below the contiguous cap
    runs = []
    i = 0x31
    while i <= CAP:
        if cls[i][0] == 'free':
            j = i
            while j <= CAP and cls[j][0] == 'free':
                j += 1
            runs.append((j - i, i, j - 1))
            i = j
        else:
            i += 1
    runs.sort(reverse=True)
    say('free runs below the contiguous cap %04X (largest first, >= 16 pages): ' % CAP + (', '.join(
        '%04X-%04X %d pages (%.1f MB)' % (lo, hi, c, c / 256.0) for c, lo, hi in runs if c >= 16) or 'none'))
    say('  largest contiguous block available below 64 MB: %.1f MB' % ((runs[0][0] / 256.0) if runs else 0))
    # the window, occupant by occupant
    say('the window %04X-%04X (%.1f MB), occupant by occupant:' % (WIN_LO, WIN_HI, (WIN_HI - WIN_LO + 1) / 256.0))
    i = WIN_LO
    while i <= WIN_HI:
        k = cls[i]
        j = i
        while j <= WIN_HI and cls[j] == k:
            j += 1
        name = TYPES[k[1]] if k[0] == 'busy' else (fixed_name(k[1]) if k[0] == 'fixed' else 'free')
        say('  %04X-%04X %5d pages (%5.1f MB) %s' % (i, j - 1, j - i, (j - i) / 256.0, name))
        i = j
    # Halo 2's region and the rest below 64 MB, summarised
    reg = {}
    for i in range(REGION_LO, REGION_HI + 1):
        k = cls[i]
        name = TYPES[k[1]] if k[0] == 'busy' else k[0]
        reg[name] = reg.get(name, 0) + 1
    say("Halo 2's region %04X-%04X: " % (REGION_LO, REGION_HI) + ', '.join('%s %d' % (nm, c) for nm, c in sorted(reg.items(), key=lambda t: -t[1])))
    low = {}
    for i in range(0, REGION_LO):
        k = cls[i]
        name = TYPES[k[1]] if k[0] == 'busy' else k[0]
        low[name] = low.get(name, 0) + 1
    say('pages 0000-%04X (below the region): ' % (REGION_LO - 1) + ', '.join('%s %d' % (nm, c) for nm, c in sorted(low.items(), key=lambda t: -t[1])))
    # above 64 MB, by range
    say('above 64 MB, range by range (runs of one type, >= 16 pages):')
    i = 0x4000
    while i < n:
        k = cls[i]
        j = i
        while j < n and cls[j] == k:
            j += 1
        if j - i >= 16 or k[0] != 'free':
            name = TYPES[k[1]] if k[0] == 'busy' else (fixed_name(k[1]) if k[0] == 'fixed' else 'free')
            if j - i >= 16 or k[0] == 'fixed':
                say('  %04X-%04X %5d pages (%5.1f MB) %s' % (i, j - 1, j - i, (j - i) / 256.0, name))
        i = j
    return out


def connect(ip, port):
    x = xp.Xbdm(ip, port, timeout=10.0)
    cpu = xp.be32(x.getmem(0x82166C40, 4), 0)
    if not (0xC0000000 <= cpu < 0xE0000000):
        raise IOError('cpuobj pointer reads %08X: the emulator is not running' % cpu)
    alias = xp.be32(x.getmem(cpu + 112, 4), 0)
    return x, alias


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ip', nargs='?', default='192.168.0.178')
    ap.add_argument('--label', default='')
    ap.add_argument('--port', type=int, default=730)
    ap.add_argument('--tag', default='')
    a = ap.parse_args(argv)
    x, alias = connect(a.ip, a.port)
    import time as _t
    t0 = _t.time()
    def prog(done, total):
        print('\r  reading the page table: %d of %d KB (%.0f s)' % (done >> 10, total >> 10, _t.time() - t0), end='', flush=True)
    lines = page_map(x, alias, a.tag, prog)
    print()
    text = '\n'.join(lines)
    print(text)
    fn = 'pages_%s_%s.txt' % (a.label or 'run', datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    open(fn, 'w').write(text + '\n')
    print('saved', fn)
    return fn


if __name__ == '__main__':
    main()
