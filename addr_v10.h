/* Halo 2 (Xbox) v1.0 addresses - from the decompilation, checked against the retail XBE */
#define H2_VERSION_TEXT "v1.0"
#define ADDR_XINPUTGETSTATE 0x409B6C
#define ADDR_TEXT_STYLE_CLEAR 0x13E8A0
#define ADDR_FONT_GET 0x1224C0
#define ADDR_DRAW_TEXT 0x1FA50
#define ADDR_DRAW_STRING_GLOBALS 0x4E73A0
#define ADDR_FONT_HANDLES 0x4E28F4
#define ADDR_CREATEFILEA 0x2D2750
#define ADDR_WRITEFILE 0x2D232E
#define ADDR_SETFILEPOINTER 0x2D2488
#define ADDR_CLOSEHANDLE 0x2D1F3A
#define ADDR_FLAG_485602 0x485602
#define ADDR_FLAG_51080C 0x51080C
/* superbounce HUD (step 2) */
#define ADDR_GAME_TIME_PTR 0x510C54
#define ADDR_OBJECTS_PTR 0x4E0300
#define ADDR_LOCAL_PLAYERS_PTR 0x4E8C20
#define ADDR_PLAYERS_PTR 0x4E8C24
#define ADDR_HAVOK_COMPONENTS_PTR 0x51E9B8
#define ADDR_HAVOK_WORLD_PTR 0x51E9A4
#define ADDR_GAME_OPTIONS_PTR 0x4E6948   /* game options: +8 == 1 is campaign (function_12b790 checks it) */
#define ADDR_AI_GLOBALS_PTR 0x4F55D0   /* s_ai_globals*: +1 active (gates the AI update; ai_dispose_from_old_map) */
#define ADDR_VIEW_BUILD 0x223080   /* function_223080: builds a view from a player's camera record (its last stack argument) */
#define ADDR_FP_SETUP 0x3D2C0     /* function_3d2c0: the view's first-person object and weapon models */
#define ADDR_FP_OBJECT 0x4C1BD0    /* g_4c1bd0: the view's first-person object (hidden; NONE = third person) */
#define ADDR_FP_MODELS 0x4C1BD4    /* g_4c1bd4: its first-person weapon models (0 = none drawn) */
#define ADDR_VIEW_USER 0x4B9ED8    /* g_4b9ed8: the player whose view is being drawn */
#define ADDR_FIRE 0x102FB0          /* function_102fb0(weapon, barrel), stdcall: a weapon firing a shot */
#define ADDR_INF_AMMO_FN 0x106030   /* function_106030(weapon): "infinite ammo?" - multiplayer game types only */
#define ADDR_TRAIT_FN 0x159DD0      /* the game-type lookup it calls (its call at +0x52) */
#define ADDR_FP_MARKER 0x16640F   /* first_person_weapon_get_marker(ecx = object; marker, position, forward, up): ret 0x10, al */
#define ADDR_PERSPECTIVE 0x155760   /* function_155760: the director's perspective for a player (eax = player; 0 first person, 3 following) */
#define ADDR_VT_GSK 0x412C48   /* the convex pair (GSK) agent's vtable: count +0x18, separating normal +0x20, TIM +0x2C (convex_pair_2ee2e0) */
#define ADDR_VT_SWEEP 0x412290   /* convex_pair_2dae40: a GSK subclass (the welder's children, seen in game) - the same TIM layout */
#define ADDR_VT_WELD 0x412F5C    /* the collection (welder) agent: {key, ?, agent} children at +0x0C / +0x10 (seen in game) */
#define ADDR_VT_XFA 0x412BA4    /* the transform agent (pair_agent_2ed170): its child agent at +0x0C */
#define ADDR_VT_XFB 0x412BC4    /* the same, the shapes the other way round */
#define ADDR_VT_LIST 0x412C08   /* the list agent (list_2eda30): one child agent per child of a list shape (type 10) - [+0x0C], +0x10 of them */
#define ADDR_VT_BVA 0x412A98    /* a BV-tree collection agent (bounded_2e9530): {key, ?, agent} entries at [+0x0C], +0x10 of them */
#define ADDR_VT_BVB 0x412ADC    /* its subclass (derived_2eb4e0) - seen inside the level's welder by the Cairo computers / trees */
#define ADDR_SCENERY_TABLE 0x51E9CC   /* pointer to u32[]: the object datum per scenery index (level-shape keys of category 3 / 4: key & 0xFFFF) - function_1ef3e0 */
#define ADDR_SAVE_MAP 0x547F70   /* main_globals.save_map: the checkpoint save request (Yelo's save button sets it) */
#define ADDR_MAP_NAME 0x5478BC
#define ADDR_READFILE 0x2D2241
#define ADDR_KT_FLUSH 0x4115D0   /* the kernel import thunk table's slot for NtFlushBuffersFile (ordinal 198): its address after loading */
#define ADDR_DATA_LO 0x461180
#define ADDR_DATA_HI 0x563B78
/* drawing (step 3a): the game's own flat-colour overlay recipe (0x40890) and the frame camera */
#define ADDR_RENDER_STATE 0x222D0
#define ADDR_VS_SELECT 0x1CC30
#define ADDR_VS_CACHE 0x1C710
#define ADDR_PS_SET 0x15180
#define ADDR_COMMIT 0x1CF50
#define ADDR_VS_CONST 0x3F7410
#define ADDR_VERTEX2S 0x3FB330
#define ADDR_BEGIN 0x3FB440
#define ADDR_END 0x3FB480
#define ADDR_VS_CACHE_ARG 0x51F0F0
#define ADDR_PS_DEF 0x484F68
#define ADDR_FRAME_STORAGE 0x485600
#define ADDR_CAMERA_STORAGE 0x4B9DA0
#define ADDR_HUD_VIEW 0x2C790
#define ADDR_PLAYER_CAMERAS 0x4E9C8C
#define ADDR_WIDESCREEN_FN 0x16A90
#define ADDR_WIDESCREEN_FLAG 0x485AC2
#define ADDR_ASPECT_FLAGS_PTR 0x510C50
/* step 3b: the current level's collision BSP (g_4e033c, the leaf search's bsp) */
#define ADDR_COLLISION_BSP 0x4E033C
/* step 3c: the current structure BSP (g_4e0348): instanced geometry instances +0x140, definitions +0x138 */
#define ADDR_STRUCTURE_BSP 0x4E0348
/* step 3c.3: the tag instances (g_4e3b44: 16 bytes each - group +0x00, data +0x08) */
#define ADDR_TAG_INSTANCES 0x4E3B44
#define ADDR_OOB_TEST 0xCC460       /* the out-of-bounds test: __stdcall bool (unit) - its body inside solid (collision 0x14800005) */
#define ADDR_VT_DEACT_SPATIAL 0x414ABC   /* Havok's spatial deactivator (short test slot 5, long slot 6) */
#define ADDR_VT_DEACT_FAKE 0x415100      /* Havok's fake deactivator: never still */
#define ADDR_VT_DEACT_HALO 0x455950      /* Halo's subclass: the veto, then Havok's tests */
#define ADDR_DEACT_VETO 0x1D1910         /* Halo's veto: bool (deactivator in eax) - reads only */
#define ADDR_HUD_DRAW 0x1917EB           /* the HUD for the view being drawn (items, messages, the player HUD): void(void) */
#define ADDR_PAD_HANDLES 0x4E61CC        /* the four controller ports' XInput handles */
#define ADDR_PAD_PREFS 0x51EA18          /* the gamepad preferences, 0x1C a port: +0x1A look inverted */
#define ADDR_VIRTUALALLOC 0x3312CC       /* XAPI VirtualAlloc (__stdcall): the predicted spots' memory, at run time */
#define ADDR_VIRTUALFREE 0x3312FA
#define ADDR_GETFILESIZE 0x2D2715
#define ADDR_SKULLS 0x4F55DC            /* bool[16]: the skulls (0 Envy ... 14 Whuppopotamus), read by every skull effect */
#define ADDR_REGION_DMG 0xDAAA0        /* damage to one of an object's regions (its parts): __stdcall(info, object, owner, region, damage, accumulator), ret 0x18 */
#define ADDR_SET_UNIT 0x14CAD0          /* player_set_unit(player [pushed], unit [ebx]), ret 4 - the game's own possession */
#define ADDR_KILL 0xDC0A0              /* the death a kill volume gives: __stdcall(object), ret 4 - the death damage (damage globals +0x14) through a full damage report: a normal death */
#define ADDR_ANAMORPHIC 0x45DFDC        /* float 4/3: the anamorphic scaling factor (Halo 2 HD's DisableAnamorphicScaling writes 1.0 here) - read-only data */
/* step 6h: the HUD Widescreen Fix - the anchor function, the widget drawers and what they draw with */
#define ADDR_HUD_ANCHOR 0x1396C7    /* the HUD anchor point: eax anchor (0 TR, 1 TL, 2 BL, 3 BR, 4 centre, else the aim point), esi float[2] out */
#define ADDR_D_GRID 0x22B8E2        /* the widget drawers: shader grid, bitmap, text */
#define ADDR_D_BMP 0x22AFF1
#define ADDR_D_TEXT 0x22AA16
#define ADDR_FRAME_WINDOW 0x485648  /* frame storage +0x48: the window, rect16 {top, left, bottom, right} */
#define ADDR_HUD_CENTRE 0x200462    /* the centre element's draw (anchor 4): ret */
#define ADDR_HUD_QUAD 0x22A664      /* a textured quad: ecx its 4 corners {x, y}, eax its texture box; ret 0xc */
#define ADDR_GRID_DRAW 0x2D140     /* a shader widget: 4th argument its 3x3 grid of {x, y, u, v} */
#define ADDR_TEXT_DRAW 0x1FB10     /* a string: ecx text, (rect16 *, float scale) - ret 8 */
#define ADDR_HUD_VIEWPORT_X0 0x4B9DD2  /* camera storage +0x32: the HUD view's viewport left */
#define ADDR_HUD_FRAME_X0 0x4B9DDA  /* camera storage +0x3A: the HUD view's safe frame left (the bottom-left anchor's x) */
#define ADDR_TRACKER 0x1A2823       /* the motion tracker's draw (eax its point, ecx the player): ret */
#define ADDR_DIRECTOR 0x1554B0     /* 6i.48: the director's frame update (__stdcall, the frame's seconds), from the frame at 0x12b647 */
#define ADDR_FILLMODE 0x3F5BA0     /* 6i.50: Direct3D SetRenderState_FillMode (__stdcall, the mode), called by the frame setup at 0x13404 */
#define ADDR_SET_VDATA4F 0x3FB2D0   /* D3D SetVertexData4f (edi register, 4 floats): ret 0x10 */
#define ADDR_CAM_MODE 0x155810      /* the director's first-person / following choice for a unit (ebx; one argument): ret 4 */
#define ADDR_MAIN_SAVE 0x12B790     /* main_save_map: the save request, from the checkpoint logic */
#define ADDR_DEATH_REVERT 0x138E40  /* the once-a-tick revert after you die (sets revert_map) */
#define ADDR_CAM_TARGET 0x1871E0    /* the following camera's target (eax player, esi record) */
#define ADDR_CAM_MARKER 0xB9EF0    /* its marker step (eax datum, ecx place) */
#define ADDR_GAME_FINISH 0x1388E0  /* 6i.156: game_finish, void(void) - the script's game_won calls it at 0x2a99b0 (the Campaign Timer's split) */
#define ADDR_KE_TICK_SLOT 0x411690  /* 6i.156: the kernel import slot holding &KeTickCount (ordinal 156; the game's GetTickCount at 0x3314b0 reads it): the console's milliseconds - the Campaign Timer's clock */
#define ADDR_OBJ_PLACE 0xB7930     /* 6i.158: Object Spawning - the creation record (0xC4 bytes, esi) filled from a tag: (tag, owner object, 0) on the stack, ret 0xC */
#define ADDR_OBJ_NEW 0xB7B40       /* ... and the object made from it: stdcall (record), the object's index or -1 */
#define ADDR_CACHE_HEADER 0x547700 /* ... the map file's header as the game keeps it (cache_file_globals 0x5476f8 + 8; 0x800 bytes; ADDR_MAP_NAME is its +0x1BC) */
#define ADDR_CACHE_TAGS_PTR 0x547F00  /* ... the tags header: the scenario's tag +0xC, the tag count +0x18 (function_122c10 reads it) */
#define ADDR_ACTOR_EQUIP 0x1E04A0  /* 6i.159: a dropped biped's brain - health / shield from the character and its weapon: eax the weapon's tag; (character, unit, second weapon, grenade) on the stack, ret 0x10 */
#define ADDR_ACTOR_NEW 0x1E0160    /* ... the actor for a unit: eax the squad (-1), edx the character; (unit, flag) on the stack, ret 8; the actor or -1 */
#define ADDR_ACTOR_ON 0x1E11B0     /* ... switched on: eax the actor, cl 1 */
#define ADDR_RECONNECT_DIALOG 0x18F973  /* 6i.160: one of the controllers' Reconnect Controller dialogs is showing - void, al (function_18f973) */
#define ADDR_SQUADS_REFRESH 0x201DF0  /* 6i.163: function_201df0, void(void) - what ai_attach (0x273040) calls after putting a unit into a squad */
#define ADDR_AFTER_WINDOWS 0x13C20  /* 6i.167: function_13c20, void(void) - the closing call (0x2c889) of the pass that draws the HUD text and then the game's windows: the menu drawn before it when a window is up */
#define ADDR_ACTORS_PTR 0x4F55F0  /* 6i.170: the actors' pool (*: +0x20 count, +0x44 data, 0x888 bytes each; an actor's team at +0x24, its unit at +0x18) */
#define ADDR_DAMAGE 0xD7B80       /* 6i.170: the damage function (stdcall: the event, the object hit, +4 more; ret 0x18) - One Shot Kills hooks its 20 calls */
#define ADDR_AI_PEACE 0x4F55EC   /* 6i.171: the AI globals whose +0xC4 is the teams' peace table (bit team x 16 + team; v1.0 0x1df560 reads it) */
#define ADDR_GAME_TICK 0x137FE0    /* 6h.25: one game tick (game_loop_tick), void(void) */
#define ADDR_FRAME_DRAW 0x223240    /* 6h.25: the frame's draw (views, menus, vblank, present), void(void) */
#define ADDR_VBLANK_COUNT 0x485AB0   /* 6i: the vblank count (u64; the vblank interrupt adds 1), read by the frame pacing (v1.0 0x12b0e0, the byte read at 0x12b172) */
#define ADDR_REFRESH_HZ 0x485AC0     /* 6i: the display refresh rate, s16 (60; 50 on a PAL 50 Hz picture) */
#define ADDR_VBLANK_WAIT 0x12B2E0   /* 6i.3: the frame pacing's wait for its vblank, void(void) - called from the draw at 0x223288 */
#define ADDR_PRESENT 0x13F10        /* 6i.3: the frame's present, __stdcall(0, 0) (ret 8) - called from the draw at 0x223296 */
#define ADDR_DISPLAY_CONFIG 0x43ED80
#define ADDR_VIS_POOL 0x547F88      /* 6i.4: the main view's visibility pool (function_1320f0, mode 0): +0xC four lists - 0 clusters, 1 dynamic lights, 2 objects, 3 instanced pieces */
#define ADDR_BSP_RENDER_PTR 0x4E0348   /* 6i.4: the level's render data: +0x9C/+0xA0 clusters (0xB0 each, box +0x54), +0x140/+0x144 instanced pieces (0x58 each, centre +0x3C, radius +0x48) */
#define ADDR_LIGHTS_PTR 0x4E030C    /* 6i.4: the dynamic lights' data array (+0x44 data, 0x110 each: centre +0x18, radius +0x24) */
#define ADDR_RENDER_CAMERA 0x4B9DA0
#define ADDR_VIEW_FWD 0x4B9E20      /* 6i.5: the view's depth plane - x, then y +0xC, z +0x18, the constant +0x24 (function_133790) */
#define ADDR_PROJ_SCALE 0x4B9ED0    /* 6i.5: pixels a unit at depth 1 (function_133790: g_4b9ed0 / depth x radius x 2 = size on screen) */

