# Spooky Ghost's Xbox 360 emulator, release R89: the code added to xefu1_1's free space - the fixes only.
# R88L's file (stubs_xefu1_1_r86l.S, 1,404 lines) held these fixes together with the render-target event logger that
# found them: a 512-entry ring, a table of surfaces seen, counters, and wrappers on 35 of the emulator's own calls. The
# logger is gone from this file. What is here is, routine for routine, R88L's code with its counters and its calls to
# the logger taken out and nothing else changed - test_r89.py runs each routine in both builds on the same machine
# states and requires the same results.
#   bm<k>_r<X>     R70  the four page bitmaps at their 128 MB size and place (31 sites)
#   w_depth_match  R78  the depth surface never takes a colour-format record over its memory for its own (0x820C5E44)
#   w_promote      R79  a view remade as a plain texture is promoted at its bind whatever the depth's state (0x820C655C)
#   w_slot         R67 / R85 / R86  a pending clear follows the depth slot; a colour clear whose record is replaced by one
#                  of another size is held (0x820C3F30)
#   w_take         R85 / R86  ... and given at its bind to the first wider record that may have it (0x820C62EC)
# HOLD is the held clear's 32 bytes (R88L kept them at its counters' block + 0x70; the address is unchanged):
#   +0 the bits, +4 the frame, +8 the width of the record it was on, +0xC the record that took the slot, +0x10 the colour (16 B)

        .section .text

# R70: the x86 glue's four page bitmaps (which pages the GPU thread watches / which have records), 128 bytes = 1024 x 64 KB
# = 64 MB each at GPUG+0x1988/0x1A08/0x1A88/0x1B08, relocated to BITMAPS (0xD7FF9000) at 256 bytes = 2048 pages = 128 MB
# each. Every 'addi rX,rBASE,0x19xx' that formed a bitmap address is now 'bl bm<k>_r<X>' (LR is saved in every function
# concerned); the fills, zeroings and the page loop at 0x820C44BC are widened by word patches in make_release_r70.py.
        .globl bm0_r3
bm0_r3:
        lis     %r3,BITMAPS@ha
        addi    %r3,%r3,BITMAPS@l+0x0
        blr
        .globl bm0_r7
bm0_r7:
        lis     %r7,BITMAPS@ha
        addi    %r7,%r7,BITMAPS@l+0x0
        blr
        .globl bm0_r9
bm0_r9:
        lis     %r9,BITMAPS@ha
        addi    %r9,%r9,BITMAPS@l+0x0
        blr
        .globl bm0_r11
bm0_r11:
        lis     %r11,BITMAPS@ha
        addi    %r11,%r11,BITMAPS@l+0x0
        blr
        .globl bm0_r30
bm0_r30:
        lis     %r30,BITMAPS@ha
        addi    %r30,%r30,BITMAPS@l+0x0
        blr
        .globl bm1_r3
bm1_r3:
        lis     %r3,BITMAPS@ha
        addi    %r3,%r3,BITMAPS@l+0x100
        blr
        .globl bm1_r4
bm1_r4:
        lis     %r4,BITMAPS@ha
        addi    %r4,%r4,BITMAPS@l+0x100
        blr
        .globl bm1_r7
bm1_r7:
        lis     %r7,BITMAPS@ha
        addi    %r7,%r7,BITMAPS@l+0x100
        blr
        .globl bm1_r11
bm1_r11:
        lis     %r11,BITMAPS@ha
        addi    %r11,%r11,BITMAPS@l+0x100
        blr
        .globl bm1_r30
bm1_r30:
        lis     %r30,BITMAPS@ha
        addi    %r30,%r30,BITMAPS@l+0x100
        blr
        .globl bm2_r5
bm2_r5:
        lis     %r5,BITMAPS@ha
        addi    %r5,%r5,BITMAPS@l+0x200
        blr
        .globl bm2_r6
bm2_r6:
        lis     %r6,BITMAPS@ha
        addi    %r6,%r6,BITMAPS@l+0x200
        blr
        .globl bm2_r8
bm2_r8:
        lis     %r8,BITMAPS@ha
        addi    %r8,%r8,BITMAPS@l+0x200
        blr
        .globl bm2_r11
bm2_r11:
        lis     %r11,BITMAPS@ha
        addi    %r11,%r11,BITMAPS@l+0x200
        blr
        .globl bm2_r30
