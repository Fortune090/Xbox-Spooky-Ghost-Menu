/* Halo 2 (Xbox) v1.5 addresses - found by matching each v1.0 function in the v1.5 XBE (match.py); each matched exactly once */
#define H2_VERSION_TEXT "v1.5"
#define ADDR_XINPUTGETSTATE 0x413C4C
#define ADDR_TEXT_STYLE_CLEAR 0x141380
#define ADDR_FONT_GET 0x124EB0
#define ADDR_DRAW_TEXT 0x1FA30
#define ADDR_DRAW_STRING_GLOBALS 0x4F3C70
#define ADDR_FONT_HANDLES 0x4EF1C4
#define ADDR_CREATEFILEA 0x2DC640
#define ADDR_WRITEFILE 0x2DC21E
#define ADDR_SETFILEPOINTER 0x2DC378
#define ADDR_CLOSEHANDLE 0x2DBE2A
#define ADDR_FLAG_485602 0x491E22
#define ADDR_FLAG_51080C 0x51DB0C
/* superbounce HUD (step 2) */
#define ADDR_GAME_TIME_PTR 0x51DF54
#define ADDR_OBJECTS_PTR 0x4ECBD0
#define ADDR_LOCAL_PLAYERS_PTR 0x4F54F0
#define ADDR_PLAYERS_PTR 0x4F54F4
#define ADDR_HAVOK_COMPONENTS_PTR 0x52BE5C
#define ADDR_HAVOK_WORLD_PTR 0x52BE48
#define ADDR_GAME_OPTIONS_PTR 0x4F3218   /* game options: +8 == 1 is campaign (function_12b790 checks it) */
#define ADDR_AI_GLOBALS_PTR 0x5020A8   /* s_ai_globals*: +1 active (gates the AI update; ai_dispose_from_old_map) */
#define ADDR_VIEW_BUILD 0x229A90   /* function_223080: builds a view from a player's camera record (its last stack argument) */
#define ADDR_FP_SETUP 0x3D2B0     /* function_3d2c0: the view's first-person object and weapon models */
#define ADDR_FP_OBJECT 0x4CE3F0    /* g_4c1bd0: the view's first-person object (hidden; NONE = third person) */
#define ADDR_FP_MODELS 0x4CE3F4    /* g_4c1bd4: its first-person weapon models (0 = none drawn) */
#define ADDR_VIEW_USER 0x4C66F8    /* g_4b9ed8: the player whose view is being drawn */
#define ADDR_FIRE 0x108080          /* function_102fb0(weapon, barrel), stdcall: a weapon firing a shot */
#define ADDR_INF_AMMO_FN 0x10B0F0   /* function_106030(weapon): "infinite ammo?" - multiplayer game types only */
#define ADDR_TRAIT_FN 0x15C7F0      /* the game-type lookup it calls (its call at +0x52) */
#define ADDR_FP_MARKER 0x1692E0   /* first_person_weapon_get_marker(ecx = object; marker, position, forward, up): ret 0x10, al */
#define ADDR_PERSPECTIVE 0x158290   /* function_155760: the director's perspective for a player (eax = player; 0 first person, 3 following) */
#define ADDR_VT_GSK 0x41CD08   /* the convex pair (GSK) agent's vtable: count +0x18, separating normal +0x20, TIM +0x2C (convex_pair_2ee2e0) */
#define ADDR_VT_SWEEP 0x41C370   /* convex_pair_2dae40: a GSK subclass (the welder's children, seen in game) - the same TIM layout */
#define ADDR_VT_WELD 0x41D01C    /* the collection (welder) agent: {key, ?, agent} children at +0x0C / +0x10 (seen in game) */
#define ADDR_VT_XFA 0x41CC64    /* the transform agent (pair_agent_2ed170): its child agent at +0x0C */
#define ADDR_VT_XFB 0x41CC84    /* the same, the shapes the other way round */
#define ADDR_VT_LIST 0x41CCC8   /* the list agent (list_2eda30): one child agent per child of a list shape (type 10) - [+0x0C], +0x10 of them */
#define ADDR_VT_BVA 0x41CB58    /* a BV-tree collection agent (bounded_2e9530): {key, ?, agent} entries at [+0x0C], +0x10 of them */
#define ADDR_VT_BVB 0x41CB9C    /* its subclass (derived_2eb4e0) - seen inside the level's welder by the Cairo computers / trees */
#define ADDR_SCENERY_TABLE 0x52BE70   /* pointer to u32[]: the object datum per scenery index (level-shape keys of category 3 / 4: key & 0xFFFF) - function_1ef3e0 */
#define ADDR_SAVE_MAP 0x556E00   /* main_globals.save_map: the checkpoint save request (Yelo's save button sets it) */
#define ADDR_MAP_NAME 0x55674C
#define ADDR_READFILE 0x2DC131
#define ADDR_KT_FLUSH 0x41B6B0   /* the kernel import thunk table's slot for NtFlushBuffersFile (ordinal 198): its address after loading */
#define ADDR_DATA_LO 0x46D6E0
#define ADDR_DATA_HI 0x573858
/* drawing (step 3a): the game's own flat-colour overlay recipe (0x40890) and the frame camera */
#define ADDR_RENDER_STATE 0x222B0
#define ADDR_VS_SELECT 0x1CC10
#define ADDR_VS_CACHE 0x1C6F0
#define ADDR_PS_SET 0x15160
#define ADDR_COMMIT 0x1CF30
#define ADDR_VS_CONST 0x4014F0
#define ADDR_VERTEX2S 0x405410
#define ADDR_BEGIN 0x405520
#define ADDR_END 0x405560
#define ADDR_VS_CACHE_ARG 0x52DCC0
#define ADDR_PS_DEF 0x491788
#define ADDR_FRAME_STORAGE 0x491E20
#define ADDR_CAMERA_STORAGE 0x4C65C0
#define ADDR_HUD_VIEW 0x2C770
#define ADDR_PLAYER_CAMERAS 0x4F655C
#define ADDR_WIDESCREEN_FN 0x16A70
#define ADDR_WIDESCREEN_FLAG 0x4922E2
#define ADDR_ASPECT_FLAGS_PTR 0x51DF50
/* step 3b: the current level's collision BSP (g_4e033c, the leaf search's bsp) */
#define ADDR_COLLISION_BSP 0x4ECC0C
/* step 3c: the current structure BSP (g_4e0348): instanced geometry instances +0x140, definitions +0x138 */
#define ADDR_STRUCTURE_BSP 0x4ECC18
/* step 3c.3: the tag instances (g_4e3b44: 16 bytes each - group +0x00, data +0x08) */
#define ADDR_TAG_INSTANCES 0x4F0414
#define ADDR_OOB_TEST 0xCE940       /* the out-of-bounds test: __stdcall bool (unit) - its body inside solid (collision 0x14800005) */
#define ADDR_VT_DEACT_SPATIAL 0x41EB6C   /* Havok's spatial deactivator (short test slot 5, long slot 6) */
#define ADDR_VT_DEACT_FAKE 0x41F1B0      /* Havok's fake deactivator: never still */
#define ADDR_VT_DEACT_HALO 0x461970      /* Halo's subclass: the veto, then Havok's tests */
#define ADDR_DEACT_VETO 0x1D9BC0         /* Halo's veto: bool (deactivator in eax) - reads only */
#define ADDR_HUD_DRAW 0x1977AB           /* the HUD for the view being drawn (items, messages, the player HUD): void(void) */
#define ADDR_PAD_HANDLES 0x4F2A9C        /* the four controller ports' XInput handles */
#define ADDR_PAD_PREFS 0x52D4A8          /* the gamepad preferences, 0x1C a port: +0x1A look inverted */
#define ADDR_VIRTUALALLOC 0x33B3AC       /* XAPI VirtualAlloc (__stdcall): the predicted spots' memory, at run time */
#define ADDR_VIRTUALFREE 0x33B3DA
#define ADDR_GETFILESIZE 0x2DC605
#define ADDR_SKULLS 0x5020B4            /* bool[16]: the skulls (0 Envy ... 14 Whuppopotamus), read by every skull effect */
#define ADDR_REGION_DMG 0xDCFF0        /* damage to one of an object's regions (its parts): __stdcall(info, object, owner, region, damage, accumulator), ret 0x18 */
#define ADDR_SET_UNIT 0x14F620          /* player_set_unit(player [pushed], unit [ebx]), ret 4 - the game's own possession */
#define ADDR_KILL 0xDE5F0              /* the death a kill volume gives: __stdcall(object), ret 4 - the death damage (damage globals +0x14) through a full damage report: a normal death */
#define ADDR_ANAMORPHIC 0x46A514        /* float 4/3: the anamorphic scaling factor (Halo 2 HD's DisableAnamorphicScaling writes 1.0 here) - read-only data */
/* step 6h: the HUD Widescreen Fix - the anchor function, the widget drawers and what they draw with */
#define ADDR_HUD_ANCHOR 0x13C1A7    /* the HUD anchor point: eax anchor (0 TR, 1 TL, 2 BL, 3 BR, 4 centre, else the aim point), esi float[2] out */
#define ADDR_D_GRID 0x2322E2        /* the widget drawers: shader grid, bitmap, text */
#define ADDR_D_BMP 0x2319F1
#define ADDR_D_TEXT 0x231416
#define ADDR_FRAME_WINDOW 0x491E68  /* frame storage +0x48: the window, rect16 {top, left, bottom, right} */
#define ADDR_HUD_CENTRE 0x208722    /* the centre element's draw (anchor 4): ret */
#define ADDR_HUD_QUAD 0x231064      /* a textured quad: ecx its 4 corners {x, y}, eax its texture box; ret 0xc */
#define ADDR_GRID_DRAW 0x2D120     /* a shader widget: 4th argument its 3x3 grid of {x, y, u, v} */
#define ADDR_TEXT_DRAW 0x1FAF0     /* a string: ecx text, (rect16 *, float scale) - ret 8 */
#define ADDR_HUD_VIEWPORT_X0 0x4C65F2  /* camera storage +0x32: the HUD view's viewport left */
#define ADDR_HUD_FRAME_X0 0x4C65FA  /* camera storage +0x3A: the HUD view's safe frame left (the bottom-left anchor's x) */
#define ADDR_TRACKER 0x1AC453       /* the motion tracker's draw (eax its point, ecx the player): ret */
#define ADDR_DIRECTOR 0x157FE0     /* 6i.48: the director's frame update (__stdcall, the frame's seconds), from the frame at 0x12e157 */
#define ADDR_FILLMODE 0x3FFC80     /* 6i.50: Direct3D SetRenderState_FillMode (__stdcall, the mode), called by the frame setup at 0x133e4 */
#define ADDR_SET_VDATA4F 0x4053B0   /* D3D SetVertexData4f (edi register, 4 floats): ret 0x10 */
#define ADDR_CAM_MODE 0x158340      /* the director's first-person / following choice for a unit (ebx; one argument): ret 4 */
#define ADDR_MAIN_SAVE 0x12E2A0     /* main_save_map: the save request, from the checkpoint logic */
#define ADDR_DEATH_REVERT 0x13B920  /* the once-a-tick revert after you die (sets revert_map) */
#define ADDR_CAM_TARGET 0x189FD0    /* the following camera's target (eax player, esi record) */
#define ADDR_CAM_MARKER 0xBBF20    /* its marker step (eax datum, ecx place) */
#define ADDR_KE_TICK_SLOT 0x41B774   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_OBJ_PLACE 0xB9960   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_OBJ_NEW 0xB9B70   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_CACHE_HEADER 0x556590   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_CACHE_TAGS_PTR 0x556D90   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_ACTOR_EQUIP 0x1E8750   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_ACTOR_NEW 0x1E8410   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_ACTOR_ON 0x1E9460   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_RECONNECT_DIALOG 0x192770   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_SQUADS_REFRESH 0x20A0B0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_AFTER_WINDOWS 0x13C00   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_ACTORS_PTR 0x5020C8   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_DAMAGE 0xDA0D0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_AI_PEACE 0x5020C4   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_GAME_TICK 0x13AAC0    /* 6h.25: one game tick (game_loop_tick), void(void) */
#define ADDR_FRAME_DRAW 0x229C50    /* 6h.25: the frame's draw (views, menus, vblank, present), void(void) */
#define ADDR_VBLANK_COUNT 0x4922D0   /* 6i: the vblank count (u64; the vblank interrupt adds 1), read by the frame pacing (v1.5 0x12dbf0, the byte read at 0x12dc82) */
#define ADDR_REFRESH_HZ 0x4922E0     /* 6i: the display refresh rate, s16 (60; 50 on a PAL 50 Hz picture) */
#define ADDR_VBLANK_WAIT 0x12DDF0   /* 6i.3: as v1.0's - called from the draw at 0x229c98 */
#define ADDR_PRESENT 0x13EF0        /* 6i.3: as v1.0's (ret 8) - called from the draw at 0x229ca6 */
#define ADDR_DISPLAY_CONFIG 0x448E40
#define ADDR_VIS_POOL 0x556E18      /* 6i.4: as v1.0's (function_1320f0's twin 0x134c00) */
#define ADDR_BSP_RENDER_PTR 0x4ECC18   /* 6i.4: as v1.0's (0x135ea0 / 0x136030) */
#define ADDR_LIGHTS_PTR 0x4ECBDC    /* 6i.4: as v1.0's (0x31500) */
#define ADDR_RENDER_CAMERA 0x4C65C0
#define ADDR_VIEW_FWD 0x4C6640      /* 6i.5: as v1.0's (0x1362a0) */
#define ADDR_PROJ_SCALE 0x4C66F0    /* 6i.5 */