/* 6i.6: Direct3D's own functions the "Direct3D probes" XBEs detour (hooks.S dv_*; names from the decompilation's calls) */
#define ADDR_DV_MAKESPACE 0x3FAAE0  /* D3DDevice_MakeSpace: pushbuffer room - waits while the GPU is behind (ret 4) */
#define ADDR_DV_BUSYWAIT 0x3FAD50   /* D3DResource_BlockUntilNotBusy: waits for the GPU to finish with a resource */
#define ADDR_DV_ISBUSY 0x3FAC30     /* the busy check the game polls in its waiting loops (edx = the resource) */
#define ADDR_DV_VISTEST 0x3F8630    /* D3DDevice_GetVisibilityTestResult */
#define ADDR_DV_DRAWIDX 0x3FAFA0    /* D3DDevice_DrawIndexedVertices */
#define ADDR_DV_DRAW 0x3FAED0       /* D3DDevice_DrawVertices */
#define ADDR_DV_SETTEX 0x3F8320     /* D3DDevice_SetTexture */
#define ADDR_DV_SETPS 0x3FB520      /* D3DDevice_SetPixelShader */
#define ADDR_DV_SETPSPROG 0x3FA3D0  /* D3DDevice_SetPixelShaderProgram */
#define ADDR_DV_VSCONST 0x3F7330    /* D3DDevice_SetVertexShaderConstantFast */
#define ADDR_D3D_DEVICE 0x407488    /* the device pointer those functions' first instructions read */
#define ADDR_D3D_VSC 0x404FE0
#define ADDR_FP_USERS_PTR 0x4E9BC8  /* 6i.9: -> the local users' first-person state (0x20CC each: +0xC two weapons of 0x1010 - flags +0 bit 0 active, weapon +4, node count +0x304, node matrices +0x308), unknown_165ce5.cpp */
#define ADDR_PASS_DRAW 0x3BC30     /* 6i.12: function_3bc30 - a render pass's records drawn (v1.0 called at 0x445b2 in function_44550, v1.5 0x445a2) */
#define ADDR_FX_A 0x25D50          /* 6i.13: function_25d50 - the screen effect chosen by ADDR_FX_MODE, before pass 16 (called v1.0 0x2c0d1 / v1.5 0x2c0b1) */
#define ADDR_FX_B 0x25DC0          /* 6i.13: function_25dc0 - the second screen effect (cl, dl flags; called v1.0 0x2c156, 0x2c1cb / v1.5 0x2c136, 0x2c1ab) */
#define ADDR_FX_MODE 0x48574C       /* 6i.13: g_48574c - the screen effect mode (1-5); 3 also draws pass 16 */
/* 6i.18: the main scene's calls marked (hooks.S MARK_HOOK) */
#define ADDR_MK_41 0x3EDE0
#define ADDR_MK_43 0x3C3C0
#define ADDR_MK_49 0x1E370
#define ADDR_MK_56 0x13870
#define ADDR_MK_57 0x13630
#define ADDR_MK_60 0x352E0
#define ADDR_MK_61 0x14BC0
/* 6i.24: what a draw is made with - the game's bound texture and its size per stage (function_14390), Direct3D's texture stage states (0x80 a stage) */
#define ADDR_TSS 0x404DD0
#define ADDR_ANISO_SET 0x189A0    /* 6i.25: function_189a0 - a material's per-stage sampler settings (edx state, 1 arg, ret 4) */
/* 6i.35: a particle system's per-frame update (function_174a30, ecx = the system, one stack argument - the frame's
 * seconds -, ret 4), called from the particle-systems loop function_174990 at v1.0 0x174a01 / v1.5 0x1778a1 */
