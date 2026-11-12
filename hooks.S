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

/* 6i.167: replaces `call 0x13c20` at 0x2c889 - the closing call of the pass that draws the HUD text (0x2c86e, above)
 * and then the game's windows (0x2c884: the pause menu, dialogs). The mod's menu, when a window of the game's is up,
 * is drawn here - after them, so in front (spooky.c spooky_after_windows) - then on into the game's call exactly as it
 * was made (no arguments). */
	.globl late_hook
late_hook:
	pushal
	pushfl
	cld
	call	spooky_after_windows
	popfl
	popal
	jmp	orig_after_windows

/* 6i.170: One Shot Kills - replaces every `call 0xd7b80` (the game's damage function: stdcall, six stack arguments -
 * the damage event, the object hit, then four more - ret 0x18; 20 calls). ohk_before(event, object) may set the
 * event's "kill" flag for this call; the function is then called with its six arguments copied and every register as
 * the caller had it; ohk_after takes the flag off again; back to the caller with the arguments taken off, as the
 * function itself would have left the stack. */
	.globl ohk_hook
ohk_hook:
	pushal
	pushfl
	cld
	pushl	44(%esp)                        /* the object hit ([esp+8] at entry) */
	pushl	44(%esp)                        /* the event ([esp+4] at entry) */
	call	ohk_before
	addl	$8, %esp
	popfl
	popal
	pushl	24(%esp)                        /* the six arguments again, last first (each push moves the next into place) */
	pushl	24(%esp)
	pushl	24(%esp)
	pushl	24(%esp)
	pushl	24(%esp)
	pushl	24(%esp)
	call	orig_damage
	pushal
	pushfl
	cld
	call	ohk_after
	popfl
	popal
	ret	$0x18

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
	.globl	tvt_hook		/* 6i.124: the scripts' trigger volume tests' call of function_11c470 (eax the volume,
				   edi the point; al: inside) - the game's own first, then tvt_note sees the answer; every register and
				   the flags left as the game's call left them */
tvt_hook:
	pushl	%eax			/* the volume */
	call	orig_tvt
	pushfl
	pushal
	movl	36(%esp), %ecx		/* the volume pushed above: past pushal (32) and pushfl (4) */
	pushl	%eax			/* the answer */
	pushl	%edi			/* the point */
	pushl	%ecx
	cld
	call	tvt_note
	addl	$12, %esp
	popal
	popfl
	leal	4(%esp), %esp		/* the saved volume dropped, the flags untouched */
	ret
	.globl	tv_matrix_call		/* 6i.124: s32 tv_matrix_call(s32 volume, float *matrix) - function_11c380 (eax the
				   volume, ebx a transform4x3f; al: placed) */
tv_matrix_call:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	20(%esp), %eax
	movl	24(%esp), %ebx
	call	orig_tv_matrix
	movzbl	%al, %eax
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
	.globl	hudd_hook		/* the view render's call to the HUD (v1.0 0x2bc93 / v1.5 0x2bc73): skipped in the free camera */
hudd_hook:
	call	hud_skip		/* C: 1 - the free camera's view: no HUD */
	testl	%eax, %eax
	jnz	1f
	jmp	orig_hud_draw		/* void(void): the game's own, as called */
1:	ret
	.globl	tp_fp_hook
tp_fp_hook:
	pushal				/* 6h.26: the first-person legs turned to the view's heading (slowed), for this build */
	pushfl
	cld
	call	ip_fp_before
	popfl
	popal
	call	orig_fp_setup
	pushal
	pushfl
	cld
	call	ip_fp_after		/* 6h.26: ... and put back */
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
 * answer in eax) reported as "following" (3) to the render code, the first-person weapon code, the camera effect
 * code and (6h.23) the object attachments' first-person tests for a player seen in third person (persp_answer); -1: the
 * game's own answer ---- */
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

/* ---- 6h.25: smooth slow motion at 30 ticks - frame interpolation. The tick loop's call to one tick (v1.0 0x137f89 ->
 * 0x137fe0, void(void)): the objects' node matrices snapshotted first (when slowed); the frame's call to the draw
 * (v1.0 0x12b659 -> 0x223240, void(void)): blended before it, put back after it ---- */
	.globl	ip_tick_hook
ip_tick_hook:
	pushfl
	pushal
	cld
	call	ip_tick_before
	popal
	popfl
	jmp	orig_game_tick

/* ---- 6i.156: the Campaign Timer's split - the script function game_won's one call to game_finish (v1.0 0x2a99b0 ->
 * 0x1388e0, void(void)): the timer told first (spooky.c ct_won), then the game's own, called and returned from plainly ---- */
	.globl	won_hook
won_hook:
	pushfl
	pushal
	cld
	call	ct_won
	popal
	popfl
	call	orig_game_finish
	ret

/* ---- 6i.158: Object Spawning - the game's two object-creation steps, called from C (cdecl), every register of the
 * caller's kept. os_place_call(record, tag): function_b7930 (v1.0 0xb7930) - the 0xC4-byte record in esi; the tag, the
 * owner object (-1: none) and 0 on the stack, which it takes off itself (ret 0xC). os_new_call(record): function_b7b40
 * (v1.0 0xb7b40) - stdcall (record), ret 4; the new object's index (-1: none) in eax ---- */
	.globl	os_place_call
os_place_call:
	pushal
	movl	36(%esp), %esi
	pushl	$0
	pushl	$-1
	pushl	48(%esp)
	call	orig_obj_place
	popal
	ret
	.globl	os_new_call
os_new_call:
	pushal
	pushl	36(%esp)
	call	orig_obj_new
	movl	%eax, 28(%esp)
	popal
	ret

