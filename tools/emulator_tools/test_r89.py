#!/usr/bin/env python3
"""R89 against R88L: the release build must do exactly what R88L does, less the logging.
 1. Outside the code cave, every word that differs from R88L is either one of R88L's logger call sites - and there R89
    has R63's own word again - or a branch into the cave, which goes to the routine of the same name in both builds.
 2. Each of the five fix routines (w_slot, w_take, w_promote, w_depth_match, and all 22 bitmap routines) is run in a
    PowerPC emulator (unicorn) in BOTH builds on the same randomized machine states - registers, records, the held
    clear, the frame counter - and must leave the same registers the site needs and the same memory, the logger's own
    counters, ring and tables aside.
usage: test_r89.py <R89_xefu1_1.xex> <R88L_xefu1_1.xex> <R63_xefu1_1.xex>      (needs unicorn)
"""
import sys, struct, os, random
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_builds_r22 as m
from unicorn import Uc, UC_ARCH_PPC, UC_MODE_PPC32, UC_MODE_BIG_ENDIAN, UC_HOOK_MEM_UNMAPPED, UC_PROT_ALL
from unicorn.ppc_const import *
n89, n88, n63 = (bytes(m.Xex(p).image()) for p in sys.argv[1:4])
B, CAVE0, CAVE1 = 0x82000000, 0x8213E800, 0x82140000
# The routines save registers with the 64-bit forms (std / ld), which the 360 needs and this emulator's 32-bit mode cannot
# run. For part 2 only, each build's cave is run from a copy in which every `std rS,d(rA)` is `stw rS,d+4(rA)` and every
# `ld rD,d(rA)` is `lwz rD,d+4(rA)` - the same value in the same place for the 32 bits these machine states hold. The
# same rewrite is applied to both builds; part 1 compares the builds as they are.
def run32(img):
    b = bytearray(img)
    for o in range(CAVE0 - 0x82000000, CAVE1 - 0x82000000, 4):
        x = struct.unpack_from('>I', b, o)[0]
        if (x >> 26) == 62 and (x & 3) == 0: x = (36 << 26) | (x & 0x03FF0000) | (((x & 0xFFFC) + 4) & 0xFFFF)
        elif (x >> 26) == 58 and (x & 3) == 0: x = (32 << 26) | (x & 0x03FF0000) | (((x & 0xFFFC) + 4) & 0xFFFF)
        struct.pack_into('>I', b, o, x)
    return bytes(b)
def w(img, a): return struct.unpack_from('>I', img, a - B)[0]
def bl_target(img, a):
    x = w(img, a)
    if (x >> 26) != 18 or not (x & 1): return None
    off = x & 0x03FFFFFC
    if off & 0x02000000: off -= 0x04000000
    return a + off
sys.argv = ['x', '/dev/null']                      # (the recipes read argv at import; only their symbol tables are wanted here)
import importlib
def syms_of(mod_name, stubs):
    import subprocess
    tmp = '/tmp/_t89_' + mod_name
    return tmp
# the two builds' routine addresses: from the branches at the sites themselves
SITE = dict(w_slot=0x820C3F30, w_take=0x820C62EC, w_promote=0x820C655C, w_depth_match=0x820C5E44)
ADDR = {k: (bl_target(n89, a), bl_target(n88, a)) for k, a in SITE.items()}
for k, (a, b_) in ADDR.items():
    assert a and b_ and CAVE0 <= a < CAVE1 and CAVE0 <= b_ < CAVE1, k

# ---- 1. the words outside the cave
diff = [B + i for i in range(0, len(n89), 4) if n89[i:i + 4] != n88[i:i + 4] and not (CAVE0 <= B + i < CAVE1)]
log_back = 0; rebranch = 0; bad = []
for a in diff:
    t89, t88 = bl_target(n89, a), bl_target(n88, a)
    if w(n89, a) == w(n63, a) and t88 and CAVE0 <= t88 < CAVE1: log_back += 1          # a logger site, R63's word again
    elif t89 and t88 and CAVE0 <= t89 < CAVE1 and CAVE0 <= t88 < CAVE1: rebranch += 1   # a branch into the cave in both
    else: bad.append(hex(a))
assert not bad, bad
# the re-aimed branches go to the same routine: the four named ones above, and the bitmap routines (compared by their code)
def code(img, a, n): return img[a - B:a - B + n]
bm = 0
for a in diff:
    t89, t88 = bl_target(n89, a), bl_target(n88, a)
    if w(n89, a) == w(n63, a): continue
    if a in SITE.values(): continue
    assert code(n89, t89, 12) == code(n88, t88, 12), hex(a)      # lis rX,BITMAPS@ha; addi rX,rX,...; blr - the same three words
    bm += 1
