/* The hooks the Spooky Ghost trainer patches into Halo 2's code at launch. Link-time code generation lets the game keep live values
 * in registers across calls it knows don't touch them, so every hook leaves all registers and the flags
 * exactly as the original call would have. */

	.section .text.hooks, "ax"

/* replaces `call 0x13c780` at 0x2c86e: the game's HUD text pass, inside the in-game frame route.
 * Run it first (it takes no arguments), then draw the menu: the game has just drawn its own text, so the
 * render target is the one text needs. Registers come back as the original call left them. */
	.globl hud_hook
hud_hook:
	call	orig_hud_text
	pushal
	pushfl
	cld
	call	spooky_frame
	popfl
	popal
	ret

/* replace the two `call 0x13cd0` (end of frame) in the two frame routes: count which route runs, then
 * continue into the original exactly as the game's call would */
	.globl end_a_hook
end_a_hook:
	pushfl
	incl	g_count_end_a
	popfl
	jmp	orig_end_frame

	.globl end_b_hook
end_b_hook:
	pushfl
	incl	g_count_end_b
	popfl
	jmp	orig_end_frame

/* replaces the main loop's `call 0x12b450` (v1.5: 0x12df60) at 0x12b6b0 (v1.5: 0x12e1c0) - the game's per-frame
 * update, called once per pass of the main loop: run our per-frame work first, then continue into it exactly as
 * the game's call would (it takes no arguments). (The 6-byte filler just before this call runs only once, before
 * the loop starts - not every frame.) */
	.globl main_hook
main_hook:
	pushal
	pushfl
	cld
	call	spooky_main_frame
	popfl
	popal
	jmp	orig_main_update

/* replaces `call 0x409b6c` (XInputGetState, __stdcall(handle, state)) at 0x124c34 in the controller poll:
 * returns XInputGetState's result in EAX like the original, preserving ECX and EDX as well */
	.globl input_hook
input_hook:
	pushl	%ecx
	pushl	%edx
	pushfl
	cld
	movl	20(%esp), %eax		/* state:  flags(0) edx(4) ecx(8) ret(12) handle(16) state(20) */
	pushl	%eax
	movl	20(%esp), %eax		/* handle: now at 16 + 4 */
	pushl	%eax
	call	spooky_input		/* cdecl u32 spooky_input(handle, state) */
	addl	$8, %esp
	popfl
	popl	%edx
	popl	%ecx
	ret	$8

/* replaces route A's `call 0x2c790` (v1.5: 0x2c770) - the HUD pass for a view - at 0x2b6e4 (v1.5: 0x2b6c4):
 * snapshot the world camera of the view record it is given (the record the world was just rendered from),
 * because by the time HUD text is drawn the HUD frame setup (function_132d0) has replaced both the frame
 * storage and g_camera_storage with the HUD's own camera. Then continue into the HUD pass unchanged. */
	.globl hud_view_hook
hud_view_hook:
	pushal
	pushfl
	cld
	pushl	40(%esp)		/* the view record: the call's argument, above pushal (32), pushfl (4), return (4) */
	call	spooky_view
	addl	$4, %esp
	popfl
	popal
	jmp	orig_hud_view

/* ---- third person (step 4g): the view build (function_223080: eax, ecx = player, edx = view, then 4 stack
 * arguments, the camera record last) gets a moved copy of the camera record; the first-person setup
 * (function_3d2c0) runs, then its object / models are cleared for a moved view ---- */
	.globl	tp_view_hook
tp_view_hook:
	pushal
	pushfl
	cld
	pushl	%ecx			/* the player */
	pushl	56(%esp)		/* the camera record: above push (4), pushfl (4), pushal (32), return (4), 3 arguments (12) */
	call	tp_camera
	addl	$8, %esp
	movl	%eax, 52(%esp)		/* the camera record argument, replaced */
	popfl
	popal
	jmp	orig_view_build

	.globl	rdmg_hook		/* the calls to the region damage (v1.0 0xdaaa0, v1.5 0xdcff0): skipped for a vehicle with
				   Invincible Vehicles on - its parts don't break (6e kept its body and death, not its regions) */