/* ---- 6i.159: a brain for a dropped biped - three of the game's AI functions, called from C (cdecl), every register of
 * the caller's kept. os_equip_call(weapon, character, unit): function_1e04a0 (v1.0 0x1e04a0) - the weapon's tag in eax;
 * the character, the unit, a second weapon (-1: none) and a grenade type (-1: none) on the stack, ret 0x10.
 * os_actor_call(character, unit): function_1e0160 (0x1e0160) - the squad in eax (-1: none), the character in edx; the
 * unit and a flag (0) on the stack, ret 8; the actor (-1: none) in eax. os_actor_on_call(actor): function_1e11b0
 * (0x1e11b0) - the actor in eax, cl 1 (on); plain ret ---- */
	.globl	os_equip_call
os_equip_call:				/* (weapon, character, unit, second weapon) */
	pushal
	pushl	$-1
	pushl	52(%esp)
	pushl	52(%esp)
	pushl	52(%esp)
	movl	52(%esp), %eax
	call	orig_actor_equip
	popal
	ret
	.globl	os_actor_call
os_actor_call:				/* (squad, character, unit, flag) */
	pushal
	movl	40(%esp), %edx
	pushl	48(%esp)
	pushl	48(%esp)
	movl	44(%esp), %eax
	call	orig_actor_new
	movl	%eax, 28(%esp)
	popal
	ret
	.globl	os_actor_on_call
os_actor_on_call:
	pushal
	movl	36(%esp), %eax
	movb	$1, %cl
	call	orig_actor_on
	popal
	ret

/* ---- 6i.182: a spawned ally's own squad. os_order_call(squad, order): function_202e90 (v1.0 0x202e90) - the order's
 * number in bx; the squad and a flag (0) on the stack, ret 8. os_follow_call(order, squad): function_204450 (0x204450) -
 * the order's number in eax; the squad on the stack, ret 4 ---- */
	.globl	os_order_call
os_order_call:				/* (squad, order) */
	pushal
	movl	40(%esp), %ebx
	pushl	$0
	pushl	40(%esp)
	call	orig_squad_order
	popal
	ret
	.globl	os_follow_call
os_follow_call:				/* (order, squad) */
	pushal
	movl	36(%esp), %eax
	pushl	40(%esp)
	call	orig_squad_follow
	popal
	ret

/* ---- 6i.183: os_request_call(unit, request): function_e6900 (v1.0 0xe6900) - the unit in eax, the request's address
 * in edi; plain ret ---- */
	.globl	os_request_call
os_request_call:
	pushal
	movl	36(%esp), %eax
	movl	40(%esp), %edi
	call	orig_unit_request
	popal
	ret

/* ---- 6i.191: the geometry cache's block. Replaces `call 0x2d15da` at v1.0 0x12da80 - XPhysicalProtect(ebx the block
 * the game has just cut from its region, ecx its size, 4), stdcall. There: ebx the block, ecx its size, esi the cache's
 * size in pages, edi in bytes, ebp the address of the region's top for this stage (lowered by ecx just before).
 * cache_geo_take(block, size) returns the mod's own block, or 0. With a block: the region gets its bytes back, ebx / esi /
 * edi become the mod's block and its size, and the call's three arguments are taken off as the function would have (the
 * block is read-write as it comes: nothing to protect). With 0: on into the game's call exactly as it was made. ---- */
	.globl	geo_cache_hook
geo_cache_hook:
	pushal
	pushfl
	cld
	pushl	%ecx
	pushl	%ebx
	call	cache_geo_take
	addl	$8, %esp
	testl	%eax, %eax
	jz	1f
	movl	%eax, 20(%esp)			/* the caller's ebx: the mod's block */
	movl	cache_geo_pages, %eax
	movl	%eax, 8(%esp)			/* esi */
	movl	cache_geo_bytes, %eax
	movl	%eax, 4(%esp)			/* edi */
	movl	12(%esp), %eax			/* ebp: the region's top for this stage */
	movl	28(%esp), %edx			/* ecx: what the game cut from it */
	addl	%edx, (%eax)
	popfl
	popal
	ret	$12
1:	popfl
	popal
	jmp	orig_geo_protect

/* ---- 6i.193: a level's screen effect, drawn (function_2d140: the tag in eax; on the stack the weights, the grid's
 * columns, its rows, the grid; it takes its four arguments off itself - ret 0x10). Replaces its call at v1.0 0x13b52 (the
 * scene's closing pass; its other call, 0x22bbbb, is the HUD's and is hgrid_hook's). se_before(grid, columns, rows) returns the grid to draw with - the caller's, or the mod's scaled copy - and
 * may enlarge the camera's rectangle; the function is then called with its arguments copied (the grid replaced) and every
 * register as the caller had it; se_after puts the rectangle back; back to the caller with the arguments taken off. ---- */
	.globl	se_hook
se_hook:
	pushal
	pushfl
	cld
	pushl	48(%esp)			/* the rows ([esp+12] at entry) */
	pushl	48(%esp)			/* the columns ([esp+8]) */
	pushl	60(%esp)			/* the grid ([esp+16]) */
	call	se_before
	addl	$12, %esp
	movl	%eax, 52(%esp)			/* the grid to draw with, in the caller's own argument (it is thrown away on return) */
	popfl
	popal
	pushl	16(%esp)			/* the four arguments again, last first */
	pushl	16(%esp)
	pushl	16(%esp)
	pushl	16(%esp)
	call	orig_screen_effect
	pushal
	pushfl
	cld
	call	se_after
	popfl
	popal
	ret	$0x10

/* ---- 6i.194: the animation cache's block. Replaces `call 0x2d15da` at v1.0 0x1232b0 - XPhysicalProtect(esi the block
 * the game has just cut from its region, ecx its size, 4), stdcall. There: esi the block, ecx its size, edi the address of
 * the region's top for this stage. cache_anim_take(block, size) returns the mod's own block, or 0. With a block: the
 * region gets its bytes back, esi becomes the mod's block, the call's three arguments are taken off as the function would
 * have. With 0: on into the game's call exactly as it was made. ---- */
	.globl	anim_cache_hook