print('outside the cave: %d words differ from R88L - %d logger sites back to R63\'s own word, %d branches into the cave re-aimed (%d bitmap, %d fixes); nothing else' % (len(diff), log_back, rebranch, bm, rebranch - bm))
assert log_back == 35 and bm == 31 and rebranch - bm == 4, (log_back, bm, rebranch)
# the cave beyond R89's code is empty
end89 = max(i for i in range(CAVE0 - B, CAVE1 - B) if n89[i]) + 1
print('R89\'s added code: %d bytes (R88L: %d)' % (end89 - (CAVE0 - B), max(i for i in range(CAVE0 - B, CAVE1 - B) if n88[i]) + 1 - (CAVE0 - B)))

# ---- 2. the routines, side by side
n89r, n88r = run32(n89), run32(n88)
CNT, RING, STAB, FRAMEC, GPUG, TABLE = 0xD7FF8000, 0xD7FF4000, 0xD7FFA000, 0x8216F64C, 0x82171480, None
RECS = [0xC4B00000 + 0x400 * i for i in range(6)]            # six records, 0x400 apart
STOP = 0x82000100                                            # where LR points: a return ends the run
FOUND = 0x820C5E64                                           # w_depth_match's "found" exit
def machine(img, state):
    mu = Uc(UC_ARCH_PPC, UC_MODE_PPC32 | UC_MODE_BIG_ENDIAN)
    mu.mem_map(B, 0x600000); mu.mem_write(B, img)
    mu.mem_map(0xD7FF0000, 0x10000)
    mu.mem_map(0xC4B00000, 0x10000)
    mu.mem_map(0x70000000, 0x10000)                          # stack
    mu.mem_map(0x60000000, 0x10000)                          # the slot table w_slot stores into, the answer word w_promote writes
    extra = []
    def unmapped(uc, access, addr, size, value, ud):
        base = addr & ~0xFFFF
        try: uc.mem_map(base, 0x10000, UC_PROT_ALL); extra.append(base)
        except Exception: return False
        return True
    mu.hook_add(UC_HOOK_MEM_UNMAPPED, unmapped)
    for a, data in state['mem']: mu.mem_write(a, data)
    for r, v in state['regs'].items(): mu.reg_write(r, v)
    mu.reg_write(UC_PPC_REG_1, 0x70008000); mu.reg_write(UC_PPC_REG_LR, STOP)
    return mu
REGS = [UC_PPC_REG_0 + i for i in range(32)]
def run(img, entry, state, until):
    mu = machine(img, state)
    try: mu.emu_start(entry, 0, count=20000)
    except Exception as e:
        pc = mu.reg_read(UC_PPC_REG_PC)
        if pc not in until: raise
    pc = mu.reg_read(UC_PPC_REG_PC)
    return mu, pc
def stop_hook(mu, until):
    from unicorn import UC_HOOK_CODE
    def h(uc, addr, size, ud):
        if addr in until: uc.emu_stop()
    mu.hook_add(UC_HOOK_CODE, h)
def execute(img, entry, state, until):
    mu = machine(img, state); stop_hook(mu, until)
    try: mu.emu_start(entry, 0, count=20000)
    except Exception as e:
        print('FAULT in', 'R89' if img is n89r else 'R88L', 'entry %08X pc %08X lr %08X' % (entry, mu.reg_read(UC_PPC_REG_PC), mu.reg_read(UC_PPC_REG_LR)), {('r%d' % i): hex(mu.reg_read(UC_PPC_REG_0 + i)) for i in (1, 3, 4, 5, 6, 23, 26, 27, 28, 31)}); raise
    pc = mu.reg_read(UC_PPC_REG_PC)
    assert pc in until, hex(pc)
    return mu, pc
rnd = random.Random(89)
def rec_bytes(width, height, pending, fmt, start, end, colour=None, page=0x100):
    r = bytearray(rnd.randbytes(0x400))
    struct.pack_into('>I', r, 0x4C, width); struct.pack_into('>I', r, 0x50, height); struct.pack_into('>I', r, 0xE4, pending)
    struct.pack_into('>I', r, 0x38, fmt); struct.pack_into('>I', r, 0x1C, start); struct.pack_into('>I', r, 0x20, end); struct.pack_into('>I', r, 0x24, page)
    struct.pack_into('>I', r, 220, 0x60000800)               # (+220 -> host flags, read by the logger)
    return bytes(r)
def records():
    out = []
    sizes = [(640, 480), (832, 624), (1280, 960), (640, 480), (512, 512), (832, 624)]
    for i, a in enumerate(RECS):
        wd, hg = rnd.choice(sizes)
        pend = rnd.choice([0, 0, 0x0F, 0x03, 0x30, 0x3F, 0x10, 0x01])
        fmt = rnd.choice([0x2D200116, 0x18280106, 0x2D000000, 0x12345678])
        st = rnd.choice([0x07894000, 0x0749E000, 0x07C54000]); en = st + rnd.choice([0x12C000, 0x200000])
        out.append((a, rec_bytes(wd, hg, pend, fmt, st, en, page=rnd.choice([0x10, 0x11, 0x7FF, 0x800, 0x900]))))
    return out