#define ADDR_PTC_UPDATE 0x174A30
/* 6i.40: the split-screen detail settings - function_2c490 (eax = local players) and what it sets: the max LoD (s32),
 * six flags (bytes: +0 special light shapes / objects' dynamic lighting, +1 unread, +2 / +3 / +4 render passes 8 / 2 / 17,
 * +5 lighting), a distance scale (float), object detail priority (float) */
#define ADDR_COOP_DETAIL 0x2C490
#define ADDR_COOP_LOD 0x4BA01C
#define ADDR_COOP_FLAGS 0x4BA020
#define ADDR_COOP_DIST 0x4BA028
#define ADDR_COOP_PRIO 0x4670E4
/* 6i.42: triple buffering (the triple_buffering XBEs) - Direct3D's CreateDevice (present parameters: its stack argument)
 * and Reset (present parameters: ebx), called from the device setup function_128c0; D3DResource_AddRef / _Release
 * (stdcall, the resource); the game's primary targets: textures [0], [1], then surfaces [0], [1] */
#define ADDR_D3D_CREATE 0x3F5240
#define ADDR_D3D_RESET 0x3F77C0
#define ADDR_D3D_ADDREF 0x3FB600
#define ADDR_D3D_RELEASE 0x3FB640
#define ADDR_RT_PRIMARY 0x509354
/* 6i.55: the game's render state cache (function_222d0 writes [state * 4 + this]) - Wireframe on puts the offsets back */
#define ADDR_RS_CACHE 0x4B81FC
/* 6i.58: the game's indexed-draw wrapper (stdcall, 3 arguments) - the water plane's draw goes through it */
#define ADDR_WATER_DRAW 0x1C8A0
/* 6i.59: Water Refraction - the copy's wrapper (function_3bea0, void), its "copied this frame" byte, function_14bc0's
 * target surface getter (eax target -> eax), its common path after the cases, the screen's depth surface pointer */