anim_cache_hook:
	pushal
	pushfl
	cld
	pushl	%ecx
	pushl	%esi
	call	cache_anim_take
	addl	$8, %esp
	testl	%eax, %eax
	jz	1f
	movl	%eax, 8(%esp)			/* the caller's esi: the mod's block */
	movl	4(%esp), %eax			/* edi: the region's top for this stage */
	movl	28(%esp), %edx			/* ecx: what the game cut from it */
	addl	%edx, (%eax)
	popfl
	popal
	ret	$12
1:	popfl
	popal
	jmp	orig_geo_protect

/* ---- 6i.198: the patchy fog's draw (function_25dc0: its two flags in cl and dl, nothing on the stack, plain ret). Its
 * two calls (v1.0 0x2c156, 0x2c1cb) already come through fx_b_hook (a timing mark, below); that now goes on to here
 * instead of straight into the game. fog_before() may put the pattern's scale right for the mod's larger picture; the
 * game's function runs with every register as the caller had it; fog_after() puts the scale back; every register as
 * the game's function left it. ---- */
	.globl	fog_hook
fog_hook:
	pushal
	pushfl
	cld
	call	fog_before
	popfl
	popal
	call	orig_fx_b
	pushal
	pushfl
	cld
	call	fog_after
	popfl
	popal
	ret

/* 6i.222: the loading code's SECOND "keep alive" swap (v1.0 0x163adb) - the one that goes out last, with the video
 * over, before the game stops swapping while the main menu loads. swap_tail_check() notes whether the video is over;
 * the game's routine is CALLED (it is __stdcall: it takes its two arguments off); then swap_settle() has the same
 * frame drawn and swapped again - see spooky.c. The game's answer and every register go back as the routine left them. */
	.globl sw_tail_hook
sw_tail_hook:
	pushal
	pushfl
	cld
	call	swap_tail_check			/* is the video over? */
	popfl
	popal
	pushl	8(%esp)				/* the second argument, then the first (now 8 up again) */
	pushl	8(%esp)
	call	orig_present			/* takes them off itself (ret 8) */
	pushfl
	pushal
	cld
	call	swap_settle
	popal
	popfl
	ret	$8

/* ---- 6i.48: the frame's call to the director (v1.0 0x12b647 -> 0x1554b0, v1.5 0x12e157 -> 0x157fe0: __stdcall, the
 * frame's seconds, ret 4) - the frame's blend made first, so the game builds its cameras from the in-between state it
 * draws; then on into the director exactly as called (its argument and return untouched) ---- */
	.globl	ip_dir_hook
ip_dir_hook:
	pushfl
	pushal
	cld
	call	ip_dir_before
	popal
	popfl
	jmp	orig_director

/* ---- 6i.50: Wireframe, Yelo's way - the frame setup's call to Direct3D's fill mode (v1.0 0x13404 -> 0x3f5ba0, v1.5
 * 0x133e4 -> 0x3ffc80: __stdcall, the mode, ret 4). wire_fill decides; wireframe: the mode argument made 0x1B01 (solid
 * is 0x1B02). Every register reaches the setter as the game left it; plain call / return (the 360's emulator). ---- */
	.globl	fill_hook
fill_hook:
	pushfl
	pushal
	cld
	call	wire_fill
	popal
	popfl
	cmpb	$0, spooky_fill_wire
	je	1f
	movl	$0x1B01, 4(%esp)
1:	jmp	orig_fillmode

	.globl	ip_draw_hook
ip_draw_hook:
	pushfl
	pushal
	cld
	call	ip_draw_before
	popal
	popfl
	call	orig_frame_draw
	pushfl
	pushal
	cld
	call	ip_draw_after
	popal
	popfl
	ret

/* ---- 6i.3: the frame-time line's two waits, timed where the game's draw (v1.0 0x223240) makes them: its own wait for
 * the frame's vblank (v1.0 0x223288 -> 0x12b2e0, void(void)) and the present (0x223296 -> 0x13f10, __stdcall with two
 * arguments, ret 8: they're passed on and taken off here as the original did). Every register and the flags reach the
 * game's function as they came, and its results come back; plain call / return (the 360's emulator). ---- */
	.globl	prof_wait_hook
prof_wait_hook:
	pushfl
	pushal
	cld
	call	prof_wait_before
	popal
	popfl
	call	orig_vblank_wait
	pushfl
	pushal
	cld
	call	prof_wait_after
	popal
	popfl
	ret

	.globl	prof_present_hook
prof_present_hook:
	pushfl
	pushal
	cld
	call	prof_present_before
	popal
	popfl
	pushl	8(%esp)                 /* the second argument, then the first (now 8 up again) */
	pushl	8(%esp)
	call	orig_present            /* takes them off itself (ret 8) */
	pushfl
	pushal
	cld
	call	prof_present_after
	popal
	popfl
	ret	$8


/* ---- 6i.23: the Direct3D probes (6i.6 - 6i.22: the "Direct3D probes" XBEs' detours) retired: per-pass GPU timing and
 * the captures answer what they did, on both consoles. ---- */

/* ---- 6i.12: the render passes - function_44550's call of function_3bc30 (the pass's records drawn: one argument, the
 * group, ret 4; edi the sources, ebx the mode - kept by it, so still the mode after). Skip render pass (test): the pass
 * it names isn't drawn. Otherwise timed into spooky_pass_cyc[mode]. The flags are kept round the sum. ---- */
	.globl	pass_hook
pass_hook:
	cmpl	spooky_skip_pass, %ebx
	je	2f
	cmpl	$31, %ebx                       /* 6i.75: Object Shadows / Smoke Pass (test) - a pass in spooky_pass_mask skipped */
	ja	3f
	btl	%ebx, spooky_pass_mask
	jc	2f
