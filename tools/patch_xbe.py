#!/usr/bin/env python3
"""Patch Halo 2's XBE (original Xbox, title 4D530064) with the Spooky Ghost section - v1.0 or the v1.5 title update.

    python3 patch_xbe.py <v1.0|v1.5> <original default.xbe> <spooky.bin> <spooky.map> <output default.xbe>

Changes, and nothing else: appends the section ".spooky"; copies the section table byte for byte into unused
zero space in the header area with a 34th entry; points the image header at it and extends the image size;
redirects calls in .text (every original call target is verified first), patches checked bytes and points checked
address operands at the section's own data (6i: the frame pacing's byte);
recomputes the digest of every section it touched. v1.0 also gets the dashboard title "Halo 2 - Spooky Ghost".
The RSA signature is not re-signed (a softmodded console does not check it). Refuses any unexpected input.
"""
import hashlib, struct, sys

BASE = 0x10000
VERSIONS = {
    'v1.0': dict(timestamp=0x4158CC50, va=0x586000, title='Halo 2 - Spooky Ghost',
                 # --tu-skip (the "1.0 TU Skip" build): XAPI's startup relaunch into T:\$u (0x2d129f) and Xbox Live's
                 # update.xbe launch (XONLINE 0x3b056f) return at once
                 tu_skip=((0x2d129f, '558bec', '33c0c3'), (0x3b056f, '558bec81ec', '33c0c20400')),
                 # 6i.148 --x360 (the "360" copy of 1.0 TU Skip): the XBE's limit-to-64 MB init flag (header 0x124 bit 2) cleared, so the
                 # 128 MB emulator's kernel reports 128 MB (was a hand edit from 6i.132 on), and the C runtime's fmod retry loop
                 # (fprem; fnstsw; sahf; jp back) made to run once: the emulator's FPREM never clears C2 for huge operands and
                 # the game hung in it (the emulator work's "xbe_fmod_guard"); a real Xbox's x87 is exact there in one step
                 x360=((0x373b7d, '7af7', '9090'),),
                 # 6i.42: triple buffering (6i.50: every build): the runtime data region 1.25 MB smaller (its size and end, as
                 # Halo 2 HD's 64 MB path) - room for a third 640x480 buffer. 6i.140: 1.375 MB - with Triple Buffering off
                 # the 1152 KB left over was 48 KB short of Supersampling's 1200 KB buffer (your 360 photo)
                 triple=((0x12b41d, 'b80050140351680040 0e03'.replace(' ', ''), 'b80050fe0251680040f802'),),   # 6i.140: 1.375 MB (was 1.25) - the third buffer, or Supersampling's 1.2 MB with it off
                 bytes=((0x3bc8d, 'ff50', 'ff15'), (0x3bc93, '08', '90'), (0xca139, '0f8534010000', '909090909090'), (0xc683f, '7546', '9090'), (0x203757, '7510', '9090'), (0xfda52, '751d', '9090'), (0x1e2ed4, '751e', '9090'), (0xc9869, '750a', '9090'), (0xd5e33, '751c', '9090'), (0xd976e, '7537', '9090'), (0x1ae51d, '750a', '9090'), (0x1ae76a, '750a', '9090'), (0x1ae7af, '7517', '9090'), (0x1e2d59, '7509', '9090'), (0x1fda13, '7552', '9090'), (0x25cc4b, '7511', '9090'), (0xd9921, '7517', '9090'), (0x1393dc, '0f95c0', 'b00090'), (0x165f2c, '0f95c0', 'b00090'),
                        # 6i: Unlock FPS - the frame pacing's game-in-progress re-read (mov dl, [ecx+0x1120]) made a read of
                        # spooky_pace_clamp (mov dl, [abs]): its ModRM byte here, its address in symrefs
                        (0x12b173, '91', '15'),),
                 symrefs=(   # 6i.71: the jump table entry for target 22 is the game's again (the 360 crashed on it)
                          (0x3bc8f, 0x24448b04, 'spooky_recp'),   # 6i.23: function_3bc30's record draw: call [spooky_recp]
                          (0x12b174, 0x1120, 'spooky_pace_clamp'),
                          # 6i.3: Late frames at once - the display settings game_loop_frame_update compares with and
                          # re-applies (mov edi, 0x43ed80 / mov eax, 0x43ed80) read from spooky_display
                          (0x137f0e, 0x43ed80, 'spooky_display'), (0x137f25, 0x43ed80, 'spooky_display'),),
                 calls=((0x2c86e, 0x13c780, 'hud_hook'), (0x124c34, 0x409b6c, 'input_hook'),
                        (0x2b6f7, 0x13cd0, 'end_a_hook'), (0x2c9e9, 0x13cd0, 'end_b_hook'),
                        (0x12b6b0, 0x12b450, 'main_hook'), (0x2b6e4, 0x2c790, 'hud_view_hook'),
                        (0x2235f0, 0x223080, 'tp_view_hook'), (0x2badc, 0x3d2c0, 'tp_fp_hook'), (0x2bc93, 0x1917eb, 'hudd_hook'), (0x3c401, 0x3ca90, 'water_fit_hook'), (0x14f7b, 0x1dd60, 'rt_size_hook'), (0x15f16, 0x1dd60, 'rt_size_hook'), (0x1961c, 0x1dd60, 'rt_size_hook'), (0x19a5c, 0x1dd60, 'rt_size_hook'), (0x2d984, 0x1dd60, 'rt_size_hook'), (0x34abd, 0x1dd60, 'rt_size_hook'), (0x34dbe, 0x1dd60, 'rt_size_hook'), (0x2a0e96, 0x11c470, 'tvt_hook'), (0x29f720, 0x11c470, 'tvt_hook'), (0x29170f, 0x11c470, 'tvt_hook'), (0x11c795, 0x11c470, 'tvt_hook'), (0xd86f8, 0xdaaa0, 'rdmg_hook'), (0xd8aa9, 0xdaaa0, 'rdmg_hook'), (0xdb0ef, 0xdaaa0, 'rdmg_hook'), (0x2c721, 0x3d2c0, 'tp_fp_hook'),
                        (0xa2de1, 0x102fb0, 'bc_fire_hook'), (0xff1bd, 0x102fb0, 'bc_fire_hook'),
                        (0xcd96e, 0x106030, 'ia_hook'), (0x10091f, 0x106030, 'ia_hook'), (0x100bce, 0x106030, 'ia_hook'),
                        (0x1025f1, 0x106030, 'ia_hook'), (0x103155, 0x106030, 'ia_hook'),
                        (0xc2dac, 0x16640f, 'fpm_hook'),
                        (0x2b9b2, 0x155760, 'persp_hook'), (0x2c1e8, 0x155760, 'persp_hook'), (0x3d323, 0x155760, 'persp_hook'), (0x3e9d6, 0x155760, 'persp_hook'), (0x1662f2, 0x155760, 'persp_hook'), (0x166cfe, 0x155760, 'persp_hook'), (0x1670cb, 0x155760, 'persp_hook'), (0x16ec24, 0x155760, 'persp_hook'), (0x16ec71, 0x155760, 'persp_hook'),
                        # 6h: the HUD Widescreen Fix
                        (0x22ab17, 0x1396c7, 'hanc_hook'), (0x22b050, 0x1396c7, 'hanc_hook'), (0x22b9e2, 0x1396c7, 'hanc_hook'), (0x20056e, 0x1396c7, 'hanc_hook'), (0x13a5ca, 0x22b8e2, 'hdraw_grid'), (0x13a625, 0x22aff1, 'hdraw_bmp'), (0x13a67c, 0x22aa16, 'hdraw_text'), (0x139693, 0x200462, 'hcentre_hook'), (0x1e936a, 0x22a664, 'hquad_hook'), (0x1e93b1, 0x22a664, 'hquad_hook'), (0x1e9400, 0x22a664, 'hquad_hook'), (0x1e944f, 0x22a664, 'hquad_hook'), (0x1e949e, 0x22a664, 'hquad_hook'), (0x1e94e4, 0x22a664, 'hquad_hook'), (0x1e9532, 0x22a664, 'hquad_hook'), (0x1e9574, 0x22a664, 'hquad_hook'), (0x1e95b6, 0x22a664, 'hquad_hook'), (0x20077f, 0x22a664, 'hquad_hook'), (0x22b397, 0x22a664, 'hquad_hook'), (0x22b55c, 0x22a664, 'hquad_hook'), (0x22b690, 0x22a664, 'hquad_hook'), (0x22b6fc, 0x22a664, 'hquad_hook'), (0x22b74f, 0x22a664, 'hquad_hook'), (0x22b7ce, 0x22a664, 'hquad_hook'), (0x22bca4, 0x22a664, 'hquad_hook'), (0x24eca0, 0x22a664, 'hquad_hook'), (0x24f0b6, 0x22a664, 'hquad_hook'), (0x24f357, 0x22a664, 'hquad_hook'), (0x24f3de, 0x22a664, 'hquad_hook'), (0x24f432, 0x22a664, 'hquad_hook'), (0x24f486, 0x22a664, 'hquad_hook'), (0x24f4fb, 0x22a664, 'hquad_hook'), (0x24f556, 0x22a664, 'hquad_hook'), (0x2b12ae, 0x22a664, 'hquad_hook'), (0x22bbbb, 0x2d140, 'hgrid_hook'), (0x22aca8, 0x1fb10, 'htext_hook'), (0x139685, 0x1a2823, 'htrack_hook'), (0x2550c8, 0x3fb2d0, 'htrkv_hook'), (0x255108, 0x3fb2d0, 'htrkv_hook'), (0x255147, 0x3fb2d0, 'htrkv_hook'), (0x255187, 0x3fb2d0, 'htrkv_hook'),
                        # 6h.11: a possessed vehicle in third person
                        (0x15596b, 0x155810, 'hcam_mode_hook'), (0x155abc, 0x155810, 'hcam_mode_hook'), (0x1557d1, 0x155810, 'hcam_mode_hook'),
                        # 6h.12: Checkpoint Settings; the possessed vehicle's driving view
                        (0x22613d, 0x12b790, 'hsave_hook'), (0x226090, 0x12b790, 'hsave_hook'), (0x138082, 0x138e40, 'hrevert_hook'), (0x18755b, 0x1871e0, 'htarget_hook'),
                        # 6h.23: a moved view (third person / free camera) - the effect draw pass's first-person marker; the object
                        # attachments' first-person tests (draw-time visibility, instances made, their markers' mode)
                        (0x31d6a, 0x16640f, 'fpm_hook'), (0x17430c, 0x155760, 'persp_hook'), (0x178f06, 0x155760, 'persp_hook'),
                        (0x179071, 0x155760, 'persp_hook'), (0x179942, 0x155760, 'persp_hook'),
                        # 6h.25: smooth slow motion - a snapshot before each tick, a blend around the draw (6i.48: made before the director)
                        (0x137f89, 0x137fe0, 'ip_tick_hook'), (0x2a99b0, 0x1388e0, 'won_hook'), (0x2c889, 0x13c20, 'late_hook'),
                        (0x12da80, 0x2d15da, 'geo_cache_hook'),   # 6i.191: the geometry cache's block (Larger Caches)
                        (0x1232b0, 0x2d15da, 'anim_cache_hook'),   # 6i.194: the animation cache's block (Larger Caches)
                        (0x13b52, 0x2d140, 'se_hook'),
                        (0x163adb, 0x13f10, 'sw_tail_hook'),   # 6i.222: after the intro video, the last frame drawn and swapped three more times (the 360's flicker)
                          # 6i.210: the landing's real contacts, logged (the solver's rows for a body's contacts)
                          # 6i.193: a level's screen effect at the larger targets' size (the function's other call, 0x22bbbb, is the HUD's - hgrid_hook has it)
                        (0xcfba0, 0xd7b80, 'ohk_hook'), (0xcfe09, 0xd7b80, 'ohk_hook'), (0xd0049, 0xd7b80, 'ohk_hook'), (0xd057d, 0xd7b80, 'ohk_hook'), (0xd7a80, 0xd7b80, 'ohk_hook'), (0xdb40d, 0xd7b80, 'ohk_hook'), (0xdc08b, 0xd7b80, 'ohk_hook'), (0xdda82, 0xd7b80, 'ohk_hook'), (0xe38c6, 0xd7b80, 'ohk_hook'), (0xec7eb, 0xd7b80, 'ohk_hook'), (0xf040a, 0xd7b80, 'ohk_hook'), (0xfae4e, 0xd7b80, 'ohk_hook'), (0xfc81a, 0xd7b80, 'ohk_hook'), (0x119f5a, 0xd7b80, 'ohk_hook'), (0x11a207, 0xd7b80, 'ohk_hook'), (0x14db61, 0xd7b80, 'ohk_hook'), (0x1da961, 0xd7b80, 'ohk_hook'), (0x1dabc9, 0xd7b80, 'ohk_hook'), (0x2a0830, 0xd7b80, 'ohk_hook'), (0x2a0912, 0xd7b80, 'ohk_hook'), (0x12b659, 0x223240, 'ip_draw_hook'), (0x12b647, 0x1554b0, 'ip_dir_hook'), (0x13404, 0x3f5ba0, 'fill_hook'), (0x223288, 0x12b2e0, 'prof_wait_hook'), (0x223296, 0x13f10, 'prof_present_hook'),
                        (0x445b2, 0x3bc30, 'pass_hook'), (0x16e44, 0x189a0, 'aniso_hook'), (0x136ac, 0x2bcd0, 'rs_scene_hook'), (0x1373b, 0x2bcd0, 'rs_scene_hook'), (0x1375a, 0x2bcd0, 'rs_scene_hook'), (0x13788, 0x2bcd0, 'rs_scene_hook'), (0x14b31, 0x3f8100, 'vp_hook'), (0x14b4f, 0x3f8100, 'vp_hook'), (0x14ebe, 0x3f8100, 'vp_hook'), (0x2c2cb, 0x40870, 'rs_ui_hook'), (0x14d2a, 0x1dcf0, 'target_surf_hook'), (0x2c317, 0x20770, 'rs_after_ui_hook'), (0x224cd2, 0x23ed30, 'sprite_hook'), (0x31f26, 0x31170, 'light_hook'), (0x33647, 0x32350, 'shadow_hook'), (0x2c03e, 0x31c80, 'lightloop_hook'), (0x1cd63, 0x3f7b30, 'rt_hook1'), (0x1cd6c, 0x3f7870, 'rt_hook2'),   # 6i.63 / 6i.65
                        (0x1c8b4, 0x3fafa0, 'drawidx_hook'), (0x4c976, 0x3fafa0, 'drawidx_hook'), (0x4dadb, 0x3fafa0, 'drawidx_hook'), (0x4db40, 0x3fafa0, 'drawidx_hook'), (0x4dbdc, 0x3fafa0, 'drawidx_hook'), (0x4dc3a, 0x3fafa0, 'drawidx_hook'), (0x4eb86, 0x3fafa0, 'drawidx_hook'), (0x2256c2, 0x3fafa0, 'drawidx_hook'), (0x44190, 0x3faed0, 'drawv_hook'), (0x4edbc, 0x3faed0, 'drawv_hook'), (0x4ef7e, 0x3faed0, 'drawv_hook'),   # 6i.55: Wireframe on
                        (0x2c0bc, 0x3ede0, 'mark_hook_41'), (0x2c132, 0x3c3c0, 'mark_hook_43'), (0x2c19a, 0x1e370, 'mark_hook_49'), (0x2c322, 0x13870, 'mark_hook_56'), (0x2c327, 0x13630, 'mark_hook_57'), (0x174a01, 0x174a30, 'ptc_update_hook'), (0x22318e, 0x2c490, 'coop_hook'), (0x129cd, 0x3f5240, 'tb_create_hook'), (0x129d8, 0x3f77c0, 'tb_reset_hook'), (0x3bed4, 0x352e0, 'mark_hook_60'), (0x3bede, 0x14bc0, 'mark_hook_61'), (0x2c0d1, 0x25d50, 'fx_a_hook'), (0x2c156, 0x25dc0, 'fx_b_hook'), (0x2c1cb, 0x25dc0, 'fx_b_hook'),   # 6i.5
                        (0x23cbc0, 0x1871e0, 'htarget_hook'), (0x23d7a0, 0x1871e0, 'htarget_hook'))),
    'v1.5': dict(timestamp=0x4600A62B, va=0x596000, title=None, bytes=((0x3bc7d, 'ff50', 'ff15'), (0x3bc83, '08', '90'), (0xcc619, '0f8534010000', '909090909090'), (0xc8d1f, '7546', '9090'), (0x20ba17, '7510', '9090'), (0x102a52, '751d', '9090'), (0x1eb184, '751e', '9090'), (0xcbd49, '750a', '9090'), (0xd8383, '751c', '9090'), (0xdbcbe, '7537', '9090'), (0x1b70dd, '750a', '9090'), (0x1b732a, '750a', '9090'), (0x1b736f, '7517', '9090'), (0x1eb009, '7509', '9090'), (0x205cd3, '7552', '9090'), (0x26695b, '7511', '9090'), (0xdbe71, '7517', '9090'), (0x13bebc, '0f95c0', 'b00090'), (0x168e03, '0f95c0', 'b00090'),
                        (0x12dc83, '91', '15'),),                  # 6i: as v1.0's
                 triple=((0x12df2d, 'b80050140351680040 0e03'.replace(' ', ''), 'b80050fe0251680040f802'),),   # 6i.42; 6i.140: 1.375 MB
                 symrefs=(   # 6i.71
                          (0x3bc7f, 0x24448b04, 'spooky_recp'),   # 6i.23
                          (0x12dc84, 0x1120, 'spooky_pace_clamp'),
                          (0x13a9ee, 0x448e40, 'spooky_display'), (0x13aa05, 0x448e40, 'spooky_display'),),   # 6i.3
                 calls=((0x2c84e, 0x13f260, 'hud_hook'), (0x127744, 0x413c4c, 'input_hook'),
                        (0x2b6d7, 0x13cb0, 'end_a_hook'), (0x2c9c9, 0x13cb0, 'end_b_hook'),
                        (0x12e1c0, 0x12df60, 'main_hook'), (0x2b6c4, 0x2c770, 'hud_view_hook'),
                        (0x22a000, 0x229a90, 'tp_view_hook'), (0x2babc, 0x3d2b0, 'tp_fp_hook'), (0x2bc73, 0x1977ab, 'hudd_hook'), (0xdac48, 0xdcff0, 'rdmg_hook'), (0xdaff9, 0xdcff0, 'rdmg_hook'), (0xdd63f, 0xdcff0, 'rdmg_hook'), (0x2c701, 0x3d2b0, 'tp_fp_hook'),
                        (0xacec1, 0x108080, 'bc_fire_hook'), (0x104304, 0x108080, 'bc_fire_hook'),
                        (0xcfe4e, 0x10b0f0, 'ia_hook'), (0x1059ef, 0x10b0f0, 'ia_hook'), (0x105c9e, 0x10b0f0, 'ia_hook'),
                        (0x1076c1, 0x10b0f0, 'ia_hook'), (0x108225, 0x10b0f0, 'ia_hook'),
                        (0xc4ddc, 0x1692e0, 'fpm_hook'),
                        (0x2b992, 0x158290, 'persp_hook'), (0x2c1c8, 0x158290, 'persp_hook'), (0x3d313, 0x158290, 'persp_hook'), (0x3e9c6, 0x158290, 'persp_hook'), (0x1691c1, 0x158290, 'persp_hook'), (0x169bcf, 0x158290, 'persp_hook'), (0x169f9c, 0x158290, 'persp_hook'), (0x171a84, 0x158290, 'persp_hook'), (0x171ad1, 0x158290, 'persp_hook'),
                        # 6h: the HUD Widescreen Fix
                        (0x231517, 0x13c1a7, 'hanc_hook'), (0x231a50, 0x13c1a7, 'hanc_hook'), (0x2323e2, 0x13c1a7, 'hanc_hook'), (0x20882e, 0x13c1a7, 'hanc_hook'), (0x13d0aa, 0x2322e2, 'hdraw_grid'), (0x13d105, 0x2319f1, 'hdraw_bmp'), (0x13d15c, 0x231416, 'hdraw_text'), (0x13c173, 0x208722, 'hcentre_hook'), (0x1f161a, 0x231064, 'hquad_hook'), (0x1f1661, 0x231064, 'hquad_hook'), (0x1f16b0, 0x231064, 'hquad_hook'), (0x1f16ff, 0x231064, 'hquad_hook'), (0x1f174e, 0x231064, 'hquad_hook'), (0x1f1794, 0x231064, 'hquad_hook'), (0x1f17e2, 0x231064, 'hquad_hook'), (0x1f1824, 0x231064, 'hquad_hook'), (0x1f1866, 0x231064, 'hquad_hook'), (0x208a3f, 0x231064, 'hquad_hook'), (0x231d97, 0x231064, 'hquad_hook'), (0x231f5c, 0x231064, 'hquad_hook'), (0x232090, 0x231064, 'hquad_hook'), (0x2320fc, 0x231064, 'hquad_hook'), (0x23214f, 0x231064, 'hquad_hook'), (0x2321ce, 0x231064, 'hquad_hook'), (0x2326a4, 0x231064, 'hquad_hook'), (0x25cbd0, 0x231064, 'hquad_hook'), (0x25cfe6, 0x231064, 'hquad_hook'), (0x25d287, 0x231064, 'hquad_hook'), (0x25d30e, 0x231064, 'hquad_hook'), (0x25d362, 0x231064, 'hquad_hook'), (0x25d3b6, 0x231064, 'hquad_hook'), (0x25d42b, 0x231064, 'hquad_hook'), (0x25d486, 0x231064, 'hquad_hook'), (0x2bafbe, 0x231064, 'hquad_hook'), (0x2325bb, 0x2d120, 'hgrid_hook'), (0x2316a8, 0x1faf0, 'htext_hook'), (0x13c165, 0x1ac453, 'htrack_hook'), (0x25ed98, 0x4053b0, 'htrkv_hook'), (0x25edd8, 0x4053b0, 'htrkv_hook'), (0x25ee17, 0x4053b0, 'htrkv_hook'), (0x25ee57, 0x4053b0, 'htrkv_hook'),
                        # 6h.11: a possessed vehicle in third person
                        (0x15849b, 0x158340, 'hcam_mode_hook'), (0x1585ec, 0x158340, 'hcam_mode_hook'), (0x158301, 0x158340, 'hcam_mode_hook'),
                        # 6h.12: Checkpoint Settings; the possessed vehicle's driving view
                        (0x22cb3d, 0x12e2a0, 'hsave_hook'), (0x22ca90, 0x12e2a0, 'hsave_hook'), (0x13ab62, 0x13b920, 'hrevert_hook'), (0x18a34b, 0x189fd0, 'htarget_hook'),
                        # 6h.23: as v1.0's
                        (0x31d5a, 0x1692e0, 'fpm_hook'), (0x1771ac, 0x158290, 'persp_hook'), (0x17bd96, 0x158290, 'persp_hook'),
                        (0x17bf01, 0x158290, 'persp_hook'), (0x17c7d2, 0x158290, 'persp_hook'),
                        # 6h.25: as v1.0's
                        (0x13aa69, 0x13aac0, 'ip_tick_hook'), (0x12e169, 0x229c50, 'ip_draw_hook'), (0x12e157, 0x157fe0, 'ip_dir_hook'), (0x133e4, 0x3ffc80, 'fill_hook'), (0x229c98, 0x12ddf0, 'prof_wait_hook'), (0x229ca6, 0x13ef0, 'prof_present_hook'),
                        (0x445a2, 0x3bc20, 'pass_hook'), (0x16e24, 0x18980, 'aniso_hook'), (0x1368c, 0x2bcb0, 'rs_scene_hook'), (0x1371b, 0x2bcb0, 'rs_scene_hook'), (0x1373a, 0x2bcb0, 'rs_scene_hook'), (0x13768, 0x2bcb0, 'rs_scene_hook'), (0x14b11, 0x4021e0, 'vp_hook'), (0x14b2f, 0x4021e0, 'vp_hook'), (0x14e9e, 0x4021e0, 'vp_hook'), (0x2c2ab, 0x40860, 'rs_ui_hook'), (0x14d0a, 0x1dcd0, 'target_surf_hook'), (0x2c2f7, 0x20750, 'rs_after_ui_hook'), (0x22b6c2, 0x248020, 'sprite_hook'), (0x31f16, 0x31150, 'light_hook'), (0x33637, 0x32340, 'shadow_hook'), (0x2c01e, 0x31c70, 'lightloop_hook'), (0x1cd43, 0x401c10, 'rt_hook1'), (0x1cd4c, 0x401950, 'rt_hook2'),   # 6i.63 / 6i.65
                        (0x1c894, 0x405080, 'drawidx_hook'), (0x4c966, 0x405080, 'drawidx_hook'), (0x4dacb, 0x405080, 'drawidx_hook'), (0x4db30, 0x405080, 'drawidx_hook'), (0x4dbcc, 0x405080, 'drawidx_hook'), (0x4dc2a, 0x405080, 'drawidx_hook'), (0x4eb76, 0x405080, 'drawidx_hook'), (0x22c0c2, 0x405080, 'drawidx_hook'), (0x44180, 0x404fb0, 'drawv_hook'), (0x4edac, 0x404fb0, 'drawv_hook'), (0x4ef6e, 0x404fb0, 'drawv_hook'),   # 6i.55: Wireframe on
                        (0x2c09c, 0x3edd0, 'mark_hook_41'), (0x2c112, 0x3c3b0, 'mark_hook_43'), (0x2c17a, 0x1e350, 'mark_hook_49'), (0x2c302, 0x13850, 'mark_hook_56'), (0x2c307, 0x13610, 'mark_hook_57'), (0x1778a1, 0x1778d0, 'ptc_update_hook'), (0x229b9e, 0x2c470, 'coop_hook'), (0x129ad, 0x3ff320, 'tb_create_hook'), (0x129b8, 0x4018a0, 'tb_reset_hook'), (0x3bec4, 0x352d0, 'mark_hook_60'), (0x3bece, 0x14ba0, 'mark_hook_61'), (0x2c0b1, 0x25d30, 'fx_a_hook'), (0x2c136, 0x25da0, 'fx_b_hook'), (0x2c1ab, 0x25da0, 'fx_b_hook'),   # 6i.5
                        (0x245eb0, 0x189fd0, 'htarget_hook'), (0x246a90, 0x189fd0, 'htarget_hook'))),
}
# v2.0: the v1.5 build brought level with v1.0. The 38 call sites added to v1.0 since 6i.155 (Object Spawning's, One Shot
# Kills', the Campaign Timer's split, the render targets' sizes, the caches', the fog's, the intro video's last swap...),
# each carried over by tools15/xmap.py: the same place in the code by a masked match of the bytes around it, and checked
# to call the same function as v1.0's does. With these the two lists hold the same 172 hooks.
VERSIONS['v1.5']['calls'] = VERSIONS['v1.5']['calls'] + (
    (0x3c3f1, 0x3ca80, 'water_fit_hook'), (0x14f5b, 0x1dd40, 'rt_size_hook'), (0x15ef6, 0x1dd40, 'rt_size_hook'), (0x195fc, 0x1dd40, 'rt_size_hook'),
    (0x19a3c, 0x1dd40, 'rt_size_hook'), (0x2d964, 0x1dd40, 'rt_size_hook'), (0x34aad, 0x1dd40, 'rt_size_hook'), (0x34dae, 0x1dd40, 'rt_size_hook'),
    (0x2aaba6, 0x11eba0, 'tvt_hook'), (0x2a9430, 0x11eba0, 'tvt_hook'), (0x29b41f, 0x11eba0, 'tvt_hook'), (0x11eec5, 0x11eba0, 'tvt_hook'),
    (0x2b3710, 0x13b3c0, 'won_hook'), (0x2c869, 0x13c00, 'late_hook'), (0x130590, 0x2db4ca, 'geo_cache_hook'), (0x125dc0, 0x2db4ca, 'anim_cache_hook'),
    (0x13b32, 0x2d120, 'se_hook'), (0x1669bb, 0x13ef0, 'sw_tail_hook'), (0xd2090, 0xda0d0, 'ohk_hook'), (0xd22f9, 0xda0d0, 'ohk_hook'),
    (0xd2589, 0xda0d0, 'ohk_hook'), (0xd2abd, 0xda0d0, 'ohk_hook'), (0xd9fd0, 0xda0d0, 'ohk_hook'), (0xdd95d, 0xda0d0, 'ohk_hook'),
    (0xde5db, 0xda0d0, 'ohk_hook'), (0xdffd2, 0xda0d0, 'ohk_hook'), (0xe5e16, 0xda0d0, 'ohk_hook'), (0xeed7b, 0xda0d0, 'ohk_hook'),
    (0xf296a, 0xda0d0, 'ohk_hook'), (0xffe4e, 0xda0d0, 'ohk_hook'), (0x10181a, 0xda0d0, 'ohk_hook'), (0x11c63a, 0xda0d0, 'ohk_hook'),
    (0x11c8e7, 0xda0d0, 'ohk_hook'), (0x150681, 0xda0d0, 'ohk_hook'), (0x1e2c11, 0xda0d0, 'ohk_hook'), (0x1e2e79, 0xda0d0, 'ohk_hook'),
    (0x2aa540, 0xda0d0, 'ohk_hook'), (0x2aa622, 0xda0d0, 'ohk_hook'),
)
# ... and the 128 MB changes (--x360) for v1.5 too: the same header flag (0x124 bit 2), and the same fmod retry loop in the C
# runtime (v1.0 0x373b7d: 7a f7 -> 90 90), which is at 0x37dc1d here with the same bytes
VERSIONS['v1.5']['x360'] = ((0x37dc1d, '7af7', '9090'),)