rdmg_hook:
	pushl	8(%esp)			/* the object (its 2nd argument) */
	call	rdmg_skip
	addl	$4, %esp
	testl	%eax, %eax
	jnz	1f
	jmp	orig_region_dmg		/* the game's own, as called */
1:	ret	$0x18			/* skipped: its own six arguments popped, as it does */
	.globl	hudd_hook		/* the view render's call to the HUD (v1.0 0x2bc93 / v1.5 0x2bc73): skipped in the free camera */
hudd_hook:
	call	hud_skip		/* C: 1 - the free camera's view: no HUD */
	testl	%eax, %eax
	jnz	1f
	jmp	orig_hud_draw		/* void(void): the game's own, as called */
1:	ret
	.globl	tp_fp_hook
tp_fp_hook:
	call	orig_fp_setup
	pushal
	pushfl
	cld
	call	tp_fp_after
	popfl
	popal
	ret

/* ---- Bottomless Clip (step 4h): a weapon's shot (function_102fb0, stdcall(weapon, barrel)) run in full, a player's
 * weapon's loaded rounds and battery noted before it and put back after ---- */
	.globl	bc_fire_hook
bc_fire_hook:
	pushal
	pushfl
	cld
	pushl	40(%esp)		/* the weapon: above pushal (32), pushfl (4), return (4) */
	call	bc_before
	addl	$4, %esp
	popfl
	popal
	pushl	8(%esp)			/* the barrel */
	pushl	8(%esp)			/* the weapon (one push lower now) */
	call	orig_fire		/* stdcall: takes its two arguments off */
	pushal
	pushfl
	cld
	call	bc_after
	popfl
	popal
	ret	$8

/* ---- Infinite Ammo (step 4h.1): the calls to function_106030 (eax = the weapon; the answer in al) - ia_answer says yes
 * (1) for a player's weapon while it's on, else (2) the game's own function answers ---- */
	.globl	ia_hook
ia_hook:
	pushfl
	pushal
	cld
	pushl	%eax			/* the weapon */
	call	ia_answer
	addl	$4, %esp
	cmpl	$2, %eax
	je	1f
	movl	%eax, 28(%esp)		/* the answer, into pushal's eax */
	popal
	popfl
	ret
1:	popal
	popfl
	jmp	orig_inf_ammo_fn

/* ---- third person, effects (step 4h.2): the effect code's call to first_person_weapon_get_marker (ecx = the object;
 * 4 stack arguments, ret 0x10; the answer in al) - "no" (fpm_answer 0) for a weapon held by a player seen in third
 * person, so its effects come from the weapon in the world; else (2) the game's own answer ---- */
	.globl	fpm_hook
fpm_hook:
	pushfl
	pushal
	cld
	pushl	%ecx			/* the object */
	call	fpm_answer
	addl	$4, %esp
	cmpl	$2, %eax
	je	1f
	movl	%eax, 28(%esp)		/* the answer, into pushal's eax */
	popal
	popfl
	ret	$0x10
1:	popal
	popfl
	jmp	orig_fp_marker

/* ---- third person, as Cartographer (step 4h.3): the director's perspective (function_155760, eax = the player, the
 * answer in eax) reported as "following" (3) to the render code, the first-person weapon code and the camera effect
 * code for a player seen in third person (persp_answer); -1: the game's own answer ---- */
	.globl	persp_hook
persp_hook:
	pushfl
	pushal
	cld
	pushl	%eax			/* the player */
	call	persp_answer
	addl	$4, %esp
	cmpl	$-1, %eax
	je	1f
	movl	%eax, 28(%esp)		/* the answer, into pushal's eax */
	popal
	popfl
	ret
1:	popal
	popfl
	jmp	orig_perspective