#define ADDR_WATER_COPY 0x3BEA0
#define ADDR_WATER_COPIED 0x509430
#define ADDR_TARGET_SURFACE 0x1DCF0
#define ADDR_TARGET_COMMON 0x14D33
#define ADDR_MAIN_DEPTH 0x509364
/* 6i.61: Render Scale - the world view's draw (function_2b790), the main target's index (g_4858b8), the camera storage's
 * view rectangle (g_4b9dd0: top, left, bottom, right) */
#define ADDR_WORLD_VIEW 0x2B790
#define ADDR_MAIN_TARGET 0x4858B8
#define ADDR_CAM_RECT 0x4B9DD0
/* 6i.63: Render Scale - the main scene (function_2bcd0) */
#define ADDR_SCENE 0x2BCD0
/* 6i.65: Render Scale - Direct3D's SetViewport */
#define ADDR_SET_VIEWPORT 0x3F8100
/* 6i.66: Render Scale - the projection builder (function_2fd90: esi view, ebx out, edx clip) */
#define ADDR_PROJ 0x2FD90
/* 6i.68: Render Scale - the screen-mapping constants (function_12fa0: ecx camera, eax projection, push scaled, mode; ret 8) */
#define ADDR_VIEW_CONSTS 0x12FA0
/* 6i.69: Render Scale - the scene's second interface step (function_40870) */
#define ADDR_UI2 0x40870
/* 6i.71: Stretch Point (test) 2 - the call after the second interface step (function_20770) */
#define ADDR_AFTER_UI 0x20770
/* 6i.72: Particles - the sprite emitter (function_23ed30) */
#define ADDR_SPRITE 0x23ED30
/* 6i.73: Particles - Direct3D's simple render-state setter (ecx method, edx value) and its shadows */
#define ADDR_RS_SIMPLE 0x3F5CF0
#define ADDR_RS_ALPHAFUNC 0x407580
#define ADDR_RS_ALPHATEST 0x407588
#define ADDR_RS_ALPHAREF 0x40758C
/* 6i.73: the camera's projection (function_2fd90's result for the camera storage; half width / height at +0xb8 / +0xbc) */
#define ADDR_PROJ_STORE 0x4B9E14
/* 6i.77: Dynamic Lights - the per-light drawing (function_31170) */
#define ADDR_LIGHT 0x31170
/* 6i.79: the render target the current pass draws to (g_4858b4, set by draw_light_source_pass) - logging */
#define ADDR_PASS_TARGET 0x4858B4
/* 6i.81: Lights & Shadows - an object's shadow (function_32350), its shadow camera (g_47fd18) and size (g_47fda0) */
#define ADDR_SHADOW 0x32350
#define ADDR_SHADOW_CAM 0x47FD18
#define ADDR_SHADOW_SIZE 0x47FDA0
/* 6i.82: the game's table of the texture bound at each stage (g_51f3c8[1]) - logging */
#define ADDR_BOUND_TEX 0x51F3D8
/* 6i.83: Screen Reads - Direct3D's texture stage states (stage 0's ADDRESSU; 0x80 a stage) and its dirty flags */
#define ADDR_TEX_STATE 0x404DD0
#define ADDR_TEX_DIRTY 0x407490
/* 6i.86: Light Filtering - the dynamic lights' loop (function_31c80) */
#define ADDR_LIGHTLOOP 0x31C80
/* 6i.90: the GPU's zpass count and clock - Direct3D's EndVisibilityTest (eax the test; ret) and the push segment size */
#define ADDR_VT_END 0x3F85D0
#define ADDR_PUSH_SEG 0x408648
/* 6i.92: function_1cd30's two Direct3D render target calls (one argument, ret 4 / two, ret 8) */
#define ADDR_RT_FAST 0x3F7B30
#define ADDR_RT_SET 0x3F7870
/* 6i.93: the tag table pointer (function_31170: [it + (tag & 0xffff) * 16 + 8] - a light's definition) */
#define ADDR_TAG_TABLE 0x4E3B44
/* 6i.113: Direct3D's D3D__RenderState array (the game's setter writes it: function_0222d0) */
#define ADDR_D3D_RS 0x407498u
/* 6i.121: Force Double Revert - game_state_globals.slot (short), +2 / +3 (a checkpoint / the one before it), the slots' "written" bytes; Tick Rate - the per-object collision rebuild (ecx object) */
#define ADDR_CP_SLOT 0x4E3B64
#define ADDR_CP_FLAG2 0x4E3B62
#define ADDR_CP_VALID 0x5020F2
#define ADDR_HAVOK_REBUILD 0x1C3850
/* 6i.122: window_manager_any_window_in_use (no arguments, al) - the free camera gives the pad to the game's UI */
#define ADDR_UI_WINDOW_IN_USE 0x149227
/* 6i.124: Trigger Volume Overlay - the scenario (g_4e0350), the point-in-volume test (eax, edi; al), the volume's placement (eax, ebx; al) */
#define ADDR_SCENARIO 0x4E0350
#define ADDR_TVT 0x11C470
#define ADDR_TV_MATRIX 0x11C380
/* 6i.128: the xbox animation cache - its loads pool, its memory, its physical object (rocket launcher animation fix) */
#define ADDR_ANIM_LOADS 0x4E3B48
#define ADDR_ANIM_MEM 0x4E3B50
#define ADDR_ANIM_PHYS 0x4E3B54
/* 6i.130: the game's kernel imports (thunk slots): MmQueryStatistics (181), MmAllocateContiguousMemory (165), MmFreeContiguousMemory (171) */
#define ADDR_KT_MMQS 0x411680
#define ADDR_KT_MMACM 0x411524
#define ADDR_KT_MMFCM 0x41152C
/* 6i.133: more kernel imports (thunk slots): MmAllocateContiguousMemoryEx (166), HalReadWritePCISpace (46), HalReturnToFirmware (49) */
#define ADDR_KT_MMACMEX 0x411560
#define ADDR_KT_HALPCI 0x4116FC
#define ADDR_KT_HALRTF 0x411534
/* 6i.134: supersampling - the scene (function_13680; bl: skip its effects), the screen copy's draw (function_34a90);
 * on (v1.5: not mapped - its addresses unchecked, the hook not placed) */