/* 6i.6: Direct3D's own functions the "Direct3D probes" XBEs detour (hooks.S dv_*; names from the decompilation's calls) */
#define ADDR_DV_MAKESPACE 0x404BC0  /* D3DDevice_MakeSpace: pushbuffer room - waits while the GPU is behind (ret 4; v1.0 0x3faae0) */
#define ADDR_DV_BUSYWAIT 0x404E30   /* D3DResource_BlockUntilNotBusy: waits for the GPU to finish with a resource */
#define ADDR_DV_ISBUSY 0x404D10     /* the busy check the game polls in its waiting loops (edx = the resource) */
#define ADDR_DV_VISTEST 0x402710    /* D3DDevice_GetVisibilityTestResult */
#define ADDR_DV_DRAWIDX 0x405080    /* D3DDevice_DrawIndexedVertices */
#define ADDR_DV_DRAW 0x404FB0       /* D3DDevice_DrawVertices */
#define ADDR_DV_SETTEX 0x402400     /* D3DDevice_SetTexture */
#define ADDR_DV_SETPS 0x405600      /* D3DDevice_SetPixelShader */
#define ADDR_DV_SETPSPROG 0x4044B0  /* D3DDevice_SetPixelShaderProgram */
#define ADDR_DV_VSCONST 0x401410    /* D3DDevice_SetVertexShaderConstantFast */
#define ADDR_D3D_DEVICE 0x411568    /* the device pointer those functions' first instructions read */
#define ADDR_D3D_VSC 0x40F0C0
#define ADDR_FP_USERS_PTR 0x4F6498  /* 6i.9: as v1.0's (0x168ba3 etc.) */
#define ADDR_PASS_DRAW 0x3BC20     /* 6i.12: function_3bc30 - a render pass's records drawn (v1.0 called at 0x445b2 in function_44550, v1.5 0x445a2) */
#define ADDR_FX_A 0x25D30          /* 6i.13: function_25d50 - the screen effect chosen by ADDR_FX_MODE, before pass 16 (called v1.0 0x2c0d1 / v1.5 0x2c0b1) */
#define ADDR_FX_B 0x25DA0          /* 6i.13: function_25dc0 - the second screen effect (cl, dl flags; called v1.0 0x2c156, 0x2c1cb / v1.5 0x2c136, 0x2c1ab) */
#define ADDR_FX_MODE 0x491F6C       /* 6i.13: g_48574c - the screen effect mode (1-5); 3 also draws pass 16 */
/* 6i.18: the main scene's calls marked (hooks.S MARK_HOOK) */
#define ADDR_MK_41 0x3EDD0
#define ADDR_MK_43 0x3C3B0
#define ADDR_MK_49 0x1E350
#define ADDR_MK_56 0x13850
#define ADDR_MK_57 0x13610
#define ADDR_MK_60 0x352D0
#define ADDR_MK_61 0x14BA0
/* 6i.24: what a draw is made with - the game's bound texture and its size per stage (function_14390), Direct3D's texture stage states (0x80 a stage) */
#define ADDR_TSS 0x40EEB0
#define ADDR_ANISO_SET 0x18980    /* 6i.25: function_189a0 - a material's per-stage sampler settings (edx state, 1 arg, ret 4) */
/* 6i.35: a particle system's per-frame update (function_174a30, ecx = the system, one stack argument - the frame's
 * seconds -, ret 4), called from the particle-systems loop function_174990 at v1.0 0x174a01 / v1.5 0x1778a1 */