/* ---- s32 guarded_call(void (*fn)(void)): fn run under a structured-exception guard - a record on the fs:[0] chain,
 * as Cartographer's __try / __except on PC. A fault in fn (an access violation: a bad pointer read) resumes at gc_resume
 * instead of crashing the game: returns 1; 0 if fn ran through. The resume stack is the record itself (ring 0: an
 * interrupt uses this stack, so nothing needed may be below esp). After a fault the x87 state is reset and the caller's
 * control word restored. The Xbox CONTEXT (nxdk): Ebp +0x220, Eip +0x224, Esp +0x230. ---- */
	.globl	guarded_call
guarded_call:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	%esp, %ebp
	subl	$4, %esp
	fnstcw	(%esp)			/* [ebp-4]: the x87 control word */
	pushl	$gc_resume		/* record +12 [ebp-8]: where a fault resumes */
	pushl	%ebp			/* record +8 [ebp-12]: the frame */
	pushl	$gc_handler		/* record +4 [ebp-16] */
	pushl	%fs:0			/* record +0 [ebp-20]: the next record */
	movl	%esp, %fs:0
	call	*20(%ebp)		/* fn */
	xorl	%eax, %eax
	jmp	1f
gc_resume:
	fninit
	fldcw	-4(%ebp)
	movl	$1, %eax
1:	movl	-20(%ebp), %ecx
	movl	%ecx, %fs:0
	movl	%ebp, %esp
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
gc_handler:				/* (exception record, our record, context, dispatcher context) */
	movl	4(%esp), %eax
	testl	$6, 4(%eax)		/* unwinding: not ours to resume */
	jnz	2f
	movl	8(%esp), %edx		/* our record */
	movl	12(%esp), %ecx		/* the context */
	movl	12(%edx), %eax
	movl	%eax, 0x224(%ecx)	/* Eip: gc_resume */
	movl	%edx, 0x230(%ecx)	/* Esp: the record (everything needed above it) */
	movl	8(%edx), %eax
	movl	%eax, 0x220(%ecx)	/* Ebp: the frame */
	xorl	%eax, %eax		/* ExceptionContinueExecution */
	ret
2:	movl	$1, %eax		/* ExceptionContinueSearch */
	ret

/* ---- thunks into the game's drawing code (step 3a): its link-time-optimised calling conventions, with every
 * register the C code relies on saved around the call ---- */
#define SAVE	pushl %ebp; pushl %ebx; pushl %esi; pushl %edi
#define RESTORE	popl %edi; popl %esi; popl %ebx; popl %ebp
	.text
	.globl gfx_render_state		/* void(u32 state, u32 value): the cached setter, ESI = state, EDI = value */
gfx_render_state:
	SAVE
	movl	20(%esp), %esi
	movl	24(%esp), %edi
	call	orig_render_state
	RESTORE
	ret
	.globl gfx_vs_select		/* void(u32 index): EDX = vertex shader index */
gfx_vs_select:
	SAVE
	movl	20(%esp), %edx
	call	orig_vs_select
	RESTORE
	ret
	.globl gfx_vs_constants		/* void(u32 first, const void *data, u32 dwords): EBX, EDX, EAX */
gfx_vs_constants:
	SAVE
	movl	20(%esp), %ebx
	movl	24(%esp), %edx
	movl	28(%esp), %eax
	call	orig_vs_const
	RESTORE
	ret
	.globl gfx_ps_set		/* void(const void *program): EDX */
gfx_ps_set:
	SAVE
	movl	20(%esp), %edx
	call	orig_ps_set
	RESTORE
	ret
	.globl gfx_call0		/* void(void *fn): fn() */
gfx_call0:
	SAVE
	movl	20(%esp), %eax
	call	*%eax
	RESTORE
	ret
	.globl game_set_unit		/* void(void *fn, u32 player, u32 unit): player_set_unit - the player pushed, the unit in ebx */