#define ADDR_SCENE_FN 0x13680
#define ADDR_SS_COPYDRAW 0x34A90
#define ADDR_SS_ON 1
/* 6i.135: XLaunchNewImageA (stdcall: the path, the launch data) - the restart for Toggle Triple Buffering */
#define ADDR_XLAUNCH 0x2D07F2
/* 6i.139: target 3's surface and texture pointers (their Data: the depth buffer's memory); MmGetPhysicalAddress (173) */
#define ADDR_T3_SURF 0x509370
#define ADDR_T3_TEX 0x509368
#define ADDR_KT_MMGPA 0x4116D8
/* 6i.144: Direct3D's dirty-state flags (bit 0x100: the viewport, re-sent with the screen-space offset at the next draw) */
#define ADDR_D3D_DIRTY 0x407490
/* 6i.147: the water plane's sizing (function_3ca90) and its mesh transform g_4c1b38 (v1.5: not hooked) */
#define ADDR_WATER_SIZE 0x3CA90
#define ADDR_WATER_XFORM 0x4C1B38
/* 6i.148: Render Scale above 100 % - the screen-sized targets' header pointers (function_14600: the back buffers' surfaces
 * g_50935c[2], their texture views g_509354[2], the depth g_509364, its texture views g_50936c (target 24); target 3's are
 * ADDR_T3_SURF / ADDR_T3_TEX), the render-target table g_4b4b58 (0x98-byte entries; entry 1 = the light accumulation
 * target) and the target size getter function_01dd60 (eax index, edx &width, ecx &height, al result) */
