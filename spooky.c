/* Spooky Ghost for Halo 2 (original Xbox, retail v1.0, title 4D530064): a Blam-style debug menu, in a modified default.xbe (v1.0 or the v1.5 title update).
 *
 * Back + D-pad Up opens and closes it. D-pad up/down selects, left/right changes a value, A toggles or
 * activates, B closes. While it is open the game sees a neutral controller, and after it closes the game
 * keeps seeing one until every button is released, so nothing leaks into gameplay.
 *
 * Text goes through the game's own renderer (c_1fa50::function_1fa50 at 0x1fa50), so it is the game's font,
 * drawn the way the game draws its own HUD text; the frame hook runs at the end of the frame, after the HUD
 * and the screen effects, so the menu sits on top of everything.
 *
 * Built two ways: for the Xbox (gcc -m32, freestanding, linked at the new XBE section's address), and with
 * -DHOST for the test harness on a PC, where the game's functions are stand-ins that record what is drawn. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;
typedef signed char s8;
typedef unsigned long long u64;
typedef signed long long s64;

typedef struct { u32 packet; u16 buttons; u8 analog[8]; s16 lx, ly, rx, ry; } xinput_state;   /* XINPUT_STATE */
typedef struct { s16 top, left, bottom, right; } rect16;                                      /* short_rectangle2d */
typedef struct { float a, r, g, b; } color4f;                                                 /* color4f: alpha first */

#define BTN_UP 0x0001
#define BTN_DOWN 0x0002
#define BTN_LEFT 0x0004
#define BTN_RIGHT 0x0008
#define BTN_BACK 0x0020
#define BTN_LS 0x0040
#define AB_A 0
#define AB_B 1
#define AB_BLACK 4                /* the Black button: analog, like the face buttons */
#define AB_WHITE 5                /* the White button */
#define ANALOG_DOWN 30            /* the Xbox face buttons are analog, 0-255 */

#ifdef HOST
#include <math.h>
u32 host_xinput_get_state(void *handle, xinput_state *st);
void host_text_style_clear(void);
void *host_font_get(u32 handle);
void host_draw_text(const u16 *text, const rect16 *bounds, float scale);
extern u8 host_draw_string_globals[0x64];
extern u32 host_font_handles[16];
#define XInputGetState_real host_xinput_get_state
#define text_style_clear host_text_style_clear
#define font_get host_font_get
#define DRAW_STRING_GLOBALS host_draw_string_globals
#define FONT_HANDLES host_font_handles
#define DRAW_TEXT(text, bounds, scale) host_draw_text(text, bounds, scale)
#define H2_VERSION_TEXT "host"
u32 host_mem32(u32 address);                   /* the test's simulated game memory */
u8 host_mem8(u32 address);
u32 host_read_file(const char *name, char *buffer, u32 size);
void host_write_file(const char *name, const char *data, u32 size);
#define ADDR_GAME_TIME_PTR 0x1000
#define ADDR_OBJECTS_PTR 0x1004
#define ADDR_LOCAL_PLAYERS_PTR 0x1008
#define ADDR_PLAYERS_PTR 0x100C
#define ADDR_HAVOK_COMPONENTS_PTR 0x1010
#define ADDR_HAVOK_WORLD_PTR 0x1014
#define ADDR_MAP_NAME 0x1100
#define ADDR_SAVE_MAP 0x1D00
#define ADDR_GAME_OPTIONS_PTR 0x1D10
#define ADDR_OOB_TEST 0x1D14
#define ADDR_SKULLS 0x1D80
#define ADDR_SET_UNIT 0x1D90
#define ADDR_KILL 0x1DA0
#define ADDR_ANAMORPHIC 0x1DB0
#define ADDR_FRAME_WINDOW 0x1248
#define ADDR_HUD_VIEWPORT_X0 0x124A
#define ADDR_HUD_FRAME_X0 0x1252
#define ADDR_VIRTUALALLOC 0x1D70
#define ADDR_VIRTUALFREE 0x1D74
#define ADDR_GETFILESIZE 0x1D78
#define ADDR_HUD_DRAW 0x1D40
#define ADDR_PAD_HANDLES 0x1D50
#define ADDR_PAD_PREFS 0x1D60
#define ADDR_VT_DEACT_SPATIAL 0x1D20
#define ADDR_VT_DEACT_FAKE 0x1D24
#define ADDR_VT_DEACT_HALO 0x1D28
#define ADDR_DEACT_VETO 0x1D2C
#define ADDR_AI_GLOBALS_PTR 0x1D20
#define ADDR_VT_GSK 0x4F000
#define ADDR_VT_SWEEP 0x4F010
#define ADDR_VT_WELD 0x4F100
#define ADDR_VT_XFA 0x4F020
#define ADDR_VT_XFB 0x4F030
#define ADDR_VT_LIST 0x4F040
#define ADDR_VT_BVA 0x4F050
#define ADDR_VT_BVB 0x4F060
#define ADDR_SCENERY_TABLE 0x1F900
#define ADDR_FP_OBJECT 0x1D30
#define ADDR_FP_MODELS 0x1D34
#define ADDR_VIEW_USER 0x1D38
#define ADDR_INF_AMMO_FN 0x49000
#define ADDR_TRAIT_FN 0x4A000
#define ADDR_DATA_LO 0x20000
#define ADDR_DATA_HI 0x21000
#define ADDR_FRAME_STORAGE 0x1200
#define ADDR_CAMERA_STORAGE 0x1218
#define ADDR_PLAYER_CAMERAS 0x1800
#define ADDR_WIDESCREEN_FN 0
#define ADDR_WIDESCREEN_FLAG 0x1A00
#define ADDR_ASPECT_FLAGS_PTR 0x1A04
#define ADDR_COLLISION_BSP 0x1B00
#define ADDR_STRUCTURE_BSP 0x1B04
#define ADDR_TAG_INSTANCES 0x1B08
#define ADDR_PS_DEF 0x1400
void host_gfx_quad(u32 color, const s16 *xy);
#define ADDR_VS_CACHE_ARG 0
#else
#ifdef H2_V15
#include "addr_v15.h"
#else
#include "addr_v10.h"
#endif
/* the game - addresses checked against the retail XBE and the decompilation */
#define XInputGetState_real ((u32 (__attribute__((stdcall)) *)(void *, xinput_state *))ADDR_XINPUTGETSTATE)
#define text_style_clear ((void (*)(void))ADDR_TEXT_STYLE_CLEAR)
#define font_get ((void *(__attribute__((fastcall)) *)(u32))ADDR_FONT_GET)   /* font handle in ECX */
#define DRAW_STRING_GLOBALS ((u8 *)ADDR_DRAW_STRING_GLOBALS)                               /* s_draw_string_globals */
#define FONT_HANDLES ((u32 *)ADDR_FONT_HANDLES)
typedef void (__attribute__((thiscall)) *draw_text_fn)(const u16 *text, const rect16 *bounds, const void *clip,
	const void *position, s32 line_gap, float scale, const color4f *color, const color4f *shadow);
/* colour and shadow come from the text state (the layout engine reads g_4e73a0.color/.shadow/.field_24), so pass none */
#define DRAW_TEXT(text, bounds, scale) ((draw_text_fn)ADDR_DRAW_TEXT)(text, bounds, 0, 0, 0, scale, 0, 0)
/* XAPI file functions (named in the XBE's library list, __stdcall) */
#define CreateFileA_x ((void *(__attribute__((stdcall)) *)(const char *, u32, u32, void *, u32, u32, void *))ADDR_CREATEFILEA)
#define WriteFile_x ((s32 (__attribute__((stdcall)) *)(void *, const void *, u32, u32 *, void *))ADDR_WRITEFILE)
#define SetFilePointer_x ((u32 (__attribute__((stdcall)) *)(void *, s32, s32 *, u32))ADDR_SETFILEPOINTER)
#define CloseHandle_x ((s32 (__attribute__((stdcall)) *)(void *))ADDR_CLOSEHANDLE)
#define ReadFile_x ((s32 (__attribute__((stdcall)) *)(void *, void *, u32, u32 *, void *))ADDR_READFILE)
#define VirtualAlloc_x ((void *(__attribute__((stdcall)) *)(void *, u32, u32, u32))ADDR_VIRTUALALLOC)
#define VirtualFree_x ((s32 (__attribute__((stdcall)) *)(void *, u32, u32))ADDR_VIRTUALFREE)
#define GetFileSize_x ((u32 (__attribute__((stdcall)) *)(void *, u32 *))ADDR_GETFILESIZE)
#define G_485602 (*(volatile u16 *)ADDR_FLAG_485602)   /* render target in use; the string drawer only draws when it is 0 */
#define G_51080C (*(volatile u8 *)ADDR_FLAG_51080C)    /* the string drawer also skips while this is set */
#endif

/* ---- a log on the hard drive: T:\spooky_log.txt = E:\TDATA\4D530064\spooky_log.txt ---- */
static s32 o_log = 1;                      /* write T:\\spooky_log.txt (launcher option "Disable log" turns it off) */
u32 g_count_end_a, g_count_end_b;          /* incremented by the end-of-frame stubs in hooks.S */
static u32 g_count_input, g_heartbeats;
static s32 g_logged_input, g_logged_hud;
static s32 g_log_started;
static s32 g_log_drive = -1;            /* which of T:, U:, Z: took the log */
static char g_log[320];
static s32 lput(s32 at, const char *s) { while (*s && at < 318) g_log[at++] = *s++; g_log[at] = 0; return at; }
static s32 lnum(s32 at, u32 v)
{
	char b[12]; s32 n = 0;
	do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v);
	while (n && at < 318) g_log[at++] = b[--n];
	g_log[at] = 0; return at;
}
static s32 lsnum(s32 at, s32 v)
{
	if (v < 0) { at = lput(at, "-"); v = -v; }
	return lnum(at, (u32)v);
}

static s32 lhex(s32 at, u32 v)
{
	s32 i; at = lput(at, "0x");
	for (i = 28; i >= 0; i -= 4) if (at < 318) g_log[at++] = "0123456789ABCDEF"[(v >> i) & 15];
	g_log[at] = 0; return at;
}
#ifdef HOST
void host_log(const char *line);
static void log_write(void) { host_log(g_log); }
#else
static void log_write(void)
{
	u32 n = 0, written;
	if (!o_log) return;
	static const char *const paths[3] = { "T:\\spooky_log.txt", "U:\\spooky_log.txt", "Z:\\spooky_log.txt" };
	void *h = (void *)-1;
	s32 d;
	for (d = g_log_drive < 0 ? 0 : g_log_drive; d < 3 && h == (void *)-1; d++)
	{
		h = CreateFileA_x(paths[d], 0x40000000 /* GENERIC_WRITE */, 1 /* FILE_SHARE_READ */, 0,
			g_log_started ? 4 /* OPEN_ALWAYS */ : 2 /* CREATE_ALWAYS: a fresh log each boot */, 0x80, 0);
		if (h != (void *)-1) g_log_drive = d;
		if (g_log_started) break;           /* once a drive worked, stay on it */
	}
	if (h == (void *)-1) return;
	g_log_started = 1;
	SetFilePointer_x(h, 0, 0, 2 /* FILE_END */);
	while (g_log[n]) n++;
	g_log[n] = '\r'; g_log[n + 1] = '\n';
	WriteFile_x(h, g_log, n + 2, &written, 0);
	g_log[n] = 0;
	CloseHandle_x(h);
}
#endif

/* no C library here: the compiler may still emit calls to these */
/* (the compiler may also turn these loops back into calls to memset / memcpy - themselves: never here) */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void *memset(void *d, int c, u32 n) { u8 *p = (u8 *)d; while (n--) *p++ = (u8)c; return d; }
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void *memcpy(void *d, const void *s, u32 n) { u8 *p = (u8 *)d; const u8 *q = (const u8 *)s; while (n--) *p++ = *q++; return d; }

/* ---- the menu ---- */
enum { K_TOGGLE, K_INT, K_ACTION, K_SUB, K_BACK, K_INT10, K_STAY };   /* K_INT10: a number in steps of 10; K_STAY: an action that leaves the menu open */
typedef struct { const char *label; u8 kind; s32 *value; s32 lo, hi; } s_item;   /* K_ACTION: lo = the action; K_SUB: lo = the menu */

static s32 o_hint = 1;        /* the "Back + D-pad Up" hint line on the game's main menu (the launcher's "Hide hint line": 0) */
static s32 o_shadow = 1;      /* drop shadow behind the menu text */
static s32 o_scale = 10;      /* text scale x10 */
static s32 o_sb_hud = 0;      /* superbounce HUD: CHARGED / not charged */
static s32 o_sb_speed = 1;    /* your speed above it */
static s32 o_sb_streak = 1;   /* bounces in a row and the level's record beside it */
static s32 o_seams = 1;       /* step 3b: the level's floor seams near you */
static s32 o_marker = 0;      /* step 3a's 3D test: a square and a post dropped in front of you */
static s32 o_land_text = 0;   /* the landing's text on the HUD (Cartographer's seam text setting) - off: less busy */
static s32 o_force_charge = 0; /* Cartographer's "Force Charged state" */
static s32 o_third_person = 0; /* Cartographer's "Third person camera": the render camera behind you (aiming / physics unchanged) */
static s32 o_tp_dist = 10, o_tp_height = 2, o_tp_side = 0;   /* tenths of a unit: behind your eye, above it, to the right */
s32 o_no_natural_cp = 0;                       /* 6h.12: Disable natural checkpoints - the game's own saves (its checkpoint logic) skipped */
s32 o_no_reverts = 0;                          /* 6h.12: Disable reverts - the revert after you die stopped */
/* 6h.21: the development lines - the crash trace (spooky_trace.txt, rewritten at each stage of every frame while you move
 * fast or just launched), the camera / seams / ground probe / seam census / nearby object lines, the per-launch and
 * per-bounce lines. Each log line opens, writes and closes a file on the hard disk: off on the consoles (the tests read
 * them). Kept: the start, the 10-second status and performance lines, menu and practice actions, faults. */
#ifdef HOST
static s32 o_diag = 1;
#else
static s32 o_diag = 0;
#endif
static s32 o_tp_nocoll = 1;                    /* Ignore Collision (6h.11): on - the camera goes through walls; off - pulled in, as a vehicle's */
static s32 o_disable_ai = 0;   /* Cartographer's "Disable AI" (the AI update's pause switch, campaign) */
static s32 o_ghost = 0;        /* Cartographer's Ghost Viewer: your Havok capsule (the ghost pill) */
static s32 o_ghost_rewind = 0; /* its rewind poses: where a backstep put you back to, and where you were */
static s32 o_ghost_hold = 8;
static s32 o_tim_overlay = 0;  /* the welder's child polygons, by their TIM (not in the menu: the travel regions show them) */
static s32 o_tim_labels = 0;   /* each one's TIM (units) - off: the believed poses and travel regions are what matter */
static s32 o_oob = 0;          /* the out-of-bounds timer (off by default) */
static s32 o_free_cam = 0;     /* the free camera (Back + Black): the view off your player, flown freely, no collision */
static s32 o_fc_lock_pitch = 1;  /* "Lock camera pitch": stops at straight up / down; off: full rotations */
static s32 o_fc_block = 1;     /* "Disable player movement": the pad flies the camera (the game sees a neutral pad); off: the
                                  camera stays put and the pad moves your player, seen from there */
static s32 o_sleep = 0;        /* the sleep timer: Havok's island deactivation countdown (off by default) */
static s32 o_pred = 0;         /* predicted monster / dreadnought spots (the H2 bounce scanner's, loaded per BSP) */
static s32 o_pred_dread = 0;
static s32 o_pred_range = 0;      /* the Monster/Dreadnought Viewer's range from your view (units, steps of 10): 0 "Max" - every BSP's whole
                                     (as 2000 was); 6h.18: then 10 .. 200 */   /* ... only the dreadnoughts (35+) */
static s32 o_believed = 1;     /* believed poses: where Havok believes you are, from the over-credits */
static s32 o_travel = 1;       /* travel regions: each weld's zone and reset sheet */
static s32 o_bel_max = 6;      /* the most believed poses drawn */
static s32 o_obj_believed = 1; /* believed poses from objects' TIMs (crates, vehicles, scenery) */
static s32 o_obj_travel = 0;   /* travel regions for object pieces (approximate: off, as Cartographer's) */
static s32 o_offset_min = 80;  /* an offset: over-credit past this, thousandths of a unit (Cartographer's 0.08) - not in the menu since 6h */
static s32 o_ofs_natural = 0; /* "Restore offsets after natural reverts" (Cartographer's offset save state; off by default) */   /* ticks a backstep keeps the pill lime and the rewind poses up */
static s32 o_inf_ammo = 0;     /* Infinite Ammo: the game's own (multiplayer) infinite ammo, for players' weapons anywhere (a call hook) */
static s32 o_bottomless = 0;   /* Bottomless Clip: a player's shots take nothing from the magazine or the battery */
static s32 o_skull[16];        /* the skulls, by the game's index: 0 Envy, 1 Grunt Birthday Party, 2 Assassins, 3 Thunderstorm,
                                  4 Famine, 5 IWHBYD, 6 Blind, 7 Ghost, 8 Black Eye, 9 Catch, 10 Sputnik, 11 Iron, 12 Mythic,
                                  13 Anger, 14 Whuppopotamus */
static s32 o_acro = 0;         /* Acrophobia: the debug build's cheat_jetpack (crouch: hover; crouch + A: fly where you look) */
static s32 o_inv_veh = 0;
static s32 o_bump = 0;
static s32 o_hud_ws = 1;       /* HUD Widescreen Fix (6h): in widescreen, each HUD element drawn 3/4 as wide around its anchor */
static s32 o_hud_fix = 0;      /* HUD Scaling Fix: Halo 2 HD's DisableAnamorphicScaling - the 4/3 anamorphic factor 1.0 (the HUD
                                  drawn in the right shape on a 16:9 picture); none of HD's resolution changes */         /* Bump Possession: the debug build's cheat_bump_possession - touch a unit no player controls, become it */      /* Invincible Vehicles: every vehicle the game's cannot-take-damage + cannot-die flags */
static s32 o_invuln = 0;       /* Cartographer's "Invulnerability": the game's own cannot-take-damage + cannot-die flags */
static s32 o_launch_fwd = 10, o_launch_up = 0, o_tp_fwd = 5, o_tp_up = 0;   /* Launch (u/s) / Teleport (units), along your view */
static volatile s32 g_req;                      /* a practice action asked for: 1 checkpoint, 2 revert, 3 launch, 4 teleport, 5 third person */
static s32 o_keep_charge = 0;  /* Cartographer's "Maintain charge state on ... reverts" (the revert part) */
static s32 o_speed = 10;       /* 6h.22: Game Speed, tenths: 0.0 to 5.0 (1.0 the game's own) */
static s32 o_pause = 0;        /* 6h.22: Pause Game - the game's speed 0 while on; Game Speed kept for when it's off */

/* As Cartographer's "Spooky Ghost Settings": its top-level items, then "Launch / Teleport" and "Superbounce Viewer
 * Settings" as submenus (A enters, B or Back returns) */
static const s_item m_main[] = {
	{ "Cheats  >", K_SUB, 0, 4, 0 },
	{ "Game Speed", K_INT, &o_speed, 0, 50 },              /* 6h.22: tenths - 0.0 to 5.0 */
	{ "Pause Game", K_TOGGLE, &o_pause, 0, 1 },            /* 6h.22 */
	{ "Checkpoint Settings  >", K_SUB, 0, 9, 0 },          /* 6h.12: Create / Revert Checkpoint moved in there */
	{ "Launch / Teleport  >", K_SUB, 0, 1, 0 },
	{ "Superbounce Viewer", K_TOGGLE, &o_sb_hud, 0, 1 },
	{ "Superbounce Viewer Settings  >", K_SUB, 0, 2, 0 },
	{ "Ghost Viewer", K_TOGGLE, &o_ghost, 0, 1 },
	{ "Ghost Viewer Settings  >", K_SUB, 0, 5, 0 },
	{ "Camera Settings  >", K_SUB, 0, 3, 0 },
	{ "Timers  >", K_SUB, 0, 6, 0 },
	{ "Text scale", K_INT, &o_scale, 6, 14 },             /* 6h.22: Passion Check back to Cheats; Close menu gone (B closes) */
};
static const s_item m_cp[] = {                   /* 6h.12: Checkpoint Settings (from the top, under Cheats) */
	{ "Create Checkpoint (Back + Left)", K_ACTION, 0, 1, 0 },
	{ "Revert Checkpoint (Back + Right)", K_ACTION, 0, 2, 0 },
	{ "Disable natural checkpoints", K_TOGGLE, &o_no_natural_cp, 0, 1 },
	{ "Disable reverts", K_TOGGLE, &o_no_reverts, 0, 1 },
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_item m_launch[] = {
	{ "Launch (Black + Up)", K_ACTION, 0, 3, 0 },
	{ "Launch forward u/s", K_INT, &o_launch_fwd, 0, 60 },
	{ "Launch up u/s", K_INT, &o_launch_up, 0, 60 },
	{ "Teleport (Black + Down)", K_ACTION, 0, 4, 0 },
	{ "Teleport forward", K_INT, &o_tp_fwd, 0, 30 },
	{ "Teleport up", K_INT, &o_tp_up, 0, 30 },
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_item m_sb[] = {
	{ "Maintain charge state on checkpoints and reverts", K_TOGGLE, &o_keep_charge, 0, 1 },
	{ "Force Charged state", K_TOGGLE, &o_force_charge, 0, 1 },
	{ "Seam info text", K_TOGGLE, &o_land_text, 0, 1 },
	{ "Speed above the charge indicator", K_TOGGLE, &o_sb_speed, 0, 1 },
	{ "Bounces in a row", K_TOGGLE, &o_sb_streak, 0, 1 },
	{ "Seams near you", K_TOGGLE, &o_seams, 0, 1 },
	{ "Monster/Dreadnought Viewer", K_TOGGLE, &o_pred, 0, 1 },
	{ "Monster/Dreadnought Viewer: 35+ only", K_TOGGLE, &o_pred_dread, 0, 1 },
	{ "Monster/Dreadnought range (units)", K_INT10, &o_pred_range, 0, 200 },   /* 6h.18: Max, then 10 .. 200 */
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_item m_tp[] = {
	{ "Third person camera (Back + Down)", K_TOGGLE, &o_third_person, 0, 1 },
	{ "Third Person Settings  >", K_SUB, 0, 7, 0 },
	{ "Free camera (Back + Black)", K_TOGGLE, &o_free_cam, 0, 1 },
	{ "Disable player movement", K_TOGGLE, &o_fc_block, 0, 1 },
	{ "Lock camera pitch", K_TOGGLE, &o_fc_lock_pitch, 0, 1 },
	{ "Teleport to camera (Back + White)", K_ACTION, 0, 7, 0 },
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_item m_tp3[] = {                 /* Camera Settings > Third Person Settings */
	{ "Distance (tenths)", K_INT, &o_tp_dist, 2, 60 },
	{ "Height (tenths)", K_INT, &o_tp_height, -10, 20 },
	{ "Side (tenths, + right)", K_INT, &o_tp_side, -20, 20 },
	{ "Ignore Collision", K_TOGGLE, &o_tp_nocoll, 0, 1 },
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_item m_cheats[] = {
	{ "Invulnerability", K_TOGGLE, &o_invuln, 0, 1 },
	{ "Infinite Ammo", K_TOGGLE, &o_inf_ammo, 0, 1 },
	{ "Bottomless Clip", K_TOGGLE, &o_bottomless, 0, 1 },
	{ "Disable AI", K_TOGGLE, &o_disable_ai, 0, 1 },
	{ "Bump Possession", K_TOGGLE, &o_bump, 0, 1 },
	{ "Cycle Possession", K_STAY, 0, 12, 0 },              /* 6h.5: the menu stays open (its message shown under it) */
	{ "Revert to player possession", K_STAY, 0, 8, 0 },
	{ "Invincible Vehicles", K_TOGGLE, &o_inv_veh, 0, 1 },
	{ "HUD Widescreen Fix", K_TOGGLE, &o_hud_ws, 0, 1 },
	{ "Passion Check", K_ACTION, 0, 9, 0 },               /* 6h.22: back in Cheats, above Skulls */
	{ "Skulls  >", K_SUB, 0, 8, 0 },
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_item m_skulls[] = {             /* in the order they're found in the campaign */
	{ "Cow Bell", K_TOGGLE, &o_skull[14], 0, 1 },           /* (the game's skull 14: Whuppopotamus) */
	{ "Thunderstorm", K_TOGGLE, &o_skull[3], 0, 1 },
	{ "Blind", K_TOGGLE, &o_skull[6], 0, 1 },
	{ "I Would Have Been Your Daddy", K_TOGGLE, &o_skull[5], 0, 1 },
	{ "Catch", K_TOGGLE, &o_skull[9], 0, 1 },
	{ "Grunt Birthday Party", K_TOGGLE, &o_skull[1], 0, 1 },
	{ "Famine", K_TOGGLE, &o_skull[4], 0, 1 },
	{ "Envy", K_TOGGLE, &o_skull[0], 0, 1 },
	{ "Assassins", K_TOGGLE, &o_skull[2], 0, 1 },
	{ "Mythic", K_TOGGLE, &o_skull[12], 0, 1 },
	{ "Sputnik", K_TOGGLE, &o_skull[10], 0, 1 },
	{ "Anger", K_TOGGLE, &o_skull[13], 0, 1 },
	{ "Ghost", K_TOGGLE, &o_skull[7], 0, 1 },
	{ "Iron", K_TOGGLE, &o_skull[11], 0, 1 },
	{ "Black Eye", K_TOGGLE, &o_skull[8], 0, 1 },
	{ "Acrophobia", K_TOGGLE, &o_acro, 0, 1 },
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_item m_ghost[] = {               /* 6h: rewind poses (off), backstep hold (8), max believed poses (6) and the
                                                   offset threshold (0.08) left at those values, out of the menu */
	{ "Save offsets (Black + Left)", K_ACTION, 0, 10, 0 },          /* 6h.27: Black (White is Game Speed's) */
	{ "Revert offsets (Black + Right)", K_ACTION, 0, 11, 0 },
	{ "Restore offsets after natural reverts", K_TOGGLE, &o_ofs_natural, 0, 1 },
	{ "Believed poses", K_TOGGLE, &o_believed, 0, 1 },
	{ "Travel regions", K_TOGGLE, &o_travel, 0, 1 },
	{ "Object believed poses", K_TOGGLE, &o_obj_believed, 0, 1 },
	{ "Object travel regions", K_TOGGLE, &o_obj_travel, 0, 1 },
	{ "Back", K_BACK, 0, 0, 0 },
};
typedef struct { const char *title; const s_item *items; s32 n; s32 parent; } s_menu;   /* parent: where B / Back goes (0: the top) */
#define NOF(a) ((s32)(sizeof(a) / sizeof(a[0])))
static const s_item m_timers[] = {
	{ "Out of bounds timer", K_TOGGLE, &o_oob, 0, 1 },
	{ "Sleep timer", K_TOGGLE, &o_sleep, 0, 1 },
	{ "Back", K_BACK, 0, 0, 0 },
};
static const s_menu g_menus[] = {
	{ "Spooky Ghost Menu - by Fortune090", m_main, NOF(m_main), 0 },
	{ "SPOOKY GHOST  Launch / Teleport", m_launch, NOF(m_launch), 0 },
	{ "SPOOKY GHOST  Superbounce Viewer Settings", m_sb, NOF(m_sb), 0 },
	{ "SPOOKY GHOST  Camera Settings", m_tp, NOF(m_tp), 0 },
	{ "SPOOKY GHOST  Cheats", m_cheats, NOF(m_cheats), 0 },
	{ "SPOOKY GHOST  Ghost Viewer Settings", m_ghost, NOF(m_ghost), 0 },
	{ "SPOOKY GHOST  Timers", m_timers, NOF(m_timers), 0 },
	{ "SPOOKY GHOST  Third Person Settings", m_tp3, NOF(m_tp3), 3 },   /* within Camera Settings */
	{ "SPOOKY GHOST  Skulls", m_skulls, NOF(m_skulls), 4 },             /* within Cheats */
	{ "SPOOKY GHOST  Checkpoint Settings", m_cp, NOF(m_cp), 0 },         /* 6h.12 */
};
/* each menu's selection and first row shown - one per menu in g_menus (6a.1 had 6: the Timers menu, the 7th, used g_sel[6]
 * = g_first[0] (the main menu's scroll) and g_first[6] past the end - down there moved other menus' state; then a crash) */
static s32 g_menu = 0, g_sel[NOF(g_menus)], g_first[NOF(g_menus)];
static s32 g_rows_cap;                          /* tests: fewer rows than the screen fits (0 = the screen decides) */   /* the menu showing; each one's selection and first row shown */
#define ITEM_COUNT (g_menus[g_menu].n)
#define g_selected (g_sel[g_menu])

static s32 g_open = 0;            /* the menu is showing */
static float g_stick[2];          /* the left stick, -1..1 (Cartographer's sb_read_input) */
static s32 g_marker_set;          /* the 3D test marker has been dropped (cleared when the menu closes: dropped again) */
static u32 g_count_hud;           /* HUD-text frames drawn through the hook */
static s32 g_ever_opened = 0;
static void *g_owner = 0;         /* the controller that opened it */
static s32 g_suppress = 0;        /* keep the owner's controller neutral until everything is released */
static u32 p1_handle(void);       /* port 1's XInput handle (the game's handle array) */
static u16 g_p1_buttons;          /* port 1's buttons and A, as read (Acrophobia) */
static u8 g_p1_a;
/* ---- the free camera (HCM's): the view off your player, flown freely - no collision. Left stick: forward / back and
 * side to side, along where you look; right stick: look; right / left trigger: up / down; D-pad up / down: faster /
 * slower (x1.5 a step from HCM's 3 u/s). Eased as HCM's (6% a 60 Hz frame toward where the input puts it). It starts
 * from the view you had. With "Disable player movement" on, the game sees a neutral pad (Start passes); off, the camera
 * stays put and the pad moves your player, seen from there. ---- */
static void *g_fc_pad;                          /* the pad flying it */
/* a stick axis as the player controls see it: the game's controller poll (v1.0 0x124d49 on) zeroes anything within
 * +-9000 of 32767 (27.5%) and stretches the rest back to full - (v - 9000) x 32767 / 23767 - each axis on its own. (6c.3
 * used its own 20%: drift between that and 9000 flew the camera.) */
static float fc_stick(s16 v)
{
	s32 x = v;
	float r = x > 9000 ? (float)(x - 9000) / 23767.f : x < -9000 ? (float)(x + 9000) / 23768.f : 0.f;
	return r > 1.f ? 1.f : r < -1.f ? -1.f : r;
}
/* a trigger: the game takes it as pressed only past 0x20 (32 of 255) - below that, nothing; past it, scaled to full */
static float fc_trigger(u8 v) { return v > 32 ? (float)(v - 32) / 223.f : 0.f; }
static float g_fc_in[6];                        /* that pad's sticks (lx, ly, rx, ry) and triggers (right, left), this read */
static s32 g_fc_speed_steps;                    /* D-pad steps: speed 3 x 1.5^steps */
static s32 g_fc_on;                             /* the camera's pose is set (from the view it started at) */
static float g_fc_p[3], g_fc_yaw, g_fc_pitch;   /* where it is */
static float g_fc_tp[3], g_fc_tyaw, g_fc_tpitch;   /* where the input puts it (it eases toward that) */
static u32 g_frames = 0;

typedef struct { void *handle; u16 buttons; u8 a, b; u8 combo; u8 repeat_dir; u16 held; u8 black, white; u8 sp_dir; u16 sp_held; } s_pad;   /* sp_: White + Left / Right held */
static s_pad g_pads[4];

static s_pad *pad_for(void *handle)
{
	s32 i, free_slot = -1;
	for (i = 0; i < 4; i++)
	{
		if (g_pads[i].handle == handle) return &g_pads[i];
		if (!g_pads[i].handle && free_slot < 0) free_slot = i;
	}
	if (free_slot < 0) free_slot = 0;
	memset(&g_pads[free_slot], 0, sizeof(s_pad));
	g_pads[free_slot].handle = handle;
	return &g_pads[free_slot];
}

static void log_write(void);
static s32 lput(s32 at, const char *s);
static s32 lnum(s32 at, u32 v);
static void set_open(s32 open, const char *why)
{
	g_open = open;
	if (open) g_menu = 0;                                                    /* always opens at the top */
	if (!open) { g_suppress = 1; g_marker_set = 0; }
	s32 at = lput(0, open ? "menu opened (" : "menu closed (");
	at = lput(at, why); at = lput(at, ", HUD-text frame "); at = lnum(at, g_count_hud); lput(at, ")");
	log_write();
}

static void change(s32 delta, s32 activate)
{
	const s_item *it = &g_menus[g_menu].items[g_selected];
	if (it->kind == K_SUB) { if (activate) g_menu = it->lo; return; }
	if (it->kind == K_BACK) { if (activate) g_menu = g_menus[g_menu].parent; return; }   /* up one level */
	if (it->kind == K_STAY) { if (activate) g_req = it->lo; return; }   /* done in the next frame, the menu left open */
	if (it->kind == K_ACTION)
	{
		if (!activate) return;
		if (it->lo) { g_req = it->lo; set_open(0, "practice action"); }       /* done in the next frame, the menu closed */
		else set_open(0, "Close menu item");
		return;
	}
	if (it->kind == K_TOGGLE) { *it->value = !*it->value; return; }
	if (activate) delta = 1;
	if (it->kind == K_INT10) delta *= 10;
	*it->value += delta;
	if (*it->value > it->hi) *it->value = activate ? it->lo : it->hi;     /* A wraps round, right stops at the top */
	if (*it->value < it->lo) *it->value = it->lo;
}

/* 6h.27: White + D-pad Left / Right (menu closed) - Game Speed down / up 0.1, 0.0 to 5.0 (stops at the ends); held:
 * again after ~0.4 s, then every ~0.1 s (as the menu's left / right). Said on screen - the menu isn't open. */
static void toast(const char *t);
static void speed_keys(s_pad *pad, u16 buttons, u16 pressed, s32 on)
{
	u8 d = on ? ((buttons & BTN_LEFT) ? 3 : (buttons & BTN_RIGHT) ? 4 : 0) : 0;
	s32 fire = 0;
	char t[24] = "Game Speed: ";
	if (d && (pressed & (BTN_LEFT | BTN_RIGHT))) { fire = 1; pad->sp_held = 0; pad->sp_dir = d; }
	else if (d && d == pad->sp_dir) { pad->sp_held++; if (pad->sp_held >= 12 && (pad->sp_held - 12) % 3 == 0) fire = 1; }
	else { pad->sp_held = 0; pad->sp_dir = d; }
	if (!fire) return;
	o_speed += d == 3 ? -1 : 1;
	if (o_speed < 0) o_speed = 0;
	if (o_speed > 50) o_speed = 50;
	t[12] = (char)('0' + o_speed / 10); t[13] = '.'; t[14] = (char)('0' + o_speed % 10); t[15] = 0;
	toast(t);
}

static void neutral(xinput_state *st)
{
	st->buttons = 0;
	memset(st->analog, 0, sizeof(st->analog));
	st->lx = st->ly = st->rx = st->ry = 0;
}

static s32 on_main_menu(void);
/* called for every controller read the game makes (XInputGetState, once per controller per input update) */
u32 spooky_input(void *handle, xinput_state *st)
{
	u32 result = XInputGetState_real(handle, st);
	g_count_input++;
	if (!g_logged_input)
	{
		g_logged_input = 1;
		s32 at = lput(0, "controller hook running (first read: handle ");
		at = lhex(at, (u32)(unsigned long)handle); at = lput(at, ", result "); at = lnum(at, result); lput(at, ")");
		log_write();
	}
	if (0)
	{
		s32 at = lput(0, "status: controller reads "); at = lnum(at, g_count_input);
		at = lput(at, ", HUD-text frames "); at = lnum(at, g_count_hud);
		at = lput(at, ", route A frame ends "); at = lnum(at, g_count_end_a);
		at = lput(at, ", route B frame ends "); at = lnum(at, g_count_end_b);
		at = lput(at, ", menu "); lput(at, g_open ? "open" : "closed");
		log_write();
	}
	if (result != 0) return result;

	s_pad *pad = pad_for(handle);
	u16 buttons = st->buttons;
	if ((u32)(unsigned long)handle == p1_handle()) { g_p1_buttons = buttons; g_p1_a = st->analog[AB_A]; }   /* port 1: Acrophobia's controls */
	u16 pressed = (u16)(buttons & ~pad->buttons);
	u8 a = st->analog[AB_A] > ANALOG_DOWN, b = st->analog[AB_B] > ANALOG_DOWN;
	g_stick[0] = (float)st->lx / 32768.f; g_stick[1] = (float)st->ly / 32768.f;      /* for air control's target */
	/* 6h: none of this on the game's main menu - a game that ended with the menu open left it up there, and the game's
	 * menus got nothing until it was closed: closed there, and neither it nor the shortcuts open (the hint line stays) */
	s32 main_menu = on_main_menu();
	if (main_menu && g_open) set_open(0, "the game's main menu");
	if (!g_open && !main_menu)
	{
		/* Cartographer's practice shortcuts: Back + D-pad Left / Right = create / revert checkpoint; Black + D-pad
		 * Up / Down = launch / teleport (Back + Up opens this menu, so not Cartographer's Back + D-pad Up / Down) */
		s32 r = 0;
		u8 black = st->analog[AB_BLACK] > ANALOG_DOWN;
		u8 white = st->analog[AB_WHITE] > ANALOG_DOWN;
		if ((buttons & BTN_BACK) && black && !pad->black) r = 6;              /* the free camera, on / off */
		else if ((buttons & BTN_BACK) && white && !pad->white) r = 7;        /* teleport to the free camera */
		else if ((buttons & BTN_BACK) && (pressed & BTN_LEFT)) r = 1;
		else if ((buttons & BTN_BACK) && (pressed & BTN_RIGHT)) r = 2;
		else if ((buttons & BTN_BACK) && (pressed & BTN_DOWN)) r = 5;         /* the third person camera, on / off */
		else if (black && (pressed & BTN_UP)) r = 3;
		else if (black && (pressed & BTN_DOWN)) r = 4;
		else if (black && o_ghost && (pressed & BTN_LEFT)) r = 10;          /* save offsets (6h; 6h.27: Black - was White) */
		else if (black && o_ghost && (pressed & BTN_RIGHT)) r = 11;         /* revert offsets */
		else if (white && !(buttons & BTN_BACK) && (pressed & BTN_DOWN))    /* 6h.27: White + Down - Pause Game */
		{
			o_pause = !o_pause;
			toast(o_pause ? "Pause Game: on" : "Pause Game: off");
		}
		if (r) g_req = r;
		speed_keys(pad, buttons, pressed, white && !black && !(buttons & BTN_BACK));   /* 6h.27: White + Left / Right */
	}
	u8 combo = (buttons & BTN_BACK) && (buttons & BTN_UP);
	u8 combo_edge = combo && !pad->combo && !main_menu;
	u8 a_edge = a && !pad->a, b_edge = b && !pad->b;
	pad->buttons = buttons; pad->a = a; pad->b = b; pad->combo = combo; pad->black = st->analog[AB_BLACK] > ANALOG_DOWN; pad->white = st->analog[AB_WHITE] > ANALOG_DOWN;

	if (combo_edge)
	{
		set_open(!g_open, "Back + D-pad Up");
		g_owner = handle;
		g_suppress = 1;
		g_ever_opened = 1;
		pad->held = 0; pad->repeat_dir = 0;
		neutral(st);
		return result;
	}

	if (g_open && handle == g_owner)
	{
		/* D-pad with Blam-style auto-repeat: once on the press, then after ~0.4 s every ~0.1 s */
		u8 dir = (buttons & BTN_UP) ? 1 : (buttons & BTN_DOWN) ? 2 : (buttons & BTN_LEFT) ? 3 : (buttons & BTN_RIGHT) ? 4 : 0;
		s32 fire = 0;
		if (!(buttons & BTN_BACK))
		{
			if (dir && (pressed & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT))) { fire = 1; pad->held = 0; pad->repeat_dir = dir; }
			else if (dir && dir == pad->repeat_dir) { pad->held++; if (pad->held >= 12 && (pad->held - 12) % 3 == 0) fire = 1; }
			else { pad->held = 0; pad->repeat_dir = dir; }
		}
		if (fire)
		{
			if (dir == 1) g_selected = (g_selected + ITEM_COUNT - 1) % ITEM_COUNT;
			else if (dir == 2) g_selected = (g_selected + 1) % ITEM_COUNT;
			else if (dir == 3) change(-1, 0);
			else if (dir == 4) change(+1, 0);
		}
		if (a_edge) change(0, 1);
		if (b_edge) { if (g_menu) g_menu = g_menus[g_menu].parent; else set_open(0, "B"); }   /* up one level; the top: close */
		neutral(st);
		return result;
	}

	if (g_suppress && handle == g_owner)
	{
		if (!buttons && !a && !b && !st->analog[2] && !st->analog[3] && st->analog[6] < ANALOG_DOWN && st->analog[7] < ANALOG_DOWN) g_suppress = 0;
		else neutral(st);
	}
	else if (o_free_cam && o_fc_block && !g_open && (!g_fc_pad || handle == g_fc_pad))
	{
		/* the free camera flies on this pad: its sticks, triggers and D-pad - the game sees a neutral pad (Start passes:
		 * you can still pause) */
		u16 start = (u16)(buttons & 0x0010);
		g_fc_pad = handle;
		g_fc_in[0] = fc_stick(st->lx); g_fc_in[1] = fc_stick(st->ly);       /* the player controls' own dead zone */
		g_fc_in[2] = fc_stick(st->rx); g_fc_in[3] = fc_stick(st->ry);
		g_fc_in[4] = fc_trigger(st->analog[7]); g_fc_in[5] = fc_trigger(st->analog[6]);   /* right / left trigger */
		if (!(buttons & BTN_BACK) && st->analog[AB_BLACK] <= ANALOG_DOWN && st->analog[AB_WHITE] <= ANALOG_DOWN)   /* 6h.27: not with Black / White */
		{ if (pressed & BTN_UP) g_fc_speed_steps++; if (pressed & BTN_DOWN) g_fc_speed_steps--; }
		neutral(st);
		st->buttons = start;
	}
	return result;
}

/* ---- superbounce (ported from Cartographer's superbounce viewer) ----
 * A crouch charge is one of the body's Havok collision pairs held at penetration level -3. A bounce: charged, falling
 * faster than 2 u/s, then launched more than 4 u/s up (or kicked more than 4 u/s sideways). The chain ends after 3 ticks
 * on the ground or a revert. Every pointer below comes from a known field (the decompilation's layouts, unchanged in
 * v1.5), and is checked before it is followed: a retail Xbox has nothing to catch a bad read. */
#ifdef HOST
#define M32(a) host_mem32((u32)(a))
#define M8(a) host_mem8((u32)(a))
static float MF(u32 a) { u32 v = host_mem32(a); float f; memcpy(&f, &v, 4); return f; }
#else
#define MF(a) (*(volatile float *)(u32)(a))
#define M32(a) (*(volatile u32 *)(u32)(a))
#define M8(a) (*(volatile u8 *)(u32)(a))
#endif
static s32 ptr_ok(u32 p) { return p >= 0x10000 && p < 0x84000000u && !(p & 3); }
static float fsqrt(float x)
{
#ifdef HOST
	float r = x > 0 ? x : 0; float g = r > 1 ? r : 1; s32 i; for (i = 0; i < 30; i++) g = 0.5f * (g + r / g); return r > 0 ? g : 0;
#else
	float r; __asm__("fsqrt" : "=t"(r) : "0"(x > 0 ? x : 0)); return r;
#endif
}

/* a data array (s_record_pool): maximum count +0x20, element size +0x24, data +0x44 */
static u32 pool_entry(u32 pool_ptr_address, s32 index, u32 min_size)
{
	u32 pool = M32(pool_ptr_address);
	if (!ptr_ok(pool) || index == -1) return 0;
	u32 data = M32(pool + 0x44), size = M32(pool + 0x24), count = M32(pool + 0x20);
	if (!ptr_ok(data) || size < min_size || (u32)(index & 0xffff) >= count) return 0;
	return data + (u32)(index & 0xffff) * size;
}

/* the first local player's unit (its biped object), or 0 */
static u32 player_object(void)
{
	u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR);
	if (!ptr_ok(lp)) return 0;
	u32 player = pool_entry(ADDR_PLAYERS_PTR, (s32)M32(lp + 0xC), 0x30);
	if (!player) return 0;
	s32 unit = (s32)M32(player + 0x2C);
	if (unit == -1) return 0;
	u32 pool = M32(ADDR_OBJECTS_PTR);
	if (!ptr_ok(pool)) return 0;
	u32 data = M32(pool + 0x44), count = M32(pool + 0x20);
	if (!ptr_ok(data) || (u32)(unit & 0xffff) >= count) return 0;
	u32 header = data + (u32)(unit & 0xffff) * 12;
	if ((M32(header) & 0xffff) != ((u32)unit >> 16)) return 0;       /* the salt: a stale index is not this unit */
	u32 object = M32(header + 8);
	return ptr_ok(object) ? object : 0;
}

/* the collision handle inside a rigid body: the word whose +0x10 points back at the body (Cartographer's search) */
static u32 body_handle(u32 entity)
{
	u32 off;
	for (off = 0x10; off <= 0x80; off += 4)
		if (M32(entity + off + 0x10) == entity && ptr_ok(M32(entity + off - 0x10))) return entity + off;
	return 0;
}

/* charged: one of the unit's bodies in a pair of its own island at penetration level -3, with a real agent (the
 * null agent is a static object in the game's .data; real agents are allocated) */
#ifdef HOST
static s32 guarded_call(void (*fn)(void)) { fn(); return 0; }
#else
extern s32 guarded_call(void (*fn)(void));
#endif
static s32 unit_charged(u32 object);
static u32 g_uc_obj;
static s32 g_uc_res;
static void unit_charged_guarded(void) { g_uc_res = unit_charged(g_uc_obj); }
static s32 unit_charged(u32 object)
{
	u32 component = pool_entry(ADDR_HAVOK_COMPONENTS_PTR, (s32)M32(object + 0xB4), 0x78);
	if (!component) return 0;
	u32 bodies = M32(component + 0x70);
	s32 n = (s32)M32(component + 0x74), i, k;
	u32 world = M32(ADDR_HAVOK_WORLD_PTR);
	if (!ptr_ok(bodies) || n <= 0 || n > 64 || !ptr_ok(world)) return 0;
	for (i = 0; i < n; i++)
	{
		u32 entity = M32(bodies + (u32)i * 0x60 + 0x40);
		if (!ptr_ok(entity) || M32(entity + 0x08) != world) continue;          /* hkRigidBody::m_world */
		u32 island = M32(entity + 0x44);                                      /* hkRigidBody::m_simulation_island */
		if (!ptr_ok(island)) continue;
		u32 entities = M32(island + 0x3C);                                    /* the island must list this body */
		s32 count = (s32)M32(island + 0x40), found = 0;
		if (!ptr_ok(entities) || count <= 0 || count > 4096) continue;
		for (k = 0; k < count && !found; k++) found = M32(entities + (u32)k * 4) == entity;
		if (!found) continue;
		u32 handle = body_handle(entity);
		u32 pairs = M32(island + 0x68);
		s32 pn = (s32)M32(island + 0x6C);
		if (!handle || !ptr_ok(pairs) || pn <= 0 || pn > 8192) continue;
		for (k = 0; k < pn; k++)
		{
			u32 p = pairs + (u32)k * 16, agent = M32(p + 8);
			if ((M32(p) == handle || M32(p + 4) == handle) && ptr_ok(agent) && !(agent >= ADDR_DATA_LO && agent < ADDR_DATA_HI)
				&& (s32)M32(p + 12) == -3) return 1;
		}
	}
	return 0;
}

/* a write into the game's memory (the charge) */
#ifdef HOST
void host_w32(u32 a, u32 v);
static void gw32(u32 a, u32 v) { host_w32(a, v); }
#else
static void gw32(u32 a, u32 v) { *(volatile u32 *)a = v; }
#endif

/* ---- Cartographer's practice tools: Create / Revert Checkpoint (the main loop's save / revert request flags, as
 * Cartographer writes them), Launch and Teleport (your unit, or the vehicle you're in, along your view). Campaign only
 * on the Xbox (the machine must own the simulation). ---- */
static char g_toast[80];
static s32 g_toast_frames;
static void toast(const char *t) { s32 i; for (i = 0; t[i] && i < 79; i++) g_toast[i] = t[i]; g_toast[i] = 0; g_toast_frames = 60; }
#ifdef HOST
static void gw8(u32 a, u8 v) { u32 al = a & ~3u, sh = (a & 3u) * 8u, w = M32(al); gw32(al, (w & ~(0xFFu << sh)) | ((u32)v << sh)); }
#else
static void gw8(u32 a, u8 v) { *(volatile u8 *)a = v; }
#endif
static void gwf(u32 a, float f) { union { float f; u32 u; } x; x.f = f; gw32(a, x.u); }
static s32 is_campaign(void) { u32 g = M32(ADDR_GAME_OPTIONS_PTR); return ptr_ok(g) && M32(g + 8) == 1; }

/* ---- 6h.22: Game Speed and Pause Game (as Yelo's and HCM's). The game's time globals (v1.0 0x510c54, v1.5 0x51df54):
 * +0 initialized, +1 paused, +2 ticks a second, +4 seconds a tick, +8 the game time, +0xC the speed (1.0), +0x10 the
 * tick fraction left over, +0x14..+0x20 a rate blend (a script's: start, target, how long). Each frame the game runs
 * ticks a second x speed x the frame's time of ticks (v1.0 0x146660, v1.5 0x149160) - none when the speed is 0 - at most
 * clamp(speed, 1, 5) x 2 a frame. The input poll (v1.0 0x124950, from 0x12b510 before the time update) runs every frame,
 * paused or not.
 * 6h.24: the ticks stay 30 a second, 1/30 s each, at every speed (the user's rule: the physics locked at 30 ticks) -
 * only the speed changes how many run each second; a tick rate found otherwise (a 6h.23 checkpoint's 60 or 300) is put
 * back to 30. (6h.23's shorter ticks - the game's own slow motion, its rate setter 0x146980 - kept a tick a frame and
 * the picture smooth, but changed the physics.) Pause Game: speed 0. In a game only: the game's main menu is its own
 * scene, and this menu can't open there to turn Pause off. ---- */
static u32 M16(u32 a);
static s32 g_speed_set;        /* a rate of ours is in the game's time globals */
static void rate_put(u32 time, s32 tps, float speed)
{
	u32 w = M32(time);
	if ((s32)(s16)(w >> 16) != tps) { gw32(time, (w & 0xFFFFu) | ((u32)(u16)tps << 16)); gwf(time + 4, 1.f / (float)tps); }
	if (MF(time + 0xC) != speed) gwf(time + 0xC, speed);
}
static void game_speed_frame(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), opt = M32(ADDR_GAME_OPTIONS_PTR);
	s32 base, tps;
	float cur, tgt;
	if (!ptr_ok(time) || !M8(time)) return;                              /* no game time yet */
	base = ptr_ok(opt) ? (s32)(s16)M16(opt + 0xE) : 30;
	if (base <= 0 || base > 300) base = 30;
	if (!ptr_ok(opt) || M32(opt + 8) == 3)                                /* no game, or the game's main menu */
	{
		if (g_speed_set) rate_put(time, base, 1.f);                       /* ours: the game's own rate back */
		g_speed_set = 0;
		return;
	}
	if (MF(time + 0x18) > 0.f && !o_pause) { g_speed_set = 0; return; }  /* a script's rate blend under way: its own */
	if (o_pause || o_speed != 10)
	{
		rate_put(time, base, o_pause ? 0.f : (float)o_speed / 10.f);       /* 30 ticks a second, always */
		g_speed_set = 1;
		return;
	}
	/* 1.0, not paused: the game's own rate. One of ours can come back with a revert (the time globals are in the saved
	 * game state): the normal rate put back - unless it's a rate a script set (its target, +0x20: the ticks match it) */
	cur = MF(time + 0xC); tps = (s32)(s16)M16(time + 2); tgt = MF(time + 0x20);
	if (tps != base || cur != 1.f)
	{
		s32 script = 0;
		if (tgt > 0.f && tgt != 1.f) { float want = (float)base / tgt, d = (float)tps - want; script = d > -1.01f && d < 1.01f; }
		if (!script) rate_put(time, base, 1.f);
	}
	g_speed_set = 0;
}
static u32 object_from_datum(s32 datum)
{
	u32 pool = M32(ADDR_OBJECTS_PTR), data, count, header;
	if (datum == -1 || !ptr_ok(pool)) return 0;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || (u32)(datum & 0xffff) >= count) return 0;
	header = data + (u32)(datum & 0xffff) * 12;
	if ((M32(header) & 0xffff) != ((u32)datum >> 16)) return 0;
	return ptr_ok(M32(header + 8)) ? M32(header + 8) : 0;
}
/* the object's Havok rigid bodies (Cartographer's object_bodies): its component's body list, entity at +0x40 of each
 * 0x60 entry, with a motion at entity +0x3C. The bodies are what get simulated; the object follows them. */
/* only bodies still in Havok's world (entity +0x08 = the world, with an island): out of the level - after a ghost off,
 * below the map - Havok takes a body out of its world while the object still lists it; acting on one of those (writing
 * it, calling its motion's setter) is what crashed Launch / Teleport there */
static s32 g_bodies_listed;
static s32 object_bodies(u32 obj, u32 *out)
{
	u32 comp = pool_entry(ADDR_HAVOK_COMPONENTS_PTR, (s32)M32(obj + 0xB4), 0x78), arr, world = M32(ADDR_HAVOK_WORLD_PTR);
	s32 n, i, k = 0;
	g_bodies_listed = 0;
	if (!comp || !ptr_ok(world)) return 0;
	arr = M32(comp + 0x70); n = (s32)M32(comp + 0x74);
	if (!ptr_ok(arr) || n <= 0 || n > 64) return 0;
	for (i = 0; i < n && k < 16; i++)
	{
		u32 e = M32(arr + (u32)i * 0x60 + 0x40);
		if (!ptr_ok(e)) continue;
		g_bodies_listed++;
		if (M32(e + 0x08) != world || !ptr_ok(M32(e + 0x44)) || !ptr_ok(M32(e + 0x3C))) continue;   /* not in the world */
		out[k++] = e;
	}
	return k;
}
/* the handle inside an entity (back-pointer to it at +0x10), found once and cached (Cartographer's handle_in_entity) */
static s32 g_handle_off = -1;
static u32 handle_in_entity(u32 e)
{
	s32 off;
	if (g_handle_off >= 0 && M32(e + (u32)g_handle_off + 0x10) == e && ptr_ok(M32(e + (u32)g_handle_off - 0x10))) return e + (u32)g_handle_off;
	for (off = 0x10; off <= 0x80; off += 4)
		if (M32(e + (u32)off + 0x10) == e && ptr_ok(M32(e + (u32)off - 0x10))) { g_handle_off = off; return e + (u32)off; }
	return 0;
}
static void shift3(u32 a, const float *d) { gwf(a, MF(a) + d[0]); gwf(a + 4, MF(a + 4) + d[1]); gwf(a + 8, MF(a + 8) + d[2]); }
/* a body's linear velocity through Havok's own setter (the motion's virtual store_linear_velocity, slot 20) - the
 * Xbox stand-in for Cartographer's object_set_velocities; the vtable and the function checked before the call */
#ifdef HOST
void host_set_linear_velocity(u32 motion, const float *v);
static void set_body_velocity(u32 motion, const float *v) { host_set_linear_velocity(motion, v); }
#else
static void set_body_velocity(u32 motion, const float *v)
{
	static float v4[4] __attribute__((aligned(16)));
	u32 vt = M32(motion), fn;
	if (!ptr_ok(vt) || vt >= ADDR_DATA_HI) return;
	fn = M32(vt + 20 * 4);
	if (fn < 0x10000 || fn >= ADDR_DATA_LO) return;
	v4[0] = v[0]; v4[1] = v[1]; v4[2] = v[2]; v4[3] = 0.f;
	((void (__attribute__((thiscall)) *)(u32, const float *))fn)(motion, v4);
}
#endif
static s32 put_i(char *b, s32 at, s32 v) { char t[12]; s32 n = 0; if (v < 0) { b[at++] = '-'; v = -v; } do t[n++] = (char)('0' + v % 10); while ((v /= 10) && n < 11); while (n) b[at++] = t[--n]; b[at] = 0; return at; }
/* Cartographer's Disable AI. On the Xbox it holds the AI globals' "enabled" flag (+0) off - the game's own pause
 * switch (the "ai" script command sets it; the AI update, 0x1C83C0, only thinks while it's on) - and not "active"
 * (+1), which keeps the AI's bookkeeping going: holding "active" off and putting it back crashed (4c). Campaign,
 * as in Cartographer; turned back off, the value it had is put back. */
static s32 g_ai_forced_off, g_ai_was;
static void apply_ai(void)
{
	u32 ai = M32(ADDR_AI_GLOBALS_PTR);
	if (ptr_ok(ai) && is_campaign() && player_object())
	{
		if (o_disable_ai)
		{
			if (!g_ai_forced_off) { g_ai_was = M8(ai); g_ai_forced_off = 1; }
			if (M8(ai)) gw8(ai, 0);
		}
		else if (g_ai_forced_off) { gw8(ai, (u8)g_ai_was); g_ai_forced_off = 0; }
	}
	else g_ai_forced_off = 0;
}
/* Invulnerability, as a switch - the game's own flags on your unit's damage flags word (+0x10A), held set while it's
 * on: 0x0080, "cannot take damage" (the object_cannot_take_damage script command's flag - the damage code then scales
 * the damage to 0 and skips shield and body damage), and 0x4000, "cannot die" (body damage stops at 0.0001 and the
 * instant kills of kill volumes / out of bounds are skipped). No health is restored: nothing is taken. A new unit
 * (a respawn) gets them from its first frame; turned off, the two bits go back to what they were. */
#ifdef HOST
static void gw16(u32 a, u16 v) { u32 al = a & ~3u, sh = (a & 3u) * 8u, w = M32(al); gw32(al, (w & ~(0xFFFFu << sh)) | ((u32)v << sh)); }
#else
static void gw16(u32 a, u16 v) { *(volatile u16 *)a = v; }
#endif
static u32 M16(u32 a);
#define IV_BITS 0x4080u
static u32 g_iv_obj, g_iv_was;
static void apply_invuln(void)
{
	u32 obj = player_object();
	if (o_invuln && obj)
	{
		if (obj != g_iv_obj) { g_iv_obj = obj; g_iv_was = M16(obj + 0x10A) & IV_BITS; }
		if ((M16(obj + 0x10A) & IV_BITS) != IV_BITS) gw16(obj + 0x10A, (u16)(M16(obj + 0x10A) | IV_BITS));
	}
	else if (g_iv_obj)
	{
		if (obj == g_iv_obj) gw16(obj + 0x10A, (u16)((M16(obj + 0x10A) & ~IV_BITS) | g_iv_was));
		g_iv_obj = 0;
	}
}
/* ---- Infinite Ammo: the game's own infinite ammo - what a multiplayer game type grants (its reserve counts as full,
 * no overheat lockout) - for any player's weapon, anywhere. function_106030 (eax = the weapon, answer in al) asks
 * "infinite ammo?", and only in a multiplayer game. Its five calls are hooked (in the file, as every hook - 4h wrote
 * into its code at run time, and the Xbox's code pages are read-only: that crashed): with Infinite Ammo on, a weapon
 * in a player's inventory gets yes - the function's own test (in inventory, +0x12C bit 0; its unit, +0x154, has a
 * player, +0x13C) without the multiplayer one; anything else is asked of the game as before. ---- */
s32 ia_answer(u32 weapon_index)                 /* 1: yes; 2: ask the game */
{
	u32 w, unit;
	if (!o_inf_ammo) return 2;
	w = object_from_datum((s32)weapon_index);
	if (!w || !(M8(w + 0x12C) & 1)) return 2;
	unit = object_from_datum((s32)M32(w + 0x154));
	if (!unit || (s32)M32(unit + 0x13C) == -1) return 2;
	return 1;
}
/* ---- Bottomless Clip: a weapon's shot runs in full; if a player carries the weapon (its unit, +0x154, has a player,
 * +0x13C), its magazines' loaded rounds (+0x224 / +0x234, +8) and its battery (+0x180) are noted before and put
 * back after - as Cartographer's invulnerability works around the damage call. AI weapons are left alone. ---- */
static u32 g_bc_w;
static u16 g_bc_rounds[2];
static float g_bc_age;
void bc_before(u32 weapon_index)
{
	u32 w, unit;
	g_bc_w = 0;
	if (!o_bottomless) return;
	w = object_from_datum((s32)weapon_index);
	if (!w) return;
	unit = object_from_datum((s32)M32(w + 0x154));
	if (!unit || (s32)M32(unit + 0x13C) == -1) return;                     /* not a player's */
	g_bc_rounds[0] = (u16)M16(w + 0x224 + 8); g_bc_rounds[1] = (u16)M16(w + 0x234 + 8);
	g_bc_age = MF(w + 0x180);
	g_bc_w = w;
}
void bc_after(void)
{
	u32 w = g_bc_w;
	if (!w) return;
	gw16(w + 0x224 + 8, g_bc_rounds[0]); gw16(w + 0x234 + 8, g_bc_rounds[1]);
	gwf(w + 0x180, g_bc_age);
	g_bc_w = 0;
}
#ifdef HOST
void host_set_ammo(s32 inf, s32 bottomless) { o_inf_ammo = inf; o_bottomless = bottomless; }
#endif
#ifdef HOST
void host_set_invuln(s32 on) { o_invuln = on; }
void host_apply_invuln(void) { apply_invuln(); }
void host_set_disable_ai(s32 on) { o_disable_ai = on; }
void host_apply_ai(void) { apply_ai(); }
#endif
/* ---- Cartographer's third person camera. The view build (function_223080) gets a copy of your camera record moved
 * behind your eye - Cartographer's placement: eye - forward x distance + world up x height + right x side - so only
 * what's drawn moves (aiming and physics use the real record). Only while the camera is at your eye (within 2.5 units
 * of your unit): a cutscene, the death camera or a vehicle's own camera is left alone. The view's first-person object
 * and weapon models are cleared for a moved view - as the game itself does for its third-person views - so your
 * body is drawn and the first-person weapon isn't (Cartographer: the director's perspective reported as "following"
 * to the render code). ---- */
#ifdef HOST
#define TP_BUF(u) (0x1E000u + (u32)(u) * 0x400u)
#else
static u8 g_tp_cam[4][0x360] __attribute__((aligned(16)));
#define TP_BUF(u) ((u32)&g_tp_cam[u][0])
#endif
static u8 g_tp_applied[4];
static u32 user_object(s32 u)                   /* local player u's unit (as player_object, for any of the four) */
{
	u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR), player;
	s32 unit;
	if (!ptr_ok(lp) || u < 0 || u > 3) return 0;
	player = pool_entry(ADDR_PLAYERS_PTR, (s32)M32(lp + 0xC + 4 * (u32)u), 0x30);
	if (!player) return 0;
	unit = (s32)M32(player + 0x2C);
	return unit == -1 ? 0 : object_from_datum(unit);
}
static float ray_first(const float *p0, const float *p1);
static float bsp_ray(const float *p0, const float *p1);
static void view_camera_fc(float *p, float *f);
static float g_tp_inv[4];                      /* 1 / the aim distance the camera converges on, eased */
static float g_tp_pull[4];                     /* with collision: how far out the camera may go (0 your eye .. 1 where you set it) */
static float g_tp_ez[4];                        /* the camera's eye height, smoothed (no shake from bumps) */
static u32 g_count_main;
/* ---- the crash trace: for 3 seconds after a Launch / Teleport, and while you move faster than 8 u/s, a short line at
 * each stage of a frame - written at once - so after a crash the last line says where it stopped (a stage started and
 * not ended: there; a whole frame ended: outside this code). Up to 600 lines a session. ---- */
static u32 g_trace_until;
static s32 g_trace_on;
static void trace_frame(void)               /* decided once a frame, in the per-frame hook */
{
	u32 obj = player_object();
	float vx = 0.f, vy = 0.f, vz = 0.f;
	if (obj) { vx = MF(obj + 0x88); vy = MF(obj + 0x8C); vz = MF(obj + 0x90); }
	g_trace_on = g_count_main < g_trace_until || vx * vx + vy * vy + vz * vz > 64.f;
}
#ifdef HOST
static char g_trace_last[64];
const char *host_trace_last(void) { return g_trace_last; }
#endif
static const char *g_ghost_stage;
static void trace(const char *stage)
{
	g_ghost_stage = stage;
	char line[64];
	s32 n = 0, k;
	u32 v = g_count_main;
	char dg[12];
	if (!g_trace_on || !o_diag) return;                                  /* 6h.21: off on the consoles */
	{ const char *p = "frame "; while (*p) line[n++] = *p++; }
	k = 0; do { dg[k++] = (char)('0' + v % 10); v /= 10; } while (v);
	while (k) line[n++] = dg[--k];
	line[n++] = ':'; line[n++] = ' ';
	while (*stage && n < 60) line[n++] = *stage++;
	line[n++] = '\r'; line[n++] = '\n';
#ifdef HOST
	for (k = 0; k < n && k < 63; k++) g_trace_last[k] = line[k];
	g_trace_last[k] = 0;
#else
	{
		/* spooky_trace.txt, overwritten: after a crash it holds the last stage reached */
		static const char *const paths[3] = { "T:\\spooky_trace.txt", "U:\\spooky_trace.txt", "Z:\\spooky_trace.txt" };
		s32 d; u32 w;
		for (d = g_log_drive < 0 ? 0 : g_log_drive; d < 3; d++)
		{
			void *h = CreateFileA_x(paths[d], 0x40000000, 0, 0, 2 /* CREATE_ALWAYS */, 0x80, 0);
			if (h == (void *)-1) continue;
			WriteFile_x(h, line, (u32)n, &w, 0);
			CloseHandle_x(h);
			break;
		}
	}
#endif
}

static float fc_speed(void)
{
	float sp = 3.f;
	s32 i;
	if (g_fc_speed_steps > 12) g_fc_speed_steps = 12;
	if (g_fc_speed_steps < -8) g_fc_speed_steps = -8;
	for (i = 0; i < g_fc_speed_steps; i++) sp *= 1.5f;
	for (i = 0; i > g_fc_speed_steps; i--) sp /= 1.5f;
	return sp;
}
static float fc_atan2(float y, float x)          /* atan2 on the x87 (fpatan) */
{
#ifdef HOST
	return (float)atan2((double)y, (double)x);
#else
	float r; __asm__("fpatan" : "=t"(r) : "0"(x), "u"(y) : "st(1)"); return r;
#endif
}
static void fc_sincos(float a, float *s, float *c)
{
#ifdef HOST
	*s = (float)sin((double)a); *c = (float)cos((double)a);
#else
	float sv, cv; __asm__("fsincos" : "=t"(cv), "=u"(sv) : "0"(a)); *s = sv; *c = cv;
#endif
}
static void fc_dirs(float yaw, float pitch, float *f, float *r, float *u)
{
	float sy, cy, sp, cp;
	fc_sincos(yaw, &sy, &cy); fc_sincos(pitch, &sp, &cp);
	f[0] = cp * cy; f[1] = cp * sy; f[2] = sp;                           /* forward (Halo: z up) */
	r[0] = sy; r[1] = -cy; r[2] = 0.f;                                   /* right */
	u[0] = -sp * cy; u[1] = -sp * sy; u[2] = cp;                         /* up, square to forward */
}
/* your look inversion: the flying pad's port (the game polls its four ports' handles in order) - that port's gamepad
 * preferences +0x1A (the game's look code multiplies the vertical by -1 when it's set: v1.0 0x2182fa) */
static s32 fc_inverted(void)
{
	s32 k;
	if (!g_fc_pad) return 0;
	for (k = 0; k < 4; k++)
		if (M32(ADDR_PAD_HANDLES + 4 * (u32)k) == (u32)(unsigned long)g_fc_pad) return M8(ADDR_PAD_PREFS + (u32)k * 0x1C + 0x1A) != 0;
	return 0;
}
/* once a frame: started from the view you had, then flown by the pad (when it has it) */
static void fc_frame(void)
{
	const float dt = 1.f / 30.f, rot = 3.f, k = 0.1164f;                 /* HCM's 6% a 60 Hz frame, at 30: 1 - 0.94^2 */
	float f[3], r[3], u[3], sp, dyaw;
	s32 i;
	if (!o_free_cam) { g_fc_on = 0; g_fc_pad = 0; return; }
	if (!g_fc_on)
	{
		float p[3], fw[3], l;
		view_camera_fc(p, fw);
		l = fsqrt(fw[0] * fw[0] + fw[1] * fw[1]);
		g_fc_yaw = g_fc_tyaw = fc_atan2(fw[1], fw[0]);
		g_fc_pitch = g_fc_tpitch = fc_atan2(fw[2], l);
		for (i = 0; i < 3; i++) g_fc_p[i] = g_fc_tp[i] = p[i];
		for (i = 0; i < 6; i++) g_fc_in[i] = 0.f;
		g_fc_on = 1;
		return;
	}
	if (o_fc_lock_pitch && (g_fc_tpitch > 1.55f || g_fc_tpitch < -1.55f))
	{
		/* locked again after full rotations: the pitch back to a view the right way up */
		while (g_fc_tpitch > 3.14159265f) { g_fc_tpitch -= 6.2831853f; g_fc_pitch -= 6.2831853f; }
		while (g_fc_tpitch < -3.14159265f) { g_fc_tpitch += 6.2831853f; g_fc_pitch += 6.2831853f; }
		if (g_fc_tpitch > 1.55f) g_fc_tpitch = 1.55f;
		if (g_fc_tpitch < -1.55f) g_fc_tpitch = -1.55f;
	}
	if (o_fc_block && !g_open)
	{
		g_fc_tyaw -= g_fc_in[2] * rot * dt;
		g_fc_tpitch += g_fc_in[3] * rot * dt * (fc_inverted() ? -1.f : 1.f);   /* your look inversion */
		if (o_fc_lock_pitch)                                              /* locked: stops at straight up / down */
		{
			if (g_fc_tpitch > 1.55f) g_fc_tpitch = 1.55f;
			if (g_fc_tpitch < -1.55f) g_fc_tpitch = -1.55f;
		}
		else if (g_fc_tpitch > 6.2831853f || g_fc_tpitch < -6.2831853f)   /* free: full rotations (kept small, eased alike) */
		{
			float w = g_fc_tpitch > 0.f ? -6.2831853f : 6.2831853f;
			g_fc_tpitch += w; g_fc_pitch += w;
		}
		fc_dirs(g_fc_tyaw, g_fc_tpitch, f, r, u);
		sp = fc_speed();
		for (i = 0; i < 3; i++)
			g_fc_tp[i] += (f[i] * g_fc_in[1] + r[i] * g_fc_in[0]) * sp * dt;
		g_fc_tp[2] += (g_fc_in[4] - g_fc_in[5]) * sp * dt;               /* the triggers: up / down */
	}
	for (i = 0; i < 3; i++) g_fc_p[i] += (g_fc_tp[i] - g_fc_p[i]) * k;
	dyaw = g_fc_tyaw - g_fc_yaw;
	g_fc_yaw += dyaw * k; g_fc_pitch += (g_fc_tpitch - g_fc_pitch) * k;
}
/* the HUD hook: no HUD while the free camera shows the view being drawn */
s32 hud_skip(void)
{
	return o_free_cam && g_fc_on && M32(ADDR_VIEW_USER) == 0 && g_tp_applied[0];
}
#ifdef HOST
void host_set_fc_lock(s32 on) { o_fc_lock_pitch = on; }
float host_fc_stick(s32 v) { return fc_stick((s16)v); }
float host_fc_trigger(s32 v) { return fc_trigger((u8)v); }
void host_req(s32 r) { g_req = r; }
s32 host_req_get(void) { return g_req; }
s32 host_speed_get(void) { return o_speed; }
s32 host_pause_get(void) { return o_pause; }
s32 host_fc_steps(void) { return g_fc_speed_steps; }
void host_set_fc_pad(u32 h) { g_fc_pad = (void *)(unsigned long)h; }
void host_fc_input(float lx, float ly, float rx, float ry, float rt, float lt) { g_fc_in[0] = lx; g_fc_in[1] = ly; g_fc_in[2] = rx; g_fc_in[3] = ry; g_fc_in[4] = rt; g_fc_in[5] = lt; }
void host_fc_pose(float *p, float *yaw, float *pitch) { s32 i; for (i = 0; i < 3; i++) p[i] = g_fc_p[i]; *yaw = g_fc_yaw; *pitch = g_fc_pitch; }
void host_set_free_cam(s32 on, s32 block) { o_free_cam = on; o_fc_block = block; }
s32 host_free_cam(void) { return o_free_cam; }
#endif
/* ---- a vehicle you've possessed (6h.11): your unit itself a vehicle, in no seat (+0x14 its parent: none; +0xAA its type,
 * 1 a vehicle - as the director reads them). With third person on (and not the free camera) the director is told to use its
 * following camera for it (its first-person / following choice, v1.0 0x155810: ebx the unit, its answer 1 and a word 1) -
 * which, for a unit in no seat, follows the unit itself by its own definition's camera (+0xD4, v1.0 0x1871e0): the
 * vehicle's own third person view, with its own collision. A biped: first person as ever, and Spooky Ghost's camera. ---- */
static s32 lone_vehicle(u32 obj) { return obj && M8(obj + 0xAA) == 1 && (s32)M32(obj + 0x14) == -1; }
static s32 vehicle_view_on(void) { return o_third_person && !(o_free_cam && g_fc_on); }
s32 g_bp_switching_c;
s32 cam_mode_answer(s32 game, u32 unit, u32 out)
{
	if ((game & 0xffff) == 1 || g_bp_switching_c || !vehicle_view_on() || !lone_vehicle(object_from_datum((s32)unit))) return game;
	if (out) gw16(out, 1);
	return (s32)(((u32)game & 0xffff0000u) | 1u);
}
/* 6h.12: the following camera's target (v1.0 0x1871e0: eax the player, esi its record - +0 the unit followed, +4 its seat or
 * -1, +8 the camera settings, +0xC a place the game fills from a marker) - for a unit in no seat it gives the unit's own
 * definition's camera (+0xD4); for a possessed vehicle that's not its driving view (off in multiplayer, and the controls,
 * which read the same settings, with it). So: the driver seat's (the first seat flagged driver, bit 2; else seat 0) - its
 * camera at seat +0x60, as the game gives a seated driver, and its marker step (v1.0 0xb9ef0) when the seat is a third
 * person driver's (bits 4 and 2), as the game takes it. Seats: definition +0x1C8 count, +0x1CC pointer, 0xB0 bytes each. */
void cam_marker_call(u32 datum, u32 place);
#ifdef HOST
u32 g_host_marker_calls;
void cam_marker_call(u32 datum, u32 place) { (void)datum; (void)place; g_host_marker_calls++; }
#endif
void cam_target_fix(u32 rec)
{
	/* 6h.14: only the camera settings swapped (+8). 6h.12 also gave the record the seat's number (+4) and ran the seat's
	 * marker step - a record that says "seated" for a unit in no seat; switching bodies out of a possessed Banshee then read
	 * through it and crashed. The seat number stays -1, as the game has it for a unit in no seat. */
	u32 obj, def, seats, n, i, seat = 0;
	if (!rec || g_bp_switching_c || !vehicle_view_on() || (s16)M16(rec + 4) != -1) return;
	obj = object_from_datum((s32)M32(rec));
	if (!lone_vehicle(obj)) return;
	def = M32(M32(ADDR_TAG_INSTANCES) + (M32(obj) & 0xffff) * 16 + 8);
	if (!ptr_ok(def)) return;
	n = M32(def + 0x1C8); seats = M32(def + 0x1CC);
	if (!n || n > 64 || !ptr_ok(seats)) return;
	for (i = 0; i < n; i++) if (M32(seats + i * 0xB0) & 4) { seat = i; break; }
	gw32(rec + 8, seats + seat * 0xB0 + 0x60);
}
/* ---- 6h.25: smooth slow motion at 30 ticks - frame interpolation. Below 1.0 the game runs fewer 1/30 s ticks than
 * frames, and draws each tick's state as it is (no in-between frames on the Xbox): it stepped. Now each frame is drawn
 * between the last two ticks, by how far the next one has come (the time globals' leftover_ticks, +0x10): every object
 * near you has its node matrices (object +0x116 offset, +0x114 size, 0x34 each) snapshotted before each tick (the tick
 * call in the tick loop, v1.0 0x137f89), blended around the draw (the frame's call to it, v1.0 0x12b659) and put back
 * after - the game's state the next tick sees is its own, the 30 ticks untouched. The camera: its eye position the
 * same way, from its record's own tick history (its turning is already a frame's: the player controls run every
 * frame, v1.0 0x1870a0); the first-person weapon is built from that camera at draw time (0x165f16), so it follows.
 * One tick behind (as any interpolation); only below 1.0, not paused - at full speed nothing changes. ---- */
static s32 ip_on(void)
{
	u32 opt = M32(ADDR_GAME_OPTIONS_PTR);
	return o_speed > 0 && o_speed < 10 && !o_pause && ptr_ok(opt) && M32(opt + 8) != 3;
}
static float ip_alpha(u32 time)                 /* how far the next tick has come: 0 .. 1 */
{
	float a = MF(time + 0x10);
	return a > 0.f ? (a < 1.f ? a : 1.f) : 0.f;
}
/* 6h.26: the camera follows its own unit's blend - the eye moved by exactly what the unit's root node is moved by (its
 * blended node 0 less its real one), so the first-person legs (the unit's own node matrices, copied as they are) and
 * the camera agree every frame; per-frame camera changes (pitch, shake) pass through untouched. (6h.25 blended the
 * record's own history: a record moving within a tick switched it off - the camera jumped between the two, the legs
 * shook.) Set by the blend (ip_blend), cleared after the draw. */
static u8 g_ip_frame;                          /* this frame is drawn blended */
static u8 g_ip_uhas[4];                        /* user u's unit was in the snapshot */
static float g_ip_uoff[4][3];                  /* its root's blend offset (0 when it wasn't blended) */
static float g_ip_ufwd[4][2];                  /* its facing, blended (x, y) */
static u8 g_ip_camok[4];                       /* this frame's first-person camera noted (below) */
static float g_ip_camp[4][3], g_ip_camf[4][3], g_ip_camu[4][3];
static u8 g_ip_cam[4];                         /* this frame's view drawn from TP_BUF, blended (not a moved view) */
static s32 ip_eye_offset(s32 user, u32 cam, float *off)
{
	(void)cam;
	off[0] = off[1] = off[2] = 0.f;
	if (user < 0 || user > 3 || !g_ip_frame || !g_ip_uhas[user]) return 0;
	off[0] = g_ip_uoff[user][0]; off[1] = g_ip_uoff[user][1]; off[2] = g_ip_uoff[user][2];
	return off[0] != 0.f || off[1] != 0.f || off[2] != 0.f;
}
/* the game's own view (first person, a scope, a possessed vehicle's): a copy with the eye blended */
static u32 ip_view(s32 user, u32 cam, s32 ipo, const float *off)
{
	u32 b;
	s32 i;
	if (g_ip_frame)                                                      /* the first-person camera, for the legs' heading */
	{
		for (i = 0; i < 3; i++)
		{
			g_ip_camp[user][i] = MF(cam + 4 * (u32)i) + off[i];
			g_ip_camf[user][i] = MF(cam + 0x20 + 4 * (u32)i); g_ip_camu[user][i] = MF(cam + 0x2C + 4 * (u32)i);
		}
		g_ip_camok[user] = 1;
	}
	if (!ipo) return cam;
	b = TP_BUF(user);
	for (i = 0; i < 0x358; i += 4) gw32(b + (u32)i, M32(cam + (u32)i));
	for (i = 0; i < 3; i++) gwf(b + 4 * (u32)i, MF(cam + 4 * (u32)i) + off[i]);
	g_ip_cam[user] = 1;
	return b;
}
u32 tp_camera(u32 cam, s32 user)
{
	trace("third person camera");
	s32 was;
	u32 obj, b;
	float eye[3], f[3], r[3], l, d2 = 0.f, off[3];
	s32 i, ipo;
	if (user < 0 || user > 3) return cam;
	was = g_tp_applied[user];
	g_tp_applied[user] = 0;
	g_ip_cam[user] = 0;
	g_ip_camok[user] = 0;
	if (!ptr_ok(cam)) return cam;
	ipo = ip_eye_offset(user, cam, off);                                     /* 6h.25: the eye between the last two ticks */
	if (o_free_cam && user == 0 && g_fc_on)
	{
		/* the free camera: your view, from where it's been flown (it took no notice of the zoom: a free view) */
		float fw[3], rt[3], up[3];
		obj = user_object(user);
		if (!obj) return cam;
		for (i = 0; i < 3; i++) { float d = MF(cam + 4 * (u32)i) - MF(obj + 0x64 + 4 * (u32)i); d2 += d * d; }
		if (!(d2 < 2.5f * 2.5f)) return cam;                                 /* not your eye (a cutscene): left alone */
		b = TP_BUF(user);
		for (i = 0; i < 0x358; i += 4) gw32(b + (u32)i, M32(cam + (u32)i));
		fc_dirs(g_fc_yaw, g_fc_pitch, fw, rt, up);
		for (i = 0; i < 3; i++) { gwf(b + 4 * (u32)i, g_fc_p[i]); gwf(b + 0x20 + 4 * (u32)i, fw[i]); gwf(b + 0x2C + 4 * (u32)i, up[i]); }
		g_tp_applied[user] = 1;
		return b;
	}
	if (!o_third_person) return ip_view(user, cam, ipo, off);
	obj = user_object(user);
	if (!obj) return ip_view(user, cam, ipo, off);
	if (lone_vehicle(obj)) return ip_view(user, cam, ipo, off);             /* a possessed vehicle: the game's own view of it */
	/* zoomed (a scope): first person - the unit's current / desired zoom level (+0x240 / +0x241), -1 when not
	 * (function_c86e0 resets them so) */
	if ((s8)M8(obj + 0x240) != -1 || (s8)M8(obj + 0x241) != -1) return ip_view(user, cam, ipo, off);
	for (i = 0; i < 3; i++)
	{
		float d;
		eye[i] = MF(cam + 4 * (u32)i); f[i] = MF(cam + 0x20 + 4 * (u32)i);
		d = eye[i] - MF(obj + 0x64 + 4 * (u32)i); d2 += d * d;
	}
	if (!(d2 < 2.5f * 2.5f)) return ip_view(user, cam, ipo, off);           /* not at your eye: as the game has it */
	l = fsqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	if (l < 1e-4f) return ip_view(user, cam, ipo, off);
	for (i = 0; i < 3; i++) eye[i] += off[i];                                /* 6h.25: the eye blended (0 when not) */
	f[0] /= l; f[1] /= l; f[2] /= l;
	r[0] = f[1]; r[1] = -f[0]; r[2] = 0.f;                                  /* forward x world up */
	l = fsqrt(r[0] * r[0] + r[1] * r[1]);
	if (l < 0.5f) { r[0] = 0.f; r[1] = -1.f; } else { r[0] /= l; r[1] /= l; }
	b = TP_BUF(user);
	for (i = 0; i < 0x358; i += 4) gw32(b + (u32)i, M32(cam + (u32)i));
	{
		float back = (float)o_tp_dist * 0.1f, up = (float)o_tp_height * 0.1f, side = (float)o_tp_side * 0.1f;
		float p[3], a0[3], a1[3], t, aim, tg[3], nf[3], u0[3], nu[3], k, l2;
		/* the eye's height smoothed: predicted from your vertical speed (falls, jumps and slopes followed exactly), only
		 * the difference from that eased (a quarter a frame) - stepping over things bumps the eye, and the camera with
		 * it; a jump over half a unit (a teleport, a respawn) taken at once */
		{
			float pred = g_tp_ez[user] + MF(obj + 0x90) / 30.f, dz = eye[2] - pred;
			if (!was || dz > 0.5f || dz < -0.5f) g_tp_ez[user] = eye[2];
			else g_tp_ez[user] = pred + dz * 0.25f;
		}
		p[0] = eye[0] - f[0] * back + r[0] * side;
		p[1] = eye[1] - f[1] * back + r[1] * side;
		p[2] = g_tp_ez[user] - f[2] * back + up;
		if (!o_tp_nocoll)
		{
			/* "Ignore Collision" off (6h.11) - as a vehicle's camera: pulled in along the line from your (smoothed) eye when
			 * the level (its collision BSP, walked) or the pieces near you (when the Superbounce viewer has them) are in the
			 * way, a quarter unit short of the wall; at once when it closes in, eased back out (a sixth of the way a frame) */
			float pv[3], dv[3], q[3], len, tt, t2, allow;
			const float m = 0.25f;
			pv[0] = eye[0]; pv[1] = eye[1]; pv[2] = g_tp_ez[user];
			for (i = 0; i < 3; i++) dv[i] = p[i] - pv[i];
			len = fsqrt(dv[0] * dv[0] + dv[1] * dv[1] + dv[2] * dv[2]);
			if (len > 0.01f)
			{
				for (i = 0; i < 3; i++) q[i] = pv[i] + dv[i] * ((len + m) / len);
				tt = bsp_ray(pv, q); t2 = ray_first(pv, q);
				if (t2 < tt) tt = t2;
				allow = tt <= 1.f ? (tt * (len + m) - m) / len : 1.f;
				if (allow < 0.f) allow = 0.f;
				if (allow > 1.f) allow = 1.f;
				if (!was || allow < g_tp_pull[user]) g_tp_pull[user] = allow;
				else g_tp_pull[user] += (allow - g_tp_pull[user]) * (1.f / 6.f);
				for (i = 0; i < 3; i++) p[i] = pv[i] + dv[i] * g_tp_pull[user];
			}
		}
		else g_tp_pull[user] = 1.f;
		gwf(b + 0, p[0]); gwf(b + 4, p[1]); gwf(b + 8, p[2]);
		/* aimed at what you aim at - so the reticle at the centre is where your shots go, the camera off to a side or
		 * not: the first wall of the level along your aim from the eye (from 1 unit out, past your own vehicle), up to 500
		 * units (6h.9 - the level's collision BSP walked; it was the surfaces within 10, else 30 units out, and past those a
		 * camera moved right showed your shots off to the left); the pieces and objects near you too, when the Superbounce
		 * viewer has them (up to 10); nothing in the way: 500 out */
		for (i = 0; i < 3; i++) { a0[i] = eye[i] + f[i] * 1.f; a1[i] = eye[i] + f[i] * 500.f; }
		t = bsp_ray(a0, a1);
		aim = t <= 1.f ? 1.f + 499.f * t : 500.f;
		for (i = 0; i < 3; i++) a1[i] = eye[i] + f[i] * 10.f;
		t = ray_first(a0, a1);
		if (t <= 1.f && 1.f + 9.f * t < aim) aim = 1.f + 9.f * t;
		/* eased, and not under 3: the camera's angle doesn't jump when your aim crosses an edge (or aim assist moves it
		 * across one) - each frame a third of the way, in 1 / distance (what the angle goes by: far changes settle fast) */
		if (aim < 3.f) aim = 3.f;
		if (!was) g_tp_inv[user] = 1.f / aim; else g_tp_inv[user] += (1.f / aim - g_tp_inv[user]) * 0.35f;
		aim = 1.f / g_tp_inv[user];
		for (i = 0; i < 3; i++) tg[i] = eye[i] + f[i] * aim;
		for (i = 0; i < 3; i++) nf[i] = tg[i] - p[i];
		l2 = fsqrt(nf[0] * nf[0] + nf[1] * nf[1] + nf[2] * nf[2]);
		if (l2 > 1e-3f)
		{
			for (i = 0; i < 3; i++) { nf[i] /= l2; u0[i] = MF(cam + 0x2C + 4 * (u32)i); }
			k = u0[0] * nf[0] + u0[1] * nf[1] + u0[2] * nf[2];                /* up, squared to the new forward */
			for (i = 0; i < 3; i++) nu[i] = u0[i] - nf[i] * k;
			l2 = fsqrt(nu[0] * nu[0] + nu[1] * nu[1] + nu[2] * nu[2]);
			if (l2 > 1e-3f)
				for (i = 0; i < 3; i++) { gwf(b + 0x20 + 4 * (u32)i, nf[i]); gwf(b + 0x2C + 4 * (u32)i, nu[i] / l2); }
		}
	}
	g_tp_applied[user] = 1;
	return b;
}
/* the effect code asks for a weapon's first-person marker: "no" for a weapon held (up its parent chain, +0x14) by a
 * player whose view is in third person, so its muzzle flash / casings / effects come from the weapon in the world.
 * 6h.23: also the effect draw pass's call (v1.0 0x31d6a: it ignores the answer and takes the marker when one comes back -
 * the left-hand weapon's, while dual wielding) - "no" leaves the effect where its update put it: the weapon in the world */
s32 fpm_answer(u32 object_index)                /* 0: no; 2: ask the game */
{
	s32 cur = (s32)object_index, root = -1, guard = 0, u;
	u32 ro;
	if (!o_third_person && !o_free_cam) return 2;                        /* a moved view: third person or the free camera */
	while (cur != -1 && guard++ < 8) { u32 o = object_from_datum(cur); if (!o) break; root = cur; cur = (s32)M32(o + 0x14); }
	if (root == -1 || !(ro = object_from_datum(root))) return 2;
	for (u = 0; u < 4; u++) if (g_tp_applied[u] && user_object(u) == ro) return 0;
	return 2;
}
/* Cartographer's perspective hook: the director's perspective reported as "following" (3) to the render code (your
 * own body drawn), the first-person weapon code (the first-person weapon inactive - its muzzle flash, casings and
 * effects from the gun in the world, as in a vehicle's third person) and the camera effect code (no screen shake,
 * melee or recoil kicks) - for a player whose view is in third person. 6h.23: and the object attachments' first-person
 * tests (lens flares, light volumes, particle and effect attachments on a weapon): which instances are drawn (v1.0
 * 0x17430c), which are made (0x178f06, 0x179071) and their markers' model (0x179942) - in a moved view the
 * first-person ones were drawn on screen (the left-hand weapon's) or the weapon's had none (the right). The HUD, aiming
 * and the rest get the real answer (their calls aren't hooked). */
s32 persp_answer(s32 user)                       /* 3: following; -1: the game's own answer */
{
	return (o_third_person || o_free_cam) && user >= 0 && user < 4 && g_tp_applied[user] ? 3 : -1;   /* third person or the free camera */
}
void tp_fp_after(void)                          /* the view's first-person object and models, cleared for a moved view */
{
	s32 u = (s32)M32(ADDR_VIEW_USER);
	if (u >= 0 && u < 4 && g_tp_applied[u]) { gw32(ADDR_FP_OBJECT, 0xFFFFFFFFu); gw32(ADDR_FP_MODELS, 0); }
}
/* the camera the world is drawn from: the moved one in third person, else your eye */
static void view_camera(float *p, float *f, float *u);
static void view_camera_fc(float *p, float *f) { view_camera(p, f, 0); }
static void view_camera(float *p, float *f, float *u)
{
	u32 b = (g_tp_applied[0] || g_ip_cam[0]) ? TP_BUF(0) : ADDR_PLAYER_CAMERAS;
	s32 i;
	for (i = 0; i < 3; i++)
	{
		if (p) p[i] = MF(b + 4 * (u32)i);
		if (f) f[i] = MF(b + 0x20 + 4 * (u32)i);
		if (u) u[i] = MF(b + 0x2C + 4 * (u32)i);
	}
}
/* 6h.25: the objects near the view (bounding sphere within IP_RANGE of the camera) - node matrices before the tick */
#define IP_NODE_CAP 6144
#define IP_ENT_CAP 1536
#define IP_RANGE 80.f
typedef struct { u32 datum, first, count, cur; float fwd[2], org[3]; } s_ip_ent;   /* cur: the matrices' address while blended (0: not); fwd: its facing (+0x70); org: its origin (+0x64) */
static s_ip_ent *g_ip_ent;
static u32 *g_ip_prev, *g_ip_back;              /* 13 words a node: before the tick; the real ones while blended */
static s32 g_ip_n, g_ip_nodes, g_ip_valid, g_ip_blended, g_ip_mem;   /* g_ip_mem: 0 not asked, 1 ok, -1 failed */
static u32 g_ip_tick;
static s32 ip_alloc(void)
{
	if (g_ip_mem) return g_ip_mem > 0;
#ifdef HOST
	{
		static s_ip_ent e[IP_ENT_CAP];
		static u32 pv[IP_NODE_CAP * 13], bk[IP_NODE_CAP * 13];
		g_ip_ent = e; g_ip_prev = pv; g_ip_back = bk;
	}
#else
	{
		u32 sz = IP_ENT_CAP * (u32)sizeof(s_ip_ent) + 2u * IP_NODE_CAP * 13u * 4u;
		u8 *m = (u8 *)VirtualAlloc_x(0, sz, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 4 /* PAGE_READWRITE */);
		if (!m) { g_ip_mem = -1; lput(0, "smooth slow motion: no memory for it (the picture steps)"); log_write(); return 0; }
		g_ip_ent = (s_ip_ent *)m; g_ip_prev = (u32 *)(m + IP_ENT_CAP * sizeof(s_ip_ent)); g_ip_back = g_ip_prev + IP_NODE_CAP * 13;
	}
#endif
	g_ip_mem = 1;
	return 1;
}
static void ip_snapshot(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), pool = M32(ADDR_OBJECTS_PTR), data, count, i;
	float ref[3];
	g_ip_valid = 0;
	if (!ip_on() || !ip_alloc() || !ptr_ok(time) || !ptr_ok(pool)) return;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || M32(pool + 0x24) != 12 || count > 8192) return;
	view_camera(ref, 0, 0);
	g_ip_n = 0; g_ip_nodes = 0;
	for (i = 0; i < count && g_ip_n < IP_ENT_CAP; i++)
	{
		u32 h = data + i * 12, salt = M32(h) & 0xffff, obj, n, noff, src, w, k;
		float dx, dy, dz, r;
		if (!salt) continue;                                                  /* a free slot */
		obj = M32(h + 8);
		if (!ptr_ok(obj)) continue;
		n = M16(obj + 0x114); noff = M16(obj + 0x116);
		if (!n || n % 0x34 || !noff || noff > 0x4000) continue;
		n /= 0x34;
		if ((u32)g_ip_nodes + n > IP_NODE_CAP) continue;
		dx = MF(obj + 0x30) - ref[0]; dy = MF(obj + 0x34) - ref[1]; dz = MF(obj + 0x38) - ref[2]; r = MF(obj + 0x3C) + IP_RANGE;
		if (!(r > 0.f && dx * dx + dy * dy + dz * dz < r * r)) continue;     /* far: drawn as it is */
		src = obj + noff; w = (u32)g_ip_nodes * 13u;
		for (k = 0; k < n * 13u; k++) g_ip_prev[w + k] = M32(src + 4 * k);
		g_ip_ent[g_ip_n].datum = (salt << 16) | i; g_ip_ent[g_ip_n].first = (u32)g_ip_nodes; g_ip_ent[g_ip_n].count = n; g_ip_ent[g_ip_n].cur = 0;
		g_ip_ent[g_ip_n].fwd[0] = MF(obj + 0x70); g_ip_ent[g_ip_n].fwd[1] = MF(obj + 0x74);
		g_ip_ent[g_ip_n].org[0] = MF(obj + 0x64); g_ip_ent[g_ip_n].org[1] = MF(obj + 0x68); g_ip_ent[g_ip_n].org[2] = MF(obj + 0x6C);
		g_ip_nodes += (s32)n; g_ip_n++;
	}
	g_ip_tick = M32(time + 8);
	g_ip_valid = 1;
}
static void ip_blend(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), i, k, uo[4];
	float a;
	s32 u;
	g_ip_blended = 0;
	g_ip_frame = 0;
	for (u = 0; u < 4; u++) { g_ip_uhas[u] = 0; g_ip_uoff[u][0] = g_ip_uoff[u][1] = g_ip_uoff[u][2] = 0.f; }
	if (!g_ip_valid || !ip_on() || !ptr_ok(time) || M32(time + 8) != g_ip_tick + 1) return;   /* the snapshot is the tick before */
	a = ip_alpha(time);
	g_ip_frame = 1;
	for (u = 0; u < 4; u++) uo[u] = user_object(u);
	for (i = 0; i < (u32)g_ip_n; i++)
	{
		s_ip_ent *e = &g_ip_ent[i];
		u32 obj = object_from_datum((s32)e->datum), cur, w = e->count * 13u, same = 1, *pv = g_ip_prev + e->first * 13u, *bk = g_ip_back + e->first * 13u;
		union { u32 u; float f; } x, y;
		float d0, d1, d2;
		e->cur = 0;
		if (!obj || M16(obj + 0x114) != e->count * 0x34) continue;            /* gone, or another object */
		cur = obj + M16(obj + 0x116);
		for (u = 0; u < 4; u++) if (uo[u] && uo[u] == obj)                 /* a local player's unit: its blended facing */
		{
			g_ip_uhas[u] = 1;
			g_ip_ufwd[u][0] = e->fwd[0] + (MF(obj + 0x70) - e->fwd[0]) * a; g_ip_ufwd[u][1] = e->fwd[1] + (MF(obj + 0x74) - e->fwd[1]) * a;
		}
		x.u = pv[10]; d0 = MF(cur + 0x28) - x.f; x.u = pv[11]; d1 = MF(cur + 0x2C) - x.f; x.u = pv[12]; d2 = MF(cur + 0x30) - x.f;
		if (!(d0 * d0 + d1 * d1 + d2 * d2 < 16.f)) continue;                 /* a jump of 4 units or more: at once */
		for (k = 0; k < w; k++) { bk[k] = M32(cur + 4 * k); if (bk[k] != pv[k]) same = 0; }
		if (same) continue;                                                    /* still: nothing to blend */
		e->cur = cur; g_ip_blended++;                                          /* (before writing: a fault still puts it back) */
		for (k = 0; k < w; k++) { x.u = pv[k]; y.u = bk[k]; gwf(cur + 4 * k, x.f + (y.f - x.f) * a); }
		for (u = 0; u < 4; u++) if (uo[u] == obj)                          /* the camera's offset: its origin's (on foot - the */
		{                                                                  /* game's eye is from it, not the bobbing pelvis), */
			if ((s32)M32(obj + 0x14) == -1)                                 /* else its root node's (a seat: world space) */
				for (k = 0; k < 3; k++) g_ip_uoff[u][k] = (e->org[k] - MF(obj + 0x64 + 4 * k)) * (1.f - a);
			else
				for (k = 0; k < 3; k++) { x.u = pv[10 + k]; y.u = bk[10 + k]; g_ip_uoff[u][k] = (x.f - y.f) * (1.f - a); }
			{ float d = g_ip_uoff[u][0] * g_ip_uoff[u][0] + g_ip_uoff[u][1] * g_ip_uoff[u][1] + g_ip_uoff[u][2] * g_ip_uoff[u][2];
			  if (!(d < 16.f)) g_ip_uoff[u][0] = g_ip_uoff[u][1] = g_ip_uoff[u][2] = 0.f; }   /* a jump: none */
		}
	}
}
static void ip_restore(void)
{
	u32 i, k;
	g_ip_frame = 0;
	if (!g_ip_blended) return;
	for (i = 0; i < (u32)g_ip_n; i++)
	{
		s_ip_ent *e = &g_ip_ent[i];
		u32 *bk = g_ip_back + e->first * 13u;
		if (!e->cur) continue;
		for (k = 0; k < e->count * 13u; k++) gw32(e->cur + 4 * k, bk[k]);
		e->cur = 0;
	}
	g_ip_blended = 0;
}
/* 6h.26: the first-person legs - the game copies the unit's node matrices, as they are, into the first-person body
 * (0x165f16: no base, world space). Your turning is a frame's (real time, 0x185be0: rate x the frame's seconds), your
 * body's facing a tick's: slowed, the legs trailed the view. For the first-person build only (the setup's call,
 * v1.0 0x2badc), on foot, the unit's (blended) nodes are turned about the camera's upright line to its heading, then
 * put back: the legs under you, facing where you look - as at full speed, where the body takes the view's facing each
 * tick. The heading from forward and up together (straight down, forward has none: up points along it). */
#define IP_FP_CAP 160
static u32 g_ip_fpbk[IP_FP_CAP * 13], g_ip_fp_at, g_ip_fp_n;
static void ip_fp_turn(void)
{
	s32 u = (s32)M32(ADDR_VIEW_USER);
	u32 obj, n, noff, m, k;
	float hx, hy, bx, by, l, cs, sn, px, py, fz, c;
	g_ip_fp_n = 0;
	if (u < 0 || u > 3 || !g_ip_frame || !g_ip_camok[u] || g_tp_applied[u] || !ip_on()) return;
	obj = user_object(u);
	if (!obj || (s32)M32(obj + 0x14) != -1) return;                         /* on foot */
	n = M16(obj + 0x114); noff = M16(obj + 0x116);
	if (!n || n % 0x34 || !noff || noff > 0x4000 || n / 0x34 > IP_FP_CAP) return;
	n /= 0x34;
	fz = g_ip_camf[u][2]; c = fsqrt(g_ip_camf[u][0] * g_ip_camf[u][0] + g_ip_camf[u][1] * g_ip_camf[u][1]);
	hx = g_ip_camf[u][0] * c - g_ip_camu[u][0] * fz; hy = g_ip_camf[u][1] * c - g_ip_camu[u][1] * fz;
	l = fsqrt(hx * hx + hy * hy); if (l < 1e-3f) return; hx /= l; hy /= l;
	if (g_ip_uhas[u]) { bx = g_ip_ufwd[u][0]; by = g_ip_ufwd[u][1]; } else { bx = MF(obj + 0x70); by = MF(obj + 0x74); }
	l = fsqrt(bx * bx + by * by); if (l < 1e-3f) return; bx /= l; by /= l;
	cs = bx * hx + by * hy; sn = bx * hy - by * hx;                        /* the turn from the body's facing to the view's */
	if (cs > 0.99999f && sn < 1e-5f && sn > -1e-5f) return;
	px = g_ip_camp[u][0]; py = g_ip_camp[u][1];
	m = obj + noff;
	for (k = 0; k < n * 13u; k++) g_ip_fpbk[k] = M32(m + 4 * k);
	g_ip_fp_at = m; g_ip_fp_n = n;                                         /* (before writing: a fault still puts it back) */
	for (k = 0; k < n; k++)
	{
		u32 nd = m + k * 0x34, v;
		float x = MF(nd + 0x28) - px, y = MF(nd + 0x2C) - py;
		gwf(nd + 0x28, px + x * cs - y * sn); gwf(nd + 0x2C, py + x * sn + y * cs);
		for (v = 0x04; v <= 0x1C; v += 0x0C)                                /* forward, left, up: turned about the upright */
		{
			float vx = MF(nd + v), vy = MF(nd + v + 4);
			gwf(nd + v, vx * cs - vy * sn); gwf(nd + v + 4, vx * sn + vy * cs);
		}
	}
}
static void ip_fp_back(void)
{
	u32 k;
	if (!g_ip_fp_n) return;
	for (k = 0; k < g_ip_fp_n * 13u; k++) gw32(g_ip_fp_at + 4 * k, g_ip_fpbk[k]);
	g_ip_fp_n = 0;
}
void ip_fp_before(void) { guarded_call(ip_fp_turn); }
void ip_fp_after(void) { guarded_call(ip_fp_back); }
void ip_tick_before(void) { if (guarded_call(ip_snapshot)) g_ip_valid = 0; }      /* the hooks (hooks.S): a bad read caught */
void ip_draw_before(void) { if (guarded_call(ip_blend)) { } }
void ip_draw_after(void) { guarded_call(ip_restore); }
#ifdef HOST
s32 host_ip_blended(void) { return g_ip_blended; }
s32 host_ip_snapped(void) { return g_ip_valid ? g_ip_n : -1; }
#endif
/* Infinite Ammo includes grenades: a player's two grenade counts (+0x23E) held at 4 (the most a player carries) */
static void apply_grenades(void)
{
	s32 u, k;
	if (!o_inf_ammo) return;
	for (u = 0; u < 4; u++)
	{
		u32 unit = user_object(u);
		if (!unit) continue;
		for (k = 0; k < 2; k++) if ((s8)M8(unit + 0x23E + (u32)k) < 4) gw8(unit + 0x23E + (u32)k, 4);
	}
}
#ifdef HOST
void host_apply_grenades(void) { apply_grenades(); }
#endif
#ifdef HOST
void host_set_third_person(s32 on, s32 dist, s32 height, s32 side) { o_third_person = on; o_tp_dist = dist; o_tp_height = height; o_tp_side = side; }
void host_set_tp_nocoll(s32 on) { o_tp_nocoll = on; }
void host_set_diag(s32 on) { o_diag = on; }
void host_set_speed(s32 tenths, s32 pause) { o_speed = tenths; o_pause = pause; }
u32 host_tp_camera(u32 cam, s32 user) { return tp_camera(cam, user); }
void host_tp_fp_after(void) { tp_fp_after(); }
#endif

static void practice(s32 what);
static void offsets_request(s32 save);
static void revert_to_player(void);
static void cycle_possession(void);
static void passion_check(void);
#ifdef HOST
void host_practice(s32 what) { practice(what); }
const char *host_toast(void) { return g_toast; }
void host_set_practice(s32 lf, s32 lu, s32 tf, s32 tu) { o_launch_fwd = lf; o_launch_up = lu; o_tp_fwd = tf; o_tp_up = tu; }
#endif
static void practice_log(s32 what)
{
	if (what == 3 || what == 4 || what == 7) g_trace_until = g_count_main + 90;   /* 3 s of trace after a launch / teleport */
	static const char *name[13] = { "", "create checkpoint", "revert checkpoint", "launch", "teleport", "third person on / off", "free camera on / off", "teleport to the camera", "revert to player possession", "passion check", "save offsets", "revert offsets", "cycle possession" };
	s32 at;
	if (what < 1 || what > 12) return;
	if (!o_diag && (what == 3 || what == 4 || what == 7)) return;           /* 6h.21: not each launch / teleport */
	at = lput(0, "practice: "); lput(at, name[what]);
	log_write();
}
static void sb_launch_delta(const float *d);
static void practice(s32 what)
{
	u32 obj, target, ents[16];
	practice_log(what);
	s32 parent, n, i;
	float f[3], r[3], up[3], l;
	char msg[80];
	if (what == 5) { o_third_person = !o_third_person; toast(o_third_person ? "Third person camera: on" : "Third person camera: off"); return; }
	if (what == 6) { o_free_cam = !o_free_cam; toast(o_free_cam ? "Free camera: on" : "Free camera: off"); return; }
	if (what == 7 && !(o_free_cam && g_fc_on)) { toast("Teleport to camera: the free camera is off"); return; }
	if (what == 8) { revert_to_player(); return; }
	if (what == 12) { cycle_possession(); return; }
	if (what == 9) { passion_check(); return; }
	if (what == 10 || what == 11) { offsets_request(what == 10); return; }
	if (what <= 2)
	{
		if (!player_object()) { toast("Checkpoints: no game in progress"); return; }
		gw8(what == 1 ? ADDR_SAVE_MAP : ADDR_SAVE_MAP - 1, 1);
		toast(what == 1 ? "Checkpoint created" : "Reverted to the last checkpoint");
		return;
	}
	obj = player_object();
	if (!obj) { toast("Launch / Teleport: no player unit"); return; }
	target = obj; parent = (s32)M32(obj + 0x14);                             /* seated: the vehicle */
	if (parent != -1) { u32 v = object_from_datum(parent); if (v) target = v; }
	/* your view: forward, right = forward x up, up = right x forward */
	for (i = 0; i < 3; i++) f[i] = MF(ADDR_PLAYER_CAMERAS + 0x20 + 4 * i);
	l = fsqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	if (l < 1e-4f) { f[0] = 1.f; f[1] = f[2] = 0.f; } else { f[0] /= l; f[1] /= l; f[2] /= l; }
	r[0] = f[1]; r[1] = -f[0]; r[2] = 0.f;
	l = fsqrt(r[0] * r[0] + r[1] * r[1]);
	if (l < 1e-4f) { r[0] = 1.f; r[1] = 0.f; } else { r[0] /= l; r[1] /= l; }
	up[0] = r[1] * f[2] - r[2] * f[1]; up[1] = r[2] * f[0] - r[0] * f[2]; up[2] = r[0] * f[1] - r[1] * f[0];
	n = object_bodies(target, ents);
	if (o_diag)                                                          /* 6h.21: a development line */
	{
		/* the log: the bodies in the world of those listed, and where */
		s32 at = lput(0, "practice: bodies in the physics world "); at = lnum(at, (u32)n); at = lput(at, " of "); at = lnum(at, (u32)g_bodies_listed);
		at = lput(at, ", position x100 "); at = lsnum(at, (s32)(MF(target + 0x64) * 100.f)); at = lput(at, " "); at = lsnum(at, (s32)(MF(target + 0x68) * 100.f));
		at = lput(at, " "); lsnum(at, (s32)(MF(target + 0x6C) * 100.f));
		log_write();
	}
	if (what == 3)
	{
		float nv[3], d[3];
		for (i = 0; i < 3; i++) { d[i] = f[i] * (float)o_launch_fwd + up[i] * (float)o_launch_up; nv[i] = MF(target + 0x88 + 4 * (u32)i) + d[i]; }
		if (target == obj) sb_launch_delta(d);                                /* 6h.17: the bounce test counts it in */
		for (i = 0; i < n; i++) set_body_velocity(M32(ents[i] + 0x3C), nv);
		for (i = 0; i < 3; i++) gwf(target + 0x88 + 4 * (u32)i, nv[i]);
		{ s32 at = 0; const char *p = "Launched: "; while (*p) msg[at++] = *p++; at = put_i(msg, at, o_launch_fwd); p = " u/s forward, "; while (*p) msg[at++] = *p++; at = put_i(msg, at, o_launch_up); p = " up"; while (*p) msg[at++] = *p++; msg[at] = 0; }
	}
	else
	{
		float d[3];
		if (what == 7) for (i = 0; i < 3; i++) d[i] = g_fc_p[i] - MF(target + 0x64 + 4 * (u32)i);   /* to the free camera (HCM's) */
		else for (i = 0; i < 3; i++) d[i] = f[i] * (float)o_tp_fwd + up[i] * (float)o_tp_up;
		shift3(target + 0x64, d);
		/* the bodies by the same amount: both swept-transform centres (so the next step doesn't sweep back), and the
		 * transform's translation in the motion state the collidable points at (Cartographer's teleport) */
		for (i = 0; i < n; i++)
		{
			u32 m = M32(ents[i] + 0x3C), h = handle_in_entity(ents[i]);
			shift3(m + 0x60, d); shift3(m + 0x70, d);
			if (h)
			{
				u32 ms = M32(h - 0x10 + 8), tr = ms + 0x50;
				if (ptr_ok(ms) && tr != m + 0x60 && tr != m + 0x70) shift3(tr, d);
			}
		}
		if (what == 7) { s32 at = 0; const char *p = "Teleported to the camera"; while (*p) msg[at++] = *p++; msg[at] = 0; }
		else { s32 at = 0; const char *p = "Teleported "; while (*p) msg[at++] = *p++; at = put_i(msg, at, o_tp_fwd); p = " forward, "; while (*p) msg[at++] = *p++; at = put_i(msg, at, o_tp_up); p = " up"; while (*p) msg[at++] = *p++; msg[at] = 0; }
	}
	if (!n) { s32 at = 0; while (msg[at]) at++; { const char *p = what == 3 ? " (the object only: no body in the physics world)" : " (position only: no body in the physics world)"; while (*p && at < 79) msg[at++] = *p++; msg[at] = 0; } }
	toast(msg);
}

/* is this rigid body one of an object's (a Havok component's)? The environment - the level - is in none */
static s32 component_body(u32 body)
{
	u32 pool = M32(ADDR_HAVOK_COMPONENTS_PTR), data, count, size, i;
	if (!ptr_ok(pool)) return 0;
	data = M32(pool + 0x44); count = M32(pool + 0x20); size = M32(pool + 0x24);
	if (!ptr_ok(data) || count > 4096 || size < 0x78 || size > 0x400) return 0;
	for (i = 0; i < count; i++)
	{
		u32 c = data + i * size, b = M32(c + 0x70), n = M32(c + 0x74), k;
		if (!ptr_ok(b) || n == 0 || n > 64) continue;
		for (k = 0; k < n; k++) if (M32(b + k * 0x60 + 0x40) == body) return 1;
	}
	return 0;
}

/* Cartographer's set_charge: in your bodies' islands, the contact pair between you and the environment with a real
 * agent gets -3 (the charged level). The environment: the partner whose owner body (handle + 0x10) belongs to no
 * object's Havok component - Cartographer checks the environment body / its MOPP shape by PC addresses. */
static s32 set_charge(u32 object)
{
	u32 component = pool_entry(ADDR_HAVOK_COMPONENTS_PTR, (s32)M32(object + 0xB4), 0x78), bodies, world;
	s32 n, i, k;
	if (!component) return 0;
	bodies = M32(component + 0x70); n = (s32)M32(component + 0x74); world = M32(ADDR_HAVOK_WORLD_PTR);
	if (!ptr_ok(bodies) || n <= 0 || n > 64 || !ptr_ok(world)) return 0;
	for (i = 0; i < n; i++)
	{
		u32 entity = M32(bodies + (u32)i * 0x60 + 0x40), island, handle, pairs;
		s32 pn;
		if (!ptr_ok(entity) || M32(entity + 0x08) != world) continue;
		island = M32(entity + 0x44);
		if (!ptr_ok(island)) continue;
		handle = body_handle(entity); pairs = M32(island + 0x68); pn = (s32)M32(island + 0x6C);
		if (!handle || !ptr_ok(pairs) || pn <= 0 || pn > 8192) continue;
		for (k = 0; k < pn; k++)
		{
			u32 p = pairs + (u32)k * 16, a = M32(p), b = M32(p + 4), agent = M32(p + 8), other, owner;
			if (!ptr_ok(agent) || (agent >= ADDR_DATA_LO && agent < ADDR_DATA_HI)) continue;   /* a real agent */
			if (a == handle) other = b; else if (b == handle) other = a; else continue;
			if (!ptr_ok(other)) continue;
			owner = M32(other + 0x10);
			if (!ptr_ok(owner) || component_body(owner)) continue;                           /* the environment */
			gw32(p + 12, (u32)-3);
			return 1;
		}
	}
	return 0;
}

static char g_map[48];
/* ---- ghost seams (Cartographer): level / piece seams whose pure-lottery landings (deep only with a late contact) have
 * launched. Counted per seam - known by its ends, within 0.02 - kept per map in spooky_ghosts.txt on the log's drive
 * (lines "map ax ay az bx by bz tries launched", in thousandths). 2+ launches at 30%+ make it amber. ---- */
#define GHOST_MAX 96
static char g_gh_map[GHOST_MAX][48];
static s32 g_gh_pt[GHOST_MAX][6], g_gh_tries[GHOST_MAX], g_gh_hits[GHOST_MAX], g_gh_n, g_gh_loaded;
#define GFILE_CAP (GHOST_MAX * 112)
#ifdef HOST
static char g_gfile_host[GFILE_CAP];
#endif
static char *g_gfile;                           /* the ghost seam file's text: run-time memory while it's read (6f: was 10.7 KB fixed) */
static s32 str_eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

static s32 ghost_find(const char *map, const float *a, const float *b)
{
	s32 i, k, pa[3], pb[3];
	for (k = 0; k < 3; k++) { pa[k] = (s32)(a[k] * 1000.f + (a[k] < 0.f ? -0.5f : 0.5f)); pb[k] = (s32)(b[k] * 1000.f + (b[k] < 0.f ? -0.5f : 0.5f)); }
	for (i = 0; i < g_gh_n; i++)
	{
		s32 same = 1, swap = 1;
		if (!str_eq(g_gh_map[i], map)) continue;
		for (k = 0; k < 3; k++)
		{
			s32 d0 = g_gh_pt[i][k] - pa[k], d1 = g_gh_pt[i][3 + k] - pb[k], e0 = g_gh_pt[i][k] - pb[k], e1 = g_gh_pt[i][3 + k] - pa[k];
			if (d0 > 20 || d0 < -20 || d1 > 20 || d1 < -20) same = 0;
			if (e0 > 20 || e0 < -20 || e1 > 20 || e1 < -20) swap = 0;
		}
		if (same || swap) return i;
	}
	return -1;
}

static void ghosts_parse(u32 n);
static void ghosts_load(void)
{
	u32 n = 0;
	g_gh_loaded = 1; g_gh_n = 0;
#ifdef HOST
	g_gfile = g_gfile_host;
#else
	g_gfile = (char *)VirtualAlloc_x(0, GFILE_CAP, 0x3000, 4);
	if (!g_gfile) return;                                                 /* no memory: no ghost seams this time */
	{
		static const char *const paths[3] = { "T:\\spooky_ghosts.txt", "U:\\spooky_ghosts.txt", "Z:\\spooky_ghosts.txt" };
		s32 d;
		for (d = 0; d < 3 && !n; d++)
		{
			void *h = CreateFileA_x(paths[d], 0x80000000, 1, 0, 3, 0x80, 0);
			if (h == (void *)-1) continue;
			if (!ReadFile_x(h, g_gfile, GFILE_CAP - 1, &n, 0)) n = 0;
			CloseHandle_x(h);
		}
	}
#endif
	g_gfile[n] = 0;
	ghosts_parse(n);
#ifndef HOST
	VirtualFree_x(g_gfile, 0, 0x8000);
#endif
	g_gfile = 0;
}
static void ghosts_parse(u32 n)
{
	u32 i = 0;
	while (i < n && g_gh_n < GHOST_MAX)
	{
		u32 a = i, len, f;
		s32 v[8], ok = 1;
		while (i < n && g_gfile[i] != ' ' && g_gfile[i] != '\r' && g_gfile[i] != '\n') i++;
		len = i - a; if (len > 47) len = 47;
		for (f = 0; f < 8; f++)
		{
			s32 neg = 0, x = 0, digits = 0;
			while (i < n && g_gfile[i] == ' ') i++;
			if (i < n && g_gfile[i] == '-') { neg = 1; i++; }
			while (i < n && g_gfile[i] >= '0' && g_gfile[i] <= '9') { x = x * 10 + (g_gfile[i++] - '0'); digits++; }
			if (!digits) ok = 0;
			v[f] = neg ? -x : x;
		}
		while (i < n && g_gfile[i] != '\n') i++;
		if (i < n) i++;
		if (!ok || !len) continue;
		memcpy(g_gh_map[g_gh_n], g_gfile + a, len); g_gh_map[g_gh_n][len] = 0;
		for (f = 0; f < 6; f++) g_gh_pt[g_gh_n][f] = v[f];
		g_gh_tries[g_gh_n] = v[6]; g_gh_hits[g_gh_n] = v[7]; g_gh_n++;
	}
}

/* the ghost seam file's text, built into buf (cap bytes): whole lines only, never past cap - its length */
static u32 ghosts_text(char *buf, u32 cap)
{
	u32 n = 0;
	s32 r, f;
	for (r = 0; r < g_gh_n && n + 160 < cap; r++)                         /* a line: a map name (<= 47) and 8 numbers (<= 12 each) */
	{
		const char *m = g_gh_map[r];
		s32 k0 = 0;
		while (*m && k0++ < 47) buf[n++] = *m++;
		for (f = 0; f < 8; f++)
		{
			s32 v = f < 6 ? g_gh_pt[r][f] : f == 6 ? g_gh_tries[r] : g_gh_hits[r], k = 0;
			u32 uv;
			char b[12];
			buf[n++] = ' ';
			if (v < 0) { buf[n++] = '-'; uv = (u32)(-(v + 1)) + 1u; } else uv = (u32)v;
			do { b[k++] = (char)('0' + uv % 10); uv /= 10; } while (uv && k < 11);
			while (k) buf[n++] = b[--k];
		}
		buf[n++] = '\r'; buf[n++] = '\n';
	}
	return n;
}
#ifdef HOST
u32 host_ghosts_text(char *buf, u32 cap) { return ghosts_text(buf, cap); }
void host_ghost_add(const char *map, s32 tries)
{
	s32 i = g_gh_n++, k;
	for (k = 0; k < 47 && map[k]; k++) g_gh_map[i][k] = map[k];
	g_gh_map[i][k] = 0;
	for (k = 0; k < 6; k++) g_gh_pt[i][k] = -2147483647 - 1 + k;
	g_gh_tries[i] = tries; g_gh_hits[i] = tries;
}
void host_ghosts_reset(void) { g_gh_n = 0; g_gh_loaded = 1; }
#endif
/* 6h.19: the file saved from its own memory, taken and freed here. g_gfile is only there while the file is read (6f made
 * it run-time memory) - and this wrote through it after (a null pointer): the crash on the first ghost seam hit, right
 * after a bounce (6h.17, 6h.18). */
static void ghosts_save(void)
{
#ifndef HOST
	u32 n, w;
	s32 d;
	char *buf;
	static const char *const paths[3] = { "T:\\spooky_ghosts.txt", "U:\\spooky_ghosts.txt", "Z:\\spooky_ghosts.txt" };
	buf = (char *)VirtualAlloc_x(0, GFILE_CAP, 0x3000, 4);
	if (!buf) return;                                                     /* no memory: not saved this time */
	n = ghosts_text(buf, GFILE_CAP);
	for (d = g_log_drive < 0 ? 0 : g_log_drive; d < 3; d++)
	{
		void *h = CreateFileA_x(paths[d], 0x40000000, 0, 0, 2, 0x80, 0);
		if (h == (void *)-1) continue;
		WriteFile_x(h, buf, n, &w, 0);
		CloseHandle_x(h);
		break;
	}
	VirtualFree_x(buf, 0, 0x8000);
#endif
}

/* sb_ghost_get: is this a ghost seam (2+ launches at 30%+)? */
static s32 ghost_get(const float *a, const float *b, s32 *tries, s32 *hits)
{
	s32 i;
	if (!g_gh_loaded) ghosts_load();
	i = ghost_find(g_map, a, b);
	if (i < 0) return 0;
	*tries = g_gh_tries[i]; *hits = g_gh_hits[i];
	return g_gh_hits[i] >= 2 && g_gh_hits[i] * 10 >= g_gh_tries[i] * 3;
}

/* sb_ghost_impact: a pure-lottery landing on this seam - did it launch? */
static void ghost_impact(const float *a, const float *b, s32 launched)
{
	s32 i, k;
	if (!g_gh_loaded) ghosts_load();
	i = ghost_find(g_map, a, b);
	if (i < 0)
	{
		const char *m = g_map;
		if (g_gh_n >= GHOST_MAX) return;
		i = g_gh_n++;
		for (k = 0; k < 47 && *m; k++) g_gh_map[i][k] = *m++;
		g_gh_map[i][k] = 0;
		for (k = 0; k < 3; k++) { g_gh_pt[i][k] = (s32)(a[k] * 1000.f + (a[k] < 0.f ? -0.5f : 0.5f)); g_gh_pt[i][3 + k] = (s32)(b[k] * 1000.f + (b[k] < 0.f ? -0.5f : 0.5f)); }
		g_gh_tries[i] = g_gh_hits[i] = 0;
	}
	g_gh_tries[i]++;
	if (launched) g_gh_hits[i]++;
	ghosts_save();
}
#ifdef HOST
s32 host_ghost(const float *a, const float *b, s32 add, s32 launched) { s32 t = 0, h = 0; if (add) ghost_impact(a, b, launched); return ghost_get(a, b, &t, &h); }
#endif

/* the verdict on the last airborne frame, when it's a pure lottery on a level / piece seam (for ghost seams) */
static s32 g_la_ok;
static float g_la_a[3], g_la_b[3];

/* the level's record, kept per map in spooky_records.txt (lines "map record"), on the log's drive */
#define RECORD_SLOTS 48
static char g_rec_map[RECORD_SLOTS][48];
static s32 g_rec_value[RECORD_SLOTS], g_rec_count, g_rec_loaded;
static char g_map[48];
static s32 g_sb_charged = -1, g_sb_streak, g_sb_record, g_sb_ground;
static float g_sb_last_bounce;                  /* 6h.16: the last superbounce's speed (u/s) */
static s32 g_sb_have_last;
static float g_sb_speed = -1.f, g_sb_prev_vz, g_sb_prev_vx, g_sb_prev_vy;
static u32 g_sb_prev_tick = 0xffffffffu;
/* 6h.17 / 6h.19: a Launch of yours: the bounce test asks for a jump beyond the launch's push AND beyond your speed without
 * it - a seam hit on the very tick after a launch (a monster / dreadnought launched into) still counts, at its speed, while
 * a launch alone never does, whether it took (6h.17's test) or not (6h.18 took one the game hadn't applied for a bounce).
 * (6h.16 skipped the 3 ticks after a launch - and lost those.) */
static float g_sb_ld[3];                          /* 6h.19: the push of a Launch since the last tick (kept apart from it) */
static s32 g_sb_peak_ticks;                       /* 6h.19: ticks left to take the bounce's peak speed */
static s32 g_sb_peak_n;                           /* 6h.20: which bounce in a row it is (for the log) */
static void sb_peak_log(void)                     /* 6h.20: the peak in the log, as Last Bounce shows it */
{
	s32 at, t;
	if (!o_diag) return;                                                  /* 6h.21: not on the consoles */
	at = lput(0, "superbounce: bounce "); t = (s32)(g_sb_last_bounce * 10.f + 0.5f);
	at = lnum(at, (u32)g_sb_peak_n); at = lput(at, " peak speed x10 "); lnum(at, (u32)t);
	log_write();
}
static void sb_launch_delta(const float *d)
{
	if (g_sb_prev_tick == 0xffffffffu) return;
	g_sb_ld[0] += d[0]; g_sb_ld[1] += d[1]; g_sb_ld[2] += d[2];
}
static s32 g_sb_airborne, g_sb_was_air;
static u32 g_sb_tick;                           /* this tick (the shown seams' ages) */
static float g_tgt[2], g_cand[2];               /* air control's target (horizontal velocity) and its candidate */
static s32 g_cand_ticks;
static float g_sb_jump = 3.08f;                 /* your measured jump take-off speed (Cartographer: 3.08 from the biped tag) */
static u32 g_sb_object;                  /* the player's object this frame (0 = none), for the log */
static s32 g_sb_logged_player;
static char g_file[2048];

static void records_load(void)
{
	u32 n = 0, i = 0;
	g_rec_loaded = 1; g_rec_count = 0;
#ifdef HOST
	n = host_read_file("spooky_records.txt", g_file, sizeof(g_file) - 1);
#else
	static const char *const paths[3] = { "T:\\spooky_records.txt", "U:\\spooky_records.txt", "Z:\\spooky_records.txt" };
	s32 d;
	for (d = 0; d < 3 && !n; d++)
	{
		void *h = CreateFileA_x(paths[d], 0x80000000 /* GENERIC_READ */, 1, 0, 3 /* OPEN_EXISTING */, 0x80, 0);
		if (h == (void *)-1) continue;
		if (!ReadFile_x(h, g_file, sizeof(g_file) - 1, &n, 0)) n = 0;
		CloseHandle_x(h);
	}
#endif
	g_file[n] = 0;
	while (i < n && g_rec_count < RECORD_SLOTS)
	{
		u32 a = i, len = 0; s32 v = 0;
		while (i < n && g_file[i] != ' ' && g_file[i] != '\r' && g_file[i] != '\n') i++;
		len = i - a; if (len > 47) len = 47;
		while (i < n && g_file[i] == ' ') i++;
		while (i < n && g_file[i] >= '0' && g_file[i] <= '9') v = v * 10 + (g_file[i++] - '0');
		while (i < n && g_file[i] != '\n') i++;
		if (i < n) i++;
		if (!len) continue;
		memcpy(g_rec_map[g_rec_count], g_file + a, len); g_rec_map[g_rec_count][len] = 0;
		g_rec_value[g_rec_count++] = v;
	}
}

static void records_save(void)
{
	u32 n = 0; s32 r;
	for (r = 0; r < g_rec_count && n < sizeof(g_file) - 64; r++)
	{
		const char *m = g_rec_map[r]; s32 v = g_rec_value[r]; char b[12]; s32 k = 0;
		while (*m) g_file[n++] = *m++;
		g_file[n++] = ' ';
		do { b[k++] = (char)('0' + v % 10); v /= 10; } while (v);
		while (k) g_file[n++] = b[--k];
		g_file[n++] = '\r'; g_file[n++] = '\n';
	}
#ifdef HOST
	host_write_file("spooky_records.txt", g_file, n);
#else
	{
		static const char *const paths[3] = { "T:\\spooky_records.txt", "U:\\spooky_records.txt", "Z:\\spooky_records.txt" };
		s32 d; u32 w;
		for (d = g_log_drive < 0 ? 0 : g_log_drive; d < 3; d++)
		{
			void *h = CreateFileA_x(paths[d], 0x40000000, 0, 0, 2 /* CREATE_ALWAYS */, 0x80, 0);
			if (h == (void *)-1) continue;
			WriteFile_x(h, g_file, n, &w, 0);
			CloseHandle_x(h);
			break;
		}
	}
#endif
}

static s32 record_slot(const char *map)
{
	s32 r, i;
	for (r = 0; r < g_rec_count; r++)
	{
		for (i = 0; map[i] && map[i] == g_rec_map[r][i]; i++) {}
		if (!map[i] && !g_rec_map[r][i]) return r;
	}
	if (g_rec_count >= RECORD_SLOTS) return -1;
	for (i = 0; map[i] && i < 47; i++) g_rec_map[g_rec_count][i] = map[i];
	g_rec_map[g_rec_count][i] = 0;
	g_rec_value[g_rec_count] = 0;
	return g_rec_count++;
}

/* the level's name: the last part of the map path ("scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa" -> "03a_oldmombasa") */
static void current_map(char *out)
{
	s32 i, start = 0, n = 0;
	char c;
	for (i = 0; i < 0x100 && (c = (char)M8(ADDR_MAP_NAME + (u32)i)); i++) if (c == '\\' || c == '/') start = i + 1;
	for (i = start; i < 0x100 && n < 47 && (c = (char)M8(ADDR_MAP_NAME + (u32)i)); i++) out[n++] = (c > ' ' && c < 127) ? c : '_';
	out[n] = 0;
}

/* once per main-loop frame: work through any new game ticks */
static void superbounce_update(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), object;
	if (!ptr_ok(time)) { g_sb_charged = -1; return; }
	u32 tick = M32(time + 8);
	g_sb_tick = tick;
	object = player_object();
	g_sb_object = object;
	if (!object) { g_sb_charged = -1; g_sb_speed = -1.f; g_sb_prev_tick = 0xffffffffu; return; }
	if (!g_sb_logged_player)
	{
		g_sb_logged_player = 1;
		s32 at = lput(0, "player found: object "); at = lhex(at, object);
		at = lput(at, ", Havok component "); at = lhex(at, M32(object + 0xB4));
		at = lput(at, ", game tick "); lnum(at, tick);
		log_write();
	}

	char map[48];
	current_map(map);
	{
		s32 i, same = 1;
		for (i = 0; same && (map[i] || g_map[i]); i++) same = map[i] == g_map[i];
		if (!same || !g_rec_loaded)
		{
			if (!g_rec_loaded) records_load();
			memcpy(g_map, map, sizeof(g_map));
			s32 r = map[0] ? record_slot(map) : -1;
			g_sb_record = r >= 0 ? g_rec_value[r] : 0;
			g_sb_streak = 0; g_sb_have_last = 0;
		}
	}

	float vx = MF(object + 0x88), vy = MF(object + 0x8C), vz = MF(object + 0x90);      /* the object's velocity */
	s32 airborne = M8(object + 0x348) & 1;
	g_sb_airborne = airborne;
	s32 charged;
	{
		/* under the crash guard (6b.4 ran it bare): it walks your island's pairs, which Havok rewrites as bodies come and
		 * go at speed - the Ghost Viewer's guard caught a fault there, then something unguarded crashed. A fault: last
		 * tick's state, logged once. */
		static s32 said;
		g_uc_obj = object; g_uc_res = 0;
		trace("charge test");
		if (guarded_call(unit_charged_guarded))
		{
			charged = g_sb_charged >= 0 ? g_sb_charged : 0;
			if (!said) { said = 1; lput(0, "charge test: a bad read was caught (guarded) - last tick's state kept"); log_write(); }
		}
		else charged = g_uc_res;
	}
	g_sb_charged = charged;
	g_sb_speed = fsqrt(vx * vx + vy * vy + vz * vz);
	if (tick == g_sb_prev_tick) return;                                    /* paused, or no new tick yet */
	if (g_sb_prev_tick != 0xffffffffu)
	{
		if (tick < g_sb_prev_tick) g_sb_streak = 0;                       /* a revert */
		else
		{
			float dx = vx - g_sb_prev_vx, dy = vy - g_sb_prev_vy;
			float lx = dx - g_sb_ld[0], ly = dy - g_sb_ld[1], lz_prev = g_sb_prev_vz + g_sb_ld[2];
			float sp = fsqrt(vx * vx + vy * vy + vz * vz);
			s32 side = dx * dx + dy * dy > 16.f && lx * lx + ly * ly > 16.f;   /* sideways: beyond both */
			/* 6h.19: under 5 u/s is never a bounce */
			if (charged && g_sb_prev_vz < -2.f && lz_prev < -2.f && (vz > 4.f || side) && sp >= 5.f)
			{
				if (g_sb_peak_ticks > 0) sb_peak_log();                         /* (the one before: its peak so far) */
				g_sb_last_bounce = sp; g_sb_have_last = 1; g_sb_peak_ticks = 5;
				g_sb_streak++;
				g_sb_peak_n = g_sb_streak;
				if (o_diag)                                                     /* 6h.21: the tests only */
				{
					/* 6h.18: each bounce in the log - a crash right after one shows it last */
					s32 at = lput(0, "superbounce: bounce "), t = (s32)(g_sb_last_bounce * 10.f + 0.5f);
					at = lnum(at, (u32)g_sb_streak); at = lput(at, " in a row, first tick speed x10 "); lnum(at, (u32)t);
					log_write();
				}
				if (g_sb_streak > g_sb_record)
				{
					s32 r = g_map[0] ? record_slot(g_map) : -1;
					g_sb_record = g_sb_streak;
					if (r >= 0) { g_rec_value[r] = g_sb_record; records_save(); }
				}
			}
			else if (g_sb_peak_ticks > 0)                                      /* 6h.19: the bounce's peak speed, 5 ticks */
			{
				float sp2 = fsqrt(vx * vx + vy * vy + vz * vz);
				if (sp2 > g_sb_last_bounce) g_sb_last_bounce = sp2;
				if (--g_sb_peak_ticks == 0) sb_peak_log();
			}
			if (airborne) g_sb_ground = 0;
			else { g_sb_ground += (s32)(tick - g_sb_prev_tick); if (g_sb_ground >= 3) g_sb_streak = 0; }
		}
	}
	/* air control's target, from how the drift just changed (Cartographer): unchanged = at the target; a step toward a
	 * stop with no stick = let go (zero); otherwise steering toward a 2.25 u/s target along the change. A new target is
	 * taken only when two ticks in a row agree on it (within 0.3 u/s). */
	if (g_sb_prev_tick != 0xffffffffu)
	{
		float cx = vx - g_sb_prev_vx, cy = vy - g_sb_prev_vy, step = 1.0485f / 30.f, moved = fsqrt(cx * cx + cy * cy);
		float vn = fsqrt(vx * vx + vy * vy), tx = vx, ty = vy, gx, gy;
		s32 stick = fsqrt(g_stick[0] * g_stick[0] + g_stick[1] * g_stick[1]) > 0.25f;
		if (moved > 0.15f * step && moved < 1.5f * step)
		{
			float dx = cx / moved, dy = cy / moved;
			s32 toward_stop = vn > 1e-4f && (vx * dx + vy * dy) / vn < -0.995f;
			if (toward_stop && !stick) tx = ty = 0.f;
			else
			{
				float along = vx * dx + vy * dy, disc = along * along - (vx * vx + vy * vy - 2.25f * 2.25f);
				if (disc >= 0.f) { float k = -along + fsqrt(disc); tx = vx + dx * k; ty = vy + dy * k; }
			}
		}
		else if (!stick && vn < 0.05f) tx = ty = 0.f;
		gx = tx - g_cand[0]; gy = ty - g_cand[1];
		if (fsqrt(gx * gx + gy * gy) < 0.3f) g_cand_ticks++;
		else { g_cand[0] = tx; g_cand[1] = ty; g_cand_ticks = 1; }
		if (g_cand_ticks >= 2) { g_tgt[0] = tx; g_tgt[1] = ty; }
	}
	/* Cartographer's charge toggles: force it; or keep it through saves and reverts. A checkpoint save is requested by
	 * main_globals.save_map, which the game's update acts on right after this hook - so seeing it set, the save happens
	 * now, with the charge you have now (Cartographer's save stamp). The last two saves are kept (its two slots). A
	 * revert (time going back) restores the charge if you had it then, or at the save it went back to; a save while
	 * charged re-applies it too (the save rebuilds the pairs). Retried for a second while the pairs come back. */
	{
		static u32 kc_tick, kc_s_tick[2];
		static s32 kc_charged, kc_pending, kc_s_ch[2], kc_s_ok[2], kc_save_prev;
		s32 save = M8(ADDR_SAVE_MAP) != 0;
		if (save && !kc_save_prev)                                           /* a checkpoint is being saved */
		{
			kc_s_tick[1] = kc_s_tick[0]; kc_s_ch[1] = kc_s_ch[0]; kc_s_ok[1] = kc_s_ok[0];
			kc_s_tick[0] = tick; kc_s_ch[0] = charged; kc_s_ok[0] = 1;
			if (o_keep_charge && charged) kc_pending = 30;
		}
		kc_save_prev = save;
		if (o_force_charge && !charged) set_charge(object);
		if (o_keep_charge)
		{
			if (kc_tick && tick + 2 < kc_tick)                                 /* reverted: to which save? */
			{
				s32 at_save = 0;
				if (kc_s_ok[0] && tick + 2 >= kc_s_tick[0] && tick <= kc_s_tick[0] + 3) at_save = kc_s_ch[0];
				else if (kc_s_ok[1] && tick + 2 >= kc_s_tick[1] && tick <= kc_s_tick[1] + 3) at_save = kc_s_ch[1];
				if (kc_charged || at_save) kc_pending = 30;
			}
			if (kc_pending > 0) { if (charged) kc_pending = 0; else { set_charge(object); kc_pending--; } }
			else kc_charged = charged;
		}
		else { kc_pending = 0; kc_charged = charged; }
		kc_tick = tick;
	}
	/* a charged landing (falling, then stopped or launched): a pure-lottery seam landing counts for ghost seams */
	if (charged && g_sb_prev_tick != 0xffffffffu && tick == g_sb_prev_tick + 1 && g_sb_prev_vz < -1.5f && vz - g_sb_prev_vz > 1.5f)
	{
		if (g_la_ok) ghost_impact(g_la_a, g_la_b, vz >= 6.f);
		g_la_ok = 0;
	}
	/* a jump from standing: the first airborne tick's upward speed (as Cartographer measures it) */
	if (airborne && !g_sb_was_air && vz > 2.f && vz < 4.5f) g_sb_jump = vz;
	g_sb_was_air = airborne;
	g_sb_prev_tick = tick; g_sb_prev_vz = vz; g_sb_prev_vx = vx; g_sb_prev_vy = vy;
	g_sb_ld[0] = g_sb_ld[1] = g_sb_ld[2] = 0.f;                            /* (a launch's push: for the tick after it only) */
}

/* ---- flat-colour drawing (step 3a): the game's own full-screen overlay recipe (function_40890), replayed ----
 * render states through the game's cached setter; vertex shader 13 with constants c177-c181 mapping viewport pixels
 * to the screen; a one-stage pixel shader whose output is a constant colour; then quads of pixel coordinates. */
#ifndef HOST
void gfx_render_state(u32 state, u32 value);
void gfx_vs_select(u32 index);
void gfx_vs_constants(u32 first, const void *data, u32 dwords);
void gfx_ps_set(const void *program);
u32 gfx_call0(u32 fn);                /* returns EAX */
void gfx_call1(u32 fn, u32 a);
void gfx_call3(u32 fn, u32 a, u32 b, u32 c);
#endif
/* the world camera of the view being drawn, snapshot from the view record route A hands the HUD pass (the record
 * the world was just rendered from: function_2b790). By the time HUD text is drawn, the HUD frame setup
 * (function_132d0) has replaced both the frame storage camera and g_camera_storage with the HUD's own camera -
 * an identity camera at the world origin. The record's camera block at +0x0C has the s_2f800_view layout:
 * position +0x00, forward +0x0C, up +0x18, field of view +0x28 (vertical, radians), viewport +0x30..+0x36. */
static u8 g_view_cam[0x40];
static s32 g_view_ok;
static float vcf(s32 off) { float f; memcpy(&f, g_view_cam + off, 4); return f; }    /* the snapshot's fields */
static u32 vcu(s32 off) { u32 v; memcpy(&v, g_view_cam + off, 4); return v; }
/* the frame storage at the same moment - after the world was drawn, before the HUD pass installs its own frame:
 * the frame setup (function_132d0) copies the whole frame record there, so it may still hold the world view's
 * camera (+0x18) and its projection block (+0x8c: function_2fd90's output - inverse transform, camera transform,
 * bounds left/right/bottom/top at unit depth, matrix, half sizes). Used only if it is provably the player's view. */
static u8 g_fs_snap[0xF0];
static s32 g_fs_ok;
static float fsf(s32 off) { float f; memcpy(&f, g_fs_snap + off, 4); return f; }
#define FSN_CAM 0x00                   /* snapshot offsets: frame storage +0x18 -> 0x00, +0x8c -> 0x30 */
#define FSN_PROJ 0x30
void spooky_view(u32 view)
{
	s32 i;
	for (i = 0; i < 0x30; i += 4) *(u32 *)(g_fs_snap + FSN_CAM + i) = M32(ADDR_FRAME_STORAGE + 0x18 + i);
	for (i = 0; i < 0xC0; i += 4) *(u32 *)(g_fs_snap + FSN_PROJ + i) = M32(ADDR_FRAME_STORAGE + 0x8C + i);
	g_fs_ok = 1;
#ifndef HOST
	if (!ptr_ok(view) || !ptr_ok(view + 0x4C)) { g_view_ok = 0; return; }
#endif
	for (i = 0; i < 0x40; i += 4) *(u32 *)(g_view_cam + i) = M32(view + 0xC + i);
	g_view_ok = 1;
}
static s32 g_gfx_open, g_gfx_quads;
static s32 g_vp_left, g_vp_top, g_vp_w = 640, g_vp_h = 480;
static float g_cam_p[3], g_cam_f[3], g_cam_u[3], g_cam_r[3], g_tan_v = 0.5f, g_tan_h = 0.66f, g_cam_fov;
/* the world frame's projection, when accepted (camera_read): view axes and unit-depth bounds */
static s32 g_use_frame;
static float g_fr_p[3], g_fr_r[3], g_fr_u[3], g_fr_b[3], g_fr_l, g_fr_rt, g_fr_bt, g_fr_tp, g_fr_dist;
static s32 g_gfx_logged;
static u32 g_ws, g_ws_flag5;           /* the game's widescreen test and its g_510c50[5] flag, this frame */
#ifdef HOST
static u32 g_host_ws, g_host_ws_flag5;
void host_set_widescreen(u32 ws, u32 flag5) { g_host_ws = ws; g_host_ws_flag5 = flag5; }
#endif

/* the projection's aspect, exactly as the game's function_2fb70 decides it:
 *   widescreen = function_016a90()  (the widescreen setting g_485ac2, except on one options screen)
 *   widescreen: aspect x 4/3 (16:9 drawn into the 4:3 buffer); if g_510c50[5] also: tangent x 0.5625 x 4/3
 * The flag is read only through a validated pointer; anything implausible leaves the marker undrawn. */
static void aspect_query(void)
{
#ifdef HOST
	g_ws = g_host_ws; g_ws_flag5 = g_host_ws_flag5;
#else
	u32 p = M32(ADDR_ASPECT_FLAGS_PTR);
	g_ws = (gfx_call0(ADDR_WIDESCREEN_FN) & 0xff) ? 1 : 0;
	g_ws_flag5 = (p && ptr_ok(p)) ? (((M32(p + 4) >> 8) & 0xff) ? 1 : 0) : 0;
#endif
}

static float ftan(float x)
{
#ifdef HOST
	/* tan for |x| < 1.6: sin/cos series is plenty for the test */
	float x2 = x * x, sn = x * (1 - x2 / 6 * (1 - x2 / 20 * (1 - x2 / 42 * (1 - x2 / 72)))), cs = 1 - x2 / 2 * (1 - x2 / 12 * (1 - x2 / 30 * (1 - x2 / 56)));
	return sn / cs;
#else
	float r; __asm__("fptan\n\tfstp %%st(0)" : "=t"(r) : "0"(x)); return r;
#endif
}

static float dot3(const float *a, const float *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

/* the camera of the view being drawn (read once per HUD pass) */
static s32 camera_read(void)
{
	s32 i;
	/* the world view's camera: local player 0's entry of g_4e9c8c (0x358 bytes per player), which the renderer
	 * passes straight to function_2f800 as the view source (function_223080): position +0x00, forward +0x20,
	 * up +0x2C, field of view +0x4C - and function_2f800 multiplies that angle by 0.785 for a single full-screen
	 * view (0.5 / 0.8 only for split-screen layouts). The camera Yelo's free camera writes (+0x54 / +0x7C /
	 * +0x88) is a second one in the same entry - the game's input to this one, not what the frame is drawn with.
	 * (The view record route A hands the HUD pass, and the camera storage, hold only the HUD's camera; the
	 * viewport still comes from there.) */
	view_camera(g_cam_p, g_cam_f, g_cam_u);                               /* third person: the moved camera */
	{
		float lf = dot3(g_cam_f, g_cam_f), lu = dot3(g_cam_u, g_cam_u);
		if (!(lf > 0.9f && lf < 1.1f && lu > 0.9f && lu < 1.1f))
			for (i = 0; i < 3; i++) { g_cam_p[i] = vcf(0x00 + 4 * i); g_cam_f[i] = vcf(0x0C + 4 * i); g_cam_u[i] = vcf(0x18 + 4 * i); }
	}
	g_vp_top = (s16)(vcu(0x30) & 0xffff); g_vp_left = (s16)(vcu(0x30) >> 16);
	g_vp_h = (s16)(vcu(0x34) & 0xffff) - g_vp_top; g_vp_w = (s16)(vcu(0x34) >> 16) - g_vp_left;
	float fov = vcf(0x28);
	if (!g_view_ok) return 0;
	{
		float angle = MF(ADDR_PLAYER_CAMERAS + 0x4C) * 0.785f;       /* function_2f800: source angle x 0.785 */
		if (angle > 0.1f && angle < 3.0f) fov = angle;
		g_cam_fov = fov;
	}
	if (g_vp_w <= 0 || g_vp_h <= 0 || !(fov > 0.1f && fov < 3.0f)) return 0;
	/* right = forward x up (Halo: x forward, z up -> -y = right) */
	g_cam_r[0] = g_cam_f[1] * g_cam_u[2] - g_cam_f[2] * g_cam_u[1];
	g_cam_r[1] = g_cam_f[2] * g_cam_u[0] - g_cam_f[0] * g_cam_u[2];
	g_cam_r[2] = g_cam_f[0] * g_cam_u[1] - g_cam_f[1] * g_cam_u[0];
	{
		float aspect = (float)g_vp_w / (float)g_vp_h;
		g_tan_v = ftan(fov * 0.5f);                                   /* function_2fb70: tan(angle / 2) */
		aspect_query();
		if (g_ws)
		{
			aspect *= 4.f / 3.f;
			if (g_ws_flag5) g_tan_v = 0.5625f * (g_tan_v * (4.f / 3.f));
		}
		g_tan_h = g_tan_v * aspect;
		if (!(g_tan_v > 0.05f && g_tan_v < 5.f && g_tan_h > 0.05f && g_tan_h < 9.f)) return 0;
	}
	/* the world frame's own projection, if the snapshot is provably the player's view: its camera within 0.25
	 * units of the player's, unit-length axes, and bounds within 25% of what this field of view and aspect
	 * predict (a weapon or effects view with its own projection fails this) */
	g_use_frame = 0;
	if (g_fs_ok)
	{
		float dx = fsf(FSN_PROJ + 0x34 + 0x28) - g_cam_p[0], dy = fsf(FSN_PROJ + 0x34 + 0x2C) - g_cam_p[1], dz = fsf(FSN_PROJ + 0x34 + 0x30) - g_cam_p[2];
		for (i = 0; i < 3; i++)
		{
			g_fr_r[i] = fsf(FSN_PROJ + 0x34 + 0x04 + 4 * i);    /* camera transform: forward = the view's right */
			g_fr_u[i] = fsf(FSN_PROJ + 0x34 + 0x10 + 4 * i);    /* left = the view's up */
			g_fr_b[i] = fsf(FSN_PROJ + 0x34 + 0x1C + 4 * i);    /* up = the view's backward */
			g_fr_p[i] = fsf(FSN_PROJ + 0x34 + 0x28 + 4 * i);    /* position */
		}
		g_fr_l = fsf(FSN_PROJ + 0x68); g_fr_rt = fsf(FSN_PROJ + 0x6C); g_fr_bt = fsf(FSN_PROJ + 0x70); g_fr_tp = fsf(FSN_PROJ + 0x74);
		g_fr_dist = fsqrt(dx * dx + dy * dy + dz * dz);
		{
			float lr = dot3(g_fr_r, g_fr_r), lu = dot3(g_fr_u, g_fr_u), lb = dot3(g_fr_b, g_fr_b);
			float th = g_tan_h * 0.25f, tv = g_tan_v * 0.25f;
			if (g_fr_dist < 0.25f && lr > 0.9f && lr < 1.1f && lu > 0.9f && lu < 1.1f && lb > 0.9f && lb < 1.1f &&
				g_fr_l < 0.f && g_fr_rt > 0.f && g_fr_bt < 0.f && g_fr_tp > 0.f &&
				g_fr_l + g_tan_h < th && g_fr_l + g_tan_h > -th && g_fr_rt - g_tan_h < th && g_fr_rt - g_tan_h > -th &&
				g_fr_bt + g_tan_v < tv && g_fr_bt + g_tan_v > -tv && g_fr_tp - g_tan_v < tv && g_fr_tp - g_tan_v > -tv)
				g_use_frame = 1;
		}
	}
	if (!g_gfx_logged)
	{
		g_gfx_logged = 1;
		s32 at = lput(0, "world camera: fov x1000 "); at = lnum(at, (u32)(fov * 1000.f));
		at = lput(at, ", viewport "); at = lnum(at, (u32)g_vp_left); at = lput(at, ","); at = lnum(at, (u32)g_vp_top);
		at = lput(at, " "); at = lnum(at, (u32)g_vp_w); at = lput(at, "x"); at = lnum(at, (u32)g_vp_h);
		at = lput(at, ", forward z x1000 "); at = lnum(at, (u32)(s32)(g_cam_f[2] * 1000.f + 1000.f));
		at = lput(at, ", widescreen "); at = lnum(at, g_ws); at = lput(at, " (flag "); at = lnum(at, g_ws_flag5); lput(at, ")");
		log_write();
	}
	return 1;
}


/* a world point to viewport pixels; depth = distance along the view (<= 0: behind) */
static void project(const float *w, float *sx, float *sy, float *depth)
{
	if (g_use_frame)
	{
		/* exactly the game's: view space (x right, y up, z backward), then the frustum bounds at unit depth */
		float d[3] = { w[0] - g_fr_p[0], w[1] - g_fr_p[1], w[2] - g_fr_p[2] };
		float z = -dot3(d, g_fr_b);
		*depth = z;
		if (z < 0.001f) z = 0.001f;
		*sx = (dot3(d, g_fr_r) / z - g_fr_l) / (g_fr_rt - g_fr_l) * (float)g_vp_w;
		*sy = (g_fr_tp - dot3(d, g_fr_u) / z) / (g_fr_tp - g_fr_bt) * (float)g_vp_h;
		return;
	}
	float d[3] = { w[0] - g_cam_p[0], w[1] - g_cam_p[1], w[2] - g_cam_p[2] };
	float z = dot3(d, g_cam_f);
	*depth = z;
	if (z < 0.001f) z = 0.001f;
	*sx = ((dot3(d, g_cam_r) / z) / g_tan_h + 1.f) * 0.5f * (float)g_vp_w;
	*sy = (1.f - (dot3(d, g_cam_u) / z) / g_tan_v) * 0.5f * (float)g_vp_h;
}

#ifndef HOST
/* one pixel shader definition per colour, each at its own address: the colour is its constant, and a translator that
 * caches a definition by its address (the Xbox 360's backward compatibility) keeps the first colour it saw at an address
 * - so an address must never change colour (5e.1 reused the game's one buffer for every colour: on the 360, nearly all
 * lines came out white) */
static u8 g_psdef[48][0xF0] __attribute__((aligned(16)));
static u32 g_ps_col[48];
static s32 g_ps_n;
static u8 *ps_for(u32 packed)
{
	s32 i, slot;
	u8 *ps;
	for (i = 0; i < g_ps_n; i++) if (g_ps_col[i] == packed) return g_psdef[i];
	slot = g_ps_n < 48 ? g_ps_n++ : (s32)(packed % 48u);
	g_ps_col[slot] = packed;
	ps = g_psdef[slot];
	memset(ps, 0, 0xF0);
	*(u32 *)(ps + 0x00) = 0x11200000;     /* PSAlphaInputs[0] */
	*(u32 *)(ps + 0x88) = 0x1200000;      /* PSRGBInputs[0] */
	*(u32 *)(ps + 0xD4) = 1;              /* PSCombinerCount */
	*(u32 *)(ps + 0x28) = packed;         /* PSConstant0[0]: the colour */
	*(u32 *)(ps + 0x68) = 0xC00;          /* PSAlphaOutputs[0] */
	*(u32 *)(ps + 0xB4) = 0xC00;          /* PSRGBOutputs[0] */
	*(u32 *)(ps + 0x20) = 0xC;            /* PSFinalCombinerInputsABCD */
	*(u32 *)(ps + 0x24) = 0x1C00;         /* PSFinalCombinerInputsEFG */
	return ps;
}
#endif
static void gfx_begin(float a, float r, float g, float b)
{
	u32 packed = ((u32)(a * 255.f) << 24) | ((u32)(r * a * 255.f) << 16) | ((u32)(g * a * 255.f) << 8) | (u32)(b * a * 255.f);
#ifdef HOST
	g_gfx_open = (s32)packed; g_gfx_quads = 0;
	(void)ADDR_PS_DEF;
#else
	u8 *ps = ps_for(packed);
	float x = 1.f / (float)g_vp_w, y = 1.f / (float)g_vp_h;
	float c[20] = { 2.f * x, 0, 0, -1.f - x,  0, -2.f * y, 0, 1.f + y,  0, 0, 0, 0.5f,  0, 0, 0, 1.f,  0, 0, 0, 1.f };
	gfx_render_state(0x93, 0);            /* CULLMODE: none */
	gfx_render_state(0x43, 0x10101);      /* COLORWRITEENABLE: rgb */
	gfx_render_state(0x3b, 1);            /* ALPHABLENDENABLE */
	gfx_render_state(0x3e, 1);            /* SRCBLEND: one (the colour is premultiplied) */
	gfx_render_state(0x3f, 0x303);        /* DESTBLEND: inverse source alpha */
	gfx_render_state(0x4a, 0x8006);       /* BLENDOP: add */
	gfx_render_state(0x3c, 0);            /* ALPHATESTENABLE */
	gfx_render_state(0x8f, 0);            /* ZENABLE: draw on top */
	gfx_render_state(0x95, 0);            /* ZBIAS */
	gfx_vs_select(13);
	gfx_call1(ADDR_VS_CACHE, ADDR_VS_CACHE_ARG);   /* function_1c710(g_51f0f0) */
	gfx_vs_constants(0xb1, c, 20);
	(void)ADDR_PS_DEF;
	gfx_ps_set(ps);
	gfx_call0(ADDR_COMMIT);
	gfx_call1(ADDR_BEGIN, 8);             /* D3DPT_QUADLIST */
	g_gfx_open = 1; g_gfx_quads = 0;
#endif
}

static void gfx_vertex(s32 x, s32 y)
{
#ifndef HOST
	gfx_call3(ADDR_VERTEX2S, 0, (u32)(s32)x, (u32)(s32)y);   /* position: emits the vertex */
#else
	(void)x; (void)y;
#endif
}

static void gfx_quad(s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2, s32 x3, s32 y3)
{
	if (!g_gfx_open) return;
	/* no vertex the GPU can't take: a float that wasn't a number becomes about -2 billion */
	if (x0 < -8192 || x0 > 8192 || y0 < -8192 || y0 > 8192 || x1 < -8192 || x1 > 8192 || y1 < -8192 || y1 > 8192 ||
	    x2 < -8192 || x2 > 8192 || y2 < -8192 || y2 > 8192 || x3 < -8192 || x3 > 8192 || y3 < -8192 || y3 > 8192) return;
	/* never a negative corner (the 360 read one as far to the right): clamped to the screen's edge */
	if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 < 0) x1 = 0; if (y1 < 0) y1 = 0;
	if (x2 < 0) x2 = 0; if (y2 < 0) y2 = 0; if (x3 < 0) x3 = 0; if (y3 < 0) y3 = 0;
#ifdef HOST
	{ s16 xy[8] = { (s16)x0, (s16)y0, (s16)x1, (s16)y1, (s16)x2, (s16)y2, (s16)x3, (s16)y3 }; host_gfx_quad((u32)g_gfx_open, xy); }
#else
	/* batches of 32 quads: a long immediate-mode batch is fine on the Xbox, but the 360's translation of it garbled the
	 * seams (squares, lines across the screen, most of them missing) - End and Begin again, the state unchanged */
	if (g_gfx_quads && !(g_gfx_quads & 31)) { gfx_call0(ADDR_END); gfx_call1(ADDR_BEGIN, 8); }
	gfx_vertex(x0, y0); gfx_vertex(x1, y1); gfx_vertex(x2, y2); gfx_vertex(x3, y3);
#endif
	g_gfx_quads++;
}

static void gfx_end(void)
{
#ifndef HOST
	if (g_gfx_open) gfx_call0(ADDR_END);
#endif
	g_gfx_open = 0;
}

static void gfx_rect(s32 x0, s32 y0, s32 x1, s32 y1) { gfx_quad(x0, y0, x1, y0, x1, y1, x0, y1); }

/* a line as a quad 2 pixels wide (viewport pixels) */
static float g_line_half = 1.f;                 /* half the line width, pixels */
/* the line clipped to the screen (less a margin for its width) before its quad is made (Liang-Barsky): no corner goes
 * negative - on the 360 a negative corner (a seam end off the left of the screen) came out far to the RIGHT (the 16-bit
 * screen position read unsigned): every such seam ran to the right edge. On the Xbox nothing changes (the GPU clipped
 * at the edge anyway). Returns 0: nothing on screen. */
static s32 clip2d(float *x0, float *y0, float *x1, float *y1, float m)
{
	float t0 = 0.f, t1 = 1.f, dx = *x1 - *x0, dy = *y1 - *y0, p[4], q[4], lo[2], hi[2];
	s32 i;
	lo[0] = m; lo[1] = m; hi[0] = (float)g_vp_w - m; hi[1] = (float)g_vp_h - m;
	p[0] = -dx; q[0] = *x0 - lo[0]; p[1] = dx; q[1] = hi[0] - *x0;
	p[2] = -dy; q[2] = *y0 - lo[1]; p[3] = dy; q[3] = hi[1] - *y0;
	for (i = 0; i < 4; i++)
	{
		if (p[i] == 0.f) { if (q[i] < 0.f) return 0; continue; }
		{
			float r = q[i] / p[i];
			if (p[i] < 0.f) { if (r > t1) return 0; if (r > t0) t0 = r; }
			else { if (r < t0) return 0; if (r < t1) t1 = r; }
		}
	}
	{ float ax = *x0, ay = *y0; *x0 = ax + dx * t0; *y0 = ay + dy * t0; *x1 = ax + dx * t1; *y1 = ay + dy * t1; }
	return 1;
}
static void gfx_line(float x0, float y0, float x1, float y1)
{
	float dx, dy, l;
	if (!(x0 == x0 && y0 == y0 && x1 == x1 && y1 == y1)) return;              /* not a number */
	if (!clip2d(&x0, &y0, &x1, &y1, g_line_half + 1.f)) return;               /* nothing on screen */
	dx = x1 - x0; dy = y1 - y0; l = fsqrt(dx * dx + dy * dy);
	if (!(l >= 0.5f && l < 20000.f)) return;                                  /* NaN fails too */
	float nx = -dy / l * g_line_half, ny = dx / l * g_line_half;
	gfx_quad((s32)(x0 + nx), (s32)(y0 + ny), (s32)(x1 + nx), (s32)(y1 + ny), (s32)(x1 - nx), (s32)(y1 - ny), (s32)(x0 - nx), (s32)(y0 - ny));
}

/* a world segment: clipped at the near plane, projected, drawn */
/* a point's depth in front of the camera the projection uses - the game's world frame when it's used, else ours: the
 * clip must be against the same camera as the projection (5e.2 clipped against one and projected with the other - a
 * point just behind the drawn camera passed the clip and projected thousands of pixels out: a seam across the screen) */
static float view_depth(const float *w)
{
	if (g_use_frame) { float d[3] = { w[0] - g_fr_p[0], w[1] - g_fr_p[1], w[2] - g_fr_p[2] }; return -dot3(d, g_fr_b); }
	{ float d[3] = { w[0] - g_cam_p[0], w[1] - g_cam_p[1], w[2] - g_cam_p[2] }; return dot3(d, g_cam_f); }
}
static void gfx_line3(const float *a, const float *b)
{
	float p[3], q[3], sa, sb, xa, ya, xb, yb, da, db;
	s32 i;
	for (i = 0; i < 3; i++) { p[i] = a[i]; q[i] = b[i]; }
	sa = view_depth(p); sb = view_depth(q);
	const float near_z = 0.25f;                                             /* closer than this to the camera: cut (an edge beside the camera spanned the screen) */
	if (sa < near_z && sb < near_z) return;
	if (sa < near_z) { float t = (near_z - sa) / (sb - sa); for (i = 0; i < 3; i++) p[i] += (q[i] - p[i]) * t; }
	if (sb < near_z) { float t = (near_z - sb) / (sa - sb); for (i = 0; i < 3; i++) q[i] += (p[i] - q[i]) * t; }
	project(p, &xa, &ya, &da); project(q, &xb, &yb, &db);
	/* written so a NaN fails it (a comparison with NaN is false): far off screen or not a number - not drawn */
	if (!(xa > -4000.f && xa < 4000.f && ya > -4000.f && ya < 4000.f && xb > -4000.f && xb < 4000.f && yb > -4000.f && yb < 4000.f)) return;
	gfx_line(xa, ya, xb, yb);
}

/* step 3a's test: a 1 x 1 square on the floor and a 1-unit post, dropped 2 units in front of you (along the
 * camera's horizontal facing) whenever the menu closes or you enter a level, then fixed in the world */
static float g_marker[3];
static u32 g_marker_object;
#ifdef HOST
void host_marker_reset(void) { g_marker_set = 0; }
void host_set_marker(s32 on) { o_marker = on; }
void host_set_seams(s32 on) { o_seams = on; }
void host_set_sb_viewer(s32 on) { o_sb_hud = on; }
void host_set_land_text(s32 on) { o_land_text = on; }
void host_set_force_charge(s32 on) { o_force_charge = on; }
void host_set_keep_charge(s32 on) { o_keep_charge = on; }
#endif

static void marker_place(u32 object)
{
	float fx = g_cam_f[0], fy = g_cam_f[1], l = fsqrt(fx * fx + fy * fy);
	if (l < 0.1f) { fx = 1.f; fy = 0.f; l = 1.f; }
	g_marker[0] = MF(object + 0x64) + 2.f * fx / l;
	g_marker[1] = MF(object + 0x68) + 2.f * fy / l;
	g_marker[2] = MF(object + 0x6C) + 0.03f;
	g_marker_set = 1; g_marker_object = object;
}

static void draw_marker(u32 object)
{
	if (!g_marker_set || g_marker_object != object) marker_place(object);
	float o[3] = { g_marker[0], g_marker[1], g_marker[2] };
	float c[4][3] = { { o[0] - 0.5f, o[1] - 0.5f, o[2] }, { o[0] + 0.5f, o[1] - 0.5f, o[2] }, { o[0] + 0.5f, o[1] + 0.5f, o[2] }, { o[0] - 0.5f, o[1] + 0.5f, o[2] } };
	float top[3] = { o[0], o[1], o[2] + 1.f };
	s32 i;
	gfx_begin(0.9f, 0.2f, 0.9f, 1.f);
	for (i = 0; i < 4; i++) gfx_line3(c[i], c[(i + 1) % 4]);
	gfx_line3(o, top);
	gfx_end();
}

/* ---- steps 3b / 3c: collision seams near you - the level's and its pieces' (Cartographer's floor seams) ----
 * A collision BSP's blocks are {count, pointer}: 3D nodes +0x00, planes +0x08 (16 bytes: normal, distance),
 * leaves +0x10, 2D references +0x18, 2D nodes +0x20, surfaces +0x28 (8 bytes: plane - 0x8000 = used flipped -
 * first edge, flags, breakable, material), edges +0x30 (12 bytes: start / end vertex, forward / reverse edge,
 * left / right surface), vertices +0x38 (16 bytes: point, first edge). Indices are 16-bit.
 * The level's is g_4e033c (what the game's leaf search walks). The pieces - instanced geometry - are in the
 * structure BSP g_4e0348: instances +0x140/+0x144 (0x58 bytes: scale +0x00, forward +0x04, left +0x10, up +0x1C,
 * position +0x28, definition +0x34), definitions +0x138/+0x13C (0xC8 bytes: bounding sphere centre +0x60, radius
 * +0x6C, collision BSP +0x70 - as the game's decal, leaf-search and item code index them). A piece's point goes
 * to the world as position + (forward x + left y + up z) x scale. Every BSP's layout is checked on a sample of
 * every block before anything is drawn from it; a failure is logged and that BSP is skipped. */
static u32 M16(u32 a) { return (M32(a & ~3u) >> ((a & 2) * 8)) & 0xffff; }
static s32 fin(float f) { return f > -50000.f && f < 50000.f; }    /* false for NaN too */
typedef struct { u32 ns, ne, nv, np, surf, edge, vert, plane; } s_cb;
typedef struct { s32 on; float fw[3], lf[3], up[3], pos[3], sc; } s_xf;

static void cb_load(s_cb *c, u32 b)
{
	c->np = M32(b + 0x08); c->plane = M32(b + 0x0C); c->ns = M32(b + 0x28); c->surf = M32(b + 0x2C);
	c->ne = M32(b + 0x30); c->edge = M32(b + 0x34); c->nv = M32(b + 0x38); c->vert = M32(b + 0x3C);
}

static void cb_vert(const s_cb *c, u32 v, float *p) { u32 a = c->vert + v * 16; p[0] = MF(a); p[1] = MF(a + 4); p[2] = MF(a + 8); }

static s32 cb_normal(const s_cb *c, u32 s, float *n)
{
	u32 p, idx, a;
	if (s >= c->ns) return 0;                                               /* a surface past the end (a Havok key's) */
	p = M16(c->surf + s * 8); idx = p & 0x7fff;
	float sg = (p & 0x8000) ? -1.f : 1.f, l;
	if (idx >= c->np) return 0;
	a = c->plane + idx * 16;
	n[0] = MF(a) * sg; n[1] = MF(a + 4) * sg; n[2] = MF(a + 8) * sg;
	l = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
	return l > 0.8f && l < 1.2f;
}

/* 0 if the layout holds; otherwise what failed */
static const char *cb_check(const s_cb *c)
{
	u32 i, step;
	if (!c->ns || !c->ne || !c->nv || !c->np || c->ns > 0xffff || c->ne > 0xffff || c->nv > 0xffff || c->np > 0xffff) return "counts";
	if (!ptr_ok(c->surf) || !ptr_ok(c->edge) || !ptr_ok(c->vert) || !ptr_ok(c->plane)) return "block pointers";
	step = c->ne / 3000 + 1;
	for (i = 0; i < c->ne; i += step)
	{
		u32 ea = c->edge + i * 12, sl = M16(ea + 8), sr = M16(ea + 10);
		if (M16(ea) >= c->nv || M16(ea + 2) >= c->nv) return "edge vertices";
		if ((sl >= c->ns && sl != 0xffff) || (sr >= c->ns && sr != 0xffff) || (sl == 0xffff && sr == 0xffff)) return "edge surfaces";
	}
	step = c->ns / 3000 + 1;
	for (i = 0; i < c->ns; i += step)
	{
		float n[3];
		if (M16(c->surf + i * 8 + 2) >= c->ne) return "surface first edge";
		if (!cb_normal(c, i, n)) return "surface plane";
	}
	step = c->nv / 3000 + 1;
	for (i = 0; i < c->nv; i += step)
	{
		float p[3];
		cb_vert(c, i, p);
		if (!fin(p[0]) || !fin(p[1]) || !fin(p[2])) return "vertex coordinates";
	}
	return 0;
}

/* the level's BSP: checked once per BSP */
static s_cb g_cbm;
static u32 g_cbm_for, g_cbm_sig;
static s32 g_cb_state;                         /* 1 good, -1 failed (this BSP), 0 none */
static s32 cb_ready(void)
{
	u32 b = M32(ADDR_COLLISION_BSP), sig;
	if (!b || !ptr_ok(b)) return 0;
	sig = M32(b + 0x28) ^ (M32(b + 0x30) << 8) ^ (M32(b + 0x38) << 16) ^ M32(b + 0x08);
	if (b != g_cbm_for || sig != g_cbm_sig)
	{
		const char *why;
		s32 at;
		g_cbm_for = b; g_cbm_sig = sig;
		cb_load(&g_cbm, b);
		why = cb_check(&g_cbm);
		at = lput(0, "collision BSP "); at = lhex(at, b);
		at = lput(at, ": surfaces "); at = lnum(at, g_cbm.ns); at = lput(at, ", edges "); at = lnum(at, g_cbm.ne);
		at = lput(at, ", vertices "); at = lnum(at, g_cbm.nv); at = lput(at, ", planes "); at = lnum(at, g_cbm.np);
		if (why) { at = lput(at, " - layout check FAILED ("); at = lput(at, why); lput(at, "): seams off for this level"); }
		else lput(at, " - layout checked OK");
		log_write();
		g_cb_state = why ? -1 : 1;
	}
	return g_cb_state > 0;
}

/* atan2 for y >= 0, x > 0 (angles up to 45 degrees here) */
static float fatan2(float y, float x)
{
#ifdef HOST
	float t = y / x, t2 = t * t;
	return t * (1.f - t2 * (0.3333333f - t2 * (0.2f - t2 * (0.1428571f - t2 * (0.1111111f - t2 * 0.0909091f)))));
#else
	float r; __asm__("fpatan" : "=t"(r) : "0"(x), "u"(y) : "st(1)"); return r;
#endif
}

/* a vertex of surface s off the line a-b (walking its edges from the first), for the bend's sign */
static s32 cb_point_off(const s_cb *c, u32 s, const float *a, const float *b, float *q)
{
	u32 e = M16(c->surf + s * 8 + 2);
	float d[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, dl2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
	s32 k, j;
	for (k = 0; k < 8 && e < c->ne; k++)
	{
		u32 ea = c->edge + e * 12;
		for (j = 0; j < 2; j++)
		{
			u32 v = M16(ea + 2 * j);
			float p[3], w[3], x[3];
			if (v >= c->nv) return 0;
			cb_vert(c, v, p);
			w[0] = p[0] - a[0]; w[1] = p[1] - a[1]; w[2] = p[2] - a[2];
			x[0] = w[1] * d[2] - w[2] * d[1]; x[1] = w[2] * d[0] - w[0] * d[2]; x[2] = w[0] * d[1] - w[1] * d[0];
			if (x[0] * x[0] + x[1] * x[1] + x[2] * x[2] > 1e-6f * dl2) { q[0] = p[0]; q[1] = p[1]; q[2] = p[2]; return 1; }
		}
		e = (M16(ea + 8) == s) ? M16(ea + 4) : M16(ea + 6);   /* forward edge if s is its left surface, else reverse */
	}
	return 0;
}

static void xf_point(const s_xf *x, const float *p, float *o)
{
	s32 i;
	if (!x->on) { o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; return; }
	for (i = 0; i < 3; i++) o[i] = x->pos[i] + (x->fw[i] * p[0] + x->lf[i] * p[1] + x->up[i] * p[2]) * x->sc;
}

static void xf_dir(const s_xf *x, const float *n, float *o)
{
	s32 i;
	if (!x->on) { o[0] = n[0]; o[1] = n[1]; o[2] = n[2]; return; }
	for (i = 0; i < 3; i++) o[i] = x->fw[i] * n[0] + x->lf[i] * n[1] + x->up[i] * n[2];
}

/* this frame's floor seams near your feet: within 8 units across, 12 below to 0.5 above; the nearest 160 kept.
 * Floor seam (as Cartographer): both faces at least 60 degrees up, within 30 degrees of each other. Bend signed:
 * + valley, - ridge, 0 flat (under 0.3 degrees). */
#define SEAM_MAX 160
#ifdef HOST
static s32 g_seam_cap = SEAM_MAX;
#else
static s32 g_seam_cap = 64;                      /* the nearest 64 (6h.20: from the start; SEAM_MAX 160 the full limit) */
#endif
#define SEAM_REACH 8.f                  /* the search (as 3d.8) */
#define RATE_REACH 8.f                  /* a seam not yet shown */
enum { SK_FLAT, SK_VALLEY, SK_RIDGE, SK_BENT };
static float g_seam_ab[SEAM_MAX][6], g_seam_d2[SEAM_MAX];
static s32 g_sf_air;                            /* this search: airborne, and your velocity (for the landing search) */
#define NB_MAX 40
static s_cb g_nb_cb[NB_MAX];                    /* the pieces' and objects' collision BSPs found near you this search */
static s_xf g_nb_xf[NB_MAX];
static s32 g_nb_n;
static float g_sf_v[3];
#define AIR_REACH 2.25f                         /* air control: how fast you can still move your landing (u/s) */
#define AIR_REACH_MAX 10.f                      /* the most around your landing that seams are kept from (Cartographer's 40: high up, a
                                                   huge area - too many seams out of reach searched and rated) */

/* where you'd be when your fall reaches height z (gravity, this velocity); 0 if never */
static s32 land_at(const float *f, float z, float *x, float *y, float *t_out)
{
	float g = 4.1712f, vz = g_sf_v[2], h = f[2] - z, disc = vz * vz + 2.f * g * h, t;
	if (disc < 0.f) return 0;
	t = (vz + fsqrt(disc)) / g;
	if (t <= 0.f) return 0;
	*x = f[0] + g_sf_v[0] * t; *y = f[1] + g_sf_v[1] * t; *t_out = t;
	return 1;
}

/* the key a seam is kept by (smaller = kept first), or -1 to leave it out. Cartographer's selection: seams below you
 * first; in the air, near where you'd land at the seam's height (air control can still move you 2.25 u/s), else near
 * your feet (8 units). Seams at your level and above only fill what room is left. */
static s32 g_land_ok;
static float g_land_p[3];
/* in the air: is something of radius r around (x, y) where you can still land? Near your feet (8), or within air
 * control's reach of your predicted landing (2.25 u/s x the fall time left, + 1, at most 40 - Cartographer's air
 * search); with no landing found yet, 40 around your feet. */
static s32 air_near(const float *f, float x, float y, float r)
{
	float dx = x - f[0], dy = y - f[1], d = fsqrt(dx * dx + dy * dy) - r, lx, ly, lt;
	if (!g_land_ok) return 0;                                                  /* no landing yet: nothing (only the landing zone) */
	if (!land_at(f, g_land_p[2], &lx, &ly, &lt)) return 0;
	{
		float reach = AIR_REACH * lt + 1.f, ex = x - g_land_p[0], ey = y - g_land_p[1];
		if (reach > AIR_REACH_MAX) reach = AIR_REACH_MAX;
		return fsqrt(ex * ex + ey * ey) - r <= reach + 2.f;
	}
}

#ifdef HOST
s32 host_air_near(const float *f, const float *v, s32 land_ok, const float *lp, float x, float y, float r)
{
	s32 k, res;
	for (k = 0; k < 3; k++) { g_sf_v[k] = v[k]; g_land_p[k] = lp[k]; }
	g_land_ok = land_ok; res = air_near(f, x, y, r); g_land_ok = 0;
	return res;
}
#endif
static float seam_key(const float *f, const float *a, const float *b);
#ifdef HOST
/* a seam kept in the air (its key >= 0): the same set-up as host_air_near */
s32 host_seam_kept_air(const float *f, const float *v, s32 land_ok, const float *lp, const float *a, const float *b)
{
	s32 k, air = g_sf_air;
	float key;
	for (k = 0; k < 3; k++) { g_sf_v[k] = v[k]; g_land_p[k] = lp[k]; }
	g_land_ok = land_ok; g_sf_air = 1; key = seam_key(f, a, b); g_land_ok = 0; g_sf_air = air;
	return key >= 0.f;
}
#endif
static float seam_key(const float *f, const float *a, const float *b)
{
	float dx = b[0] - a[0], dy = b[1] - a[1], t, cx, cy, d2, lx, ly, lt, l2 = 1e9f;
	s32 below = (a[2] < b[2] ? a[2] : b[2]) <= f[2] - 0.3f;
	t = dx * dx + dy * dy;
	t = t > 1e-8f ? ((f[0] - a[0]) * dx + (f[1] - a[1]) * dy) / t : 0.f;
	if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
	cx = a[0] + dx * t - f[0]; cy = a[1] + dy * t - f[1]; d2 = cx * cx + cy * cy;
	if (g_sf_air)
	{
		/* in the air: only around the landing point itself - within your air control's reach of it, and at its height
		 * (2 below to 1 above). No landing found: none. (5e.17 took, for each seam, where the trajectory crosses ITS height:
		 * seams higher up met the arc earlier, away from the reticle - and the shown set crawled along it.) */
		float r, u, lo_z = (a[2] < b[2] ? a[2] : b[2]), hi_z = (a[2] > b[2] ? a[2] : b[2]);
		if (!g_land_ok || hi_z < g_land_p[2] - 2.f || lo_z > g_land_p[2] + 1.f) return -1.f;
		if (!land_at(f, g_land_p[2], &lx, &ly, &lt)) return -1.f;
		r = AIR_REACH * lt + 1.f;
		if (r > AIR_REACH_MAX) r = AIR_REACH_MAX;
		u = dx * dx + dy * dy;
		u = u > 1e-8f ? ((g_land_p[0] - a[0]) * dx + (g_land_p[1] - a[1]) * dy) / u : 0.f;
		if (u < 0.f) u = 0.f; else if (u > 1.f) u = 1.f;
		lx = a[0] + dx * u - g_land_p[0]; ly = a[1] + dy * u - g_land_p[1]; l2 = lx * lx + ly * ly;
		return l2 <= r * r ? l2 : -1.f;
	}
	if (0)
	{
		float r = AIR_REACH * lt + 1.f, u;
		if (r > AIR_REACH_MAX) r = AIR_REACH_MAX;
		u = dx * dx + dy * dy;
		u = u > 1e-8f ? ((lx - a[0]) * dx + (ly - a[1]) * dy) / u : 0.f;
		if (u < 0.f) u = 0.f; else if (u > 1.f) u = 1.f;
		lx = a[0] + dx * u - lx; ly = a[1] + dy * u - ly; l2 = lx * lx + ly * ly;
		if (l2 <= r * r) return l2;                                     /* near your landing: first */
	}
	/* in the air with a landing found: only seams your air control can still reach around it - not the 8 units around your
	 * feet as well (5e.13: off a short jump that ran 5-10 units past the reticle - unreachable, and more work) */
	if (g_sf_air) return -1.f;                                                 /* in the air: only the landing zone (none yet: none) */
	if (d2 > SEAM_REACH * SEAM_REACH) return -1.f;
	return below ? (g_sf_air ? 1600.f + d2 : d2) : 10000.f + d2;        /* below you, then the rest */
}
static u8 g_seam_kind[SEAM_MAX], g_seam_piece[SEAM_MAX];
static s32 g_seam_n, g_seam_kinds[4], g_seam_far = -1, g_piece_seams, g_pieces_near, g_object_seams, g_objects_near, g_object_bsps;
#ifdef HOST
static u32 g_def_bad;
s32 host_object_bsps(void) { return g_object_bsps; }
s32 host_seam_count(s32 kind) { return kind == -4 ? g_object_seams : kind == -3 ? (s32)g_def_bad : kind == -2 ? g_piece_seams : kind < 0 ? g_seam_n : g_seam_kinds[kind]; }
#endif

static void seam_add(const float *a, const float *b, u8 kind, float d2, u8 piece)
{
	s32 slot = g_seam_n < g_seam_cap ? g_seam_n++ : g_seam_far, i;
	if (slot == g_seam_far) { g_seam_kinds[g_seam_kind[slot]]--; if (g_seam_piece[slot] == 1) g_piece_seams--; if (g_seam_piece[slot] == 2) g_object_seams--; }
	for (i = 0; i < 3; i++) { g_seam_ab[slot][i] = a[i]; g_seam_ab[slot][3 + i] = b[i]; }
	g_seam_d2[slot] = d2; g_seam_kind[slot] = kind; g_seam_kinds[kind]++; g_seam_piece[slot] = piece;
	if (piece == 1) g_piece_seams++;
	if (piece == 2) g_object_seams++;
	if (g_seam_n == g_seam_cap)
	{
		g_seam_far = 0;
		for (i = 1; i < g_seam_cap; i++) if (g_seam_d2[i] > g_seam_d2[g_seam_far]) g_seam_far = i;
	}
}

static void seam_edge(const s_cb *c, const s_xf *x, const float *f, u8 piece, u32 e);
static void seams_from(const s_cb *c, const s_xf *x, const float *f, u8 piece)
{
	u32 e;
	for (e = 0; e < c->ne; e++) seam_edge(c, x, f, piece, e);
}
static u32 g_lsd[6];                            /* the level's edges this search: handed over, too high / low, out of reach, behind the full table, not a floor seam, kept */
static void seam_edge(const s_cb *c, const s_xf *x, const float *f, u8 piece, u32 e)
{
	if (!piece) g_lsd[0]++;
	{
		u32 ea = c->edge + e * 12, va = M16(ea), vb = M16(ea + 2), sl = M16(ea + 8), sr = M16(ea + 10);
		float la[3], lb[3], a[3], b[3], na[3], nb[3], wa[3], wb[3], dx, dy, t, cx, cy, d2, cs, cr[3], bend;
		u8 kind = SK_FLAT;
		if (sl >= c->ns || sr >= c->ns || sl == sr || va >= c->nv || vb >= c->nv) return;
		cb_vert(c, va, la); cb_vert(c, vb, lb);
		xf_point(x, la, a); xf_point(x, lb, b);
		if ((a[2] < b[2] ? a[2] : b[2]) > f[2] + 0.5f || (a[2] > b[2] ? a[2] : b[2]) < f[2] - (g_sf_air ? 400.f : 30.f)) { if (!piece) g_lsd[1]++; return; }
		d2 = seam_key(f, a, b);
		if (d2 < 0.f) { if (!piece) g_lsd[2]++; return; }
		if (g_seam_far >= 0 && d2 >= g_seam_d2[g_seam_far]) { if (!piece) g_lsd[3]++; return; }     /* full, and not kept before the last one */
		if (!cb_normal(c, sl, na) || !cb_normal(c, sr, nb)) { if (!piece) g_lsd[4]++; return; }
		xf_dir(x, na, wa); xf_dir(x, nb, wb);                              /* the floor test in the world */
		cs = dot3(na, nb);
		if (wa[2] < 0.5f || wb[2] < 0.5f || cs < 0.866f) { if (!piece) g_lsd[4]++; return; }
		cr[0] = na[1] * nb[2] - na[2] * nb[1]; cr[1] = na[2] * nb[0] - na[0] * nb[2]; cr[2] = na[0] * nb[1] - na[1] * nb[0];
		bend = fatan2(fsqrt(dot3(cr, cr)), cs) * 57.29578f;               /* in the piece's own space: same angle */
		if (bend >= 0.3f)
		{
			float qa[3], qb[3];
			kind = SK_BENT;
			if (cb_point_off(c, sl, la, lb, qa) && cb_point_off(c, sr, la, lb, qb))
			{
				float above = (qb[0] - la[0]) * na[0] + (qb[1] - la[1]) * na[1] + (qb[2] - la[2]) * na[2]
					+ (qa[0] - la[0]) * nb[0] + (qa[1] - la[1]) * nb[1] + (qa[2] - la[2]) * nb[2];
				kind = above > 0.f ? SK_VALLEY : SK_RIDGE;                 /* signs kept by rotation + uniform scale */
			}
		}
		seam_add(a, b, kind, d2, piece);
	}
}

/* ---- the level's floor-seam edges by area: built once per level (both faces floors under 30 deg apart - the test
 * seam_edge applies to the level, which is not transformed), each in the cell of its midpoint (cells of at least 8
 * units); edges longer than 32 on a list read every time; a search reads the cells around you (widened by the
 * longest half-edge) instead of every edge. Too big for the tables: every edge, as before. ---- */
#define LG_CELLS 64
#define LG_MAX 12288                            /* floor-seam edges in the grid (Cairo 3532, Headlong 2328); more: the near list */
#define LG_LONG 512
/* 32-bit (5e.8 and before: 16-bit): on the 360 the 16-bit version came out empty - "2328 of 2328 floor-seam edges missing
 * from their cells" - while the same code works on the Xbox; plain 32-bit loads and stores avoid whatever it mistranslated */
static u16 g_lg_items[LG_MAX], g_lg_start[LG_CELLS * LG_CELLS + 1], g_lg_long[LG_LONG];   /* 16-bit again (5e.12's memory) */
static u8 g_lg_chalf[LG_CELLS * LG_CELLS];       /* each cell's own longest half-edge, whole units up (0-16): a cell too far for its own
                                                    edges to reach is skipped (the search reached 8 + the LEVEL's longest, ~24) */
static float g_lg_zmin;                          /* the level's lowest vertex: the trajectory stops below it */
static s32 g_lg_ok, g_lg_nx, g_lg_ny, g_lg_nlong, g_lg_off;
static float g_lg_x0, g_lg_y0, g_lg_cs, g_lg_half;
static u32 g_lg_for, g_lg_sig;
/* a point's cell, by counting cell boundaries with comparisons - no float-to-integer conversion (on the 360 the build's
 * conversions all came out as cell 0: "placed 2328 of 2328 ... list 2328-2328") - and one function, not inlined (called
 * with the x87 stack empty), so the build, its check and the search always agree */
static __attribute__((noinline)) void lg_cell(float x, float y, s32 *cx, s32 *cy)
{
	s32 i = 0, j = 0;
	while (i < g_lg_nx - 1 && x >= g_lg_x0 + (float)(i + 1) * g_lg_cs) i++;
	while (j < g_lg_ny - 1 && y >= g_lg_y0 + (float)(j + 1) * g_lg_cs) j++;
	*cx = i; *cy = j;
}
static s32 lg_floor_edge(const s_cb *c, u32 e, float *mid, float *half)
{
	u32 ea = c->edge + e * 12, va = M16(ea), vb = M16(ea + 2), sl = M16(ea + 8), sr = M16(ea + 10);
	float a[3], b[3], na[3], nb[3], dx, dy;
	if (sl >= c->ns || sr >= c->ns || sl == sr || va >= c->nv || vb >= c->nv) return 0;
	if (!cb_normal(c, sl, na) || !cb_normal(c, sr, nb)) return 0;
	if (na[2] < 0.5f || nb[2] < 0.5f || dot3(na, nb) < 0.866f) return 0;
	cb_vert(c, va, a); cb_vert(c, vb, b);
	mid[0] = (a[0] + b[0]) * 0.5f; mid[1] = (a[1] + b[1]) * 0.5f;
	dx = b[0] - a[0]; dy = b[1] - a[1]; *half = fsqrt(dx * dx + dy * dy) * 0.5f;
	return 1;
}
static const char *g_sb_stage = "";             /* where the seam search is (a caught fault says) */
static void sb_stage(const char *st) { g_sb_stage = st; trace(st); }   /* traced too: a freeze leaves it in spooky_trace.txt */
static void lg_build(const s_cb *c)
{
	sb_stage("the level grid build");
	float lo[2] = { 1e30f, 1e30f }, hi[2] = { -1e30f, -1e30f }, mid[2], half, ext;
	u32 v, e;
	s32 pass, total = 0;
	g_lg_ok = 0; g_lg_nlong = 0; g_lg_half = 0.f; g_lg_off = 0;
	if (c->ne > 65535 || c->nv == 0) return;
	g_lg_zmin = 1e30f;
	for (v = 0; v < c->nv; v++)
	{
		float p[3];
		cb_vert(c, v, p);
		if (p[0] < lo[0]) lo[0] = p[0]; if (p[0] > hi[0]) hi[0] = p[0];
		if (p[1] < lo[1]) lo[1] = p[1]; if (p[1] > hi[1]) hi[1] = p[1];
		if (p[2] < g_lg_zmin) g_lg_zmin = p[2];
	}
	ext = hi[0] - lo[0] > hi[1] - lo[1] ? hi[0] - lo[0] : hi[1] - lo[1];
	g_lg_cs = ext / (float)LG_CELLS + 0.01f; if (g_lg_cs < 8.f) g_lg_cs = 8.f;
	g_lg_x0 = lo[0]; g_lg_y0 = lo[1];
	g_lg_nx = (s32)((hi[0] - lo[0]) / g_lg_cs) + 1; if (g_lg_nx > LG_CELLS) g_lg_nx = LG_CELLS;
	g_lg_ny = (s32)((hi[1] - lo[1]) / g_lg_cs) + 1; if (g_lg_ny > LG_CELLS) g_lg_ny = LG_CELLS;
	for (v = 0; v <= (u32)(g_lg_nx * g_lg_ny); v++) g_lg_start[v] = 0;
	for (v = 0; v < (u32)(g_lg_nx * g_lg_ny); v++) g_lg_chalf[v] = 0;
	for (pass = 0; pass < 2; pass++)
	{
		if (pass == 1)
		{
			s32 sum = 0, k;
			for (k = 0; k < g_lg_nx * g_lg_ny; k++) { s32 n = (s32)g_lg_start[k]; g_lg_start[k] = (u16)sum; sum += n; }
			g_lg_start[g_lg_nx * g_lg_ny] = (u16)sum; total = sum;
			if (total > LG_MAX) return;                                       /* too big: every edge, as before */
		}
		for (e = 0; e < c->ne; e++)
		{
			s32 cx, cy, k;
			if (!lg_floor_edge(c, e, mid, &half)) continue;
			if (half > 16.f)
			{
				if (pass == 0) { if (g_lg_nlong >= LG_LONG) return; g_lg_long[g_lg_nlong++] = (u16)e; }
				continue;
			}
			if (pass == 0 && half > g_lg_half) g_lg_half = half;
			lg_cell(mid[0], mid[1], &cx, &cy);
			k = cy * g_lg_nx + cx;
			if (pass == 0)
			{
				s32 hq = 0;
				if (g_lg_start[k] >= 65535) return;
				g_lg_start[k]++;
				while (hq < 17 && (float)hq < half) hq++;                         /* whole units up, by comparison */
				if (hq > g_lg_chalf[k]) g_lg_chalf[k] = (u8)hq;
			}
			else { u32 at = g_lg_start[k]; while (at < (u32)g_lg_start[k + 1] && g_lg_items[at] != 0xFFFF) at++; if (at < (u32)g_lg_start[k + 1]) g_lg_items[at] = (u16)e; }
		}
		if (pass == 0) { s32 k; for (k = 0; k < LG_MAX; k++) g_lg_items[k] = 0xFFFF; }
	}
	g_lg_ok = 1;
	sb_stage("the level grid build's check");
	{
		/* checked: every floor-seam edge must be in its own cell's list - if any isn't, the grid is off (the full search,
		 * every edge, as before it) - Headlong handed over nothing from the cells around you */
		u32 missing = 0, checked = 0;
		for (e = 0; e < c->ne; e++)
		{
			s32 cx, cy, k;
			u32 t, found = 0;
			if (!lg_floor_edge(c, e, mid, &half) || half > 16.f) continue;
			checked++;
			lg_cell(mid[0], mid[1], &cx, &cy);
			k = cy * g_lg_nx + cx;
			for (t = g_lg_start[k]; t < g_lg_start[k + 1]; t++) if (g_lg_items[t] == e) { found = 1; break; }
			if (!found && !missing)
			{
				/* the first one missing, in detail - what the 360 does differently */
				u32 placed = 0, q;
				s32 at;
				for (q = 0; q < (u32)total && q < LG_MAX; q++) if (g_lg_items[q] != 0xFFFF) placed++;
				at = lput(0, "level seam grid detail: placed "); at = lnum(at, placed); at = lput(at, " of "); at = lnum(at, (u32)total);
				at = lput(at, "; first missing edge "); at = lnum(at, e); at = lput(at, " mid x100 "); at = lsnum(at, (s32)(mid[0] * 100.f));
				at = lput(at, " "); at = lsnum(at, (s32)(mid[1] * 100.f)); at = lput(at, " cell "); at = lnum(at, (u32)cx); at = lput(at, ","); at = lnum(at, (u32)cy);
				at = lput(at, " (k "); at = lnum(at, (u32)k); at = lput(at, ") list "); at = lnum(at, g_lg_start[k]); at = lput(at, "-"); at = lnum(at, g_lg_start[k + 1]);
				at = lput(at, " first item "); at = lhex(at, g_lg_start[k] < LG_MAX ? g_lg_items[g_lg_start[k]] : 0);
				at = lput(at, "; cs x1000 "); at = lnum(at, (u32)(g_lg_cs * 1000.f)); at = lput(at, " origin x100 "); at = lsnum(at, (s32)(g_lg_x0 * 100.f));
				at = lput(at, " "); lsnum(at, (s32)(g_lg_y0 * 100.f));
				log_write();
			}
			if (!found) missing++;
		}
		if (missing)
		{
			s32 at = lput(0, "level seam grid: "); at = lnum(at, missing); at = lput(at, " of "); at = lnum(at, checked);
			lput(at, " floor-seam edges missing from their cells - the grid is OFF (the full search: every edge)");
			log_write();
			g_lg_ok = 0;
			return;
		}
	}
	{
		s32 at = lput(0, "level seam grid: ");
		at = lnum(at, (u32)total); at = lput(at, " floor-seam edges of "); at = lnum(at, c->ne);
		at = lput(at, " (room for "); at = lnum(at, LG_MAX); at = lput(at, "), long "); at = lnum(at, (u32)g_lg_nlong);
		at = lput(at, ", cells "); at = lnum(at, (u32)g_lg_nx); at = lput(at, " x "); at = lnum(at, (u32)g_lg_ny);
		at = lput(at, " of "); lnum(at, (u32)(g_lg_cs + 0.5f));
		log_write();
	}
}
#ifdef HOST
static s32 g_lg_disable, g_lg_force_off;
void host_set_level_grid(s32 on) { g_lg_disable = !on; g_lg_for = 0; }
void host_force_grid_failed(s32 on) { g_lg_force_off = on; }
#endif
/* the level's seams lost: while a search keeps none of them, one line every ~10 s - how many edges the grid handed over and
 * which test dropped them (Headlong: every seam came from pieces) */
static void lsd_report(const float *f)
{
	static u32 last;
	static s32 zero_run;
	s32 at;
	if (g_lsd[5]) { zero_run = 0; return; }
	if (++zero_run < 30 || g_count_main - last < 300) return;
	last = g_count_main;
	at = lput(0, "level seams: none kept - edges handed over "); at = lnum(at, g_lsd[0]);
	at = lput(at, ", too high / low "); at = lnum(at, g_lsd[1]); at = lput(at, ", out of reach "); at = lnum(at, g_lsd[2]);
	at = lput(at, ", behind a full table "); at = lnum(at, g_lsd[3]); at = lput(at, ", not a floor seam "); at = lnum(at, g_lsd[4]);
	at = lput(at, "; feet x100 "); at = lsnum(at, (s32)(f[0] * 100.f)); at = lput(at, " "); at = lsnum(at, (s32)(f[1] * 100.f)); at = lput(at, " "); at = lsnum(at, (s32)(f[2] * 100.f));
	at = lput(at, ", grid "); at = lput(at, g_lg_ok ? "on" : "off"); at = lput(at, " origin x100 "); at = lsnum(at, (s32)(g_lg_x0 * 100.f)); at = lput(at, " ");
	at = lsnum(at, (s32)(g_lg_y0 * 100.f)); at = lput(at, ", cells "); at = lnum(at, (u32)g_lg_nx); at = lput(at, " x "); at = lnum(at, (u32)g_lg_ny);
	log_write();
}
/* ~every 10 s: the cell under your feet - the edges its list holds against those that belong there (counted from the BSP
 * itself); different: the grid was damaged after it was built - off (the full search), and logged */
static void lg_check(const float *f)
{
	static u32 last;
	static s32 said_ok;
	s32 cx, cy, k, at;
	u32 listed, actual = 0, e;
	float mid[2], half;
	if (!g_lg_ok || g_count_main - last < 300) return;
	last = g_count_main;
	sb_stage("the level grid check");
	if (f[0] < g_lg_x0 || f[1] < g_lg_y0 || f[0] >= g_lg_x0 + (float)g_lg_nx * g_lg_cs || f[1] >= g_lg_y0 + (float)g_lg_ny * g_lg_cs) return;
	lg_cell(f[0], f[1], &cx, &cy);
	k = cy * g_lg_nx + cx;
	listed = (u32)g_lg_start[k + 1] - (u32)g_lg_start[k];
	for (e = 0; e < g_cbm.ne; e++)
	{
		s32 ex, ey;
		if (!lg_floor_edge(&g_cbm, e, mid, &half) || half > 16.f) continue;
		lg_cell(mid[0], mid[1], &ex, &ey);
		if (ey * g_lg_nx + ex == k) actual++;
	}
	if (listed == actual && said_ok) return;
	at = lput(0, "level seam grid check: the cell under you ("); at = lnum(at, (u32)cx); at = lput(at, ", "); at = lnum(at, (u32)cy);
	at = lput(at, ") lists "); at = lnum(at, listed); at = lput(at, " edges, holds "); at = lnum(at, actual);
	if (listed == actual) { lput(at, " - ok"); said_ok = 1; }
	else { lput(at, " - the grid was damaged: OFF (the full search)"); g_lg_ok = 0; }
	log_write();
}
/* the grid off (the 360): not every edge each search - a list of the floor-seam edges within reach, rebuilt only when
 * you've moved 4 units from where it was made (or gone between the ground and the air: in the air the landing reaches
 * 40 units) - built from the floor test and a distance alone */
#define NL_MAX 2048
static u32 g_nl[NL_MAX], g_nl_for, g_nl_sig;
static s32 g_nl_n, g_nl_ok, g_nl_air;
static float g_nl_c[2];
static void near_list_seams(const float *f, const s_xf *id)
{
	s32 i;
	float dx = f[0] - g_nl_c[0], dy = f[1] - g_nl_c[1];
	if (!g_nl_ok || g_nl_for != g_cbm_for || g_nl_sig != g_cbm_sig || g_nl_air != g_sf_air || dx * dx + dy * dy > 16.f)
	{
		float r = (g_sf_air ? 64.f : 28.f), mid[2], half;
		u32 e;
		g_nl_n = 0; g_nl_ok = 1; g_nl_for = g_cbm_for; g_nl_sig = g_cbm_sig; g_nl_air = g_sf_air; g_nl_c[0] = f[0]; g_nl_c[1] = f[1];
		for (e = 0; e < g_cbm.ne && g_nl_n < NL_MAX; e++)
		{
			float ex, ey, reach;
			if (!lg_floor_edge(&g_cbm, e, mid, &half)) continue;
			ex = mid[0] - f[0]; ey = mid[1] - f[1]; reach = r + half + 4.f;       /* + the 4 you may move before it's rebuilt */
			if (ex * ex + ey * ey <= reach * reach) g_nl[g_nl_n++] = e;
		}
	}
	for (i = 0; i < g_nl_n; i++) seam_edge(&g_cbm, id, f, 0, g_nl[i]);
}
static void level_seams(const float *f)
{
	s_xf id;
	float lo[2], hi[2], r;
	s32 x0, x1, y0, y1, cx, cy, k;
	id.on = 0;
	if (g_lg_for != g_cbm_for || g_lg_sig != g_cbm_sig) { g_lg_for = g_cbm_for; g_lg_sig = g_cbm_sig; lg_build(&g_cbm); }
#ifdef HOST
	if (g_lg_disable) { seams_from(&g_cbm, &id, f, 0); return; }
#endif
	lg_check(f);
	sb_stage("the level's seams");
#ifdef HOST
	if (g_lg_force_off) { near_list_seams(f, &id); return; }               /* as the 360: the grid failed */
#endif
	if (!g_lg_ok) { near_list_seams(f, &id); return; }
	/* the area a seam can be kept from: SEAM_REACH around you; in the air, also air control's reach of your landing */
	r = SEAM_REACH + g_lg_half;
	lo[0] = f[0] - r; hi[0] = f[0] + r; lo[1] = f[1] - r; hi[1] = f[1] + r;
	if (g_sf_air && g_land_ok)                                                 /* only around the landing */
	{
		lo[0] = g_land_p[0]; hi[0] = g_land_p[0]; lo[1] = g_land_p[1]; hi[1] = g_land_p[1];
	}
	if (g_sf_air)
	{
		float c[2], rr = AIR_REACH_MAX + 2.f + g_lg_half;
		c[0] = g_land_ok ? g_land_p[0] : f[0]; c[1] = g_land_ok ? g_land_p[1] : f[1];
		if (c[0] - rr < lo[0]) lo[0] = c[0] - rr; if (c[0] + rr > hi[0]) hi[0] = c[0] + rr;
		if (c[1] - rr < lo[1]) lo[1] = c[1] - rr; if (c[1] + rr > hi[1]) hi[1] = c[1] + rr;
	}
	lg_cell(lo[0], lo[1], &x0, &y0); lg_cell(hi[0], hi[1], &x1, &y1);        /* the same cells as the build (clamped inside) */
	{
		/* where seams are kept from: around you (8) on the ground; around the landing (its reach, at most 10, + 2) in the air */
		float ctr[2], reach;
		if (g_sf_air && g_land_ok) { ctr[0] = g_land_p[0]; ctr[1] = g_land_p[1]; reach = AIR_REACH_MAX + 2.f; }
		else if (g_sf_air) { ctr[0] = f[0]; ctr[1] = f[1]; reach = 1e9f; }                  /* no landing: as before */
		else { ctr[0] = f[0]; ctr[1] = f[1]; reach = SEAM_REACH + 0.5f; }
		for (cy = y0; cy <= y1; cy++)
			for (cx = x0; cx <= x1; cx++)
			{
				s32 cell = cy * g_lg_nx + cx, t;
				if (g_lg_start[cell] == g_lg_start[cell + 1]) continue;
				if (reach < 1e8f)
				{
					float rx0 = g_lg_x0 + (float)cx * g_lg_cs, ry0 = g_lg_y0 + (float)cy * g_lg_cs, ddx, ddy, rr = reach + (float)g_lg_chalf[cell];
					ddx = ctr[0] < rx0 ? rx0 - ctr[0] : ctr[0] > rx0 + g_lg_cs ? ctr[0] - rx0 - g_lg_cs : 0.f;
					ddy = ctr[1] < ry0 ? ry0 - ctr[1] : ctr[1] > ry0 + g_lg_cs ? ctr[1] - ry0 - g_lg_cs : 0.f;
					if (ddx * ddx + ddy * ddy > rr * rr) continue;                   /* too far for its own edges */
				}
				for (t = (s32)g_lg_start[cell]; t < (s32)g_lg_start[cell + 1]; t++) seam_edge(&g_cbm, &id, f, 0, g_lg_items[t]);
			}
	}
	for (k = 0; k < g_lg_nlong; k++) seam_edge(&g_cbm, &id, f, 0, g_lg_long[k]);
}

/* ---- each object BSP's size, measured once from its vertices (cached by address): far ones skipped before their
 * edges are walked ---- */
#define OR_N 512
static u32 g_or_key[OR_N];
static float g_or_r[OR_N];
static float bsp_radius(const s_cb *c, u32 key)
{
	u32 h = (key >> 4) % OR_N, n, v;
	float m = 0.f;
	for (n = 0; n < 8; n++, h = (h + 1) % OR_N)
	{
		if (g_or_key[h] == key) return g_or_r[h];
		if (!g_or_key[h]) break;
	}
	for (v = 0; v < c->nv; v++) { float p[3], d; cb_vert(c, v, p); d = dot3(p, p); if (d > m) m = d; }
	m = fsqrt(m);
	if (n < 8) { g_or_key[h] = key; g_or_r[h] = m; }
	return m;
}

/* an object's floor polygons with more than three corners, split as Havok - which superbounces hit - splits them
 * into triangles: a fan from the first corner. Each internal diagonal is a seam between two triangles of the same
 * plane: flat. (Cartographer gets objects as triangles, so it shows these; a polygon's own edges come from
 * seams_from as usual.) Corners: the surface's edge loop - forward when it is an edge's left surface, else reverse. */
static void diagonals_from(const s_cb *c, const s_xf *x, const float *f, u8 piece)
{
	u32 s;
	for (s = 0; s < c->ns; s++)
	{
		u32 vtx[16], e, e0, k = 0, i;
		float n[3], wn[3];
		if (!cb_normal(c, s, n)) continue;
		xf_dir(x, n, wn);
		if (wn[2] < 0.5f) continue;                                         /* not a floor */
		e = e0 = M16(c->surf + s * 8 + 2);
		while (e < c->ne && k < 16)
		{
			u32 ea = c->edge + e * 12, left = M16(ea + 8) == s;
			vtx[k++] = M16(ea + (left ? 0 : 2));
			e = M16(ea + (left ? 4 : 6));
			if (e == e0) break;
		}
		if (e != e0 || k < 4) continue;                                     /* not a closed polygon, or a triangle */
		for (i = 2; i + 1 < k; i++)
		{
			float la[3], lb[3], a[3], b[3], dx, dy, t, cx, cy, d2;
			if (vtx[0] >= c->nv || vtx[i] >= c->nv) break;
			cb_vert(c, vtx[0], la); cb_vert(c, vtx[i], lb);
			xf_point(x, la, a); xf_point(x, lb, b);
			if ((a[2] < b[2] ? a[2] : b[2]) > f[2] + 0.5f || (a[2] > b[2] ? a[2] : b[2]) < f[2] - (g_sf_air ? 400.f : 30.f)) continue;
			d2 = seam_key(f, a, b);
			if (d2 < 0.f) continue;
			if (g_seam_far >= 0 && d2 >= g_seam_d2[g_seam_far]) continue;
			seam_add(a, b, SK_FLAT, d2, piece);
		}
	}
}

/* the pieces: one check per definition per level (0 unchecked, 1 good, 2 no collision, 3 failed) */
#define DEF_MAX 1024
static u8 g_def_state[DEF_MAX];
static float g_def_rad[DEF_MAX];               /* each definition's farthest collision vertex from its origin */
#ifndef HOST
static u32 g_def_bad;
#endif
static u32 g_sbsp_for, g_def_logged;
static void pieces_seams(const float *f)
{
	u32 sb = M32(ADDR_STRUCTURE_BSP), ni, pi, nd, pd, i;
	if (!sb || !ptr_ok(sb)) return;
	ni = M32(sb + 0x140); pi = M32(sb + 0x144); nd = M32(sb + 0x138); pd = M32(sb + 0x13C);
	if (sb != g_sbsp_for)
	{
		s32 at;
		g_sbsp_for = sb; g_def_bad = 0; g_def_logged = 0;
		for (i = 0; i < DEF_MAX; i++) g_def_state[i] = 0;
		at = lput(0, "structure BSP "); at = lhex(at, sb); at = lput(at, ": pieces (instanced geometry) ");
		at = lnum(at, ni); at = lput(at, " of "); at = lnum(at, nd); lput(at, " definitions");
		log_write();
	}
	if (!ni || ni > 8192 || !ptr_ok(pi) || !nd || nd > DEF_MAX || !ptr_ok(pd)) return;
	for (i = 0; i < ni; i++)
	{
		u32 in = pi + i * 0x58, di = M16(in + 0x34), def, k;
		s_xf x;
		s_cb c;
		float r, dx, dy;
		if (di >= nd) continue;
		x.on = 1; x.sc = MF(in);
		for (k = 0; k < 3; k++) { x.fw[k] = MF(in + 0x04 + 4 * k); x.lf[k] = MF(in + 0x10 + 4 * k); x.up[k] = MF(in + 0x1C + 4 * k); x.pos[k] = MF(in + 0x28 + 4 * k); }
		if (!(x.sc > 0.01f && x.sc < 100.f) || !fin(x.pos[0]) || !fin(x.pos[1]) || !fin(x.pos[2])) continue;
		{
			float l1 = dot3(x.fw, x.fw), l2 = dot3(x.lf, x.lf), l3 = dot3(x.up, x.up);
			if (!(l1 > 0.9f && l1 < 1.1f && l2 > 0.9f && l2 < 1.1f && l3 > 0.9f && l3 < 1.1f)) continue;
		}
		def = pd + di * 0xC8;
		cb_load(&c, def + 0x70);
		if (g_def_state[di] == 0)
		{
			const char *why = (!c.ns || !c.ne) ? 0 : cb_check(&c);
			g_def_state[di] = (!c.ns || !c.ne) ? 2 : why ? 3 : 1;
			if (g_def_state[di] == 1)
			{
				/* its reach: the farthest of its own collision vertices from its origin (measured, not trusted) */
				u32 v;
				float m = 0.f, p[3];
				for (v = 0; v < c.nv; v++) { float d; cb_vert(&c, v, p); d = dot3(p, p); if (d > m) m = d; }
				g_def_rad[di] = fsqrt(m);
			}
			if (why)
			{
				g_def_bad++;
				if (g_def_logged < 4)
				{
					s32 at = lput(0, "piece definition "); at = lnum(at, di); at = lput(at, ": collision layout check FAILED (");
					at = lput(at, why); lput(at, ") - skipped");
					log_write(); g_def_logged++;
				}
			}
		}
		if (g_def_state[di] != 1) continue;
		/* cheap reject: the piece's position and its measured reach x its scale */
		r = g_def_rad[di] * x.sc;
		dx = x.pos[0] - f[0]; dy = x.pos[1] - f[1];
		if (g_sf_air ? !air_near(f, x.pos[0], x.pos[1], r) : fsqrt(dx * dx + dy * dy) - r > SEAM_REACH) continue;
		if (x.pos[2] - r > f[2] + 0.5f || x.pos[2] + r < f[2] - (g_sf_air ? 400.f : 30.f)) continue;
		g_pieces_near++;
		seams_from(&c, &x, f, 1);
		if (g_nb_n < NB_MAX) { g_nb_cb[g_nb_n] = c; g_nb_xf[g_nb_n] = x; g_nb_n++; }
	}
}

/* ---- step 3c.3: seams on objects - scenery, machines, controls, crates, vehicles ----
 * object +0x00 tag -> the tag instances g_4e3b44 (16 bytes: group +0x00, data +0x08) -> the object definition,
 * whose +0x38 is its model ('hlmt', group checked), whose +0x0C is its collision model ('coll', group checked):
 * regions +0x1C (12 bytes: permutations +0x04), permutation 0 (bsps +0x04), bsps 0x44 bytes (node index +0x00,
 * collision BSP +0x04). A bsp's node places it: the object's node matrices start node_matrices_offset (+0x116)
 * into it, 0x34 bytes each (scale, forward, left, up, position - as a piece's). Object type +0xAA; its
 * bounding sphere (centre +0x30, radius +0x3C) skips far ones. Each collision BSP is layout-checked once. */
static u32 tag_data(u32 index, u32 group)
{
	u32 tab = M32(ADDR_TAG_INSTANCES), e, d;
	if (!ptr_ok(tab) || index == 0xffffffffu) return 0;
	e = tab + (index & 0xffff) * 16;
	if (group && M32(e) != group) return 0;
	d = M32(e + 8);
	return ptr_ok(d) ? d : 0;
}

#define OBJ_BSP_CACHE 96
static u32 g_ob_addr[OBJ_BSP_CACHE], g_ob_level, g_object_bad, g_ob_logged;
static u8 g_ob_state[OBJ_BSP_CACHE];
static s32 g_ob_n;
static u32 g_ob_desc[24], g_ob_desc_n;          /* objects already described in the log, this level */
static u32 g_ob_census[48], g_ob_census_n, g_ob_pool_logged;   /* objects within 12 units logged once (any type) */

static u32 tag_group(u32 index)
{
	u32 tab = M32(ADDR_TAG_INSTANCES);
	if (!ptr_ok(tab) || index == 0xffffffffu) return 0;
	return M32(tab + (index & 0xffff) * 16);
}

/* a 4-character tag group, as text */
static s32 lgroup(s32 at, u32 g)
{
	char t[5];
	s32 i;
	for (i = 0; i < 4; i++) { char ch = (char)((g >> (24 - 8 * i)) & 0xff); t[i] = (ch >= 32 && ch < 127) ? ch : '?'; }
	t[4] = 0;
	at = lput(at, "'"); at = lput(at, t); return lput(at, "'");
}

/* once per object per level, for one that stops in the chain: where, and what it has instead */
static void obj_stopped(u32 obj, u32 type, const char *where, u32 def, u32 model)
{
	u32 i;
	s32 at;
	if (!o_diag || g_ob_desc_n >= 24) return;                            /* 6h.21: a development line */
	for (i = 0; i < g_ob_desc_n; i++) if (g_ob_desc[i] == obj) return;
	g_ob_desc[g_ob_desc_n++] = obj;
	at = lput(0, "object "); at = lhex(at, obj); at = lput(at, " type "); at = lnum(at, type);
	at = lput(at, ", tag "); at = lhex(at, M32(obj)); at = lput(at, " "); at = lgroup(at, tag_group(M32(obj)));
	at = lput(at, ": stopped at "); at = lput(at, where);
	if (def) { at = lput(at, ", model ref "); at = lhex(at, M32(def + 0x38)); at = lput(at, " "); at = lgroup(at, tag_group(M32(def + 0x38))); }
	if (model)
	{
		at = lput(at, ", collision ref "); at = lhex(at, M32(model + 0x0C)); at = lput(at, " "); at = lgroup(at, tag_group(M32(model + 0x0C)));
		at = lput(at, ", physics model ref "); at = lhex(at, M32(model + 0x24)); at = lput(at, " "); at = lgroup(at, tag_group(M32(model + 0x24)));
	}
	at = lput(at, ", Havok component "); at = lhex(at, M32(obj + 0xB4));
	at = lput(at, ", origin x100"); for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(MF(obj + 0x64 + 4 * i) * 100.f)); }
	log_write();
}

/* once per object per level: what its collision is and where it lands */
static void obj_describe(u32 obj, u32 type, u32 coll, u32 nreg, u32 preg, u32 noff)
{
	u32 i, r, nbsp = 0, nsurf = 0, npoly = 0;
	float w[3] = { 0, 0, 0 };
	s32 at, have = 0;
	if (!o_diag || g_ob_desc_n >= 24) return;                            /* 6h.21: a development line */
	for (i = 0; i < g_ob_desc_n; i++) if (g_ob_desc[i] == obj) return;
	g_ob_desc[g_ob_desc_n++] = obj;
	for (r = 0; r < nreg; r++)
	{
		u32 reg = preg + r * 12, pp = M32(reg + 8), nb, pb, b;
		if (!M32(reg + 4) || !ptr_ok(pp)) continue;
		nb = M32(pp + 4); pb = M32(pp + 8);
		if (!nb || nb > 64 || !ptr_ok(pb)) continue;
		for (b = 0; b < nb; b++)
		{
			s_cb c;
			u32 sidx;
			cb_load(&c, pb + b * 0x44 + 4);
			if (cb_check(&c)) continue;
			nbsp++; nsurf += c.ns;
			for (sidx = 0; sidx < c.ns; sidx++)
			{
				u32 e, e0, k = 0;
				e = e0 = M16(c.surf + sidx * 8 + 2);
				while (e < c.ne && k < 17) { u32 ea = c.edge + e * 12, left = M16(ea + 8) == sidx; k++; e = M16(ea + (left ? 4 : 6)); if (e == e0) break; }
				if (k > 3) npoly++;
			}
			if (!have)
			{
				u32 node = M16(pb + b * 0x44), m = obj + noff + node * 0x34, k2;
				s_xf x;
				float p[3];
				x.on = 1; x.sc = MF(m);
				for (k2 = 0; k2 < 3; k2++) { x.fw[k2] = MF(m + 0x04 + 4 * k2); x.lf[k2] = MF(m + 0x10 + 4 * k2); x.up[k2] = MF(m + 0x1C + 4 * k2); x.pos[k2] = MF(m + 0x28 + 4 * k2); }
				cb_vert(&c, 0, p); xf_point(&x, p, w); have = 1;
			}
		}
	}
	at = lput(0, "object "); at = lhex(at, obj); at = lput(at, " type "); at = lnum(at, type);
	at = lput(at, ", tag "); at = lhex(at, M32(obj)); at = lput(at, ": collision "); at = lhex(at, coll);
	at = lput(at, " regions "); at = lnum(at, nreg); at = lput(at, ", BSPs "); at = lnum(at, nbsp);
	at = lput(at, ", surfaces "); at = lnum(at, nsurf); at = lput(at, " (over 3 corners: "); at = lnum(at, npoly);
	at = lput(at, "), origin x100"); for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(MF(obj + 0x64 + 4 * i) * 100.f)); }
	at = lput(at, ", first vertex x100"); for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(w[i] * 100.f)); }
	log_write();
}

static s32 obj_bsp_ok(u32 bsp)
{
	s32 i;
	s_cb c;
	const char *why;
	for (i = 0; i < g_ob_n; i++) if (g_ob_addr[i] == bsp) return g_ob_state[i] == 1;
	cb_load(&c, bsp);
	why = cb_check(&c);
	if (g_ob_n < OBJ_BSP_CACHE) { g_ob_addr[g_ob_n] = bsp; g_ob_state[g_ob_n] = why ? 2 : 1; g_ob_n++; }
	if (why)
	{
		g_object_bad++;
		if (g_ob_logged < 4)
		{
			s32 at = lput(0, "object collision BSP "); at = lhex(at, bsp); at = lput(at, ": layout check FAILED (");
			at = lput(at, why); lput(at, ") - skipped");
			log_write(); g_ob_logged++;
		}
	}
	return !why;
}

static void objects_seams(const float *f, u32 self)
{
	u32 pool = M32(ADDR_OBJECTS_PTR), data, count, i;
	if (g_ob_level != g_cbm_for) { g_ob_level = g_cbm_for; g_ob_n = 0; g_object_bad = 0; g_ob_logged = 0; g_ob_desc_n = 0; g_ob_census_n = 0; g_ob_pool_logged = 0; }
	if (!ptr_ok(pool)) return;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!g_ob_pool_logged)
	{
		u32 hi = 0, used = 0, j;
		s32 at;
		g_ob_pool_logged = 1;
		if (ptr_ok(data) && count && count <= 4096)
			for (j = 0; j < count; j++) if (M32(data + j * 12) & 0xffff) { used++; hi = j; }
		at = lput(0, "object pool "); at = lhex(at, pool); at = lput(at, ": +0x20 = "); at = lnum(at, count);
		at = lput(at, ", +0x24 = "); at = lnum(at, M32(pool + 0x24)); at = lput(at, ", slots in use "); at = lnum(at, used);
		at = lput(at, ", highest "); lnum(at, hi);
		log_write();
	}
	if (!ptr_ok(data) || !count || count > 4096) return;
	for (i = 0; i < count; i++)
	{
		u32 h = data + i * 12, obj, type, def, model, coll, nreg, preg, ncn, noff, r;
		float c[3], rad, dx, dy;
		if (!(M32(h) & 0xffff)) continue;                          /* a free slot */
		obj = M32(h + 8);
		if (!ptr_ok(obj) || obj == self) continue;
		type = (M32(obj + 0xA8) >> 16) & 0xff;
		/* census: any object whose origin is within 12 units, once per level */
		{
			float ox = MF(obj + 0x64) - f[0], oy = MF(obj + 0x68) - f[1], oz = MF(obj + 0x6C) - f[2];
			if (o_diag && ox * ox + oy * oy + oz * oz < 144.f && g_ob_census_n < 48)   /* 6h.21: a development line */
			{
				u32 j;
				for (j = 0; j < g_ob_census_n && g_ob_census[j] != obj; j++) ;
				if (j == g_ob_census_n)
				{
					s32 at;
					g_ob_census[g_ob_census_n++] = obj;
					at = lput(0, "near: object "); at = lhex(at, obj); at = lput(at, " slot "); at = lnum(at, i);
					at = lput(at, " type "); at = lnum(at, type); at = lput(at, " "); at = lgroup(at, tag_group(M32(obj)));
					at = lput(at, ", origin x100"); for (j = 0; j < 3; j++) { at = lput(at, " "); at = lsnum(at, (s32)(MF(obj + 0x64 + 4 * j) * 100.f)); }
					at = lput(at, ", sphere x100"); for (j = 0; j < 3; j++) { at = lput(at, " "); at = lsnum(at, (s32)(MF(obj + 0x30 + 4 * j) * 100.f)); }
					at = lput(at, " r "); lsnum(at, (s32)(MF(obj + 0x3C) * 100.f));
					log_write();
				}
			}
		}
		if (!(type == 1 || type == 6 || type == 7 || type == 8 || type == 11)) continue;   /* vehicle, scenery, machine, control, crate */
		c[0] = MF(obj + 0x30); c[1] = MF(obj + 0x34); c[2] = MF(obj + 0x38); rad = MF(obj + 0x3C);
		if (!(rad > 0.f && rad < 300.f && fin(c[0]) && fin(c[1]) && fin(c[2])))
		{ c[0] = MF(obj + 0x64); c[1] = MF(obj + 0x68); c[2] = MF(obj + 0x6C); rad = 50.f; }
		dx = c[0] - f[0]; dy = c[1] - f[1];
		{
			s32 out = (g_sf_air ? !air_near(f, c[0], c[1], rad) : fsqrt(dx * dx + dy * dy) - rad > SEAM_REACH) || c[2] - rad > f[2] + 0.5f || c[2] + rad < f[2] - (g_sf_air ? 400.f : 30.f);
			if (out && !g_sf_air && (type == 6 || type == 7 || type == 8))
			{
				/* scenery may not keep its sphere current: its origin within 50 units lets it through to the
				 * per-BSP check below, which uses each BSP's own measured size */
				float ox = MF(obj + 0x64) - f[0], oy = MF(obj + 0x68) - f[1], oz = MF(obj + 0x6C) - f[2];
				if (ox * ox + oy * oy + oz * oz < 2500.f) out = 0;
			}
			if (out) continue;
		}
		def = tag_data(M32(obj), 0);
		model = def ? tag_data(M32(def + 0x38), 0x686C6D74u) : 0;            /* 'hlmt' */
		coll = model ? tag_data(M32(model + 0x0C), 0x636F6C6Cu) : 0;         /* 'coll' */
		if (!def) { if (type != 11 && type != 1) obj_stopped(obj, type, "its definition", 0, 0); continue; }
		if (!model) { if (type != 11 && type != 1) obj_stopped(obj, type, "its model (not 'hlmt')", def, 0); continue; }
		if (!coll) { if (type != 11 && type != 1) obj_stopped(obj, type, "its collision model (none, or not 'coll')", def, model); continue; }
		nreg = M32(coll + 0x1C); preg = M32(coll + 0x20); ncn = M32(coll + 0x2C);
		if (!nreg || nreg > 64 || !ptr_ok(preg)) { obj_stopped(obj, type, "the collision model's regions", def, model); continue; }
		noff = M16(obj + 0x116);
		if (!noff || noff > 0x4000) { obj_stopped(obj, type, "its node matrices offset", def, model); continue; }
		g_objects_near++;
		if (o_diag) obj_describe(obj, type, coll, nreg, preg, noff);      /* 6h.21: a development line */
		for (r = 0; r < nreg; r++)
		{
			u32 reg = preg + r * 12, np = M32(reg + 4), pp = M32(reg + 8), nb, pb, b;
			if (!np || np > 64 || !ptr_ok(pp)) continue;
			nb = M32(pp + 4); pb = M32(pp + 8);                                   /* permutation 0's bsps */
			if (!nb || nb > 64 || !ptr_ok(pb)) continue;
			for (b = 0; b < nb; b++)
			{
				u32 be = pb + b * 0x44, node = M16(be), m, k;
				s_xf x;
				s_cb cc;
				if (node > 63 || (ncn && node >= ncn)) continue;
				if (!obj_bsp_ok(be + 4)) continue;
				m = obj + noff + node * 0x34;
				x.on = 2; x.sc = MF(m);                              /* on = 2: an object (its polygons' triangles count) */
				for (k = 0; k < 3; k++) { x.fw[k] = MF(m + 0x04 + 4 * k); x.lf[k] = MF(m + 0x10 + 4 * k); x.up[k] = MF(m + 0x1C + 4 * k); x.pos[k] = MF(m + 0x28 + 4 * k); }
				if (!(x.sc > 0.01f && x.sc < 100.f) || !fin(x.pos[0]) || !fin(x.pos[1]) || !fin(x.pos[2])) continue;
				{
					float l1 = dot3(x.fw, x.fw), l2 = dot3(x.lf, x.lf), l3 = dot3(x.up, x.up);
					if (!(l1 > 0.9f && l1 < 1.1f && l2 > 0.9f && l2 < 1.1f && l3 > 0.9f && l3 < 1.1f)) continue;
				}
				cb_load(&cc, be + 4);
				{
					/* this BSP's own reach: its measured size around its node */
					float br = bsp_radius(&cc, be + 4) * x.sc, bx = x.pos[0] - f[0], by = x.pos[1] - f[1];
					if (g_sf_air ? !air_near(f, x.pos[0], x.pos[1], br) : fsqrt(bx * bx + by * by) - br > SEAM_REACH) continue;
					if (x.pos[2] - br > f[2] + 0.5f || x.pos[2] + br < f[2] - (g_sf_air ? 400.f : 30.f)) continue;
				}
				/* objects' own seams aren't searched or drawn (crates, racks, doors, vehicles, scenery - ordinary objects);
				 * the object is still a surface near you: it blocks paths and your view, and you can land on it */
				if (g_nb_n < NB_MAX) { g_nb_cb[g_nb_n] = cc; g_nb_xf[g_nb_n] = x; g_nb_n++; g_object_bsps++; }
			}
		}
	}
}

static s32 surf_poly(const s_cb *c, const s_xf *x, u32 s, float (*pv)[3], u32 *ev, float *wn);
static s32 poly_inside(float (*pv)[3], s32 k, const float *n, const float *q);
static s32 g_land_ok;
static float g_land_p[3];
/* ---- Cartographer's blockers (sb_grid_build / sb_blocked_g): the surfaces near you, bucketed in 4-unit columns, so a
 * segment can be tested for anything in its way. Box: 10 units around you, from 31 below to your jump's apex above (in
 * the air, also around your last predicted landing). On the Xbox: at most 384 surfaces of up to 6 corners, 16 x 16
 * columns (fixed tables). A surface blocks a segment that crosses its plane, strictly, inside its polygon. ---- */
#define BLK_MAX 768
/* 6h.20: the lighter limits from the start on both consoles - the original Xbox ran the full ones (768 surfaces, 160 seams)
 * until a search took over 15 ms, at 8-14 ms a search meanwhile (25.9-29.3 fps); the 360 has always run these. (The tests
 * start from the full limits, which they check.) */
#ifdef HOST
static s32 g_blk_cap = BLK_MAX;
#else
static s32 g_blk_cap = 192;                      /* surfaces near you: 192 (BLK_MAX 768 the full limit) */
#endif
static s32 g_search_div = 2;                     /* a search every Nth frame: 2, 3, 4 or 6 by its cost */
#ifdef HOST
static s32 g_slow_hw;
#else
static s32 g_slow_hw = 1;                        /* the lighter limits (6h.20: from the start) */
#endif
/* the search rate from a search's cost (us): over 30 ms every 6th frame, over 15 ms every 4th, over 9 ms every 3rd (6h.20),
 * else every other - and back to a faster rate only once the cost is under 70% of the bar that slowed it (6h.20: the 360
 * flipped 4, 2, 4, 2 around 15 ms, each 2 a heavy stretch) */
s32 search_div_for(u32 per, s32 cur)
{
	s32 want = per > 30000 ? 6 : per > 15000 ? 4 : per > 9000 ? 3 : 2;
	if (want < cur)
	{
		u32 bar = cur >= 6 ? 30000 : cur >= 4 ? 15000 : 9000;
		if (per * 10 >= bar * 7) want = cur;                                 /* not clearly cheaper: stay */
	}
	return want;
}
#define BLK_V 4                                  /* triangles and quads; bigger polygons stored as triangles */
static float g_bk_v[BLK_MAX][BLK_V][3], g_bk_nm[BLK_MAX][3], g_bk_lo[BLK_MAX][3], g_bk_hi[BLK_MAX][3];
static u8 g_bk_k[BLK_MAX];
static u32 g_bk_stamp[BLK_MAX], g_bk_gen;
static s32 g_bk_count, g_gr_nx, g_gr_ny;
static float g_gr_x0, g_gr_y0, g_gr_sx, g_gr_sy;
static u16 g_gr_start[16 * 16 + 1], g_gr_items[BLK_MAX * 8];

static s32 g_bk_full;                           /* the table ran out of room this build (the farthest left out) */
static void blocker_put(float (*pv)[3], s32 k, const float *wn)
{
	s32 i, j;
	if (g_bk_count >= g_blk_cap) { g_bk_full = 1; return; }
	for (j = 0; j < 3; j++) g_bk_lo[g_bk_count][j] = g_bk_hi[g_bk_count][j] = pv[0][j];
	for (i = 0; i < k; i++)
		for (j = 0; j < 3; j++)
		{
			g_bk_v[g_bk_count][i][j] = pv[i][j];
			if (pv[i][j] < g_bk_lo[g_bk_count][j]) g_bk_lo[g_bk_count][j] = pv[i][j];
			if (pv[i][j] > g_bk_hi[g_bk_count][j]) g_bk_hi[g_bk_count][j] = pv[i][j];
		}
	for (j = 0; j < 3; j++) g_bk_nm[g_bk_count][j] = wn[j];
	g_bk_k[g_bk_count] = (u8)k; g_bk_count++;
}
static s32 box_hit(const float *blo, const float *bhi, const float *lo, const float *hi)
{
	return !(bhi[0] < lo[0] || blo[0] > hi[0] || bhi[1] < lo[1] || blo[1] > hi[1] || bhi[2] < lo[2] || blo[2] > hi[2]);
}
/* pass 0: the surfaces in the core box (nearest you); pass 1: the rest of the box */
static void sbx_ready(void);
static void sbx_query(const s_cb *c, const s_xf *x, const float *lo, const float *hi);
static s32 sbx_out(const s_cb *c, const s_xf *x, u32 s, const float *lo, const float *hi);
static void blockers_add(const s_cb *c, const s_xf *x, const float *lo, const float *hi, const float *clo, const float *chi, s32 pass)
{
	u32 s;
	sbx_query(c, x, lo, hi);
	for (s = 0; s < c->ns && g_bk_count < g_blk_cap; s++)
	{
		float pv[16][3], wn[3], blo[3], bhi[3];
		s32 k, i, j, core;
		if (sbx_out(c, x, s, lo, hi)) continue;                              /* by its cached box: not built */
		k = surf_poly(c, x, s, pv, 0, wn);
		if (!k) continue;
		for (j = 0; j < 3; j++) blo[j] = bhi[j] = pv[0][j];
		for (i = 1; i < k; i++) for (j = 0; j < 3; j++) { if (pv[i][j] < blo[j]) blo[j] = pv[i][j]; if (pv[i][j] > bhi[j]) bhi[j] = pv[i][j]; }
		if (!box_hit(blo, bhi, lo, hi)) continue;
		core = box_hit(blo, bhi, clo, chi);
		if (pass == 0 ? !core : core) continue;
		if (k <= BLK_V) blocker_put(pv, k, wn);
		else
			for (i = 1; i + 1 < k; i++)                                       /* a fan of triangles */
			{
				float tri[3][3];
				for (j = 0; j < 3; j++) { tri[0][j] = pv[0][j]; tri[1][j] = pv[i][j]; tri[2][j] = pv[i + 1][j]; }
				blocker_put(tri, 3, wn);
			}
	}
	if (g_bk_count >= g_blk_cap && s < c->ns) g_bk_full = 1;
}

static u32 tsc_lo(void);
static float g_bk_at[3];
static s32 g_bk_air = -1, g_bk_age;
static u32 g_bk_bsp, g_bk_sig;
static void blockers_build_now(const float *f);
/* rebuilt only when needed: moved over 1 unit, took off or landed, a new level, or a second old (its box reaches 10
 * units around you, so 1 unit of movement loses nothing) */
static void blockers_build(const float *f)
{
	float dx = f[0] - g_bk_at[0], dy = f[1] - g_bk_at[1], dz = f[2] - g_bk_at[2];
	float keep = g_slow_hw ? (g_sb_airborne ? 36.f : 9.f) : (g_sb_airborne ? 16.f : 1.f);   /* slow: 3 units (6 in the air) */
	if (g_bk_air == g_sb_airborne && g_bk_bsp == g_cbm_for && g_bk_sig == g_cbm_sig && g_bk_age < 15 && dx * dx + dy * dy + dz * dz < keep)
	{ g_bk_age++; return; }
	g_bk_at[0] = f[0]; g_bk_at[1] = f[1]; g_bk_at[2] = f[2]; g_bk_air = g_sb_airborne; g_bk_bsp = g_cbm_for; g_bk_sig = g_cbm_sig; g_bk_age = 0;
	blockers_build_now(f);
}
static u32 g_cyc_search, g_cyc_block, g_cyc_rate, g_cyc_draw, g_cyc_level, g_cyc_pieces, g_cyc_objects, g_cyc_land;
static void blockers_build_now(const float *f)
{
	if (cb_ready()) sbx_ready();
	float lo[3], hi[3], apex = g_sb_jump * g_sb_jump / (2.f * 4.1712f);
	s32 i, x, y, n, cnt[16 * 16];
	s_xf id;
	{
		float rb = g_slow_hw ? 13.f : 10.f;                                 /* slow: kept over 3 units of movement - 3 wider */
		lo[0] = f[0] - rb; lo[1] = f[1] - rb; lo[2] = f[2] - 31.f;
		hi[0] = f[0] + rb; hi[1] = f[1] + rb; hi[2] = f[2] + apex + 1.f;
	}
	if (g_sb_airborne && g_land_ok)
	{
		if (g_land_p[0] - 10.f < lo[0]) lo[0] = g_land_p[0] - 10.f;
		if (g_land_p[1] - 10.f < lo[1]) lo[1] = g_land_p[1] - 10.f;
		if (g_land_p[0] + 10.f > hi[0]) hi[0] = g_land_p[0] + 10.f;
		if (g_land_p[1] + 10.f > hi[1]) hi[1] = g_land_p[1] + 10.f;
		if (g_land_p[2] - 31.f < lo[2]) lo[2] = g_land_p[2] - 31.f;
	}
	g_bk_count = 0; g_bk_full = 0; id.on = 0;
	{
		/* nearest first: the core (6 around you, 10 below to your jump's top) from every source, then the rest */
		float clo[3] = { f[0] - 6.f, f[1] - 6.f, f[2] - 10.f }, chi[3] = { f[0] + 6.f, f[1] + 6.f, f[2] + apex + 1.f };
		s32 pass;
		for (pass = 0; pass < 2; pass++)
		{
			if (cb_ready()) blockers_add(&g_cbm, &id, lo, hi, clo, chi, pass);
			for (i = 0; i < g_nb_n; i++) blockers_add(&g_nb_cb[i], &g_nb_xf[i], lo, hi, clo, chi, pass);
		}
	}
	/* the columns */
	g_gr_x0 = lo[0]; g_gr_y0 = lo[1];
	g_gr_nx = (s32)((hi[0] - lo[0]) / 4.f) + 1; if (g_gr_nx > 16) g_gr_nx = 16;
	g_gr_ny = (s32)((hi[1] - lo[1]) / 4.f) + 1; if (g_gr_ny > 16) g_gr_ny = 16;
	g_gr_sx = (float)g_gr_nx / (hi[0] - lo[0] > 1e-3f ? hi[0] - lo[0] : 1e-3f);
	g_gr_sy = (float)g_gr_ny / (hi[1] - lo[1] > 1e-3f ? hi[1] - lo[1] : 1e-3f);
	for (i = 0; i < g_gr_nx * g_gr_ny; i++) cnt[i] = 0;
	for (n = 0; n < 2; n++)                                              /* count, then fill */
	{
		if (n == 1)
		{
			s32 sum = 0;
			for (i = 0; i < g_gr_nx * g_gr_ny; i++) { g_gr_start[i] = (u16)sum; sum += cnt[i]; cnt[i] = 0; }
			g_gr_start[g_gr_nx * g_gr_ny] = (u16)sum;
		}
		for (i = 0; i < g_bk_count; i++)
		{
			s32 x0 = (s32)((g_bk_lo[i][0] - g_gr_x0) * g_gr_sx), x1 = (s32)((g_bk_hi[i][0] - g_gr_x0) * g_gr_sx);
			s32 y0 = (s32)((g_bk_lo[i][1] - g_gr_y0) * g_gr_sy), y1 = (s32)((g_bk_hi[i][1] - g_gr_y0) * g_gr_sy);
			if (x0 < 0) x0 = 0;
			if (y0 < 0) y0 = 0;
			if (x1 > g_gr_nx - 1) x1 = g_gr_nx - 1;
			if (y1 > g_gr_ny - 1) y1 = g_gr_ny - 1;
			for (y = y0; y <= y1; y++) for (x = x0; x <= x1; x++)
			{
				s32 c = y * g_gr_nx + x;
				if (n == 1) { s32 at = g_gr_start[c] + cnt[c]; if (at < BLK_MAX * 8) g_gr_items[at] = (u16)i; }
				cnt[c]++;
			}
		}
	}
	if (g_gr_start[g_gr_nx * g_gr_ny] > BLK_MAX * 8) g_bk_count = 0;   /* too many cell entries: no blockers this time */
}

/* sb_blocked_g: does anything cross the segment p0-p1? */
static s32 blocked(const float *p0, const float *p1)
{
	float lo[3], hi[3];
	s32 x, y, x0, x1, y0, y1, j;
	if (!g_bk_count) return 0;
	if (++g_bk_gen == 0) { for (j = 0; j < BLK_MAX; j++) g_bk_stamp[j] = 0; g_bk_gen = 1; }
	for (j = 0; j < 3; j++) { lo[j] = p0[j] < p1[j] ? p0[j] : p1[j]; hi[j] = p0[j] > p1[j] ? p0[j] : p1[j]; }
	x0 = (s32)((lo[0] - g_gr_x0) * g_gr_sx); x1 = (s32)((hi[0] - g_gr_x0) * g_gr_sx);
	y0 = (s32)((lo[1] - g_gr_y0) * g_gr_sy); y1 = (s32)((hi[1] - g_gr_y0) * g_gr_sy);
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > g_gr_nx - 1) x1 = g_gr_nx - 1;
	if (y1 > g_gr_ny - 1) y1 = g_gr_ny - 1;
	for (y = y0; y <= y1; y++)
		for (x = x0; x <= x1; x++)
		{
			s32 c = y * g_gr_nx + x, t;
			for (t = g_gr_start[c]; t < g_gr_start[c + 1]; t++)
			{
				s32 s = g_gr_items[t];
				float *v0 = g_bk_v[s][0], *n = g_bk_nm[s], d0, d1, q[3];
				if (g_bk_stamp[s] == g_bk_gen) continue;
				g_bk_stamp[s] = g_bk_gen;
				if (g_bk_hi[s][0] < lo[0] || g_bk_lo[s][0] > hi[0] || g_bk_hi[s][1] < lo[1] || g_bk_lo[s][1] > hi[1] || g_bk_hi[s][2] < lo[2] || g_bk_lo[s][2] > hi[2]) continue;
				d0 = (p0[0] - v0[0]) * n[0] + (p0[1] - v0[1]) * n[1] + (p0[2] - v0[2]) * n[2];
				d1 = (p1[0] - v0[0]) * n[0] + (p1[1] - v0[1]) * n[1] + (p1[2] - v0[2]) * n[2];
				if ((d0 > 0.f && d1 > 0.f) || (d0 < 0.f && d1 < 0.f) || d0 == d1) continue;
				for (j = 0; j < 3; j++) q[j] = p0[j] + (p1[j] - p0[j]) * (d0 / (d0 - d1));
				if (poly_inside(g_bk_v[s], g_bk_k[s], n, q)) return 1;
			}
		}
	return 0;
}
/* the nearest crossing along p0-p1 (0..1), or 2 when nothing is in the way */
static float ray_first(const float *p0, const float *p1)
{
	float lo[3], hi[3], best = 2.f;
	s32 x, y, x0, x1, y0, y1, j;
	if (!g_bk_count) return best;
	if (++g_bk_gen == 0) { for (j = 0; j < BLK_MAX; j++) g_bk_stamp[j] = 0; g_bk_gen = 1; }
	for (j = 0; j < 3; j++) { lo[j] = p0[j] < p1[j] ? p0[j] : p1[j]; hi[j] = p0[j] > p1[j] ? p0[j] : p1[j]; }
	x0 = (s32)((lo[0] - g_gr_x0) * g_gr_sx); x1 = (s32)((hi[0] - g_gr_x0) * g_gr_sx);
	y0 = (s32)((lo[1] - g_gr_y0) * g_gr_sy); y1 = (s32)((hi[1] - g_gr_y0) * g_gr_sy);
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > g_gr_nx - 1) x1 = g_gr_nx - 1;
	if (y1 > g_gr_ny - 1) y1 = g_gr_ny - 1;
	for (y = y0; y <= y1; y++)
		for (x = x0; x <= x1; x++)
		{
			s32 c = y * g_gr_nx + x, t;
			for (t = g_gr_start[c]; t < g_gr_start[c + 1]; t++)
			{
				s32 s = g_gr_items[t];
				float *v0 = g_bk_v[s][0], *n = g_bk_nm[s], d0, d1, q[3], u;
				if (g_bk_stamp[s] == g_bk_gen) continue;
				g_bk_stamp[s] = g_bk_gen;
				if (g_bk_hi[s][0] < lo[0] || g_bk_lo[s][0] > hi[0] || g_bk_hi[s][1] < lo[1] || g_bk_lo[s][1] > hi[1] || g_bk_hi[s][2] < lo[2] || g_bk_lo[s][2] > hi[2]) continue;
				d0 = (p0[0] - v0[0]) * n[0] + (p0[1] - v0[1]) * n[1] + (p0[2] - v0[2]) * n[2];
				d1 = (p1[0] - v0[0]) * n[0] + (p1[1] - v0[1]) * n[1] + (p1[2] - v0[2]) * n[2];
				if ((d0 > 0.f && d1 > 0.f) || (d0 < 0.f && d1 < 0.f) || d0 == d1) continue;
				u = d0 / (d0 - d1);
				if (u >= best) continue;
				for (j = 0; j < 3; j++) q[j] = p0[j] + (p1[j] - p0[j]) * u;
				if (poly_inside(g_bk_v[s], g_bk_k[s], n, q)) best = u;
			}
		}
	return best;
}

/* ---- the level's collision BSP walked along a segment (6h.9, the third person camera's aim): the fraction of the way to
 * the first point inside solid, 2 if none. Its 3D nodes (+0x00 count, +0x04 pointer) are 8 bytes: a plane (s16, into
 * +0x08 / +0x0C: normal, distance) then the back and front children, 24-bit and signed - -1 solid, any other with the top
 * bit a leaf (open), else a node; a point with n.p - d >= 0 goes to the front - as the game's leaf search (v1.0 0x14a280,
 * from node 0) walks them. Near side first, so the first solid met is the nearest. Every index checked; a bad one, a deep
 * tree or too many steps: 2. ---- */
static s32 bsp_child(u32 a)
{
	u32 v = M8(a) | (M8(a + 1) << 8) | (M8(a + 2) << 16);
	return (s32)(v & 0x800000 ? v | 0xFF000000u : v);
}
static float bsp_ray(const float *p0, const float *p1)
{
	struct { s32 c; float t0, t1; } st[64];
	u32 b, nn, nodes, np, planes;
	s32 sp = 0, steps = 0, j;
	if (!cb_ready() || g_cb_state != 1) return 2.f;
	b = M32(ADDR_COLLISION_BSP);
	nn = M32(b); nodes = M32(b + 4); np = M32(b + 8); planes = M32(b + 0xC);
	if (!nn || nn > 0x7fffff || !ptr_ok(nodes) || !np || np > 0xffff || !ptr_ok(planes)) return 2.f;
	st[0].c = 0; st[0].t0 = 0.f; st[0].t1 = 1.f; sp = 1;
	while (sp > 0)
	{
		s32 c;
		float t0, t1;
		sp--; c = st[sp].c; t0 = st[sp].t0; t1 = st[sp].t1;
		for (;;)
		{
			u32 node, pi, pa;
			float s0 = 0.f, s1 = 0.f;
			s32 back, front, f0, f1;
			if (c == -1) return t0;                                          /* solid: the nearest */
			if (c < 0) break;                                                /* a leaf: open */
			if ((u32)c >= nn || ++steps > 4096) return 2.f;
			node = nodes + (u32)c * 8;
			pi = (u32)(s32)(s16)M16(node);
			if (pi >= np) return 2.f;
			pa = planes + pi * 16;
			for (j = 0; j < 3; j++)
			{
				float n = MF(pa + 4 * (u32)j), d = p1[j] - p0[j];
				s0 += n * (p0[j] + d * t0); s1 += n * (p0[j] + d * t1);
			}
			s0 -= MF(pa + 12); s1 -= MF(pa + 12);
			if (!(s0 > -1e6f && s0 < 1e6f && s1 > -1e6f && s1 < 1e6f)) return 2.f;
			back = bsp_child(node + 2); front = bsp_child(node + 5);
			f0 = s0 >= 0.f; f1 = s1 >= 0.f;
			if (f0 == f1) { c = f0 ? front : back; continue; }
			{
				float tm = t0 + (t1 - t0) * (s0 / (s0 - s1));
				if (sp >= 64) return 2.f;
				st[sp].c = f1 ? front : back; st[sp].t0 = tm; st[sp].t1 = t1; sp++;   /* the far side, after */
				c = f0 ? front : back; t1 = tm;                                          /* the near side first */
			}
		}
	}
	return 2.f;
}
#ifdef HOST
float host_bsp_ray(const float *p0, const float *p1) { return bsp_ray(p0, p1); }
#endif
#ifdef HOST
s32 host_rated_seams(void);
#endif

static void seams_find(u32 object)
{
	float f[3] = { MF(object + 0x64), MF(object + 0x68), MF(object + 0x6C) };
	s_xf id;
	id.on = 0;
	g_sf_air = g_sb_airborne;
	g_nb_n = 0;
	g_sf_v[0] = MF(object + 0x88); g_sf_v[1] = MF(object + 0x8C); g_sf_v[2] = MF(object + 0x90);
	g_seam_n = 0; g_seam_far = -1; g_piece_seams = 0; g_pieces_near = 0; g_object_seams = 0; g_objects_near = 0; g_object_bsps = 0;
	g_seam_kinds[0] = g_seam_kinds[1] = g_seam_kinds[2] = g_seam_kinds[3] = 0;
	{
		u32 t0 = tsc_lo(), t1, t2;
		{
			s32 q;
			for (q = 0; q < 6; q++) g_lsd[q] = 0;
			sb_stage("the level's collision");
			if (cb_ready()) level_seams(f);
			g_lsd[5] = (u32)g_seam_n;
			lsd_report(f);
		}
		t1 = tsc_lo(); g_cyc_level += t1 - t0;
		sb_stage("the pieces");
		pieces_seams(f);
		t2 = tsc_lo(); g_cyc_pieces += t2 - t1;
		sb_stage("the objects");
		objects_seams(f, object);
		g_cyc_objects += tsc_lo() - t2;
	}
	{
		u32 t0 = tsc_lo();
		blockers_build(f);
		g_cyc_block += tsc_lo() - t0;
	}
}

/* ---- timing: the CPU's cycle counter (733 MHz) - the seam work's cost and the game's frame rate ---- */
#ifdef HOST
static u32 g_host_tsc;
static u32 tsc_lo(void) { g_host_tsc += 1000; return g_host_tsc; }
static s64 tsc64(void) { return (s64)tsc_lo(); }
#else
static u32 tsc_lo(void) { u32 lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); return lo; }
static s64 tsc64(void) { u32 lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); return (s64)(((u64)hi << 32) | lo); }
#endif
static u32 g_seam_cyc_sum, g_seam_cyc_max, g_seam_cyc_n, g_seam_searches;

static s64 g_fps_t0;
static u32 g_fps_f0;

/* ---- step 3d: Cartographer's seam rating (the core) ----
 * For each seam below you (0.3 to 30 units; 400 in the air), at its nearest half-unit piece in reach: the fall onto it,
 * tick by tick as the game does it (Cartographer's sb_fall: gravity 4.1712 u/s^2; the floor's contact helper appears once
 * the height h <= -0.025 + 1.2 x the next step; depth = 0.86 x step - 0.85 x h + 0.01), on time and one tick late.
 * On the ground: walking off (0 up) and jumping (your take-off speed). Green "likely": arriving at 5.445 u/s or more
 * (the sweep speed, 0.1815 u/tick at 30) with depth 0.17 or more, on time. Yellow "lottery": only late. Red: neither,
 * or a lottery whose late contact would start 0.19 or more below the floor (a debounce: you fall through). */
#define SB_GRAVITY 4.1712f
#define SB_DT (1.f / 30.f)
#define SB_NEED (0.1815f * 30.f)
#define SB_DEPTH_MIN 0.17f
#define SB_DEBOUNCE 0.19f
/* what Cartographer keeps of a rating (s_sb_rated): the way (0 walk off, 1 jump), the arrival speeds and depths */
typedef struct { s32 q, opt, debounce, air; float walk, jump, dw, dj, lw, lj, late_start, down; const char *why; } s_rd;
static s8 g_seam_q[SEAM_MAX];                   /* -1 unrated (not below you), 0 red, 1 yellow, 2 green */
static u8 g_seam_vis[SEAM_MAX];                 /* drawn: rated, or unrated but reachable (seams_vis) */
static u8 g_seam_los[SEAM_MAX], g_seam_np[SEAM_MAX];   /* its sections in view from the camera (bits), how many */
static void seam_line(s32 i);
/* the seams as last finished (all three steps): what's drawn - a search spread over frames never shows half-updated */
static float d_seam_ab[SEAM_MAX][6];
static u8 d_seam_kind[SEAM_MAX], d_seam_vis[SEAM_MAX], d_seam_los[SEAM_MAX], d_seam_np[SEAM_MAX];
static s8 d_seam_q[SEAM_MAX];
static s32 d_seam_n;
static s32 d_land_ok, d_ld_q, d_ld_seam, d_ld_arrow;
static float d_land_p[3], d_ld_sa[3], d_ld_sb[3], d_ld_dir[3], d_ld_upax[3], d_ld_up;
#ifdef HOST
static s32 g_host_step_div;                      /* the tests' slow-console schedule (0: all three steps a frame) */
#endif
static s_rd g_seam_rd[SEAM_MAX];
static float g_seam_near[SEAM_MAX][3], g_seam_drop[SEAM_MAX];
static u32 g_seam_open[SEAM_MAX];
static u8 g_seam_pieces[SEAM_MAX];
static s32 g_seam_gt[SEAM_MAX], g_seam_gh[SEAM_MAX];
static float g_fall_late_start;

static float sb_depth(float step, float h) { return 0.86f * step - 0.85f * h + 0.01f; }

static s32 sb_fall(float z, float vz, float zf, float *d_on, float *d_late, float *v_on, float *v_late)
{
	s32 i;
	g_fall_late_start = 1.f;
	for (i = 0; i < 900; i++)
	{
		float vz1 = vz - SB_GRAVITY * SB_DT, step1 = -vz1 * SB_DT, h = z - zf;
		if (h <= -0.025f + 1.2f * step1)
		{
			float vz2 = vz1 - SB_GRAVITY * SB_DT;
			g_fall_late_start = h - step1;
			*d_on = sb_depth(step1, h);
			*d_late = sb_depth(-vz2 * SB_DT, h - step1);
			*v_on = -vz1; *v_late = -vz2;
			return 1;
		}
		vz = vz1;
		z += vz * SB_DT;
	}
	return 0;
}

static s32 sb_rate_d(float feet_z, float pt_z, float vh, float vz, s32 air, float jump, s32 walk_ok, s32 jump_ok, s_rd *r);
#ifdef HOST
s32 host_rate(float feet_z, float pt_z, float vh, float vz, s32 air, float jump, s32 walk_ok, s32 jump_ok)
#else
static s32 sb_rate(float feet_z, float pt_z, float vh, float vz, s32 air, float jump, s32 walk_ok, s32 jump_ok)
#endif
{
	s_rd r;
	return sb_rate_d(feet_z, pt_z, vh, vz, air, jump, walk_ok, jump_ok, &r);
}

static s32 sb_rate_d(float feet_z, float pt_z, float vh, float vz, s32 air, float jump, s32 walk_ok, s32 jump_ok, s_rd *rd)
{
	float d, l, v, vl;
	s32 q = 0;
	float late_start = 1.f;
	rd->opt = 0; rd->debounce = 0; rd->air = air; rd->walk = rd->jump = rd->dw = rd->dj = rd->lw = rd->lj = rd->down = 0.f; rd->why = 0;
	if (air)
	{
		s32 fa = sb_fall(feet_z, vz, pt_z, &d, &l, &v, &vl);
		float s_on = fsqrt(vh * vh + v * v), s_late = fsqrt(vh * vh + vl * vl);
		late_start = g_fall_late_start;
		if (fa && s_on >= SB_NEED && d >= SB_DEPTH_MIN) q = 2;
		else if (fa && s_late >= SB_NEED && l >= SB_DEPTH_MIN) q = 1;
		rd->dw = d; rd->lw = l; rd->walk = q == 2 ? s_on : s_late;
		rd->down = q == 2 ? v : q == 1 ? vl : 0.f;
	}
	else
	{
		float dw, lw, vw, vlw, dj, lj, vj, vlj, lsw, lsj;
		s32 fw = sb_fall(feet_z, 0.f, pt_z, &dw, &lw, &vw, &vlw);
		lsw = g_fall_late_start;
		{
			s32 fj = sb_fall(feet_z, jump, pt_z, &dj, &lj, &vj, &vlj);
			float sw_on = fsqrt(vh * vh + vw * vw), sw_late = fsqrt(vh * vh + vlw * vlw);
			float sj_on = fsqrt(vh * vh + vj * vj), sj_late = fsqrt(vh * vh + vlj * vlj);
			lsj = g_fall_late_start;
			s32 w_on, j_on;
			fw = fw && walk_ok; fj = fj && jump_ok;                                       /* the ways there that are open */
			w_on = fw && sw_on >= SB_NEED && dw >= SB_DEPTH_MIN; j_on = fj && sj_on >= SB_NEED && dj >= SB_DEPTH_MIN;
			if (w_on) { q = 2; rd->down = vw; }                                           /* walking off: likely */
			else if (j_on) { q = 2; rd->opt = 1; rd->down = vj; }                         /* jumping: likely */
			else if (fw && sw_late >= SB_NEED && lw >= SB_DEPTH_MIN) { q = 1; late_start = lsw; rd->down = vlw; }   /* lottery */
			else if (fj && sj_late >= SB_NEED && lj >= SB_DEPTH_MIN) { q = 1; late_start = lsj; rd->opt = 1; rd->down = vlj; }
			rd->walk = w_on ? sw_on : sw_late; rd->jump = j_on ? sj_on : sj_late;
			rd->dw = dw; rd->dj = dj; rd->lw = lw; rd->lj = lj;
			if (q != 1) late_start = rd->opt == 1 ? lsj : lsw;
		}
	}
	if (q == 1 && late_start <= -SB_DEBOUNCE) { q = 0; rd->debounce = 1; }                  /* a debounce */
	rd->late_start = late_start; rd->q = q;
	return q;
}
#ifdef HOST
#define sb_rate host_rate
s32 host_rated_seams(void) { s32 i, n = 0; for (i = 0; i < g_seam_n; i++) if (g_seam_q[i] >= 0) n++; return n; }
#endif

static s32 shown_find(const float *a, const float *b);
static float sb_tilt(const float *vel, const float *t);
/* in the air: the colour is the verdict you'd get landing ON it - the reticle's own checks (all but "off the seam"): charged;
 * the drift across it (too fast from 13 deg, too little under 0.6 - flat seams only: a bend's own push can make up for
 * too little; edge from 8, or under 1 on a flat seam); speed or depth at the threshold. (5e.18 rated only the fall: green
 * seams the reticle then called amber, yellow or red.) */
static void rate_as_verdict(s32 i)
{
	s_rd *rd = &g_seam_rd[i];
	if (g_sb_charged != 1) { g_seam_q[i] = 0; rd->why = "NOT CHARGED"; }
	else if (g_seam_q[i] == 2)
	{
		float sd[3] = { g_seam_ab[i][3] - g_seam_ab[i][0], g_seam_ab[i][4] - g_seam_ab[i][1], g_seam_ab[i][5] - g_seam_ab[i][2] };
		float sl = fsqrt(dot3(sd, sd)), vc[3] = { g_sf_v[0], g_sf_v[1], -rd->down }, deg, d_on = rd->walk - SB_NEED;
		s32 flat = g_seam_kind[i] == SK_FLAT;
		if (sl > 1e-6f)
		{
			sd[0] /= sl; sd[1] /= sl; sd[2] /= sl;
			deg = sb_tilt(vc, sd) * 57.29578f;
			if (deg >= 13.f) { g_seam_q[i] = 1; rd->why = "ANGLE: drifting too fast across"; }
			else if (flat && deg < 0.6f) { g_seam_q[i] = 1; rd->why = "ANGLE: too little drift across"; }
			else if (deg >= 8.f || (flat && deg < 1.f) || (d_on < 0.f ? -d_on : d_on) < 0.05f || rd->dw < 0.21f)
			{ g_seam_q[i] = 3; rd->why = "EDGE (about 1 in 2)"; g_seam_gt[i] = g_seam_gh[i] = 0; }
		}
	}
}
#ifdef HOST
/* the rule alone: a seam a-b of a kind, rated likely by its fall (down speed, contact speed, depth), with your drift and
 * charge - its colour and why */
s32 host_rate_as_verdict(const float *a, const float *b, s32 kind, float down, float walk, float dw, const float *v, s32 charged, const char **why)
{
	s32 k, ch = g_sb_charged;
	for (k = 0; k < 3; k++) { g_seam_ab[0][k] = a[k]; g_seam_ab[0][3 + k] = b[k]; g_sf_v[k] = v[k]; }
	g_seam_kind[0] = (u8)kind; g_seam_q[0] = 2; g_seam_rd[0].down = down; g_seam_rd[0].walk = walk; g_seam_rd[0].dw = dw; g_seam_rd[0].why = 0;
	g_sb_charged = charged; rate_as_verdict(0); g_sb_charged = ch;
	*why = g_seam_rd[0].why;
	return g_seam_q[0];
}
#endif
static void seams_rate(u32 object)
{
	float f[3] = { MF(object + 0x64), MF(object + 0x68), MF(object + 0x6C) };
	float vx = MF(object + 0x88), vy = MF(object + 0x8C), vz = MF(object + 0x90), vh = fsqrt(vx * vx + vy * vy);
	float max_drop = g_sb_airborne ? 400.f : 30.f;
	s32 i, cnt = 0;
	u8 rate_it[SEAM_MAX];
	/* Cartographer rates at most the 96 nearest candidates: the rest stay unrated (dimmed) */
	for (i = 0; i < g_seam_n; i++) rate_it[i] = 0;
	while (cnt < 96 && cnt < g_seam_n)
	{
		s32 best = -1;
		for (i = 0; i < g_seam_n; i++) if (!rate_it[i] && (best < 0 || g_seam_d2[i] < g_seam_d2[best])) best = i;
		if (best < 0) break;
		rate_it[best] = 1; cnt++;
	}
	for (i = 0; i < g_seam_n; i++)
	{
		float *a = &g_seam_ab[i][0], *b = &g_seam_ab[i][3], dx = b[0] - a[0], dy = b[1] - a[1], dz = b[2] - a[2];
		float len = fsqrt(dx * dx + dy * dy + dz * dz), best = 1e9f, pz = 0.f;
		s32 pieces = (s32)(len / 0.5f), k;
		g_seam_q[i] = -1;
		if (!rate_it[i]) continue;
		if ((a[2] < b[2] ? a[2] : b[2]) > f[2] - 0.3f) continue;                  /* not below you: not rated */
		if (pieces < 1) pieces = 1; else if (pieces > 24) pieces = 24;
		s32 near_walk = 0, near_jump = 0, was = shown_find(a, b) >= 0;
		float reach = was ? 9.5f : RATE_REACH, rr_extra = was ? 1.5f : 0.f;
		u32 open = 0;
		float apex = g_sb_jump * g_sb_jump / (2.f * 4.1712f), nx = 0.f, ny = 0.f;
		for (k = 0; k < pieces; k++)
		{
			float t = ((float)k + 0.5f) / (float)pieces, mx = a[0] + dx * t - f[0], my = a[1] + dy * t - f[1], mz = a[2] + dz * t;
			float drop = f[2] - mz, d2 = mx * mx + my * my, lx, ly, lt, key = 1e9f;
			float mid[3] = { a[0] + dx * t, a[1] + dy * t, mz };
			s32 walk_ok, jump_ok;
			if (drop < 0.3f || drop > max_drop) continue;
			if (g_sb_airborne && land_at(f, mz, &lx, &ly, &lt))
			{
				float r = AIR_REACH * lt + 1.f, ex = a[0] + dx * t - lx, ey = a[1] + dy * t - ly, l2 = ex * ex + ey * ey;
				if (r > AIR_REACH_MAX) r = AIR_REACH_MAX;
				r += rr_extra;
				if (l2 <= r * r) key = l2;
			}
			if (key >= 1e8f)
			{
				if (d2 > reach * reach) continue;
				key = d2 + (g_sb_airborne ? 1600.f : 0.f);
			}
			if (key >= best) continue;
			/* covered? (something between it and your height) - then the ways there (Cartographer) */
			{
				float p0[3] = { mid[0], mid[1], mid[2] + 0.05f }, p1[3] = { mid[0], mid[1], f[2] + 0.05f };
				float w0[3] = { f[0], f[1], f[2] + 0.6f }, w1[3] = { mid[0], mid[1], f[2] + 0.6f };
				float j1[3] = { mid[0], mid[1], f[2] + apex + 0.05f }, j2[3] = { f[0], f[1], f[2] + apex + 0.3f }, j3[3] = { mid[0], mid[1], f[2] + apex + 0.3f };
				if (blocked(p0, p1)) continue;                                    /* covered */
				open |= 1u << k;
				walk_ok = g_sb_airborne ? 1 : !blocked(w0, w1);                   /* straight there at your height (on the ground only) */
				jump_ok = !g_sb_airborne && !blocked(p0, j1) && !blocked(j2, j3); /* over the top of a jump */
				if (!walk_ok && !jump_ok) continue;
			}
			best = key; pz = mz; near_walk = walk_ok; near_jump = jump_ok; nx = mid[0]; ny = mid[1];
		}
		g_seam_open[i] = open; g_seam_pieces[i] = (u8)pieces;
		if (best >= 1e8f) continue;
		g_seam_q[i] = (s8)sb_rate_d(f[2], pz, vh, vz, g_sb_airborne, g_sb_jump, near_walk, near_jump, &g_seam_rd[i]);
		g_seam_near[i][0] = nx; g_seam_near[i][1] = ny; g_seam_near[i][2] = pz; g_seam_drop[i] = f[2] - pz;
		g_seam_gt[i] = g_seam_gh[i] = 0;
		if (g_seam_q[i] == 1 && g_seam_piece[i] != 2)
		{
			s32 gt, gh;
			if (ghost_get(&g_seam_ab[i][0], &g_seam_ab[i][3], &gt, &gh)) { g_seam_q[i] = 3; g_seam_gt[i] = gt; g_seam_gh[i] = gh; }   /* a ghost seam: amber */
		}
		/* in the air: the colour is the verdict you'd get landing ON it - the reticle's own checks (all but "off the seam"):
		 * charged; the drift across it (too fast from 13 deg, too little under 0.6 - flat seams only: a bend's own push can
		 * make up for too little; edge from 8, or under 1 on a flat seam); speed or depth at the threshold. (5e.18 rated only
		 * the fall: green seams the reticle then called amber, yellow or red.) */
		if (g_sb_airborne) rate_as_verdict(i);
	}
}

/* ---- step 3d.3 (ported): Cartographer's landing - sb_predict_landing and sb_draw_landing ----
 * The path: the feet's, per tick - air control steps the drift toward its target (1.0485 u/s^2), then gravity, then
 * position - for 8 s at most. It lands at its first crossing, from the open side, of a surface you can land on
 * (n.z >= 0.2): inside the surface's polygon. Then the landing's depth: the floor's helper appears (on time) on the
 * first tick whose feet are within -0.025 + 1.2 x the next step of the landing surface (vertically); contact follows,
 * on time or a tick late. The seam: the landing surface's own edge nearest the landing point, where a floor seam
 * meets it (Cartographer's sb_nearest_seam; for objects, the fan diagonals too). The verdict: sb_draw_landing's. */
#define PATH_MAX 241
static float g_pts[PATH_MAX][3], g_vels[PATH_MAX][3];
static s32 g_pts_n, g_arc_n, g_land_ok, g_land_seg;
static float g_land_p[3], g_land_frac = 2.f, g_land_n[3], g_land_p0[3], g_land_v[3];
static s_cb g_land_cb;
static s_xf g_land_xf;
static u32 g_land_s;
static s32 g_land_obj;
/* the landing's verdict */
static s32 g_ld_q = -1, g_ld_seam, g_ld_have_depth;
static float g_ld_sa[3], g_ld_sb[3], g_ld_off, g_ld_bend, g_ld_depth, g_ld_late, g_ld_speed, g_ld_tilt, g_ld_down;
static const char *g_ld_why = "";
#ifdef HOST
s32 host_landing(float *p) { if (g_land_ok) { p[0] = g_land_p[0]; p[1] = g_land_p[1]; p[2] = g_land_p[2]; } return g_land_ok; }
s32 host_landing_q(void) { return g_ld_q; }
#endif

static void path_compute(u32 object)
{
	float p[3] = { MF(object + 0x64), MF(object + 0x68), MF(object + 0x6C) }, v[3] = { MF(object + 0x88), MF(object + 0x8C), MF(object + 0x90) };
	const float dt = 1.f / 30.f, step = 1.0485f * dt;
	s32 k;
	for (k = 0; k < 3; k++) { g_pts[0][k] = p[k]; g_vels[0][k] = v[k]; }
	g_pts_n = 1;
	while (g_pts_n < PATH_MAX)
	{
		float tx = g_tgt[0] - v[0], ty = g_tgt[1] - v[1], gap = fsqrt(tx * tx + ty * ty);
		if (gap <= step) { v[0] = g_tgt[0]; v[1] = g_tgt[1]; }
		else { v[0] += tx / gap * step; v[1] += ty / gap * step; }
		v[2] -= 4.1712f * dt;
		for (k = 0; k < 3; k++) { p[k] += v[k] * dt; g_pts[g_pts_n][k] = p[k]; g_vels[g_pts_n][k] = v[k]; }
		g_pts_n++;
		if (g_lg_ok && g_lg_for == g_cbm_for && g_lg_sig == g_cbm_sig && v[2] < 0.f && p[2] < g_lg_zmin - 1.f) break;   /* below the whole level (this level's) */
	}
}

/* is q (on the plane with normal n) inside the polygon? (Cartographer's sb_inside) */
static s32 poly_inside(float (*pv)[3], s32 k, const float *n, const float *q)
{
	s32 i, pos = 0, neg = 0;
	for (i = 0; i < k; i++)
	{
		float *a = pv[i], *b = pv[(i + 1) % k], e[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, w[3] = { q[0] - a[0], q[1] - a[1], q[2] - a[2] };
		float c[3] = { e[1] * w[2] - e[2] * w[1], e[2] * w[0] - e[0] * w[2], e[0] * w[1] - e[1] * w[0] }, side = dot3(c, n);
		if (side > 1e-5f) pos++; else if (side < -1e-5f) neg++;
	}
	return pos == 0 || neg == 0;
}

/* a surface's polygon in the world (its edge loop) and its unit world normal; 0 if not a closed polygon */
static s32 surf_poly(const s_cb *c, const s_xf *x, u32 s, float (*pv)[3], u32 *ev, float *wn)
{
	float n[3], l;
	u32 e, e0, k = 0, j;
	if (!cb_normal(c, s, n)) return 0;
	xf_dir(x, n, wn);
	l = fsqrt(dot3(wn, wn));
	for (j = 0; j < 3; j++) wn[j] /= l;
	e = e0 = M16(c->surf + s * 8 + 2);
	while (e < c->ne && k < 16)
	{
		u32 ea = c->edge + e * 12, left = M16(ea + 8) == s, v = M16(ea + (left ? 0 : 2));
		float lp[3];
		if (v >= c->nv) return 0;
		cb_vert(c, v, lp); xf_point(x, lp, pv[k]); if (ev) ev[k] = e; k++;
		e = M16(ea + (left ? 4 : 6));
		if (e == e0) break;
	}
	return (e == e0 && k >= 3) ? (s32)k : 0;
}

#define CHUNK 10
static float g_ck_lo[PATH_MAX / CHUNK + 2][3], g_ck_hi[PATH_MAX / CHUNK + 2][3];
static s32 g_ck_n;
static void chunks_build(void)
{
	s32 c, i, j;
	g_ck_n = (g_pts_n - 1 + CHUNK - 1) / CHUNK;
	for (c = 0; c < g_ck_n; c++)
	{
		s32 i0 = c * CHUNK, i1 = i0 + CHUNK; if (i1 > g_pts_n - 1) i1 = g_pts_n - 1;
		for (j = 0; j < 3; j++) g_ck_lo[c][j] = g_ck_hi[c][j] = g_pts[i0][j];
		for (i = i0 + 1; i <= i1; i++) for (j = 0; j < 3; j++) { if (g_pts[i][j] < g_ck_lo[c][j]) g_ck_lo[c][j] = g_pts[i][j]; if (g_pts[i][j] > g_ck_hi[c][j]) g_ck_hi[c][j] = g_pts[i][j]; }
	}
}
/* the level's surfaces: the range of grid cells each one's box covers (and whether it's floor enough to land on, the top
 * bit), measured once per level - 4 bytes a surface: the landing and the blockers built every one of the level's ~7,000-
 * 15,000 polygons before looking at their boxes, each search (most of the 360's cost). (5e.12 kept whole float boxes -
 * 393 KB: the game didn't boot.) The cells come from the grid's comparison cell function - no float-to-integer conversion. */
#define SBX_MAX 16384
#ifdef HOST
static u8 g_sxy_host[SBX_MAX][4];
static u8 (*g_sxy)[4] = g_sxy_host;
#else
static u8 (*g_sxy)[4];                          /* cell x from (bit 7: can land), y from, x to, y to - 64 KB taken the first
                                                   time it's needed (6h.3: out of the mod's size at boot); none: searched without it */
#endif
static u32 g_sbx_for, g_sbx_sig;
static s32 g_sbx_n, g_sbx_ok, g_sq[4], g_sq_ok;
static void sbx_ready(void)
{
	s_xf id;
	u32 s;
	if (g_sbx_ok && g_sbx_for == g_cbm_for && g_sbx_sig == g_cbm_sig) return;
	g_sbx_ok = 0;
	if (!g_lg_ok || g_lg_for != g_cbm_for || g_lg_sig != g_cbm_sig || g_cbm.ns > SBX_MAX) return;   /* the grid's cells first */
#ifndef HOST
	if (!g_sxy)
	{
		static s32 said;
		g_sxy = (u8 (*)[4])VirtualAlloc_x(0, SBX_MAX * 4, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 4 /* PAGE_READWRITE */);
		if (!g_sxy) { if (!said) { said = 1; lput(0, "surface cache: not enough memory (64 KB) - the searches run without it"); log_write(); } return; }
	}
#endif
	g_sbx_for = g_cbm_for; g_sbx_sig = g_cbm_sig;
	id.on = 0;
	for (s = 0; s < g_cbm.ns; s++)
	{
		float pv[16][3], wn[3], lo[2], hi[2];
		s32 k = surf_poly(&g_cbm, &id, s, pv, 0, wn), i, j, x0, y0, x1, y1;
		if (!k) { g_sxy[s][0] = 0x7F; g_sxy[s][1] = 0x7F; g_sxy[s][2] = 0; g_sxy[s][3] = 0; continue; }   /* never overlaps */
		for (j = 0; j < 2; j++) lo[j] = hi[j] = pv[0][j];
		for (i = 1; i < k; i++) for (j = 0; j < 2; j++) { if (pv[i][j] < lo[j]) lo[j] = pv[i][j]; if (pv[i][j] > hi[j]) hi[j] = pv[i][j]; }
		lg_cell(lo[0], lo[1], &x0, &y0); lg_cell(hi[0], hi[1], &x1, &y1);
		g_sxy[s][0] = (u8)(x0 | (wn[2] >= 0.2f ? 0x80 : 0)); g_sxy[s][1] = (u8)y0; g_sxy[s][2] = (u8)x1; g_sxy[s][3] = (u8)y1;
	}
	g_sbx_n = (s32)g_cbm.ns; g_sbx_ok = 1;
}
/* the query box's cells, once per land_from / blockers_add on the level */
static void sbx_query(const s_cb *c, const s_xf *x, const float *lo, const float *hi)
{
	g_sq_ok = c == &g_cbm && !x->on && g_sbx_ok;
	if (g_sq_ok) { lg_cell(lo[0], lo[1], &g_sq[0], &g_sq[1]); lg_cell(hi[0], hi[1], &g_sq[2], &g_sq[3]); }
}
/* the level's surface s is outside the query's cells (by its cached cells) - its polygon needn't be built */
static s32 sbx_out(const s_cb *c, const s_xf *x, u32 s, const float *lo, const float *hi)
{
	(void)c; (void)x; (void)lo; (void)hi;
	if (!g_sq_ok || (s32)s >= g_sbx_n) return 0;
	return g_sxy[s][2] < g_sq[0] || (g_sxy[s][0] & 0x7F) > g_sq[2] || g_sxy[s][3] < g_sq[1] || g_sxy[s][1] > g_sq[3];
}
static void land_from(const s_cb *c, const s_xf *x, const float *lo, const float *hi, s32 obj)
{
	u32 s;
	sbx_query(c, x, lo, hi);
	for (s = 0; s < c->ns; s++)
	{
		float pv[16][3], wn[3], blo[3], bhi[3];
		s32 k, i, j;
		if (g_sq_ok && (s32)s < g_sbx_n && (!(g_sxy[s][0] & 0x80) || sbx_out(c, x, s, lo, hi))) continue;   /* by its cached cells: not built */
		k = surf_poly(c, x, s, pv, 0, wn);
		if (!k || wn[2] < 0.2f) continue;                                    /* floors and slopes you can land on */
		for (j = 0; j < 3; j++) { blo[j] = bhi[j] = pv[0][j]; }
		for (i = 1; i < k; i++) for (j = 0; j < 3; j++) { if (pv[i][j] < blo[j]) blo[j] = pv[i][j]; if (pv[i][j] > bhi[j]) bhi[j] = pv[i][j]; }
		if (bhi[0] < lo[0] || blo[0] > hi[0] || bhi[1] < lo[1] || blo[1] > hi[1] || bhi[2] < lo[2] || blo[2] > hi[2]) continue;
		for (i = 0; i + 1 < g_pts_n && i <= g_land_seg; i++)
		{
			float *p, *q, t, hit[3];
			if (i % CHUNK == 0)                                                   /* a whole chunk away from this surface: skip it */
			{
				s32 ck = i / CHUNK;
				if (ck < g_ck_n && (g_ck_hi[ck][0] < blo[0] || g_ck_lo[ck][0] > bhi[0] || g_ck_hi[ck][1] < blo[1] || g_ck_lo[ck][1] > bhi[1] || g_ck_hi[ck][2] < blo[2] || g_ck_lo[ck][2] > bhi[2]))
				{ i += CHUNK - 1; continue; }
			}
			p = g_pts[i]; q = g_pts[i + 1];
			float d0 = (p[0] - pv[0][0]) * wn[0] + (p[1] - pv[0][1]) * wn[1] + (p[2] - pv[0][2]) * wn[2];
			float d1 = (q[0] - pv[0][0]) * wn[0] + (q[1] - pv[0][1]) * wn[1] + (q[2] - pv[0][2]) * wn[2];
			if (!(d0 > 0.f && d1 <= 0.f)) continue;                            /* crossing it from the open side */
			t = d0 / (d0 - d1);
			if (i == g_land_seg && t >= g_land_frac) continue;
			for (j = 0; j < 3; j++) hit[j] = p[j] + (q[j] - p[j]) * t;
			if (!poly_inside(pv, k, wn, hit)) continue;
			g_land_ok = 1; g_land_seg = i; g_land_frac = t;
			for (j = 0; j < 3; j++) { g_land_p[j] = hit[j]; g_land_n[j] = wn[j]; g_land_p0[j] = pv[0][j]; g_land_v[j] = g_vels[i + 1][j]; }
			g_land_cb = *c; g_land_xf = *x; g_land_s = s; g_land_obj = obj;
			break;
		}
	}
}

static void closest_on(const float *p, const float *a, const float *b, float *o)
{
	float d[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, t = dot3(d, d);
	s32 k;
	t = t > 1e-12f ? ((p[0] - a[0]) * d[0] + (p[1] - a[1]) * d[1] + (p[2] - a[2]) * d[2]) / t : 0.f;
	if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
	for (k = 0; k < 3; k++) o[k] = a[k] + d[k] * t;
}

/* the landing surface's own edge nearest the landing point, where a floor seam meets it (sb_nearest_seam) */
static s32 landing_nearest_seam(void)
{
	float pv[16][3], wn[3], best = 1e9f;
	u32 ev[16];
	s32 k = surf_poly(&g_land_cb, &g_land_xf, g_land_s, pv, ev, wn), i, found = 0;
	if (!k) return 0;
	for (i = 0; i < k; i++)
	{
		u32 ea = g_land_cb.edge + ev[i] * 12, el = M16(ea + 8), er = M16(ea + 10), other = (el == g_land_s) ? er : el;
		float on[3], won[3], c[3], d;
		if (other >= g_land_cb.ns) continue;
		if (!cb_normal(&g_land_cb, other, on)) continue;
		xf_dir(&g_land_xf, on, won);
		{ float l = fsqrt(dot3(won, won)); won[0] /= l; won[1] /= l; won[2] /= l; }
		if (wn[2] < 0.5f || won[2] < 0.5f || dot3(wn, won) < 0.866f) continue;   /* sb_floor_seam */
		closest_on(g_land_p, pv[i], pv[(i + 1) % k], c);
		c[0] -= g_land_p[0]; c[1] -= g_land_p[1]; c[2] -= g_land_p[2];
		d = fsqrt(dot3(c, c));
		if (d < best)
		{
			float cs = dot3(wn, won), cr[3] = { wn[1] * won[2] - wn[2] * won[1], wn[2] * won[0] - wn[0] * won[2], wn[0] * won[1] - wn[1] * won[0] };
			s32 j;
			best = d; found = 1;
			for (j = 0; j < 3; j++) { g_ld_sa[j] = pv[i][j]; g_ld_sb[j] = pv[(i + 1) % k][j]; }
			g_ld_bend = fatan2(fsqrt(dot3(cr, cr)), cs) * 57.29578f;
			if (g_ld_bend < 0.3f) g_ld_bend = 0.f;
			else
			{
				/* its sign: + valley, - ridge (each face's off-edge corner against the other's plane) */
				float qo[3], la[3], lb[3], above = 0.f;
				s32 m;
				for (m = 0; m < k; m++)
				{
					float w[3] = { pv[m][0] - pv[i][0], pv[m][1] - pv[i][1], pv[m][2] - pv[i][2] };
					if (m != i && m != (i + 1) % k) { above += dot3(w, won); break; }
				}
				(void)qo; (void)la; (void)lb;
				if (above < 0.f) g_ld_bend = -g_ld_bend;
			}
		}
	}
	if (g_land_obj && k >= 4)                                              /* an object's polygon: its triangles' seams too */
		for (i = 2; i + 1 < k; i++)
		{
			float c[3], d;
			closest_on(g_land_p, pv[0], pv[i], c);
			c[0] -= g_land_p[0]; c[1] -= g_land_p[1]; c[2] -= g_land_p[2];
			d = fsqrt(dot3(c, c));
			if (d < best) { s32 j; best = d; found = 1; for (j = 0; j < 3; j++) { g_ld_sa[j] = pv[0][j]; g_ld_sb[j] = pv[i][j]; } g_ld_bend = 0.f; }
		}
	g_ld_off = best;
	return found;
}

/* Cartographer's sb_seam_yaw: the seam's bend seen from above at the landing - the faces under the pill (radius 0.1875)
 * each push outward from their nearest edge; the pair nearest opposite (within 15 deg, at least 0.3 deg off) gives a
 * sideways push of its own. Faces looked at: the landing surface and its neighbours across its edges (a face within
 * 0.19 of a point on the landing surface touches it there). */
static s32 g_ld_bent;
static float g_ld_yaw, g_ld_side[3];
static s32 seam_yaw(void)
{
	float pushes[17][2], r = 0.1875f, pv[16][3], wn[3], at[2] = { g_land_p[0], g_land_p[1] };
	u32 ev[16], cand[17];
	s32 k = surf_poly(&g_land_cb, &g_land_xf, g_land_s, pv, ev, wn), np = 0, nc = 0, i, j, found = 0;
	if (!k) return 0;
	cand[nc++] = g_land_s;
	for (i = 0; i < k && nc < 17; i++)
	{
		u32 ea = g_land_cb.edge + ev[i] * 12, el = M16(ea + 8), er = M16(ea + 10), other = (el == g_land_s) ? er : el;
		if (other < g_land_cb.ns) cand[nc++] = other;
	}
	for (i = 0; i < nc; i++)
	{
		float qv[16][3], qn[3], cx = 0.f, cy = 0.f, best = 1e9f, out[2] = { 0, 0 }, zlo, zhi;
		s32 m = surf_poly(&g_land_cb, &g_land_xf, cand[i], qv, 0, qn), e;
		if (!m || qn[2] <= 0.2f) continue;
		zlo = zhi = qv[0][2];
		for (e = 0; e < m; e++) { cx += qv[e][0]; cy += qv[e][1]; if (qv[e][2] < zlo) zlo = qv[e][2]; if (qv[e][2] > zhi) zhi = qv[e][2]; }
		if (zhi < g_land_p[2] - 0.5f || zlo > g_land_p[2] + 0.5f) continue;
		cx /= (float)m; cy /= (float)m;
		for (e = 0; e < m; e++)
		{
			float px = qv[e][0], py = qv[e][1], qx = qv[(e + 1) % m][0], qy = qv[(e + 1) % m][1], dx = qx - px, dy = qy - py, l2 = dx * dx + dy * dy, t, clx, cly, d, nx, ny, nl;
			if (l2 < 1e-8f) continue;
			t = ((at[0] - px) * dx + (at[1] - py) * dy) / l2;
			if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
			clx = px + dx * t; cly = py + dy * t;
			d = fsqrt((at[0] - clx) * (at[0] - clx) + (at[1] - cly) * (at[1] - cly));
			if (d >= best) continue;
			nx = dy; ny = -dx; nl = fsqrt(nx * nx + ny * ny); nx /= nl; ny /= nl;
			if (nx * (cx - clx) + ny * (cy - cly) > 0.f) { nx = -nx; ny = -ny; }      /* outward, away from the face */
			best = d; out[0] = nx; out[1] = ny;
		}
		if (best <= r && np < 17) { pushes[np][0] = out[0]; pushes[np][1] = out[1]; np++; }
	}
	g_ld_yaw = 0.f;
	for (i = 0; i < np; i++)
		for (j = i + 1; j < np; j++)
		{
			float c = pushes[i][0] * pushes[j][0] + pushes[i][1] * pushes[j][1], mc, dev, sx, sy, sl;
			if (c > -0.966f) continue;                                             /* not within 15 deg of opposite */
			mc = -c; if (mc > 1.f) mc = 1.f;
			dev = fatan2(fsqrt(1.f - mc * mc), mc);                                 /* acos(-c) */
			if (dev <= g_ld_yaw || dev < 0.3f / 57.29578f) continue;
			sx = pushes[i][0] + pushes[j][0]; sy = pushes[i][1] + pushes[j][1]; sl = fsqrt(sx * sx + sy * sy);
			if (sl < 1e-9f) continue;
			g_ld_yaw = dev; g_ld_side[0] = sx / sl; g_ld_side[1] = sy / sl; g_ld_side[2] = 0.f; found = 1;
		}
	return found;
}

/* Cartographer's sb_up_estimate: the launch straight up for a total tilt (radians) */
static float sb_up_estimate(float tilt)
{
	float deg = tilt * 57.29578f, up;
	if (deg < 0.55f) return 0.f;
	up = 15.f - 0.55f * deg; if (up < 0.f) up = 0.f;
	return deg < 2.15f ? up * (deg / 2.15f) : up;
}
static float g_ld_total, g_ld_dir[3], g_ld_upax[3], g_ld_up;
static s32 g_ld_arrow, g_arrow_lbl, g_arrow_isup;
static float g_arrow_sx, g_arrow_sy;

/* Cartographer's sb_tilt: the drift across the seam, as an angle from straight down */
static float sb_tilt(const float *vel, const float *t)
{
	float vt = dot3(vel, t), ap[3] = { vel[0] - t[0] * vt, vel[1] - t[1] * vt, vel[2] - t[2] * vt };
	float ut = t[2], vert[3] = { -t[0] * ut, -t[1] * ut, 1.f - t[2] * ut }, l = fsqrt(dot3(vert, vert)), side[3], ls, a, b;
	vert[0] /= l; vert[1] /= l; vert[2] /= l;
	side[0] = t[1] * vert[2] - t[2] * vert[1]; side[1] = t[2] * vert[0] - t[0] * vert[2]; side[2] = t[0] * vert[1] - t[1] * vert[0];
	ls = fsqrt(dot3(side, side)); side[0] /= ls; side[1] /= ls; side[2] /= ls;
	a = dot3(ap, side); b = dot3(ap, vert);
	if (a < 0.f) a = -a;
	if (b < 0.f) b = -b;
	return b > 1e-9f ? fatan2(a, b) : 1.5707963f;
}

static void landing_find(u32 object)
{
	float lo[3], hi[3], cv[3], lv[3];
	s32 i, j;
	s_xf id;
	g_land_ok = 0; g_land_seg = PATH_MAX; g_land_frac = 2.f; g_ld_q = -1; g_arc_n = 0; g_ld_arrow = 0; g_ld_bent = 0; g_ld_total = 0.f; g_arrow_lbl = 0;
	if (!g_sb_airborne) return;
	if (cb_ready()) sbx_ready();
	path_compute(object);
	chunks_build();
	for (j = 0; j < 3; j++) lo[j] = hi[j] = g_pts[0][j];
	for (i = 1; i < g_pts_n; i++) for (j = 0; j < 3; j++) { if (g_pts[i][j] < lo[j]) lo[j] = g_pts[i][j]; if (g_pts[i][j] > hi[j]) hi[j] = g_pts[i][j]; }
	id.on = 0;
	if (cb_ready()) land_from(&g_cbm, &id, lo, hi, 0);
	for (i = 0; i < g_nb_n; i++) land_from(&g_nb_cb[i], &g_nb_xf[i], lo, hi, g_nb_xf[i].on == 2);
	g_arc_n = g_land_ok ? g_land_seg + 1 : g_pts_n;
	if (!g_land_ok) return;
	/* the depth (sb_predict_landing) */
	g_ld_have_depth = 0;
	{
		float h_last = 1.f, step = 0.f;
		for (j = 0; j < 3; j++) cv[j] = lv[j] = g_land_v[j];
		if (g_land_n[2] > 0.05f)
			for (i = 0; i + 2 < g_pts_n; i++)
			{
				float hk = ((g_pts[i][0] - g_land_p0[0]) * g_land_n[0] + (g_pts[i][1] - g_land_p0[1]) * g_land_n[1] + (g_pts[i][2] - g_land_p0[2]) * g_land_n[2]) / g_land_n[2];
				float step1 = -g_vels[i + 1][2] / 30.f, hn;
				if (hk > -0.025f + 1.2f * step1) continue;
				hn = ((g_pts[i + 1][0] - g_land_p0[0]) * g_land_n[0] + (g_pts[i + 1][1] - g_land_p0[1]) * g_land_n[1] + (g_pts[i + 1][2] - g_land_p0[2]) * g_land_n[2]) / g_land_n[2];
				h_last = hk; step = step1;
				g_ld_depth = sb_depth(step1, hk);
				g_ld_late = sb_depth(-g_vels[i + 2][2] / 30.f, hn);
				for (j = 0; j < 3; j++) { cv[j] = g_vels[i + 1][j]; lv[j] = g_vels[i + 2][j]; }
				g_ld_have_depth = 1;
				break;
			}
		/* the verdict (sb_draw_landing) */
		{
			const float vsweep = SB_NEED;
			s32 seam = landing_nearest_seam(), hard = 0, fix = 0, off_edge = 0, weak = 0, fast = 0;
			s32 bent = seam && seam_yaw() && g_ld_yaw * 57.29578f >= 0.6f;          /* the seam's own sideways push */
			float s_on = fsqrt(dot3(cv, cv)), s_late = fsqrt(dot3(lv, lv)), d_on = s_on - vsweep;
			s32 speed_edge = !bent && (d_on < 0.f ? -d_on : d_on) < 0.05f;
			s32 on_ok = (s_on >= vsweep || bent || speed_edge) && (!g_ld_have_depth || g_ld_depth >= SB_DEPTH_MIN);
			s32 late_ok = g_ld_have_depth && (s_late >= vsweep || bent) && g_ld_late >= SB_DEPTH_MIN;
			g_ld_bent = bent;
			float *vc = (on_ok || !late_ok) ? cv : lv;
			g_ld_seam = seam;
			g_ld_speed = fsqrt(dot3(vc, vc)); g_ld_tilt = 0.f; g_ld_down = vc[2] < 0.f ? -vc[2] : 0.f;
			g_ld_why = "";
			if (g_sb_charged != 1) { g_ld_why = "NOT CHARGED"; hard = 1; }
			if (!seam) { g_ld_why = "no seam here"; hard = 1; }
			else if (!on_ok && !late_ok) { g_ld_why = (s_late < vsweep && !bent) ? "TOO SLOW" : "TOO SHALLOW"; hard = 1; }
			if (seam && g_ld_have_depth && h_last - step <= -SB_DEBOUNCE && !on_ok && late_ok) { g_ld_why = "DEBOUNCE: you fall through"; hard = 1; }
			if (seam)
			{
				float sd[3] = { g_ld_sb[0] - g_ld_sa[0], g_ld_sb[1] - g_ld_sa[1], g_ld_sb[2] - g_ld_sa[2] }, l = fsqrt(dot3(sd, sd)), deg;
				sd[0] /= l; sd[1] /= l; sd[2] /= l;
				g_ld_tilt = g_ld_speed >= vsweep ? sb_tilt(vc, sd) : 0.f;
				/* the drift's up tilt and the seam's bend (sideways) add as vectors */
				{
					float sy = bent ? g_ld_yaw : 0.f, ul;
					s32 m;
					g_ld_upax[0] = -sd[0] * sd[2]; g_ld_upax[1] = -sd[1] * sd[2]; g_ld_upax[2] = 1.f - sd[2] * sd[2];
					ul = fsqrt(dot3(g_ld_upax, g_ld_upax)); for (m = 0; m < 3; m++) g_ld_upax[m] /= ul;
					g_ld_total = fsqrt(g_ld_tilt * g_ld_tilt + sy * sy);
					g_ld_dir[0] = 0.f; g_ld_dir[1] = 0.f; g_ld_dir[2] = 1.f;
					if (g_ld_total > 1e-5f)
					{
						float dl;
						for (m = 0; m < 3; m++) g_ld_dir[m] = g_ld_upax[m] * g_ld_tilt + g_ld_side[m] * sy;
						dl = fsqrt(dot3(g_ld_dir, g_ld_dir)); for (m = 0; m < 3; m++) g_ld_dir[m] /= dl;
					}
				}
				if (g_ld_off > 0.06f + 0.01f) { if (!hard) g_ld_why = "OFF THE SEAM"; fix = 1; }
				else if (g_ld_off > 0.06f) off_edge = 1;
				deg = g_ld_total * 57.29578f;
				if (deg < 0.6f) { if (!hard && !fix) g_ld_why = "ANGLE: too little drift across"; fix = 1; }
				else if (deg >= 13.f) { if (!hard && !fix) g_ld_why = "ANGLE: drifting too fast across"; fix = 1; }
				else if (deg >= 8.f) fast = 1;
				else if (deg < 1.f) weak = 1;
			}
			{
				s32 depth_edge = on_ok && g_ld_have_depth && g_ld_depth < 0.21f;
				s32 edge = on_ok && (speed_edge || depth_edge || weak || off_edge || fast);
				g_ld_q = hard ? 0 : ((fix || !on_ok) ? 1 : (edge ? 3 : 2));
				if (!hard && !fix) g_ld_why = !on_ok ? "LOTTERY (about 1 in 30)" : edge ? "EDGE" : "LIKELY (9 in 10)";
				/* a ghost seam: its pure-lottery landings have launched - amber, with what it did */
				{
					s32 pure = g_ld_q == 1 && !fix && !hard, gt = 0, gh = 0;
					g_la_ok = pure && seam && !g_land_obj;
					if (g_la_ok) { s32 m; for (m = 0; m < 3; m++) { g_la_a[m] = g_ld_sa[m]; g_la_b[m] = g_ld_sb[m]; } }
					if (g_la_ok && ghost_get(g_ld_sa, g_ld_sb, &gt, &gh))
					{
						static char why[72];
						s32 w = 0, v, k2;
						char b[12];
						const char *p1 = "GHOST SEAM: ", *p2 = " of ", *p3 = " lottery landings here launched";
						while (*p1) why[w++] = *p1++;
						v = gh; k2 = 0; do { b[k2++] = (char)('0' + v % 10); v /= 10; } while (v); while (k2) why[w++] = b[--k2];
						while (*p2) why[w++] = *p2++;
						v = gt; k2 = 0; do { b[k2++] = (char)('0' + v % 10); v /= 10; } while (v); while (k2) why[w++] = b[--k2];
						while (*p3 && w < 71) why[w++] = *p3++;
						why[w] = 0;
						g_ld_q = 3; g_ld_why = why;
					}
				}
				/* the launch arrow: everything you control is right and it's deep on time */
				g_ld_arrow = seam && !hard && !fix && on_ok;
				g_ld_up = g_ld_arrow ? sb_up_estimate(g_ld_total) : 0.f;
			}
		}
	}
}

/* Cartographer's sb_quality_color: amber edge, green likely, yellow lottery, red no */
static void q_colour(s32 q, float a)
{
	if (q == 3) gfx_begin(a, 1.f, 150.f / 255.f, 40.f / 255.f);
	else if (q >= 2) gfx_begin(a, 80.f / 255.f, 230.f / 255.f, 90.f / 255.f);
	else if (q == 1) gfx_begin(a, 1.f, 210.f / 255.f, 50.f / 255.f);
	else gfx_begin(a, 1.f, 80.f / 255.f, 70.f / 255.f);
}

/* the landing seam's own rating (its colour among the seams), or -1: unrated / not among them */
static s32 landing_seam_q(void)
{
	s32 i, k;
	for (i = 0; i < d_seam_n; i++)
	{
		float da = 0.f, db = 0.f;
		for (k = 0; k < 3; k++)
		{
			float a = d_seam_ab[i][k] - g_ld_sa[k], b = d_seam_ab[i][3 + k] - g_ld_sb[k], c = d_seam_ab[i][k] - g_ld_sb[k], d = d_seam_ab[i][3 + k] - g_ld_sa[k];
			da += a * a + b * b; db += c * c + d * d;
		}
		if (da < 1e-6f || db < 1e-6f) return d_seam_q[i];
	}
	return -1;
}
#ifdef HOST
s32 host_landing_seam_q(const float *a, const float *b) { s32 k; for (k = 0; k < 3; k++) { g_ld_sa[k] = a[k]; g_ld_sb[k] = b[k]; } return landing_seam_q(); }
s32 host_drawn_n(void) { return d_seam_n; }
float host_drawn_sum(void) { s32 i, k, t = 0; for (i = 0; i < d_seam_n; i++) for (k = 0; k < 6; k++) { float v = d_seam_ab[i][k] * 1000.f; t += (s32)(v < 0.f ? v - 0.5f : v + 0.5f) * (k + 1); } return (float)t; }
void host_set_step_div(s32 d) { g_host_step_div = d; }
void host_set_seam_cap(s32 c) { g_seam_cap = c; }
/* the kept seam nearest the point (x, y): its rating and why */
s32 host_seam_q_at(float x, float y, const char **why)
{
	s32 i, best = -1;
	float bd = 1e30f;
	for (i = 0; i < g_seam_n; i++)
	{
		float mx = (g_seam_ab[i][0] + g_seam_ab[i][3]) * 0.5f - x, my = (g_seam_ab[i][1] + g_seam_ab[i][4]) * 0.5f - y, d = mx * mx + my * my;
		if (d < bd) { bd = d; best = i; }
	}
	if (best < 0) return -9;
	*why = g_seam_rd[best].why;
	return g_seam_q[best];
}
s32 host_drawn_landing(float *p) { p[0] = d_land_p[0]; p[1] = d_land_p[1]; p[2] = d_land_p[2]; return d_land_ok; }
/* the kept seams' keys, sorted: the first n into out */
s32 host_seam_keys(float *out, s32 n) { s32 i, j, m = g_seam_n < n ? g_seam_n : n; float k[SEAM_MAX]; for (i = 0; i < g_seam_n; i++) k[i] = g_seam_d2[i]; for (i = 1; i < g_seam_n; i++) { float t = k[i]; j = i - 1; while (j >= 0 && k[j] > t) { k[j + 1] = k[j]; j--; } k[j + 1] = t; } for (i = 0; i < m; i++) out[i] = k[i]; return g_seam_n; }
u32 host_count_hud(void) { return g_count_hud; }
void host_put_seam(s32 i, const float *a, const float *b, s32 q)          /* a finished seam: the search's and the drawn copy */
{
	s32 k;
	for (k = 0; k < 3; k++) { g_seam_ab[i][k] = a[k]; g_seam_ab[i][3 + k] = b[k]; d_seam_ab[i][k] = a[k]; d_seam_ab[i][3 + k] = b[k]; }
	g_seam_q[i] = (s8)q; d_seam_q[i] = (s8)q;
	if (g_seam_n <= i) g_seam_n = i + 1;
	if (d_seam_n <= i) d_seam_n = i + 1;
}
#endif

/* sb_draw_landing's drawing: the path, the landing seam highlighted (4 px) in its own rating's colour - the verdict
 * shows on the circle and the arrow only - a white line to it, the circle */
static void arc_draw(void)
{
	s32 i;
	if (!g_sb_airborne || g_arc_n < 2) return;
	g_line_half = 0.5f;
	gfx_begin(140.f / 255.f, 200.f / 255.f, 200.f / 255.f, 1.f);
	for (i = 0; i + 1 < g_arc_n; i++) gfx_line3(g_pts[i], g_pts[i + 1]);
	if (g_land_ok) gfx_line3(g_pts[g_arc_n - 1], g_land_p);
	gfx_end();
	if (g_land_ok)
	{
		float c[3];
		if (g_ld_seam)
		{
			{
				s32 sq = landing_seam_q();                                      /* the seam keeps its own colour */
				g_line_half = 2.f;
				if (sq >= 0) q_colour(sq, 1.f); else gfx_begin(1.f, 1.f, 1.f, 1.f);
				gfx_line3(g_ld_sa, g_ld_sb); gfx_end();
			}
			closest_on(g_land_p, g_ld_sa, g_ld_sb, c);
			g_line_half = 0.75f; gfx_begin(220.f / 255.f, 1.f, 1.f, 1.f); gfx_line3(g_land_p, c); gfx_end();
		}
		g_line_half = 1.f;
		q_colour(g_ld_q, 1.f);
		for (i = 0; i < 16; i++)
		{
			/* cos / sin of 22.5-degree steps, as a 16-point table */
			static const float cs[17][2] = { { 1.f, 0.f }, { 0.92388f, 0.38268f }, { 0.70711f, 0.70711f }, { 0.38268f, 0.92388f }, { 0.f, 1.f },
				{ -0.38268f, 0.92388f }, { -0.70711f, 0.70711f }, { -0.92388f, 0.38268f }, { -1.f, 0.f }, { -0.92388f, -0.38268f },
				{ -0.70711f, -0.70711f }, { -0.38268f, -0.92388f }, { 0.f, -1.f }, { 0.38268f, -0.92388f }, { 0.70711f, -0.70711f },
				{ 0.92388f, -0.38268f }, { 1.f, 0.f } };
			float p0[3] = { g_land_p[0] + cs[i][0] * 0.15f, g_land_p[1] + cs[i][1] * 0.15f, g_land_p[2] + 0.01f };
			float p1[3] = { g_land_p[0] + cs[i + 1][0] * 0.15f, g_land_p[1] + cs[i + 1][1] * 0.15f, g_land_p[2] + 0.01f };
			gfx_line3(p0, p1);
		}
		gfx_end();
		if (g_ld_arrow && g_ld_up > 0.05f)
		{
			float d[3], tip[3], side[3], sl, length, head, e0[3], e1[3];
			s32 m, up = g_ld_dir[2] > 0.7f;
			if (up)
			{
				/* sb_arrow: straight up, as long as half the root of the height it throws you */
				float height = g_ld_up * g_ld_up / (2.f * 4.1712f);
				length = 0.5f * fsqrt(height); if (length < 0.25f) length = 0.25f; if (length > 4.f) length = 4.f;
				for (m = 0; m < 3; m++) d[m] = g_ld_upax[m];
				head = 0.18f * length; if (head < 0.1f) head = 0.1f;
				side[0] = d[1]; side[1] = -d[0]; side[2] = 0.f;
			}
			else
			{
				/* sb_dir_arrow: the direction it throws you */
				length = 0.08f * g_ld_up; if (length > 3.f) length = 3.f;
				for (m = 0; m < 3; m++) d[m] = g_ld_dir[m];
				head = 0.2f;
				if (d[2] > -0.9f && d[2] < 0.9f) { side[0] = d[1]; side[1] = -d[0]; side[2] = 0.f; }
				else { side[0] = 0.f; side[1] = d[2]; side[2] = -d[1]; }
			}
			sl = fsqrt(dot3(side, side));
			if (sl < 1e-3f) { side[0] = 1.f; side[1] = side[2] = 0.f; sl = 1.f; }
			for (m = 0; m < 3; m++) { side[m] /= sl; tip[m] = g_land_p[m] + d[m] * length; }
			g_line_half = 1.5f;
			q_colour(g_ld_q, 1.f);
			gfx_line3(g_land_p, tip);
			for (m = 0; m < 3; m++) { e0[m] = tip[m] - d[m] * head + side[m] * (up ? 0.5f * head : 0.1f); e1[m] = tip[m] - d[m] * head - side[m] * (up ? 0.5f * head : 0.1f); }
			gfx_line3(tip, e0); gfx_line3(tip, e1);
			gfx_end();
			/* its label goes at the tip, a little above (as Cartographer's text) */
			{
				float lp[3] = { tip[0], tip[1], tip[2] + 0.1f }, dep;
				project(lp, &g_arrow_sx, &g_arrow_sy, &dep);
				g_arrow_lbl = dep > 0.05f && g_arrow_sx > -100.f && g_arrow_sx < (float)g_vp_w + 100.f && g_arrow_sy > 0.f && g_arrow_sy < (float)g_vp_h;
				g_arrow_isup = up;
			}
		}
	}
	g_line_half = 1.f;
}

/* ---- Cartographer's air reach (sb_reach_build / sb_reach_ok): in the air, 17 drifts - air control at 2.25 u/s in 16
 * directions, and the stick let go - outline where you can be at each moment of the fall; a point is in reach if it lies
 * inside the outline (or within 0.35: the pill's width and a little) when the fall gets down to its height. Cartographer
 * keeps up to 20 s of outlines; on the Xbox 5 s (150 ticks - a 30-unit fall from rest), the last one used beyond. ---- */
#define REACH_N 2                      /* the 3d.10 air-reach outline: unused since 3d.12 */
static float g_rch[REACH_N][17][2], g_rch_z0, g_rch_vz0;
static s32 g_rch_on, g_rch_n;
static void reach_build(const float *feet, const float *vel)
{
	float p[17][2], v[17][2], tg[17][2], step = 1.0485f / 30.f, tmax, disc;
	static const float cs[16][2] = { { 1.f, 0.f }, { 0.92388f, 0.38268f }, { 0.70711f, 0.70711f }, { 0.38268f, 0.92388f }, { 0.f, 1.f },
		{ -0.38268f, 0.92388f }, { -0.70711f, 0.70711f }, { -0.92388f, 0.38268f }, { -1.f, 0.f }, { -0.92388f, -0.38268f },
		{ -0.70711f, -0.70711f }, { -0.38268f, -0.92388f }, { 0.f, -1.f }, { 0.38268f, -0.92388f }, { 0.70711f, -0.70711f }, { 0.92388f, -0.38268f } };
	s32 j, k;
	g_rch_on = 1; g_rch_z0 = feet[2]; g_rch_vz0 = vel[2];
	disc = vel[2] * vel[2] + 2.f * 4.1712f * 400.f;
	tmax = (vel[2] + fsqrt(disc > 0.f ? disc : 0.f)) / 4.1712f; if (tmax > 20.f) tmax = 20.f;
	g_rch_n = (s32)(tmax * 30.f) + 2; if (g_rch_n > REACH_N) g_rch_n = REACH_N;
	for (j = 0; j < 17; j++)
	{
		p[j][0] = feet[0]; p[j][1] = feet[1]; v[j][0] = vel[0]; v[j][1] = vel[1];
		tg[j][0] = j < 16 ? cs[j][0] * 2.25f : 0.f; tg[j][1] = j < 16 ? cs[j][1] * 2.25f : 0.f;
	}
	for (k = 0; k < g_rch_n; k++)
		for (j = 0; j < 17; j++)
		{
			float tx = tg[j][0] - v[j][0], ty = tg[j][1] - v[j][1], gap = fsqrt(tx * tx + ty * ty);
			g_rch[k][j][0] = p[j][0]; g_rch[k][j][1] = p[j][1];
			if (gap <= step) { v[j][0] = tg[j][0]; v[j][1] = tg[j][1]; } else { v[j][0] += tx * (step / gap); v[j][1] += ty * (step / gap); }
			p[j][0] += v[j][0] / 30.f; p[j][1] += v[j][1] / 30.f;
		}
}
static s32 reach_ok(const float *q)
{
	float disc, t, dmin = 1e9f;
	s32 k, j, inside = 1;
	if (!g_rch_on || !g_rch_n) return 1;
	disc = g_rch_vz0 * g_rch_vz0 + 2.f * 4.1712f * (g_rch_z0 - q[2]);
	if (disc < 0.f) return 0;                                                /* above the top of your arc */
	t = (g_rch_vz0 + fsqrt(disc)) / 4.1712f;
	k = (s32)(t * 30.f + 0.5f); if (k < 0) k = 0; if (k > g_rch_n - 1) k = g_rch_n - 1;
	for (j = 0; j < 16; j++)
	{
		float *a = g_rch[k][j], *b = g_rch[k][(j + 1) % 16], dx = b[0] - a[0], dy = b[1] - a[1], l2 = dx * dx + dy * dy, u, cx, cy, d;
		if (dx * (q[1] - a[1]) - dy * (q[0] - a[0]) < 0.f) inside = 0;      /* the 16 run counter-clockwise */
		u = l2 > 1e-12f ? ((q[0] - a[0]) * dx + (q[1] - a[1]) * dy) / l2 : 0.f;
		if (u < 0.f) u = 0.f; else if (u > 1.f) u = 1.f;
		cx = a[0] + dx * u - q[0]; cy = a[1] + dy * u - q[1]; d = fsqrt(cx * cx + cy * cy);
		if (d < dmin) dmin = d;
	}
	return inside || dmin <= 0.35f;
}
#ifdef HOST
s32 host_reach_ok(const float *feet, const float *vel, const float *q) { reach_build(feet, vel); { s32 r = reach_ok(q); g_rch_on = 0; return r; } }
#endif

/* ---- Cartographer's Ghost Viewer, stage 1: the ghost pill - your Havok capsule where Havok has it, from your body's
 * pair with the level (in its island's pair list; the level's side is a body in no Havok component): pink, lime
 * while a backstep is on (S > 0, or the motion's previous swept centre jumping back from where the last tick's current
 * one was) and for the hold after it, red when stuck (S = -3). With the rewind poses: where the backstep put you back
 * to (yellow) and where you were (orange). Offsets as Cartographer's (PC Havok, the same layout - teleport's
 * transform write at the motion state's +0x50 works on the Xbox). ---- */
typedef struct { float a[3], b[3], r; } s_cap;
/* a pointer read out of a Havok object: Havok's objects are in the game's own memory below 0x80000000 as well as above
 * (5b.1 allowed only the image and 0x80000000 up, and lost the capsule) - so the ordinary test. What stops a field of
 * the wrong kind being followed is reading only fields of known classes (read_children). A vtable must be in the game
 * image: Havok's are all in its read-only data. */
#define mem_ok(p) ptr_ok(p)
#ifdef HOST
#define img_ok(p) ptr_ok(p)
#else
static s32 img_ok(u32 p) { return !(p & 3) && p >= 0x10000 && p < 0x800000; }
#endif
static struct
{
	s32 valid, S, have, ever_back, have_rewind;
	s_cap cap;                                  /* in the world */
	float cur[3], prev[3], last_cur[3], rew_from[3], rew_to[3];
	u32 last_tick, last_back, rewind_tick;
} g_gp;
/* a shape's type: its getType (vtable +0x14) read from its code - "mov eax, imm32; ret" or "xor eax, eax; ret" - never
 * called (Cartographer can call it under a guard; the Xbox build has none) */
static s32 shape_type(u32 shape)
{
	u32 vt, fn;
	if (!mem_ok(shape)) return -1;
	vt = M32(shape);
	if (!img_ok(vt)) return -1;                                             /* a vtable: in the game image */
	fn = M32(vt + 0x14);
	if (fn < 0x10000 || fn >= ADDR_DATA_LO) return -1;
	if (M8(fn) == 0xB8 && M8(fn + 5) == 0xC3) { u32 t = M32(fn + 1); return t < 64 ? (s32)t : -1; }
	if (M8(fn) == 0x33 && M8(fn + 1) == 0xC0 && M8(fn + 2) == 0xC3) return 0;
	return -1;
}
/* the capsule (type 7: radius +0x0C, ends +0x10 / +0x20), or one a level or two below it (sweep / transform wrappers) */
static s32 read_capsule(u32 shape, s_cap *c, s32 depth)
{
	u32 off;
	s32 k;
	if (!mem_ok(shape)) return 0;
	if (shape_type(shape) == 7)
	{
		c->r = MF(shape + 0x0C);
		for (k = 0; k < 3; k++) { c->a[k] = MF(shape + 0x10 + 4 * (u32)k); c->b[k] = MF(shape + 0x20 + 4 * (u32)k); }
		return c->r > 0.01f && c->r < 5.f;
	}
	if (depth >= 2) return 0;
	for (off = 0x08; off <= 0x40; off += 4)
	{
		u32 child = M32(shape + off);
		if (child != shape && mem_ok(child) && read_capsule(child, c, depth + 1)) return 1;
	}
	return 0;
}
/* a body's pose: the transform at its motion state (handle - 0x10 + 8) +0x20 - three unit columns, then the position */
static s32 body_pose(u32 handle, float (*col)[3], float *t)
{
	u32 ms = M32(handle - 0x10 + 8);
	s32 i, k;
	if (!ptr_ok(ms)) return 0;
	for (i = 0; i < 3; i++)
	{
		float l2 = 0.f;
		for (k = 0; k < 3; k++) { col[i][k] = MF(ms + 0x20 + (u32)(i * 16 + k * 4)); l2 += col[i][k] * col[i][k]; }
		if (!(l2 > 0.8f && l2 < 1.2f)) return 0;
	}
	for (k = 0; k < 3; k++) t[k] = MF(ms + 0x50 + 4 * (u32)k);
	return t[0] == t[0] && t[1] == t[1] && t[2] == t[2];
}
static void cap_xf(float (*col)[3], const float *t, const float *p, float *out)
{
	s32 k;
	for (k = 0; k < 3; k++) out[k] = col[0][k] * p[0] + col[1][k] * p[1] + col[2][k] * p[2] + t[k];
}
static s32 component_body(u32 body);
/* the log, once per change: the pill found (its size), or lost and why */
static s32 g_gp_said = -1;
static s32 g_gp_from_body;                      /* the pill from your body alone: no level pair right now */
static void ghost_say(s32 what)                 /* 0 found; 1 no player; 2 no Havok body; 3 no level pair; 4 no capsule; 5 from your body */
{
	static const char *why[6] = { "", "no player unit", "no Havok body for the unit", "no pair with the level (in a vehicle?)", "the capsule shape not found", "" };
	s32 at;
	if (what == g_gp_said) return;
	g_gp_said = what;
	if (what == 0)
	{
		float dx = g_gp.cap.b[0] - g_gp.cap.a[0], dy = g_gp.cap.b[1] - g_gp.cap.a[1], dz = g_gp.cap.b[2] - g_gp.cap.a[2];
		at = lput(0, "ghost pill: found - radius x1000 "); at = lnum(at, (u32)(g_gp.cap.r * 1000.f));
		at = lput(at, ", core length x1000 "); lnum(at, (u32)(fsqrt(dx * dx + dy * dy + dz * dz) * 1000.f));
	}
	else if (what == 5) lput(0, "ghost pill: from your body (no pair with the level right now) - objects still read");
	else { at = lput(0, "ghost pill: none - "); lput(at, why[what]); }
	if (o_diag) log_write();   /* 6h.21 */
}
static void sc_say(const char *why);
static void tims_update(u32 agent, const float *prev, const float *cur);
static u32 g_level_agent;                       /* the level's pair agent: every other pair is an object */
static void objects_update(u32 best_h, const float *prev, const float *cur);
static s32 g_ok_n, g_obj_offsets;
static void ghost_derive(void);
static void objects_log(u32 best_h);
static u32 g_objlog_h;
static s32 g_objlog_off;
static void objects_log_one(void);
static s32 g_kid_n, g_kid_offsets;
static u32 g_ofs_last_tick;                     /* the offset save state (below): the last tick it saw, 0 unknown */
static s32 ofs_need(void);
static void offsets_tick(u32 obj, u32 tick);
static void ghost_update(void)
{
	u32 obj = player_object(), comp, bodies, world, best_h = 0, best_agent = 0, own_h = 0;
	s32 n, i, k, best_S = 0;
	float own_col[3][3], own_t[3];
	float best_d = 2.f, best_col[3][3], best_t[3];
	g_gp.valid = 0;
	if (!o_ghost) { g_gp_said = -1; g_ofs_last_tick = 0; return; }
	if (!obj) { ghost_say(1); return; }
	comp = pool_entry(ADDR_HAVOK_COMPONENTS_PTR, (s32)M32(obj + 0xB4), 0x78);
	world = M32(ADDR_HAVOK_WORLD_PTR);
	if (!comp || !ptr_ok(world)) { ghost_say(2); return; }
	bodies = M32(comp + 0x70); n = (s32)M32(comp + 0x74);
	if (!ptr_ok(bodies) || n <= 0 || n > 64) { ghost_say(2); return; }
	for (i = 0; i < n; i++)
	{
		u32 entity = M32(bodies + (u32)i * 0x60 + 0x40), island, pairs, handle;
		s32 pn;
		if (!ptr_ok(entity) || M32(entity + 0x08) != world) continue;
		island = M32(entity + 0x44); handle = body_handle(entity);
		if (!ptr_ok(island) || !handle) continue;
		if (!own_h && body_pose(handle, own_col, own_t)) own_h = handle;          /* your body, for when there's no level pair */
		pairs = M32(island + 0x68); pn = (s32)M32(island + 0x6C);
		if (!ptr_ok(pairs) || pn <= 0 || pn > 8192) continue;
		for (k = 0; k < pn; k++)
		{
			u32 p = pairs + (u32)k * 16, agent = M32(p + 8), other;
			float col[3][3], t[3], dx, dy, dz, d;
			if (M32(p) == handle) other = M32(p + 4); else if (M32(p + 4) == handle) other = M32(p); else continue;
			if (!ptr_ok(agent) || (agent >= ADDR_DATA_LO && agent < ADDR_DATA_HI)) continue;      /* the null agent */
			if (!ptr_ok(other) || component_body(M32(other + 0x10))) continue;                 /* not the level */
			if (!body_pose(handle, col, t)) continue;
			dx = t[0] - MF(obj + 0x64); dy = t[1] - MF(obj + 0x68); dz = t[2] - MF(obj + 0x6C);
			d = fsqrt(dx * dx + dy * dy + dz * dz);
			/* the level is the pair whose agent is the welder (its triangle list) - only that: static scenery is in no
			 * component either, but comes as a single convex agent (or a transform of one) - an object. (5d.2 let any static
			 * pair stand in for the level when there was no welder - a scenery box you stood on, so it was never measured.) */
			if (M32(agent) != ADDR_VT_WELD || d >= best_d) continue;
			{ s32 m, q; best_d = d; best_h = handle; best_agent = agent; best_S = (s32)M32(p + 12); for (m = 0; m < 3; m++) { best_t[m] = t[m]; for (q = 0; q < 3; q++) best_col[m][q] = col[m][q]; } }
		}
	}
	{
		s32 from_body = 0;
		if (!best_h)
		{
			/* no pair with the level right now: the pill from your own body all the same (no level TIMs, S 0), and the
			 * objects still read - 5d.2 stopped here, so nothing near you showed while it lasted */
			s32 m, q;
			if (!own_h) { ghost_say(3); g_kid_n = 0; g_kid_offsets = 0; return; }
			best_h = own_h; best_agent = 0; best_S = 0; from_body = 1;
			for (m = 0; m < 3; m++) { best_t[m] = own_t[m]; for (q = 0; q < 3; q++) best_col[m][q] = own_col[m][q]; }
		}
		g_level_agent = best_agent;
		g_gp_from_body = from_body;
	}
	if (!g_objlog_off) { g_objlog_h = best_h; if (guarded_call(objects_log_one)) { g_objlog_off = 1; sc_say("the object pair log faulted - off for this session"); } }
	{
		s_cap local;
		u32 tick = g_sb_tick, entity = M32(best_h + 0x10), motion = ptr_ok(entity) ? M32(entity + 0x3C) : 0;
		if (!read_capsule(M32(best_h - 0x10), &local, 0)) { ghost_say(4); return; }
		cap_xf(best_col, best_t, local.a, g_gp.cap.a); cap_xf(best_col, best_t, local.b, g_gp.cap.b); g_gp.cap.r = local.r;
		g_gp.S = best_S; g_gp.valid = 1;
		ghost_say(g_gp_from_body ? 5 : 0);
		if (best_S > 0) { g_gp.last_back = tick; g_gp.ever_back = 1; }
		/* a rewind: once per tick after integrate (motion time 1), the previous swept centre must be where the last
		 * tick's current one was - if it jumped back (up to a unit), a backstep put you there */
		if (ptr_ok(motion))
		{
			float prev[3], cur[3], mt = MF(motion + 0x7C);
			for (k = 0; k < 3; k++) { prev[k] = MF(motion + 0x60 + 4 * (u32)k); cur[k] = MF(motion + 0x70 + 4 * (u32)k); g_gp.cur[k] = cur[k]; g_gp.prev[k] = prev[k]; }
			if (mt > 1.f - 1e-4f && mt < 1.f + 1e-4f && tick != g_gp.last_tick)
			{
				if (g_gp.have && tick == g_gp.last_tick + 1)
				{
					float dx = prev[0] - g_gp.last_cur[0], dy = prev[1] - g_gp.last_cur[1], dz = prev[2] - g_gp.last_cur[2], d = fsqrt(dx * dx + dy * dy + dz * dz);
					if (d > 1e-4f && d < 1.f)
					{
						s32 at;
						for (k = 0; k < 3; k++) { g_gp.rew_from[k] = g_gp.last_cur[k]; g_gp.rew_to[k] = prev[k]; }
						g_gp.have_rewind = 1; g_gp.rewind_tick = tick; g_gp.last_back = tick; g_gp.ever_back = 1;
						/* the log, as Cartographer's: the tick, the distance, from / to (x1000), S */
						at = lput(0, "ghost rewind: tick "); at = lnum(at, tick);
						at = lput(at, ", x1000 distance "); at = lnum(at, (u32)(d * 1000.f + 0.5f));
						at = lput(at, " from "); at = lsnum(at, (s32)(g_gp.rew_from[0] * 1000.f)); at = lput(at, " "); at = lsnum(at, (s32)(g_gp.rew_from[1] * 1000.f)); at = lput(at, " "); at = lsnum(at, (s32)(g_gp.rew_from[2] * 1000.f));
						at = lput(at, " to "); at = lsnum(at, (s32)(prev[0] * 1000.f)); at = lput(at, " "); at = lsnum(at, (s32)(prev[1] * 1000.f)); at = lput(at, " "); at = lsnum(at, (s32)(prev[2] * 1000.f));
						at = lput(at, ", S "); lsnum(at, best_S);
						if (o_diag) log_write();   /* 6h.21 */
					}
				}
				for (k = 0; k < 3; k++) g_gp.last_cur[k] = cur[k];
				g_gp.last_tick = tick; g_gp.have = 1;
			}
			trace("ghost: level TIMs");
		if (o_tim_overlay || o_believed || o_travel || ofs_need()) tims_update(best_agent, prev, cur);
		}
		else if (o_tim_overlay || o_believed || o_travel || ofs_need()) tims_update(best_agent, 0, 0);
		trace("ghost: objects");
		if (o_obj_believed || o_obj_travel || ofs_need()) objects_update(best_h, ptr_ok(motion) ? g_gp.prev : 0, ptr_ok(motion) ? g_gp.cur : 0);
		else { g_ok_n = 0; g_obj_offsets = 0; }
		trace("ghost: poses / regions");
		ghost_derive();
		if (g_level_agent) offsets_tick(obj, tick);                     /* the offset save state (6h): this frame's pairs */
	}
}
/* ---- Ghost Viewer stage 2: the TIMs (Cartographer's welder children). The pill's pair agent keeps a child for each
 * triangle near you ({key, ?, agent} at +0x0C / +0x10, walked two levels down); a GSK child (convex_pair_2ee2e0) holds
 * its contact count (+0x18), separating normal (+0x20) and TIM - the distance Havok believes you still have to it
 * (+0x2C). Its key names the triangle: category 1 a level surface, 5 a piece's (instance, surface). A child skipping
 * (TIM > 0, no contact) whose TIM is more than the real gap - its over-credit, as Cartographer measures it - past the
 * threshold is an OFFSET: red, and the pill lime. ---- */
#define KID_MAX 48
typedef struct { u32 key, agent; s32 manifold, k, state, skip, through, pre; float tim, over, gap, n[3], n0[3], pv[8][3], c[3]; } s_kid;   /* pre: its polygon placed already (baked scenery) */   /* state: 0 ok, 1 contact, 2 offset; n0: the TIM's side */
static s_kid g_kids[KID_MAX];
static s32 g_kid_n, g_kid_offsets, g_kid_leaves, g_kid_other;
static u32 g_kid_unknown[4];
static s32 g_kid_unknown_n;
static struct { u32 key, seen; s8 sign; } g_side[64];        /* a skipping TIM's side, kept while it lives */
static u32 g_side_frame;
static s32 g_said_offset_n;
static u32 g_said_offsets[16];
static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static float seg_seg_dist2(const float *p1, const float *q1, const float *p2, const float *q2)   /* Ericson 5.1.9 */
{
	float d1[3], d2[3], r[3], a, e, f, s, t, c, b, den, d[3];
	s32 k;
	for (k = 0; k < 3; k++) { d1[k] = q1[k] - p1[k]; d2[k] = q2[k] - p2[k]; r[k] = p1[k] - p2[k]; }
	a = dot3(d1, d1); e = dot3(d2, d2); f = dot3(d2, r);
	if (a <= 1e-12f && e <= 1e-12f) return dot3(r, r);
	if (a <= 1e-12f) { s = 0.f; t = clampf(f / e, 0.f, 1.f); }
	else
	{
		c = dot3(d1, r);
		if (e <= 1e-12f) { t = 0.f; s = clampf(-c / a, 0.f, 1.f); }
		else
		{
			b = dot3(d1, d2); den = a * e - b * b;
			s = den != 0.f ? clampf((b * f - c * e) / den, 0.f, 1.f) : 0.f;
			t = (b * s + f) / e;
			if (t < 0.f) { t = 0.f; s = clampf(-c / a, 0.f, 1.f); }
			else if (t > 1.f) { t = 1.f; s = clampf((b - c) / a, 0.f, 1.f); }
		}
	}
	for (k = 0; k < 3; k++) d[k] = (p1[k] + d1[k] * s) - (p2[k] + d2[k] * t);
	return dot3(d, d);
}
static void poly_normal(float (*pv)[3], float *n)
{
	float e1[3], e2[3], l;
	s32 k;
	for (k = 0; k < 3; k++) { e1[k] = pv[1][k] - pv[0][k]; e2[k] = pv[2][k] - pv[0][k]; }
	n[0] = e1[1] * e2[2] - e1[2] * e2[1]; n[1] = e1[2] * e2[0] - e1[0] * e2[2]; n[2] = e1[0] * e2[1] - e1[1] * e2[0];
	l = fsqrt(dot3(n, n)); if (l > 1e-12f) { n[0] /= l; n[1] /= l; n[2] /= l; }
}
/* p (in the plane) within the polygon - whichever way n points: every edge on the same side (a TIM's side normal is
 * the winding's or its reverse; 5c.4-5c.7 needed the winding's, so with the other "through" was never seen) */
static s32 in_poly(float (*pv)[3], s32 k, const float *n, const float *p)
{
	s32 i, j, pos = 0, neg = 0;
	for (i = 0; i < k; i++)
	{
		float *a = pv[i], *b = pv[(i + 1) % k], ab[3], ap[3], x[3], d;
		for (j = 0; j < 3; j++) { ab[j] = b[j] - a[j]; ap[j] = p[j] - a[j]; }
		x[0] = ab[1] * ap[2] - ab[2] * ap[1]; x[1] = ab[2] * ap[0] - ab[0] * ap[2]; x[2] = ab[0] * ap[1] - ab[1] * ap[0];
		d = dot3(x, n);
		if (d < -1e-6f) neg = 1; else if (d > 1e-6f) pos = 1;
	}
	return !(pos && neg);
}
static float seg_poly_dist(const float *A, const float *B, float (*pv)[3], s32 k)   /* Cartographer's seg_poly_dist */
{
	float n[3], da, db, best = 1e18f;
	s32 i, j, e;
	if (k < 3) return 1e9f;
	poly_normal(pv, n);
	{
		float a0[3], b0[3];
		for (j = 0; j < 3; j++) { a0[j] = A[j] - pv[0][j]; b0[j] = B[j] - pv[0][j]; }
		da = dot3(a0, n); db = dot3(b0, n);
	}
	if ((da > 0.f) != (db > 0.f) && (da - db > 1e-9f || db - da > 1e-9f))
	{
		float x[3];
		for (j = 0; j < 3; j++) x[j] = A[j] + (B[j] - A[j]) * (da / (da - db));
		if (in_poly(pv, k, n, x)) return 0.f;
	}
	for (e = 0; e < 2; e++)
	{
		const float *p = e ? B : A;
		float h, q[3], d0[3];
		for (j = 0; j < 3; j++) d0[j] = p[j] - pv[0][j];
		h = dot3(d0, n);
		for (j = 0; j < 3; j++) q[j] = p[j] - n[j] * h;
		if (in_poly(pv, k, n, q) && h * h < best) best = h * h;
	}
	for (i = 0; i < k; i++) { float d = seg_seg_dist2(A, B, pv[i], pv[(i + 1) % k]); if (d < best) best = d; }
	return fsqrt(best);
}
/* a key's polygon: category 1 a level surface; 5 an instanced piece's surface, placed with its instance */
static s32 key_polygon(u32 key, float (*pv)[3])
{
	u32 cat = key >> 29;
	float wn[3], tmp[16][3];
	s32 k, i, j;
	if (cat == 1)
	{
		s_xf id;
		id.on = 0;
		if (!cb_ready()) return 0;
		k = surf_poly(&g_cbm, &id, key & 0xFFFF, tmp, 0, wn);
	}
	else if (cat == 5)
	{
		u32 sb = M32(ADDR_STRUCTURE_BSP), inst = key & 0xFFFF, surf = (key >> 16) & 0x1FFF, ni, pi, nd, pd, in, di;
		s_xf x;
		s_cb c;
		if (!ptr_ok(sb)) return 0;
		ni = M32(sb + 0x140); pi = M32(sb + 0x144); nd = M32(sb + 0x138); pd = M32(sb + 0x13C);
		if (inst >= ni || ni > 8192 || !ptr_ok(pi) || !nd || nd > DEF_MAX || !ptr_ok(pd)) return 0;
		in = pi + inst * 0x58; di = M16(in + 0x34);
		if (di >= nd) return 0;
		if (g_def_state[di] == 0)
		{
			/* not checked yet (the seam search runs only with the Superbounce Viewer on): checked here, as it does */
			s_cb dc;
			cb_load(&dc, pd + di * 0xC8 + 0x70);
			g_def_state[di] = (!dc.ns || !dc.ne) ? 2 : cb_check(&dc) ? 3 : 1;
		}
		if (g_def_state[di] != 1) return 0;
		x.on = 1; x.sc = MF(in);
		for (i = 0; i < 3; i++) { x.fw[i] = MF(in + 0x04 + 4 * i); x.lf[i] = MF(in + 0x10 + 4 * i); x.up[i] = MF(in + 0x1C + 4 * i); x.pos[i] = MF(in + 0x28 + 4 * i); }
		cb_load(&c, pd + di * 0xC8 + 0x70);
		if (surf >= c.ns) return 0;                                         /* past the piece's surfaces */
		k = surf_poly(&c, &x, surf, tmp, 0, wn);
	}
	else return 0;
	if (k < 3) return 0;
	if (k > 8) k = 8;
	for (i = 0; i < k; i++) for (j = 0; j < 3; j++) pv[i][j] = tmp[i][j];
	return k;
}
/* ---- baked scenery (HCM's port of the level shape's child lookup - here function_1ef3e0): a level-welder entry whose key
 * is category 3 / 4 is a scenery object baked into the level's shape: key & 0xFFFF a scenery index ([ADDR_SCENERY_TABLE]
 * gives its object datum), (key >> 16) & 0x1F the model region, (key >> 21) & 0xFF the permutation. Category 4 - its
 * collision model: the entry's agent (a transform agent) holds the scenery's own welder, whose TIM leaves are keyed by
 * surface of that collision BSP - each placed with the object's node and read as a level triangle. Category 3 (its
 * physics model) is counted, not read yet. ---- */
static u32 g_sc3_key[16], g_sc3_agent[16];
static s32 g_sc3_n;                                /* this frame's category-3 entries (the objects walk them) */
static u32 g_scb_addr[16];
static u8 g_scb_ok[16];
static s32 g_scb_n, g_sc3_seen, g_sc4_seen;
static void sc_say(const char *why)
{
	static const char *said[8];
	static s32 said_n;
	s32 i, at;
	for (i = 0; i < said_n; i++) if (said[i] == why) return;
	if (said_n < 8) said[said_n++] = why;
	at = lput(0, "ghost: baked scenery - "); lput(at, why);
	log_write();
}
static s32 scenery_bsp(u32 key, s_cb *c, s_xf *x, u32 *obj_out)
{
	u32 tp = M32(ADDR_SCENERY_TABLE), idx = key & 0xFFFF, reg = (key >> 16) & 0x1F, perm = (key >> 21) & 0xFF;
	u32 obj, def, model, coll, nmr, pmr, mreg, npm, ppm, mperm, nreg, preg, creg, ncp, pcp, cperm, nb, pb, node, noff, m, bsp;
	s32 cri, cpi, i, k;
	if (!ptr_ok(tp) || idx > 8191) { sc_say("the scenery table not found"); return 0; }
	obj = object_from_datum((s32)M32(tp + 4 * idx));
	if (!obj) { sc_say("a scenery object not found (gone?)"); return 0; }
	def = tag_data(M32(obj), 0);
	model = def ? tag_data(M32(def + 0x38), 0x686C6D74u) : 0;              /* 'hlmt' */
	coll = model ? tag_data(M32(model + 0x0C), 0x636F6C6Cu) : 0;           /* 'coll' */
	if (!coll) { sc_say("a scenery object without a collision model"); return 0; }
	nmr = M32(model + 0x70); pmr = M32(model + 0x74);                      /* the model's regions, 16 bytes */
	if (reg >= nmr || nmr > 64 || !ptr_ok(pmr)) { sc_say("the model region out of range"); return 0; }
	mreg = pmr + reg * 16; npm = M32(mreg + 8); ppm = M32(mreg + 0xC);       /* its permutations, 8 bytes */
	if (perm >= npm || npm > 256 || !ptr_ok(ppm)) { sc_say("the model permutation out of range"); return 0; }
	mperm = ppm + perm * 8;
	cri = (s8)M8(mreg + 4); cpi = (s8)M8(mperm + 5);                       /* the collision region / permutation */
	nreg = M32(coll + 0x1C); preg = M32(coll + 0x20);
	if (cri < 0 || (u32)cri >= nreg || nreg > 64 || !ptr_ok(preg)) { sc_say("the collision region out of range"); return 0; }
	creg = preg + (u32)cri * 12; ncp = M32(creg + 4); pcp = M32(creg + 8);
	if (cpi < 0 || (u32)cpi >= ncp || ncp > 256 || !ptr_ok(pcp)) { sc_say("the collision permutation out of range"); return 0; }
	cperm = pcp + (u32)cpi * 20; nb = M32(cperm + 4); pb = M32(cperm + 8);
	if (!nb || nb > 64 || !ptr_ok(pb)) { sc_say("no collision BSP"); return 0; }
	node = M16(pb); noff = M16(obj + 0x116); bsp = pb + 4;
	if (!noff || noff > 0x4000 || node > 63) { sc_say("the node matrices"); return 0; }
	/* the BSP's layout checked once (a small cache) */
	for (i = 0; i < g_scb_n; i++) if (g_scb_addr[i] == bsp) break;
	if (i == g_scb_n)
	{
		cb_load(c, bsp);
		if (g_scb_n < 16) { g_scb_addr[g_scb_n] = bsp; g_scb_ok[g_scb_n] = (u8)(c->ns && c->ne && !cb_check(c)); i = g_scb_n++; }
		else if (!(c->ns && c->ne && !cb_check(c))) return 0;
		else i = -1;
	}
	else cb_load(c, bsp);
	if (i >= 0 && !g_scb_ok[i]) { sc_say("a collision BSP's layout check failed"); return 0; }
	m = obj + noff + node * 0x34;
	x->on = 1; x->sc = MF(m);
	for (k = 0; k < 3; k++) { x->fw[k] = MF(m + 0x04 + 4 * (u32)k); x->lf[k] = MF(m + 0x10 + 4 * (u32)k); x->up[k] = MF(m + 0x1C + 4 * (u32)k); x->pos[k] = MF(m + 0x28 + 4 * (u32)k); }
	if (!(x->sc > 0.01f && x->sc < 100.f)) x->sc = 1.f;
	*obj_out = obj;
	return 1;
}
static s32 read_leaf(u32 agent, u32 key, s_kid *c);
static void scenery_entry(u32 key, u32 agent)
{
	u32 cat = key >> 29, obj = 0, arr, sa;
	s_cb cb;
	s_xf x;
	s32 n, i, hops = 0;
	if (cat == 3) { if (g_sc3_n < 16) { g_sc3_key[g_sc3_n] = key; g_sc3_agent[g_sc3_n] = agent; g_sc3_n++; } return; }   /* walked with the objects */
	if (!scenery_bsp(key, &cb, &x, &obj)) return;
	/* the entry's agent: transform agents stepped through to the scenery's own welder (a collection) */
	sa = agent;
	while (hops < 3 && (M32(sa) == ADDR_VT_XFA || M32(sa) == ADDR_VT_XFB) && mem_ok(M32(sa + 0x0C))) { sa = M32(sa + 0x0C); hops++; }
	if (M32(sa) != ADDR_VT_WELD && M32(sa) != ADDR_VT_BVA && M32(sa) != ADDR_VT_BVB) { sc_say("its agent not a welder"); return; }
	arr = M32(sa + 0x0C); n = (s32)M32(sa + 0x10);
	if (!mem_ok(arr) || n <= 0 || n > 4096) return;
	if (!g_sc4_seen++)
	{
		s32 at = lput(0, "ghost: baked scenery found (category 4) - object "); at = lhex(at, obj); at = lput(at, ", collision BSP surfaces "); at = lnum(at, cb.ns);
		at = lput(at, ", entries "); lnum(at, (u32)n);
		log_write();
	}
	for (i = 0; i < n && g_kid_n < KID_MAX; i++)
	{
		u32 sk = M32(arr + (u32)i * 12), la = M32(arr + (u32)i * 12 + 8);
		s_kid *c = &g_kids[g_kid_n];
		float tmp[16][3], wn[3];
		s32 k, j, m;
		if (!ptr_ok(la) || (la >= ADDR_DATA_LO && la < ADDR_DATA_HI)) continue;
		{ s32 h2 = 0; while (h2 < 3 && (M32(la) == ADDR_VT_XFA || M32(la) == ADDR_VT_XFB) && mem_ok(M32(la + 0x0C))) { la = M32(la + 0x0C); h2++; } }
		if (!read_leaf(la, 0, c)) continue;
		k = surf_poly(&cb, &x, sk & 0xFFFF, tmp, 0, wn);                       /* a surface of the scenery's collision BSP */
		if (k < 3) continue;
		if (k > 8) k = 8;
		for (j = 0; j < k; j++) for (m = 0; m < 3; m++) c->pv[j][m] = tmp[j][m];
		c->k = k; c->pre = 1;
		c->key = 0x80000000u | ((key & 0xFFFF) << 15) | (sk & 0x7FFF);          /* unique per scenery surface (its side) */
		g_kid_n++; g_kid_leaves++;
	}
}
static s32 read_leaf(u32 agent, u32 key, s_kid *c)
{
	s32 m, k;
	u32 vt = M32(agent);
	if (vt != ADDR_VT_GSK && vt != ADDR_VT_SWEEP) return 0;             /* the GSK agent, or its subclass */
	m = (s32)M32(agent + 0x18); if (m < 0 || m > 16) m = 0;
	c->key = key; c->agent = agent; c->manifold = m; c->pre = 0;
	for (k = 0; k < 3; k++) c->n[k] = MF(agent + 0x20 + 4 * (u32)k);
	c->tim = MF(agent + 0x2C);
	if (!(c->n[0] == c->n[0] && c->n[1] == c->n[1] && c->n[2] == c->n[2])) { c->n[0] = 0.f; c->n[1] = 0.f; c->n[2] = 0.f; }
	return c->tim == c->tim && c->tim < 1e6f && c->tim > -1e6f;
}
/* only the collection class's list is walked: an agent of any other class is counted, never followed - its fields are
 * not a list (5b followed a leaf's +0x0C, which holds small ids, and crashed: no guarded reads on the Xbox) */
static void read_children(u32 agent, s32 depth)
{
	u32 arr, vt = M32(agent);
	s32 n, i;
	if (depth == 0 && g_kid_n < KID_MAX && read_leaf(agent, 0, &g_kids[g_kid_n])) { g_kid_n++; g_kid_leaves++; return; }
	if (vt != ADDR_VT_WELD && vt != ADDR_VT_BVA && vt != ADDR_VT_BVB) return;   /* only collection classes hold a list */
	arr = M32(agent + 0x0C); n = (s32)M32(agent + 0x10);
	if (n <= 0 || n > 4096 || !mem_ok(arr)) return;
	for (i = 0; i < n && g_kid_n < KID_MAX; i++)
	{
		u32 ca = M32(arr + (u32)i * 12 + 8), key = M32(arr + (u32)i * 12);
		if (!mem_ok(ca) || (ca >= ADDR_DATA_LO && ca < ADDR_DATA_HI)) continue;      /* none, or the null agent */
		if (depth == 0 && ((key >> 29) == 3 || (key >> 29) == 4)) { scenery_entry(key, ca); continue; }   /* baked scenery */
		{
			/* a transform agent (pair_agent_2ed170): one child agent at +0x0C - stepped into, with this entry's key */
			s32 hops = 0;
			while (hops < 3 && (M32(ca) == ADDR_VT_XFA || M32(ca) == ADDR_VT_XFB) && mem_ok(M32(ca + 0x0C))) { ca = M32(ca + 0x0C); hops++; }
		}
		if (read_leaf(ca, key, &g_kids[g_kid_n])) { g_kid_n++; g_kid_leaves++; continue; }
		g_kid_other++;
		{
			u32 vt = M32(ca); s32 j, seen = 0;
			for (j = 0; j < g_kid_unknown_n; j++) if (g_kid_unknown[j] == vt) seen = 1;
			if (!seen && g_kid_unknown_n < 4) g_kid_unknown[g_kid_unknown_n++] = vt;
		}
		if (depth < 2 && (M32(ca) == ADDR_VT_WELD || M32(ca) == ADDR_VT_BVA || M32(ca) == ADDR_VT_BVB)) read_children(ca, depth + 1);
	}
}
/* a skipping TIM's side: kept once you've been OVER its triangle - before that (your centre past the edge of it) the
 * side of its plane means nothing (a plane goes on forever: by a slant you can be under its extended plane), so it's
 * worked out fresh each frame and not stored (5c.4 stored it from the first sight: a believed pose in the floor) */
static s32 side_of(u32 key, s32 fresh_sign, s32 over_it)
{
	s32 i, free_i = -1;
	for (i = 0; i < 64; i++)
	{
		if (g_side[i].seen && g_side[i].key == key && g_side_frame - g_side[i].seen <= 2) { g_side[i].seen = g_side_frame; return g_side[i].sign; }   /* only while it lives */
		if (free_i < 0 && (!g_side[i].seen || g_side_frame - g_side[i].seen > 2)) free_i = i;
	}
	if (over_it && free_i >= 0) { g_side[free_i].key = key; g_side[free_i].sign = (s8)fresh_sign; g_side[free_i].seen = g_side_frame; }
	return fresh_sign;
}
/* is your centre over the polygon: put onto its plane, within it by your radius */
static s32 over_poly(const s_kid *c, const float *n, const float *mid, float r)
{
	float q[3], e[3], m0[3], h, l;
	s32 k;
	for (k = 0; k < 3; k++) m0[k] = mid[k] - c->pv[0][k];
	h = dot3(m0, n);
	for (k = 0; k < 3; k++) q[k] = mid[k] - n[k] * h;
	if (in_poly((float (*)[3])c->pv, c->k, n, q)) return 1;
	for (k = 0; k < 3; k++) e[k] = c->c[k] - q[k];
	l = fsqrt(dot3(e, e));
	if (l > 1e-4f) { float t = r < l ? r : l; for (k = 0; k < 3; k++) q[k] += e[k] * (t / l); }
	return in_poly((float (*)[3])c->pv, c->k, n, q);
}
static void tims_update(u32 agent, const float *prev, const float *cur)
{
	float a[3], b[3], mid[3], over_min = (float)o_offset_min * 0.001f;
	s32 i, k, j;
	g_kid_n = 0; g_kid_offsets = 0; g_kid_leaves = 0; g_kid_other = 0; g_kid_unknown_n = 0; g_side_frame++; g_sc3_n = 0;
	if (!mem_ok(agent)) return;
	read_children(agent, 0);
	for (k = 0; k < 3; k++) { a[k] = g_gp.cap.a[k]; b[k] = g_gp.cap.b[k]; mid[k] = (a[k] + b[k]) * 0.5f; }
	for (i = 0; i < g_kid_n; i++)
	{
		s_kid *c = &g_kids[i];
		float n0[3], d[3], skipping;
		if (!c->pre) c->k = key_polygon(c->key, c->pv);
		c->state = 0; c->over = 0.f;
		if (!c->k)
		{
			/* a key this reader can't place (the level is category 1, pieces 5): each category said once, with an example */
			static u32 said_cat;
			u32 cat = c->key >> 29;
			if (!(said_cat & (1u << cat)))
			{
				s32 at;
				said_cat |= 1u << cat;
				at = lput(0, "ghost TIMs: a key of category "); at = lnum(at, cat); at = lput(at, " not placed - e.g. key "); at = lhex(at, c->key);
				at = lput(at, ", TIM x1000 "); at = lsnum(at, (s32)(c->tim * 1000.f)); at = lput(at, ", contacts "); lnum(at, (u32)c->manifold);
				if (o_diag) log_write();   /* 6h.21 */
			}
			continue;
		}
		for (k = 0; k < 3; k++) { c->c[k] = 0.f; for (j = 0; j < c->k; j++) c->c[k] += c->pv[j][k]; c->c[k] /= (float)c->k; d[k] = mid[k] - c->c[k]; }
		poly_normal(c->pv, n0);
		skipping = c->tim > 0.f && c->manifold == 0;
		c->skip = skipping != 0.f;
		if (c->manifold > 0) c->state = 1;
		if (skipping)
		{
			if (side_of(c->key, dot3(n0, d) < 0.f ? -1 : 1, over_poly(c, n0, mid, g_gp.cap.r)) < 0) { n0[0] = -n0[0]; n0[1] = -n0[1]; n0[2] = -n0[2]; }
		}
		else if (dot3(n0, d) < 0.f) { n0[0] = -n0[0]; n0[1] = -n0[1]; n0[2] = -n0[2]; }
		if (skipping)
		{
			/* the centre's height above the plane on the TIM's side; the real gap while still on that side, else how far
			 * the capsule is through; over-credit = TIM - (gap - 1.1 x tolerance), less this tick's motion toward it */
			float m0[3], ab[3], sh, half, true_dist, over;
			s32 through = 0;
			for (k = 0; k < 3; k++) { m0[k] = mid[k] - c->pv[0][k]; ab[k] = a[k] - b[k]; }
			sh = dot3(m0, n0);
			half = g_gp.cap.r + 0.5f * (dot3(ab, n0) < 0.f ? -dot3(ab, n0) : dot3(ab, n0));
			if (sh < 0.f)
			{
				/* past the triangle's plane: through the triangle only if your centre, put onto its plane, is within it (by
				 * your radius) - beside a wall or slope you cross its plane without passing through it, and the TIM is still
				 * an honest distance (counting it as "through" made big false offsets, believed poses far away) */
				float q[3], cen[3], e[3], l;
				for (k = 0; k < 3; k++) q[k] = mid[k] - n0[k] * sh;
				through = in_poly(c->pv, c->k, n0, q);
				if (!through)
				{
					for (k = 0; k < 3; k++) { cen[k] = c->c[k]; e[k] = cen[k] - q[k]; }
					l = fsqrt(dot3(e, e));
					if (l > 1e-4f) { float t = g_gp.cap.r < l ? g_gp.cap.r : l; for (k = 0; k < 3; k++) q[k] += e[k] * (t / l); }
					through = in_poly(c->pv, c->k, n0, q);                     /* within the radius of it */
				}
			}
			true_dist = through ? sh - half : seg_poly_dist(a, b, c->pv, c->k) - g_gp.cap.r;
			c->through = through; c->gap = true_dist;
			over = (sh < 0.f || true_dist > -0.5f) ? c->tim - (true_dist - 1.1f * 0.0328f) : 0.f;
			if (prev && cur)
			{
				float st[3] = { cur[0] - prev[0], cur[1] - prev[1], cur[2] - prev[2] };
				if (dot3(st, st) < 1.f && over != 0.f) over += dot3(st, n0);
			}
			if (!(over == over)) over = 0.f;                                      /* not a number: no offset */
			c->over = over;
			if (over > over_min) { c->state = 2; g_kid_offsets++; }
		}
		for (k = 0; k < 3; k++) c->n0[k] = n0[k];
		if (c->state == 2)
		{
			/* the log, once per offset episode: the key, its TIM and over-credit (x1000) */
			s32 seen = 0, at;
			for (j = 0; j < g_said_offset_n; j++) if (g_said_offsets[j] == c->key) seen = 1;
			if (!seen)
			{
				if (g_said_offset_n < 16) g_said_offsets[g_said_offset_n++] = c->key;
				at = lput(0, "ghost offset: key "); at = lhex(at, c->key); at = lput(at, ", TIM x1000 "); at = lsnum(at, (s32)(c->tim * 1000.f));
				at = lput(at, ", over-credit x1000 "); at = lsnum(at, (s32)(c->over * 1000.f));
				at = lput(at, ", real gap x1000 "); at = lsnum(at, (s32)(c->gap * 1000.f)); lput(at, c->through ? ", through it" : "");
				if (o_diag) log_write();   /* 6h.21 */
			}
		}
	}
	if (!g_kid_offsets) g_said_offset_n = 0;                       /* the episode over: the next is logged again */
	{
		/* the diagnostics, when the classes change (the pair agent's, the unrecognised children's) - not every count */
		static u32 said_sig = 1;
		u32 sig = M32(agent) * 31u;
		s32 q;
		for (q = 0; q < g_kid_unknown_n; q++) sig = sig * 131u + g_kid_unknown[q];
		if (sig != said_sig)
		{
			s32 at, u;
			said_sig = sig;
			at = lput(0, "ghost TIMs: pair agent vtable "); at = lhex(at, M32(agent)); at = lput(at, ", entries "); at = lnum(at, M32(agent + 0x10));
			at = lput(at, ", TIM leaves "); at = lnum(at, (u32)g_kid_leaves); at = lput(at, ", other children "); at = lnum(at, (u32)g_kid_other);
			for (u = 0; u < g_kid_unknown_n; u++) { at = lput(at, " [vt "); at = lhex(at, g_kid_unknown[u]); at = lput(at, "]"); }
			if (o_diag) log_write();   /* 6h.21 */
		}
	}
}
/* ---- Ghost Viewer, objects (Cartographer's object TIMs / believed poses / travel regions). Your log: every object pair
 * is one shape of type 23 - Halo's convex sweep shape: a child (+0x30) extended by a sweep vector (+0x10) - under a
 * single TIM leaf agent. The child is a box (6: half extents +0x10, radius +0x0C), capsule (7), sphere (4), triangle
 * (5), or a transform (21: child +0x0C under +0x10) of one. Its real gap to your capsule: our own GJK distance between
 * your capsule's core and the swept piece (support points from the shapes' data - no call into Havok), less your
 * radius and the piece's. Over-credit and the side as Cartographer's (the side kept per agent). ---- */
typedef struct
{
	s32 kind;                                   /* 0 box, 1 sphere, 2 capsule, 3 triangle, 4 convex hull */
	float c[3], ax[3][3], he[3], a[3], b[3], v[3][3], sw[3], r;
	s32 nv;                                     /* the hull's vertices, in the world */
	float hv[64][3];
} s_piece;
typedef struct { u32 agent, key; s32 manifold, state, skip; float tim, over, gap, n[3], mn[3], mx[3]; s_piece pc; } s_okid;
#define OKID_MAX 16
static s_okid g_ok[OKID_MAX];
static s32 g_ok_n, g_obj_offsets;
static struct { u32 agent, seen; s8 sign; } g_oside[16];
static u32 g_said_obj[8];
static s32 g_said_obj_n;
static void piece_support(const s_piece *p, const float *d, float *out)
{
	s32 k, i;
	if (p->kind == 0)
	{
		for (k = 0; k < 3; k++) out[k] = p->c[k];
		for (i = 0; i < 3; i++) { float sg = dot3(d, p->ax[i]) >= 0.f ? p->he[i] : -p->he[i]; for (k = 0; k < 3; k++) out[k] += p->ax[i][k] * sg; }
	}
	else if (p->kind == 1) for (k = 0; k < 3; k++) out[k] = p->c[k];
	else if (p->kind == 2) { const float *q = dot3(d, p->a) >= dot3(d, p->b) ? p->a : p->b; for (k = 0; k < 3; k++) out[k] = q[k]; }
	else if (p->kind == 3) { s32 best = 0; float bd = dot3(d, p->v[0]); for (i = 1; i < 3; i++) { float t = dot3(d, p->v[i]); if (t > bd) { bd = t; best = i; } } for (k = 0; k < 3; k++) out[k] = p->v[best][k]; }
	else { s32 best = 0; float bd = dot3(d, p->hv[0]); for (i = 1; i < p->nv; i++) { float t = dot3(d, p->hv[i]); if (t > bd) { bd = t; best = i; } } for (k = 0; k < 3; k++) out[k] = p->hv[best][k]; }
	if (dot3(d, p->sw) > 0.f) for (k = 0; k < 3; k++) out[k] += p->sw[k];          /* swept along +sw */
}
/* the closest point to the origin on a simplex of 1-4 points (Ericson ch. 5), keeping only the points it needs */
static void simplex_closest(float (*w)[3], s32 *n, float *v)
{
	s32 k;
	if (*n == 1) { for (k = 0; k < 3; k++) v[k] = w[0][k]; return; }
	if (*n == 2)
	{
		float ab[3], t, l2;
		for (k = 0; k < 3; k++) ab[k] = w[1][k] - w[0][k];
		l2 = dot3(ab, ab); t = l2 > 1e-12f ? -dot3(w[0], ab) / l2 : 0.f;
		if (t <= 0.f) { *n = 1; for (k = 0; k < 3; k++) v[k] = w[0][k]; return; }
		if (t >= 1.f) { *n = 1; for (k = 0; k < 3; k++) { w[0][k] = w[1][k]; v[k] = w[1][k]; } return; }
		for (k = 0; k < 3; k++) v[k] = w[0][k] + ab[k] * t;
		return;
	}
	if (*n == 3)
	{
		/* ClosestPtPointTriangle with p = origin */
		float *a = w[0], *b = w[1], *c = w[2], ab[3], ac[3], ap[3], bp[3], cp[3], d1, d2, d3, d4, d5, d6, va, vb, vc, den, vv, ww;
		for (k = 0; k < 3; k++) { ab[k] = b[k] - a[k]; ac[k] = c[k] - a[k]; ap[k] = -a[k]; bp[k] = -b[k]; cp[k] = -c[k]; }
		d1 = dot3(ab, ap); d2 = dot3(ac, ap);
		if (d1 <= 0.f && d2 <= 0.f) { *n = 1; for (k = 0; k < 3; k++) v[k] = a[k]; return; }
		d3 = dot3(ab, bp); d4 = dot3(ac, bp);
		if (d3 >= 0.f && d4 <= d3) { *n = 1; for (k = 0; k < 3; k++) { w[0][k] = b[k]; v[k] = b[k]; } return; }
		vc = d1 * d4 - d3 * d2;
		if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f) { float t = (d1 - d3) != 0.f ? d1 / (d1 - d3) : 0.f; *n = 2; for (k = 0; k < 3; k++) v[k] = a[k] + ab[k] * t; return; }
		d5 = dot3(ab, cp); d6 = dot3(ac, cp);
		if (d6 >= 0.f && d5 <= d6) { *n = 1; for (k = 0; k < 3; k++) { w[0][k] = c[k]; v[k] = c[k]; } return; }
		vb = d5 * d2 - d1 * d6;
		if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f) { float t = (d2 - d6) != 0.f ? d2 / (d2 - d6) : 0.f; *n = 2; for (k = 0; k < 3; k++) { w[1][k] = c[k]; v[k] = a[k] + ac[k] * t; } return; }
		va = d3 * d6 - d5 * d4;
		if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
		{
			float sum = (d4 - d3) + (d5 - d6), t = sum != 0.f ? (d4 - d3) / sum : 0.f;
			*n = 2; for (k = 0; k < 3; k++) { v[k] = b[k] + (c[k] - b[k]) * t; w[0][k] = b[k]; w[1][k] = c[k]; }
			return;
		}
		if (va + vb + vc == 0.f) { *n = 1; for (k = 0; k < 3; k++) v[k] = a[k]; return; }   /* a degenerate triangle */
		den = 1.f / (va + vb + vc); vv = vb * den; ww = vc * den;
		for (k = 0; k < 3; k++) v[k] = a[k] + ab[k] * vv + ac[k] * ww;
		return;
	}
	{
		/* a tetrahedron: the origin inside (distance 0), else the closest of its faces */
		static const s32 face[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };
		float best = 1e30f, bv[3] = { 0, 0, 0 }, bw[3][3];
		s32 f, bn = 0, inside = 1;
		for (f = 0; f < 4; f++)
		{
			float tw[3][3], tv[3], e1[3], e2[3], nn[3], d0, d1;
			s32 tn = 3, o = 6 - face[f][0] - face[f][1] - face[f][2], j;
			for (j = 0; j < 3; j++) for (k = 0; k < 3; k++) tw[j][k] = w[face[f][j]][k];
			for (k = 0; k < 3; k++) { e1[k] = tw[1][k] - tw[0][k]; e2[k] = tw[2][k] - tw[0][k]; }
			nn[0] = e1[1] * e2[2] - e1[2] * e2[1]; nn[1] = e1[2] * e2[0] - e1[0] * e2[2]; nn[2] = e1[0] * e2[1] - e1[1] * e2[0];
			{ float t0[3], t1[3]; for (k = 0; k < 3; k++) { t0[k] = -tw[0][k]; t1[k] = w[o][k] - tw[0][k]; } d0 = dot3(t0, nn); d1 = dot3(t1, nn); }
			if (d0 * d1 < 0.f) inside = 0;                                         /* the origin outside this face */
			simplex_closest(tw, &tn, tv);
			if (dot3(tv, tv) < best) { best = dot3(tv, tv); bn = tn; for (k = 0; k < 3; k++) bv[k] = tv[k]; for (j = 0; j < tn; j++) for (k = 0; k < 3; k++) bw[j][k] = tw[j][k]; }
		}
		if (inside) { for (k = 0; k < 3; k++) v[k] = 0.f; return; }
		*n = bn; for (k = 0; k < 3; k++) v[k] = bv[k];
		{ s32 j; for (j = 0; j < bn; j++) for (k = 0; k < 3; k++) w[j][k] = bw[j][k]; }
	}
}
/* the distance between your capsule's core [a, b] and the swept piece (its core: before its radius) - GJK */
static float gjk_dist(const float *a, const float *b, const s_piece *p)
{
	float w[4][3], v[3], d[3], sa[3], sb[3], q[3];
	s32 n = 0, it, k;
	piece_support(p, a, sb);
	for (k = 0; k < 3; k++) v[k] = a[k] - sb[k];
	for (it = 0; it < 32; it++)
	{
		float vv = dot3(v, v), vw;
		s32 j, dup = 0;
		if (vv < 1e-12f) return 0.f;
		for (k = 0; k < 3; k++) d[k] = -v[k];
		{ const float *e = dot3(d, a) >= dot3(d, b) ? a : b; for (k = 0; k < 3; k++) sa[k] = e[k]; }   /* the segment's support */
		for (k = 0; k < 3; k++) q[k] = -d[k];
		piece_support(p, q, sb);
		{ float wn[3]; for (k = 0; k < 3; k++) wn[k] = sa[k] - sb[k]; vw = dot3(v, wn);
		  if (!(vv == vv)) return 1e9f;                                             /* not a number: not measured */
		  if (vv - vw <= 1e-6f * vv + 1e-9f) return fsqrt(vv);                     /* converged */
		  for (j = 0; j < n; j++) if (wn[0] == w[j][0] && wn[1] == w[j][1] && wn[2] == w[j][2]) dup = 1;
		  if (dup || n >= 4) return fsqrt(vv);
		  for (k = 0; k < 3; k++) w[n][k] = wn[k]; n++; }
		simplex_closest(w, &n, v);
	}
	{ float r = fsqrt(dot3(v, v)); return r == r ? r : 1e9f; }                 /* not a number: not measured */
}
/* an object body's shape, as a piece in the world: 23 (sweep: + its vector, child +0x30) and 21 (transform: child
 * +0x0C under +0x10) followed down (up to 4), to a box (6), sphere (4), capsule (7), triangle (5) or convex hull (8:
 * vertex blocks of four [+0x30], +0x34 of them - the physics model's polyhedra) */
static s32 read_piece(u32 shape, float (*col)[3], const float *t, s_piece *p, s32 *child_type)
{
	float tc[3][3], tt[3], sw[3] = { 0, 0, 0 };
	s32 st, k, i, depth;
	for (i = 0; i < 3; i++) for (k = 0; k < 3; k++) tc[i][k] = col[i][k];
	for (k = 0; k < 3; k++) tt[k] = t[k];
	*child_type = -1;
	for (depth = 0; depth < 4; depth++)
	{
		if (!mem_ok(shape)) return 0;
		st = shape_type(shape);
		*child_type = st;
		if (st == 23)
		{
			float l[3];
			for (k = 0; k < 3; k++) l[k] = MF(shape + 0x10 + 4 * (u32)k);
			for (k = 0; k < 3; k++) sw[k] += tc[0][k] * l[0] + tc[1][k] * l[1] + tc[2][k] * l[2];
			shape = M32(shape + 0x30);
			continue;
		}
		if (st == 19) { shape = M32(shape + 0x0C); continue; }                 /* a wrapper (0x13): its child */
		if (st == 21)
		{
			float c2[3][3], t2[3], nc[3][3], nt[3];
			for (i = 0; i < 3; i++) for (k = 0; k < 3; k++) c2[i][k] = MF(shape + 0x10 + (u32)(i * 16 + k * 4));
			for (k = 0; k < 3; k++) t2[k] = MF(shape + 0x40 + 4 * (u32)k);
			for (i = 0; i < 3; i++) for (k = 0; k < 3; k++) nc[i][k] = tc[0][k] * c2[i][0] + tc[1][k] * c2[i][1] + tc[2][k] * c2[i][2];
			for (k = 0; k < 3; k++) nt[k] = tc[0][k] * t2[0] + tc[1][k] * t2[1] + tc[2][k] * t2[2] + tt[k];
			for (i = 0; i < 3; i++) for (k = 0; k < 3; k++) tc[i][k] = nc[i][k];
			for (k = 0; k < 3; k++) tt[k] = nt[k];
			shape = M32(shape + 0x0C);
			continue;
		}
		break;
	}
	if (depth >= 4) return 0;
	for (k = 0; k < 3; k++) p->sw[k] = sw[k];
	p->r = MF(shape + 0x0C);
	if (!(p->r >= 0.f && p->r < 2.f)) p->r = 0.f;
	if (st == 6)
	{
		p->kind = 0;
		for (k = 0; k < 3; k++) { p->he[k] = MF(shape + 0x10 + 4 * (u32)k); p->c[k] = tt[k]; for (i = 0; i < 3; i++) p->ax[i][k] = tc[i][k]; }
		return p->he[0] >= 0.f && p->he[0] < 100.f && p->he[1] >= 0.f && p->he[1] < 100.f && p->he[2] >= 0.f && p->he[2] < 100.f;
	}
	if (st == 4) { p->kind = 1; for (k = 0; k < 3; k++) p->c[k] = tt[k]; return 1; }
	if (st == 7 || st == 5)
	{
		float l[3][3];
		s32 m = st == 7 ? 2 : 3;
		for (i = 0; i < m; i++) for (k = 0; k < 3; k++) l[i][k] = MF(shape + 0x10 + (u32)(i * 16 + k * 4));
		for (i = 0; i < m; i++) for (k = 0; k < 3; k++) { float wv = tc[0][k] * l[i][0] + tc[1][k] * l[i][1] + tc[2][k] * l[i][2] + tt[k]; if (st == 7) (i ? p->b : p->a)[k] = wv; else p->v[i][k] = wv; }
		p->kind = st == 7 ? 2 : 3;
		if (st == 5) p->r = 0.f;
		return 1;
	}
	if (st == 8)
	{
		u32 blk = M32(shape + 0x30);
		s32 nb = (s32)M32(shape + 0x34), j;
		if (!mem_ok(blk) || nb <= 0 || nb > 16) return 0;
		p->kind = 4; p->nv = 0;
		for (j = 0; j < nb; j++)
			for (i = 0; i < 4 && p->nv < 64; i++)
			{
				float l[3];
				u32 bb = blk + (u32)j * 0x30;
				l[0] = MF(bb + 4 * (u32)i); l[1] = MF(bb + 0x10 + 4 * (u32)i); l[2] = MF(bb + 0x20 + 4 * (u32)i);
				if (!(l[0] == l[0] && l[0] < 1e4f && l[0] > -1e4f && l[1] < 1e4f && l[1] > -1e4f && l[2] < 1e4f && l[2] > -1e4f)) return 0;
				for (k = 0; k < 3; k++) p->hv[p->nv][k] = tc[0][k] * l[0] + tc[1][k] * l[1] + tc[2][k] * l[2] + tt[k];
				p->nv++;
			}
		return p->nv > 0;
	}
	return 0;
}
static void piece_box(const s_piece *p, float *mn, float *mx)       /* its world box from six support points + radius */
{
	s32 k;
	for (k = 0; k < 3; k++)
	{
		float d[3] = { 0, 0, 0 }, o[3];
		d[k] = 1.f; piece_support(p, d, o); mx[k] = o[k] + p->r;
		d[k] = -1.f; piece_support(p, d, o); mn[k] = o[k] - p->r;
	}
}

static s32 g_obj_stage;                          /* where in an object pair (a caught fault says) */
static u32 g_op_key;                             /* the piece's path inside its object (list / BV-tree entries): with its shape and
                                                    place, what the offset save state knows it by after a revert */
/* one piece of an object: its TIM leaf (agent) and its shape under a transform - measured into the nearest-16 list */
static s_okid g_ok_spare;
static s32 g_said_shape_n;
static u32 g_said_shape[8];
static void obj_piece(u32 agent, const s_kid *leaf, u32 shape, float (*col)[3], const float *t, const float *a, const float *b,
	const float *mid, const float *prev, const float *cur, float over_min)
{
	s_okid *o = g_ok_n < OKID_MAX ? &g_ok[g_ok_n] : &g_ok_spare;
	float pcen[3], d[3], n[3], l;
	s32 ct = -1, k, i;
	g_obj_stage = 5;
	if (!read_piece(shape, col, t, &o->pc, &ct))
	{
		/* a piece this port doesn't measure: logged once, never called into */
		s32 j, seen = 0, at;
		for (j = 0; j < g_said_shape_n; j++) if (g_said_shape[j] == (u32)ct) seen = 1;
		if (!seen && g_said_shape_n < 8)
		{
			g_said_shape[g_said_shape_n++] = (u32)ct;
			at = lput(0, "ghost object piece: shape type "); at = lsnum(at, ct); lput(at, " not measured (box, capsule, sphere, triangle, convex hull are)");
			if (o_diag) log_write();   /* 6h.21 */
		}
		return;
	}
	o->agent = agent; o->key = g_op_key; o->manifold = leaf->manifold; o->tim = leaf->tim; o->state = leaf->manifold > 0 ? 1 : 0; o->over = 0.f;
	o->skip = leaf->tim > 0.f && leaf->manifold == 0;
	piece_box(&o->pc, o->mn, o->mx);
	for (k = 0; k < 3; k++) { pcen[k] = (o->mn[k] + o->mx[k]) * 0.5f; d[k] = mid[k] - pcen[k]; }
	/* the side: the cached normal (or toward you), kept per agent while its TIM lives (Cartographer's) */
	l = fsqrt(dot3(leaf->n, leaf->n));
	if (l > 0.5f) for (k = 0; k < 3; k++) n[k] = leaf->n[k] / l;
	else { l = fsqrt(dot3(d, d)); for (k = 0; k < 3; k++) n[k] = l > 1e-6f ? d[k] / l : (k == 2 ? 1.f : 0.f); }
	{
		s32 sign = dot3(n, d) < 0.f ? -1 : 1, found = 0, free_i = -1;
		if (o->skip)
		{
			for (i = 0; i < 16; i++)
			{
				if (g_oside[i].seen && g_oside[i].agent == agent && g_side_frame - g_oside[i].seen <= 2) { g_oside[i].seen = g_side_frame; sign = g_oside[i].sign; found = 1; break; }
				if (free_i < 0 && (!g_oside[i].seen || g_side_frame - g_oside[i].seen > 2)) free_i = i;
			}
			if (!found && free_i >= 0) { g_oside[free_i].agent = agent; g_oside[free_i].sign = (s8)sign; g_oside[free_i].seen = g_side_frame; }
		}
		if (sign < 0) for (k = 0; k < 3; k++) n[k] = -n[k];
	}
	for (k = 0; k < 3; k++) o->n[k] = n[k];
	g_obj_stage = 6;
	o->gap = gjk_dist(a, b, &o->pc) - g_gp.cap.r - o->pc.r;
	if (!(o->gap == o->gap) || o->gap > 1e8f) return;                       /* not measured */
	if (o->skip && o->gap > -1e-4f)                                         /* measured: a gap at / under 0 is a misread */
	{
		float over = o->tim - (o->gap - 1.1f * 0.0328f);
		if (prev && cur) { float st[3] = { cur[0] - prev[0], cur[1] - prev[1], cur[2] - prev[2] }; if (dot3(st, st) < 1.f && over != 0.f) over += dot3(st, n); }
		if (!(over == over)) over = 0.f;
		o->over = over;
		if (over > over_min)
		{
			s32 j, seen = 0, at;
			o->state = 2;
			for (j = 0; j < g_said_offset_n; j++) if (g_said_offsets[j] == agent) seen = 1;
			if (!seen)
			{
				if (g_said_offset_n < 16) g_said_offsets[g_said_offset_n++] = agent;
				at = lput(0, "ghost offset: object agent "); at = lhex(at, agent); at = lput(at, ", TIM x1000 "); at = lsnum(at, (s32)(o->tim * 1000.f));
				at = lput(at, ", over-credit x1000 "); at = lsnum(at, (s32)(over * 1000.f)); at = lput(at, ", real gap x1000 "); lsnum(at, (s32)(o->gap * 1000.f));
				if (o_diag) log_write();   /* 6h.21 */
			}
		}
	}
	{
		/* the first 24 objects measured, a line each: what piece, how near, its TIM - whether scenery gets this far */
		static u32 said_agent[24];
		static s32 said_n;
		s32 j, seen = 0, at;
		static const char *kind[5] = { "box", "sphere", "capsule", "triangle", "convex hull" };
		for (j = 0; j < said_n; j++) if (said_agent[j] == agent) seen = 1;
		if (!seen && said_n < 24 && o->gap < 3.f)
		{
			said_agent[said_n++] = agent;
			at = lput(0, "ghost object measured: "); at = lput(at, kind[o->pc.kind]);
			at = lput(at, ", box x100 "); at = lsnum(at, (s32)((o->mx[0] - o->mn[0]) * 100.f)); at = lput(at, " x "); at = lsnum(at, (s32)((o->mx[1] - o->mn[1]) * 100.f));
			at = lput(at, " x "); at = lsnum(at, (s32)((o->mx[2] - o->mn[2]) * 100.f));
			at = lput(at, ", gap x1000 "); at = lsnum(at, (s32)(o->gap * 1000.f)); at = lput(at, ", TIM x1000 "); at = lsnum(at, (s32)(o->tim * 1000.f));
			at = lput(at, ", contacts "); lnum(at, (u32)o->manifold);
			if (o_diag) log_write();   /* 6h.21 */
		}
	}
	if (g_ok_n < OKID_MAX) g_ok_n++;
	else
	{
		/* full: the farthest replaced if this one is nearer (weapons on a box could crowd the box out) */
		s32 worst = 0, j;
		for (j = 1; j < OKID_MAX; j++) if (g_ok[j].gap > g_ok[worst].gap) worst = j;
		if (g_ok_spare.gap < g_ok[worst].gap) g_ok[worst] = g_ok_spare;
	}
}
static void step_xform_agent(u32 *agent)                  /* through transform agents (pair_agent_2ed170: child +0x0C) */
{
	s32 hops = 0;
	while (hops < 3 && (M32(*agent) == ADDR_VT_XFA || M32(*agent) == ADDR_VT_XFB) && mem_ok(M32(*agent + 0x0C))) { *agent = M32(*agent + 0x0C); hops++; }
}
/* each object pair measured under its own guard: a pair whose data faults drops only itself (5d.7 dropped the whole
 * frame's overlay - next to the Cairo boxes, every frame) - and says once where (the step, its shape and agent) */
static u32 g_op_pair, g_op_best;
static float g_op_a[3], g_op_b[3], g_op_mid[3], g_op_prev[3], g_op_cur[3], g_op_over_min;
static s32 g_op_have_motion, g_op_tried;
static u32 g_said_fault[16];
static s32 g_said_fault_n, g_obj_faults;
static void obj_walk(u32 agent, u32 shape, float (*col0)[3], const float *t0, s32 depth)
{
	s_kid leaf;
	float col[3][3], t[3];
	const float *prev = g_op_have_motion ? g_op_prev : 0, *cur = g_op_have_motion ? g_op_cur : 0;
	s32 hops, k, i, m;
	if (depth > 3) return;
	for (m = 0; m < 3; m++) for (k = 0; k < 3; k++) col[m][k] = col0[m][k];
	for (k = 0; k < 3; k++) t[k] = t0[k];
	g_obj_stage = 2;
	for (hops = 0; hops < 4 && mem_ok(shape) && shape_type(shape) == 21; hops++)
	{
		float c2[3][3], t2[3], nc[3][3], nt[3];
		for (m = 0; m < 3; m++) for (k = 0; k < 3; k++) c2[m][k] = MF(shape + 0x10 + (u32)(m * 16 + k * 4));
		for (k = 0; k < 3; k++) t2[k] = MF(shape + 0x40 + 4 * (u32)k);
		for (m = 0; m < 3; m++) for (k = 0; k < 3; k++) nc[m][k] = col[0][k] * c2[m][0] + col[1][k] * c2[m][1] + col[2][k] * c2[m][2];
		for (k = 0; k < 3; k++) nt[k] = col[0][k] * t2[0] + col[1][k] * t2[1] + col[2][k] * t2[2] + t[k];
		for (m = 0; m < 3; m++) for (k = 0; k < 3; k++) col[m][k] = nc[m][k];
		for (k = 0; k < 3; k++) t[k] = nt[k];
		shape = M32(shape + 0x0C);
	}
	step_xform_agent(&agent);
	if (!mem_ok(shape)) return;
	if (M32(agent) == ADDR_VT_LIST && shape_type(shape) == 10)
	{
		u32 arr = M32(agent + 0x0C), ent = M32(shape + 0x0C);
		s32 cn = (s32)M32(agent + 0x10);
		g_obj_stage = 3;
		if (!mem_ok(arr) || !mem_ok(ent) || cn <= 0 || cn > 64) return;
		for (i = 0; i < cn && i < 16; i++)
		{
			u32 ca = M32(arr + 4 * (u32)i), cs = M32(ent + 8 * (u32)i), up = g_op_key;
			if (!ptr_ok(ca) || (ca >= ADDR_DATA_LO && ca < ADDR_DATA_HI) || !mem_ok(cs)) continue;   /* none / filtered */
			g_op_key = up * 0x9E3779B1u + (u32)i + 1u;
			obj_walk(ca, cs, col, t, depth + 1);
			g_op_key = up;
		}
		return;
	}
	if ((M32(agent) == ADDR_VT_BVA || M32(agent) == ADDR_VT_BVB || M32(agent) == ADDR_VT_WELD) && shape_type(shape) == 19)
	{
		/* a BV-tree agent over a wrapper around a list: {key, ?, agent} entries, the key the piece's index in the list */
		u32 list = M32(shape + 0x0C), arr = M32(agent + 0x0C), ent;
		s32 cn = (s32)M32(agent + 0x10), ln;
		g_obj_stage = 3;
		if (!mem_ok(list) || shape_type(list) != 10 || !mem_ok(arr) || cn <= 0 || cn > 256) return;
		ent = M32(list + 0x0C); ln = (s32)M32(list + 0x10);
		if (!mem_ok(ent)) return;
		for (i = 0; i < cn && i < 32; i++)
		{
			u32 key = M32(arr + 12 * (u32)i), ca = M32(arr + 12 * (u32)i + 8), cs;
			if (!ptr_ok(ca) || (ca >= ADDR_DATA_LO && ca < ADDR_DATA_HI)) continue;
			if (ln > 0 && ln <= 4096 && (key & 0xFFFF) >= (u32)ln) continue;
			cs = M32(ent + 8 * (key & 0xFFFF));
			if (!mem_ok(cs)) continue;
			{ u32 up = g_op_key; g_op_key = up * 0x9E3779B1u + key + 1u; obj_walk(ca, cs, col, t, depth + 1); g_op_key = up; }
		}
		return;
	}
	g_obj_stage = 4;
	if (!read_leaf(agent, 0, &leaf)) return;
	obj_piece(agent, &leaf, shape, col, t, g_op_a, g_op_b, g_op_mid, prev, cur, g_op_over_min);
}
static void obj_pair_one(void)
{
	u32 p = g_op_pair, agent = M32(p + 8), other;
	float col[3][3], t[3];
	g_obj_stage = 1;
	if (M32(p) == g_op_best) other = M32(p + 4); else if (M32(p + 4) == g_op_best) other = M32(p); else return;
	if (!ptr_ok(agent) || (agent >= ADDR_DATA_LO && agent < ADDR_DATA_HI) || !ptr_ok(other)) return;
	if (agent == g_level_agent) return;                                      /* the level (static scenery is not) */
	if (!body_pose(other, col, t)) return;
	g_op_tried++;
	g_op_key = 0;
	obj_walk(agent, M32(other - 0x10), col, t, 0);
}
/* category 3 - a scenery's physics model, as the game reads it (function_1ef500 / the shape builder 0x1d6560): the model
 * region's physics region (+5) and the choice's physics permutation (+6) -> 'phmo' (+0x24 of the model) regions [+0xC4]
 * (12 bytes, permutations [+8]) -> permutation (12 bytes: rigid body indices +4 / [+8]) -> rigid bodies [+0x3C], 0x90
 * each: node +0, shape [+0x38]. Placed with that node. */
static s32 scenery_phmo(u32 key, u32 *shape_out, float (*col)[3], float *t, u32 *obj_out)
{
	u32 tp = M32(ADDR_SCENERY_TABLE), idx = key & 0xFFFF, reg = (key >> 16) & 0x1F, perm = (key >> 21) & 0xFF;
	u32 obj, def, model, phmo, nmr, pmr, mreg, npm, ppm, mperm, ng, pg, grp, ne, pe, ent, ni, pi, nb, pb, rb, noff, m, shape;
	s32 pr, pp, rbi, node, k;
	if (!ptr_ok(tp) || idx > 8191) return 0;
	obj = object_from_datum((s32)M32(tp + 4 * idx));
	if (!obj) { sc_say("a scenery object not found (gone?)"); return 0; }
	def = tag_data(M32(obj), 0);
	model = def ? tag_data(M32(def + 0x38), 0x686C6D74u) : 0;              /* 'hlmt' */
	phmo = model ? tag_data(M32(model + 0x24), 0x70686D6Fu) : 0;           /* 'phmo' */
	if (!phmo) { sc_say("a category-3 scenery object without a physics model"); return 0; }
	nmr = M32(model + 0x70); pmr = M32(model + 0x74);
	if (reg >= nmr || nmr > 64 || !ptr_ok(pmr)) { sc_say("the model region out of range"); return 0; }
	mreg = pmr + reg * 16; npm = M32(mreg + 8); ppm = M32(mreg + 0xC);
	if (perm >= npm || npm > 256 || !ptr_ok(ppm)) { sc_say("the model permutation out of range"); return 0; }
	mperm = ppm + perm * 8;
	pr = (s8)M8(mreg + 5); pp = (s8)M8(mperm + 6);                         /* the physics region / permutation */
	ng = M32(phmo + 0xC0); pg = M32(phmo + 0xC4);
	if (pr < 0 || (u32)pr >= ng || ng > 64 || !ptr_ok(pg)) { sc_say("the physics region out of range"); return 0; }
	grp = pg + (u32)pr * 12; ne = M32(grp + 4); pe = M32(grp + 8);
	if (pp < 0 || (u32)pp >= ne || ne > 256 || !ptr_ok(pe)) { sc_say("the physics permutation out of range"); return 0; }
	ent = pe + (u32)pp * 12; ni = M32(ent + 4); pi = M32(ent + 8);
	if (!ni || ni > 256 || !ptr_ok(pi)) { sc_say("no rigid body for the permutation"); return 0; }
	rbi = (s16)M16(pi);
	nb = M32(phmo + 0x38); pb = M32(phmo + 0x3C);
	if (rbi < 0 || (u32)rbi >= nb || nb > 256 || !ptr_ok(pb)) { sc_say("the rigid body out of range"); return 0; }
	rb = pb + (u32)rbi * 0x90; shape = M32(rb + 0x38); node = (s16)M16(rb);
	if (!mem_ok(shape)) { sc_say("the rigid body's shape"); return 0; }
	if (node < 0) node = 0;
	noff = M16(obj + 0x116);
	if (!noff || noff > 0x4000 || node > 63) { sc_say("the node matrices"); return 0; }
	m = obj + noff + (u32)node * 0x34;
	for (k = 0; k < 3; k++) { col[0][k] = MF(m + 0x04 + 4 * (u32)k); col[1][k] = MF(m + 0x10 + 4 * (u32)k); col[2][k] = MF(m + 0x1C + 4 * (u32)k); t[k] = MF(m + 0x28 + 4 * (u32)k); }
	*shape_out = shape; *obj_out = obj;
	return 1;
}
static u32 g_sc3_cur;
static void sc3_one(void)                       /* one category-3 entry, under its own guard */
{
	u32 shape, obj;
	float col[3][3], t[3];
	g_obj_stage = 1;
	if (!scenery_phmo(g_sc3_key[g_sc3_cur], &shape, col, t, &obj)) return;
	if (!g_sc3_seen++)
	{
		s32 at = lput(0, "ghost: baked scenery found (category 3, physics model) - object "); at = lhex(at, obj);
		at = lput(at, ", shape type "); lsnum(at, shape_type(shape));
		log_write();
	}
	g_op_key = g_sc3_key[g_sc3_cur];
	obj_walk(g_sc3_agent[g_sc3_cur], shape, col, t, 0);
}
static void objects_update(u32 best_h, const float *prev, const float *cur)
{
	u32 entity = M32(best_h + 0x10), island = ptr_ok(entity) ? M32(entity + 0x44) : 0, pairs;
	s32 pn, q, k, i;
	g_ok_n = 0; g_obj_offsets = 0;
	if (!ptr_ok(island)) return;
	pairs = M32(island + 0x68); pn = (s32)M32(island + 0x6C);
	if (!ptr_ok(pairs) || pn <= 0 || pn > 8192) return;
	for (k = 0; k < 3; k++) { g_op_a[k] = g_gp.cap.a[k]; g_op_b[k] = g_gp.cap.b[k]; g_op_mid[k] = (g_op_a[k] + g_op_b[k]) * 0.5f; }
	g_op_have_motion = prev && cur;
	if (g_op_have_motion) for (k = 0; k < 3; k++) { g_op_prev[k] = prev[k]; g_op_cur[k] = cur[k]; }
	g_op_over_min = (float)o_offset_min * 0.001f; g_op_best = best_h; g_op_tried = 0;
	for (q = 0; q < pn && g_op_tried < 48; q++)
	{
		g_op_pair = pairs + (u32)q * 16;
		g_obj_stage = 0;
		if (guarded_call(obj_pair_one))
		{
			/* a bad read in this pair: it alone dropped; said once per (step, its shape's class, its agent's class) */
			u32 p = g_op_pair, other = M32(p) == best_h ? M32(p + 4) : M32(p), sv = 0, av = M32(p + 8), sig;
			s32 j, seen = 0, at;
			static const char *step[7] = { "pair", "pose", "transforms", "list", "TIM leaf", "piece", "distance" };
			g_obj_faults++;
			if (ptr_ok(other) && ptr_ok(M32(other - 0x10))) sv = M32(M32(other - 0x10));
			sig = (u32)g_obj_stage * 0x9E3779B1u ^ sv ^ (av * 3u);
			for (j = 0; j < g_said_fault_n; j++) if (g_said_fault[j] == sig) seen = 1;
			if (!seen && g_said_fault_n < 16)
			{
				g_said_fault[g_said_fault_n++] = sig;
				at = lput(0, "ghost object: a bad read caught at the "); at = lput(at, step[g_obj_stage < 7 ? g_obj_stage : 0]);
				at = lput(at, " step - shape class "); at = lhex(at, sv); at = lput(at, ", agent class "); at = lhex(at, av);
				at = lput(at, " - that object skipped (object faults so far "); at = lnum(at, (u32)g_obj_faults); lput(at, ")");
				log_write();
			}
		}
	}
	/* this frame's category-3 scenery (found in the level's welder): walked as objects */
	for (i = 0; i < g_sc3_n; i++)
	{
		g_sc3_cur = (u32)i;
		if (guarded_call(sc3_one)) { g_obj_faults++; sc_say("a bad read caught in a category-3 scenery (skipped)"); }
	}
	for (i = 0; i < g_ok_n; i++) if (g_ok[i].state == 2) g_obj_offsets++;
}
#ifdef HOST
float host_obj_min_gap(void) { s32 i; float m = 1e30f; for (i = 0; i < g_ok_n; i++) if (g_ok[i].gap < m) m = g_ok[i].gap; return m; }
void host_cam(float *p, float *f, float *r) { s32 k; for (k = 0; k < 3; k++) { p[k] = g_cam_p[k]; f[k] = g_cam_f[k]; } r[0] = g_cam_f[1]; r[1] = -g_cam_f[0]; r[2] = 0.f; }
void host_line3(const float *a, const float *b) { gfx_begin(1.f, 1.f, 1.f, 1.f); gfx_line3(a, b); gfx_end(); }
s32 host_objects(s32 *offsets, float *over0, float *gap0) { *offsets = g_obj_offsets; if (g_ok_n) { *over0 = g_ok[0].over; *gap0 = g_ok[0].gap; } return g_ok_n; }
float host_gjk_box(const float *a, const float *b, const float *c, const float *he, const float *sw)
{
	s_piece p; s32 k, i;
	p.kind = 0; p.r = 0.f;
	for (k = 0; k < 3; k++) { p.c[k] = c[k]; p.he[k] = he[k]; p.sw[k] = sw[k]; for (i = 0; i < 3; i++) p.ax[i][k] = i == k ? 1.f : 0.f; }
	return gjk_dist(a, b, &p);
}
#endif
/* ---- Offset save state (Cartographer's, step 6h): Save offsets snapshots every TIM you hold that is skipping (no contact,
 * TIM > 0) - level polygons by key, object pieces by shape, place and path - and asks for a checkpoint the normal way;
 * once the checkpoint has rebuilt your pairs (3 ticks on) the snapshot is written back, so you keep your offsets. Revert
 * offsets reverts the normal way, waits for game time to go back, checks you are where they were saved (within 0.05)
 * and writes them back. Written per pair: the cached separating normal (+0x20) and the TIM (+0x2C), corrected by any
 * movement since the save along that normal; never a pair in contact. Tried for 60 frames while the pairs come back.
 * With "Restore offsets after natural reverts" on, any revert that puts you back where they were saved (death, the
 * pause menu, Revert Checkpoint) restores them too. It reads your pairs through the Ghost Viewer, so needs it on. ---- */
#define OFS_MAX 64
typedef struct { u8 obj, kind; u32 key; s32 c[3]; float n[3], tim; } s_ofs;
static s_ofs g_ofs[OFS_MAX];
static u8 g_ofs_done[OFS_MAX];
static s32 g_ofs_n, g_ofs_valid, g_ofs_req, g_ofs_phase, g_ofs_load, g_ofs_frames, g_ofs_restored;   /* phase: 0 idle, 1 wait, 2 apply */
static float g_ofs_pos[3];
static u32 g_ofs_save_tick, g_ofs_req_tick, g_ofs_last_tick;
static s32 ofs_busy(void) { return g_ofs_req != 0 || g_ofs_phase != 0; }
static s32 ofs_need(void) { return ofs_busy() || (o_ofs_natural && g_ofs_valid); }   /* this frame's pairs read for it */
static s32 ofs_q(float f) { f *= 64.f; return (s32)(f < 0.f ? f - 0.5f : f + 0.5f); }   /* a place to 1/64 unit */
static void ofs_piece_c(const s_okid *o, s32 *c) { s32 k; for (k = 0; k < 3; k++) c[k] = ofs_q((o->mn[k] + o->mx[k]) * 0.5f); }
static void ofs_say(const char *a, s32 x, const char *b, s32 y, const char *tail)
{
	/* "<a><x><b><y><tail>" (y < 0: left out) - the toast and the log */
	char t[80];
	s32 n = 0, i, d;
	char digits[12];
	const char *parts[3] = { a, b, tail };
	s32 nums[2] = { x, y };
	for (i = 0; i < 3; i++)
	{
		const char *q = parts[i];
		while (q && *q && n < 78) t[n++] = *q++;
		if (i < 2 && nums[i] >= 0)
		{
			u32 v = (u32)nums[i]; d = 0;
			do { digits[d++] = (char)('0' + v % 10); v /= 10; } while (v && d < 11);
			while (d && n < 78) t[n++] = digits[--d];
		}
	}
	t[n] = 0;
	toast(t);
	lput(lput(0, "offset save state: "), t);
	log_write();
}
static void ofs_finish(void) { g_ofs_phase = 0; }
static u32 ofs_agent(const s_ofs *e)          /* the pair holding this entry now (this frame's children / pieces) */
{
	s32 i, c[3];
	if (!e->obj) { for (i = 0; i < g_kid_n; i++) if (g_kids[i].key == e->key && g_kids[i].agent) return g_kids[i].agent; return 0; }
	for (i = 0; i < g_ok_n; i++)
	{
		const s_okid *o = &g_ok[i];
		if (o->pc.kind != e->kind || o->key != e->key) continue;
		ofs_piece_c(o, c);
		if (c[0] - e->c[0] <= 1 && e->c[0] - c[0] <= 1 && c[1] - e->c[1] <= 1 && e->c[1] - c[1] <= 1 && c[2] - e->c[2] <= 1 && e->c[2] - c[2] <= 1) return o->agent;
	}
	return 0;
}
static void ofs_take(const float *pos, u32 tick)   /* Save offsets: the snapshot (this frame, before the game's save) */
{
	s32 i, k;
	s_kid live;
	g_ofs_n = 0;
	for (i = 0; i < g_kid_n + g_ok_n && g_ofs_n < OFS_MAX; i++)
	{
		s_ofs *e = &g_ofs[g_ofs_n];
		u32 agent = i < g_kid_n ? g_kids[i].agent : g_ok[i - g_kid_n].agent;
		if (!agent || !read_leaf(agent, 0, &live) || !(live.tim > 0.f) || live.manifold != 0) continue;   /* skipping ones only */
		e->obj = i >= g_kid_n;
		if (!e->obj) { e->kind = 0; e->key = g_kids[i].key; e->c[0] = e->c[1] = e->c[2] = 0; }
		else { const s_okid *o = &g_ok[i - g_kid_n]; e->kind = (u8)o->pc.kind; e->key = o->key; ofs_piece_c(o, e->c); }
		for (k = 0; k < 3; k++) e->n[k] = live.n[k];
		e->tim = live.tim;
		g_ofs_done[g_ofs_n] = 0;
		g_ofs_n++;
	}
	for (k = 0; k < 3; k++) g_ofs_pos[k] = pos[k];
	g_ofs_valid = 1; g_ofs_save_tick = tick; g_ofs_phase = 1; g_ofs_load = 0; g_ofs_frames = 0; g_ofs_restored = 0;
	ofs_say("Offsets saved: ", g_ofs_n, " TIMs - checkpoint made", -1, 0);
}
static void ofs_apply_start(s32 load)
{
	s32 i;
	g_ofs_phase = 2; g_ofs_load = load; g_ofs_frames = 0; g_ofs_restored = 0;
	for (i = 0; i < g_ofs_n; i++) g_ofs_done[i] = 0;
}
/* once a frame, from the Ghost Viewer's update (with a pair against the level): this frame's children and pieces read */
static void offsets_tick(u32 obj, u32 tick)
{
	float pos[3], dp[3], d2;
	s32 i, k;
	for (k = 0; k < 3; k++) { pos[k] = MF(obj + 0x64 + 4 * (u32)k); dp[k] = pos[k] - g_ofs_pos[k]; }
	d2 = dot3(dp, dp);
	if (g_ofs_req == 1) { g_ofs_req = 0; ofs_take(pos, tick); }
	else if (g_ofs_phase == 1)
	{
		/* Save: the checkpoint is taken in this frame's game update -> written back from 3 ticks on. Revert: once game
		 * time has gone back (the revert done) */
		s32 ready = g_ofs_load ? tick < g_ofs_req_tick : tick >= g_ofs_save_tick + 3;
		g_ofs_frames++;
		if (ready) { ofs_apply_start(g_ofs_load); }
		else if (g_ofs_frames > 300)
		{
			ofs_finish();
			if (g_ofs_load) ofs_say("Revert offsets: the revert didn't happen (nothing restored)", -1, 0, -1, 0);
			else ofs_say("Offsets saved (no checkpoint seen - offsets untouched)", -1, 0, -1, 0);
		}
	}
	else if (g_ofs_phase == 0 && o_ofs_natural && g_ofs_valid && g_ofs_last_tick && tick < g_ofs_last_tick && d2 < 0.05f * 0.05f)
		ofs_apply_start(1);                                                   /* a natural revert, back where they were saved */
	if (g_ofs_phase == 2)
	{
		g_ofs_frames++;
		if (g_ofs_load && g_ofs_frames == 1 && d2 > 0.05f * 0.05f)
		{
			ofs_finish();
			ofs_say("Revert offsets: not where they were saved (nothing restored)", -1, 0, -1, 0);
		}
		else
		{
			for (i = 0; i < g_ofs_n; i++)
			{
				const s_ofs *e = &g_ofs[i];
				s_kid live;
				u32 agent;
				float tim;
				if (g_ofs_done[i]) continue;
				agent = ofs_agent(e);
				if (!agent || !read_leaf(agent, 0, &live) || live.manifold > 0) continue;   /* never a pair in contact */
				tim = e->tim + dot3(e->n, dp);                                  /* less any movement toward it since the save */
				if (!(tim > 0.f)) continue;
				gwf(agent + 0x20, e->n[0]); gwf(agent + 0x24, e->n[1]); gwf(agent + 0x28, e->n[2]); gwf(agent + 0x2C, tim);
				g_ofs_done[i] = 1; g_ofs_restored++;
			}
			if (g_ofs_restored == g_ofs_n || g_ofs_frames >= 60)
			{
				ofs_finish();
				ofs_say(g_ofs_load ? "Offsets restored: " : "Offsets kept: ", g_ofs_restored, " of ", g_ofs_n,
					g_ofs_restored == g_ofs_n ? " TIMs" : " TIMs (the rest had no pair)");
			}
		}
	}
	g_ofs_last_tick = tick;
}
/* the menu's / White + D-pad's requests (practice actions 10, 11 - at the start of the frame, before the game's update) */
static void offsets_request(s32 save)
{
	if (!o_ghost) { toast(save ? "Save offsets: turn the Ghost Viewer on first" : "Revert offsets: turn the Ghost Viewer on first"); return; }
	if (!player_object()) { toast("Offsets: no game in progress"); return; }
	if (save)
	{
		/* the checkpoint asked for now - before the superbounce update, which notes a save for Maintain charge state - and
		 * the snapshot taken by this frame's Ghost Viewer update, both before the game's update saves */
		g_ofs_req = 1; g_ofs_phase = 0;
		gw8(ADDR_SAVE_MAP, 1);
		return;
	}
	if (!g_ofs_valid) { ofs_say("Revert offsets: nothing saved yet", -1, 0, -1, 0); return; }
	g_ofs_phase = 1; g_ofs_load = 1; g_ofs_frames = 0; g_ofs_restored = 0; g_ofs_req_tick = g_sb_tick;
	gw8(ADDR_SAVE_MAP - 1, 1);
	ofs_say("Revert offsets: reverting", -1, 0, -1, 0);
}
static void offsets_frame_end(void)          /* a save asked for that this frame's update couldn't read (no level pair) */
{
	if (g_ofs_req != 1) return;
	g_ofs_req = 0; g_ofs_valid = 0; g_ofs_phase = 0;
	ofs_say("Save offsets: no TIMs readable right now - checkpoint made", -1, 0, -1, 0);
}
#ifdef HOST
void host_set_ofs_natural(s32 on) { o_ofs_natural = on; }
s32 host_ofs(s32 *phase, s32 *restored) { *phase = g_ofs_phase; *restored = g_ofs_restored; return g_ofs_valid ? g_ofs_n : -1; }
#endif
/* ---- Ghost Viewer stage 3 (Cartographer's): BELIEVED POSES - for each skipping child over-credited past 0.005, your
 * capsule moved along its separating normal (on the TIM's side) by the over-credit: where Havok believes you are; the
 * largest first. TRAVEL REGIONS - for each skipping child (offsets first, six drawn): its weld zone (the triangle's
 * box grown by your capsule's half-size + 1.5 x the tolerance; your centre must stay inside) and its reset sheet (the
 * triangle moved to your centre - normal x TIM). ---- */
typedef struct { float over, dir[3]; } s_bel;
static s_bel g_bel[16];
static s32 g_bel_n, g_trav_n, g_trav_i[6], g_otrav_n, g_otrav_i[6];
static void kid_dir(const s_kid *c, float *d)    /* the separating normal on the TIM's side, else the plane's */
{
	float l = fsqrt(dot3(c->n, c->n));
	s32 k;
	if (l > 0.5f) { for (k = 0; k < 3; k++) d[k] = c->n[k] / l; if (dot3(d, c->n0) < 0.f) for (k = 0; k < 3; k++) d[k] = -d[k]; }
	else for (k = 0; k < 3; k++) d[k] = c->n0[k];
}
static void ghost_derive(void)
{
	s32 i, j, k, idx[KID_MAX], n = 0;
	(void)k;
	g_bel_n = 0; g_trav_n = 0;
	for (i = 0; i < g_kid_n; i++)
	{
		const s_kid *c = &g_kids[i];
		if (!c->k || !c->skip) continue;
		/* over 20 units is no real offset - a moment like a high-speed impact: left out of the drawing */
		if (o_believed && c->over > 0.005f && c->over < 20.f && g_bel_n < 16) { g_bel[g_bel_n].over = c->over; kid_dir(c, g_bel[g_bel_n].dir); g_bel_n++; }
		if (n < 12 && c->tim < 20.f && c->over < 20.f) idx[n++] = i;
	}
	if (o_obj_believed)                                                     /* the objects' too */
		for (i = 0; i < g_ok_n && g_bel_n < 16; i++)
			if (g_ok[i].skip && g_ok[i].over > 0.005f && g_ok[i].over < 20.f)
			{
				for (k = 0; k < 3; k++) g_bel[g_bel_n].dir[k] = g_ok[i].n[k];
				g_bel[g_bel_n].over = g_ok[i].over; g_bel_n++;
			}
	g_otrav_n = 0;
	if (o_obj_travel)
	{
		s32 oi[OKID_MAX], on = 0;
		for (i = 0; i < g_ok_n; i++) if (g_ok[i].skip && g_ok[i].tim < 20.f && g_ok[i].over < 20.f) oi[on++] = i;
		/* offsets first (the largest), then the nearest (5d.6 kept the list order - by a box covered in weapons, the six
		 * drawn could all be weapons) */
		for (i = 1; i < on; i++)
			for (j = i; j > 0; j--)
			{
				const s_okid *x = &g_ok[oi[j]], *y = &g_ok[oi[j - 1]];
				s32 xo = x->state == 2, yo = y->state == 2;
				if (xo > yo || (xo == yo && (xo ? x->over > y->over : x->gap < y->gap))) { k = oi[j]; oi[j] = oi[j - 1]; oi[j - 1] = k; }
				else break;
			}
		for (i = 0; i < on && i < 6; i++) g_otrav_i[g_otrav_n++] = oi[i];
	}
	for (i = 1; i < g_bel_n; i++)                                          /* the largest first */
		for (j = i; j > 0 && g_bel[j].over > g_bel[j - 1].over; j--) { s_bel t = g_bel[j]; g_bel[j] = g_bel[j - 1]; g_bel[j - 1] = t; }
	if (g_bel_n > o_bel_max) g_bel_n = o_bel_max;
	for (i = 1; i < n; i++)                                                 /* offsets first, the order kept otherwise */
		for (j = i; j > 0 && g_kids[idx[j]].over > g_kids[idx[j - 1]].over; j--) { k = idx[j]; idx[j] = idx[j - 1]; idx[j - 1] = k; }
	for (i = 0; i < n && i < 6; i++) g_trav_i[g_trav_n++] = idx[i];
}
/* a filled polygon in the world (its corners in front of the camera): a fan of quads */
static void gfx_fill3(float (*pv)[3], s32 k)
{
	float sx[8], sy[8], dep;
	s32 i;
	for (i = 0; i < k; i++)
	{
		if (view_depth(pv[i]) < 0.25f) return;                               /* the projection's own camera */
		project(pv[i], &sx[i], &sy[i], &dep);
		if (!(sx[i] > -4000.f && sx[i] < 4000.f && sy[i] > -4000.f && sy[i] < 4000.f)) return;   /* as gfx_line3: far off screen */
	}
	for (i = 1; i + 1 < k; i++)
		gfx_quad((s32)sx[0], (s32)sy[0], (s32)sx[i], (s32)sy[i], (s32)sx[i + 1], (s32)sy[i + 1], (s32)sx[i + 1], (s32)sy[i + 1]);
}
static void box3(const float *mn, const float *mx)        /* an axis-aligned box's 12 edges */
{
	s32 i;
	for (i = 0; i < 4; i++)
	{
		float a[3], b[3], c[3], d[3];
		a[0] = (i & 1) ? mx[0] : mn[0]; a[1] = (i & 2) ? mx[1] : mn[1]; a[2] = mn[2];
		b[0] = a[0]; b[1] = a[1]; b[2] = mx[2];
		gfx_line3(a, b);                                                    /* the vertical edges */
		c[0] = mn[0]; c[1] = (i & 1) ? mx[1] : mn[1]; c[2] = (i & 2) ? mx[2] : mn[2];
		d[0] = mx[0]; d[1] = c[1]; d[2] = c[2];
		gfx_line3(c, d);                                                    /* along x */
		c[0] = (i & 1) ? mx[0] : mn[0]; c[1] = mn[1]; c[2] = (i & 2) ? mx[2] : mn[2];
		d[0] = c[0]; d[1] = mx[1]; d[2] = c[2];
		gfx_line3(c, d);                                                    /* along y */
	}
}
static void draw_capsule(const s_cap *c, const float *shift);
static void ghost_extras_draw(void)
{
	float mid[3], cmn[3], cmx[3], cen[3], g[3];
	s32 i, k, j;
	for (k = 0; k < 3; k++)
	{
		mid[k] = (g_gp.cap.a[k] + g_gp.cap.b[k]) * 0.5f;
		cmn[k] = (g_gp.cap.a[k] < g_gp.cap.b[k] ? g_gp.cap.a[k] : g_gp.cap.b[k]) - g_gp.cap.r;
		cmx[k] = (g_gp.cap.a[k] > g_gp.cap.b[k] ? g_gp.cap.a[k] : g_gp.cap.b[k]) + g_gp.cap.r;
		cen[k] = (cmn[k] + cmx[k]) * 0.5f; g[k] = (cmx[k] - cmn[k]) * 0.5f + 1.5f * 0.0328f;
	}
	if (g_bel_n)                                /* the level's (Believed poses) and the objects' (Object believed poses) */
	{
		/* each believed pose: the capsule moved by direction x over-credit (cyan), and a line back to you */
		g_line_half = 0.8f;
		for (i = 0; i < g_bel_n; i++)
		{
			float d[3], p0[3], p1[3];
			for (k = 0; k < 3; k++) { d[k] = g_bel[i].dir[k] * g_bel[i].over; p0[k] = mid[k] + d[k]; p1[k] = mid[k]; }
			gfx_begin(220.f / 255.f, 0.f, 224.f / 255.f, 1.f); draw_capsule(&g_gp.cap, d); gfx_end();
			gfx_begin(140.f / 255.f, 0.f, 224.f / 255.f, 1.f); gfx_line3(p0, p1); gfx_end();
		}
	}
	if (o_travel && g_trav_n)
	{
		static const float hue[6][3] = { { 1.f, 0.863f, 0.f }, { 0.f, 0.863f, 1.f }, { 1.f, 0.471f, 1.f }, { 0.471f, 1.f, 0.471f }, { 1.f, 0.588f, 0.235f }, { 0.627f, 0.627f, 1.f } };
		float cs = 0.06f;
		g_line_half = 1.f;
		gfx_begin(1.f, 1.f, 1.f, 1.f);                                      /* your capsule's centre */
		for (k = 0; k < 3; k++) { float a[3] = { cen[0], cen[1], cen[2] }, b[3] = { cen[0], cen[1], cen[2] }; a[k] -= cs; b[k] += cs; gfx_line3(a, b); }
		gfx_end();
		for (i = 0; i < g_trav_n; i++)
		{
			const s_kid *c = &g_kids[g_trav_i[i]];
			float n[3], pmn[3], pmx[3], zmn[3], zmx[3], p0[3], sheet[8][3];
			kid_dir(c, n);
			for (k = 0; k < 3; k++)
			{
				pmn[k] = pmx[k] = c->pv[0][k];
				for (j = 1; j < c->k; j++) { if (c->pv[j][k] < pmn[k]) pmn[k] = c->pv[j][k]; if (c->pv[j][k] > pmx[k]) pmx[k] = c->pv[j][k]; }
				zmn[k] = pmn[k] - g[k]; zmx[k] = pmx[k] + g[k];
				p0[k] = cen[k] - n[k] * c->tim;
			}
			gfx_begin(190.f / 255.f, hue[i][0], hue[i][1], hue[i][2]); box3(zmn, zmx);        /* the weld zone */
			for (j = 0; j < c->k; j++) gfx_line3(c->pv[j], c->pv[(j + 1) % c->k]);         /* the triangle */
			gfx_end();
			if (c->over > 0.0005f || c->tim >= g_gp.cap.r)
			{
				/* the reset sheet: the triangle moved onto the plane through your centre - normal x TIM */
				for (j = 0; j < c->k; j++)
				{
					float t[3] = { c->pv[j][0] - p0[0], c->pv[j][1] - p0[1], c->pv[j][2] - p0[2] }, h = dot3(n, t);
					for (k = 0; k < 3; k++) sheet[j][k] = c->pv[j][k] - n[k] * h;
				}
				gfx_begin(45.f / 255.f, 1.f, 40.f / 255.f, 40.f / 255.f); gfx_fill3(sheet, c->k); gfx_end();
				gfx_begin(1.f, 1.f, 40.f / 255.f, 40.f / 255.f);
				for (j = 0; j < c->k; j++) gfx_line3(sheet[j], sheet[(j + 1) % c->k]);
				gfx_end();
				gfx_begin(200.f / 255.f, 1.f, 40.f / 255.f, 40.f / 255.f); gfx_line3(cen, p0); gfx_end();
			}
		}
	}
	if (o_obj_travel && g_otrav_n)
	{
		/* object travel regions (Cartographer's, approximate): the piece's box grown as the level's zone; a 0.7-unit
		 * reset square at your centre - normal x TIM; the piece outlined */
		static const float hue[6][3] = { { 1.f, 0.863f, 0.f }, { 0.f, 0.863f, 1.f }, { 1.f, 0.471f, 1.f }, { 0.471f, 1.f, 0.471f }, { 1.f, 0.588f, 0.235f }, { 0.627f, 0.627f, 1.f } };
		g_line_half = 1.f;
		for (i = 0; i < g_otrav_n; i++)
		{
			const s_okid *o = &g_ok[g_otrav_i[i]];
			float zmn[3], zmx[3], p0[3], h[3], u1[3], u2[3], l, sq[4][3];
			for (k = 0; k < 3; k++) { zmn[k] = o->mn[k] - g[k]; zmx[k] = o->mx[k] + g[k]; p0[k] = cen[k] - o->n[k] * o->tim; }
			gfx_begin(190.f / 255.f, hue[i][0], hue[i][1], hue[i][2]); box3(zmn, zmx);
			if (o->pc.kind == 0)
			{
				/* the box piece's edges */
				s32 e;
				for (e = 0; e < 12; e++)
				{
					static const s32 ed[12][2] = { {0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7} };
					float pa[3], pb[3];
					s32 m;
					for (k = 0; k < 3; k++)
					{
						pa[k] = o->pc.c[k]; pb[k] = o->pc.c[k];
						for (m = 0; m < 3; m++)
						{
							pa[k] += o->pc.ax[m][k] * ((ed[e][0] >> m) & 1 ? o->pc.he[m] : -o->pc.he[m]);
							pb[k] += o->pc.ax[m][k] * ((ed[e][1] >> m) & 1 ? o->pc.he[m] : -o->pc.he[m]);
						}
					}
					gfx_line3(pa, pb);
				}
			}
			else if (o->pc.kind == 2) gfx_line3(o->pc.a, o->pc.b);
			else if (o->pc.kind == 3) { gfx_line3(o->pc.v[0], o->pc.v[1]); gfx_line3(o->pc.v[1], o->pc.v[2]); gfx_line3(o->pc.v[2], o->pc.v[0]); }
			else if (o->pc.kind == 4) box3(o->mn, o->mx);                                    /* a hull: its box */
			gfx_end();
			if (o->over > 0.0005f || o->tim >= g_gp.cap.r)
			{
				h[0] = 0.f; h[1] = 0.f; h[2] = 1.f;
				if (o->n[2] > 0.9f || o->n[2] < -0.9f) { h[0] = 1.f; h[2] = 0.f; }
				u1[0] = o->n[1] * h[2] - o->n[2] * h[1]; u1[1] = o->n[2] * h[0] - o->n[0] * h[2]; u1[2] = o->n[0] * h[1] - o->n[1] * h[0];
				l = fsqrt(dot3(u1, u1)); for (k = 0; k < 3; k++) u1[k] = u1[k] / l * 0.35f;
				u2[0] = o->n[1] * u1[2] - o->n[2] * u1[1]; u2[1] = o->n[2] * u1[0] - o->n[0] * u1[2]; u2[2] = o->n[0] * u1[1] - o->n[1] * u1[0];
				for (k = 0; k < 3; k++)
				{
					sq[0][k] = p0[k] + u1[k] + u2[k]; sq[1][k] = p0[k] - u1[k] + u2[k];
					sq[2][k] = p0[k] - u1[k] - u2[k]; sq[3][k] = p0[k] + u1[k] - u2[k];
				}
				gfx_begin(45.f / 255.f, 1.f, 40.f / 255.f, 40.f / 255.f); gfx_fill3(sq, 4); gfx_end();
				gfx_begin(1.f, 1.f, 40.f / 255.f, 40.f / 255.f);
				for (k = 0; k < 4; k++) gfx_line3(sq[k], sq[(k + 1) & 3]);
				gfx_end();
				gfx_begin(200.f / 255.f, 1.f, 40.f / 255.f, 40.f / 255.f); gfx_line3(cen, p0); gfx_end();
			}
		}
	}
	g_line_half = 1.f;
}
#ifdef HOST
s32 host_believed(float *over0, float *dir0) { s32 k; if (g_bel_n) { *over0 = g_bel[0].over; for (k = 0; k < 3; k++) dir0[k] = g_bel[0].dir[k]; } return g_bel_n; }
s32 host_travel(void) { return g_trav_n; }
void host_set_obj_travel(s32 on) { o_obj_travel = on; }
void host_set_believed(s32 on) { o_believed = on; }
s32 host_otravel(void) { return g_otrav_n; }
#endif
static void objects_log_one(void) { objects_log(g_objlog_h); }
static void objects_log(u32 best_h)
{
	/* objects (Cartographer's object TIMs / believed poses / travel regions), stage 0: your body's pairs with objects
	 * - each object shape's type, the pair agent's class, its TIM leaves - logged once per kind, so the object port is
	 * built on what the Xbox actually uses. Read through the same rules: a list only inside the welder's class. */
	static u32 said_sig[24];
	static s32 said_n;
	u32 island = M32(M32(best_h + 0x10) + 0x44), pairs = ptr_ok(island) ? M32(island + 0x68) : 0;
	s32 pn = ptr_ok(island) ? (s32)M32(island + 0x6C) : 0, q;
	if (ptr_ok(pairs) && pn > 0 && pn <= 8192)
		for (q = 0; q < pn; q++)
		{
			u32 p = pairs + (u32)q * 16, agent = M32(p + 8), other, oshape, sig, avt;
			s32 st, j, seen = 0, at, keep_n = g_kid_n, keep_l = g_kid_leaves, keep_o = g_kid_other, keep_u = g_kid_unknown_n;
			if (M32(p) == best_h) other = M32(p + 4); else if (M32(p + 4) == best_h) other = M32(p); else continue;
			if (!ptr_ok(agent) || (agent >= ADDR_DATA_LO && agent < ADDR_DATA_HI) || !ptr_ok(other)) continue;
			if (agent == g_level_agent) continue;                                    /* the level: done above */
			{
				s32 comp_in = component_body(M32(other + 0x10)) != 0;
				oshape = M32(other - 0x10); st = shape_type(oshape); avt = M32(agent);
				sig = ((u32)(st & 0xff) << 24) ^ avt ^ (img_ok(M32(oshape)) ? M32(oshape) : 0) * 7u ^ (comp_in ? 0x5A5A5A5Au : 0);
			}
			for (j = 0; j < said_n; j++) if (said_sig[j] == sig) seen = 1;
			if (seen || said_n >= 24) continue;
			said_sig[said_n++] = sig;
			g_kid_n = KID_MAX - 16; g_kid_leaves = 0; g_kid_other = 0; g_kid_unknown_n = 0;   /* count into the spare slots */
			{
				u32 inner = agent;
				s32 hops = 0;
				while (hops < 3 && (M32(inner) == ADDR_VT_XFA || M32(inner) == ADDR_VT_XFB) && mem_ok(M32(inner + 0x0C))) { inner = M32(inner + 0x0C); hops++; }
				read_children(inner, 0);
			}
			at = lput(0, component_body(M32(other + 0x10)) ? "ghost object pair (component): shape type " : "ghost object pair (static): shape type ");
			at = lsnum(at, st);
			at = lput(at, " (vt "); at = lhex(at, img_ok(M32(oshape)) ? M32(oshape) : 0); at = lput(at, "), pair agent vt "); at = lhex(at, avt);
			at = lput(at, ", entries "); at = lnum(at, avt == ADDR_VT_WELD ? M32(agent + 0x10) : 0);
			at = lput(at, ", TIM leaves "); at = lnum(at, (u32)g_kid_leaves); at = lput(at, ", other children "); at = lnum(at, (u32)g_kid_other);
			for (j = 0; j < g_kid_unknown_n; j++) { at = lput(at, " [vt "); at = lhex(at, g_kid_unknown[j]); at = lput(at, "]"); }
			log_write();
			g_kid_n = keep_n; g_kid_leaves = keep_l; g_kid_other = keep_o; g_kid_unknown_n = keep_u;
		}
}
/* a bad read caught in the ghost update (guarded_call): this frame's results dropped, the log told */
static s32 g_faults;
static void ghost_faulted(void)
{
	g_gp.valid = 0; g_kid_n = 0; g_kid_offsets = 0; g_ok_n = 0; g_obj_offsets = 0; g_bel_n = 0; g_trav_n = 0; g_otrav_n = 0;
	g_faults++;
	if (g_faults <= 5 || !(g_faults & 63))
	{
		s32 at = lput(0, "ghost: a bad read was caught (guarded) in \""); at = lput(at, g_ghost_stage);
		at = lput(at, "\" - that frame's overlay skipped; caught so far: "); lnum(at, (u32)g_faults);
		log_write();
	}
}
static s32 ghost_hold(void) { return g_kid_offsets > 0 || g_obj_offsets > 0 || (g_gp.ever_back && g_sb_tick >= g_gp.last_back && g_sb_tick - g_gp.last_back <= (u32)o_ghost_hold); }
/* cos / sin at 22.5-degree steps (no maths library on the Xbox side) */
static const float g_cs16[17][2] = { { 1.f, 0.f }, { 0.92388f, 0.38268f }, { 0.70711f, 0.70711f }, { 0.38268f, 0.92388f }, { 0.f, 1.f },
	{ -0.38268f, 0.92388f }, { -0.70711f, 0.70711f }, { -0.92388f, 0.38268f }, { -1.f, 0.f }, { -0.92388f, -0.38268f },
	{ -0.70711f, -0.70711f }, { -0.38268f, -0.92388f }, { 0.f, -1.f }, { 0.38268f, -0.92388f }, { 0.70711f, -0.70711f },
	{ 0.92388f, -0.38268f }, { 1.f, 0.f } };
/* a capsule's wireframe, as Cartographer's capsule(): rings at both ends and the middle (16 segments), 8 lines along it,
 * and over each end 4 half-circles through the pole (8 segments each) - matching the 8 body lines */
static void draw_capsule(const s_cap *c, const float *shift)
{
	float u[3], h[3], e1[3], e2[3], l, a[3], b[3], mid[3];
	s32 i, k, m, e;
	for (k = 0; k < 3; k++) { a[k] = c->a[k] + (shift ? shift[k] : 0.f); b[k] = c->b[k] + (shift ? shift[k] : 0.f); u[k] = b[k] - a[k]; mid[k] = (a[k] + b[k]) * 0.5f; }
	l = fsqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
	if (l > 1e-6f) { u[0] /= l; u[1] /= l; u[2] /= l; } else { u[0] = 0.f; u[1] = 0.f; u[2] = 1.f; }
	if (u[2] < 0.9f && u[2] > -0.9f) { h[0] = 0.f; h[1] = 0.f; h[2] = 1.f; } else { h[0] = 1.f; h[1] = 0.f; h[2] = 0.f; }
	e1[0] = u[1] * h[2] - u[2] * h[1]; e1[1] = u[2] * h[0] - u[0] * h[2]; e1[2] = u[0] * h[1] - u[1] * h[0];   /* u x h */
	l = fsqrt(e1[0] * e1[0] + e1[1] * e1[1] + e1[2] * e1[2]); for (k = 0; k < 3; k++) e1[k] /= l;
	e2[0] = u[1] * e1[2] - u[2] * e1[1]; e2[1] = u[2] * e1[0] - u[0] * e1[2]; e2[2] = u[0] * e1[1] - u[1] * e1[0];   /* u x e1 */
	for (e = 0; e < 3; e++)                     /* the rings: at a, at b, in the middle */
	{
		const float *cen = e == 0 ? a : e == 1 ? b : mid;
		for (i = 0; i < 16; i++)
		{
			float p0[3], p1[3];
			for (k = 0; k < 3; k++)
			{
				p0[k] = cen[k] + (e1[k] * g_cs16[i][0] + e2[k] * g_cs16[i][1]) * c->r;
				p1[k] = cen[k] + (e1[k] * g_cs16[i + 1][0] + e2[k] * g_cs16[i + 1][1]) * c->r;
			}
			gfx_line3(p0, p1);
		}
	}
	for (i = 0; i < 8; i++)                     /* 8 lines along it, every 45 degrees */
	{
		float p0[3], p1[3];
		for (k = 0; k < 3; k++)
		{
			float o = (e1[k] * g_cs16[2 * i][0] + e2[k] * g_cs16[2 * i][1]) * c->r;
			p0[k] = a[k] + o; p1[k] = b[k] + o;
		}
		gfx_line3(p0, p1);
	}
	for (e = 0; e < 2; e++)                     /* the caps: 4 half-circles through each pole */
	{
		const float *cen = e ? b : a;
		float sg = e ? 1.f : -1.f;
		for (m = 0; m < 4; m++)
		{
			float side[3];
			for (k = 0; k < 3; k++) side[k] = e1[k] * g_cs16[2 * m][0] + e2[k] * g_cs16[2 * m][1];
			for (i = 0; i < 8; i++)
			{
				/* from one side over the pole to the other: angle x from 0 to 180 - (side x -cos x + dir x sin x) */
				float p0[3], p1[3];
				for (k = 0; k < 3; k++)
				{
					p0[k] = cen[k] + (side[k] * -g_cs16[i][0] + u[k] * sg * g_cs16[i][1]) * c->r;
					p1[k] = cen[k] + (side[k] * -g_cs16[i + 1][0] + u[k] * sg * g_cs16[i + 1][1]) * c->r;
				}
				gfx_line3(p0, p1);
			}
		}
	}
}
static s32 g_tim_lbl_n, g_tim_lbl_i[16];
static float g_tim_lbl_x[16], g_tim_lbl_y[16];
static void tims_draw(void)
{
	s32 st, i, j;
	g_tim_lbl_n = 0;
	for (st = 0; st < 3; st++)                  /* ok (green), contact (pale yellow), offset (red, heavy) */
	{
		s32 any = 0;
		for (i = 0; i < g_kid_n; i++) if (g_kids[i].k && g_kids[i].state == st) { any = 1; break; }
		if (!any) continue;
		g_line_half = st == 2 ? 2.f : 0.75f;
		if (st == 2) gfx_begin(1.f, 1.f, 48.f / 255.f, 48.f / 255.f);
		else if (st == 1) gfx_begin(0.9f, 1.f, 1.f, 128.f / 255.f);
		else gfx_begin(0.9f, 128.f / 255.f, 1.f, 128.f / 255.f);
		for (i = 0; i < g_kid_n; i++)
			if (g_kids[i].k && g_kids[i].state == st)
				for (j = 0; j < g_kids[i].k; j++) gfx_line3(g_kids[i].pv[j], g_kids[i].pv[(j + 1) % g_kids[i].k]);
		gfx_end();
	}
	g_line_half = 1.f;
	if (o_tim_labels)
		for (i = 0; i < g_kid_n && g_tim_lbl_n < 16; i++)
		{
			float x, y, dep;
			if (!g_kids[i].k) continue;
			project(g_kids[i].c, &x, &y, &dep);
			if (dep <= 0.05f || x < 0.f || y < 0.f || x > (float)g_vp_w || y > (float)g_vp_h) continue;
			g_tim_lbl_x[g_tim_lbl_n] = x; g_tim_lbl_y[g_tim_lbl_n] = y; g_tim_lbl_i[g_tim_lbl_n++] = i;
		}
}
static void ghost_draw(void)
{
	s32 hold;
	g_tim_lbl_n = 0;
	if (!o_ghost || !g_gp.valid) return;
	if (o_tim_overlay) tims_draw();
	ghost_extras_draw();
	hold = ghost_hold();
	g_line_half = 1.f;
	if (g_gp.S == -3) gfx_begin(1.f, 1.f, 32.f / 255.f, 32.f / 255.f);                /* stuck: red */
	else if (hold) gfx_begin(1.f, 170.f / 255.f, 1.f, 0.f);                           /* backstep: lime */
	else gfx_begin(1.f, 1.f, 105.f / 255.f, 180.f / 255.f);                           /* pink */
	draw_capsule(&g_gp.cap, 0);
	gfx_end();
	if (o_ghost_rewind && g_gp.have_rewind && g_sb_tick - g_gp.rewind_tick <= (u32)o_ghost_hold)
	{
		/* where the backstep put you back to (yellow) and where you were (orange): the capsule moved by the swept
		 * centres' difference from where it is now, with a line between */
		float to[3], from[3], m[3];
		s32 k;
		for (k = 0; k < 3; k++) { to[k] = g_gp.rew_to[k] - g_gp.cur[k]; from[k] = g_gp.rew_from[k] - g_gp.cur[k]; m[k] = (g_gp.cap.a[k] + g_gp.cap.b[k]) * 0.5f; }
		gfx_begin(200.f / 255.f, 1.f, 230.f / 255.f, 0.f); draw_capsule(&g_gp.cap, to);
		{ float p0[3], p1[3]; for (k = 0; k < 3; k++) { p0[k] = m[k] + from[k]; p1[k] = m[k] + to[k]; } gfx_line3(p0, p1); }
		gfx_end();
		gfx_begin(200.f / 255.f, 1.f, 90.f / 255.f, 0.f); draw_capsule(&g_gp.cap, from); gfx_end();
	}
}
#ifdef HOST
void host_draw_cap(const float *a, const float *b, float r) { s_cap c; s32 k; for (k = 0; k < 3; k++) { c.a[k] = a[k]; c.b[k] = b[k]; } c.r = r; gfx_begin(1.f, 1.f, 1.f, 1.f); draw_capsule(&c, 0); gfx_end(); }
void host_set_ghost(s32 on) { o_ghost = on; }
s32 host_ghost_state(void) { return !g_gp.valid ? -1 : g_gp.S == -3 ? 2 : ghost_hold() ? 1 : 0; }   /* -1 none, 0 pink, 1 lime, 2 red */
s32 host_kids(s32 *offsets, s32 *leaves, s32 *other) { *offsets = g_kid_offsets; *leaves = g_kid_leaves; *other = g_kid_other; return g_kid_n; }
s32 host_kid(s32 i, u32 *key, s32 *state, float *tim, float *over, s32 *corners) { *key = g_kids[i].key; *state = g_kids[i].state; *tim = g_kids[i].tim; *over = g_kids[i].over; *corners = g_kids[i].k; return 1; }
s32 host_ghost_rewind(float *from, float *to) { s32 k; for (k = 0; k < 3; k++) { from[k] = g_gp.rew_from[k]; to[k] = g_gp.rew_to[k]; } return g_gp.have_rewind; }
void host_ghost_cap(float *a, float *b, float *r) { s32 k; for (k = 0; k < 3; k++) { a[k] = g_gp.cap.a[k]; b[k] = g_gp.cap.b[k]; } *r = g_gp.cap.r; }
#endif

/* ---- Cartographer's shown seams: the rated ones, each kept 1.5 s after it last qualified (fading), nearest first (to your
 * landing in the air), at most 24 (48 in the air); only their pieces that passed; red ones dimmed; the 3 nearest
 * (not red first) labelled with "Seam info text" on. ---- */
#define SHOWN_MAX 1                    /* the 3d.9 shown-seams table: unused since 3d.12 */
#define SHOWN_HOLD 45
typedef struct { float a[3], b[3], near[3], drop; s_rd r; s32 q, gt, gh, pieces; u32 seen, piece_seen[24]; } s_shown;
static s_shown g_shown[SHOWN_MAX];
static s32 g_shown_n;
static s32 g_lbl_n, g_lbl_idx[3], g_lbl_first = -1;
static u8 g_whole[SHOWN_MAX], g_inreach[SHOWN_MAX];
#ifdef HOST
void host_shown_reset(void) { g_shown_n = 0; }
s32 host_shown_count(void) { return g_shown_n; }
#endif
static float g_lbl_x[3], g_lbl_y[3];

static s32 shown_find(const float *a, const float *b)
{
	s32 j;
	for (j = 0; j < g_shown_n; j++)
	{
		float *ea = g_shown[j].a, *eb = g_shown[j].b;
		if (ea[0] == a[0] && ea[1] == a[1] && ea[2] == a[2] && eb[0] == b[0] && eb[1] == b[1] && eb[2] == b[2]) return j;
	}
	return -1;
}

static void shown_update(void)
{
	s32 i, j, k, idx[SEAM_MAX], n = 0;
	float key[SEAM_MAX];
	u32 now = g_sb_tick + 1;
	/* Cartographer rates at most the 96 nearest candidates: the same here (nearest by the seam's own key) */
	for (i = 0; i < g_seam_n; i++) if (g_seam_q[i] >= 0) { idx[n] = i; key[n] = g_seam_d2[i]; n++; }
	for (i = 1; i < n; i++)
		for (j = i; j > 0 && key[j] < key[j - 1]; j--) { float t = key[j]; s32 o = idx[j]; key[j] = key[j - 1]; idx[j] = idx[j - 1]; key[j - 1] = t; idx[j - 1] = o; }
	if (n > 96) n = 96;
	for (k = 0; k < n; k++)
	{
		s_shown *e = 0;
		i = idx[k];
		j = shown_find(&g_seam_ab[i][0], &g_seam_ab[i][3]);
		if (j >= 0) e = &g_shown[j];
		if (!e)
		{
			s32 m;
			if (g_shown_n < SHOWN_MAX) e = &g_shown[g_shown_n++];
			else
			{
				/* full: the oldest that isn't from this frame (never one just updated) */
				e = 0;
				for (j = 0; j < g_shown_n; j++) if (g_shown[j].seen != now && (!e || g_shown[j].seen < e->seen)) e = &g_shown[j];
				if (!e) continue;
			}
			for (m = 0; m < 3; m++) { e->a[m] = g_seam_ab[i][m]; e->b[m] = g_seam_ab[i][3 + m]; }
			for (m = 0; m < 24; m++) e->piece_seen[m] = 0;
		}
		e->seen = now; e->q = g_seam_q[i]; e->r = g_seam_rd[i]; e->gt = g_seam_gt[i]; e->gh = g_seam_gh[i];
		e->pieces = g_seam_pieces[i]; e->drop = g_seam_drop[i];
		{
			s32 m;
			for (m = 0; m < 3; m++) e->near[m] = g_seam_near[i][m];
			for (m = 0; m < e->pieces && m < 24; m++) if (g_seam_open[i] & (1u << m)) e->piece_seen[m] = now;
		}
	}
	for (j = 0; j < g_shown_n; )                                             /* older than the hold: gone */
		if (now - g_shown[j].seen > SHOWN_HOLD) g_shown[j] = g_shown[--g_shown_n]; else j++;
}

static void seams_draw(void)
{
	float oc[2];
	s32 order[SHOWN_MAX], n = 0, i, j, cap = (g_sb_airborne && g_land_ok) ? 48 : 24, lab[SHOWN_MAX];
	float d2s[SHOWN_MAX];
	u32 now = g_sb_tick + 1;
	if (!g_sb_object) return;
	oc[0] = (g_sb_airborne && g_land_ok) ? g_land_p[0] : MF(g_sb_object + 0x64);
	oc[1] = (g_sb_airborne && g_land_ok) ? g_land_p[1] : MF(g_sb_object + 0x68);
	for (i = 0; i < g_shown_n; i++)
	{
		float dx = g_shown[i].near[0] - oc[0], dy = g_shown[i].near[1] - oc[1];
		order[n] = i; d2s[n] = dx * dx + dy * dy; n++;
	}
	for (i = 1; i < n; i++)                                                  /* nearest first */
		for (j = i; j > 0 && d2s[j] < d2s[j - 1]; j--) { float t = d2s[j]; s32 o = order[j]; d2s[j] = d2s[j - 1]; order[j] = order[j - 1]; d2s[j - 1] = t; order[j - 1] = o; }
	if (n > cap) n = cap;
	{
		/* in the air: only the seams (and the parts of them) air control can still reach; your landing's seam whole */
		s32 clip = g_sb_airborne && g_land_ok;
		if (g_sb_airborne)
		{
			float ft[3] = { MF(g_sb_object + 0x64), MF(g_sb_object + 0x68), MF(g_sb_object + 0x6C) }, vl[3] = { MF(g_sb_object + 0x88), MF(g_sb_object + 0x8C), MF(g_sb_object + 0x90) };
			reach_build(ft, vl);
		}
		else g_rch_on = 0;
		for (i = 0; i < n; i++)
		{
			s_shown *e = &g_shown[order[i]];
			s32 k, any = 0;
			g_whole[i] = 1; g_inreach[i] = 1;
			if (!clip) continue;
			if (g_land_ok && g_ld_seam)
			{
				float da = 0.f, db = 0.f;
				for (k = 0; k < 3; k++) { da += (e->a[k] - g_ld_sa[k]) * (e->a[k] - g_ld_sa[k]) + (e->b[k] - g_ld_sb[k]) * (e->b[k] - g_ld_sb[k]); db += (e->a[k] - g_ld_sb[k]) * (e->a[k] - g_ld_sb[k]) + (e->b[k] - g_ld_sa[k]) * (e->b[k] - g_ld_sa[k]); }
				if (da < 1e-4f || db < 1e-4f) continue;                         /* the seam your landing cursor is on */
			}
			for (k = 0; k <= 24 && !any; k++)
			{
				float q[3], t = (float)k / 24.f;
				s32 m;
				for (m = 0; m < 3; m++) q[m] = e->a[m] + (e->b[m] - e->a[m]) * t;
				any = reach_ok(q);
			}
			g_whole[i] = 0; g_inreach[i] = any;
		}
	}
	for (i = 0; i < n; i++) lab[i] = 0;
	g_lbl_n = 0;
	for (i = 0; i < n && g_lbl_n < 3; i++) if (g_inreach[i] && g_shown[order[i]].q != 0) { lab[i] = 1; g_lbl_idx[g_lbl_n++] = order[i]; }
	for (i = 0; i < n && g_lbl_n < 3; i++) if (g_inreach[i] && !lab[i]) { lab[i] = 1; g_lbl_idx[g_lbl_n++] = order[i]; }
	g_lbl_first = n > 0 && lab[0] ? order[0] : -1;                          /* the nearest, for the "biggest kick" line */
	for (i = 0; i < n; i++)
	{
		s_shown *e = &g_shown[order[i]];
		if (!g_inreach[i]) continue;                                         /* nowhere air control can take you */
		float age = (float)(now - e->seen) / (float)SHOWN_HOLD, alpha;
		s32 red = e->q == 0, k;
		alpha = (1.f - 0.7f * age) * (red ? 0.35f : 1.f);
		g_line_half = red ? 0.5f : (lab[i] ? 1.5f : 0.75f);
		q_colour(e->q, alpha);
		for (k = 0; k < e->pieces && k < 24; k++)
			if (e->piece_seen[k] && now - e->piece_seen[k] <= SHOWN_HOLD)
			{
				float t0 = (float)k / (float)e->pieces, t1 = (float)(k + 1) / (float)e->pieces, p0[3], p1[3];
				s32 m, st;
				for (m = 0; m < 3; m++) { p0[m] = e->a[m] + (e->b[m] - e->a[m]) * t0; p1[m] = e->a[m] + (e->b[m] - e->a[m]) * t1; }
				if (g_whole[i]) { gfx_line3(p0, p1); continue; }
				for (st = 0; st < 4; st++)                                       /* only the stretches in reach */
				{
					float s0[3], s1[3], mid[3];
					for (m = 0; m < 3; m++) { s0[m] = p0[m] + (p1[m] - p0[m]) * ((float)st / 4.f); s1[m] = p0[m] + (p1[m] - p0[m]) * ((float)(st + 1) / 4.f); mid[m] = (s0[m] + s1[m]) * 0.5f; }
					if (reach_ok(mid)) gfx_line3(s0, s1);
				}
			}
		gfx_end();
	}
	g_line_half = 1.f;
	for (i = 0; i < g_lbl_n; i++)                                             /* where the labels go */
	{
		s_shown *e = &g_shown[g_lbl_idx[i]];
		float p[3] = { e->near[0], e->near[1], e->near[2] + 0.15f }, dep;
		project(p, &g_lbl_x[i], &g_lbl_y[i], &dep);
		if (dep <= 0.05f || g_lbl_y[i] < 0.f || g_lbl_y[i] > (float)g_vp_h) g_lbl_idx[i] = -1;
	}
}

/* ---- the seams as 3d.8 showed them (preferred): every seam in range, rated ones in their verdict's colour, the rest
 * dimmed by bend. The labels ("Seam info text") go on the 3 nearest rated seams (not red first). ---- */
/* ---- where the road's seams go (Headlong): the surface you stand on - the level's or a near piece's - and its edges as
 * the grid sees them; and the nearest kept seams with their visibility. Logged ~every 10 s, 6 times a session. ---- */
static s32 probe_ground(const s_cb *c, const s_xf *x, const float *f, float *best_z, float *n_out, u32 *surf_out, u32 *ev_out, s32 *ne_out)
{
	u32 s2, ev[16];
	float pv[16][3], wn[3];
	s32 k, i, found = 0;
	for (s2 = 0; s2 < c->ns && s2 < 20000; s2++)
	{
		float z;
		s32 in = 0;
		k = surf_poly(c, x, s2, pv, ev, wn);
		if (k < 3 || wn[2] < 0.3f) continue;
		for (i = 0; i < k; i++)                                       /* your feet's x, y within it (crossings) */
		{
			const float *a = pv[i], *b = pv[(i + 1) % k];
			if ((a[1] > f[1]) != (b[1] > f[1]) && f[0] < a[0] + (b[0] - a[0]) * (f[1] - a[1]) / (b[1] - a[1])) in = !in;
		}
		if (!in) continue;
		z = pv[0][2] - ((f[0] - pv[0][0]) * wn[0] + (f[1] - pv[0][1]) * wn[1]) / wn[2];
		if (z > f[2] + 0.3f || z < f[2] - 3.f || z <= *best_z) continue;
		*best_z = z; *surf_out = s2; *ne_out = k; for (i = 0; i < k && i < 16; i++) ev_out[i] = ev[i];
		n_out[0] = wn[0]; n_out[1] = wn[1]; n_out[2] = wn[2]; found = 1;
	}
	return found;
}
static u32 g_probe_last;
static s32 g_probe_times;
static void seam_probes(const float *f)
{
	u32 last = g_probe_last;
	s32 times = g_probe_times;
	float bz = -1e30f, n[3] = { 0, 0, 0 };
	u32 surf = 0, ev[16];
	s32 ne = 0, which = -1, i, at;
	s_xf id;
	if (times >= 6 || (times && g_count_main - last < 300)) return;
	g_probe_last = g_count_main; g_probe_times = times + 1;
	id.on = 0;
	if (cb_ready() && probe_ground(&g_cbm, &id, f, &bz, n, &surf, ev, &ne)) which = 0;
	for (i = 0; i < g_nb_n; i++) if (probe_ground(&g_nb_cb[i], &g_nb_xf[i], f, &bz, n, &surf, ev, &ne)) which = 1 + i;
	at = lput(0, "ground probe: ");
	if (which < 0) { lput(at, "no surface found under your feet (level or near pieces)"); log_write(); }
	else
	{
		const s_cb *c = which == 0 ? &g_cbm : &g_nb_cb[which - 1];
		at = lput(at, which == 0 ? "the LEVEL's surface " : "a PIECE's surface "); at = lnum(at, surf);
		at = lput(at, ", z x100 "); at = lsnum(at, (s32)(bz * 100.f)); at = lput(at, ", normal x100 ");
		at = lsnum(at, (s32)(n[0] * 100.f)); at = lput(at, " "); at = lsnum(at, (s32)(n[1] * 100.f)); at = lput(at, " "); at = lsnum(at, (s32)(n[2] * 100.f));
		at = lput(at, "; its edges:");
		for (i = 0; i < ne && i < 8; i++)
		{
			u32 e = ev[i], ea = c->edge + e * 12, sl = M16(ea + 8), sr = M16(ea + 10);
			float mid[2], half, na[3], nb[3];
			s32 fl = lg_floor_edge(c, e, mid, &half);
			at = lput(at, " ["); at = lnum(at, e); at = lput(at, fl ? " floor seam" : " not:");
			if (!fl)
			{
				if (sl >= c->ns || sr >= c->ns || sl == sr) at = lput(at, " one surface");
				else if (!cb_normal(c, sl, na) || !cb_normal(c, sr, nb)) at = lput(at, " a plane unreadable");
				else { at = lput(at, " z x100 "); at = lsnum(at, (s32)(na[2] * 100.f)); at = lput(at, "/"); at = lsnum(at, (s32)(nb[2] * 100.f)); at = lput(at, " dot x100 "); at = lsnum(at, (s32)(dot3(na, nb) * 100.f)); }
			}
			at = lput(at, "]");
		}
		log_write();
	}
	/* the nearest kept seams: level / piece, kind, distance, drawn (visible), in view (sections), rating */
	at = lput(0, "seam census: "); at = lnum(at, (u32)g_seam_n); at = lput(at, " kept; nearest:");
	{
		s32 used[SEAM_MAX], m;
		for (i = 0; i < g_seam_n; i++) used[i] = 0;
		for (m = 0; m < 8; m++)
		{
			s32 best = -1;
			static const char *kn[4] = { "flat", "valley", "ridge", "?" };
			for (i = 0; i < g_seam_n; i++) if (!used[i] && (best < 0 || g_seam_d2[i] < g_seam_d2[best])) best = i;
			if (best < 0) break;
			used[best] = 1;
			at = lput(at, " ["); at = lput(at, g_seam_piece[best] ? "piece " : "level "); at = lput(at, kn[g_seam_kind[best] & 3]);
			at = lput(at, " key "); at = lnum(at, (u32)g_seam_d2[best]); at = lput(at, g_seam_vis[best] ? " drawn" : " hidden");
			at = lput(at, g_seam_los[best] ? " in view" : " not in view"); at = lput(at, " q "); at = lsnum(at, g_seam_q[best]); at = lput(at, "]");
		}
	}
	log_write();
}
#ifdef HOST
void host_probe(const float *f) { g_probe_times = 0; seam_probes(f); }
#endif
static void seam_probes_one(void);
static void seam_probes_one(void)
{
	float f[3];
	u32 o = g_sb_object;
	if (!o || !o_diag) return;                                            /* 6h.21: development lines */
	f[0] = MF(o + 0x64); f[1] = MF(o + 0x68); f[2] = MF(o + 0x6C);                  /* the feet: the origin (as the seam search) */
	seam_probes(f);
}
static void seams_draw_d8(void)
{
	static const float col[4][3] = { { 0.95f, 0.95f, 0.95f }, { 0.30f, 0.90f, 0.40f }, { 1.00f, 0.60f, 0.15f }, { 1.00f, 0.90f, 0.20f } };
	/* rated (below you): Cartographer's quality colours - red (dimmed), yellow lottery, green likely */
	static const float qcol[4][4] = { { 0.55f, 1.f, 0.31f, 0.27f }, { 0.95f, 1.f, 0.82f, 0.20f }, { 0.95f, 0.31f, 0.90f, 0.35f }, { 0.95f, 1.f, 0.59f, 0.16f } };   /* sb_quality_color (red dimmed) */
	s32 k, i, any;
	for (k = 0; k < 4; k++)                                                   /* unrated: by bend, dimmed */
	{
		any = 0;
		for (i = 0; i < d_seam_n; i++) if (d_seam_kind[i] == k && d_seam_q[i] < 0 && d_seam_vis[i] && d_seam_los[i]) { any = 1; break; }
		if (!any) continue;
		gfx_begin(0.45f, col[k][0], col[k][1], col[k][2]);
		for (i = 0; i < d_seam_n; i++)
			if (d_seam_kind[i] == k && d_seam_q[i] < 0 && d_seam_vis[i] && d_seam_los[i]) seam_line(i);
		gfx_end();
	}
	for (k = 0; k < 4; k++)
	{
		any = 0;
		for (i = 0; i < d_seam_n; i++) if (d_seam_q[i] == k && d_seam_los[i]) { any = 1; break; }
		if (!any) continue;
		gfx_begin(qcol[k][0], qcol[k][1], qcol[k][2], qcol[k][3]);
		for (i = 0; i < d_seam_n; i++)
			if (d_seam_q[i] == k && d_seam_los[i]) seam_line(i);
		gfx_end();
	}
}

static s_shown g_lbl_e[3];
static void labels_pick(void)
{
	s32 i, k, pick[3], np = 0, used[SEAM_MAX];
	for (i = 0; i < g_seam_n; i++) used[i] = 0;
	for (k = 0; k < 2 && np < 3; k++)                                         /* not red first, then any rated */
		while (np < 3)
		{
			s32 best = -1;
			for (i = 0; i < g_seam_n; i++)
				if (!used[i] && g_seam_q[i] >= 0 && (k || g_seam_q[i] != 0) && (best < 0 || g_seam_d2[i] < g_seam_d2[best])) best = i;
			if (best < 0) break;
			used[best] = 1; pick[np++] = best;
		}
	g_lbl_n = 0; g_lbl_first = -1;
	for (k = 0; k < np; k++)
	{
		s_shown *e = &g_lbl_e[g_lbl_n];
		float p[3], dep;
		s32 m;
		i = pick[k];
		for (m = 0; m < 3; m++) { e->a[m] = g_seam_ab[i][m]; e->b[m] = g_seam_ab[i][3 + m]; e->near[m] = g_seam_near[i][m]; }
		e->drop = g_seam_drop[i]; e->r = g_seam_rd[i]; e->q = g_seam_q[i]; e->gt = g_seam_gt[i]; e->gh = g_seam_gh[i];
		p[0] = e->near[0]; p[1] = e->near[1]; p[2] = e->near[2] + 0.15f;
		project(p, &g_lbl_x[g_lbl_n], &g_lbl_y[g_lbl_n], &dep);
		if (dep <= 0.05f || g_lbl_y[g_lbl_n] < 0.f || g_lbl_y[g_lbl_n] > (float)g_vp_h) continue;
		if (k == 0) g_lbl_first = g_lbl_n;
		g_lbl_idx[g_lbl_n] = g_lbl_n; g_lbl_n++;
	}
}

/* ---- which seams are drawn: the rated ones (they passed the covered / walk / jump checks); in the air, only those;
 * on the ground an unrated one below you is hidden (it failed those checks, or was too far to rate), and one at your
 * level or a little above is drawn only if you can get to it - a straight walk at your height to its nearest point
 * clear, or (above you, within your jump) the jump over to it clear. Nothing above your jump. Behind walls and on
 * floors you can't reach: hidden. ---- */
static void seams_vis(u32 object)
{
	float f[3] = { MF(object + 0x64), MF(object + 0x68), MF(object + 0x6C) };
	float apex = g_sb_jump * g_sb_jump / (2.f * 4.1712f);
	s32 i;
	for (i = 0; i < g_seam_n; i++)
	{
		float *a = &g_seam_ab[i][0], *b = &g_seam_ab[i][3], dx = b[0] - a[0], dy = b[1] - a[1], l2 = dx * dx + dy * dy, t, p[3];
		g_seam_vis[i] = g_seam_q[i] >= 0;
		if (g_seam_vis[i] || g_sb_airborne) continue;
		if ((a[2] < b[2] ? a[2] : b[2]) <= f[2] - 0.3f) continue;               /* below you, not rated: hidden */
		t = l2 > 1e-8f ? ((f[0] - a[0]) * dx + (f[1] - a[1]) * dy) / l2 : 0.f;
		if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
		p[0] = a[0] + dx * t; p[1] = a[1] + dy * t; p[2] = a[2] + (b[2] - a[2]) * t;
		if (p[2] > f[2] + apex) continue;                                     /* above your jump */
		if (p[2] <= f[2] + 0.3f)
		{
			float w0[3] = { f[0], f[1], f[2] + 0.6f }, w1[3] = { p[0], p[1], (p[2] > f[2] ? p[2] : f[2]) + 0.6f };
			g_seam_vis[i] = !blocked(w0, w1);                                 /* walk to it */
		}
		else
		{
			float h = f[2] + apex + 0.3f, j0[3] = { f[0], f[1], h }, j1[3] = { p[0], p[1], h }, v0[3] = { p[0], p[1], p[2] + 0.05f };
			g_seam_vis[i] = !blocked(j0, j1) && !blocked(v0, j1);             /* jump over onto it */
		}
	}
}
/* ---- in view: a seam's sections (1 under 3 units long, 2 under 6, else 3) each drawn only if the line from the
 * camera to it is clear - the nearby surfaces block it (a wall, a pillar, a floor between) - so seams don't show
 * through walls. (The overlay is drawn over the world: nothing else hides what's behind.) ---- */
static void seams_los(void)
{
	float cam[3], fwd[3], d2;
	s32 i, k, j;
	view_camera(cam, fwd, 0);                                               /* the camera the world is drawn from */
	d2 = 0.f;
	if (g_sb_object) for (j = 0; j < 3; j++) { float d = cam[j] - MF(g_sb_object + 0x64 + 4 * (u32)j); d2 += d * d; }
	for (i = 0; i < g_seam_n; i++)
	{
		float *a = &g_seam_ab[i][0], *b = &g_seam_ab[i][3], dx = b[0] - a[0], dy = b[1] - a[1], dz = b[2] - a[2];
		float len = fsqrt(dx * dx + dy * dy + dz * dz);
		s32 np = len < 3.f ? 1 : len < 6.f ? 2 : 3;
		g_seam_np[i] = (u8)np; g_seam_los[i] = (u8)((1 << np) - 1);
		if (!g_seam_vis[i] || !g_bk_count || !(d2 < 400.f)) continue;         /* no table, or a camera far from you */
		g_seam_los[i] = 0;
		for (k = 0; k < np; k++)
		{
			float t = ((float)k + 0.5f) / (float)np, p[3], v[3], l;
			p[0] = a[0] + dx * t; p[1] = a[1] + dy * t; p[2] = a[2] + dz * t;
			v[0] = cam[0] - p[0]; v[1] = cam[1] - p[1]; v[2] = cam[2] - p[2];
			l = fsqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
			/* more than 60 deg off your view (wider than the screen): off screen anyway - no ray, the drawing clips it */
			if (-(v[0] * fwd[0] + v[1] * fwd[1] + v[2] * fwd[2]) < 0.5f * l) { g_seam_los[i] |= (u8)(1 << k); continue; }
			if (l > 1e-3f) { p[0] += v[0] * (0.05f / l); p[1] += v[1] * (0.05f / l); p[2] += v[2] * (0.05f / l); }
			p[2] += 0.02f;                                                 /* just off the floors the seam lies on */
			if (!blocked(cam, p)) g_seam_los[i] |= (u8)(1 << k);
		}
	}
}
static void seam_line(s32 i)                    /* the sections of seam i in view */
{
	float *a = &d_seam_ab[i][0], *b = &d_seam_ab[i][3];
	s32 np = d_seam_np[i] ? d_seam_np[i] : 1, k, m;
	if (d_seam_los[i] == (u8)((1 << np) - 1)) { gfx_line3(a, b); return; }
	for (k = 0; k < np; k++)
		if (d_seam_los[i] & (1 << k))
		{
			float p0[3], p1[3];
			for (m = 0; m < 3; m++) { p0[m] = a[m] + (b[m] - a[m]) * ((float)k / (float)np); p1[m] = a[m] + (b[m] - a[m]) * ((float)(k + 1) / (float)np); }
			gfx_line3(p0, p1);
		}
}
#ifdef HOST
s32 host_seams_drawn(void) { s32 i, n = 0; for (i = 0; i < g_seam_n; i++) if (g_seam_vis[i] && g_seam_los[i]) n++; return n; }
s32 host_blockers(void) { return g_bk_count; }
float host_seam_sum(void)                      /* order-independent: thousandths, added as integers */
{
	s32 i, k, t = 0;
	for (i = 0; i < g_seam_n; i++) for (k = 0; k < 6; k++) { float v = g_seam_ab[i][k] * 1000.f; t += (s32)(v < 0.f ? v - 0.5f : v + 0.5f) * (k + 1); }
	return (float)t;
}
#endif

/* Cartographer's seam label text */
static s32 put_int(s32 at, s32 v);
static s32 put_f(s32 at, float v, s32 dec);
static s32 put(s32 at, const char *s);
static void seam_label(const s_shown *e)
{
	const s_rd *r = &e->r;
	s32 at;
	const char *opt = r->opt == 1 ? "JUMP" : "WALK OFF";
	float spd = r->opt == 1 ? r->jump : r->walk, dep = r->opt == 1 ? r->dj : r->dw;
	s32 best = (s32)(sb_up_estimate(0.06112f) + 0.5f);                    /* the best tilt's launch: tan(3.5 deg) */
	if (r->air)
	{
		at = put(0, "falling: arrive "); at = put_f(at, r->walk, 2); at = put(at, " u/s");
		if (e->q == 2) { at = put(at, ", depth "); at = put_f(at, r->dw, 2); at = put(at, r->dw < 0.21f ? " - EDGE (about 1 in 2) -> about " : " - LIKELY (9 in 10) -> about "); at = put_int(at, best); at = put(at, " u/s up"); }
		else if (e->q == 3 && e->gt > 0) { at = put(at, " - GHOST SEAM: "); at = put_int(at, e->gh); at = put(at, " of "); at = put_int(at, e->gt); at = put(at, " lottery landings here launched"); }
		else if (r->why) { at = put(at, ", depth "); at = put_f(at, r->dw, 2); at = put(at, " - "); at = put(at, r->why); }
		else if (e->q == 1) at = put(at, ", LOTTERY (about 1 in 30): deep only if Havok's contact comes a tick late");
		else if (r->debounce) { at = put(0, "falling: DEBOUNCE - the late contact this needs starts "); at = put_f(at, -r->late_start, 2); at = put(at, " below the floor: you fall through"); }
		else if (r->walk < SB_NEED) { at = put(at, " - too slow (< "); at = put_f(at, SB_NEED, 2); at = put(at, ")"); }
		else { at = put(at, " - too shallow ("); at = put_f(at, r->dw, 2); at = put(at, ", late "); at = put_f(at, r->lw, 2); at = put(at, ")"); }
		if (e->q == 2 && r->late_start <= -SB_DEBOUNCE) put(at, " - DEBOUNCE if the contact is late");
		return;
	}
	at = put(0, "drop "); at = put_f(at, e->drop, 1); at = put(at, ": ");
	if (e->q == 2)
	{
		at = put(at, opt); at = put(at, dep < 0.21f ? " EDGE (about 1 in 2) (" : " LIKELY (9 in 10) (");
		at = put_f(at, spd, 2); at = put(at, " u/s, depth "); at = put_f(at, dep, 2); at = put(at, ") -> about "); at = put_int(at, best); at = put(at, " u/s up");
		if (r->late_start <= -SB_DEBOUNCE) put(at, " - DEBOUNCE if the contact is late");
	}
	else if (e->q == 3 && e->gt > 0)
	{
		at = put(at, "GHOST SEAM: "); at = put_int(at, e->gh); at = put(at, " of "); at = put_int(at, e->gt);
		at = put(at, " lottery landings here launched - "); at = put(at, opt); at = put(at, " "); at = put_f(at, spd, 2); put(at, " u/s (its contact comes late)");
	}
	else if (e->q == 1) { at = put(at, "LOTTERY (about 1 in 30), "); at = put(at, opt); at = put(at, " "); at = put_f(at, spd, 2); put(at, " u/s: deep only if Havok's contact comes a tick late"); }
	else if (r->debounce) { at = put(at, "DEBOUNCE - the late contact this needs starts "); at = put_f(at, -r->late_start, 2); put(at, " below the floor: you fall through"); }
	else if ((r->walk > r->jump ? r->walk : r->jump) < SB_NEED)
	{ at = put(at, "too slow (walk "); at = put_f(at, r->walk, 2); at = put(at, ", jump "); at = put_f(at, r->jump, 2); at = put(at, " < "); at = put_f(at, SB_NEED, 2); put(at, ")"); }
	else
	{
		at = put(at, "too shallow (walk "); at = put_f(at, r->dw, 2); at = put(at, " / late "); at = put_f(at, r->lw, 2);
		at = put(at, ", jump "); at = put_f(at, r->dj, 2); at = put(at, " / late "); at = put_f(at, r->lj, 2); put(at, ")");
	}
}

/* ---- drawing ---- */
static const color4f c_title = { 1.0f, 1.0f, 0.62f, 0.25f };      /* orange, like Blam's menu headers */
static const color4f c_item = { 1.0f, 0.85f, 0.85f, 0.85f };
static const color4f c_selected = { 1.0f, 1.0f, 1.0f, 0.2f };      /* yellow */
static const color4f c_hint = { 0.85f, 0.75f, 0.75f, 0.75f };
static const color4f c_shadow = { 1.0f, 0.0f, 0.0f, 0.0f };
static const color4f c_charged = { 1.0f, 1.0f, 0.35f, 0.35f };     /* red, as in Cartographer */
static const color4f c_uncharged = { 0.9f, 0.67f, 0.67f, 0.67f };
static const color4f c_speed = { 1.0f, 0.92f, 0.92f, 0.92f };
static const color4f c_land[4] = { { 1.0f, 1.f, 0.31f, 0.27f }, { 1.0f, 1.f, 0.82f, 0.20f }, { 1.0f, 0.31f, 0.90f, 0.35f }, { 1.0f, 1.f, 0.59f, 0.16f } };   /* red, yellow, green, amber */
static const color4f c_streak = { 1.0f, 0.47f, 1.0f, 0.55f };       /* green while a chain runs */
static const color4f c_record = { 1.0f, 1.0f, 0.82f, 0.35f };       /* gold */

static u16 g_text[160];

static s32 put(s32 at, const char *s) { while (*s && at < 159) g_text[at++] = (u16)(u8)*s++; g_text[at] = 0; return at; }
static s32 put_int(s32 at, s32 v);
static s32 put_f(s32 at, float v, s32 dec)
{
	s32 m = dec == 1 ? 10 : 100, x;
	if (v < 0.f) { at = put(at, "-"); v = -v; }
	x = (s32)(v * (float)m + 0.5f);
	at = put_int(at, x / m); at = put(at, ".");
	if (dec == 2 && x % m < 10) at = put(at, "0");
	return put_int(at, x % m);
}
static s32 put_speed(s32 at, float v)
{
	s32 tenths = (s32)(v * 10.f + 0.5f);
	at = put_int(at, tenths / 10); at = put(at, "."); at = put_int(at, tenths % 10);
	return put(at, " u/s");
}
static s32 put_int(s32 at, s32 v)
{
	char buf[12]; s32 n = 0;
	if (v < 0) { at = put(at, "-"); v = -v; }
	do { buf[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 11);
	while (n && at < 159) g_text[at++] = (u16)(u8)buf[--n];
	g_text[at] = 0;
	return at;
}

static s32 g_full_height = 18;     /* the font's unscaled line height: every line's rectangle gets all of it */

static void line_at(s32 left, s32 y, s32 width, const color4f *color, s32 justification)
{
	rect16 r;
	r.top = (s16)y; r.left = (s16)left; r.bottom = (s16)(y + g_full_height + 6); r.right = (s16)(left + width);
	*(s32 *)(DRAW_STRING_GLOBALS + 0x0c) = justification;               /* 0 left, 2 centred */
	memcpy(DRAW_STRING_GLOBALS + 0x10, color, sizeof(color4f));
	*(DRAW_STRING_GLOBALS + 0x20) = o_shadow ? 1 : 0;
	memcpy(DRAW_STRING_GLOBALS + 0x24, &c_shadow, sizeof(color4f));
	DRAW_TEXT(g_text, &r, (float)o_scale * 0.1f);
	*(s32 *)(DRAW_STRING_GLOBALS + 0x0c) = 0;
}

static void line_at_s(s32 left, s32 y, s32 width, const color4f *color, s32 justification, float mult)
{
	rect16 r;
	r.top = (s16)y; r.left = (s16)left; r.bottom = (s16)(y + g_full_height + 6); r.right = (s16)(left + width);
	*(s32 *)(DRAW_STRING_GLOBALS + 0x0c) = justification;
	memcpy(DRAW_STRING_GLOBALS + 0x10, color, sizeof(color4f));
	*(DRAW_STRING_GLOBALS + 0x20) = o_shadow ? 1 : 0;
	memcpy(DRAW_STRING_GLOBALS + 0x24, &c_shadow, sizeof(color4f));
	DRAW_TEXT(g_text, &r, (float)o_scale * 0.1f * mult);
	*(s32 *)(DRAW_STRING_GLOBALS + 0x0c) = 0;
}
static void line(s32 x, s32 y, s32 h, const color4f *color)
{
	rect16 r;
	(void)h;
	r.top = (s16)y; r.left = (s16)x; r.bottom = (s16)(y + g_full_height + 6); r.right = (s16)(x + 520);
	memcpy(DRAW_STRING_GLOBALS + 0x10, color, sizeof(color4f));       /* .color */
	*(DRAW_STRING_GLOBALS + 0x20) = o_shadow ? 1 : 0;                  /* .shadow */
	memcpy(DRAW_STRING_GLOBALS + 0x24, &c_shadow, sizeof(color4f));   /* .field_24: the shadow colour */
	DRAW_TEXT(g_text, &r, (float)o_scale * 0.1f);
}

/* called once per frame, at the end of the frame (before the game's end-of-frame state reset) */
/* the seam search, under the crash guard (as the Ghost Viewer's): a fault drops this search, not the game - and says once
 * per step where (g_sb_stage) */
/* the landing as last finished - drawn with the seams of the same search (5e.17 drew the live landing against the last
 * finished seams: for a few frames its seam wasn't among them - white until the next commit) */
static void ld_swap(void)                        /* the drawn landing <-> the live one */
{
	s32 k, t;
	float f;
	t = g_land_ok; g_land_ok = d_land_ok; d_land_ok = t;
	t = g_ld_q; g_ld_q = d_ld_q; d_ld_q = t;
	t = g_ld_seam; g_ld_seam = d_ld_seam; d_ld_seam = t;
	t = g_ld_arrow; g_ld_arrow = d_ld_arrow; d_ld_arrow = t;
	f = g_ld_up; g_ld_up = d_ld_up; d_ld_up = f;
	for (k = 0; k < 3; k++)
	{
		f = g_land_p[k]; g_land_p[k] = d_land_p[k]; d_land_p[k] = f;
		f = g_ld_sa[k]; g_ld_sa[k] = d_ld_sa[k]; d_ld_sa[k] = f;
		f = g_ld_sb[k]; g_ld_sb[k] = d_ld_sb[k]; d_ld_sb[k] = f;
		f = g_ld_dir[k]; g_ld_dir[k] = d_ld_dir[k]; d_ld_dir[k] = f;
		f = g_ld_upax[k]; g_ld_upax[k] = d_ld_upax[k]; d_ld_upax[k] = f;
	}
}
static void shown_commit(void)                   /* the finished seams become the drawn ones */
{
	s32 i, k;
	d_land_ok = g_land_ok; d_ld_q = g_ld_q; d_ld_seam = g_ld_seam; d_ld_arrow = g_ld_arrow; d_ld_up = g_ld_up;
	for (k = 0; k < 3; k++) { d_land_p[k] = g_land_p[k]; d_ld_sa[k] = g_ld_sa[k]; d_ld_sb[k] = g_ld_sb[k]; d_ld_dir[k] = g_ld_dir[k]; d_ld_upax[k] = g_ld_upax[k]; }
	d_seam_n = g_seam_n;
	for (i = 0; i < g_seam_n; i++)
	{
		for (k = 0; k < 6; k++) d_seam_ab[i][k] = g_seam_ab[i][k];
		d_seam_kind[i] = g_seam_kind[i]; d_seam_q[i] = g_seam_q[i]; d_seam_vis[i] = g_seam_vis[i]; d_seam_los[i] = g_seam_los[i]; d_seam_np[i] = g_seam_np[i];
	}
}
/* the search in three steps - one a frame while it runs slowly (the 360), all three in one frame otherwise */
static void sb_step_find(void)
{
	u32 ts = tsc_lo();
	sb_stage("the search");
	seams_find(g_sb_object); g_seam_searches++;
	g_cyc_search += tsc_lo() - ts;
}
static void sb_step_land(void)
{
	u32 tl = tsc_lo();
	sb_stage("the landing");
	landing_find(g_sb_object);
	g_cyc_land += tsc_lo() - tl; g_cyc_search += tsc_lo() - tl;
}
static void sb_step_rate(void)
{
	u32 tr = tsc_lo();
	sb_stage("the rating"); seams_rate(g_sb_object);
	g_cyc_rate += tsc_lo() - tr;
}
static void sb_step_view(void)
{
	u32 tr = tsc_lo();
	sb_stage("the visibility"); seams_vis(g_sb_object);
	sb_stage("the line of sight"); seams_los();
	sb_stage("the labels"); labels_pick();
	g_cyc_rate += tsc_lo() - tr;
	sb_stage("the probes"); seam_probes_one();
	shown_commit();
	sb_stage("the search done");
}
static void sb_search_one(void)
{
	u32 ts = tsc_lo(), tr;
	sb_stage("the search");
	seams_find(g_sb_object); g_seam_searches++;
	sb_stage("the landing");
	{ u32 tl = tsc_lo(); landing_find(g_sb_object); g_cyc_land += tsc_lo() - tl; }
	g_cyc_search += tsc_lo() - ts;
	tr = tsc_lo();
	sb_stage("the rating"); seams_rate(g_sb_object);
	sb_stage("the visibility"); seams_vis(g_sb_object);
	sb_stage("the line of sight"); seams_los();
	sb_stage("the labels"); labels_pick();
	g_cyc_rate += tsc_lo() - tr;
	sb_stage("the probes"); seam_probes_one();
	sb_stage("the search done");
}
static void sb_faulted(void)
{
	static const char *said[12];
	static s32 said_n;
	s32 i, at;
	g_seam_n = 0; g_land_ok = 0; d_seam_n = 0;                              /* nothing half-made is drawn */
	for (i = 0; i < said_n; i++) if (said[i] == g_sb_stage) return;
	if (said_n < 12) said[said_n++] = g_sb_stage;
	at = lput(0, "superbounce: a bad read was caught (guarded) in "); at = lput(at, g_sb_stage); lput(at, " - that search skipped");
	log_write();
}
/* ---- the out-of-bounds timer (HCM's bars) - the game's player update (v1.0 0x14d8b0, v1.5 0x1503d0), read, not changed:
 * the player's counter (+0x3E, a byte) goes up by one each tick the game's test (v1.0 0xcc460, v1.5 0xce940 - the unit's
 * Havok body inside solid collision, which outside the level is) says so, and back to 0 the first tick it doesn't; while
 * it's 0 the test only runs on ticks where (game time + player index) % 16 == 0; past round(ticks a second x 0.5) - 15
 * at 30 Hz - the unit is killed. Not in game options mode 4.
 * HCM's two bars: the DEATH bar while counting (green draining to red, "OUT OF BOUNDS - N ticks" above; "kill blocked" when
 * the counter runs past the limit, as with invulnerability) and the DETECTION bar while not (the ticks to the next 16-tick
 * check, "OOB check in N ticks" below, white "OOB CHECK NOW" on the check tick). ---- */
static s32 g_oob_state;                          /* 0 none; 1 counting (the death bar); 2 not counting (the detection bar) */
static s32 g_oob_left, g_oob_total, g_oob_next, g_oob_blocked;
static void oob_update(void)
{
	u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR), opt = M32(ADDR_GAME_OPTIONS_PTR), time = M32(ADDR_GAME_TIME_PTR), player, unit;
	s32 datum, tps, c, L;
	g_oob_state = 0;
	if (!o_oob || !ptr_ok(lp) || !ptr_ok(time)) return;
	if (ptr_ok(opt) && M8(opt + 0xC) == 4) return;                       /* the game doesn't check in mode 4 */
	datum = (s32)M32(lp + 0xC);
	player = pool_entry(ADDR_PLAYERS_PTR, datum, 0x40);
	if (!player) return;
	unit = M32(player + 0x2C);
	if (unit == 0xFFFFFFFFu || !unit) return;
	tps = (s32)(s16)M16(time + 2);
	if (tps <= 0 || tps > 3000) tps = 30;                                 /* 6h.23: slow motion runs up to 300 (a script's 0.01: 3000) */
	L = tps / 2; if ((tps & 1) && (L & 1)) L++;                         /* round(tps x 0.5): a half rounds to even (the FPU) */
	c = (s32)M8(player + 0x3E);
	g_oob_total = L + 1;                                                  /* killed when the counter passes L */
	if (c > 0)
	{
		g_oob_left = g_oob_total - c; if (g_oob_left < 0) g_oob_left = 0;
		g_oob_blocked = c > g_oob_total;                                  /* past the limit and still alive: the kill blocked */
		g_oob_state = 1;
	}
	else
	{
		u32 gt = M32(time + 8), idx = (u32)datum & 0xFFFFu;
		g_oob_next = (s32)((16u - ((gt + idx) & 15u)) & 15u);              /* 0: this tick is a check */
		g_oob_state = 2;
	}
}
/* a bar as HCM draws it: a dark backdrop, the fill (fraction f, colour rgb), a white outline */
static void oob_bar(s32 x0, s32 y, s32 x1, s32 h, float f, float r, float g, float b, float outline)
{
	s32 w = (s32)((float)(x1 - x0) * (f < 0.f ? 0.f : f > 1.f ? 1.f : f));
	gfx_begin(0.66f, 0.f, 0.f, 0.f); gfx_rect(x0 - 3, y - 3, x1 + 3, y + h + 3); gfx_end();
	if (w > 0) { gfx_begin(0.92f, r, g, b); gfx_rect(x0, y, x0 + w, y + h); gfx_end(); }
	gfx_begin(outline, 1.f, 1.f, 1.f);
	gfx_rect(x0, y, x1, y + 1); gfx_rect(x0, y + h - 1, x1, y + h); gfx_rect(x0, y, x0 + 1, y + h); gfx_rect(x1 - 1, y, x1, y + h);
	gfx_end();
}
static void oob_draw(s32 height)
{
	const s32 x0 = 164, x1 = 476, y = 150, h = 8;                         /* 312 wide, centred (6b.3's 240, 30% wider) */
	if (g_oob_state == 1)
	{
		float f = (float)g_oob_left / (float)g_oob_total, r = 1.f - f, g = (220.f * f + 35.f * (1.f - f)) / 255.f, b = 40.f / 255.f;
		color4f lc;
		s32 at;
		lc.a = 1.f; lc.r = r; lc.g = g; lc.b = b;
		oob_bar(x0, y, x1, h, f, r, g, b, 120.f / 255.f);
		at = put(0, g_oob_blocked ? "OUT OF BOUNDS (kill blocked) - " : "OUT OF BOUNDS - "); at = put_int(at, g_oob_left); put(at, g_oob_left == 1 ? " tick" : " ticks");
		line_at(70, y - height - 8, 500, &lc, 2);                       /* the label above */
	}
	else if (g_oob_state == 2)
	{
		static const color4f c_detect = { 1.f, 150.f / 255.f, 200.f / 255.f, 1.f };
		s32 dy = y + h + 14, at;
		if (g_oob_next == 0) oob_bar(x0, dy, x1, 6, 1.f, 1.f, 1.f, 1.f, 110.f / 255.f);                 /* the check tick: full, white */
		else oob_bar(x0, dy, x1, 6, (float)g_oob_next / 16.f, 70.f / 255.f, 160.f / 255.f, 1.f, 110.f / 255.f);
		if (g_oob_next == 0) put(0, "OOB CHECK NOW");
		else { at = put(0, "OOB check in "); at = put_int(at, g_oob_next); put(at, g_oob_next == 1 ? " tick" : " ticks"); }
		line_at(70, dy + 6 + 4, 500, &c_detect, 2);                     /* the label below */
	}
}
#ifdef HOST
s32 host_oob(s32 *left, s32 *next) { *left = g_oob_left; *next = g_oob_next; return g_oob_state; }
void host_set_oob(s32 on) { o_oob = on; }
#endif

/* ---- the sleep timer (HCM's countdown, exact) - Havok's island deactivation, as the world's step runs it (v1.0 0x30fef0
 * calling 0x316b30 for each active island while the world's flag +0x23D is set; v1.5 0x319f90 / 0x320c80, the same bytes):
 * each tick both of an island's timers (A +0x34, B +0x38) grow by the step's delta (the world's copy, +0x150); A past its
 * period (world +0x30, the island's world at +0x20) asks every body's deactivator (entity +0x54) its short test - all still
 * adds a round (+0x2C) below 5, and at 5 puts the island to sleep; a check that isn't all still changes nothing. Otherwise
 * B past its period (+0x34) asks the long test - all pass: asleep at once.
 * The deactivators (v1.0 / v1.5 vtables): Havok's spatial one (0x414abc / 0x41eb6c: short 0x3144b0, long 0x314580) - still
 * when, since its reference pose (short +0x10 / +0x20, long +0x30 / +0x40), the body (its motion: entity +0x3C) has moved
 * (motion +0xB0) less than +0x50 (long +0x58) and turned (motion +0x30, a quaternion; x radiusSqrd +0x08) less than +0x54
 * (long +0x5C) - a test that fails RESAMPLES its reference, so the next one passes if the body stays still; the fake one
 * (0x415100 / 0x41f1b0): never still; Halo's (0x455950 / 0x461970): never still - without resampling - while its veto
 * (0x1d1910 / 0x1d9bc0, reads only) holds (a moving or airborne biped, a moving vehicle, a body the game drives), else
 * Havok's tests. A body with none: never still.
 * So, "if still": the first quiet check counts only if every body is still now (else it fails and resamples, and the rest
 * count); the first long check passes only if every body passes the long test now (else it fails and resamples, and the
 * next one passes); a body that's never still keeps the island awake. Read - the veto called as the game calls it, under
 * the crash guard - never changed. ---- */
typedef struct { u32 island; s32 bodies, rounds, to_a, to_b, to_sleep, cycle, awake; float dist; } s_isl;
enum { AW_NO = 0, AW_VETO, AW_NONE, AW_FAKE, AW_UNKNOWN };
static s_isl g_isl_own, g_isl_near[2];
static s32 g_isl_near_n, g_sleep_state;          /* 0 nothing; 1 shown; 2 deactivation off in this world */
u32 game_call_eax(u32 fn, u32 a);
/* one of Havok's two tests, now, with its own float steps (SSE single precision; the x87 compares exact in double) */
static s32 deact_test(u32 d, u32 motion, s32 lng)
{
	u32 rp = d + (lng ? 0x30u : 0x10u), rq = d + (lng ? 0x40u : 0x20u);
	float m = MF(d + (lng ? 0x58u : 0x50u)), t = MF(d + (lng ? 0x5Cu : 0x54u)), rs = MF(d + 0x08);
	volatile float dx = MF(rp) - MF(motion + 0xB0), dy = MF(rp + 4) - MF(motion + 0xB4), dz = MF(rp + 8) - MF(motion + 0xB8);
	volatile float sx = dx * dx, sy = dy * dy, sz = dz * dz, s1 = sy + sx, pos = sz + s1;
	volatile float q0 = MF(rq) - MF(motion + 0x30), q1 = MF(rq + 4) - MF(motion + 0x34), q2 = MF(rq + 8) - MF(motion + 0x38), q3 = MF(rq + 12) - MF(motion + 0x3C);
	volatile float r0 = q0 * q0, r1 = q1 * q1, r2 = q2 * q2, r3 = q3 * q3, a = r2 + r0, b = r3 + r1, rot = a + b;
	if (!((double)pos < (double)m * (double)m)) return 0;               /* moved too far (or not a number) */
	if (!((double)t * (double)t > (double)rot * (double)rs)) return 0;  /* turned too far */
	return 1;
}
/* an island's bodies now: why it's kept awake (AW_*), else whether all pass the short and the long test */
static s32 isl_quiet(u32 island, s32 *short_all, s32 *long_all)
{
	u32 ents = M32(island + 0x3C);
	s32 n = (s32)M32(island + 0x40), i;
	*short_all = *long_all = 1;
	if (!ptr_ok(ents) || n <= 0 || n > 4096) return AW_NONE;
	for (i = 0; i < n; i++)
	{
		u32 e = M32(ents + 4 * (u32)i), d, vt, mo;
		if (!ptr_ok(e)) return AW_NONE;
		d = M32(e + 0x54); mo = M32(e + 0x3C);
		if (!d) return AW_NONE;                                         /* no deactivator: never still */
		if (!ptr_ok(d) || !ptr_ok(mo)) return AW_UNKNOWN;
		vt = M32(d);
		if (vt == ADDR_VT_DEACT_FAKE) return AW_FAKE;
		if (vt == ADDR_VT_DEACT_HALO) { if (game_call_eax(ADDR_DEACT_VETO, d) & 0xFFu) return AW_VETO; }
		else if (vt != ADDR_VT_DEACT_SPATIAL) return AW_UNKNOWN;
		if (!deact_test(d, mo, 0)) *short_all = 0;
		if (!deact_test(d, mo, 1)) *long_all = 0;
	}
	return AW_NO;
}
static s32 isl_count(u32 island, float delta, s_isl *o)
{
	u32 world = M32(island + 0x20);
	float pa, pb, ta, tb;
	s32 r, n, sa, la, first_a = 1, first_b = 1;
	if (!ptr_ok(world)) return 0;
	pa = MF(world + 0x30); pb = MF(world + 0x34);
	if (!(pa > 0.f && pa < 100.f) || !(pb > 0.f && pb < 1000.f)) return 0;
	ta = MF(island + 0x34); tb = MF(island + 0x38); r = (s32)(s8)M8(island + 0x2C);
	if (r < 0 || r > 5) return 0;
	o->island = island; o->rounds = r; o->bodies = (s32)M32(island + 0x40);
	o->to_a = o->to_b = o->to_sleep = -1;
	o->cycle = ((s32)(pa / delta) + 1) * 6;                               /* six checks from rounds 0: the bar's length */
	o->awake = isl_quiet(island, &sa, &la);
	for (n = 1; n <= 2400; n++)                                           /* the game's own order, tick by tick */
	{
		volatile float va = ta + delta, vb = tb + delta;                  /* stored as floats, as the game stores them */
		ta = va; tb = vb;
		if (ta > pa)
		{
			ta = ta - pa;
			if (o->to_a < 0) o->to_a = n;
			if (o->awake) continue;                                       /* never all still: nothing counts */
			if (first_a) { first_a = 0; if (!sa) continue; }              /* not all still now: this one fails (and resamples) */
			if (r < 5) r++;
			else { o->to_sleep = n; return 1; }
		}
		else if (tb > pb)
		{
			tb = tb - pb;
			if (o->to_b < 0) o->to_b = n;
			if (o->awake) continue;
			if (first_b) { first_b = 0; if (!la) continue; }              /* fails now (and resamples): the next one passes */
			o->to_sleep = n; return 1;                                     /* all pass the long test: asleep at once */
		}
	}
	return 1;
}
static void sleep_update_body(void)
{
	u32 world = M32(ADDR_HAVOK_WORLD_PTR), own = 0, isl, near_isl[2] = { 0, 0 };
	float delta, pp[3], near_d[2] = { 1e30f, 1e30f };
	s32 i, k, n;
	if (!o_sleep || !g_sb_object || !ptr_ok(world)) return;
	if (!M8(world + 0x23D)) { g_sleep_state = 2; return; }                /* deactivation off: nothing ever sleeps */
	delta = MF(world + 0x150);
	if (!(delta > 0.001f && delta < 0.5f)) delta = 1.f / 30.f;
	for (k = 0; k < 3; k++) pp[k] = MF(g_sb_object + 0x64 + 4 * (u32)k);
	{
		/* your island: the first of your unit's bodies in this world (the charge test's path) */
		u32 component = pool_entry(ADDR_HAVOK_COMPONENTS_PTR, (s32)M32(g_sb_object + 0xB4), 0x78), bodies;
		if (component && ptr_ok(bodies = M32(component + 0x70)))
		{
			n = (s32)M32(component + 0x74);
			for (i = 0; i < n && i < 64 && !own; i++)
			{
				u32 entity = M32(bodies + (u32)i * 0x60 + 0x40);
				if (ptr_ok(entity) && M32(entity + 0x08) == world && ptr_ok(M32(entity + 0x44))) own = M32(entity + 0x44);
			}
		}
	}
	{
		/* the nearest two other active islands within 20 units (by their nearest body's centre) */
		u32 list = M32(world + 0x08);
		s32 cnt = (s32)M32(world + 0x0C);
		if (ptr_ok(list) && cnt > 0 && cnt <= 1024)
			for (i = 0; i < cnt; i++)
			{
				u32 ents;
				s32 m, b;
				float best = 1e30f;
				isl = M32(list + 4 * (u32)i);
				if (!ptr_ok(isl) || isl == own) continue;
				ents = M32(isl + 0x3C); m = (s32)M32(isl + 0x40);
				if (!ptr_ok(ents) || m <= 0 || m > 4096) continue;
				for (b = 0; b < m && b < 16; b++)
				{
					u32 e = M32(ents + 4 * (u32)b), mo;
					float d[3], d2;
					if (!ptr_ok(e) || !ptr_ok(mo = M32(e + 0x3C))) continue;
					for (k = 0; k < 3; k++) d[k] = MF(mo + 0x70 + 4 * (u32)k) - pp[k];
					d2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
					if (d2 < best) best = d2;
				}
				if (best > 400.f) continue;
				if (best < near_d[0]) { near_d[1] = near_d[0]; near_isl[1] = near_isl[0]; near_d[0] = best; near_isl[0] = isl; }
				else if (best < near_d[1]) { near_d[1] = best; near_isl[1] = isl; }
			}
	}
	if (own && isl_count(own, delta, &g_isl_own)) { g_isl_own.dist = 0.f; g_sleep_state = 1; }
	else g_isl_own.island = 0;
	for (i = 0; i < 2; i++)
	{
		s_isl t;
		if (!near_isl[i] || !isl_count(near_isl[i], delta, &t)) continue;
		t.dist = fsqrt(near_d[i]);
		g_isl_near[g_isl_near_n++] = t;
		g_sleep_state = 1;
	}
}
static void sleep_update(void)
{
	g_sleep_state = 0; g_isl_near_n = 0; g_isl_own.island = 0;
	if (!o_sleep || !g_sb_object) return;
	if (guarded_call(sleep_update_body)) { g_sleep_state = 0; g_isl_near_n = 0; g_isl_own.island = 0; }   /* a fault: nothing shown */
}
static const char *awake_why(s32 aw)
{
	return aw == AW_VETO ? "the game keeps it awake (a moving biped or vehicle)" : aw == AW_FAKE ? "a body that never sleeps"
		: aw == AW_NONE ? "a body without a deactivator" : "an unknown deactivator";
}
static void sleep_colour(s32 t, color4f *c)                              /* HCM's: red at 3 or less, yellow at 15 or less */
{
	c->a = 1.f;
	if (t >= 0 && t <= 3) { c->r = 1.f; c->g = 0.35f; c->b = 0.35f; }
	else if (t >= 0 && t <= 15) { c->r = 1.f; c->g = 0.85f; c->b = 0.2f; }
	else { c->r = 0.75f; c->g = 0.9f; c->b = 1.f; }
}
static void sleep_draw(s32 height)
{
	const s32 x0 = 164, x1 = 476, y = 214;                                  /* 312 wide, centred, as the out-of-bounds bars */
	s32 at, i, ly;
	color4f c;
	if (!g_sleep_state) return;
	if (g_sleep_state == 2)
	{
		static const color4f c_off = { 1.f, 0.6f, 0.6f, 0.6f };
		put(0, "SLEEP: off (Havok's deactivation is disabled here)");
		line_at(70, y, 500, &c_off, 2);
		return;
	}
	ly = y + 6 + 4;
	if (g_isl_own.island)
	{
		s32 t = g_isl_own.to_sleep, cyc = g_isl_own.cycle > 3 ? g_isl_own.cycle : 4;
		/* draining: full is a whole cycle (six quiet checks) away, empty is asleep; kept awake (or not soon): full */
		float f = (g_isl_own.awake || t < 0) ? 1.f : (float)t / (float)cyc;
		sleep_colour(g_isl_own.awake ? -1 : t, &c);
		oob_bar(x0, y, x1, 6, f, c.r, c.g, c.b, 110.f / 255.f);
		{
			/* the jump line: where the fill is with 3 ticks to go */
			s32 mx = x0 + (s32)((float)(x1 - x0) * 3.f / (float)cyc);
			gfx_begin(0.95f, 1.f, 1.f, 1.f); gfx_rect(mx - 1, y - 3, mx + 1, y + 6 + 3); gfx_end();
		}
		if (g_isl_own.awake) { at = put(0, "SLEEP: kept awake - "); put(at, awake_why(g_isl_own.awake)); }
		else
		{
			if (t >= 0) { at = put(0, "SLEEP in "); at = put_int(at, t); at = put(at, t == 1 ? " tick if still - quiet rounds " : " ticks if still - quiet rounds "); }
			else at = put(0, "SLEEP: not within 80 s - quiet rounds ");
			at = put_int(at, g_isl_own.rounds); put(at, "/5");
		}
		line_at(70, ly, 500, &c, 2);
		ly += height + 2;
	}
	for (i = 0; i < g_isl_near_n; i++)
	{
		const s_isl *o = &g_isl_near[i];
		sleep_colour(o->to_sleep, &c);
		at = put(0, "island "); at = put_int(at, o->bodies); at = put(at, o->bodies == 1 ? " body, " : " bodies, ");
		at = put_f(at, o->dist, 1); at = put(at, " wu: ");
		if (o->awake) at = put(at, "kept awake");
		else if (o->to_sleep >= 0) { at = put_int(at, o->to_sleep); at = put(at, " ticks"); } else at = put(at, "not soon");
		at = put(at, " ("); at = put_int(at, o->rounds); put(at, "/5)");
		line_at(70, ly, 500, &c, 2);
		ly += height + 2;
	}
}
#ifdef HOST
s32 host_sleep(s32 *to_sleep, s32 *to_a, s32 *to_b, s32 *rounds) { *to_sleep = g_isl_own.to_sleep; *to_a = g_isl_own.to_a; *to_b = g_isl_own.to_b; *rounds = g_isl_own.rounds; return g_sleep_state; }
s32 host_sleep_awake(void) { return g_isl_own.awake; }
static u32 g_host_veto;
void host_set_veto(s32 v) { g_host_veto = (u32)v; }
u32 game_call_eax(u32 fn, u32 a) { (void)fn; (void)a; return g_host_veto; }
s32 host_sleep_near(s32 i, s32 *to_sleep, float *dist) { if (i >= g_isl_near_n) return 0; *to_sleep = g_isl_near[i].to_sleep; *dist = g_isl_near[i].dist; return 1; }
void host_set_sleep(s32 on) { o_sleep = on; }
#endif

/* ---- predicted monster / dreadnought spots (the H2 bounce scanner's predictions - HCM's "Predicted spots"), per BSP.
 * The files (the "spots" folder, beside the log: T:\spots\) are the scanner's CSVs packed: index.bin - for each file its
 * spots' bounds and 8 sample spots; NN.bin - 12 bytes a spot (position to 1/16 unit from an origin, the strongest launch
 * x4, the windows 20+ / 35+ x255, the best case's fall u/s and drift x10). HCM finds a file by an MCC map key; here the
 * BSP is matched by its geometry: the file whose spots lie in the level and whose samples sit on its floor surfaces. Only
 * that BSP's spots are read - into memory asked for at run time (VirtualAlloc), freed when the BSP changes - so nothing is
 * added to the mod's own size. ---- */
#define PS_REC 12
#define PS_IDX_MAX 16384
#define PS_IDX_REC 168
#define PS_NEAR 1024
static u8 *g_ps_buf;
static u32 g_ps_bytes, g_pspot_n, g_ps_for, g_ps_sig;
static float g_ps_org[3];
static s32 g_ps_state;                           /* 0 not tried; 1 loaded; 2 no spots folder; 3 no file matches this BSP; 4 a file unreadable; 5 no memory */
static char g_ps_name[41];
static s32 g_ps_score;
static u16 g_ps_near[PS_NEAR];
static s32 g_ps_near_n, g_ps_best = -1;
static float g_ps_best_d;
#ifdef HOST
#include <stdio.h>
#include <stdlib.h>
static char g_host_spots[256] = ".";
void host_set_spots_dir(const char *d) { s32 i; for (i = 0; d[i] && i < 255; i++) g_host_spots[i] = d[i]; g_host_spots[i] = 0; }
static s32 ps_read(const char *rel, u8 *fixed, u32 cap, u8 **out, u32 *size)
{
	char p[512]; FILE *f; long sz; u8 *b;
	snprintf(p, sizeof(p), "%s/%s", g_host_spots, rel);
	if (!(f = fopen(p, "rb"))) return 0;
	fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
	if (sz < 8 || (fixed && (u32)sz > cap)) { fclose(f); return 0; }
	b = fixed ? fixed : (u8 *)malloc((size_t)sz);
	if (!b) { fclose(f); return -1; }
	if (fread(b, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); if (!fixed) free(b); return 0; }
	fclose(f); *out = b; *size = (u32)sz; return 1;
}
static void ps_free(u8 *b) { free(b); }
#else
static s32 ps_read(const char *rel, u8 *fixed, u32 cap, u8 **out, u32 *size)
{
	static const char *const drv[3] = { "T:\\spots\\", "U:\\spots\\", "Z:\\spots\\" };
	s32 d;
	for (d = 0; d < 3; d++)
	{
		char p[64]; s32 n = 0, k; void *h; u32 sz, got = 0; u8 *b;
		for (k = 0; drv[d][k]; k++) p[n++] = drv[d][k];
		for (k = 0; rel[k] && n < 62; k++) p[n++] = rel[k];
		p[n] = 0;
		h = CreateFileA_x(p, 0x80000000u /* GENERIC_READ */, 1, 0, 3 /* OPEN_EXISTING */, 0x80, 0);
		if (h == (void *)-1) continue;
		sz = GetFileSize_x(h, 0);
		if (sz == 0xFFFFFFFFu || sz < 8 || (fixed && sz > cap)) { CloseHandle_x(h); return 0; }
		b = fixed ? fixed : (u8 *)VirtualAlloc_x(0, sz, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 4 /* PAGE_READWRITE */);
		if (!b) { CloseHandle_x(h); return -1; }
		ReadFile_x(h, b, sz, &got, 0);
		CloseHandle_x(h);
		if (got != sz) { if (!fixed) VirtualFree_x(b, 0, 0x8000); return 0; }
		*out = b; *size = sz; return 1;
	}
	return 0;
}
static void ps_free(u8 *b) { VirtualFree_x(b, 0, 0x8000 /* MEM_RELEASE */); }
#endif
static float ps_f(const u8 *p) { float f; memcpy(&f, p, 4); return f; }
static s32 ps_magic(const u8 *p, const char *m) { return p[0] == (u8)m[0] && p[1] == (u8)m[1] && p[2] == (u8)m[2] && p[3] == (u8)m[3]; }
/* a sample on the level's floor: a surface whose polygon holds (x, y) and is within 0.08 of z there. Every level surface
 * is tried - nothing here uses the Superbounce Viewer's grid (6d built that grid itself, at its own point in the frame, and
 * it came out a sliver of the level: the viewer's seams went with it). Most surfaces are dropped by their plane's height
 * at (x, y), from their first vertex; the few left get the polygon test. */
static s32 ps_on_level(float x, float y, float z)
{
	s_xf id;
	u32 s;
	id.on = 0;
	for (s = 0; s < g_cbm.ns; s++)
	{
		float n[3], v0[3], pz, pv[16][3], wn[3];
		u32 e, v;
		s32 k, i, in = 0;
		if (!cb_normal(&g_cbm, s, n) || n[2] < 0.1f) continue;                 /* floors */
		e = M16(g_cbm.surf + s * 8 + 2);
		if (e >= g_cbm.ne) continue;
		v = M16(g_cbm.edge + e * 12);
		if (v >= g_cbm.nv) continue;
		cb_vert(&g_cbm, v, v0);
		pz = v0[2] - (n[0] * (x - v0[0]) + n[1] * (y - v0[1])) / n[2];
		if (pz < z - 0.08f || pz > z + 0.08f) continue;
		k = surf_poly(&g_cbm, &id, s, pv, 0, wn);
		if (k < 3) continue;
		for (i = 0; i < k; i++)
		{
			const float *p0 = pv[i], *p1 = pv[(i + 1) % k];
			if ((p0[1] > y) != (p1[1] > y) && x < p0[0] + (p1[0] - p0[0]) * (y - p0[1]) / (p1[1] - p0[1])) in = !in;
		}
		if (in) return 1;
	}
	return 0;
}
/* the x87 stack's depth (0 = empty, as it should be between functions) - logged with the load, a check on the floats */
static s32 x87_depth(void)
{
#ifdef HOST
	return 0;
#else
	u16 sw, tw;
	u8 env[28];
	s32 k, d = 0;
	__asm__ volatile("fnstsw %0" : "=m"(sw));
	__asm__ volatile("fnstenv %0; fldenv %0" : "+m"(env));
	tw = (u16)(env[8] | (env[9] << 8));
	for (k = 0; k < 8; k++) if (((tw >> (2 * k)) & 3) != 3) d++;
	(void)sw;
	return d;
#endif
}
static float g_ps_lo[3], g_ps_hi[2];
static s32 g_ps_x87;
static void ps_unload(void)
{
	if (g_ps_buf) ps_free(g_ps_buf);
	g_ps_buf = 0; g_pspot_n = 0; g_ps_bytes = 0; g_ps_near_n = 0; g_ps_best = -1;
}
static void ps_say(void)
{
	s32 at = lput(0, "predicted spots: ");
	if (g_ps_state == 1) { at = lput(at, g_ps_name); at = lput(at, " - "); at = lnum(at, g_pspot_n); at = lput(at, " spots (samples on the level "); at = lnum(at, (u32)g_ps_score); lput(at, "/8)"); }
	else at = lput(at, g_ps_state == 2 ? "no spots folder (T:\\spots\\index.bin)" : g_ps_state == 3 ? "no file matches this BSP" : g_ps_state == 4 ? "a file couldn't be read" : "not enough memory for this BSP's spots");
	at = lput(at, "; level x100 "); at = lsnum(at, (s32)(g_ps_lo[0] * 100.f)); at = lput(at, ".."); at = lsnum(at, (s32)(g_ps_hi[0] * 100.f));
	at = lput(at, " / "); at = lsnum(at, (s32)(g_ps_lo[1] * 100.f)); at = lput(at, ".."); at = lsnum(at, (s32)(g_ps_hi[1] * 100.f));
	at = lput(at, ", x87 depth "); lnum(at, (u32)g_ps_x87);
	log_write();
	toast(g_ps_state == 1 ? "Monster/Dreadnought Viewer: spots loaded" : g_ps_state == 2 ? "Monster/Dreadnought Viewer: no spots folder" : g_ps_state == 3 ? "Monster/Dreadnought Viewer: none for this BSP" : g_ps_state == 5 ? "Monster/Dreadnought Viewer: not enough memory" : "Monster/Dreadnought Viewer: a file couldn't be read");
}
static void ps_load_body(void)
{
	u8 *ix; u32 isz, cnt, e;
	s32 best = -1, best_score = 0, r;
	u32 best_n = 0;
	float lo[2], hi[2];
	g_ps_x87 = x87_depth();
	{
		/* the level's own bounds, from its vertices (not the viewer's grid) */
		u32 v;
		g_ps_lo[0] = g_ps_lo[1] = g_ps_lo[2] = 1e30f; g_ps_hi[0] = g_ps_hi[1] = -1e30f;
		for (v = 0; v < g_cbm.nv; v++)
		{
			float p[3];
			cb_vert(&g_cbm, v, p);
			if (p[0] < g_ps_lo[0]) g_ps_lo[0] = p[0];
			if (p[0] > g_ps_hi[0]) g_ps_hi[0] = p[0];
			if (p[1] < g_ps_lo[1]) g_ps_lo[1] = p[1];
			if (p[1] > g_ps_hi[1]) g_ps_hi[1] = p[1];
			if (p[2] < g_ps_lo[2]) g_ps_lo[2] = p[2];
		}
		if (!g_cbm.nv) { g_ps_state = 3; return; }
	}
	r = ps_read("index.bin", 0, 0, &ix, &isz);                           /* run-time memory, freed once matched */
	if (r < 0) { g_ps_state = 5; return; }
	if (r == 0) { g_ps_state = 2; return; }
	cnt = *(const u32 *)(ix + 4);
	if (!ps_magic(ix, "SGI1") || isz > PS_IDX_MAX * 4 || 8 + cnt * PS_IDX_REC > isz) { ps_free(ix); g_ps_state = 2; return; }
	lo[0] = g_ps_lo[0] - 5.f; lo[1] = g_ps_lo[1] - 5.f;
	hi[0] = g_ps_hi[0] + 5.f; hi[1] = g_ps_hi[1] + 5.f;
	for (e = 0; e < cnt; e++)
	{
		const u8 *rec = ix + 8 + e * PS_IDX_REC;
		s32 k, score = 0;
		if (ps_f(rec + 8) < lo[0] || ps_f(rec + 12) < lo[1] || ps_f(rec + 20) > hi[0] || ps_f(rec + 24) > hi[1]) continue;   /* the spots outside the level */
		if (ps_f(rec + 16) < g_ps_lo[2] - 5.f) continue;
		for (k = 0; k < 8; k++) score += ps_on_level(ps_f(rec + 32 + 12 * (u32)k), ps_f(rec + 36 + 12 * (u32)k), ps_f(rec + 40 + 12 * (u32)k));
		if (score > best_score || (score == best_score && score && *(const u32 *)(rec + 4) > best_n)) { best = (s32)e; best_score = score; best_n = *(const u32 *)(rec + 4); }
	}
	if (best < 0 || best_score < 3) { ps_free(ix); g_ps_state = 3; return; }
	{
		const u8 *rec = ix + 8 + (u32)best * PS_IDX_REC;
		u16 num = *(const u16 *)rec;
		char fn[8];
		u8 *b; u32 bsz;
		s32 k;
		fn[0] = (char)('0' + num / 10 % 10); fn[1] = (char)('0' + num % 10); fn[2] = '.'; fn[3] = 'b'; fn[4] = 'i'; fn[5] = 'n'; fn[6] = 0;
		for (k = 0; k < 40 && rec[128 + k]; k++) g_ps_name[k] = (char)rec[128 + k];
		g_ps_name[k] = 0;
		g_ps_score = best_score;
		ps_free(ix);                                                          /* (the name and number copied out) */
		r = ps_read(fn, 0, 0, &b, &bsz);
		if (r < 0) { g_ps_state = 5; return; }
		if (r == 0 || !ps_magic(b, "SGP1") || 20 + *(const u32 *)(b + 4) * PS_REC != bsz) { if (r > 0) ps_free(b); g_ps_state = 4; return; }
		g_ps_buf = b; g_ps_bytes = bsz; g_pspot_n = *(const u32 *)(b + 4);
		g_ps_org[0] = ps_f(b + 8); g_ps_org[1] = ps_f(b + 12); g_ps_org[2] = ps_f(b + 16);
		g_ps_state = 1;
	}
}
/* once a frame: (re)load for a new BSP; every 4th frame, the spots within 25 units of the view (nearest 160) */
static void pred_update(void)
{
	float p[3];
	s32 i;
	if (!o_pred) return;
	if (!cb_ready()) return;
	if (g_ps_for != g_cbm_for || g_ps_sig != g_cbm_sig || !g_ps_state)
	{
		ps_unload();
		g_ps_for = g_cbm_for; g_ps_sig = g_cbm_sig;
		g_ps_state = 0;
		if (guarded_call(ps_load_body)) { ps_unload(); g_ps_state = 4; }
		ps_say();
	}
	if (g_ps_state != 1 || (g_count_hud & 3)) return;
	view_camera(p, 0, 0);
	{
		/* every spot within the range of your view (the Range setting); more than PS_NEAR of them: the nearest PS_NEAR -
		 * counted into 64 distance bands, then taken band by band from the nearest (two passes, no sorting) */
		const float R = o_pred_range > 0 ? (float)o_pred_range : 2000.f, R2 = R * R;   /* 0: Max (every BSP's whole) */
		s32 q[3], qr = (s32)(R * 16.f), band[64], k, cut = 64, need = PS_NEAR, n_in = 0;
		float bd = 1e30f;
		for (k = 0; k < 64; k++) band[k] = 0;
		g_ps_near_n = 0; g_ps_best = -1;
		if (qr > 65535) qr = 65535;
		for (i = 0; i < 3; i++) q[i] = (s32)((p[i] - g_ps_org[i]) * 16.f);
		for (k = 0; k < 2; k++)
		{
			for (i = 0; i < (s32)g_pspot_n; i++)
			{
				const u8 *r = g_ps_buf + 20 + (u32)i * PS_REC;
				s32 dx = *(const s16 *)r - q[0], dy = *(const s16 *)(r + 2) - q[1], dz = *(const s16 *)(r + 4) - q[2], b;
				float d2;
				if (dx > qr || dx < -qr || dy > qr || dy < -qr || dz > qr || dz < -qr) continue;
				if (o_pred_dread && r[6] < 140) continue;                     /* 35+ only (x4) */
				d2 = ((float)dx * dx + (float)dy * dy + (float)dz * dz) / 256.f;
				if (d2 > R2) continue;
				b = (s32)(d2 / R2 * 64.f); if (b > 63) b = 63;
				if (k == 0) { band[b]++; n_in++; continue; }
				if (b > cut || (b == cut && need <= 0)) continue;
				if (b == cut) need--;
				if (g_ps_near_n < PS_NEAR) g_ps_near[g_ps_near_n++] = (u16)i;
				if (d2 < bd) { bd = d2; g_ps_best = i; }
			}
			if (k == 0)
			{
				/* the band where PS_NEAR runs out: all of the nearer ones, as many of it as still fit */
				s32 sum = 0;
				if (n_in <= PS_NEAR) { cut = 64; need = PS_NEAR; continue; }
				for (cut = 0; cut < 64; cut++) { if (sum + band[cut] >= PS_NEAR) break; sum += band[cut]; }
				need = PS_NEAR - sum;
			}
		}
		g_ps_best_d = g_ps_best >= 0 ? fsqrt(bd) : 0.f;
	}
}
static void ps_pos(s32 i, float *w)
{
	const u8 *r = g_ps_buf + 20 + (u32)i * PS_REC;
	w[0] = g_ps_org[0] + (float)*(const s16 *)r / 16.f; w[1] = g_ps_org[1] + (float)*(const s16 *)(r + 2) / 16.f; w[2] = g_ps_org[2] + (float)*(const s16 *)(r + 4) / 16.f;
}
/* each spot in view: its LAUNCH as an arrow up from the landing point, sized as the seam predictions' (half the root of the
 * height that speed would throw you straight up; orange a monster 20-35, magenta a dreadnought 35+) - straight up because
 * the scanner's files give the launch's size, not its direction - and its best APPROACH as an arrow in (cyan) along the
 * velocity it needs on the landing tick (the fall and the drift), ending on the spot. Every spot gets its labels: the
 * launch's u/s at its tip, the approach's speed at its tail. */
static float ps_launch_len(float up)                              /* 40% of the seam arrow's (6d.4: as big) */
{
	float h = up * up / (2.f * 4.1712f), l = 0.2f * fsqrt(h);
	return l < 0.1f ? 0.1f : l > 1.6f ? 1.6f : l;
}
#define PS_IN_LEN 0.6f                                            /* the approach arrow: one length for every spot */
/* the approach arrow: flat on the floor (just above the spot), along the drift - the way you come in - ending on the spot;
 * 0 when there's no drift (a straight drop: a cross instead) */
static s32 ps_in_arrow(const u8 *r, const float *w, float *tail, float *tip)
{
	float hx = (float)(s8)r[10], hy = (float)(s8)r[11], hl = fsqrt(hx * hx + hy * hy);
	tip[0] = w[0]; tip[1] = w[1]; tip[2] = w[2] + 0.02f;
	if (hl < 0.5f) return 0;
	tail[0] = w[0] - hx / hl * PS_IN_LEN; tail[1] = w[1] - hy / hl * PS_IN_LEN; tail[2] = tip[2];
	return 1;
}
static void ps_entry(const u8 *r, float *d, float *speed)          /* the approach: unit direction of travel, its speed */
{
	float v[3], l;
	v[0] = (float)(s8)r[10] / 10.f; v[1] = (float)(s8)r[11] / 10.f; v[2] = -(float)r[9];
	l = fsqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	*speed = l;
	if (l < 1e-3f) { d[0] = 0.f; d[1] = 0.f; d[2] = -1.f; return; }
	d[0] = v[0] / l; d[1] = v[1] / l; d[2] = v[2] / l;
}
static s32 ps_label_launch(const u8 *r) { s32 at = put_f(0, (float)r[6] / 4.f, 1); return put(at, " u/s launch"); }
static s32 ps_label_entry(const u8 *r) { float d[3], sp; s32 at; ps_entry(r, d, &sp); at = put(0, "in "); at = put_f(at, sp, 1); return put(at, " u/s"); }
static void ps_arrow(const float *from, const float *to, float head)
{
	float d[3], side[3], l, e0[3], e1[3];
	s32 m;
	for (m = 0; m < 3; m++) d[m] = to[m] - from[m];
	l = fsqrt(dot3(d, d));
	if (l < 1e-3f) return;
	for (m = 0; m < 3; m++) d[m] /= l;
	if (d[2] > -0.9f && d[2] < 0.9f) { side[0] = d[1]; side[1] = -d[0]; side[2] = 0.f; }
	else { side[0] = 1.f; side[1] = 0.f; side[2] = 0.f; }
	l = fsqrt(dot3(side, side)); if (l < 1e-3f) l = 1.f;
	for (m = 0; m < 3; m++) side[m] /= l;
	gfx_line3(from, to);
	for (m = 0; m < 3; m++) { e0[m] = to[m] - d[m] * head + side[m] * 0.5f * head; e1[m] = to[m] - d[m] * head - side[m] * 0.5f * head; }
	gfx_line3(to, e0); gfx_line3(to, e1);
}
static void pred_draw(s32 height, s32 cam)
{
	static const color4f c_mon = { 1.f, 1.f, 0.6f, 0.2f }, c_dread = { 1.f, 1.f, 0.4f, 0.95f }, c_in = { 1.f, 0.45f, 0.95f, 1.f };
	s32 pass, k;
	if (!o_pred || g_ps_state != 1 || !g_ps_near_n || !cam) return;
	g_line_half = 1.5f;
	for (pass = 0; pass < 3; pass++)                                   /* 0 monsters' launches, 1 dreadnoughts', 2 approaches */
	{
		s32 any = 0;
		for (k = 0; k < g_ps_near_n; k++)
		{
			const u8 *r = g_ps_buf + 20 + (u32)g_ps_near[k] * PS_REC;
			float w[3], t[3], d[3], sp, l;
			s32 m;
			if (pass < 2 && (r[6] >= 140) != pass) continue;
			if (!any)
			{
				if (pass == 0) gfx_begin(1.f, 1.f, 0.6f, 0.2f); else if (pass == 1) gfx_begin(1.f, 1.f, 0.4f, 0.95f); else gfx_begin(0.9f, 0.45f, 0.95f, 1.f);
				any = 1;
			}
			ps_pos(g_ps_near[k], w);
			if (pass < 2)
			{
				l = ps_launch_len((float)r[6] / 4.f);
				t[0] = w[0]; t[1] = w[1]; t[2] = w[2] + l;
				ps_arrow(w, t, l * 0.18f < 0.05f ? 0.05f : l * 0.18f);
			}
			else
			{
				/* flat on the floor, along the drift (the way you come in), one length for every spot; no drift (a
				 * straight drop): a small cross */
				float tip[3];
				if (!ps_in_arrow(r, w, t, tip))
				{
					float a0[3] = { w[0] - 0.12f, w[1], tip[2] }, a1[3] = { w[0] + 0.12f, w[1], tip[2] };
					float b0[3] = { w[0], w[1] - 0.12f, tip[2] }, b1[3] = { w[0], w[1] + 0.12f, tip[2] };
					gfx_line3(a0, a1); gfx_line3(b0, b1);
				}
				else ps_arrow(t, tip, 0.12f);
				(void)d; (void)sp; (void)m; (void)l;
			}
		}
		if (any) gfx_end();
	}
	g_line_half = 1.f;
	/* every spot in view: its labels */
	for (k = 0; k < g_ps_near_n; k++)
	{
		const u8 *r = g_ps_buf + 20 + (u32)g_ps_near[k] * PS_REC;
		float w[3], p[3], d[3], sp, sx, sy, dep, l;
		s32 m;
		if (o_pred_dread && r[6] < 140) continue;
		ps_pos(g_ps_near[k], w);
		l = ps_launch_len((float)r[6] / 4.f);
		p[0] = w[0]; p[1] = w[1]; p[2] = w[2] + l + 0.05f;                  /* the launch's: above its tip */
		project(p, &sx, &sy, &dep);
		if (dep > 0.05f && sx > -100.f && sx < (float)g_vp_w + 100.f && sy > 0.f && sy < (float)g_vp_h)
		{ ps_label_launch(r); line_at_s((s32)sx + g_vp_left - 150, (s32)sy + g_vp_top - height * 3 / 4, 300, r[6] >= 140 ? &c_dread : &c_mon, 2, 0.75f); }
		p[0] = w[0]; p[1] = w[1]; p[2] = w[2] + 0.02f;                       /* the approach's: beside its tip (the spot) */
		project(p, &sx, &sy, &dep);
		if (dep > 0.05f && sx > -100.f && sx < (float)g_vp_w + 100.f && sy > 0.f && sy < (float)g_vp_h)
		{ ps_label_entry(r); line_at_s((s32)sx + g_vp_left + 8, (s32)sy + g_vp_top - height * 3 / 8, 300, &c_in, 0, 0.75f); }
		(void)d; (void)sp; (void)m;
	}
}
#ifdef HOST
s32 host_pred(s32 *n, s32 *near_n, s32 *best, float *best_d) { *n = (s32)g_pspot_n; *near_n = g_ps_near_n; *best = g_ps_best; *best_d = g_ps_best_d; return g_ps_state; }
void host_set_pred(s32 on, s32 dread) { o_pred = on; o_pred_dread = dread; }
void host_pred_reset(void) { ps_unload(); g_ps_state = 0; }
u32 host_lg_for(void) { return g_lg_for; }
void host_set_pred_range(s32 r) { o_pred_range = r; }
s32 host_pred_near_d(s32 k, float *d) { float w[3], v[3]; ps_pos(g_ps_near[k], w); view_camera(v, 0, 0); *d = fsqrt((w[0] - v[0]) * (w[0] - v[0]) + (w[1] - v[1]) * (w[1] - v[1]) + (w[2] - v[2]) * (w[2] - v[2])); return g_ps_near[k]; }
s32 host_pred_range(void) { return o_pred_range; }
s32 host_ps_in_arrow(s32 i, float *tail, float *tip) { float w[3]; ps_pos(i, w); return ps_in_arrow(g_ps_buf + 20 + (u32)i * PS_REC, w, tail, tip); }
float host_ps_launch_len(float up) { return ps_launch_len(up); }
void host_pred_labels(s32 i, char *a, char *b) { const u8 *r = g_ps_buf + 20 + (u32)i * PS_REC; s32 n, k; n = ps_label_launch(r); for (k = 0; k < n; k++) a[k] = (char)g_text[k]; a[n] = 0; n = ps_label_entry(r); for (k = 0; k < n; k++) b[k] = (char)g_text[k]; b[n] = 0; }
void host_set_sb_hud(s32 on) { o_sb_hud = on; }
s32 host_sb_hud(void) { return o_sb_hud; }
void host_lg_forget(void) { g_lg_for = 0; g_lg_sig = 0; }
#endif

/* ---- HUD Widescreen Fix (6h / 6h.2). In widescreen the game draws its HUD for a 4:3 picture into the 640x480 frame, and
 * the TV stretches the frame 4/3 wide - so every HUD element comes out 4/3 too wide. Each element is drawn 3/4 as wide
 * around its own anchor - the point the game places it from: the safe frame's corner for the corner pieces, the centre or
 * the aim point for crosshairs - so it keeps its place and gets its shape back, and the pieces of one element (a meter and
 * its frame) stay together. The world and the rest of the picture are untouched.
 *   The anchor: the HUD's anchor function (v1.0 0x1396c7: eax anchor, esi out), as the widget drawers (bitmap 0x22aff1,
 *   shader grid 0x22b8e2, text 0x22aa16) and the centre element (0x200462) ask it once per element. Each of those drawers
 *   is called through a plain wrapper (its arguments passed on, the call made, then the squeeze ended - 6h's wrappers
 *   swapped return addresses, and the 360's emulator crashed on them). While it lasts:
 *   - a textured quad (0x22a664, ecx = its 4 corners): drawn from a copy, its x's pulled toward the anchor by 3/4;
 *   - a shader widget's 3x3 grid (0x2d140, its 4th argument: 9 x {x, y, u, v}): every point moved the same way, its texture
 *     coordinates as the game made them (6h.2 changed them instead: bullets drawn half-filled);
 *   - a text (0x1fb10): moved with the squeeze, its letters their own width (6h.2's wider window lost the text);
 *   - the motion tracker's quad (its own texture put on the screen): pulled toward the bottom-left anchor, as its frame.
 *   6h.1 narrowed the Direct3D viewport instead: on the Xbox it changed nothing (the HUD's shaders place it themselves).
 *   Only in a game (campaign or multiplayer) and only while the game itself draws in widescreen (its own test, 0x16a90). ---- */
#define HSQ_K 0.75f
volatile s32 g_hsq_in;                             /* inside an element's drawer */
volatile s32 g_hsq_on;                             /* its anchor known: squeezed around g_hsq_ax */
float g_hsq_ax;
static float g_hsq_quad[4][8];                     /* the corners passed on - a few in turn */
static u32 g_hsq_qi;
static s32 g_hsq_said;
#ifdef HOST
static s32 hsq_widescreen(void) { u32 g = M32(ADDR_GAME_OPTIONS_PTR); return M8(ADDR_WIDESCREEN_FLAG) && ptr_ok(g) && (M32(g + 8) == 1 || M32(g + 8) == 2); }
#else
static s32 hsq_widescreen(void) { u32 g = M32(ADDR_GAME_OPTIONS_PTR); return ptr_ok(g) && (M32(g + 8) == 1 || M32(g + 8) == 2) && (gfx_call0(ADDR_WIDESCREEN_FN) & 0xff); }
#endif
s32 g_hsq_type;                                     /* hanc_hook: the anchor kind asked for (eax): 2 bottom-left */
static s32 g_hsq_bl;
void hsq_anchor(const float *out)                   /* after the game worked out an element's anchor */
{
	g_hsq_on = 0; g_hsq_bl = 0;
	if (!g_hsq_in || !o_hud_ws || !hsq_widescreen()) return;
	g_hsq_ax = out[0];
	if (!(g_hsq_ax > -4096.f && g_hsq_ax < 4096.f)) return;
	g_hsq_on = 1;
	g_hsq_bl = g_hsq_type == 2;
	if (!g_hsq_said) { g_hsq_said = 1; lput(lput(0, "HUD Widescreen Fix: "), "the first HUD element squeezed around its anchor"); log_write(); }
}
static float hsq_x(float x) { return g_hsq_ax + (x - g_hsq_ax) * HSQ_K; }
u32 hsq_quad(u32 corners)
{
	float *q;
	s32 i;
	if (!g_hsq_on || !corners) return corners;
	q = g_hsq_quad[g_hsq_qi++ & 3];
	for (i = 0; i < 4; i++) { q[2 * i] = hsq_x(MF(corners + 8 * (u32)i)); q[2 * i + 1] = MF(corners + 8 * (u32)i + 4); }
	if (g_hsq_bl)
	{
		/* 6h.13: an element anchored bottom-left (the motion tracker's frame, the bar on it): its right edge 2 pixels further
		 * out - the frame's right side showed cut off by about that much */
		float cx = 0.25f * (q[0] + q[2] + q[4] + q[6]);
		for (i = 0; i < 4; i++) if (q[2 * i] > cx) q[2 * i] += 2.f;
	}
	return (u32)(unsigned long)q;
}
void hsq_grid(u32 v)                                   /* 6h.3: the points moved, the texture coordinates left as the game made them */
{
	u32 i;
	if (!g_hsq_on || !v) return;
	for (i = 0; i < 9; i++)
	{
		float x = MF(v + i * 16);
		if (!(x > -8192.f && x < 8192.f)) return;
	}
	for (i = 0; i < 9; i++) gwf(v + i * 16, hsq_x(MF(v + i * 16)));
}
void hsq_text_before(u32 rect, u32 frame)             /* rect16 {top, left, bottom, right}, viewport-relative; the drawer's frame */
{
	/* the text's place follows the squeeze, its letters keep their width - always toward the anchor, so never off the screen
	 * (6h.2 drew it as if the window were wider and moved it out past the screen's edge: the game skipped it). 6h.4: moved
	 * by the point the game places it from - its left edge, centre or right edge as it's justified (the text drawer keeps
	 * that point at [ebp-8], viewport-relative, v1.0 0x22aa16) - not its centre: a number placed by its left edge stays on
	 * its mark (6h.3's centre left it 1/8 of its width off, into its box's edge) */
	s32 l, r, sh;
	float axr, pos;
	if (!g_hsq_on || !rect) return;
	l = (s16)M16(rect + 2); r = (s16)M16(rect + 6);
	axr = g_hsq_ax - (float)(s16)M16(ADDR_HUD_VIEWPORT_X0);
	pos = frame ? MF(frame - 8) : 0.f;
	if (!frame || !(pos > (float)l - 2048.f && pos < (float)r + 2048.f && pos > -4096.f && pos < 4096.f)) pos = 0.5f * (float)(l + r);
	{
		/* a centred text (its placement point its middle - the drawer put it at pos - w/2 .. pos + w/2) beside its anchor:
		 * squeezed by the point halfway between its middle and its edge nearest the anchor - its letters' extra width split
		 * 3 to 1, away from the anchor. (By its middle (6h.3) both ammo counts sat a pixel toward the screen's edges; by its
		 * anchor-side edge (6h.5) a pixel toward the middle.) One around its anchor (a message in the middle of the screen)
		 * stays where it is. A text placed by an edge keeps that edge (6h.4). */
		float c = 0.5f * (float)(l + r), dl = pos - (float)l, dc = pos - c, dr = pos - (float)r;
		if (dl < 0.f) dl = -dl;
		if (dc < 0.f) dc = -dc;
		if (dr < 0.f) dr = -dr;
		if (dc < dl && dc < dr)
		{
			/* 6h.10: by its side - right of its anchor (a left-side element: the left weapon's count) by the point halfway to
			 * its left edge (6h.9: right where you wanted it); left of its anchor (a right-side element: the right weapon's
			 * count) by its middle (6h.4: where it always was) */
			if (axr < (float)l) pos = 0.5f * (c + (float)l);
			else if (axr > (float)r) pos = c;
			else pos = axr;
		}
	}
	sh = (s32)((axr - pos) * (1.f - HSQ_K) + (axr >= pos ? 0.5f : -0.5f));
	gw16(rect + 2, (u16)(s16)(l + sh));
	gw16(rect + 6, (u16)(s16)(r + sh));
}
/* the motion tracker: drawn into its own texture, then put on the screen as one quad (v1.0 0x2548f0: 4 vertices through
 * SetVertexData4f) - that quad's x's pulled toward the bottom-left anchor, as its frame is */
static volatile s32 g_trk_on;
static float g_trk_ax;
void hsq_track_begin(void)
{
	g_trk_on = 0;
	if (!o_hud_ws || !hsq_widescreen()) return;
	g_trk_ax = (float)(s16)M16(ADDR_HUD_FRAME_X0);
	g_trk_on = 1;
}
void hsq_track_end(void) { g_trk_on = 0; }
void hsq_trkv(float *x) { if (g_trk_on && *x > -8192.f && *x < 8192.f) *x = g_trk_ax + (*x - g_trk_ax) * HSQ_K; }
#ifdef HOST
void host_set_hud_ws(s32 on) { o_hud_ws = on; }
s32 host_hsq_on(void) { return g_hsq_on; }
void host_hsq_in(s32 in) { g_hsq_in = in; if (!in) g_hsq_on = 0; }
void host_hsq_type(s32 t) { g_hsq_type = t; }
s32 host_trk_on(void) { return g_trk_on; }
float host_hsq_quad_x(u32 corners, s32 i) { u32 q = hsq_quad(corners); return q == corners ? MF(corners + 8 * (u32)i) : g_hsq_quad[(g_hsq_qi - 1) & 3][2 * i]; }
#endif
/* the game's main menu: the game options' state (+8) is 3 - 1 is the campaign, 2 multiplayer (the window manager's own
 * main menu code tests it so). 6f.3 took "no player record" - the main menu has one too: the hint never showed. */
static s32 on_main_menu(void)
{
	u32 g = M32(ADDR_GAME_OPTIONS_PTR);
	return ptr_ok(g) && M32(g + 8) == 3;
}
void spooky_frame(void)
{
	g_frames++;
	g_count_hud++;
	s32 hint = o_hint && on_main_menu();         /* the "Back + D-pad Up" hint: on the game's main menu only (its state, 3) -
	                                                 never in a level, dead or alive (6f.2 showed it while dead); the launcher's
	                                                 "Hide hint line" hides it entirely. 6h.6: on every screen of it (lobbies
	                                                 too), drawn as low as the info line in a game, below the menus' panels */
	s32 hud = o_sb_hud && g_sb_charged >= 0;     /* in a level, with a player */
	s32 marker = o_marker && g_sb_object && g_sb_charged >= 0;
	s32 seams = o_sb_hud && g_sb_object && g_sb_charged >= 0;
	{ static s32 was_on; if (seams && !was_on) { g_trace_until = g_count_main + 90; g_trace_on = 1; } was_on = seams; }   /* turned on: 3 s of trace, from this very search */     /* the Superbounce Viewer: the search, the landing, the trajectory -
	                                                                  "Seams near you" only draws the seams themselves (5e.2 hid all of it) */
	if (!g_open && !hint && !hud && !marker && !seams && !o_ghost && !o_oob && !o_sleep && !o_pred && !g_toast_frames) return;   /* 6g.2: and a message pending (Launch's speed, Passion Check...): it showed only with another overlay on */   /* (6a.2 left out the Ghost Viewer and the
	                                                 timers: with the hint line and the Superbounce Viewer off, they stopped drawing) */

	u8 saved[0x64];                              /* text_style_clear zeroes exactly 0x64 bytes (25 dwords) */
	memcpy(saved, DRAW_STRING_GLOBALS, sizeof(saved));
	text_style_clear();

	s32 font = 9;                                 /* the font the game's own HUD messages use */
	s32 height = 16;
	u8 *header = (u8 *)font_get(FONT_HANDLES[font]);
	if (header) height = *(s16 *)(header + 4) + *(s16 *)(header + 6) + *(s16 *)(header + 8);
	if (!g_logged_hud)
	{
		g_logged_hud = 1;
		s32 at = lput(0, "HUD-text hook running: render target flag (must be 0) ");
#ifndef HOST
		at = lnum(at, G_485602); at = lput(at, ", skip flag (must be 0) "); at = lnum(at, G_51080C);
#endif
		at = lput(at, ", font 9 handle "); at = lhex(at, FONT_HANDLES[font]);
		at = lput(at, ", font header "); at = lhex(at, (u32)(unsigned long)header);
		at = lput(at, ", line height "); lnum(at, (u32)height);
		log_write();
	}
	g_full_height = height;
	height = height * o_scale / 10;
	if (height < 8) height = 8;
	*(s32 *)(DRAW_STRING_GLOBALS + 0x00) = font;
	*(u32 *)(DRAW_STRING_GLOBALS + 0x04) = 1;     /* wrap only where breaking is allowed (as the game's HUD text) */
	*(s32 *)(DRAW_STRING_GLOBALS + 0x08) = -1;    /* no style */
	*(s32 *)(DRAW_STRING_GLOBALS + 0x0c) = 0;     /* left-justified */

	s32 cam = camera_read();
	if (g_open)
	{
		s32 x = 60, y = 48, i, rows, first, last, shown;
		const s_menu *mn = &g_menus[g_menu];
		/* the rows that fit between the title and the footer (480 lines, 36 kept clear at the bottom); a longer list
		 * scrolls with the selection, a "more" line marking each end that has more */
		rows = (480 - 36 - y - 3 * height) / height;
		if (g_rows_cap > 0 && rows > g_rows_cap) rows = g_rows_cap;
		if (rows < 3) rows = 3;
		first = g_first[g_menu];
		if (mn->n <= rows) first = 0;
		else
		{
			s32 inner = rows - 2;                                             /* room left beside the two "more" lines */
			if (g_selected < first + 1) first = g_selected - 1;
			if (g_selected > first + inner) first = g_selected - inner;
			if (first < 0) first = 0;
			if (first > mn->n - rows) first = mn->n - rows;
		}
		g_first[g_menu] = first;
		last = mn->n <= rows ? mn->n : first + rows;
		shown = last - first;
		if (cam)
		{
			gfx_begin(0.55f, 0.f, 0.f, 0.f);                          /* a dark panel behind the menu */
			{
				/* the right edge mirrors the left margin (6b.1 ran it to x + 600 - 660 on a 640-wide screen: off the right) */
				s32 pl = x - 12 - g_vp_left;
				gfx_rect(pl, y - 8 - g_vp_top, g_vp_w - pl, y + height * (shown + 4) - g_vp_top);
			}
			gfx_end();
		}
		put(0, mn->title);
		line(x, y, height, &c_title);
		y += height + height / 2;
		for (i = first; i < last; i++)
		{
			const s_item *it = &mn->items[i];
			if (mn->n > rows && ((i == first && first > 0) || (i == last - 1 && last < mn->n)))
			{
				put(0, i == first ? "  ... more above" : "  ... more below");
				line(x, y, height, &c_hint);
				y += height;
				continue;
			}
			s32 at = put(0, i == g_selected ? "> " : "  ");
			at = put(at, it->label);
			if (it->kind == K_TOGGLE) at = put(at, *it->value ? ": on" : ": off");
			else if (it->kind == K_INT || it->kind == K_INT10)
			{
				at = put(at, ": ");
				if (it->value == &o_scale || it->value == &o_speed) { at = put_int(at, *it->value / 10); at = put(at, "."); at = put_int(at, *it->value % 10); }
				else if (it->kind == K_INT10 && *it->value == 0) at = put(at, "Max");       /* 6h.18: the range's Max */
				else at = put_int(at, *it->value);
			}
			line(x, y, height, i == g_selected ? &c_selected : &c_item);
			y += height;
		}
		y += height / 2;
		put(0, g_menu ? "D-pad: select/change   A: toggle/enter   B: back" : "D-pad: select/change   A: toggle/enter   B or Back+Up: close");
		line(x, y, height, &c_hint);
		if (g_toast_frames > 0)                                              /* 6h.5: an action that leaves the menu open: its result */
		{
			put(0, g_toast);
			line(x, y + height + height / 2, height, &c_selected);
			g_toast_frames--;
		}
	}
	else
	{
		s32 y_hint = 480 - 36 - height, y_charge = y_hint - height - 4, y_speed = y_charge - height;
		if (seams && cam)
		{
			u32 t0 = tsc_lo(), dt;
#ifdef HOST
			s32 search = 1;
#else
			s32 search = !(g_count_hud % (u32)g_search_div);   /* every Nth frame (2, slower hardware more); drawn every frame */
#endif
			s32 step;                                        /* -1 none this frame; 9 all four; 0 seams / 1 landing / 2 rating / 3 view */
#ifdef HOST
			step = 9;
			if (g_host_step_div) { u32 m = g_count_hud % (u32)g_host_step_div; step = g_host_step_div == 3 ? (m == 2 ? 23 : (s32)m) : m < 4 ? (s32)m : -1; }   /* as on a slow console */
#else
			if (g_search_div <= 2) step = search ? 9 : -1;
			else { u32 m = g_count_hud % (u32)g_search_div; step = g_search_div == 3 ? (m == 2 ? 23 : (s32)m) : m < 4 ? (s32)m : -1; }   /* 3: rating and view together (6h.20) */
#endif
			if (step == 9 || step == 0) { if (guarded_call(sb_step_land)) sb_faulted(); }   /* the landing first: the seams use it */
			if (step == 9 || step == 1) { if (guarded_call(sb_step_find)) sb_faulted(); }
			if (step == 9 || step == 2 || step == 23) { if (guarded_call(sb_step_rate)) sb_faulted(); }
			if (step == 9 || step == 3 || step == 23) { if (guarded_call(sb_step_view)) sb_faulted(); }
			if (step != 9 && step != 0 && g_sb_airborne && g_arc_n > 0)
			{
				/* between searches the trajectory follows your motion each frame (the path alone: cheap, no surfaces); the
				 * landing cursor keeps the last search's */
				s32 keep_n = g_arc_n;
				path_compute(g_sb_object);
				g_arc_n = keep_n < g_pts_n ? keep_n : g_pts_n;
			}
			if (o_seams) { u32 td = tsc_lo(); seams_draw_d8(); g_cyc_draw += tsc_lo() - td; }
			ld_swap(); arc_draw(); ld_swap();                 /* the landing drawn as last finished, with its seams */
			dt = tsc_lo() - t0;
			g_seam_cyc_sum += dt; g_seam_cyc_n++;
			if (dt > g_seam_cyc_max) g_seam_cyc_max = dt;
		}
		if (cam && o_ghost) { trace("draw start"); ghost_draw(); trace("draw end"); }   /* the ghost pill */
		oob_update(); oob_draw(height);                                     /* the out-of-bounds timer */
		sleep_update(); sleep_draw(height);                                 /* the sleep timer */
		pred_update(); pred_draw(height, cam != 0);                         /* the predicted monster / dreadnought spots */
		if (marker && cam) draw_marker(g_sb_object);
		if (hint)                                                            /* 6h.6: on the info line's row (the lowest); 6h.8: */
		{                                                                    /* in the bottom-left corner, as far from the screen's left
		                                                                        edge as its letters are from the bottom (~12 lines; a pixel
		                                                                        here is 4/3 as wide on a 16:9 TV - 9 px) */
			put(0, "Spooky Ghost: Back + D-pad Up opens the menu in game.");
			line(9, y_hint + height + 4, height, &c_hint);
			g_toast_frames = 0;                                              /* (a game's last message: not over it) */
		}
		if (g_toast_frames > 0)                                              /* a practice action's result, 2 s */
		{
			put(0, g_toast);
			line(60, y_hint + height + 4, height, &c_hint);
			g_toast_frames--;
		}
		if (hud)
		{
			put(0, g_sb_charged ? "CHARGED" : "not charged");
			line_at(170, y_charge, 300, g_sb_charged ? &c_charged : &c_uncharged, 2);
			{
				/* 6h.16: the last superbounce's speed, on the charge state's line, to its left - right-aligned to end clear of
				 * "not charged" (centred at 320, about 5.5 letters each side) at any text scale */
				s32 right = 320 - (s32)(5.5f * (float)o_scale) - 12, at = put(0, "Last Bounce: ");
				if (g_sb_have_last) { s32 t = (s32)(g_sb_last_bounce * 10.f + 0.5f); at = put_int(at, t / 10); at = put(at, "."); at = put_int(at, t % 10); put(at, "u/s"); }
				else put(at, "--");
				line_at(right - 250, y_charge, 250, &c_speed, 1);
			}
			if (o_sb_speed && g_sb_speed >= 0.f) { put_speed(0, g_sb_speed); line_at(170, y_speed, 300, &c_speed, 2); }
			if (g_sb_airborne && g_land_ok && g_ld_arrow && g_arrow_lbl && g_ld_up > 0.05f)
			{
				/* the arrow's label (sb_arrow: "N u/s up, H wu high"; sb_dir_arrow: "N u/s UP / DOWN / SIDEWAYS") */
				s32 at = put_int(0, (s32)(g_ld_up + 0.5f));
				if (g_arrow_isup) { at = put(at, " u/s up, "); at = put_f(at, g_ld_up * g_ld_up / (2.f * 4.1712f), 1); put(at, " wu high"); }
				else put(at, g_ld_dir[2] > 0.7f ? " u/s UP" : g_ld_dir[2] < -0.5f ? " u/s DOWN" : " u/s SIDEWAYS");
				line_at((s32)g_arrow_sx + g_vp_left - 150, (s32)g_arrow_sy + g_vp_top - height, 300, &c_land[g_ld_q == 3 ? 3 : g_ld_q], 2);
			}
			if (o_land_text && seams)
			{
				s32 li;
				for (li = 0; li < g_lbl_n; li++)
				{
					s_shown *e;
					if (g_lbl_idx[li] < 0) continue;
					e = &g_lbl_e[g_lbl_idx[li]];
					seam_label(e);
					line_at((s32)g_lbl_x[li] + g_vp_left - 260, (s32)g_lbl_y[li] + g_vp_top - height, 520, &c_land[e->q == 3 ? 3 : e->q], 2);
					if (g_lbl_idx[li] == g_lbl_first && e->q >= 1 && e->r.down > 0.f)
					{
						/* Cartographer: "biggest kick: drift across it at about X u/s at contact (steer to it: ...)" */
						s32 at = put(0, "biggest kick: drift across it at about "); at = put_f(at, 0.0612f * e->r.down, 2);
						put(at, " u/s at contact (steer to it: with, against or off the stick)");
						line_at((s32)g_lbl_x[li] + g_vp_left - 260, (s32)g_lbl_y[li] + g_vp_top - 2 * height, 520, &c_land[e->q == 3 ? 3 : e->q], 2);
					}
				}
			}
			if (o_ghost && o_tim_labels && g_tim_lbl_n)
			{
				/* each child polygon's TIM (units), its state's colour */
				s32 li;
				for (li = 0; li < g_tim_lbl_n; li++)
				{
					s_kid *kd = &g_kids[g_tim_lbl_i[li]];
					s32 at = put_f(0, kd->tim, 3);
					if (kd->state == 2) { at = put(at, " +"); put_f(at, kd->over, 3); }
					line_at((s32)g_tim_lbl_x[li] + g_vp_left - 40, (s32)g_tim_lbl_y[li] + g_vp_top - height / 2, 80, kd->state == 2 ? &c_land[0] : kd->state == 1 ? &c_land[1] : &c_land[2], 1);
				}
			}
			if (o_land_text && g_sb_airborne && g_land_ok && g_ld_q >= 0)
			{
				/* Cartographer's landing text: land X off SHAPE seam, speed, depth (late), tilt - then the verdict */
				s32 at = 0, q = g_ld_q;
				if (g_ld_seam)
				{
					at = put(at, "land "); at = put_f(at, g_ld_off, 2); at = put(at, " off ");
					at = put(at, g_ld_bend >= 1.f ? "valley" : g_ld_bend <= -1.f ? "ridge" : "flat"); at = put(at, " seam, ");
				}
				else at = put(at, "land: ");
				at = put_f(at, g_ld_speed, 2); at = put(at, " u/s, depth "); at = put_f(at, g_ld_have_depth ? g_ld_depth : 0.f, 2);
				at = put(at, " (late "); at = put_f(at, g_ld_have_depth ? g_ld_late : 0.f, 2); at = put(at, "), tilt ");
				at = put_f(at, g_ld_tilt * 57.29578f, 1);
				if (g_ld_bent) { at = put(at, " + bend "); at = put_f(at, g_ld_yaw * 57.29578f, 1); }
				line_at(60, y_speed - 2 * height, 560, &c_land[q == 3 ? 3 : q], 2);
				put(0, g_ld_why);
				line_at(60, y_speed - height, 560, &c_land[q == 3 ? 3 : q], 2);
				if (g_ld_seam && g_ld_speed >= SB_NEED && g_ld_down > 0.1f)
				{
					/* Cartographer: "best: drift across the seam at about X u/s (now Y)" */
					s32 at = put(0, "best: drift across the seam at about "); at = put_f(at, 0.0612f * g_ld_down, 2);
					at = put(at, " u/s (now "); at = put_f(at, ftan(g_ld_tilt) * g_ld_down, 2); put(at, ")");
					line_at(60, y_speed - 3 * height, 560, &c_speed, 2);
				}
			}
			if (o_sb_streak)
			{
				s32 at = put(0, "Bounces in a row: "); put_int(at, g_sb_streak);
				line_at(400, y_speed, 220, g_sb_streak > 0 ? &c_streak : &c_speed, 0);
				at = put(0, "Level record: "); put_int(at, g_sb_record);
				line_at(400, y_charge, 220, &c_record, 0);
			}
		}
	}

	memcpy(DRAW_STRING_GLOBALS, saved, sizeof(saved));
}

/* ---- the trainer: installed once by the launcher (see etm_stub.S), then a per-frame main-loop hook ---- */
#ifdef SPOOKY_TRAINER
static u32 g_installed;            /* bit per hook site patched */
#endif
#ifdef SPOOKY_TRAINER
extern void hud_hook(void), end_a_hook(void), end_b_hook(void), input_hook(void), main_hook(void);

static s32 patch_call(u32 site, u32 expect, void (*hook)(void), u32 bit)
{
	u8 *p = (u8 *)site;
	if (p[0] != 0xE8 || site + 5 + *(s32 *)(p + 1) != expect) return 0;      /* not the call we expect: leave it */
	*(s32 *)(p + 1) = (s32)((u32)hook - (site + 5));
	g_installed |= bit;
	return 1;
}

/* called once by etm_stub.S with the trainer's base and its option flags (one byte per launcher option) */
void spooky_install(const u8 *options)
{
	if (g_installed) return;
	o_log = !options[0];               /* option 0: Disable log */
	o_hint = !options[1];              /* option 1: Hide hint line */
	patch_call(0x2c86e, 0x13c780, hud_hook, 1);       /* after the game's HUD text: draw the menu */
	patch_call(0x124c34, 0x409b6c, input_hook, 2);    /* the game's only XInputGetState call */
	patch_call(0x2b6f7, 0x13cd0, end_a_hook, 4);      /* end of frame, route A (counted) */
	patch_call(0x2c9e9, 0x13cd0, end_b_hook, 8);      /* end of frame, route B (counted) */
	{
		/* the main loop's 6-byte filler `lea ebx,[ebx+0]` before the frame update (Yelo's per-frame hook) */
		u8 *p = (u8 *)0x12b6aa;
		if (p[0] == 0x8D && p[1] == 0x9B && !p[2] && !p[3] && !p[4] && !p[5])
		{
			*(s32 *)(p + 1) = (s32)((u32)main_hook - (0x12b6aa + 5));
			p[5] = 0x90;
			p[0] = 0xE8;
			g_installed |= 16;
		}
	}
	g_installed |= 0x80000000;
}
#endif

/* every frame, from the main loop (before the game's frame update) */
/* ---- the skulls (Cheats > Skulls): bool[16] at ADDR_SKULLS, which every skull effect reads. A skull turned on here is set
 * each frame (a new map clears them); turned off, only the ones set here are cleared - a skull you picked up in a level
 * stays. In multiplayer: every effect also requires the campaign (game mode 1) - 17 of those tests are patched out in the
 * XBE, the ones where "not the campaign" and "skull off" lead to the same place (so nothing changes with the skull off);
 * Envy's and Iron's aren't (Envy's test also guards the flashlight's own camo; Iron reverts to a checkpoint). ---- */
static u16 g_skull_mine;
static void apply_skulls(void)
{
	s32 i;
	for (i = 0; i < 15; i++)
	{
		u32 a = ADDR_SKULLS + (u32)i;
		if (o_skull[i]) { if (!M8(a)) gw8(a, 1); g_skull_mine |= (u16)(1u << i); }
		else if (g_skull_mine & (1u << i)) { gw8(a, 0); g_skull_mine &= (u16)~(1u << i); }
	}
}
/* ---- Invincible Vehicles: every vehicle (object type 1, the object table entry's +3) gets the game's own cannot-take-
 * damage + cannot-die flags, as Invulnerability gives you; turned off, each gets back what it had ---- */
#define IVV_MAX 128
static s32 g_ivv_datum[IVV_MAX];
static u16 g_ivv_was[IVV_MAX];
static s32 g_ivv_n, g_ivv_said;
/* the region damage hook asks: a vehicle, with Invincible Vehicles on? Then its parts take no damage (no bumper, wing or
 * panel breaks off) - the cannot-take-damage flag keeps the body whole, but the region code doesn't look at it */
s32 rdmg_skip(u32 object_index)
{
	u32 pool, data, count, k, h;
	if (!o_inv_veh || object_index == 0xFFFFFFFFu) return 0;
	pool = M32(ADDR_OBJECTS_PTR);
	if (!ptr_ok(pool)) return 0;
	data = M32(pool + 0x44); count = M32(pool + 0x20); k = object_index & 0xffff;
	if (!ptr_ok(data) || k >= count) return 0;
	h = data + k * 12;
	return (M32(h) & 0xffff) == (object_index >> 16) && M8(h + 3) == 1;
}
static void apply_inv_veh(void)
{
	u32 pool = M32(ADDR_OBJECTS_PTR), data, count, k;
	s32 n = 0;
	if (!o_inv_veh)
	{
		s32 i;
		for (i = 0; i < g_ivv_n; i++)
		{
			u32 obj = object_from_datum(g_ivv_datum[i]);
			if (obj) gw16(obj + 0x10A, (u16)((M16(obj + 0x10A) & ~IV_BITS) | g_ivv_was[i]));
		}
		g_ivv_n = 0; g_ivv_said = 0;
		return;
	}
	if (!ptr_ok(pool)) return;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || count > 8192) return;
	for (k = 0; k < count; k++)
	{
		u32 h = data + k * 12, obj, id = M32(h) & 0xffff;
		if (!id || M8(h + 3) != 1) continue;                               /* empty, or not a vehicle */
		obj = M32(h + 8);
		if (!ptr_ok(obj)) continue;
		n++;
		if ((M16(obj + 0x10A) & IV_BITS) == IV_BITS) continue;
		if (g_ivv_n < IVV_MAX) { g_ivv_datum[g_ivv_n] = (s32)((id << 16) | k); g_ivv_was[g_ivv_n++] = (u16)(M16(obj + 0x10A) & IV_BITS); }
		gw16(obj + 0x10A, (u16)(M16(obj + 0x10A) | IV_BITS));
	}
	if (!g_ivv_said && n) { s32 at = lput(0, "invincible vehicles: "); at = lnum(at, (u32)n); lput(at, " vehicles protected"); log_write(); g_ivv_said = 1; }
}
/* ---- Acrophobia: the debug build's cheat_jetpack (halo2_tag_test.exe 0x8b6384), once a game tick, on foot.
 * 6h.29: A on the ground is the game's own jump, as ever (held through it: nothing more). A pressed while you're in the
 * air (the biped's airborne bit, +0x348), then held: you fly where you look - s = your speed along your view (0 if
 * backwards), a = min(1, s / 16), thrust 9 + 9a - 18a^2 u/s^2 (9 at rest, 10.1 at 4 u/s, 0 at 16); the velocity off your
 * view loses 20% a tick. Let go of A, or land, and the flight ends. Crouch held in the air (not flying): you hover - your
 * velocity x -0.2 (80% of it gone a tick). Flying or hovering, gravity is cancelled (its pull for the tick added back).
 * (6e flew only with crouch held; 6h.28 dropped the hover and flew off a held jump.) Not while this menu has the pad
 * (A selects there), nor while the free camera holds your player still. The velocity is set on the object and its Havok
 * bodies, as Launch does. ---- */
static u32 p1_handle(void) { return M32(ADDR_PAD_HANDLES); }
static u32 g_acro_tick;
static u8 g_acro_a_was, g_acro_fly;            /* 6h.29: A as last tick (a press is a change), flying */
static void acro_tick(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), obj, tick, ents[16];
	s32 tps, n, i;
	float v[3], d[3], l, dt, sp, a, t;
	if (!o_acro || !ptr_ok(time)) return;
	tick = M32(time + 8);
	if (tick == g_acro_tick) return;                                      /* once a tick */
	g_acro_tick = tick;
	{
		u8 a_now = g_p1_a > ANALOG_DOWN, press = a_now && !g_acro_a_was, air;
		g_acro_a_was = a_now;
		obj = player_object();
		if (!obj || (s32)M32(obj + 0x14) != -1 || g_open || g_suppress || (o_free_cam && o_fc_block))
		{ g_acro_fly = 0; return; }                                       /* on foot only; A is this menu's; the free camera holds you */
		air = M8(obj + 0x348) & 1;
		if (!air || !a_now) g_acro_fly = 0;                               /* landed, or let go */
		else if (press) g_acro_fly = 1;                                   /* A pressed in the air (the jump's own press was on the ground) */
		if (!g_acro_fly && !(air && (g_p1_buttons & BTN_LS))) return;    /* neither flying nor hovering (crouch in the air) */
	}
	tps = (s32)(s16)M16(time + 2); if (tps <= 0 || tps > 3000) tps = 30;
	dt = 1.f / (float)tps;
	for (i = 0; i < 3; i++) v[i] = MF(obj + 0x88 + 4 * (u32)i);
	if (g_acro_fly)
	{
		for (i = 0; i < 3; i++) d[i] = MF(ADDR_PLAYER_CAMERAS + 0x20 + 4 * (u32)i);   /* where you look */
		l = fsqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
		if (l < 1e-4f) return;
		for (i = 0; i < 3; i++) d[i] /= l;
		sp = d[0] * v[0] + d[1] * v[1] + d[2] * v[2]; if (sp < 0.f) sp = 0.f;
		a = sp * 0.0625f; if (a > 1.f) a = 1.f;
		t = a * 9.f - a * a * 18.f + 9.f;
		for (i = 0; i < 3; i++) v[i] = v[i] - (v[i] - sp * d[i]) * 0.2f + d[i] * t * dt;
	}
	else for (i = 0; i < 3; i++) v[i] *= -0.2f;                          /* hovering */
	v[2] += 4.1712f * dt;                                                 /* gravity cancelled */
	for (i = 0; i < 3; i++) gwf(obj + 0x88 + 4 * (u32)i, v[i]);
	n = object_bodies(obj, ents);
	for (i = 0; i < n; i++) set_body_velocity(M32(ents[i] + 0x3C), v);
}
#ifdef HOST
void host_set_skull(s32 i, s32 on) { o_skull[i] = on; }
s32 host_skull(s32 i) { return o_skull[i]; }
void host_apply_skulls(void) { apply_skulls(); }
void host_set_inv_veh(s32 on) { o_inv_veh = on; }
void host_apply_inv_veh(void) { apply_inv_veh(); }
void host_set_acro(s32 on) { o_acro = on; }
void host_acro_tick(void) { acro_tick(); }
void host_set_p1(u16 b, u8 a) { g_p1_buttons = b; g_p1_a = a; }
#endif

/* ---- Bump Possession: the debug build's cheat_bump_possession (halo2_tag_test.exe: a unit no player controls - a biped
 * or a vehicle - bumping yours long enough: player_set_unit(you, it)). The retail Xbox keeps no bump record, so the touch is
 * checked here, once a tick: a biped within 0.7 of you across and 1.2 up / down; a vehicle with you well inside its
 * bounding sphere (+0x30 centre, +0x3C radius: 75% of it across, all of it up / down). Not you, not the vehicle you're in,
 * not a dead one (damage flags bit 2), and not the body you just left until you're 3 units from it. Three ticks of the same
 * touch, then the game's own player_set_unit (v1.0 0x14cad0, v1.5 0x14f620 - the player pushed, the unit in ebx: it
 * detaches the old unit and hands the new one your control), under the crash guard; then a second's rest. ---- */
#ifdef HOST
static u32 g_host_su_player, g_host_su_unit;
static s32 g_host_su_n;
static void game_set_unit(void *fn, u32 player, u32 unit) { (void)fn; g_host_su_player = player; g_host_su_unit = unit; g_host_su_n++; }
s32 host_set_unit_calls(u32 *player, u32 *unit) { *player = g_host_su_player; *unit = g_host_su_unit; return g_host_su_n; }
#else
extern void game_set_unit(void *fn, u32 player, u32 unit);
#endif
static u32 g_bp_tick, g_bp_rest, g_bp_left, g_bp_cand, g_bp_player, g_bp_unit;
static u32 g_bp_home = 0xFFFFFFFFu;             /* the body you were in before your first possession ("Revert to player") */
static s32 g_bp_count;
extern s32 g_bp_switching_c;                    /* 6h.14: the game switching your body - the camera hooks give its own answers */
static void bp_call(void) { g_bp_switching_c = 1; game_set_unit((void *)ADDR_SET_UNIT, g_bp_player, g_bp_unit); }
static s32 possess_call(void) { s32 bad = guarded_call(bp_call); g_bp_switching_c = 0; return bad; }   /* (cleared even if caught) */
static s32 g_bp_inposs;                          /* 6h.14: in a possessed unit (bump / cycle) - g_bp_home is your own body, kept */
static s32 unit_ok(u32 datum)                    /* a biped or vehicle, there and alive */
{
	u32 pool = M32(ADDR_OBJECTS_PTR), data, h, k = datum & 0xffff;
	if ((s32)datum == -1 || !ptr_ok(pool) || !object_from_datum((s32)datum)) return 0;
	data = M32(pool + 0x44);
	if (!ptr_ok(data) || k >= M32(pool + 0x20)) return 0;
	h = data + k * 12;
	if ((M32(h) & 0xffff) != (datum >> 16) || M8(h + 3) > 1) return 0;
	return !(M16(object_from_datum((s32)datum) + 0x10A) & 4);
}
static void bump_tick(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), lp = M32(ADDR_LOCAL_PLAYERS_PTR), pool = M32(ADDR_OBJECTS_PTR), tick, player, data, count, k, me, cand = 0xFFFFFFFFu;
	s32 pdatum, udatum, parent;
	float p[3];
	if (!o_bump || !ptr_ok(time) || !ptr_ok(lp) || !ptr_ok(pool)) return;
	tick = M32(time + 8);
	if (tick == g_bp_tick) return;                                        /* once a tick */
	g_bp_tick = tick;
	if (tick < g_bp_rest) return;
	pdatum = (s32)M32(lp + 0xC);
	player = pool_entry(ADDR_PLAYERS_PTR, pdatum, 0x40);
	if (!player) return;
	udatum = (s32)M32(player + 0x2C);
	me = object_from_datum(udatum);
	if (!me) return;
	parent = (s32)M32(me + 0x14);
	p[0] = MF(me + 0x64); p[1] = MF(me + 0x68); p[2] = MF(me + 0x6C);
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || count > 8192) return;
	for (k = 0; k < count; k++)
	{
		u32 h = data + k * 12, id = M32(h) & 0xffff, obj, type;
		s32 datum;
		float dx, dy, dz, r;
		if (!id) continue;
		type = M8(h + 3);
		if (type > 1) continue;                                           /* a biped (0) or a vehicle (1) */
		datum = (s32)((id << 16) | k);
		if (datum == udatum || datum == parent) continue;
		obj = M32(h + 8);
		if (!ptr_ok(obj) || (s32)M32(obj + 0x13C) != -1) continue;        /* a player's */
		if (M16(obj + 0x10A) & 4) continue;                               /* dead */
		if (type == 0)
		{
			dx = MF(obj + 0x64) - p[0]; dy = MF(obj + 0x68) - p[1]; dz = MF(obj + 0x6C) - p[2];
			if (dx * dx + dy * dy > 0.7f * 0.7f || dz > 1.2f || dz < -1.2f) continue;
		}
		else
		{
			r = MF(obj + 0x3C);
			if (!(r > 0.f && r < 50.f)) continue;
			dx = MF(obj + 0x30) - p[0]; dy = MF(obj + 0x34) - p[1]; dz = MF(obj + 0x38) - p[2];
			if (dx * dx + dy * dy > 0.5625f * r * r || dz > r || dz < -r) continue;
		}
		if ((u32)datum == g_bp_left)
		{
			dx = MF(obj + 0x64) - p[0]; dy = MF(obj + 0x68) - p[1]; dz = MF(obj + 0x6C) - p[2];
			if (dx * dx + dy * dy + dz * dz < 9.f) continue;                  /* the body you just left: not yet */
			g_bp_left = 0xFFFFFFFFu;
		}
		cand = (u32)datum;
		break;
	}
	if (cand == 0xFFFFFFFFu) { g_bp_cand = cand; g_bp_count = 0; return; }
	if (cand != g_bp_cand) { g_bp_cand = cand; g_bp_count = 1; return; }
	if (++g_bp_count < 3) return;
	g_bp_player = (u32)pdatum; g_bp_unit = cand;
	g_bp_count = 0; g_bp_cand = 0xFFFFFFFFu;
	{
		/* the body to revert to: the one you're leaving, unless one is already kept and still alive */
		if (!g_bp_inposs) g_bp_home = unit_ok((u32)udatum) ? (u32)udatum : 0xFFFFFFFFu;   /* 6h.14: your own body, as you leave it */
	}
	{
		s32 at;
		if (possess_call()) { lput(0, "bump possession: a bad read was caught (guarded)"); log_write(); return; }
		g_bp_inposs = 1;
		at = lput(0, "bump possession: player "); at = lhex(at, g_bp_player); at = lput(at, " now in unit "); lhex(at, g_bp_unit);
		log_write();
		toast("Bump Possession: you took control");
	}
	g_bp_left = (u32)udatum;
	{ s32 tps = (s32)(s16)M16(time + 2); g_bp_rest = tick + (u32)(tps > 0 && tps <= 3000 ? tps : 30); }
}
#ifdef HOST
void host_set_bump(s32 on) { o_bump = on; }
void host_bump_tick(void) { bump_tick(); }
#endif

/* ---- Checkpoint Settings (6h.12). The game asks for its saves and reverts by flags in main_globals, which its main loop
 * (v1.0 0x12b450) acts on: save_map (ADDR_SAVE_MAP: v1.0 0x547f70) and revert_map (the byte before it). Create / Revert
 * Checkpoint set them directly. "Disable natural checkpoints": the game's checkpoint logic (v1.0 0x226050 - the scripts'
 * game_save requests, waited out until it's safe) asks for its save through 0x12b790 - a call (0x22613d) and a jump
 * (0x226090) - and those now skip it; its request is dropped as if done. "Disable reverts": after you die, 0x138e40 (called
 * once a tick, 0x138082) counts down and sets revert_map; a revert flag it raises is lowered again right after - so you
 * stay down (Revert Checkpoint still reverts; Cycle Possession takes you into something else). ---- */
s32 g_cp_skip;                                   /* hsave_hook: this save skipped */
static s32 g_cp_said, g_rv_before, g_rv_said;
void natural_save_check(void)
{
	g_cp_skip = o_no_natural_cp != 0;
	if (g_cp_skip) { toast("Natural checkpoint skipped"); if (!g_cp_said) { g_cp_said = 1; lput(0, "checkpoints: a natural checkpoint skipped (Disable natural checkpoints)"); log_write(); } }
}
void death_revert_before(void) { g_rv_before = M8(ADDR_SAVE_MAP - 1) != 0; }
void death_revert_after(void)
{
	if (o_no_reverts && !g_rv_before && M8(ADDR_SAVE_MAP - 1))
	{
		gw8(ADDR_SAVE_MAP - 1, 0);
		if (!g_rv_said) { g_rv_said = 1; toast("Revert on death stopped (Disable reverts)"); lput(0, "checkpoints: the revert on death stopped (Disable reverts)"); log_write(); }
	}
	else if (!M8(ADDR_SAVE_MAP - 1)) g_rv_said = 0;
}
#ifdef HOST
void host_set_cp(s32 natural, s32 reverts) { o_no_natural_cp = natural; o_no_reverts = reverts; }
#endif

/* ---- "Revert to player": back into the body you were in before your first possession (player_set_unit again), if it's
 * still there and alive; the unit you leave is then left alone (the rest, and not until you're 3 units from it). ---- */
static void revert_to_player(void)
{
	u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR), time = M32(ADDR_GAME_TIME_PTR), player, home;
	s32 pdatum;
	if (!ptr_ok(lp)) { toast("Revert to player possession: no game in progress"); return; }
	pdatum = (s32)M32(lp + 0xC);
	player = pool_entry(ADDR_PLAYERS_PTR, pdatum, 0x40);
	home = object_from_datum((s32)g_bp_home);
	if (!player) { toast("Revert to player possession: no player"); return; }
	if (g_bp_home == 0xFFFFFFFFu || M32(player + 0x2C) == g_bp_home) { toast("Revert to player possession: you're in your own body"); g_bp_inposs = 0; return; }
	if (!home || !unit_ok(g_bp_home)) { toast("Revert to player possession: your own body is gone"); g_bp_home = 0xFFFFFFFFu; g_bp_inposs = 0; return; }
	g_bp_left = M32(player + 0x2C);
	g_bp_player = (u32)pdatum; g_bp_unit = g_bp_home;
	if (possess_call()) { lput(0, "revert to player: a bad read was caught (guarded)"); log_write(); return; }
	g_bp_home = 0xFFFFFFFFu; g_bp_inposs = 0;
	if (ptr_ok(time)) { s32 tps = (s32)(s16)M16(time + 2); g_bp_rest = M32(time + 8) + (u32)(tps > 0 && tps <= 3000 ? tps : 30); }
	toast("Back in your own body");
}

/* ---- "Cycle Possession" (6h.4): into the next unit in the level you could possess - the bipeds and vehicles no player
 * controls, alive - in the object table's order after the last one it took (wrapping round), as Bump Possession takes
 * them (player_set_unit); your own body is kept for "Revert to player possession". ---- */
static u32 g_cyc_k = 0xFFFFFFFFu;               /* the table index it took last */
static void cycle_possession(void)
{
	u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR), pool = M32(ADDR_OBJECTS_PTR), time = M32(ADDR_GAME_TIME_PTR), player, me, data, count, n, k, found = 0xFFFFFFFFu, total = 0;
	s32 pdatum, udatum, parent, nth = 0;
	char msg[80];
	if (!ptr_ok(lp) || !ptr_ok(pool)) { toast("Cycle Possession: no game in progress"); return; }
	pdatum = (s32)M32(lp + 0xC);
	player = pool_entry(ADDR_PLAYERS_PTR, pdatum, 0x40);
	if (!player) { toast("Cycle Possession: no player"); return; }
	udatum = (s32)M32(player + 0x2C);
	me = object_from_datum(udatum);
	parent = me ? (s32)M32(me + 0x14) : -1;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || !count || count > 8192) { toast("Cycle Possession: no objects"); return; }
	for (n = 1; n <= count; n++)
	{
		u32 h, id, obj, type;
		s32 datum;
		k = (g_cyc_k == 0xFFFFFFFFu ? (me ? ((u32)udatum & 0xffff) : count - 1) : g_cyc_k) + n;
		k %= count;
		h = data + k * 12; id = M32(h) & 0xffff;
		if (!id) continue;
		type = M8(h + 3);
		if (type > 1) continue;                                           /* a biped (0) or a vehicle (1) */
		datum = (s32)((id << 16) | k);
		if (datum == udatum || datum == parent) continue;
		obj = M32(h + 8);
		if (!ptr_ok(obj) || (s32)M32(obj + 0x13C) != -1) continue;        /* a player's */
		if (M16(obj + 0x10A) & 4) continue;                               /* dead */
		if (found == 0xFFFFFFFFu) { found = (u32)datum; g_cyc_k = k; }
		total++;
	}
	if (found == 0xFFFFFFFFu) { toast("Cycle Possession: nothing to possess in this level"); return; }
	{
		/* which one of how many (in the table's order) */
		u32 kk;
		for (kk = 0; kk < count; kk++)
		{
			u32 h = data + kk * 12, id = M32(h) & 0xffff, obj, type;
			s32 datum;
			if (!id || (type = M8(h + 3)) > 1) continue;
			datum = (s32)((id << 16) | kk);
			if (datum == udatum || datum == parent) continue;
			obj = M32(h + 8);
			if (!ptr_ok(obj) || (s32)M32(obj + 0x13C) != -1 || (M16(obj + 0x10A) & 4)) continue;
			nth++;
			if ((u32)datum == found) break;
		}
	}
	{
		if (!g_bp_inposs) g_bp_home = unit_ok((u32)udatum) ? (u32)udatum : 0xFFFFFFFFu;   /* the body to revert to: yours (6h.14) */
	}
	g_bp_player = (u32)pdatum; g_bp_unit = found;
	if (possess_call()) { lput(0, "cycle possession: a bad read was caught (guarded)"); log_write(); return; }
	g_bp_inposs = 1;
	g_bp_left = (u32)udatum;
	if (ptr_ok(time)) { s32 tps = (s32)(s16)M16(time + 2); g_bp_rest = M32(time + 8) + (u32)(tps > 0 && tps <= 3000 ? tps : 30); }
	{
		s32 at = 0, m;
		const char *a = "Cycle Possession: ", *b = M8(data + g_cyc_k * 12 + 3) ? "vehicle " : "biped ", *c = " of ";
		char dg[12];
		u32 v;
		while (*a && at < 70) msg[at++] = *a++;
		while (*b && at < 70) msg[at++] = *b++;
		v = (u32)nth; m = 0; do { dg[m++] = (char)('0' + v % 10); v /= 10; } while (v && m < 11); while (m && at < 76) msg[at++] = dg[--m];
		while (*c && at < 76) msg[at++] = *c++;
		v = total; m = 0; do { dg[m++] = (char)('0' + v % 10); v /= 10; } while (v && m < 11); while (m && at < 79) msg[at++] = dg[--m];
		msg[at] = 0;
		toast(msg);
		at = lput(0, "cycle possession: player "); at = lhex(at, g_bp_player); at = lput(at, " now in unit "); lhex(at, g_bp_unit);
		log_write();
	}
}
/* ---- "Passion Check": closes the menu and kills you - whatever unit you're in - as a kill volume does (v1.0 0xdc0a0, v1.5
 * 0xde5f0, __stdcall(object): the game's death damage (damage globals +0x14) through a full damage report, as a lethal hit
 * - the body falls, the usual respawn wait), even with Invulnerability on: its cannot-take-damage / cannot-die flags are
 * cleared first. (6f.1 called 0xd6800 alone - the body marked depleted, the unit never told: alive, unhealable, untouchable;
 * 6f.2 called 0xd6bc0, the object kill - the unit removed at once, an instant respawn, as off the map's edge.) ---- */
static u32 g_kill_unit;
static u32 g_pc_rolls;
static u32 tsc_lo(void);
#ifdef HOST
static u32 g_host_kill_unit; static s32 g_host_kill_n;
static void kill_call(void) { g_host_kill_unit = g_kill_unit; g_host_kill_n++; }
s32 host_kill_calls(u32 *unit) { *unit = g_host_kill_unit; return g_host_kill_n; }
#else
static void kill_call(void) { ((void (__attribute__((stdcall)) *)(u32))ADDR_KILL)(g_kill_unit); }
#endif
static void passion_check(void)
{
	u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR), player, unit;
	s32 pdatum;
	if (!ptr_ok(lp)) { toast("Passion Check: no game in progress"); return; }
	pdatum = (s32)M32(lp + 0xC);
	player = pool_entry(ADDR_PLAYERS_PTR, pdatum, 0x40);
	if (!player || !(unit = object_from_datum((s32)M32(player + 0x2C)))) { toast("Passion Check: no player unit"); return; }
	{
		/* 6h.12: a roll - 1 in 7 passes (and you live); the processor's cycle count, well mixed */
		u32 r = tsc_lo() ^ (g_pc_rolls++ * 0x9E3779B9u);
		r ^= r >> 16; r *= 0x7FEB352Du; r ^= r >> 15; r *= 0x846CA68Bu; r ^= r >> 16;
		if (r % 7u == 0u) { toast("Passion Check passed"); lput(0, "passion check: passed (1 in 7)"); log_write(); return; }
	}
	gw16(unit + 0x10A, (u16)(M16(unit + 0x10A) & ~IV_BITS));            /* invulnerable or not */
	g_kill_unit = M32(player + 0x2C);
	if (guarded_call(kill_call)) { lput(0, "passion check: a bad read was caught (guarded)"); log_write(); return; }
	{ s32 at = lput(0, "passion check: unit "); lhex(at, g_kill_unit); log_write(); }
	toast("Passion Check failed");                                        /* (it killed you) */
}
#ifdef HOST
void host_revert_to_player(void) { revert_to_player(); }
void host_clear_toast(void) { g_toast_frames = 0; }
void host_cycle_possession(void) { cycle_possession(); }
void host_passion_check(void) { passion_check(); }
u32 host_bp_home(void) { return g_bp_home; }
#endif

/* ---- HUD Scaling Fix (Halo 2 HD's DisableAnamorphicScaling, that part alone): the game's anamorphic factor is the float 4/3
 * at v1.0 0x45DFDC / v1.5 0x46A514, read by the view and HUD setup at four places (v1.0 0x2fc88, 0x171257, 0x172729,
 * 0x264070). Those four reads are pointed, in the XBE, at spooky_anamorphic below - the mod's own data - which holds the
 * game's value until this toggle sets 1.0. (6g wrote the game's constant itself, in read-only memory, Halo 2 HD's way -
 * interrupts off, the write-protect bit cleared - and toggling it in play crashed.) Nothing of the game's is written. ---- */
float spooky_anamorphic = 1.3333334f;            /* 4/3, as the game's own (0x3FAAAAAB) */
static void apply_hud_fix(void)
{
	float game = MF(ADDR_ANAMORPHIC);                                     /* the game's own, untouched */
	if (!(game > 0.5f && game < 2.f)) game = 1.3333334f;
	spooky_anamorphic = o_hud_fix ? 1.f : game;
}
#ifdef HOST
void host_set_hud_fix(s32 on) { o_hud_fix = on; }
void host_apply_hud_fix(void) { apply_hud_fix(); }
#endif

void spooky_main_frame(void)
{
	g_count_main++;
	game_speed_frame();                                                   /* 6h.22: Game Speed / Pause Game, before the game's time update */
	trace_frame();
	fc_frame();                                                           /* the free camera: flown once a frame */
	trace("main start");
	if (g_req) { s32 r = g_req; g_req = 0; practice(r); }               /* a practice action, before the game's update */
	apply_ai();
	apply_invuln();
	apply_grenades();
	apply_skulls();                                                       /* Cheats > Skulls */
	apply_inv_veh();                                                      /* Invincible Vehicles */
	acro_tick();                                                          /* Acrophobia */
	bump_tick();                                                          /* Bump Possession */
	trace("superbounce");
	superbounce_update();
	trace("ghost");
	if (guarded_call(ghost_update)) ghost_faulted();                    /* after: it reads this tick; a bad read caught */
	offsets_frame_end();
	trace("main end");
	if (g_count_main == 30)            /* about a second in: the game has mapped T: by now */
	{
		s32 at = lput(0, "Spooky Ghost step 6h.29 running in Halo 2 " H2_VERSION_TEXT " (modified default.xbe); per-frame hook OK; log on ");
		lput(at, g_log_drive == 1 ? "U: (E:\\UDATA\\4D530064)" : g_log_drive == 2 ? "Z: (cache drive)" : "T: (E:\\TDATA\\4D530064)");
		log_write();
	}
	if (g_count_main % 300 == 0 && g_heartbeats < 24)     /* about every 10 s at 30 fps, for about 4 minutes */
	{
		g_heartbeats++;
		s32 at = lput(0, "status: main-loop frames "); at = lnum(at, g_count_main);
		at = lput(at, ", controller reads "); at = lnum(at, g_count_input);
		at = lput(at, ", HUD-text frames "); at = lnum(at, g_count_hud);
		at = lput(at, ", route A frame ends "); at = lnum(at, g_count_end_a);
		at = lput(at, ", route B frame ends "); at = lnum(at, g_count_end_b);
		at = lput(at, ", player "); at = g_sb_object ? lhex(at, g_sb_object) : lput(at, "none");
		at = lput(at, ", charged "); at = lnum(at, (u32)(g_sb_charged + 1)); at = lput(at, " (0 none, 1 no, 2 yes)");
		at = lput(at, ", speed x10 "); at = lnum(at, g_sb_speed > 0 ? (u32)(g_sb_speed * 10.f) : 0);
		at = lput(at, ", bounces "); at = lnum(at, (u32)g_sb_streak); at = lput(at, ", record "); at = lnum(at, (u32)g_sb_record);
		at = lput(at, ", menu "); lput(at, g_open ? "open" : "closed");
		log_write();
		{
			s64 now = tsc64();
			if (g_fps_t0)
			{
				float secs = (float)(now - g_fps_t0) / 733333333.f;
				at = lput(0, "  performance: game frames per second x10 ");
				at = lnum(at, secs > 0.f ? (u32)((float)(g_count_hud - g_fps_f0) * 10.f / secs) : 0);
				at = lput(at, ", seam work per frame (us) avg ");
				at = lnum(at, g_seam_cyc_n ? (u32)((float)g_seam_cyc_sum / (float)g_seam_cyc_n / 733.333f) : 0);
				at = lput(at, " max "); at = lnum(at, (u32)((float)g_seam_cyc_max / 733.333f));
				at = lput(at, ", searches "); at = lnum(at, g_seam_searches); at = lput(at, " in "); at = lnum(at, g_seam_cyc_n);
				if (g_seam_cyc_n)
				{
					/* where the time goes (us per frame on average; the blockers are part of the search) */
					at = lput(at, " - surfaces near you "); at = lnum(at, (u32)g_bk_count); at = lput(at, g_bk_full ? " (FULL)" : "");
					at = lput(at, " - search "); at = lnum(at, (u32)((float)g_cyc_search / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, " (blockers "); at = lnum(at, (u32)((float)g_cyc_block / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, ": level "); at = lnum(at, (u32)((float)g_cyc_level / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, ", pieces "); at = lnum(at, (u32)((float)g_cyc_pieces / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, ", objects "); at = lnum(at, (u32)((float)g_cyc_objects / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, ", landing "); at = lnum(at, (u32)((float)g_cyc_land / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, "), rating "); at = lnum(at, (u32)((float)g_cyc_rate / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, ", drawing "); lnum(at, (u32)((float)g_cyc_draw / (float)g_seam_cyc_n / 733.333f));
				}
				log_write();
			}
			g_fps_t0 = now; g_fps_f0 = g_count_hud;
			{
				/* the search rate from its cost per search (not per frame - that drops when it searches less): see
				 * search_div_for */
				u32 per = g_seam_searches ? (u32)((float)g_seam_cyc_sum / (float)g_seam_searches / 733.333f) : 0;
				s32 div = search_div_for(per, g_search_div);
				/* once a search has been slow, the lighter limits stay (no flip back to the full load: 5e.16 went 4 -> 2 -> 4,
				 * each 2 a heavy window) */
				if (div > 2) g_slow_hw = 1;
				if (g_slow_hw && (g_blk_cap != 192 || g_seam_cap != 64)) { g_blk_cap = 192; g_seam_cap = 64; }
				if (g_seam_searches && div != g_search_div)
				{
					s32 at = lput(0, "superbounce: a search every "); at = lnum(at, (u32)div); at = lput(at, " frames (it costs ~");
					at = lnum(at, per / 1000); lput(at, " ms a search)");
					log_write();
					g_search_div = div;
					g_blk_cap = g_slow_hw ? 192 : BLK_MAX;
					g_seam_cap = g_slow_hw ? 64 : SEAM_MAX;
				}
			}
			g_seam_cyc_sum = g_seam_cyc_max = g_seam_cyc_n = g_seam_searches = 0;
			g_cyc_search = g_cyc_block = g_cyc_rate = g_cyc_draw = g_cyc_level = g_cyc_pieces = g_cyc_objects = g_cyc_land = 0;
		}
		if (o_diag && g_sb_object && g_gfx_logged)                         /* 6h.21: development lines */
		{
			float sx, sy, depth;
			s32 i;
			project(g_marker, &sx, &sy, &depth);
			at = lput(0, "camera x100: position");
			for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(g_cam_p[i] * 100.f)); }
			at = lput(at, ", forward");
			for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(g_cam_f[i] * 100.f)); }
			at = lput(at, ", up");
			for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(g_cam_u[i] * 100.f)); }
			at = lput(at, ", fov x1000 "); at = lsnum(at, (s32)(g_cam_fov * 1000.f));
			at = lput(at, ", widescreen "); at = lnum(at, g_ws);
			at = lput(at, " (setting byte "); at = lnum(at, (M32(ADDR_WIDESCREEN_FLAG & ~3u) >> ((ADDR_WIDESCREEN_FLAG & 3) * 8)) & 0xff);
			at = lput(at, ", flag "); at = lnum(at, g_ws_flag5);
			at = lput(at, "), tan h/v x1000 "); at = lsnum(at, (s32)(g_tan_h * 1000.f)); at = lput(at, "/"); at = lsnum(at, (s32)(g_tan_v * 1000.f));
			at = lput(at, ", world frame projection "); at = lput(at, g_use_frame ? "USED" : "not used");
			at = lput(at, " (camera off by x1000 "); at = lsnum(at, (s32)(g_fr_dist * 1000.f));
			at = lput(at, ", bounds L/R/B/T x1000 "); at = lsnum(at, (s32)(g_fr_l * 1000.f)); at = lput(at, "/"); at = lsnum(at, (s32)(g_fr_rt * 1000.f));
			at = lput(at, "/"); at = lsnum(at, (s32)(g_fr_bt * 1000.f)); at = lput(at, "/"); at = lsnum(at, (s32)(g_fr_tp * 1000.f)); at = lput(at, ")");
			log_write();
			at = lput(0, "  seams "); at = lnum(at, (u32)g_seam_n);
			at = lput(at, " (valley "); at = lnum(at, (u32)g_seam_kinds[SK_VALLEY]); at = lput(at, ", ridge "); at = lnum(at, (u32)g_seam_kinds[SK_RIDGE]);
			at = lput(at, ", flat "); at = lnum(at, (u32)g_seam_kinds[SK_FLAT]); at = lput(at, ")");
			at = lput(at, ", from pieces "); at = lnum(at, (u32)g_piece_seams); at = lput(at, " (pieces near "); at = lnum(at, (u32)g_pieces_near);
			at = lput(at, ", bad definitions "); at = lnum(at, g_def_bad); at = lput(at, ")");
			at = lput(at, ", from objects "); at = lnum(at, (u32)g_object_seams); at = lput(at, " (objects near "); at = lnum(at, (u32)g_objects_near);
			at = lput(at, ", bad "); at = lnum(at, g_object_bad); at = lput(at, ")");
			at = lput(at, ", feet");
			for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(MF(g_sb_object + 0x64 + 4 * i) * 100.f)); }
			at = lput(at, ", marker");
			for (i = 0; i < 3; i++) { at = lput(at, " "); at = lsnum(at, (s32)(g_marker[i] * 100.f)); }
			at = lput(at, g_marker_set ? " (placed)" : " (not placed yet)");
			at = lput(at, ", on screen "); at = lsnum(at, (s32)sx); at = lput(at, ","); at = lsnum(at, (s32)sy);
			at = lput(at, " depth "); lsnum(at, (s32)(depth * 100.f));
			log_write();
		}
	}
}

#ifdef HOST
/* the test harness reads these */
s32 host_menu_open(void) { return g_open; }
s32 host_menu_selected(void) { return g_selected; }
s32 host_value(s32 i) { const s_item *it = &g_menus[g_menu].items[i]; return it->value ? *it->value : 0; }
s32 host_menu(void) { return g_menu; }
void host_set_menu_open(s32 open, s32 m) { g_open = open; g_menu = m; }
s32 host_sel_of(s32 m) { return g_sel[m]; }
s32 host_first_of(s32 m) { return g_first[m]; }
s32 host_menu_count(void) { return NOF(g_menus); }
void host_set_rows_cap(s32 n) { g_rows_cap = n; }
s32 host_suppressing(void) { return g_suppress; }
#endif