game_set_unit:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	20(%esp), %eax		/* fn */
	movl	28(%esp), %ebx		/* the unit */
	pushl	24(%esp)			/* the player (it pops it: ret 4) */
	call	*%eax
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl game_call_eax		/* u32(void *fn, u32 a): fn with a in eax (Halo's deactivation veto) - its al */
game_call_eax:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	20(%esp), %ecx
	movl	24(%esp), %eax
	call	*%ecx
	movzbl	%al, %eax
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl game_call1r		/* u32(void *fn, u32 a): __stdcall fn(a) - its eax (gfx_call1's SAVE/RESTORE drop it) */
game_call1r:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	20(%esp), %eax
	pushl	24(%esp)
	call	*%eax			/* __stdcall: it pops its argument */
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl gfx_call1		/* void(void *fn, u32 a): __stdcall fn(a) */
gfx_call1:
	SAVE
	movl	20(%esp), %eax
	pushl	24(%esp)
	call	*%eax
	RESTORE
	ret
	.globl gfx_call3		/* void(void *fn, u32 a, u32 b, u32 c): __stdcall fn(a, b, c) */
gfx_call3:
	SAVE
	movl	20(%esp), %eax
	pushl	32(%esp)
	pushl	32(%esp)
	pushl	32(%esp)
	call	*%eax
	RESTORE
	ret

	.section .note.GNU-stack,"",@progbits

/* ---- step 6h.2: the HUD Widescreen Fix (spooky.c: hsq_*) - every hook a plain call and return (6h's return-address swaps
 * crashed the 360's emulator) and nothing of Direct3D's touched ---- */
	.section .text.hooks, "ax"
	.globl	hanc_hook		/* the drawers' calls to the HUD anchor point (v1.0 0x1396c7: eax anchor, esi out) */
hanc_hook:
	movl	%eax, g_hsq_type	/* 6h.13: the anchor kind (2 bottom-left) - mov leaves the flags alone */
	call	orig_hud_anchor		/* as called: eax, esi */
	pushal
	pushfl
	cld
	pushl	%esi			/* the point it wrote */
	call	hsq_anchor
	addl	$4, %esp
	popfl
	popal
	ret

/* the calls to an element's drawer: inside it (no squeeze until its anchor is known), its stack arguments passed on, the
 * call made, then the squeeze over; registers and flags as the original call leaves them */
	.macro	HDRAW name, orig, nargs
	.globl	\name
\name:
	pushfl
	movl	$1, g_hsq_in
	movl	$0, g_hsq_on
	popfl
	.rept	\nargs
	pushl	(4 * \nargs)(%esp)	/* the arguments, last first: each push moves the next one into place */
	.endr
	call	\orig
	pushfl
	movl	$0, g_hsq_in
	movl	$0, g_hsq_on
	popfl
	.if	\nargs
	ret	$(4 * \nargs)
	.else
	ret
	.endif
	.endm
	HDRAW	hdraw_grid, orig_d_grid, 2	/* (eax: its widget, as given) - ret 8 */
	HDRAW	hdraw_bmp, orig_d_bmp, 4	/* ret 0x10 */
	HDRAW	hdraw_text, orig_d_text, 4	/* ret 0x10 */
	HDRAW	hcentre_hook, orig_hud_centre, 0	/* (eax: the player) - ret */

	.globl	hquad_hook		/* every call to the HUD's textured quad (v1.0 0x22a664): ecx = its 4 corners */
hquad_hook:
	pushfl
	cmpl	$0, g_hsq_on
	je	1f
	cld
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	call	hsq_quad		/* cdecl u32 hsq_quad(u32 corners): a squeezed copy */
	addl	$4, %esp
	movl	%eax, %ecx
	popl	%edx
	popl	%eax
1:	popfl
	jmp	orig_hud_quad

	.globl	hgrid_hook		/* the shader widget drawer's call to its grid draw (v1.0 0x2d140 at 0x22bbbb) */
hgrid_hook:
	pushal
	pushfl
	cld
	pushl	0x34(%esp)		/* its 9 vertices: the 4th argument, above pushfl (4), pushal (32), return (4), 3 arguments */
	call	hsq_grid
	addl	$4, %esp
	popfl
	popal
	jmp	orig_grid_draw

	.globl	htext_hook		/* the text widget drawer's call to the string draw (v1.0 0x1fb10: ecx text; rect, scale) */