#define ADDR_PTC_UPDATE 0x1778D0
/* 6i.40: the split-screen detail settings - function_2c490 (eax = local players) and what it sets: the max LoD (s32),
 * six flags (bytes: +0 special light shapes / objects' dynamic lighting, +1 unread, +2 / +3 / +4 render passes 8 / 2 / 17,
 * +5 lighting), a distance scale (float), object detail priority (float) */
#define ADDR_COOP_DETAIL 0x2C470
#define ADDR_COOP_LOD 0x4C683C
#define ADDR_COOP_FLAGS 0x4C6840
#define ADDR_COOP_DIST 0x4C6848
#define ADDR_COOP_PRIO 0x473644
/* 6i.42: triple buffering (the triple_buffering XBEs) - Direct3D's CreateDevice (present parameters: its stack argument)
 * and Reset (present parameters: ebx), called from the device setup function_128c0; D3DResource_AddRef / _Release
 * (stdcall, the resource); the game's primary targets: textures [0], [1], then surfaces [0], [1] */
#define ADDR_D3D_CREATE 0x3FF320
#define ADDR_D3D_RESET 0x4018A0
#define ADDR_D3D_ADDREF 0x4056E0
#define ADDR_D3D_RELEASE 0x405720
#define ADDR_RT_PRIMARY 0x515E34
/* 6i.55: the game's render state cache (function_222d0 writes [state * 4 + this]) - Wireframe on puts the offsets back */
#define ADDR_RS_CACHE 0x4C4A1C
/* 6i.58: the game's indexed-draw wrapper (stdcall, 3 arguments) - the water plane's draw goes through it */
#define ADDR_WATER_DRAW 0x1C880
/* 6i.59: Water Refraction - the copy's wrapper (function_3bea0, void), its "copied this frame" byte, function_14bc0's
 * target surface getter (eax target -> eax), its common path after the cases, the screen's depth surface pointer */