3:
	pushfl
	pushal
	cld
	pushl	%ebx
	call	pass_in                         /* 6i.17: timed in C (and, GPU time by pass on, the GPU drained) */
	addl	$4, %esp
	popal
	popfl
	pushl	4(%esp)
	call	orig_pass_draw
	pushfl
	pushal
	cld
	pushl	%ebx
	call	pass_out
	addl	$4, %esp
	popal
	popfl
2:	ret	$4

/* ---- 6i.13: the two screen-wide effects the main scene draws around pass 16 (function_2bcd0): function_25d50 (chosen by
 * the game's screen effect mode, g_48574c; before pass 16) and function_25dc0 (its two flags in cl / dl). Registers and
 * flags reach them as they came; timed into spooky_pass_cyc[25] / [26]; Skip render pass (test) 25 / 26 skips them. ---- */
.macro FX_HOOK name, k, orig, mk               /* 6i.22: now a mark only (their own timing and skip retired: effect mode 0 throughout) */
	.globl	\name
\name:
	pushfl
	pushal
	cld
	pushl	$\mk
	call	mark
	addl	$4, %esp
	popal
	popfl
	jmp	\orig
.endm
	FX_HOOK	fx_a_hook, 25, orig_fx_a, 58
	.globl	fx_b_hook			/* 6i.198: the mark as before, then through fog_hook (above) into the game's function */
fx_b_hook:
	pushfl
	pushal
	cld
	pushl	$59
	call	mark
	addl	$4, %esp
	popal
	popfl
	jmp	fog_hook


/* ---- 6i.17: the game's KickPushBuffer (v1.0 0x3fac30 - ADDR_DV_ISBUSY, named in 6i.6 as the busy check: it's the kick,
 * the game testing busy itself after it), as the game calls it - the device in edx - from C: gpu_kick(device). ---- */
	.globl	gpu_kick
gpu_kick:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	20(%esp), %edx
	call	orig_dv_isbusy
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/* ---- 6i.18: marks through the main scene (function_2bcd0) - each of its calls between and after the passes enters here
 * first: mark(id) (with GPU time by pass on: the GPU drained, the time since the last mark put to that mark's part), then
 * on into the game's own as called (a jump: its arguments, registers and return untouched). ---- */
.macro MARK_HOOK id, orig
	.globl	mark_hook_\id
mark_hook_\id:
	pushfl
	pushal
	cld
	pushl	$\id
	call	mark
	addl	$4, %esp
	popal
	popfl
	jmp	\orig
.endm
/* 6i.20: four of them skippable (Skip render pass (test) = the mark's number): the call returns at once, its arguments
 * taken off as the game's own would (ret 8 / ret) - 41 the sky model, 43 the transparents drawn (3c3c0), 49 the
 * interface (1e370), 56 the scene's finishing effects (13870) */
.macro MARK_SKIP id, orig, pop
	.globl	mark_hook_\id
mark_hook_\id:
	pushfl
	pushal
	cld
	pushl	$\id
	call	mark
	addl	$4, %esp
	popal
	popfl
	cmpl	$\id, spooky_skip_pass
	jne	\orig
	ret	$\pop
.endm
	MARK_SKIP 41, orig_mk_41, 0		/* sky model */
	MARK_SKIP 43, orig_mk_43, 8		/* 3c3c0 */
	MARK_SKIP 49, orig_mk_49, 0		/* 1e370 (interface) */
	MARK_SKIP 56, orig_mk_56, 8		/* 13870 */
	MARK_HOOK 57, orig_mk_57		/* 13630 (end) */
/* 6i.28: the water's refraction copy (function_352e0(target, multiple); ret 8). Skip render pass 60: never made (the test);
 * spooky_copy_keep: this frame keeps the last one; else made (spooky_copy_ran set) - with spooky_copy_one, its second
 * argument 0: one sample a pixel instead of four. */
	.globl	mark_hook_60
mark_hook_60:
	pushfl
	pushal
	cld
	pushl	$60
	call	mark
	addl	$4, %esp
	popal
	popfl
	cmpl	$60, spooky_skip_pass
	je	1f
	cmpb	$0, spooky_copy_keep
	jne	1f
	cmpb	$0, spooky_copy_late            /* 6i.59: late - not here (before the water plane): after it */
	je	3f
	cmpb	$0, spooky_in_late
	jne	3f
	movb	$1, spooky_copy_due
	jmp	1f
3:	movb	$1, spooky_copy_ran
	cmpb	$0, spooky_copy_one
	je	2f
	movl	$0, 8(%esp)
2:	jmp	orig_mk_60
1:	ret	$8
	MARK_HOOK 61, orig_mk_61		/* 6i.21: after it - the water drawn (3c3c0 on), to the next mark */

/* ---- 6i.23: each record a render pass draws - function_3bc30's loop: call [eax+4] (the record's draw: seven arguments,
 * its own ret 0x1c) then mov eax, [esp+8]; those seven bytes replaced by call [spooky_recp] + nop (v1.0 0x3bc8d, v1.5
 * 0x3bc7d), returning to the nop. Here the seven arguments are copied on and the draw called as the game would, then the
 * moved instruction done (the loop's [esp+8]: ours +40 - the return and the seven originals between) and the originals
 * taken off. spooky_rec_on 0: nothing else. On: rec_in(record, mode) before - spooky_rec_skip 1: not drawn (Skip shader) -
 * and rec_out after (a capture's per-shader times). eax (the record's source) is kept through it all. ---- */
	.globl	rec_hook
rec_hook:
	cmpb	$0, spooky_rec_on
	jne	2f
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	call	*4(%eax)
	movl	40(%esp), %eax
	ret	$28
2:	pushfl
	pushal
	cld
	pushl	%ebx
	pushl	%esi
	call	rec_in
	addl	$8, %esp
	popal
	popfl
	cmpb	$0, spooky_rec_skip
	jne	3f
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	pushl	28(%esp)
	call	*4(%eax)
	pushfl
	pushal
	cld
	call	rec_out
	popal
	popfl