htext_hook:				/* 6h.3: the rectangle moved (only), then on into the game's own */
	pushal
	pushfl
	cld
	pushl	%ebp			/* the text drawer's frame: its placement point at [ebp-8] */
	pushl	44(%esp)		/* the rectangle: above the push (4), pushfl (4), pushal (32), return (4) */
	call	hsq_text_before
	addl	$8, %esp
	popfl
	popal
	jmp	orig_text_draw

	.globl	htrack_hook		/* the player HUD's call to the motion tracker (v1.0 0x1a2823: eax point, ecx player) */
htrack_hook:
	pushal
	pushfl
	cld
	call	hsq_track_begin
	popfl
	popal
	call	orig_tracker		/* eax, ecx as given */
	pushal
	pushfl
	cld
	call	hsq_track_end
	popfl
	popal
	ret

	.globl	htrkv_hook		/* the tracker's on-screen quad: its 4 calls to SetVertexData4f (edi register, 4 floats) */
htrkv_hook:
	pushfl
	cmpl	$-1, %edi		/* the position */
	jne	1f
	pushal
	cld
	leal	40(%esp), %eax		/* its x: above pushal (32), pushfl (4), return (4) */
	pushl	%eax
	call	hsq_trkv
	addl	$4, %esp
	popal
1:	popfl
	jmp	orig_set_vdata4f

/* ---- step 6h.11: a possessed vehicle in third person - the director's first-person / following choice (v1.0 0x155810:
 * ebx the unit, one argument - where its word answer goes; ax its answer) asked, then our say (spooky.c: cam_mode_answer) */
	.section .text.hooks, "ax"
	.globl	hcam_mode_hook
hcam_mode_hook:
	pushl	4(%esp)			/* its word */
	call	orig_cam_mode		/* stdcall: takes it off; ebx as given */
	pushal
	pushfl
	cld
	pushl	40(%esp)		/* the word: above pushfl (4), pushal (32), return (4) */
	pushl	%ebx			/* the unit */
	pushl	%eax			/* the game's answer */
	call	cam_mode_answer
	addl	$12, %esp
	movl	%eax, 32(%esp)		/* ours, in the saved eax */
	popfl
	popal
	ret	$4

/* ---- step 6h.12: Checkpoint Settings and the possessed vehicle's driving view ---- */
	.section .text.hooks, "ax"
	.globl	hsave_hook		/* the game's checkpoint logic asking for a save (v1.0 0x12b790): a call and a tail jump */
hsave_hook:
	pushal
	pushfl
	cld
	call	natural_save_check	/* g_cp_skip: Disable natural checkpoints */
	popfl
	popal
	cmpl	$0, g_cp_skip
	jne	1f
	jmp	orig_main_save
1:	ret				/* skipped: back as the save would have come back */

	.globl	hrevert_hook		/* the once-a-tick call that reverts you after you die (v1.0 0x138e40, no arguments) */
hrevert_hook:
	pushal
	pushfl
	cld
	call	death_revert_before
	popfl
	popal
	call	orig_death_revert
	pushal
	pushfl
	cld
	call	death_revert_after	/* a revert it raised, with Disable reverts on: lowered */
	popfl
	popal
	ret

	.globl	htarget_hook		/* the following camera's target (v1.0 0x1871e0: eax the player, esi its record) */
htarget_hook:
	call	orig_cam_target		/* eax, esi as given */
	pushal
	pushfl
	cld
	pushl	%esi
	call	cam_target_fix		/* a possessed vehicle: its driver seat's camera */
	addl	$4, %esp
	popfl
	popal
	ret

	.globl	cam_marker_call		/* cdecl void (u32 datum, u32 place): the game's marker step (v1.0 0xb9ef0: eax datum, ecx place) */
cam_marker_call:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	20(%esp), %eax
	movl	24(%esp), %ecx
	call	orig_cam_marker
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