#define ADDR_WATER_COPY 0x3BE90
#define ADDR_WATER_COPIED 0x515F10
#define ADDR_TARGET_SURFACE 0x1DCD0
#define ADDR_TARGET_COMMON 0x14D13
#define ADDR_MAIN_DEPTH 0x515E44
/* 6i.61: Render Scale - the world view's draw (function_2b790), the main target's index (g_4858b8), the camera storage's
 * view rectangle (g_4b9dd0: top, left, bottom, right) */
#define ADDR_WORLD_VIEW 0x2B770
#define ADDR_MAIN_TARGET 0x4920D8
#define ADDR_CAM_RECT 0x4C65F0
/* 6i.63: Render Scale - the main scene (function_2bcd0) */
#define ADDR_SCENE 0x2BCB0
/* 6i.65: Render Scale - Direct3D's SetViewport */
#define ADDR_SET_VIEWPORT 0x4021E0
/* 6i.66: Render Scale - the projection builder (function_2fd90: esi view, ebx out, edx clip) */
#define ADDR_PROJ 0x2FD70
/* 6i.68: Render Scale - the screen-mapping constants (function_12fa0: ecx camera, eax projection, push scaled, mode; ret 8) */
#define ADDR_VIEW_CONSTS 0x12F80
/* 6i.69: Render Scale - the scene's second interface step (function_40870) */
#define ADDR_UI2 0x40860
/* 6i.71: Stretch Point (test) 2 - the call after the second interface step (function_20770) */
#define ADDR_AFTER_UI 0x20750
/* 6i.72: Particles - the sprite emitter (function_23ed30) */
#define ADDR_SPRITE 0x248020
/* 6i.73: Particles - Direct3D's simple render-state setter (ecx method, edx value) and its shadows */
#define ADDR_RS_SIMPLE 0x3FFDD0
#define ADDR_RS_ALPHAFUNC 0x411660
#define ADDR_RS_ALPHATEST 0x411668
#define ADDR_RS_ALPHAREF 0x41166C
/* 6i.73: the camera's projection (function_2fd90's result for the camera storage; half width / height at +0xb8 / +0xbc) */
#define ADDR_PROJ_STORE 0x4C6634
/* 6i.77: Dynamic Lights - the per-light drawing (function_31170) */
#define ADDR_LIGHT 0x31150
/* 6i.79: the render target the current pass draws to (g_4858b4, set by draw_light_source_pass) - logging */
#define ADDR_PASS_TARGET 0x4920D4
/* 6i.81: Lights & Shadows - an object's shadow (function_32350), its shadow camera (g_47fd18) and size (g_47fda0) */
#define ADDR_SHADOW 0x32340
#define ADDR_SHADOW_CAM 0x48C530
#define ADDR_SHADOW_SIZE 0x48C5B8
/* 6i.82: the game's table of the texture bound at each stage (g_51f3c8[1]) - logging */
#define ADDR_BOUND_TEX 0x52DFA8
/* 6i.83: Screen Reads - Direct3D's texture stage states (stage 0's ADDRESSU; 0x80 a stage) and its dirty flags */
#define ADDR_TEX_STATE 0x40EEB0
#define ADDR_TEX_DIRTY 0x411570
/* 6i.86: Light Filtering - the dynamic lights' loop (function_31c80) */
#define ADDR_LIGHTLOOP 0x31C70
/* 6i.90: the GPU's zpass count and clock - Direct3D's EndVisibilityTest (eax the test; ret) and the push segment size */
#define ADDR_VT_END 0x4026B0
#define ADDR_PUSH_SEG 0x412728
/* 6i.92: function_1cd30's two Direct3D render target calls (one argument, ret 4 / two, ret 8) */
#define ADDR_RT_FAST 0x401C10
#define ADDR_RT_SET 0x401950
/* 6i.93: the tag table pointer (function_31170: [it + (tag & 0xffff) * 16 + 8] - a light's definition) */
#define ADDR_TAG_TABLE 0x4F0414
/* 6i.113: Direct3D's D3D__RenderState array (the game's setter writes it: function_0222d0) */
#define ADDR_D3D_RS 0x411578u
/* 6i.121: Force Double Revert - game_state_globals.slot (short), +2 / +3 (a checkpoint / the one before it), the slots' "written" bytes; Tick Rate - the per-object collision rebuild (ecx object) */
#define ADDR_CP_SLOT 0x4F0434   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_CP_FLAG2 0x4F0432   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_CP_VALID 0x50EBCA   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_HAVOK_REBUILD 0x1CC3F0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.122: window_manager_any_window_in_use (no arguments, al) - the free camera gives the pad to the game's UI */
#define ADDR_UI_WINDOW_IN_USE 0x14BD64   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.124: Trigger Volume Overlay - the scenario (g_4e0350), the point-in-volume test (eax, edi; al), the volume's placement (eax, ebx; al) */
#define ADDR_SCENARIO 0x4ECC20   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_TVT 0x11EBA0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_TV_MATRIX 0x11EAB0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.128: the xbox animation cache - its loads pool, its memory, its physical object (rocket launcher animation fix) */
#define ADDR_ANIM_LOADS 0x4F0418   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_ANIM_MEM 0x4F0420   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_ANIM_PHYS 0x4F0424   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.130: the game's kernel imports (thunk slots): MmQueryStatistics (181), MmAllocateContiguousMemory (165), MmFreeContiguousMemory (171) */
#define ADDR_KT_MMQS 0x41B764   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_KT_MMACM 0x41B604   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_KT_MMFCM 0x41B60C   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.133: more kernel imports (thunk slots): MmAllocateContiguousMemoryEx (166), HalReadWritePCISpace (46), HalReturnToFirmware (49) */
#define ADDR_KT_MMACMEX 0x41B640   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_KT_HALPCI 0x41B7E0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_KT_HALRTF 0x41B614   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.134: supersampling - the scene (function_13680; bl: skip its effects), the screen copy's draw (function_34a90);
 * on (v1.5: not mapped - its addresses unchecked, the hook not placed) */