bm2_r30:
        lis     %r30,BITMAPS@ha
        addi    %r30,%r30,BITMAPS@l+0x200
        blr
        .globl bm3_r3
bm3_r3:
        lis     %r3,BITMAPS@ha
        addi    %r3,%r3,BITMAPS@l+0x300
        blr
        .globl bm3_r6
bm3_r6:
        lis     %r6,BITMAPS@ha
        addi    %r6,%r6,BITMAPS@l+0x300
        blr
        .globl bm3_r9
bm3_r9:
        lis     %r9,BITMAPS@ha
        addi    %r9,%r9,BITMAPS@l+0x300
        blr
        .globl bm3_r11
bm3_r11:
        lis     %r11,BITMAPS@ha
        addi    %r11,%r11,BITMAPS@l+0x300
        blr
        .globl bm3_r29
bm3_r29:
        lis     %r29,BITMAPS@ha
        addi    %r29,%r29,BITMAPS@l+0x300
        blr
        .globl bm3_r30
bm3_r30:
        lis     %r30,BITMAPS@ha
        addi    %r30,%r30,BITMAPS@l+0x300
        blr

# R78: the depth surface's record taken from the wrong record. The render-target setup builds ONE lookup descriptor, with
# the COLOUR target's format, and uses it for the depth surface too with only the pitch word replaced. So its search for
# the depth surface's record can only ever match a record with a colour format over the depth buffer's memory - and the
# game has exactly one: the view through which it samples the depth buffer as a texture. When the search runs with that
# view alive, the view becomes the depth target, and what the game samples as "the depth" is never brought up to date.
# 0x820C5E44: beq cr6,0x820C5E64 (the descriptor matched: record found) becomes bl here. cr6 is that compare; r31 the
# record. Not a match: back (the next record, 0x820C5E48). A match on a record whose format is a depth format (0x2D..):
# found, as before. A match on anything else - a colour-format record: the view - is refused: the search goes on, finds
# nothing, and the setup makes the depth surface a record of its own. r3 is free there.
        .globl w_depth_match
w_depth_match:
        bnelr   %cr6
        lwz     %r3,0x38(%r31)
        srwi    %r3,%r3,24
        cmplwi  %cr6,%r3,0x2D
        bnelr   %cr6
        b       DEPTHFOUND

# R79L: the view that stays a plain texture. When the view (the colour-format texture the game lays over the depth
# buffer's memory) has been destroyed, the next sampling makes it again as a plain record of the texture table; the bind
# (0x820C6210...) is what turns such a record into a render-target-table one - which the "switch" then refreshes from
# the depth each frame - but only if 0x820C1AA8 finds a render-target record over its range whose state is EXACTLY 2
# (0x820C1AF0: cmpwi r10,2). The depth surface's state is 6 for nearly the whole frame (your captures: 421 of 424 depth
# setups in state 6, 3 in state 2) - 6 whenever it is the bound target - so a view remade while the depth is bound is
# never promoted, and that is every frame from then on: it stays a plain texture, read once from memory the GPU never
# writes. Your three R78L captures are that state both times (C4B6C290, then C4D18B40: table 1, state 3, over the depth).
# 0x820C655C: bl 0x820C1AA8 (r3 the record, r4 -> the answer) goes through this: the game's test first; if its answer
# is no and a depth-format record of the render-target table covers exactly the record's range - whatever its state -
# the answer is made yes.
        .globl w_promote
w_promote:
        mflr    %r0
        stwu    %r1,-96(%r1)
        stw     %r0,88(%r1)
        stw     %r3,72(%r1)
        stw     %r4,76(%r1)
        bl      PROMOTEQ
        stw     %r3,80(%r1)
        lwz     %r4,76(%r1)
        lwz     %r3,72(%r1)
        lwz     %r11,0(%r4)
        cmpwi   %cr6,%r11,0
        beq     %cr6,3f
        b       9f
3:      lwz     %r10,0x38(%r3)              # the record itself a depth surface: not this
        srwi    %r10,%r10,24
        cmplwi  %cr6,%r10,0x2D
        beq     %cr6,9f
        lwz     %r12,0x24(%r3)
        cmplwi  %cr6,%r12,0x800
        bge     %cr6,9f
        lis     %r9,TABLE@ha
        addi    %r9,%r9,TABLE@l
        slwi    %r10,%r12,2
        lwzx    %r9,%r10,%r9
        li      %r8,64