# 6i.6: the Direct3D probes (patch_xbe.py ... --probes; the "Direct3D probes" XBEs only): each of ten of Direct3D's own
# functions starts with jmp [spooky_dvp_*] - FF 25 here, the pointer's address in symrefs - into a thunk in the section
# (hooks.S dv_*) that counts it (two also timed) and runs the six-plus bytes it replaced. In Direct3D's section ('D3D').
PROBES = {
    'v1.0': dict(bytes=((0x3faae0, '83ec', 'ff25'),
                        (0x3fad50, '568b', 'ff25'),
                        (0x3fac30, '8b42', 'ff25'),
                        (0x3f8630, '8b0d', 'ff25'),
                        (0x3fafa0, '558b', 'ff25'),
                        (0x3faed0, '5355', 'ff25'),
                        (0x3f8320, '83ec', 'ff25'),
                        (0x3fb520, '8b07', 'ff25'),
                        (0x3fa3d0, '5185', 'ff25'),
                        (0x3f7330, '5657', 'ff25'),),
                 symrefs=((0x3faae2, 0x8b565308, 'spooky_dvp_makespace'),
                          (0x3fad52, 0x40748835, 'spooky_dvp_busywait'),
                          (0x3fac32, 0x5604a808, 'spooky_dvp_isbusy'),
                          (0x3f8632, 0x00407488, 'spooky_dvp_vistest'),
                          (0x3fafa2, 0x08ec83ec, 'spooky_dvp_drawidx'),
                          (0x3faed2, 0x74882d8b, 'spooky_dvp_draw'),
                          (0x3f8322, 0x1d8b5308, 'spooky_dvp_settex'),
                          (0x3fb522, 0x81f08b56, 'spooky_dvp_setps'),
                          (0x3fa3d2, 0x1d8b53c0, 'spooky_dvp_setpsprog'),
                          (0x3f7332, 0x4fe03d8b, 'spooky_dvp_vsconst'),)),
    'v1.5': dict(bytes=((0x404bc0, '83ec', 'ff25'),
                        (0x404e30, '568b', 'ff25'),
                        (0x404d10, '8b42', 'ff25'),
                        (0x402710, '8b0d', 'ff25'),
                        (0x405080, '558b', 'ff25'),
                        (0x404fb0, '5355', 'ff25'),
                        (0x402400, '83ec', 'ff25'),
                        (0x405600, '8b07', 'ff25'),
                        (0x4044b0, '5185', 'ff25'),
                        (0x401410, '5657', 'ff25'),),
                 symrefs=((0x404bc2, 0x8b565308, 'spooky_dvp_makespace'),
                          (0x404e32, 0x41156835, 'spooky_dvp_busywait'),
                          (0x404d12, 0x5604a808, 'spooky_dvp_isbusy'),
                          (0x402712, 0x00411568, 'spooky_dvp_vistest'),
                          (0x405082, 0x08ec83ec, 'spooky_dvp_drawidx'),
                          (0x404fb2, 0x15682d8b, 'spooky_dvp_draw'),
                          (0x402402, 0x1d8b5308, 'spooky_dvp_settex'),
                          (0x405602, 0x81f08b56, 'spooky_dvp_setps'),
                          (0x4044b2, 0x1d8b53c0, 'spooky_dvp_setpsprog'),
                          (0x401412, 0xf0c03d8b, 'spooky_dvp_vsconst'),)),
}
NEW_NAME = b'.spooky\0'
FILLER = bytes.fromhex('8d9b00000000')          # lea ebx,[ebx+0]: does nothing


