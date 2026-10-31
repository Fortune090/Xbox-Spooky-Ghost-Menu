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
#define ADDR_SET_VDATA4F 0x4053B0   /* D3D SetVertexData4f (edi register, 4 floats): ret 0x10 */
#define ADDR_CAM_MODE 0x158340      /* the director's first-person / following choice for a unit (ebx; one argument): ret 4 */
#define ADDR_MAIN_SAVE 0x12E2A0     /* main_save_map: the save request, from the checkpoint logic */
#define ADDR_DEATH_REVERT 0x13B920  /* the once-a-tick revert after you die (sets revert_map) */
#define ADDR_CAM_TARGET 0x189FD0    /* the following camera's target (eax player, esi record) */
#define ADDR_CAM_MARKER 0xBBF20    /* its marker step (eax datum, ecx place) */
