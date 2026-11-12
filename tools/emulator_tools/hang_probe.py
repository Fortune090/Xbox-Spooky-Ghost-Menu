#!/usr/bin/env python3
"""hang_probe - while R38 sits at the white logo (XBDM still answers): what the emulated Xbox kernel's memory looks
like and what its current thread was doing. Read-only.

  python hang_probe.py 192.168.0.178 --label r38

Reads, through the game RAM's kernel alias: the kernel's page table (= its page frame database, 128 KB at x86
0x87FE0000 on a 128 MB kernel, 64 KB at 0x83FF0000 on 64 MB), the processor block, the current thread and its
stack (listing every word that looks like a return address into Halo 2 or the kernel), the emulator's CPU state
page, Halo 2's physical-memory globals, and the kernel's memory counters. Saves everything raw too.
"""
import argparse
import datetime
import struct
import xefu_probe as xp


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ip', nargs='?', default='192.168.0.178')
    ap.add_argument('--label', default='')
    ap.add_argument('--port', type=int, default=730)
    a = ap.parse_args(argv)
    stamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    fn = 'hang_probe_%s_%s.txt' % (a.label or 'run', stamp)
    raw = {}
    out = []

    log = open(fn, 'w')

    def say(t):
        print(t)
        out.append(t)
        log.write(t + '\n')
        log.flush()

    x = xp.Xbdm(a.ip, a.port, timeout=10.0)
    try:
        c, h, ls = x.cmd('modules')
        mods = [l.strip() for l in ls if 'xefu' in l.lower()]
    except (IOError, ValueError, OSError):
        mods = []
    try:
        cpu = xp.be32(x.getmem(0x82166C40, 4), 0)
    except (IOError, ValueError, OSError):
        cpu = 0
    if not mods or not (0xC0000000 <= cpu < 0xE0000000):
        say('the emulator is not running: %s; cpuobj pointer reads %08X. Nothing of the x86 side is left to read.' % ('no xefu module loaded' if not mods else mods[0], cpu))
        try:
            c, h, ls = x.cmd('modules')
            say('loaded modules: ' + ' | '.join(l.strip()[:60] for l in ls[:12]))
        except (IOError, ValueError, OSError):
            pass
        log.close()
        print('saved', fn)
        return fn
    alias = xp.be32(x.getmem(cpu + 112, 4), 0)
    end = xp.be32(x.getmem(cpu + 116, 4), 0)
    mb = (end - alias) >> 20
    say('cpuobj %08X, game RAM alias %08X..%08X (%d MB)' % (cpu, alias, end, mb))
    try:
        state = xp.be32(x.getmem(cpu + 180, 4), 0)
        say('x86 state page %08X: EFLAGS %08X' % (state, xp.be32(x.getmem(state + 164, 4), 0)))
        blk = x.getmem(state + 560, 160)
        say('  state +560..+716 (the service request area; +616 code, +632 = request posted flag the service thread spins on, +640 async queue write): ' + ' '.join('+%d=%08X' % (560 + k, xp.be32(blk, k)) for k in range(0, 160, 4)))
        c = x.getmem(cpu + 1140, 24)
        say('  cpuobj +1140..+1160: ' + ' '.join('%08X' % xp.be32(c, k) for k in range(0, 24, 4)))
    except (IOError, ValueError, OSError) as e:
        say('state page: %s' % e)
    # --- the emulator's threads: XBDM cannot give their real contexts (every thread reports the kernel's switch-out
    # frame), so scan each thread's stack (threadinfo base/limit) for emulator return addresses instead ---
    try:
        roles = dict(xp.ROLES)
        roles[0x816A0400] = 'x86 (Halo 2 + Xbox kernel)'
        say('')
        roles_seen = [roles.get(xp.num(info.get('start', '0')), '') for tid, info in xp.threads(x)]
        if not any(r.startswith(('x86', 'GPU', 'service')) for r in roles_seen):
            say('THE EMULATOR\'S OWN THREADS ARE GONE (no x86 / GPU / service thread in the list): the emulator has exited - a fatal crash - and what follows is read from its remains')
        say('emulator threads: return addresses on each stack, from the stack base down (outermost call first; the live chain ends where stale frames begin):')
        for tid, info in xp.threads(x):
            try:
                st = xp.num(info.get('start', '0'))
                base, limit = xp.num(info.get('base', '0')), xp.num(info.get('limit', '0'))
            except ValueError:
                continue
            role = roles.get(st, 'thread %08X' % st)
            # unnamed 360 system threads (stacks at 0x3A0xxxxx-0x3A1xxxxx / 0x7A0xxxxx-0x7A1xxxxx) have always been idle in the
            # kernel: listed, not scanned (each scan is dozens of XBDM reads)
            if role.startswith('thread ') and st != 0x8012E3C0 and (0x3A000000 <= base < 0x3A200000 or 0x7A000000 <= base < 0x7A200000):
                say('  %-36s stack %08X-%08X: not scanned (system thread, idle in every earlier probe)' % (role, limit, base))
                continue
            if role.startswith(('service', 'async', 'x86', 'GPU', '360 D3D', 'emulator worker')):
                say('  %-36s threadinfo: %s' % (role, ' '.join('%s=%s' % (k, v) for k, v in sorted(info.items()) if k not in ('start',))))
            if not (base and limit and limit < base and base - limit <= 0x40000):
                say('  %-36s stack %08X-%08X (not scanned)' % (role, limit, base))
                continue
            try:
                blk = x.getmem(limit, base - limit)
            except (IOError, ValueError, OSError) as e:
                say('  %-36s stack %08X-%08X unreadable: %s' % (role, limit, base, e))
                continue
            cands = []
            for k in range(0, len(blk) - 3, 4):
                v = xp.be32(blk, k)
                if 0x82000000 <= v < 0x82140000 and v % 4 == 0:
                    cands.append((limit + k, v))
            # keep only values that sit right after a call instruction (bl / bctrl / blrl): real return addresses
            valid = []
            checked = {}
            for a, v in cands[-160:]:          # the top 160 candidates (the live frames are nearest the stack base)
                if v not in checked:
                    try:
                        w = xp.be32(x.getmem(v - 4, 4), 0)
                        checked[v] = ((w >> 26) == 18 and (w & 1) == 1) or w in (0x4E800421, 0x4E800021)
                    except (IOError, ValueError, OSError):
                        checked[v] = False
                if checked[v]:
                    valid.append((a, v))
            say('  %-36s stack %08X-%08X: %d return addresses%s' % (role, limit, base, len(valid), '' if valid else ' (idle in the kernel)'))
            if valid:
                # from the stack base downwards: outermost frame first, the most recent call last (stale frames may follow)
                say('    ' + ' '.join('%08X@%X' % (v, a - limit) for a, v in reversed(valid)))
    except (IOError, ValueError, OSError) as e:
        say('thread stacks: %s' % e)

    def x86(addr, n):
        return x.getmem(alias + (addr - 0x80000000), n)

    def le(addr):
        return struct.unpack('<I', x86(addr, 4))[0]

    def phys_of(va):
        pde = struct.unpack('<I', x.getmem(alias + 0xF000 + ((va >> 22) << 2), 4))[0]
        if not pde & 1:
            return None
        if pde & 0x80:
            return (pde & 0xFFC00000) | (va & 0x3FFFFF)
        pte = struct.unpack('<I', x.getmem(alias + (pde & 0xFFFFF000) + (((va >> 12) & 0x3FF) << 2), 4))[0]
        if not pte & 1:
            return None
        return (pte & 0xFFFFF000) | (va & 0xFFF)

    def x86v(va, n):
        """read any x86 virtual address by walking its page tables (pool / stack space included)"""
        out = bytearray()
        while len(out) < n:
            pa = phys_of(va + len(out))
            if pa is None:
                raise IOError('x86 VA %08X not mapped' % (va + len(out)))
            k = min(n - len(out), 0x1000 - (pa & 0xFFF))
            out += x.getmem(alias + pa, k)
        return bytes(out)

    # 1. kernel memory counters
    total, free, used = le(0x80030598), le(0x8002FF0C), le(0x8002FF04)
    say('kernel pages: total %d (%d MB), free %d (%.1f MB), [0x8002FF04] %d' % (total, total >> 8, free, free / 256.0, used))
    pde = x86(0x8000F800, 0x100)
    raw['pde_kernel'] = pde
    say('PDEs 0x80000000..0x8FFFFFFF: ' + ' '.join('%08X' % struct.unpack_from('<I', pde, i)[0] for i in range(0, 0x80, 4)))

    # 2. the page table / page frame database
    tab = struct.unpack_from('<I', pde, 0)[0] & 0xFFFFF000 | 0x80000000   # from the PDE for 0x80000000 (R38: 0x87FE0000, R39: 0x87FC0000)
    n = 0x8000 if total > 0x4000 else 0x4000
    say('reading the page table at %08X (%d entries)...' % (tab, n))
    pt = bytearray()
    for off in range(0, n * 4, 0x400):
        pt += x86(tab + off, 0x400)
    raw['pagetable'] = bytes(pt)
    ents = [struct.unpack_from('<I', pt, 4 * i)[0] for i in range(n)]
    # ranges by state: present (bit 0) with its flag bits, or free (link words)
    import pages as pg
    def state(i):
        # busy pages carry the kernel's BusyType (bits 28-31, bit 16 set); the kernel's own fixed pages are identity PTEs
        k, v = pg.classify(i, ents[i])
        if k == 'busy':
            return 'busy: %s' % pg.TYPES[v]
        if k == 'fixed':
            return pg.fixed_name(v)
        return 'free'
    rng = []
    i = 0
    while i < n:
        s = state(i)
        j = i
        while j < n and state(j) == s:
            j += 1
        rng.append((i, j, s))
        i = j
    say('page ranges (%d):' % len(rng))
    for i, j, s in rng:
        if s != 'free' or j - i >= 16:
            say('  %04X-%04X (%5d pages, %6.1f MB) %s%s' % (i, j - 1, j - i, (j - i) / 256.0, s,
                '' if s == 'free' else '  e.g. %08X' % ents[i]))
    busy = [(i, j, s) for i, j, s in rng if s != 'free' and i < 0x2FE5 and j > 0x61]
    mapped = [i for i in range(0x61, 0x2FE5) if ents[i] & 1 and (ents[i] >> 12) == i]
    say("Halo 2's region 0x61-0x2FE4: %d of %d pages carry an identity PTE (allocated); first %s, last %s" % (
        len(mapped), 0x2FE5 - 0x61, '%04X' % mapped[0] if mapped else '-', '%04X' % mapped[-1] if mapped else '-'))
    odd = [(i, ents[i]) for i in range(0x61, 0x2FE5) if ents[i] & 1 and (ents[i] >> 12) != i]
    say('  entries in the region that are present but not identity (junk): %d%s' % (len(odd), ' e.g. ' + ' '.join('%04X=%08X' % e for e in odd[:12]) if odd else ''))
    say('NOT free inside Halo 2\'s region 0x61-0x2FE4: %s' % (', '.join('%04X-%04X %s' % (i, j - 1, s) for i, j, s in busy) or 'nothing'))

    holes = [i for i in range(0x61, 0x2FE5) if not (ents[i] & 1 and (ents[i] >> 12) == i)]
    say('region holes: %d pages; entries of the first 40: ' % len(holes))
    for i in holes[:40]:
        e = ents[i]
        say('  %04X = %08X  type %X, bits27-18 %03X, bit16 %d, low word %04X' % (i, e, e >> 28, (e >> 18) & 0x3FF, (e >> 16) & 1, e & 0xFFFF))
    # where are those pages mapped? walk every page table reachable from the page directory
    pd = x86(0x8000F000, 0x1000)
    raw['pd'] = pd
    pdes = [struct.unpack_from('<I', pd, 4 * i)[0] for i in range(1024)]
    say('page directory: %d present entries: ' % sum(1 for v in pdes if v & 1) + ' '.join(
        '[%03X]=%08X' % (i, v) for i, v in enumerate(pdes) if v & 1 and not (0x200 <= i < 0x220)))
    want = set(holes[:600])
    found = {}
    for i, v in enumerate(pdes):
        if not v & 1 or v & 0x80 or 0x200 <= i < 0x220:
            continue
        try:
            pt = x.getmem(alias + (v & 0xFFFFF000), 0x1000)
        except (IOError, ValueError):
            say('  PT of PDE %03X (page %05X) unreadable' % (i, v >> 12))
            continue
        raw['pt_%03X' % i] = pt
        for j in range(1024):
            e = struct.unpack_from('<I', pt, 4 * j)[0]
            if e & 1 and (e >> 12) in want:
                found.setdefault(e >> 12, []).append(((i << 22) | (j << 12), e))
    for i, v in enumerate(pdes):
        if v & 1 and not v & 0x80 and not (0x200 <= i < 0x220) and ('pt_%03X' % i) in raw:
            pt = raw['pt_%03X' % i]
            pres = [(j, struct.unpack_from('<I', pt, 4 * j)[0]) for j in range(1024) if struct.unpack_from('<I', pt, 4 * j)[0] & 1]
            say('  PDE %03X (VA %08X, PT page %05X): %d present PTEs; first: %s' % (
                i, i << 22, v >> 12, len(pres), ' '.join('[%03X]=%08X' % e for e in pres[:12])))

    # the emulated NV2A push buffer: DMA_PUT (Halo 2's write pointer) vs DMA_GET (the GPU thread's read pointer)
    try:
        mirror = xp.be32(x.getmem(0x82171480 + 0xACC, 4), 0)
        put, get = xp.be32(x.getmem(mirror + 0x800040, 4), 0), xp.be32(x.getmem(mirror + 0x800044, 4), 0)
        say('NV2A push buffer: register mirror %08X, DMA_PUT %08X, DMA_GET %08X (%s)' % (mirror, put, get, 'GPU caught up' if put == get else 'GPU behind by 0x%X bytes' % ((put - get) & 0xFFFFFFFF)))
        say('  GPU globals +0..+64: ' + ' '.join('%08X' % xp.be32(x.getmem(0x82171480, 64), k) for k in range(0, 64, 4)))
    except (IOError, ValueError, OSError) as e:
        say('NV2A push buffer: %s' % e)
    # raw XBDM contexts of the x86 thread and the GPU thread (stopped for a moment): what the debugger returns for a
    # thread parked inside the emulator is its real switch-out frame, so r1 there is a usable kernel stack pointer
    try:
        roles = dict(xp.ROLES)
        roles[0x816A0400] = 'x86 (Halo 2 + Xbox kernel)'
        ctx_threads = [(tid, roles.get(xp.num(info.get('start', '0')), '')) for tid, info in xp.threads(x)]
        ctx_threads = [(t, r) for t, r in ctx_threads if r.startswith(('x86', 'GPU', 'service'))]
        x.cmd('stop')
        try:
            for tid, role in ctx_threads:
                for form in ('control int', 'full', 'control'):
                    try:
                        c, h, ls = x.cmd('getcontext thread=0x%08X %s' % (tid, form))
                        say('  getcontext %s [%s]: %s' % (role, form, (h + ' ' + ' '.join(ls))[:420]))
                        d = xp.kv((h + ' ' + ' '.join(ls)).replace('0q', '0x'))
                        sp = None
                        for key in ('r1', 'gpr1', 'sp'):
                            if key in d:
                                try:
                                    sp = int(d[key], 16) & 0xFFFFFFFF
                                except ValueError:
                                    pass
                        if sp and 0x30000000 <= sp < 0xE0000000:
                            # proper back-chain walk: [sp] = caller's frame, the function's saved LR sits at caller_frame - 8
                            chain = []
                            cur = sp
                            for _ in range(40):
                                try:
                                    nxt = xp.be32(x.getmem(cur, 4), 0)
                                    if not (cur < nxt < cur + 0x20000):
                                        break
                                    lr = xp.be32(x.getmem(nxt - 8, 4), 0)
                                    chain.append((cur, lr))
                                    cur = nxt
                                except (IOError, ValueError, OSError):
                                    break
                            say('    call chain (innermost first): ' + ' '.join('%08X' % lr for c, lr in chain))
                            blk = x.getmem(sp & ~0xF, 0x800)
                            raw['ctxstack_%s' % role[:3]] = (sp & ~0xF, blk)
                            words = [xp.be32(blk, k) for k in range(0, 0x800, 4)]
                            say('    stack words above r1=%08X: %s' % (sp, ' '.join('%08X' % v for v in words[:96])))
                            # R71 probe: the kernel objects this thread waits on - the emulator-heap pointers (0x3000xxxx-0x3007xxxx)
                            # that sit just before the kernel's wait frame; each object's dispatcher header: type, size, signal state
                            try:
                                objs = []
                                for k, v in enumerate(words[:96]):
                                    if 0x30000000 <= v < 0x30080000 and v % 4 == 0 and v not in objs:
                                        objs.append(v)
                                for o in objs[:6]:
                                    h = x.getmem(o, 0x20)
                                    say('    waits on? object %08X: header %s (type byte %d, signal state %08X)' % (o, ' '.join('%08X' % xp.be32(h, k) for k in range(0, 0x20, 4)), h[0], xp.be32(h, 4)))
                            except (IOError, ValueError, OSError) as e:
                                say('    wait objects: %s' % e)
                            emu = [v for v in words if 0x82000000 <= v < 0x82580000]
                            krn = [v for v in words if 0x80000000 <= v < 0x82000000]
                            usr = [v for v in words if 0x70000000 <= v < 0xC0000000 and not (0x7001C000 <= v < 0x70020000)]
                            say('    emulator: %s | 360 kernel: %s | user/translated: %s' % (' '.join('%08X' % v for v in emu[:16]), ' '.join('%08X' % v for v in krn[:12]), ' '.join('%08X' % v for v in usr[:16])))
                            # translated-code addresses carry a 0x50-byte header: show it for the first few (the x86 EIP should be in it)
                            shown = 0
                            for v in usr:
                                if shown >= 4:
                                    break
                                try:
                                    hdr = x.getmem(v - 0x50, 0x50)
                                    say('    header of %08X: %s' % (v, ' '.join('%08X' % xp.be32(hdr, k) for k in range(0, 0x50, 4))))
                                    shown += 1
                                except (IOError, ValueError, OSError):
                                    pass
                        break
                    except (IOError, ValueError) as e:
                        say('  getcontext %s [%s]: %s' % (role, form, e))
        finally:
            x.cmd('go')
    except (IOError, ValueError, OSError) as e:
        say('contexts: %s' % e)
    # is any thread parked by the debug monitor (an exception caught for a debugger that isn't there)? XBDM 'isstopped'
    try:
        say('')
        for tid, info in xp.threads(x):
            try:
                c, h, ls = x.cmd('isstopped thread=0x%08X' % tid)
                txt = (h + ' ' + ' '.join(ls)).strip()
                if 'not stopped' not in txt.lower():
                    say('  thread 0x%08X (start %s): %s' % (tid, info.get('start', '?'), txt[:300]))
            except (IOError, ValueError) as e:
                say('  thread 0x%08X: isstopped %s' % (tid, e))
    except (IOError, ValueError, OSError) as e:
        say('isstopped: %s' % e)
    # R60+: the GPU thread's per-page resource tables live at the top of game RAM (alias + 0x7FF0000, 2 x 2048 x 4 B).
    # Which guest 64 KB pages above 64 MB have entries - i.e. what did the x86 side register as a GPU resource up there?
    try:
        tabs = x.getmem(alias + 0x7FF0000, 0x4000)
        if any(tabs):
            for kind in (0, 1):
                hits = []
                for pg in range(2048):
                    v = xp.be32(tabs, kind * 0x2000 + pg * 4)
                    if v:
                        hits.append((pg, v))
                say('GPU resource table kind %d: %d pages with entries; above 64 MB: %s' % (kind, len(hits),
                    ' '.join('%03X=%08X' % (pg, v) for pg, v in hits if pg >= 0x400)[:600] or 'none'))
                for pg, v in hits:
                    if pg >= 0x400:
                        pt = raw.get('pagetable', b'')
                        ents = [struct.unpack_from('<I', pt, 4 * (pg * 16 + k))[0] for k in range(16)] if len(pt) >= 4 * (pg * 16 + 16) else []
                        say('  page %03X (guest %08X): x86 kernel entries %s' % (pg, pg << 16, ' '.join('%08X' % e for e in ents[:16])))
                        try:
                            rec = x.getmem(v, 0x60)
                            say('    record %08X: %s' % (v, ' '.join('%08X' % xp.be32(rec, k) for k in range(0, 0x60, 4))))
                        except (IOError, ValueError, OSError):
                            say('    record %08X unreadable' % v)
        else:
            say('GPU resource tables at alias+0x7FF0000: all zero (not an R60+ emulator, or nothing registered)')
    except (IOError, ValueError, OSError) as e:
        say('GPU resource tables: %s' % e)
    # the x86 kernel's bug-check record (KiBugCheckData, ordinal 343 at 0x80024CFC): code + 4 parameters - set when the
    # Xbox kernel itself gave up (an unhandled x86 exception: code 0x7E / 0x1E with the EIP in a parameter)
    try:
        bc = x.getmem(alias + 0x24CFC, 20)
        say('x86 KiBugCheckData: ' + ' '.join('%08X' % struct.unpack_from('<I', bc, k)[0] for k in range(0, 20, 4)))
    except (IOError, ValueError, OSError) as e:
        say('KiBugCheckData: %s' % e)
    # the pending request blocks (cpuobj+264 / +268 point at user addresses; kernel view is + the user->kernel delta)
    try:
        delta = xp.be32(x.getmem(cpu + 124, 4), 0)
        for off in (264, 268):
            ptr = xp.be32(x.getmem(cpu + off, 4), 0)
            if ptr:
                blk = x.getmem(ptr + delta, 64)
                say('request block at cpuobj+%d (%08X -> %08X): ' % (off, ptr, ptr + delta) + ' '.join('%08X' % xp.be32(blk, k) for k in range(0, 64, 4)))
    except (IOError, ValueError, OSError) as e:
        say('request blocks: %s' % e)
    cp = x.getmem(cpu + 256, 32)
    # the emulator's own translation tables (HV structure at cpuobj+1116): where does it think these x86 VAs are?
    try:
        hv = xp.be32(x.getmem(cpu + 1116, 4), 0)
        tb_va, tb_pa = xp.be32(x.getmem(cpu + 1124, 4), 0), xp.be32(x.getmem(cpu + 1132, 4), 0)
        say('emulator tables: HV struct %08X, table pool VA %08X (phys ref %08X)' % (hv, tb_va, tb_pa))
        for va in (0xD0000000, 0xD0000798, 0xC0340000, 0x87FA0000, 0x87FB0000, 0x8042A000, 0x00010000):
            seg = va >> 28
            tbyte = x.getmem(hv + 0x840 + seg, 1)[0]
            if tbyte & 2:                       # 64 KB pages: one table per 128 MB
                idx = struct.unpack('>H', x.getmem(hv + 0x800 + (va >> 27) * 2, 2))[0]
                kind = '64K'
                off = ((va >> 16) & 0x7FF) * 4
            else:
                idx = struct.unpack('>H', x.getmem(hv + (va >> 23) * 2, 2))[0]
                kind = '4K'
                off = ((va >> 12) & 0x7FF) * 4
            if not idx:
                say('  VA %08X: segment type %02X (%s), no table' % (va, tbyte, kind))
                continue
            tab = tb_va + idx * 8192 - tb_pa
            ent = xp.be32(x.getmem(tab + off, 4), 0)
            say('  VA %08X: segment type %02X (%s) table %08X entry %08X -> 360 physical %08X' % (va, tbyte, kind, tab, ent, ent & 0xFFFFF000))
    except (IOError, ValueError, OSError) as e:
        say('emulator tables unreadable: %s' % e)
    # what sits in physical page 0x3FA0 (the 64 MB-aliased twin of the pool page table 0x7FA0)?
    for pg in (0x3FA0, 0x7FA0):
        try:
            b = x.getmem(alias + (pg << 12), 64)
            say('physical page %04X first 16 words: ' % pg + ' '.join('%08X' % struct.unpack_from('<I', b, k)[0] for k in range(0, 64, 4)))
        except (IOError, ValueError, OSError) as e:
            say('physical page %04X unreadable: %s' % (pg, e))
    say('cpuobj +256..+287: ' + ' '.join('%08X' % xp.be32(cp, k) for k in range(0, 32, 4)))
    say('hole pages found mapped elsewhere: %d of %d' % (len(found), len(want)))
    for pg in sorted(found)[:60]:
        say('  page %04X mapped at ' % pg + ', '.join('VA %08X (PTE %08X)' % m for m in found[pg]))
    unmapped = sorted(want - set(found))
    say('hole pages mapped nowhere: %d%s' % (len(unmapped), ' e.g. ' + ' '.join('%04X' % p for p in unmapped[:20]) if unmapped else ''))
    try:
      # 3. processor block, current thread, its stack
      pcr = x86(xp.X86_PCR, 0x60)
      raw['pcr'] = pcr
      cur = struct.unpack_from('<I', pcr, 0x28)[0]
      say('PCR: IRQL %d, current thread %08X, next %08X, idle %08X' % (pcr[0x24], cur, struct.unpack_from('<I', pcr, 0x2C)[0], struct.unpack_from('<I', pcr, 0x30)[0]))
      try:
          kt = x86v(cur, 0x110)
      except (IOError, ValueError) as e:
          say('current thread block %08X unreadable: %s' % (cur, e))
          kt = bytes(0x110)
      raw['kthread'] = kt
      say('KTHREAD %08X: ' % cur + ' '.join('%08X' % struct.unpack_from('<I', kt, i)[0] for i in range(0, 0x60, 4)))
      # candidate stack range: the kernel-space pointers in the first 0x40 bytes with the lowest / highest values
      ptrs = sorted({struct.unpack_from('<I', kt, i)[0] for i in range(0x10, 0x40, 4) if 0xD0000000 <= struct.unpack_from('<I', kt, i)[0] < 0xE0000000})
      say('stack candidates in the thread block: ' + ' '.join('%08X' % p for p in ptrs))
      if ptrs:
          lo, hi = min(ptrs) & ~0xFFF, (max(ptrs) + 0xFFF) & ~0xFFF
          if hi - lo > 0x6000:
              lo = hi - 0x6000
          try:
              st = x86v(lo, hi - lo)
              raw['stack_%08X' % lo] = st
              say('stack %08X-%08X: words that look like return addresses (Halo 2 < 0x00600000, kernel 0x8001xxxx-0x8003xxxx):' % (lo, hi))
              for i in range(0, len(st) - 3, 4):
                  v = struct.unpack_from('<I', st, i)[0]
                  if 0x10000 <= v < 0x600000 or 0x80010000 <= v < 0x80031000:
                      say('  [%08X] %08X' % (lo + i, v))
              # interrupt frames [EIP][CS=8][EFLAGS] left on the stack by the timer: the EIP the x86 was interrupted at
              frames = []
              for i in range(0, len(st) - 12, 4):
                  eip, cs, efl = struct.unpack_from('<III', st, i)
                  if cs == 8 and (efl & 0xFFC08000) == 0 and (efl & 0x200) and (eip < 0x600000 or 0x80010000 <= eip < 0x80040000):
                      frames.append((lo + i, eip, efl))
              say('interrupt frames on the stack (where the x86 was when the timer interrupted it - the LAST ones are the spin): ' +
                  (' '.join('[%08X] EIP %08X' % (a, e) for a, e, f in frames[-12:]) if frames else 'none'))
          except (IOError, ValueError) as e:
              say('stack unreadable: %s' % e)

      # 4. the emulator's CPU state page and the Halo 2 globals
      state = xp.be32(x.getmem(cpu + 180, 4), 0)
      sp = x.getmem(state, 0x400)
      raw['state'] = sp
      say('state page %08X: +160 %08X +164 (EFLAGS) %08X, request +616 %08X' % (state, xp.be32(sp, 160), xp.be32(sp, 164), xp.be32(sp, 616)))
      say('state page words 0..0x100: ' + ' '.join('%08X' % xp.be32(sp, i) for i in range(0, 0x100, 4)))
      # R71 probe: what the x86 thread was doing when the emulator blocked it - the state page's +608 (x86 EIP) / +612
      # (translated code) and the MMIO address on its 360 stack, decoded for the Xbox memory map
      try:
          eip = xp.be32(sp, 608)
          say('x86 at EIP %08X (translated %08X) when the emulator blocked; request code +616 %08X, posted flag +632 %08X' % (eip, xp.be32(sp, 612), xp.be32(sp, 616), xp.be32(sp, 632)))
      except Exception as e:
          say('x86 EIP: %s' % e)

      try:
          g = x86v(0x4E6420, 0x40)
      except (IOError, ValueError):
          g = bytes(0x40)
      raw['halo_globals'] = g
      say('Halo 2 physical memory globals 0x4E6420: ' + ' '.join('%08X' % struct.unpack_from('<I', g, i)[0] for i in range(0, 0x40, 4)))
      # D3D's push buffer bookkeeping: the device at [0x407488]: +0 put, +4 limit, +0x24 base, +0x28 end, +0x30 -> the
      # GET pointer D3D reads, +0x34/+0x38/+0x40/+0x44 fence ring; [0x404fd0] = the 'wait for the GPU' flag; and the
      # NV2A PFIFO registers the kick loop compares (mirror +0x3240 DMA_PUT / +0x3244 DMA_GET)
      try:
          dev = struct.unpack_from('<I', x86v(0x407488, 4), 0)[0]
          d = x86v(dev, 0x50)
          w = lambda o: struct.unpack_from('<I', d, o)[0]
          say('D3D device %08X: put %08X limit %08X base %08X end %08X getptr %08X fence +34 %08X +38 %08X +40 %08X +44 %08X flags+8 %08X' % (
              dev, w(0), w(4), w(0x24), w(0x28), w(0x30), w(0x34), w(0x38), w(0x40), w(0x44), w(8)))
          try:
              gp = w(0x30)
              say('  *getptr (the GET D3D sees): %08X' % struct.unpack_from('<I', x86v(gp, 4), 0)[0])
          except (IOError, ValueError) as e:
              say('  *getptr unreadable: %s' % e)
          say('  [0x404fd0] wait-for-GPU flag: %08X' % struct.unpack_from('<I', x86v(0x404fd0, 4), 0)[0])
          regs = struct.unpack_from('<I', x86v(dev + 0x934, 4), 0)[0] if False else None
      except (IOError, ValueError) as e:
          say('D3D device: %s' % e)
      try:
          mirror = xp.be32(x.getmem(0x82171480 + 0xACC, 4), 0)
          say('NV2A PFIFO mirror: +3240 DMA_PUT %08X +3244 DMA_GET %08X; +800040 %08X +800044 %08X' % (
              xp.be32(x.getmem(mirror + 0x3240, 4), 0), xp.be32(x.getmem(mirror + 0x3244, 4), 0),
              xp.be32(x.getmem(mirror + 0x800040, 4), 0), xp.be32(x.getmem(mirror + 0x800044, 4), 0)))
      except (IOError, ValueError, OSError) as e:
          say('NV2A PFIFO mirror: %s' % e)
      # the x86 last-error of the current thread's TEB: fs:[4] = thread's TLS/TEB pointer in the KPCR? skipped
    except (IOError, ValueError, OSError) as e:
        say('later part failed: %s' % e)
    import pickle
    pickle.dump(raw, open(fn.replace('.txt', '.bin'), 'wb'))
    log.close()
    print('saved', fn, 'and the raw .bin')
    return fn


if __name__ == '__main__':
    main()