def hold_bytes():
    frame = rnd.choice([100, 101])
    h = struct.pack('>IIII', rnd.choice([0, 0, 0x0F, 0x03]), frame, rnd.choice([640, 832, 1280]), rnd.choice(RECS + [0])) + rnd.randbytes(16)
    return h
def base_state():
    mem = records()
    mem.append((CNT + 0x70, hold_bytes()))
    mem.append((FRAMEC, struct.pack('>I', rnd.choice([100, 101]))))
    regs = {UC_PPC_REG_0 + i: rnd.getrandbits(32) for i in range(2, 32)}
    return dict(mem=mem, regs=regs)
def snapshot(mu, keep_regs, mem_ranges):
    return (tuple(mu.reg_read(r) for r in keep_regs), tuple(bytes(mu.mem_read(a, n)) for a, n in mem_ranges))
REC_RANGES = [(a, 0x400) for a in RECS]
HOLD_RANGE = [(CNT + 0x70, 0x20)]                            # the held clear (the counters at +0x90 / +0x94 are the logger's)
ALLR = lambda *skip: [UC_PPC_REG_0 + i for i in range(1, 32) if i not in skip]

# w_slot: r28 the record being replaced, r31 the one taking over, r23 the slot, r27 + r26 where the slot's word is stored.
# The site's contract: r5 / r6 are dead (and r0 / LR are R88L's to use across its logger call).
n = 0; held_now = moved = 0
for t in range(4000):
    st = base_state()
    st['regs'][UC_PPC_REG_28] = rnd.choice(RECS + [0]); st['regs'][UC_PPC_REG_31] = rnd.choice(RECS + [0])
    st['regs'][UC_PPC_REG_23] = rnd.choice([0, 0, 1, 1, 2, 3]); st['regs'][UC_PPC_REG_27] = 0x60000000; st['regs'][UC_PPC_REG_26] = 4 * rnd.randrange(8)
    res = []
    for img, ent in ((n89r, ADDR['w_slot'][0]), (n88r, ADDR['w_slot'][1])):
        mu, pc = execute(img, ent, st, (STOP,))
        res.append(snapshot(mu, ALLR(5, 6), REC_RANGES + HOLD_RANGE + [(0x60000000, 0x40)]))
    assert res[0] == res[1], ('w_slot', t)
    held_now += res[0][1][len(REC_RANGES)] != dict(st['mem'])[CNT + 0x70]
    moved += any(res[0][1][k] != dict(st['mem'])[RECS[k]] for k in range(len(RECS)))
    n += 1
assert held_now > 100 and moved > 100, (held_now, moved)
print('w_slot: %d machine states - the same registers (all but r0, r5, r6), records, held clear and slot table in both builds (a colour clear held %d times, a depth clear moved %d times)' % (n, held_now, moved))

# w_take: r11 the colour record being bound; returns r10. Everything else as it was.
n = 0; gave = 0
for t in range(4000):
    st = base_state(); st['regs'][UC_PPC_REG_11] = rnd.choice(RECS)
    res = []
    for img, ent in ((n89r, ADDR['w_take'][0]), (n88r, ADDR['w_take'][1])):
        mu, pc = execute(img, ent, st, (STOP,))
        res.append(snapshot(mu, ALLR(), REC_RANGES + HOLD_RANGE))
        if img is n89r:
            for i in range(1, 32):
                if i != 10: assert mu.reg_read(UC_PPC_REG_0 + i) == (st['regs'].get(UC_PPC_REG_0 + i, 0x70008000 if i == 1 else None)), ('w_take changed r%d' % i, t)
    assert res[0] == res[1], ('w_take', t)
    gave += any(res[0][1][k] != dict(st['mem'])[RECS[k]] for k in range(len(RECS)))
    before = dict(st['mem'])[CNT + 0x70][:4]; 
    n += 1
assert gave > 50, gave
print('w_take: %d machine states - the same r10, records and held clear in both builds; in R89 every other register is as it was (the held clear given %d times)' % (n, gave))

# w_promote: r3 the record, r4 -> the answer word. It calls the game's own test (0x820C1AA8) first; that routine's real
# body uses 64-bit register saves this emulator cannot run, so in both runnable copies it is replaced by one that returns
# a fixed r3 and leaves the answer word as the machine state has it (0: the game said no; 1: yes). What is compared is
# the stub's part: the search of the render-target table for a depth-format record over exactly the record's range.
PROMOTEQ = 0x820C1AA8
def with_fixed_test(img):
    b = bytearray(img); struct.pack_into('>II', b, PROMOTEQ - B, 0x38600055, 0x4E800020); return bytes(b)      # li r3,0x55 ; blr