3:	movl	40(%esp), %eax
	ret	$28

/* ---- 6i.25: a material's sampler settings - function_189a0 (edx the shader state, its one argument the range, ret 4),
 * called as the game would; then, No anisotropic filtering on, aniso_clamp(range) puts the stages it set to linear. ---- */
	.globl	aniso_hook
aniso_hook:
	pushl	4(%esp)
	call	orig_aniso_set
	cmpb	$0, spooky_aniso
	jne	2f
	cmpb	$0, spooky_light_loop           /* 6i.86: Light Filtering - a shadow or the light loop running */
	jne	2f
	cmpb	$0, spooky_in_shadow
	je	1f
2:	pushfl
	pushal
	cld
	pushl	40(%esp)
	call	aniso_clamp
	addl	$4, %esp
	popal
	popfl
1:	ret	$4

/* 6i.35: particle systems kept through frames with no tick (Unlock FPS). Particles are spawned on ticks (the effects
 * update, function_176e40, runs in the game tick), but each particle system's update runs every frame, and deletes a
 * system that neither has particles nor spawned since the previous frame (function_174a30: flag9 = flag8, flag8 cleared,
 * then kept only if particles, children or flag9). At 30 every frame has a tick; at 60 every other one doesn't - so a system
 * still building up to its first particle (a slow or far emitter: less than one particle a tick) was deleted on the frame
 * after each tick, its build-up with it, and never showed (the Midship lifts). On a frame with no tick since the last
 * (spooky_ptc_hold, from spooky.c), a system that spawned at that tick (flag9, word +0xc bit 9 = byte +0xd bit 1) counts as
 * having spawned again (flag8, byte +0xd bit 0) - the game's own rule then applies at the next tick's frame. */
	.globl	ptc_update_hook
ptc_update_hook:
	cmpb	$0, spooky_ptc_hold
	je	1f
	testb	$2, 0xd(%ecx)
	je	1f
	orb	$1, 0xd(%ecx)
	incl	spooky_ptc_kept
1:	jmp	orig_ptc_update

/* 6i.40: the split-screen detail settings (function_2c490, the local player count in eax) - the game's own first, then
 * the Performance Mode Settings' levers for one view (spooky.c coop_apply). Called from the view build (function_223080
 * at v1.0 0x22318e / v1.5 0x229b9e); the caller's registers as the game's function left them */
	.globl	coop_hook
coop_hook:
	call	orig_coop_detail
	pushfl
	pushal
	cld
	call	coop_apply
	popal
	popfl
	ret

/* 6i.42: triple buffering - with spooky_triple set (the triple_buffering XBEs: patch_xbe.py --triple), the device asks
 * for 2 back buffers with the discard swap effect (Halo 2 HD's): BackBufferCount (+0x0C) = 2, SwapEffect (+0x14) = 1 in
 * the present parameters, just before Direct3D reads them. The memory for the third buffer: the game's runtime data region
 * 1.25 MB smaller at startup (patch_xbe.py). Registers as the game left them for Direct3D. */
	.globl	tb_create_hook
tb_create_hook:                     /* CreateDevice: the present parameters its one stack argument (eax, ecx: its others) */
	pushal                              /* 6i.135: the setting read first (T:\spooky_triple.txt) - every register as it was */
	pushfl
	cld
	call	tb_load
	popfl
	popal
	cmpb	$0, spooky_triple
	je	1f
	pushl	%edx
	movl	8(%esp), %edx
	movl	$2, 0x0c(%edx)
	movl	$1, 0x14(%edx)
	popl	%edx
1:	jmp	orig_d3d_create

	.globl	tb_reset_hook
tb_reset_hook:                      /* Reset: the present parameters in ebx */
	cmpb	$0, spooky_triple
	je	1f
	movl	$2, 0x0c(%ebx)
	movl	$1, 0x14(%ebx)
1:	jmp	orig_d3d_reset

/* ---- 6i.90: Direct3D's visibility tests, called as the game calls them (the GPU's zpass count and clock) ---- */
	.text
	.globl	vt_space
vt_space:                                     /* u32 vt_space(u32 size): MakeSpace - eax the size, its half on the stack; ret 4 */
	movl	4(%esp), %eax
	movl	%eax, %ecx
	shrl	$1, %ecx
	pushl	%ecx
	call	orig_dv_makespace
	ret
	.globl	vt_end
vt_end:                                       /* u32 vt_end(u32 index): EndVisibilityTest - eax the test; ret */
	movl	4(%esp), %eax
	call	orig_vt_end
	ret
	.globl	vt_get
vt_get:                                       /* u32 vt_get(u32 index, u32 *pixels, u32 *time): GetVisibilityTestResult -
                                                 eax the test, edx the time (8 bytes), the pixels' address pushed; ret 4 */
	movl	4(%esp), %eax
	movl	12(%esp), %edx
	pushl	8(%esp)
	call	orig_dv_vistest
	ret
/* ---- 6i.92: render target switches - function_1cd30's two Direct3D calls (v1.0 0x1cd63 -> 0x3f7b30: one argument,
   ret 4; 0x1cd6c -> 0x3f7870: two, ret 8; v1.5 0x1cd43 / 0x1cd4c -> 0x401c10 / 0x401950). rt_switch first (the capture's
   part 27 starts there), eax / ecx / edx as the game had them, then a tail jump: the game's own arguments and return ---- */
	.text
	.data
	.globl	spooky_rt_on
spooky_rt_on:	.byte	0
	.text
	.globl	rt_hook1
rt_hook1:
	cmpb	$0, spooky_rt_on                /* 6i.109: only when measuring - else straight on */
	je	1f
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	16(%esp)                        /* 6i.106: the target (the game's first argument) */
	call	rt_switch
	addl	$4, %esp
	popl	%edx
	popl	%ecx
	popl	%eax
1:	jmp	orig_rt_fast
	.globl	rt_hook2
