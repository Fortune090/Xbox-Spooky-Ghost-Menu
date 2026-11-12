#!/usr/bin/env python3
"""fp_audit.py <spooky.elf> - float-to-integer conversions made while another value is held on the x87 stack.

The 360's emulator has got such conversions wrong three times (the level grid, the landing recorder, the legs). This
searches the built mod two ways and exits 1 if either finds one:
  1. the next x87 instruction after a conversion uses a value it did not load (or the conversion does not pop);
  2. a count of the values held at each conversion (a forward pass over each routine).
A conversion that must stay goes through fti() (spooky.c), which is out of line: the stack holds nothing else there.
"""
import re,collections,subprocess,sys
elf=sys.argv[1]
subprocess.run('objdump -d --no-show-raw-insn %s > /tmp/_fpa.txt'%elf,shell=True)
L=open('/tmp/_fpa.txt').read().split('\n')
fns=collections.OrderedDict(); fn=None
for l in L:
    m=re.match(r'^[0-9a-f]+ <([^>]+)>:',l)
    if m: fn=m.group(1); fns[fn]=[]; continue
    m=re.match(r'^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$',l)
    if m and fn: fns[fn].append((int(m.group(1),16),m.group(2),m.group(3)))
x87=lambda mn: mn.startswith('f') and mn not in ('fnstcw','fldcw','fwait','fnstsw','fninit','fs')
def eff(mn):
    if mn in ('fnstcw','fldcw','fnstsw','fwait','fxch','fchs','fabs','fsqrt','fsin','fcos','frndint','fscale','fprem','fprem1','ftst','fxam','fnop','fclex','fnclex','ffree'): return 0
    if mn.startswith(('fld','fild')) or mn=='fptan': return 1
    if mn in ('fpatan','fyl2x'): return -1
    if mn in ('fcompp','fucompp'): return -2
    if mn.startswith(('fstp','fistp','fisttp','fcomp','fucomp','fcomip','fucomip','fbstp')): return -1
    if mn.startswith(('fst','fist','ficom')): return 0
    if mn.endswith('p'): return -1
    return 0
tot=0; one=[]; two=[]
for f,ins in fns.items():
    depth={}; d=0; tgt={}
    for k,(a,mn,op) in enumerate(ins):
        if a in tgt: d=tgt[a]
        depth[a]=d
        if x87(mn) or mn in ('fnstcw','fldcw'): d=max(d+eff(mn),0)
        if mn=='call':
            d=0
            for j in range(k+1,min(k+12,len(ins))):
                m2=ins[j][1]
                if x87(m2):
                    if not m2.startswith(('fld','fild')): d=1
                    break
                if m2 in ('call','ret','jmp'): break
        if mn.startswith('j'):
            m=re.match(r'([0-9a-f]+)',op)
            if m: tgt.setdefault(int(m.group(1),16),d)
        if mn in ('ret','jmp'): d=0
    for i,(a,mn,op) in enumerate(ins):
        if not mn.startswith('fist'): continue
        tot+=1
        nxt=None
        for j in range(i+1,min(i+40,len(ins))):
            if ins[j][1] in ('ret','jmp'): break
            if x87(ins[j][1]): nxt=ins[j]; break
        if mn in ('fist','fistl','fists') or (nxt is not None and not nxt[1].startswith(('fld','fild'))): one.append((f,hex(a),mn))
        if depth[a]+eff(mn)>0: two.append((f,hex(a),mn,depth[a]+eff(mn)))
print('float-to-integer conversions: %d; with another value held: %d (by the next instruction), %d (by the count)'%(tot,len(one),len(two)))
for x in one: print('  next-instruction: %s %s %s'%x)
for x in two: print('  count:            %s %s %s (held after: %d)'%x)
sys.exit(1 if one or two else 0)
