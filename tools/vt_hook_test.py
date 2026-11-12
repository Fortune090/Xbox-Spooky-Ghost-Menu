#!/usr/bin/env python3
"""6i.90: runs hooks.S's vt_space / vt_end / vt_get for real (32-bit, no libc) against stand-ins with Direct3D's own
conventions - MakeSpace (eax the size, its half on the stack, ret 4, the push pointer back in eax), EndVisibilityTest
(eax the test, plain ret) and GetVisibilityTestResult (eax the test, edx where the 8-byte time goes, the pixels' address
on the stack, ret 4) - called as C calls them: each argument where Direct3D reads it, the result back, and the stack
and the callee-saved registers as they were. Needs gcc with -m32."""
import os, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
h = open(os.path.join(HERE, 'hooks.S')).read()
i = h.index('/* ---- 6i.90'); j = h.rindex('/* ----', 0, h.index('\t.globl\tdrawidx_hook'))   # 6i.92: through the render target hooks
STUBS = r'''
	.text
	.globl	orig_dv_makespace, orig_vt_end, orig_dv_vistest, _start, orig_rt_fast, orig_rt_set, rt_switch
orig_rt_fast:                                 /* one argument, ret 4: records it and eax as it arrived */
	movl	4(%esp), %ecx
	movl	%ecx, rt1_arg
	movl	%eax, rt1_eax
	ret	$4
orig_rt_set:                                  /* two arguments, ret 8 */
	movl	4(%esp), %ecx
	movl	%ecx, rt2_a1
	movl	8(%esp), %ecx
	movl	%ecx, rt2_a2
	ret	$8
rt_switch:                                    /* the C side: counted, its target kept; eax / ecx / edx clobbered as C may */
	incl	rts_n
	movl	4(%esp), %eax
	movl	%eax, rts_arg
	movl	$0x5a5a5a5a, %eax
	movl	$0x5a5a5a5a, %ecx
	movl	$0x5a5a5a5a, %edx
	ret
orig_dv_makespace:
	movl	%eax, ms_eax
	movl	4(%esp), %ecx
	movl	%ecx, ms_half
	movl	$0x5a5a5a5a, %edx
	movl	$0x1234, %eax
	ret	$4
orig_vt_end:
	movl	%eax, ve_eax
	movl	$0x5a5a5a5a, %ecx
	movl	$0x5a5a5a5a, %edx
	xorl	%eax, %eax
	ret
orig_dv_vistest:
	movl	%eax, vg_eax
	movl	4(%esp), %ecx
	movl	$77, (%ecx)
	movl	$0x11, (%edx)
	movl	$0x22, 4(%edx)
	movl	$0x5a5a5a5a, %ecx
	xorl	%eax, %eax
	ret	$4
'''
CHK = r'''
	.globl	chk_%(n)s
chk_%(n)s:                                    /* int chk(void): the C call, then esp and ebx/esi/edi/ebp as they were */
	pushl	%%ebx
	pushl	%%esi
	pushl	%%edi
	pushl	%%ebp
	movl	$0x11111111, %%ebx
	movl	$0x22222222, %%esi
	movl	$0x33333333, %%edi
	movl	$0x44444444, %%ebp
	movl	%%esp, saved_esp
%(push)s
	call	%(n)s
	movl	%%eax, %(n)s_ret
	addl	$%(argb)d, %%esp
	xorl	%%eax, %%eax
	cmpl	saved_esp, %%esp
	jne	1f
	cmpl	$0x11111111, %%ebx
	jne	1f
	cmpl	$0x22222222, %%esi
	jne	1f
	cmpl	$0x33333333, %%edi
	jne	1f
	cmpl	$0x44444444, %%ebp
	jne	1f
	movl	$1, %%eax
1:	popl	%%ebp
	popl	%%edi
	popl	%%esi
	popl	%%ebx
	ret
'''
A = STUBS + CHK % dict(n='vt_space', push='\tpushl\t$0x8000', argb=4) + CHK % dict(n='vt_end', push='\tpushl\t$0x2001', argb=4) \
    + CHK % dict(n='vt_get', push='\tpushl\t$cts\n\tpushl\t$cpx\n\tpushl\t$0x2000', argb=12) + r'''
	.globl	chk_rt
chk_rt:                                       /* int chk_rt(void): both call sites as the game makes them */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	$0x11111111, %ebx
	movl	$0x22222222, %esi
	movl	$0x33333333, %edi
	movl	$0x44444444, %ebp
	movl	%esp, saved_esp
	movl	$0x55555555, %eax
	pushl	%esi
	call	rt_hook1
	movl	%eax, rt1_after
	pushl	%ebx
	pushl	%esi
	call	rt_hook2
	xorl	%eax, %eax
	cmpl	saved_esp, %esp
	jne	1f
	cmpl	$0x11111111, %ebx
	jne	1f
	cmpl	$0x22222222, %esi
	jne	1f
	cmpl	$0x33333333, %edi
	jne	1f
	cmpl	$0x44444444, %ebp
	jne	1f
	movl	$1, %eax
1:	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.data
	.globl	rt1_arg, rt1_eax, rt2_a1, rt2_a2, rts_n, rt1_after, rts_arg
rts_arg: .long 0
rt1_arg: .long 0
rt1_eax: .long 0
rt2_a1: .long 0
rt2_a2: .long 0
rts_n: .long 0
rt1_after: .long 0
	.globl	ms_eax, ms_half, ve_eax, vg_eax, cpx, cts, vt_space_ret, vt_end_ret, vt_get_ret
ms_eax: .long 0
ms_half: .long 0
ve_eax: .long 0
vg_eax: .long 0
cpx: .long 0
cts: .long 0, 0
saved_esp: .long 0
vt_space_ret: .long 0
vt_end_ret: .long 1
vt_get_ret: .long 1
	.text
_start:
	call	test_main
	movl	%eax, %ebx
	movl	$1, %eax
	int	$0x80
'''
MAIN = r'''
extern unsigned ms_eax, ms_half, ve_eax, vg_eax, cpx, cts[2], vt_space_ret, vt_end_ret, vt_get_ret;
int chk_vt_space(void), chk_vt_end(void), chk_vt_get(void), chk_rt(void);
extern unsigned rt1_arg, rt1_eax, rt2_a1, rt2_a2, rts_n, rts_arg; extern unsigned char spooky_rt_on;
int test_main(void)
{
	if (!chk_vt_space()) return 11;
	if (vt_space_ret != 0x1234 || ms_eax != 0x8000 || ms_half != 0x4000) return 12;
	if (!chk_vt_end()) return 21;
	if (vt_end_ret != 0 || ve_eax != 0x2001) return 22;
	if (!chk_vt_get()) return 31;
	if (vt_get_ret != 0 || vg_eax != 0x2000 || cpx != 77 || cts[0] != 0x11 || cts[1] != 0x22) return 32;
	spooky_rt_on = 1;
	if (!chk_rt()) return 41;
	if (rts_n != 2 || rt1_arg != 0x22222222 || rt1_eax != 0x55555555 || rt2_a1 != 0x22222222 || rt2_a2 != 0x11111111) return 42;
	if (rts_arg != 0x22222222) return 43;                    /* 6i.106: the target handed to rt_switch */
	spooky_rt_on = 0; rt1_arg = rt2_a1 = 0;
	if (!chk_rt() || rts_n != 2 || rt1_arg != 0x22222222 || rt2_a1 != 0x22222222 || rt2_a2 != 0x11111111) return 44;   /* 6i.109: off - straight on, not counted */
	return 0;
}
'''
d = tempfile.mkdtemp()
open(os.path.join(d, 't.S'), 'w').write(h[i:j] + A)
open(os.path.join(d, 'm.c'), 'w').write(MAIN)
cc = subprocess.run(['gcc', '-m32', '-O1', '-ffreestanding', '-nostdlib', '-static', '-fno-pic', '-fno-stack-protector', '-o',
                     os.path.join(d, 't'), os.path.join(d, 'm.c'), os.path.join(d, 't.S')], capture_output=True, text=True)
if cc.returncode: print(cc.stderr[-1500:]); sys.exit(2)
r = subprocess.run([os.path.join(d, 't')]).returncode
print('visibility-test thunks and the render target hooks: Direct3D\'s own conventions, the game\'s arguments and return, the stack and registers as they were' if r == 0 else 'FAILED %d' % r)
sys.exit(r)