rt_hook2:
	cmpb	$0, spooky_rt_on                /* 6i.109: only when measuring - else straight on */
	je	1f
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	16(%esp)                        /* 6i.106: the target (the game's first argument) */
	call	rt_switch
	addl	$4, %esp
	popl	%edx
	popl	%ecx
	popl	%eax
1:	jmp	orig_rt_set
/* ---- 6i.55: Wireframe on - the game's calls of Direct3D's DrawIndexedVertices (8) and DrawVertices (3): stdcall, three
 * arguments (the primitive type first). Drawn as the game asked; then, with spooky_wire_on (the world's frame) and a
 * polygon type (5 and up: triangles, strips, fans, quads), the same draw again between wire_line_begin and
 * wire_line_end - as white lines (spooky.c). ---- */
	.globl	drawidx_hook
drawidx_hook:
	pushl	%eax                            /* 6i.98: the draw's count (its second argument) for the capture */
	movl	12(%esp), %eax
	movl	%eax, spooky_draw_n
	movl	16(%esp), %eax                  /* 6i.110: and its index data (the third) - which surface it is */
	movl	%eax, spooky_draw_ip
	popl	%eax
	cmpb	$0, spooky_scr_point            /* 6i.83: Screen Reads fast - full-screen linear textures point-sampled */
	je	5f
	pushfl
	pushal
	cld
	call	scr_pre
	popal
	popfl
5:	cmpb	$0, spooky_skip_draw            /* 6i.111: a light draw of index data the camera never read - not drawn */
	jne	7f
	pushl	12(%esp)
	pushl	12(%esp)
	pushl	12(%esp)
	call	orig_dv_drawidx
	cmpb	$0, spooky_scr_set
	je	6f
	pushfl
	pushal
	cld
	call	scr_post
	popal
	popfl
6:	cmpb	$0, spooky_wire_on
	je	1f
	cmpl	$5, 4(%esp)
	jb	1f
	pushfl
	pushal
	cld
	call	wire_line_begin
	popal
	popfl
	pushl	12(%esp)
	pushl	12(%esp)
	pushl	12(%esp)
	call	orig_dv_drawidx
	pushfl
	pushal
	cld
	call	wire_line_end
	popal
	popfl
1:	ret	$12
7:	movb	$0, spooky_skip_draw            /* 6i.111: skipped - its texture states still put back */
	cmpb	$0, spooky_scr_set
	je	1b
	pushfl
	pushal
	cld
	call	scr_post
	popal
	popfl
	jmp	1b

	.data
	.globl	spooky_draw_n
spooky_draw_n:	.long	0                       /* 6i.98: the last draw's count */
	.globl	spooky_draw_ip
spooky_draw_ip:	.long	0                      /* 6i.110: the last indexed draw's index data */
	.globl	spooky_skip_draw
spooky_skip_draw:	.byte	0                     /* 6i.111: scr_pre's verdict - skip this draw */
	.text
	.globl	drawv_hook
/* 6i.56: DrawVertices isn't stdcall-3 as DrawIndexedVertices is - the vertex count in eax, the first vertex in ecx, only
 * the primitive type on the stack (ret 4). 6i.55 passed it three stack words and popped 12: the stack ran off - the crash
 * at every level's start. Both registers kept here for the second call. */
drawv_hook:
	movl	%eax, spooky_draw_n             /* 6i.98: its vertex count is in eax */
	movl	$0, spooky_draw_ip               /* 6i.110: no index data */
	cmpb	$0, spooky_scr_point            /* 6i.83 */
	je	5f
	pushfl
	pushal
	cld
	call	scr_pre
	popal
	popfl
5:	pushl	%eax
	pushl	%ecx
	pushl	12(%esp)                        /* the type */
	call	orig_dv_draw                    /* eax, ecx as the game set them */
	cmpb	$0, spooky_scr_set
	je	6f
	pushfl
	pushal
	cld
	call	scr_post
	popal
	popfl
6:	cmpb	$0, spooky_wire_on
	je	2f
	cmpl	$5, 12(%esp)
	jb	2f
	pushfl
	pushal
	cld
	call	wire_line_begin
	popal
	popfl
	movl	4(%esp), %eax
	movl	(%esp), %ecx
	pushl	12(%esp)
	call	orig_dv_draw
	pushfl
	pushal
	cld
	call	wire_line_end
	popal
	popfl
2:	addl	$8, %esp
	ret	$4

/* ---- 6i.58: Water Surface (test) - the level's water plane's one draw (function_3c3c0's call of the game's indexed-draw
 * wrapper function_1c8a0 - stdcall, three arguments): with spooky_water_hide, not drawn. ---- */
	.globl	water_draw_hook
water_draw_hook:
	cmpb	$0, spooky_water_hide
	jne	3f
	pushl	12(%esp)
	pushl	12(%esp)
	pushl	12(%esp)
	call	orig_water_draw                 /* the game's own, its three arguments */
	cmpb	$0, spooky_copy_due             /* 6i.59: Water Refraction late - the copy now, after the plane */
	je	2f
	pushfl
	pushal
	cld
	call	water_late_copy
	popal
	popfl
2:	movb	$1, %al                         /* as the wrapper returns (the caller doesn't read it) */
	ret	$12
3:	movb	$0, spooky_copy_due
	movb	$1, %al
	ret	$12

/* 6i.59: the game's copy wrapper (function_3bea0: no arguments, keeps esi itself) - for water_late_copy */
	.globl	water_copy_call
water_copy_call:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	call	orig_water_copy
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/* 6i.59: Water Refraction fast - function_14bc0's case for target 22 (the refraction copy's texture), reached through its
 * jump table entry (v1.0 0x14f0c, v1.5 0x14eec - here instead of 0x14d1e / 0x14cfe): as the game's (mov eax, 0x16; call
 * the target's surface; edi = it; esi = 0, no depth), but with spooky_copy_keepz esi = the screen's depth surface (target
 * 0's: [0x509364] / [0x515e44]) - the copy draws without depth, and the depth buffer never comes unbound. Then the
 * game's common path, as the case would. */
	.globl	copy_target_stub