def die(msg):
    sys.exit('patch_xbe: ' + msg)


HUD_HOOKS = ('hanc_hook', 'hdraw_grid', 'hdraw_bmp', 'hdraw_text', 'hcentre_hook', 'hquad_hook', 'hgrid_hook', 'htext_hook', 'htrack_hook', 'htrkv_hook')


def main(version, src, binpath, mappath, out, *opts):
    cfg = VERSIONS.get(version) or die('version must be v1.0 or v1.5')
    # 6i.23: --probes retired (the Direct3D probes' detours); PROBES stays for unpatch.py to undo older probe XBEs
    if '--tu-skip' in opts:                # v1.0: the title update checks skipped (the "1.0 TU Skip" build)
        if not cfg.get('tu_skip'): die('--tu-skip: only for v1.0')
        cfg = dict(cfg); cfg['bytes'] = tuple(cfg['bytes']) + tuple(cfg['tu_skip'])
    x360 = '--x360' in opts                # 6i.148: the 1.0 TU Skip copy for the RGH 360's 128 MB emulator
    if x360:
        if not cfg.get('x360'): die('--x360: no 128 MB changes listed for this version')
        cfg = dict(cfg); cfg['bytes'] = tuple(cfg['bytes']) + tuple(cfg['x360'])
    # 6i.50: triple buffering in every build (6i.42 - 6i.49: --triple, the triple_buffering XBEs): the region smaller here,
    # the section's flag set in the C source (spooky_triple = 1)
    cfg = dict(cfg); cfg['bytes'] = tuple(cfg['bytes']) + tuple(cfg['triple'])
    if '--no-hud' in opts:                 # (testing) the HUD Widescreen Fix's call sites left as the game's
        cfg = dict(cfg, calls=tuple(c for c in cfg['calls'] if c[2] not in HUD_HOOKS))
    x = bytearray(open(src, 'rb').read())
    code = open(binpath, 'rb').read()
    syms = {p[2]: int(p[0], 16) for p in (l.split() for l in open(mappath)) if len(p) == 3}
    if x[:4] != b'XBEH': die('not an XBE')
    base, hdr_size, img_size, img_hdr_size, ts, cert, nsec, secaddr = struct.unpack_from('<8I', x, 0x104)
    if base != BASE or hdr_size != 0x2000: die('unexpected image base / header size')
    if struct.unpack_from('<I', x, cert - BASE + 8)[0] != 0x4D530064: die('not Halo 2')
    if ts != cfg['timestamp']: die('image timestamp %08X is not Halo 2 %s (%08X)' % (ts, version, cfg['timestamp']))
    if nsec != 33: die('unexpected section count %d (already patched?)' % nsec)
    if syms.get('__section_start') != cfg['va']: die('spooky.bin was not linked at 0x%X' % cfg['va'])
    if x360:                               # the init flags (header 0x124): 0x80000005 -> 0x80000001 (bit 2 = limit to 64 MB)
        if struct.unpack_from('<I', x, 0x124)[0] != 0x80000005: die('header 0x124 is not the stock init flags')
        struct.pack_into('<I', x, 0x124, 0x80000001)
    vsize = syms['__section_end'] - cfg['va']
    if len(code) != vsize: die('spooky.bin size does not match its map')
    # 6i.150: the zero-initialised variables (bss) are not written to the file - the section's raw size stops at __bss_start
    # and the kernel zero-fills up to the virtual size, as it does for Halo 2's own .data (vsize 0x1029F8, rsize 0x1EF7C)
    rsize = syms.get('__bss_start', cfg['va'] + vsize) - cfg['va']
    if not 0 < rsize <= vsize or any(code[rsize:]): die('spooky.bin: data past __bss_start')
    raw_code = code[:rsize]

    secs = []
    for i in range(nsec):
        o = secaddr - BASE + 56 * i
        fl, va, vs, raw, rs, na, rc, hr, tr = struct.unpack_from('<9I', x, o)
        secs.append(dict(hdr=bytes(x[o:o + 56]), flags=fl, va=va, vsize=vs, raw=raw, rsize=rs, name=x[na - BASE:].split(b'\0')[0].decode(),
                         refs=(hr - BASE, tr - BASE), name_off=na - BASE))
    if max(s['va'] + s['vsize'] for s in secs) > cfg['va']: die('section address not free')
    data_flags = next(s['flags'] for s in secs if s['name'] == '.data')

    # free zero space in the header area: after everything the headers use, before the embedded PE header
    used_end = max([0x178, cert - BASE + struct.unpack_from('<I', x, cert - BASE)[0], secaddr - BASE + 56 * nsec] +
                   [s['name_off'] + len(s['name']) + 1 for s in secs] + [r + 2 for s in secs for r in s['refs']])
    for field in (0x14C, 0x150, 0x154):
        a = struct.unpack_from('<I', x, field)[0]
        if a: used_end = max(used_end, a - BASE + 128)
    nlib, libaddr = struct.unpack_from('<II', x, 0x160); used_end = max(used_end, libaddr - BASE + 16 * nlib)
    logo, logosz = struct.unpack_from('<II', x, 0x170); used_end = max(used_end, logo - BASE + logosz)
    pe = x.find(b'MZ', used_end, 0x2000); limit = pe if pe > 0 else 0x2000
    table_off = (used_end + 3) & ~3
    need = 56 * (nsec + 1) + len(NEW_NAME) + 4
    if table_off + need > limit or any(x[table_off:table_off + need]): die('no free zero space in the header area')

    def file_off(va):
        for i, s in enumerate(secs):
            if s['va'] <= va < s['va'] + s['rsize']: return i, s['raw'] + va - s['va']
        die('0x%X is not in a section' % va)
    touched = set()
    for site, target, hook in cfg['calls']:
        i, off = file_off(site)
        if secs[i]['name'] != '.text' or x[off] not in (0xE8, 0xE9): die('0x%X is not a call or jump in .text' % site)
        cur = site + 5 + struct.unpack_from('<i', x, off + 1)[0]
        if cur != target: die('0x%X calls 0x%X, expected 0x%X' % (site, cur, target))
    # the address operands pointed at the mod's own data (step 6g.1: the HUD Scaling Fix's factor): each must hold the address expected
    for site, want, sym in cfg.get('symrefs', ()):
        i, off = file_off(site)
        if secs[i]['name'] not in ('.text', 'D3D') or struct.unpack_from('<I', x, off)[0] != want:
            die('0x%X holds 0x%X, expected 0x%X' % (site, struct.unpack_from('<I', x, off)[0], want))
        if sym not in syms: die('no symbol %s in the map' % sym)
    # the byte patches (the skulls' campaign-only tests, step 6e; v1.0's title update checks, 6h.10: XAPI's startup relaunch
    # into T:\$u (0x2d129f) and Xbox Live's update.xbe launch (XONLINE 0x3b056f) return at once): each must find exactly the bytes expected
    for site, want, new in cfg.get('bytes', ()):
        i, off = file_off(site)
        a, b = bytes.fromhex(want), bytes.fromhex(new)
        if secs[i]['name'] not in ('.text', 'XONLINE', 'D3D') or bytes(x[off:off + len(a)]) != a or len(a) != len(b):
            die('0x%X holds %s, expected %s' % (site, bytes(x[off:off + len(a)]).hex(), want))

    # append the section's data, page-aligned in the file
    raw_addr = (len(x) + 0xFFF) & ~0xFFF
    x += b'\0' * (raw_addr - len(x)) + raw_code
    x += b'\0' * (((len(x) + 0xFFF) & ~0xFFF) - len(x))
    for site, target, hook in cfg['calls']:
        i, off = file_off(site); struct.pack_into('<i', x, off + 1, syms[hook] - (site + 5)); touched.add(i)
    for site, want, new in cfg.get('bytes', ()):
        i, off = file_off(site); b = bytes.fromhex(new); x[off:off + len(b)] = b; touched.add(i)
    for site, want, sym in cfg.get('symrefs', ()):
        i, off = file_off(site); struct.pack_into('<I', x, off, syms[sym]); touched.add(i)

    if cfg['title']:
        c = cert - BASE
        x[c + 0xC:c + 0xC + 80] = cfg['title'].encode('utf-16le').ljust(80, b'\0')

    # the new table: the 33 originals byte for byte (touched sections with their new digests), then .spooky
    name_off = table_off + 56 * (nsec + 1); refs_off = name_off + len(NEW_NAME)
    table = bytearray()
    for i, s in enumerate(secs):
        h = bytearray(s['hdr'])
        if i in touched:
            h[36:56] = hashlib.sha1(struct.pack('<I', s['rsize']) + bytes(x[s['raw']:s['raw'] + s['rsize']])).digest()
            struct.pack_into('20s', x, secaddr - BASE + 56 * i + 36, bytes(h[36:56]))      # the old table stays valid too
        table += h
    new = bytearray(56)
    struct.pack_into('<9I', new, 0, data_flags, cfg['va'], vsize, raw_addr, len(raw_code), BASE + name_off, 0, BASE + refs_off, BASE + refs_off + 2)
    new[36:56] = hashlib.sha1(struct.pack('<I', len(raw_code)) + raw_code).digest()
    table += new
    x[table_off:table_off + len(table)] = table
    x[name_off:name_off + len(NEW_NAME)] = NEW_NAME
    x[refs_off:refs_off + 4] = b'\0' * 4
    struct.pack_into('<I', x, 0x11C, nsec + 1)
    struct.pack_into('<I', x, 0x120, BASE + table_off)
    struct.pack_into('<I', x, 0x10C, cfg['va'] + vsize - BASE)
    open(out, 'wb').write(x)
    print('%s patched: .spooky at 0x%X (%d bytes in memory, %d in the file, file 0x%X); table at 0x%X; %s%s'
          % (version, cfg['va'], len(code), len(raw_code), raw_addr, table_off,
             ', '.join('0x%X -> %s 0x%X' % (s, h, syms[h]) for s, t, h in cfg['calls']),
             '; title "%s"' % cfg['title'] if cfg['title'] else ''))


if __name__ == '__main__':
    if len(sys.argv) not in (6, 7, 8): sys.exit(__doc__)
    main(*sys.argv[1:])
