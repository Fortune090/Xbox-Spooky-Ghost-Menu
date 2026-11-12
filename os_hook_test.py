#!/usr/bin/env python3
"""6i.158 / 6i.159: runs hooks.S's won_hook (the Campaign Timer's split), os_place_call and os_new_call (Object Spawning)
and os_equip_call, os_actor_call and os_actor_on_call (a dropped biped's brain) for real
(32-bit, no libc) against stand-ins for the game's functions that take their arguments as the game's do - game_finish
(void), function_b7930 (the record in esi; tag, owner, 0 on the stack, ret 0xC) and function_b7b40 (stdcall (record), the
object in eax) - and that wreck every register they may: the arguments must arrive right, the result come back, and the
caller's registers and stack be as they were. Needs gcc with -m32."""
import os, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
HARNESS = r"""
	.text
	.globl	orig_game_finish
orig_game_finish:                     /* void(void): notes that the timer was told first */
	movl	won_n, %eax
	movl	%eax, finish_saw
	incl	finish_n
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	orig_obj_place
orig_obj_place:                       /* esi the record; (tag, owner, 0) on the stack; ret 0xC */
	movl	%esi, pl_esi
	movl	4(%esp), %eax
	movl	%eax, pl_tag
	movl	8(%esp), %eax
	movl	%eax, pl_owner
	movl	12(%esp), %eax
	movl	%eax, pl_last
	movl	$0x55, (%esi)
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	ret	$12
	.globl	orig_obj_new
orig_obj_new:                         /* stdcall (record); the object in eax */
	movl	4(%esp), %eax
	movl	%eax, nw_rec
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %esi
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	movl	$0xE1230007, %eax
	ret	$4
	.globl	orig_actor_equip
orig_actor_equip:                     /* eax the weapon; (character, unit, second weapon, grenade) on the stack; ret 0x10 */
	movl	%eax, eq_eax
	movl	4(%esp), %eax
	movl	%eax, eq_a1
	movl	8(%esp), %eax
	movl	%eax, eq_a2
	movl	12(%esp), %eax
	movl	%eax, eq_a3
	movl	16(%esp), %eax
	movl	%eax, eq_a4
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %esi
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	ret	$16
	.globl	orig_actor_new
orig_actor_new:                       /* eax the squad, edx the character; (unit, flag) on the stack; ret 8; the actor in eax */
	movl	%eax, an_eax
	movl	%edx, an_edx
	movl	4(%esp), %eax
	movl	%eax, an_a1
	movl	8(%esp), %eax
	movl	%eax, an_a2
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %esi
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	movl	$0xAC700003, %eax
	ret	$8
	.globl	orig_actor_on
orig_actor_on:                        /* eax the actor, cl on */
	movl	%eax, on_eax
	movzbl	%cl, %eax
	movl	%eax, on_cl
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %esi
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	xorl	%eax, %eax
	ret
	.globl	orig_squad_order
orig_squad_order:                     /* bx the order; (squad, flag) on the stack; ret 8 */
	movzwl	%bx, %eax
	movl	%eax, so_bx
	movl	4(%esp), %eax
	movl	%eax, so_a1
	movl	8(%esp), %eax
	movl	%eax, so_a2
	incl	so_n
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %esi
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	ret	$8
	.globl	orig_squad_follow
orig_squad_follow:                    /* eax the order; (squad) on the stack; ret 4 */
	movl	%eax, sf_eax
	movl	4(%esp), %eax
	movl	%eax, sf_a1
	incl	sf_n
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %esi
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	ret	$4
	.globl	orig_unit_request
orig_unit_request:                    /* eax the unit, edi the request; plain ret */
	movl	%eax, ur_eax
	movl	%edi, ur_edi
	movl	(%edi), %eax
	movl	%eax, ur_type
	movzbl	4(%edi), %eax
	movl	%eax, ur_flag
	incl	ur_n
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	movl	$0xdeadbeef, %ebx
	movl	$0xdeadbeef, %esi
	movl	$0xdeadbeef, %edi
	movl	$0xdeadbeef, %ebp
	ret
	.globl	cache_geo_take
cache_geo_take:                       /* the mod's C function, stood in for: its two arguments recorded, geo_ret returned; ecx / edx wrecked (as C may) */
	movl	4(%esp), %eax
	movl	%eax, gt_a1
	movl	8(%esp), %eax
	movl	%eax, gt_a2
	incl	gt_n
	movl	geo_ret, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	orig_geo_protect
orig_geo_protect:                     /* XPhysicalProtect(block, size, protection): stdcall, ret 12 */
	movl	4(%esp), %eax
	movl	%eax, gp_a1
	movl	8(%esp), %eax
	movl	%eax, gp_a2
	movl	12(%esp), %eax
	movl	%eax, gp_a3
	incl	gp_n
	xorl	%eax, %eax
	ret	$12
	.globl	cache_anim_take
cache_anim_take:                      /* the mod's C function, stood in for: its two arguments recorded, anim_ret returned */
	movl	4(%esp), %eax
	movl	%eax, at_a1
	movl	8(%esp), %eax
	movl	%eax, at_a2
	incl	at_n
	movl	anim_ret, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	anim_drive
anim_drive:                           /* the registers as the game has them at 0x1232b0, the call, what came back */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	%esp, ad_esp0
	movl	$0x80A00000, %esi
	movl	$0x300000, %ecx
	movl	$ad_top, %edi
	movl	$0x0B0B0B0B, %ebx
	movl	$0xB9B9B9B9, %ebp
	pushl	$4
	pushl	%ecx
	pushl	%esi
	call	anim_cache_hook
	movl	%esi, ad_esi
	movl	%edi, ad_edi
	movl	%ecx, ad_ecx
	xorl	%eax, %eax
	cmpl	$0x0B0B0B0B, %ebx
	jne	2f
	cmpl	$0xB9B9B9B9, %ebp
	jne	2f
	cmpl	ad_esp0, %esp
	jne	2f
	movl	$1, %eax
2:	movl	%eax, ad_kept
	movl	ad_esp0, %esp
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl	geo_drive
geo_drive:                            /* the registers as the game has them at 0x12da80, the call, what came back */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	%esp, gd_esp0
	movl	$0x80C00000, %ebx
	movl	$0x680000, %ecx
	movl	$0x680, %esi
	movl	$0x680000, %edi
	movl	$gd_top, %ebp
	movl	$0x11112222, %eax
	movl	$0x33334444, %edx
	pushl	$4
	pushl	%ecx
	pushl	%ebx
	call	geo_cache_hook
	movl	%ebx, gd_ebx
	movl	%esi, gd_esi
	movl	%edi, gd_edi
	movl	%ebp, gd_ebp
	movl	%ecx, gd_ecx
	movl	%esp, %eax
	subl	gd_esp0, %eax
	movl	%eax, gd_desp
	movl	gd_esp0, %esp
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl	se_before
se_before:                            /* the mod's C function, stood in for: (grid, columns, rows) recorded; returns se_ret if set, else the grid */
	movl	4(%esp), %eax
	movl	%eax, sb_grid
	movl	8(%esp), %eax
	movl	%eax, sb_cols
	movl	12(%esp), %eax
	movl	%eax, sb_rows
	incl	sb_n
	movl	se_ret, %eax
	testl	%eax, %eax
	jnz	1f
	movl	4(%esp), %eax
1:	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	se_after
se_after:
	movl	sx_n, %eax                    /* (called only after the game's function has run) */
	movl	%eax, sa_after_draws
	incl	sa_n
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	orig_screen_effect
orig_screen_effect:                   /* the game's: eax the tag; weights, columns, rows, grid on the stack; ret 0x10 */
	movl	%eax, sx_tag
	movl	4(%esp), %eax
	movl	%eax, sx_a1
	movl	8(%esp), %eax
	movl	%eax, sx_a2
	movl	12(%esp), %eax
	movl	%eax, sx_a3
	movl	16(%esp), %eax
	movl	%eax, sx_a4
	incl	sx_n
	movl	$0x0C0FFEE0, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret	$0x10
	.globl	se_drive
se_drive:                             /* as 0x13b52: the tag in eax, four arguments pushed; what came back */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	%esp, sd_esp0
	movl	$0x0B0B0B0B, %ebx
	movl	$0x51515151, %esi
	movl	$0xD1D1D1D1, %edi
	movl	$0xB9B9B9B9, %ebp
	movl	$0xE1230042, %eax
	pushl	$0x00777000                   /* the grid */
	pushl	$2
	pushl	$3
	pushl	$0x00666000                   /* the weights */
	call	se_hook
	movl	%eax, sd_eax
	xorl	%eax, %eax
	cmpl	$0x0B0B0B0B, %ebx
	jne	2f
	cmpl	$0x51515151, %esi
	jne	2f
	cmpl	$0xD1D1D1D1, %edi
	jne	2f
	cmpl	$0xB9B9B9B9, %ebp
	jne	2f
	cmpl	sd_esp0, %esp
	jne	2f
	movl	$1, %eax
2:	movl	%eax, sd_kept
	movl	sd_esp0, %esp
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl	fog_before
fog_before:                           /* the mod's C functions, stood in for: the order they and the game's function run in is recorded */
	incl	fo_seq
	movl	fo_seq, %eax
	movl	%eax, fo_before
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	fog_after
fog_after:
	incl	fo_seq
	movl	fo_seq, %eax
	movl	%eax, fo_after
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	orig_fx_b
orig_fx_b:                        /* the game's: cl and dl its two flags; plain ret */
	movl	%ecx, fo_ecx
	movl	%edx, fo_edx
	incl	fo_seq
	movl	fo_seq, %eax
	movl	%eax, fo_draw
	movl	$0x0A0B0C0D, %eax
	movl	$0x11112222, %ecx
	movl	$0x33334444, %edx
	ret
	.globl	fo_drive
fo_drive:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	%esp, fo_esp0
	movl	$0x0B0B0B0B, %ebx
	movl	$0x51515151, %esi
	movl	$0xD1D1D1D1, %edi
	movl	$0xB9B9B9B9, %ebp
	movl	$0x77770001, %ecx
	movl	$0x88880000, %edx
	call	fog_hook
	movl	%eax, fo_eax
	movl	%ecx, fo_ecx2
	movl	%edx, fo_edx2
	xorl	%eax, %eax
	cmpl	$0x0B0B0B0B, %ebx
	jne	2f
	cmpl	$0x51515151, %esi
	jne	2f
	cmpl	$0xD1D1D1D1, %edi
	jne	2f
	cmpl	$0xB9B9B9B9, %ebp
	jne	2f
	cmpl	fo_esp0, %esp
	jne	2f
	movl	$1, %eax
2:	movl	%eax, fo_kept
	movl	fo_esp0, %esp
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl	swap_tail_check
swap_tail_check:                      /* the mod's C function, stood in for */
	incl	sl_seq
	movl	sl_seq, %eax
	movl	%eax, sl_log
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	orig_present
orig_present:                         /* the game's: cdecl (bitmap, front) */
	movl	%ecx, sg_ecx
	movl	4(%esp), %eax
	movl	%eax, sg_a1
	movl	8(%esp), %eax
	movl	%eax, sg_a2
	incl	sl_seq
	movl	sl_seq, %eax
	movl	%eax, sg_run
	movl	$0x05A9F00D, %eax
	cmpl	$0, sg_stdcall
	jne	1f
	ret
1:	ret	$8
	.globl	swap_settle
swap_settle:                          /* the mod's C function, stood in for */
	incl	sl_seq
	movl	sl_seq, %eax
	movl	%eax, ss_run
	movl	$0xdeadbeef, %eax
	movl	$0xdeadbeef, %ecx
	movl	$0xdeadbeef, %edx
	ret
	.globl	st_drive
st_drive:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	%esp, sl_esp0
	movl	$1, sg_stdcall
	movl	$0x0B0B0B0B, %ebx
	movl	$0x51515151, %esi
	movl	$0xD1D1D1D1, %edi
	movl	$0xB9B9B9B9, %ebp
	movl	$0x77770004, %ecx
	pushl	$0x44
	pushl	$0x33
	call	sw_tail_hook                  /* (the hook takes the two arguments off, as the game's routine would) */
st_ret:
	movl	%eax, sl_eax
	xorl	%eax, %eax
	cmpl	$0x0B0B0B0B, %ebx
	jne	2f
	cmpl	$0x51515151, %esi
	jne	2f
	cmpl	$0xD1D1D1D1, %edi
	jne	2f
	cmpl	$0xB9B9B9B9, %ebp
	jne	2f
	cmpl	sl_esp0, %esp
	jne	2f
	movl	$1, %eax
2:	movl	%eax, sl_kept
	movl	$0, sg_stdcall
	movl	sl_esp0, %esp
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl	st_ret_addr
st_ret_addr:
	movl	$st_ret, %eax
	ret
	.globl	orig_after_windows
orig_after_windows:                   /* the game's function_13c20, no arguments: notes every register as it arrives */
	movl	%eax, aw_reg
	movl	%ecx, aw_reg+4
	movl	%edx, aw_reg+8
	movl	%ebx, aw_reg+12
	movl	%esi, aw_reg+16
	movl	%edi, aw_reg+20
	movl	%ebp, aw_reg+24
	incl	aw_n
	ret
	.globl	orig_damage
orig_damage:                          /* the game's function_d7b80: stdcall, six arguments, ret 0x18 - notes them and every register as it arrives */
	movl	%eax, dm_reg
	movl	%ecx, dm_reg+4
	movl	%edx, dm_reg+8
	movl	%ebx, dm_reg+12
	movl	%esi, dm_reg+16
	movl	%edi, dm_reg+20
	movl	%ebp, dm_reg+24
	movl	4(%esp), %eax
	movl	%eax, dm_arg
	movl	8(%esp), %eax
	movl	%eax, dm_arg+4
	movl	12(%esp), %eax
	movl	%eax, dm_arg+8
	movl	16(%esp), %eax
	movl	%eax, dm_arg+12
	movl	20(%esp), %eax
	movl	%eax, dm_arg+16
	movl	24(%esp), %eax
	movl	%eax, dm_arg+20
	movl	ohk_before_n, %eax
	movl	%eax, dm_before_at            /* ohk_before had run by now; ohk_after had not */
	movl	ohk_after_n, %eax
	movl	%eax, dm_after_at
	incl	dm_n
	movl	$0x5EED0001, %eax             /* (clobbers what a callee may) */
	movl	$0x5EED0002, %ecx
	movl	$0x5EED0003, %edx
	ret	$0x18
	.globl	guard
guard:                                /* unsigned guard(which, a, b): calls a thunk with sentinels in the callee-saved registers; the result, or 0xBAD00000 + n if one changed or the stack moved */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	$0x11111111, %ebx
	movl	$0x22222222, %esi
	movl	$0x33333333, %edi
	movl	$0x44444444, %ebp
	movl	%esp, sp_before
	movl	20(%esp), %eax
	cmpl	$0, %eax
	je	g_won
	cmpl	$1, %eax
	je	g_place
	cmpl	$6, %eax
	je	g_late
	cmpl	$7, %eax
	je	g_ohk
	cmpl	$3, %eax
	je	g_equip
	cmpl	$4, %eax
	je	g_actor
	cmpl	$5, %eax
	je	g_on
	cmpl	$8, %eax
	je	g_order
	cmpl	$9, %eax
	je	g_follow
	cmpl	$10, %eax
	je	g_request
	pushl	24(%esp)
	call	os_new_call
	addl	$4, %esp
	jmp	g_check
g_equip:
	pushl	36(%esp)
	pushl	36(%esp)
	pushl	36(%esp)
	pushl	36(%esp)
	call	os_equip_call
	addl	$16, %esp
	jmp	g_check
g_actor:
	pushl	36(%esp)
	pushl	36(%esp)
	pushl	36(%esp)
	pushl	36(%esp)
	call	os_actor_call
	addl	$16, %esp
	jmp	g_check
g_on:
	pushl	24(%esp)
	call	os_actor_on_call
	addl	$4, %esp
	jmp	g_check
g_order:
	pushl	28(%esp)
	pushl	28(%esp)
	call	os_order_call
	addl	$8, %esp
	jmp	g_check
g_follow:
	pushl	28(%esp)
	pushl	28(%esp)
	call	os_follow_call
	addl	$8, %esp
	jmp	g_check
g_request:
	pushl	28(%esp)
	pushl	28(%esp)
	call	os_request_call
	addl	$8, %esp
	jmp	g_check
g_ohk:
	movl	$0xAAAA0001, %eax
	movl	$0xBBBB0002, %ecx
	movl	$0xCCCC0003, %edx
	pushl	$0x66660006
	pushl	$0x55550005
	pushl	$0x44440004
	pushl	$0x33330003
	pushl	$0x00110008
	pushl	$0x0DA7A000
	call	ohk_hook                      /* (takes its six arguments off, as the game's function does) */
	movl	%eax, dm_ret
	jmp	g_check
g_late:
	movl	$0xAAAA0001, %eax
	movl	$0xBBBB0002, %ecx
	movl	$0xCCCC0003, %edx
	call	late_hook
	jmp	g_check
g_place:
	pushl	28(%esp)
	pushl	28(%esp)
	call	os_place_call
	addl	$8, %esp
	jmp	g_check
g_won:
	call	won_hook
g_check:
	cmpl	sp_before, %esp
	jne	g_bad1
	cmpl	$0x11111111, %ebx
	jne	g_bad2
	cmpl	$0x22222222, %esi
	jne	g_bad3
	cmpl	$0x33333333, %edi
	jne	g_bad4
	cmpl	$0x44444444, %ebp
	jne	g_bad5
	jmp	g_out
g_bad1:	movl	$0xBAD00001, %eax
	movl	sp_before, %esp
	jmp	g_out
g_bad2:	movl	$0xBAD00002, %eax
	jmp	g_out
g_bad3:	movl	$0xBAD00003, %eax
	jmp	g_out
g_bad4:	movl	$0xBAD00004, %eax
	jmp	g_out
g_bad5:	movl	$0xBAD00005, %eax
g_out:
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.data
	.globl	won_n, finish_n, finish_saw, pl_esi, pl_tag, pl_owner, pl_last, nw_rec, sp_before
	.globl	dm_reg, dm_arg, dm_n, dm_before_at, dm_after_at, dm_ret
dm_reg: .long 0, 0, 0, 0, 0, 0, 0
dm_arg: .long 0, 0, 0, 0, 0, 0
dm_n: .long 0
dm_before_at: .long 0
dm_after_at: .long 0
dm_ret: .long 0
	.globl	gt_a1, gt_a2, gt_n, geo_ret, gp_a1, gp_a2, gp_a3, gp_n, gd_esp0, gd_top, gd_ebx, gd_esi, gd_edi, gd_ebp, gd_ecx, gd_desp, cache_geo_pages, cache_geo_bytes
gt_a1: .long 0
gt_a2: .long 0
gt_n: .long 0
geo_ret: .long 0
gp_a1: .long 0
gp_a2: .long 0
gp_a3: .long 0
gp_n: .long 0
gd_esp0: .long 0
gd_top: .long 0
gd_ebx: .long 0
gd_esi: .long 0
gd_edi: .long 0
gd_ebp: .long 0
gd_ecx: .long 0
gd_desp: .long 0
cache_geo_pages: .long 0
cache_geo_bytes: .long 0
	.globl	sb_grid, sb_cols, sb_rows, sb_n, se_ret, sa_n, sa_after_draws, sx_tag, sx_a1, sx_a2, sx_a3, sx_a4, sx_n, sd_esp0, sd_eax, sd_kept
sb_grid: .long 0
sb_cols: .long 0
sb_rows: .long 0
sb_n: .long 0
se_ret: .long 0
sa_n: .long 0
sa_after_draws: .long 0
sx_tag: .long 0
sx_a1: .long 0
sx_a2: .long 0
sx_a3: .long 0
sx_a4: .long 0
sx_n: .long 0
sd_esp0: .long 0
sd_eax: .long 0
sd_kept: .long 0
	.globl	at_a1, at_a2, at_n, anim_ret, ad_esp0, ad_top, ad_esi, ad_edi, ad_ecx, ad_kept
at_a1: .long 0
at_a2: .long 0
at_n: .long 0
anim_ret: .long 0
ad_esp0: .long 0
ad_top: .long 0
ad_esi: .long 0
ad_edi: .long 0
ad_ecx: .long 0
ad_kept: .long 0
	.globl	fo_seq, fo_before, fo_after, fo_draw, fo_ecx, fo_edx, fo_esp0, fo_eax, fo_ecx2, fo_edx2, fo_kept
fo_seq: .long 0
fo_before: .long 0
fo_after: .long 0
fo_draw: .long 0
fo_ecx: .long 0
fo_edx: .long 0
fo_esp0: .long 0
fo_eax: .long 0
fo_ecx2: .long 0
fo_edx2: .long 0
fo_kept: .long 0
	.globl	sl_arg, sl_seq, sl_log, sg_ecx, sg_a1, sg_a2, sg_run, sl_esp0, sl_eax, sl_kept, ef_runs, g_count_end_c
sl_arg: .long 0
	.globl	sg_stdcall, ss_run
sg_stdcall: .long 0
ss_run: .long 0
sl_seq: .long 0
sl_log: .long 0
sg_ecx: .long 0
sg_a1: .long 0
sg_a2: .long 0
sg_run: .long 0
sl_esp0: .long 0
sl_eax: .long 0
sl_kept: .long 0
ef_runs: .long 0
g_count_end_c: .long 0
	.globl	ur_eax, ur_edi, ur_type, ur_flag, ur_n
ur_eax: .long 0
ur_edi: .long 0
ur_type: .long 0
ur_flag: .long 0
ur_n: .long 0
	.globl	so_bx, so_a1, so_a2, so_n, sf_eax, sf_a1, sf_n
so_bx: .long 0
so_a1: .long 0
so_a2: .long 0
so_n: .long 0
sf_eax: .long 0
sf_a1: .long 0
sf_n: .long 0
	.globl	aw_reg, aw_n
aw_reg: .long 0, 0, 0, 0, 0, 0, 0
aw_n: .long 0
	.globl	eq_eax, eq_a1, eq_a2, eq_a3, eq_a4, an_eax, an_edx, an_a1, an_a2, on_eax, on_cl
eq_eax: .long 0
eq_a1: .long 0
eq_a2: .long 0
eq_a3: .long 0
eq_a4: .long 0
an_eax: .long 0
an_edx: .long 0
an_a1: .long 0
an_a2: .long 0
on_eax: .long 0
on_cl: .long 0
won_n: .long 0
finish_n: .long 0
finish_saw: .long 0
pl_esi: .long 0
pl_tag: .long 0
pl_owner: .long 0
pl_last: .long 0
nw_rec: .long 0
sp_before: .long 0
	.text
"""
MAIN = r"""
extern unsigned won_n, finish_n, finish_saw, pl_esi, pl_tag, pl_owner, pl_last, nw_rec;
extern unsigned eq_eax, eq_a1, eq_a2, eq_a3, eq_a4, an_eax, an_edx, an_a1, an_a2, on_eax, on_cl;
unsigned guard(unsigned which, unsigned a, unsigned b, unsigned c, unsigned d);
void ct_won(void) { won_n++; }
extern unsigned dm_reg[7], dm_arg[6], dm_n, dm_before_at, dm_after_at, dm_ret; unsigned ohk_before_n, ohk_after_n, ohk_b_data, ohk_b_obj;
void ohk_before(unsigned data, unsigned obj) { volatile unsigned j[4] = { 1, 2, 3, 4 }; ohk_before_n += j[0]; ohk_b_data = data; ohk_b_obj = obj; }
void ohk_after(void) { volatile unsigned j[4] = { 1, 2, 3, 4 }; ohk_after_n += j[0]; }
extern unsigned aw_reg[7], aw_n; unsigned late_n; void spooky_after_windows(void) { volatile unsigned junk[4] = { 1, 2, 3, 4 }; late_n += junk[0]; }
int test_main(void)
{
	static unsigned rec[0x31];
	unsigned r;
	r = guard(0, 0, 0, 0, 0);                                            /* won_hook: the timer told, then the game's own, once each */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 10 + (int)(r & 15u);
	if (!(won_n == 1 && finish_n == 1 && finish_saw == 1)) return 20;
	r = guard(1, (unsigned)rec, 0xE4560123u, 0, 0);                      /* os_place_call(record, tag) */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 30 + (int)(r & 15u);
	if (!(pl_esi == (unsigned)rec && pl_tag == 0xE4560123u && pl_owner == 0xFFFFFFFFu && pl_last == 0 && rec[0] == 0x55)) return 40;
	r = guard(2, (unsigned)rec, 0, 0, 0);                                /* os_new_call(record): the object back */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 50 + (int)(r & 15u);
	if (!(nw_rec == (unsigned)rec && r == 0xE1230007u)) return 60;
	r = guard(3, 0x77770001u, 0xC4A20002u, 0x00550003u, 0x77780009u);   /* os_equip_call(weapon, character, unit, second weapon) */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 70 + (int)(r & 15u);
	if (!(eq_eax == 0x77770001u && eq_a1 == 0xC4A20002u && eq_a2 == 0x00550003u && eq_a3 == 0x77780009u && eq_a4 == 0xFFFFFFFFu)) return 80;
	r = guard(4, 0x0000002Au, 0xC4A20002u, 0x00550003u, 1);        /* os_actor_call(squad, character, unit, flag); the actor back */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 90 + (int)(r & 15u);
	if (!(an_eax == 0x0000002Au && an_edx == 0xC4A20002u && an_a1 == 0x00550003u && an_a2 == 1 && r == 0xAC700003u)) return 100;
	r = guard(5, 0xAC700003u, 0, 0, 0);                               /* os_actor_on_call(actor) */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 110 + (int)(r & 15u);
	if (!(on_eax == 0xAC700003u && on_cl == 1)) return 120;
	r = guard(6, 0, 0, 0, 0);                                      /* late_hook: the menu's late drawing first, then the game's call with every register as the game had it */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 130 + (int)(r & 15u);
	if (!(late_n == 1 && aw_n == 1 && aw_reg[0] == 0xAAAA0001u && aw_reg[1] == 0xBBBB0002u && aw_reg[2] == 0xCCCC0003u
	      && aw_reg[3] == 0x11111111u && aw_reg[4] == 0x22222222u && aw_reg[5] == 0x33333333u && aw_reg[6] == 0x44444444u)) return 140;
	r = guard(7, 0, 0, 0, 0);                                      /* ohk_hook: before(event, object), the game's function with its six arguments and every register as the caller had them, after - and the stack as the game's function leaves it */
	if ((r & 0xFFF00000u) == 0xBAD00000u) return 150 + (int)(r & 15u);
	if (!(ohk_before_n == 1 && ohk_after_n == 1 && dm_n == 1 && dm_before_at == 1 && dm_after_at == 0 && ohk_b_data == 0x0DA7A000u && ohk_b_obj == 0x00110008u)) return 160;
	if (!(dm_arg[0] == 0x0DA7A000u && dm_arg[1] == 0x00110008u && dm_arg[2] == 0x33330003u && dm_arg[3] == 0x44440004u && dm_arg[4] == 0x55550005u && dm_arg[5] == 0x66660006u)) return 161;
	if (!(dm_reg[0] == 0xAAAA0001u && dm_reg[1] == 0xBBBB0002u && dm_reg[2] == 0xCCCC0003u && dm_reg[3] == 0x11111111u && dm_reg[4] == 0x22222222u && dm_reg[5] == 0x33333333u && dm_reg[6] == 0x44444444u)) return 162;
	if (dm_ret != 0x5EED0001u) return 163;                         /* what the game's function returned in eax comes back */
	{
		extern unsigned so_bx, so_a1, so_a2, so_n, sf_eax, sf_a1, sf_n;
		r = guard(8, 0x0000002Au, 0x00000007u, 0, 0);                  /* os_order_call(squad, order): the order in bx, (squad, 0) on the stack */
		if ((r & 0xFFF00000u) == 0xBAD00000u) return 170 + (int)(r & 15u);
		if (!(so_n == 1 && so_bx == 7 && so_a1 == 0x2Au && so_a2 == 0)) return 180;
		r = guard(9, 0x00000007u, 0x0000002Au, 0, 0);                  /* os_follow_call(order, squad): the order in eax, (squad) on the stack */
		if ((r & 0xFFF00000u) == 0xBAD00000u) return 190 + (int)(r & 15u);
		if (!(sf_n == 1 && sf_eax == 7 && sf_a1 == 0x2Au)) return 200;
	}
	{
		extern unsigned ur_eax, ur_edi, ur_type, ur_flag, ur_n;
		static unsigned char req[0x20] = { 0x2C, 0, 0, 0, 1 };
		r = guard(10, 0x00550003u, (unsigned)req, 0, 0);               /* os_request_call(unit, request): the unit in eax, the request in edi */
		if ((r & 0xFFF00000u) == 0xBAD00000u) return 210 + (int)(r & 15u);
		if (!(ur_n == 1 && ur_eax == 0x00550003u && ur_edi == (unsigned)req && ur_type == 0x2C && ur_flag == 1)) return 220;
	}
	{
		/* 6i.191: geo_cache_hook, with the registers as the game has them at the call it replaces */
		extern unsigned gt_a1, gt_a2, gt_n, geo_ret, gp_a1, gp_a2, gp_a3, gp_n, gd_top, gd_ebx, gd_esi, gd_edi, gd_ebp, gd_ecx, gd_desp, cache_geo_pages, cache_geo_bytes;
		void geo_drive(void);
		/* the mod has a block: the region gets its bytes back; ebx / esi / edi are the block and its size; the game's call is not made; the three arguments are off the stack */
		geo_ret = 0x8F000000u; cache_geo_pages = 0x1000u; cache_geo_bytes = 0x1000000u; gd_top = 0x00C00000u;
		geo_drive();
		if (!(gt_n == 1 && gt_a1 == 0x80C00000u && gt_a2 == 0x680000u)) return 230;
		if (!(gd_ebx == 0x8F000000u && gd_esi == 0x1000u && gd_edi == 0x1000000u && gd_top == 0x00C00000u + 0x680000u)) return 231;
		if (!(gp_n == 0 && gd_desp == 0 && gd_ebp == (unsigned)&gd_top && gd_ecx == 0x680000u)) return 232;
		/* it has none: the game's call as it was made, nothing else touched */
		geo_ret = 0; gd_top = 0x00C00000u;
		geo_drive();
		if (!(gt_n == 2 && gp_n == 1 && gp_a1 == 0x80C00000u && gp_a2 == 0x680000u && gp_a3 == 4)) return 233;
		if (!(gd_ebx == 0x80C00000u && gd_esi == 0x680u && gd_edi == 0x680000u && gd_top == 0x00C00000u && gd_desp == 0 && gd_ebp == (unsigned)&gd_top)) return 234;
	}
	{
		/* 6i.222: sw_tail_hook, as the game calls function_13f10 at its last "keep alive" - the check first, then the
		 * game's routine with its two arguments (it takes them off: the hook must leave the stack as
		 * that call would), then swap_settle; the game's answer back; registers and stack kept */
		extern unsigned sl_arg, sl_log, sg_ecx, sg_a1, sg_a2, sg_run, sl_eax, sl_kept, ss_run, sl_seq;
		void st_drive(void); unsigned st_ret_addr(void);
		unsigned base = sl_seq;
		st_drive();
		if (!(sl_log == base + 1 && sg_run == base + 2 && ss_run == base + 3)) return 298;
		if (!(sg_a1 == 0x33 && sg_a2 == 0x44 && sg_ecx == 0x77770004u && sl_eax == 0x05A9F00Du && sl_kept == 1)) return 299;
	}
	{
		/* 6i.198: fog_hook, as the game calls function_25dc0 - fog_before, the game's function with cl / dl as the caller set
		 * them, fog_after, in that order; the registers as the game's function left them; the stack as it was */
		extern unsigned fo_before, fo_after, fo_draw, fo_ecx, fo_edx, fo_eax, fo_ecx2, fo_edx2, fo_kept;
		void fo_drive(void);
		fo_drive();
		if (!(fo_before == 1 && fo_draw == 2 && fo_after == 3)) return 280;
		if (!(fo_ecx == 0x77770001u && fo_edx == 0x88880000u)) return 281;
		if (!(fo_eax == 0x0A0B0C0Du && fo_ecx2 == 0x11112222u && fo_edx2 == 0x33334444u && fo_kept == 1)) return 282;
	}
	{
		/* 6i.194: anim_cache_hook, with the registers as the game has them at 0x1232b0 */
		extern unsigned at_a1, at_a2, at_n, anim_ret, ad_top, ad_esi, ad_edi, ad_ecx, ad_kept, gp_n, gp_a1, gp_a2, gp_a3;
		void anim_drive(void);
		unsigned gp0 = gp_n;
		anim_ret = 0x8E000000u; ad_top = 0x00A00000u; anim_drive();        /* the mod has a block */
		if (!(at_n == 1 && at_a1 == 0x80A00000u && at_a2 == 0x300000u)) return 260;
		if (!(ad_esi == 0x8E000000u && ad_top == 0x00A00000u + 0x300000u && ad_edi == (unsigned)&ad_top && ad_ecx == 0x300000u && ad_kept == 1 && gp_n == gp0)) return 261;
		anim_ret = 0; ad_top = 0x00A00000u; anim_drive();                   /* it has none: the game's call as it was */
		if (!(at_n == 2 && gp_n == gp0 + 1 && gp_a1 == 0x80A00000u && gp_a2 == 0x300000u && gp_a3 == 4)) return 262;
		if (!(ad_esi == 0x80A00000u && ad_top == 0x00A00000u && ad_edi == (unsigned)&ad_top && ad_kept == 1)) return 263;
	}
	{
		/* 6i.193: se_hook, as the game calls function_2d140 at 0x13b52 */
		extern unsigned sb_grid, sb_cols, sb_rows, sb_n, se_ret, sa_n, sa_after_draws, sx_tag, sx_a1, sx_a2, sx_a3, sx_a4, sx_n, sd_eax, sd_kept;
		void se_drive(void);
		/* the mod hands back the caller's grid: the game's function gets the tag and its four arguments as they were; se_after after it; the stack and registers as the function leaves them */
		se_ret = 0; se_drive();
		if (!(sb_n == 1 && sb_grid == 0x00777000u && sb_cols == 3 && sb_rows == 2)) return 250;
		if (!(sx_n == 1 && sx_tag == 0xE1230042u && sx_a1 == 0x00666000u && sx_a2 == 3 && sx_a3 == 2 && sx_a4 == 0x00777000u)) return 251;
		if (!(sa_n == 1 && sa_after_draws == 1 && sd_eax == 0x0C0FFEE0u && sd_kept == 1)) return 252;
		/* the mod hands back its own copy: the game's function draws with THAT grid; everything else the same */
		se_ret = 0x00999000u; se_drive();
		if (!(sx_n == 2 && sx_tag == 0xE1230042u && sx_a1 == 0x00666000u && sx_a2 == 3 && sx_a3 == 2 && sx_a4 == 0x00999000u)) return 253;
		if (!(sb_n == 2 && sa_n == 2 && sa_after_draws == 2 && sd_eax == 0x0C0FFEE0u && sd_kept == 1)) return 254;
	}
	return 0;
}
"""
START = '\t.text\n\t.globl\t_start\n_start:\n\tcall\ttest_main\n\tmovl\t%eax, %ebx\n\tmovl\t$1, %eax\n\tint\t$0x80\n'
h = open(os.path.join(HERE, 'hooks.S')).read()
i0 = h.index('\t.globl ohk_hook'); j0 = h.index('/* replace the two `call 0x13cd0`'); i1 = h.index('\t.globl late_hook'); j1 = h.index('/* 6i.170: One Shot Kills')
i = h.index('\t.globl\twon_hook')
j = h.index("/* ---- 6i.48: the frame's call to the director")
d = tempfile.mkdtemp()
open(os.path.join(d, 't.S'), 'w').write(HARNESS + h[i1:j1] + h[i0:j0] + h[i:j])
open(os.path.join(d, 'm.c'), 'w').write(MAIN)
open(os.path.join(d, 's.S'), 'w').write(START)
cc = subprocess.run(['gcc', '-m32', '-O1', '-ffreestanding', '-nostdlib', '-static', '-fno-pic', '-fno-stack-protector', '-o', os.path.join(d, 't'),
                     os.path.join(d, 's.S'), os.path.join(d, 'm.c'), os.path.join(d, 't.S')], capture_output=True, text=True)
if cc.returncode: print(cc.stderr[-1500:]); sys.exit(2)
r = subprocess.run([os.path.join(d, 't')]).returncode
print('ohk_hook, late_hook, won_hook, os_place_call, os_new_call, os_equip_call, os_actor_call, os_actor_on_call, os_order_call, os_follow_call, os_request_call, geo_cache_hook, anim_cache_hook, se_hook, fog_hook, sw_tail_hook: the game\'s conventions, the arguments and the result right, the caller\'s registers and stack as they were' if r == 0 else 'FAILED: code %d' % r)
sys.exit(r)