copy_target_stub:
	movl	$0x16, %eax
	call	orig_target_surface
	xorl	%esi, %esi
	movl	%eax, %edi
	cmpb	$0, spooky_copy_keepz
	je	1f
	movl	orig_main_depth, %esi
1:	jmp	orig_target_common

/* 6i.60: Distortion Effects fast - the same for target 19 (the screen copy the see-through effects read: function_352e0
 * with no target), its case's jump table entry v1.0 0x14f00 / v1.5 0x14ee0 (was 0x14ca9 / 0x14c89: mov eax, 0x13 and the
 * shared surface call - no depth). With spooky_fx_keepz, the screen's depth stays bound. */
	.globl	copy_target19_stub
copy_target19_stub:
	movl	$0x13, %eax
	call	orig_target_surface
	xorl	%esi, %esi
	movl	%eax, %edi
	cmpb	$0, spooky_fx_keepz
	je	1f
	movl	orig_main_depth, %esi
1:	jmp	orig_target_common

/* ---- 6i.61: Render Scale - the world view's draw (function_2b790, at the views loop's call: v1.0 0x2b6b4, v1.5 0x2b694):
 * rs_before(view) cuts its rectangle, then the game's own (tail jump: its arguments and return untouched) ---- */
	.globl	rs_view_hook
rs_view_hook:
	pushal
	pushfl
	cld
	pushl	40(%esp)                        /* the view: the call's first argument, above pushal, pushfl and the return */
	call	rs_before
	addl	$4, %esp
	popfl
	popal
	jmp	orig_world_view
/* rs_copy(target, multiple): the game's screen copy, function_352e0 - stdcall, two arguments */
	.globl	rs_copy
rs_copy:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushl	24(%esp)                        /* multiple */
	pushl	24(%esp)                        /* target */
	call	orig_mk_60
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret
/* rs_target(target): the game's render target switch back, function_14bc0 - as its copy's caller does it (push 0, push
 * the target, al 1; ret 8) */
	.globl	rs_target
rs_target:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushl	$0
	pushl	24(%esp)                        /* the target */
	movb	$1, %al
	call	orig_mk_61
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/* ---- 6i.63: Render Scale - the main scene's start (function_2bcd0, stdcall, its four calls in the frame's draw:
 * v1.0 0x136ac 0x1373b 0x1375a 0x13788, v1.5 0x1368c 0x1371b 0x1373a 0x13768): rs_cut, then the game's own (tail jump) */
	.globl	rs_scene_hook
rs_scene_hook:
	pushal
	pushfl
	cld
	call	rs_cut
	popfl
	popal
	jmp	orig_scene

/* ---- 6i.65: Render Scale - the game's three calls of Direct3D's SetViewport (stdcall, one argument: the viewport):
 * vp_filter may hand back a cut copy in its place; then the game's own (tail jump) ---- */
	.globl	vp_hook
vp_hook:
	pushal
	pushfl
	cld
	pushl	40(%esp)
	call	vp_filter
	addl	$4, %esp
	movl	%eax, 40(%esp)                  /* the argument: the game's viewport or the cut copy */
	popfl
	popal
	jmp	orig_set_viewport

/* ---- 6i.66: Render Scale - the world view's projection (function_2fd90: esi the view, ebx the result, edx the clip
 * box or 0), at function_2ba10's two calls: proj_clip may hand back a clip box (in edx), then the game's own ---- */
	.globl	proj_hook
proj_hook:
	pushal
	pushfl
	cld
	pushl	%edx                            /* the game's clip box */
	pushl	%esi                            /* the view */
	call	proj_clip
	addl	$8, %esp
	movl	%eax, 24(%esp)                  /* the saved edx (above pushfl: edi esi ebp esp ebx edx) */
	popfl
	popal
	jmp	orig_proj

/* ---- 6i.68: rs_view_consts(camera, projection, mode): the game's screen-mapping constants, function_12fa0, called the
 * way the game calls it (ecx the camera, eax the projection, then 0 - not scaled - and the mode pushed; ret 8) ---- */
	.globl	rs_view_consts
rs_view_consts:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	28(%esp), %edx                  /* the mode */
	pushl	%edx
	pushl	$0                              /* not scaled */
	movl	32(%esp), %eax                  /* the projection */
	movl	28(%esp), %ecx                  /* the camera */
	call	orig_view_consts
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/* ---- 6i.69: Render Scale's stretch point - the main scene's call of function_40870 (its second interface step, after
 * the see-through work; no arguments): rs_ui_point, then the game's own (tail jump) ---- */
	.globl	rs_ui_hook
rs_ui_hook:
	pushal
	pushfl
	cld
	call	rs_ui_point
	popfl
	popal
	jmp	orig_ui2

/* ---- 6i.71: Water Refraction fast without touching the jump table. Your 360 bisect: 6i.58 loads, 6i.59 crashes at
 * level load even with Performance Mode off - 6i.59 pointed function_14bc0's jump table entry for target 22 at
 * copy_target_stub, in this section; the 360's emulator doesn't survive that. The table is the game's again. Instead,
 * the call every one of those cases makes to the target surface getter (v1.0 0x14d2a -> 0x1dcf0, v1.5 0x14d0a ->
 * 0x1dcd0; eax the target) comes here, a plain call like every other hook: anything but the water's copy with fast on
 * goes straight on to the game's own; the water's copy with fast on is finished here - the screen's depth kept in
 * esi - and carries on at the shared path past the game's xor esi, esi (0x14d33 / 0x14d13). ---- */
	.globl	target_surf_hook