1:      cmplwi  %cr6,%r9,0
        beq     %cr6,9f
        addic.  %r8,%r8,-1
        beq     9f
        cmplw   %cr6,%r9,%r3
        beq     %cr6,2f
        lwz     %r10,0x1C(%r9)
        lwz     %r11,0x1C(%r3)
        cmplw   %cr6,%r10,%r11
        bne     %cr6,2f
        lwz     %r10,0x20(%r9)
        lwz     %r11,0x20(%r3)
        cmplw   %cr6,%r10,%r11
        bne     %cr6,2f
        lwz     %r10,0x38(%r9)
        srwi    %r10,%r10,24
        cmplwi  %cr6,%r10,0x2D
        bne     %cr6,2f
        li      %r11,1
        stw     %r11,0(%r4)
        b       9f
2:      lwz     %r10,0x24(%r9)
        subf    %r10,%r10,%r12
        addi    %r10,%r10,0x42
        slwi    %r10,%r10,2
        lwzx    %r9,%r10,%r9
        b       1b
9:      lwz     %r3,80(%r1)
        lwz     %r0,88(%r1)
        addi    %r1,%r1,96
        mtlr    %r0
        blr

# R86L: R85L gave the held clear to the wrong surface again - your capture: "HELD CLEAR GIVEN (R85) at its bind to
# C4B69C00 832x624 ... guest 0749E000", the FIRST larger target, at its SECOND bind in the frame (its own clear had been
# applied at the first, so by then it had none pending and passed R85L's test). The scene's target (guest 07894000) was
# restored as ever. Now the record that took the slot at the moment the clear was held is remembered (HOLD+0xC) and is
# never given it; the next wider record with no clear of its own is the scene's.
# R85L: the sky above 100 % Render Scale - what R84L's capture showed, and the fix that follows from it.
# One frame at 130 % (R84L, your capture of 10 Nov), the game's own clears and R83's move:
#   GUEST CLEAR mask F3 (colour + depth + stencil)  - the scene's clear. It is noted on the game's 640x480 scene record
#                                                     (C4EFFF90 @ 07C54000): pending 3F.
#   GUEST CLEAR mask F0                              - the next target's own clear. Inside it the colour slot goes to the
#                                                     first of the mod's larger targets (@ 0749E000) - and THERE R83 handed
#                                                     over the scene's clear. That target is cleared anyway.
#   setup colour <the mod's larger SCENE target @ 07894000> -> RESTORE -> the scene is drawn.
# So R83 carried the scene's clear to the wrong surface: to the first larger one to take the slot, not to the larger one
# that stands in for the scene's. The scene's target was still never cleared - and its ALPHA with it, which the game's glow
# pass reads: stale alpha where only the sky is would make the sky glow, build up and smear as you turn. (That last step -
# alpha as the cause of what you see - is my reading, not something a capture shows.)
# R85L, in place of R83's move:
#  w_slot (0x820C3F30, slot 0): when a record with a colour clear pending is replaced by one of ANOTHER SIZE, the clear is
#    COPIED into a holding place (HOLD: bits, +0x74 the frame, +0x78 the width of the record it was on, +0x80 the colour)
#    - no record is changed (the 640x480 record keeps its own, as before R83). 
#  w_take (0x820C62EC, the bind's `lwz r10,0xe4(r11)` - where it takes the pending clear of the colour record it is
#    binding): if a clear is being held from THIS frame, and this record has NO colour clear of its own pending (the
#    first larger target has - it is skipped), and it is WIDER than the record the clear came from, the held clear and
#    its colour are put on this record and the bind goes on with it: a clear in place of the restore. The holding place
#    is emptied. Held from another frame: dropped. 
# At 100 % no record of another size ever takes the colour slot: nothing is held, nothing is given.
# The depth slot is as R67 left it. Slots >= 2 are never touched.
        .globl w_slot
w_slot:
        cmplwi  %cr6,%r28,0
        beq     %cr6,9f
        cmplwi  %cr6,%r31,0
        beq     %cr6,9f
        cmplw   %cr6,%r28,%r31
        beq     %cr6,9f
        cmpwi   %cr6,%r23,1
        beq     %cr6,5f
        cmpwi   %cr6,%r23,0
        bne     %cr6,9f
        lwz     %r5,0xE4(%r28)              # the colour slot
        andi.   %r5,%r5,0x0F
        beq     9f
        lwz     %r6,0x4C(%r28)              # only when a record of another size takes over
        lwz     %r5,0x4C(%r31)
        cmplw   %cr6,%r6,%r5
        bne     %cr6,3f
        lwz     %r6,0x50(%r28)
        lwz     %r5,0x50(%r31)
        cmplw   %cr6,%r6,%r5
        beq     %cr6,9f