p89, p88 = with_fixed_test(n89r), with_fixed_test(n88r)
# the table's address, from R89's own code: lis r9,TABLE@ha ; addi r9,r9,TABLE@l
TABLE = None
for o in range(ADDR['w_promote'][0] - B, ADDR['w_promote'][0] - B + 200, 4):
    x, y = struct.unpack_from('>II', n89, o)
    if (x >> 16) == 0x3D20 and (y >> 16) == 0x3929:
        lo = y & 0xFFFF; lo -= 0x10000 if lo & 0x8000 else 0
        TABLE = ((x & 0xFFFF) << 16) + lo & 0xFFFFFFFF; break
assert TABLE, 'table address not found in w_promote'
n = 0; yes = made = 0
for t in range(4000):
    st = base_state()
    P = 0x10
    mem = []
    for k, (a, data) in enumerate(st['mem'][:len(RECS)]):
        d = bytearray(data)
        struct.pack_into('>I', d, 0x24, rnd.choice([P, P, P, 0x7FF, 0x800]))
        struct.pack_into('>I', d, 0x1C, rnd.choice([0x07894000, 0x0749E000])); struct.pack_into('>I', d, 0x20, rnd.choice([0x079C0000, 0x07A00000]))
        struct.pack_into('>I', d, 0x38, rnd.choice([0x2D200116, 0x18280106, 0x2D000000]))
        struct.pack_into('>I', d, 0x108, rnd.choice(RECS + [0, 0]))                    # the chain's link for page P
        mem.append((a, bytes(d)))
    st['mem'] = mem + st['mem'][len(RECS):]
    st['mem'].append((TABLE + 4 * P, struct.pack('>I', rnd.choice(RECS + [0]))))
    st['regs'][UC_PPC_REG_3] = rnd.choice(RECS); st['regs'][UC_PPC_REG_4] = 0x60000100
    ans0 = rnd.choice([0, 0, 0, 1]); st['mem'].append((0x60000100, struct.pack('>I', ans0)))
    res = []
    for img, ent in ((p89, ADDR['w_promote'][0]), (p88, ADDR['w_promote'][1])):
        mu, pc = execute(img, ent, st, (STOP,))
        res.append(snapshot(mu, [UC_PPC_REG_3, UC_PPC_REG_1, UC_PPC_REG_LR] + [UC_PPC_REG_0 + i for i in range(13, 32)], REC_RANGES + [(0x60000100, 4)]))
    assert res[0] == res[1], ('w_promote', t)
    ans = struct.unpack('>I', res[0][1][-1])[0]
    yes += ans0; made += (ans and not ans0); n += 1
assert made > 50 and yes > 100, (made, yes)
print('w_promote: %d machine states - the same answer, r3 and kept registers in both builds (the game\'s own yes %d times, made yes by the fix %d times, no otherwise)' % (n, yes, made))

# w_depth_match: cr6 the descriptor compare, r31 the record. Ends at the return (not a match / refused) or at "found".
n = 0; outs = {STOP: 0, FOUND: 0}
for t in range(3000):
    st = base_state(); st['regs'][UC_PPC_REG_31] = rnd.choice(RECS)
    st['regs'][UC_PPC_REG_CR] = rnd.choice([0x00000020, 0x00000040, 0x00000080, 0x22222222, 0x44444444])   # cr6: eq / gt / lt among others
    res = []
    for img, ent in ((n89r, ADDR['w_depth_match'][0]), (n88r, ADDR['w_depth_match'][1])):
        mu, pc = execute(img, ent, st, (STOP, FOUND))
        res.append((pc, snapshot(mu, ALLR(3, 4), REC_RANGES)))
    assert res[0] == res[1], ('w_depth_match', t)
    outs[res[0][0]] += 1; n += 1
print('w_depth_match: %d machine states - the same exit (%d back to the search, %d found), registers (all but r3, r4) and records in both builds' % (n, outs[STOP], outs[FOUND]))

# the bitmap routines: each returns one address in one register; found by following every bitmap site
sites = [a for a in diff if w(n89, a) != w(n63, a) and a not in SITE.values()]
n = 0
for a in sites:
    st = base_state(); res = []
    for img, real in ((n89r, n89), (n88r, n88)):
        mu, pc = execute(img, bl_target(real, a), st, (STOP,))
        res.append(snapshot(mu, ALLR(), []))
    assert res[0] == res[1], hex(a); n += 1
print('bitmap routines: %d sites - the same registers out in both builds' % n)
print('R89 OK')