target_surf_hook:
	cmpl	$0x16, %eax                     /* the water's refraction copy (target 22) */
	jne	1f
	cmpb	$0, spooky_copy_keepz
	jne	2f
1:	jmp	orig_target_surface             /* the game's own: back to its xor esi, esi */
2:	addl	$4, %esp                        /* the game's call's return - our own way on instead */
	call	orig_target_surface
	movl	orig_main_depth, %esi           /* the screen's depth kept */
	movl	%eax, %edi
	jmp	orig_target_common

/* ---- 6i.71: Stretch Point (test) 2 - the scene's call of function_20770, right after the second interface step
 * (v1.0 0x2c317, v1.5 0x2c2f7; no arguments): rs_after_ui_point, then the game's own ---- */
	.globl	rs_after_ui_hook
rs_after_ui_hook:
	pushal
	pushfl
	cld
	call	rs_after_ui_point
	popfl
	popal
	jmp	orig_after_ui

/* ---- 6i.73: Particles - function_224550's call of the sprite emitter (function_23ed30: eight stack arguments, ret
 * 0x20, and ebx): sprite_before (the alpha test on), the emitter called with the game's arguments copied (every
 * register as the game left it), sprite_after (put back), then back to the game with its arguments taken off ---- */
	.globl	sprite_hook
sprite_hook:
	pushal
	pushfl
	cld
	call	sprite_before
	popfl
	popal
	pushl	32(%esp)                        /* the eight arguments again, last first: each push moves the next to 32 */
	pushl	32(%esp)
	pushl	32(%esp)
	pushl	32(%esp)
	pushl	32(%esp)
	pushl	32(%esp)
	pushl	32(%esp)
	pushl	32(%esp)
	call	orig_sprite
	pushal
	pushfl
	cld
	call	sprite_after
	popfl
	popal
	ret	$0x20
/* d3d_rs_simple(method, value): Direct3D's simple render-state setter, the way function_142f0 calls it */
	.globl	d3d_rs_simple
d3d_rs_simple:
	movl	4(%esp), %ecx
	movl	8(%esp), %edx
	jmp	orig_rs_simple

/* ---- 6i.77: Dynamic Lights - the light loop's call of function_31170 (v1.0 0x31f26, v1.5 0x31f16): eax the tag, ecx
 * the shape, then four on the stack (the light's index, its source = datum + 0x84, a flag, a scale), ret 0x10.
 * light_begin(source) before, the game's own called with its arguments copied (every register as the game left them),
 * light_end after ---- */
	.globl	light_hook
light_hook:
	pushal
	pushfl
	cld
	pushl	28(%esp)                        /* 6i.94: ecx as the game set it - the light's view (s_view_source) - the third argument */
	pushl	36(%esp)                        /* 6i.93: eax - the light's tag - the second */
	pushl	52(%esp)                        /* the source: the second stack argument, above pushal, pushfl, the return, ecx and eax */
	call	light_begin
	addl	$12, %esp
	popfl
	popal
	pushl	16(%esp)                        /* the four arguments again, last first */
	pushl	16(%esp)
	pushl	16(%esp)
	pushl	16(%esp)
	call	orig_light
	pushal
	pushfl
	cld
	call	light_end
	popfl
	popal
	ret	$0x10
/* d3d_set_viewport(viewport): Direct3D's SetViewport (stdcall, one argument) - the game's own, not through vp_hook */
	.globl	d3d_set_viewport
d3d_set_viewport:
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushl	20(%esp)
	call	orig_set_viewport
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/* ---- 6i.81: Lights & Shadows - an object's shadow (function_32350: stdcall, the object and an alpha; ret 8) marked
 * while it runs, at its one call (v1.0 0x33647, v1.5 0x33637) ---- */
	.globl	shadow_hook
shadow_hook:
	movb	$1, spooky_in_shadow
	pushl	8(%esp)                         /* the two arguments again */
	pushl	8(%esp)
	call	orig_shadow
	movb	$0, spooky_in_shadow
	ret	$8

/* ---- 6i.86: Light Filtering - the main scene's call of the light loop (function_31c80; v1.0 0x2c03e, v1.5 0x2c01e):
 * marked for the texture setup hook (aniso_clamp), then the game's own; the mark is cleared at the sky's mark ---- */
	.globl	lightloop_hook
lightloop_hook:
	movb	$1, spooky_light_loop
	jmp	orig_lightloop

/* ---- 6i.147: the water plane's sizing (function_3ca90, eax the water's state), at function_3c3c0's call (v1.0 0x3c401):
 * the game's own, then water_fit widens the mesh to the view (Halo 2's widescreen bug); every register as the game's left */
	.globl	water_fit_hook
water_fit_hook:
	pushl	%eax                            /* the state */
	call	orig_water_size
	pushal
	pushfl
	cld
	pushl	36(%esp)                        /* the state: past pushfl (4) and pushal (32) */
	call	water_fit
	addl	$4, %esp
	popfl
	popal
	addl	$4, %esp                        /* the saved state dropped, the flags untouched */
	ret

/* ---- 6i.148: Render Scale above 100 % - the target size getter (function_01dd60: eax the target, edx &width, ecx &height,
 * al the result) at its seven calls (v1.0 0x14f7b 0x15f16 0x1961c 0x19a5c 0x2d984 0x34abd 0x34dbe): while the scene is
 * drawn into the bigger targets, rt_size answers for the screen-sized ones (0, 3, 24 and the light accumulation target 1)
 * with the bigger size; otherwise the game's own. Every register as the game's left but the result ---- */
	.globl	rt_size_hook
rt_size_hook:
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	pushl	%ecx                            /* &height */
	pushl	%edx                            /* &width */
	pushl	%eax                            /* the target */
	call	rt_size
	addl	$12, %esp
	testl	%eax, %eax
	popl	%eax
	popl	%edx
	popl	%ecx
	jz	orig_rt_size                    /* not ours: the game's own getter */
	movb	$1, %al
	ret