#define ADDR_SCENE_FN 0x13660
#define ADDR_SS_COPYDRAW 0x34A80
#define ADDR_SS_ON 1   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.135: XLaunchNewImageA (stdcall: the path, the launch data) - the restart for Toggle Triple Buffering */
#define ADDR_XLAUNCH 0x2DA6E2   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.139: target 3's surface and texture pointers (their Data: the depth buffer's memory); MmGetPhysicalAddress (173) */
#define ADDR_T3_SURF 0x515E50   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_T3_TEX 0x515E48   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_KT_MMGPA 0x41B7BC   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.144: Direct3D's dirty-state flags (bit 0x100: the viewport, re-sent with the screen-space offset at the next draw) */
#define ADDR_D3D_DIRTY 0x411570   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.147: the water plane's sizing (function_3ca90) and its mesh transform g_4c1b38 (v1.5: not hooked) */
#define ADDR_WATER_SIZE 0x3CA80
#define ADDR_WATER_XFORM 0x4CE358   /* v2.0: carried over from v1.0 by tools15/xmap.py */
/* 6i.148: Render Scale above 100 % (v1.5: not hooked) */
#define ADDR_BB_SURF 0x515E3C   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_BB_TEX 0x515E34   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_T24_TEX 0x515E4C   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_RT_TABLE 0x4C1378   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_RT_SIZE 0x1DD40
#define ADDR_RT_CACHE 0x52DFC4   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_SQUAD_ORDER 0x20B150   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_SQUAD_FOLLOW 0x20C710   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_SQUADS_PTR 0x52BE7C   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_UNIT_REQUEST 0xE8E50   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_PHYS_GLOBALS 0x4F2CF0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_TEX_PHYS 0x4F2D34   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_GEO_PHYS 0x4F2D6C   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_SND_PHYS 0x50EBE4   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_GEO_PROTECT 0x2DB4CA   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_SCREEN_EFFECT 0x2D120   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_FOG_SCALE 0x4C65B0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_RESET_MAP 0x556DFE   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_KEEPALIVE_DRAW 0x229CB0   /* v2.0: carried over from v1.0 by tools15/xmap.py */
#define ADDR_GAME_FINISH 0x13B3C0   /* v2.0: carried over from v1.0 (0x1388E0) - the Campaign Timer's split hook goes on into it */
#define ADDR_BINK_GLOBALS 0x4F5A58   /* v2.0: the video player's state (v1.0 0x4E9188) - the intro flicker fix reads it */