#define ADDR_BB_SURF 0x50935C
#define ADDR_BB_TEX 0x509354
#define ADDR_T24_TEX 0x50936C
#define ADDR_RT_TABLE 0x4B4B58
#define ADDR_RT_SIZE 0x1DD60
/* 6i.149 (test): the game's render-target bind cache (function_1cd30: g_51f3f4 target, g_51f3f8 depth) */
#define ADDR_RT_CACHE 0x51F3F4
#define ADDR_SQUAD_ORDER 0x202E90   /* 6i.182: a squad given an order (the game's function_202e90): the order's number in bx; (squad, flag) on the stack, ret 8 */
#define ADDR_SQUAD_FOLLOW 0x204450  /* ... a squad's areas rebuilt around its follow target (function_204450): the order's number in eax; (squad) on the stack, ret 4 */
#define ADDR_SQUADS_PTR 0x51E9D8    /* ... the squads' pool (*: +0x20 count, +0x44 data, 0x98 bytes each: +0x08 actors, +0x0A alive, +0x2A the order, +0x80 the unit it follows) */
#define ADDR_UNIT_REQUEST 0xE6900    /* 6i.183: a unit told to do something (the game's function_e6900): the unit in eax, the request in edi; plain ret */
#define ADDR_PHYS_GLOBALS 0x4E6420   /* ... the game's memory region: the stage +0, each stage's used top +0xC[5] and limit +0x20[5] */
#define ADDR_TEX_PHYS 0x4E6464       /* ... * the texture cache's allocator (pages +0x30, page shift +0x34) */
#define ADDR_GEO_PHYS 0x4E649C       /* ... * the geometry cache's */
#define ADDR_SND_PHYS 0x50210C       /* ... * the sound cache's */
#define ADDR_GEO_PROTECT 0x2D15DA    /* 6i.191: XPhysicalProtect, as called at 0x12da80 in the geometry cache's set-up (block, size, protection) - stdcall */
#define ADDR_SCREEN_EFFECT 0x2D140   /* 6i.193: function_2d140 - a level's screen effect drawn over the picture (eax the tag; the weights, columns, rows, grid on the stack; ret 0x10) */
#define ADDR_FOG_SCALE 0x4B9D90      /* ... the float that turns a screen position into a place in its 320 x 240 pattern: the game's 0.5 */
#define ADDR_RESET_MAP 0x547F6E      /* 6i.201: main_globals.reset_map - the game's Restart Level flag (its pause menu writes 1: 0x2323b7) */
#define ADDR_KEEPALIVE_DRAW 0x2232A0  /* 6i.222: function_2232a0 - the loading code's frame (the video's, or the dark one once it is over); cdecl, no arguments */
#define ADDR_BINK_GLOBALS 0x4E9188   /* the video player's state (g_4e9188: +0 initialized, +4 finished, +0xC the movie) - the intro flicker fix reads it */