3:      lis     %r6,HOLD@ha
        addi    %r6,%r6,HOLD@l
        lwz     %r5,0xE4(%r28)
        andi.   %r5,%r5,0x0F
        stw     %r5,0(%r6)                  # held: the bits
        lis     %r5,FRAMEC@ha
        lwz     %r5,FRAMEC@l(%r5)
        stw     %r5,4(%r6)                  # ... the frame
        lwz     %r5,0x4C(%r28)
        stw     %r5,8(%r6)                  # ... the width of the record it was on
        stw     %r31,0xC(%r6)               # ... the record that took the slot just now - it is NOT the one to give it to
        lwz     %r5,0xF0(%r28)
        stw     %r5,0x10(%r6)               # ... the colour
        lwz     %r5,0xF4(%r28)
        stw     %r5,0x14(%r6)
        lwz     %r5,0xF8(%r28)
        stw     %r5,0x18(%r6)
        lwz     %r5,0xFC(%r28)
        stw     %r5,0x1C(%r6)
        b       9f
5:      lwz     %r5,0xE4(%r28)              # the depth slot (R67)
        andi.   %r5,%r5,0x30
        beq     9f
        lwz     %r6,0xE4(%r31)
        or      %r6,%r6,%r5
        stw     %r6,0xE4(%r31)
        lwz     %r6,0x100(%r28)
        stw     %r6,0x100(%r31)
        lwz     %r6,0x104(%r28)
        stw     %r6,0x104(%r31)
        li      %r6,0
        stw     %r6,0xE4(%r28)
9:      stwx    %r31,%r27,%r26
        blr

# 0x820C62EC: lwz r10,0xe4(r11) - r11 the colour record the bind is binding. Returns r10 = its pending word (with a held
# clear put on it, if it takes one). Everything else as it was (cr0 / cr6 are set again by the code that follows:
# 0x820C62F0 cmplwi cr6). Uses r5-r8, saved and put back; makes no call, so the link register is never touched.
        .globl w_take
w_take:
        lwz     %r10,0xE4(%r11)
        stwu    %r1,-96(%r1)
        std     %r5,48(%r1)
        std     %r6,56(%r1)
        std     %r7,64(%r1)
        std     %r8,72(%r1)
        lis     %r5,HOLD@ha
        addi    %r5,%r5,HOLD@l
        lwz     %r6,0(%r5)
        cmplwi  %cr6,%r6,0
        beq     %cr6,8f                     # nothing held
        lis     %r7,FRAMEC@ha
        lwz     %r7,FRAMEC@l(%r7)
        lwz     %r8,4(%r5)
        cmplw   %cr6,%r7,%r8
        bne     %cr6,7f                     # held from another frame: dropped
        andi.   %r7,%r10,0x0F
        bne     8f                          # this record has a colour clear of its own pending
        lwz     %r7,0xC(%r5)                # the record that took the slot when the clear was held: never that one
        cmplw   %cr6,%r7,%r11
        beq     %cr6,8f
        lwz     %r7,0x4C(%r11)
        lwz     %r8,8(%r5)
        cmplw   %cr6,%r7,%r8
        ble     %cr6,8f                     # only a record wider than the one the clear came from
        or      %r10,%r10,%r6
        stw     %r10,0xE4(%r11)
        lwz     %r7,0x10(%r5)
        stw     %r7,0xF0(%r11)
        lwz     %r7,0x14(%r5)
        stw     %r7,0xF4(%r11)
        lwz     %r7,0x18(%r5)
        stw     %r7,0xF8(%r11)
        lwz     %r7,0x1C(%r5)
        stw     %r7,0xFC(%r11)
        li      %r7,0
        stw     %r7,0(%r5)
        b       8f
7:      li      %r7,0
        stw     %r7,0(%r5)
8:      ld      %r5,48(%r1)
        ld      %r6,56(%r1)
        ld      %r7,64(%r1)
        ld      %r8,72(%r1)
        addi    %r1,%r1,96
        blr
