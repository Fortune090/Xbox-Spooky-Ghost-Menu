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
#define ADDR_SET_VDATA4F 0x3FB2D0   /* D3D SetVertexData4f (edi register, 4 floats): ret 0x10 */
#define ADDR_CAM_MODE 0x155810      /* the director's first-person / following choice for a unit (ebx; one argument): ret 4 */
#define ADDR_MAIN_SAVE 0x12B790     /* main_save_map: the save request, from the checkpoint logic */
#define ADDR_DEATH_REVERT 0x138E40  /* the once-a-tick revert after you die (sets revert_map) */
#define ADDR_CAM_TARGET 0x1871E0    /* the following camera's target (eax player, esi record) */
#define ADDR_CAM_MARKER 0xB9EF0    /* its marker step (eax datum, ecx place) */
