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
#define ADDR_CP_SLOT 0x1F00
#define ADDR_CP_FLAG2 0x1F02
#define ADDR_CP_VALID 0x1F04
#define ADDR_HAVOK_REBUILD 0x1F10
#define ADDR_UI_WINDOW_IN_USE 0x1F14
#define ADDR_SCENARIO 0x1F20
#define ADDR_KE_TICK_SLOT 0x1F24
#define ADDR_CACHE_HEADER 0xE0000              /* 6i.158: the map file's header, as the game keeps it (0x800 bytes) */
#define ADDR_CACHE_TAGS_PTR 0x1F28
#define ADDR_GAME_OPTIONS_PTR 0x1D10
#define ADDR_OOB_TEST 0x1D14
#define ADDR_SKULLS 0x1D80
#define ADDR_SET_UNIT 0x1D90
#define ADDR_KILL 0x1DA0
#define ADDR_ANAMORPHIC 0x1DB0
#define ADDR_VBLANK_COUNT 0x1DC0
#define ADDR_REFRESH_HZ 0x1DC4
#define ADDR_DISPLAY_CONFIG 0x1DC8
#define ADDR_VIS_POOL 0x1E00
#define ADDR_BSP_RENDER_PTR 0x1DE0
#define ADDR_LIGHTS_PTR 0x1DE4
#define ADDR_RENDER_CAMERA 0x1DEC
#define ADDR_FP_USERS_PTR 0x1E90
#define ADDR_FX_MODE 0x1EA0
#define ADDR_TSS 0x7F000
#define ADDR_D3D_DEVICE 0x1EB0
#define ADDR_COOP_LOD 0x1EC0
#define ADDR_COOP_FLAGS 0x1EC4
#define ADDR_COOP_DIST 0x1ECC
#define ADDR_COOP_PRIO 0x1ED0
#define ADDR_RT_PRIMARY 0x1EE0
#define ADDR_D3D_ADDREF 0x1EF0
#define ADDR_D3D_RELEASE 0x1EF4
#define ADDR_VIEW_FWD 0x1E50
#define ADDR_PROJ_SCALE 0x1E80
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
#define ADDR_ACTORS_PTR 0x1DF8
#define ADDR_SQUADS_PTR 0x1F40
#define ADDR_RESET_MAP 1
#define ADDR_UNIT_REQUEST 1
#define ADDR_PHYS_GLOBALS 0x1F50
#define ADDR_TEX_PHYS 0x1F88
#define ADDR_GEO_PHYS 0x1F8C
#define ADDR_SND_PHYS 0x1F90
#define ADDR_SQUAD_ORDER 1
#define ADDR_SQUAD_FOLLOW 1
#define ADDR_AI_PEACE 0x1DFC
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
#define ADDR_RS_ALPHAFUNC 0x7E800                 /* 6i.73: the host stand-ins for Direct3D's shadows */
#define ADDR_PROJ_STORE 0x7E900                  /* 6i.73: the host stand-in for the camera's projection */
#define ADDR_CAM_RECT 0x7EA30                    /* 6i.77: the host stand-in for the camera storage's rectangle (camera at -0x30) */
#define ADDR_SHADOW_CAM 0x7EC00                  /* 6i.81: host stand-ins for g_47fd18 / g_47fda0 */
#define ADDR_SHADOW_SIZE 0x7ED00
#define ADDR_BOUND_TEX 0x7EE00                   /* 6i.82: host stand-in for the bound-texture table */
#define ADDR_TEX_STATE 0x7EF00                   /* 6i.83: host stand-ins for Direct3D's texture states and dirty flags */
#define ADDR_TEX_DIRTY 0x7F200
#define ADDR_PUSH_SEG 0x7F300                    /* 6i.90: host stand-in for Direct3D's push segment size */
#define ADDR_TAG_TABLE 0x7F304                   /* 6i.93: host stand-in for the tag table pointer */
#define ADDR_ANIM_LOADS 0x7F320                  /* 6i.128: host stand-ins - the animation cache's loads, memory, blocks */
#define ADDR_ANIM_MEM 0x7F324
#define ADDR_ANIM_PHYS 0x7F328
#define ADDR_T3_SURF 0x7F330                     /* 6i.139: host stand-ins - target 3's surface and texture pointers */
#define ADDR_D3D_DIRTY 0x7F338                   /* 6i.144: host stand-in for Direct3D's dirty-state flags */
#define ADDR_WATER_XFORM 0x7F340                 /* 6i.147: host stand-in for the water mesh's transform (3 x 4 floats) */
#define ADDR_T3_TEX 0x7F334
#define ADDR_BB_SURF 0x7F350                     /* 6i.148: host stand-ins - the back buffers' surfaces (2), their textures (2), target 24's texture, the render-target table */
#define ADDR_BB_TEX 0x7F358
#define ADDR_T24_TEX 0x7F360
#define ADDR_MAIN_DEPTH 0x7F364
#define ADDR_MAIN_TARGET 0x7F368
#define ADDR_RT_CACHE 0x7F36C
#define ADDR_RT_TABLE 0x7F400
#define ADDR_RS_ALPHATEST 0x7E808
#define ADDR_RS_ALPHAREF 0x7E80C
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

/* ---- a log on the hard drive: T:\spooky_log.txt = E:\TDATA\4D530064\spooky_log.txt ----
 * 6i.36: SPOOKY_LOG 0 - all logging off for now: lput / lnum / lms / lsnum / lhex / log_write are empty, so every log
 * line, its text and the live-log page are compiled out of the XBE. Every log line is still in the source - the
 * superbounce logging (the landing records the predictor work used) with the rest: SPOOKY_LOG 1 brings it all back. The
 * host tests build with it on (they read the log). */
/* 6i.138: your note - no logging or probing in the console build: SPOOKY_LOG 0 there (every log line, the live page and the
 * White + D-pad Up capture compiled out); the host tests keep it on (they read the log) */
/* 6i.180: your note of 10 Nov - the log was not to go until you call a build final; 6i.138 took it out too soon. It is on
 * again in the console build: every log line and the live page (h2test's "live"), written to T:\spooky_log.txt as
 * before (on the 360: HDD:\Compatibility\Xbox1\TDATA\4d530064\spooky_log.txt). About 54 KB more in memory. The one
 * thing NOT brought back with it is the White + D-pad Up capture shortcut (SPOOKY_CAPTURE_KEY 0): a button combination
 * that starts something is not "logging", and nobody has asked for it back. */
/* 6i.224: your word of 12 Nov - "I don't suspect there to be any other need for logging past this": the mod's logging
 * is out of the console build. With SPOOKY_LOG 0 every log line, its text, the file T:\spooky_log.txt and the live page
 * the PC tools read are not compiled into the XBE at all (lput / lnum / lsnum / lhex / log_write are empty). The host
 * tests keep it on - they check the mod by reading what it would have logged - so the lines stay in the source; one
 * word here brings them back if a fault ever needs them. */
#ifdef HOST
#define SPOOKY_LOG 1
#else
#define SPOOKY_LOG 0
#endif
#define SPOOKY_CAPTURE_KEY 0
/* 6i.138 took the memory check for Render Scale above 100 % out - wrongly (you asked for the logging to go, not the feature);
 * 6i.139: back, its findings shown on screen instead of in the log (Performance Mode Settings, under the title, while Render
 * Scale is above 100 %) */
#define SPOOKY_PROBE 1
/* 6i.36: the build's name kept in the XBE (the log's "running" line, which carried it, is compiled out with the log) */
/* v2.0 (12 Nov 2026): the build is named "Spooky Ghost Menu v2.0" here - in the XBE - and in the patcher; nothing in the
 * game shows it (your instruction). The step numbers (6i.NNN) end with 6i.225. */
__attribute__((used)) static const char g_build_name[] = "Spooky Ghost Menu v2.0";
static s32 o_log __attribute__((unused)) = 1;   /* write T:\\spooky_log.txt (launcher option "Disable log" turns it off); 6i.138: unused with SPOOKY_LOG 0 */
u32 g_count_end_a, g_count_end_b;          /* incremented by the end-of-frame stubs in hooks.S */
static u32 g_count_input, g_heartbeats;
static s32 g_logged_input, g_logged_hud;
static s32 g_log_started __attribute__((unused));   /* (6i.138: unused with SPOOKY_LOG 0) */
static s32 g_log_drive = -1;            /* which of T:, U:, Z: took the log */
static char g_log[560];
/* ---- 6i.213: FLOAT TO WHOLE NUMBER, ALWAYS OUT OF LINE WHERE ANOTHER VALUE IS BEING HELD ----
 * Three times the 360 has got a float-to-integer conversion wrong where the Xbox's own processor cannot: the level grid
 * (5e.12), the landing recorder (6h.36: y came out as 100 x its x) and the legs (6i.19x). Each time the compiler had
 * left other values on the floating-point stack around the conversion, and each time the cure was the same: convert
 * out of line, where the stack holds nothing else. The rest of the mod had never been checked for the pattern.
 * The built mod was searched for it two ways (the next floating-point instruction after a conversion uses a value it
 * did not load; and a count of the values held at each conversion): 198 conversions, 39 of them made with a value
 * still held, in 17 routines - the 3D line and the colour set-up, the light clipping's rectangles, the "is it covered"
 * grid the seams and the third person camera's collision use, the seams' pieces, the Monster/Dreadnought labels, the
 * Havok Island bar, the third person camera's field of view, three log lines. Those now go through fti(). None of
 * them is known to have gone wrong; the point is that they can no longer. Both searches now find none. ---- */
static __attribute__((noinline)) s32 fti(float v) { return (s32)v; }
#if SPOOKY_LOG
static s32 lput(s32 at, const char *s) { while (*s && at < 558) g_log[at++] = *s++; g_log[at] = 0; return at; }
static s32 lnum(s32 at, u32 v);
static s32 lput(s32 at, const char *t);
static s32 lhex(s32 at, u32 v);
static void log_write(void);
static s32 lnum(s32 at, u32 v)
{
	char b[12]; s32 n = 0;
	do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v);
	while (n && at < 558) g_log[at++] = b[--n];
	g_log[at] = 0; return at;
}
static s32 lms(s32 at, u32 us) { at = lnum(at, us / 1000u); at = lput(at, "."); return lnum(at, us / 100u % 10u); }   /* 6i.3: us as ms.t */
static s32 lsnum(s32 at, s32 v)
{
	if (v < 0) { at = lput(at, "-"); v = -v; }
	return lnum(at, (u32)v);
}

static s32 lhex(s32 at, u32 v)
{
	s32 i; at = lput(at, "0x");
	for (i = 28; i >= 0; i -= 4) if (at < 558) g_log[at++] = "0123456789ABCDEF"[(v >> i) & 15];
	g_log[at] = 0; return at;
}
#else
static inline s32 lput(s32 at, const char *s) { (void)s; return at; }
static inline s32 lnum(s32 at, u32 v) { (void)v; return at; }
static inline s32 lms(s32 at, u32 us) { (void)us; return at; }
static inline s32 lsnum(s32 at, s32 v) { (void)v; return at; }
static inline s32 lhex(s32 at, u32 v) { (void)v; return at; }
#endif
#if SPOOKY_LOG
/* 6h.39: THE LIVE LOG - every log line also goes into one page of memory (4 KB, page-aligned, so it's one piece wherever
 * the console maps it) behind a header a PC tool finds and reads while you play: on the 360, through XBDM (the debug
 * monitor Xbox 360 Neighborhood talks to) - tools/spooky_live.py. The file log is unchanged. Header: "SPKYLIVE", then
 * (little-endian) the count of bytes ever written, the ring's size, this page's own Xbox address; the text follows,
 * written round the ring with a line feed after each line. */
#define LIVE_DATA (4096 - 32)
typedef struct { char magic[8]; u32 seq, size, self, version, lines, pad; char data[LIVE_DATA]; } s_live;
#ifdef HOST
static s_live g_live_host __attribute__((aligned(4096)));
static s_live *g_live = &g_live_host;
#else
static s_live *g_live;                           /* one page of run-time memory (a whole page: one piece of the console's RAM) */
#endif
static void live_put(const char *t)
{
	u32 q;
#ifndef HOST
	if (!g_live)
	{
		static s32 tried;
		if (tried) return;
		tried = 1;
		g_live = (s_live *)VirtualAlloc_x(0, 4096, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 4 /* PAGE_READWRITE */);
		if (!g_live) return;
	}
#endif
	q = g_live->seq;
	if (g_live->magic[0] != 'S')
	{
		static const char m[8] = { 'S', 'P', 'K', 'Y', 'L', 'I', 'V', 'E' };
		s32 k;
		for (k = 0; k < 8; k++) g_live->magic[k] = m[k];
		g_live->size = LIVE_DATA; g_live->self = (u32)(unsigned long)g_live; g_live->version = 1;
	}
	while (*t) { g_live->data[q % LIVE_DATA] = *t++; q++; }
	g_live->data[q % LIVE_DATA] = '\n'; q++;
	g_live->lines++;
	g_live->seq = q;                                                         /* last: a reader sees whole lines */
}
#ifdef HOST
void host_log(const char *line);
static s32 g_log_hold;
static void log_write(void) { live_put(g_log); host_log(g_log); }
const void *host_live_page(void) { return g_live; }
#else
static s32 g_log_hold;                  /* 6i.7: the 10-second lines' flush held to the last of them (one, not one a line) */
static void log_write(void)
{
	u32 n = 0, written;
	live_put(g_log);
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
	/* 6h.41: each line pushed to the disk before the file is closed - the kernel's NtFlushBuffersFile, through the game's
	 * own import of it (Halo 2 imports it: thunk slot ADDR_KT_FLUSH). On the 360 the emulator wrote the Xbox's T: drive out
	 * in batches, so tools\spooky_live.py saw the log in bursts. Called only if the slot holds a resolved address (not the
	 * unresolved ordinal 0x800000C6) */
	{
		u32 fn = *(volatile u32 *)ADDR_KT_FLUSH, iosb[2];
		if (!g_log_hold && (fn & 0x7FFFFFFFu) >= 0x10000u) ((s32 (__attribute__((stdcall)) *)(void *, void *))fn)(h, iosb);   /* (unresolved: 0x80000000 | the ordinal) */
	}
	CloseHandle_x(h);
}
#endif
#else
static s32 g_log_hold;
static inline void log_write(void) { }
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
static s32 o_ctimer = 0;       /* 6i.156: the Campaign Timer (Other Tools; off by default) */
static s32 o_hiv = 0;          /* 6i.178: Other Tools > Havok Island Viewer (under Sleep Timer) */
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
static s32 o_ohk = 0;            /* 6i.170: Cheats > One Shot Kills */
static s32 o_inf_ammo = 0;     /* Infinite Ammo: the game's own (multiplayer) infinite ammo, for players' weapons anywhere (a call hook) */
static s32 o_bottomless = 0;   /* Bottomless Clip: a player's shots take nothing from the magazine or the battery */
static s32 o_skull[16];        /* the skulls, by the game's index: 0 Envy, 1 Grunt Birthday Party, 2 Assassins, 3 Thunderstorm,
                                  4 Famine, 5 IWHBYD, 6 Blind, 7 Ghost, 8 Black Eye, 9 Catch, 10 Sputnik, 11 Iron, 12 Mythic,
                                  13 Anger, 14 Whuppopotamus */
static s32 o_acro = 0;         /* Acrophobia: the debug build's cheat_jetpack (crouch: hover; crouch + A: fly where you look) */
static s32 o_inv_veh = 0;
static s32 o_bump = 0;
static s32 o_hide_hud = 0;     /* 6i.120: Hide HUD (Camera Settings) - the game's HUD (its messages too) left out; the first-person model stays */
static s32 o_fov = 0;          /* 6i.120: Field of View, degrees (70 - 120 in the menu); set to the game's own once it's read, which the menu marks * */
static s32 g_fov_def;          /* the game's own on-foot field of view, whole degrees (0: not read yet) */
#ifdef HOST
static s32 g_fov_settle = 0;   /* (the tests: at once, unless they ask) */
void host_set_fov_settle(s32 n) { g_fov_settle = n; }
static u32 g_frames;
void host_frames_add(u32 n) { g_frames += n; }
#else
#define g_fov_settle 10        /* 6i.220: frames the camera's angle must hold still before it is taken as the game's own */
#endif
static float g_fov_edge = 1.f; /* 6i.219: what the view's angle is widened by for the water's edges (see tp_camera_fov) */
static u32 g_ws, g_ws_flag5;   /* (the game's widescreen test and flag, this frame - set where the overlays' projection is; needed here first) */
static float g_fov_def_rad;    /* ... exactly, radians */
static float g_fp_s[4] = { 1.f, 1.f, 1.f, 1.f };   /* 6i.128: the first-person body's widening for the Field of View (1: none) */
static s32 o_stored_vel = 0;   /* 6i.120: Show Stored Velocity (Other Tools) */
static s32 o_trate = 1;        /* 6i.120: Tick Rate (Other Tools) - 0 25, 1 30 (the game's), 2 60 */
static const u8 g_tick_rates[3] = { 25, 30, 60 };
static s32 g_fc_zoom;          /* 6i.120: the free camera's zoom - 0 off, else its level (6i.122: the right stick's click in / out) */
static float g_fc_zs = 1.f;    /* 6i.123: the zoom as shown - 1 / magnification, eased toward the set one */
static s32 g_fc_zlv = 2;       /* 6i.122: the zoom's level, 2 .. 20 (D-pad Right / Left; held: quickly), kept while zoomed out */
static u8 g_fc_zdir;
static u16 g_fc_zheld;
static s32 o_crouch_turbo = 0; /* 6i.122: Crouch Turbo (Other Tools) */
static s32 o_trig = 0, o_trig_names = 1, o_trig_bars = 1, o_trig_flash = 1, o_trig_hl = 1;   /* 6i.124: Trigger Volume Overlay and its settings */
static s32 o_hud_ws = 1;       /* HUD Widescreen Fix (6h): in widescreen, each HUD element drawn 3/4 as wide around its anchor */
/* 6i.40: Game Hacks > Performance Mode Settings - with Performance Mode on. VSync, Late Frames, FPS Limit and Anisotropic
 * Filtering back as settings (6i.31 / 6i.3 / 6i.26), at what Performance Mode applied (6i.36). The split-screen detail
 * levers - what the game itself does for 2 - 4 players (function_2c490, every frame from the view build function_223080,
 * the local player count in eax), each on its own for one view: hooks.S coop_hook calls the game's, then coop_apply:
 *   Lower Max LoD           the highest level of detail models may use (g_4ba01c: 4 -> 3)
 *   Coop Render Passes      three render passes off (g_4ba022 / 23 / 24: passes 8, 2, 17)
 *   Coop Dynamic Lighting   special light shapes and objects' dynamic lighting off (g_4ba020 / g_4ba025)
 *   Scale Down Object Detail object detail priority 0.85 -> 0.3 (g_4670e4) and a distance scale 1.0 -> 0.8 (g_4ba028)
 * (its other two, g_4ba021 and g_4ba02c, are read nowhere.) Off by default. */
static s32 o_rs_point = 4;                                 /* 6i.71: Stretch Point - 4 (before function_13630): your 6i.71 test; 6i.72: no longer in the menu */
static s32 o_particles = 0;                                 /* 6i.72: Particles; 6i.73: 0 game's, 1 fast (alpha cut 8), 2 faster (alpha cut 32) */
static s32 o_shadows = 0;                                   /* 6i.75: Object Shadows; 6i.77: 0 on, 2 off (6i.76's half rate gone - < 1 fps) */
static s32 o_lightf = 2;                                    /* 6i.85: Light Filtering - 0 game's, 1 fast; 6i.90: 2 faster; 6i.92: faster by default (your test: no difference seen, +1 fps) */
static s32 o_world_mip = 0;                                 /* 6i.92: World Mip Blending - on: the game's; off: the world's passes from the nearest mip level; 6i.104: off by default (your test: 43 -> 50 fps, no difference seen) */
extern volatile u8 spooky_scr_point;                          /* 6i.83: below, with scr_pre */
static u32 g_scr_n;
static u32 g_lfs_n[4];                            /* 6i.87: Light Filtering - stages changed (a capture's) */
static u8 g_aniso_world;                          /* 6i.88: capture 3 - Anisotropic Filtering off in the world's passes only */
static u8 g_pass_now;                             /* 6i.87: the pass being drawn (0 between passes) */
/* 6i.88's GPU register readout and 6i.87's first-light-draw stages retired in 6i.90 (answered: your 6i.89 log) */
static s32 o_shadow_on = 1;                                /* 6i.77: Object Shadows on / off (the menu toggle) */
static s32 o_lights = 0;                                    /* 6i.77: Dynamic Lights - 0 game's, 1 fast (each light's drawing clipped to its reach on screen); 6i.165: game's by default - fast changes how lights show on other players */
static s32 o_pass12 = 1;                                    /* 6i.75: Smoke Pass (test) - off: render pass 12 skipped */
volatile u32 spooky_pass_mask;                              /* 6i.75: hooks.S pass_hook - bit k set: pass k skipped */
static s32 o_water_rate = 0;                                /* 6i.73: Water Refraction - 0 every frame, 1 half rate, 2 off (for the 360) */
static s32 o_rscale = 10;                                  /* 6i.61: Render Scale - 5 .. 10 = 50 .. 100 %; 6i.130: to 15; 6i.149v: to 20 (200 % = 1280 x 960, 3 x 4800 KB - the memory check decides) */
static s32 o_rs_adapt;                                     /* 6i.130: Adaptive Resolution Scaling */
static s32 g_rs_eff = 100;                                 /* 6i.130: the scale drawn at, set at each frame's start; 6i.133: in percent (50 .. 100) */
static s32 g_rs_hw_cut;                                    /* 6i.133: the half sizes were cut this frame (so the stretch puts them back) */
static void rs_adapt_sample(u32 busy_us);
static void rs_eff_set(void);
#ifdef HOST
static s32 o_fx_copy = 0;                                   /* 6i.60: Distortion Effects; 6i.61: gone - no difference at Regret's hologram (target 19 untouched) */
static s32 o_water_surface = 1, o_water_refr = 1;         /* 6i.61: no longer in the menu - fast always (your 6i.59 test) */
#else
/* 6i.143 - your note: in widescreen the water stops short of the screen's left and right edges (both consoles). The
 * suspect: the "fast" water refraction (6i.59, fixed on since 6i.61) - the water's refraction copy drawn with the screen's
 * depth bound (spooky_copy_keepz) instead of its own; the water draws from that copy. Back to the game's own path (0) for
 * this build: if the cut-off goes, the fast path is the cause, and it gets fixed for widescreen rather than left off. */
static const s32 o_fx_copy = 0, o_water_surface = 1, o_water_refr = 1;   /* 6i.145: fast again - your 6i.144 test: the cut-off stayed on the game's path, so it wasn't this */
#endif
/*          6i.58: Water Surface (test); 6i.59: Water Refraction - 0 game's, 1 fast, 2 late, 3 off */
/* v2.0 (13 Nov): YOUR NOTE - Larger Caches ON AT EVERY START where the console has 128 MB. The setting starts unset (-1)
 * and is settled the first time the console's memory is known (lc_default, the first frame): on with more than 96 MB,
 * off otherwise. It is NOT kept in spooky_settings.txt (your instruction: "Don't save this setting") - turning it off
 * lasts until the game is next started, when it is on again. A "Larger Caches (128 MB)=" line in an old settings file
 * is ignored. What the setting does is unchanged: the caches are taken at a level's load; a 64 MB console cannot have it. */
static s32 o_big_cache = -1;    /* 6i.191: Game Hacks > Larger Caches (128 MB): -1 not chosen yet (the default by the console's memory), 0 off, 1 on */
static s32 big_cache_can(void);  /* 6i.192: the console has more than 96 MB */
static s32 on_main_menu(void);
static s32 lc_level_loaded(void); /* 6i.201: a level is loaded, and the game's "restart the level" flag is known */
static s32 o_tear = 1, o_vsync = 1, o_fps_cap, o_aniso = 1;   /* 6i.189: Allow Tearing ON by default, and by Reset to defaults (your note; 6i.63 had it off) */   /* 6i.151: Anisotropic Filtering 2x fast by default (was off) */   /* 6i.63: Allow Tearing off by default (a steady 60 is in reach now) */    /* 6i.46: o_fps_cap - an index into g_fps_caps (0: none) */
static const u8 g_fps_caps[13] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120 };   /* 6i.46 / 6i.47: by 10 (the PAL steps 25 and 75 dropped) */
/* 6i.42: FPS Counter (Performance Mode Settings, off) - "FPS: N" in the screen's top-left corner, as far in as the main
 * menu's hint line sits in the bottom-left. Frames counted against the screen's own refreshes (vblanks): once a refresh
 * rate's worth has gone by (~1 s), N = frames x rate / refreshes - true at 30, 60, uncapped or slowed. */
static s32 o_fps_show;
static u32 g_fps_frames, g_fps_vb, g_fps_shown;
/* 6i.64: Render Scale's per-frame counts (the readout beside the FPS Counter while Render Scale is under 100 %): main
 * scenes drawn, cuts made, interface marks (49), stretches, of those at the HUD view (the fallback) - the last frame's */
static u32 g_rs_frame, g_rs_done = 0xFFFFFFFFu;
static u8 g_rsc[5], g_rs_show[5];

static u32 g_cap_t;
static s32 o_coop_lod, o_coop_passes, o_coop_lights, o_coop_detail;
static const s32 o_trk_dx = 0, o_trk_dy = 1, o_trk_w = 98;    /* 6i.39 - 6i.47: the motion tracker's ring - moved (pixels) and its width (%): your values, set */
static const s32 o_bar_dx = 1, o_bar_dy = 0, o_bar_w = 107;    /* 6i.41 - 6i.47: the health (shields) bar on it - your values, set */
static s32 o_wire;                                          /* 6i.44: Wireframe (Cheats); 6i.52: 0 off, 1 on (wire_pass), 2 xray (wire_fill) */
static const s32 o_gpu_step = 0;   /* 6i.39's GPU overclock, retired in 6i.41 (the fans stayed up after; not worth the risk) */
/* 6i.39's hard drive UDMA mode (test) retired in 6i.42 - no change seen */
static s32 o_hud_fix = 0;      /* HUD Scaling Fix: Halo 2 HD's DisableAnamorphicScaling - the 4/3 anamorphic factor 1.0 (the HUD
                                  drawn in the right shape on a 16:9 picture); none of HD's resolution changes */         /* Bump Possession: the debug build's cheat_bump_possession - touch a unit no player controls, become it */      /* Invincible Vehicles: every vehicle the game's cannot-take-damage + cannot-die flags */
static s32 o_invuln = 0;       /* Cartographer's "Invulnerability": the game's own cannot-take-damage + cannot-die flags */
static s32 o_launch_fwd = 10, o_launch_up = 0, o_tp_fwd = 5, o_tp_up = 0;   /* Launch (u/s) / Teleport (units), along your view */
static volatile s32 g_req;                      /* a practice action asked for: 1 checkpoint, 2 revert, 3 launch, 4 teleport, 5 third person */
static s32 o_keep_charge = 0;  /* Cartographer's "Maintain charge state on ... reverts" (the revert part) */
static s32 o_speed = 10;       /* 6h.22: Game Speed, tenths: 0.0 to 5.0 (1.0 the game's own) */
static s32 o_pause = 0;        /* 6h.22: Pause Game - the game's speed 0 while on; Game Speed kept for when it's off */
/* 6i: Unlock FPS (Camera Settings). The game's frame pacing (v1.0 0x12b0e0 / v1.5 0x12dbf0, at the start of each frame)
 * sets the frame's last vblank: the one the last frame was shown on plus N - the display setting's 1 (D3D presents every
 * vblank, the setting at 0x43ed80), raised to 2 in a game (0x12b191: "if N <= 2, N = 2"): 30 a second on a 60 Hz TV. The
 * draw waits for it (0x12b2e0) and presents. The frame's seconds the game runs its ticks by are those vblanks / the
 * refresh rate. With this on (in a game), the raise is skipped: N = 1 - a frame as soon as one is ready, up to 60. The
 * ticks stay 30 a second, 1/30 s each (the game's time update turns the frame's seconds into ticks, the rest carried:
 * at 60 a tick every other frame); every frame is drawn between the last two ticks - the slow motion's interpolation
 * (6h.25). The byte the raise is tested on there (game in progress - read again: the code only gets there when it's
 * non-zero, tested at 0x12b14e) is read from spooky_pace_clamp instead (the XBE's instruction at v1.0 0x12b172 / v1.5
 * 0x12dc82 points at it): 1, the game's own pacing, exactly; 0, the raise skipped. Cutscene cameras aren't blended (the
 * interpolation's known gap): a moving one steps at 30 as before. The mod's own timing counted in frames (messages, held-button repeats, the
 * Superbounce Viewer's search schedule, the free and third person cameras' easing) runs on the vblanks while this is
 * on: the same per second at any frame rate. Off, everything is as before, a frame at a time. */
static s32 o_unlock_fps = 0;
/* 6i.3: Late frames at once (Camera Settings, under Unlock FPS; only while it's on). At 60, a frame that misses its vblank
 * by a little waits for the next one - 33 ms on screen for 17 of work, the GPU idle meanwhile (your logs: a frame in four
 * toward Headlong's cranes). With this on, such a frame is shown the moment it's done, from where the TV's picture has
 * got to (a tear) - D3D's "one vblank, or at once if late" (D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE), the game's own display
 * setting byte (v1.0 0x485ac9; Halo 2 ships it 0). The game re-applies its display settings itself when they differ from
 * the ones it wants (game_loop_frame_update: memcmp of the runtime ones, v1.0 0x485ac8, with v1.0 0x43ed80 / v1.5
 * 0x448e40, then: its buffers released (0x14980), the device reset (0x128c0), taken again (0x14600)) - those two reads of
 * what it wants point at spooky_display instead: the game's own values each frame, the byte set while this is on. Turning
 * it on or off costs that one re-apply (a moment). */

/* 6i.30's tear zone retired in 6i.31 (the tearing seen is the GPU finishing frames late, which the CPU can't time) */
/* 6i.31: Vsync (Camera Settings, on). Off, with Unlock FPS: the display set to the game's own "immediate" mode - its
 * config's high word (the frame interval in refreshes, byte 2 of its first dword) 0: Direct3D shows each frame the
 * moment it's done (function_128c0: present interval IMMEDIATE), and the frame pacing doesn't wait for a refresh at all
 * (function_12b0e0 skips its interval when it's 0; the next frame's time is then the refreshes that really passed, so the
 * game's clock stays true). FPS cap (tens, 0 none): with vsync off, each frame waits until a tenth-step rate's time since
 * the last (30 - 120). */

/* 6i.32 / 6i.33's Let loading run retired in 6i.34: a 1 ms sleep every frame didn't bring the lifts back - not the loader */
/* 6i.35: Particle fix (60 fps), Camera Settings, on. The lifts' effects that never came back at 60 (Midship): particles
 * spawn on ticks, but a particle system with no particles yet was deleted on any frame that had no tick since the last
 * (hooks.S ptc_update_hook says how). Set here: a tick clears spooky_ptc_hold (ip_tick_before); each frame's draw sets it
 * (ip_draw_before) - so it's 1 exactly while a frame's update runs with no tick since the last frame's. Unlock FPS off: 0.
 * (6i.34's Keep unseen particles test and its counts, which found it, retired.) */
u8 spooky_ptc_hold;
u32 spooky_ptc_kept;
u32 spooky_display[3] = { 0x00010000u, 3u, 1u };   /* what the game wants its display to be (its constant's values; 6i.3) */
/* 6i.3: where a frame's time goes (the 10-second line) - its start (frame_time_update), the game's draw (ip_draw_before /
 * after, around v1.0 0x223240: its views, HUD and menus), and inside the draw its own wait for the vblank and the present
 * (hooks.S prof_wait_hook / prof_present_hook) - so: update (to the draw: the ticks, the mod's frame), drawing (the draw,
 * the two waits taken out), the vblank wait, the present (a GPU that's behind holds it here - or in the drawing, where
 * Direct3D waits for room), after the draw (the main loop to the next frame); the mod's own, inside those. */
enum { PF_UPDATE, PF_DRAW, PF_WAIT, PF_PRESENT, PF_AFTER, PF_MOD, PF_N };
static u32 g_pf_t0, g_pf_d0, g_pf_d1, g_pf_mod, g_pf_ip, g_pf_have;   /* this frame: start, draw start / end, the mod's own, interpolation */
static u32 g_pf_w, g_pf_p, g_pf_wat;                                 /* this frame: the vblank wait, the present; a wait's start */
static u32 g_pf_n, g_pf_sum[PF_N], g_pf_max[PF_N], g_pf_ip_sum;     /* since the last 10-second line (us) */
/* 6i.182: the in-between frames' own time, split (your note: find what it costs before making it cheaper): the tick's
 * snapshot, and each frame's three passes over what it tracks - save (the current pose copied and compared), blend,
 * restore - in timer counts since the last 10-second line; with how much there was to do */
static u32 g_ipt_snap, g_ipt_save, g_ipt_blend, g_ipt_rest, g_ipt_ticks, g_ipt_objs, g_ipt_parts, g_ipt_blended;
static u32 g_pf_slow_n, g_pf_slow_sum[PF_N], g_pf_last[PF_N + 1];   /* frames over 17 ms: how many, their sums; the last frame's (and its whole) */
/* 6i.5: render work timed inside the frame (hooks.S prof_*_hook): 0 a view's visibility (with its objects' preparation),
 * 1 the view's objects' preparation, 2 the lights' objects', 3 the texture cache's update, 4 objects' lighting samples
 * (and how many); the frame's render memory at its draw's end */
/* 6i.8: the render-work probes retired (answered: visibility 2-3 ms, its objects under 1, memory under 25 KB); the
 * visibility call is still hooked, for the census's eye */
static u32 tsc_lo(void);

/* 6i.12: every render pass draws through one call - function_44550 (v1.0 0x445b2 / v1.5 0x445a2) -> function_3bc30(group;
 * edi the pass's record sources, ebx its mode; ret 4) - so hooks.S pass_hook times each pass's drawing (the CPU's work
 * issuing it, and any wait for the GPU's command buffer while it does) into spooky_pass_cyc[mode], and draws nothing
 * for the pass Skip render pass (test) names: to see what each pass draws (what goes missing) and what it costs. In the
 * game's code, not Direct3D's: the regular files, both consoles. */
u32 spooky_pass_cyc[64], spooky_pass_t0, spooky_fx_t0;   /* 6i.18: 64 - the marks' parts from 28 on */
/* 6i.17: GPU time by pass (test, Camera Settings). The passes' times above are the CPU's - the GPU draws later, so a pass
 * whose pixels are expensive (a big texture, the sky, fog) costs the CPU nothing and shows nowhere. With this on, the GPU is
 * made to finish everything before each pass and finish the pass itself before going on (the game's own way of waiting
 * for it: KickPushBuffer - v1.0 0x3fac30, the device in edx - then the push buffer's get and put equal and the graphics
 * engine idle, the device's registers at +0x934: +0x3240 / +0x3244 / +0x400700), so each pass's time is its whole cost,
 * CPU and GPU; everything drawn outside the passes (the sky, effects, the HUD, the first-person model...) adds up in
 * slot 28. The frame rate drops while it's on (nothing overlaps): it's for finding what's expensive. */
static s32 o_gpu_time;
static u32 g_gt_t0, g_mk_last, g_mk_id;
/* 6i.19: a capture (White + D-pad Up, in a game): 30 frames timed with the GPU drained at every mark, whatever GPU time by
 * pass is set to, then one line - "capture N:" and each part's ms a frame - and back as it was. A spot in a second. */
static u32 g_cap_frames, g_cap_n;
/* 6i.26: a capture is 30 frames after a warm-up: the parts timed on 15 (no waits between draws - as 6i.19 - 6i.22 were),
 * each draw timed on the other 15 (waits round every draw: the shaders line, whose many-draw entries those waits still
 * inflate). 6i.23 - 6i.25 did both in every frame, which inflated the parts of passes with many draws (pass 16's 120+). */
static u8 g_cap_live, g_cap_rec, g_cap_nat;      /* 6i.90: g_cap_nat - one of the capture's frames run as normal */
static u8 g_cap_pxoff, g_cap_nomip;              /* 6i.91: the series' second and third captures - no pixels drawn / no mip blending */
static u8 g_cap_bump;                             /* 6i.119: the third capture - a repeat of the first (a check that the numbers hold) */

static s32 nv2a_ok(void);                        /* 6i.109: used before its definition */
#ifdef HOST
u8 spooky_rt_on;                                 /* 6i.109: hooks.S's byte on the console */
#else
extern u8 spooky_rt_on;
#endif                                /* 6i.109: the light's own depth function, as the game set it */
static u32 g_rt_n;                               /* 6i.92: render target switches in a capture's parts frames */
static u32 g_li[2][8]; static u8 g_li_n;           /* 6i.95: the capture's first two kinds of light (light_note) */
/* 6i.98: WHAT A DRAW COSTS THE GPU BEYOND ITS PIXELS. Your logs: with no pixels drawn the lighting pass still costs the
 * GPU 2 ms at Regret over ~60 draws - twice what each colour-pass draw costs. Either its draws carry many vertices (the
 * lighting pass's vertex program is the longer one) or each draw carries a fixed cost (its state: four textures, two of
 * them palettized and their palettes loaded). hooks.S keeps each draw's vertex / index count (spooky_draw_n: Direct3D's
 * DrawIndexedVertices count, DrawVertices' in eax); a capture's draws' frames sum it per record, and each pass's records
 * are fitted, GPU time against count: GPU ns = a + b x count - a, the fixed cost of a draw; b, a vertex's. */
#ifdef HOST
u32 spooky_draw_n;
#else
extern u32 spooky_draw_n;
#endif
static u32 g_rec_ix, g_fx_ix[32];
/* 6i.110: DOES THE LIGHT DRAW SURFACES THE CAMERA NEVER DREW? Your 6i.109 log: the light's depth test is already strict
 * ("less-or-equal" in the game's setter cache, but its pixel count shows the light really tests for equal), and
 * loosening it bought nothing but extra lit pixels - the GPU won't skip a hidden pixel early here. So the hidden pixels
 * have to be kept from being drawn at all. Many may belong to surfaces the camera can't see - rooms behind the
 * hologram's walls, inside its reach - which the colour pass never drew this frame. Each draw's index data (Direct3D's
 * DrawIndexedVertices: its third argument, kept by hooks.S as spooky_draw_ip) identifies its surface: the colour and
 * lighting passes' are noted each frame, and every record of the light's pass is classed as one the camera drew or not,
 * with its GPU time and pixels. A big "not drawn by the camera" share means skipping those draws is the fix - nothing
 * you could see changes. */
#ifdef HOST
u32 spooky_draw_ip;
#else
extern u32 spooky_draw_ip;
#endif
/* 6i.111: your 6i.110 log - matched by where each draw's index data STARTS, 12 of the light's 22 records looked unseen
 * yet put 87,000 pixels on screen: the light draws parts of the same index data the camera drew, starting elsewhere in
 * it. Now a draw is the RANGE of index data it reads (start, count x 2 bytes), and a light draw counts as seen when its
 * range overlaps any range the colour or lighting pass read this frame. A light draw of index data the camera's passes
 * never touched cannot put a pixel on screen: the light tests depth for equal, and only surfaces the camera drew wrote
 * that depth. Those draws are skipped - Skip Hidden Light Draws (test), and the third capture - and nothing changes on
 * screen. Too many camera draws to hold (256 a frame): nothing is skipped that frame.
 * 6i.112: your test - no difference on or off, 4-5 fps; the light 2.8 -> 1.7 ms at Regret. Made a setting, on by
 * default. Your 6i.111 log also showed the skipped draws would have put 2,000 pixels (1 %) on screen: surfaces some
 * other pass drew. So every draw of any pass outside a light now counts as the camera's (only ever fewer skips), and
 * the light's shadow pass (11) is skipped by the same rule. And the third capture asks whether the colour pass - the
 * one that writes depth, 2.6 ms with 1.8 layers drawn over every pixel - gets its hidden pixels thrown away before the
 * work, or after, as the lights do: drawn with its depth test off, its pixel count rises to all it rasterizes; if its
 * time barely moves, its hidden pixels were paid for all along. */
static u32 g_cr[1024][2], g_cr_n, g_cr_frame, g_rec_un, g_rec_se, g_un[2][3];   /* 6i.150: 1024 ranges (was 256: a busy frame overflowed it and Skip Hidden Light Draws switched itself off) */   /* g_un: [unseen, seen] x [records, GPU ns, pixels] */
static s32 o_light_skip = 1;                       /* 6i.111: Skip Hidden Light Draws; 6i.112: a setting, on by default (your test: no difference seen, 4-5 fps); 6i.143: always on with Performance Mode, its menu item gone (your note) */
#ifdef HOST
u8 spooky_skip_draw;
#else
extern u8 spooky_skip_draw;
#endif
void cam_range_add(u32 ip, u32 n)                  /* 6i.150: a camera draw's index range [ip, ip + 2n); one that continues the previous entry extends it
                                                    * (the passes walk index buffers contiguously), so the table stays small; past 1024 only the count grows */
{
	u32 e = ip + 2u * n;
	if (g_cr_n && g_cr_n <= 1024u && g_cr[g_cr_n - 1u][1] == ip) { g_cr[g_cr_n - 1u][1] = e; return; }
	if (g_cr_n < 1024u) { g_cr[g_cr_n][0] = ip; g_cr[g_cr_n][1] = e; }
	g_cr_n++;
}
void cam_range_reset(void) { g_cr_n = 0; }
s32 cam_seen(u32 a, u32 n)                         /* does [a, a + 2n) overlap a range the camera's passes read this frame? */
{
	u32 b = a + 2u * n, q;
	if (g_cr_n > 1024u) return 1;                  /* overflowed: count it seen (never skip) */
	for (q = 0; q < g_cr_n; q++) if (a < g_cr[q][1] && g_cr[q][0] < b) return 1;
	return 0;
}
/* 6i.99: WHERE THE GAME DRAWS - and whether the GPU's memory tiles cover it. Your 6i.98 log: the lighting pass's cost
 * beyond its pixels is all per vertex (68 ns a vertex, nothing fixed a draw) - so its draws' setup isn't the cost; the
 * rest of every pass is pixels. On the NV2A a target the memory controller tiles (PFB tiles: base, limit, pitch; a
 * depth buffer's with Z compression) is written and read faster. Direct3D tiles the frame buffer and depth it makes
 * itself; the game's own texture accumulation buffer (pass 1 writes all of it, pass 3 and every light read it) may not
 * be. The capture reads the eight tiles (regs + 0x100240 + 16 i, a real NV2A only) and where pass 1's and pass 3's
 * targets and the depth sit (Direct3D's device + 0x1a04 / + 0x1a08: SetRenderTarget's own stores, v1.0 0x3f78e7 /
 * 0x3f790f; a surface's data + 4, format + 0xc, size + 0x10). */
static u32 g_rtd[3][3];
static double g_fx[32][5];                       /* by pass: records, sum count, sum ns, sum count x ns, sum count^2 */
void fit_line(const double *s, double *a, double *b)
{
	double d = s[0] * s[4] - s[1] * s[1];
	*b = d > 0.0 ? (s[0] * s[3] - s[1] * s[2]) / d : 0.0;
	*a = s[0] > 0.0 ? (s[2] - *b * s[1]) / s[0] : 0.0;
}
/* 6i.23: a capture's shaders. Every record a render pass draws goes through hooks.S rec_hook (function_3bc30's loop: its
 * call of the record's draw, replaced by call [spooky_recp]). While a capture runs, each record's draw is timed with the
 * GPU drained before and after, and summed by its shader (the sort record's +0x10: the shader's compiled pass, the same
 * for every draw of that material) and pass; the capture's second line lists the costliest, and the toast names the top
 * one. Skip shader (test) 1 - 6: the last capture's 1st - 6th shader isn't drawn - to see what it is. */
#define SH_N 12
static u32 g_sh_key[SH_N], g_sh_mode[SH_N], g_sh_cyc[SH_N], g_sh_n[SH_N], g_sh_top[6], g_sh_t0, g_sh_cur, g_sh_cur_mode;
/* 6i.82: per shader in a capture, the textures it drew with (stages 0 - 3, as the game's own table of what's bound has
 * them right after its draw: v1.0 0x51f3d8, v1.5 0x52dfa8): the NV2A format, width, height, mip levels, linear or
 * swizzled - to see why the costliest draws cost what they do */
/* 6i.82's per-shader texture listing retired in 6i.93 (answered: the formats are known) */
#ifdef HOST
static s32 o_skip_shader;
#else
static const s32 o_skip_shader = 0;                 /* 6i.93: Skip shader (test) - not in any menu: a constant on the console */
#endif
volatile u8 spooky_rec_on, spooky_rec_skip;
/* 6i.24's Shader details retired in 6i.28 (it told us the ground's and the scenery's layers; the room went to the water) */
/* 6i.28: the water's refraction copy (function_352e0(22, 1): the frame, four samples a pixel, ~1.2 ms) - two tests.
 * Water copy every other frame: with Unlock FPS on, a frame right after one that made the copy keeps it (the texture keeps
 * what was copied into it) - every other copy, whatever the frame rate (6i.22's only did that after a frame already at 60,
 * so at the water it never started). Water copy one sample: the copy's second argument made 0 - one sample a pixel
 * instead of four (as Vista's single blit), the refraction a little less soft. hooks.S mark_hook_60 does both. */
volatile u8 spooky_copy_keep, spooky_copy_ran, spooky_copy_one;
/* 6i.59: WATER REFRACTION (Performance Mode Settings) - game's / fast / late / off. Your 6i.58 test: the slowdown is the
 * copy - off, smooth even with the surface drawn. Its own GPU time is small (0.4 ms one-sample), so the cost is what it
 * does to what follows: the game switches the render target to the copy's texture (target 22, function_14bc0's case)
 * with NO depth surface, and back - unbinding the screen's depth buffer, which costs the GPU its early depth rejection
 * for what's drawn next. Next is the water plane: built from the camera across the whole view at the water's height,
 * mostly hidden behind the ground - normally rejected early, after the copy shaded pixel by pixel (four textures each).
 *   fast  the copy's target set WITH the screen's depth surface (left as it is - the copy draws without depth): nothing
 *         unbound, nothing lost. Looks exactly as the game. (hooks.S copy_target_stub, at the case's jump table entry)
 *   late  the copy made after the water plane instead of before (the next frame's refraction): whatever the copy costs
 *         what follows, the plane is drawn before it. Shows the water a frame later (and itself, faintly).
 *   off   no copy (6i.58's test): what's under the water stays frozen.
 * Every mode but off makes the copy every frame now (one sample, as Performance Mode has since 6i.40): every other frame
 * (6i.28 - 6i.58) left the water a frame behind on alternate frames - the image on it jittered when you turned. */
volatile u8 spooky_copy_keepz, spooky_copy_late, spooky_copy_due, spooky_in_late;
/* 6i.60: DISTORTION EFFECTS (Performance Mode Settings) - game's / fast, fast by default. The same cost as the water's, for
 * the effects that see through themselves - Regret's hologram among them: before they're drawn the game copies the
 * screen (function_352e0 with no target: target 19, four samples - called by function_423c0, function_41cc0 and
 * function_1c0d0 for a shader flagged for it), and target 19's case in function_14bc0 also sets no depth surface. fast:
 * that case keeps the screen's depth bound too (hooks.S copy_target19_stub) - nothing lost for what's drawn after. */
volatile u8 spooky_fx_keepz;
/* 6i.58: Water Surface (test) - the level's water plane (function_3c3c0's one draw, ~2 ms of the GPU where it fills the
 * view) left out, to see what it costs; hooks.S water_draw_hook */
volatile u8 spooky_water_hide;
void water_copy_call(void);                      /* hooks.S: the game's copy wrapper (function_3bea0), void */
static void gw8(u32 a, u8 v);
void water_late_copy(void)                       /* 6i.59: late - from water_draw_hook, after the plane's draw */
{
	spooky_copy_due = 0;
#ifndef HOST
	gw8(ADDR_WATER_COPIED, 0);                   /* the game's "copied this frame" - so its wrapper makes it now */
	spooky_in_late = 1;
	water_copy_call();
	spooky_in_late = 0;
#endif
}
/* 6i.25: both lock onto the material chosen when they're set (the n-th of the capture before), so later captures, which
 * reorder the list, don't change what's skipped or described: set the number again (or 0 and back) to pick afresh */
static s32 g_skip_prev;
static u32 g_skip_key;
/* 6i.25: No anisotropic filtering (test). A material's own sampler settings are set by function_189a0 (edx the shader
 * state, its one argument the range: stages in its top 7 bits; ret 4; called once, function_16b90: v1.0 0x16e44 / v1.5
 * 0x16e24). After it, with this on, each of those stages that minifies or magnifies anisotropically (3) is set to linear
 * (2) and its max anisotropy to 0, in Direct3D's own stage states, before the material draws - the ground's lighting pass
 * draws its 256 x 256 palettized layer with 4x anisotropic filtering. */
/* 6i.53: four levels (Performance Mode Settings > Anisotropic Filtering): off (as 6i.25), 2x fast, 2x, full (the
 * game's own - 4x on the ground). 2x: a layer the game filters anisotropically keeps it, its anisotropy at most 2 - half
 * the samples where the ground meets the eye at a shallow angle (the GPU takes one trilinear sample per step of
 * anisotropy). 2x fast: the same, and that layer's mipmap levels not blended (bilinear, not trilinear) - half again. */
volatile u8 spooky_aniso;                        /* 6i.36: set each frame - Performance Mode; 6i.53: 0 the game's, 1 off, 2 2x, 3 2x fast */
static s32 g_cap_prev;
static void capture_start(void), capture_frame(void), capture_ab_start(void);
/* 6i.18: with GPU time by pass on, the frame is cut at marks - each pass's start and end, each call the main scene makes
 * between and after the passes (hooks.S MARK_HOOK, ids 30 - 57), the two screen effects (58, 59), the draw's start (29)
 * and the present: at each, the GPU is drained and the time since the last mark goes to the last mark's part. 28: after a
 * pass, to the next mark. */   /* 6i.13: [25] / [26] the two screen effects (hooks.S fx_*_hook) */
s32 spooky_skip_pass;
/* 6i.14's Reflections every other frame (the planar reflection, function_296c0) retired in 6i.27: it never ran on the
 * maps tested; the room went to your body's interpolation */
/* 6i.22's Water refraction every other frame retired in 6i.25 (it kept the copy only after a frame already at 60,
 * so at the water - at 30 - it never engaged); Skip render pass 60 still leaves the copy out (the test) */
/* 6i.16's missing-textures line retired in 6i.23 (it showed the water's textures streaming; that question's answered) */
static u32 tsc_lo(void);
volatile u8 spooky_pace_clamp = 1;              /* read by the game's frame pacing (above); 1 until a frame of ours sets it */
static s32 g_vb = 2;                            /* vblanks since the last frame (Unlock FPS on: counted; off: 2, a 30 Hz frame) */
static s32 g_steps = 1;                         /* this frame's 30 Hz steps (on: from the vblanks, 0 or 1 at 60; off: 1 a frame) */
static float g_fdt = 1.f / 30.f;                /* this frame's seconds (off: a 30 Hz frame) */
static u32 g_t30;                               /* the mod's 30 Hz clock (off: the main-loop frames, as g_count_main) */
static u32 g_hud30;                             /* the HUD pass's 30 Hz clock while Unlock FPS is on (its schedules) */
static u32 g_vb_last, g_step_acc, g_vb_hist[4]; /* the last frame's vblank count; the steps' remainder; frames of 1 / 2 / 3 / 4+ vblanks */

/* As Cartographer's "Spooky Ghost Settings": its top-level items, then "Launch / Teleport" and "Superbounce Viewer
 * Settings" as submenus (A enters, B returns - 6i.36: the Back items gone, the help line says so) */
static const s_item m_main[] = {
	{ "Cheats  >", K_SUB, 0, 4, 0 },
	{ "Checkpoint Settings  >", K_SUB, 0, 9, 0 },          /* 6i.40: Game Speed and Pause Game moved to Cheats */          /* 6h.12: Create / Revert Checkpoint moved in there */
	{ "Launch / Teleport  >", K_SUB, 0, 1, 0 },
	{ "Superbounce Viewer", K_TOGGLE, &o_sb_hud, 0, 1 },
	{ "Superbounce Viewer Settings  >", K_SUB, 0, 2, 0 },
	{ "Ghost Viewer", K_TOGGLE, &o_ghost, 0, 1 },
	{ "Ghost Viewer Settings  >", K_SUB, 0, 5, 0 },
	{ "Other Tools  >", K_SUB, 0, 6, 0 },                  /* 6i.120: was Timers */   /* 6i.186: your note - above Camera Settings */
	{ "Camera Settings  >", K_SUB, 0, 3, 0 },
	{ "Game Hacks  >", K_SUB, 0, 10, 0 },                   /* 6i.40 */
	{ "Text scale", K_INT, &o_scale, 6, 14 },             /* 6h.22: Passion Check back to Cheats; Close menu gone (B closes) */
};
static const s_item m_cp[] = {                   /* 6h.12: Checkpoint Settings (from the top, under Cheats) */
	{ "Create Checkpoint (Back + Left)", K_ACTION, 0, 1, 0 },
	{ "Revert Checkpoint (Back + Right)", K_ACTION, 0, 2, 0 },
	{ "Force Double Revert", K_ACTION, 0, 13, 0 },                       /* 6i.121: the checkpoint before the last */
	{ "Disable natural checkpoints", K_TOGGLE, &o_no_natural_cp, 0, 1 },
	{ "Disable reverts", K_TOGGLE, &o_no_reverts, 0, 1 },
};
static const s_item m_launch[] = {
	{ "Launch (Black + Up)", K_ACTION, 0, 3, 0 },
	{ "Launch forward u/s", K_INT, &o_launch_fwd, 0, 60 },
	{ "Launch up u/s", K_INT, &o_launch_up, 0, 60 },
	{ "Teleport (Black + Down)", K_ACTION, 0, 4, 0 },
	{ "Teleport forward", K_INT, &o_tp_fwd, 0, 30 },
	{ "Teleport up", K_INT, &o_tp_up, 0, 30 },
};
static const s_item m_sb[] = {
	{ "Maintain charge state on checkpoints and reverts", K_TOGGLE, &o_keep_charge, 0, 1 },
	{ "Force Charged state", K_TOGGLE, &o_force_charge, 0, 1 },
	{ "Seam info text", K_TOGGLE, &o_land_text, 0, 1 },
	{ "Speed above the charge indicator", K_TOGGLE, &o_sb_speed, 0, 1 },
	{ "Bounces in a row", K_TOGGLE, &o_sb_streak, 0, 1 },
	{ "Seams near you", K_TOGGLE, &o_seams, 0, 1 },
	{ "Passion Check", K_ACTION, 0, 9, 0 },                       /* 6i.170: here, under Seams near you (was in Cheats) */
	{ "Monster/Dreadnought Viewer", K_TOGGLE, &o_pred, 0, 1 },
	{ "Monster/Dreadnought Viewer: 35+ only", K_TOGGLE, &o_pred_dread, 0, 1 },
	{ "Monster/Dreadnought range (units)", K_INT10, &o_pred_range, 0, 200 },   /* 6h.18: Max, then 10 .. 200 */
};
static const s_item m_tp[] = {
	{ "Third person camera (Back + Down)", K_TOGGLE, &o_third_person, 0, 1 },
	{ "Third Person Settings  >", K_SUB, 0, 7, 0 },
	{ "Free camera (Back + Black)", K_TOGGLE, &o_free_cam, 0, 1 },
	{ "Disable freecam player input", K_TOGGLE, &o_fc_block, 0, 1 },   /* 6i.125: was Disable player movement */
	{ "Lock camera pitch", K_TOGGLE, &o_fc_lock_pitch, 0, 1 },
	{ "Teleport to camera (Back + White)", K_STAY, 0, 7, 0 },            /* 6i.121: the menu stays open */
	{ "Hide HUD", K_TOGGLE, &o_hide_hud, 0, 1 },                          /* 6i.120 */
	{ "Field of View", K_INT, &o_fov, 50, 120 },                         /* 6i.120: the game's own marked * */
};   /* 6i.36: Unlock FPS -> Performance Mode (Cheats); its settings and the test settings out of the menu */
static const s_item m_tp3[] = {                 /* Camera Settings > Third Person Settings */
	{ "Distance (tenths)", K_INT, &o_tp_dist, 2, 60 },
	{ "Height (tenths)", K_INT, &o_tp_height, -10, 20 },
	{ "Side (tenths, + right)", K_INT, &o_tp_side, -20, 20 },
	{ "Ignore Collision", K_TOGGLE, &o_tp_nocoll, 0, 1 },
};
static const s_item m_cheats[] = {
	{ "Invulnerability", K_TOGGLE, &o_invuln, 0, 1 },
	{ "Infinite Ammo", K_TOGGLE, &o_inf_ammo, 0, 1 },
	{ "Bottomless Clip", K_TOGGLE, &o_bottomless, 0, 1 },
	{ "One Shot Kills", K_TOGGLE, &o_ohk, 0, 1 },              /* 6i.170 */
	{ "Disable AI", K_TOGGLE, &o_disable_ai, 0, 1 },
	{ "Game Speed", K_INT, &o_speed, 0, 50 },              /* 6h.22: tenths - 0.0 to 5.0; 6i.40: from the main menu */
	{ "Pause Game", K_TOGGLE, &o_pause, 0, 1 },            /* 6h.22; 6i.40: from the main menu */
	{ "Bump Possession", K_TOGGLE, &o_bump, 0, 1 },
	{ "Cycle Possession", K_STAY, 0, 12, 0 },              /* 6h.5: the menu stays open (its message shown under it) */
	{ "Revert to player possession", K_STAY, 0, 8, 0 },
	{ "Invincible Vehicles", K_TOGGLE, &o_inv_veh, 0, 1 },
	{ "Skulls  >", K_SUB, 0, 8, 0 },
};
static const s_item m_hacks[] = {              /* 6i.40: Game Hacks (from the top, under Timers) */
	{ "HUD Widescreen Fix", K_TOGGLE, &o_hud_ws, 0, 1 },
	{ "Performance Mode", K_TOGGLE, &o_unlock_fps, 0, 1 },       /* 6i.36: was Camera Settings' Unlock FPS - see fps_unlocked() */
	{ "Performance Mode Settings  >", K_SUB, 0, 11, 0 },
	{ "Tick Rate", K_INT, &o_trate, 0, 2 },                              /* 6i.120: 25 / 30 / 60; 6i.121: from Other Tools */
	{ "Toggle Triple Buffering (restarts)", K_SUB, 0, 13, 0 },            /* 6i.135: asks first - see tb_toggle_restart; 6i.197: here, above Larger Caches (your note; was in Performance Mode Settings) */
	{ "Larger Caches (128 MB)", K_TOGGLE, &o_big_cache, 0, 1 },           /* 6i.191: see cache_geo_take - from the next level load */
};
static const s_item m_perf[] = {               /* 6i.40: Game Hacks > Performance Mode Settings */
	{ "FPS Counter", K_TOGGLE, &o_fps_show, 0, 1 },                   /* 6i.42; 6i.152: off / on / detailed (the frame-time lines - ~1 ms of text a frame - only at detailed) */
	{ "VSync", K_INT, &o_vsync, 0, 2 },                           /* off / on / 30 fps (6i.176: the third) */
	{ "Allow Tearing", K_TOGGLE, &o_tear, 0, 1 },                /* 6i.57: was Late Frames */
	{ "FPS Limit (0 none)", K_INT, &o_fps_cap, 0, 12 },           /* with VSync off; 6i.46: the frame rate itself (g_fps_caps) */
	{ "Render Scale", K_INT, &o_rscale, 5, 20 },                        /* 6i.61: 50 - 100 % in tens; 6i.130: to 150 % (6i.138 took it out by mistake; 6i.139: back) */
	{ "Adaptive Resolution Scaling", K_TOGGLE, &o_rs_adapt, 0, 1 },      /* 6i.130 */
	{ "Anisotropic Filtering", K_INT, &o_aniso, 0, 3 },                 /* 6i.53: off / 2x fast / 2x / full; 6i.189: under Adaptive Resolution Scaling (your note) */
	{ "Light Filtering", K_INT, &o_lightf, 0, 2 },                       /* 6i.85: game's / fast; 6i.90: faster */
	{ "World Mip Blending", K_TOGGLE, &o_world_mip, 0, 1 },              /* 6i.92 */
	{ "Lights & Shadows", K_INT, &o_lights, 0, 1 },                     /* 6i.77: game's / fast; 6i.81: objects' shadows too */
	{ "Object Shadows", K_TOGGLE, &o_shadow_on, 0, 1 },                 /* 6i.75: off - passes 10 / 11 skipped (6i.76's half rate gone) */
	{ "Dynamic Lighting", K_TOGGLE, &o_pass12, 0, 1 },                 /* 6i.75: off - pass 12 skipped */
	{ "Coop Render Passes", K_TOGGLE, &o_coop_passes, 0, 1 },          /* 6i.120: Coop Dynamic Lighting's two flags too - one setting */
	{ "Lower Shader Detail", K_TOGGLE, &o_coop_detail, 0, 1 },              /* 6i.120: was Scale Down Object Detail - what it does */
	{ "Reset to defaults", K_STAY, 0, 20, 0 },                            /* 6i.120 */
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
};
/* ---- 6i.158: Object Spawning (Other Tools) - H2EK's "Drop Objects" debug menu on the retail game. The list is H2EK's
 * (its names, its order, its headings); what a menu shows is what the loaded level actually holds of it (os_build, below):
 * the retail game has no tag files, only the tags compiled into the level's map. Vehicles: the ones a player can operate
 * (H2EK's Pelican, Phantom, Shadow and Uberchassis are left out). ---- */
enum { OSC_HW, OSC_CW, OSC_PU, OSC_HV, OSC_CV, OSC_BH, OSC_BC, OSC_BF, OSC_OBJ, OSC_MISC, OSC_N };
#define OSG_WEAP 0x77656170u
#define OSG_EQIP 0x65716970u
#define OSG_VEHI 0x76656869u
#define OSG_BIPD 0x62697064u
#define OSG_BLOC 0x626C6F63u                     /* a crate */
typedef struct { u8 cat; u32 group; const char *label, *path; } s_os_def;
static const s_os_def os_defs[] = {
	{ OSC_HW, OSG_EQIP, "Frag Grenade", "objects\\weapons\\grenade\\frag_grenade\\frag_grenade" },
	{ OSC_HW, OSG_WEAP, "Pistol", "objects\\weapons\\pistol\\magnum\\magnum" },
	{ OSC_HW, OSG_WEAP, "SMG", "objects\\weapons\\rifle\\smg\\smg" },
	{ OSC_HW, OSG_WEAP, "SMG (silenced)", "objects\\weapons\\rifle\\smg_silenced\\smg_silenced" },
	{ OSC_HW, OSG_WEAP, "Battle Rifle", "objects\\weapons\\rifle\\battle_rifle\\battle_rifle" },
	{ OSC_HW, OSG_WEAP, "Battle Rifle (alpha)", "digsite\\objects\\weapons\\rifle\\battle_rifle_combo\\battle_rifle_combo" },
	{ OSC_HW, OSG_WEAP, "Shotgun", "objects\\weapons\\rifle\\shotgun\\shotgun" },
	{ OSC_HW, OSG_WEAP, "Sniper Rifle", "objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle" },
	{ OSC_HW, OSG_WEAP, "Rocket Launcher", "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher" },
	{ OSC_HW, OSG_WEAP, "GPMG", "objects\\weapons\\rifle\\gpmg\\gpmg" },
	{ OSC_CW, OSG_EQIP, "Plasma Grenade", "objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade" },
	{ OSC_CW, OSG_WEAP, "Energy Sword", "objects\\weapons\\melee\\energy_blade\\energy_blade" },
	{ OSC_CW, OSG_WEAP, "Plasma Pistol", "objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol" },
	{ OSC_CW, OSG_WEAP, "Plasma Rifle", "objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle" },
	{ OSC_CW, OSG_WEAP, "Brute Plasma Rifle", "objects\\weapons\\rifle\\brute_plasma_rifle\\brute_plasma_rifle" },
	{ OSC_CW, OSG_WEAP, "Needler", "objects\\weapons\\pistol\\needler\\needler" },
	{ OSC_CW, OSG_WEAP, "Carbine", "objects\\weapons\\rifle\\covenant_carbine\\covenant_carbine" },
	{ OSC_CW, OSG_WEAP, "Beam Rifle", "objects\\weapons\\rifle\\beam_rifle\\beam_rifle" },
	{ OSC_CW, OSG_WEAP, "Fuel Rod Cannon", "objects\\weapons\\support_high\\flak_cannon\\flak_cannon" },
	{ OSC_CW, OSG_WEAP, "Brute Shot", "objects\\weapons\\support_low\\brute_shot\\brute_shot" },
	{ OSC_CW, OSG_WEAP, "Sentinel Beam", "objects\\characters\\sentinel_aggressor\\weapons\\beam\\sentinel_aggressor_beam" },
	{ OSC_CW, OSG_WEAP, "Sentinel Beam (blue)", "objects\\characters\\sentinel_aggressor\\weapons\\beam_elim\\sent_agg_beam_elim" },
	{ OSC_CW, OSG_WEAP, "Scarab Gun", "scenarios\\objects\\covenant\\military\\scarab\\weapons\\main_gun\\scarab_main_gun_handheld" },
	{ OSC_PU, OSG_EQIP, "Overshield", "objects\\powerups\\over_shield\\over_shield" },
	{ OSC_PU, OSG_EQIP, "Active Camouflage", "objects\\powerups\\active_camouflage\\active_camouflage" },
	{ OSC_HV, OSG_VEHI, "Warthog", "objects\\vehicles\\warthog\\warthog" },
	{ OSC_HV, OSG_VEHI, "Scorpion", "objects\\vehicles\\scorpion\\scorpion" },
	{ OSC_CV, OSG_VEHI, "Ghost", "objects\\vehicles\\ghost\\ghost" },
	{ OSC_CV, OSG_VEHI, "Banshee", "objects\\vehicles\\banshee\\banshee" },
	{ OSC_CV, OSG_VEHI, "Spectre", "objects\\vehicles\\spectre\\spectre" },
	{ OSC_CV, OSG_VEHI, "Wraith", "objects\\vehicles\\wraith\\wraith" },
	{ OSC_BH, OSG_BIPD, "Chief", "objects\\characters\\masterchief\\masterchief" },
	{ OSC_BH, OSG_BIPD, "Johnson", "objects\\characters\\marine\\marine_johnson\\marine_johnson" },
	{ OSC_BH, OSG_BIPD, "ODST", "objects\\characters\\marine\\marine_odst\\marine_odst" },
	{ OSC_BH, OSG_BIPD, "Marine M (random)", "objects\\characters\\marine\\marine" },
	{ OSC_BH, OSG_BIPD, "Marine F", "objects\\characters\\marine\\marine_female\\marine_female" },
	{ OSC_BH, OSG_BIPD, "Crewman", "objects\\characters\\marine\\crewman\\crewman" },
	{ OSC_BH, OSG_BIPD, "Miranda", "objects\\characters\\miranda\\miranda" },
	{ OSC_BH, OSG_BIPD, "Lord Hood", "objects\\characters\\lord_hood\\lord_hood" },
	{ OSC_BH, OSG_BIPD, "Cortana", "objects\\characters\\cortana\\cortana" },
	{ OSC_BC, OSG_BIPD, "Arbiter", "objects\\characters\\dervish\\dervish" },
	{ OSC_BC, OSG_BIPD, "Elite", "objects\\characters\\elite\\elite" },
	{ OSC_BC, OSG_BIPD, "Grunt", "objects\\characters\\grunt\\grunt" },
	{ OSC_BC, OSG_BIPD, "Jackal", "objects\\characters\\jackal\\jackal" },
	{ OSC_BC, OSG_BIPD, "Hunter", "objects\\characters\\hunter\\hunter" },
	{ OSC_BC, OSG_BIPD, "Bugger", "objects\\characters\\bugger\\bugger" },
	{ OSC_BC, OSG_BIPD, "Heretic", "objects\\characters\\heretic\\heretic" },
	{ OSC_BC, OSG_BIPD, "Heretic Grunt", "objects\\characters\\heretic_grunt\\heretic_grunt" },
	{ OSC_BC, OSG_BIPD, "Brute", "objects\\characters\\brute\\brute" },
	{ OSC_BC, OSG_BIPD, "Prophet", "objects\\characters\\prophet\\prophet_minor" },
	{ OSC_BF, OSG_BIPD, "Monitor", "objects\\characters\\monitor\\monitor" },
	{ OSC_BF, OSG_BIPD, "Sentinel Aggressor", "objects\\characters\\sentinel_aggressor\\sentinel_aggressor" },
	{ OSC_BF, OSG_BIPD, "Sentinel Enforcer", "objects\\characters\\sentinel_enforcer\\sentinel_enforcer" },
	{ OSC_BF, OSG_BIPD, "Sentinel Constructor", "objects\\characters\\sentinel_constructor\\sentinel_constructor" },
	{ OSC_BF, OSG_BIPD, "Flood Infection Form", "objects\\characters\\flood_infection\\flood_infection" },
	{ OSC_BF, OSG_BIPD, "Flood Carrier Form", "objects\\characters\\floodcarrier\\floodcarrier" },
	{ OSC_BF, OSG_BIPD, "Flood Combat Form (human)", "objects\\characters\\floodcombat_human\\floodcombat_human" },
	{ OSC_BF, OSG_BIPD, "Flood Combat Form (elite)", "objects\\characters\\floodcombat_elite\\floodcombat_elite" },
	{ OSC_BF, OSG_BIPD, "Flood Juggernaut", "objects\\characters\\flood_juggernaut\\flood_juggernaut" },
	{ OSC_OBJ, OSG_BLOC, "Multi Crate", "objects\\gear\\human\\industrial\\crate_multi\\crate_multi" },
	{ OSC_OBJ, OSG_BLOC, "Multi Crate (single)", "objects\\gear\\human\\industrial\\crate_multi_single\\crate_multi_single" },
	{ OSC_OBJ, OSG_BLOC, "Tech Crate (large)", "objects\\gear\\human\\industrial\\crate_tech_giant\\crate_tech_giant" },
	{ OSC_OBJ, OSG_BLOC, "Tech Crate (small)", "objects\\gear\\human\\industrial\\crate_tech\\crate_tech" },
	{ OSC_OBJ, OSG_BLOC, "Packing Crate (large)", "objects\\gear\\human\\military\\crate_packing_giant\\crate_packing_giant" },
	{ OSC_OBJ, OSG_BLOC, "Packing Crate (small)", "objects\\gear\\human\\military\\crate_packing\\crate_packing" },
	{ OSC_OBJ, OSG_BLOC, "Shield Cover", "scenarios\\objects\\human\\military\\deployable_shield_corner_2\\deployable_shield_corner_2" },
	{ OSC_OBJ, OSG_BLOC, "Pallet", "objects\\gear\\human\\military\\pallet\\pallet" },
	{ OSC_OBJ, OSG_BLOC, "Fusion Coil", "objects\\gear\\human\\military\\fusion_coil\\fusion_coil" },
	{ OSC_OBJ, OSG_BLOC, "Covenant Battery", "objects\\gear\\covenant\\military\\battery\\battery" },
	{ OSC_OBJ, OSG_BLOC, "Covenant Crate", "objects\\gear\\covenant\\military\\crate_space\\crate_space" },
	{ OSC_OBJ, OSG_BLOC, "Gas Container (full)", "scenarios\\objects\\forerunner\\industrial\\gas_container_full\\gas_container_full" },
	{ OSC_OBJ, OSG_BLOC, "Gas Container (empty)", "scenarios\\objects\\forerunner\\industrial\\gas_container\\gas_container" },
	{ OSC_OBJ, OSG_BLOC, "Expelled Core (Containment)", "scenarios\\objects\\multi\\containment\\expelled_core\\expelled_core" },
	{ OSC_OBJ, OSG_BLOC, "Soccer Ball", "scenarios\\objects\\solo\\earthcity\\soccer_ball\\soccer_ball" },
	{ OSC_MISC, OSG_WEAP, "MP Bomb", "objects\\weapons\\multiplayer\\assault_bomb\\assault_bomb" },
	{ OSC_MISC, OSG_WEAP, "MP Ball", "objects\\weapons\\multiplayer\\ball\\ball" },
	{ OSC_MISC, OSG_WEAP, "MP Flag", "objects\\weapons\\multiplayer\\flag\\flag" },
};
#define OS_DEFS ((s32)(sizeof(os_defs) / sizeof(os_defs[0])))
#define OS_MENU 14                               /* g_menus: Object Spawning, then its 11 menus */
#define OS_MENU_BIPEDS 20
#define OS_MENU_LAST 25
#define OSG_SCEN 0x7363656Eu
#define OS_VAR_MAX 4                             /* 6i.177: the Warthog's other model variants, listed beside it */
#define OS_PER 16                                /* the most one heading lists (Objects: 15) */
static const u8 os_cat_menu[OSC_N] = { 15, 16, 17, 18, 19, 21, 22, 23, 24, 25 };
static const char *const os_cat_title[OSC_N] = { "Human Weapons  >", "Covenant Weapons  >", "Powerups  >", "Human Vehicles  >", "Covenant Vehicles  >", "Human  >", "Covenant  >", "Forerunner/Flood  >", "Objects  >", "Misc  >" };
#define OS_NONE { "Nothing from the list is in this level", K_BACK, 0, 0, 0 }
static s_item g_os_top[8] = { OS_NONE };         /* Object Spawning: the headings that have something (Bipeds one of them) */
static s32 o_os_ally = 0;                        /* 6i.170: Bipeds > Spawn as Ally - the dropped biped on your side */
static s32 o_os_enemy = 0;                       /* 6i.171: Bipeds > Spawn as Enemy - against you (the two exclude each other) */
static s32 o_os_ai = 1;                          /* 6i.162: Bipeds > Spawn with AI - a dropped biped is given its brain (6i.159); off: a body, as H2EK's drop */
static s_item g_os_bip[6] = { OS_NONE };         /* Bipeds: Spawn with AI, Spawn as Ally (6i.170), Spawn as Enemy (6i.171), then Human / Covenant / Forerunner-Flood */
static s_item g_os_items[OSC_N][OS_PER] = { { OS_NONE }, { OS_NONE }, { OS_NONE }, { OS_NONE }, { OS_NONE }, { OS_NONE }, { OS_NONE }, { OS_NONE }, { OS_NONE }, { OS_NONE } };
static s32 g_os_req;                             /* an item chosen: its os_defs index + 1 - spawned at the next tick */
typedef struct { const char *title; const s_item *items; s32 n; s32 parent; } s_menu;   /* parent: where B / Back goes (0: the top) */
#define NOF(a) ((s32)(sizeof(a) / sizeof(a[0])))
static const s_item m_timers[] = {
	{ "Trigger Volume Overlay", K_TOGGLE, &o_trig, 0, 1 },                 /* 6i.124; 6i.126: at the top */
	{ "Trigger Volume Overlay Settings  >", K_SUB, 0, 12, 0 },
	{ "Out of Bounds Timer", K_TOGGLE, &o_oob, 0, 1 },
	{ "Sleep Timer", K_TOGGLE, &o_sleep, 0, 1 },
	{ "Havok Island Viewer", K_TOGGLE, &o_hiv, 0, 1 },                    /* 6i.178 */
	{ "Show Stored Velocity", K_TOGGLE, &o_stored_vel, 0, 1 },           /* 6i.120 */
	{ "Crouch Turbo", K_TOGGLE, &o_crouch_turbo, 0, 1 },                 /* 6i.122: Steam's Hold to Repeat for crouch, ~10 ms a press */
	{ "Wireframe", K_INT, &o_wire, 0, 2 },                               /* 6i.52: off / on / xray; 6i.120: from Cheats */
	{ "Campaign Timer", K_TOGGLE, &o_ctimer, 0, 1 },                     /* 6i.156: starts, splits and stops by itself; 6i.170: below Wireframe */
	{ "Object Spawning  >", K_SUB, 0, OS_MENU, 0 },                      /* 6i.158: H2EK's Drop Objects */
};
static const s_item m_trig[] = {                /* 6i.124: Other Tools > Trigger Volume Overlay Settings */
	{ "Trigger names", K_TOGGLE, &o_trig_names, 0, 1 },
	{ "Trigger timer bars", K_TOGGLE, &o_trig_bars, 0, 1 },
	{ "Flash volumes on trigger", K_TOGGLE, &o_trig_flash, 0, 1 },
	{ "Highlight volumes when activated", K_TOGGLE, &o_trig_hl, 0, 1 },
};
static const s_item m_tb_confirm[] = {        /* 6i.135: Performance Mode Settings > Toggle Triple Buffering (restarts) */
	{ "Yes", K_ACTION, 0, 14, 0 },                                          /* the practice action 14: saved, then the game restarted */
	{ "No", K_BACK, 0, 0, 0 },
};
/* 6i.201: Larger Caches asks before it changes while a level is loaded (your note: as Toggle Triple Buffering does) -
 * the setting only takes effect at a level load, so the question offers to restart the level there and then. */
static const s_item m_lc_confirm[] = {
	{ "Yes", K_ACTION, 0, 15, 0 },                                         /* the practice action 15: changed, saved, the level restarted */
	{ "No", K_BACK, 0, 0, 0 },
};
#define LC_MENU 26                               /* g_menus: the last one */
static s_menu g_menus[] = {                     /* (6i.158: not const - the Object Spawning menus' lists are made per level) */
	{ "Spooky Ghost Menu - by Fortune090", m_main, NOF(m_main), 0 },
	{ "SPOOKY GHOST  Launch / Teleport", m_launch, NOF(m_launch), 0 },
	{ "SPOOKY GHOST  Superbounce Viewer Settings", m_sb, NOF(m_sb), 0 },
	{ "SPOOKY GHOST  Camera Settings", m_tp, NOF(m_tp), 0 },
	{ "SPOOKY GHOST  Cheats", m_cheats, NOF(m_cheats), 0 },
	{ "SPOOKY GHOST  Ghost Viewer Settings", m_ghost, NOF(m_ghost), 0 },
	{ "SPOOKY GHOST  Other Tools", m_timers, NOF(m_timers), 0 },
	{ "SPOOKY GHOST  Third Person Settings", m_tp3, NOF(m_tp3), 3 },   /* within Camera Settings */
	{ "SPOOKY GHOST  Skulls", m_skulls, NOF(m_skulls), 4 },             /* within Cheats */
	{ "SPOOKY GHOST  Checkpoint Settings", m_cp, NOF(m_cp), 0 },         /* 6h.12 */
	{ "SPOOKY GHOST  Game Hacks", m_hacks, NOF(m_hacks), 0 },            /* 6i.40 (6i.42: 10 - the Tests menu gone) */
	{ "SPOOKY GHOST  Performance Mode Settings", m_perf, NOF(m_perf), 10 },   /* 6i.40: within Game Hacks */
	{ "SPOOKY GHOST  Trigger Volume Overlay Settings", m_trig, NOF(m_trig), 6 },   /* 6i.124: within Other Tools */
	{ "This will force the game to restart, are you sure?", m_tb_confirm, NOF(m_tb_confirm), 10 },   /* 6i.135: within Performance Mode Settings */
	{ "SPOOKY GHOST  Object Spawning", g_os_top, 1, 6 },                  /* 14 (OS_MENU) - 6i.158: within Other Tools; the lists and their lengths: os_menus_fill */
	{ "SPOOKY GHOST  Human Weapons", g_os_items[OSC_HW], 1, OS_MENU },
	{ "SPOOKY GHOST  Covenant Weapons", g_os_items[OSC_CW], 1, OS_MENU },
	{ "SPOOKY GHOST  Powerups", g_os_items[OSC_PU], 1, OS_MENU },
	{ "SPOOKY GHOST  Human Vehicles", g_os_items[OSC_HV], 1, OS_MENU },
	{ "SPOOKY GHOST  Covenant Vehicles", g_os_items[OSC_CV], 1, OS_MENU },
	{ "SPOOKY GHOST  Bipeds", g_os_bip, 1, OS_MENU },                     /* 20 (OS_MENU_BIPEDS) */
	{ "SPOOKY GHOST  Bipeds: Human", g_os_items[OSC_BH], 1, OS_MENU_BIPEDS },
	{ "SPOOKY GHOST  Bipeds: Covenant", g_os_items[OSC_BC], 1, OS_MENU_BIPEDS },
	{ "SPOOKY GHOST  Bipeds: Forerunner/Flood", g_os_items[OSC_BF], 1, OS_MENU_BIPEDS },
	{ "SPOOKY GHOST  Objects", g_os_items[OSC_OBJ], 1, OS_MENU },
	{ "SPOOKY GHOST  Misc", g_os_items[OSC_MISC], 1, OS_MENU },           /* 25 (OS_MENU_LAST) */
	{ "This will restart the level, are you sure?", m_lc_confirm, NOF(m_lc_confirm), 10 },   /* 26 (LC_MENU) - 6i.201: within Game Hacks */
};
/* each menu's selection and first row shown - one per menu in g_menus (6a.1 had 6: the Timers menu, the 7th, used g_sel[6]
 * = g_first[0] (the main menu's scroll) and g_first[6] past the end - down there moved other menus' state; then a crash) */
static s32 g_menu = 0, g_sel[NOF(g_menus)], g_first[NOF(g_menus)];
static s32 g_rows_cap;                          /* tests: fewer rows than the screen fits (0 = the screen decides) */   /* the menu showing; each one's selection and first row shown */
#define ITEM_COUNT (g_menus[g_menu].n)
#define g_selected (g_sel[g_menu])

static s32 g_open = 0;            /* the menu is showing */
static s32 reconnect_dialog_up(void);   /* 6i.160 */
static s32 g_dlg = 0;             /* 6i.160: the game's Reconnect Controller dialog is up (read each frame while the menu is open): the menu not drawn over it */
static s32 g_peek = 0;            /* 6i.159: White held with the menu open - the menu hidden and the pad the game's while it is held; let go, the menu is back where it was */
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

static s32 ui_window_in_use(void);               /* 6i.170 */
typedef struct { void *handle; u16 buttons; u8 a, b; u8 combo; u8 repeat_dir; u16 held; u8 black, white; u8 sp_dir; u16 sp_held; u8 back_hold, back_combo, back_tap; } s_pad;   /* sp_: White + Left / Right held */
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
static s32 settings_save(void);                   /* 6i.152: spooky_settings.txt */
static s32 g_settings_loaded;
static void set_open(s32 open, const char *why)
{
	g_open = open; g_peek = 0;
	if (open) g_menu = 0;                                                    /* always opens at the top */
	if (!open) { g_suppress = 1; g_marker_set = 0; if (g_settings_loaded) settings_save(); }   /* 6i.152: the settings kept (once the start-up read has happened) */
	s32 at = lput(0, open ? "menu opened (" : "menu closed (");
	at = lput(at, why); at = lput(at, ", HUD-text frame "); at = lnum(at, g_count_hud); lput(at, ")");
	log_write();
}

static void toast(const char *t);
/* 6i.120: Performance Mode Settings > Reset to defaults - every setting in that menu back to what a new build starts
 * with (Performance Mode itself, in Game Hacks, is left as it is) */
static void perf_defaults(void)
{
	o_fps_show = 0; o_vsync = 1; o_tear = 1; o_fps_cap = 0; o_aniso = 1; o_rscale = 10; o_rs_adapt = 0; o_particles = 0; o_lightf = 2;
	o_world_mip = 0; o_lights = 0; o_shadow_on = 1; o_pass12 = 1; o_coop_passes = 0; o_coop_lights = 0; o_coop_detail = 0;
	o_light_skip = 1;
	settings_save();
	toast("Performance Mode Settings: back to defaults");
}
static s32 speed_locked(void);                    /* 6i.169 */
static void change(s32 delta, s32 activate)
{
	const s_item *it = &g_menus[g_menu].items[g_selected];
	if (it->kind == K_SUB) { if (activate) { g_menu = it->lo; if (it->lo == 13) g_sel[13] = 1; } return; }   /* 6i.135: the restart question opens on No */
	if (it->kind == K_BACK) { if (activate) g_menu = g_menus[g_menu].parent; return; }   /* up one level */
	if (it->kind == K_STAY && it->lo >= 100) { if (activate) g_os_req = it->lo - 100 + 1; return; }   /* (6i.177: 2000 + n a Warthog variant - told apart in os_run) */   /* 6i.158: Object Spawning - at the next tick, the menu left open */
	if (it->kind == K_STAY && it->lo == 20) { if (activate) perf_defaults(); return; }   /* 6i.120: at once, the menu left open */
	/* 6i.202: your note - on the game's main menu an action that needs a level (checkpoints, Launch, Teleport, Passion
	 * Check, Save / Revert Offsets, the cameras ...) says "Only in a level" and the menu STAYS where it is; it had closed
	 * the menu first and said so afterwards. The restart (14) is the one action that runs there. */
	if ((it->kind == K_STAY || (it->kind == K_ACTION && it->lo && it->lo != 14)) && on_main_menu()) { if (activate) toast("Only in a level"); return; }
	if (it->kind == K_STAY) { if (activate) g_req = it->lo; return; }   /* done in the next frame, the menu left open */
	if (it->kind == K_ACTION)
	{
		if (!activate) return;
		if (it->lo) { g_req = it->lo; set_open(0, "practice action"); }       /* done in the next frame, the menu closed */
		else set_open(0, "Close menu item");
		return;
	}
	if (it->kind == K_TOGGLE)
	{
		if (it->value == &o_big_cache && !big_cache_can()) { o_big_cache = 0; toast("Larger Caches: needs a 128 MB console"); return; }   /* 6i.192: your note - not to be turned on where it can do nothing */
		if (it->value == &o_big_cache && lc_level_loaded()) { if (activate) { g_menu = LC_MENU; g_sel[LC_MENU] = 1; } return; }   /* 6i.201: in a level it asks first (opens on No); on the game's main menu it just changes */
		*it->value = !*it->value;
		if (it->value == &o_os_ally && o_os_ally) o_os_enemy = 0;           /* 6i.171: one or the other */
		if (it->value == &o_os_enemy && o_os_enemy) o_os_ally = 0;
		return;
	}
	if (it->value == &o_speed && speed_locked()) { toast("Game Speed: 1.0 while the Campaign Timer is on"); return; }   /* 6i.169 */
	if (activate) delta = 1;
	if (it->kind == K_INT10) delta *= 10;
	*it->value += delta;
	if (*it->value > it->hi) *it->value = activate ? it->lo : it->hi;     /* A wraps round, right stops at the top */
	if (*it->value < it->lo) *it->value = it->lo;
}

/* 6h.27: White + D-pad Left / Right (menu closed) - Game Speed down / up 0.1, 0.0 to 5.0 (stops at the ends); held:
 * again after ~0.4 s, then every ~0.1 s (as the menu's left / right). Said on screen - the menu isn't open. */
static void toast(const char *t);
/* 6i: a held D-pad's repeat - again after 12 steps, then every 3 - counted in 30 Hz steps (a frame each, as ever, with
 * Unlock FPS off; with it on, the vblanks' steps: the same ~0.4 s and ~0.1 s at 60) */
static s32 held_steps(u16 *held)
{
	s32 k, fire = 0;
	for (k = 0; k < g_steps; k++) { (*held)++; if (*held >= 12 && (*held - 12) % 3 == 0) fire = 1; }
	return fire;
}
/* 6i.169: with the Campaign Timer on, Game Speed cannot be changed in a campaign game (no time is recorded at any speed
 * but 1.0) and goes back to 1.0 when a campaign mission starts (ct_tick). Multiplayer, where the timer does not run, is
 * not touched. */
static s32 is_campaign(void);
static s32 speed_locked(void) { return o_ctimer && is_campaign(); }
static void speed_keys(s_pad *pad, u16 buttons, u16 pressed, s32 on)
{
	u8 d = on ? ((buttons & BTN_LEFT) ? 3 : (buttons & BTN_RIGHT) ? 4 : 0) : 0;
	s32 fire = 0;
	char t[24] = "Game Speed: ";
	if (d && (pressed & (BTN_LEFT | BTN_RIGHT))) { fire = 1; pad->sp_held = 0; pad->sp_dir = d; }
	else if (d && d == pad->sp_dir) { if (held_steps(&pad->sp_held)) fire = 1; }
	else { pad->sp_held = 0; pad->sp_dir = d; }
	if (!fire) return;
	if (speed_locked()) { toast("Game Speed: 1.0 while the Campaign Timer is on"); return; }   /* 6i.169 */
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
static s32 game_paused(void);
static s32 g_fc_startlog;
static void fc_start_log(void);
static void toast(const char *t);
static void fc_zoom_say(void)                  /* 6i.122: "Zoom: 6x" / "Zoom: off (6x)" */
{
	char t[24] = "Zoom: ";
	s32 n = 6, v = g_fc_zlv;
	if (!g_fc_zoom) { t[n++] = 'o'; t[n++] = 'f'; t[n++] = 'f'; t[n++] = ' '; t[n++] = '('; }
	if (v >= 10) t[n++] = (char)('0' + v / 10);
	t[n++] = (char)('0' + v % 10); t[n++] = 'x';
	if (!g_fc_zoom) t[n++] = ')';
	t[n] = 0;
	toast(t);
}
/* 6i.122: Crouch Turbo (Other Tools) - Steam's "Hold to Repeat (Turbo)" for crouch. 6i.123 (your test: not fast enough):
 * 6i.122 flipped it by the console's clock (~5.7 ms each way), but the game reads the pad once a frame, and at 30 a frame is
 * 33 ms - the clock's phase drifted only ~1 ms a frame, so the game saw six frames pressed, six released. Now it follows
 * the game's own ticks: while the button your layout crouches with is held, the game sees it pressed for one tick and
 * released for the next, over and over - the fastest repeat the game can act on (15 presses a second at 30 ticks), at 30
 * or 60 frames a second alike. Crouch's button is the port's own: logical button 10 of its gamepad preferences' button
 * map (PAD_PREFS + 8; the left stick in every layout the game offers). Physical 0 - 7 are the analog buttons (A B X Y
 * Black White LT RT), 8 - 15 the digital ones. */
static void crouch_turbo_apply(xinput_state *st, s32 phys, u32 clock)
{
	if (!(clock & 1u)) return;                                                  /* 6i.123: even ticks as you hold it, odd ticks released */
	if (phys >= 8 && phys < 16) st->buttons &= (u16)~(1u << (phys - 8));
	else if (phys >= 0 && phys < 8) st->analog[phys] = 0;
}
static void crouch_turbo(void *handle, xinput_state *st);
#ifdef HOST
u16 host_turbo(u16 buttons, u8 *analog, s32 phys, u32 clock)
{
	xinput_state s;
	s32 i;
	memset(&s, 0, sizeof(s));
	s.buttons = buttons; for (i = 0; i < 8; i++) s.analog[i] = analog[i];
	crouch_turbo_apply(&s, phys, clock);
	for (i = 0; i < 8; i++) analog[i] = s.analog[i];
	return s.buttons;
}
#endif
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
	/* 6i.198: YOUR NOTE - the mod's menu on the game's main menu again, with Back treated there as it is with the game's
	 * pause menu up (held back from the game while it is down; a tap of it alone still reaches the game, on release), and
	 * drawn over every screen of the game's (it went behind the lobby screens before). So Back + D-pad Up opens it there
	 * too. What kept it off (6h, 6i.37) and how each is met now:
	 *  - a game ending with the menu open left it open there, the game's menus dead: it is simply open now - B closes it;
	 *  - after a close the pad was held back until EVERYTHING read released, and a controller whose X / Y rest a little
	 *    above 0 never did (your soft lock): X and Y count as released under the same threshold as the other buttons;
	 *  - the practice shortcuts (checkpoints, launch, the cameras) stay off there, and a menu action that needs a level
	 *    says so instead of running (the frame's side: practice actions on the main menu). */
	s32 main_menu = on_main_menu();
	if (!g_open && !main_menu)
	{
		/* Cartographer's practice shortcuts: Back + D-pad Left / Right = create / revert checkpoint; Black + D-pad
		 * Up / Down = launch / teleport (Back + Up opens this menu, so not Cartographer's Back + D-pad Up / Down) */
		s32 r = 0;
		u8 black = st->analog[AB_BLACK] > ANALOG_DOWN;
		u8 white = st->analog[AB_WHITE] > ANALOG_DOWN;
		if ((buttons & BTN_BACK) && black && !((pad->buttons & BTN_BACK) && pad->black)) r = 6;   /* the free camera, on / off; 6i.165: in either order (both held, and not both before) - once a press */
		else if ((buttons & BTN_BACK) && white && !pad->white) r = 7;        /* teleport to the free camera */
		else if ((buttons & BTN_BACK) && (pressed & BTN_LEFT)) r = 1;
		else if ((buttons & BTN_BACK) && (pressed & BTN_RIGHT)) r = 2;
		else if ((buttons & BTN_BACK) && (pressed & BTN_DOWN)) r = 5;         /* the third person camera, on / off */
		else if (black && (pressed & BTN_UP)) r = 3;
		else if (black && (pressed & BTN_DOWN)) r = 4;
		else if (black && o_ghost && (pressed & BTN_LEFT)) r = 10;          /* save offsets (6h; 6h.27: Black - was White) */
		else if (black && o_ghost && (pressed & BTN_RIGHT)) r = 11;         /* revert offsets */
		else if (SPOOKY_LOG && SPOOKY_CAPTURE_KEY && white && !(buttons & BTN_BACK) && (pressed & BTN_UP)) capture_ab_start();   /* 6i.78: an A / B pair */   /* 6i.19: White + Up - a capture (6i.36: only with logging - it reports there) */
		else if (white && !(buttons & BTN_BACK) && (pressed & BTN_DOWN))    /* 6h.27: White + Down - Pause Game */
		{
			o_pause = !o_pause;
			toast(o_pause ? "Pause Game: on" : "Pause Game: off");
		}
		if (r) g_req = r;
		speed_keys(pad, buttons, pressed, white && !black && !(buttons & BTN_BACK));   /* 6h.27: White + Left / Right */
	}
	/* 6i.163: Black + White (either pressed while the other is held), in free camera mode only - "Disable freecam player
	 * input" on / off: the pad flies the camera, or moves you. Not with Back (Back + Black / Back + White are the free
	 * camera's own shortcuts), not in first or third person. While both are held with the free camera on, neither
	 * reaches the game (its grenade switch and flashlight). */
	/* 6i.164: your 6i.163 test - held, it toggled back and forth: the buttons were taken off the pad for the game before
	 * the pad's last state was noted, so they never read as held and every read was a new press. The state noted is
	 * now the buttons as they really are: once a press, however long both are held. */
	u8 bk = st->analog[AB_BLACK] > ANALOG_DOWN, wh = st->analog[AB_WHITE] > ANALOG_DOWN;
	{
		if (o_free_cam && !main_menu && bk && wh && !(buttons & BTN_BACK))
		{
			if (!(pad->black && pad->white))
			{
				o_fc_block = !o_fc_block;
				toast(o_fc_block ? "Disable freecam player input: on" : "Disable freecam player input: off");
			}
			st->analog[AB_BLACK] = 0;
			if (!g_open) st->analog[AB_WHITE] = 0;                          /* (menu open: White is still what holds the menu aside, below - and is taken off there) */
		}
	}
	/* 6i.170: Back first, then D-pad Up, with the game's pause menu up - the pause menu took Back as its own "back" and
	 * closed before the combination was complete. While a window of the game's is in use (not on its main menu), Back
	 * is held back from the game for as long as it is down; let go with nothing else pressed meanwhile, the game is
	 * given a short press of it then (so Back alone still backs out of the game's menus, on release); used in a
	 * combination, the game never sees it. */
	u8 back_hide = 0;
	if (buttons & BTN_BACK)
	{
		if (ui_window_in_use() || main_menu || pad->back_hold)                /* 6i.198: on the game's main menu too */
		{
			pad->back_hold = 1; back_hide = 1;
			if ((buttons & (u16)~BTN_BACK) || bk || wh || a || b || g_open) pad->back_combo = 1;
		}
	}
	else
	{
		if (pad->back_hold && !pad->back_combo && !g_open && (ui_window_in_use() || main_menu)) pad->back_tap = 2;
		pad->back_hold = 0; pad->back_combo = 0;
	}
	u8 combo = (buttons & BTN_BACK) && (buttons & BTN_UP);
	u8 combo_edge = combo && !pad->combo;                                  /* 6i.198: on the game's main menu too */
	u8 a_edge = a && !pad->a, b_edge = b && !pad->b;
	pad->buttons = buttons; pad->a = a; pad->b = b; pad->combo = combo; pad->black = bk; pad->white = wh;

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
		/* 6i.159: White held - the menu steps aside: not drawn, and this pad goes to the game as it is, but for White
		 * itself (the game's flashlight: not switched by this). Nothing of the menu moves; let go, it is back where it
		 * was (a button still held then is not a new press: the pad's last state is kept up to date above) */
		if (st->analog[AB_WHITE] > ANALOG_DOWN)
		{
			g_peek = 1; pad->held = 0; pad->repeat_dir = 0;
			st->analog[AB_WHITE] = 0;
			g_suppress = 0;
			goto pad_out_of_menu;                                            /* 6i.163: as with the menu closed - the free camera's pad if it is flying, else the game's */
		}
		if (reconnect_dialog_up())                                           /* 6i.160: the game's Reconnect Controller dialog - the pad is the game's (its "A to continue"), the menu out of its way, back when it is gone */
		{
			g_peek = 1; pad->held = 0; pad->repeat_dir = 0;
			return result;
		}
		g_peek = 0;
		/* D-pad with Blam-style auto-repeat: once on the press, then after ~0.4 s every ~0.1 s */
		u8 dir = (buttons & BTN_UP) ? 1 : (buttons & BTN_DOWN) ? 2 : (buttons & BTN_LEFT) ? 3 : (buttons & BTN_RIGHT) ? 4 : 0;
		s32 fire = 0;
		if (!(buttons & BTN_BACK))
		{
			if (dir && (pressed & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT))) { fire = 1; pad->held = 0; pad->repeat_dir = dir; }
			else if (dir && dir == pad->repeat_dir) { if (held_steps(&pad->held)) fire = 1; }
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

pad_out_of_menu:
	if (back_hide) st->buttons &= (u16)~BTN_BACK;                          /* 6i.170 (above) */
	else if (pad->back_tap) { st->buttons |= BTN_BACK; pad->back_tap--; }
	if (g_suppress && handle == g_owner)
	{
		if (!buttons && !a && !b && st->analog[2] < ANALOG_DOWN && st->analog[3] < ANALOG_DOWN && st->analog[6] < ANALOG_DOWN && st->analog[7] < ANALOG_DOWN) g_suppress = 0;   /* 6i.198: X / Y by the threshold too (a pad whose X / Y rest above 0 never read released) */
		else neutral(st);
	}
	else if (o_free_cam && o_fc_block && (!g_open || g_peek) && !main_menu && (!g_fc_pad || handle == g_fc_pad))   /* 6i.37: never on the main menu */
	{
		/* the free camera flies on this pad: its sticks, triggers and D-pad - the game sees a neutral pad (Start passes:
		 * you can still pause) */
		u16 start = (u16)(buttons & 0x0010);
		g_fc_pad = handle;
		if (g_fc_startlog > 0 && SPOOKY_LOG) fc_start_log();                 /* 6i.122: after Start, three reads logged: what the pause test saw */
		if (game_paused())                                                  /* 6i.120: the game's pause menu is up - the pad is the game's, the camera holds still */
		{
			s32 q;
			for (q = 0; q < 6; q++) g_fc_in[q] = 0.f;
			return result;
		}
		if (pressed & 0x0010) g_fc_startlog = 3;
		g_fc_in[0] = fc_stick(st->lx); g_fc_in[1] = fc_stick(st->ly);       /* the player controls' own dead zone */
		g_fc_in[2] = fc_stick(st->rx); g_fc_in[3] = fc_stick(st->ry);
		g_fc_in[4] = fc_trigger(st->analog[7]); g_fc_in[5] = fc_trigger(st->analog[6]);   /* right / left trigger */
		if (!(buttons & BTN_BACK) && st->analog[AB_BLACK] <= ANALOG_DOWN && st->analog[AB_WHITE] <= ANALOG_DOWN)   /* 6h.27: not with Black / White */
		{
			if (pressed & BTN_UP) g_fc_speed_steps++;
			if (pressed & BTN_DOWN) g_fc_speed_steps--;
			if (pressed & 0x0080) { g_fc_zoom = g_fc_zoom ? 0 : g_fc_zlv; fc_zoom_say(); }   /* 6i.122: the right stick's click - zoom in / out */
			{
				/* 6i.122: D-pad Right / Left - the level, 2x to 20x; held, again after 0.2 s, then 15 a second */
				u8 d = (buttons & BTN_LEFT) ? 3 : (buttons & BTN_RIGHT) ? 4 : 0;
				s32 fire = 0, k;
				if (d && (pressed & (BTN_LEFT | BTN_RIGHT))) { fire = 1; g_fc_zheld = 0; g_fc_zdir = d; }
				else if (d && d == g_fc_zdir) { for (k = 0; k < g_steps; k++) { g_fc_zheld++; if (g_fc_zheld >= 6 && !((g_fc_zheld - 6) & 1)) fire = 1; } }
				else { g_fc_zheld = 0; g_fc_zdir = d; }
				if (fire)
				{
					g_fc_zlv += d == 3 ? -1 : 1;
					if (g_fc_zlv < 2) g_fc_zlv = 2;
					if (g_fc_zlv > 20) g_fc_zlv = 20;
					if (g_fc_zoom) g_fc_zoom = g_fc_zlv;
					fc_zoom_say();
				}
			}
		}
		neutral(st);
		st->buttons = start;
	}
	else if (o_crouch_turbo && !main_menu && !g_open && !g_suppress) crouch_turbo(handle, st);   /* 6i.122 */
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
/* 6i.171: on a console with 128 MB (the 360's emulator since 6i.150) the game's own memory reaches up to 0x88000000 -
 * its render targets sit at 123 to 126 MB there. A pointer above 0x84000000 was refused as not a pointer, so anything
 * of the game's that lands in the upper half read as "not there" (Field of View on the 360 is the candidate: its
 * camera record is refused before anything else is done). The limit follows the memory the console reports. */
static u32 g_ptr_top = 0x84000000u;
static s32 ptr_ok(u32 p) { return p >= 0x10000 && p < g_ptr_top && !(p & 3); }
/* 6i.120: the game's own pause (its pause menu sets the time globals' +1: c_pause_game_screen, v1.0 0x23203e).
 * 6i.122: your 6i.121 test - the pause menu still got a dead pad and the camera still flew - so the flag alone isn't
 * enough; now also the game's own "any window in use" (window_manager_any_window_in_use, v1.0 0x149227 - what the game
 * itself asks before it resumes time): a pause menu, a dialog, any screen of the game's own UI */
#ifdef HOST
static s32 g_host_ui_window;
void host_set_ui_window(s32 on) { g_host_ui_window = on; }
static s32 ui_window_in_use(void) { return g_host_ui_window; }
#else
static s32 ui_window_in_use(void) { return ADDR_UI_WINDOW_IN_USE ? ((u8 (*)(void))ADDR_UI_WINDOW_IN_USE)() != 0 : 0; }
#endif
/* 6i.160: one of the game's "Reconnect Controller" dialogs is showing (the game's own test, v1.0 0x18f973: void, al) -
 * while it is, an open menu steps aside as it does for White: the dialog was drawn under the menu and its A never
 * reached the game until the menu was closed */
#ifdef HOST
static s32 g_host_reconnect;
void host_set_reconnect(s32 on) { g_host_reconnect = on; }
static s32 reconnect_dialog_up(void) { return g_host_reconnect; }
#else
u32 gfx_call0(u32 fn);
static s32 reconnect_dialog_up(void) { return ADDR_RECONNECT_DIALOG ? (gfx_call0(ADDR_RECONNECT_DIALOG) & 0xFFu) != 0 : 0; }
#endif
static s32 g_paused_why;                            /* 1 the time flag, 2 a window in use (the log's) */
static s32 game_paused(void)
{
	u32 t = M32(ADDR_GAME_TIME_PTR);
	g_paused_why = 0;
	if (ptr_ok(t) && M8(t) && M8(t + 1)) g_paused_why = 1;
	else if (ui_window_in_use()) g_paused_why = 2;
	return g_paused_why != 0;
}
#ifdef HOST
s32 host_game_paused(void) { return game_paused(); }
#endif
static s32 lput(s32 at, const char *s);
static s32 lnum(s32 at, u32 v);
static void log_write(void);
static u32 tsc_lo(void);
static void crouch_turbo(void *handle, xinput_state *st)
{
	s32 k, phys = 14;                                                           /* the left stick, unless the layout says */
	for (k = 0; k < 4; k++)
		if (M32(ADDR_PAD_HANDLES + 4 * (u32)k) == (u32)(unsigned long)handle)
		{
			s32 p = (s8)M8(ADDR_PAD_PREFS + (u32)k * 0x1C + 8 + 10);
			if (p >= 0 && p < 16) phys = p;
			break;
		}
	{
		u32 t = M32(ADDR_GAME_TIME_PTR);
		if (ptr_ok(t)) crouch_turbo_apply(st, phys, M32(t + 8));            /* 6i.123: the game's tick (the next one alternates) */
	}
}
static void fc_start_log(void)
{
	s32 at, gp = game_paused();
	u32 tm = M32(ADDR_GAME_TIME_PTR);
	g_fc_startlog--;
	at = lput(0, "free camera after Start: paused "); at = lnum(at, (u32)gp); at = lput(at, " (time flag ");
	at = lnum(at, ptr_ok(tm) ? (u32)M8(tm + 1) : 99u); at = lput(at, ", window in use "); at = lnum(at, (u32)ui_window_in_use()); lput(at, ")");
	log_write();
}
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
static void toast_step(void) { g_toast_frames -= g_steps; if (g_toast_frames < 0) g_toast_frames = 0; }   /* 6i: 30 Hz steps */
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
static s32 rate_put(u32 time, s32 tps, float speed)                      /* 6i.121: returns 1 when it changed the tick rate */
{
	u32 w = M32(time);
	s32 changed = (s32)(s16)(w >> 16) != tps;
	if (changed) { gw32(time, (w & 0xFFFFu) | ((u32)(u16)tps << 16)); gwf(time + 4, 1.f / (float)tps); }
	if (MF(time + 0xC) != speed) gwf(time + 0xC, speed);
	return changed;
}
/* 6i.121: a tick-rate change rebuilds the objects' collision, as HCM's Master Tickrate does on MCC. The Havok shape
 * builder (function_1d7400) bakes the live tick rate into each shape when it's built: (extent x size x 0.5 + g_47f05c)
 * x the time globals' ticks a second - MCC's "x 30 collision scalar" - and the shapes are kept, so a change reaches only
 * objects made after it. The game's own per-object rebuild, function_1c3850 (ecx: the object; through function_1c3770:
 * out of Havok's world, torn down, built again by function_1cf120 at the current rate, back in, velocities kept), is
 * called for every object with a Havok component - once, at the start of the next frame, before any tick. */
static s32 g_rebuild_due, g_trate_said = 1;
static u32 g_rb_objects, g_rb_runs;
#ifdef HOST
static void havok_rebuild_call(u32 datum) { (void)datum; }
u32 host_rebuilt(void) { return g_rb_runs; }
u32 host_rebuilt_objects(void) { return g_rb_objects; }
#else
static void havok_rebuild_call(u32 datum) { ((void (__attribute__((fastcall)) *)(u32))ADDR_HAVOK_REBUILD)(datum); }
#endif
static void collision_rebuild(void)
{
	u32 pool = M32(ADDR_OBJECTS_PTR), data, count, i;
	g_rb_runs++; g_rb_objects = 0;
	if (!ADDR_HAVOK_REBUILD || !ptr_ok(pool)) return;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || !count || count > 4096) return;
	for (i = 0; i < count; i++)
	{
		u32 h = data + i * 12, salt = M32(h) & 0xffffu, obj;
		if (!salt) continue;
		obj = M32(h + 8);
		if (!ptr_ok(obj) || M32(obj + 0xB4) == 0xFFFFFFFFu) continue;
		havok_rebuild_call((salt << 16) | i);
		g_rb_objects++;
	}
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
	if (o_trate == 0 || o_trate == 2) base = g_tick_rates[o_trate];      /* 6i.120: Tick Rate - yours, in a game (the main menu: the game's, above) */
	if (MF(time + 0x18) > 0.f && !o_pause) { g_speed_set = o_trate != 1; return; }  /* a script's rate blend under way: its own */
	if (o_pause || o_speed != 10)
	{
		if (rate_put(time, base, o_pause ? 0.f : (float)o_speed / 10.f)) g_rebuild_due = 1;   /* the rate (30, or Tick Rate's) - 6i.121: changed: collision rebuilt */
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
		if (!script && rate_put(time, base, 1.f)) g_rebuild_due = 1;      /* 6i.121: changed: collision rebuilt */
	}
	g_speed_set = o_trate != 1;                                           /* 6i.120: ours (Tick Rate) - put back on the main menu */
}
/* ---- 6i: Unlock FPS - the frame clock, and the game's pacing byte (see o_unlock_fps). Each frame, first, before the
 * game's own frame (its pacing reads spooky_pace_clamp later in this same frame). On: the vblanks since the last frame
 * (the game's count, v1.0 0x485ab0 / v1.5 0x4922d0 - the vblank interrupt adds 1), so this frame's seconds by the
 * refresh rate (v1.0 0x485ac0 / v1.5 0x4922e0: 60; 50 on a PAL 50 Hz picture), and its 30 Hz steps - the remainder
 * carried, so at 60 a step every other frame. Off: a step and 1/30 s a frame, as the mod always counted. The game's
 * raise to 2 vblanks is skipped only in a game: its main menu keeps its own pacing. ---- */
static s32 fps_unlocked(void) { u32 g = M32(ADDR_GAME_OPTIONS_PTR); return o_unlock_fps && ptr_ok(g) && M32(g + 8) != 3; }
/* ---- 6i.209: CUTSCENES PLAY AT 30 FPS WITH PERFORMANCE MODE ON (your decision, 11 Nov) ----
 * Above 30 fps the jaws and fingers of cutscene characters stutter. Eleven builds (6i.198 - 6i.208) did not cure it.
 * What was established: on the first frame drawn after each tick the game rewrites the same seventeen parts of such a
 * character while it draws, and on the tick's later frames it does not. What was never established: where it puts them
 * against a body that is moving; five ways of handling those parts changed nothing you could see. That work is out of
 * the mod (it is kept, with what it found, in the package's "retired" folder).
 * The compromise: while a cutscene is playing (the game's own flag - the cinematic globals' +5, the one the free camera
 * and the Campaign Timer already read) the mod behaves as if VSync were set to "30 fps": the game's own pacing, two
 * refreshes a frame, one frame a tick - exactly what you found cures it. The setting itself is not changed; when the
 * cutscene ends, whatever VSync and FPS Limit you chose apply again. With VSync off there is one moment's pause at each
 * end of a cutscene, as when that setting is changed by hand (the game re-applies its display mode). ---- */
static s32 fps_unlocked(void);
static s32 g_cut30;                               /* a cutscene is being held to 30 (the log's, the tests') */
static s32 cutscene_playing(void)
{
	u32 cg = ADDR_ASPECT_FLAGS_PTR ? M32(ADDR_ASPECT_FLAGS_PTR) : 0;
	return ptr_ok(cg) && M8(cg + 5);
}
static s32 vsync_now(void)                        /* the VSync setting in force: yours, or 30 fps (2) while a cutscene plays */
{
	s32 cut = o_vsync != 2 && fps_unlocked() && cutscene_playing();
	if (cut != g_cut30)
	{
		g_cut30 = cut;
		lput(0, cut ? "cutscene: held to 30 fps while it plays (Performance Mode)" : "cutscene over: your VSync setting again");
		log_write();
	}
	return cut ? 2 : o_vsync;
}
#ifdef HOST
s32 host_cut30(void) { return g_cut30; }
s32 host_vsync_now(void) { return vsync_now(); }
#endif
static void frame_time_update(void)
{
	u32 now = M32(ADDR_VBLANK_COUNT), hz = (u32)(s32)(s16)M16(ADDR_REFRESH_HZ), v;
	/* 6i.176: VSync "30 fps" - Performance Mode with the game's own pacing left on: every frame waits for at least two
	 * vblanks (the game's "if N <= 2, N = 2", read from spooky_pace_clamp - see o_unlock_fps), so a level that cannot
	 * hold 60 runs at a steady 30 instead of moving between 60 and 30. Everything else of Performance Mode stays as it
	 * is; the frame clock below already counts vblanks, so two a frame is one 30 Hz step and 1/30 s, as with it off. */
	spooky_pace_clamp = fps_unlocked() && vsync_now() != 2 ? 0 : 1;   /* 6i.209: a cutscene counts as "30 fps" */
	capture_frame();                                                      /* 6i.19 */
	{
		/* 6i.3: the display the game wants - its own constant, with "or at once if late" while Late frames at once is on
		 * (and Unlock FPS, in a game); the game re-applies it itself when that differs from what it has */
		u32 k, was = spooky_display[0], want = o_tear && fps_unlocked(), novs = !vsync_now() && fps_unlocked();   /* 6i.40: settings again */
		for (k = 0; k < 3; k++) spooky_display[k] = M32(ADDR_DISPLAY_CONFIG + 4 * k);
		if (want) spooky_display[0] |= 0x100u;                                 /* byte 1 (v1.0 0x485ac9 once applied) */
		if (novs) spooky_display[0] &= 0xFFFFu;                                /* 6i.31: VSync off - the frame interval 0 */
		if ((was ^ spooky_display[0]) & 0x100u)
		{
			lput(0, want ? "late frames: shown at once" : "late frames: held to the next vblank");
			log_write();
		}
	}
	{
		/* 6i.3: where the last frame's time went (the 10-second line) - see g_pf_* */
		u32 t = tsc_lo();
		if (g_pf_have == 3)
		{
			u32 d[PF_N], k, all, dr = g_pf_d1 - g_pf_d0, wp = g_pf_w + g_pf_p;
			d[PF_UPDATE] = (g_pf_d0 - g_pf_t0) / 733u; d[PF_DRAW] = (dr > wp ? dr - wp : 0u) / 733u;
			d[PF_WAIT] = g_pf_w / 733u; d[PF_PRESENT] = g_pf_p / 733u; d[PF_AFTER] = (t - g_pf_d1) / 733u; d[PF_MOD] = g_pf_mod / 733u;
			all = (t - g_pf_t0) / 733u;
			for (k = 0; k < PF_N; k++) { g_pf_sum[k] += d[k]; if (d[k] > g_pf_max[k]) g_pf_max[k] = d[k]; g_pf_last[k] = d[k]; }
			g_pf_last[PF_N] = all;
			if (all > 17000u) { g_pf_slow_n++; for (k = 0; k < PF_N; k++) g_pf_slow_sum[k] += d[k]; }
			rs_adapt_sample(all > d[PF_WAIT] ? all - d[PF_WAIT] : 0u);          /* 6i.130: the frame's work, less its vblank wait */
			g_pf_ip_sum += g_pf_ip / 733u;
			g_pf_n++;
		}
		g_pf_t0 = t; g_pf_have = 1; g_pf_mod = 0; g_pf_ip = 0; g_pf_w = 0; g_pf_p = 0;
		rs_eff_set();                                                         /* 6i.130: the scale this frame draws at */
	}
	v = now - g_vb_last; g_vb_last = now;
	{                                                                      /* 6i.42: the FPS Counter's figure */
		u32 r = (hz >= 24u && hz <= 240u) ? hz : 60u;
		g_fps_frames++; g_fps_vb += v < 600u ? v : 600u;
		{ s32 k; for (k = 0; k < 5; k++) { g_rs_show[k] = g_rsc[k]; g_rsc[k] = 0; } g_rs_frame++; }   /* 6i.64 */
		if (g_fps_vb >= r) { g_fps_shown = (g_fps_frames * r + g_fps_vb / 2u) / g_fps_vb; g_fps_frames = 0; g_fps_vb = 0; }
	}
	spooky_copy_keep = (u8)(fps_unlocked() && (o_water_refr >= 3 || o_water_rate == 2 || (o_water_rate == 1 && (g_rs_frame & 1))));
	/* 6i.75: your captures. Smoke (original Xbox): render pass 12 appears only in the smoke (2.5 ms a frame - one shader
	 * 1.9 ms over 6 draws) and the part after the water copy grows 3.7 ms - not the particle sprites the Particles
	 * setting worked on. The 360 at the water: the copy costs 0.1 ms and the water itself 1.7 - but passes 10 and 11
	 * (objects' dynamic shadows) 3.9 ms and pass 3 (lighting) 2.5 more than looking away. Object Shadows off skips 10 and
	 * 11; Smoke Pass (test) off skips 12, to see what it is and what it costs. Performance Mode only. */
	/* 6i.76: your test - Object Shadows off steadies the 360 at the water; you want them cheaper, not gone. Half rate:
	 * pass 10 (drawing the objects' shadows into their own texture) only every other frame; pass 11 (laying them onto
	 * the world) every frame from the last one drawn - the shadows stay, moving objects' shadows a frame behind. */
	o_shadows = o_shadow_on ? 0 : 2;                                          /* 6i.77: half rate (1) no longer offered */
	spooky_scr_point = (u8)((fps_unlocked() && nv2a_ok() && (o_lightf || !o_world_mip || o_light_skip)) || g_cap_nomip || g_cap_bump || g_cap_frames);   /* 6i.109: not on the 360 - see rt_on */
	spooky_rt_on = (u8)(g_cap_frames || o_gpu_time);                    /* 6i.109: the switch hooks call in only when measuring */
	spooky_pass_mask = fps_unlocked() ? ((o_shadows == 2 ? ((1u << 10) | (1u << 11)) : (o_shadows == 1 && (g_rs_frame & 1)) ? (1u << 10) : 0u)
	                                     | (o_pass12 ? 0u : (1u << 12))) : 0u;
	/* 6i.73: Water Refraction (Performance Mode Settings) - every frame / half rate / off. Your 360 test: the water still
	 * costs a lot there - its emulator pays for every switch into the copy's texture and back. Half rate makes the copy
	 * every other frame (the water's view behind it a frame old on the others - it may step slightly as you turn); off
	 * never makes it (the water shows what it last had). The original Xbox wants every frame. (6i.28 - 6i.58 also kept it
	                                                                          * every other frame: on those frames the water showed the frame before - the
	                                                                          * jitter you saw on it when turning. Now made every frame - cheap with fast.) */
	spooky_copy_keepz = (u8)(fps_unlocked() && o_water_refr == 1);        /* 6i.59: fast - the copy's target with the screen's depth */
	spooky_copy_late = (u8)(fps_unlocked() && o_water_refr == 2);         /* 6i.59: late - after the water plane */
	spooky_fx_keepz = (u8)(fps_unlocked() && o_fx_copy);                  /* 6i.60: Distortion Effects fast */
	spooky_water_hide = (u8)(fps_unlocked() && !o_water_surface);         /* 6i.58: Water Surface (test) off */
	spooky_copy_ran = 0; spooky_copy_one = (u8)fps_unlocked();
	if (!o_unlock_fps) { g_vb = 2; g_steps = 1; g_fdt = 1.f / 30.f; g_step_acc = 0; g_t30++; return; }
	if (!(hz >= 24 && hz <= 240)) hz = 60;
	if (v > 8) v = 8;                                                     /* the first frame on, a level load: 8 at most */
	if (v && fps_unlocked()) g_vb_hist[v >= 4 ? 3 : v - 1]++;
	g_vb = (s32)v;
	g_step_acc += v * 30u;
	g_steps = (s32)(g_step_acc / hz); g_step_acc %= hz;
	g_fdt = (float)v / (float)hz;
	g_t30 += (u32)g_steps;
}
/* an easing step set for a 30 Hz frame (f of the way there), as this frame's - the same per second at any frame rate:
 * 1 - (1 - f)^(vblanks / 2), exact at 60 Hz. Unlock FPS off: f, as ever. */
static float ease_k(float f)
{
	float q, r = 1.f;
	s32 v;
	if (!o_unlock_fps) return f;
	q = fsqrt(1.f - f);
	for (v = 0; v < g_vb; v++) r *= q;
	return 1.f - r;
}
#ifdef HOST
void host_set_unlock_fps(s32 on) { o_unlock_fps = on; }
s32 host_unlock_fps(void) { return o_unlock_fps; }
s32 host_pace_clamp(void) { return spooky_pace_clamp; }
u32 host_pf_last(s32 k) { return g_pf_last[k]; }   /* 6i.3: the last frame's parts (us), [PF_N] its whole */
void host_pass(s32 k, u32 cyc, s32 skip) { spooky_pass_cyc[k] += cyc; spooky_skip_pass = skip; }   /* 6i.12: what pass_hook would time */
void host_set_gpu_time(s32 on) { o_gpu_time = on; }
void host_capture(void) { capture_start(); }
void host_copy_ran(void) { spooky_copy_ran = 1; }
s32 host_copy_keep(void) { return spooky_copy_keep; }
void host_cap_live(s32 live, s32 rec) { g_cap_live = (u8)live; g_cap_rec = (u8)rec; }
u32 host_pf_slow(void) { return g_pf_slow_n; }
u32 host_display(s32 k) { return spooky_display[k]; }
s32 host_steps(void) { return g_steps; }
u32 host_t30(void) { return g_t30; }
float host_fdt(void) { return g_fdt; }
float host_ease_k(float f) { return ease_k(f); }
#endif
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
 * +0x13C), its magazines' loaded rounds (+0x224 / +0x234, +8) and its heat (+0x180: an energy weapon does not overheat) are noted before and put
 * back after - as Cartographer's invulnerability works around the damage call. AI weapons are left alone.
 * ---- */
/* ---- 6i.166 / 6i.167: Infinite Ammo and batteries - an energy weapon's charge neither drains nor refills while
 * Infinite Ammo is on: it stays what it was when the option was turned on or the weapon was picked up, whatever uses it
 * (a shot, a charged shot, a sword's hit, a beam held on). The game's own infinite ammo, which the option turns on for
 * your weapons, covers the reserve of rounds only. Each tick the object table is gone through for weapons (type 2,
 * +0xAA) in a player's inventory (ia_answer's test) - a new one is noted with its charge, a known one has it put back;
 * one no longer carried is forgotten. Before each frame's draw the known ones are put back again, so the HUD never
 * shows the tick's drain.
 * 6i.167: your 6i.166 test - the percentage still went down. The field held was the wrong one. The weapon's two reals
 * are +0x180 and +0x184; the game's own lookup of a weapon's values by name (a string id's top byte is the name's
 * length) gives +0x184 for the 3-letter name - "age", the battery (0 new, 1 spent; a shot adds the barrel's age per
 * round) - and +0x180 for the 4-letter one - "heat" (a shot adds the barrel's heat, it cools every tick, capped at the
 * overheat level). 6i.166 held +0x180: the heat - which is what Bottomless Clip holds, rightly (no overheat; its
 * battery drains: 6i.168). ---- */
#define W_AGE 0x184
#define IA_MAX 16
static u32 g_ia_w[IA_MAX], g_ia_age[IA_MAX];     /* the weapons (datums) and their charge as noted (the float's bits) */
static s32 g_ia_n;
static void ia_hold(s32 scan)                    /* scan 1: a tick (the table gone through); 0: a draw (the known ones only) */
{
	u32 pool = M32(ADDR_OBJECTS_PTR), data, count, i;
	u8 seen[IA_MAX];
	s32 k, n;
	if (!o_inf_ammo) { g_ia_n = 0; return; }
	if (!scan)
	{
		for (k = 0; k < g_ia_n; k++)
		{
			u32 w = object_from_datum((s32)g_ia_w[k]);
			if (w && M8(w + 0xAA) == 2 && M32(w + W_AGE) != g_ia_age[k]) gw32(w + W_AGE, g_ia_age[k]);
		}
		return;
	}
	if (!ptr_ok(pool)) return;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || count > 8192u) return;
	for (k = 0; k < IA_MAX; k++) seen[k] = 0;
	for (i = 0; i < count; i++)
	{
		u32 header = data + i * 12u, salt = M32(header) & 0xffffu, w = M32(header + 8), unit, datum;
		if (!salt || !ptr_ok(w) || M8(w + 0xAA) != 2 || !(M8(w + 0x12C) & 1)) continue;   /* a weapon in an inventory */
		unit = object_from_datum((s32)M32(w + 0x154));
		if (!unit || (s32)M32(unit + 0x13C) == -1) continue;               /* a player's */
		datum = (salt << 16) | i;
		for (k = 0; k < g_ia_n && g_ia_w[k] != datum; k++) ;
		if (k < g_ia_n) { if (M32(w + W_AGE) != g_ia_age[k]) gw32(w + W_AGE, g_ia_age[k]); seen[k] = 1; }
		else if (g_ia_n < IA_MAX) { g_ia_w[g_ia_n] = datum; g_ia_age[g_ia_n] = M32(w + W_AGE); seen[g_ia_n] = 1; g_ia_n++; }
	}
	for (k = 0, n = 0; k < g_ia_n; k++) if (seen[k]) { g_ia_w[n] = g_ia_w[k]; g_ia_age[n] = g_ia_age[k]; n++; }   /* dropped, or gone: forgotten */
	g_ia_n = n;
}
/* ---- 6i.170: One Shot Kills (Cheats) - the effect of the engine's cheat_omnipotent: whatever a player damages dies
 * of it. The retail game has no such switch left, but it has what the switch drove: a damage event's own "kill" flag
 * (bit 2 of its flags, +4) - the game's damage function (v1.0 function_d7b80: stdcall, the event at [esp+4], the
 * object hit at [esp+8]) reads it as "this damage kills" and, if the damage reaches the object at all, depletes its
 * body whatever the amount. hooks.S ohk_hook stands at all 20 of that function's calls: ohk_before sets the flag for
 * that one call when the event's owner is a player (its owner, +8: player +8, object +0xC) and the object hit is not
 * a player's unit, not the owner itself and not what the owner rides; ohk_after takes it off again - one event goes on
 * to every object in a blast, your own body among them. Nothing else is touched: what is invulnerable stays so, and
 * damage that is not yours does what it always did. (Calls nest - an object's damage reaches its children - so the
 * events marked are kept on a small stack.) ---- */
#define OHK_DEPTH 8
static u32 g_ohk_data[OHK_DEPTH];
static s32 g_ohk_n;
void ohk_before(u32 data, u32 object_index)
{
	u32 o, owner;
	if (g_ohk_n < OHK_DEPTH) g_ohk_data[g_ohk_n] = 0;
	g_ohk_n++;
	if (!o_ohk || g_ohk_n > OHK_DEPTH || data < 0x10000u || (data & 3u)) return;   /* 6i.171: the event is on the game's STACK (0xD0000000 and up on the Xbox) - ptr_ok refused it, and so the option did nothing at all */
	if ((s32)M32(data + 8) == -1 || (M32(data + 4) & 4u)) return;          /* not a player's damage; or a kill already */
	o = object_from_datum((s32)object_index);
	if (!o || (s32)M32(o + 0x13C) != -1) return;                           /* a player's unit: never */
	if (object_index == M32(data + 0xC)) return;                           /* the owner itself */
	owner = object_from_datum((s32)M32(data + 0xC));
	if (owner && M32(owner + 0x14) == object_index) return;                /* what the owner rides */
	gw32(data + 4, M32(data + 4) | 4u);
	g_ohk_data[g_ohk_n - 1] = data;
}
void ohk_after(void)
{
	if (g_ohk_n <= 0) return;
	g_ohk_n--;
	if (g_ohk_n < OHK_DEPTH && g_ohk_data[g_ohk_n]) { gw32(g_ohk_data[g_ohk_n] + 4, M32(g_ohk_data[g_ohk_n] + 4) & ~4u); g_ohk_data[g_ohk_n] = 0; }
}
#ifdef HOST
void host_set_ohk(s32 on) { o_ohk = on; }
void host_set_scale(s32 v) { o_scale = v; }
#endif
static void ia_hold_tick(void) { ia_hold(1); }
static void ia_hold_draw(void) { ia_hold(0); }
#ifdef HOST
void host_ia_hold(s32 scan) { ia_hold(scan); }
s32 host_ia_n(void) { return g_ia_n; }
#endif
static u32 g_bc_w;
static u16 g_bc_rounds[2];
static float g_bc_heat;
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
	g_bc_heat = MF(w + 0x180);                                              /* 6i.168: the heat - so the weapon never overheats; its battery (+0x184) drains as the game has it */
	g_bc_w = w;
}
void bc_after(void)
{
	u32 w = g_bc_w;
	if (!w) return;
	gw16(w + 0x224 + 8, g_bc_rounds[0]); gw16(w + 0x234 + 8, g_bc_rounds[1]);
	gwf(w + 0x180, g_bc_heat);
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
	g_trace_on = g_t30 < g_trace_until || vx * vx + vy * vy + vz * vz > 64.f;
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
/* 6i.123: the zoom animated, as the free camera's motion is: each frame the shown magnification eases toward the set
 * one (in 1 / magnification - the field of view's own scale), 20 % a 30 Hz step (about 0.15 s, scaled to the frame's
 * length), in and out and between levels */
static void fc_zoom_step(void)
{
	float want = g_fc_zoom >= 2 ? 1.f / (float)g_fc_zoom : 1.f, d;
	g_fc_zs += (want - g_fc_zs) * ease_k(0.487f);                         /* 6i.126: 0.05 s (1 - e^(-1/30 / 0.05) a 30 Hz step) */
	d = g_fc_zs - want;
	if (d < 0.0005f && d > -0.0005f) g_fc_zs = want;
}
#ifdef HOST
float host_fc_zs(void) { return g_fc_zs; }
void host_fc_zoom_step(s32 zoom) { g_fc_zoom = zoom; fc_zoom_step(); }
#endif
/* once a frame: started from the view you had, then flown by the pad (when it has it) */
static void fc_frame(void)
{
	const float rot = 3.f;
	float dt = 1.f / 30.f, k = ease_k(0.1164f);                           /* HCM's 6% a 60 Hz frame, at 30: 1 - 0.94^2 */
	float f[3], r[3], u[3], sp, dyaw;
	if (o_unlock_fps) dt = g_fdt;                                         /* 6i: this frame's seconds (k: 6% a vblank at 60) */
	s32 i;
	if (!o_free_cam) { g_fc_on = 0; g_fc_pad = 0; g_fc_zoom = 0; g_fc_zs = 1.f; return; }   /* (the level kept for next time) */
	fc_zoom_step();
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
	if (o_fc_block && (!g_open || g_peek))                               /* 6i.163: held aside with White, the menu is as closed */
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
	if (o_hide_hud) return 1;                    /* 6i.120: Hide HUD - function_1917eb draws all of it (items, crosshair, messages, player HUD) */
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
void host_fc_target(float *p) { s32 i; for (i = 0; i < 3; i++) p[i] = g_fc_tp[i]; }   /* 6i */
void host_set_free_cam(s32 on, s32 block) { o_free_cam = on; o_fc_block = block; }
s32 host_fc_block(void) { return o_fc_block; }
float host_fc_in(s32 k) { return g_fc_in[k]; }
void host_fc_set(float x, float y, float z, float yaw, float pitch) { g_fc_on = 1; g_fc_p[0] = x; g_fc_p[1] = y; g_fc_p[2] = z; g_fc_yaw = yaw; g_fc_pitch = pitch; }   /* 6i.160: the camera put somewhere */
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
static s32 ip_on(void)                          /* 6i: and at any speed with Unlock FPS on (60 frames, 30 ticks) */
{
	u32 opt = M32(ADDR_GAME_OPTIONS_PTR);
	return o_speed > 0 && (o_speed < 10 || o_unlock_fps) && !o_pause && ptr_ok(opt) && M32(opt + 8) != 3;
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
/* 6i.15: the view's heading as each of the last two ticks began (the last frame's camera, taken by the snapshot), and how
 * far the frame is between them - for the first-person legs (ip_fp_turn) */
static float g_ip_tcam[4][2][2], g_ip_a;
static u8 g_ip_tcam_n[4], g_ip_tcam_want[4];      /* 6i.30: a tick's view is the first frame drawn AFTER it (ip_view) */
static s32 ip_cam_heading(s32 u, float *h)       /* the camera's heading, level (looking up or down: from its up vector) */
{
	float fz = g_ip_camf[u][2], c = fsqrt(g_ip_camf[u][0] * g_ip_camf[u][0] + g_ip_camf[u][1] * g_ip_camf[u][1]), l;
	h[0] = g_ip_camf[u][0] * c - g_ip_camu[u][0] * fz; h[1] = g_ip_camf[u][1] * c - g_ip_camu[u][1] * fz;
	l = fsqrt(h[0] * h[0] + h[1] * h[1]);
	if (!(l > 1e-3f)) return 0;
	h[0] /= l; h[1] /= l;
	return 1;
}
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
		if (g_ip_tcam_want[user])                                         /* 6i.30: the first frame after a tick: its view */
		{
			float h[2];
			if (ip_cam_heading(user, h)) { g_ip_tcam[user][1][0] = h[0]; g_ip_tcam[user][1][1] = h[1]; if (g_ip_tcam_n[user] < 2) g_ip_tcam_n[user]++; }
			g_ip_tcam_want[user] = 0;
		}
	}
	if (!ipo) return cam;
	b = TP_BUF(user);
	for (i = 0; i < 0x358; i += 4) gw32(b + (u32)i, M32(cam + (u32)i));
	for (i = 0; i < 3; i++) gwf(b + 4 * (u32)i, MF(cam + 4 * (u32)i) + off[i]);
	g_ip_cam[user] = 1;
	return b;
}
/* ---- 6i.120: Field of View (Camera Settings) and the free camera's zoom. The view is built from the camera record's
 * field of view at +0x4C (radians; function_2f800 takes 0.785 of it as the vertical angle). Changed only in the copy
 * the view is built from (TP_BUF) - the game's own record keeps its value, so nothing of the game's is left changed.
 * Scaled in tangent space, so a scope still narrows it by its own amount. On foot only, with the camera at your eye
 * (first and third person, the free camera): vehicles and cutscenes keep the game's. The game's own on-foot value comes
 * from the map's data, so it's read while you play (unzoomed, on foot) and the menu marks it *. ---- */
static float ftan(float x);
/* 6i.172: Field of View still did nothing on the 360 with the pointer limit raised (6i.171), so that was not it. What
 * the Field of View path uses that little else does is the x87's arctangent (fpatan) and tangent (fptan): if the 360's
 * translation of either gives a value out of range, the new angle is refused and the game's is kept - "does nothing".
 * Not confirmed. Both are now plain arithmetic here (the same on both consoles): the tangent from the sine and cosine
 * the free camera already uses; the arctangent by halving the range twice and a five-term series (good to 0.00002).
 * And the menu now says why when the angle asked for is not the one in use (fov_why). */
/* 6i.173: your 6i.172 test on the 360 - "(not in use: out of range)" for any angle but the game's own. So the angle
 * worked out was not between 0.005 and 3 radians, with the arctangent already plain arithmetic: that leaves the
 * tangent, which 6i.172 still took from the x87's fsincos. It is a series now as well (sine and cosine to x^11: good
 * to 0.00001 up to 1.5 radians) - nothing in this path asks the x87 for anything but + - x / any more. And the menu
 * line gives the four numbers behind an "out of range" (g_fov_dbg), in thousandths. */
static float fov_tan(float x) { return ftan(x); }
static __attribute__((noinline)) float fp_tan(float x)   /* 6i.185: sine over cosine by their series - no comparison, no branch; for |x| under 1 */
{
	float x2 = x * x;
	float sn = x * (1.f - x2 / 6.f * (1.f - x2 / 20.f * (1.f - x2 / 42.f * (1.f - x2 / 72.f * (1.f - x2 / 110.f)))));
	float cs = 1.f - x2 / 2.f * (1.f - x2 / 12.f * (1.f - x2 / 30.f * (1.f - x2 / 56.f * (1.f - x2 / 90.f))));
	return sn / cs;
}   /* 6i.174: the one series (ftan) for every tangent */
static float fov_atan(float t)                   /* t >= 0 */
{
	float add = 0.f, x2, r;
	s32 inv = 0;
	if (t > 1.f) { t = 1.f / t; inv = 1; }
	if (t > 0.41421356f) { t = (t - 1.f) / (t + 1.f); add = 0.78539816f; }
	x2 = t * t;
	r = add + t * (1.f - x2 * (1.f / 3.f - x2 * (1.f / 5.f - x2 * (1.f / 7.f - x2 * (1.f / 9.f)))));
	return inv ? 1.57079633f - r : r;
}
static s32 g_fov_why;                            /* why the angle asked for is not in use: 0 it is; 1 no camera; 2 the game's angle unreadable; 3 not on foot at your eye; 4 the game's own not read yet; 5 the result out of range */
#ifdef HOST
float host_fov_atan(float t) { return fov_atan(t); }
float host_fov_tan(float x) { return fov_tan(x); }
s32 host_fov_why(void) { return g_fov_why; }
#endif
static float g_fov_used;                         /* the field of view local player 0's last world view was built with */
static s32 g_fov_over;                           /* ... changed by us (Field of View or the zoom) */
static s32 fov_on_foot_eye(u32 cam, s32 user)
{
	u32 obj = user_object(user);
	float d2 = 0.f;
	s32 i;
	if (!obj || (s32)M32(obj + 0x14) != -1) return 0;                          /* in a vehicle */
	for (i = 0; i < 3; i++) { float d = MF(cam + 4 * (u32)i) - MF(obj + 0x64 + 4 * (u32)i); d2 += d * d; }
	return d2 < 2.5f * 2.5f;
}
static u32 tp_camera_in(u32 cam, s32 user);
/* 6i.126: the overlays' field of view is the WORLD view's - the frame's first view build for player 0. Your test: with the
 * free camera zoomed the overlays' lines lost their perspective. Cause: a later view build in the same frame (not zoomed)
 * reset what the projection reads to the game's own field of view; only the first now sets it (cleared at each frame's
 * start) */
static s32 g_fov_seen;
static u32 tp_camera_fov(u32 cam, s32 user);
u32 tp_camera(u32 cam, s32 user)
{
	s32 first = user == 0 && !g_fov_seen, keep_over = g_fov_over;
	float keep_used = g_fov_used;
	u32 r = tp_camera_fov(cam, user);
	if (user == 0) { if (!first) { g_fov_over = keep_over; g_fov_used = keep_used; } g_fov_seen = 1; }
	return r;
}
#ifdef HOST
float host_fov_used(void) { return g_fov_used; }
void host_fov_frame(void) { g_fov_seen = 0; }
#endif
static u32 tp_camera_fov(u32 cam, s32 user)
{
	u32 r = tp_camera_in(cam, user), obj;
	float game, k = 1.f;
	s32 i, eye, zoomed = 0, ws55, deg;
	if (user < 0 || user > 3 || !ptr_ok(r) || !ptr_ok(cam)) { if (user == 0) g_fov_why = 1; return r; }
	game = MF(r + 0x4C);
	g_fp_s[user] = 1.f;
	if (user == 0) { g_fov_over = 0; g_fov_used = game; }
	if (!(game > 0.05f && game < 3.f)) { if (user == 0) g_fov_why = 2; return r; }
	obj = user_object(user);
	if (obj) zoomed = (s8)M8(obj + 0x240) != -1 || (s8)M8(obj + 0x241) != -1;
	eye = fov_on_foot_eye(cam, user);
	/* ---- 6i.220: THE VIEW AND CROSSHAIR JUMPING AS YOU COME OUT OF THE SCOPE (your video) ----
	 * The mod learns "the game's own angle" from the camera whenever you are on foot and not zoomed, and scales the view
	 * from it. But when you let go of the scope the game does not snap back: your unit stops being "zoomed" at once
	 * while the camera's angle EASES back to its own over the next few frames - and with it the game eases the view's
	 * centre (and the crosshair that sits on it) from the scope's middle of the screen back down to where they belong.
	 * On each of those frames the mod took the half-way angle to be the game's own - 40 degrees, then 50 - and so
	 * scaled the view straight out to your full Field of View. The picture was already wide while the game's centre
	 * was still on its way down: the view and the crosshair sat too high for a frame or two, then dropped. At every
	 * Field of View, at any Render Scale, with or without Performance Mode - it is in the angle, nothing else.
	 * Now the game's own angle is taken only once it has HELD STILL for FOV_SETTLE frames. While it is easing, the last
	 * settled value stands, so the view is scaled by the same factor all the way down - the game's own ease, at your
	 * Field of View. ---- */
	if (user == 0 && eye && !zoomed && game > 0.5f && game < 2.5f)           /* the game's own, read on foot */
	{
		float seen = MF(cam + 0x4C) > 0.5f && MF(cam + 0x4C) < 2.5f ? MF(cam + 0x4C) : game;
		static float cand; static s32 cand_n; static u32 cand_frame = 0xFFFFFFFFu;
		if (seen > cand - 0.0005f && seen < cand + 0.0005f) { if (cand_frame != g_frames) cand_n++; }
		else { cand = seen; cand_n = 0; }
		cand_frame = g_frames;
		if (cand_n < g_fov_settle && g_fov_def > 0) seen = g_fov_def_rad;     /* still moving: the last settled angle stands */
		else if (cand_n < g_fov_settle) seen = 0.f;                           /* nothing settled yet: wait */
	  if (seen > 0.5f)
	  {
		g_fov_def_rad = seen;
		/* 6i.215: YOUR NOTE - the * must stay on 55; you have seen it move between 55 and 56. The game's own angle is
		 * 0.967 rad = 55.4 degrees (your 6i.186 log), and this rounded it to the NEAREST degree: at 55.4 that is 55, and
		 * the moment the game's angle reads a tenth of a degree more it is 56. Rounded as before, but an answer of 56
		 * is 55: the retail angle's whole neighbourhood (54.5 up to 56.5 degrees) is "55". Other angles are untouched. */
		{ s32 dg = fti(g_fov_def_rad * 57.29578f + 0.5f); g_fov_def = dg == 56 ? 55 : dg; }
		if (o_fov <= 0) o_fov = g_fov_def;
	  }
	}
	if (user == 0) g_fov_why = !eye ? 3 : g_fov_def <= 0 ? 4 : 0;
	/* 6i.225: YOUR NOTE - with the water's widening, 56 came out wider (57.1 degrees) than 57 (57.0), so stepping the
	 * Field of View went out, in a little, and out again. In widescreen the two settings now trade angles: 56 gives 57
	 * degrees exactly and 57 gives what 56 gave (56 degrees, 2 % wider = 57.1). So the view only ever widens as the
	 * number goes up: 55* 56.8, 56 57.0, 57 57.1, 58 58.0. Both are clear of the band where the water is cut. In 4:3
	 * nothing is widened and nothing is traded. */
	ws55 = eye && g_fov_def == 55 && (g_ws || g_ws_flag5);
	deg = o_fov;
	if (ws55 && o_fov == 56) deg = 57; else if (ws55 && o_fov == 57) deg = 56;
	if (eye && o_fov > 0 && g_fov_def > 0 && o_fov != g_fov_def)
		k = fov_tan((float)deg * (0.5f / 57.29578f)) / fov_tan(g_fov_def_rad * 0.5f);
	if (user == 0 && o_free_cam && g_fc_on && g_tp_applied[0] && g_fc_zs < 0.9999f) k *= g_fc_zs;   /* 6i.123: the eased zoom */
	/* ---- 6i.219: THE WATER CUT OFF AT THE SCREEN'S EDGES IN WIDESCREEN - your fix, from your own measurements ----
	 * The game itself does this (you checked the unmodified game): in widescreen, with the view at or near its own angle
	 * (55.4 degrees), the water stops short of the screen's left and right edges, along a curved line. Your round with
	 * the test stepper (6i.218), by the angles its log wrote down:
	 *     Field of View 55*: cut up to +2.0 % (56.52 degrees), right at +2.5 % (56.80)
	 *     Field of View 56 : right at +2.0 % (57.12)
	 *     Field of View 54 : right as it is (54.00, 54.27); cut again when pushed to +2.0 % (55.08)
	 * So the water is cut for a view between about 55 and 56.5 degrees and whole either side. WHY the game does that I
	 * have not found: its view-building routine (function_2f800 / function_223080) treats every angle alike, and the
	 * water's own drawing code is not in the decompilation. This is therefore a way round it, not a cure of the cause -
	 * and it is exactly what you asked for: at Field of View 55* the view is 2.5 % wider, at 56 it is 2 % wider, and
	 * no other setting is touched. Only in widescreen (in 4:3 nothing is changed). Consequences, so they are not a
	 * surprise: 55* is no longer the game's exact angle but 56.8 degrees (the first-person body's fix therefore runs
	 * there, with a factor of about 1.03). (6i.219 put the 2 % on the 56 setting, which made it a hair wider than 57;
	 * 6i.225 trades the two - see above.) The 6i.215 -
	 * 6i.218 test setting is gone. ---- */
	{
		s32 widen = 0;                                                        /* (a whole number: no float == on the 360 - see ftan) */
		g_fov_edge = 1.f;
		if (ws55)
		{
			if (o_fov <= 0 || o_fov == 55) { g_fov_edge = 1.025f; widen = 1; }
			else if (o_fov == 57) { g_fov_edge = 1.02f; widen = 1; }             /* (6i.225: on 56 degrees - the angle the 56 setting had) */
		}
		if (k > 0.99999f && k < 1.00001f && !widen) return r;                 /* (6i.175: was k == 1 - a parity jump; see ftan) */
	}
	{
		float want = (k > 0.99999f && k < 1.00001f) ? game : 2.f * fov_atan(fov_tan(game * 0.5f) * k);
		want *= g_fov_edge;                                                   /* 6i.219: 1.025 at 55*, 1.02 at 56, in widescreen; else 1 */
		/* 6i.187: FOUND - your 6i.186 log: "the game's angle 0x4471C8D5" = 967.14, where the game's angle is 0.967. It had
		 * been multiplied by 1000 - by the line that stood here: g_fov_dbg[0] = fov_milli(game), i.e. (s32)(game * 1000),
		 * a number for the menu's "out of range" note that I added in 6i.173. On the Xbox that leaves `game` alone; on the
		 * 360's emulator the float-to-whole-number conversion left the multiplied value where `game` was kept, and every
		 * use of it after - the body's factor - got 967 for 0.967. (The view itself was right: its angle is worked out
		 * before this line.) So the legs' fix has been refused on the 360 since 6i.173 by a diagnostic of mine.
		 * The four conversions are gone. RULE, for anything that must run on the 360: no (s32)(float) of a value that is
		 * used again afterwards - copy it to a volatile first, or take its bits. */
		if (!(want > 0.005f && want < 3.f)) { if (user == 0) g_fov_why = 5; return r; }
		if (r == cam) { u32 b = TP_BUF(user); for (i = 0; i < 0x358; i += 4) gw32(b + (u32)i, M32(cam + (u32)i)); r = b; }
		gwf(r + 0x4C, want);
		if (user == 0) { g_fov_over = 1; g_fov_used = want; }
		/* 6i.185: your 6i.184 log on the 360 - "the fix is turned away by its test 2 ...; factor (thousandths) 0": this
		 * ratio came out 0 there (about 1.5 on the Xbox), so the fix refused it. The same ftan gives the Field of View its
		 * right answer two lines up, so it is this use of it - and ftan begins with a test (is the angle a sane number?)
		 * that returns 0 when it fails. On the 360 a test like that has gone the wrong way before (6i.175). I cannot see
		 * from here that it does so here; but a tangent with NO test in it cannot be turned away by one: fp_tan is the
		 * plain series (these angles are under 0.85 rad - no reduction needed). The log now has the factor's exact bits. */
		/* 6i.186: your 6i.185 log on the 360 - still refused, and now with a number: "the factor's bits 0x3C3BE1A6" =
		 * 0.0115, where the Xbox makes about 1; on screen 0.010 at 50 degrees rising to 0.031 at 120. So the top of the
		 * ratio is right in shape (it grows as the tangent does) and the whole is about 88 times too small - with a
		 * tangent that has no test in it. I do not know which step goes wrong there, and guessing has cost three rounds.
		 * So: the factor is worked out three ways that share no code - the series (fp_tan), the general tangent (ftan),
		 * and a plain polynomial - and each is checked against what it must be near (the ratio of the two angles
		 * themselves, which a tangent ratio at these angles is within about a third of). The first that passes is used;
		 * if none does, the ratio of the angles is used - a close fit, not the exact one. And the log has every number
		 * in between, as its exact bits: next round says which step it is. */
		if (eye && !g_tp_applied[user])                                      /* 6i.128: the projection's 0.785 x the field of view */
		{
			volatile float xa = 0.3925f * want, xb = 0.3925f * game;
			volatile float a1 = fp_tan(xa), b1 = fp_tan(xb), sa = a1 / b1;
			volatile float a2 = ftan(xa), b2 = ftan(xb), sb = a2 / b2;
			volatile float pa = xa * (1.f + xa * xa * (0.33333334f + xa * xa * (0.13333334f + xa * xa * (0.053968254f + xa * xa * 0.021869488f))));
			volatile float pb = xb * (1.f + xb * xb * (0.33333334f + xb * xb * (0.13333334f + xb * xb * (0.053968254f + xb * xb * 0.021869488f))));
			volatile float sc = pa / pb, near = want / game, s_use;
			if (sa > near * 0.6f && sa < near * 1.7f) s_use = sa;
			else if (sb > near * 0.6f && sb < near * 1.7f) s_use = sb;
			else if (sc > near * 0.6f && sc < near * 1.7f) s_use = sc;
			else s_use = near;
			g_fp_s[user] = s_use;
		}
	}
	return r;
}
#ifdef HOST
void host_set_trate(s32 v) { o_trate = v; }
void host_set_fov(s32 v) { o_fov = v; }
s32 host_fov_get(void) { return o_fov; }
s32 host_fov_def(void) { return g_fov_def; }
s32 host_fc_zoom(void) { return g_fc_zoom; }
#endif
static u32 tp_camera_in(u32 cam, s32 user)
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
		{
			/* 6i.185: your 6i.184 test - still thrown out of the free camera in a vehicle. 6i.179 widened the limit for a
			 * rider but measured from the rider's own position - which, for something riding something else, is kept
			 * RELATIVE TO WHAT IT RIDES, not in the world: the distance was nonsense and still too far. Now measured from
			 * what it rides (up the chain of parents, +0x14, to the one with none - the vehicle - whose position is the
			 * world's). On foot: your own, as ever. */
			u32 root = obj, par;
			s32 up;
			for (up = 0; up < 4 && (s32)(par = M32(root + 0x14)) != -1; up++) { u32 po = object_from_datum((s32)par); if (!po) break; root = po; }
			for (i = 0; i < 3; i++) { float d = MF(cam + 4 * (u32)i) - MF(root + 0x64 + 4 * (u32)i); d2 += d * d; }
		}
		/* 6i.179: your note - in a vehicle the free camera gave way to the game's third person view until you got out.
		 * This test ("the game's camera is within 2.5 units of your body, or it is not your eye but a cutscene's") is
		 * why: a vehicle's own camera sits well behind it. Riding something (the unit has a parent, +0x14), the limit
		 * is 20 units; and a cutscene is told by the game's own flag as well (the cinematic globals' +5), on foot or not */
		{
			u32 cg = ADDR_ASPECT_FLAGS_PTR ? M32(ADDR_ASPECT_FLAGS_PTR) : 0;
			float lim = (s32)M32(obj + 0x14) != -1 ? 20.f : 2.5f;
			if ((ptr_ok(cg) && M8(cg + 5)) || !(d2 < lim * lim)) return cam;     /* not your eye (a cutscene): left alone */
		}
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
		float p[3], nf[3], u0[3], nu[3], k, l2;
		/* the eye's height smoothed: predicted from your vertical speed (falls, jumps and slopes followed exactly), only
		 * the difference from that eased (a quarter a frame) - stepping over things bumps the eye, and the camera with
		 * it; a jump over half a unit (a teleport, a respawn) taken at once */
		{
			/* 6i.122: in the air exactly your eye. 6i.123 (your test: no shake on jumps, but on hills, slopes and seams): on the
			 * ground too - the speed prediction and easing put the camera somewhere your body (drawn blended, a tick behind)
			 * wasn't, wherever the ground's height changed unevenly. The camera now moves with your eye exactly, as first person
			 * does; a step up moves it with you */
			g_tp_ez[user] = eye[2];
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
				else g_tp_pull[user] += (allow - g_tp_pull[user]) * ease_k(1.f / 6.f);
				for (i = 0; i < 3; i++) p[i] = pv[i] + dv[i] * g_tp_pull[user];
			}
		}
		else g_tp_pull[user] = 1.f;
		gwf(b + 0, p[0]); gwf(b + 4, p[1]); gwf(b + 8, p[2]);
		/* 6i.125: the camera looks exactly the way your view does - it turns only when you turn. 6h.9 - 6i.124 aimed it at
		 * the first wall along your aim (eased, never nearer than 3 units) so the reticle sat where your shots land; close
		 * to a wall that point jumped as you walked and the camera's aim got bumped (your test). Now nothing in the world
		 * moves it: with the camera to one side, the reticle is that far off your shots' line, the same at any distance */
		for (i = 0; i < 3; i++) nf[i] = f[i];
		l2 = 1.f;
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
static void view_camera(float *p, float *f, float *u);
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
/* 6h.25: the objects near the view (bounding sphere within IP_RANGE of the camera) - node matrices before the tick.
 * 6i.1: its memory sized to what the console can spare. 6i asked for 6,144 nodes' worth at run time - 678 KB - and your
 * 360 didn't have it ("smooth slow motion: no memory for it", the moment Unlock FPS went on): Unlock FPS ran with no
 * in-between frames, each tick shown for one, two or three frames as they fell - the jitter. Now (ip_alloc): the
 * Superbounce Viewer's own run-time memory is taken first (what its first use takes anyway), then the largest free block
 * is measured, and the interpolation takes 2048, 1024, 512 or 256 nodes' worth - the most that leaves IP_KEEP free beside
 * it - or, short of all of those, 128 (~15 KB) if 16 KB still stays free. Logged: what it got, and the largest block there
 * was. None: tried again every 30 s. Taken in order (ip_snapshot): your own unit, what it rides and what it holds; then
 * objects within 20 units of the view; then out to 80 - so a small budget still smooths your view, your body and what's
 * nearest. */
#define IP_NODE_MAX 2048                        /* the most asked for: ~226 KB */
#define IP_NODE_MIN 128                         /* the least worth having: ~15 KB - your unit and what it carries */
#define IP_RANGE 80.f
#define IP_NEAR 20.f
#define IP_KEEP_SPOTS (192u * 1024u)            /* left free beside it with the Monster/Dreadnought Viewer on: its largest spots file (~170 KB) and a ghost file */
#define IP_KEEP (32u * 1024u)                   /* otherwise: a ghost file and a little (6i.2; it was 192 KB always - with 112 KB free, 128 nodes) */
#define IP_SAVED_MAX 1024
static u8 g_ip_saved[IP_SAVED_MAX];               /* by a tracked object's place: this tick's pose has been read once already (6i.204: counts a first-person weapon change once a tick) */
typedef struct { u32 datum, first, count, cur; float fwd[2], org[3]; } s_ip_ent;   /* cur: the matrices' address while blended (0: not); fwd: its facing (+0x70); org: its origin (+0x64) */
static s_ip_ent *g_ip_ent;
static u32 *g_ip_prev, *g_ip_back;              /* 13 words a node: before the tick; the real ones while blended */
static s32 g_ip_n, g_ip_nodes, g_ip_valid, g_ip_blended, g_ip_mem;   /* g_ip_mem: 0 not asked, 1 ok, -1 failed */
static s32 g_ip_node_cap, g_ip_ent_cap, g_ip_left_out, g_ip_body_n;   /* 6i.27: your body's nodes, -2 left out */   /* what it got; objects in range the last snapshot had no room for */
static u32 g_ip_tick, g_ip_retry, g_ip_largest;
static u32 g_ip_fpw[4][2], g_ip_fpw_n;            /* 6i.204: the weapon each first-person block held at the snapshot; times a change was met - see ip_blend_nodes' note */
static u32 ip_ents_for(u32 nodes) { return nodes / 4u < 64u ? 64u : nodes / 4u; }
static u32 ip_bytes(u32 nodes) { return ip_ents_for(nodes) * (u32)sizeof(s_ip_ent) + nodes * 104u; }   /* 13 words twice a node */
/* the nodes to ask for, given the largest block free: the most of 2048 / 1024 / 512 / 256 that leaves IP_KEEP free; short
 * of those, 128 if 16 KB still stays free; else none */
static u32 ip_nodes_for(u32 largest, u32 keep)
{
	u32 n;
	for (n = IP_NODE_MAX; n > IP_NODE_MIN; n /= 2u) if (ip_bytes(n) + keep <= largest) return n;
	return ip_bytes(IP_NODE_MIN) + 16384u <= largest ? IP_NODE_MIN : 0u;
}
static u32 ip_keep(void) { return o_pred ? IP_KEEP_SPOTS : IP_KEEP; }
#ifndef HOST
/* the largest block VirtualAlloc gives right now, to 16 KB: taken and given straight back, halving the gap (2 MB at most) */
static u32 mem_largest(void)
{
	u32 lo = 0, hi = 2048u * 1024u;
	while (hi - lo > 16384u)
	{
		u32 mid = ((lo + hi) / 2u) & ~16383u;
		void *m;
		if (mid <= lo) mid = lo + 16384u;
		m = VirtualAlloc_x(0, mid, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 4 /* PAGE_READWRITE */);
		if (m) { VirtualFree_x(m, 0, 0x8000 /* MEM_RELEASE */); lo = mid; } else hi = mid;
	}
	return lo;
}
#endif
static void sb_mem_reserve(void);
#ifdef HOST
static u32 g_host_ip_largest = 4u * 1024u * 1024u;
#endif
static s32 ip_alloc(void)
{
	u32 n;
	if (g_ip_mem > 0) return 1;
	if (g_ip_mem < 0 && (s32)(g_t30 - g_ip_retry) < 0) return 0;
#ifdef HOST
	{
		static s_ip_ent e[IP_NODE_MAX / 4];
		static u32 pv[IP_NODE_MAX * 13], bk[IP_NODE_MAX * 13];
		g_ip_largest = g_host_ip_largest;
		n = ip_nodes_for(g_ip_largest, ip_keep());
		if (n) { g_ip_ent = e; g_ip_prev = pv; g_ip_back = bk; }
	}
#else
	{
		u8 *m = 0;
		sb_mem_reserve();                                                     /* the Superbounce Viewer's own first (6i.38: always again, as up to 6i.35) */
		g_ip_largest = mem_largest();
		n = ip_nodes_for(g_ip_largest, ip_keep());
		while (n >= IP_NODE_MIN && !(m = (u8 *)VirtualAlloc_x(0, ip_bytes(n), 0x3000, 4))) n = n > IP_NODE_MIN ? n / 2u : 0u;   /* (gone since: smaller) */
		if (m) { g_ip_ent = (s_ip_ent *)m; g_ip_prev = (u32 *)(m + ip_ents_for(n) * sizeof(s_ip_ent)); g_ip_back = g_ip_prev + n * 13u; }
		else n = 0;
	}
#endif
	if (!n)
	{
		if (!g_ip_mem)                                                         /* said once; tried again every 30 s */
		{
			s32 at = lput(0, "frame interpolation: no memory for it (the largest free block "); at = lnum(at, g_ip_largest / 1024u);
			lput(at, " KB) - the picture steps; tried again every 30 s");
			log_write();
		}
		g_ip_mem = -1; g_ip_retry = g_t30 + 900u;
		return 0;
	}
	g_ip_node_cap = (s32)n; g_ip_ent_cap = (s32)ip_ents_for(n);
	g_ip_mem = 1;
	{
		s32 at = lput(0, "frame interpolation: memory for "); at = lnum(at, n); at = lput(at, " nodes (");
		at = lnum(at, (ip_bytes(n) + 1023u) / 1024u); at = lput(at, " KB); the largest free block was ");
		at = lnum(at, g_ip_largest / 1024u); lput(at, " KB");
		log_write();
	}
	return 1;
}
static s32 ip_give_back(const char *why);
/* 6i.2: the memory given back - when Unlock FPS goes off (and the game isn't slowed: nothing to draw between), and when the
 * Monster/Dreadnought spots can't get theirs - so the next use sizes it afresh to what's there then. Not mid-draw. */
static s32 ip_give_back(const char *why)
{
	if (g_ip_mem <= 0 || g_ip_blended) return 0;
#ifndef HOST
	VirtualFree_x(g_ip_ent, 0, 0x8000 /* MEM_RELEASE */);
#endif
	g_ip_ent = 0; g_ip_prev = g_ip_back = 0;
	g_ip_mem = 0; g_ip_valid = 0; g_ip_n = 0; g_ip_nodes = 0; g_ip_node_cap = g_ip_ent_cap = 0;
	{ s32 at = lput(0, "frame interpolation: its memory given back ("); at = lput(at, why); lput(at, ") - sized afresh next time"); log_write(); }
	return 1;
}
#ifdef HOST
u32 host_ip_nodes_for(u32 largest, u32 keep) { return ip_nodes_for(largest, keep); }
s32 host_ip_give_back(void) { return ip_give_back("test"); }
void host_ip_mem(u32 largest) { g_ip_mem = 0; g_host_ip_largest = largest; }   /* allocate afresh, as if the console had this */
void host_set_ip_largest(u32 b) { g_host_ip_largest = b; }   /* the free memory changing (a retry sees it) */
s32 host_ip_cap(void) { return g_ip_mem > 0 ? g_ip_node_cap : g_ip_mem; }
s32 host_ip_left_out(void) { return g_ip_left_out; }
s32 host_ip_body(void) { return g_ip_body_n; }
u32 host_ip_datum(s32 k) { return k < g_ip_n ? g_ip_ent[k].datum : 0xFFFFFFFFu; }
#endif
/* 6i.50: a seat's camera follows its vehicle's PHYSICS body, not its nodes: the camera target (v1.0 0x1871e0) takes the
 * seat's camera point by 0xb9ef0, which walks to the root object and reads its Havok component's camera body (+0x18, an
 * index into its bodies, +0x70 / +0x74) - that body's motion, its centre of mass (+0x70). Havok steps in the tick, so the
 * camera jumped ahead 30 times a second while the vehicle, drawn between ticks, caught up behind it. So that centre is
 * snapshotted with yours and blended for the director and the observer too, put back with them (exactly). */
typedef struct { u32 datum, motion, set; float v[3]; u32 bk[3]; } s_ip_hk;
static s_ip_hk g_ip_hk[4];
static s32 g_ip_hk_n;
static u32 ip_cam_motion(u32 datum)              /* 0xb9ef0's body for this object: its root's camera body's motion, or 0 */
{
	u32 obj = object_from_datum((s32)datum), p, comp, bodies, ent, mo;
	s32 k = 0, idx;
	while (obj && (p = M32(obj + 0x14)) != 0xFFFFFFFFu && k++ < 8) obj = object_from_datum((s32)p);
	if (!obj || !(comp = pool_entry(ADDR_HAVOK_COMPONENTS_PTR, (s32)M32(obj + 0xB4), 0x78))) return 0;
	idx = (s32)(s8)M8(comp + 0x18);
	if (idx < 0 || idx >= (s32)M32(comp + 0x74) || !ptr_ok(bodies = M32(comp + 0x70))) return 0;
	ent = M32(bodies + (u32)idx * 0x60u + 0x40u);
	if (!ptr_ok(ent) || !ptr_ok(mo = M32(ent + 0x3Cu))) return 0;
	return mo;
}
static void ip_hk_take(u32 datum)
{
	u32 mo = ip_cam_motion(datum);
	s32 k;
	if (!mo || g_ip_hk_n >= 4) return;
	for (k = 0; k < g_ip_hk_n; k++) if (g_ip_hk[k].motion == mo) return;   /* (two of you in one vehicle) */
	g_ip_hk[g_ip_hk_n].datum = datum; g_ip_hk[g_ip_hk_n].motion = mo; g_ip_hk[g_ip_hk_n].set = 0;
	for (k = 0; k < 3; k++) g_ip_hk[g_ip_hk_n].v[k] = MF(mo + 0x70u + 4u * (u32)k);
	g_ip_hk_n++;
}
/* 6i.48: the cameras blended too. Looking down, the game's first-person camera rides on your pelvis: its eye (v1.0
 * 0xcafc0 -> 0xdf380, the biped's camera interpolation - past an angle down, the eye follows node 0 by the biped's movement
 * scales) is made from your unit's origin (+0x64), crouch (+0x2C4), looking vector (+0x168, set in the tick) and node 0 -
 * every frame, by the game's director (v1.0 0x1554b0), but BEFORE the draw, where the blend was. So the eye came from the
 * last tick's pelvis and moved 30 times a second, while the body drawn under it moved smoothly: the camera and everything
 * near it out of step every other frame (the doubling - worse the more you look down; in third person, built from that
 * eye; in a vehicle, its seat's camera from the vehicle's nodes, origin and facing). Now the frame's blend is made before
 * the director (hooks.S ip_dir_hook), and for it and the observer only, your unit's origin, facing, up, looking vector and
 * crouch, and what it rides (origin, facing, up), are blended as well - the game builds every camera from the same in-
 * between state it draws. They're put back before the draw (the nodes after it, as ever). 6h.26's offset - the origin's
 * blend added to the eye afterwards, which missed the pelvis - goes: the cameras come out of the game already blended. */
#define IP_OWN_MAX 8
static const u16 g_ip_own_off[13] = { 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C, 0x80, 0x84, 0x168, 0x16C, 0x170, 0x2C4 };
typedef struct { u32 datum, obj; s32 n; float v[13]; u32 bk[13]; } s_ip_own;
static s_ip_own g_ip_own[IP_OWN_MAX];
static s32 g_ip_own_n, g_ip_own_set;            /* snapshotted before the tick; written (blended) now */
static u8 g_ip_dir;                              /* this frame's blend made before the director */
static void ip_own_take(u32 datum)
{
	u32 obj = object_from_datum((s32)datum);
	u8 type;
	s32 k;
	s_ip_own *o;
	if (!obj || g_ip_own_n >= IP_OWN_MAX) return;
	for (k = 0; k < g_ip_own_n; k++) if (g_ip_own[k].datum == datum) return;   /* (two of you in one vehicle) */
	type = M8(obj + 0xAA);
	o = &g_ip_own[g_ip_own_n++];
	o->datum = datum; o->obj = 0;
	o->n = type == 0 ? 13 : type == 1 ? 12 : 9;                         /* a unit: its looking vector too; a biped: its crouch */
	for (k = 0; k < o->n; k++) o->v[k] = MF(obj + g_ip_own_off[k]);
}
static void ip_nlerp(float *r, const float *p, const float *c, float a)   /* a direction between two (normalised; else c) */
{
	float l = 0.f;
	s32 k;
	if (p[0] == c[0] && p[1] == c[1] && p[2] == c[2]) { r[0] = c[0]; r[1] = c[1]; r[2] = c[2]; return; }
	for (k = 0; k < 3; k++) { r[k] = p[k] + (c[k] - p[k]) * a; l += r[k] * r[k]; }
	if (!(l > 1e-6f)) { r[0] = c[0]; r[1] = c[1]; r[2] = c[2]; return; }
	l = 1.f / fsqrt(l);
	for (k = 0; k < 3; k++) r[k] *= l;
}
static void ip_own_blend(void)
{
	float a = g_ip_a;
	s32 i, k;
	for (i = 0; i < g_ip_own_n; i++)
	{
		s_ip_own *o = &g_ip_own[i];
		u32 obj = object_from_datum((s32)o->datum);
		float c[13], r[13], d2 = 0.f;
		s32 same = 1;
		o->obj = 0;
		if (!obj) continue;
		for (k = 0; k < o->n; k++)
		{
			c[k] = MF(obj + g_ip_own_off[k]);
			if (!(c[k] == c[k] && o->v[k] == o->v[k] && c[k] < 1e9f && c[k] > -1e9f)) break;
			if (c[k] != o->v[k]) same = 0;
		}
		if (k < o->n || same) continue;                                     /* not a number, or still: as it is */
		for (k = 0; k < 3; k++) d2 += (c[k] - o->v[k]) * (c[k] - o->v[k]);
		if (!(d2 < 16.f)) continue;                                        /* a jump of 4 units or more: at once */
		for (k = 0; k < 3; k++) r[k] = o->v[k] == c[k] ? c[k] : o->v[k] + (c[k] - o->v[k]) * a;
		ip_nlerp(r + 3, o->v + 3, c + 3, a); ip_nlerp(r + 6, o->v + 6, c + 6, a);
		if (o->n > 9) ip_nlerp(r + 9, o->v + 9, c + 9, a);
		if (o->n > 12) r[12] = o->v[12] == c[12] ? c[12] : o->v[12] + (c[12] - o->v[12]) * a;
		for (k = 0; k < o->n; k++) o->bk[k] = M32(obj + g_ip_own_off[k]);
		o->obj = obj; g_ip_own_set++;                                      /* (before writing: a fault still puts it back) */
		for (k = 0; k < o->n; k++) gwf(obj + g_ip_own_off[k], r[k]);
	}
	for (i = 0; i < g_ip_hk_n; i++)                                        /* 6i.50: the vehicle camera's body */
	{
		s_ip_hk *h = &g_ip_hk[i];
		float c[3], d2 = 0.f;
		h->set = 0;
		if (ip_cam_motion(h->datum) != h->motion) continue;               /* not the same body now: as it is */
		for (k = 0; k < 3; k++) { c[k] = MF(h->motion + 0x70u + 4u * (u32)k); if (!(c[k] == c[k] && h->v[k] == h->v[k] && c[k] < 1e9f && c[k] > -1e9f)) break; d2 += (c[k] - h->v[k]) * (c[k] - h->v[k]); }
		if (k < 3 || !(d2 < 16.f) || d2 == 0.f) continue;                  /* not a number, a jump, or still */
		for (k = 0; k < 3; k++) h->bk[k] = M32(h->motion + 0x70u + 4u * (u32)k);
		h->set = 1; g_ip_own_set++;
		for (k = 0; k < 3; k++) gwf(h->motion + 0x70u + 4u * (u32)k, h->v[k] + (c[k] - h->v[k]) * a);
	}
}
static void ip_own_restore(void)
{
	s32 i, k;
	for (i = 0; i < g_ip_own_n; i++)
	{
		s_ip_own *o = &g_ip_own[i];
		if (!o->obj) continue;
		for (k = 0; k < o->n; k++) gw32(o->obj + g_ip_own_off[k], o->bk[k]);
		o->obj = 0;
	}
	for (i = 0; i < g_ip_hk_n; i++)                                        /* 6i.50 */
	{
		s_ip_hk *h = &g_ip_hk[i];
		if (!h->set) continue;
		for (k = 0; k < 3; k++) gw32(h->motion + 0x70u + 4u * (u32)k, h->bk[k]);
		h->set = 0;
	}
	g_ip_own_set = 0;
}
/* ---- 6i.189: THE IN-BETWEEN FRAMES, CHEAPER (your logs of 10 Nov, a busy scene: the tick's snapshot 1.6 - 2.0 ms; a
 * frame's save 0.4 - 0.6, blend 0.8 - 1.9, restore 0.1 - 0.2 ms; 80 - 120 objects and 900 - 1500 parts tracked) ----
 * What is drawn is the same. Three things made it slow, none of them the arithmetic:
 *  1. THE SNAPSHOT walked the whole object table four times - once for your body, once for the rest of yours, once for
 *     what is near, once for what is further - doing every slot's checks each time (a campaign level has a couple of
 *     thousand slots). Now ONE walk sorts the slots that qualify into those four lists (IP_CAND each), and the four
 *     passes run over the lists - the same objects in the same order, with the same room rules.
 *  2. EVERY WORD of every part was read and written through the mod's general memory accessor, which is declared so the
 *     compiler may not combine or reorder accesses (right for reading the game's state; needless for copying a block the
 *     mod has just located). The copies - snapshot, save, restore - and the blend's writes now go through plain pointers.
 *  3. THE BLEND worked on every part of an object if any part of it had moved. A part that is where it was a tick ago
 *     (bit for bit) is now left as it is: blending it gives the same part back.
 * Not done (bigger changes, to be decided on the next log): leaving out what is off screen; blending less often far away. */
#define IP_CAND 1024
#ifdef HOST
#define IP_RD(a) M32(a)
#define IP_WR(a, v) gw32((a), (v))
#define IP_WRF(a, v) gwf((a), (v))
#else
typedef u32 __attribute__((may_alias)) ip_u32;
typedef float __attribute__((may_alias)) ip_f32;
#define IP_RD(a) (*(const ip_u32 *)(u32)(a))
#define IP_WR(a, v) (*(ip_u32 *)(u32)(a) = (v))
#define IP_WRF(a, v) (*(ip_f32 *)(u32)(a) = (v))
#endif
static u16 g_ip_cand[4][IP_CAND];
static void ip_snapshot(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), pool = M32(ADDR_OBJECTS_PTR), data, count, i, ud[4], up[4], ci;
	float ref[3];
	s32 u, pass, cn[4] = { 0, 0, 0, 0 };
	g_ip_valid = 0;
	if (!ip_on() || !ip_alloc() || !ptr_ok(time) || !ptr_ok(pool)) return;
	data = M32(pool + 0x44); count = M32(pool + 0x20);
	if (!ptr_ok(data) || M32(pool + 0x24) != 12 || count > 8192) return;
	view_camera(ref, 0, 0);
	for (u = 0; u < 4; u++)                                                   /* your units (datums) and what each rides */
	{
		u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR), pl, uob;
		ud[u] = up[u] = 0xFFFFFFFFu;
		if (!ptr_ok(lp) || !(pl = pool_entry(ADDR_PLAYERS_PTR, (s32)M32(lp + 0xC + 4 * (u32)u), 0x30))) continue;
		ud[u] = M32(pl + 0x2C);
		if ((s32)ud[u] == -1 || !(uob = object_from_datum((s32)ud[u]))) { ud[u] = 0xFFFFFFFFu; continue; }
		up[u] = M32(uob + 0x14);
	}
	g_ip_own_n = 0; g_ip_hk_n = 0;                                            /* 6i.48: your units' fields, and what each rides */
	for (u = 0; u < 4; u++)
		if (ud[u] != 0xFFFFFFFFu && (ip_own_take(ud[u]), up[u] != 0xFFFFFFFFu)) { ip_own_take(up[u]); ip_hk_take(ud[u]); }   /* 6i.50: seated - the vehicle's body */
	g_ip_n = 0; g_ip_nodes = 0; g_ip_left_out = 0;
	{ s32 q; for (q = 0; q < IP_SAVED_MAX; q++) g_ip_saved[q] = 0; }       /* (no pose of this tick read yet) */
	for (u = 0; u < 4; u++)                                                   /* 6i.15: the view each tick was made with */
	{
		/* 6i.30: the game turns your body in a tick by the view of the frame drawn right after it - not the frame before
		 * (6i.15 took that one: a frame's turn behind, so the legs swung between two places a frame apart - the "two
		 * images"). So the tick's view is filled in by the next frame's camera (ip_view); one no frame was drawn for
		 * since (frames slower than ticks) takes the last frame's. */
		float h[2];
		if (g_ip_tcam_want[u])
		{
			if (g_ip_camok[u] && ip_cam_heading(u, h)) { g_ip_tcam[u][1][0] = h[0]; g_ip_tcam[u][1][1] = h[1]; if (g_ip_tcam_n[u] < 2) g_ip_tcam_n[u]++; }
			else g_ip_tcam_n[u] = 0;
		}
		g_ip_tcam[u][0][0] = g_ip_tcam[u][1][0]; g_ip_tcam[u][0][1] = g_ip_tcam[u][1][1];
		g_ip_tcam_want[u] = 1;
	}
	/* 6i.9: your first-person arms and weapon - their node matrices (camera-relative, from the first-person animation each
	 * tick, unknown_165ce5.cpp s_first_person_weapon) are not an object's: the first-person setup (0x3d2c0) builds the
	 * drawn models from them and the frame's camera, so the camera followed smoothly but the arms stepped at 30. Each
	 * active weapon's block is an entry of its own, marked 0xFFFF in the datum's top half (user, weapon below), ahead of
	 * everything else */
	/* 6i.27: your body first - the legs you see looking down are drawn from its nodes; when the 128 ran out it could be
	 * left out behind your weapons and the first-person arms, and then stepped at 30 (and the legs' turn, which needs
	 * it, did nothing). Now: your body, the first-person arms, the rest of yours (seat, weapons), within 20, out to 80. */
	g_ip_body_n = -1;
	/* 6i.189: one walk of the table - each slot that qualifies goes on the list of the pass that would have taken it */
	for (i = 0; i < count && i < 0x10000u; i++)
	{
		u32 h = data + i * 12, salt = M32(h) & 0xffff, obj, n, noff, k, datum, mine = 0, par;
		float dx, dy, dz, d2, rad, rn, rf;
		s32 cls, live;
		u8 type;
		if (!salt) continue;                                                 /* a free slot */
		obj = M32(h + 8);
		if (!ptr_ok(obj)) continue;
		n = M16(obj + 0x114); noff = M16(obj + 0x116);
		if (!n || n % 0x34 || !noff || noff > 0x4000) continue;
		datum = (salt << 16) | i; par = M32(obj + 0x14);
		for (u = 0; u < 4; u++)
			if (ud[u] != 0xFFFFFFFFu && (datum == ud[u] || datum == up[u] || par == ud[u])) mine = datum == ud[u] ? 2 : 1;   /* it (2), its seat, its weapons */
		if (mine) cls = mine == 2 ? 0 : 1;
		else
		{
			type = M8(obj + 0xAA);
			live = type == 0 || type == 1 || type == 7 || type == 12 || par != 0xFFFFFFFFu;   /* (6i.2 / 6i.9: see the pass below) */
			for (k = 0; !live && k < 6; k++) if (M32(obj + 0x88 + 4 * k) & 0x7FFFFFFFu) live = 1;
			if (!live) continue;
			dx = MF(obj + 0x30) - ref[0]; dy = MF(obj + 0x34) - ref[1]; dz = MF(obj + 0x38) - ref[2]; rad = MF(obj + 0x3C);
			d2 = dx * dx + dy * dy + dz * dz; rn = rad + IP_NEAR; rf = rad + IP_RANGE;
			if (rn > 0.f && d2 < rn * rn) cls = 2;
			else if (rf > 0.f && d2 < rf * rf) cls = 3;
			else continue;
		}
		if (cn[cls] < IP_CAND) g_ip_cand[cls][cn[cls]++] = (u16)i; else g_ip_left_out++;   /* (more than a list holds: left out, as when the room runs out) */
	}
	for (pass = 0; pass < 4; pass++)
	{
		if (pass == 1)
		{
			for (u = 0; u < 4; u++)
			{
				u32 fp = M32(ADDR_FP_USERS_PTR), wi;
				if (ud[u] == 0xFFFFFFFFu || !ptr_ok(fp)) continue;
				for (wi = 0; wi < 2; wi++)
				{
					u32 w = fp + (u32)u * 0x20CCu + 0xCu + wi * 0x1010u, n = M32(w + 0x304), src = w + 0x308, k, at;
					if (!(M32(w) & 1) || M32(w + 4) == 0xFFFFFFFFu || !n || n > 64) continue;   /* active, armed */
					if (g_ip_n >= g_ip_ent_cap || g_ip_nodes + (s32)n > g_ip_node_cap) { g_ip_left_out++; continue; }
					at = (u32)g_ip_nodes * 13u;
					for (k = 0; k < n * 13u; k++) g_ip_prev[at + k] = IP_RD(src + 4 * k);
					g_ip_ent[g_ip_n].datum = 0xFFFF0000u | ((u32)u << 8) | wi; g_ip_ent[g_ip_n].first = (u32)g_ip_nodes; g_ip_ent[g_ip_n].count = n; g_ip_ent[g_ip_n].cur = 0;
					g_ip_fpw[u][wi] = M32(w + 4);                                   /* 6i.204: the weapon these are the nodes of */
					g_ip_nodes += (s32)n; g_ip_n++;
				}
			}
		}
		/* 6i.2: only what can change between two ticks - a biped, vehicle, machine or creature (they move on their own), or
		 * anything with a velocity or spin; scenery and weapons at rest never need drawing between (your 6i.1 log: 64
		 * objects of one or two nodes, crates and weapons on Headlong, filled the room). 6i.9: or carried / mounted on
		 * something. Pass 2: within 20 of the view by its bounding sphere; 3: the rest out to 80 (past: as it is). */
		for (ci = 0; ci < (u32)cn[pass]; ci++)
		{
			u32 h, salt, obj, n, noff, src, w, k, datum;
			i = g_ip_cand[pass][ci];
			h = data + i * 12; salt = M32(h) & 0xffff; obj = M32(h + 8);
			n = M16(obj + 0x114) / 0x34u; noff = M16(obj + 0x116);
			datum = (salt << 16) | i;
			if (!pass) g_ip_body_n = (s32)n;                                  /* 6i.27: your body's nodes (the first user's) */
			if (g_ip_n >= g_ip_ent_cap || g_ip_nodes + (s32)n > g_ip_node_cap) { g_ip_left_out++; if (!pass) g_ip_body_n = -2; continue; }   /* no room */
			src = obj + noff; w = (u32)g_ip_nodes * 13u;
			for (k = 0; k < n * 13u; k++) g_ip_prev[w + k] = IP_RD(src + 4 * k);
			g_ip_ent[g_ip_n].datum = datum; g_ip_ent[g_ip_n].first = (u32)g_ip_nodes; g_ip_ent[g_ip_n].count = n; g_ip_ent[g_ip_n].cur = 0;
			g_ip_ent[g_ip_n].fwd[0] = MF(obj + 0x70); g_ip_ent[g_ip_n].fwd[1] = MF(obj + 0x74);
			g_ip_ent[g_ip_n].org[0] = MF(obj + 0x64); g_ip_ent[g_ip_n].org[1] = MF(obj + 0x68); g_ip_ent[g_ip_n].org[2] = MF(obj + 0x6C);
			g_ip_nodes += (s32)n; g_ip_n++;
		}
	}
	g_ip_tick = M32(time + 8);
	g_ip_valid = 1;
}
/* 6i.129: the blend of one block's nodes (13 floats each: scale, forward, left, up, position).
 * Your 6i.128 report: with Performance Mode on, the rocket launcher's barrels vanished for one frame at the end of its
 * animation. The barrels turn round in a single tick there, and a straight blend between two matrices that far apart passes
 * through the middle: the axes shrink (at 180 deg to nothing). A node that turned more than ~75 deg between the two ticks
 * (its axes' dot products summed under half their squared lengths' sum / 2 - 1.5 for unit axes, 1 + 2 cos) now keeps this
 * tick's matrix, as it is - the same rule as Harc's Fix 1b on MCC, per node rather than per model, so the rest of the
 * weapon stays smooth. The first-person arms and weapon also keep their axes' lengths through a smaller turn (a blend
 * between two directions is shorter than either: 60 deg - 13 % smaller halfway). */
static u32 g_ip_snaps;
/* 6i.204: YOUR VIDEO - drawing a weapon, one frame at the very start has the weapon in the wrong place, then it is
 * right. In the frames: the first frame of the draw shows the weapon large and level at the right of the screen, the
 * next ones show it rising from below as it should. The first-person arms and weapon are blended between ticks from
 * one block of nodes for each hand. That block holds whichever weapon the hand has: on the tick the hand takes
 * another weapon, the nodes of the tick before are the OTHER weapon's - a pose that has nothing to do with the new
 * one - and with the same number of nodes (which is all the blend checked) the frame drawn between them is half one
 * weapon's pose and half the other's. Now the weapon each block held is noted with the tick's snapshot (the block's
 * weapon_index, +4), and a block whose weapon has changed since is drawn as the game has it, not blended, for that
 * tick. That it is the weapon changing, and not the same weapon starting a new animation from a distant pose, is my
 * reading of the frames; the log counts the times it acts ("a first-person weapon changed between two ticks"). */
/* (g_ip_fpw, g_ip_fpw_n: declared with the snapshot's own variables, above) */
static void ip_blend_nodes(u32 cur, const u32 *pv, const u32 *bk, u32 count, float a, s32 fp)
{
	u32 n, k;
	for (n = 0; n < count; n++)
	{
		const u32 *p0 = pv + n * 13u, *p1 = bk + n * 13u;
		u32 nd = cur + n * 0x34u, diff = 0;
		float A[13], B[13], t, sa = 0.f, sb = 0.f;
		union { u32 u; float f; } x;
		for (k = 0; k < 13; k++) diff |= p0[k] ^ p1[k];
		if (!diff) continue;                                                 /* 6i.189: this part is where it was a tick ago: nothing to blend */
		for (k = 0; k < 13; k++) { x.u = p0[k]; A[k] = x.f; x.u = p1[k]; B[k] = x.f; }
		t = 0.f;
		for (k = 1; k <= 9; k++) { t += A[k] * B[k]; sa += A[k] * A[k]; sb += B[k] * B[k]; }
		if (t < 0.25f * (sa + sb)) { g_ip_snaps++; continue; }                /* turned too far: this tick's (it is what is there already - 6i.189: not written again) */
		for (k = 0; k < 13; k++) IP_WRF(nd + 4 * k, A[k] + (B[k] - A[k]) * a);
		if (fp)
			for (k = 1; k <= 7; k += 3)                                         /* forward, left, up: their lengths kept */
			{
				float la = fsqrt(A[k] * A[k] + A[k + 1] * A[k + 1] + A[k + 2] * A[k + 2]), lb = fsqrt(B[k] * B[k] + B[k + 1] * B[k + 1] + B[k + 2] * B[k + 2]);
				float v0 = MF(nd + 4 * k), v1 = MF(nd + 4 * k + 4), v2 = MF(nd + 4 * k + 8), l = fsqrt(v0 * v0 + v1 * v1 + v2 * v2), s;
				if (!(l > 1e-6f)) continue;
				s = (la + (lb - la) * a) / l;
				gwf(nd + 4 * k, v0 * s); gwf(nd + 4 * k + 4, v1 * s); gwf(nd + 4 * k + 8, v2 * s);
			}
	}
}
#ifdef HOST
void host_ip_blend_nodes(u32 cur, const u32 *pv, const u32 *bk, u32 count, float a, s32 fp) { ip_blend_nodes(cur, pv, bk, count, a, fp); }
u32 host_ip_snaps(void) { return g_ip_snaps; }
#endif
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
	g_ip_a = a;                                                               /* 6i.15 */
	g_ip_frame = 1;
	for (u = 0; u < 4; u++) uo[u] = user_object(u);
	for (i = 0; i < (u32)g_ip_n; i++)
	{
		s_ip_ent *e = &g_ip_ent[i];
		u32 obj = object_from_datum((s32)e->datum), cur, same = 1, *pv = g_ip_prev + e->first * 13u, *bk = g_ip_back + e->first * 13u;
		union { u32 u; float f; } x, y;
		float d0, d1, d2;
		e->cur = 0;
		if ((e->datum >> 16) == 0xFFFFu)                                      /* 6i.9: a first-person weapon's block */
		{
			u32 fp = M32(ADDR_FP_USERS_PTR), w;
			if (!ptr_ok(fp)) continue;
			w = fp + ((e->datum >> 8) & 3u) * 0x20CCu + 0xCu + (e->datum & 1u) * 0x1010u;
			if (!(M32(w) & 1) || M32(w + 0x304) != e->count) continue;
			if (M32(w + 4) != g_ip_fpw[(e->datum >> 8) & 3u][e->datum & 1u]) { if (!(i < IP_SAVED_MAX && g_ip_saved[i])) g_ip_fpw_n++; if (i < IP_SAVED_MAX) g_ip_saved[i] = 1; continue; }   /* 6i.204: another weapon than at the last tick - as it is */
			cur = w + 0x308; obj = 0;
		}
		else
		{
			if (!obj || M16(obj + 0x114) != e->count * 0x34) continue;        /* gone, or another object */
			cur = obj + M16(obj + 0x116);
		}
		for (u = 0; u < 4; u++) if (obj && uo[u] && uo[u] == obj)          /* a local player's unit: its blended facing */
		{
			g_ip_uhas[u] = 1;
			g_ip_ufwd[u][0] = e->fwd[0] + (MF(obj + 0x70) - e->fwd[0]) * a; g_ip_ufwd[u][1] = e->fwd[1] + (MF(obj + 0x74) - e->fwd[1]) * a;
		}
		x.u = pv[10]; d0 = MF(cur + 0x28) - x.f; x.u = pv[11]; d1 = MF(cur + 0x2C) - x.f; x.u = pv[12]; d2 = MF(cur + 0x30) - x.f;
		if (!(d0 * d0 + d1 * d1 + d2 * d2 < 16.f)) continue;                 /* a jump of 4 units or more: at once */
		{ u32 ts = tsc_lo(), w = e->count * 13u; for (k = 0; k < w; k++) { bk[k] = IP_RD(cur + 4 * k); if (bk[k] != pv[k]) same = 0; } if (i < IP_SAVED_MAX) g_ip_saved[i] = 1; g_ipt_save += tsc_lo() - ts; }
		if (same) continue;                                                    /* still: nothing to blend */
		e->cur = cur; g_ip_blended++;                                          /* (before writing: a fault still puts it back) */
		{ u32 ts = tsc_lo(); ip_blend_nodes(cur, pv, bk, e->count, a, (e->datum >> 16) == 0xFFFFu); g_ipt_blend += tsc_lo() - ts; }   /* 6i.129: a node turned too far keeps its tick (was a straight blend) */   /* 6i.129: a node turned too far keeps its tick (was a straight blend) */
		for (u = 0; u < 4; u++) if (obj && uo[u] == obj)                   /* the camera's offset: its origin's (on foot - the */
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
	u32 i, k, ts = tsc_lo();
	g_ip_frame = 0;
	g_ipt_objs += (u32)g_ip_n; g_ipt_parts += (u32)g_ip_nodes; g_ipt_blended += (u32)g_ip_blended;   /* 6i.182: this frame's load */
	if (!g_ip_blended) return;
	for (i = 0; i < (u32)g_ip_n; i++)
	{
		s_ip_ent *e = &g_ip_ent[i];
		u32 *bk = g_ip_back + e->first * 13u;
		if (!e->cur) continue;
		for (k = 0; k < e->count * 13u; k++) IP_WR(e->cur + 4 * k, bk[k]);
		e->cur = 0;
	}
	g_ip_blended = 0;
	g_ipt_rest += tsc_lo() - ts;
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
	/* 6i.15: the first-person body is built from the unit's nodes (blended between the last two ticks, so a tick behind),
	 * but the view turns every frame: seen from the view, the legs swung by up to a tick's turn and back each frame - the
	 * jitter turning. 6h.26 turned them all the way to the view (ahead of where the game puts them: 6i.9 then kept that for
	 * slow motion only). Now they're turned by how far the view has turned past the view the ticks were made with (the
	 * view's heading as the last two ticks began, blended as far as the nodes are): the legs keep the game's own lag behind
	 * the view, steadily, at any speed. */
	s32 u = (s32)M32(ADDR_VIEW_USER);
	u32 obj, n, noff, m, k;
	float hx, hy, bx, by, l, cs, sn, px, py, h[2];
	g_ip_fp_n = 0;
	if (u < 0 || u > 3 || !g_ip_frame || !g_ip_camok[u] || g_tp_applied[u] || !ip_on() || g_ip_tcam_n[u] < 2) return;
	obj = user_object(u);
	if (!obj || (s32)M32(obj + 0x14) != -1) return;                         /* on foot */
	n = M16(obj + 0x114); noff = M16(obj + 0x116);
	if (!n || n % 0x34 || !noff || noff > 0x4000 || n / 0x34 > IP_FP_CAP) return;
	n /= 0x34;
	if (!ip_cam_heading(u, h)) return;
	hx = h[0]; hy = h[1];                                                     /* the view now */
	bx = g_ip_tcam[u][0][0] + (g_ip_tcam[u][1][0] - g_ip_tcam[u][0][0]) * g_ip_a;   /* the view the nodes were made with */
	by = g_ip_tcam[u][0][1] + (g_ip_tcam[u][1][1] - g_ip_tcam[u][0][1]) * g_ip_a;
	l = fsqrt(bx * bx + by * by); if (l < 1e-3f) return; bx /= l; by /= l;
	cs = bx * hx + by * hy; sn = bx * hy - by * hx;                        /* the turn between them */
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
/* ---- 6i.128: Harc's FP Scale Fix (HCM FPScaleFix.cpp) - the first-person body at the game's own field of view ----
 * The first-person body (the legs) is the unit's own nodes drawn through the world's projection, so a wider Field of View
 * shrinks it and narrower grows it (the arms and weapon are built around the camera). As in Harc's fix, every node is
 * widened about the camera's line of sight by s = tan(half the view now) / tan(half the game's own view): across the view
 * x s, along it unchanged - v' = s v + (1 - s)(f.v) f for the position (from the camera) and each axis. The legs then
 * fill the screen as they would at the game's own view. After the legs' turn (ip_fp_turn), put back with it (ip_fp_back).
 * (Harc's fix also moves a "tuck" constant of MCC's own: Halo 2 on the Xbox has no such step.) */
static void fp_scale_nodes(u32 m, u32 n, float s, const float *c, const float *f)
{
	u32 k, v;
	for (k = 0; k < n; k++)
	{
		u32 nd = m + k * 0x34;
		float d[3], dp;
		for (v = 0; v < 3; v++) d[v] = MF(nd + 0x28 + 4 * v) - c[v];
		dp = (f[0] * d[0] + f[1] * d[1] + f[2] * d[2]) * (1.f - s);
		for (v = 0; v < 3; v++) gwf(nd + 0x28 + 4 * v, c[v] + s * d[v] + dp * f[v]);
		for (v = 0x04; v <= 0x1C; v += 0x0C)                                    /* forward, left, up */
		{
			float a[3] = { MF(nd + v), MF(nd + v + 4), MF(nd + v + 8) }, ap = (f[0] * a[0] + f[1] * a[1] + f[2] * a[2]) * (1.f - s);
			gwf(nd + v, s * a[0] + ap * f[0]); gwf(nd + v + 4, s * a[1] + ap * f[1]); gwf(nd + v + 8, s * a[2] + ap * f[2]);
		}
	}
}
#ifdef HOST
void host_fp_scale_nodes(u32 m, u32 n, float s, const float *c, const float *f) { fp_scale_nodes(m, n, s, c, f); }
#endif
/* (6i.224: the leg report is gone - the body's fix saying on the Field of View's menu line, and in the log, which of its
 * own tests had turned it away. It found the fault on the 360 in 6i.187. g_fp_why stays: the arms' fix goes by it.) */
static s32 g_fp_why;
/* ---- 6i.183: the game's memory and its caches, in the log (for the 128 MB work: RESEARCH_128MB.txt) ----
 * Once for each level, when it is first being played: the game's own region (the stage in use, what is used and where
 * the stage ends: ADDR_PHYS_GLOBALS +0, +0xC[stage], +0x20[stage]), the size of each of its caches - texture, geometry,
 * animation, sound (each cache's allocator: pages +0x30 << page shift +0x34) - and what the console has free. The
 * texture cache has no size of its own (it is given what the region has left when the level has loaded): this is where
 * that figure can be read. Nothing is changed. ---- */
static u32 g_cache_scn;
static u32 kt_query(u32 *st);
static s32 wire_in_game(void);
static u32 cache_kb(u32 at)
{
	u32 ph = at ? M32(at) : 0, pages, shift;
	if (!ptr_ok(ph)) return 0;
	pages = M32(ph + 0x30); shift = M32(ph + 0x34);
	if (pages > 0x100000u || shift < 8u || shift > 20u) return 0;
	return shift >= 10u ? pages << (shift - 10u) : pages >> (10u - shift);
}
static void cache_log_now(void)
{
	u32 st[9], stage, lo, hi;
	s32 at;
	stage = M32(ADDR_PHYS_GLOBALS);
	if (stage > 4u) return;
	lo = M32(ADDR_PHYS_GLOBALS + 0xC + 4u * stage); hi = M32(ADDR_PHYS_GLOBALS + 0x20 + 4u * stage);
	at = lput(0, "memory: the game's region - stage "); at = lnum(at, stage); at = lput(at, ", used to "); at = lhex(at, lo); at = lput(at, ", ends "); at = lhex(at, hi);
	at = lput(at, ", left "); at = lnum(at, hi >= lo ? (hi - lo) / 1024u : 0u); lput(at, " KB");
	log_write();
	at = lput(0, "memory: caches KB - texture "); at = lnum(at, cache_kb(ADDR_TEX_PHYS)); at = lput(at, ", geometry "); at = lnum(at, cache_kb(ADDR_GEO_PHYS));
	at = lput(at, ", animation "); at = lnum(at, cache_kb(ADDR_ANIM_PHYS)); at = lput(at, ", sound "); at = lnum(at, cache_kb(ADDR_SND_PHYS));
	for (lo = 0; lo < 9u; lo++) st[lo] = 0;
	if (kt_query(st) == 0u) { at = lput(at, "; the console: "); at = lnum(at, st[1] / 256u); at = lput(at, " MB, free "); at = lnum(at, st[2] * 4u); lput(at, " KB"); }
	log_write();
}
static void cache_log(void)                      /* once for each level, while it is being played */
{
	u32 scn = ADDR_SCENARIO ? M32(ADDR_SCENARIO) : 0;
	if (!SPOOKY_LOG || !ADDR_PHYS_GLOBALS || !ptr_ok(scn) || scn == g_cache_scn || on_main_menu() || !wire_in_game()) return;
	g_cache_scn = scn;
	cache_log_now();
}
#ifdef HOST
void host_cache_log(void) { cache_log_now(); }
#endif
static void fp_fov_scale(void)
{
	s32 u = (s32)M32(ADDR_VIEW_USER);
	u32 obj, n, noff, m, k;
	float s, c[3], f[3], l;
	if (u < 0 || u > 3) { g_fp_why = 1; return; }
	s = g_fp_s[u];
#define FP_NO(code) do { if (u == 0) g_fp_why = (code); return; } while (0)
	if (!(s > 1.0001f || s < 0.9999f) || !(s > 0.1f && s < 10.f)) FP_NO(2);
	if (g_tp_applied[u]) FP_NO(3);
	obj = user_object(u);
	if (!obj || (s32)M32(obj + 0x14) != -1) FP_NO(4);                       /* on foot (no body in a vehicle) */
	n = M16(obj + 0x114); noff = M16(obj + 0x116);
	if (!n || n % 0x34 || !noff || noff > 0x4000 || n / 0x34 > IP_FP_CAP) FP_NO(5);
	n /= 0x34;
	for (k = 0; k < 3; k++) { c[k] = MF(ADDR_RENDER_CAMERA + 4 * k); f[k] = MF(ADDR_RENDER_CAMERA + 0xC + 4 * k); }
	l = fsqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	if (!(l > 0.5f && l < 2.f)) FP_NO(6);
	f[0] /= l; f[1] /= l; f[2] /= l;
	m = obj + noff;
	if (!g_ip_fp_n) { for (k = 0; k < n * 13u; k++) g_ip_fpbk[k] = M32(m + 4 * k); g_ip_fp_at = m; g_ip_fp_n = n; }   /* the legs' turn saved them otherwise */
	if (g_ip_fp_at != m) FP_NO(7);
#undef FP_NO
	fp_scale_nodes(m, n, s, c, f);
	if (u == 0) g_fp_why = 0;
}
#ifdef HOST
s32 host_fp_why(s32 *thousandths) { *thousandths = 0; return g_fp_why; }
#endif
/* ---- 6i.195: THE FIELD OF VIEW AND YOUR ARMS AND WEAPON (your note: apply the legs' fix to the first-person arms and
 * gun) ----
 * Like the legs, the first-person arms and weapon are drawn through the world's projection: a wider Field of View makes
 * them smaller and pushes them out, a narrower one larger. The legs' fix widens the body's nodes about the camera's line
 * of sight by s = tan(half the view now) / tan(half the game's own) so they fill the screen as at the game's own view.
 * The arms and weapon are not the body's nodes: the first-person set-up (function_3d2c0 -> first_person_weapons_get_models,
 * v1.0 0x165f16) builds up to four drawn models into its own table (the count at ADDR_FP_MODELS, the models right after
 * it: 12 bytes - render model, object, a flag - then 64 node matrices of 0x34 bytes, in WORLD space; model 0 the arms, then
 * a model for each weapon in hand, then the body, which is a copy of the unit's own nodes). So, right after that set-up
 * has run (ip_fp_after - where the legs' nodes are put back), the same widening is applied to the arms' and the weapons'
 * models, with the same factor, camera and line of sight; the body's model is left (it was built from nodes already
 * widened). The table is rebuilt every frame: nothing to put back. Only when the legs' fix itself ran this frame (on
 * foot, at your eye, the Field of View not the game's own): otherwise nothing is touched. ---- */
#ifdef HOST
#define FP_MODEL_ARRAY 0xEB000u
#else
#define FP_MODEL_ARRAY (ADDR_FP_MODELS + 4u)
#endif
#define FP_MODEL_BYTES 0xD0Cu                     /* 12 + 64 x 0x34 */
static s32 g_fp_models_done;                      /* how many models were widened this frame (the tests, the log) */
static void fp_fov_models(void)
{
	s32 u = (s32)M32(ADDR_VIEW_USER);
	u32 count, i, k, unit;
	float s, c[3], f[3], l;
	g_fp_models_done = 0;
	if (u < 0 || u > 3 || !ADDR_FP_MODELS) return;
	if (u == 0 && g_fp_why) return;                                          /* the legs' fix did not run for this view: nor this */
	s = g_fp_s[u];
	if (!(s > 1.0001f || s < 0.9999f) || !(s > 0.1f && s < 10.f) || g_tp_applied[u]) return;
	count = M32(ADDR_FP_MODELS);
	if (!count || count > 4u) return;
	unit = M32(ADDR_FP_OBJECT);
	for (k = 0; k < 3; k++) { c[k] = MF(ADDR_RENDER_CAMERA + 4 * k); f[k] = MF(ADDR_RENDER_CAMERA + 0xC + 4 * k); }
	l = fsqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	if (!(l > 0.5f && l < 2.f)) return;
	f[0] /= l; f[1] /= l; f[2] /= l;
	for (i = 0; i < count; i++)
	{
		u32 model = FP_MODEL_ARRAY + i * FP_MODEL_BYTES, n;
		if (i > 0u && M32(model + 4) == unit) continue;                      /* the body: built from the unit's nodes, widened already */
		if (M32(model) == 0xFFFFFFFFu) continue;                              /* no render model: not drawn */
		for (n = 0; n < 64u; n++)                                            /* (a slot the model does not use holds no sane matrix: left) */
		{
			u32 nd = model + 12u + n * 0x34u;
			float px = MF(nd + 0x28) - c[0], py = MF(nd + 0x2C) - c[1], pz = MF(nd + 0x30) - c[2], sc = MF(nd);
			if (!(px > -64.f && px < 64.f && py > -64.f && py < 64.f && pz > -64.f && pz < 64.f && sc > 0.01f && sc < 100.f)) continue;
			fp_scale_nodes(nd, 1, s, c, f);
		}
		g_fp_models_done++;
	}
}
#ifdef HOST
s32 host_fp_models_done(void) { return g_fp_models_done; }
void host_fp_models(void) { fp_fov_models(); }
void host_fp_state(s32 user, float s, s32 why) { g_fp_s[user] = s; if (user == 0) g_fp_why = why; }
#endif
void ip_fp_before(void) { guarded_call(ip_fp_turn); guarded_call(fp_fov_scale); }
void ip_fp_after(void) { guarded_call(ip_fp_back); guarded_call(fp_fov_models); }

/* ---- 6i.128: Harc's Animation Fixes (HCM AnimationFixes.cpp, Fix 3) - the rocket launcher's animation pop ----
 * A fault in the rocket launcher's shipped animation data (the "grenade throw pop"): 51 eight-byte pieces of one animation,
 * found by its 40-byte header, rewritten. On the Xbox animation data isn't in the tags: it's streamed into the "xbox
 * animation cache" (src/animation_cache.cpp) - each load (the "xbox animation" pool, v1.0 [0x4E3B48], 0x10 each: salt, done
 * +4, resource +0xC) holds a resource (size +4, block +0xC) in a block of the cache's memory (the physical object
 * [0x4E3B54]: page shift +0x34, blocks +0x64 -> +0x44, 0x18 each, offset +8; the memory [0x4E3B50]). Every finished load is
 * searched once (a few per frame, at most 128 KB of searching a frame) and patched in place - again whenever it's reloaded.
 * Only where every one of the 51 pieces reads as the original (or already fixed): anything else is left alone. The rest
 * of Harc's fixes are for MCC's own 60-tick interpolation (its codec snap, its interpolation cache, its legs and crouch
 * paths) - the Xbox has none of it; Performance Mode's interpolation already blends the legs (the unit's nodes) and the
 * crouch (6i.48). */
static const u8 c_af_sig[40] = { 0x03, 0x15, 0x04, 0x00, 0x28, 0x96, 0xCF, 0x38, 0x33, 0x36, 0x10, 0x3F, 0x00, 0x1D, 0x00, 0x00, 0x40, 0x25, 0x00, 0x00, 0x60, 0x01, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0xB0, 0x00, 0x00, 0x00, 0x73, 0xBD, 0x5E, 0xFD, 0xAC, 0x03, 0xC3, 0x92 };
static const struct { u16 rel; u8 fix[8], orig[8]; } c_af_patch[51] = {
	{ 0x180, { 0x15, 0xA3, 0xA9, 0xF5, 0x4E, 0xBA, 0x3B, 0xCB }, { 0x25, 0x17, 0xE2, 0xE5, 0xA5, 0x12, 0x46, 0x86 } },
	{ 0x440, { 0x00, 0x00, 0xF9, 0x5D, 0x00, 0x00, 0x18, 0xA9 }, { 0x00, 0x00, 0xF8, 0x5D, 0x00, 0x00, 0x19, 0xA9 } },
	{ 0x700, { 0xEF, 0x73, 0x09, 0xDD, 0xFB, 0xEF, 0xBF, 0xD9 }, { 0x39, 0xE4, 0x32, 0xB3, 0x76, 0xFF, 0x74, 0x9D } },
	{ 0x188, { 0xE3, 0xA3, 0xEF, 0xF4, 0x71, 0xBA, 0xCE, 0xC9 }, { 0x95, 0x18, 0xAA, 0xE5, 0x7D, 0x13, 0xBD, 0x86 } },
	{ 0x448, { 0x00, 0x00, 0x67, 0x5E, 0x00, 0x00, 0x91, 0xA9 }, { 0x00, 0x00, 0x67, 0x5E, 0x00, 0x00, 0x91, 0xA9 } },
	{ 0x708, { 0x42, 0x73, 0x0C, 0xDC, 0xDA, 0xF0, 0x4B, 0xD8 }, { 0x6A, 0xE2, 0x74, 0xB2, 0x36, 0x00, 0x91, 0x9E } },
	{ 0x190, { 0x9A, 0xA6, 0xBB, 0xF2, 0x2C, 0xBB, 0x07, 0xC5 }, { 0x4A, 0x1D, 0x37, 0xE5, 0x77, 0x16, 0x68, 0x88 } },
	{ 0x450, { 0x00, 0x00, 0x91, 0x5F, 0x00, 0x00, 0xDB, 0xAA }, { 0x00, 0x00, 0x91, 0x5F, 0x00, 0x00, 0xDB, 0xAA } },
	{ 0x710, { 0xBC, 0x70, 0xA1, 0xD8, 0x25, 0xF3, 0xBF, 0xD3 }, { 0x75, 0xDC, 0xE3, 0xB0, 0x3F, 0x02, 0xE6, 0xA1 } },
	{ 0x198, { 0x89, 0xAD, 0x59, 0xEE, 0x6B, 0xBE, 0x80, 0xB9 }, { 0x46, 0x28, 0x5D, 0xE5, 0x45, 0x1E, 0x65, 0x8D } },
	{ 0x458, { 0x00, 0x00, 0x43, 0x61, 0x00, 0x00, 0xCC, 0xAC }, { 0x00, 0x00, 0x43, 0x61, 0x00, 0x00, 0xCC, 0xAC } },
	{ 0x718, { 0x45, 0x69, 0x0E, 0xD0, 0xAE, 0xF6, 0x00, 0xCA }, { 0x6E, 0xCE, 0xA7, 0xB0, 0x63, 0x05, 0xD3, 0xA8 } },
	{ 0x1A0, { 0x09, 0xC7, 0x3F, 0xE5, 0x8C, 0xCF, 0xA1, 0x9B }, { 0xEB, 0x44, 0xBE, 0xEA, 0x24, 0x35, 0x96, 0xA4 } },
	{ 0x460, { 0x00, 0x00, 0x49, 0x63, 0x00, 0x00, 0x39, 0xAF }, { 0x00, 0x00, 0x49, 0x63, 0x00, 0x00, 0x39, 0xAF } },
	{ 0x720, { 0x7C, 0x4A, 0x03, 0xB9, 0xF7, 0xFC, 0xEF, 0xB3 }, { 0x47, 0xA9, 0xFE, 0xBC, 0xA0, 0x09, 0x97, 0xBE } },
	{ 0x1A8, { 0xDD, 0xFA, 0xA8, 0xDE, 0xA8, 0xF6, 0xE2, 0x84 }, { 0x7B, 0x5F, 0x98, 0xFA, 0xA5, 0x4D, 0x3E, 0xDD } },
	{ 0x468, { 0x00, 0x00, 0x72, 0x65, 0x00, 0x00, 0xF3, 0xB1 }, { 0x00, 0x00, 0x71, 0x65, 0x00, 0x00, 0xF3, 0xB1 } },
	{ 0x728, { 0x4E, 0x09, 0x83, 0xA4, 0xA6, 0x05, 0x26, 0xA7 }, { 0x33, 0x85, 0x56, 0xE3, 0xA8, 0x0A, 0xD4, 0xEC } },
	{ 0x1B0, { 0xB9, 0x10, 0xCE, 0xDC, 0x2A, 0x08, 0x5A, 0x86 }, { 0xA4, 0x5D, 0x14, 0x03, 0xC0, 0x56, 0x24, 0xF7 } },
	{ 0x470, { 0x00, 0x00, 0x26, 0x6B, 0x00, 0x00, 0xFC, 0xB9 }, { 0x00, 0x00, 0x26, 0x6B, 0x00, 0x00, 0xFC, 0xB9 } },
	{ 0x730, { 0xA2, 0xED, 0xE8, 0x9E, 0x22, 0x0A, 0x48, 0xAF }, { 0xE6, 0x80, 0x95, 0xF5, 0x95, 0x0A, 0x9A, 0x02 } },
	{ 0x1B8, { 0x47, 0x17, 0xBC, 0xDB, 0xAE, 0x0F, 0x80, 0x88 }, { 0x00, 0x54, 0x98, 0x07, 0x45, 0x60, 0x8C, 0xFF } },
	{ 0x478, { 0x00, 0x00, 0x24, 0x72, 0x00, 0x00, 0x15, 0xC6 }, { 0x00, 0x00, 0x24, 0x72, 0x00, 0x00, 0x15, 0xC6 } },
	{ 0x738, { 0xC9, 0xE4, 0xA8, 0x98, 0x29, 0x0D, 0xCC, 0xBA }, { 0xEF, 0x80, 0x15, 0xFC, 0x29, 0x0A, 0xE2, 0x0A } },
	{ 0x1C0, { 0xD9, 0x19, 0x3E, 0xDC, 0x23, 0x13, 0x62, 0x89 }, { 0xDE, 0x4C, 0x92, 0x0A, 0xC3, 0x65, 0xA3, 0x02 } },
	{ 0x480, { 0x00, 0x00, 0x7E, 0x75, 0x00, 0x00, 0x39, 0xCD }, { 0x00, 0x00, 0x7E, 0x75, 0x00, 0x00, 0x39, 0xCD } },
	{ 0x740, { 0x67, 0xE1, 0x36, 0x95, 0x45, 0x0F, 0x47, 0xC2 }, { 0x2E, 0x81, 0xA4, 0xFE, 0xC4, 0x08, 0xD6, 0x0E } },
	{ 0x1C8, { 0x59, 0x18, 0xE6, 0xE0, 0x76, 0x15, 0x28, 0x88 }, { 0xD3, 0x3D, 0x27, 0x0C, 0x63, 0x6F, 0x4A, 0x02 } },
	{ 0x488, { 0x00, 0x00, 0x5C, 0x79, 0x00, 0x00, 0x52, 0xD7 }, { 0x00, 0x00, 0x5B, 0x79, 0x00, 0x00, 0x52, 0xD7 } },
	{ 0x748, { 0xC4, 0xE1, 0x6C, 0x8E, 0x82, 0x0F, 0xBE, 0xCF }, { 0x10, 0x81, 0x96, 0xFE, 0xDB, 0x01, 0x3D, 0x10 } },
	{ 0x1D0, { 0xEB, 0x18, 0xDF, 0xDF, 0x58, 0x15, 0x86, 0x88 }, { 0x46, 0x40, 0x2A, 0x0C, 0xFC, 0x6D, 0xA2, 0x02 } },
	{ 0x490, { 0x00, 0x00, 0xEC, 0x78, 0x00, 0x00, 0x0A, 0xD6 }, { 0x00, 0x00, 0xEB, 0x78, 0x00, 0x00, 0x0A, 0xD6 } },
	{ 0x750, { 0x98, 0xE1, 0xA3, 0x8F, 0xD4, 0x0F, 0x30, 0xCD }, { 0x1B, 0x81, 0xA8, 0xFE, 0xC6, 0x03, 0x3F, 0x10 } },
	{ 0x1D8, { 0xE8, 0x19, 0x01, 0xDE, 0xBF, 0x14, 0x26, 0x89 }, { 0x48, 0x45, 0x12, 0x0C, 0xE6, 0x6A, 0x05, 0x03 } },
	{ 0x498, { 0x00, 0x00, 0xDC, 0x77, 0x00, 0x00, 0x18, 0xD3 }, { 0x00, 0x00, 0xDC, 0x77, 0x00, 0x00, 0x18, 0xD3 } },
	{ 0x758, { 0x65, 0xE1, 0x23, 0x92, 0x47, 0x10, 0x37, 0xC8 }, { 0x3B, 0x81, 0xA9, 0xFE, 0x1E, 0x07, 0x1F, 0x10 } },
	{ 0x1E0, { 0x99, 0x1A, 0x99, 0xDC, 0xD7, 0x13, 0x8F, 0x89 }, { 0xD1, 0x49, 0xDD, 0x0B, 0xD8, 0x67, 0x0A, 0x03 } },
	{ 0x4A0, { 0x00, 0x00, 0xAA, 0x76, 0x00, 0x00, 0x06, 0xD0 }, { 0x00, 0x00, 0xA9, 0x76, 0x00, 0x00, 0x06, 0xD0 } },
	{ 0x760, { 0x94, 0xE1, 0x9C, 0x94, 0x92, 0x10, 0x95, 0xC3 }, { 0x62, 0x81, 0x51, 0xFE, 0xE9, 0x09, 0xC8, 0x0F } },
	{ 0x1E8, { 0x88, 0x1A, 0x01, 0xDC, 0xE8, 0x12, 0x91, 0x89 }, { 0xC3, 0x4B, 0x99, 0x0B, 0x7B, 0x66, 0x6B, 0x02 } },
	{ 0x4A8, { 0x00, 0x00, 0x17, 0x76, 0x00, 0x00, 0xA1, 0xCE }, { 0x00, 0x00, 0x17, 0x76, 0x00, 0x00, 0xA1, 0xCE } },
	{ 0x768, { 0x62, 0xE2, 0xBB, 0x95, 0xA5, 0x10, 0x45, 0xC1 }, { 0x71, 0x81, 0x8A, 0xFD, 0x50, 0x0B, 0x38, 0x0F } },
	{ 0x1F0, { 0x40, 0x1A, 0x31, 0xDB, 0x53, 0x11, 0x84, 0x89 }, { 0x50, 0x4F, 0xF5, 0x0A, 0xD9, 0x63, 0x64, 0x01 } },
	{ 0x4B0, { 0x00, 0x00, 0xD9, 0x74, 0x00, 0x00, 0xC1, 0xCB }, { 0x00, 0x00, 0xD8, 0x74, 0x00, 0x00, 0xC1, 0xCB } },
	{ 0x770, { 0x8E, 0xE3, 0xA7, 0x97, 0x89, 0x10, 0x93, 0xBD }, { 0x83, 0x81, 0x67, 0xFC, 0xF9, 0x0C, 0x38, 0x0E } },
	{ 0x1F8, { 0x72, 0x19, 0x77, 0xDA, 0x5E, 0x0E, 0x2C, 0x89 }, { 0x4B, 0x56, 0x8F, 0x09, 0x0B, 0x5E, 0x89, 0xFF } },
	{ 0x4B8, { 0x00, 0x00, 0x98, 0x71, 0x00, 0x00, 0x03, 0xC5 }, { 0x00, 0x00, 0x97, 0x71, 0x00, 0x00, 0x03, 0xC5 } },
	{ 0x778, { 0x8D, 0xE5, 0xCC, 0x9B, 0xEA, 0x0F, 0x96, 0xB6 }, { 0x92, 0x81, 0x7B, 0xFA, 0xD8, 0x0E, 0x1E, 0x0C } },
	{ 0x200, { 0xF4, 0x16, 0xC2, 0xDA, 0x6A, 0x0A, 0x2A, 0x88 }, { 0x74, 0x5C, 0x94, 0x07, 0x18, 0x58, 0xF1, 0xFB } },
	{ 0x4C0, { 0x00, 0x00, 0x86, 0x6D, 0x00, 0x00, 0xC2, 0xBD }, { 0x00, 0x00, 0x85, 0x6D, 0x00, 0x00, 0xC3, 0xBD } },
	{ 0x780, { 0x48, 0xE9, 0xEF, 0x9F, 0xB5, 0x0E, 0xDD, 0xAF }, { 0x85, 0x81, 0x64, 0xF7, 0x6D, 0x0F, 0x88, 0x08 } },
};
#define AF_MAX 1024
static u32 g_af_key[AF_MAX], g_af_scn;
static s32 g_af_next;
static u32 g_af_checked, g_af_kb, g_af_patched, g_af_odd;
static s32 af_match(u32 p)
{
	s32 j;
	for (j = 0; j < 40; j++) if (M8(p + (u32)j) != c_af_sig[j]) return 0;
	return 1;
}
static void af_apply(u32 h)
{
	s32 k, j, fixed = 1;
	for (k = 0; k < 51; k++)
	{
		s32 isfix = 1, isorig = 1;
		for (j = 0; j < 8; j++) { u8 b = M8(h + c_af_patch[k].rel + (u32)j); if (b != c_af_patch[k].fix[j]) isfix = 0; if (b != c_af_patch[k].orig[j]) isorig = 0; }
		if (!isfix && !isorig) { g_af_odd++; lput(0, "animation fix: an animation with the rocket launcher's header but other data - left alone"); log_write(); return; }
		if (!isfix) fixed = 0;
	}
	if (fixed) return;
	for (k = 0; k < 51; k++) for (j = 0; j < 8; j++) gw8(h + c_af_patch[k].rel + (u32)j, c_af_patch[k].fix[j]);
	g_af_patched++;
	{ s32 at = lput(0, "animation fix: the rocket launcher's grenade-throw animation patched (51 pieces) at "); lhex(at, h); log_write(); }
}
static void af_step(void)
{
	u32 pool, phys, mem, data, blocks, max, shift, scn = ADDR_SCENARIO ? M32(ADDR_SCENARIO) : 0, bytes = 0;
	s32 n;
	if (!ADDR_ANIM_LOADS) return;
	if (scn != g_af_scn)
	{
		if (g_af_scn && g_af_checked) { s32 at = lput(0, "animation fix (last level): "); at = lnum(at, g_af_checked); at = lput(at, " animations searched ("); at = lnum(at, g_af_kb); at = lput(at, " KB), "); at = lnum(at, g_af_patched); at = lput(at, " patched, "); at = lnum(at, g_af_odd); lput(at, " left alone"); log_write(); }
		g_af_scn = scn; g_af_checked = g_af_kb = g_af_patched = g_af_odd = 0;
		memset(g_af_key, 0, sizeof(g_af_key));
	}
	pool = M32(ADDR_ANIM_LOADS); phys = M32(ADDR_ANIM_PHYS); mem = M32(ADDR_ANIM_MEM);
	if (!ptr_ok(pool) || !ptr_ok(phys) || !ptr_ok(mem)) return;
	max = M32(pool + 0x20); data = M32(pool + 0x44); shift = M32(phys + 0x34);
	if (!max || max > AF_MAX || M32(pool + 0x24) != 0x10 || !ptr_ok(data) || shift > 20 || !ptr_ok(M32(phys + 0x64))) return;
	blocks = M32(M32(phys + 0x64) + 0x44);
	if (!ptr_ok(blocks)) return;
	if (g_af_next >= (s32)max) g_af_next = 0;
	for (n = 0; n < (s32)max; n++)
	{
		u32 i = (u32)g_af_next, ld = data + i * 0x10, res, size, off, addr, key, p, end;
		if (!M16(ld)) { g_af_key[i] = 0; g_af_next = (s32)((i + 1) % max); continue; }
		if (!M8(ld + 4)) { g_af_next = (s32)((i + 1) % max); continue; }     /* still loading */
		res = M32(ld + 0xC);
		if (!ptr_ok(res)) { g_af_next = (s32)((i + 1) % max); continue; }
		size = M32(res + 4); off = M32(blocks + (M32(res + 0xC) & 0xffffu) * 0x18u + 8u);
		key = ((u32)M16(ld) ^ (off * 2654435761u)) | 1u;
		if (g_af_key[i] == key) { g_af_next = (s32)((i + 1) % max); continue; }
		if (bytes && bytes + size > 0x20000u) break;                         /* the rest next frame */
		g_af_key[i] = key; g_af_next = (s32)((i + 1) % max);
		if (size < 0x790u || size > 0x400000u) continue;
		addr = mem + (off << shift);
		if (!ptr_ok(addr) || !ptr_ok((addr + size - 1u) & ~3u)) continue;      /* (ptr_ok wants a 4-byte boundary) */
		bytes += size; g_af_checked++; g_af_kb += size >> 10;
		end = addr + size - 0x790u;                                          /* room for the last piece (+0x780) */
		for (p = addr; p <= end; p++)
			if (M32(p) == 0x00041503u && af_match(p)) af_apply(p);
	}
}
#ifdef HOST
void host_af_step(void) { af_step(); }
u32 host_af_patched(void) { return g_af_patched; }
u32 host_af_odd(void) { return g_af_odd; }
const u8 *host_af_sig(void) { return c_af_sig; }
void host_af_piece(s32 k, u32 *rel, const u8 **fix, const u8 **orig) { *rel = c_af_patch[k].rel; *fix = c_af_patch[k].fix; *orig = c_af_patch[k].orig; }
#endif
#ifdef HOST
void gpu_kick(u32 device) { (void)device; }
#else
void gpu_kick(u32 device);                       /* hooks.S: the game's KickPushBuffer, device in edx */
#endif
/* ---- 6i.39: GPU overclock and hard drive speed (tests, from Halo 2 HD by grimdoomer) ----
 * GPU: the NV2A's core clock is its PLL's NDIV (NVPLL_COEFF 0xFD680500, bits 8-15; clock = NDIV x 16.667 / 2 MHz: 28 =
 * 233, the stock). Written live, with the GPU drained first. The fans set to 75% (the SMC over SMBus: the kernel's
 * HalWriteSMBusValue, export 50, found in its export table - Halo 2 doesn't import it) while above stock, back to the
 * SMC's own control at stock. Only if the PLL reads as a real NV2A's (the 360 has none: nothing written there).
 * Hard drive: the drive told to use UDMA mode N (ATA SET FEATURES 0x03, 0x40 + N) with the interrupts held off, the
 * selected device put back after (the DVD drive shares the channel) - what Halo 2 HD does once at startup. UDMA 4 and 5
 * need an 80-wire cable. Both applied when the menu closes, said on screen. */
#ifdef HOST
u32 g_host_nvpll = 0x00011C01u, g_host_fan[4], g_host_fan_n;
#endif
static u32 g_nvpll_stock;
static s32 g_nv2a = 0, g_gpu_applied;   /* g_nv2a: 0 not checked, 1 a real NV2A, -1 not one */
static s32 nv2a_ok(void)
{
	if (!g_nv2a)
	{
#ifdef HOST
		u32 v = g_host_nvpll;
#else
		u32 v = *(volatile u32 *)0xFD680500u;
#endif
		u32 n = (v >> 8) & 0xFFu, m = v & 0xFFu;
		g_nv2a = (n >= 20u && n <= 60u && m >= 1u && m <= 4u) ? 1 : -1;
		g_nvpll_stock = v;
	}
	return g_nv2a > 0;
}
static void stall_ms(u32 ms) { u32 t = tsc_lo(); while (tsc_lo() - t < ms * 733333u) ; }
#if SPOOKY_PROBE
static char *say_s(char *p, const char *s);
static char *say_u(char *p, u32 v);
static void smc_write(u32 cmd, u32 val);
/* ---- 6i.130: Render Scale above 100 % - first, the memory ----
 * Drawing above 100 % needs the world's screen targets bigger than the game's 640 x 480: the picture, the light
 * accumulation (texaccum) and the depth, 4 bytes a pixel each - 704 x 528 at 110 % (1.4 MB each) to 960 x 720 at 150 % (2.6
 * MB each). Halo 2 leaves little of a 64 MB console free; a 128 MB console has another 64 MB, which a game only gets with
 * its XBE's "limit to 64 MB" flag clear (as Halo 2 HD does; Halo 2's is set). So this build measures: the console's memory
 * (the kernel's MmQueryStatistics), what's free, the largest single block (MmAllocateContiguousMemory, halving down from 64
 * MB, each block freed at once), and the XBE's flag - through the game's own kernel imports (v1.0 thunks 0x411680, 0x411524,
 * 0x41152C). Logged at startup; and choosing 110 - 150 % says on screen whether that scale's targets would fit. The world
 * is still drawn at 100 % above it. */
#ifdef HOST
u32 g_host_mem_pages = 16384u, g_host_mem_avail = 160u, g_host_mem_big = 0x9C000u, g_host_memtop = 64u;
static u32 kt_memtop_mb(void) { return g_host_memtop; }
static u32 kt_query(u32 *st) { st[1] = g_host_mem_pages; st[2] = g_host_mem_avail; return 0u; }
u32 g_host_alloc_max;                              /* (the tests: the largest block asked for) */
static u32 kt_alloc(u32 size) { if (size > g_host_alloc_max) g_host_alloc_max = size; return size <= g_host_mem_big ? 0x100000u : 0u; }
static u32 kt_alloc_wc(u32 size) { return size <= g_host_mem_big ? 0xF0200000u : 0u; }
static u32 kt_phys(u32 va) { return va & 0x0FFFFFFFu; }
static void kt_free(u32 p) { (void)p; }
#else
static u32 kt_query(u32 *st) { u32 f = ADDR_KT_MMQS ? M32(ADDR_KT_MMQS) : 0u; if (!f || f < 0x80000000u) return 1u; return ((u32 (__attribute__((stdcall)) *)(u32 *))f)(st); }
/* 6i.133: MmAllocateContiguousMemoryEx (166) - any address (the kernel searches from the top of RAM down), page-aligned */
static u32 kt_alloc(u32 size) { u32 f = ADDR_KT_MMACMEX ? M32(ADDR_KT_MMACMEX) : 0u; if (!f || f < 0x80000000u) return 0u; return ((u32 (__attribute__((stdcall)) *)(u32, u32, u32, u32, u32))f)(size, 0u, 0xFFFFFFFFu, 4096u, 4u); }
/* 6i.139: memory the GPU draws into - write-combined (PAGE_READWRITE | PAGE_WRITECOMBINE), as Direct3D's own targets - and
 * its physical address (MmGetPhysicalAddress, 173), what a Direct3D resource's Data holds */
static u32 kt_alloc_wc(u32 size) { u32 f = ADDR_KT_MMACMEX ? M32(ADDR_KT_MMACMEX) : 0u; if (!f || f < 0x80000000u) return 0u; return ((u32 (__attribute__((stdcall)) *)(u32, u32, u32, u32, u32))f)(size, 0u, 0xFFFFFFFFu, 4096u, 0x404u); }
static u32 kt_phys(u32 va) { u32 f = ADDR_KT_MMGPA ? M32(ADDR_KT_MMGPA) : 0u; if (!f || f < 0x80000000u) return 0u; return ((u32 (__attribute__((stdcall)) *)(u32))f)(va); }
/* 6i.133: the RAM fitted, from the host bridge (PCI 0:0, register 0x87, CPU_MEMTOP_LIMIT): (byte + 1) x 16 MB - HalReadWritePCISpace (46) */
static u32 kt_memtop_mb(void) { u32 f = ADDR_KT_HALPCI ? M32(ADDR_KT_HALPCI) : 0u; u8 b = 0; if (!f || f < 0x80000000u) return 0u; ((void (__attribute__((stdcall)) *)(u32, u32, u32, void *, u32, u32))f)(0u, 0u, 0x87u, &b, 1u, 0u); return ((u32)b + 1u) * 16u; }
static void kt_free(u32 p) { u32 f = ADDR_KT_MMFCM ? M32(ADDR_KT_MMFCM) : 0u; if (f >= 0x80000000u && p) ((void (__attribute__((stdcall)) *)(u32))f)(p); }
#endif
static u32 g_mem_mb, g_mem_free_kb, g_mem_big_kb, g_mem_top_mb;
/* ---- 6i.191: LARGER CACHES ON A 128 MB CONSOLE - step 1: the geometry cache out of the game's region ----
 * (RESEARCH_128MB.txt; Halo-2-HD's method, without its kernel part and without shrinking the game's region.)
 * The game takes one region of memory at start-up, sized for 64 MB, and at each level load cuts its caches from the top
 * of it: geometry 6.5 or 7 MB, animation 3 or 4, sound 3 - and the TEXTURE cache gets whatever is left (your logs: 16.6
 * to 17.7 MB). So a cache that stops taking its memory from the region makes the texture cache that much larger by
 * itself, with no change to the texture cache and nothing in the region moved (checkpoints and saves as they were).
 * This step: the geometry cache. In the game's set-up for it (v1.0 0x12da08..0x12daad) the block is cut from the region
 * (the region's top for the stage, [ebp], lowered by the size, ecx) and then made read-write by a call at 0x12da80 -
 * XPhysicalProtect(ebx the block, ecx its size, 4). That call is replaced (hooks.S geo_cache_hook): when this setting
 * is on and the console has the memory, the region is given its bytes back ([ebp] += ecx), the block (ebx) becomes one of
 * the mod's own - CACHE_GEO_BYTES, taken once with MmAllocateContiguousMemoryEx, which looks from the top of memory down,
 * and kept for the session - and the size the game goes on to record (esi pages, edi bytes) becomes that block's. The
 * game then builds the cache in it exactly as it would in its own. Result: geometry 16 MB (was 6.5 / 7), textures about
 * 7 MB more.
 * OFF by default, and only from the next level load: a level keeps the caches it was loaded with.
 * NOT KNOWN until it runs on the 360: that the emulator draws from vertex data above 64 MB (the mod's larger render
 * targets live there and work; geometry is another path), and that the emulator's own memory has room for more of the
 * game's textures staying loaded. If a level's geometry comes out wrong with this on: turn it off and load the level again.
 * Never on a 64 MB console. ---- */
#define CACHE_GEO_BYTES 0x1000000u               /* 16 MB */
u32 cache_geo_pages, cache_geo_bytes;            /* what geo_cache_hook puts in esi / edi */
static u32 g_cache_geo;                          /* the mod's block for it (0: none taken) */
static s32 g_cache_geo_said;
static s32 big_cache_can(void) { return g_mem_mb > 96u; }
static void lc_default(void)                     /* v2.0: the unset setting takes its default once the memory is known */
{
	if (o_big_cache >= 0 || !g_mem_mb) return;
	o_big_cache = big_cache_can() ? 1 : 0;
}
/* 6i.201: the game's own Restart Level is one flag - its pause menu's "yes" (pause_game_restart_level_confirmed, v1.0
 * 0x2323b7) writes 1 to main_globals.reset_map (0x547F6E) and the main loop does the rest at the top of its next
 * pass. Larger Caches > Yes: the setting changed and saved, then that flag set. */
static s32 on_main_menu(void);
static s32 wire_in_game(void);
static s32 lc_level_loaded(void) { return ADDR_RESET_MAP && wire_in_game() && !on_main_menu(); }
#ifdef HOST
u32 g_host_level_reset;
#endif
static void lc_toggle_restart(void)
{
	if (!big_cache_can()) { o_big_cache = 0; toast("Larger Caches: needs a 128 MB console"); return; }
	lc_default();
	o_big_cache = o_big_cache > 0 ? 0 : 1;
	if (g_settings_loaded) settings_save();
	{ s32 at = lput(0, "caches: Larger Caches "); at = lput(at, o_big_cache ? "on" : "off"); lput(at, lc_level_loaded() ? " - restarting the level" : " - from the next level load"); log_write(); }
	if (!lc_level_loaded()) { toast(o_big_cache ? "Larger Caches: on, from the next level load" : "Larger Caches: off, from the next level load"); return; }
#ifdef HOST
	g_host_level_reset++;
#else
	gw8(ADDR_RESET_MAP, 1);
#endif
	toast(o_big_cache ? "Larger Caches: on - restarting the level" : "Larger Caches: off - restarting the level");
}
u32 cache_geo_take(u32 game_block, u32 game_size)
{
	s32 at;
	lc_default();
	if (o_big_cache > 0 && !big_cache_can()) o_big_cache = 0;                /* 6i.192: saved "on" (or a settings file from a 128 MB console) on a 64 MB one: off, and shown off */
	if (o_big_cache <= 0 || g_mem_mb <= 96u)
	{
		if (g_cache_geo && o_big_cache <= 0) { kt_free(g_cache_geo); g_cache_geo = 0; lput(0, "caches: Larger Caches off - the geometry cache's block given back; the game's own from here"); log_write(); }
		return 0u;
	}
	if (!g_cache_geo) g_cache_geo = kt_alloc(CACHE_GEO_BYTES);
	if (!g_cache_geo)
	{
		if (!g_cache_geo_said) { g_cache_geo_said = 1; lput(0, "caches: no 16 MB block for the geometry cache - the game's own is used"); log_write(); }
		return 0u;
	}
	cache_geo_bytes = CACHE_GEO_BYTES; cache_geo_pages = CACHE_GEO_BYTES >> 12;
	at = lput(0, "caches: the geometry cache is in the mod's block at "); at = lhex(at, g_cache_geo); at = lput(at, ", "); at = lnum(at, CACHE_GEO_BYTES / 1024u);
	at = lput(at, " KB (the game had taken "); at = lnum(at, game_size / 1024u); at = lput(at, " KB at "); at = lhex(at, game_block); lput(at, " - given back to its region, for the texture cache)");
	log_write();
	return g_cache_geo;
}
/* ---- 6i.194: LARGER CACHES, step 2 - the animation cache out of the game's region too ----
 * Your 6i.191 round: step 1 works on the 360 (the log: texture 16960 -> 23468 / 26112 KB, geometry 16384 KB; the levels
 * right). The animation cache is cut from the region the same way (function animation_cache_memory_initialize, v1.0
 * 0x123230: 4 MB at the first stages, 3 MB from stage 2 on) and made read-write by the same call, at 0x1232b0 -
 * XPhysicalProtect(esi the block, ecx its size, 4), with edi the address of the region's top. That call goes through
 * anim_cache_hook (hooks.S): with Larger Caches on, the region gets its bytes back and the block becomes one of the
 * mod's own (CACHE_ANIM_BYTES, taken once and kept). The cache keeps the size the game gave it - it is the textures
 * that gain: 3 MB more (4 at the first stages). Off, or no block to be had: the game's own, as ever.
 * Not moved: the sound cache (3 MB). It is taken once when the game starts, not at each level load, so it cannot follow
 * a setting changed in the menu; it would have to be decided before the game's own start-up. Left for a decision. ---- */
#define CACHE_ANIM_BYTES 0x400000u               /* 4 MB: the larger of the two sizes the game asks for */
static u32 g_cache_anim;
static s32 g_cache_anim_said;
u32 cache_anim_take(u32 game_block, u32 game_size)
{
	s32 at;
	lc_default();
	if (o_big_cache > 0 && !big_cache_can()) o_big_cache = 0;
	if (o_big_cache <= 0)
	{
		if (g_cache_anim) { kt_free(g_cache_anim); g_cache_anim = 0; lput(0, "caches: Larger Caches off - the animation cache's block given back; the game's own from here"); log_write(); }
		return 0u;
	}
	if (game_size > CACHE_ANIM_BYTES) return 0u;                              /* (larger than the mod's block: the game's own) */
	if (!g_cache_anim) g_cache_anim = kt_alloc(CACHE_ANIM_BYTES);
	if (!g_cache_anim)
	{
		if (!g_cache_anim_said) { g_cache_anim_said = 1; lput(0, "caches: no 4 MB block for the animation cache - the game's own is used"); log_write(); }
		return 0u;
	}
	at = lput(0, "caches: the animation cache is in the mod's block at "); at = lhex(at, g_cache_anim); at = lput(at, " (the game had taken "); at = lnum(at, game_size / 1024u);
	at = lput(at, " KB at "); at = lhex(at, game_block); lput(at, " - given back to its region, for the texture cache)");
	log_write();
	return g_cache_anim;
}
#ifdef HOST
u32 host_cache_anim(void) { return g_cache_anim; }
void host_set_big_cache(s32 on) { o_big_cache = on; }
s32 host_get_big_cache(void) { return o_big_cache; }
void host_lc_default(void) { lc_default(); }
void host_set_mem_mb(u32 mb) { g_mem_mb = mb; }
u32 host_get_mem_mb(void) { return g_mem_mb; }
u32 host_cache_geo(void) { return g_cache_geo; }
#endif
/* 6i.140 - your 360 photo: "RAM fitted 16 MB" (the emulator doesn't answer the host bridge's register - it read 0) and
 * "kernel 64 MB" (its Xbox kernel manages 64 MB). Halo 2 HD's 128 MB way needs both - its README: not the 360's emulator,
 * "nor will it ever be". So memory above 64 MB, if the emulator gives the game any, has to be reached without the kernel:
 * the physical pages mapped directly (MmMapIoSpace, kernel export 177 - not one Halo 2 imports, so found in the kernel's
 * export table, as the SMC's) and looked at READ-ONLY, 16 MB apart from 64 MB up: a page identical to the one 64 MB
 * lower is the low memory seen again (a 64 MB machine's address lines wrap) - nothing there; one that differs is memory of
 * its own - empty (all zero) or in use. Also the GPU's own word on its memory (NV_PFB_CSTATUS, 0xFD10020C). All shown
 * under Performance Mode Settings' title. Supersampling's buffer goes there when the kernel has no room and the memory
 * found is empty and outside the kernel's 64 MB. */
static u32 g_pfb_mb;
static s32 g_mem_known, g_pfn;                    /* g_pfn: 0 not tried, 1 raised (0x3FDF -> 0x7FCF), -1 not found */
/* 6i.133 - your 6i.132 test: still no room on the 360 (your emulator giving it 128 MB). Halo 2 HD (grimdoomer) shows what
 * else a game must do: clearing the XBE's flag isn't enough - the kernel's contiguous allocator caps the pages it hands out
 * at 0x3FDF (64 MB) whatever the RAM. HD finds that constant in MmAllocateContiguousMemoryEx's first 0x80 bytes and raises
 * it to 0x7FCF (the Chihiro arcade's 128 MB) with write protect off; then the top 64 MB can be allocated. Here: only with
 * 128 MB fitted (the host bridge says so) and the XBE's limit flag clear (the flag-test copy); once. As HD warns, the
 * kernel is then unfit to run anything else: on a real Xbox, leaving the game (HalReturnToFirmware) becomes a cold reboot
 * (the SMC's reset, then the PCI reset - HD's way). Not on the 360 (no real NV2A there). */
#ifndef HOST
static void __attribute__((stdcall)) kt_cold_reboot(u32 routine)
{
	(void)routine;
	smc_write(2u, 1u);                                                       /* SMC_COMMAND_RESET, assert reset */
	__asm__ volatile("movw $0xCF9, %%dx\n\tmovb $0x0E, %%al\n\toutb %%al, %%dx" ::: "eax", "edx");   /* full PCI reset */
	for (;;) __asm__ volatile("cli; hlt");
}
#endif
static void kt_pfn_raise(void)
{
#ifndef HOST
	u32 f = ADDR_KT_MMACMEX ? M32(ADDR_KT_MMACMEX) : 0u, p;
	if (g_pfn || !f || f < 0x80000000u) return;
	g_pfn = -1;
	for (p = f; p < f + 0x80u; p++)
	{
		if (*(volatile u32 *)p == 0x00007FAFu || *(volatile u32 *)p == 0x00007FCFu) { g_pfn = 1; break; }   /* 6i.149z: already 128 MB (the R71 emulator's kernel is built with 0x7FAF) - nothing to write. The write below is what froze the 360 three times (149x, R70, 149y): a store into the kernel's code with CR0.WP cleared, which the emulator cannot take */
		if (*(volatile u32 *)p == 0x00003FDFu && !nv2a_ok())
		{
			__asm__ volatile("pushf\n\tcli\n\tmov %%cr0, %%eax\n\tmov %%eax, %%ecx\n\tand $0xFFFEFFFF, %%eax\n\tmov %%eax, %%cr0\n\t"
			                 "movl $0x00007FCF, (%0)\n\tmov %%ecx, %%cr0\n\tpopf" :: "r"(p) : "eax", "ecx", "memory", "cc");   /* one block: flags and stack as they were */
			g_pfn = 1;
			if (nv2a_ok() && ADDR_KT_HALRTF && M32(ADDR_KT_HALRTF) >= 0x80000000u) gw32(ADDR_KT_HALRTF, (u32)kt_cold_reboot);
			break;
		}
	}
#else
	if (!g_pfn) g_pfn = 1;
#endif
}
/* 6i.171: the console's memory read once at the start, for ptr_ok's limit alone (mem_probe, which also allocates to
 * find the largest block, still runs only for Render Scale above 100 %) */
static void mem_size_note(void)
{
	u32 st[9], k;
	for (k = 0; k < 9; k++) st[k] = 0;
	if (kt_query(st) == 0u && st[1] / 256u > 96u) g_ptr_top = 0x88000000u;
}
#ifdef HOST
void host_mem_size_note(void) { mem_size_note(); }
s32 host_ptr_ok(u32 p) { return ptr_ok(p); }
#endif
static void mem_probe(void)
{
	u32 st[9], lo = 0, hi = 1024, k;
	for (k = 0; k < 9; k++) st[k] = 0;
	g_mem_top_mb = kt_memtop_mb();
#ifdef HOST
	g_pfb_mb = 64u;
#else
	{ u32 r = *(volatile u32 *)0xFD10020Cu; g_pfb_mb = (r == 0x04000000u || r == 0x08000000u) ? r >> 20 : 0u; }   /* 6i.140: the GPU's own word */
#endif
	st[0] = 36u;                                                              /* MM_STATISTICS: its length first */
	if (kt_query(st) == 0u) { g_mem_mb = st[1] / 256u; g_mem_free_kb = st[2] * 4u; if (g_mem_mb > 96u) g_ptr_top = 0x88000000u; }   /* 6i.171 */
	/* 6i.133: 128 MB fitted and the XBE's limit clear: HD's way. 6i.149w: on the 360 the host bridge does not report the 128 MB
	 * the emulator's kernel has (the kernel does, through MmQueryStatistics), so the kernel's word counts too - the contiguous
	 * allocator's cap goes to 128 MB and the big targets of Render Scale 170 % and up can be made (R69L lifts the GPU thread's
	 * 64 MB bounds on the record side; this is the test of what else in the emulator stops at 64 MB) */
	if ((g_mem_top_mb == 128u || (nv2a_ok() && g_mem_mb > 96u)) && !(M32(0x10124u) & 4u)) kt_pfn_raise();   /* 6i.149z: on the emulator this only looks (recognises R71's 0x7FAF; never writes - see kt_pfn_raise). 6i.149y: on the emulator the raise is no longer done here (at start-up / menu time): 149x and R70 both froze at the main menu with the game's own allocations above 64 MB; it is done in rs_big_make, for the big targets only - see there */
	/* 6i.200: YOUR R87L RUN - the restart now brings the game back (to its intro) and it then froze loading the main
	 * menu. The probe: the mod's "memory:" line was never written the second time, and at the freeze one block of exactly
	 * 60 MB had just been taken, leaving 4.2 MB free in the whole console. That block is this search's: it finds the
	 * largest free block by taking one and giving it straight back, halving the gap - and after a restart the upper
	 * 64 MB is one free run, so it could take 60 MB of the 64 that were free, where from a cold start the largest it
	 * can get is 51. With 4 MB left, whatever the game or the kernel asked for in that instant failed, and the kernel
	 * stopped. So where more than 32 MB is free the search no longer asks for more than what is free less 16 MB: it can
	 * still report every size the mod has a use for (the largest it ever needs is three targets of 4.8 MB and a 16 MB
	 * cache), and never starves the console while it looks. (That this is the whole of the second freeze is my reading of that probe.) */
	if (g_mem_free_kb > 32768u && (g_mem_free_kb - 16384u) / 64u < hi) hi = (g_mem_free_kb - 16384u) / 64u;   /* (with 32 MB or less free - the original Xbox's few hundred KB - as it always was) */
	while (lo < hi) { u32 mid = (lo + hi + 1u) / 2u, p = kt_alloc(mid << 16); if (p) { kt_free(p); lo = mid; } else hi = mid - 1u; }   /* in 64 KB steps */
	g_mem_big_kb = lo * 64u;
	g_mem_known = 1;
}
static u32 rs_big_w(s32 s) { return 640u * (u32)s / 10u; }                /* 704 .. 1280: multiples of 64 (6i.149v: to 200 %) */
static u32 rs_big_h(s32 s) { return 480u * (u32)s / 10u; }
static s32 g_rs_probed = 10;
static void rs_probe_step(void)
{
	char t[200], *p;
	u32 each, need_kb;
	s32 s = o_rscale, fits;
	if (s == g_rs_probed) return;
	g_rs_probed = s;
	if (s <= 10) return;
	guarded_call(mem_probe);
	if (!g_pfn && g_mem_mb > 96u && !(M32(0x10124u) & 4u) && nv2a_ok()) kt_pfn_raise();   /* 6i.149aa: on the emulator recognise R71's cap at the first step above 100 % (the start-up probe ran before the GPU check could answer), so the memory lines read right; never writes there */
	each = rs_big_w(s) * rs_big_h(s) * 4u; need_kb = 3u * each / 1024u;
	fits = g_mem_big_kb * 1024u >= each && g_mem_free_kb >= need_kb;
	p = say_s(t, "Render Scale "); p = say_u(p, (u32)s * 10u); p = say_s(p, "%: ");                                 /* 6i.152: short - the figures went to the log */
	if (fits) { p = say_u(p, rs_big_w(s)); p = say_s(p, "x"); p = say_u(p, rs_big_h(s)); p = say_s(p, " - drawn"); }
	else p = say_s(p, "no room - stays at 100%");
	toast(t);
	{
		s32 at = lput(0, "render scale "); at = lnum(at, (u32)s * 10u); at = lput(at, "%: targets "); at = lnum(at, rs_big_w(s)); at = lput(at, "x"); at = lnum(at, rs_big_h(s));
		at = lput(at, ", 3 x "); at = lnum(at, each / 1024u); at = lput(at, " KB; "); at = lnum(at, g_mem_mb); at = lput(at, " MB console, "); at = lnum(at, g_mem_free_kb);
		at = lput(at, " KB free, largest block "); at = lnum(at, g_mem_big_kb); at = lput(at, fits ? " KB: they'd fit" : " KB: no room");
		at = lput(at, "; RAM fitted (host bridge) "); at = lnum(at, g_mem_top_mb); at = lput(at, " MB, XBE limit "); at = lput(at, (M32(0x10124u) & 4u) ? "set" : "clear");
		lput(at, g_pfn == 1 ? ", kernel cap raised to 128 MB" : g_pfn < 0 ? ", kernel cap NOT found" : ", kernel cap untouched"); log_write();
	}
}
#ifdef HOST
void host_rs_probe_step(void) { rs_probe_step(); }
u32 host_mem_big_kb(void) { return g_mem_big_kb; }
void host_mem_probe(void) { mem_probe(); }
#endif
#endif /* SPOOKY_PROBE */
static void smc_write(u32 cmd, u32 val)
{
#ifdef HOST
	if (g_host_fan_n < 4) g_host_fan[g_host_fan_n++] = (cmd << 8) | val;
#else
	u32 base = 0x80010000u, pe, ex, f;
	if (*(volatile u16 *)base != 0x5A4Du) return;                         /* the kernel's image: MZ, PE, its exports */
	pe = base + M32(base + 0x3C);
	if (M32(pe) != 0x00004550u) return;
	ex = base + M32(pe + 0x78);
	if (50u < M32(ex + 0x10) || 50u - M32(ex + 0x10) >= M32(ex + 0x14)) return;
	f = base + M32(base + M32(ex + 0x1C) + 4u * (50u - M32(ex + 0x10)));
	((s32 (__attribute__((stdcall)) *)(u32, u32, u32, u32))f)(0x20u, cmd, 0u, val);
#endif
}
static char *say_s(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }
static char *say_u(char *p, u32 v) { char b[12]; s32 n = 0; do { b[n++] = (char)('0' + v % 10u); v /= 10u; } while (v); while (n) *p++ = b[--n]; *p = 0; return p; }
void coop_apply(void)                           /* 6i.40: hooks.S coop_hook - right after the game's own */
{
	if (!fps_unlocked() || M32(ADDR_COOP_LOD) != 4u) return;               /* one view only (split-screen sets its own) */
	if (o_coop_lod) gw32(ADDR_COOP_LOD, 3u);
	if (o_coop_passes || o_coop_lights)                                   /* 6i.120: Coop Render Passes - the passes and the dynamic lighting, one setting */
	{ gw8(ADDR_COOP_FLAGS + 0u, 0); gw8(ADDR_COOP_FLAGS + 2u, 0); gw8(ADDR_COOP_FLAGS + 3u, 0); gw8(ADDR_COOP_FLAGS + 4u, 0); gw8(ADDR_COOP_FLAGS + 5u, 0); }
	if (o_coop_detail) { gw32(ADDR_COOP_PRIO, 0x3E99999Au /* 0.3f */); gw32(ADDR_COOP_DIST, 0x3F4CCCCDu /* 0.8f */); }
}
static void gpu_drain(void);
static void gpu_clock_apply(void)
{
	char t[80], *p;
	u32 n, stock;
	g_gpu_applied = o_gpu_step;
	if (!nv2a_ok()) { toast("GPU overclock: original Xbox only - not changed"); return; }
	stock = (g_nvpll_stock >> 8) & 0xFFu;
	n = o_gpu_step > 0 ? 28u + (u32)o_gpu_step : stock;
	if (n > stock) { smc_write(6u, 75u / 2u); stall_ms(100u); smc_write(5u, 1u); }   /* fans to 75% first */
	gpu_drain();
#ifdef HOST
	g_host_nvpll = (g_nvpll_stock & ~0xFF00u) | (n << 8);
#else
	*(volatile u32 *)0xFD680500u = (g_nvpll_stock & ~0xFF00u) | (n << 8);
#endif
	if (n <= stock) smc_write(5u, 0u);                                     /* the fans back to the SMC */
	p = say_s(t, "GPU clock: "); p = say_u(p, n * 25u / 3u); p = say_s(p, n > stock ? " MHz, fans 75%" : " MHz (stock)");
	toast(t);
}
#ifdef HOST
s32 host_gpu_applied(void) { return g_gpu_applied; }
void host_set_gpu(s32 step, s32 udma) { (void)step; (void)udma; }
void host_set_tear(s32 on) { o_tear = on; }
void host_set_vsync(s32 on, s32 cap) { o_vsync = on; o_fps_cap = cap; }
void host_set_aniso(s32 on) { o_aniso = on; }
void host_set_water(s32 surface, s32 refr) { o_water_surface = surface; o_water_refr = refr; }
void host_set_fx_copy(s32 v) { o_fx_copy = v; }
void host_set_fps_show(s32 on) { o_fps_show = on; }
u32 host_fps_shown(void) { return g_fps_shown; }
void host_set_coop(s32 lod, s32 passes, s32 lights, s32 detail) { o_coop_lod = lod; o_coop_passes = passes; o_coop_lights = lights; o_coop_detail = detail; }
void host_nv2a_reset(u32 v) { g_nv2a = 0; g_host_nvpll = v; }
#endif
static void gpu_drain(void)                      /* 6i.17: wait for the GPU to finish everything sent */
{
	u32 dev = M32(ADDR_D3D_DEVICE), regs, n;
	if (!ptr_ok(dev)) return;
	gpu_kick(dev);
	regs = M32(dev + 0x934);
	if (!regs) return;
	for (n = 0; n < 4000000u; n++) if (M32(regs + 0x3240) == M32(regs + 0x3244) && !M32(regs + 0x400700)) break;
}
/* 6i.90: THE GPU'S OWN COUNTER AND CLOCK. Your 6i.89 log: Light Filtering fast took the lights from 12.5 to 10.5 ms in
 * the capture, yet the FPS Counter read 41 either way. The capture times each part with the GPU made to finish first -
 * the CPU's work and the GPU's added together - so it can't say which of the two sets the frame rate. The NV2A itself
 * counts the pixels that pass the depth test and stamps the time it got there (its zpass report: Direct3D's visibility
 * tests, which the game's lens flares use - function_20bb0 pushes the begin itself, 0x817c8, 1, 1;
 * D3DDevice_EndVisibilityTest v1.0 0x3f85d0 / v1.5 0x4026b0, eax the test; GetVisibilityTestResult v1.0 0x3f8630 /
 * v1.5 0x402710: the pixels, and the time in its first 8 bytes). Tests 0x2000 on: Direct3D keeps 64 pages of 256 (its
 * release loop, v1.0 0x3fc69d); the game's own are under 1536 (511 slots x 3). In a capture:
 *  - each record drawn in the draws' frames gets a test just before it (where the GPU starts it) and one around it (its
 *    pixels, where it ends): the GPU's own time for that draw beside the CPU's time to send it, by pass (a light's own
 *    11 / 12 apart: 11L / 12L) and by shader;
 *  - first, 15 frames run as normal - nothing waits for the GPU but the end of each: the CPU's time for the frame and
 *    its drawing, the GPU's from the draw's start to the present, and how long it was still busy when the frame was
 *    handed over (it then waits, so the next frame starts clean). Original Xbox: the GPU's clock read beside the CPU's
 *    (PTIMER, regs + 0x9400) to check its unit - nanoseconds. */
#define VT_BASE 0x2000u
#ifdef HOST
u32 g_host_vt_n[2], g_host_vt_on, g_host_vt_t[128];
static u32 vt_space(u32 size) { (void)size; return 0u; }
static u32 vt_end(u32 index) { (void)index; g_host_vt_n[0]++; return g_host_vt_on ? 0u : 1u; }
static u32 vt_get(u32 index, u32 *pixels, u32 *ts) { g_host_vt_n[1]++; *pixels = 0u; ts[1] = 0u; ts[0] = g_host_vt_on && index - VT_BASE < 128u ? g_host_vt_t[index - VT_BASE] : 0u; return g_host_vt_on ? 0u : 1u; }
static u32 ptimer(void) { return 0u; }
#else
u32 vt_space(u32 size);                          /* hooks.S: Direct3D's MakeSpace - eax the size, its half on the stack */
u32 vt_end(u32 index);                           /* hooks.S: D3DDevice_EndVisibilityTest - eax the test */
u32 vt_get(u32 index, u32 *pixels, u32 *ts);     /* hooks.S: D3DDevice_GetVisibilityTestResult - eax, edx the time */
static u32 ptimer(void)                          /* the NV2A's own clock, low word - a real NV2A only */
{
	u32 dev = M32(ADDR_D3D_DEVICE), regs;
	if (!nv2a_ok() || !ptr_ok(dev)) return 0u;
	regs = M32(dev + 0x934u);
	return regs ? M32(regs + 0x9400u) : 0u;
}
#endif
static s32 vt_begin(void)                        /* the begin as the game pushes it: the count cleared and on */
{
#ifdef HOST
	return (s32)g_host_vt_on;
#else
	u32 dev = M32(ADDR_D3D_DEVICE), p;
	if (!ptr_ok(dev)) return 0;
	p = M32(dev);
	if (p >= M32(dev + 4u)) p = vt_space(M32(ADDR_PUSH_SEG));
	if (p < 0x10000u || (p & 3u)) return 0;
	gw32(p, 0x817C8u); gw32(p + 4u, 1u); gw32(p + 8u, 1u);
	gw32(dev, p + 12u);
	return 1;
#endif
}
static s32 vt_stamp(u32 index) { return vt_begin() && !vt_end(index); }        /* around nothing: the GPU's time there */
static s32 vt_read(u32 index, u32 *pixels, u32 *t)                              /* the GPU drained first: 1 if it's in */
{
	u32 ts[2];
	ts[0] = ts[1] = 0u; *pixels = 0u;
	if (vt_get(index, pixels, ts)) return 0;
	*t = ts[0];
	return 1;
}
static u32 g_vt_cpu[32], g_vt_gpu[32], g_vt_px[32], g_vt_n[32], g_vt_got;   /* a capture's draws by pass (CPU ticks, GPU ns) */
static u32 g_sh_gpu[SH_N], g_sh_px[SH_N];
static u8 g_vt_ok;
static u32 g_nat_t0, g_nat_t1, g_nat_prev, g_nat_sum[4], g_nat_n, g_nat_g;  /* frames as normal: CPU frame, drawing, GPU, still busy */
/* 6i.91: THE GPU'S OWN TIMELINE, in the frames as normal - a stamp at the draw's start (29), at each pass's start (its
 * number; a light's own 11 / 12: 62 / 63), after it (28), at each of the scene's marks (30 - 61) and at the present:
 * the GPU's time from one stamp to the next goes to the part it began - what every part of the frame costs the GPU
 * while nothing waits for it. Your 6i.90 log: the GPU 23.3 ms a frame against the CPU's 11.9 - it holds the rate - but
 * the draws' own GPU times add to only 18.7; this finds the rest. */
#define NT_MAX 96
static u8 g_nt_id[NT_MAX], g_nt_n;
static u32 g_nt_sum[64];
static void nt_stamp(u32 id) { if (g_nt_n < NT_MAX && vt_stamp(VT_BASE + 16u + g_nt_n)) g_nt_id[g_nt_n++] = (u8)id; }
static void nat_start(void) { g_nat_t0 = tsc_lo(); g_nt_n = 0; nt_stamp(29u); }
static void nat_present(void) { g_nat_t1 = tsc_lo() - g_pf_w; nt_stamp(0u); }   /* the vblank wait taken out */
static void nat_end(void)
{
	u32 t = tsc_lo(), w, p, i, ok = 1u, tt[NT_MAX];
	gpu_drain();
	w = tsc_lo() - t;
	if (g_nat_prev && g_nat_t1 - g_nat_prev < 733000000u)
	{
		g_nat_sum[0] += g_nat_t1 - g_nat_prev; g_nat_sum[1] += g_nat_t1 - g_nat_t0; g_nat_sum[3] += w; g_nat_n++;
		for (i = 0; i < g_nt_n && ok; i++) ok = (u32)vt_read(VT_BASE + 16u + i, &p, &tt[i]);
		if (ok && g_nt_n >= 2u)
		{
			g_nat_sum[2] += tt[g_nt_n - 1u] - tt[0]; g_nat_g++;
			for (i = 0; i + 1u < g_nt_n; i++) if (g_nt_id[i] < 64u) g_nt_sum[g_nt_id[i]] += tt[i + 1u] - tt[i];
		}
	}
	g_nt_n = 0;
	g_nat_prev = tsc_lo();
}
/* 6i.78: an A / B capture - Dynamic Lights fast, then game's, one after the other (the setting put back after), each
 * with the lights' own numbers for its timed frames: per frame, lights drawn, of them clipped, their rectangles' size
 * against the screen, passes 11 and 12 inside a light and outside one, viewports set; and the first clipped light's
 * rectangle, centre and radius - so one capture answers what used to take a test each. */
static u8 g_ab;
static u32 g_ls[8];
static u8 g_in_light;
/* 6i.79: per capture - records drawn a frame by pass (the 360's emulator pays by the draw), and for passes 11 and 12
 * drawn outside any light (the sun's object shadows; the light at Regret's hologram not through function_31170): the
 * render target they draw to and the return addresses on the stack at the time - which game functions led there */
static u8 g_ab_scr;
/* 6i.89: your 6i.88 capture showed why 6i.85 - 6i.87 changed nothing: your Anisotropic Filtering is set to off, so no
 * texture anywhere was filtered anisotropically - nothing for Light Filtering to take off (0 draws, every stage 2/2/0, the
 * GPU's own registers a0 everywhere). Its third capture (anisotropic off only in the world's passes) left the lights'
 * own setups alone: there the lights' bump maps (stage 2) came in anisotropic (3/3/4), 83 draws a frame, Light Filtering
 * took it off at each draw (the GPU's register after pass 12: a0) - and the lights cost the same 11.4 ms as with it all
 * off. So the capture now runs what answers it outright, whatever your settings: anisotropic full with Light Filtering
 * game's, the same with fast, then anisotropic off - both settings put back after. */
static void capture_ab_start(void)
{
	if (g_cap_frames || g_ab) return;
	g_ab = 1; g_ab_scr = (u8)o_lightf; g_aniso_world = 0; g_cap_pxoff = g_cap_nomip = g_cap_bump = 0;   /* 6i.91: as you have it first */
	capture_start();
}
static void capture_start(void)
{
	{ s32 k; for (k = 0; k < 8; k++) g_ls[k] = 0; for (k = 0; k < 32; k++) { g_vt_cpu[k] = g_vt_gpu[k] = g_vt_px[k] = g_vt_n[k] = 0; } g_scr_n = 0; for (k = 0; k < 4; k++) { g_lfs_n[k] = 0; g_nat_sum[k] = 0; } g_vt_got = g_nat_n = g_nat_g = g_nat_prev = 0; for (k = 0; k < 64; k++) g_nt_sum[k] = 0; g_nt_n = 0; g_rt_n = 0; g_li_n = 0; for (k = 0; k < 32; k++) { u32 q; g_fx_ix[k] = 0; for (q = 0; q < 5; q++) g_fx[k][q] = 0.0; } for (k = 0; k < 6; k++) g_un[k / 3][k % 3] = 0; }   /* 6i.78 - 6i.110 */
	u32 k;
	if (g_cap_frames) return;
	for (k = 0; k < 64; k++) spooky_pass_cyc[k] = 0;
	for (k = 0; k < SH_N; k++) g_sh_key[k] = g_sh_cyc[k] = g_sh_n[k] = g_sh_gpu[k] = g_sh_px[k] = 0;
	g_cap_prev = o_gpu_time; o_gpu_time = 0; g_mk_last = 0; g_cap_frames = 49; g_cap_live = 0; g_cap_nat = 0; g_cap_n++;   /* 6i.90: 49 */
	toast("capture: hold still");
}
static void capture_frame(void)                  /* each frame: the 31st frame closes it (30 whole frames timed) */
{
	u32 k, us, shg[6], shp[6], shn = 0;
	s32 at;
	if (o_skip_shader != g_skip_prev) { g_skip_prev = o_skip_shader; g_skip_key = o_skip_shader > 0 && o_skip_shader <= 6 ? g_sh_top[o_skip_shader - 1] : 0; }   /* 6i.25: locked */
	spooky_rec_on = (u8)(g_cap_frames || g_skip_key);                     /* 6i.23 */
	spooky_aniso = (u8)(!fps_unlocked() || o_aniso >= 3 ? 0 : o_aniso == 2 ? 2 : o_aniso == 1 ? 3 : 1);   /* 6i.25 / 6i.26; 6i.40: with Performance Mode; 6i.53: the level */
	if (!g_cap_frames || --g_cap_frames)
	{
		g_cap_live = (u8)(g_cap_frames && g_cap_frames <= 30); g_cap_rec = (u8)(g_cap_frames & 1);
		g_cap_nat = (u8)(g_cap_frames >= 33u && g_cap_frames <= 47u);        /* 6i.90: 15 frames as normal first */
		if (g_cap_frames) o_gpu_time = g_cap_frames <= 32u;                 /* then the GPU made to finish at each mark */
		return;
	}
	g_cap_live = 0; g_cap_nat = 0;
	at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, ", ms a frame:");
	for (k = 0; k < 64; k++)
	{
		us = spooky_pass_cyc[k] / 733u / 15u; spooky_pass_cyc[k] = 0;
		if (us >= 50u) { at = lput(at, " "); at = lnum(at, k); at = lput(at, ":"); at = lms(at, us); }
	}
	log_write();
	at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " shaders, ms a frame (draws):");   /* 6i.23: the costliest */
	for (k = 0; k < 6; k++)
	{
		u32 j, best = SH_N;
		for (j = 0; j < SH_N; j++) if (g_sh_key[j] && (best == SH_N || g_sh_cyc[j] > g_sh_cyc[best])) best = j;
		g_sh_top[k] = 0;
		if (best == SH_N) continue;
		g_sh_top[k] = g_sh_key[best];
		shg[shn] = g_sh_gpu[best]; shp[shn] = g_sh_px[best]; shn++;                 /* 6i.90 */
		at = lput(at, " "); at = lnum(at, k + 1); at = lput(at, ") pass "); at = lnum(at, g_sh_mode[best]); at = lput(at, " ");
		at = lhex(at, g_sh_key[best]); at = lput(at, " "); at = lms(at, g_sh_cyc[best] / 733u / 15u);
		at = lput(at, " ("); at = lnum(at, g_sh_n[best] / 15u); at = lput(at, ")");
		g_sh_key[best] = 0;
	}
	log_write();
	if (g_vt_got)                                                            /* 6i.90: the same shaders - the GPU's own */
	{
		at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " shaders, the GPU's own ms / pixels (thousands) a frame:");
		for (k = 0; k < shn; k++) { at = lput(at, " "); at = lnum(at, k + 1); at = lput(at, ") "); at = lms(at, shg[k] / 15000u); at = lput(at, " / "); at = lnum(at, shp[k] / 15000u); }
		log_write();
	}
	at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, g_cap_pxoff ? " (no pixels drawn" : g_cap_bump ? " (as you have it, again" : g_cap_nomip ? " (no mip blending" : " (as you have it");   /* 6i.91; 6i.119 */
	at = lput(at, ", target switches a frame "); at = lnum(at, g_rt_n / 15u);                    /* 6i.92 */
	{ u32 q; for (q = 0; q < g_li_n; q++)                                                       /* 6i.95: what the lights are */
	{
		at = lput(at, q ? "; tag " : ", lights: tag "); at = lnum(at, g_li[q][0]); at = lput(at, " flags "); at = lhex(at, g_li[q][1]);
		at = lput(at, " view "); at = lnum(at, g_li[q][2]); at = lput(at, " slopes x1000 "); at = lnum(at, g_li[q][3]); at = lput(at, "/"); at = lnum(at, g_li[q][4]);
		at = lput(at, " near "); at = lnum(at, g_li[q][5]); at = lput(at, " far "); at = lnum(at, g_li[q][6]); at = lput(at, " clip % "); at = lnum(at, g_li[q][7]);
	} }
	at = lput(at, ")");
	log_write();
	if (g_vt_got)                                                            /* 6i.90: by pass - the CPU's part, the GPU's own */
	{
		at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " by pass, a frame - CPU / GPU ms, pixels (thousands):");
		for (k = 0; k < 32u; k++) if (g_vt_n[k])
		{
			at = lput(at, " "); at = lnum(at, k >= 30u ? k - 19u : k); if (k >= 30u) at = lput(at, "L");
			at = lput(at, ":"); at = lms(at, g_vt_cpu[k] / 733u / 15u); at = lput(at, "/"); at = lms(at, g_vt_gpu[k] / 15000u);
			at = lput(at, " "); at = lnum(at, g_vt_px[k] / 15000u); at = lput(at, "k "); at = lnum(at, g_vt_n[k] / 15u); at = lput(at, "d");
		}
		log_write();
		at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " a draw's GPU cost by pass - draws a frame, vertices a draw, us a draw = fixed + ns a vertex:");   /* 6i.98 */
		for (k = 0; k < 32u; k++) if (g_fx[k][0] >= 10.0)
		{
			double a, b;
			fit_line(g_fx[k], &a, &b);
			at = lput(at, " "); at = lnum(at, k >= 30u ? k - 19u : k); if (k >= 30u) at = lput(at, "L");
			at = lput(at, ": "); at = lnum(at, (u32)(g_fx[k][0] / 15.0 + 0.5)); at = lput(at, "d "); at = lnum(at, (u32)(g_fx[k][1] / g_fx[k][0])); at = lput(at, "v ");
			at = lnum(at, (u32)(g_fx[k][2] / g_fx[k][0] / 1000.0)); at = lput(at, "us = "); at = lput(at, a < 0.0 ? "-" : ""); at = lnum(at, (u32)((a < 0.0 ? -a : a) / 1000.0));
			at = lput(at, "us + "); at = lput(at, b < 0.0 ? "-" : ""); at = lms(at, (u32)((b < 0.0 ? -b : b) * 1000.0)); at = lput(at, "ns");
		}
		log_write();
		{ u32 c; at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " the light's records a frame - surfaces the camera didn't draw / did: ");   /* 6i.110 */
		  for (c = 0; c < 2u; c++) { if (c) at = lput(at, " / "); at = lnum(at, g_un[c][0] / 15u); at = lput(at, " records, "); at = lms(at, g_un[c][1] / 15000u); at = lput(at, " ms, "); at = lnum(at, g_un[c][2] / 15000u); at = lput(at, "k px"); }
		  log_write(); }
#ifndef HOST
		if (nv2a_ok())                                                         /* 6i.99: the memory tiles and the targets */
		{
			u32 dev = M32(ADDR_D3D_DEVICE), regs = ptr_ok(dev) ? M32(dev + 0x934u) : 0u, i2, j2;
			at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " memory tiles (base-limit pitch zcomp):");
			if (regs) for (i2 = 0; i2 < 8u; i2++)
			{
				u32 t = M32(regs + 0x100240u + 16u * i2), l = M32(regs + 0x100244u + 16u * i2);
				if (!(t & 1u)) continue;
				at = lput(at, " "); at = lnum(at, i2); at = lput(at, ":"); at = lhex(at, t & 0x03FFC000u); at = lput(at, "-"); at = lhex(at, l);
				at = lput(at, " "); at = lhex(at, M32(regs + 0x100248u + 16u * i2)); at = lput(at, " "); at = lhex(at, M32(regs + 0x100300u + 4u * i2));
			}
			for (j2 = 0; j2 < 3u; j2++)
			{
				u32 d = g_rtd[j2][0] & 0x03FFFFFFu, tl = 0;
				at = lput(at, j2 == 0 ? "; pass 1 into " : j2 == 1 ? "; pass 3 into " : "; depth "); at = lhex(at, g_rtd[j2][0]);
				at = lput(at, " fmt "); at = lhex(at, g_rtd[j2][1]); at = lput(at, " size "); at = lhex(at, g_rtd[j2][2]);
				if (regs) for (i2 = 0; i2 < 8u; i2++)
				{
					u32 t = M32(regs + 0x100240u + 16u * i2), l = M32(regs + 0x100244u + 16u * i2) | 0x3FFFu;
					if ((t & 1u) && d >= (t & 0x03FFC000u) && d <= l) tl = i2 + 1u;
				}
				if (tl) { at = lput(at, " in tile "); at = lnum(at, tl - 1u); } else at = lput(at, " in no tile");
			}
			log_write();
		}
#endif
	}
	{
		u32 n = g_nat_n ? g_nat_n : 1u;
		at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " as normal ("); at = lnum(at, g_nat_n); at = lput(at, " frames): the CPU ");
		at = lms(at, g_nat_sum[0] / 733u / n); at = lput(at, " ms a frame, drawing "); at = lms(at, g_nat_sum[1] / 733u / n);
		if (g_nat_g) { at = lput(at, "; the GPU drawing "); at = lms(at, g_nat_sum[2] / 1000u / g_nat_g); at = lput(at, " ms"); }
		else at = lput(at, "; the GPU's time not read");
		at = lput(at, "; still busy at the present "); at = lms(at, g_nat_sum[3] / 733u / n); at = lput(at, " ms");
		log_write();
	}
	if (g_nat_g)                                                             /* 6i.91: the GPU's timeline by part */
	{
		at = lput(0, "capture "); at = lnum(at, g_cap_n); at = lput(at, " GPU by part (as normal), ms a frame:");
		for (k = 1; k < 64u; k++) if (g_nt_sum[k] / 1000u / g_nat_g >= 50u)
		{
			at = lput(at, " "); at = lnum(at, k >= 62u ? k - 51u : k); if (k >= 62u) at = lput(at, "L");
			at = lput(at, ":"); at = lms(at, g_nt_sum[k] / 1000u / g_nat_g);
		}
		log_write();
	}
	o_gpu_time = g_cap_prev; g_mk_last = 0;
	g_log[60] = 0; toast(g_log + 10);                                      /* "N shaders, ms a frame (draws): 1) pass 3 ..." */
	if (g_ab == 1) { g_ab = 2; g_cap_pxoff = 1; capture_start(); }              /* 6i.91: no pixels drawn */
	else if (g_ab == 2) { g_ab = 3; g_cap_pxoff = 0; g_cap_bump = 1; capture_start(); }   /* 6i.119: as you have it, again */
	else if (g_ab == 3) { g_ab = 0; g_cap_bump = 0; g_cap_nomip = 0; g_aniso_world = 0; o_lightf = g_ab_scr; }
}
#ifndef HOST
extern void rec_hook(void);
void *spooky_recp = (void *)rec_hook;            /* 6i.23: function_3bc30's call [spooky_recp] */
#endif
void rec_in(u32 record, u32 mode)                /* hooks.S rec_hook, before a record's draw */
{
	u32 key = M32(record + 0x10);
	spooky_rec_skip = (u8)(g_skip_key && key == g_skip_key);                 /* 6i.25: the locked one */
	if (spooky_rec_skip || !g_cap_live || !g_cap_rec) return;              /* 6i.26: the draws' frames only */
	gpu_drain();
	g_sh_cur = key; g_sh_cur_mode = mode;
	g_vt_ok = (u8)(vt_stamp(VT_BASE) && vt_begin());                          /* 6i.90: the GPU's time at its start, then its count */
	g_rec_ix = 0;                                                               /* 6i.98 */
	g_rec_un = g_rec_se = 0;                                                    /* 6i.110 */
	g_sh_t0 = tsc_lo();
}
void rec_out(void)                               /* ... and after it */
{
	u32 dt, k, small = 0, tc, gpu = 0u, px = 0u, b;
	if (!g_cap_live || !g_cap_rec) return;
	tc = tsc_lo() - g_sh_t0;                                                   /* 6i.90: the CPU's own part */
	if (g_vt_ok && vt_end(VT_BASE + 1u)) g_vt_ok = 0;
	gpu_drain();
	dt = tsc_lo() - g_sh_t0;
	if (g_vt_ok)
	{
		u32 a = 0u, c = 0u, p0;
		if (vt_read(VT_BASE, &p0, &a) && vt_read(VT_BASE + 1u, &px, &c)) gpu = c - a; else { g_vt_ok = 0; px = 0u; }
	}
	b = g_sh_cur_mode < 30u ? g_sh_cur_mode : 29u;
	if (g_in_light && (b == 11u || b == 12u)) b += 19u;                           /* 30 / 31: a light's own 11 / 12 */
	g_vt_cpu[b] += tc; g_vt_n[b]++;
	if (g_vt_ok) { g_vt_gpu[b] += gpu; g_vt_px[b] += px; g_vt_got++; }
	if ((b == 31u || b == 30u) && (g_rec_un || g_rec_se)) { u32 c = g_rec_se ? 1u : 0u; g_un[c][0]++; if (g_vt_ok) { g_un[c][1] += gpu; g_un[c][2] += px; } }   /* 6i.110 / 6i.112: 11L too */
	if (g_vt_ok && g_rec_ix) { double x = (double)g_rec_ix, y = (double)gpu; g_fx[b][0] += 1.0; g_fx[b][1] += x; g_fx[b][2] += y; g_fx[b][3] += x * y; g_fx[b][4] += x * x; g_fx_ix[b] += g_rec_ix; }   /* 6i.98 */
	for (k = 0; k < SH_N; k++) if (g_sh_key[k] == g_sh_cur && g_sh_mode[k] == g_sh_cur_mode) { g_sh_cyc[k] += dt; g_sh_n[k]++; g_sh_gpu[k] += gpu; g_sh_px[k] += px; return; }
	for (k = 0; k < SH_N; k++) { if (!g_sh_key[k]) { small = k; break; } if (g_sh_cyc[k] < g_sh_cyc[small]) small = k; }
	if (g_sh_key[small] && g_sh_cyc[small] >= dt) return;
	g_sh_key[small] = g_sh_cur; g_sh_mode[small] = g_sh_cur_mode; g_sh_cyc[small] = dt; g_sh_n[small] = 1; g_sh_gpu[small] = gpu; g_sh_px[small] = px;
}
/* 6i.86: Light Filtering, take two. Your 6i.85 test: no change - inside passes 11 and 12 the texture states were already
 * set; the game prepares a light's (and a shadow's) texture states before drawing its passes, as it gathers the
 * surfaces it reaches. The main scene runs: the world's passes (1, 3, 5, 8, 20), then the objects' shadows
 * (function_335e0 - each through function_32350, marked by shadow_hook), then the dynamic lights (function_31c80, the
 * light loop: v1.0 called at 0x2c03e, v1.5 0x2c01e; hooks.S lightloop_hook marks it), then the sky (mark 41). So the
 * texture setup hook itself (here) turns anisotropic filtering off while a shadow or the light loop runs, and the mark
 * is cleared at the sky, the water, the interface, the scene's start and the HUD's view. The world's own textures - set
 * up in its passes before - keep theirs. */
volatile u8 spooky_light_loop;
extern volatile u8 spooky_in_shadow;
/* ---- 6i.147: THE WATER IN WIDESCREEN (your test: the stock XBE too - Halo 2's own bug) ----
 * Halo 2 draws its big water as a mesh placed in front of the camera, sized each frame by function_3ca90 (called by the
 * water plane's function_3c3c0, v1.0 0x3c401, eax the water's state). Its width comes from the view's horizontal reach,
 * tan(half the field of view) x the view rectangle's width / height - but the rectangle is 640 x 480 in widescreen too
 * (the picture is stretched by the TV), so it's a 4:3 reach and the mesh stops short of a 16:9 view's sides; it also
 * caps the half angle at ~45 degrees (a wider Field of View: the same cut-off). The mesh is its transform g_4c1b38 (3 rows
 * of 4 floats: column 0 forward x distance, column 1 left x width). After the game's own sizing, water_fit works out the
 * real reach - the frame's field of view, the rectangle, x 4/3 in widescreen (as the overlays' camera_read) - and widens
 * column 1 to it (with the game's own 1.13 margin) when the game's is narrower. */
static u32 g_ws, g_ws_flag5;                      /* the game's widescreen test and its g_510c50[5] flag, this frame */
static void aspect_query(void);
void water_fit(u32 state)
{
	u32 m = ADDR_WATER_XFORM, fs = ADDR_FRAME_STORAGE;
	float fov, ratio, tangent, tv, aspect, th, c0 = 0.f, c1 = 0.f, need, want;
	s32 i, w, h;
	if (!m || !ptr_ok(state)) return;
	fov = MF(state + 0x7C); ratio = MF(state + 0x80);
	if (!(fov > 0.01f && fov < 3.1f) || !(ratio > 0.01f && ratio < 100.f)) return;
	w = (s32)(s16)M16(fs + 0x4E) - (s32)(s16)M16(fs + 0x4A); h = (s32)(s16)M16(fs + 0x4C) - (s32)(s16)M16(fs + 0x48);
	if (w <= 0 || h <= 0) return;
	tv = MF(fs + 0x40);                                                     /* the frame's own field of view (its view +0x28) */
	if (!(tv > 0.01f && tv < 3.1f)) return;
	tv = ftan(tv * 0.5f);
	aspect = (float)w / (float)h;
	aspect_query();
	if (g_ws) { aspect *= 4.f / 3.f; if (g_ws_flag5) tv = 0.5625f * (tv * (4.f / 3.f)); }
	th = tv * aspect;                                                       /* the view's real horizontal reach */
	tangent = ftan(fov * 0.5f) * ratio;                                     /* the mesh's own */
	if (!(tangent > 0.001f) || !(th > 0.001f && th < 20.f)) return;
	for (i = 0; i < 3; i++) { float a = MF(m + 16u * (u32)i), b = MF(m + 16u * (u32)i + 4u); c0 += a * a; c1 += b * b; }
	c0 = fsqrt(c0); c1 = fsqrt(c1);                                         /* the distance, the game's width */
	if (!(c0 > 0.f) || !(c1 > 0.f)) return;
	need = th / tangent;
	if (need <= 1.f) return;
	want = need * 1.13f * c0;
	if (want <= c1 * 1.001f) return;
	for (i = 0; i < 3; i++) gwf(m + 16u * (u32)i + 4u, MF(m + 16u * (u32)i + 4u) * (want / c1));
}
void aniso_clamp(u32 range)                      /* 6i.25: hooks.S aniso_hook, after function_189a0; 6i.53: by level */
{
	u32 n = M16(range) >> 9, s, b, lv = spooky_aniso;
	/* 6i.86 turned anisotropic off here while a shadow or the light loop ran: no change measured (6i.87: see scr_pre) */
	if (g_aniso_world && g_pass_now != 1u && g_pass_now != 3u && g_pass_now != 5u && g_pass_now != 8u && g_pass_now != 20u) return;   /* 6i.88 */
	if (!lv) return;                                                                         /* the game's */
	if (n > 4) n = 4;
	for (s = 0; s < n; s++)
	{
		b = ADDR_TSS + 0x80 * s;                     /* Direct3D's stage states: +0x0C mag, +0x10 min, +0x14 mip filter, +0x20 max anisotropy */
		if (lv == 1)                                 /* off: linear, anisotropy 0 */
		{
			if (M32(b + 0x10) == 3) gw32(b + 0x10, 2);
			if (M32(b + 0x0C) == 3) gw32(b + 0x0C, 2);
			gw32(b + 0x20, 0);
		}
		else if (M32(b + 0x10) == 3 || M32(b + 0x0C) == 3)   /* a layer filtered anisotropically */
		{
			if (M32(b + 0x20) > 2) gw32(b + 0x20, 2);    /* 2x */
			if (lv == 3 && M32(b + 0x14) == 2) gw32(b + 0x14, 1);   /* fast: its mipmap levels not blended */
		}
	}
}
#ifdef HOST
s32 host_rec_skip(void) { return spooky_rec_skip; }
void host_set_skip_shader(s32 n) { o_skip_shader = n; }
void host_shader_locks(u32 *skip, u32 *info) { *skip = g_skip_key; *info = 0; }
#endif
void rs_stretch(void);                           /* 6i.63: Render Scale's stretch (below) */
void mark(u32 id)                                /* 6i.18: a cut in the frame (GPU time by pass on) */
{
	if (g_cap_nat) { if (id == 29u) nat_start(); else nt_stamp(id); }          /* 6i.90 / 6i.91: the draw's start, the scene's marks */
	if ((id == 56 && o_rs_point == 3) || (id == 57 && o_rs_point == 4)) rs_stretch();   /* 6i.71: Stretch Point (test) */
	if (id == 41 || id == 43 || id == 49) spooky_light_loop = 0;                        /* 6i.86: the light loop is over */
	u32 t;
	if (!o_gpu_time) return;
	gpu_drain();
	t = tsc_lo();
	if (g_mk_last && g_mk_id < 64u && g_cap_live && !g_cap_rec) spooky_pass_cyc[g_mk_id] += t - g_mk_last;   /* 6i.26: the parts' frames only */
	g_mk_last = t; g_mk_id = id;
}
/* 6i.92: RENDER TARGET SWITCHES. Your 360 log: the part after each pass until the next mark (28) costs the 360 6.7 ms a
 * frame (the original Xbox 1.0). Between passes the game switches the drawing between the texture accumulation buffer
 * (pass 1, each light's pass 14), the screen (passes 3, 12) and the shadow's texture - and the 360's emulator finishes
 * and writes out each target it leaves. hooks.S rt_hook1 / rt_hook2 sit at function_1cd30's two Direct3D calls (it skips
 * a target already set, so only real switches get there): each starts the capture's part 27 - the switch until the
 * pass starts, on both consoles - and is counted. */
/* 6i.106: part 27 (2.8 ms of the GPU at Regret, nothing known inside) broken down by where each switch goes: the
 * target's width (Direct3D's surface size, + 0x10, bits 0-11) - 640 -> part 22, 320 -> 23, anything else -> 24 */
/* 6i.109: from the 360 emulator's profile (another conversation): the two switch hooks called rt_switch on every
 * switch - 1.19 % of Halo 2's x86 time there - though it only does anything in a capture; and the per-draw texture-
 * state tweaks (Light Filtering faster, World Mip Blending off) cost the 360's x86 on every draw, where its frame is
 * held by the CPU, not the GPU. Now the hooks check a byte (spooky_rt_on: a capture or GPU time by pass) and go
 * straight on to Direct3D otherwise, and the per-draw tweaks run on a real NV2A only (nv2a_ok). */
void rt_switch(u32 surf)
{
	u32 w = ptr_ok(surf) ? (M32(surf + 0x10u) & 0xFFFu) + 1u : 0u;
	if (g_cap_live && !g_cap_rec && g_rt_n < 0xFFFFFF00u) g_rt_n++;
	mark(w == 640u ? 22u : w == 320u ? 23u : 24u);
}
#ifdef HOST
u32 host_rt_n(void) { return g_rt_n; }
void host_cap_ltex(s32 v) { g_cap_bump = (u8)v; }
u32 host_un(s32 c) { return g_un[c][0]; }
void host_set_light_skip(s32 v) { o_light_skip = v; }
void host_set_world_mip(s32 v) { o_world_mip = v; }
#endif
/* 6i.77: DYNAMIC LIGHTS (Performance Mode Settings) - game's / fast. Your 6i.75 test: Smoke Pass (test) is pass 12 - the
 * dynamic lights (the rocket's light on the world around it, 57 vs 70 fps; Regret's green hologram light). Each light
 * is drawn by function_31170 (per light, from the light loop: v1.0 0x31f26, v1.5 0x31f16; eax the tag, ecx its shape,
 * then the light's index, its source - the light's datum + 0x84 - a flag and a scale on the stack; ret 0x10: hooks.S
 * light_hook), which re-draws every piece of the world inside the light's sphere with the light added (pass 12; its
 * shadows, pass 11, the same way) - over every pixel those pieces cover on screen, most of them far past where the
 * light reaches. fast: for that light's passes 11 and 12, the drawing area (Direct3D's viewport - which for the game's
 * own shaders only clips, as Render Scale showed) is cut to the screen rectangle the light's sphere (its datum: centre
 * +0x18, radius +0x24) covers, worked out from the camera as the scene started. The sphere touching the camera, or a
 * rectangle as big as the screen: left alone. After the pass, the game's own viewport back (function_14bc0 again). */
static float g_lcam[11];                          /* the scene's camera: position, forward, up, focal x / y (pixels) */
static s32 g_lcam_r[4], g_lcam_ok;                 /* ... and the screen's drawing rectangle: left, top, right, bottom */
static s32 g_lr[4];
static u8 g_light_on, g_light_set;
void d3d_set_viewport(u32 vp);                   /* hooks.S: Direct3D's SetViewport, the game's own */
static u32 g_lvp[6];
static float fget(u32 a) { union { u32 u; float f; } v; v.u = M32(a); return v.f; }
static void cam_snap(void)
{
	u32 d0 = M32(ADDR_FRAME_STORAGE + 0x48), d1 = M32(ADDR_FRAME_STORAGE + 0x4C);
	s32 k;
	for (k = 0; k < 9; k++) g_lcam[k] = fget(ADDR_CAM_RECT - 0x30 + 4 * (u32)k);   /* position, forward, up */
	g_lcam[9] = fget(ADDR_PROJ_STORE + 0xB8); g_lcam[10] = fget(ADDR_PROJ_STORE + 0xBC);
	g_lcam_r[0] = (s32)(d0 >> 16); g_lcam_r[1] = (s32)(d0 & 0xFFFFu); g_lcam_r[2] = (s32)(d1 >> 16); g_lcam_r[3] = (s32)(d1 & 0xFFFFu);
	g_lcam_ok = g_lcam_r[2] > g_lcam_r[0] && g_lcam_r[3] > g_lcam_r[1] && g_lcam[9] > 1.f && g_lcam[10] > 1.f;
}
static s32 sphere_rect(float px, float py, float pz, float r);
static s32 frustum_rect(u32 source, u32 tag, u32 shape);
static void light_note(u32 tag, u32 shape);
void light_begin(u32 source, u32 tag, u32 shape) /* hooks.S light_hook: before a light's passes (6i.93: its tag, eax; 6i.94: its view, ecx) */
{
	float px, py, pz, r;
	u32 light = source - 0x84u;
	g_light_on = 0;
	g_in_light = 1;                                                                /* 6i.78 */
	if (g_cap_live && g_ls[0] < 0xFFFFFF00u) g_ls[0]++;
	if (!o_lights || !fps_unlocked() || !g_lcam_ok || !ptr_ok(light) || !ptr_ok(light + 0x28)) { light_note(tag, shape); return; }
	if (!frustum_rect(source, tag, shape))                                         /* 6i.95: a beam light's own frustum */
	{
		px = fget(light + 0x18); py = fget(light + 0x1C); pz = fget(light + 0x20); r = fget(light + 0x24);
		if (!(r > 0.f) || r > 10000.f || !sphere_rect(px, py, pz, r)) { light_note(tag, shape); return; }
	}
	g_light_on = 1;
	light_note(tag, shape);                                                       /* 6i.95 */
	if (g_cap_live)                                                                /* 6i.78 */
	{
		u32 sa = (u32)((g_lcam_r[2] - g_lcam_r[0]) * (g_lcam_r[3] - g_lcam_r[1])), la = (u32)((g_lr[2] - g_lr[0]) * (g_lr[3] - g_lr[1]));
		g_ls[1]++; g_ls[2] += sa ? la * 100u / sa : 0u;
	}
}
/* 6i.77 / 6i.81: a sphere's rectangle on screen (g_lr) from the scene's camera - 0: not worth clipping (it reaches the
 * camera, or covers the screen) */
static s32 sphere_rect(float px, float py, float pz, float r)
{
	float dx, dy, dz, x, y, z, rx, ry, rz, ux, uy, uz, cx, cy, a, b, lo, hi;
	dx = px - g_lcam[0]; dy = py - g_lcam[1]; dz = pz - g_lcam[2];
	z = dx * g_lcam[3] + dy * g_lcam[4] + dz * g_lcam[5];                              /* along forward */
	if (z - r < 0.05f) return 0;                                                   /* the sphere reaches the camera */
	rx = g_lcam[4] * g_lcam[8] - g_lcam[5] * g_lcam[7];                                /* right = forward x up */
	ry = g_lcam[5] * g_lcam[6] - g_lcam[3] * g_lcam[8];
	rz = g_lcam[3] * g_lcam[7] - g_lcam[4] * g_lcam[6];
	ux = ry * g_lcam[5] - rz * g_lcam[4];                                            /* up' = right x forward */
	uy = rz * g_lcam[3] - rx * g_lcam[5];
	uz = rx * g_lcam[4] - ry * g_lcam[3];
	x = dx * rx + dy * ry + dz * rz; y = dx * ux + dy * uy + dz * uz;
	cx = (float)(g_lcam_r[0] + g_lcam_r[2]) * 0.5f; cy = (float)(g_lcam_r[1] + g_lcam_r[3]) * 0.5f;
	a = (x - r) / (z - r); b = (x - r) / (z + r); lo = a < b ? a : b;              /* the sphere's box, the nearer depth: wider */
	a = (x + r) / (z - r); b = (x + r) / (z + r); hi = a > b ? a : b;
	g_lr[0] = fti(cx + g_lcam[9] * lo) - 2; g_lr[2] = fti(cx + g_lcam[9] * hi) + 3;
	a = (y + r) / (z - r); b = (y + r) / (z + r); hi = a > b ? a : b;              /* screen y runs down */
	a = (y - r) / (z - r); b = (y - r) / (z + r); lo = a < b ? a : b;
	g_lr[1] = fti(cy - g_lcam[10] * hi) - 2; g_lr[3] = fti(cy - g_lcam[10] * lo) + 3;
	if (g_lr[0] < g_lcam_r[0]) g_lr[0] = g_lcam_r[0];
	if (g_lr[1] < g_lcam_r[1]) g_lr[1] = g_lcam_r[1];
	if (g_lr[2] > g_lcam_r[2]) g_lr[2] = g_lcam_r[2];
	if (g_lr[3] > g_lcam_r[3]) g_lr[3] = g_lcam_r[3];
	if (g_lr[2] <= g_lr[0] || g_lr[3] <= g_lr[1]) { g_lr[0] = g_lr[1] = 0; g_lr[2] = g_lr[3] = 1; }   /* off screen: next to nothing */
	if (g_lr[0] == g_lcam_r[0] && g_lr[1] == g_lcam_r[1] && g_lr[2] == g_lcam_r[2] && g_lr[3] == g_lcam_r[3]) return 0;
	return 1;
}
/* 6i.95: A BEAM LIGHT, CLIPPED TO ITS OWN FRUSTUM. 6i.93 / 6i.94 looked only for a light the CAMERA carries (its definition's
 * flags 0x20 and 0x40000) - but the light loop (function_31c80) says the flashlight is one the first-person WEAPON
 * carries: flags 0x20 WITHOUT 0x40000, its frame each frame from the weapon's marker (first_person_weapon_get_marker:
 * its position at light + 0x84, its beam's direction at + 0xac, its up at + 0xb8 - the source, + 0x28, + 0x34). So no
 * light ever matched. Now any beam light - the view's type (function_31170's ECX) non-zero - gets the screen rectangle
 * of its frustum: from the source's position along the beam, near (+0x60) to far (+0x64), out to the sum of its two
 * slopes (+0x4c, +0x50) - a square that holds the frustum however it's turned round the beam - all eight corners
 * projected from the scene's camera, and 4 pixels. The camera's own (flags 0x20 and 0x40000 - none in your logs), a
 * corner at or behind the camera, or a rectangle as big as the screen: left alone (the sphere as before). */
/* 6i.96: THE LIGHT'S VIEW LIVES ON THE STACK. Your 6i.95 logs named the flashlight - tag 3588, flags 0x00201028 (0x20
 * without 0x40000: the weapon's) - but its view read 65535: function_31c80 builds the view in a local (shape[0x7c]) and
 * hands 31170 its address, a stack address, which ptr_ok turns away (it takes the game's data below 0x84000000 only) -
 * so 6i.94 and 6i.95 stopped before reading a byte of it. A view is taken when it sits on our own stack, above us and
 * within 64 KB (31c80's frame is a few KB up: its lights' list, the view, six projections). */
s32 view_in_stack(u32 p, u32 sp) { return !(p & 3u) && p > sp && p - sp < 0x10000u; }
static s32 view_ok(u32 p)
{
#ifdef HOST
	return ptr_ok(p);
#else
	u32 sp;
	__asm__ volatile ("movl %%esp, %0" : "=r" (sp));
	return view_in_stack(p, sp);
#endif
}
static s32 frustum_rect(u32 source, u32 tag, u32 shape)
{
	u32 tt = M32(ADDR_TAG_TABLE), def, fl, k;
	float sx, sy, s, n, f, P[3], F[3], U[3], R[3], l, d, c[3], x, y, z, lo[2], hi[2];
	if (!ptr_ok(tt) || !view_ok(shape) || !ptr_ok(source) || !(M32(shape) & 0xFFFFu)) return 0;   /* a sphere light: its sphere */
	def = M32(tt + ((tag & 0xFFFFu) << 4) + 8u);
	if (!ptr_ok(def)) return 0;
	fl = M32(def);
	sx = fget(shape + 0x4Cu); sy = fget(shape + 0x50u); n = fget(shape + 0x60u); f = fget(shape + 0x64u);
	if (!(sx > 0.001f && sx < 8.f && sy > 0.001f && sy < 8.f && f > n && f < 10000.f)) return 0;
	/* 6i.97: your 6i.96 logs read the flashlight at last - a beam (view 2), slopes 0.466 / 0.466 (about 50 degrees across,
	 * against the view's ~70), "near" 2 and far 6 - and it still wasn't cut: the sum of its slopes (0.93, to hold any roll)
	 * is wider than the screen. Its gel lights a round spot inside its square, and a circle stays inside the square
	 * however the beam rolls - so the larger slope alone holds it. And "near" isn't a cut: function_497a0 uses it only to
	 * scale the light's fade with depth (anything nearer is lit, its fade clamped) - so the beam is taken from 0.25 out
	 * (nearer than that, something is right at the gun). */
	s = sx > sy ? sx : sy;
	n = n > 0.25f ? 0.25f : (n < 0.05f ? 0.05f : n);
	if ((fl & 0x40020u) == 0x40020u) return 0;                                       /* the camera's own (none seen in your logs): its sphere */
	{
		for (k = 0; k < 3u; k++) { P[k] = fget(source + 4u * k); F[k] = fget(source + 0x28u + 4u * k); U[k] = fget(source + 0x34u + 4u * k); }
		l = fsqrt(F[0] * F[0] + F[1] * F[1] + F[2] * F[2]);
		if (!(l > 0.0001f)) return 0;
		for (k = 0; k < 3u; k++) F[k] /= l;
		d = U[0] * F[0] + U[1] * F[1] + U[2] * F[2];
		for (k = 0; k < 3u; k++) U[k] -= F[k] * d;
		l = fsqrt(U[0] * U[0] + U[1] * U[1] + U[2] * U[2]);
		if (!(l > 0.0001f)) return 0;
		for (k = 0; k < 3u; k++) U[k] /= l;
	}
	R[0] = F[1] * U[2] - F[2] * U[1]; R[1] = F[2] * U[0] - F[0] * U[2]; R[2] = F[0] * U[1] - F[1] * U[0];
	{
		float cr[3], cu[3];                                                          /* the camera's right and up */
		cr[0] = g_lcam[4] * g_lcam[8] - g_lcam[5] * g_lcam[7]; cr[1] = g_lcam[5] * g_lcam[6] - g_lcam[3] * g_lcam[8]; cr[2] = g_lcam[3] * g_lcam[7] - g_lcam[4] * g_lcam[6];
		cu[0] = cr[1] * g_lcam[5] - cr[2] * g_lcam[4]; cu[1] = cr[2] * g_lcam[3] - cr[0] * g_lcam[5]; cu[2] = cr[0] * g_lcam[4] - cr[1] * g_lcam[3];
		lo[0] = lo[1] = 1e9f; hi[0] = hi[1] = -1e9f;
		for (k = 0; k < 8u; k++)
		{
			float dd = (k & 4u) ? f : n, a = (k & 1u) ? s * dd : -s * dd, b = (k & 2u) ? s * dd : -s * dd;
			u32 q;
			for (q = 0; q < 3u; q++) c[q] = P[q] + F[q] * dd + R[q] * a + U[q] * b - g_lcam[q];
			z = c[0] * g_lcam[3] + c[1] * g_lcam[4] + c[2] * g_lcam[5];
			if (z < 0.05f) return 0;                                                 /* at or behind the camera */
			x = (c[0] * cr[0] + c[1] * cr[1] + c[2] * cr[2]) / z; y = (c[0] * cu[0] + c[1] * cu[1] + c[2] * cu[2]) / z;
			if (x < lo[0]) lo[0] = x;
			if (x > hi[0]) hi[0] = x;
			if (y < lo[1]) lo[1] = y;
			if (y > hi[1]) hi[1] = y;
		}
	}
	x = (float)(g_lcam_r[0] + g_lcam_r[2]) * 0.5f; y = (float)(g_lcam_r[1] + g_lcam_r[3]) * 0.5f;
	if (lo[0] < -100.f || hi[0] > 100.f || lo[1] < -100.f || hi[1] > 100.f) return 0;
	g_lr[0] = fti(x + g_lcam[9] * lo[0]) - 4; g_lr[2] = fti(x + g_lcam[9] * hi[0]) + 5;
	g_lr[1] = fti(y - g_lcam[10] * hi[1]) - 4; g_lr[3] = fti(y - g_lcam[10] * lo[1]) + 5;     /* screen y runs down */
	if (g_lr[0] < g_lcam_r[0]) g_lr[0] = g_lcam_r[0];
	if (g_lr[1] < g_lcam_r[1]) g_lr[1] = g_lcam_r[1];
	if (g_lr[2] > g_lcam_r[2]) g_lr[2] = g_lcam_r[2];
	if (g_lr[3] > g_lcam_r[3]) g_lr[3] = g_lcam_r[3];
	if (g_lr[2] <= g_lr[0] || g_lr[3] <= g_lr[1]) { g_lr[0] = g_lr[1] = 0; g_lr[2] = g_lr[3] = 1; }  /* off screen: next to nothing */
	return !(g_lr[0] == g_lcam_r[0] && g_lr[1] == g_lcam_r[1] && g_lr[2] == g_lcam_r[2] && g_lr[3] == g_lcam_r[3]);
}
/* 6i.95: the capture's lights - the first two kinds seen: tag, definition flags, the view's type, slopes, near, far,
 * and the share of the screen its rectangle covers (0 when not clipped) - so the log says what each light is */
static void light_note(u32 tag, u32 shape)
{
	u32 tt = M32(ADDR_TAG_TABLE), def, *e;
	if (!g_cap_live || g_li_n >= 2u || (g_li_n == 1u && g_li[0][0] == (tag & 0xFFFFu))) return;
	e = g_li[g_li_n++];
	def = ptr_ok(tt) ? M32(tt + ((tag & 0xFFFFu) << 4) + 8u) : 0u;
	e[0] = tag & 0xFFFFu; e[1] = ptr_ok(def) ? M32(def) : 0u; e[2] = view_ok(shape) ? M32(shape) & 0xFFFFu : 0xFFFFu;
	e[3] = e[4] = e[5] = e[6] = 0u;
	if (view_ok(shape)) { e[3] = (u32)fti(fget(shape + 0x4Cu) * 1000.f); e[4] = (u32)fti(fget(shape + 0x50u) * 1000.f); e[5] = (u32)fti(fget(shape + 0x60u) * 1000.f); e[6] = (u32)fti(fget(shape + 0x64u) * 1000.f); }
	e[7] = g_light_on ? (u32)((g_lr[2] - g_lr[0]) * (g_lr[3] - g_lr[1]) * 100) / (u32)((g_lcam_r[2] - g_lcam_r[0]) * (g_lcam_r[3] - g_lcam_r[1]) + 1) : 0u;
}
void light_end(void) { g_light_on = 0; g_in_light = 0; }
/* 6i.81: objects' shadows (your 6i.80 logs: on both consoles every shadow pass outside a light comes from
 * function_32350 - the per-object shadow: v1.0 called at 0x33647, v1.5 0x33637 -> 0x32340; stdcall (object, alpha),
 * ret 8: hooks.S shadow_hook marks it). It lays an object's shadow (pass 11) by re-drawing every world piece inside the
 * shadow's box over every pixel those pieces cover - most of them nowhere near the shadow. The box is the game's own:
 * its shadow camera (g_47fd18: the object's centre +0, the light's direction +0x28) and size (g_47fda0: width +0x20 -
 * twice the shadow's radius - near +0x2c, far +0x30). Lights & Shadows fast clips pass 11 to the screen rectangle
 * around that box (taken generously: from the object back along the light by the box's depth, out past its far end,
 * the radius around it), as for a light. */
volatile u8 spooky_in_shadow;
/* 6i.85: LIGHT FILTERING (Performance Mode Settings) - game's / fast, fast by default. Your 6i.84 captures at Regret:
 * every texture's anisotropic filtering off took the two lights (pass 12) from 18.2 to 12.9 ms and their shadow (pass
 * 11) from 4.8 to 2.3 - and changed nothing else: the lighting pass, the colour pass and the shiny floor stayed the same.
 * Each light re-draws every surface it reaches, and every one of those draws filters its textures anisotropically
 * again - the same bump maps over and over, for a light's soft shading that the filtering does nothing visible for.
 * fast: inside the light and shadow passes (11 and 12) only, anisotropic filtering is off (the game's own texture
 * setup, through the Anisotropic Filtering hook - spooky_aniso); everywhere else it stays as you have it. */
#ifdef HOST
void host_set_lightf(s32 v) { o_lightf = v; }
void host_set_aniso_world(s32 v) { g_aniso_world = (u8)v; }
void host_capture_ab(void) { capture_ab_start(); }
void host_ab_state(s32 *ab, s32 *aniso, s32 *lightf) { *ab = g_ab; *aniso = o_aniso; *lightf = o_lightf; }
void host_set_aniso_opt(s32 v) { o_aniso = v; }
#endif
static void light_pass_in(u32 k)
{
	if (g_cap_live && (k == 11u || k == 12u)) g_ls[g_in_light ? (k == 11u ? 3 : 4) : (k == 11u ? 5 : 6)]++;   /* 6i.78 */
	if (!g_light_on || (k != 11u && k != 12u)) return;
	if (g_cap_live) g_ls[7]++;
	g_lvp[0] = (u32)g_lr[0]; g_lvp[1] = (u32)g_lr[1]; g_lvp[2] = (u32)(g_lr[2] - g_lr[0]); g_lvp[3] = (u32)(g_lr[3] - g_lr[1]);
	g_lvp[4] = 0u; g_lvp[5] = 0x3F800000u;                                         /* MinZ 0, MaxZ 1 */
#ifndef HOST
	d3d_set_viewport((u32)(unsigned long)g_lvp);
#endif
	g_light_set = 1;
}
void rs_target(u32 target);
static void light_pass_out(u32 k)
{
	(void)k;
	if (!g_light_set) return;
	g_light_set = 0;
#ifndef HOST
	rs_target(M32(ADDR_MAIN_TARGET));                                              /* the game's own viewport back */
#endif
}
#ifdef HOST
void host_cam_snap(void) { cam_snap(); }
s32 host_light_rect(s32 k) { return k < 0 ? g_light_on : g_lr[k]; }
void host_set_lights(s32 v) { o_lights = v; }
void host_light_pass(u32 k, s32 in) { if (in) light_pass_in(k); else light_pass_out(k); }
s32 host_light_set(void) { return g_light_set; }
#endif
static void rt_note(u32 k)
{
#ifndef HOST
	u32 dev = M32(ADDR_D3D_DEVICE), s, j = k == 1u ? 0u : 1u;
	if (!ptr_ok(dev)) return;
	s = M32(dev + 0x1A04u);
	if (ptr_ok(s)) { g_rtd[j][0] = M32(s + 4u); g_rtd[j][1] = M32(s + 0xCu); g_rtd[j][2] = M32(s + 0x10u); }
	s = M32(dev + 0x1A08u);
	if (ptr_ok(s)) { g_rtd[2][0] = M32(s + 4u); g_rtd[2][1] = M32(s + 0xCu); g_rtd[2][2] = M32(s + 0x10u); }
#else
	(void)k;
#endif
}
/* 6i.91: the capture's NO PIXELS DRAWN - each world and light pass (1, 3, 5, 8, 20; 11 - 14) drawn into a 1 x 1 corner:
 * every vertex still processed, next to no pixels - what's left is the pass's vertex and setup cost. Put back after
 * (function_14bc0: target 1 for passes 1 and 14 - the texture accumulation buffer - the main one for the rest). */
static u32 g_pxvp[6] = { 0u, 0u, 1u, 1u, 0u, 0x3F800000u };
static u8 g_px_set;
#ifdef HOST
s32 host_px_set(void) { return g_px_set; }
void host_cap_flags(s32 nat, s32 pxoff, s32 nomip) { g_cap_nat = (u8)nat; g_cap_pxoff = (u8)pxoff; g_cap_nomip = (u8)nomip; }
u32 host_nt_sum(s32 id) { return id < 0 ? g_nat_sum[2] : g_nt_sum[id]; }
#endif
void pass_in(u32 k)                              /* hooks.S pass_hook: before a pass's records are drawn */
{
	g_pass_now = (u8)(k < 256u ? k : 0u);
	light_pass_in(k);                                                       /* 6i.77 */
	if (g_cap_pxoff && (k == 1u || k == 3u || k == 5u || k == 8u || k == 20u || (k >= 11u && k <= 14u)))
	{
#ifndef HOST
		d3d_set_viewport((u32)(unsigned long)g_pxvp);
#endif
		g_px_set = (u8)(k == 1u || k == 14u ? 2u : 1u);
	}
	if (g_cap_nat) nt_stamp(g_in_light && (k == 11u || k == 12u) ? k + 51u : k < 64u ? k : 0u);   /* 6i.91 */
	if (g_cap_live && (k == 1u || k == 3u)) rt_note(k);                       /* 6i.99 */
	if (o_gpu_time) mark(k);
	else g_gt_t0 = tsc_lo();
}
static s32 nv2a_ok(void);
static void gpu_drain(void);
void pass_out(u32 k)                             /* ... and after */
{
	g_pass_now = 0;
	if (g_cap_nat) nt_stamp(28u);                                           /* 6i.91 */
	light_pass_out(k);                                                      /* 6i.77 */
	if (g_px_set)                                                           /* 6i.91 */
	{
#ifndef HOST
		rs_target(g_px_set == 2u ? 1u : M32(ADDR_MAIN_TARGET));
#endif
		g_px_set = 0;
	}
	if (o_gpu_time) mark(28);
	else if (k < 64u && !g_cap_frames) spooky_pass_cyc[k] += tsc_lo() - g_gt_t0;   /* 6i.90: not during a capture (its frames as normal) */
}
void prof_wait_before(void)                     /* 6i.3: hooks.S prof_wait_hook / prof_present_hook */
{
	if (!vsync_now() && o_fps_cap > 0 && o_fps_cap <= 12 && fps_unlocked())     /* 6i.31 / 6i.40: FPS Limit - the frame's time since the last */
	{
		u32 period = 733333333u / (u32)g_fps_caps[o_fps_cap];                /* 6i.46: the rate itself */
		while (tsc_lo() - g_cap_t < period) ;
	}
	g_cap_t = tsc_lo();
	g_pf_wat = tsc_lo();
}
void prof_wait_after(void) { g_pf_w += tsc_lo() - g_pf_wat; }
void prof_present_before(void)
{
	if (g_cap_nat) { nat_present(); } if (o_gpu_time && g_mk_last) mark(28); g_mk_last = 0; g_pf_wat = tsc_lo(); }   /* 6i.17 / 6i.18: the frame's last part closed */
/* 6i.42: triple buffering (the triple_buffering XBEs; patch_xbe.py --triple sets spooky_triple). Halo 2 HD's swap fix: the
 * game keeps its primary targets - surfaces [0] / [1], and textures over the same memory - as the back and front buffers
 * and swaps them each frame; with three buffers that's wrong, and anything using the last frame's picture (depth of
 * field) flickers. So right after the present's Swap (function_13f10 calls it): both surfaces = the device's current
 * back buffer (its buffer list, +0x1A14; a reference each - the old two released), both textures' data = its data. */
volatile u8 spooky_triple = 1;                    /* 6i.50: every build (6i.42 - 6i.49: the triple_buffering XBEs only); 6i.135: a setting */
/* ---- 6i.135: TOGGLE TRIPLE BUFFERING (RESTARTS) - 6i.136: a plain button, no on / off shown (your note: it read like a toggle) ----
 * The back buffers are made when the game creates its Direct3D device, at startup - so the setting can only change with a
 * restart. Performance Mode Settings > Toggle Triple Buffering (restarts) asks "This will force the game to restart, are
 * you sure?" (No first); Yes saves the other way in T:\spooky_triple.txt ("triple buffering: on" / "off"; U:, then Z:
 * if T: won't take it) and relaunches the game - the game's own XLaunchNewImageA("D:\default.xbe") (v1.0 0x2d07f2). At
 * the next start tb_load (hooks.S tb_create_hook, just before the device is created) reads it back; no file: on, as
 * every build since 6i.50. (The 1.25 MB the third buffer takes from the game's runtime data region stays set aside either
 * way - a fixed patch in the XBE.) */
static s32 g_tb_loaded;
#ifdef HOST
s32 g_host_tb_file = -1;                          /* the stand-in file: -1 none, 0 off, 1 on */
u32 g_host_relaunch;
#else
static const char *const c_tb_paths[3] = { "T:\\spooky_triple.txt", "U:\\spooky_triple.txt", "Z:\\spooky_triple.txt" };   /* 6i.152: read only, once, if spooky_settings.txt has no Triple Buffering line (the older builds' file) */
#endif
/* 6i.152: ONE settings file, spooky_settings.txt (T:, U: or Z: - the first that takes it), "label=value" a line for each
 * setting settings_kept says so for, Triple Buffering first. Written when the menu closes, after Reset to defaults and on
 * the Triple Buffering restart; read once at the start (with Triple Buffering, before the game's device is made). */
#ifdef HOST
char g_host_settings[4096]; s32 g_host_settings_len = -1;   /* the stand-in file: -1 none */
#else
static const char *const c_set_paths[3] = { "T:\\spooky_settings.txt", "U:\\spooky_settings.txt", "Z:\\spooky_settings.txt" };
#endif
static s32 str_eq(const char *a, const char *b);
static s32 s_len(const char *a) { s32 n = 0; while (a[n]) n++; return n; }
static s32 s_same(const char *a, const char *b, s32 n) { s32 i; for (i = 0; i < n; i++) if (a[i] != b[i]) return 0; return 1; }
static s32 settings_kept(s32 menu, const s_item *it)
{
	/* 6i.152: what the file keeps, by menu (g_menus' order): the main menu's Text scale only (the Superbounce / Ghost Viewer
	 * toggles start off); Launch / Teleport; Superbounce Viewer Settings; Camera Settings: Field of View only (the camera
	 * modes and the rest start at their defaults); Ghost Viewer Settings; Third Person Settings; Game Hacks: HUD Widescreen
	 * Fix only (Performance Mode starts off, Tick Rate at 30); Performance Mode Settings, all of them (Reset to defaults
	 * puts the mod's own defaults back and saves them). Not kept: Cheats, Skulls, Checkpoint Settings, Other Tools and the
	 * Trigger Volume Overlay Settings. */
	if (!it->value || (it->kind != K_TOGGLE && it->kind != K_INT && it->kind != K_INT10)) return 0;
	switch (menu)
	{
	case 0: return str_eq(it->label, "Text scale");
	case 2: return !str_eq(it->label, "Force Charged state");                 /* game state, not a setting */
	case 1: case 5: case 7: case 11: return 1;
	case 3: return str_eq(it->label, "Field of View");
	case 10: return str_eq(it->label, "HUD Widescreen Fix");                   /* (v2.0: Larger Caches is deliberately NOT kept - on at every start with 128 MB) */
	default: return 0;                                                         /* 4 Cheats, 6 Other Tools, 8 Skulls, 9 Checkpoint Settings, 12 Trigger Volume Overlay Settings, 13 */
	}
}
static s32 settings_text(char *b, s32 cap)                                  /* the file's text; its length */
{
	s32 at = 0, m, i;
	const char *t = "Triple Buffering=";
	while (*t && at < cap - 8) b[at++] = *t++;
	b[at++] = spooky_triple ? '1' : '0'; b[at++] = '\r'; b[at++] = '\n';
	for (m = 0; m < (s32)NOF(g_menus); m++)
		for (i = 0; i < g_menus[m].n; i++)
		{
			const s_item *it = &g_menus[m].items[i];
			char num[12]; s32 n = 0, v;
			if (!settings_kept(m, it)) continue;
			v = *it->value;
			t = it->label;
			if (at + s_len(t) + 16 > cap) return at;
			while (*t) b[at++] = *t++;
			b[at++] = '=';
			if (v < 0) { b[at++] = '-'; v = -v; }
			do { num[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 11);
			while (n) b[at++] = num[--n];
			b[at++] = '\r'; b[at++] = '\n';
		}
	return at;
}
static s32 settings_save(void)
{
	static char b[4096];
	s32 n = settings_text(b, (s32)sizeof(b)), saved = 0;
#ifdef HOST
	memcpy(g_host_settings, b, (u32)n); g_host_settings_len = n; saved = 1;
#else
	{
		s32 d; u32 put = 0;
		for (d = 0; d < 3 && !saved; d++)
		{
			void *h = CreateFileA_x(c_set_paths[d], 0x40000000u, 0u, 0, 2u, 0x80u, 0);   /* GENERIC_WRITE, CREATE_ALWAYS */
			if (h == (void *)-1) continue;
			if (WriteFile_x(h, b, (u32)n, &put, 0) && put == (u32)n) saved = 1;
			CloseHandle_x(h);
		}
	}
#endif
	return saved;
}
static s32 settings_apply_line(const char *key, s32 klen, s32 v)            /* one "key=value": 1 if a setting took it */
{
	s32 m, i;
	if (klen == 16 && s_same(key, "Triple Buffering", 16)) { spooky_triple = (u8)(v != 0); return 1; }
	for (m = 0; m < (s32)NOF(g_menus); m++)
		for (i = 0; i < g_menus[m].n; i++)
		{
			const s_item *it = &g_menus[m].items[i];
			if (!settings_kept(m, it) || s_len(it->label) != klen || !s_same(it->label, key, klen)) continue;
			if (it->kind == K_TOGGLE) *it->value = v ? 1 : 0;
			else { if (v < it->lo) v = it->lo; if (v > it->hi) v = it->hi; *it->value = v; }
			return 1;
		}
	return 0;
}
static s32 settings_parse(const char *b, s32 n)                              /* the file's text; the number of lines a setting took */
{
	s32 i = 0, took = 0;
	while (i < n)
	{
		s32 k0 = i, k1, v = 0, neg = 0, have = 0;
		while (i < n && b[i] != '=' && b[i] != '\r' && b[i] != '\n') i++;
		k1 = i;
		if (i < n && b[i] == '=')
		{
			i++;
			if (i < n && b[i] == '-') { neg = 1; i++; }
			while (i < n && b[i] >= '0' && b[i] <= '9') { v = v * 10 + (b[i] - '0'); i++; have = 1; }
			if (have && k1 > k0) took += settings_apply_line(b + k0, k1 - k0, neg ? -v : v);
		}
		while (i < n && b[i] != '\n') i++;
		if (i < n) i++;
	}
	return took;
}
static s32 g_settings_loaded;
static s32 settings_load(void)                                                /* 1 if the file had a Triple Buffering line */
{
	static char b[4096];
	s32 n = -1, had_tb = 0;
	if (g_settings_loaded) return g_settings_loaded > 0;
#ifdef HOST
	if (g_host_settings_len >= 0) { n = g_host_settings_len; memcpy(b, g_host_settings, (u32)n); }
#else
	{
		s32 d; u32 got = 0;
		for (d = 0; d < 3 && n < 0; d++)
		{
			void *h = CreateFileA_x(c_set_paths[d], 0x80000000u, 1u, 0, 3u, 0x80u, 0);   /* GENERIC_READ, OPEN_EXISTING */
			if (h == (void *)-1) continue;
			if (ReadFile_x(h, b, (u32)sizeof(b) - 1u, &got, 0) && got > 0u) n = (s32)got;
			CloseHandle_x(h);
		}
	}
#endif
	if (n > 0)
	{
		s32 i;
		settings_parse(b, n);
		for (i = 0; i + 16 <= n; i++) if (s_same(b + i, "Triple Buffering", 16)) { had_tb = 1; break; }
	}
	g_settings_loaded = had_tb ? 1 : -1;
	return had_tb;
}
void tb_load(void)
{
	if (g_tb_loaded) return;
	g_tb_loaded = 1;
	if (settings_load()) return;                                               /* 6i.152: Triple Buffering came with the settings */
#ifdef HOST
	if (g_host_tb_file >= 0) spooky_triple = (u8)g_host_tb_file;
#else
	{
		s32 d, i;
		for (d = 0; d < 3; d++)
		{
			char b[40];
			u32 got = 0;
			void *h = CreateFileA_x(c_tb_paths[d], 0x80000000u, 1u, 0, 3u, 0x80u, 0);   /* GENERIC_READ, OPEN_EXISTING */
			if (h == (void *)-1) continue;
			if (ReadFile_x(h, b, 39u, &got, 0) && got > 0u && got < 40u)
			{
				s32 off = 0, on = 0;
				b[got] = 0;
				for (i = 0; b[i]; i++) { if (b[i] == 'o' && b[i + 1] == 'f' && b[i + 2] == 'f') off = 1; if (b[i] == 'o' && b[i + 1] == 'n') on = 1; }
				if (off) spooky_triple = 0; else if (on) spooky_triple = 1;
			}
			CloseHandle_x(h);
			break;
		}
	}
#endif
}
static void tb_toggle_restart(void)
{
	u8 next = spooky_triple ? 0u : 1u, was = spooky_triple;
	s32 saved, relaunch;
	spooky_triple = next; saved = settings_save(); spooky_triple = was;        /* 6i.152: in spooky_settings.txt with the rest (this run keeps what it was made with) */
	/* 6i.196: YOUR 6i.194 RUN - the restart froze the 360 (Cairo, Larger Caches on). The run tool's probe: the game's
	 * processor stopped inside the Xbox kernel (EIP 0x80014A92) on a request to the emulator (code 0x104) that never
	 * came back, the kernel's free count at 0 - the relaunch itself (XLaunchNewImage -> the kernel's quick reboot) hung
	 * in the emulator. I do not know whether that is the 128 MB emulator's quick reboot as such or Larger Caches' blocks
	 * being in use at that moment: no earlier log of yours has a relaunch on the 360 to compare. Either way a relaunch
	 * from inside a level is a restart with the game's map files open - the very thing that corrupted the map cache
	 * once (9 Nov). 6i.196 stopped relaunching on the 360 for that reason. 6i.197: YOU WANT THE RESTART KEPT - it is back
	 * on both consoles, as it was. The hang is not fixed (its cause is not known); the log line is written before the
	 * relaunch is asked for, so a hang still shows where it was. */
	relaunch = 1;                                                            /* 6i.197: your word - the restart stays, on the 360 too */
#ifdef HOST
	g_host_tb_file = next; if (relaunch) g_host_relaunch++;
#endif
	{ s32 at = lput(0, "triple buffering: "); at = lput(at, next ? "on" : "off"); lput(at, !saved ? " NOT saved (no drive took the file) - no restart" : relaunch ? " from the next start - restarting the game" : " from the next start - saved; no relaunch on the 360 (quit to the dashboard and start the game again)"); log_write(); }
	if (!saved) { toast("Triple Buffering: couldn't save the setting"); return; }
	if (!relaunch) { toast("Triple Buffering: saved - quit to the dashboard and start the game again"); return; }
#ifndef HOST
	if (ADDR_XLAUNCH) ((u32 (__attribute__((stdcall)) *)(const char *, void *))ADDR_XLAUNCH)("D:\\default.xbe", 0);
	toast("Triple Buffering: saved - restart the game to apply it");   /* only if the relaunch came back */
#endif
}
#ifdef HOST
s32 host_triple(void) { return spooky_triple; }
void host_settings_save(void) { settings_save(); }                          /* 6i.152 */
void host_settings_reload(void) { g_settings_loaded = 0; settings_load(); }
s32 host_aniso(void) { return o_aniso; }
void host_tb_reload(void) { g_tb_loaded = 0; tb_load(); }
#endif
#ifdef HOST
u32 g_host_addref, g_host_release;
static void d3d_addref(u32 r) { (void)r; g_host_addref++; }
static void d3d_release(u32 r) { (void)r; g_host_release++; }
#else
static void d3d_addref(u32 r) { ((u32 (__attribute__((stdcall)) *)(u32))ADDR_D3D_ADDREF)(r); }
static void d3d_release(u32 r) { ((u32 (__attribute__((stdcall)) *)(u32))ADDR_D3D_RELEASE)(r); }
#endif
static void tb_after_swap(void)
{
	u32 dev, back, t0, t1, s0, s1;
	if (!spooky_triple) return;
	dev = M32(ADDR_D3D_DEVICE);
	if (!ptr_ok(dev)) return;
	back = M32(dev + 0x1A14u);
	t0 = M32(ADDR_RT_PRIMARY); t1 = M32(ADDR_RT_PRIMARY + 4u); s0 = M32(ADDR_RT_PRIMARY + 8u); s1 = M32(ADDR_RT_PRIMARY + 12u);
	if (!ptr_ok(back) || !ptr_ok(t0) || !ptr_ok(t1) || !ptr_ok(s0) || !ptr_ok(s1)) return;   /* not set up yet */
	d3d_release(s0); d3d_release(s1);
	d3d_addref(back); d3d_addref(back);
	gw32(ADDR_RT_PRIMARY + 8u, back); gw32(ADDR_RT_PRIMARY + 12u, back);
	gw32(t0 + 4u, M32(back + 4u)); gw32(t1 + 4u, M32(back + 4u));
}
void prof_present_after(void) { g_pf_p += tsc_lo() - g_pf_wat; guarded_call(tb_after_swap); if (g_cap_nat) nat_end(); }
static void ip_unblend(void)                     /* 6i.48: anything left blended put back (none should be outside a frame's draw) */
{
	if (g_ip_own_set) ip_own_restore();
	if (g_ip_blended) ip_restore();
	g_ip_dir = 0; g_ip_frame = 0;
}
/* ---- 6i.120: Show Stored Velocity (Other Tools) - HCM's storage detection (handoff: stored velocity detection and HUD
 * read-out) on the Xbox's own Havok body: its velocity at the motion's +0x40 (Havok's setter, v1.0 0x319890, stores
 * there; +0x50 is the angular one - MCC's layout less 0x10, as the centres at +0x60 / +0x70 are), its centre at +0x70.
 * Stored: the body stays put while its velocity is 0.5 u/s or more (6i.187: was 5) - two ticks in a row moving less than half of what
 * that velocity would carry it pin it; it stays stored within 0.05 units across / 0.3 up and down of where it was
 * pinned (Halo 2's stored body bobs ~0.11 every other tick), and ends when it leaves (the release) or after 15 ticks
 * under 0.5 u/s. A revert, a load, more than 5 ticks skipped, a teleport or a vehicle start it over. Read once a tick
 * (before each: the last tick's result), under the crash guard. ---- */
static s32 g_sv_have, g_sv_cand, g_sv_pinned, g_sv_slow;
static u32 g_sv_tick, g_sv_obj;
static float g_sv_prev[3], g_sv_anchor[3], g_sv_speed;
/* 6i.181: the stored velocity's DIRECTION - an arrow from the bottom centre of your capsule (your body's own position,
 * its feet) along the velocity the stored body holds, as long as the Superbounce Viewer's launch arrow is for the same
 * speed (0.08 units for each u/s, 3 at most). It is where that velocity points NOW - the way you leave at the moment
 * of release - not a path: gravity bends what follows. */
static float g_sv_vel[3], g_sv_foot[3];
/* 6i.188: your note - a small stored velocity sometimes takes a second or two to show. The rule was "two ticks in a row,
 * each moving under half of what the velocity would carry it". A stored body bobs a little from tick to tick; at 5 u/s
 * and up the bob is small beside the travel expected, but at 1 u/s the travel expected in a tick is 0.03 units and the bob
 * alone is more than half of that - so the rule kept starting over until two quiet ticks happened to come together.
 * Added, beside it: the body is where it was TWO ticks ago (within half of what the velocity would carry it in two
 * ticks) - the bob goes and comes back, so over two ticks it cancels. Either rule takes it: at the third reading,
 * always. Walking does not pass (two ticks of walking is two ticks of travel). */
static float g_sv_prev2[3];                      /* where it was two readings ago */
static s32 g_sv_n;                               /* readings in a row (one tick apart) */
/* 6i.192: YOUR NOTE - "don't show storage if the Havok object isn't asleep / at rest". The log of your 6i.191 round: eight
 * pick-ups, every one by the two-tick rule, every one while you were moving - "speed 1.13 (before 3.25), moved this tick
 * 0.076 of 0.038": a turn-round or a stop, which puts the body back near where it was two ticks before with a velocity
 * still on it. No rule about how far a body moved can tell that from a velocity held, and three builds of sharpening one
 * did not. What does: a stored velocity is a body that Havok has put to SLEEP (its island moved to the world's inactive
 * list - what the Havok Island Viewer draws green, what the Sleep Timer counts down to) while it still had a velocity.
 * Asleep, it cannot move; awake, nothing is stored. So now: "STORED" shows when the body's island is asleep and its
 * velocity is 0.5 u/s or more - at once, at the first reading - and goes when the island wakes. The movement rules no
 * longer decide anything; when one of them would have fired on a body that is awake, the log says so (to find out if a
 * stored velocity ever sits on an awake body - then this is too strict). */
static s32 g_sv_asleep;                          /* the body's island is on the world's inactive list (sv_sample) */
static float g_sv_psp, g_sv_psp2;                /* the speed at the reading before (6i.189), and the one before that (6i.190) */
static s32 sv_arrow(float *from, float *to)       /* 1: stored - the arrow's two ends */
{
	float len = 0.08f * g_sv_speed;
	s32 k;
	if (!g_sv_pinned || !(g_sv_speed > 0.5f)) return 0;
	if (len > 3.f) len = 3.f;
	for (k = 0; k < 3; k++) { from[k] = g_sv_foot[k]; to[k] = g_sv_foot[k] + g_sv_vel[k] / g_sv_speed * len; }
	return 1;
}
#define SV_MIN 0.5f                              /* 6i.187: your note - a stored velocity shows from 0.5 u/s (was 5) */
static void sv_reset(void) { g_sv_have = 0; g_sv_cand = 0; g_sv_pinned = 0; g_sv_slow = 0; }
static void sv_step(u32 obj, u32 tick, const float *p, const float *v, float dt)
{
	float sp, ex, d2 = 0.f, d, dh2, dz, lim;
	s32 k, two = 0;
	if (!g_sv_have || obj != g_sv_obj || tick <= g_sv_tick || tick > g_sv_tick + 5u)
	{
		sv_reset(); g_sv_have = 1; g_sv_obj = obj; g_sv_tick = tick; g_sv_n = 1; g_sv_psp2 = 0.f; g_sv_psp = fsqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);   /* (this reading's speed: it is the first of a row) */
		for (k = 0; k < 3; k++) g_sv_prev[k] = g_sv_prev2[k] = p[k];
		return;
	}
	if (!(dt > 0.001f && dt < 0.2f)) dt = 1.f / 30.f;
	sp = fsqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	ex = sp * dt * (float)(tick - g_sv_tick);                                 /* how far that velocity alone carries it */
	{
		float q2 = 0.f;
		s32 one = tick - g_sv_tick == 1u;                                    /* (a reading a tick: the two-tick rule needs them in a row) */
		for (k = 0; k < 3; k++) { float q = p[k] - g_sv_prev2[k]; q2 += q * q; }
		for (k = 0; k < 3; k++) { float q = p[k] - g_sv_prev[k]; d2 += q * q; g_sv_prev2[k] = g_sv_prev[k]; g_sv_prev[k] = p[k]; }
		g_sv_n = one ? g_sv_n + 1 : 1;
		/* 6i.189: your 6i.188 test - it showed on ordinary jumps. At a jump's first tick the body HAS its new velocity and
		 * has not moved yet (a velocity acts a tick after it is set), so "where it was two ticks ago, with a velocity"
		 * was true for that one tick. A stored velocity is one the body has HELD without going anywhere: the velocity
		 * must have been there on the tick before as well (the travel expected is then both ticks' worth). */
		/* 6i.190: your 6i.189 test - still "STORED" at ordinary jumps. Worked through with a jump's numbers: the velocity
		 * appears a tick before the body moves (2.6 u/s up, nothing moved), and the next tick the body has moved ONE tick's
		 * worth (0.082) - while the rule allowed half of TWO ticks' worth (0.084). One tick's lag at the start of any
		 * movement looked exactly like a velocity half held. So the velocity must now have been there for the whole two
		 * ticks measured - at this reading, the one before, AND the one before that (where the body is measured from).
		 * A jump's start has it at two of the three. A stored velocity is taken one tick later than in 6i.188: at the
		 * third reading that has it, a tenth of a second after it appears. */
		two = g_sv_n >= 3 && sp >= SV_MIN && g_sv_psp >= SV_MIN && g_sv_psp2 >= SV_MIN && fsqrt(q2) < 0.5f * (sp + g_sv_psp) * dt;
		g_sv_psp2 = g_sv_psp; g_sv_psp = sp;
	}
	d = fsqrt(d2);
	g_sv_tick = tick;
	lim = 4.f * ex + 1.f;
	if (d > (lim > 5.f ? lim : 5.f)) { sv_reset(); g_sv_have = 1; g_sv_obj = obj; g_sv_n = 1; for (k = 0; k < 3; k++) g_sv_prev2[k] = p[k]; return; }   /* a teleport */
	if (!g_sv_pinned)
	{
		/* 6i.185: your note - "sometimes doesn't pick up". Not changed: I do not know which case it misses. When the body
		 * holds a velocity it is not moving at (1 u/s or more, under 0.9 of the way it should go) for ten ticks running
		 * and this rule (5 u/s or more, under half) does not take it, the log says what it saw - at most every 5 s. */
		{
			static s32 held; static u32 said;
			if (sp >= SV_MIN && d < 0.9f * ex) held++; else held = 0;
			if (held >= 10 && !(sp >= SV_MIN && d < 0.5f * ex) && tick - said > 150u)
			{
				s32 at = lput(0, "stored velocity: not picked up - speed x100 "); at = lnum(at, (u32)(sp * 100.f));
				at = lput(at, ", moved x1000 "); at = lnum(at, (u32)(d * 1000.f)); at = lput(at, " of "); at = lnum(at, (u32)(ex * 1000.f));
				lput(at, " a tick (it takes 0.5 u/s or more and under half the way)"); log_write(); said = tick;
			}
		}
		if (two) g_sv_cand = 2;                                              /* 6i.188: back where it was two ticks ago, with a velocity */
		if (two || (sp >= SV_MIN && d < 0.5f * ex && g_sv_cand >= 1))         /* 6i.190: what is about to be taken as stored, in the log (at most every 2 s) */
		{
			static u32 said;
			if (tick - said > 60u)
			{
				s32 at = lput(0, g_sv_asleep ? "stored velocity: asleep, and also by the " : "stored velocity: NOT shown (the body is awake) - the "); at = lput(at, two ? "two-tick rule" : "two-in-a-row rule"); at = lput(at, g_sv_asleep ? "" : " would have");
				at = lput(at, " - speed x100 "); at = lnum(at, (u32)(sp * 100.f)); at = lput(at, " (before: "); at = lnum(at, (u32)(g_sv_psp2 * 100.f));
				at = lput(at, "), moved this tick x1000 "); at = lnum(at, (u32)(d * 1000.f)); at = lput(at, " of "); at = lnum(at, (u32)(ex * 1000.f));
				at = lput(at, "; velocity x100 "); at = lsnum(at, (s32)(v[0] * 100.f)); at = lput(at, " "); at = lsnum(at, (s32)(v[1] * 100.f)); at = lput(at, " "); at = lsnum(at, (s32)(v[2] * 100.f));
				log_write(); said = tick;
			}
		}
		if (two || (sp >= SV_MIN && d < 0.5f * ex)) { if (two || ++g_sv_cand >= 2) g_sv_cand = 2; }   /* (6i.192: the movement rules only count now) */
		else g_sv_cand = 0;
		if (g_sv_asleep && sp >= SV_MIN)                                     /* 6i.192: asleep with a velocity - stored */
		{
			g_sv_pinned = 1; g_sv_slow = 0; g_sv_speed = sp;
			for (k = 0; k < 3; k++) { g_sv_anchor[k] = p[k]; g_sv_vel[k] = v[k]; }
		}
		return;
	}
	if (!g_sv_asleep) { g_sv_pinned = 0; g_sv_cand = 0; return; }             /* 6i.192: woken - released */
	dh2 = (p[0] - g_sv_anchor[0]) * (p[0] - g_sv_anchor[0]) + (p[1] - g_sv_anchor[1]) * (p[1] - g_sv_anchor[1]);
	dz = p[2] - g_sv_anchor[2];
	if (dh2 > 0.05f * 0.05f || dz > 0.3f || dz < -0.3f) { g_sv_pinned = 0; g_sv_cand = 0; return; }   /* released */
	g_sv_speed = sp;
	for (k = 0; k < 3; k++) g_sv_vel[k] = v[k];
	if (sp < 0.5f) { if (++g_sv_slow >= 15) { g_sv_pinned = 0; g_sv_cand = 0; } }   /* fizzled out */
	else g_sv_slow = 0;
}
static void sv_sample(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR), obj, ents[16], motion;
	float p[3], v[3];
	s32 n, k;
	if (!ptr_ok(time) || !M8(time)) { sv_reset(); return; }
	obj = player_object();
	if (!obj || (s32)M32(obj + 0x14) != -1) { sv_reset(); return; }          /* no player, or in a vehicle */
	n = object_bodies(obj, ents);
	motion = n > 0 ? M32(ents[0] + 0x3C) : 0;
	if (!ptr_ok(motion)) { sv_reset(); return; }
	for (k = 0; k < 3; k++) { p[k] = MF(motion + 0x70 + 4 * (u32)k); v[k] = MF(motion + 0x40 + 4 * (u32)k); }
	/* 6i.181: this read "p == p && v == v && ..." - the first two (a test for "not a number") compile to the parity jump
	 * that goes the wrong way on the 360 for ordinary numbers (6i.175), which would reset the viewer there every tick so
	 * that nothing is ever shown, the arrow included. They added nothing: something that is not a number fails the four
	 * ordered tests that follow as well. Taken out; the same on the Xbox. */
	for (k = 0; k < 3; k++) if (!(p[k] < 1e6f && p[k] > -1e6f && v[k] < 1e5f && v[k] > -1e5f)) { sv_reset(); return; }
	for (k = 0; k < 3; k++) g_sv_foot[k] = MF(obj + 0x64 + 4 * (u32)k);       /* 6i.181: the capsule's bottom centre */
	{
		/* 6i.192: is the body asleep - its island (entity +0x44) on the world's list of inactive islands (+0x14 / +0x18) */
		u32 world = M32(ADDR_HAVOK_WORLD_PTR), isl = M32(ents[0] + 0x44), list, cnt, i;
		g_sv_asleep = 0;
		if (ptr_ok(world) && ptr_ok(isl) && ptr_ok(list = M32(world + 0x14)) && (cnt = M32(world + 0x18)) <= 4096u)
			for (i = 0; i < cnt; i++) if (M32(list + 4u * i) == isl) { g_sv_asleep = 1; break; }
	}
	sv_step(obj, M32(time + 8), p, v, MF(time + 4));
}
#ifdef HOST
s32 host_sv_step(u32 obj, u32 tick, float px, float py, float pz, float vx, float vy, float vz)
{
	float p[3], v[3];
	p[0] = px; p[1] = py; p[2] = pz; v[0] = vx; v[1] = vy; v[2] = vz;
	sv_step(obj, tick, p, v, 1.f / 30.f);
	return g_sv_pinned;
}
float host_sv_speed(void) { return g_sv_speed; }
void host_sv_asleep(s32 on) { g_sv_asleep = on; }
void host_sv_foot(float x, float y, float z) { g_sv_foot[0] = x; g_sv_foot[1] = y; g_sv_foot[2] = z; }
s32 host_sv_arrow(float *from, float *to) { return sv_arrow(from, to); }
void host_set_stored_vel(s32 on) { o_stored_vel = on; if (!on) sv_reset(); }
#endif
static void ct_tick(void);                       /* 6i.156: the Campaign Timer counts the game's ticks */
static void os_run(void);                        /* 6i.158: Object Spawning - the chosen object made here, between ticks */
static void fc_level_reset(void);                /* 6i.160: a level's first tick turns the free camera off */
static s32 g_hiv_nb, g_hiv_ni, g_hiv_npb; static void hiv_tick(void); static void hiv_tick_g(void) { hiv_tick(); }
static s32 g_os_fw_squad, g_os_fw_manual; static void os_follow_tick(void); static void os_follow_tick_g(void) { os_follow_tick(); }
void ip_tick_before(void) { u32 t = tsc_lo(); spooky_ptc_hold = 0; if (o_ctimer) ct_tick(); if (o_free_cam) fc_level_reset(); if (g_os_req) os_run(); if (g_os_fw_squad >= 0 && g_os_fw_manual) guarded_call(os_follow_tick_g); if (o_inf_ammo || g_ia_n) guarded_call(ia_hold_tick); if (o_hiv || g_hiv_npb) { if (guarded_call(hiv_tick_g)) { o_hiv = 0; g_hiv_nb = g_hiv_ni = g_hiv_npb = 0; } } if (o_stored_vel) { if (guarded_call(sv_sample)) sv_reset(); } else if (g_sv_have) sv_reset(); if (g_ip_own_set || g_ip_blended) guarded_call(ip_unblend); { u32 ts = tsc_lo(); if (guarded_call(ip_snapshot)) g_ip_valid = 0; g_ipt_snap += tsc_lo() - ts; g_ipt_ticks++; } t = tsc_lo() - t; g_pf_mod += t; g_pf_ip += t; }   /* the hooks (hooks.S): a bad read caught */
void ip_dir_before(void)                          /* 6i.48: hooks.S ip_dir_hook - the frame's blend, before the director */
{
	u32 t = tsc_lo();
	s32 u;
	if (g_ip_own_set || g_ip_blended) guarded_call(ip_unblend);
	if (guarded_call(ip_blend)) { }
	if (g_ip_frame)
	{
		for (u = 0; u < 4; u++) { g_ip_uhas[u] = 0; g_ip_uoff[u][0] = g_ip_uoff[u][1] = g_ip_uoff[u][2] = 0.f; }   /* no offset: built blended */
		g_ip_dir = 1;
		if (guarded_call(ip_own_blend)) guarded_call(ip_own_restore);
	}
	t = tsc_lo() - t; g_pf_mod += t; g_pf_ip += t;
}
/* 6i.50: Wireframe, Yelo's way (Yelo: Halo 2, Kornman00 / Bitterbanana - its A + D-pad Down). The game sets Direct3D's fill
 * mode to solid (0x1B02) each time it sets up a frame (v1.0 0x132d0: push 0x1b02 / call 0x3f5ba0, its render state 0x8b);
 * Yelo patched that push to 0x1B01, wireframe, so the GPU draws every triangle's edges - the whole scene at once, nothing
 * collected. Here the call is hooked instead (hooks.S fill_hook - no code written at run time, the 360 too) and asks
 * wire_fill: wireframe from the start of each frame's draw (ip_draw_before) until the HUD's pass begins (spooky_view) -
 * the world, objects and the first-person model as lines, the HUD and this menu solid. In a game only. */
volatile u8 spooky_fill_wire;
static u8 g_wire_arm;
static s32 wire_in_game(void) { u32 g = M32(ADDR_GAME_OPTIONS_PTR); return ptr_ok(g) && M32(g + 8) != 3; }
void wire_fill(void)                              /* xray (6i.52; 6i.50's Wireframe on) */
{
	spooky_fill_wire = (u8)(o_wire == 2 && g_wire_arm && wire_in_game());
}
/* 6i.55: Wireframe on - xray's lines on the world as drawn: each draw of the world's frame (the game's calls of Direct3D's
 * DrawIndexedVertices / DrawVertices - the level, objects, all of it, whichever pass draws it) drawn again at once as
 * lines (the fill mode wireframe), white (the frame buffer's logic op SET: every line pixel white whatever its shader
 * makes) and pulled toward the camera as the game pulls its decals (wireframe offset, -200). Each draw's own depth test:
 * lines behind something stay hidden. Points and lines (effects) aren't drawn again. hooks.S drawidx_hook / drawv_hook (6i.56:
 * DrawVertices takes its count and first vertex in eax / ecx);
 * on from the world's frame (ip_draw_before) until the HUD's (spooky_view). (6i.52 drew only the main pass again - the
 * level's surfaces are drawn outside it, so their lines never showed.)
 * 6i.172: 6i.171 also ADDED the lines to the picture (for the 360, whose GPU has no logic op, so its lines are drawn in
 * the surface's own colour). Your test: the 360's screen went white over time and the Xbox, fine before, got white and
 * half-transparent faces - the blend states were not put back as the game had them (what the mod reads as "the game's
 * value" for them is not what the game goes by). Taken out whole: the Xbox is as it was in 6i.170. On the 360
 * Wireframe "on" shows no lines again; white there needs the emulator to give the logic op a meaning. */
volatile u8 spooky_wire_on;
static u32 g_wl_zoff, g_wl_slope;
#ifndef HOST
void gfx_render_state(u32 state, u32 value);
#endif
void wire_line_begin(void)
{
#ifndef HOST
	g_wl_zoff = M32(ADDR_RS_CACHE + 4u * 0x4eu); g_wl_slope = M32(ADDR_RS_CACHE + 4u * 0x4du);   /* as the game left them */
	gfx_render_state(0x8b, 0x1B01);              /* FILLMODE: wireframe */
	gfx_render_state(0x50, 1);                   /* WIREFRAMEOFFSETENABLE */
	gfx_render_state(0x4d, 0);                   /* POLYGONOFFSETZSLOPESCALE: 0.0 */
	gfx_render_state(0x4e, 0xC3480000u);         /* POLYGONOFFSETZOFFSET: -200.0, the game's for decals */
	gfx_render_state(0x96, 0x150F);              /* LOGICOP: SET - white */
#endif
}
void wire_line_end(void)
{
#ifndef HOST
	gfx_render_state(0x96, 0);                   /* the logic op off */
	gfx_render_state(0x4e, g_wl_zoff);           /* the offsets as they were (the game's decals set their own) */
	gfx_render_state(0x4d, g_wl_slope);
	gfx_render_state(0x50, 0);
	gfx_render_state(0x8b, 0x1B02);              /* solid again */
#endif
}
void ip_draw_before(void) { u32 t = tsc_lo(); if (g_ia_n) guarded_call(ia_hold_draw); g_wire_arm = 1; spooky_wire_on = (u8)(o_wire == 1 && wire_in_game()); spooky_ptc_hold = (u8)fps_unlocked(); mark(29); if (g_pf_have == 1) { g_pf_d0 = t; g_pf_have = 2; } if (g_ip_dir) { if (g_ip_own_set) guarded_call(ip_own_restore); } else if (guarded_call(ip_blend)) { } t = tsc_lo() - t; g_pf_mod += t; g_pf_ip += t; }   /* 6i.48: blended at the director - your fields back for the draw */
void ip_draw_after(void) { u32 t = tsc_lo(), e; if (g_ip_own_set) guarded_call(ip_own_restore); guarded_call(ip_restore); g_ip_dir = 0; e = tsc_lo(); g_pf_mod += e - t; g_pf_ip += e - t; if (g_pf_have == 2) { g_pf_d1 = e; g_pf_have = 3; } }
#ifdef HOST
s32 host_ip_blended(void) { return g_ip_blended; }
s32 host_ip_own(s32 *set, s32 *dir) { *set = g_ip_own_set; *dir = g_ip_dir; return g_ip_own_n; }   /* 6i.48 */
s32 host_ip_hk(void) { return g_ip_hk_n; }   /* 6i.50 */
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
static void streak_reset(const char *why);
static void offsets_request(s32 save);
static void revert_to_player(void);
static void cycle_possession(void);
static void passion_check(void);
#ifdef HOST
void host_practice(s32 what) { practice(what); }
const char *host_toast(void) { return g_toast; }
void host_set_practice(s32 lf, s32 lu, s32 tf, s32 tu) { o_launch_fwd = lf; o_launch_up = lu; o_tp_fwd = tf; o_tp_up = tu; }
#endif
static const char *prac_name(s32 what)
{
	static const char *name[16] = { "", "create checkpoint", "revert checkpoint", "launch", "teleport", "third person on / off", "free camera on / off", "teleport to the camera", "revert to player possession", "passion check", "save offsets", "revert offsets", "cycle possession", "force double revert", "toggle triple buffering (restart)", "Larger Caches (restart the level)" };
	return what >= 1 && what <= 15 ? name[what] : "an action";
}
/* 6h.34: the practice actions run under the fault guard (the 360 crashed on Launch: after a level change the Ghost Viewer's
 * guard caught a bad read, then Launch read the same kind of memory - your unit's Havok bodies - unguarded). Where it got
 * to, for the log line if a bad read is caught */
static const char *g_prac_stage = "";
static s32 g_prac_what;
static void practice_guarded(void) { practice(g_prac_what); }
static void practice_log(s32 what)
{
	if (what == 3 || what == 4 || what == 7) g_trace_until = g_t30 + 90;   /* 3 s of trace after a launch / teleport */
	s32 at;
	if (what < 1 || what > 13) return;
	if (!o_diag && (what == 3 || what == 4 || what == 7)) return;           /* 6h.21: not each launch / teleport */
	at = lput(0, "practice: "); lput(at, prac_name(what));
	log_write();
}
static void sb_launch_delta(const float *d);
/* 6i.121: Checkpoint Settings > Force Double Revert - HCM's, on the Xbox's own checkpoint ring. The game keeps TWO saved
 * checkpoints (function_123e20 saves into slot = (slot + 1) % 2; the revert, function_123f60, loads
 * game_state_globals.slot and nothing else), so pointing the slot at the other one and asking for a revert goes back to
 * the checkpoint before the last. Guarded as HCM's notes warn: a revert the game refuses restarts the level (123f60's
 * failure sets reset_map) - so only when both exist: game_state_globals +2 (a checkpoint exists), +3 (the one before it
 * too; the save copies +2 there before advancing) and the other slot's "written" byte (g_5020f2[slot]). v1.0: slot
 * 0x4e3b64 (short), flags 0x4e3b62 / 63, written 0x5020f2. */
static void double_revert(void)
{
	s32 slot, other;
	if (!player_object()) { toast("Checkpoints: no game in progress"); return; }
	if (!ADDR_CP_SLOT) { toast("Force Double Revert: not mapped for this version yet"); return; }
	slot = (s32)(s16)M16(ADDR_CP_SLOT);
	if (slot != 0 && slot != 1) { toast("Force Double Revert: no checkpoints yet"); return; }
	other = 1 - slot;
	if (!M8(ADDR_CP_FLAG2) || !M8(ADDR_CP_FLAG2 + 1u) || !M8(ADDR_CP_VALID + (u32)other)) { toast("Force Double Revert: no earlier checkpoint saved"); return; }
	gw16(ADDR_CP_SLOT, (u16)other);
	gw8(ADDR_SAVE_MAP - 1, 1);                                               /* revert_map: the revert, from the other slot */
	toast("Double revert: back to the checkpoint before the last");
}
static void practice(s32 what)
{
	u32 obj, target, ents[16];
	practice_log(what);
	s32 parent, n, i;
	float f[3], r[3], up[3], l;
	char msg[80];
	if (what == 14) { tb_toggle_restart(); return; }                      /* 6i.135 */
	if (what == 15) { lc_toggle_restart(); return; }                      /* 6i.201 */
	if (what == 5) { o_third_person = !o_third_person; toast(o_third_person ? "Third person camera: on" : "Third person camera: off"); return; }
	if (what == 6) { o_free_cam = !o_free_cam; toast(o_free_cam ? "Free camera: on" : "Free camera: off"); return; }
	if (what == 7 && !(o_free_cam && g_fc_on)) { toast("Teleport to camera: the free camera is off"); return; }
	if (what == 8) { revert_to_player(); return; }
	if (what == 12) { cycle_possession(); return; }
	if (what == 13) { double_revert(); return; }                            /* 6i.121 */
	if (what == 9) { passion_check(); return; }
	if (what == 10 || what == 11) { offsets_request(what == 10); return; }
	if (what <= 2)
	{
		/* 6i.165: Revert works while you are dead (the wait before the respawn) - in a campaign game, with or without a
		 * body; Create still needs you alive: a checkpoint made while dead is one you cannot get out of */
		if (!player_object())
		{
			if (!is_campaign()) { toast("Checkpoints: no game in progress"); return; }
			if (what == 1) { toast("Checkpoint: not while you are dead"); return; }
		}
		gw8(what == 1 ? ADDR_SAVE_MAP : ADDR_SAVE_MAP - 1, 1);
		toast(what == 1 ? "Checkpoint created" : "Reverted to the last checkpoint");
		return;
	}
	g_prac_stage = "finding your unit";
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
	g_prac_stage = "listing its Havok bodies";
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
		g_prac_stage = "setting its bodies' velocity (Havok's setter)";
		for (i = 0; i < n; i++) set_body_velocity(M32(ents[i] + 0x3C), nv);
		g_prac_stage = "setting its own velocity";
		for (i = 0; i < 3; i++) gwf(target + 0x88 + 4 * (u32)i, nv[i]);
		{ s32 at = 0; const char *p = "Launched: "; while (*p) msg[at++] = *p++; at = put_i(msg, at, o_launch_fwd); p = " u/s forward, "; while (*p) msg[at++] = *p++; at = put_i(msg, at, o_launch_up); p = " up"; while (*p) msg[at++] = *p++; msg[at] = 0; }
	}
	else
	{
		float d[3];
		if (what == 7) for (i = 0; i < 3; i++) d[i] = g_fc_p[i] - MF(target + 0x64 + 4 * (u32)i);   /* to the free camera (HCM's) */
		else for (i = 0; i < 3; i++) d[i] = f[i] * (float)o_tp_fwd + up[i] * (float)o_tp_up;
		g_prac_stage = "moving it and its bodies";
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
	streak_reset(what == 3 ? "Launch used" : "Teleport used");          /* 6h.42: a streak running - reset (its message then) */
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
/* 6h.42: a bounce streak only counts at the game's own speed or faster, and without Launch or Teleport - slowing the game
 * (Game Speed under 1.0, Pause Game) or using Launch / Teleport during a streak resets it to 0 (and the record with it:
 * the record only grows with the streak). Said on screen and in the log */
static s32 sb_slowed(void);
static float g_sb_last_bounce;                  /* 6h.16: the last superbounce's speed (u/s) */
static s32 g_sb_have_last;
static float g_sb_speed = -1.f, g_sb_prev_vz, g_sb_prev_vx, g_sb_prev_vy;
static u32 g_sb_prev_tick = 0xffffffffu;
/* 6h.17 / 6h.19: a Launch of yours: the bounce test asks for a jump beyond the launch's push AND beyond your speed without
 * it - a seam hit on the very tick after a launch (a monster / dreadnought launched into) still counts, at its speed, while
 * a launch alone never does, whether it took (6h.17's test) or not (6h.18 took one the game hadn't applied for a bounce).
 * (6h.16 skipped the 3 ticks after a launch - and lost those.) */
/* ---- THE FLICKER AFTER THE INTRO VIDEO IS SKIPPED, ON THE 360 (6i.221 found it, 6i.222 fixed it; you confirmed) ----
 * After the video is skipped the game does not swap at all for about 1.3 seconds while the main menu loads (6i.221's
 * swap log: nothing between refresh 397 and 473). Through such a stretch the 360's emulator shows the last two
 * pictures in turn, where an Xbox holds the last one - and the last two were the video's last frame and the one dark
 * frame drawn after it: the flicker. Why the emulator does that is not known; the cure does not need it. When the
 * loading code's last "keep alive" swap (function_13f10, called at v1.0 0x163adb) goes out with the video over, the
 * game is made to draw and swap that same dark frame three more times - with the two calls the loading loop itself
 * makes (function_2232a0, then function_13f10) - so whatever is cycled through is one picture. Three, for triple
 * buffering's three buffers. About a tenth of a second, once a start, on both consoles.
 * 6i.223: the swap log itself (every swap, its caller, the frames finished before it) is gone - it had done its work. ---- */
#define SW_SETTLE 3
static s32 g_sw_settle_due;
#ifdef HOST
static u32 g_host_settle_draws, g_host_settle_swaps;
u32 host_settle_calls(u32 *swaps) { *swaps = g_host_settle_swaps; return g_host_settle_draws; }
#endif
/* before the game's own swap there (hooks.S sw_tail_hook): is the video over? */
void swap_tail_check(void)
{
#ifdef HOST
	if (!(M8(0x1E00) && M32(0x1E00 + 0xC))) g_sw_settle_due = 1;
#else
	if (!(M8(ADDR_BINK_GLOBALS) && M32(ADDR_BINK_GLOBALS + 0xC))) g_sw_settle_due = 1;
#endif
}
/* ... and after it */
void swap_settle(void)
{
	s32 k;
	if (!g_sw_settle_due) return;
	g_sw_settle_due = 0;
#ifndef HOST
	if (!ADDR_KEEPALIVE_DRAW || !ADDR_PRESENT) return;
#endif
	for (k = 0; k < SW_SETTLE; k++)
	{
#ifdef HOST
		g_host_settle_draws++; g_host_settle_swaps++;
#else
		((void (*)(void))ADDR_KEEPALIVE_DRAW)();
		((void (__attribute__((stdcall)) *)(u32, u32))ADDR_PRESENT)(0u, 0u);
#endif
	}
	if (SPOOKY_LOG) { s32 at = lput(0, "after the video: the same frame drawn and swapped "); at = lnum(at, SW_SETTLE); lput(at, " more times, so the pictures behind it are one picture"); log_write(); }
}
/* ---- 6i.223: YOUR NOTE - a hitch at every hard landing. It was the landing-contacts log (6i.210 / 6i.211): at each
 * superbounce it wrote a dozen or more lines to the log file - the game's contacts for the last three ticks, then the
 * viewer's own, worked out afresh - there and then, in the tick. A file write in the middle of a tick is a pause. The
 * log had done its work (the viewer's solver gives the Xbox's launch to a few hundredths of a u/s on the game's own
 * contacts; 37 landings, one more than 1 u/s out), so it is gone, with its hook on the game's contact routine. It is
 * kept, with what it found, in the package's "retired_landing_contacts" folder. ---- */
static u32 g_sb_object;
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
/* (g_sb_object - the player's object this frame, 0 = none - is declared above, with the landing contacts' log) */
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

static void lr_landed(u32 object, s32 streak_before);
static void lr_tick(u32 object);
static u32 g_fall_id;
static void ex_reset_shown(void);
static s32 g_lr_prev_streak;
/* once per main-loop frame: work through any new game ticks */
static s32 sb_slowed(void) { return o_pause || o_speed < 10; }
static void streak_reset(const char *why)
{
	char msg[80];
	s32 at = 0, n = g_sb_streak;
	const char *p = "Bounce streak reset: ";
	g_sb_streak = 0;
	if (n <= 0) return;
	while (*p) msg[at++] = *p++;
	while (*why && at < 79) msg[at++] = *why++;
	msg[at] = 0;
	toast(msg);
	at = lput(0, "superbounce: bounce streak of "); at = lnum(at, (u32)n); at = lput(at, " reset - "); lput(at, msg + 21);
	log_write();
}
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
	if (g_sb_streak > 0 && sb_slowed()) streak_reset(o_pause ? "the game was paused" : "the game was slowed");   /* 6h.42 (paused: no ticks - so here) */
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
				if (sb_slowed()) { toast("Bounce not counted: the game is slowed"); g_sb_streak = -1; }   /* 6h.42: not counted (the ++ below: 0) */
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
	/* 6h.34 (HCM 2026-11-01: a Force Launch isn't recorded as a bounce or a remembered spot): a Launch's own push isn't a
	 * landing - it's taken off the change the way the bounce test takes it off (6h.17), so a Launch while falling no longer
	 * counts for a ghost seam or logs a "landing"; a real landing the tick after one still does */
	if (charged && g_sb_prev_tick != 0xffffffffu && tick == g_sb_prev_tick + 1 && g_sb_prev_vz < -1.5f && vz - (g_sb_prev_vz + g_sb_ld[2]) > 1.5f)
	{
		if (g_la_ok) ghost_impact(g_la_a, g_la_b, vz >= 6.f);
		g_la_ok = 0;
		ex_reset_shown();                                                /* 6h.40: the next fall's verdict starts afresh (a bounce too) */
		if (o_sb_hud) lr_landed(object, g_lr_prev_streak);                 /* 6h.30 (6h.35: it reads the object itself) */
	}
	else lr_tick(object);
	g_lr_prev_streak = g_sb_streak;
	if (!airborne) ex_reset_shown();                                      /* 6h.30: a new fall's verdict starts afresh */
	/* a jump from standing: the first airborne tick's upward speed (as Cartographer measures it) */
	if (airborne && !g_sb_was_air && vz > 2.f && vz < 4.5f) g_sb_jump = vz;
	/* 6h.31: which fall this is (the exact air ratings are kept per fall) */
	if (airborne)
	{
		float vzp = g_sb_prev_vz - 4.1712f / 30.f, e = vz - vzp;
		if (!g_sb_was_air || g_sb_prev_tick == 0xffffffffu || tick != g_sb_prev_tick + 1 || e > 0.06f || e < -0.06f) g_fall_id++;
	}
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
/* 6i.61: RENDER SCALE (Performance Mode Settings) - 100 % (the game's, untouched) down to 50 % in tens. The world's view
 * (function_223080's record: its rectangle at +0xB0 - top, left, bottom, right - and the camera copy's at +0x3C) is cut
 * to that share of its width and height just before the views loop draws it (function_2b790, hooks.S rs_view_hook):
 * every screen-sized target's viewport (function_14bc0: from the frame storage's copy at +0x48) and the screen copies'
 * source area (function_34a90: the camera storage's at its +0x30) follow it, so the world is drawn into the top-left of
 * the screen at that size. Then, as the HUD's view begins (spooky_view), the game's own screen copy (function_352e0, one
 * sample) twice: the smaller picture stretched across target 3 (screen-sized; 6i.61 used 19, half-sized), and target
 * 3 back over the whole screen. The rectangles go back as built and the HUD draws at full size. One
 * view only (a full-screen rectangle at 0, 0), in a game, with Performance Mode. */
static u8 g_rs_on;
static u32 g_rs_full[2];
void rs_copy(u32 target, u32 multiple);          /* hooks.S: function_352e0 (stdcall, two) */
void rs_target(u32 target);                      /* hooks.S: function_14bc0 (al 1; target, 0) */
void rs_before(u32 view) { (void)view; }
void rs_stretch(void);        /* 6i.61's record cut - gone in 6i.63 (it didn't move the drawing) */
/* 6i.63: the cut at the main scene's start (hooks.S rs_scene_hook, at function_2bcd0's four calls in the frame's draw):
 * the rectangles the drawing actually follows - the frame storage's (function_14bc0: every screen target's viewport)
 * and the camera storage's (function_34a90: the screen copies' source area) - cut to the share, kept top-left. Your 6i.62
 * test: cutting the view record's rectangle left the world full-size (and the stretch then magnified its top-left). */
static u32 g_rs_cam[2];
/* 6i.68: the screen mapping itself. Your 6i.67 test (no change; what you aim at isn't what's shown): the world's vertex
 * shaders place everything on screen with twelve constants (c[-96]) that function_12fa0 works out from the frame
 * storage's view rectangle (+0x48) - set up before the scene starts, so cutting that rectangle at the scene's start only
 * cropped. Now, right after the cut, function_12fa0 runs again the way the game runs it (v1.0 0x39267: ecx the frame
 * storage's view +0x18, eax its projection +0x8C, then 0 and the main target pushed; ret 8 - hooks.S rs_view_consts):
 * the whole view mapped into the top-left share. After the stretch, once more with the rectangle back - the HUD's
 * mapping full again. (6i.66 / 6i.67's projection window is off - it was the wrong lever, and widened what's drawn.) */
void rs_view_consts(u32 camera, u32 projection, u32 mode);   /* hooks.S: function_12fa0 */
static u32 g_rs_cutv;
/* 6i.73: the sun's glow (your report: it grows as the scale drops). Things the game sizes in screen pixels - the sun's
 * glow among them - take the pixels-per-unit from the projection's half width and height (function_2fd90 stores them
 * at +0xb8 / +0xbc: the camera's projection, v1.0 0x4b9e14 / v1.5 0x4c6634, and the frame storage's copy at +0x8c),
 * which stay full-size while the world is drawn small: the glow came out too big, then stretched bigger still. While
 * the cut is on, both copies' half width and height are cut by the same share (after the screen mapping is redone,
 * which doesn't read them) and put back at the stretch. Anything else that sizes by them (level of detail too) sees
 * the smaller picture's size - as it should. */
static u32 g_rs_hwh[4];
static void rs_half_sizes(s32 cut)
{
	/* 6i.133: not under Adaptive Resolution Scaling - see rs_eff_set */
	if (cut && o_rs_adapt) { g_rs_hw_cut = 0; return; }
	static const u32 at[4] = { ADDR_PROJ_STORE + 0xB8, ADDR_PROJ_STORE + 0xBC, ADDR_FRAME_STORAGE + 0x8C + 0xB8, ADDR_FRAME_STORAGE + 0x8C + 0xBC };
	union { u32 u; float f; } v;
	s32 k;
	for (k = 0; k < 4; k++)
	{
		if (cut) { g_rs_hwh[k] = M32(at[k]); v.u = g_rs_hwh[k]; v.f = v.f * (float)g_rs_eff * 0.01f; gw32(at[k], v.u); }
		else if (g_rs_hw_cut) gw32(at[k], g_rs_hwh[k]);
	}
	g_rs_hw_cut = cut;
}
static void rs_cut_inner(void);
static void cam_snap(void);
void rs_cut(void) { spooky_light_loop = 0; rs_cut_inner(); cam_snap(); }   /* 6i.86: cleared at the scene's start */   /* 6i.77: the scene's camera kept for Dynamic Lights */
/* ---- 6i.130: Adaptive Resolution Scaling ----
 * With it on, the scale the world is drawn at follows the frame's cost: from 50 % up to the Render Scale setting (100 % for
 * a setting above that). A frame's work is its time less what it waited for the vblank (the frame profile: busy). The
 * budget is a 60 Hz frame (16.7 ms), or the FPS Limit's with VSync off; a frame over 95 % of it is late. The scale changes
 * only between frames. The FPS Counter shows it ("res 70%").
 * 6i.131 - your 6i.130 test: it dropped and stayed low. It only rose when the slowest frame, grown as if all of its work
 * were pixels, fitted the next step - but most of a Halo 2 frame isn't pixels, so that guess was far too high and the rise
 * hardly ever came. Now, every 30 frames (half a second):
 *  - 3 or more late: down 10 %. The next half second then says whether that helped: under 3 % less work means the frames
 *    are slow for another reason (the CPU, not the pixels) - the drop is undone and no drop comes for 10 seconds.
 *  - none late, and the wait over: up 10 %, tried for real. A rise that's followed within a second by a drop doubles the
 *    wait before the next try (1 s, 2, 4 .. 16); one that holds puts it back to 1 s; a half second with every frame under
 *    70 % (a lighter scene) cuts any wait to 1 s. So it comes back up by itself (50 to 100 % in about 3 seconds once the
 *    load goes), and where the top step doesn't fit it tries less and less often. */
static s32 g_rs_ad = 10, g_rs_ad_n, g_rs_ad_late, g_rs_ad_hold, g_rs_ad_back = 60, g_rs_ad_watch, g_rs_ad_nodrop;
static u32 g_rs_ad_sum, g_rs_ad_light, g_rs_ad_pre;
static s32 rs_top(void) { return o_rscale >= 5 && o_rscale < 10 ? o_rscale : 10; }
/* 6i.134: one ladder for both adaptive settings - 12 four passes, 11 two (Adaptive Supersampling, with Render Scale at 100 %+),
 * 10 .. 5 the scale (Adaptive Resolution Scaling); each part only when its setting is on */
static s32 ad_on(void) { return o_rs_adapt; }                              /* 6i.155: Render Scale's adaptive mode alone (Adaptive Supersampling gone with Supersampling) */
static s32 ad_top(void) { return rs_top(); }
static s32 ad_bottom(void) { return o_rs_adapt ? 5 : (rs_top() < 10 ? rs_top() : 10); }
static void rs_adapt_sample(u32 busy_us)
{
	s32 top = ad_top(), bot = ad_bottom(), s;
	u32 budget = (!vsync_now() && o_fps_cap > 0 && o_fps_cap <= 12 && g_fps_caps[o_fps_cap]) ? 1000000u / g_fps_caps[o_fps_cap] : vsync_now() == 2 ? 33333u : 16667u, avg;   /* 6i.176: at 30 fps the frame has twice the time */
	if (!ad_on() || !fps_unlocked() || !wire_in_game())
	{ g_rs_ad = top; g_rs_ad_n = g_rs_ad_late = 0; g_rs_ad_sum = g_rs_ad_light = g_rs_ad_pre = 0; g_rs_ad_hold = g_rs_ad_watch = g_rs_ad_nodrop = 0; g_rs_ad_back = 60; return; }
	if (g_rs_ad > top) g_rs_ad = top;
	if (g_rs_ad < bot) g_rs_ad = bot;
	g_rs_ad_n++; g_rs_ad_sum += busy_us;
	if (busy_us * 100u > budget * 95u) g_rs_ad_late++;
	if (busy_us * 100u < budget * 70u) g_rs_ad_light++;
	if (g_rs_ad_hold > 0) g_rs_ad_hold--;
	if (g_rs_ad_nodrop > 0) g_rs_ad_nodrop--;
	if (g_rs_ad_watch > 0 && --g_rs_ad_watch == 0) g_rs_ad_back = 60;          /* the last rise held: the short wait again */
	if (g_rs_ad_n < 30) return;
	avg = g_rs_ad_sum / (u32)g_rs_ad_n; s = g_rs_ad;
	if (g_rs_ad_light == (u32)g_rs_ad_n && g_rs_ad_hold > 60) { g_rs_ad_hold = 60; g_rs_ad_back = 60; }   /* a lighter scene: no long wait */
	if (g_rs_ad_pre)                                                            /* the half second after a drop: did it help? */
	{
		if ((u64)avg * 100u > (u64)g_rs_ad_pre * 97u && s < top) { g_rs_ad = s + 1; g_rs_ad_nodrop = 600; g_rs_ad_hold = 600; g_rs_ad_watch = 0; }
		g_rs_ad_pre = 0;
	}
	else if (g_rs_ad_late >= 3 && !g_rs_ad_nodrop && s > bot)
	{
		g_rs_ad = s - 1; g_rs_ad_pre = avg ? avg : 1u;
		g_rs_ad_back = g_rs_ad_watch ? (g_rs_ad_back * 2 > 960 ? 960 : g_rs_ad_back * 2) : 60;
		g_rs_ad_hold = g_rs_ad_back; g_rs_ad_watch = 0;
	}
	else if (!g_rs_ad_late && !g_rs_ad_hold && s < top) { g_rs_ad = s + 1; g_rs_ad_watch = 60; }
	g_rs_ad_n = g_rs_ad_late = 0; g_rs_ad_sum = g_rs_ad_light = 0;
}
/* 6i.133 - your 6i.131 test: a stutter each time the adaptive scale changed. Two causes taken out:
 *  - a 10 % change was a jump from one frame to the next. Now the drawn scale glides there 2 % a frame (60 to 70 %: five
 *    frames, ~0.1 s) - the step itself is spread out;
 *  - with each change the projection's half sizes (6i.73, scaled with the cut for the sun's glow - it never helped
 *    there) changed too, and the game picks models' and textures' detail from them: every step swapped detail levels
 *    across the view, and anything not already in the caches had to be read in. Under Adaptive Resolution Scaling they're
 *    now left full size, so the detail stays put whatever the scale (a fixed Render Scale keeps 6i.73's way).
 * A fixed Render Scale is still applied at once. */
static void rs_big_tidy(void);
static void rs_eff_set(void)
{
	s32 lv = ad_on() && fps_unlocked() ? g_rs_ad : rs_top(), want;
	rs_big_tidy();                                                          /* 6i.148: the big targets' housekeeping, at the frame's start */
	want = (lv > 10 ? 10 : lv) * 10;
	if (!ad_on() && o_rscale > 10 && o_rscale <= 20) want = o_rscale * 10;  /* 6i.148: a fixed Render Scale above 100 %: drawn at it (into the big targets) */
	if (!o_rs_adapt || !fps_unlocked()) { g_rs_eff = want; return; }
	if (g_rs_eff < want) g_rs_eff = g_rs_eff + 2 > want ? want : g_rs_eff + 2;
	else if (g_rs_eff > want) g_rs_eff = g_rs_eff - 2 < want ? want : g_rs_eff - 2;
}
#ifdef HOST
s32 host_rs_adapt_feed(u32 busy_us, s32 frames) { s32 k; for (k = 0; k < frames; k++) { rs_adapt_sample(busy_us); rs_eff_set(); } return ad_on() ? g_rs_ad : rs_top(); }
s32 host_rs_eff(void) { return g_rs_eff; }
/* a frame whose work is cpu_us plus gpu_us at 100 %, the pixel part shrinking with the scale squared; counts the scale's changes */
s32 host_rs_adapt_model(u32 cpu_us, u32 gpu_us, s32 frames, s32 *changes) { s32 k, was = g_rs_ad; for (k = 0; k < frames; k++) { rs_adapt_sample((cpu_us + gpu_us * (u32)(g_rs_eff * g_rs_eff) / 10000u));  rs_eff_set(); if (g_rs_ad != was) { (*changes)++; was = g_rs_ad; } } return g_rs_ad; }
void host_set_rs_adapt(s32 on) { o_rs_adapt = on; }
#endif
/* ---- 6i.148: RENDER SCALE ABOVE 100 % - DRAWN FOR REAL ----
 * Below 100 % the world is drawn into the top-left share of the game's own 640 x 480 targets and stretched; above it there
 * is nothing to draw into - until now. With the 128 MB console (your R63 emulator: 9.5 MB contiguous below 64 MB) the mod
 * makes three bigger targets of its own - the picture, the depth, and the light accumulation target (the game's target 1)
 * - 704 x 528 at 110 % up to 960 x 720 at 150 %, 4 bytes a pixel, in the kernel's contiguous write-combined memory (as
 * Supersampling's buffer, 6i.139). The game reaches its screen-sized targets through header pointers it reads every time
 * it binds one (function_14bc0) or samples one (function_14f60): the back buffers' surfaces g_50935c, their texture views
 * g_509354, the depth g_509364, and the colour views of the depth's memory - target 3 (g_509370 / g_509368) and target 24
 * (g_50936c). While the scene is drawn, those pointers are turned to the mod's own headers: copies of the game's, with the
 * bigger memory and size; target 1's entry in the render-target table (g_4b4b58[1]) gets the bigger memory and size in
 * place. The view rectangle, the screen mapping and the viewports follow the same way as the cut below 100 % does, only
 * bigger; the game's target size getter (function_01dd60, which says 640 x 480 for the screen-sized targets) answers with
 * the bigger size meanwhile (hooks.S rt_size_hook), so every screen-space copy the game makes stays 1:1. At the stretch
 * point (before the HUD, as below 100 %) the game's pointers are put back and the big picture is drawn over the back
 * buffer by the game's own screen copy, filtered down. No room (a real Xbox's 64 MB): drawn at 100 %, said once. */
static u32 g_bg_w, g_bg_h, g_bg_col, g_bg_dep, g_bg_acc, g_bg_colp, g_bg_depp, g_bg_accp;
#ifdef BIG_SIZE_TEST
static u32 g_bg_raw[3];                                     /* 6i.149q: the allocations themselves when the buffers sit inside them with guard gaps (0 = the buffer is the allocation) */
static u8 g_bg_gaps, g_bg_depfill, g_bg_fill_dep, g_bg_fill_col, g_bg_fill_acc;
#endif
static u32 g_rs_stretch_us, g_rs_cut_us, g_rs_copy_us, g_menu_us, g_hud_us;                     /* 6i.149t: the CPU time of the stretch (copies) and of the cut (swap) last frame, us */
static s32 g_bg_made, g_bg_nomem;                           /* the scale (11..15) the targets were made for / that found no room */
static u8 g_bg_on, g_bg_acc_on, g_bg_tex_kept, g_bg_inplace, g_bg_copyonly;
/* ---- 6i.199: CUTSCENES ABOVE 100 % - the larger picture stands down while a cutscene is blurring ----
 * 6i.198 enlarged the extra target a cutscene's blur uses (the table's entry 18). Your round: it took effect ("a
 * cutscene's blur target (18) at the larger size" is in the log) and the blur is still wrong - the far part of the
 * picture flickers between itself and what look like other textures, even with the game paused. So enlarging that
 * target is not enough, or not right; I do not yet know what else in that path is still 640 x 480 (one thing I have
 * seen and not followed through: with the cutscene's letterbox flag set, the game's screen-quad routine uses the fixed
 * bounds 0, 0, 640, 480 for the scene's target). Rather than a fourth guess at it, that code is OUT, and for now:
 * while that extra target exists - i.e. while a cutscene is blurring - the scene is drawn at the game's own 640 x 480,
 * exactly as with Render Scale at 100 %, and the larger picture comes back by itself when the target is gone. That is
 * a stand-down, not a fix: those shots are not sharper for the setting. It also stops the scene being drawn past the
 * end of that target's memory, which on a real Xbox (no clipping to the target) would write over whatever lies after
 * it. The log says once when it happens. ---- */
static u8 g_bg_t18_said;
static s32 rs_t18_present(void)
{
	u32 e18 = ADDR_RT_TABLE ? ADDR_RT_TABLE + 18u * 0x98u : 0u;
	return e18 && M32(e18 + 0x88) && (s32)M32(e18 + 0x78) > 0;
}
static u32 g_bg_x8, g_bg_tvar;
static u8 g_bg_colonly, g_bg_coldep, g_bg_depperm, g_bg_colskip, g_bg_dep_in;
static u32 g_bg_dep_saved;
static u32 g_bg_surf[3][6], g_bg_tex[3][5];                 /* surfaces: 0 the picture, 1 the depth, 2 target 3; textures: 0 the picture, 1 target 3, 2 target 24 */
static u32 g_bg_save[14];
static u32 g_bg_frames;
static u32 g_bg_rb[6], g_bg_rb_n;                           /* 6i.149 (test): pixels read back at the stretch - the big picture's centre, top-left, bottom-right; the back buffer's centre before and after the copy; the copy count */
static u32 rd_px(u32 va) { return ptr_ok(va & ~3u) || va >= 0x80000000u ? *(volatile u32 *)va : 0xDEADBEEFu; }
static u32 bg_px_va(u32 data_form, u32 va, u32 phys, u32 x, u32 y, u32 pitch) { u32 base = (data_form & 0xF0000000u) ? va : (0x80000000u | phys); return base + y * pitch + x * 4u; }
static u32 rs_big_sizeword(u32 w, u32 h) { return ((w * 4u / 64u - 1u) << 24) | ((h - 1u) << 12) | (w - 1u); }
static u32 rs_big_form(u32 game_data, u32 va, u32 phys) { return (game_data & 0xF0000000u) ? va : phys; }   /* in the form the game's own Data has */
static void rs_big_free(void)
{
#ifdef BIG_SIZE_TEST
	if (g_bg_raw[0]) kt_free(g_bg_raw[0]); else
#endif
	if (g_bg_col) kt_free(g_bg_col);
#ifdef BIG_SIZE_TEST
	if (g_bg_raw[1]) kt_free(g_bg_raw[1]); else
#endif
	if (g_bg_dep) kt_free(g_bg_dep);
#ifdef BIG_SIZE_TEST
	if (g_bg_raw[2]) kt_free(g_bg_raw[2]); else
#endif
	if (g_bg_acc) kt_free(g_bg_acc);
#ifdef BIG_SIZE_TEST
	g_bg_raw[0] = g_bg_raw[1] = g_bg_raw[2] = 0;
#endif
	g_bg_col = g_bg_dep = g_bg_acc = g_bg_colp = g_bg_depp = g_bg_accp = 0; g_bg_made = 0; g_bg_w = g_bg_h = 0;
}
static s32 rs_big_make(s32 s)
{
	u32 bytes;
	if (s < 11 || s > 20 || !ADDR_BB_SURF || !ADDR_T3_SURF) return 0;
	if (g_bg_made == s) return 1;
	if (g_bg_made) rs_big_free();
	if (g_bg_nomem == s) return 0;
	/* 6i.149y: the kernel's contiguous cap is raised to 128 MB here, only when a step that needs it is chosen (170 % and up:
	 * each buffer > the largest block below 64 MB), and only on a kernel that counts more than 96 MB with the XBE's limit
	 * flag clear. The allocator searches top-down, so the three buffers are the first GPU-visible memory above 64 MB - the
	 * probe for the emulator's GPU thread above 64 MB (R69L / R70 lifted its known bounds), with the game's own allocations
	 * left where they were */
	if (s >= 17 && !g_pfn && g_mem_mb > 96u && !(M32(0x10124u) & 4u) && !nv2a_ok()) kt_pfn_raise();   /* 6i.149z: on the emulator (nv2a_ok) never - its kernel carries the cap (R71); the write froze the 360 */
	g_bg_w = rs_big_w(s); g_bg_h = rs_big_h(s);
	/* 6i.149 (test): which size the emulator's texture path accepts - 110 % the real 704 x 528; 120 % the mod's own buffers at
	 * 640 x 480 (nothing bigger, only elsewhere); 130 % 640 wide x 528 tall; 140 % 704 wide x 480 tall; 150 % 704 x 528 */
#ifdef BIG_SIZE_TEST
	/* 6i.149f (test): the emulator never resolves the picture target into its texture (the log: the depth and the light target
	 * are, every frame; the picture isn't). The light target is read with a texture header whose format differs from the
	 * surface's, which gives the emulator two records for one memory and a copy between them at every bind. So: 110 % the
	 * picture's texture header reads the target as LIN_X8R8G8B8 (0x1E) instead of LIN_A8R8G8B8 (0x12) - same bytes, a
	 * different record; 120 % as 6i.148 (control, orange); 130 % the X8 view at 640 x 480 */
	/* 6i.149h: the emulator moves a render target's content into a texture only between TWO records of the same memory
	 * (the game's back buffer texture differs from its surface record; the mod's picture texture matched its record exactly,
	 * so it only ever got the restore). Harmless differences that make a second record: 110 % the texture one row shorter
	 * (704 x 527, same pitch); 120 % one column narrower (703 x 528); 130 % two mip levels declared (Format bits 16-19);
	 * 140 % 704 x 527 AND mip 2; 150 % control (as 6i.148). The picture at any of them = the route; then pick the cleanest. */
	/* 6i.149i: the emulator's records say the picture target leaves the scene with its "eDRAM holds content" flag already
	 * clear, so nothing is saved when the target is unbound; the game's bloom pass (after the first-person pass and its depth
	 * clear) already finds it stale. Suspect: the depth clear before the first-person pass resets the colour record. So the
	 * stretch is moved before it: 110 % Stretch Point 3 (before 13870), 120 % point 4 (as now - the control), 130 % point 1
	 * (before the 2nd UI), 140 % point 2 (after the 2nd UI). The picture at 110 % = confirmed. */
	/* 6i.149j: the emulator's AA policy (0x820DDCC8) calls a target "screen-sized" only at 640/720/1280/1440 wide and
	 * 480/576/960/1134 tall, and only those get the MSAA path, whose resolve is what the game's own screen reads sample;
	 * every other size is sampled from guest memory (the orange). 720 x 576 is the PAL screen - a size the emulator fully
	 * handles. So: 110 % = 720 x 576 (the view 12.5 % wider, 20 % taller; the copy brings it back), 120 % = 704 x 528
	 * (the control), 130 % = 720 x 576 at Stretch Point 3. */
	/* 6i.149m: 6i.149k showed a draw made at the stretch DOES reach the texture the copy samples (the light texture came out
	 * as a black 640 x 480 square; the strip beyond 640 x 480 stayed orange - the draw was clipped to 640 x 480). So now the
	 * same draw covers only the BOTTOM-RIGHT QUARTER of the picture target: the rest of the final picture then shows what the
	 * target's eDRAM held at the stretch - the world (then the save/fetch is the fault), or orange/garbage (then the world
	 * never reaches, or does not survive in, this target's eDRAM). 110 % = quarter draw at the stretch (Stretch Point 4);
	 * 120 % = the same at Stretch Point 3 (before the first-person pass); 130 % = quarter draw right at the scene's START
	 * (after the swap, before the game draws anything - should be overdrawn by the world if the world lands);
	 * 140 % = as 6i.148 (control). */
	/* 6i.149o: 6i.149n proved it - with the game's own depth kept, the world renders into the mod's colour buffer and comes
	 * back (640 x 480). The mod-made depth target is what loses the game's draws. Now: 110 % = colour only at 704 x 528 with
	 * the game's 640 x 480 depth (mismatched; the emulator clips to the depth's extent - shows whether the big colour path
	 * works at all); 120 % = colour AND depth ours at 640 x 480, nothing else (target 3 / 24 / light stay the game's):
	 * isolates "a second depth surface" at a matching size; 130 % = colour only at 640 x 480 (6i.149n's working case, the
	 * reference); 140 % = as 6i.148 (control). */
	/* 6i.149p: a second depth surface (even 640 x 480, even alone) loses the game's draws; the game's own depth works. Is it
	 * "two depth surfaces alive" or "a depth the emulator did not create at start-up"? The depth is now replaced ONCE, for
	 * the whole time the setting is on, never put back per frame (depth-permanent): 110 % = the game's picture as it is,
	 * depth permanently ours at 640 x 480 (the only depth in use: normal picture = a runtime depth is fine when it is the
	 * only one); 120 % = picture as it is, depth permanently ours at 704 x 528 (bigger than the picture: still normal?);
	 * 130 % = the goal: colour ours per frame at 704 x 528 + depth permanently ours at 704 x 528; 140 % = as 6i.148. */
	/* 6i.149r: what 6i.149q showed (video, 8 Nov): the world renders at 704 x 528 in every variant - the control too, because
	 * the four variants reused the same physical memory in one session and the depth's memory kept the far values that the
	 * prefill variant had put there. Then the picture decays over seconds: a white / yellow blob grows out of the first-person
	 * weapon's highlights and covers the screen, rock surfaces turn into jagged repeating bands, images drag. That is the picture
	 * of a scene whose depth (and colour) is never cleared: the glow pass feeds on last frame's picture, old depth hides new
	 * draws. The emulator's own log (R64L3, at the stop) shows the mod's pair treated as ordinary render targets: the depth is
	 * resolved to its guest memory (D3D Resolve) and restored from it at the next bind (0x820C1058, host-flags 2) - a bind-time
	 * restore that can undo the game's clear. The game's own 640 x 480 pair is the emulator's "screen" class and never goes
	 * that way. Test: if the restore is what the GPU draws against, writing the guest memory from the CPU before each scene
	 * is a clear the emulator cannot undo. All four variants in 6i.148's layout with guard gaps and the far prefill at creation:
	 *   110 %  the DEPTH's guest memory written to the far plane (0xFFFFFFFF) at every scene start, before the game's clears.
	 *          A clean, stable picture = the bind-time restore IS the depth the game draws against (and the fix is known).
	 *   120 %  110 + the picture (colour) written black each scene start - whether the haze is its own leftover.
	 *   130 %  120 + target 1 (the light / albedo target) black too.
	 *   140 %  no per-scene writes (= 6i.149q's 130 %): the decaying picture, for comparison. */
	g_bg_inplace = 0;
	g_bg_colonly = 0;
	g_bg_coldep = 0;
	g_bg_depperm = 0;
	g_bg_colskip = 0;
	g_bg_gaps = 1;
	g_bg_depfill = 1;
	g_bg_fill_dep = (s == 11 || s == 12 || s == 13);
	g_bg_fill_col = (s == 12 || s == 13);
	g_bg_fill_acc = (s == 13);
	g_bg_w = 704u; g_bg_h = 528u;
	g_bg_x8 = 0;
	g_bg_tvar = 0;
	o_rs_point = 4;
#endif
	bytes = g_bg_w * g_bg_h * 4u;
#ifdef BIG_SIZE_TEST
	if (g_bg_gaps)
	{	/* 6i.149q: each buffer 64 KB-aligned inside its own allocation, >= 64 KB unused on both sides */
		u32 k, *dst[3];
		dst[0] = &g_bg_col; dst[1] = &g_bg_dep; dst[2] = &g_bg_acc;
		for (k = 0; k < 3; k++)
		{
			u32 raw = kt_alloc_wc(bytes + 0x30000u);
			g_bg_raw[k] = raw;
			*dst[k] = raw ? ((raw + 0x1FFFFu) & ~0xFFFFu) : 0u;   /* first 64 KB boundary at least 64 KB in */
		}
	}
	else
#endif
	{ g_bg_col = kt_alloc_wc(bytes); g_bg_dep = kt_alloc_wc(bytes); g_bg_acc = kt_alloc_wc(bytes); }
	if (g_bg_col) g_bg_colp = kt_phys(g_bg_col);
	if (g_bg_dep) g_bg_depp = kt_phys(g_bg_dep);
	if (g_bg_acc) g_bg_accp = kt_phys(g_bg_acc);
	if (!g_bg_col || !g_bg_dep || !g_bg_acc || !g_bg_colp || !g_bg_depp || !g_bg_accp)
	{
		char t[96], *p;
		rs_big_free(); g_bg_nomem = s;
		p = say_s(t, "Render Scale "); p = say_u(p, (u32)s * 10u); p = say_s(p, "%: no room for its 3 x "); p = say_u(p, bytes / 1024u); say_s(p, " KB - drawn at 100%");
		toast(t);
		return 0;
	}
	g_bg_made = s;
#ifdef BIG_SIZE_TEST
	{	/* 6i.149e (test): the picture buffer filled with a solid orange by the CPU, so a copy that reads the memory (not the
		 * render target) shows orange, one that reads nothing shows black, and one that reads the target shows the world */
		u32 base = g_bg_col, n = bytes / 4u, k;
		for (k = 0; k < n; k++) *(volatile u32 *)(base + 4u * k) = 0xFFFF8000u;
		if (g_bg_depfill) { base = g_bg_dep; for (k = 0; k < n; k++) *(volatile u32 *)(base + 4u * k) = 0xFFFFFFFFu; }   /* 6i.149q: the depth's memory = the far plane */
	}
#endif
	return 1;
}
static void rs_big_tidy(void)                              /* at a frame's start: pointers left turned (no stretch came) put back; targets no longer wanted freed */
{
	if (g_bg_on) { void rs_big_swap(s32 on, s32 keep_tex); rs_big_swap(0, 0); }
#ifdef BIG_SIZE_TEST
	if (g_bg_dep_in && (g_bg_made == 0 || o_rscale != g_bg_made || ad_on() || !wire_in_game())) { gw32(ADDR_MAIN_DEPTH, g_bg_dep_saved); g_bg_dep_in = 0; }   /* 6i.149p: the game's depth back before the targets go */
#endif
	if (g_bg_made && (o_rscale != g_bg_made || ad_on())) rs_big_free();
	if (g_bg_nomem && o_rscale != g_bg_nomem) g_bg_nomem = 0;   /* another scale: tried again */
}
static u32 g_bg_ip[14];                                     /* 6i.149 (test): the game's headers' own Data / Size words, edited in place */
#ifdef BIG_SIZE_TEST
#ifndef HOST
void rs_copy(u32 target, u32 multiple);
void rs_target(u32 target);
static void bg_quarter_draw(u32 main)                     /* 6i.149m: the light target's texture drawn into the bottom-right quarter of the picture target */
{
	u32 full = g_bg_h | (g_bg_w << 16), quarter_tl = (g_bg_h / 2u) | ((g_bg_w / 2u) << 16);
	gw32(ADDR_FRAME_STORAGE + 0x48, quarter_tl); gw32(ADDR_FRAME_STORAGE + 0x4C, full);   /* viewport = bottom-right quarter */
	gw32(ADDR_MAIN_TARGET, 1u);
	rs_copy(0, 0);
	gw32(ADDR_MAIN_TARGET, main);
	gw32(ADDR_FRAME_STORAGE + 0x48, 0u); gw32(ADDR_FRAME_STORAGE + 0x4C, full);
	rs_target(main);
}
#endif
static u32 g_bg_touches;
static void bg_touch(void)                                  /* 6i.149d (test): one dword of every 64 KB page of the picture written back as it is (a CPU write the emulator sees) */
{
	u32 base = (g_bg_tex[0][1] & 0xF0000000u) ? g_bg_col : (0x80000000u | g_bg_colp), bytes = g_bg_w * g_bg_h * 4u, o;
	for (o = 0; o < bytes; o += 0x10000u) { volatile u32 *p = (volatile u32 *)(base + o + 0x40u); *p = *p; }
	g_bg_touches++;
}
#endif
static void bg_hdr_set(u32 hdr, u32 data, u32 size) { gw32(hdr + 4, data); gw32(hdr + 0x10, size); }
#ifdef BIG_SIZE_TEST
static void bg_fill32(u32 base, u32 bytes, u32 v)          /* 6i.149r: a buffer's guest memory written by the CPU, two dwords a step */
{
	volatile u32 *p = (volatile u32 *)base, *e = (volatile u32 *)(base + bytes);
	while (p < e) { p[0] = v; p[1] = v; p += 2; }
}
static void bg_scene_fill(void)                              /* 6i.149r: at the scene start, after the swap: the "clear" the emulator cannot undo */
{
	u32 bytes = g_bg_w * g_bg_h * 4u;
	if (!g_bg_on || !(g_bg_fill_dep || g_bg_fill_col || g_bg_fill_acc)) return;
	if (g_bg_fill_dep && g_bg_dep) bg_fill32(g_bg_dep, bytes, 0xFFFFFFFFu);
	if (g_bg_fill_col && g_bg_col) bg_fill32(g_bg_col, bytes, 0xFF000000u);
	if (g_bg_fill_acc && g_bg_acc && g_bg_acc_on) bg_fill32(g_bg_acc, bytes, 0xFF000000u);
}
#endif
void rs_big_swap(s32 on, s32 keep_tex)
{
	u32 e = ADDR_RT_TABLE ? ADDR_RT_TABLE + 0x98u : 0u;
	s32 k;
	if (ADDR_RT_CACHE) { gw32(ADDR_RT_CACHE, 0u); gw32(ADDR_RT_CACHE + 4, 0u); }   /* 6i.149 (test): the game's bind cache (g_51f3f4 / f8) cleared: the next bind reaches Direct3D */
	if (on && g_bg_inplace)
	{
		u32 s0 = M32(ADDR_BB_SURF), dp = M32(ADDR_MAIN_DEPTH), t3s = M32(ADDR_T3_SURF), t3t = M32(ADDR_T3_TEX), t24 = M32(ADDR_T24_TEX), tx0 = M32(ADDR_BB_TEX), tx1 = M32(ADDR_BB_TEX + 4);
		u32 size = rs_big_sizeword(g_bg_w, g_bg_h), hd[7], j;
		if (g_bg_on || !ptr_ok(s0) || !ptr_ok(dp) || !ptr_ok(t3s) || !ptr_ok(t3t) || !ptr_ok(t24) || !ptr_ok(tx0) || !ptr_ok(tx1)) return;
		hd[0] = s0; hd[1] = dp; hd[2] = t3s; hd[3] = t3t; hd[4] = t24; hd[5] = tx0; hd[6] = tx1;
		for (j = 0; j < 7; j++) { g_bg_ip[2 * j] = M32(hd[j] + 4); g_bg_ip[2 * j + 1] = M32(hd[j] + 0x10); }
		g_bg_save[0] = s0; g_bg_save[1] = dp; g_bg_save[2] = t3s; g_bg_save[3] = t3t; g_bg_save[4] = t24; g_bg_save[5] = tx0; g_bg_save[6] = tx1;
		bg_hdr_set(s0, rs_big_form(g_bg_ip[0], g_bg_col, g_bg_colp), size);
		bg_hdr_set(dp, rs_big_form(g_bg_ip[2], g_bg_dep, g_bg_depp), size);
		bg_hdr_set(t3s, rs_big_form(g_bg_ip[4], g_bg_dep, g_bg_depp), size);
		bg_hdr_set(t3t, rs_big_form(g_bg_ip[6], g_bg_dep, g_bg_depp), size);
		bg_hdr_set(t24, rs_big_form(g_bg_ip[8], g_bg_dep, g_bg_depp), size);
		bg_hdr_set(tx0, rs_big_form(g_bg_ip[10], g_bg_col, g_bg_colp), size);
		bg_hdr_set(tx1, rs_big_form(g_bg_ip[12], g_bg_col, g_bg_colp), size);
		g_bg_acc_on = 0;
		if (e && M32(e + 0x88) && (s32)M32(e + 0x78) > 0 && !M8(e + 0x95))
		{
			g_bg_save[7] = M32(e + 8); g_bg_save[8] = M32(e + 0x14); g_bg_save[9] = M32(e + 0x1C); g_bg_save[10] = M32(e + 0x28); g_bg_save[11] = M32(e + 0x80); g_bg_save[12] = M32(e + 0x84);
			gw32(e + 8, rs_big_form(g_bg_save[7], g_bg_acc, g_bg_accp)); gw32(e + 0x14, size);
			gw32(e + 0x1C, rs_big_form(g_bg_save[9], g_bg_acc, g_bg_accp)); gw32(e + 0x28, size);
			gw32(e + 0x80, g_bg_w); gw32(e + 0x84, g_bg_h);
			g_bg_acc_on = 1;
		}
		for (k = 0; k < 5; k++) g_bg_tex[0][k] = M32(tx0 + 4u * k);        /* (for the readback's Data form) */
		g_bg_on = 1; g_bg_tex_kept = 0; g_bg_frames++;
		return;
	}
	if (!on && g_bg_inplace)
	{
		if (!g_bg_on && !g_bg_tex_kept) return;
		if (g_bg_on)
		{
			bg_hdr_set(g_bg_save[0], g_bg_ip[0], g_bg_ip[1]); bg_hdr_set(g_bg_save[1], g_bg_ip[2], g_bg_ip[3]); bg_hdr_set(g_bg_save[2], g_bg_ip[4], g_bg_ip[5]);
			bg_hdr_set(g_bg_save[3], g_bg_ip[6], g_bg_ip[7]); bg_hdr_set(g_bg_save[4], g_bg_ip[8], g_bg_ip[9]);
			if (g_bg_acc_on && e)
			{
				gw32(e + 8, g_bg_save[7]); gw32(e + 0x14, g_bg_save[8]); gw32(e + 0x1C, g_bg_save[9]); gw32(e + 0x28, g_bg_save[10]); gw32(e + 0x80, g_bg_save[11]); gw32(e + 0x84, g_bg_save[12]);
				g_bg_acc_on = 0;
			}
			g_bg_on = 0;
		}
		if (keep_tex) { g_bg_tex_kept = 1; return; }
		bg_hdr_set(g_bg_save[5], g_bg_ip[10], g_bg_ip[11]); bg_hdr_set(g_bg_save[6], g_bg_ip[12], g_bg_ip[13]); g_bg_tex_kept = 0;
		return;
	}
	if (on)
	{
		u32 s0 = M32(ADDR_BB_SURF), dp = M32(ADDR_MAIN_DEPTH), t3s = M32(ADDR_T3_SURF), t3t = M32(ADDR_T3_TEX), t24 = M32(ADDR_T24_TEX), tx0 = M32(ADDR_BB_TEX), tx1 = M32(ADDR_BB_TEX + 4);
		u32 size = rs_big_sizeword(g_bg_w, g_bg_h);
#ifdef BIG_SIZE_TEST
		if (g_bg_dep_in) dp = g_bg_dep_saved;                            /* 6i.149p: the game's header is the one to copy from, not ours */
#endif
		if (g_bg_on || !ptr_ok(s0) || !ptr_ok(dp) || !ptr_ok(t3s) || !ptr_ok(t3t) || !ptr_ok(t24) || !ptr_ok(tx0) || !ptr_ok(tx1)) return;
		g_bg_save[0] = s0; g_bg_save[1] = dp; g_bg_save[2] = t3s; g_bg_save[3] = t3t; g_bg_save[4] = t24; g_bg_save[5] = tx0; g_bg_save[6] = tx1;
		for (k = 0; k < 6; k++) { g_bg_surf[0][k] = M32(s0 + 4u * k); g_bg_surf[1][k] = M32(dp + 4u * k); g_bg_surf[2][k] = M32(t3s + 4u * k); }
#ifdef BIG_SIZE_TEST
		if (g_bg_dep_in) { g_bg_surf[1][1] = rs_big_form(g_bg_surf[1][1], g_bg_dep, g_bg_depp); g_bg_surf[1][4] = size; }   /* keep our depth header as it is */
#endif
		for (k = 0; k < 5; k++) { g_bg_tex[0][k] = M32(tx0 + 4u * k); g_bg_tex[1][k] = M32(t3t + 4u * k); g_bg_tex[2][k] = M32(t24 + 4u * k); }
		g_bg_surf[0][1] = rs_big_form(g_bg_surf[0][1], g_bg_col, g_bg_colp); g_bg_surf[0][4] = size;
		g_bg_surf[1][1] = rs_big_form(g_bg_surf[1][1], g_bg_dep, g_bg_depp); g_bg_surf[1][4] = size;
		g_bg_surf[2][1] = rs_big_form(g_bg_surf[2][1], g_bg_dep, g_bg_depp); g_bg_surf[2][4] = size;
		g_bg_tex[0][1] = rs_big_form(g_bg_tex[0][1], g_bg_col, g_bg_colp); g_bg_tex[0][4] = size;
#ifdef BIG_SIZE_TEST
		if (g_bg_x8) g_bg_tex[0][3] = (g_bg_tex[0][3] & ~0xFF00u) | (g_bg_x8 << 8);   /* 6i.149g: another format's view of the picture */
		if (g_bg_tvar == 11 || g_bg_tvar == 14) g_bg_tex[0][4] = rs_big_sizeword(g_bg_w, g_bg_h - 1u);           /* 6i.149h: one row shorter */
		if (g_bg_tvar == 12) g_bg_tex[0][4] = (rs_big_sizeword(g_bg_w, g_bg_h) & ~0xFFFu) | (g_bg_w - 2u);        /* one column narrower, pitch kept */
		if (g_bg_tvar == 13 || g_bg_tvar == 14) g_bg_tex[0][3] = (g_bg_tex[0][3] & ~0xF0000u) | 0x20000u;        /* two mip levels declared */
#endif
		g_bg_tex[1][1] = rs_big_form(g_bg_tex[1][1], g_bg_dep, g_bg_depp); g_bg_tex[1][4] = size;
		g_bg_tex[2][1] = rs_big_form(g_bg_tex[2][1], g_bg_dep, g_bg_depp); g_bg_tex[2][4] = size;
		g_bg_acc_on = 0;
#ifdef BIG_SIZE_TEST
		if (g_bg_depperm && !g_bg_dep_in)                              /* 6i.149p: the depth replaced once, kept across frames */
		{
			g_bg_dep_saved = dp;
			gw32(ADDR_MAIN_DEPTH, (u32)(unsigned long)g_bg_surf[1]);
			g_bg_dep_in = 1;
		}
		if (g_bg_colskip)                                               /* 6i.149p: the picture stays the game's; only the depth is ours */
		{
			g_bg_on = 1; g_bg_tex_kept = 0; g_bg_frames++;
			g_bg_save[0] = s0; g_bg_save[5] = tx0; g_bg_save[6] = tx1;
			return;
		}
		if (g_bg_colonly || g_bg_coldep)                               /* 6i.149n/o: only the colour (and, for coldep, the depth) are ours; target 3 / 24 and the light target stay the game's */
		{
			gw32(ADDR_BB_SURF, (u32)(unsigned long)g_bg_surf[0]);
			if (g_bg_coldep) gw32(ADDR_MAIN_DEPTH, (u32)(unsigned long)g_bg_surf[1]);
			gw32(ADDR_BB_TEX, (u32)(unsigned long)g_bg_tex[0]); gw32(ADDR_BB_TEX + 4, (u32)(unsigned long)g_bg_tex[0]);
			g_bg_on = 1; g_bg_tex_kept = 0; g_bg_frames++;
			return;
		}
#endif
		if (e && M32(e + 0x88) && (s32)M32(e + 0x78) > 0 && !M8(e + 0x95))   /* the light accumulation target: made, in use, one element at least */
		{
			g_bg_save[7] = M32(e + 8); g_bg_save[8] = M32(e + 0x14); g_bg_save[9] = M32(e + 0x1C); g_bg_save[10] = M32(e + 0x28); g_bg_save[11] = M32(e + 0x80); g_bg_save[12] = M32(e + 0x84);
			gw32(e + 8, rs_big_form(g_bg_save[7], g_bg_acc, g_bg_accp)); gw32(e + 0x14, size);
			gw32(e + 0x1C, rs_big_form(g_bg_save[9], g_bg_acc, g_bg_accp)); gw32(e + 0x28, size);
			gw32(e + 0x80, g_bg_w); gw32(e + 0x84, g_bg_h);
			g_bg_acc_on = 1;
		}
		gw32(ADDR_BB_SURF, (u32)(unsigned long)g_bg_surf[0]); gw32(ADDR_MAIN_DEPTH, (u32)(unsigned long)g_bg_surf[1]); gw32(ADDR_T3_SURF, (u32)(unsigned long)g_bg_surf[2]);
		gw32(ADDR_T3_TEX, (u32)(unsigned long)g_bg_tex[1]); gw32(ADDR_T24_TEX, (u32)(unsigned long)g_bg_tex[2]);
		gw32(ADDR_BB_TEX, (u32)(unsigned long)g_bg_tex[0]); gw32(ADDR_BB_TEX + 4, (u32)(unsigned long)g_bg_tex[0]);
		g_bg_on = 1; g_bg_tex_kept = 0; g_bg_frames++;
		return;
	}
	if (!g_bg_on && !g_bg_tex_kept) return;
	if (g_bg_on)
	{
#ifdef BIG_SIZE_TEST
		if (!g_bg_colskip)
#endif
		gw32(ADDR_BB_SURF, g_bg_save[0]);
#ifdef BIG_SIZE_TEST
		if (g_bg_coldep) gw32(ADDR_MAIN_DEPTH, g_bg_save[1]);
		if (!g_bg_colonly && !g_bg_coldep && !g_bg_colskip)
#endif
		{
#ifdef BIG_SIZE_TEST
			if (!g_bg_depperm)
#endif
			gw32(ADDR_MAIN_DEPTH, g_bg_save[1]);
			gw32(ADDR_T3_SURF, g_bg_save[2]);
			gw32(ADDR_T3_TEX, g_bg_save[3]); gw32(ADDR_T24_TEX, g_bg_save[4]);
		}
		if (g_bg_acc_on && e)
		{
			gw32(e + 8, g_bg_save[7]); gw32(e + 0x14, g_bg_save[8]); gw32(e + 0x1C, g_bg_save[9]); gw32(e + 0x28, g_bg_save[10]); gw32(e + 0x80, g_bg_save[11]); gw32(e + 0x84, g_bg_save[12]);
			g_bg_acc_on = 0;
		}
		g_bg_on = 0;
	}
	if (keep_tex) { g_bg_tex_kept = 1; return; }                        /* the picture's texture stays ours for the copy */
#ifdef BIG_SIZE_TEST
	if (g_bg_colskip) { g_bg_tex_kept = 0; return; }
#endif
	gw32(ADDR_BB_TEX, g_bg_save[5]); gw32(ADDR_BB_TEX + 4, g_bg_save[6]); g_bg_tex_kept = 0;
}
/* ---- 6i.193: THE LEVEL'S SCREEN EFFECT ABOVE 100 % (your note: Cairo Station's fog / smoke layer zooms into the top
 * left, more with each step from 110 to 200 % - "like the world camera did before") ----
 * What that layer is: a level can name screen effects (the scenario's block at +0x3D0, each with a tag and two ramps),
 * and the game draws each over the finished picture in the scene's closing pass (function_13870, v1.0 0x13870; the
 * draw is function_2d140, called at 0x13b52: the tag in eax; on the stack the weights, the grid's columns and rows, and
 * the grid - the corners of the screen as PIXELS, with the texture's 0..1 beside them). The corners are taken from the
 * camera's rectangle (0x4b9dd0), and so is the size function_2d140 itself works with - and that rectangle is the
 * game's 640 x 480 screen. That pass runs BEFORE the mod stretches the larger picture back down, i.e. while the picture
 * is still the larger target: so the whole effect was drawn into the top-left 640 x 480 of it - a share that shrinks as
 * the scale grows - and then stretched back with everything else. (The first lead I followed, the size the game is told
 * for the depth's views, turned out to be right already: the mod has answered that since 6i.148.)
 * The fix, for that one draw: its call goes through se_hook (hooks.S). While the larger targets stand in, se_before
 * hands the draw a copy of the grid with its corners scaled from the camera's rectangle to the larger target, and puts
 * the larger size in the camera's rectangle for the length of the call; se_after puts the rectangle back. Nothing else
 * that reads the rectangle sees the change. At 100 % and below, and on a level with no screen effect, nothing happens.
 * Only that call (0x13b52) goes through it. The function's other call (0x22bbbb) is the HUD's own use of the same
 * drawing routine, after the stretch, and is the HUD Widescreen Fix's (hgrid_hook). ---- */
#define SE_GRID_MAX 16
static float g_se_grid[SE_GRID_MAX][4];
static u32 g_se_rect[2];
static s32 g_se_on, g_se_said;
u32 se_before(u32 grid, u32 columns, u32 rows)
{
	u32 n = columns * rows, d0, d1, i;
	float w, h, sx, sy;
	g_se_on = 0;
	if (!g_bg_on || !ptr_ok(grid) || !n || n > SE_GRID_MAX || columns > SE_GRID_MAX || rows > SE_GRID_MAX || g_bg_w < 640u || g_bg_h < 480u) return grid;
	d0 = M32(ADDR_CAM_RECT); d1 = M32(ADDR_CAM_RECT + 4);
	w = (float)(s32)((d1 >> 16) - (d0 >> 16)); h = (float)(s32)((d1 & 0xFFFFu) - (d0 & 0xFFFFu));     /* right - left, bottom - top */
	if (!(w >= 320.f && h >= 240.f) || (d0 & 0xFFFFu) || (d0 >> 16)) return grid;                   /* (a whole-screen rectangle from 0, 0: anything else is left) */
	sx = (float)g_bg_w / w; sy = (float)g_bg_h / h;
	for (i = 0; i < n; i++)
	{
		g_se_grid[i][0] = MF(grid + i * 16u) * sx; g_se_grid[i][1] = MF(grid + i * 16u + 4u) * sy;
		g_se_grid[i][2] = MF(grid + i * 16u + 8u); g_se_grid[i][3] = MF(grid + i * 16u + 12u);
	}
	g_se_rect[0] = d0; g_se_rect[1] = d1;
	gw32(ADDR_CAM_RECT, 0u); gw32(ADDR_CAM_RECT + 4, g_bg_h | (g_bg_w << 16));
	g_se_on = 1;
	if (!g_se_said) { s32 at = lput(0, "render scale: a level's screen effect drawn at the larger size ("); g_se_said = 1; at = lnum(at, g_bg_w); at = lput(at, " x "); at = lnum(at, g_bg_h); lput(at, ")"); log_write(); }
#ifdef HOST
	return 0xE9B00u;                                                         /* (the tests read the copy from their own memory) */
#else
	return (u32)g_se_grid;
#endif
}
void se_after(void)
{
	if (!g_se_on) return;
	g_se_on = 0;
	gw32(ADDR_CAM_RECT, g_se_rect[0]); gw32(ADDR_CAM_RECT + 4, g_se_rect[1]);
}
#ifdef HOST
void host_bg_state(s32 on, u32 w, u32 h) { g_bg_on = (u8)on; g_bg_w = w; g_bg_h = h; }
float host_se_grid(s32 i, s32 k) { return g_se_grid[i][k]; }
#endif
/* ---- 6i.198: CAIRO STATION'S SMOKE ABOVE 100 % - FOUND ----
 * Your 6i.197 round: no "fog:" line with the smoke on screen, and Fog test changed nothing - so the quad I had hooked
 * (vertex routine 0x26710) is not the one that draws your smoke. That sent me to the fog code's other draw, and there
 * it is in plain sight. The patchy fog is drawn by function_25dc0 (v1.0 0x25dc0): it builds the moving smoke pattern
 * from the level's picture (your "bridge_fog") into a 320 x 240 working target, then lays the fog over the scene. Both
 * steps turn a position on the screen, in pixels, into a place in that pattern by multiplying by ONE number, the float
 * at 0x4b9d90 - and the fog's own per-frame set-up (function_27aa0, 0x27b4a) writes it as the constant 0.5: half of
 * 640 x 480 is 320 x 240. With the picture larger, half of it runs past the pattern's 320 x 240: the building step
 * draws only the top-left of the pattern into the target, and the fog reads past its edge, which repeats the last row
 * and column - right in the top-left 640 / W of the screen, stretched beyond it, worse with every step. Exactly your
 * video.
 * The fix: the two calls of function_25dc0 (0x2c156, 0x2c1cb; its two flags in cl and dl, nothing on the stack) - which
 * already came through the mod for a timing mark (fx_b_hook) - go on through fog_hook (hooks.S). While the mod's larger
 * targets stand in, the number is 320 / (the larger picture's
 * width) for the length of that call - 0.25 at 200 % - and the game's 0.5 again after it. At 100 % and below the
 * larger targets are not in use and nothing is touched. (The picture keeps the screen's shape at every Render Scale
 * step, so the one number serves across and down, as the game's own does.)
 * The 6i.197 probe, its hook and "Fog test (temporary)" are gone. ---- */
#define ADDR_FOG_SCALE_HOST 0xE9F80u
#ifdef HOST
#define FOG_SCALE_AT ADDR_FOG_SCALE_HOST
#else
#define FOG_SCALE_AT ADDR_FOG_SCALE
#endif
static u32 g_fog_scale_was;
static s32 g_fog_scale_on, g_fog_said;
void fog_before(void)
{
	union { float f; u32 u; } v;
	g_fog_scale_on = 0;
	if (!FOG_SCALE_AT || !g_bg_on || g_bg_w <= 640u) return;
	g_fog_scale_was = M32(FOG_SCALE_AT);
	if (g_fog_scale_was != 0x3F000000u) return;                             /* (not the game's 0.5: something I do not know about - left) */
	v.f = 320.f / (float)g_bg_w;
	gw32(FOG_SCALE_AT, v.u);
	g_fog_scale_on = 1;
	if (!g_fog_said) { s32 at = lput(0, "render scale: the patchy fog's pattern scaled for the larger picture ("); g_fog_said = 1; at = lnum(at, g_bg_w); at = lput(at, " x "); at = lnum(at, g_bg_h); lput(at, ")"); log_write(); }
}
void fog_after(void)
{
	if (!g_fog_scale_on) return;
	g_fog_scale_on = 0;
	gw32(FOG_SCALE_AT, g_fog_scale_was);
}
/* the target size getter's answer (hooks.S rt_size_hook; eax the target): the screen-sized targets - the back buffer (0),
 * target 3, target 24 and the light accumulation target (1) - are the big size while the scene is drawn into it */
u32 rt_size(u32 index, u32 *w, u32 *h)
{
	if (!g_bg_on || (index != 0u && index != 3u && index != 24u && index != 1u)) return 0;
	if (index == 1u && !g_bg_acc_on) return 0;
	*w = g_bg_w; *h = g_bg_h;
	return 1;
}
#ifdef HOST
s32 host_bg_on(void) { return g_bg_on; }
s32 host_bg_made(void) { return g_bg_made; }
u32 host_bg_surf(s32 which) { return (u32)(unsigned long)g_bg_surf[which]; }
u32 host_bg_tex(s32 which) { return (u32)(unsigned long)g_bg_tex[which]; }
u32 host_bg_word(s32 which, s32 k) { return g_bg_surf[which][k]; }
u32 host_bg_texword(s32 which, s32 k) { return g_bg_tex[which][k]; }
#endif
static void rs_cut_inner(void)
{
	u32 d0, d1, top, left, bottom, right, cut;
	if (g_rsc[0] < 99) g_rsc[0]++;
	if (g_rs_done == g_rs_frame) return;          /* 6i.64: stretched already this frame - the rest at full size */
	if (g_rs_on)                                  /* 6i.64: another scene before the stretch - the same cut */
	{
		if (M32(ADDR_FRAME_STORAGE + 0x48) == g_rs_full[0] && M32(ADDR_FRAME_STORAGE + 0x4C) == g_rs_full[1]) gw32(ADDR_FRAME_STORAGE + 0x4C, g_rs_cutv);
		return;
	}
	if (g_rs_eff == 100 || g_rs_eff < 50 || g_rs_eff > 200 || !fps_unlocked() || !wire_in_game()) return;
	d0 = M32(ADDR_FRAME_STORAGE + 0x48); d1 = M32(ADDR_FRAME_STORAGE + 0x4C);
	top = d0 & 0xFFFFu; left = d0 >> 16; bottom = d1 & 0xFFFFu; right = d1 >> 16;
	if (top || left || right < 600u || bottom < 400u || right > 2000u || bottom > 2000u) return;   /* one full-screen view */
	if (g_rs_eff > 100)                                                     /* 6i.148: above 100 % - into the big targets, or not at all */
	{
		if (rs_t18_present())                                                /* 6i.199: a cutscene is blurring - the game's own size for now */
		{
			if (!g_bg_t18_said) { g_bg_t18_said = 1; lput(0, "render scale: a cutscene is blurring (its extra target exists) - the scene at 640 x 480 until it has gone"); log_write(); }
			return;
		}
		g_bg_t18_said = 0;
		if (right != 640u || bottom != 480u || !rs_big_make(g_rs_eff / 10)) return;
		{ u32 t_cut = tsc_lo(); rs_big_swap(1, 0); g_rs_cut_us = (tsc_lo() - t_cut) / 733u; }   /* 6i.149t */
		if (!g_bg_on) return;
#ifdef BIG_SIZE_TEST
		bg_scene_fill();
#endif
#if defined(BIG_SIZE_TEST) && !defined(HOST)
		if (g_bg_tvar == 23 && g_bg_acc_on) { gw32(ADDR_FRAME_STORAGE + 0x4C, g_bg_h | (g_bg_w << 16)); bg_quarter_draw(M32(ADDR_MAIN_TARGET)); }   /* 6i.149m: quarter draw before the world */
#endif
	}
	g_rs_full[0] = d0; g_rs_full[1] = d1;
	cut = g_rs_eff > 100 ? (g_bg_h | (g_bg_w << 16)) : ((bottom * (u32)g_rs_eff / 100u) | ((right * (u32)g_rs_eff / 100u) << 16));
#ifdef BIG_SIZE_TEST
	if (g_bg_colskip) cut = d1;                                             /* 6i.149p: the game draws at its own size */
#endif
	g_rs_cutv = cut;
	gw32(ADDR_FRAME_STORAGE + 0x4C, cut);
#ifndef HOST
	rs_view_consts(ADDR_FRAME_STORAGE + 0x18, ADDR_FRAME_STORAGE + 0x8C, M32(ADDR_MAIN_TARGET));   /* 6i.68: the mapping, cut */
#endif
#ifdef BIG_SIZE_TEST
	if (!g_bg_colskip)
#endif
	rs_half_sizes(1);                                                       /* 6i.73: the sun's glow and the like */
#ifndef HOST
	g_rs_cam[0] = M32(ADDR_CAM_RECT); g_rs_cam[1] = M32(ADDR_CAM_RECT + 4);
	if (g_rs_cam[0] == d0 && g_rs_cam[1] == d1) gw32(ADDR_CAM_RECT + 4, cut);
#endif
	g_rs_on = 1;
	if (g_rsc[1] < 99) g_rsc[1]++;
}
/* 6i.65: the drawing area itself. Your 6i.64 test (rs 1 1 1 1 0, still zoomed): cutting the frame storage's rectangle
 * didn't shrink the screen's own drawing - function_14bc0 gives the screen target a fixed 640 x 480 viewport. Every
 * viewport the game sets goes through Direct3D's SetViewport at three calls (function_14bc0's, and function_14ac0's
 * full-screen set and restore: v1.0 0x14b31 0x14b4f 0x14ebe -> 0x3f8100, v1.5 0x14b11 0x14b2f 0x14e9e -> 0x4021e0;
 * hooks.S vp_hook). While the cut is on, a full-screen one (at 0, 0, at least 600 x 400) is passed on cut to the share
 * - the projection is left as it was, so the same picture lands in the top-left, smaller. */
/* 6i.66: the picture itself made smaller. The world view's projection (function_2fd90, called twice by function_2ba10:
 * v1.0 0x2bad7 0x2bb1c, v1.5 0x2bab7 0x2bafc - esi the view, ebx the result, edx an optional clip box: hooks.S
 * proj_hook) is given a clip box wider and taller than the view - x -1 .. -1 + 2/s, y 1 - 2/s .. 1 (box2f: x0, x1, y0,
 * y1) - so the whole view lands in the top-left share s of the screen (the off-center projection function_2fb70
 * already supports). With the drawing area cut to that share (6i.63 / 6i.65), nothing outside it is drawn, and the
 * stretch brings it back. Your 6i.65 test: without this the cut only cropped - the stretch magnified the corner. */
static float g_rs_clip[4];
/* 6i.67: your 6i.66 test - no change. The world view's calls pass a clip box of their own (function_2b790 has
 * function_2f970 work one out for the view and hands it down through function_2ba10), and 6i.66 left any passed box
 * alone. Now the game's box is taken as the base - the share's window is cut from it: x0, x0 + (x1 - x0) / s,
 * y1 - (y1 - y0) / s, y1 (no box: -1 .. 1 both ways). */
u32 proj_clip(u32 view, u32 clip)
{
	u32 d0, d1, top, left, bottom, right, k;
	float sc, b[4];
	union { u32 u; float f; } cv;
	if (g_rs_eff >= 100 || g_rs_eff < 50 || !fps_unlocked() || !wire_in_game() || g_rs_done == g_rs_frame || !ptr_ok(view)) return clip;
	if (clip && !ptr_ok(clip)) return clip;
	d0 = M32(view + 0x30); d1 = M32(view + 0x34);
	top = d0 & 0xFFFFu; left = d0 >> 16; bottom = d1 & 0xFFFFu; right = d1 >> 16;
	if (top || left || right < 600u || bottom < 400u || right > 2000u || bottom > 2000u) return clip;   /* one full-screen view */
	b[0] = -1.f; b[1] = 1.f; b[2] = -1.f; b[3] = 1.f;
	if (clip) for (k = 0; k < 4; k++) { cv.u = M32(clip + 4 * k); b[k] = cv.f; }
	if (!(b[1] > b[0]) || !(b[3] > b[2]) || b[1] - b[0] > 8.f || b[3] - b[2] > 8.f) return clip;   /* not a box to cut */
	sc = (float)g_rs_eff * 0.01f;
	g_rs_clip[0] = b[0]; g_rs_clip[1] = b[0] + (b[1] - b[0]) / sc;
	g_rs_clip[2] = b[3] - (b[3] - b[2]) / sc; g_rs_clip[3] = b[3];
	return (u32)(unsigned long)g_rs_clip;
}
#ifdef HOST
float host_rs_clip(s32 k) { return g_rs_clip[k]; }
#endif
/* 6i.83: SCREEN READS (Performance Mode Settings) - game's / fast, fast by default. Your 6i.82 captures: the costliest
 * draws of the lighting pass (pass 3: 0x80270D38, 2.3 ms of the Xbox's 5.6 at Beaver Creek) and of the lights at Regret
 * (pass 12: 0x804CF2A6 / 0x804CF63C, 5 - 6 ms each) all read the colour pass back from a full-screen LINEAR texture
 * (LIN_A8R8G8B8 640 x 480 - a render target, unswizzled), one texel per pixel; their other textures are small and
 * compressed or palettized. Read with bilinear filtering, a linear texture costs two rows of memory per pixel; read 1:1
 * it gives the same colour point-sampled, at one. fast: for a draw with a full-screen linear texture at a stage filtered
 * linear or anisotropic, that stage's filters are point for that draw (Direct3D's own state table: v1.0 0x404dd0, v1.5
 * 0x40eeb0, 0x80 a stage, MAGFILTER +0xc / MINFILTER +0x10; its dirty flags v1.0 0x407490 / v1.5 0x411570, bit = the
 * stage - as the game's own function_351a0 sets them) and put back right after, around every one of the game's draw
 * calls (hooks.S drawidx_hook / drawv_hook). */
volatile u8 spooky_scr_point, spooky_scr_set;
/* 6i.87: LIGHT FILTERING, take three - at the draw itself. 6i.85 (at the light passes' start) and 6i.86 (at the
 * material setups from the first shadow to the sky) both measured nothing: the light and shadow passes don't set up
 * their texture filtering through the game's material setup at all - their draws run with whatever filtering
 * Direct3D's stage states still hold from the last material set up before them (the world's: anisotropic). That's why
 * every texture's anisotropic off (6i.82 / 6i.84: set at every material setup, the world's included) did cut them -
 * 19.3 -> 10.4 ms and 5.1 -> 1.1 in 6i.82 - and nothing aimed at the lights' own setup could. So now: before each draw
 * in pass 11 or 12, any stage set anisotropic (min / mag filter 3, or max anisotropy over 1) is made linear with no
 * anisotropy for that draw, and put back right after - the world's draws untouched. The capture logs how many draws and
 * which stages were changed, and the first light draw's four stages as Direct3D holds them. */
/* (6i.87: its counters are declared with the settings - the capture uses them first) */
static u32 g_scr_old[4][6];                     /* 6i.90: mag, min, max anisotropy, mip filter; LOD bias; 6i.107: max mip level */
static u8 g_scr_mask;
static u32 g_scr_n;                              /* draws changed - for the capture's numbers */
#ifndef HOST
void d3d_rs_simple(u32 method, u32 value);       /* hooks.S: Direct3D's simple render-state setter */
#endif
void scr_pre(void)
{
	if (g_cap_live && g_cap_rec) g_rec_ix += spooky_draw_n;                     /* 6i.98 */
	if ((g_cap_live || o_light_skip) && spooky_draw_ip)                         /* 6i.110 / 6i.111: the camera's ranges, the light's draws */
	{
		if (g_cr_frame != g_frames) { g_cr_n = 0; g_cr_frame = g_frames; }
		if (g_pass_now && !g_in_light) cam_range_add(spooky_draw_ip, spooky_draw_n);   /* 6i.112: every pass outside a light */
		else if (g_in_light && (g_pass_now == 12u || g_pass_now == 11u))      /* 6i.112: the light's shadow pass too */
		{
			if (cam_seen(spooky_draw_ip, spooky_draw_n)) g_rec_se++;
			else { g_rec_un++; if (o_light_skip) spooky_skip_draw = 1; }
		}
	}
	u32 st, base, mag, mn;
	g_scr_mask = 0;
	u32 lp = (u32)(o_lightf && (g_pass_now == 11u || g_pass_now == 12u));     /* 6i.87: a light's draw */
	u32 wm = (u32)(!o_world_mip && g_pass_now && g_pass_now != 11u && g_pass_now != 12u && fps_unlocked());   /* 6i.92: World Mip Blending off */
	if (lp || (g_cap_nomip && g_pass_now) || wm)                             /* 6i.91: or the capture's no mip blending */
	{
		for (st = 0; st < 4u; st++)
		{
			u32 an, mp, fa, ani;
			base = ADDR_TEX_STATE + st * 0x80u;
			mag = M32(base + 0xCu); mn = M32(base + 0x10u); an = M32(base + 0x20u); mp = M32(base + 0x14u);
			fa = (u32)(((lp && o_lightf >= 2) || g_cap_nomip || wm) && mp == 2u);     /* 6i.90 faster: mip levels blended */
			ani = lp && (mag == 3u || mn == 3u || an > 1u);
			if (!ani && !fa) continue;
			g_scr_old[st][0] = mag; g_scr_old[st][1] = mn; g_scr_old[st][2] = an; g_scr_old[st][3] = mp; g_scr_old[st][4] = M32(base + 0x18u); g_scr_old[st][5] = M32(base + 0x1Cu);
			if (ani)
			{
				if (mag == 3u) gw32(base + 0xCu, 2u);
				if (mn == 3u) gw32(base + 0x10u, 2u);
				gw32(base + 0x20u, 0u);                                        /* as Anisotropic Filtering off sets it */
			}
			if (fa) gw32(base + 0x14u, 1u);                                    /* 6i.90: the nearest level, no blend */
			g_scr_mask |= (u8)(1u << st);
		}
	}
	/* 6i.155: the Screen Reads (6i.83) and Bump Filtering (6i.84) stage changes that followed here are gone - retired in 6i.84/85, kept as a skill */
	if (g_scr_mask)
	{
		gw32(ADDR_TEX_DIRTY, M32(ADDR_TEX_DIRTY) | g_scr_mask);
		if (g_cap_live) g_scr_n++;
	}
	spooky_scr_set = (u8)(g_scr_mask != 0);
}
void scr_post(void)
{
	u32 st, base;
	for (st = 0; st < 4u; st++) if (g_scr_mask & (1u << st))
	{
		base = ADDR_TEX_STATE + st * 0x80u;
		gw32(base + 0xCu, g_scr_old[st][0]); gw32(base + 0x10u, g_scr_old[st][1]); gw32(base + 0x20u, g_scr_old[st][2]); gw32(base + 0x14u, g_scr_old[st][3]); gw32(base + 0x18u, g_scr_old[st][4]); gw32(base + 0x1Cu, g_scr_old[st][5]);
	}
	gw32(ADDR_TEX_DIRTY, M32(ADDR_TEX_DIRTY) | g_scr_mask);
	g_scr_mask = 0; spooky_scr_set = 0;
}
#ifdef HOST
u32 host_scr_n(void) { return g_scr_n; }
#endif
static u32 g_vp_cut[6];
u32 vp_filter(u32 vp)
{
	u32 k, w, h;
	if (!g_rs_on || !ptr_ok(vp)) return vp;
	w = M32(vp + 8); h = M32(vp + 12);
	if (M32(vp) || M32(vp + 4) || w < 600u || h < 400u || w > 2000u || h > 2000u) return vp;
	for (k = 0; k < 6; k++) g_vp_cut[k] = M32(vp + 4 * k);
#ifdef BIG_SIZE_TEST
	if (g_bg_colskip) return vp;                                            /* 6i.149p: the game draws at its own size */
#endif
	if (g_bg_on) { g_vp_cut[2] = g_bg_w; g_vp_cut[3] = g_bg_h; }          /* 6i.148: the big targets' whole size */
	else { g_vp_cut[2] = w * (u32)g_rs_eff / 100u; g_vp_cut[3] = h * (u32)g_rs_eff / 100u; }
	return (u32)(unsigned long)g_vp_cut;
}
#ifdef HOST
u32 host_vp_cut(s32 k) { return g_vp_cut[k]; }
#endif
/* 6i.69: the stretch moved later - just before the scene's second interface step (function_40870, v1.0 0x2c2cb, v1.5
 * 0x2c2ab: hooks.S rs_ui_hook). Your 6i.68 photos: the world right, but the teleporter, the sign's glow and the rocket
 * gone or under things below 100 %. The main scene draws its see-through work - function_25dc0, function_3fdf0 /
 * function_404b0 (effects, transparent objects) - after the first interface step (mark 49, where the stretch was), so
 * those were drawn full-size over the stretched picture, tested against a depth buffer still laid out small. Now they're
 * drawn small with everything else, then stretched. */
void rs_stretch(void);
/* 6i.71: STRETCH POINT (test). 6i.69 (before the scene's second interface step) left the first-person model's layers
 * through the weapon; 6i.70 (the HUD's view) zoomed the HUD again - so the HUD is drawn between the two, and so are some
 * of the first-person layers. Five points to try, in frame order: 1 before the second interface step (function_40870 -
 * 6i.69), 2 after it (the scene's call of function_20770: v1.0 0x2c317, v1.5 0x2c2f7 - hooks.S rs_after_ui_hook), 3
 * before function_13870 (mark 56), 4 before function_13630 (mark 57), 5 the HUD's view (6i.70; also the fallback). */
void rs_ui_point(void)
{
	if (g_rsc[2] < 99) g_rsc[2]++;
	if (o_rs_point == 1) rs_stretch();
}
void rs_after_ui_point(void) { if (o_rs_point == 2) rs_stretch(); }
/* 6i.72 / 6i.73: PARTICLES (Performance Mode Settings) - game's / fast / faster. Your 6i.72 test: thinning picked whole
 * effects (what it went by is shared by every particle of one), and you'd rather they were drawn cheaper than fewer.
 * Smoke's cost is blending: every pixel of every sprite reads the screen and writes it back, and much of each sprite is
 * close to fully see-through. Now each particle sprite (function_224550's call of the emitter - v1.0 0x224cd2, v1.5
 * 0x22b6c2; hooks.S sprite_hook, which calls the emitter itself, the game's own arguments copied) is drawn with the
 * GPU's alpha test on - pixels at or below a small alpha (fast 8 / 255, faster 32 / 255) are dropped before the blend -
 * and the render state put back right after. Set the way the game sets its own (function_142f0): Direct3D's simple
 * setter (v1.0 0x3f5cf0, v1.5 0x3ffdd0: ecx the method, edx the value) and its shadow copies (ALPHAFUNC / ALPHATEST /
 * ALPHAREF: v1.0 0x407580 / 0x407588 / 0x40758c, v1.5 0x411660 / 0x411668 / 0x41166c). Each sprite is its own
 * begin / end in the push buffer, so the state changes sit between sprites. Left alone when the game has alpha test on. */
void d3d_rs_simple(u32 method, u32 value);       /* hooks.S: Direct3D's simple render-state setter */
/* 6i.74: your 6i.73 test - no change. Most likely the particles already draw with the game's own alpha test on (cutting
 * only fully clear pixels), and 6i.73 left any sprite with it on alone. Now: with it on and greater / greater-or-equal,
 * its reference is raised to ours when lower (put back after); with it off, as 6i.73 (on, greater than ours, off after). */
static u8 g_sp_set;
static u32 g_sp_old[3];
void sprite_before(void)
{
	u32 ref, on, fn, old;
	g_sp_set = 0;
	if (!o_particles || !fps_unlocked()) return;
	ref = o_particles == 2 ? 32u : 8u;
	on = M32(ADDR_RS_ALPHATEST); fn = M32(ADDR_RS_ALPHAFUNC); old = M32(ADDR_RS_ALPHAREF);
	if (on && fn != 0x204u && fn != 0x206u) return;  /* the game's own test, of another kind: left alone */
	if (on && old >= ref) return;                     /* already as strict */
	g_sp_old[0] = fn; g_sp_old[1] = old; g_sp_old[2] = on;
#ifndef HOST
	if (!on) d3d_rs_simple(0x4033Cu, 0x204u);    /* ALPHA_FUNC: greater */
	d3d_rs_simple(0x40340u, ref);                /* ALPHA_REF */
	if (!on) d3d_rs_simple(0x40300u, 1u);        /* ALPHA_TEST_ENABLE */
#endif
	if (!on) gw32(ADDR_RS_ALPHAFUNC, 0x204u);
	gw32(ADDR_RS_ALPHAREF, ref); gw32(ADDR_RS_ALPHATEST, 1u);
	g_sp_set = 1;
}
void sprite_after(void)
{
	if (!g_sp_set) return;
	g_sp_set = 0;
#ifndef HOST
	if (!g_sp_old[2]) d3d_rs_simple(0x40300u, 0u);
	if (!g_sp_old[2]) d3d_rs_simple(0x4033Cu, g_sp_old[0]);
	d3d_rs_simple(0x40340u, g_sp_old[1]);
#endif
	gw32(ADDR_RS_ALPHATEST, g_sp_old[2]); gw32(ADDR_RS_ALPHAFUNC, g_sp_old[0]); gw32(ADDR_RS_ALPHAREF, g_sp_old[1]);
}
/* 6i.70: your 6i.69 test - the see-through world right, but the first-person model's own layers (the overshield on the
 * arms, the SPNKR lettering from the launcher's far side) showing through the weapon. The scene draws the first-person
 * model across this point: some of it before, some after (function_40870 and on), so the two halves were drawn at
 * different sizes against one depth buffer. Now nothing in the world view is stretched part-way: the whole scene is
 * drawn small, and the stretch is as the HUD's view begins (spooky_view - 6i.61's place, where it ran as the fallback).
 * Anything the scene itself draws over the world (if any HUD pieces are among it) is stretched with it - softer, in place. */
void rs_stretch(void)                            /* 6i.63: before the scene's interface (mark 49) - the HUD then at full size;
                                                  * spooky_view's call is the fallback */
{
	if (!g_rs_on) return;
	g_rs_on = 0;
	g_rs_done = g_rs_frame;                       /* 6i.64: once a frame */
	if (g_rsc[3] < 99) g_rsc[3]++;
#ifdef BIG_SIZE_TEST
	if (g_bg_copyonly)                            /* 6i.149e: the filled buffer over the screen, the game's texture pointers turned for the copy only */
	{
		u32 main = M32(ADDR_MAIN_TARGET), big = g_bg_h | (g_bg_w << 16), tx0 = M32(ADDR_BB_TEX), tx1 = M32(ADDR_BB_TEX + 4);
		s32 k;
		g_bg_copyonly = 0;
		if (!ptr_ok(tx0)) return;
		for (k = 0; k < 5; k++) g_bg_tex[0][k] = M32(tx0 + 4u * k);
		g_bg_tex[0][1] = rs_big_form(g_bg_tex[0][1], g_bg_col, g_bg_colp); g_bg_tex[0][4] = rs_big_sizeword(g_bg_w, g_bg_h);
		gw32(ADDR_BB_TEX, (u32)(unsigned long)g_bg_tex[0]); gw32(ADDR_BB_TEX + 4, (u32)(unsigned long)g_bg_tex[0]);
		gw32(ADDR_CAM_RECT, 0u); gw32(ADDR_CAM_RECT + 4, big);
		rs_copy(main, 0);
		gw32(ADDR_CAM_RECT, g_rs_full[0]); gw32(ADDR_CAM_RECT + 4, g_rs_full[1]);
		gw32(ADDR_BB_TEX, tx0); gw32(ADDR_BB_TEX + 4, tx1);
		rs_target(main); g_bg_rb_n++;
		return;
	}
#endif
#ifdef BIG_SIZE_TEST
	if (g_bg_on && g_bg_colskip)                  /* 6i.149p: the picture is the game's own; just the bookkeeping */
	{
		gw32(ADDR_FRAME_STORAGE + 0x48, g_rs_full[0]); gw32(ADDR_FRAME_STORAGE + 0x4C, g_rs_full[1]);
		rs_big_swap(0, 0);
		rs_half_sizes(0);
#ifndef HOST
		rs_view_consts(ADDR_FRAME_STORAGE + 0x18, ADDR_FRAME_STORAGE + 0x8C, M32(ADDR_MAIN_TARGET));
#endif
		return;
	}
#endif
	if (g_bg_on)                                  /* 6i.148: above 100 % - the big picture drawn over the back buffer, filtered down */
	{
		u32 main = M32(ADDR_MAIN_TARGET), big = g_bg_h | (g_bg_w << 16);
		u32 t_stretch = tsc_lo();                  /* 6i.149t: how long the x86 side spends here (the copies may wait for the GPU) */
		gw32(ADDR_FRAME_STORAGE + 0x48, g_rs_full[0]); gw32(ADDR_FRAME_STORAGE + 0x4C, g_rs_full[1]);   /* the targets' viewports whole */
#if defined(BIG_SIZE_TEST) && !defined(HOST)
		if (g_bg_tvar == 22) bg_quarter_draw(main);   /* 6i.149m/n: the light texture into the bottom-right quarter only */
#endif
		rs_big_swap(0, 1);                        /* the game's targets back - but the picture's texture is still the big one */
#ifndef HOST
		/* 6i.149u: the 6i.149 test's CPU readback of the big picture and the back buffer is gone from here - on the 360 a CPU
		 * read of memory the GPU thread writes (the resolved picture) can make the emulator hold the x86 thread until the GPU
		 * thread has caught up, and 6i.149t measured the stretch at 9-11 ms a frame */
#ifdef BIG_SIZE_TEST
		if (o_rscale == 14) bg_touch();
#endif
		rs_target(main);                          /* the back buffer bound, its whole viewport */
#ifdef BIG_SIZE_TEST
		if (o_rscale == 11 || o_rscale == 13 || o_rscale == 15) bg_touch();
#endif
		gw32(ADDR_CAM_RECT, 0u); gw32(ADDR_CAM_RECT + 4, big);   /* the copy's source: the whole big picture */
		{ u32 t_copy = tsc_lo(); rs_copy(main, 0); g_rs_copy_us = (tsc_lo() - t_copy) / 733u; }   /* the main target's texture (the big picture) over the whole back buffer; 6i.149u: timed alone */
		gw32(ADDR_CAM_RECT, g_rs_full[0]); gw32(ADDR_CAM_RECT + 4, g_rs_full[1]);
		g_bg_rb_n++;
#endif
		rs_big_swap(0, 0);                        /* the texture pointers back too */
#ifndef HOST
		rs_target(main);
#endif
		rs_half_sizes(0);
#ifndef HOST
		rs_view_consts(ADDR_FRAME_STORAGE + 0x18, ADDR_FRAME_STORAGE + 0x8C, M32(ADDR_MAIN_TARGET));
#endif
		g_rs_stretch_us = (tsc_lo() - t_stretch) / 733u;
		return;
	}
#ifndef HOST
	{
		u32 main = M32(ADDR_MAIN_TARGET);
		gw32(ADDR_FRAME_STORAGE + 0x48, g_rs_full[0]); gw32(ADDR_FRAME_STORAGE + 0x4C, g_rs_full[1]);   /* the targets' viewports whole */
		rs_copy(3, 0);                           /* the camera storage's rectangle (still cut) across target 3 (6i.62: was 19 - half
		                                          * the screen's size: the picture came back at half size, its edges smeared
		                                          * across the rest. 3 is screen-sized - 640 x 480 - with its own texture,
		                                          * g_509368, that function_14f60 binds; its content is done with by now) */
		rs_target(main);
		gw32(ADDR_CAM_RECT, g_rs_full[0]); gw32(ADDR_CAM_RECT + 4, g_rs_full[1]);   /* (6i.63: the frame storage's full - its own was the same) */
		gw32(ADDR_MAIN_TARGET, 3);               /* the copy reads the "main" target: 3 now */
		rs_copy(main, 0);                        /* target 3 over the whole screen */
		gw32(ADDR_MAIN_TARGET, main);
		rs_target(main);
	}
#endif
	gw32(ADDR_FRAME_STORAGE + 0x48, g_rs_full[0]); gw32(ADDR_FRAME_STORAGE + 0x4C, g_rs_full[1]);   /* as they were */
	rs_half_sizes(0);                                                       /* 6i.73 */
#ifndef HOST
	rs_view_consts(ADDR_FRAME_STORAGE + 0x18, ADDR_FRAME_STORAGE + 0x8C, M32(ADDR_MAIN_TARGET));   /* 6i.68: the mapping full again */
#endif
}
#ifdef HOST
void host_set_shadows(s32 v, s32 p12) { o_shadow_on = v != 2; o_pass12 = p12; }
void host_set_water_rate(s32 v) { o_water_rate = v; }
void host_set_particles(s32 v) { o_particles = v; }
void host_set_rs_point(s32 v) { o_rs_point = v; }
void host_set_rscale(s32 v) { o_rscale = v; g_rs_eff = (v >= 5 && v < 10) || (v > 10 && v <= 20) ? v * 10 : 100; }   /* 6i.148: above 100 % too */
s32 host_rs_on(void) { return g_rs_on; }
#endif
void spooky_view(u32 view)
{
	s32 i;
	spooky_light_loop = 0;                                                  /* 6i.86 */
	if (g_rs_on && g_rsc[4] < 99) g_rsc[4]++;                              /* 6i.64: the fallback counted */
	rs_stretch();                                                           /* 6i.61 */
	g_wire_arm = 0;                                                         /* 6i.50: the HUD's pass begins - its frame solid (Wireframe) */
	spooky_wire_on = 0;                                                     /* 6i.55: and no lines on it */
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
/* (g_ws, g_ws_flag5: declared above water_fit, 6i.147) */
#ifdef HOST
static u32 g_host_ws, g_host_ws_flag5;
void host_set_widescreen(u32 ws, u32 flag5) { g_host_ws = ws; g_host_ws_flag5 = flag5; }
#endif

/* the projection's aspect, exactly as the game's function_2fb70 decides it:
 *   widescreen = function_016a90()  (the widescreen setting g_485ac2, except on one options screen)
 *   widescreen: aspect x 4/3 (16:9 drawn into the 4:3 buffer); if g_510c50[5] also: tangent x 0.5625 x 4/3
 * The flag is read only through a validated pointer; anything implausible leaves the marker undrawn. */
#ifdef HOST
static void aspect_query(void);
void host_aspect_query(void) { aspect_query(); }
#endif
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

/* 6i.174: the tangent as plain arithmetic, for every use (it was the x87's fptan). Your 6i.173 test on the 360: the
 * Field of View worked once its own tangent was a series, but the first-person body's fix did not - its factor is a
 * ratio of two tangents and still came from fptan, which on the 360's emulator gives a value the fix then refuses (as
 * the Field of View's did). So no tangent in the mod asks the x87 any more: reduced to 0..pi/4 (tan(x) = 1 / tan(pi/2
 * - x) above it; odd; period pi), then sine and cosine series to x^11 - within 0.00001 of the library's across
 * -1.56..1.56 (checked in the host tests). */
static float ftan_small(float x)                 /* |x| <= pi/4 */
{
	float x2 = x * x;
	float sn = x * (1.f - x2 / 6.f * (1.f - x2 / 20.f * (1.f - x2 / 42.f * (1.f - x2 / 72.f * (1.f - x2 / 110.f)))));
	float cs = 1.f - x2 / 2.f * (1.f - x2 / 12.f * (1.f - x2 / 30.f * (1.f - x2 / 56.f * (1.f - x2 / 90.f))));
	return sn / cs;
}
static float ftan(float x)
{
	float t;
	s32 neg = 0, guard = 0;
	/* 6i.175: your 6i.174 test on the 360 - Field of View "out of range 0 967 0 0" at every angle: the factor and the
	 * angle worked out were 0, i.e. this function returned 0. 6i.174 began it with "if (x != x) return 0" (not a
	 * number). The compiler makes that "compare x with itself, jump if the PARITY flag is set" (fucomi / jp) - and on
	 * the 360's emulator that jump was taken for ordinary numbers. Not confirmed in the emulator's code; but it is the
	 * one thing 6i.174 added to a path 6i.173 ran, and it produces exactly a 0. A test for "equal" or "not equal"
	 * between floats always compiles to a parity jump; "greater" and "less" never do. So: no float == or != in this
	 * path at all - a value that is not a number fails the two ordered tests below instead. */
	if (!(x > -1000.f && x < 1000.f)) return 0.f;
	while (x > 1.5707963f && guard++ < 64) x -= 3.1415927f;
	while (x < -1.5707963f && guard++ < 64) x += 3.1415927f;
	if (x < 0.f) { x = -x; neg = 1; }
	if (x <= 0.7853982f) t = ftan_small(x);
	else { t = ftan_small(1.5707963f - x); t = t > 1.0e-6f ? 1.f / t : 1.0e6f; }
	return neg ? -t : t;
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
		float angle = (g_fov_over ? g_fov_used : MF(ADDR_PLAYER_CAMERAS + 0x4C)) * 0.785f;   /* 6i.120: the view's own (ours when changed) */       /* function_2f800: source angle x 0.785 */
		if (angle > 0.002f && angle < 3.0f) fov = angle;                     /* 6i.126: down to 20x zoom - at 0.1 (just past 8x) the overlays fell back to another projection */
		g_cam_fov = fov;
	}
	if (g_vp_w <= 0 || g_vp_h <= 0 || !(fov > 0.002f && fov < 3.0f)) return 0;
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
		if (!(g_tan_v > 0.0005f && g_tan_v < 5.f && g_tan_h > 0.0005f && g_tan_h < 9.f)) return 0;
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
#define PS_SLOTS 128                              /* 6i.150: 128 (was 48, and a 49th colour re-used an address with a new colour: the 360's cache bug by another road) */
static u8 g_psdef[PS_SLOTS][0xF0] __attribute__((aligned(16)));
static u32 g_ps_col[PS_SLOTS];
static s32 g_ps_n;
static u32 ps_dist(u32 a, u32 b)                  /* colour distance: the sum over the four channels of the squared difference */
{
	u32 d = 0, k;
	for (k = 0; k < 32u; k += 8u) { s32 x = (s32)((a >> k) & 0xFFu) - (s32)((b >> k) & 0xFFu); d += (u32)(x * x); }
	return d;
}
static u8 *ps_for(u32 packed)
{
	s32 i, slot;
	u8 *ps;
	for (i = 0; i < g_ps_n; i++) if (g_ps_col[i] == packed) return g_psdef[i];
	if (g_ps_n >= PS_SLOTS)
	{	/* 6i.150: full - the nearest colour already defined is used; an address never changes colour (the 360 keeps the
		 * first colour it saw at an address). gfx_begin quantises to 16 steps a channel, so the table fills slowly */
		u32 best = 0xFFFFFFFFu; slot = 0;
		for (i = 0; i < PS_SLOTS; i++) { u32 d = ps_dist(g_ps_col[i], packed); if (d < best) { best = d; slot = i; } }
		return g_psdef[slot];
	}
	slot = g_ps_n++;
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
#ifdef HOST
u32 g_host_gfx_packed;                            /* 6i.150: the last colour gfx_begin packed (the host test checks the 16-step quantisation) */
#endif
static void gfx_begin(float a, float r, float g, float b)
{
	u32 packed = ((u32)fti(a * 255.f) << 24) | ((u32)fti(r * a * 255.f) << 16) | ((u32)fti(g * a * 255.f) << 8) | (u32)fti(b * a * 255.f);   /* 6i.213 */
	packed = (packed & 0xF0F0F0F0u) | ((packed >> 4) & 0x0F0F0F0Fu);   /* 6i.150: 16 steps a channel (each nibble doubled): the gradients (OOB countdown, trigger flash) make a few colours, not hundreds */
#ifdef HOST
	g_gfx_open = (s32)packed; g_gfx_quads = 0; g_host_gfx_packed = packed;
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
#ifdef HOST
u32 host_gfx_colour(float a, float r, float g, float b) { gfx_begin(a, r, g, b); gfx_end(); return g_host_gfx_packed; }   /* 6i.150: the host test's view of the packed, quantised colour */
#endif

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
	gfx_quad(fti(x0 + nx), fti(y0 + ny), fti(x1 + nx), fti(y1 + ny), fti(x1 - nx), fti(y1 - ny), fti(x0 - nx), fti(y0 - ny));   /* 6i.213 */
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
void host_set_exact_circle(s32 on) { (void)on; }       /* 6i.212: there is no other verdict to switch to */
s32 host_exact_circle(void) { return 1; }
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
/* 6h.34 (HCM 2026-11-01, sb_ledge_seam): ledge edges are seams too - a floor meeting a face that goes DOWN from the edge (a post
 * top, a ledge, a step down), bend 0 (flat). A floor meeting a face that goes up from it (an inside corner) is still left
 * out. 1: a floor seam (both faces floors - normals 0.5+ up - under 30 deg apart), 2: a ledge seam, 0: neither. na / nb: the
 * faces' own normals; la / lb the edge's ends in the BSP's own space. */
static s32 seam_type(const s_cb *c, const s_xf *x, u32 sl, u32 sr, const float *la, const float *lb, float *na, float *nb)
{
	float wa[3], wb[3];
	if (!cb_normal(c, sl, na) || !cb_normal(c, sr, nb)) return 0;
	xf_dir(x, na, wa); xf_dir(x, nb, wb);
	if (wa[2] >= 0.5f && wb[2] >= 0.5f) return dot3(na, nb) >= 0.866f ? 1 : 0;
	if (wa[2] >= 0.5f || wb[2] >= 0.5f)
	{
		u32 wall = wa[2] >= 0.5f ? sr : sl;
		float q[3], qw[3], aw[3], bw[3], low;
		if (!cb_point_off(c, wall, la, lb, q)) return 0;
		xf_point(x, q, qw); xf_point(x, la, aw); xf_point(x, lb, bw);
		low = aw[2] < bw[2] ? aw[2] : bw[2];
		if (qw[2] < low - 0.05f) return 2;                                     /* the other face goes down from the edge */
	}
	return 0;
}
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
		{
			s32 ty = seam_type(c, x, sl, sr, la, lb, na, nb);                 /* the floor test in the world */
			if (!ty) { if (!piece) g_lsd[4]++; return; }
			if (ty == 2) { seam_add(a, b, SK_FLAT, d2, piece); return; }      /* 6h.34: a ledge seam - flat */
		}
		cs = dot3(na, nb);
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
/* ---- 6i.184: the Superbounce Viewer's search, measured (logging only - nothing about the search is changed) ----
 * Your note: heavy in campaign; at the start of The Great Journey, where the faces and seams are very large, a jump
 * alone nearly halves the frame rate. Read from the code, the likely cause is the level's seam grid giving up - it
 * does so, silently until now, for a level with more than 65535 edges, more than LG_MAX floor-seam edges, more than
 * LG_LONG edges over 32 units long, or a cell that overflows - after which every search uses the "near list", which
 * is rebuilt from EVERY edge of the level each time you leave or touch the ground and holds a much wider area in the
 * air. That is a reading, not a measurement. So the log now says:
 *  - why the grid is off, when it is (one line, at the level's first search);
 *  - every ten seconds, with the viewer on: the searches on the ground and in the air apart - how many, their average
 *    and longest time, the level's edges handed over, the seams kept, the pieces and objects near - and the near
 *    list's rebuilds: how many, how many of them for an air / ground change, their average time, the list's size. */
static s32 g_lg_why;                              /* why the grid is off: 0 it is on; 1 more than 65535 edges; 2 too many floor-seam edges; 3 too many long ones; 4 a cell overflowed; 5 its check failed */
static u32 g_lg_why_n;                            /* the count that was too many */
static u32 g_sm_n[2], g_sm_cyc[2], g_sm_max[2], g_sm_edges[2], g_sm_kept[2], g_sm_pieces[2], g_sm_objects[2];   /* [0] on the ground, [1] in the air: since the last line */
static u32 g_nlr_n, g_nlr_flip, g_nlr_cyc, g_nlr_size, g_nlr_full, g_sm_t0;
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
	s_xf id;
	if (sl >= c->ns || sr >= c->ns || sl == sr || va >= c->nv || vb >= c->nv) return 0;
	cb_vert(c, va, a); cb_vert(c, vb, b);
	id.on = 0;
	if (!seam_type(c, &id, sl, sr, a, b, na, nb)) return 0;                 /* floor seams and (6h.34) ledge seams */
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
	g_lg_why = 1; g_lg_why_n = c->ne;                                        /* (6i.184: why it is off, until it is on) */
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
	g_lg_nx = fti((hi[0] - lo[0]) / g_lg_cs) + 1; if (g_lg_nx > LG_CELLS) g_lg_nx = LG_CELLS;
	g_lg_ny = fti((hi[1] - lo[1]) / g_lg_cs) + 1; if (g_lg_ny > LG_CELLS) g_lg_ny = LG_CELLS;
	for (v = 0; v <= (u32)(g_lg_nx * g_lg_ny); v++) g_lg_start[v] = 0;
	for (v = 0; v < (u32)(g_lg_nx * g_lg_ny); v++) g_lg_chalf[v] = 0;
	for (pass = 0; pass < 2; pass++)
	{
		if (pass == 1)
		{
			s32 sum = 0, k;
			for (k = 0; k < g_lg_nx * g_lg_ny; k++) { s32 n = (s32)g_lg_start[k]; g_lg_start[k] = (u16)sum; sum += n; }
			g_lg_start[g_lg_nx * g_lg_ny] = (u16)sum; total = sum;
			if (total > LG_MAX) { g_lg_why = 2; g_lg_why_n = (u32)total; return; }   /* too big: every edge, as before */
		}
		for (e = 0; e < c->ne; e++)
		{
			s32 cx, cy, k;
			if (!lg_floor_edge(c, e, mid, &half)) continue;
			if (half > 16.f)
			{
				if (pass == 0) { if (g_lg_nlong >= LG_LONG) { g_lg_why = 3; g_lg_why_n = e; return; } g_lg_long[g_lg_nlong++] = (u16)e; }
				continue;
			}
			if (pass == 0 && half > g_lg_half) g_lg_half = half;
			lg_cell(mid[0], mid[1], &cx, &cy);
			k = cy * g_lg_nx + cx;
			if (pass == 0)
			{
				s32 hq = 0;
				if (g_lg_start[k] >= 65535) { g_lg_why = 4; g_lg_why_n = (u32)k; return; }
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
			g_lg_ok = 0; g_lg_why = 5; g_lg_why_n = missing;
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
	g_lg_why = 0;
}
static void lg_why_log(const s_cb *c)             /* 6i.184: the grid gave up - said once, with why */
{
	static const char *const why[6] = { "", "the level has more edges than the grid can number (65535): ", "more floor-seam edges than the grid has room for: ",
		"more edges over 32 units long than its list holds (512) - reached at edge ", "a cell overflowed: cell ", "its own check failed, edges missing: " };
	s32 at;
	if (g_lg_ok || g_lg_why < 1 || g_lg_why > 5) return;
	at = lput(0, "level seam grid: OFF - "); at = lput(at, why[g_lg_why]); at = lnum(at, g_lg_why_n);
	at = lput(at, "; the level has "); at = lnum(at, c->ne); at = lput(at, " edges, "); at = lnum(at, c->nv); at = lput(at, " vertices; long ones found so far "); at = lnum(at, (u32)g_lg_nlong);
	lput(at, ". Every search uses the near list instead (rebuilt from every edge when you move 4 units or leave / touch the ground)");
	log_write();
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
	if (++zero_run < 30 || g_t30 - last < 300) return;
	last = g_t30;
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
	if (!g_lg_ok || g_t30 - last < 300) return;
	last = g_t30;
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
		u32 e, ts = tsc_lo();
		g_nlr_n++; if (g_nl_ok && g_nl_for == g_cbm_for && g_nl_sig == g_cbm_sig && g_nl_air != g_sf_air) g_nlr_flip++;   /* 6i.184 */
		g_nl_n = 0; g_nl_ok = 1; g_nl_for = g_cbm_for; g_nl_sig = g_cbm_sig; g_nl_air = g_sf_air; g_nl_c[0] = f[0]; g_nl_c[1] = f[1];
		for (e = 0; e < g_cbm.ne && g_nl_n < NL_MAX; e++)
		{
			float ex, ey, reach;
			if (!lg_floor_edge(&g_cbm, e, mid, &half)) continue;
			ex = mid[0] - f[0]; ey = mid[1] - f[1]; reach = r + half + 4.f;       /* + the 4 you may move before it's rebuilt */
			if (ex * ex + ey * ey <= reach * reach) g_nl[g_nl_n++] = e;
		}
		g_nlr_cyc += tsc_lo() - ts; g_nlr_size = (u32)g_nl_n; if (g_nl_n >= NL_MAX) g_nlr_full++;
	}
	for (i = 0; i < g_nl_n; i++) seam_edge(&g_cbm, id, f, 0, g_nl[i]);
}
static void level_seams(const float *f)
{
	s_xf id;
	float lo[2], hi[2], r;
	s32 x0, x1, y0, y1, cx, cy, k;
	id.on = 0;
	if (g_lg_for != g_cbm_for || g_lg_sig != g_cbm_sig) { g_lg_for = g_cbm_for; g_lg_sig = g_cbm_sig; lg_build(&g_cbm); lg_why_log(&g_cbm); }
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
static float g_bk_land[3] = { 1e9f, 1e9f, 1e9f };
static u32 g_bk_bsp, g_bk_sig;
static void blockers_build_now(const float *f);
/* rebuilt only when needed: moved over 1 unit, took off or landed, a new level, or a second old (its box reaches 10
 * units around you, so 1 unit of movement loses nothing) */
static void blockers_build(const float *f)
{
	float dx = f[0] - g_bk_at[0], dy = f[1] - g_bk_at[1], dz = f[2] - g_bk_at[2];
	float keep = g_slow_hw ? (g_sb_airborne ? 36.f : 9.f) : (g_sb_airborne ? 16.f : 1.f);   /* slow: 3 units (6 in the air) */
	s32 land_ok = 1;
	if (g_bk_air == g_sb_airborne && g_bk_bsp == g_cbm_for && g_bk_sig == g_cbm_sig && g_bk_age < 15 && dx * dx + dy * dy + dz * dz < keep && land_ok)
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
		for (i = 0; i < 3; i++) g_bk_land[i] = g_sb_airborne && g_land_ok ? g_land_p[i] : 1e9f;
		s32 pass;
		for (pass = 0; pass < 2; pass++)
		{
			if (cb_ready()) blockers_add(&g_cbm, &id, lo, hi, clo, chi, pass);
			for (i = 0; i < g_nb_n; i++) blockers_add(&g_nb_cb[i], &g_nb_xf[i], lo, hi, clo, chi, pass);
		}
	}
	/* the columns */
	g_gr_x0 = lo[0]; g_gr_y0 = lo[1];
	g_gr_nx = fti((hi[0] - lo[0]) / 4.f) + 1; if (g_gr_nx > 16) g_gr_nx = 16;
	g_gr_ny = fti((hi[1] - lo[1]) / 4.f) + 1; if (g_gr_ny > 16) g_gr_ny = 16;
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
			s32 x0 = fti((g_bk_lo[i][0] - g_gr_x0) * g_gr_sx), x1 = fti((g_bk_hi[i][0] - g_gr_x0) * g_gr_sx);
			s32 y0 = fti((g_bk_lo[i][1] - g_gr_y0) * g_gr_sy), y1 = fti((g_bk_hi[i][1] - g_gr_y0) * g_gr_sy);
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
	x0 = fti((lo[0] - g_gr_x0) * g_gr_sx); x1 = fti((hi[0] - g_gr_x0) * g_gr_sx);
	y0 = fti((lo[1] - g_gr_y0) * g_gr_sy); y1 = fti((hi[1] - g_gr_y0) * g_gr_sy);
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
	x0 = fti((lo[0] - g_gr_x0) * g_gr_sx); x1 = fti((hi[0] - g_gr_x0) * g_gr_sx);
	y0 = fti((lo[1] - g_gr_y0) * g_gr_sy); y1 = fti((hi[1] - g_gr_y0) * g_gr_sy);
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

/* 6i.14: the render census (6i.4 - 6i.13) and Hide unseen objects (6i.8) retired: hiding saved about a draw an object hidden
 * (6i.10's logs) - the room went to the reflections */
#ifdef HOST
float host_bsp_ray(const float *p0, const float *p1) { return bsp_ray(p0, p1); }
#endif
#ifdef HOST
s32 host_rated_seams(void);
#endif

static void objects_seams(const float *f, u32 self);
static void seams_find(u32 object)
{
	float f[3] = { MF(object + 0x64), MF(object + 0x68), MF(object + 0x6C) };
	s_xf id;
	id.on = 0;
	g_sf_air = g_sb_airborne;
	g_sm_t0 = tsc_lo();
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
	{                                                                        /* 6i.184: this search, on the ground or in the air */
		s32 a = g_sf_air ? 1 : 0;
		u32 d = tsc_lo() - g_sm_t0;
		g_sm_n[a]++; g_sm_cyc[a] += d; if (d > g_sm_max[a]) g_sm_max[a] = d;
		g_sm_edges[a] += g_lsd[0]; g_sm_kept[a] += (u32)g_seam_n; g_sm_pieces[a] += (u32)g_pieces_near; g_sm_objects[a] += (u32)g_objects_near;
	}
}

/* ---- timing: the CPU's cycle counter (733 MHz) - the seam work's cost and the game's frame rate ---- */
#ifdef HOST
static u32 g_host_tsc, g_host_tsc_step = 1000;
static u32 tsc_lo(void) { g_host_tsc += g_host_tsc_step; return g_host_tsc; }
void host_tsc(u32 v, u32 step) { g_host_tsc = v; g_host_tsc_step = step; }   /* 6i.3: the clock set (step 0: held) */
u32 host_tsc_now(void) { return g_host_tsc; }
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
typedef struct { s32 q, opt, debounce, air; float walk, jump, dw, dj, lw, lj, late_start, down; const char *why; s32 exact, ex_n, air_fall; float ex_sp; } s_rd;   /* 6h.30: exact rating */
static s8 g_seam_q[SEAM_MAX];                   /* -1 unrated (not below you), 0 red, 1 yellow, 2 green */
static u8 g_seam_vis[SEAM_MAX];                 /* drawn: rated, or unrated but reachable (seams_vis) */
static u8 g_seam_los[SEAM_MAX], g_seam_np[SEAM_MAX];   /* its sections in view from the camera (bits), how many */
static void seam_line(s32 i);
/* the seams as last finished (all three steps): what's drawn - a search spread over frames never shows half-updated */
static float d_seam_ab[SEAM_MAX][6];
static u8 d_seam_kind[SEAM_MAX], d_seam_vis[SEAM_MAX], d_seam_los[SEAM_MAX], d_seam_np[SEAM_MAX], d_seam_pend[SEAM_MAX], d_seam_dq[SEAM_MAX];   /* dq: the drawn colour, 6h.34 physics included */
static s8 d_seam_q[SEAM_MAX];
static s32 d_seam_n;
static s32 d_land_ok, d_ld_q, d_ld_seam, d_ld_arrow, d_land_edge;
static u32 d_land_s;                              /* 6h.37: the drawn landing's polygon (with d_land_cb / d_land_xf below) */
static float d_land_p[3], d_ld_sa[3], d_ld_sb[3], d_ld_dir[3], d_ld_upax[3], d_ld_up;
#ifdef HOST
static s32 g_host_step_div;                      /* the tests' slow-console schedule (0: all three steps a frame) */
#endif
static s_rd g_seam_rd[SEAM_MAX];
static float g_seam_near[SEAM_MAX][3], g_seam_drop[SEAM_MAX];
static u32 g_seam_open[SEAM_MAX];
static u8 g_seam_pieces[SEAM_MAX];
static u8 g_seam_ways[SEAM_MAX];                /* 6h.30: bit 0 walking off is open, bit 1 jumping (the nearest open piece) */
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
/* 6h.34: back to 6h.29's rule - in the air a seam's colour is the verdict you'd get landing ON it with the drift you have:
 * charged; the drift across it (too fast from 13 deg, too little under 0.6 on a flat seam: yellow; 8-13 deg, or under 1 on a
 * flat seam, or speed / depth at the threshold: amber EDGE). 6h.30-6h.33 left the drift out to stop lines changing colour as
 * you steered - and then a seam you were drifting across wrongly stayed green: your 6h.33 log, green lines under 19 landings,
 * 7 bounced. The drift is what you steer, so the colour follows it again; what's new is a margin so it doesn't flicker on a
 * threshold: a seam that's green stays green until the angle is half a degree past a limit (a tenth under 1 deg), and one
 * that isn't comes back only half a degree inside it. */
static s32 prev_seam_q(s32 i)                                                /* this seam's colour last search, or -2 */
{
	s32 j, k;
	for (j = 0; j < d_seam_n; j++)
	{
		float da = 0.f, db = 0.f;
		for (k = 0; k < 3; k++)
		{
			float a = d_seam_ab[j][k] - g_seam_ab[i][k], b = d_seam_ab[j][3 + k] - g_seam_ab[i][3 + k], c = d_seam_ab[j][k] - g_seam_ab[i][3 + k], d = d_seam_ab[j][3 + k] - g_seam_ab[i][k];
			da += a * a + b * b; db += c * c + d * d;
		}
		if (da < 1e-6f || db < 1e-6f) return d_seam_q[j];
	}
	return -2;
}
static void rate_as_verdict(s32 i)
{
	s_rd *rd = &g_seam_rd[i];
	if (g_sb_charged != 1) { g_seam_q[i] = 0; rd->why = "NOT CHARGED"; }
	else if (g_seam_q[i] == 2)
	{
		float sd[3] = { g_seam_ab[i][3] - g_seam_ab[i][0], g_seam_ab[i][4] - g_seam_ab[i][1], g_seam_ab[i][5] - g_seam_ab[i][2] };
		float sl = fsqrt(dot3(sd, sd)), vc[3] = { g_sf_v[0], g_sf_v[1], -rd->down }, deg, d_on = rd->walk - SB_NEED;
		s32 flat = g_seam_kind[i] == SK_FLAT, was = prev_seam_q(i);
		/* the margin: limits moved half a degree outward if it was green, inward if it wasn't (a tenth for the 0.6 / 1 deg ones) */
		float m = was == 2 ? 1.f : was >= 0 ? -1.f : 0.f;
		float hi = 13.f + 0.5f * m, lo = 0.6f - 0.1f * m, ehi = 8.f + 0.5f * m, elo = 1.f - 0.1f * m;
		if (sl > 1e-6f)
		{
			sd[0] /= sl; sd[1] /= sl; sd[2] /= sl;
			deg = sb_tilt(vc, sd) * 57.29578f;
			if (deg >= hi) { g_seam_q[i] = 1; rd->why = "ANGLE: drifting too fast across"; }
			else if (flat && deg < lo) { g_seam_q[i] = 1; rd->why = "ANGLE: too little drift across"; }
			else if (deg >= ehi || (flat && deg < elo) || (d_on < 0.f ? -d_on : d_on) < 0.05f || rd->dw < 0.21f)
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
static void ex_rate_seams(const float *f);
static void ph_rate_seams(const float *f);
static void landing_seam_include(void);
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
		s32 pieces = fti(len / 0.5f), k;
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
		g_seam_open[i] = open; g_seam_pieces[i] = (u8)pieces; g_seam_ways[i] = (u8)((near_walk ? 1 : 0) | (near_jump ? 2 : 0));
		if (best >= 1e8f) continue;
		g_seam_q[i] = (s8)sb_rate_d(f[2], pz, vh, vz, g_sb_airborne, g_sb_jump, near_walk, near_jump, &g_seam_rd[i]);
		g_seam_rd[i].exact = 0; g_seam_rd[i].ex_n = 0; g_seam_rd[i].ex_sp = 0.f; g_seam_rd[i].air_fall = 0;
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
	ex_rate_seams(f);                                                        /* 6h.30: the exact ratings (below) */
	ph_rate_seams(f);                                                        /* 6h.34: the physics colours (HCM) */
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
static s_cb d_land_cb;                            /* 6h.37: the drawn landing's polygon */
static float d_land_c[3];                        /* 6h.38: where the search put it (the follow stays within 1 of it) */
static s_xf d_land_xf;
static s32 g_land_obj;
/* the landing's verdict */
static s32 g_ld_q = -1, g_ld_seam, g_ld_have_depth;
static float g_ld_sa[3], g_ld_sb[3], g_ld_off, g_ld_bend, g_ld_depth, g_ld_late, g_ld_speed, g_ld_tilt, g_ld_down;
static const char *g_ld_why = "";
static void landing_seam_include(void)
{
	s32 i;
	/* 6h.31: the seam your landing is on is always among them (the 64-seam cap could cut it: "seam unrated" in the log) */
	if (g_sb_airborne && g_land_ok && g_ld_seam)
	{
		s32 have = 0, k2;
		for (i = 0; i < g_seam_n && !have; i++)
		{
			float da = 0.f, db = 0.f;
			for (k2 = 0; k2 < 3; k2++)
			{
				float p1 = g_seam_ab[i][k2] - g_ld_sa[k2], p2 = g_seam_ab[i][3 + k2] - g_ld_sb[k2], p3 = g_seam_ab[i][k2] - g_ld_sb[k2], p4 = g_seam_ab[i][3 + k2] - g_ld_sa[k2];
				da += p1 * p1 + p2 * p2; db += p3 * p3 + p4 * p4;
			}
			have = da < 1e-6f || db < 1e-6f;
		}
		if (!have) seam_add(g_ld_sa, g_ld_sb, (u8)(g_ld_bend >= 1.f ? SK_VALLEY : g_ld_bend <= -1.f ? SK_RIDGE : SK_FLAT), 0.f, (u8)(g_land_obj ? 2 : 0));
	}
}
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
#define LAND_REACH 0.2183f                       /* 6h.35: the pill's radius 0.175 + Havok's 0.01 + 0.0333 of contact reach */
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
		for (j = 0; j < 3; j++) { g_ck_lo[c][j] -= LAND_REACH; g_ck_hi[c][j] += LAND_REACH; }   /* 6h.35 */
	}
}
/* the level's surfaces: the range of grid cells each one's box covers (and whether it's floor enough to land on, the top
 * bit), measured once per level - 4 bytes a surface: the landing and the blockers built every one of the level's ~7,000-
 * 15,000 polygons before looking at their boxes, each search (most of the 360's cost). (5e.12 kept whole float boxes -
 * 393 KB: the game didn't boot.) The cells come from the grid's comparison cell function - no float-to-integer conversion. */
/* 6i.185: YOUR 6i.184 LOG, The Great Journey - "search 27204 (... level 1849, pieces 893, objects 168, landing 22230)":
 * the LANDING is the cost, 22 ms of a 27 ms search, and only in the air (the landing is only looked for there) - a jump
 * halving the frame rate. Why: this cache held 16384 surfaces at most, and for a level with more it was simply not built
 * (the test below returned, without a word) - so every search in the air built every one of the level's polygons to look
 * at its box, which is what the cache was made to stop. The Great Journey has 35419 edges; a multiplayer map (13389 in
 * the same log) fits, and there the same jump costs nothing. (The grid was ON - my reading in 6i.184, that it had given
 * up, was wrong.) Now the cache is as large as the level: up to SBX_MAX = 65536 surfaces - as many as the level's
 * collision can number - 4 bytes each, taken (and, for a larger level, taken again) when a level first needs it; the log
 * says how much. */
#define SBX_MAX 65536
#ifdef HOST
static u8 g_sxy_host[SBX_MAX][4];
static u8 (*g_sxy)[4] = g_sxy_host;
static u32 g_sxy_cap = SBX_MAX;
#else
static u8 (*g_sxy)[4];                          /* cell x from (bit 7: can land), y from, x to, y to - 4 bytes a surface, taken the
                                                   first time it's needed (6h.3: out of the mod's size at boot); none: searched without it */
static u32 g_sxy_cap;                            /* surfaces it has room for */
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
	if (!g_sxy || g_sxy_cap < g_cbm.ns)                                     /* 6i.185: as large as this level (in whole 64 KB steps) */
	{
		static u32 said_for;
		u32 want = (g_cbm.ns + 16383u) & ~16383u;
		s32 at;
		if (g_sxy) { VirtualFree_x(g_sxy, 0, 0x8000 /* MEM_RELEASE */); g_sxy = 0; g_sxy_cap = 0; }
		g_sxy = (u8 (*)[4])VirtualAlloc_x(0, want * 4u, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 4 /* PAGE_READWRITE */);
		if (!g_sxy)
		{
			if (said_for != g_cbm_sig) { said_for = g_cbm_sig; at = lput(0, "surface cache: not enough memory ("); at = lnum(at, want * 4u / 1024u); lput(at, " KB) - the searches run without it (slow in the air)"); log_write(); }
			return;
		}
		g_sxy_cap = want;
		at = lput(0, "surface cache: "); at = lnum(at, want * 4u / 1024u); at = lput(at, " KB for this level's "); at = lnum(at, g_cbm.ns); lput(at, " surfaces");
		log_write();
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
static s32 g_land_edge;                           /* the landing is on a polygon's edge the feet passed just outside */
static void closest_on(const float *p, const float *a, const float *b, float *o);
static __attribute__((noinline)) s32 land_edge_ok(float (*pv)[3], s32 k, const float *wn, s32 i);
static u32 g_land_follows;                         /* frames the circle followed the path between searches (the tests) */
static void land_follow(void);
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
			s32 edge = 0;
			if (!(d0 > 0.f && d1 <= 0.f)) continue;                            /* crossing it from the open side */
			t = d0 / (d0 - d1);
			if (i == g_land_seg && t >= g_land_frac + (g_land_edge ? 1e-3f : 0.f)) continue;
			for (j = 0; j < 3; j++) hit[j] = p[j] + (q[j] - p[j]) * t;
			if (!poly_inside(pv, k, wn, hit))
			{
				/* 6h.35: the feet cross its plane just outside it - but the pill (radius 0.175, plus Havok's 0.0433 of contact
				 * reach) still catches its edge: a ledge or post clipped on the way down, or a crack between two polygons the
				 * point test falls through. Your 6h.34 log: 7 of 31 bounces came where the prediction pointed somewhere else
				 * 1 tick before (up to 18 units away) - you'd caught an edge it didn't see. Taken when the exact contact test
				 * finds the pill touching this polygon on that tick or the next; the circle goes on its nearest edge point */
				float best = 1e9f, cp[3], ep[3];
				s32 m;
				if (i == g_land_seg && t >= g_land_frac - 1e-3f) continue;     /* never ahead of a landing on the polygon itself */
				for (m = 0; m < k; m++)
				{
					float d2;
					closest_on(hit, pv[m], pv[(m + 1) % k], ep);
					d2 = (ep[0] - hit[0]) * (ep[0] - hit[0]) + (ep[1] - hit[1]) * (ep[1] - hit[1]) + (ep[2] - hit[2]) * (ep[2] - hit[2]);
					if (d2 < best) { best = d2; cp[0] = ep[0]; cp[1] = ep[1]; cp[2] = ep[2]; }
				}
				if (best > LAND_REACH * LAND_REACH || !land_edge_ok(pv, k, wn, i)) continue;
				edge = 1;                                                    /* 6h.38: the circle stays at your feet's crossing */
			}
			g_land_ok = 1; g_land_seg = i; g_land_frac = t; g_land_edge = edge;
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

/* (6i.212: seam_yaw - the seam's own sideways push, from Cartographer's landing verdict - is gone with it) */

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

static s32 landing_seam_q(void);
static s32 landing_seam_src(void);
/* ---- 6h.30: the EXACT landing physics (ex_) - MCC halo2.dll 1.3528 / Havok, as h2sb_core.h and HCM's H2CV_SbScan.cpp,
 * verified on 1,000+ recorded landings (the superbounce model, Oct 31). Double math only (x87-native), no libm.
 *  - the landing tick collides BEFORE integrating: the pill at its start-of-tick origin as a convex sweep shape = the
 *    capsule (axis feet+0.175 .. feet+0.55, r 0.175) extended by sweep = pre-solve velocity * dt (only when |v| > 5.4465)
 *  - contacts against the flat surface polygons: least overlap (SAT) when the cores overlap -> depth -(overlap + r + 0.01);
 *    else closest points -> depth (distance - r - 0.01), kept when < 0.0333; one contact per surface
 *  - Havok's order on the landing tick: deepest first, each point coupled with the one before it unless that one is
 *    (952 of 954 recorded landings deepest first, 948 alternating)
 *  - the solver: rhs = min(0.05|d|, dt) * 8/dt (at most 8 u/s a point); 8 passes, tau 0.6, damp 1, coupled 2x2 with
 *    0.99 n0.n1, pushes only, rhs -= n.v after each pass; the body leaves with the AVERAGE of the 8 pass velocities
 *  - all contacts on two-sided / invisible surfaces: you fall through (0 of 65 logged launches were on those alone)
 * This replaces the empirical fit (sb_depth's linear depth, sb_up_estimate's 15 - 0.55 deg, the drift-angle cut-offs) for
 * the landing's verdict: those were fitted to early landings and could not tell a 2 deg near-opposite pair from a 10 deg one. */
#define EX_DT (1.0 / 30.0)
#define EX_SWEEP_MIN 5.4465
#define EX_R_TRI 0.01
#define EX_TOL 0.0333
#define EX_TAU 0.6
#define EX_COUPLE 0.99
#define EX_PASSES 8
#define EX_CAP_LO 0.175
#define EX_CAP_HI 0.55
#define EX_CAP_R 0.175
#define EX_POLY 16
#define EX_ROWS 32
#define EX_SURF_MAX 48                           /* the surfaces kept around one landing (the nearest) */
typedef struct { float n[3]; float v[EX_POLY][3]; s32 nv; float lo[3], hi[3]; s32 soft; u32 key; float dist; } s_exs;
typedef struct { double n[3], d; s32 pair, soft; u32 key; } s_exr;
#ifdef HOST
static s_exs g_ex_s_host[EX_SURF_MAX];
static s_exs *g_ex_s = g_ex_s_host;
#else
static s_exs *g_ex_s;                            /* EX_SURF_MAX surfaces: run-time memory with the rating cache (ex_mem_ready) */
#endif
static s32 g_ex_n;

static double dsqrt(double x)
{
#ifdef HOST
	return x > 0 ? sqrt(x) : 0;
#else
	double r; __asm__("fsqrt" : "=t"(r) : "0"(x > 0 ? x : 0)); return r;
#endif
}
/* 6h.36: float -> integer conversions for the log and the verdict's words, out of line: GCC keeps values on the x87 stack
 * around a conversion (fnstcw / fldcw / fistp), and on the 360 that sequence went wrong in 6h.35's recorder - the landing
 * point's y came out as 100 x its x in your log, a result the Xbox's own processor can't produce (the same machine code
 * run on a real x87 prints it right). Called out of line, the stack is empty around the conversion (5e.12's fix for the
 * level grid on the 360). */
static __attribute__((noinline)) s32 f2i(double v) { return (s32)v; }
static double ex_dot(const double *a, const double *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static void ex_cross(const double *a, const double *b, double *o) { o[0] = a[1] * b[2] - a[2] * b[1]; o[1] = a[2] * b[0] - a[0] * b[2]; o[2] = a[0] * b[1] - a[1] * b[0]; }
static void ex_sub(const double *a, const double *b, double *o) { o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; o[2] = a[2] - b[2]; }
static void ex_madd(const double *a, const double *b, double s, double *o) { o[0] = a[0] + b[0] * s; o[1] = a[1] + b[1] * s; o[2] = a[2] + b[2] * s; }
static double ex_len(const double *a) { return dsqrt(ex_dot(a, a)); }
static double ex_fabs(double x) { return x < 0 ? -x : x; }
static double ex_clamp01(double x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
static s32 ex_norm(const double *a, double *o)
{
	double l = ex_len(a);
	if (l < 1e-12) return 0;
	o[0] = a[0] / l; o[1] = a[1] / l; o[2] = a[2] / l;
	return 1;
}
static s32 ex_point_in_poly(const double *p, const double (*pv)[3], s32 nv, const double *n)
{
	s32 i;
	for (i = 0; i < nv; i++)
	{
		double e[3], w[3], c[3];
		ex_sub(pv[(i + 1) % nv], pv[i], e); ex_sub(p, pv[i], w); ex_cross(e, w, c);
		if (ex_dot(c, n) < -1e-9) return 0;
	}
	return 1;
}
static __attribute__((noinline)) void ex_seg_seg(const double *p1, const double *q1, const double *p2, const double *q2, double *c1, double *c2)
{
	double d1[3], d2[3], r[3], a, e, f, s, t;
	ex_sub(q1, p1, d1); ex_sub(q2, p2, d2); ex_sub(p1, p2, r);
	a = ex_dot(d1, d1); e = ex_dot(d2, d2); f = ex_dot(d2, r);
	if (a < 1e-12 && e < 1e-12) { c1[0] = p1[0]; c1[1] = p1[1]; c1[2] = p1[2]; c2[0] = p2[0]; c2[1] = p2[1]; c2[2] = p2[2]; return; }
	if (a < 1e-12) { s = 0; t = ex_clamp01(f / e); }
	else
	{
		double cc = ex_dot(d1, r);
		if (e < 1e-12) { t = 0; s = ex_clamp01(-cc / a); }
		else
		{
			double b = ex_dot(d1, d2), den = a * e - b * b;
			s = den > 1e-12 ? ex_clamp01((b * f - cc * e) / den) : 0.0;
			t = (b * s + f) / e;
			if (t < 0) { t = 0; s = ex_clamp01(-cc / a); }
			else if (t > 1) { t = 1; s = ex_clamp01((b - cc) / a); }
		}
	}
	ex_madd(p1, d1, s, c1); ex_madd(p2, d2, t, c2);
}
/* one surface against the (swept) pill core: the contact's normal and depth, or 0 (none within tolerance) */
static __attribute__((noinline)) s32 ex_contact(const double core[4][3], s32 nc, const s_exs *s, double *n_out, double *d_out)
{
	double pv[EX_POLY][3], sn[3], axes[64][3], cd[2][3], best = 1e30, bn[3] = { 0, 0, 0 }, bd = 1e30, ba[3] = { 0, 0, 0 }, bb[3] = { 0, 0, 0 }, pd, dist;
	s32 na = 0, ncd = 0, i, k, sg, j, nce, nv = s->nv;
	static const s32 ce[4][2] = { {0, 1}, {2, 3}, {0, 2}, {1, 3} };
	for (k = 0; k < nv; k++) { pv[k][0] = s->v[k][0]; pv[k][1] = s->v[k][1]; pv[k][2] = s->v[k][2]; }
	sn[0] = s->n[0]; sn[1] = s->n[1]; sn[2] = s->n[2];
	axes[na][0] = sn[0]; axes[na][1] = sn[1]; axes[na][2] = sn[2]; na++;
	ex_sub(core[1], core[0], cd[ncd]); ncd++;
	if (nc == 4) { double pn[3], c[3]; ex_sub(core[2], core[0], cd[ncd]); ncd++; ex_cross(cd[0], cd[1], c); if (ex_norm(c, pn)) { axes[na][0] = pn[0]; axes[na][1] = pn[1]; axes[na][2] = pn[2]; na++; } }
	for (i = 0; i < ncd; i++)
		for (k = 0; k < nv; k++)
		{
			double e[3], c[3], ax[3];
			ex_sub(pv[(k + 1) % nv], pv[k], e); ex_cross(cd[i], e, c);
			if (na < 64 && ex_norm(c, ax)) { axes[na][0] = ax[0]; axes[na][1] = ax[1]; axes[na][2] = ax[2]; na++; }
		}
	for (i = 0; i < na; i++)
		for (sg = -1; sg <= 1; sg += 2)
		{
			double n[3] = { axes[i][0] * sg, axes[i][1] * sg, axes[i][2] * sg }, mp = -1e30, mc = 1e30;
			for (k = 0; k < nv; k++) { double y = ex_dot(n, pv[k]); if (y > mp) mp = y; }
			for (k = 0; k < nc; k++) { double y = ex_dot(n, core[k]); if (y < mc) mc = y; }
			if (mp - mc < best) { best = mp - mc; bn[0] = n[0]; bn[1] = n[1]; bn[2] = n[2]; }
		}
	if (best > 0) { n_out[0] = bn[0]; n_out[1] = bn[1]; n_out[2] = bn[2]; *d_out = -(best + EX_CAP_R + EX_R_TRI); return 1; }
#define EX_CONSIDER(pa, pb) do { double dd_[3], dd; ex_sub(pa, pb, dd_); dd = ex_dot(dd_, dd_); if (dd < bd) { bd = dd; ba[0] = (pa)[0]; ba[1] = (pa)[1]; ba[2] = (pa)[2]; bb[0] = (pb)[0]; bb[1] = (pb)[1]; bb[2] = (pb)[2]; } } while (0)
	pd = ex_dot(sn, pv[0]);
	for (k = 0; k < nc; k++) { double q[3]; ex_madd(core[k], sn, -(ex_dot(sn, core[k]) - pd), q); if (ex_point_in_poly(q, pv, nv, sn)) EX_CONSIDER(core[k], q); }
	nce = nc == 4 ? 4 : 1;
	for (i = 0; i < nv; i++)
		for (j = 0; j < nce; j++) { double a[3], b[3]; ex_seg_seg(core[ce[j][0]], core[ce[j][1]], pv[i], pv[(i + 1) % nv], a, b); EX_CONSIDER(a, b); }
	if (nc == 4)
	{
		double e1[3], e2[3], c[3], pn[3];
		ex_sub(core[1], core[0], e1); ex_sub(core[2], core[0], e2); ex_cross(e1, e2, c);
		if (ex_norm(c, pn))
		{
			double quad[4][3], pnn[3] = { -pn[0], -pn[1], -pn[2] };
			for (k = 0; k < 3; k++) { quad[0][k] = core[0][k]; quad[1][k] = core[1][k]; quad[2][k] = core[3][k]; quad[3][k] = core[2][k]; }
			for (k = 0; k < nv; k++)
			{
				double q[3];
				ex_madd(pv[k], pn, -(ex_dot(pn, pv[k]) - ex_dot(pn, core[0])), q);
				if (ex_point_in_poly(q, quad, 4, pn) || ex_point_in_poly(q, quad, 4, pnn)) EX_CONSIDER(q, pv[k]);
			}
		}
	}
#undef EX_CONSIDER
	dist = dsqrt(bd) - EX_CAP_R - EX_R_TRI;
	if (dist >= EX_TOL) return 0;
	{ double d[3]; ex_sub(ba, bb, d); if (!ex_norm(d, n_out)) { n_out[0] = sn[0]; n_out[1] = sn[1]; n_out[2] = sn[2]; } }
	*d_out = dist;
	return 1;
}
/* Havok's contact solver on the landing tick: the exit velocity (the average of the 8 passes) */
#define EX_SOLVE_VMAX 4.085                       /* 6h.34: the solver's start velocity is capped (HCM 2026-11-01) */
static __attribute__((noinline)) void ex_solve(const s_exr *rows, s32 n, const double *v0, double *out)
{
	double v[3] = { v0[0], v0[1], v0[2] }, vsum[3] = { 0, 0, 0 }, rhs[EX_ROWS], l = ex_len(v0);
	s32 i, pass;
	/* 6h.34: before its contact solver the game scales the whole velocity (after the tick's gravity) down to 4.085 u/s,
	 * keeping its direction - the contacts still come from the real fall (the swept pill: deep), only the solver starts
	 * from the capped velocity. Found on MCC: 50 recorded launches solved from the real fall were off by a median of
	 * 1.24 u/s, from the capped start by 0.013; a live probe caught (0.4629, 1.6156, -3.7231), length 4.0849, mid-step.
	 * It explains falls running ~7% under the old model, the game "cancelling" sideways drift (shrunk by the same factor),
	 * and starting bounces (4.85 u/s falls launching 7.0 and 7.4). */
	if (l > EX_SOLVE_VMAX) { v[0] *= EX_SOLVE_VMAX / l; v[1] *= EX_SOLVE_VMAX / l; v[2] *= EX_SOLVE_VMAX / l; }
	for (i = 0; i < n; i++) { double m = 0.05 * ex_fabs(rows[i].d); if (m > EX_DT) m = EX_DT; rhs[i] = rows[i].d < 0 ? m * 8.0 / EX_DT : 0.0; }
	for (pass = 0; pass < EX_PASSES; pass++)
	{
		for (i = 0; i < n;)
		{
			if (i + 1 < n && rows[i + 1].pair)
			{
				const double *a = rows[i].n, *b = rows[i + 1].n;
				double cp = EX_COUPLE * ex_dot(a, b), d0 = EX_TAU * rhs[i] - ex_dot(a, v), d1 = EX_TAU * rhs[i + 1] - ex_dot(b, v);
				double det = 1.0 - cp * cp, l0 = (d0 - cp * d1) / det;
				if (l0 <= 0) { if (d1 > 0) ex_madd(v, b, d1, v); }
				else
				{
					double l1 = (d1 - cp * d0) / det;
					if (l1 > 0) { ex_madd(v, a, l0, v); ex_madd(v, b, l1, v); }
					else if (d0 > 0) ex_madd(v, a, d0, v);
				}
				i += 2;
			}
			else
			{
				double d = EX_TAU * rhs[i] - ex_dot(rows[i].n, v);
				if (d > 0) ex_madd(v, rows[i].n, d, v);
				i++;
			}
		}
		for (i = 0; i < n; i++) rhs[i] -= ex_dot(rows[i].n, v);
		vsum[0] += v[0]; vsum[1] += v[1]; vsum[2] += v[2];
	}
	out[0] = vsum[0] / EX_PASSES; out[1] = vsum[1] / EX_PASSES; out[2] = vsum[2] / EX_PASSES;
}
/* the landing tick's contacts for feet at `feet` (the tick's start) with pre-solve velocity `vpre`, against the gathered
 * surfaces: sorted deepest first and paired Havok's way. Returns the count. */
static __attribute__((noinline)) s32 ex_contacts(const s_exs *S, const s32 *idx, s32 ns, const double *feet, const double *vpre, s_exr *rows, s32 maxr)
{
	double core[4][3], lo[3], hi[3], mg = EX_CAP_R + 0.05;
	s32 nc = 2, n = 0, i, k, prev_paired;
	core[0][0] = feet[0]; core[0][1] = feet[1]; core[0][2] = feet[2] + EX_CAP_LO;
	core[1][0] = feet[0]; core[1][1] = feet[1]; core[1][2] = feet[2] + EX_CAP_HI;
	if (ex_len(vpre) > EX_SWEEP_MIN) { ex_madd(core[0], vpre, EX_DT, core[2]); ex_madd(core[1], vpre, EX_DT, core[3]); nc = 4; }
	for (k = 0; k < 3; k++) lo[k] = hi[k] = core[0][k];
	for (i = 1; i < nc; i++) for (k = 0; k < 3; k++) { if (core[i][k] < lo[k]) lo[k] = core[i][k]; if (core[i][k] > hi[k]) hi[k] = core[i][k]; }
	for (i = 0; i < ns; i++)
	{
		const s_exs *s = &S[idx ? idx[i] : i];
		double nn[3], d;
		if (s->hi[0] < lo[0] - mg || s->lo[0] > hi[0] + mg || s->hi[1] < lo[1] - mg || s->lo[1] > hi[1] + mg || s->hi[2] < lo[2] - mg || s->lo[2] > hi[2] + mg) continue;
		if (!ex_contact(core, nc, s, nn, &d)) continue;
		if (n >= maxr) break;
		rows[n].n[0] = nn[0]; rows[n].n[1] = nn[1]; rows[n].n[2] = nn[2]; rows[n].d = d; rows[n].pair = 0; rows[n].soft = s->soft; rows[n].key = s->key; n++;
	}
	for (i = 1; i < n; i++)                                                  /* stable, deepest first */
	{
		s_exr t = rows[i]; s32 j = i - 1;
		while (j >= 0 && rows[j].d > t.d) { rows[j + 1] = rows[j]; j--; }
		rows[j + 1] = t;
	}
	prev_paired = 1;
	for (k = 0; k < n; k++) { rows[k].pair = k > 0 && !prev_paired; prev_paired = rows[k].pair; }
	return n;
}
/* one landing tick, start to finish. Returns 0 no contact (it falls on), 1 soft surfaces only (it falls through), 2 landed:
 * out = the exit velocity. *deepest = the deepest contact's depth (positive = into the surface), *pen = a penetrating contact */
static u32 g_ex_ticks;                                 /* landing ticks worked out (the tests count them) */
static __attribute__((noinline)) s32 ex_tick(const s_exs *S, const s32 *idx, s32 ns, const double *feet, const double *vpre, double *out, double *deepest, s32 *pen)
{
	g_ex_ticks++;
	s_exr rows[EX_ROWS];
	s32 n = ex_contacts(S, idx, ns, feet, vpre, rows, EX_ROWS), k, hard = 0;
	out[0] = vpre[0]; out[1] = vpre[1]; out[2] = vpre[2];
	*deepest = n ? -rows[0].d : 0.0; *pen = n && rows[0].d < -0.001;
	if (n == 0) return 0;
	for (k = 0; k < n; k++) if (!rows[k].soft) hard = 1;
	if (!hard) return 1;
	ex_solve(rows, n, vpre, out);
	return 2;
}
/* 6h.35: does the pill touch this one polygon on path tick i or i + 1 (the exact contact test, the game's reach)? */
static __attribute__((noinline)) s32 land_edge_ok(float (*pv)[3], s32 k, const float *wn, s32 i)
{
	static s_exs one;
	s32 m, j, tk;
	if (k > EX_POLY) return 0;
	for (m = 0; m < k; m++) for (j = 0; j < 3; j++) one.v[m][j] = pv[m][j];
	one.nv = k; one.soft = 0; one.key = 0;
	for (j = 0; j < 3; j++) { one.n[j] = wn[j]; one.lo[j] = one.hi[j] = pv[0][j]; }
	for (m = 1; m < k; m++) for (j = 0; j < 3; j++) { if (pv[m][j] < one.lo[j]) one.lo[j] = pv[m][j]; if (pv[m][j] > one.hi[j]) one.hi[j] = pv[m][j]; }
	for (tk = i; tk <= i + 1 && tk + 1 < g_pts_n; tk++)
	{
		double feet[3], vpre[3], out[3], deep;
		s32 pen;
		for (j = 0; j < 3; j++) { feet[j] = g_pts[tk][j]; vpre[j] = g_vels[tk + 1][j]; }
		if (ex_tick(&one, 0, 1, feet, vpre, out, &deep, &pen)) return 1;
	}
	return 0;
}
/* a launch, as the bounce counter takes one (6h.19): up over 4 u/s or thrown sideways over 4 u/s, at 5 u/s or more */
static s32 ex_launched(const double *vout, const double *vpre)
{
	/* up 4+ u/s or thrown 4+ u/s sideways, at 5+ u/s (the bounce counter's rule). 6h.33's extra check (the contacts must
	 * change your velocity by half your fall) is gone: with the solver starting from the capped 4.085 u/s, falling on
	 * through comes out at ~4 u/s, never a 22 u/s "MONSTER" */
	double dx = vout[0] - vpre[0], dy = vout[1] - vpre[1];
	return (vout[2] > 4.0 || dx * dx + dy * dy > 16.0) && ex_len(vout) >= 5.0;
}
static double ex_push(const double *vout, const double *vpre)                /* what the contacts did: |exit - entry| */
{
	double d[3] = { vout[0] - vpre[0], vout[1] - vpre[1], vout[2] - vpre[2] };
	return ex_len(d);
}

/* the surfaces around a point - every polygon (floors, walls, two-sided ones too) of the level and of the pieces and
 * objects near you, within r across and zlo..zhi up; the nearest EX_SURF_MAX kept */
static void ex_sift(s_exs *h, s32 n, s32 i)                                 /* max-heap on dist: restore from i down */
{
	for (;;)
	{
		s32 l = 2 * i + 1, r = l + 1, m = i;
		s_exs t;
		if (l < n && h[l].dist > h[m].dist) m = l;
		if (r < n && h[r].dist > h[m].dist) m = r;
		if (m == i) return;
		t = h[i]; h[i] = h[m]; h[m] = t; i = m;
	}
}
static void ex_add_from(s_exs *out, s32 *n_out, s32 cap, const s_cb *c, const s_xf *x, const float *ctr, float r, float zlo, float zhi)
{
	float lo[3] = { ctr[0] - r, ctr[1] - r, ctr[2] + zlo }, hi[3] = { ctr[0] + r, ctr[1] + r, ctr[2] + zhi };
	u32 s;
	if (x->on)                                                                /* 6h.32: a piece or object wholly outside the box: skipped */
	{
		float br = bsp_radius(c, c->vert) * x->sc, d2 = 0.f;
		s32 j;
		for (j = 0; j < 3; j++) { float d = x->pos[j] < lo[j] ? lo[j] - x->pos[j] : x->pos[j] > hi[j] ? x->pos[j] - hi[j] : 0.f; d2 += d * d; }
		if (d2 > br * br) return;
	}
	sbx_query(c, x, lo, hi);
	for (s = 0; s < c->ns; s++)
	{
		float pv[16][3], wn[3], blo[3], bhi[3], dist = 0.f;
		s32 k, i, j, slot;
		if (g_sq_ok && (s32)s < g_sbx_n && sbx_out(c, x, s, lo, hi)) continue;
		k = surf_poly(c, x, s, pv, 0, wn);
		if (!k) continue;
		for (j = 0; j < 3; j++) blo[j] = bhi[j] = pv[0][j];
		for (i = 1; i < k; i++) for (j = 0; j < 3; j++) { if (pv[i][j] < blo[j]) blo[j] = pv[i][j]; if (pv[i][j] > bhi[j]) bhi[j] = pv[i][j]; }
		if (bhi[0] < lo[0] || blo[0] > hi[0] || bhi[1] < lo[1] || blo[1] > hi[1] || bhi[2] < lo[2] || blo[2] > hi[2]) continue;
		for (j = 0; j < 3; j++) { float d = ctr[j] < blo[j] ? blo[j] - ctr[j] : ctr[j] > bhi[j] ? ctr[j] - bhi[j] : 0.f; dist += d * d; }
		if (*n_out < cap) slot = (*n_out)++;
		else
		{
			if (out[0].dist <= dist) continue;                                 /* the heap's root is the farthest kept */
			slot = 0;
		}
		{
			s_exs *e = &out[slot];
			for (i = 0; i < k; i++) for (j = 0; j < 3; j++) e->v[i][j] = pv[i][j];
			e->nv = k; e->dist = dist; e->key = (u32)((c == &g_cbm ? 0u : 0x10000u) | s);
			for (j = 0; j < 3; j++) { e->n[j] = wn[j]; e->lo[j] = blo[j]; e->hi[j] = bhi[j]; }
			e->soft = (M8(c->surf + s * 8 + 4) & 3) != 0;                      /* the surface's flags: two-sided (1), invisible (2) */
		}
		if (*n_out == cap)
		{
			if (slot == cap - 1) { for (i = cap / 2 - 1; i >= 0; i--) ex_sift(out, cap, i); }   /* just filled: heapify */
			else if (slot == 0) ex_sift(out, cap, 0);
		}
	}
}
static void ex_gather_z(const float *ctr, float zhi)
{
	s_xf id;
	s32 i;
	id.on = 0;
	g_ex_n = 0;
	if (cb_ready()) ex_add_from(g_ex_s, &g_ex_n, EX_SURF_MAX, &g_cbm, &id, ctr, 1.3f, -1.6f, zhi);
	for (i = 0; i < g_nb_n; i++) ex_add_from(g_ex_s, &g_ex_n, EX_SURF_MAX, &g_nb_cb[i], &g_nb_xf[i], ctr, 1.3f, -1.6f, zhi);
}


/* ---- 6h.31: the exact rating of the seams - on the ground AND in the air ----
 * 6h.30 rated the seams you stand over exactly but left the ones in the air on the empirical fall rule (arrive at 5.45+ u/s,
 * estimated depth 0.17+: green for nearly every fast fall, whatever your drift) - so every seam recoloured the moment you
 * left the ground, and recoloured again as the nearest piece or your fall changed. Now both use the exact physics, with one
 * meaning: a seam is
 *   green  LIKELY - there is an approach that launches, and it still launches 0.5 u/s of drift either way and 0.06 off the
 *                   point either way (at least 7 of those 8 neighbours);
 *   amber  EDGE   - something launches, but no approach has that margin;
 *   yellow LOTTERY- nothing launches on time, but a contact a tick late would;
 *   red           - nothing launches;
 *   grey  pending - not rated exactly yet (a few each search, nearest the landing first).
 * The approaches: on the ground, walking off and jumping at a run (2.25) and a walk (1.0) toward its nearest open point. In the
 * air, the drift you have and 8 drifts around it within air control's reach before you land - and a seam is rated ONCE per
 * fall (your fall's height and speed are fixed from take-off, so its landing tick is too): its colour is set for the rest of
 * the fall. A fall is new when you take off, or when your vertical speed jumps (a bounce, a Launch, a revert). Until a seam is
 * rated in the air it keeps the colour it had on the ground (if it was rated there), so taking off changes nothing at once.
 * The surfaces come from an area cache (around you on the ground, around your landing in the air); a seam is only rated where
 * the cache is complete around it. */
#define EXA_CAP 384
#define EXR_CACHE 160
#define EXR_MAX_PER_SEARCH 24
#define EXR_CYC_BUDGET 2200000u                    /* ~3 ms of the 733 MHz CPU a search for the exact seam ratings */
#ifdef HOST
static s_exs g_exa_host[EXA_CAP];
static s_exs *g_exa = g_exa_host;
#else
static s_exs *g_exa;                               /* EXA_CAP surfaces, ~100 KB, taken the first time it's needed */
#endif
static s32 g_exa_n, g_exa_ok, g_exa_nb, g_exa_air, g_exa_builds;
static float g_exa_ctr[3], g_exa_zlo, g_exa_zhi, g_exa_r;   /* the area's centre, height band, and the radius it's complete to */
static u32 g_exa_for, g_exa_sig, g_exa_age;
typedef struct { float a[3], b[3], near[3], fz, hx, hy, sp; s8 q; u8 opt, n, late; u32 age, fall; s_rd rd; s32 gt, gh; } s_exrc;   /* 6h.33: a held seam colour */
#ifdef HOST
static s_exrc g_exrc_host[EXR_CACHE];
static s_exrc *g_exrc = g_exrc_host;
static s32 ex_mem_ready(void) { return 1; }
#else
static s_exrc *g_exrc;
/* the exact model's working memory (the landing's surfaces and the seam rating cache, ~24 KB) is taken from the game the
 * first time it's needed - the mod's own size is near its boot limit (6h.3). None: the exact model stays off, said once. */
static s32 ex_mem_ready(void)
{
	static s32 said;
	u8 *m;
	if (g_ex_s && g_exrc) return 1;
	m = (u8 *)VirtualAlloc_x(0, EX_SURF_MAX * sizeof(s_exs) + EXR_CACHE * sizeof(s_exrc), 0x3000, 4);
	if (!m) { if (!said) { said = 1; lput(0, "exact landing physics: not enough memory (24 KB) - the empirical verdicts stay"); log_write(); } return 0; }
	g_ex_s = (s_exs *)m; g_exrc = (s_exrc *)(m + EX_SURF_MAX * sizeof(s_exs));
	return 1;
}
#endif
/* 6i.1: the Superbounce Viewer's run-time memory taken now - what its first use takes anyway (the surface cache 64 KB, the
 * exact model ~24 KB, the seam rating ~100 KB) - before the frame interpolation sizes its own to what's left (ip_alloc).
 * One that fails is tried again, and said, by its own code when the viewer first needs it. */
static void sb_mem_reserve(void)
{
#ifndef HOST
	if (!g_sxy) g_sxy = (u8 (*)[4])VirtualAlloc_x(0, SBX_MAX * 4, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 4 /* PAGE_READWRITE */);
	if (!g_ex_s || !g_exrc)
	{
		u8 *m = (u8 *)VirtualAlloc_x(0, EX_SURF_MAX * sizeof(s_exs) + EXR_CACHE * sizeof(s_exrc), 0x3000, 4);
		if (m) { g_ex_s = (s_exs *)m; g_exrc = (s_exrc *)(m + EX_SURF_MAX * sizeof(s_exs)); }
	}
	if (!g_exa) g_exa = (s_exs *)VirtualAlloc_x(0, EXA_CAP * sizeof(s_exs), 0x3000, 4);
#endif
}
static s32 g_exrc_n;
static u32 g_exrc_clock, g_exrc_for, g_exrc_sig;
/* 6i.36 gave the mod's run-time memory back on the main menu (for Lockout - it didn't help); 6i.38 took that out: the next
 * level's loading could use it up, the interpolation then found none (tried again every 30 s, for good) - your body no
 * longer blended or turned with the view: the legs' swing back. The memory is taken once and held again, as up to 6i.35. */
static s32 g_exr_rated, g_exr_cached, g_exr_pending, g_exr_held;   /* this search */
static u8 g_seam_pend[SEAM_MAX];                   /* not rated exactly yet: drawn grey */
static s32 g_exr_max = EXR_MAX_PER_SEARCH;         /* seams rated a search at most (the tests change it) */
static u32 g_cyc_exact;                            /* the exact model's cycles (the performance line) */
static void ex_area_build(const float *ctr, float zlo, float zhi, s32 air)
{
	s_xf id;
	s32 i;
	float d2 = (ctr[0] - g_exa_ctr[0]) * (ctr[0] - g_exa_ctr[0]) + (ctr[1] - g_exa_ctr[1]) * (ctr[1] - g_exa_ctr[1]), dz = ctr[2] - g_exa_ctr[2];
	if (g_exa_ok && g_exa_air == air && g_exa_for == g_cbm_for && g_exa_sig == g_cbm_sig && d2 < 3.f * 3.f && dz > -1.f && dz < 1.f && g_exa_nb == g_nb_n && ++g_exa_age < 90) return;
	g_exa_builds++;
#ifndef HOST
	if (!g_exa)
	{
		static s32 said;
		g_exa = (s_exs *)VirtualAlloc_x(0, EXA_CAP * sizeof(s_exs), 0x3000, 4);
		if (!g_exa) { if (!said) { said = 1; lput(0, "exact seam rating: not enough memory (100 KB) - the seams keep the empirical colours"); log_write(); } g_exa_ok = 0; return; }
	}
#endif
	id.on = 0;
	g_exa_n = 0;
	if (cb_ready()) ex_add_from(g_exa, &g_exa_n, EXA_CAP, &g_cbm, &id, ctr, 9.5f, zlo, zhi);
	for (i = 0; i < g_nb_n; i++) ex_add_from(g_exa, &g_exa_n, EXA_CAP, &g_nb_cb[i], &g_nb_xf[i], ctr, 9.5f, zlo, zhi);
	g_exa_r = 9.5f;
	if (g_exa_n >= EXA_CAP)                                                  /* full: complete only out to the farthest kept */
	{
		float far = 0.f;
		for (i = 0; i < g_exa_n; i++) if (g_exa[i].dist > far) far = g_exa[i].dist;
		g_exa_r = fsqrt(far);
	}
	g_exa_ok = 1; g_exa_air = air; g_exa_for = g_cbm_for; g_exa_sig = g_cbm_sig; g_exa_nb = g_nb_n; g_exa_age = 0; g_exa_zlo = zlo; g_exa_zhi = zhi;
	for (i = 0; i < 3; i++) g_exa_ctr[i] = ctr[i];
	if (g_exrc_for != g_cbm_for || g_exrc_sig != g_cbm_sig) { g_exrc_n = 0; g_exrc_for = g_cbm_for; g_exrc_sig = g_cbm_sig; }   /* a new level only */
}
/* is the area complete around a point (everything within reach of a landing there was gathered)? */
static s32 ex_area_covers(const float *p)
{
	float dx = p[0] - g_exa_ctr[0], dy = p[1] - g_exa_ctr[1], dz = p[2] - g_exa_ctr[2], r = g_exa_r - 1.9f;
	if (dx < 0.f) dx = -dx;
	if (dy < 0.f) dy = -dy;
	if (dx > 9.5f - 1.3f || dy > 9.5f - 1.3f || dz < g_exa_zlo + 1.6f || dz > g_exa_zhi - 1.4f) return 0;
	return r > 0.f && dx * dx + dy * dy + dz * dz <= r * r;
}
/* the cached surfaces around a point: their indices */
static s32 ex_area_near(const float *ctr, s32 *idx, s32 max)
{
	s32 i, n = 0;
	float lo[3] = { ctr[0] - 1.3f, ctr[1] - 1.3f, ctr[2] - 1.6f }, hi[3] = { ctr[0] + 1.3f, ctr[1] + 1.3f, ctr[2] + 1.4f };
	for (i = 0; i < g_exa_n && n < max; i++)
	{
		const s_exs *e = &g_exa[i];
		if (e->hi[0] < lo[0] || e->lo[0] > hi[0] || e->hi[1] < lo[1] || e->lo[1] > hi[1] || e->hi[2] < lo[2] || e->lo[2] > hi[2]) continue;
		idx[n++] = i;
	}
	return n;
}
/* one landing tick at a seam point: feet start the tick so that the drift d carries them over `near` as they reach its
 * height (frac of the tick), pre-solve velocity (d, vz1). Launched (2), launched only a tick late (1), or not (0). */
static s32 g_ex_try_late = 1;                      /* also try the tick after (off for the neighbours: 6h.32) */
static s32 ex_try(const s32 *idx, s32 ns, const float *near, double ox, double oy, double dx, double dy, double zstart, double vz1, double frac, double *sp)
{
	double feet[3], vpre[3], vout[3], deep, f2[3], v2[3];
	s32 pen, k;
	feet[0] = near[0] + ox - dx * EX_DT * frac; feet[1] = near[1] + oy - dy * EX_DT * frac; feet[2] = zstart;
	vpre[0] = dx; vpre[1] = dy; vpre[2] = vz1;
	if (ex_tick(g_exa, idx, ns, feet, vpre, vout, &deep, &pen) == 2 && ex_launched(vout, vpre)) { *sp = ex_len(vout); return 2; }
	if (!g_ex_try_late) { *sp = 0.0; return 0; }
	for (k = 0; k < 3; k++) { f2[k] = feet[k] + vpre[k] * EX_DT; v2[k] = vpre[k]; }
	v2[2] -= 4.1712 * EX_DT;
	if (ex_tick(g_exa, idx, ns, f2, v2, vout, &deep, &pen) == 2 && ex_launched(vout, v2)) { *sp = ex_len(vout); return 1; }
	*sp = 0.0;
	return 0;
}
/* the landing tick for a fall from (z, vz) onto height pz: its start height, pre-solve vertical speed and the fraction of the
 * tick at which the feet reach pz. 0: never (it lands nowhere near). */
static s32 ex_fall_to(double z, double vz, double pz, double *zstart, double *vz1_out, double *frac, double *t_out)
{
	s32 t;
	for (t = 0; t < 400; t++)
	{
		double vz1 = vz - 4.1712 * EX_DT, zn = z + vz1 * EX_DT;
		if (z > pz && zn <= pz) { *zstart = z; *vz1_out = vz1; *frac = (z - pz) / (z - zn); *t_out = (double)t * EX_DT; return 1; }
		z = zn; vz = vz1;
	}
	return 0;
}
/* 6h.32: the exact work a search has a deadline, checked before every landing tick (6h.31 checked it only between seams,
 * and a seam's 26 ticks ran it far over: ~15 ms a search, the search rate fell to every 6th frame, and most lines stayed
 * grey). A seam cut off by the deadline isn't kept; it's rated next search. */
typedef struct { double dx, dy, zs, vz1, frac; s32 way; } s_appr;
static u32 g_ex_t0, g_ex_budget;
#ifdef HOST
static u32 g_ex_budget_host;                      /* the tests: 1 = no time at all, else cycles */
void host_set_ex_budget(u32 c) { g_ex_budget_host = c; }
#endif
static s32 ex_late(void) { return tsc_lo() - g_ex_t0 > g_ex_budget; }
/* the seam's direction across (horizontal unit, perpendicular to it) */
static void seam_across(const float *a, const float *b, double *ax, double *ay)
{
	double dx = b[0] - a[0], dy = b[1] - a[1], l = dsqrt(dx * dx + dy * dy);
	if (l < 1e-6) { *ax = 1.0; *ay = 0.0; return; }
	*ax = -dy / l; *ay = dx / l;
}
/* one approach and its margin: the approach itself, its drift 0.5 u/s either way ACROSS the seam, and the landing point 0.06
 * either way across it (5 landing ticks; a tick-late check if none launch). green: the approach and 3+ of the 4 neighbours
 * launch; amber: any of the 5; yellow: only a tick late; red: none. -2: the deadline cut it short. */
static s32 ex_rate_one(const s32 *idx, s32 ns, const float *near, const s_appr *ap, double ax, double ay, float *sp_out, s32 *n_out)
{
	static const double nb[4][2] = { { 0.5, 0 }, { -0.5, 0 }, { 0, 0.06 }, { 0, -0.06 } };
	double sp, best = 0.0;
	s32 k, r, nl = 0, nom;
	if (ex_late()) return -2;
	r = ex_try(idx, ns, near, 0, 0, ap->dx, ap->dy, ap->zs, ap->vz1, ap->frac, &sp);
	nom = r == 2;
	if (nom) best = sp;
	else if (r == 1 && !nom) { /* late-only for the approach itself: remembered below */ }
	g_ex_try_late = 0;
	for (k = 0; k < 4; k++)
	{
		double sp2;
		if (ex_late()) { g_ex_try_late = 1; return -2; }
		if (ex_try(idx, ns, near, ax * nb[k][1], ay * nb[k][1], ap->dx + ax * nb[k][0], ap->dy + ay * nb[k][0], ap->zs, ap->vz1, ap->frac, &sp2) == 2)
		{ nl++; if (sp2 > best) best = sp2; }
	}
	g_ex_try_late = 1;
	*n_out = nl + nom;
	if (nom && nl >= 3) { *sp_out = (float)best; return 2; }
	if (nom || nl) { *sp_out = (float)best; return 3; }
	*sp_out = r == 1 ? (float)sp : 0.f;
	return r == 1 ? 1 : 0;
}
/* on the ground: walking off (or jumping, where walking off is blocked) toward the seam's nearest open point at a run;
 * if that doesn't launch, at a walk */
static s32 ex_rate_ground(const float *f, const float *near, s32 ways, double ax, double ay, float *sp, s32 *opt, s32 *n)
{
	s32 idx[EX_SURF_MAX], ns = ex_area_near(near, idx, EX_SURF_MAX), m, way = (ways & 1) ? 0 : 1, q = 0, qm;
	double hx = near[0] - f[0], hy = near[1] - f[1], hl = dsqrt(hx * hx + hy * hy), t;
	static const double mags[2] = { 2.25, 1.0 };
	s_appr ap;
	float spm; s32 nm;
	if (hl < 0.05) { hx = 1.0; hy = 0.0; } else { hx /= hl; hy /= hl; }
	*sp = 0.f; *n = 0; *opt = way;
	if (!ex_fall_to(f[2], way ? (double)g_sb_jump : 0.0, near[2], &ap.zs, &ap.vz1, &ap.frac, &t)) return 0;
	ap.way = way;
	for (m = 0; m < 2; m++)
	{
		ap.dx = hx * mags[m]; ap.dy = hy * mags[m];
		qm = ex_rate_one(idx, ns, near, &ap, ax, ay, &spm, &nm);
		if (qm == -2) return -2;
		if (qm == 2 || (qm == 3 && q != 2) || (qm == 1 && q == 0)) { q = qm; *sp = spm; *n = nm; }
		if (q == 2) break;
	}
	return q;
}
/* in the air: the drift that lands you ON that point - your drift now, changed toward it by what air control can still do
 * before you get there (6h.31 tried your drift and 8 around it: 26 landing ticks a seam, and the colour was whichever of them
 * launched best, not the one you'd fly) */
static s32 ex_rate_air(const float *f, const float *v, const float *near, double ax, double ay, float *sp, s32 *n)
{
	s32 idx[EX_SURF_MAX], ns = ex_area_near(near, idx, EX_SURF_MAX);
	double t, r, cx, cy, cl, vmax;
	s_appr ap;
	if (!ex_fall_to(f[2], v[2], near[2], &ap.zs, &ap.vz1, &ap.frac, &t)) return -1;
	/* where you'd land with no change: g_land_p (its height ~ near's); the change to land at `near` instead, spread over t */
	cx = t > 0.05 ? (near[0] - g_land_p[0]) / t : 0.0; cy = t > 0.05 ? (near[1] - g_land_p[1]) / t : 0.0;
	r = 1.0485 * t; cl = dsqrt(cx * cx + cy * cy);
	if (cl > r && cl > 1e-9) { cx *= r / cl; cy *= r / cl; }
	ap.dx = v[0] + cx; ap.dy = v[1] + cy;
	vmax = dsqrt((double)v[0] * v[0] + (double)v[1] * v[1]); if (vmax < 2.25) vmax = 2.25;
	{ double l = dsqrt(ap.dx * ap.dx + ap.dy * ap.dy); if (l > vmax) { ap.dx *= vmax / l; ap.dy *= vmax / l; } }
	ap.way = 0;
	return ex_rate_one(idx, ns, near, &ap, ax, ay, sp, n);
}
static s32 exrc_find(const float *a, const float *b, u32 fall, const float *near, float fz, float hx, float hy)
{
	s32 i, k;
	for (i = 0; i < g_exrc_n; i++)
	{
		const s_exrc *c = &g_exrc[i];
		float da = 0.f, db = 0.f, dn = 0.f;
		if (c->fall != fall) continue;
		for (k = 0; k < 3; k++) { da += (c->a[k] - a[k]) * (c->a[k] - a[k]) + (c->b[k] - b[k]) * (c->b[k] - b[k]); db += (c->a[k] - b[k]) * (c->a[k] - b[k]) + (c->b[k] - a[k]) * (c->b[k] - a[k]); }
		if (da > 1e-4f && db > 1e-4f) continue;
		if (fall) return i;                                                  /* in the air: once per fall, whatever point */
		for (k = 0; k < 3; k++) dn += (c->near[k] - near[k]) * (c->near[k] - near[k]);
		if (dn > 0.25f || c->fz - fz > 0.1f || fz - c->fz > 0.1f || c->hx * hx + c->hy * hy < 0.866f) continue;   /* within 0.5, 0.1 of height, 30 deg */
		return i;
	}
	return -1;
}
/* the latest ground rating of a seam (any standing point): held through the fall until it's rated in the air */
static s32 exrc_ground_latest(const float *a, const float *b)
{
	s32 i, k, best = -1;
	for (i = 0; i < g_exrc_n; i++)
	{
		const s_exrc *c = &g_exrc[i];
		float da = 0.f, db = 0.f;
		if (c->fall) continue;
		for (k = 0; k < 3; k++) { da += (c->a[k] - a[k]) * (c->a[k] - a[k]) + (c->b[k] - b[k]) * (c->b[k] - b[k]); db += (c->a[k] - b[k]) * (c->a[k] - b[k]) + (c->b[k] - a[k]) * (c->b[k] - a[k]); }
		if (da > 1e-4f && db > 1e-4f) continue;
		if (best < 0 || c->age > g_exrc[best].age) best = i;
	}
	return best;
}
static void ex_put(s32 si, s32 c)
{
	s_rd *rd = &g_seam_rd[si];
	rd->exact = 1; rd->ex_sp = g_exrc[c].sp; rd->ex_n = g_exrc[c].n; rd->opt = g_exrc[c].opt; rd->air_fall = g_exrc[c].fall != 0;
	g_seam_q[si] = g_exrc[c].q;
	g_seam_gt[si] = g_seam_gh[si] = 0;
	if (g_exrc[c].q == 1 && g_seam_piece[si] != 2)
	{
		s32 gt, gh;
		if (ghost_get(&g_seam_ab[si][0], &g_seam_ab[si][3], &gt, &gh)) { g_seam_q[si] = 3; g_seam_gt[si] = gt; g_seam_gh[si] = gh; }   /* a ghost seam: amber */
	}
}
/* 6h.33: the seam lines are the empirical rating again - Cartographer's fall rule (arrive at 5.45+ u/s, depth 0.17+ on time:
 * green; only late: yellow; neither: red), fitted to 1,000+ landings. Your logs scored it: in 6h.30's log the seam under a
 * landing was green 16 times and 14 of those bounced, yellow 6 (none bounced), red never bounced. The exact per-seam
 * ratings of 6h.31-32 did worse (6h.32's log: green 7 times, 1 bounced) and left lines grey while they caught up. The exact
 * model stays where it scored well: the landing circle and arrow.
 * What made the lines change in the air (your first report) is fixed by holding: in the air each seam's colour is taken the
 * first time it's rated in a fall and kept for the rest of that fall (a fall: from take-off, or a bounce / Launch / revert -
 * g_fall_id). Steering, the nearest point moving along the seam, the seam set changing - none recolour it. Uncharged: red. */
static void ex_rate_seams(const float *f)
{
	s32 i;
	(void)f;
	g_exr_rated = g_exr_cached = g_exr_pending = g_exr_held = 0;
	for (i = 0; i < g_seam_n; i++) g_seam_pend[i] = 0;                    /* 6h.34: no hold - the colour follows your drift */
}
#ifdef HOST
s32 host_exr_counts(s32 *rated, s32 *cached) { *rated = g_exr_rated; *cached = g_exr_cached; return g_exrc_n; }
s32 host_exr_more(s32 *pending, s32 *held) { *pending = g_exr_pending; *held = g_exr_held; return (s32)g_fall_id; }
s32 host_exr_area(void) { return g_exa_n; }
void host_set_exr_max(s32 n) { g_exr_max = n; }
u32 host_ex_ticks(void) { return g_ex_ticks; }
/* one contact (normal, depth) solved from an entry velocity: the exit */
void host_ex_solve1(const double *n, double d, const double *vin, double *out) { s_exr r; s32 k; for (k = 0; k < 3; k++) r.n[k] = n[k]; r.d = d; r.pair = 0; r.soft = 0; r.key = 0; ex_solve(&r, 1, vin, out); }
s32 host_ex_launched(double vx, double vy, double vz, double px, double py, double pz) { double o[3] = { vx, vy, vz }, p[3] = { px, py, pz }; return ex_launched(o, p); }
/* the gather alone, into the area cache with a small cap: which surfaces it kept */
s32 host_ex_gather(const float *ctr, s32 cap, u32 *keys)
{
	s_xf id; s32 i, n = 0;
	id.on = 0;
	if (cb_ready()) ex_add_from(g_exa, &n, cap, &g_cbm, &id, ctr, 9.5f, -14.f, 1.5f);
	for (i = 0; i < n; i++) keys[i] = g_exa[i].key;
	g_exa_ok = 0;
	return n;
}
void host_exr_forget(void) { g_exrc_n = 0; g_exa_ok = 0; }
/* the kept seam nearest (x, y): its colour (-1 unrated), and where it came from (0 empirical, 1 exact ground, 2 exact this
 * fall, 3 pending) */
s32 host_seam_src_at(float x, float y, s32 *q)
{
	s32 i, best = -1, k, m; float bd = 1e9f;
	for (i = 0; i < g_seam_n; i++)
	{
		float mx = 0.5f * (g_seam_ab[i][0] + g_seam_ab[i][3]) - x, my = 0.5f * (g_seam_ab[i][1] + g_seam_ab[i][4]) - y, d = mx * mx + my * my;
		if (d < bd) { bd = d; best = i; }
	}
	if (best < 0) { *q = -2; return -1; }
	*q = g_seam_q[best];
	if (!g_sb_airborne) return 0;
	for (k = 0; k < g_exrc_n; k++)                                         /* held this fall: 2 */
	{
		float da = 0.f;
		if (g_exrc[k].fall != g_fall_id) continue;
		for (m = 0; m < 3; m++) da += (g_exrc[k].a[m] - g_seam_ab[best][m]) * (g_exrc[k].a[m] - g_seam_ab[best][m]);
		if (da < 1e-4f) return 2;
	}
	return 0;
}
float ftanf_host(float x) { return ftan(x); }
float fp_tan_host(float x) { return fp_tan(x); }
void host_set_prev_q(const float *pa, const float *pb, s32 q) { s32 k; if (q < -2) { d_seam_n = 0; return; } d_seam_n = 1; for (k = 0; k < 3; k++) { d_seam_ab[0][k] = pa[k]; d_seam_ab[0][3 + k] = pb[k]; } d_seam_q[0] = (s8)q; }
s32 host_vis_uncoloured(void) { s32 i, n = 0; for (i = 0; i < g_seam_n; i++) if (g_seam_vis[i] && g_seam_q[i] < 0) n++; return n; }
s32 host_exr_exact_seams(float *sp) { s32 i, n = 0; *sp = 0.f; for (i = 0; i < g_seam_n; i++) if (g_seam_q[i] >= 0 && g_seam_rd[i].exact) { n++; if (g_seam_rd[i].ex_sp > *sp) *sp = g_seam_rd[i].ex_sp; } return n; }
#endif


/* ---- 6h.34: the seam colours on the validated physics (HCM 2026-11-01, H2CV_Superbounce.cpp) ----
 * In the air, the PH_SEAMS seams nearest your landing that aren't already green get a physics check. Your height and fall
 * decide WHEN you land; your forward speed along your heading decides WHERE you are on that tick. The fall is stepped tick by
 * tick at forward speeds 0 - 2.25 (the biped's most) along your heading until the game's rules find a contact (the swept
 * pill; slower than 5.4465 u/s unswept - it sinks in first), then the capped solver gives the launch:
 *   green       launches at your forward speed now
 *   light blue  slow down (tap or fragment forward)
 *   purple      speed up (up to 2.25)
 *   unchanged   no forward speed launches
 * Its label gets a PHYSICS line: the forward speeds that work and the launch. Your heading is the way you're moving (your
 * view's when you're barely moving). Each search re-checks the oldest few within ~3 ms; a result is used for 10 ticks. */
#define PH_SEAMS 6
#define PH_NS 10                                   /* forward speeds 0, 0.25 .. 2.25 */
#define PH_KEEP 10                                 /* ticks a result stays in use */
static u8 g_seam_ph[SEAM_MAX];                     /* 0 not checked, 1 launches at your speed (green), 2 slow down (light blue), 3 speed up (purple), 4 none launches (red) - 6i.127 */
static float g_seam_ph_lo[SEAM_MAX], g_seam_ph_hi[SEAM_MAX], g_seam_ph_sp[SEAM_MAX], g_seam_ph_cur[SEAM_MAX];
typedef struct { float a[3], b[3], lo, hi, sp, cur; u32 tick, fall; u8 ph, used; } s_phc;   /* 6i.127: cur = 1 in the air, 0 a jump from here */
static s_phc g_phc[PH_SEAMS * 2];
static u32 g_ph_checks;                            /* checks done (the tests) */
/* 6i.127: one seam - HCM's per-seam physics check as it stands now (Harc's Build, H2CV_Superbounce.cpp sb_phys_check,
 * 2026-11-05: "a pure function of its inputs"). What changed since the 6h.34 merge, the model the same:
 *  - the way you go: in the air your drift's direction (when over 0.3 u/s), else straight toward the seam's nearest point
 *    (6h.34: your view's direction); on the GROUND too - the jump from here (your take-off speed) holding forward toward it;
 *  - the fall stepped tick by tick from where you are, and the FIRST landing the rules find - within 1.5 wu above the
 *    seam's height, while descending, wherever on the way - decides (6h.34: only a landing within 0.9 of where your heading
 *    met the seam line counted);
 *  - a launch = the solved velocity up over 4 u/s (6h.34: the bounce counter's up-or-sideways 4 at 5 u/s);
 *  - your speed is one of the ten (0, 0.25 .. 2.25: yours rounded to the nearest, 2.25 for a jump); missing it, the nearest
 *    launching speed below (on a tie) or above says slow down / speed up, and the range shown is the unbroken run of
 *    launching speeds around it, with the best launch in that run (6h.34: every launching speed, best anywhere);
 *  - none launches: RED, whatever Cartographer's rule said (6h.34: left as it was).
 * The surfaces are gathered around where the test is (1.3 wu, re-gathered as the fall moves on), as HCM's landingLaunch
 * gathers within 1 wu of the feet. Returns 1 keep (green), 2 slow down (light blue), 3 speed up (purple), 4 none (red),
 * -1 out of time (not kept). */
static u32 g_ph_t0;                                /* 6h.35: this search's start (the budget is checked between landings too) */
static __attribute__((noinline)) s32 ph_check(const float *f, const float *v, s32 air, const float *near, float *lo, float *hi, float *bsp)
{
	double g1 = 4.1712 * EX_DT, h0 = dsqrt((double)v[0] * v[0] + (double)v[1] * v[1]), ux, uy, ul, lv[PH_NS];
	float gc[3];
	s32 ok[PH_NS], i, k, nok = 0, i0, pick = -1, code, gathered = 0;
	*lo = *hi = *bsp = 0.f;
	gc[0] = f[0]; gc[1] = f[1]; gc[2] = f[2];
	if (air && h0 > 0.3) { ux = v[0] / h0; uy = v[1] / h0; }
	else { ux = (double)near[0] - f[0]; uy = (double)near[1] - f[1]; }
	ul = dsqrt(ux * ux + uy * uy);
	if (ul > 1e-4) { ux /= ul; uy /= ul; } else { ux = 1.0; uy = 0.0; }
	for (i = 0; i < PH_NS; i++)
	{
		double h = 0.25 * (double)i, x = f[0], y = f[1], z = f[2], vz = air ? (double)v[2] : (double)g_sb_jump;
		ok[i] = 0; lv[i] = 0.0;
		for (k = 0; k < 600 && z > (double)near[2] - 0.6; k++)
		{
			if (vz - g1 < 0.0 && z - (double)near[2] < 1.5)              /* descending, near its height: a landing? */
			{
				double feet[3], vpre[3], out[3], deep, gx = x - gc[0], gy = y - gc[1], gz = z - gc[2];
				s32 pen, kind;
				if (tsc_lo() - g_ph_t0 > EXR_CYC_BUDGET) return -1;      /* 6h.35: out of time mid-seam - not kept */
				if (!gathered || gx * gx + gy * gy > 0.09 || gz > 0.5 || gz < -0.5) { gc[0] = (float)x; gc[1] = (float)y; gc[2] = (float)z; ex_gather_z(gc, 2.4f); gathered = 1; }
				feet[0] = x; feet[1] = y; feet[2] = z; vpre[0] = ux * h; vpre[1] = uy * h; vpre[2] = vz - g1;
				kind = ex_tick(g_ex_s, 0, g_ex_n, feet, vpre, out, &deep, &pen);
				if (kind == 2) { if (out[2] > 4.0) { ok[i] = 1; lv[i] = ex_len(out); nok++; } break; }   /* the landing (hard surfaces) */
			}
			vz -= g1; z += vz * EX_DT; x += ux * h * EX_DT; y += uy * h * EX_DT;
		}
	}
	g_ph_checks++;
	i0 = air ? (s32)(h0 / 0.25 + 0.5) : PH_NS - 1;
	if (i0 < 0) i0 = 0;
	if (i0 > PH_NS - 1) i0 = PH_NS - 1;
	if (ok[i0]) { code = 1; pick = i0; }
	else if (nok)
	{
		s32 ib = -1, ia = -1;
		for (i = i0 - 1; i >= 0; i--) if (ok[i]) { ib = i; break; }
		for (i = i0 + 1; i < PH_NS; i++) if (ok[i]) { ia = i; break; }
		if (ib >= 0 && (ia < 0 || i0 - ib <= ia - i0)) { code = 2; pick = ib; } else { code = 3; pick = ia; }
	}
	else return 4;
	{
		s32 a = pick, b = pick;
		double best = 0.0;
		while (a > 0 && ok[a - 1]) a--;
		while (b < PH_NS - 1 && ok[b + 1]) b++;
		for (i = a; i <= b; i++) if (lv[i] > best) best = lv[i];
		*lo = 0.25f * (float)a; *hi = 0.25f * (float)b; *bsp = (float)best;
	}
	return code;
}
static void ph_rate_seams(const float *f)
{
	s32 i, k, n = 0, order[SEAM_MAX];
	float v[3] = { g_sf_v[0], g_sf_v[1], g_sf_v[2] }, dk[SEAM_MAX];
	s32 air = g_sb_airborne != 0;
	u32 t0 = tsc_lo();
	g_ph_t0 = t0;
	for (i = 0; i < g_seam_n; i++) g_seam_ph[i] = 0;
	if (g_sb_charged != 1 || !ex_mem_ready()) return;                    /* 6i.127: on the ground too - a jump from here (HCM) */
	/* the nearest PH_SEAMS (to your landing) - 6h.35: green ones too (HCM checks only the others). Your 6h.34 log: the
	 * exact physics called 24 of 24 bounces at the landing, the seam under you was green 25 times and 18 bounced; a
	 * Cartographer-green seam is now checked too, and shown by the physics where it finds an answer */
	for (i = 0; i < g_seam_n; i++)
		if (g_seam_q[i] >= 0)
		{
			const float *rp = air && g_land_ok ? g_land_p : f;                   /* 6i.127: in the air your landing, else you */
			float dx = g_seam_near[i][0] - rp[0], dy = g_seam_near[i][1] - rp[1];
			order[n] = i; dk[n] = dx * dx + dy * dy; n++;
		}
	for (i = 1; i < n; i++) { s32 t = order[i]; float td = dk[i]; s32 j = i - 1; while (j >= 0 && dk[j] > td) { order[j + 1] = order[j]; dk[j + 1] = dk[j]; j--; } order[j + 1] = t; dk[j + 1] = td; }
	if (n > PH_SEAMS) n = PH_SEAMS;
	for (k = 0; k < PH_SEAMS * 2; k++) g_phc[k].used = 0;
	/* from the cache where fresh; the oldest re-checked first, while there's time */
	for (;;)
	{
		s32 pick = -1, pc = -1;
		u32 oldest = 0;
		for (k = 0; k < n; k++)
		{
			s32 si = order[k], c = -1, m;
			for (m = 0; m < PH_SEAMS * 2 && c < 0; m++)
			{
				float da = 0.f, db = 0.f; s32 q;
				for (q = 0; q < 3; q++) { da += (g_phc[m].a[q] - g_seam_ab[si][q]) * (g_phc[m].a[q] - g_seam_ab[si][q]) + (g_phc[m].b[q] - g_seam_ab[si][3 + q]) * (g_phc[m].b[q] - g_seam_ab[si][3 + q]); db += (g_phc[m].a[q] - g_seam_ab[si][3 + q]) * (g_phc[m].a[q] - g_seam_ab[si][3 + q]) + (g_phc[m].b[q] - g_seam_ab[si][q]) * (g_phc[m].b[q] - g_seam_ab[si][q]); }
				if ((da < 1e-4f || db < 1e-4f) && g_phc[m].fall == g_fall_id) c = m;
			}
			if (c >= 0 && g_sb_tick - g_phc[c].tick <= PH_KEEP) continue;  /* fresh */
			{
				u32 age = c >= 0 ? g_sb_tick - g_phc[c].tick : 0xffffffffu;
				if (pick < 0 || age > oldest) { pick = si; pc = c; oldest = age; }
			}
		}
		if (pick < 0 || tsc_lo() - t0 > EXR_CYC_BUDGET) break;
		{
			float lo, hi, bsp;
			s32 ph = ph_check(f, v, air, &g_seam_near[pick][0], &lo, &hi, &bsp), m;
			if (ph < 0) break;                                               /* out of time: the rest next search */
			if (pc < 0)
			{
				pc = 0;
				for (m = 1; m < PH_SEAMS * 2; m++) if (g_phc[m].tick < g_phc[pc].tick || g_phc[m].fall != g_fall_id) { pc = m; if (g_phc[m].fall != g_fall_id) break; }
			}
			for (m = 0; m < 3; m++) { g_phc[pc].a[m] = g_seam_ab[pick][m]; g_phc[pc].b[m] = g_seam_ab[pick][3 + m]; }
			g_phc[pc].ph = (u8)ph; g_phc[pc].lo = lo; g_phc[pc].hi = hi; g_phc[pc].sp = bsp; g_phc[pc].cur = air ? 1.f : 0.f; g_phc[pc].tick = g_sb_tick; g_phc[pc].fall = g_fall_id;
		}
	}
	/* the results onto the seams */
	for (k = 0; k < n; k++)
	{
		s32 si = order[k], m;
		for (m = 0; m < PH_SEAMS * 2; m++)
		{
			float da = 0.f, db = 0.f; s32 q;
			if (g_phc[m].fall != g_fall_id || g_sb_tick - g_phc[m].tick > PH_KEEP) continue;
			for (q = 0; q < 3; q++) { da += (g_phc[m].a[q] - g_seam_ab[si][q]) * (g_phc[m].a[q] - g_seam_ab[si][q]) + (g_phc[m].b[q] - g_seam_ab[si][3 + q]) * (g_phc[m].b[q] - g_seam_ab[si][3 + q]); db += (g_phc[m].a[q] - g_seam_ab[si][3 + q]) * (g_phc[m].a[q] - g_seam_ab[si][3 + q]) + (g_phc[m].b[q] - g_seam_ab[si][q]) * (g_phc[m].b[q] - g_seam_ab[si][q]); }
			if (da < 1e-4f || db < 1e-4f)
			{
				g_seam_ph[si] = g_phc[m].ph; g_seam_ph_lo[si] = g_phc[m].lo; g_seam_ph_hi[si] = g_phc[m].hi; g_seam_ph_sp[si] = g_phc[m].sp; g_seam_ph_cur[si] = g_phc[m].cur;
				break;
			}
		}
	}
	g_cyc_exact += tsc_lo() - t0;
}
#ifdef HOST
u32 host_ph_checks(void) { return g_ph_checks; }
s32 host_ph_check(const float *f, const float *v, s32 air, const float *near, float *lo, float *hi, float *sp) { return ph_check(f, v, air, near, lo, hi, sp); }
void host_ph_forget(void) { s32 k; for (k = 0; k < PH_SEAMS * 2; k++) { g_phc[k].fall = 0xffffffffu; g_phc[k].tick = 0; } }
s32 host_seam_ph_at(float x, float y, float *lo, float *hi, float *sp)
{
	s32 i, best = -1; float bd = 1e9f;
	for (i = 0; i < g_seam_n; i++)
	{
		float mx = 0.5f * (g_seam_ab[i][0] + g_seam_ab[i][3]) - x, my = 0.5f * (g_seam_ab[i][1] + g_seam_ab[i][4]) - y, d = mx * mx + my * my;
		if (d < bd) { bd = d; best = i; }
	}
	if (best < 0) return -1;
	*lo = g_seam_ph_lo[best]; *hi = g_seam_ph_hi[best]; *sp = g_seam_ph_sp[best];
	return g_seam_ph[best];
}
#endif

/* the landing's exact verdict. The landing tick is the first of the path's ticks around the floor crossing whose swept pill
 * penetrates something (the one before the crossing when a rail or wall is hit first); its exit velocity decides. Around
 * it: the same tick with the drift changed by 0.5 u/s each way (how much room the approach has), and the tick after (a
 * contact a tick late). The result is shown once two searches in a row agree (no single-search flicker). */
static s32 g_ex_q = -1, g_ex_kind, g_ex_tick, g_ex_launch, g_ex_late, g_ex_robust, g_ex_fix, g_ex_nb, g_ex_t2, g_ex_early;
static double g_ex_t2sp;                           /* 6h.37: the tick after a predicted launch: contact (1) or clear (0), and the speed after it */
static double g_ex_v[3], g_ex_vpre[3], g_ex_sp, g_ex_deep, g_ex_late_sp, g_ex_late_deep;
static char g_ex_why[96];
static s32 g_ex_shown_q = -1, g_ex_pend_q = -1, g_ex_pend_n;
static char g_ex_shown_why[96];
static float g_ex_shown_up, g_ex_shown_dir[3];
static s32 g_ex_shown_arrow;
#ifdef HOST
#include <stdio.h>
void host_ex_dump(void)
{
	s32 i, k; double feet[3], vpre[3]; s_exr rows[EX_ROWS]; s32 n;
	fprintf(stderr, "ex: %d surfaces; landing tick %d of %d, seg %d\n", g_ex_n, g_ex_tick, g_pts_n, g_land_seg);
	for (i = 0; i < g_ex_n; i++) fprintf(stderr, "  surf key %x n %.3f %.3f %.3f nv %d lo %.2f %.2f %.2f hi %.2f %.2f %.2f soft %d\n", g_ex_s[i].key, g_ex_s[i].n[0], g_ex_s[i].n[1], g_ex_s[i].n[2], g_ex_s[i].nv, g_ex_s[i].lo[0], g_ex_s[i].lo[1], g_ex_s[i].lo[2], g_ex_s[i].hi[0], g_ex_s[i].hi[1], g_ex_s[i].hi[2], g_ex_s[i].soft);
	for (k = 0; k < 3; k++) { feet[k] = g_pts[g_ex_tick][k]; vpre[k] = g_vels[g_ex_tick + 1][k]; }
	n = ex_contacts(g_ex_s, 0, g_ex_n, feet, vpre, rows, EX_ROWS);
	fprintf(stderr, "  feet %.3f %.3f %.3f v %.2f %.2f %.2f: %d rows\n", feet[0], feet[1], feet[2], vpre[0], vpre[1], vpre[2], n);
	for (i = 0; i < n; i++) fprintf(stderr, "   row key %x n %.3f %.3f %.3f d %.3f pair %d\n", rows[i].key, rows[i].n[0], rows[i].n[1], rows[i].n[2], rows[i].d, rows[i].pair);
}
s32 host_ex_q(void) { return g_ex_q; }
double host_ex_speed(void) { return g_ex_sp; }
const char *host_ex_why(void) { return g_ex_why; }
s32 host_ex_shown_q(void) { return g_ex_shown_q; }
#endif
static s32 why_put(s32 at, const char *s) { while (*s && at < 94) g_ex_why[at++] = *s++; g_ex_why[at] = 0; return at; }
static s32 why_num(s32 at, s32 v)
{
	char b[12]; s32 n = 0;
	if (v < 0) { at = why_put(at, "-"); v = -v; }
	do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v);
	while (n && at < 94) g_ex_why[at++] = b[--n];
	g_ex_why[at] = 0; return at;
}
static s32 why_speed(s32 at, double sp)                                       /* "23.4 u/s" */
{
	s32 t = f2i(sp * 10.0 + 0.5);
	at = why_num(at, t / 10); at = why_put(at, "."); at = why_num(at, t % 10); return why_put(at, " u/s");
}
static const char *ex_class(double sp) { return sp >= 35.0 ? " DREADNOUGHT" : sp >= 20.0 ? " MONSTER" : ""; }
static void ex_reset_shown(void) { g_ex_shown_q = -1; g_ex_pend_q = -1; g_ex_pend_n = 0; g_ex_q = -1; }
static void ex_verdict(void)
{
	double feet[3], vpre[3], vout[3], deep, late_v[3], late_deep, best_fix = 0.0;
	s32 j, j0 = g_land_seg, pen, kind, q, launch, late = 0, robust = 1, fix = -1, k, at;
	u32 tv0 = tsc_lo();
	if (!g_land_ok || j0 + 1 >= g_pts_n || !ex_mem_ready()) return;
	/* 6h.38: the landing tick is the FIRST tick whose pill touches anything - up to 4 ticks before the floor crossing. The game
	 * solves on that tick with your velocity capped at 4.085: a graze of an edge or wall on the way down (a sideways push)
	 * kills the fall, and the floor below then gets a 4 u/s landing. Your 6h.37 log: three green / amber circles that
	 * didn't bounce came down at "first tick 4.6 - 4.8 u/s (up -3.8 to -4.0)" - the capped velocity after such a graze; the
	 * old rule looked at the crossing tick and the one before it only, and took that one only if it was already deep */
	{
		s32 js = j0 > 4 ? j0 - 4 : 0;
		float zhi = g_pts[js][2] - g_land_p[2] + 1.0f;
		ex_gather_z(g_land_p, zhi > 1.4f ? zhi : 1.4f);
		j = j0;
		for (k = js; k < j0; k++)
		{
			s32 kk;
			for (kk = 0; kk < 3; kk++) { feet[kk] = g_pts[k][kk]; vpre[kk] = g_vels[k + 1][kk]; }
			if (ex_tick(g_ex_s, 0, g_ex_n, feet, vpre, vout, &deep, &pen) != 2) continue;
			/* a contact before the crossing tick: already deep - the landing is here (as before, now up to 4 ticks back); or a
			 * graze that leaves you still falling (a sideways push, your velocity capped at ~4 u/s: exit still going down) -
			 * the landing is here too, and it stops the fall. A floor touched just within reach (it stops you: exit not going
			 * down) is left to the crossing tick, as HCM's scanner does */
			if (pen || vout[2] < -2.0) { j = k; break; }
		}
		g_ex_early = j0 - j;
	}
	for (k = 0; k < 3; k++) { feet[k] = g_pts[j][k]; vpre[k] = g_vels[j + 1][k]; }
	kind = ex_tick(g_ex_s, 0, g_ex_n, feet, vpre, vout, &deep, &pen);
	if (kind == 0 && j == j0 && (ex_len(vpre) <= EX_SWEEP_MIN || g_land_edge) && j0 + 1 < g_pts_n)
	{
		/* 6h.34: a slow fall - this tick's unswept pill touches nothing, it moves into the floor, and the next tick's
		 * contacts (deep) come from the sunk position: a starting bounce's landing tick */
		j = j0 + 1;
		for (k = 0; k < 3; k++) { feet[k] = g_pts[j][k]; vpre[k] = j + 1 < g_pts_n ? g_vels[j + 1][k] : g_vels[j][k]; }
		if (j + 1 >= g_pts_n) vpre[2] -= 4.1712 * EX_DT;
		kind = ex_tick(g_ex_s, 0, g_ex_n, feet, vpre, vout, &deep, &pen);
	}
	launch = kind == 2 && ex_launched(vout, vpre);
	g_ex_tick = j; g_ex_kind = kind; g_ex_deep = deep;
	/* 6h.37: the tick after a launch, for the recorder - your logs' false greens all came down ~0.4 higher than predicted
	 * with no bounce: launched for one tick (11.9 u/s x 1/30 = 0.40 up), then stopped. Does the pill, moved by the launch,
	 * still touch something on the next tick, and what speed is left after it? (logged only - not in the verdict yet) */
	g_ex_t2 = -1; g_ex_t2sp = 0.0;
	if (kind == 2 && launch)
	{
		double f2[3], v2[3], o2[3], d2; s32 p2, k2b;
		for (k2b = 0; k2b < 3; k2b++) { f2[k2b] = feet[k2b] + vout[k2b] * EX_DT; v2[k2b] = vout[k2b]; }
		v2[2] -= 4.1712 * EX_DT;
		g_ex_t2 = ex_tick(g_ex_s, 0, g_ex_n, f2, v2, o2, &d2, &p2) != 0;
		g_ex_t2sp = ex_len(g_ex_t2 ? o2 : v2);
	}
	for (k = 0; k < 3; k++) { g_ex_v[k] = vout[k]; g_ex_vpre[k] = vpre[k]; }
	g_ex_sp = kind == 2 ? ex_len(vout) : 0.0;
	/* a tick late */
	g_ex_late_sp = 0.0; g_ex_late_deep = 0.0;
	if (j + 2 < g_pts_n)
	{
		double f2[3], v2[3];
		for (k = 0; k < 3; k++) { f2[k] = g_pts[j + 1][k]; v2[k] = g_vels[j + 2][k]; }
		if (ex_tick(g_ex_s, 0, g_ex_n, f2, v2, late_v, &late_deep, &pen) == 2) { late = ex_launched(late_v, v2); g_ex_late_sp = ex_len(late_v); g_ex_late_deep = late_deep; }
	}
	/* the drift changed by 0.5 u/s each way (6h.30's check - its LIKELYs bounced 10 of 12): does it still launch, or start to? */
	{
		static const double dd[4][2] = { { 0.5, 0 }, { -0.5, 0 }, { 0, 0.5 }, { 0, -0.5 } };
		s32 nl = 0;
		for (k = 0; k < 4; k++)
		{
			double vp[3] = { vpre[0] + dd[k][0], vpre[1] + dd[k][1], vpre[2] }, vo[3], dp; s32 pn;
			s32 l = ex_tick(g_ex_s, 0, g_ex_n, feet, vp, vo, &dp, &pn) == 2 && ex_launched(vo, vp);
			if (l) nl++;
			if (launch && !l) robust = 0;
			if (!launch && l && ex_len(vo) > best_fix) { best_fix = ex_len(vo); fix = k; }
		}
		g_ex_nb = nl;
		g_ex_launch = launch; g_ex_late = late; g_ex_robust = robust; g_ex_fix = fix;
		/* the words */
		at = 0;
		if (g_sb_charged != 1) { q = 0; at = why_put(at, "NOT CHARGED"); }
		else if (kind == 0) { q = 0; at = why_put(at, "no contact on the landing tick"); }
		else if (kind == 1) { q = 0; at = why_put(at, "FALLS THROUGH: two-sided / invisible surfaces only"); }
		else if (launch)
		{
			q = robust ? 2 : 3;
			at = why_put(at, robust ? "LIKELY: " : "EDGE: "); at = why_speed(at, g_ex_sp); at = why_put(at, ex_class(g_ex_sp));
			if (!robust) at = why_put(at, " - 0.5 u/s of drift either way loses it");
		}
		else if (late)
		{
			q = 1;
			at = why_put(at, "LOTTERY: a tick late it launches "); at = why_speed(at, g_ex_late_sp); at = why_put(at, ex_class(g_ex_late_sp));
		}
		else if (fix >= 0)
		{
			double hx = vpre[0], hy = vpre[1], hl = dsqrt(hx * hx + hy * hy), f, r;
			const char *dir;
			if (hl < 0.3) { hx = 1.0; hy = 0.0; hl = 1.0; }
			hx /= hl; hy /= hl;
			f = dd[fix][0] * hx + dd[fix][1] * hy; r = dd[fix][0] * hy - dd[fix][1] * hx;
			dir = (f > 0.25) ? "FASTER" : (f < -0.25) ? "SLOWER" : (r > 0) ? "to the RIGHT" : "to the LEFT";
			q = 1;
			at = why_put(at, "FIXABLE: drift 0.5 u/s "); at = why_put(at, dir); at = why_put(at, " for "); at = why_speed(at, best_fix); at = why_put(at, ex_class(best_fix));
		}
		else if (g_ex_early > 0 && deep < 0.02)
		{
			q = 0;
			at = why_put(at, "NO LAUNCH: you graze an edge first ("); at = why_num(at, g_ex_early); at = why_put(at, g_ex_early == 1 ? " tick" : " ticks");
			at = why_put(at, " early) - it stops the fall");
		}
		else
		{
			q = 0;
			at = why_put(at, "NO LAUNCH (push "); at = why_speed(at, ex_push(vout, vpre)); at = why_put(at, ", depth "); at = why_num(at, f2i(deep * 100.0 + 0.5)); at = why_put(at, "/100)");
		}
	}
	g_ex_q = q;
	/* a ghost seam: a pure lottery landing on a seam whose late contacts have launched before (6h: remembered per level) */
	if (q == 1 && late && g_ld_seam && !g_land_obj)
	{
		s32 gt = 0, gh = 0;
		g_la_ok = 1;
		for (k = 0; k < 3; k++) { g_la_a[k] = g_ld_sa[k]; g_la_b[k] = g_ld_sb[k]; }
		if (ghost_get(g_ld_sa, g_ld_sb, &gt, &gh))
		{
			at = why_put(0, "GHOST SEAM: "); at = why_num(at, gh); at = why_put(at, " of "); at = why_num(at, gt); at = why_put(at, " lottery landings here launched");
			g_ex_q = q = 3;
		}
	}
	else g_la_ok = 0;
	/* shown once two searches agree; a new fall starts afresh (ex_reset_shown on the ground) */
	if (g_ex_shown_q < 0 || j <= 12) { g_ex_shown_q = q; g_ex_pend_q = q; g_ex_pend_n = 0; }   /* 6h.32: no hold in the last 0.4 s */
	else if (q != g_ex_shown_q)
	{
		if (q == g_ex_pend_q) g_ex_pend_n++; else { g_ex_pend_q = q; g_ex_pend_n = 1; }
		if (g_ex_pend_n >= 2) { g_ex_shown_q = q; g_ex_pend_n = 0; }
	}
	else g_ex_pend_n = 0;
	if (g_ex_shown_q == q)
	{
		for (k = 0; k < 96; k++) { g_ex_shown_why[k] = g_ex_why[k]; if (!g_ex_why[k]) break; }
		g_ex_shown_arrow = launch;
		g_ex_shown_up = (float)g_ex_sp;
		if (launch && g_ex_sp > 1e-6)
		{
			double d[3] = { vout[0], vout[1], vout[2] > 0.0 ? vout[2] : 0.0 }, l = ex_len(d);
			if (l < 1e-6) { d[2] = 1.0; l = 1.0; }
			for (k = 0; k < 3; k++) g_ex_shown_dir[k] = (float)(d[k] / l);
		}
	}
	/* into the viewer's landing: the circle's and arrow's colour, the words, the arrow (along the exit velocity) */
	g_ld_q = g_ex_shown_q; g_ld_why = g_ex_shown_why;
	g_ld_arrow = g_ex_shown_arrow; g_ld_up = g_ex_shown_arrow ? g_ex_shown_up : 0.f;
	if (g_ex_shown_arrow) for (k = 0; k < 3; k++) { g_ld_dir[k] = g_ex_shown_dir[k]; g_ld_upax[k] = g_ex_shown_dir[k]; }
	g_ld_have_depth = kind != 0; g_ld_depth = (float)deep; g_ld_late = (float)g_ex_late_deep;
	g_ld_speed = (float)ex_len(vpre);
	g_cyc_exact += tsc_lo() - tv0;
}

/* the landing recorder: what the viewer showed for the landing you're headed for, logged when you land (every charged
 * landing, viewer on) with what happened - so the predictions can be checked line by line from the log */
static s32 g_lr_have, g_lr_q, g_lr_sq, g_lr_seam, g_lr_wait, g_lr_streak0, g_lr_nb, g_lr_src, g_lr_xq, g_lr_edge, g_lr_seg, g_lr_t2, g_lr_early;
static float g_lr_from[3], g_lr_t2sp;
static char g_lr_xwhy[96];
static float g_lr_p[3], g_lr_sp, g_lr_fall, g_lr_peak;
static char g_lr_why[96];
static u32 g_lr_tick;
static u32 g_lr_kept;                              /* 6h.40: snapshots skipped because a landing came first (the tests) */
static void lr_snapshot(void)                        /* after each search's landing: what's shown now */
{
	s32 k;
	/* 6h.40: your 8-minute 360 log - 23 of the session's bounces were scored STALE: the search that runs in the frame of
	 * the landing (after the game's tick) predicted the NEXT fall from the new position (its path started where you'd
	 * landed - "from x100 ..." = "landed x100 ...") and replaced the prediction the landing should be scored against, before
	 * the landing check (at the start of the next frame) saw it. A landing since that check: the old prediction stays */
	if (g_sb_object && g_sb_prev_tick != 0xffffffffu && g_sb_prev_vz < -1.5f && MF(g_sb_object + 0x90) - (g_sb_prev_vz + g_sb_ld[2]) > 1.5f) { g_lr_kept++; return; }
	if (!g_land_ok || g_ld_q < 0) { g_lr_have = 0; return; }
	g_lr_have = 1; g_lr_q = g_ld_q; g_lr_seam = g_ld_seam; g_lr_sq = landing_seam_q(); g_lr_nb = g_ex_nb; g_lr_src = landing_seam_src(); g_lr_xq = g_ex_q; g_lr_edge = g_land_edge; g_lr_seg = g_land_seg; g_lr_t2 = g_ex_t2; g_lr_early = g_ex_early; g_lr_t2sp = (float)g_ex_t2sp;
	for (k = 0; k < 3; k++) g_lr_from[k] = g_pts[0][k];
	for (k = 0; k < 96; k++) { g_lr_xwhy[k] = g_ex_why[k]; if (!g_ex_why[k]) break; }
	g_lr_sp = (float)(g_ex_launch ? g_ex_sp : g_ex_late ? g_ex_late_sp : g_ex_sp);
	g_lr_fall = (float)ex_len(g_ex_vpre); g_lr_tick = g_sb_tick;
	for (k = 0; k < 3; k++) g_lr_p[k] = g_land_p[k];
	for (k = 0; k < 96; k++) { g_lr_why[k] = g_ld_why[k]; if (!g_ld_why[k]) break; }
}
static const char *q_name(s32 q) { return q == 5 ? "purple" : q == 4 ? "light blue" : q == 3 ? "amber" : q == 2 ? "green" : q == 1 ? "yellow" : q == 0 ? "red" : "unrated"; }
/* 6h.35: the recorder reads the object's position and velocity itself, out of line - your 6h.34 log had result lines with
 * a peak below the first tick's speed, which the code can't produce; its speeds now come from one read, kept for both
 * lines, and the result line repeats the first tick's speed so a mismatch shows */
static float g_lr_first;
static __attribute__((noinline)) void lr_landed(u32 object, s32 streak_before)
{
	s32 at;
	float dx, dy, dist, p[3], vx, vy, vz, sp1;
	u32 age;
	p[0] = MF(object + 0x64); p[1] = MF(object + 0x68); p[2] = MF(object + 0x6C);
	vx = MF(object + 0x88); vy = MF(object + 0x8C); vz = MF(object + 0x90);
	sp1 = fsqrt(vx * vx + vy * vy + vz * vz);
	if (!(sp1 < 10000.f)) sp1 = 0.f;                                         /* (a bad read: NaN) */
	g_lr_wait = 4; g_lr_peak = sp1; g_lr_first = sp1; g_lr_streak0 = streak_before;
	if (!g_lr_have) { at = lput(0, "landing: no prediction this fall, landed x100 "); }
	else
	{
		dx = p[0] - g_lr_p[0]; dy = p[1] - g_lr_p[1]; dist = fsqrt(dx * dx + dy * dy); age = g_sb_tick - g_lr_tick;
		if (dist > 1.f || age > 20u)
		{
			/* 6h.31: the last prediction was for another landing (a launch, a turn, a revert since) - not scored */
			at = lput(0, "landing: STALE prediction (it was "); at = lput(at, q_name(g_lr_q)); at = lput(at, " \""); at = lput(at, g_lr_why);
			at = lput(at, "\", "); at = lnum(at, age); at = lput(at, " ticks old, x100 "); at = lnum(at, (u32)f2i(dist * 100.0));
			/* 6h.37: where it pointed and from where (your last log: 10 bounces came where the prediction 1 tick before pointed
			 * 8 - 13 units away) */
			at = lput(at, " away: at x100 "); at = lsnum(at, f2i(g_lr_p[0] * 100.0)); at = lput(at, " "); at = lsnum(at, f2i(g_lr_p[1] * 100.0)); at = lput(at, " "); at = lsnum(at, f2i(g_lr_p[2] * 100.0));
			at = lput(at, " in "); at = lnum(at, (u32)g_lr_seg); at = lput(at, " ticks from x100 "); at = lsnum(at, f2i(g_lr_from[0] * 100.0)); at = lput(at, " ");
			at = lsnum(at, f2i(g_lr_from[1] * 100.0)); at = lput(at, " "); at = lsnum(at, f2i(g_lr_from[2] * 100.0)); at = lput(at, "), landed x100 ");
		}
		else
		{
			at = lput(0, "landing: shown "); at = lput(at, q_name(g_lr_q)); at = lput(at, " \""); at = lput(at, g_lr_why);
			at = lput(at, "\"");                                              /* (6i.212: the exact physics' - the only verdict there is) */
			at = lput(at, ", predicted exit x10 ");
			at = lnum(at, (u32)f2i(g_lr_sp * 10.0 + 0.5)); at = lput(at, ", "); at = lnum(at, (u32)g_lr_nb); at = lput(at, " of 4 next to it, fall x10 ");
			at = lnum(at, (u32)f2i(g_lr_fall * 10.0 + 0.5));
			at = lput(at, ", at x100 "); at = lsnum(at, f2i(g_lr_p[0] * 100.0)); at = lput(at, " "); at = lsnum(at, f2i(g_lr_p[1] * 100.0)); at = lput(at, " "); at = lsnum(at, f2i(g_lr_p[2] * 100.0));
			at = lput(at, ", seam "); at = lput(at, g_lr_seam ? q_name(g_lr_sq) : "none");
			if (g_lr_edge) at = lput(at, ", on an edge");                      /* 6h.36: a 6h.35 edge landing */
			if (g_lr_early > 0) { at = lput(at, ", first contact "); at = lnum(at, (u32)g_lr_early); at = lput(at, " early"); }   /* 6h.38 */
			if (g_lr_t2 >= 0) { at = lput(at, g_lr_t2 ? ", tick 2 contact x10 " : ", tick 2 clear x10 "); at = lnum(at, (u32)f2i(g_lr_t2sp * 10.0 + 0.5)); }   /* 6h.37 */
			at = lput(at, ", landed x100 ");
		}
	}
	at = lsnum(at, f2i(p[0] * 100.0)); at = lput(at, " "); at = lsnum(at, f2i(p[1] * 100.0)); at = lput(at, " "); at = lsnum(at, f2i(p[2] * 100.0));
	at = lput(at, ", first tick speed x10 "); at = lnum(at, (u32)f2i(sp1 * 10.0 + 0.5));
	at = lput(at, " (up x10 "); at = lsnum(at, f2i(vz * 10.0)); lput(at, ")");
	log_write();
	g_lr_have = 0;
}
static __attribute__((noinline)) void lr_tick(u32 object)   /* the ticks after: the peak and whether it counted as a bounce */
{
	float vx, vy, vz, sp;
	if (g_lr_wait <= 0) return;
	vx = MF(object + 0x88); vy = MF(object + 0x8C); vz = MF(object + 0x90);
	sp = fsqrt(vx * vx + vy * vy + vz * vz);
	if (sp < 10000.f && sp > g_lr_peak) g_lr_peak = sp;
	if (--g_lr_wait == 0)
	{
		s32 at = lput(0, "landing result: "); at = lput(at, g_sb_streak > g_lr_streak0 ? "BOUNCE" : "no bounce"); at = lput(at, ", peak speed x10 ");
		at = lnum(at, (u32)f2i(g_lr_peak * 10.0 + 0.5)); at = lput(at, " (first tick x10 "); at = lnum(at, (u32)f2i(g_lr_first * 10.0 + 0.5)); lput(at, ")");
		log_write();
	}
}

static void landing_find(u32 object)
{
	float lo[3], hi[3], cv[3], lv[3];
	s32 i, j;
	s_xf id;
	g_land_ok = 0; g_land_seg = PATH_MAX; g_land_frac = 2.f; g_land_edge = 0; g_ld_q = -1; g_arc_n = 0; g_ld_arrow = 0; g_ld_total = 0.f; g_arrow_lbl = 0;
	if (!g_sb_airborne) return;
	if (cb_ready()) sbx_ready();
	path_compute(object);
	chunks_build();
	for (j = 0; j < 3; j++) lo[j] = hi[j] = g_pts[0][j];
	for (i = 1; i < g_pts_n; i++) for (j = 0; j < 3; j++) { if (g_pts[i][j] < lo[j]) lo[j] = g_pts[i][j]; if (g_pts[i][j] > hi[j]) hi[j] = g_pts[i][j]; }
	for (j = 0; j < 3; j++) { lo[j] -= LAND_REACH; hi[j] += LAND_REACH; }       /* 6h.35: what the pill reaches beside the path */
	id.on = 0;
	if (cb_ready()) land_from(&g_cbm, &id, lo, hi, 0);
	for (i = 0; i < g_nb_n; i++) land_from(&g_nb_cb[i], &g_nb_xf[i], lo, hi, g_nb_xf[i].on == 2);
	g_arc_n = g_land_ok ? g_land_seg + 1 : g_pts_n;
	if (!g_land_ok) return;
	/* 6i.212: ONE MODEL FOR THE LANDING. Until now Cartographer's verdict was worked out here first on every search -
	 * its linear depth estimate, the drift's tilt, the seam's sideways push, its words - and then replaced by the exact
	 * physics' (ex_verdict); it lived on only in the log, as a cross-check. Your note: no second model, no frame time for
	 * one. It is gone. What is left here is what the exact verdict itself needs: the seam under the landing (its ghost-seam
	 * rule and the text's "land X off a ... seam"), and a plain state for the one case where the exact physics cannot run
	 * (its memory is not there): yellow, saying so. */
	(void)cv; (void)lv; (void)j;
	g_ld_have_depth = 0; g_ld_depth = g_ld_late = 0.f; g_ld_tilt = 0.f; g_ld_total = 0.f;
	g_ld_seam = landing_nearest_seam();
	g_ld_speed = fsqrt(dot3(g_land_v, g_land_v)); g_ld_down = g_land_v[2] < 0.f ? -g_land_v[2] : 0.f;
	g_ld_q = 1; g_ld_why = "no verdict: the exact physics could not run"; g_ld_arrow = 0; g_ld_up = 0.f; g_la_ok = 0;
	ex_verdict();                                                            /* 6h.30: the exact physics decide (above) */
	lr_snapshot();
}

/* Cartographer's sb_quality_color: amber edge, green likely, yellow lottery, red no */
static void q_colour(s32 q, float a)
{
	if (q == 4) gfx_begin(a, 0.40f, 0.80f, 1.00f);                         /* 6h.34: light blue - slow down */
	else if (q == 5) gfx_begin(a, 0.72f, 0.38f, 1.00f);                    /* 6h.34: purple - speed up */
	else if (q == 3) gfx_begin(a, 1.f, 150.f / 255.f, 40.f / 255.f);
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
		if (da < 1e-6f || db < 1e-6f) return d_seam_dq[i] == 255 ? -1 : (s32)d_seam_dq[i];   /* 6h.34: as drawn (physics colours too) */
	}
	return -1;
}
/* 6h.31: where the landing seam's colour came from (the recorder): 0 empirical, 1 exact (ground), 2 exact (this fall),
 * 3 pending, 4 not among the seams */
static s32 landing_seam_src(void)
{
	s32 i, k;
	for (i = 0; i < g_seam_n; i++)
	{
		float da = 0.f, db = 0.f;
		for (k = 0; k < 3; k++)
		{
			float a = g_seam_ab[i][k] - g_ld_sa[k], b = g_seam_ab[i][3 + k] - g_ld_sb[k], c = g_seam_ab[i][k] - g_ld_sb[k], d = g_seam_ab[i][3 + k] - g_ld_sa[k];
			da += a * a + b * b; db += c * c + d * d;
		}
		if (da < 1e-6f || db < 1e-6f) return g_seam_q[i] >= 0 && g_sb_airborne ? 2 : 0;
	}
	return 4;
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
u32 host_land_follows(void) { return g_land_follows; }
u32 host_lr_kept(void) { return g_lr_kept; }
void host_drawn_landing_c(float *p) { p[0] = d_land_c[0]; p[1] = d_land_c[1]; p[2] = d_land_c[2]; }
/* the kept seams' keys, sorted: the first n into out */
s32 host_seam_keys(float *out, s32 n) { s32 i, j, m = g_seam_n < n ? g_seam_n : n; float k[SEAM_MAX]; for (i = 0; i < g_seam_n; i++) k[i] = g_seam_d2[i]; for (i = 1; i < g_seam_n; i++) { float t = k[i]; j = i - 1; while (j >= 0 && k[j] > t) { k[j + 1] = k[j]; j--; } k[j + 1] = t; } for (i = 0; i < m; i++) out[i] = k[i]; return g_seam_n; }
u32 host_count_hud(void) { return g_count_hud; }
void host_put_seam(s32 i, const float *a, const float *b, s32 q)          /* a finished seam: the search's and the drawn copy */
{
	s32 k;
	for (k = 0; k < 3; k++) { g_seam_ab[i][k] = a[k]; g_seam_ab[i][3 + k] = b[k]; d_seam_ab[i][k] = a[k]; d_seam_ab[i][3 + k] = b[k]; }
	g_seam_q[i] = (s8)q; d_seam_q[i] = (s8)q; d_seam_dq[i] = (u8)(q < 0 ? 255 : q); g_seam_ph[i] = 0;
	if (g_seam_n <= i) g_seam_n = i + 1;
	if (d_seam_n <= i) d_seam_n = i + 1;
}
#endif

/* sb_draw_landing's drawing: the path, the landing seam highlighted (4 px) in its own rating's colour - the verdict
 * shows on the circle and the arrow only - a white line to it, the circle */
/* 6h.37: between searches the trajectory is recomputed every frame, but the landing circle stayed where the last search
 * put it - redrawn only every 3-4 frames (searches) and one more step behind on a slow console: the circle dragged behind
 * the line. Now every frame the new path is crossed with the drawn landing's own polygon (one polygon - cheap): where it
 * comes down on it (or, for an edge landing, within the pill's reach of its edge) the circle and the path's end move
 * there. If the path no longer comes down on that polygon, the circle waits for the next search. (The verdict - colour,
 * words, arrow - is still the search's.) */
/* ---- 6i.224: THE CIRCLE STICKING AT SEAMS (your note: it "gets stuck to seams and doesn't follow the landing path
 * line 1:1 ... hard to aim when there are a lot of seams around") ----
 * Between searches the circle followed the path only while it came down INSIDE the polygon the last search had found.
 * The moment the path crossed that polygon's edge - a seam - the circle stopped there and waited for the next search to
 * find the polygon on the other side. On the 360 a search runs every 4 frames (your logs: "a search every 4 frames (it
 * costs ~15 ms a search)"), so at every seam it hung for up to four frames and then jumped. With many seams about,
 * that is most of the time.
 * Now it follows the path over the edge too: up to LAND_OVER (0.75 of a unit) beyond the polygon, on that polygon's
 * own plane, and - as before - never more than 1 unit from where the last search put it. Across a seam between two
 * faces at a few degrees that is within a few hundredths of a unit of the real floor; the next search puts it exactly
 * right. What it costs: where the floor really ends at that edge (a ledge), the circle can run on past it, level, for
 * up to those four frames before the search corrects it. The verdict - colour, words, arrow - is the search's, as
 * ever. ---- */
#define LAND_OVER 0.75f
static s32 follow_ok(float (*pv)[3], s32 k, const float *wn, const float *hit)
{
	float best = 1e9f, ep[3];
	s32 m;
	if (poly_inside(pv, k, wn, hit)) return 1;
	for (m = 0; m < k; m++)
	{
		float d2;
		closest_on(hit, pv[m], pv[(m + 1) % k], ep);
		d2 = (ep[0] - hit[0]) * (ep[0] - hit[0]) + (ep[1] - hit[1]) * (ep[1] - hit[1]) + (ep[2] - hit[2]) * (ep[2] - hit[2]);
		if (d2 < best) best = d2;
	}
	return best <= LAND_OVER * LAND_OVER;
}
#ifdef HOST
s32 host_follow_ok(float (*pv)[3], s32 k, const float *wn, const float *hit) { return follow_ok(pv, k, wn, hit); }
#endif
static void land_follow(void)
{
	float pv[16][3], wn[3];
	s32 k, i, j;
	if (!d_land_ok || g_pts_n < 2) return;
	k = surf_poly(&d_land_cb, &d_land_xf, d_land_s, pv, 0, wn);
	if (!k) return;
	for (i = 0; i + 1 < g_pts_n; i++)
	{
		float *p = g_pts[i], *q = g_pts[i + 1], t, hit[3];
		float d0 = (p[0] - pv[0][0]) * wn[0] + (p[1] - pv[0][1]) * wn[1] + (p[2] - pv[0][2]) * wn[2];
		float d1 = (q[0] - pv[0][0]) * wn[0] + (q[1] - pv[0][1]) * wn[1] + (q[2] - pv[0][2]) * wn[2];
		if (!(d0 > 0.f && d1 <= 0.f)) continue;
		t = d0 / (d0 - d1);
		for (j = 0; j < 3; j++) hit[j] = p[j] + (q[j] - p[j]) * t;
		if (!follow_ok(pv, k, wn, hit)) continue;                            /* 6i.224: inside the polygon, or just over its edge */
		/* 6h.38: only a small move from where the search put it (6h.37 followed the path to any crossing with that polygon -
		 * on a big floor that could be far off, the circle "stuck to my left away from where I was about to land") */
		{
			float mx = hit[0] - d_land_c[0], my = hit[1] - d_land_c[1];
			if (mx * mx + my * my > 1.0f) return;
		}
		for (j = 0; j < 3; j++) d_land_p[j] = hit[j];
		g_arc_n = i + 1;
		g_land_follows++;
		return;
	}
}
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
	if (SPOOKY_LOG && !g_objlog_off) { g_objlog_h = best_h; if (guarded_call(objects_log_one)) { g_objlog_off = 1; sc_say("the object pair log faulted - off for this session"); } }
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
				at = lput(0, "ghost offset: object agent "); at = lhex(at, agent); at = lput(at, ", TIM x1000 "); at = lsnum(at, fti(o->tim * 1000.f));
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
			at = lput(at, ", gap x1000 "); at = lsnum(at, (s32)(o->gap * 1000.f)); at = lput(at, ", TIM x1000 "); at = lsnum(at, fti(o->tim * 1000.f));
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
static s32 g_ofs_n, g_ofs_valid, g_ofs_req, g_ofs_phase, g_ofs_load, g_ofs_frames, g_ofs_restored, g_ofs_pass;   /* 6i: frames: 30 Hz steps; pass: frames of a restore */   /* phase: 0 idle, 1 wait, 2 apply */
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
	g_ofs_phase = 2; g_ofs_load = load; g_ofs_frames = 0; g_ofs_restored = 0; g_ofs_pass = 0;
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
		g_ofs_frames += g_steps;
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
		g_ofs_frames += g_steps; g_ofs_pass++;
		if (g_ofs_load && g_ofs_pass == 1 && d2 > 0.05f * 0.05f)
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
typedef struct { float a[3], b[3], near[3], drop; s_rd r; s32 q, gt, gh, pieces; u32 seen, piece_seen[24]; s32 ph; float ph_lo, ph_hi, ph_sp, ph_cur; } s_shown;   /* 6h.34: ph* - the physics check */
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

#ifdef HOST
void host_set_wire(s32 v) { o_wire = v; }
#endif
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
	if (times >= 6 || (times && g_t30 - last < 300)) return;
	g_probe_last = g_t30; g_probe_times = times + 1;
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
				else { at = lput(at, " z x100 "); at = lsnum(at, fti(na[2] * 100.f)); at = lput(at, "/"); at = lsnum(at, fti(nb[2] * 100.f)); at = lput(at, " dot x100 "); at = lsnum(at, fti(dot3(na, nb) * 100.f)); }
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
	static const float qcol[6][4] = { { 0.55f, 1.f, 0.31f, 0.27f }, { 0.95f, 1.f, 0.82f, 0.20f }, { 0.95f, 0.31f, 0.90f, 0.35f }, { 0.95f, 1.f, 0.59f, 0.16f },
		{ 0.95f, 0.40f, 0.80f, 1.00f }, { 0.95f, 0.72f, 0.38f, 1.00f } };   /* sb_quality_color (red dimmed); 6h.34: light blue (slow down), purple (speed up) */
	s32 k, i, any;
	for (k = 0; k < 4; k++)                                                   /* unrated: by bend, dimmed */
	{
		any = 0;
		for (i = 0; i < d_seam_n; i++) if (d_seam_kind[i] == k && d_seam_q[i] < 0 && !d_seam_pend[i] && d_seam_vis[i] && d_seam_los[i]) { any = 1; break; }
		if (!any) continue;
		gfx_begin(0.45f, col[k][0], col[k][1], col[k][2]);
		for (i = 0; i < d_seam_n; i++)
			if (d_seam_kind[i] == k && d_seam_q[i] < 0 && !d_seam_pend[i] && d_seam_vis[i] && d_seam_los[i]) seam_line(i);
		gfx_end();
	}
	/* 6h.31: pending - below you and reachable, not rated exactly yet: grey */
	for (any = 0, i = 0; i < d_seam_n; i++) if (d_seam_pend[i] && d_seam_los[i]) { any = 1; break; }
	if (any)
	{
		gfx_begin(0.5f, 0.6f, 0.6f, 0.6f);
		for (i = 0; i < d_seam_n; i++) if (d_seam_pend[i] && d_seam_los[i]) seam_line(i);
		gfx_end();
	}
	for (k = 0; k < 6; k++)
	{
		any = 0;
		for (i = 0; i < d_seam_n; i++) if (d_seam_dq[i] == k && d_seam_los[i]) { any = 1; break; }
		if (!any) continue;
		gfx_begin(qcol[k][0], qcol[k][1], qcol[k][2], qcol[k][3]);
		for (i = 0; i < d_seam_n; i++)
			if (d_seam_dq[i] == k && d_seam_los[i]) seam_line(i);
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
		e->ph = g_seam_ph[i]; e->ph_lo = g_seam_ph_lo[i]; e->ph_hi = g_seam_ph_hi[i]; e->ph_sp = g_seam_ph_sp[i]; e->ph_cur = g_seam_ph_cur[i];
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
		g_seam_vis[i] = g_seam_q[i] >= 0;                                     /* 6h.33: only seams with a colour are drawn */
		continue;
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
	if (r->exact)                                                             /* 6h.30: the exact physics' words */
	{
		at = put(at, opt);
		if (e->q == 2) { at = put(at, " LIKELY: "); at = put_f(at, r->ex_sp, 1); at = put(at, " u/s"); at = put(at, ex_class((double)r->ex_sp)); at = put(at, " ("); at = put_int(at, r->ex_n); put(at, " of 4 approaches launch)"); }
		else if (e->q == 3 && e->gt > 0) { at = put(at, " GHOST SEAM: "); at = put_int(at, e->gh); at = put(at, " of "); at = put_int(at, e->gt); put(at, " lottery landings here launched"); }
		else if (e->q == 3) { at = put(at, " EDGE: "); at = put_f(at, r->ex_sp, 1); at = put(at, " u/s"); at = put(at, ex_class((double)r->ex_sp)); put(at, " (1 of 4 approaches launches)"); }
		else if (e->q == 1) { at = put(at, " LOTTERY: a tick late it launches "); at = put_f(at, r->ex_sp, 1); at = put(at, " u/s"); put(at, ex_class((double)r->ex_sp)); }
		else put(at, " NO LAUNCH (walking off or jumping onto it, at a run or a walk)");
		return;
	}
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
static const color4f c_stored = { 1.0f, 0.35f, 0.85f, 1.0f };      /* 6i.120: a stored velocity - sky blue */
static const color4f c_land[4] = { { 1.0f, 1.f, 0.31f, 0.27f }, { 1.0f, 1.f, 0.82f, 0.20f }, { 1.0f, 0.31f, 0.90f, 0.35f }, { 1.0f, 1.f, 0.59f, 0.16f } };   /* red, yellow, green, amber */
static const color4f c_streak = { 1.0f, 0.47f, 1.0f, 0.55f };       /* green while a chain runs */
static const color4f c_record = { 1.0f, 1.0f, 0.82f, 0.35f };       /* gold */

static u16 g_text[160];
static s32 put_hex(s32 at, u32 v) { s32 k; for (k = 28; k >= 0 && at < 159; k -= 4) g_text[at++] = (u16)"0123456789ABCDEF"[(v >> k) & 15u]; g_text[at] = 0; return at; }   /* 6i.149 (test) */

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
	d_land_cb = g_land_cb; d_land_xf = g_land_xf; d_land_s = g_land_s; d_land_edge = g_land_edge;   /* 6h.37 */
	for (k = 0; k < 3; k++) d_land_c[k] = g_land_p[k];
	for (k = 0; k < 3; k++) { d_land_p[k] = g_land_p[k]; d_ld_sa[k] = g_ld_sa[k]; d_ld_sb[k] = g_ld_sb[k]; d_ld_dir[k] = g_ld_dir[k]; d_ld_upax[k] = g_ld_upax[k]; }
	d_seam_n = g_seam_n;
	for (i = 0; i < g_seam_n; i++)
	{
		for (k = 0; k < 6; k++) d_seam_ab[i][k] = g_seam_ab[i][k];
		d_seam_kind[i] = g_seam_kind[i]; d_seam_q[i] = g_seam_q[i]; d_seam_vis[i] = g_seam_vis[i]; d_seam_los[i] = g_seam_los[i]; d_seam_np[i] = g_seam_np[i]; d_seam_pend[i] = g_seam_pend[i];
		d_seam_dq[i] = (u8)(g_seam_q[i] < 0 ? 255 : g_seam_ph[i] == 1 ? 2 : g_seam_ph[i] == 2 ? 4 : g_seam_ph[i] == 3 ? 5 : g_seam_ph[i] == 4 ? 0 : g_seam_q[i]);   /* 6i.127: none launches: red */
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
static s_isl g_isl_own;                         /* 6i.48: your island only (the two nearest other islands' lines gone) */
static s32 g_sleep_state;                        /* 0 nothing; 1 shown; 2 deactivation off in this world */
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
	o->cycle = (fti(pa / delta) + 1) * 6;                               /* six checks from rounds 0: the bar's length */
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
	u32 world = M32(ADDR_HAVOK_WORLD_PTR), own = 0;
	float delta;
	s32 i, n;
	if (!o_sleep || !g_sb_object || !ptr_ok(world)) return;
	if (!M8(world + 0x23D)) { g_sleep_state = 2; return; }                /* deactivation off: nothing ever sleeps */
	delta = MF(world + 0x150);
	if (!(delta > 0.001f && delta < 0.5f)) delta = 1.f / 30.f;
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
	if (own && isl_count(own, delta, &g_isl_own)) { g_isl_own.dist = 0.f; g_sleep_state = 1; }
	else g_isl_own.island = 0;
}
static void sleep_update(void)
{
	g_sleep_state = 0; g_isl_own.island = 0;
	if (!o_sleep || !g_sb_object) return;
	if (guarded_call(sleep_update_body)) { g_sleep_state = 0; g_isl_own.island = 0; }   /* a fault: nothing shown */
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
	s32 at, ly;
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
		if (g_isl_own.awake) put(0, "ASLEEP");                             /* 6i.48: was "SLEEP: kept awake - " and why */
		else
		{
			if (t >= 0) { at = put(0, "SLEEP in "); at = put_int(at, t); at = put(at, t == 1 ? " tick if still - quiet rounds " : " ticks if still - quiet rounds "); }
			else at = put(0, "SLEEP: not within 80 s - quiet rounds ");
			at = put_int(at, g_isl_own.rounds); put(at, "/5");
		}
		line_at(70, ly, 500, &c, 2);
	}
	(void)height;
}
#ifdef HOST
s32 host_sleep(s32 *to_sleep, s32 *to_a, s32 *to_b, s32 *rounds) { *to_sleep = g_isl_own.to_sleep; *to_a = g_isl_own.to_a; *to_b = g_isl_own.to_b; *rounds = g_isl_own.rounds; return g_sleep_state; }
s32 host_sleep_awake(void) { return g_isl_own.awake; }
static u32 g_host_veto;
void host_set_veto(s32 v) { g_host_veto = (u32)v; }
u32 game_call_eax(u32 fn, u32 a) { (void)fn; (void)a; return g_host_veto; }
void host_set_sleep(s32 on) { o_sleep = on; }
#endif

/* ---- 6i.160: a level (re)started with the free camera on - the pad was still the camera's, so the opening cutscene
 * could not be skipped and you had no input. A level's first tick (the game's tick count 0: a new level, Restart Level)
 * turns the free camera off: back in first person, the pad the game's ---- */
static void fc_level_reset(void)
{
	u32 time = M32(ADDR_GAME_TIME_PTR);
	if (!o_free_cam || !ptr_ok(time) || M32(time + 8) != 0) return;
	o_free_cam = 0;
	toast("Free camera: off (the level started)");
}
#ifdef HOST
void host_fc_level_reset(void) { fc_level_reset(); }
#endif

/* ---- 6i.158: Object Spawning - the level's side (the list and its menus are with g_menus).
 * WHAT A LEVEL HOLDS: the loaded map's tag table is in memory (the instances at *0x4e3b44, 16 bytes each: the tag's group,
 * its index, its data; their count in the tags header *0x547f00 +0x18, the scenario's index +0xC) - but not the tags'
 * names: those are in the map file (d:\maps\<map>.map), one string a tag, in the table's order. The header the game keeps
 * in memory (0x547700, 0x800 bytes; the scenario's path at +0x1BC is ADDR_MAP_NAME) says where: four dwords - the names'
 * count (= the tag count), the strings' offset, their size, their index's offset - expected at +0x2BC, looked for
 * anywhere in the header if not there. The strings are read through once (32 KB at a time); a name whose tag is a weapon,
 * equipment, vehicle, biped or crate is compared with the list. Taken as right only if the scenario's own tag reads back
 * as the scenario's path; otherwise nothing is listed and the menu says so. Done once a level, when the menu is opened.
 * SPAWNING: the game's own two steps (what a unit dropping its weapon uses) - the creation record filled from the tag
 * (v1.0 0xb7930: the 0xC4-byte record in esi; the tag, the owner object (none) and 0 on the stack, ret 0xC), then the
 * object made from it (0xb7b40, stdcall (record), the object's index or -1) - hooks.S os_place_call / os_new_call. In
 * between, the record's position (+0x1C), forward (+0x28) and up (+0x34) are set: in front of your unit (its origin
 * +0x64, along its facing +0x70 flattened), facing as you do. Run from the tick hook, between two ticks - where the
 * game itself makes objects - never from the menu's drawing or the pad's hook.
 * 6i.159 - A BRAIN FOR A DROPPED BIPED: what the game's own squad spawn (v1.0 0x1e0850) does after making a unit, with
 * no squad. A unit's AI is an actor made from a CHARACTER tag ('char': its parent +0x08, the unit it drives +0x10, its
 * general settings +0x34 - their first byte's bit 0: a swarm - and its weapons +0xCC, 0xCC bytes each, the weapon's
 * tag at +8; a block not filled in is its parent's). When the level is read, each biped of the list is given a
 * character of this level whose unit it is - one with no parent (the plain rank) before one that has one (a major, an
 * ultra...). After the biped is made: function_1e04a0 (eax the weapon's tag; character, unit, a second weapon (none),
 * a grenade type (none) on the stack, ret 0x10) sets its health and shield from the character and puts the
 * character's first weapon in its hands; function_1e0160 (eax the squad - none; edx the character; unit, 0 on the
 * stack, ret 8) makes the actor - with no squad its team is the unit's, else its kind's own (0x44aee8), as for the
 * turret gunners the game makes the same way (0x1bbdf0); function_1e11b0 (eax the actor, cl 1) switches it on, as
 * there. Not for a swarm character (those are made another way), nor where AI is not running (*0x4f55d0 +1: off in
 * multiplayer), nor for a biped no character of this level drives: those arrive as bodies, and the message says why. ---- */
static u32 g_os_char[OS_DEFS];                   /* 6i.159: each biped's character in this level (0xFFFFFFFF: none) */
static u8 g_os_char_base[OS_DEFS];               /* ... and whether that one has no parent */
static s32 g_os_state;                           /* 0 not read for a level; 1 read; -1 the names couldn't be read */
static char g_os_map[0x100];                     /* ... for this scenario path */
static u32 g_os_scn;                             /* ... and this scenario tag */
static u32 g_os_tag[OS_DEFS];                    /* each list entry's tag in this level (0xFFFFFFFF: not in it) */
static s32 g_os_found;
/* ---- 6i.177 / 6i.178: the WARTHOG'S VARIANTS, and a COUNT of the level's scenery and crates ----
 * 6i.178: 6i.177's "Level Objects" heading is taken out again (you want no new heading - those objects in the existing
 * "Objects" list - and not before it is known how many there are). Its list, names, sorting and ten-row jump are gone
 * from the build (they are in 6i.177's packaged source). What stays is the COUNT: every scenery ('scen') and crate
 * ('bloc') tag the level has loaded is counted in the pass through the map file's tag names that fills the existing
 * lists, and the log says how many - nothing is listed or spawnable from it.
 * The Warthog's variants: the game arms and dresses an object by its MODEL VARIANT - the placement's name at +0xC
 * (none: the object tag's own default, +0x30), looked up in the model tag's variants (the object's model at +0x38; the
 * model's variants +0x50 count / +0x54 address, 0x38 bytes each, the name's string id first) - the game's
 * function_ba540 / function_16d180. So every variant of the Warthog's model other than its default is listed beside
 * "Warthog" in Human Vehicles and spawned by putting its name in the placement. The entry's label is the variant's
 * own name, read from the map file's string table (found in the header as the tag names are: a count, a size and two
 * offsets that check out; a name is trusted only if its length is the one the string id itself carries) - "Warthog
 * (Gauss)" if the name has "gauss" in it, else "Warthog (<name>)"; if the names cannot be read, "Warthog (variant
 * N)". NOT KNOWN until it runs on a console: that the Gauss Warthog IS a model variant of the one Warthog tag. If
 * the model has a single variant, nothing is added and the log says so. */
static u32 g_os_var_id[OS_VAR_MAX]; static char g_os_var_label[OS_VAR_MAX][44]; static s32 g_os_var_n, g_os_var_def = -1, g_os_var_seen;
static u32 g_os_str_n, g_os_str_io, g_os_str_bo, g_os_str_size;   /* the map file's string table (0: not found) */
#ifdef HOST
static u32 g_host_os_variant;
void host_os_rebuild(void);
u32 host_os_variant(void) { return g_host_os_variant; }
#endif
#ifdef HOST
__attribute__((weak)) u32 host_os_read(u32 offset, u8 *to, u32 bytes) { (void)offset; (void)to; (void)bytes; return 0; }   /* the test's map file */
static u8 g_host_os_buf[0x8000];
static u32 g_host_os_place_tag, g_host_os_new_n, g_host_os_result = 0xE1230007u; static float g_host_os_rec[9];
static void os_place_call(void *rec, u32 tag) { memset(rec, 0, 0xC4); *(u32 *)rec = tag; g_host_os_place_tag = tag; }
static u32 os_new_call(void *rec) { memcpy(&g_host_os_variant, (u8 *)rec + 0xC, 4); memcpy(g_host_os_rec, (u8 *)rec + 0x1C, 12); memcpy(g_host_os_rec + 3, (u8 *)rec + 0x28, 24); g_host_os_new_n++; return g_host_os_result; }
static u32 g_host_os_ai[8], g_host_os_ai_n, g_host_os_actor = 0xAC700003u;   /* the three AI calls' arguments, in order; how many were made */
static u32 g_host_os_sq[4];                      /* 6i.163: the second weapon, the squad and its flag as passed; the squad refreshes made */
static void os_equip_call(u32 weapon, u32 character, u32 unit, u32 weapon2) { g_host_os_ai[0] = weapon; g_host_os_ai[1] = character; g_host_os_ai[2] = unit; g_host_os_sq[0] = weapon2; g_host_os_ai[7] = g_host_os_ai_n++; }
static u32 os_actor_call(u32 squad, u32 character, u32 unit, u32 flag) { g_host_os_sq[1] = squad; g_host_os_sq[2] = flag; g_host_os_ai[3] = character; g_host_os_ai[4] = unit; g_host_os_ai_n++; return g_host_os_actor; }
static void os_actor_on_call(u32 actor) { g_host_os_ai[5] = actor; g_host_os_ai_n++; }
static void os_squads_refresh(void) { g_host_os_sq[3]++; }
#else
void os_equip_call(u32 weapon, u32 character, u32 unit, u32 weapon2);   /* hooks.S: function_1e04a0 */
u32 os_actor_call(u32 squad, u32 character, u32 unit, u32 flag);   /* hooks.S: function_1e0160 */
static void os_squads_refresh(void) { if (ADDR_SQUADS_REFRESH) gfx_call0(ADDR_SQUADS_REFRESH); }   /* function_201df0, as ai_attach calls it after */
void os_actor_on_call(u32 actor);                /* hooks.S: function_1e11b0, on */
void os_place_call(void *rec, u32 tag);          /* hooks.S: function_b7930 */
u32 os_new_call(void *rec);                      /* hooks.S: function_b7b40 */
static u8 *g_os_buf;                             /* run-time memory, 32 KB, kept */
static void *g_os_file;
#endif
static s32 os_open(void)
{
#ifdef HOST
	return 1;
#else
	char path[72], map[48];
	s32 i, k;
	if (!ADDR_SETFILEPOINTER) return 0;
	if (!g_os_buf) { g_os_buf = (u8 *)VirtualAlloc_x(0, 0x8000, 0x3000, 4); if (!g_os_buf) return 0; }
	current_map(map);
	{ const char *a = "d:\\maps\\"; for (k = 0; a[k]; k++) path[k] = a[k]; for (i = 0; map[i] && k < 60; i++) path[k++] = map[i]; path[k++] = '.'; path[k++] = 'm'; path[k++] = 'a'; path[k++] = 'p'; path[k] = 0; }
	g_os_file = CreateFileA_x(path, 0x80000000, 1, 0, 3, 0x80, 0);
	return g_os_file != (void *)-1;
#endif
}
static u32 os_read(u32 offset, u8 **at, u32 bytes)   /* up to 32 KB from the map file: the bytes read */
{
#ifdef HOST
	*at = g_host_os_buf;
	return host_os_read(offset, g_host_os_buf, bytes > 0x8000u ? 0x8000u : bytes);
#else
	u32 got = 0;
	*at = g_os_buf;
	if (bytes > 0x8000u) bytes = 0x8000u;
	if (SetFilePointer_x(g_os_file, (s32)offset, 0, 0) == 0xFFFFFFFFu || !ReadFile_x(g_os_file, g_os_buf, bytes, &got, 0)) return 0;
	return got;
#endif
}
static void os_close(void)
{
#ifndef HOST
	CloseHandle_x(g_os_file);
#endif
}
/* the names read through from the strings' offset: 1 if the scenario's tag read back as the scenario's path (g_os_tag
 * then holds what the level has of the list) */
static s32 os_names_walk(u32 off, u32 size, u32 count, u32 inst, u32 scn)
{
	char name[160];
	u32 done = 0, i = 0, n = 0, k, got;
	s32 d, scn_ok = 0, over = 0;
	u8 *b;
	for (d = 0; d < OS_DEFS; d++) g_os_tag[d] = 0xFFFFFFFFu;
	g_os_found = 0;
	while (done < size && i < count)
	{
		got = os_read(off + done, &b, size - done);
		if (!got) return 0;
		for (k = 0; k < got && i < count; k++)
		{
			u8 c = b[k];
			if (c) { if (n < sizeof(name) - 1) name[n++] = (char)c; else over = 1; continue; }
			name[n] = 0;
			if (!over)
			{
				u32 group = M32(inst + i * 16u);
				if (i == (scn & 0xFFFFu)) scn_ok = str_eq(name, g_os_map);
				if (group == OSG_WEAP || group == OSG_EQIP || group == OSG_VEHI || group == OSG_BIPD || group == OSG_BLOC)
					for (d = 0; d < OS_DEFS; d++)
						if (os_defs[d].group == group && g_os_tag[d] == 0xFFFFFFFFu && str_eq(name, os_defs[d].path)) { g_os_tag[d] = M32(inst + i * 16u + 4); g_os_found++; break; }
			}
			n = 0; over = 0; i++;
		}
		done += got;
	}
	return scn_ok && i == count;
}
static void os_menus_fill(s32 ok)                /* the menus' lists from g_os_tag (ok 0: the names couldn't be read) */
{
	static const s_item none = OS_NONE, unread = { "This level's tag names could not be read", K_BACK, 0, 0, 0 };
	s32 n[OSC_N], c, d, top = 0, bip = 0, m;
	for (c = 0; c < OSC_N; c++) n[c] = 0;
	for (d = 0; d < OS_DEFS; d++)
		if (ok && g_os_tag[d] != 0xFFFFFFFFu && n[os_defs[d].cat] < OS_PER)
		{
			s_item *it = &g_os_items[os_defs[d].cat][n[os_defs[d].cat]++];
			it->label = os_defs[d].label; it->kind = K_STAY; it->value = 0; it->lo = 100 + d; it->hi = 0;
		}
	for (c = 0; c < OSC_N; c++)
	{
		s_item sub;
		sub.label = os_cat_title[c]; sub.kind = K_SUB; sub.value = 0; sub.lo = os_cat_menu[c]; sub.hi = 0;
		if (!n[c]) g_os_items[c][0] = none;
		g_menus[os_cat_menu[c]].n = n[c] ? n[c] : 1;
		if (!n[c]) continue;
		if (c >= OSC_BH && c <= OSC_BF)
		{
			if (!bip)
			{
				s_item b, ai;
				b.label = "Bipeds  >"; b.kind = K_SUB; b.value = 0; b.lo = OS_MENU_BIPEDS; b.hi = 0; g_os_top[top++] = b;
				ai.label = "Spawn with AI"; ai.kind = K_TOGGLE; ai.value = &o_os_ai; ai.lo = 0; ai.hi = 1; g_os_bip[bip++] = ai;
				ai.label = "Spawn as Ally"; ai.value = &o_os_ally; g_os_bip[bip++] = ai;   /* 6i.170 */
				ai.label = "Spawn as Enemy"; ai.value = &o_os_enemy; g_os_bip[bip++] = ai;   /* 6i.171 */   /* 6i.162: the Bipeds menu's first item */
			}
			g_os_bip[bip++] = sub;
		}
		else g_os_top[top++] = sub;
	}
	/* 6i.177: the Warthog's other variants, after it in Human Vehicles */
	if (ok && g_os_var_def >= 0 && g_os_tag[g_os_var_def] != 0xFFFFFFFFu)
	{
		s32 hv = os_defs[g_os_var_def].cat, k, at;
		for (at = 0; at < n[hv] && g_os_items[hv][at].lo != 100 + g_os_var_def; at++) ;
		for (k = 0; k < g_os_var_n && n[hv] < OS_PER && at < n[hv]; k++)
		{
			s32 q;
			for (q = n[hv]; q > at + 1 + k; q--) g_os_items[hv][q] = g_os_items[hv][q - 1];
			g_os_items[hv][at + 1 + k].label = g_os_var_label[k]; g_os_items[hv][at + 1 + k].kind = K_STAY; g_os_items[hv][at + 1 + k].value = 0;
			g_os_items[hv][at + 1 + k].lo = 2000 + k; g_os_items[hv][at + 1 + k].hi = 0;
			n[hv]++;
		}
		g_menus[os_cat_menu[hv]].n = n[hv] ? n[hv] : 1;
	}
	if (!top) g_os_top[0] = ok ? none : unread;
	if (!bip) g_os_bip[0] = none;
	g_menus[OS_MENU].n = top ? top : 1;
	g_menus[OS_MENU_BIPEDS].n = bip ? bip : 1;
	for (m = OS_MENU; m <= OS_MENU_LAST; m++) { g_sel[m] = 0; g_first[m] = 0; }
}
#define OSG_CHAR 0x63686172u
static void os_chars(u32 inst, u32 count)        /* 6i.159: each listed biped's character in this level */
{
	u32 i;
	s32 d;
	for (i = 0; i < count; i++)
	{
		u32 def, unit, base;
		if (M32(inst + i * 16u) != OSG_CHAR) continue;
		def = M32(inst + i * 16u + 8);
		if (!ptr_ok(def)) continue;
		unit = M32(def + 0x10); base = M32(def + 8) == 0xFFFFFFFFu;
		if (unit == 0xFFFFFFFFu) continue;
		for (d = 0; d < OS_DEFS; d++)
			if (os_defs[d].group == OSG_BIPD && g_os_tag[d] == unit && (g_os_char[d] == 0xFFFFFFFFu || (base && !g_os_char_base[d])))
			{ g_os_char[d] = M32(inst + i * 16u + 4); g_os_char_base[d] = (u8)base; }
	}
}
/* a character's tag data, by its tag; a block of it (count at off, address at off + 4) - its own or the nearest parent's
 * that has one (as the game reads them, 0x1e4990 / 0x1e5280): its address, 0 if none */
static u32 os_char_def(u32 character)
{
	u32 inst = M32(ADDR_TAG_INSTANCES), th = M32(ADDR_CACHE_TAGS_PTR), def;
	if (character == 0xFFFFFFFFu || !ptr_ok(inst) || !ptr_ok(th) || (character & 0xFFFFu) >= M32(th + 0x18)) return 0;
	if (M32(inst + (character & 0xFFFFu) * 16u + 4) != character || M32(inst + (character & 0xFFFFu) * 16u) != OSG_CHAR) return 0;
	def = M32(inst + (character & 0xFFFFu) * 16u + 8);
	return ptr_ok(def) ? def : 0;
}
static u32 os_char_block(u32 character, u32 off)
{
	s32 depth;
	for (depth = 0; depth < 8; depth++)                                    /* (a chain longer than the game has: not followed for ever) */
	{
		u32 def = os_char_def(character);
		if (!def) return 0;
		if ((s32)M32(def + off) > 0 && ptr_ok(M32(def + off + 4))) return M32(def + off + 4);
		character = M32(def + 8);
	}
	return 0;
}
/* 6i.163: your 6i.162 test - a Bugger came with a battle rifle, and none of them moved. The character tag's weapon
 * list is not what a unit is armed with (it is the character's settings for each weapon it may hold), and an actor in no
 * squad has no orders and no firing positions to move between. Both come from the level's SQUADS (the scenario,
 * *0x4e0350: squads +0x160 count / +0x164, 0x74 bytes each - flags +0x20, team +0x24, character +0x36 (an index into
 * the character palette, +0x178 / +0x17C, 8 bytes each, the tag at +4), first and second weapon +0x3C / +0x3E (indices
 * into the weapon palette, +0x98 / +0x9C, 0x28 bytes each, the tag at +4), starting locations +0x48 / +0x4C - a
 * location: position +4, reference frame +0x10, its own character +0x20 and weapons +0x22 / +0x24, each -1 for the
 * squad's). So the dropped biped JOINS A SQUAD of this level whose character drives that biped - the one whose first
 * starting location is nearest to where it was dropped - exactly as the script's ai_attach does (v1.0 0x273040:
 * function_1e0160 with the squad, then function_201df0): it takes that squad's team and orders, and is armed with that
 * squad's weapons. A biped no squad of this level uses keeps 6i.159's way (an actor in no squad, of the character
 * found in the tag table): it stands where it is, unarmed - said in the message. */
typedef struct { s32 squad; u32 character, weapon, weapon2, flag; } s_os_squad;
static s32 os_squad_find(u32 biped_tag, const float *at, s_os_squad *out, u32 prefer)
{
	u32 scn = ADDR_SCENARIO ? M32(ADDR_SCENARIO) : 0, squads, pal, wpal;
	s32 n, np, nw, i, found = 0, best_rank = -1;
	float best = 0.f;
	if (!ptr_ok(scn)) return 0;
	n = (s32)M32(scn + 0x160); squads = M32(scn + 0x164); np = (s32)M32(scn + 0x178); pal = M32(scn + 0x17C); nw = (s32)M32(scn + 0x98); wpal = M32(scn + 0x9C);
	if (n <= 0 || n > 1024 || !ptr_ok(squads) || np <= 0 || !ptr_ok(pal)) return 0;
	for (i = 0; i < n; i++)
	{
		u32 sq = squads + (u32)i * 0x74u, loc = 0, def, ch;
		s32 e = (s32)(s16)M16(sq + 0x36), w1 = (s32)(s16)M16(sq + 0x3C), w2 = (s32)(s16)M16(sq + 0x3E), k;
		float d2 = 1.0e30f;
		if ((s32)M32(sq + 0x48) > 0 && ptr_ok(M32(sq + 0x4C))) loc = M32(sq + 0x4C);
		if (loc)
		{
			if ((s16)M16(loc + 0x20) != -1) e = (s32)(s16)M16(loc + 0x20);
			if ((s16)M16(loc + 0x22) != -1) w1 = (s32)(s16)M16(loc + 0x22);
			if ((s16)M16(loc + 0x24) != -1) w2 = (s32)(s16)M16(loc + 0x24);
			if ((s16)M16(loc + 0x10) == -1) { d2 = 0.f; for (k = 0; k < 3; k++) { float d = MF(loc + 4 + 4 * (u32)k) - at[k]; d2 += d * d; } }
		}
		if (e < 0 || e >= np) continue;
		ch = M32(pal + (u32)e * 8u + 4);
		def = os_char_def(ch);
		if (!def || M32(def + 0x10) != biped_tag) continue;
		{
			/* 6i.170: an armed squad before an unarmed one, then one of the biped's own base character (a Jackal's, not
			 * its sniper's) before another's, then the nearest */
			u32 wt = (w1 >= 0 && w1 < nw && ptr_ok(wpal)) ? M32(wpal + (u32)w1 * 0x28u + 4) : 0xFFFFFFFFu;
			s32 rank = (wt != 0xFFFFFFFFu ? 2 : 0) + (ch == prefer ? 1 : 0);
			if (found && (rank < best_rank || (rank == best_rank && !(d2 < best)))) continue;
			found = 1; best = d2; best_rank = rank;
			out->squad = i; out->character = ch; out->flag = (M32(sq + 0x20) >> 10) & 1u;
			out->weapon = wt;
			out->weapon2 = (w2 >= 0 && w2 < nw && ptr_ok(wpal)) ? M32(wpal + (u32)w2 * 0x28u + 4) : 0xFFFFFFFFu;
		}
	}
	return found;
}
/* 6i.170: your 6i.169 test - Elites, Brutes and Marines came with no weapon, a Jackal with its shield and a sniper's
 * rifle. A unit is armed with what its squad's placement names and nothing else (the game's own placement, v1.0
 * 0x2902b0: the two palette weapons handed to function_1e04a0), and the squad found for a biped can name none, or be
 * another kind's (the nearest Jackal squad was a sniper one). So each kind of biped has a usual weapon, taken from
 * this list in order - the first the level has - when its squad names none; a Jackal (which carries a shield as
 * dropped) always takes its own. A biped not listed here is armed by its squad or not at all. */
static const struct { const char *biped; const char *arms[4]; u8 always; } os_arms[] = {
	{ "Elite", { "Plasma Rifle", "Carbine", "Needler", "Plasma Pistol" }, 0 },
	{ "Arbiter", { "Plasma Rifle", "Carbine", "Energy Sword", "Plasma Pistol" }, 0 },
	{ "Heretic", { "Plasma Rifle", "Carbine", "Needler", "Plasma Pistol" }, 0 },
	{ "Brute", { "Brute Plasma Rifle", "Brute Shot", "Carbine", "Plasma Rifle" }, 0 },
	{ "Grunt", { "Plasma Pistol", "Needler", 0, 0 }, 0 },
	{ "Heretic Grunt", { "Plasma Pistol", "Needler", 0, 0 }, 0 },
	{ "Jackal", { "Plasma Pistol", "Needler", 0, 0 }, 1 },
	{ "Bugger", { "Plasma Pistol", "Needler", 0, 0 }, 0 },
	{ "Chief", { "Battle Rifle", "SMG", "Pistol", 0 }, 0 },
	{ "Johnson", { "Battle Rifle", "SMG", "Pistol", 0 }, 0 },
	{ "ODST", { "Battle Rifle", "SMG", "Pistol", 0 }, 0 },
	{ "Marine M (random)", { "Battle Rifle", "SMG", "Pistol", 0 }, 0 },
	{ "Marine F", { "Battle Rifle", "SMG", "Pistol", 0 }, 0 },
	{ "Crewman", { "Pistol", "SMG", "Battle Rifle", 0 }, 0 },
	{ "Miranda", { "SMG", "Pistol", "Battle Rifle", 0 }, 0 },
	{ "Flood Combat Form (human)", { "SMG", "Shotgun", "Battle Rifle", "Pistol" }, 0 },
	{ "Flood Combat Form (elite)", { "Plasma Rifle", "Needler", "Carbine", "Plasma Pistol" }, 0 },
};
static s32 os_same(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
/* 6i.171: your 6i.170 test - spawned AI looked at each other and did not always fire. An actor fires a weapon by its
 * character's settings FOR THAT WEAPON (the character's weapons block, +0xCC: 0xCC bytes an entry, the weapon's tag at
 * +8, its own or a parent's - the game's lookup, v1.0 0x1e5280); given a weapon its character has no entry for, it has
 * no firing pattern and holds its fire. So a usual weapon is only one the character has settings for; if none of the
 * list is, the first weapon of the character's own list that the level has loaded. */
static s32 os_char_has_weapon(u32 character, u32 weapon)
{
	s32 depth, i;
	for (depth = 0; depth < 8; depth++)
	{
		u32 def = os_char_def(character), n, a;
		if (!def) return 0;
		n = M32(def + 0xCC); a = M32(def + 0xD0);
		if ((s32)n > 0 && n <= 64u && ptr_ok(a)) for (i = 0; i < (s32)n; i++) if (M32(a + (u32)i * 0xCCu + 8) == weapon) return 1;
		character = M32(def + 8);
	}
	return 0;
}
static u32 os_char_first_weapon(u32 character)
{
	u32 inst = M32(ADDR_TAG_INSTANCES), th = M32(ADDR_CACHE_TAGS_PTR);
	s32 depth, i;
	if (!ptr_ok(inst) || !ptr_ok(th)) return 0xFFFFFFFFu;
	for (depth = 0; depth < 8; depth++)
	{
		u32 def = os_char_def(character), n, a;
		if (!def) break;
		n = M32(def + 0xCC); a = M32(def + 0xD0);
		if ((s32)n > 0 && n <= 64u && ptr_ok(a))
			for (i = 0; i < (s32)n; i++)
			{
				u32 w = M32(a + (u32)i * 0xCCu + 8);
				if (w != 0xFFFFFFFFu && (w & 0xFFFFu) < M32(th + 0x18) && M32(inst + (w & 0xFFFFu) * 16u + 4) == w && M32(inst + (w & 0xFFFFu) * 16u) == OSG_WEAP) return w;
			}
		character = M32(def + 8);
	}
	return 0xFFFFFFFFu;
}
static u32 os_usual_weapon(s32 d, s32 *always, u32 character)
{
	s32 r, k, w;
	*always = 0;
	for (r = 0; r < (s32)(sizeof(os_arms) / sizeof(os_arms[0])); r++)
	{
		if (!os_same(os_arms[r].biped, os_defs[d].label)) continue;
		*always = os_arms[r].always;
		for (k = 0; k < 4 && os_arms[r].arms[k]; k++)
			for (w = 0; w < OS_DEFS; w++)
				if (os_defs[w].group == OSG_WEAP && g_os_tag[w] != 0xFFFFFFFFu && os_same(os_arms[r].arms[k], os_defs[w].label) && os_char_has_weapon(character, g_os_tag[w])) return g_os_tag[w];
		break;
	}
	return os_char_first_weapon(character);
}
/* 6i.170 / 6i.171: Spawn as Ally / Spawn as Enemy. Sides are the game's teams: two teams are at peace when their bit is
 * set in the AI globals' table (*ADDR_AI_PEACE + 0xC4, bit team x 16 + team - what the game's own "are these
 * enemies" asks, v1.0 0x1df560). A biped's side is its unit's team (+0x138) and, with a brain, its actor's (the actors'
 * pool *ADDR_ACTORS_PTR, 0x888 bytes each, the team at +0x24) - the two places the game's "set this actor's team"
 * writes (v1.0 0x1e1150).
 * 6i.171: your 6i.170 test - allies stood where their own kind's squad sent them. What makes the game's allies come
 * with you is their SQUAD (its orders), so the biped now joins a squad of the side asked for: among this level's
 * squads whose team is at peace with yours (Ally) or is not (Enemy), one of its own kind first, then an armed one,
 * then the nearest - to YOU for an ally (the allies around you now), to where it was dropped for an enemy. It takes
 * that squad's team. Its character stays its own kind's when the squad is another kind's (an Elite among Marines is
 * still an Elite). No squad of that side in the level: no squad - it is put on your team (Ally), or on the first
 * team at war with yours (Enemy), and stays where it is. */
static s32 os_peace(u32 a, u32 b)
{
	u32 g = ADDR_AI_PEACE ? M32(ADDR_AI_PEACE) : 0, bit = a * 16u + b;
	if (a == b) return 1;
	if (!ptr_ok(g) || a >= 16u || b >= 16u) return 0;
	return (M32(g + 0xC4 + 4u * (bit >> 5)) >> (bit & 31u)) & 1u;
}
static void os_set_team(u32 unit, u32 actor, u32 team)
{
	u32 o = object_from_datum((s32)unit), pool, data;
	if (o) gw16(o + 0x138, (u16)team);
	if (actor != 0xFFFFFFFFu && ADDR_ACTORS_PTR && ptr_ok(pool = M32(ADDR_ACTORS_PTR)) && ptr_ok(data = M32(pool + 0x44)) && (actor & 0xFFFFu) < M32(pool + 0x20))
		gw16(data + (actor & 0xFFFFu) * 0x888u + 0x24, (u16)team);
}
static u32 os_my_team(u32 *me_out)
{
	u32 me = player_object();
	if (!me) me = user_object(0);
	*me_out = me;
	return me ? M16(me + 0x138) : 0xFFFFu;
}
/* the team for a biped of the side asked for that joins no squad */
static u32 os_side_team(s32 ally)
{
	u32 me, mine = os_my_team(&me), t;
	if (ally || mine == 0xFFFFu) return mine;
	for (t = 2; t < 8u; t++) if (!os_peace(t, mine)) return t;            /* (2 human, 3 covenant, 4 flood, 5 sentinel, 6 heretic, 7 prophet) */
	return 0xFFFFu;
}
/* a squad of that side: 1 found (out filled; out->character the squad's own; *same: its character drives this biped) */
static s32 os_side_squad(s32 ally, u32 biped_tag, const float *at, s_os_squad *out, s32 *same, u32 *team)
{
	u32 scn = ADDR_SCENARIO ? M32(ADDR_SCENARIO) : 0, squads, pal, wpal, me, mine = os_my_team(&me);
	s32 n, np, nw, i, found = 0, best_rank = -1;
	float best = 0.f, ref[3];
	if (!ptr_ok(scn) || mine == 0xFFFFu) return 0;
	n = (s32)M32(scn + 0x160); squads = M32(scn + 0x164); np = (s32)M32(scn + 0x178); pal = M32(scn + 0x17C); nw = (s32)M32(scn + 0x98); wpal = M32(scn + 0x9C);
	if (n <= 0 || n > 1024 || !ptr_ok(squads) || np <= 0 || !ptr_ok(pal)) return 0;
	for (i = 0; i < 3; i++) ref[i] = ally && me ? MF(me + 0x64 + 4u * (u32)i) : at[i];
	for (i = 0; i < n; i++)
	{
		u32 sq = squads + (u32)i * 0x74u, loc = 0, def, ch, t = M16(sq + 0x24), wt;
		s32 e = (s32)(s16)M16(sq + 0x36), w1 = (s32)(s16)M16(sq + 0x3C), w2 = (s32)(s16)M16(sq + 0x3E), k, rank, kind;
		float d2 = 1.0e30f;
		if (t == 0 || t >= 16u || (os_peace(t, mine) != 0) != (ally != 0)) continue;
		if ((s32)M32(sq + 0x48) > 0 && ptr_ok(M32(sq + 0x4C))) loc = M32(sq + 0x4C);
		if (loc)
		{
			if ((s16)M16(loc + 0x20) != -1) e = (s32)(s16)M16(loc + 0x20);
			if ((s16)M16(loc + 0x22) != -1) w1 = (s32)(s16)M16(loc + 0x22);
			if ((s16)M16(loc + 0x24) != -1) w2 = (s32)(s16)M16(loc + 0x24);
			if ((s16)M16(loc + 0x10) == -1) { d2 = 0.f; for (k = 0; k < 3; k++) { float d = MF(loc + 4 + 4 * (u32)k) - ref[k]; d2 += d * d; } }
		}
		if (e < 0 || e >= np) continue;
		ch = M32(pal + (u32)e * 8u + 4);
		def = os_char_def(ch);
		if (!def) continue;
		{ u32 gen = os_char_block(ch, 0x34); if (!gen || (M8(gen) & 1)) continue; }   /* (not a swarm's squad) */
		kind = M32(def + 0x10) == biped_tag;
		wt = (w1 >= 0 && w1 < nw && ptr_ok(wpal)) ? M32(wpal + (u32)w1 * 0x28u + 4) : 0xFFFFFFFFu;
		rank = (kind ? 2 : 0) + (wt != 0xFFFFFFFFu ? 1 : 0);
		if (found && (rank < best_rank || (rank == best_rank && !(d2 < best)))) continue;
		found = 1; best = d2; best_rank = rank;
		out->squad = i; out->character = ch; out->flag = (M32(sq + 0x20) >> 10) & 1u;
		out->weapon = wt;
		out->weapon2 = (w2 >= 0 && w2 < nw && ptr_ok(wpal)) ? M32(wpal + (u32)w2 * 0x28u + 4) : 0xFFFFFFFFu;
		*same = kind; *team = t;
	}
	return found;
}

/* ---- 6i.182: A SPAWNED ALLY FOLLOWS YOU - its own squad, with an order to follow ----
 * There is no "follow" for one actor in this game: following is a SQUAD's ORDER. An order (the scenario's block at
 * +0x240 count / +0x244, 0x7C bytes each) flagged 0x10 at +0x24 ("follow closest player") makes the squad's follow
 * target (the squad's +0x80) the nearest player's unit, and every two seconds the game rebuilds the squad's list of
 * areas around that unit (function_204450: the order's own areas at +0x54 / +0x58 - 8 bytes each, a zone at +4 and an
 * area at +6 - within the area's radius plus the order's follow radius at +0x50, else the nearest one; an order with
 * no areas of its own uses the squad's zone). The squad then keeps to those areas. function_202e90 gives a squad an
 * order; function_203360, each tick, does the two-second refresh for a flagged order.
 * So, for Spawn as Ally with AI (you asked for "its own squad", then "the target set directly"):
 *  1. THE ORDER: the level's order flagged "follow closest player" whose nearest area is nearest you (one with no
 *     areas of its own counts as far). None flagged in the level: the order, of any kind, with the area nearest you -
 *     and the following is then done by hand (3).
 *  2. THE SQUAD: one of the level's squads that has nobody in it (the squads' pool: +0x08 actors, +0x0A alive, both
 *     0) - preferring one with no starting locations of its own (the level's script is then less likely to fill it),
 *     else the last such. The ally's actor is made in it and the squad is given the order. Later allies join the same
 *     squad while it still has someone and still has that order. The existing squads are not touched.
 *  3. BY HAND, when the order is not a "follow" one: every two seconds the squad's follow target is set to your unit
 *     and the game's own function_204450 is run for it - the same areas-around-you list, from an order that does not
 *     ask for it itself.
 * WHAT THIS CANNOT DO: lead an ally where the level has no areas. The areas are the level designers' (firing
 * positions grouped in zones); between and beyond them an ally goes to the nearest one and stops.
 * No order in the level at all, no empty squad, v1.5 (not mapped), or AI off: as before 6i.182 - the ally joins the
 * nearest squad at peace with you, or none. ---- */
static s32 g_os_fw_squad = -1, g_os_fw_order = -1, g_os_fw_manual;
static u32 g_os_fw_scn, g_os_fw_tick;
#ifdef HOST
static u32 g_host_os_order[3], g_host_os_follow[3];   /* squad, order, calls; order, squad, calls */
static void os_order_call(u32 squad, u32 order) { u32 p = M32(ADDR_SQUADS_PTR), d = ptr_ok(p) ? M32(p + 0x44) : 0; g_host_os_order[0] = squad; g_host_os_order[1] = order; g_host_os_order[2]++; if (ptr_ok(d)) gw16(d + squad * 0x98u + 0x2A, (u16)order); }
static void os_follow_call(u32 order, u32 squad) { g_host_os_follow[0] = order; g_host_os_follow[1] = squad; g_host_os_follow[2]++; }
u32 host_os_order(s32 k) { return g_host_os_order[k]; }
u32 host_os_follow(s32 k) { return g_host_os_follow[k]; }
s32 host_os_fw(s32 k) { return k == 0 ? g_os_fw_squad : k == 1 ? g_os_fw_order : g_os_fw_manual; }
static void os_follow_keep(u32 tick);
void host_os_follow_keep(u32 tick) { os_follow_keep(tick); }
void host_os_fw_reset(void) { g_os_fw_squad = -1; g_os_fw_order = -1; g_os_fw_manual = 0; g_host_os_order[2] = 0; g_host_os_follow[2] = 0; }
#else
void os_order_call(u32 squad, u32 order);         /* hooks.S: function_202e90 */
void os_follow_call(u32 order, u32 squad);        /* hooks.S: function_204450 */
#endif
static u32 os_rt_squad(s32 i)                     /* a squad's own record while the level runs, or 0 */
{
	u32 p = ADDR_SQUADS_PTR ? M32(ADDR_SQUADS_PTR) : 0, d;
	if (!ptr_ok(p) || i < 0 || (u32)i >= M32(p + 0x20) || !ptr_ok(d = M32(p + 0x44))) return 0;
	return d + (u32)i * 0x98u;
}
static float os_order_near(u32 scn, u32 def, const float *at)   /* how near (squared) the order's nearest area is; 1e30: it has none */
{
	s32 n = (s32)M32(def + 0x54), nz = (s32)M32(scn + 0x168), k, q;
	u32 list = M32(def + 0x58), zones = M32(scn + 0x16C);
	float best = 1.0e30f;
	if (n <= 0 || n > 256 || !ptr_ok(list) || nz <= 0 || !ptr_ok(zones)) return best;
	for (k = 0; k < n; k++)
	{
		s32 z = (s32)(s16)M16(list + (u32)k * 8u + 4), ar = (s32)(s16)M16(list + (u32)k * 8u + 6);
		u32 zone, area;
		float d2 = 0.f;
		if (z < 0 || z >= nz) continue;
		zone = zones + (u32)z * 0x38u;
		if (ar < 0 || ar >= (s32)M32(zone + 0x30) || !ptr_ok(M32(zone + 0x34))) continue;
		area = M32(zone + 0x34) + (u32)ar * 0x88u;
		if ((s16)M16(area + 0x30) != -1) continue;                           /* placed on something that moves: not measured */
		for (q = 0; q < 3; q++) { float d = MF(area + 0x24 + 4u * (u32)q) - at[q]; d2 += d * d; }
		if (d2 < best) best = d2;
	}
	return best;
}
/* the order and the empty squad for an ally that follows: 1 found (*manual: the order is not a "follow" one) */
static s32 os_follow_pick(const float *at, s32 *squad, s32 *order, s32 *manual)
{
	u32 scn = ADDR_SCENARIO ? M32(ADDR_SCENARIO) : 0, defs, squads, rt;
	s32 n, ns, i, best = -1, best_f = 0, sq = -1, sq_bare = 0;
	float bd = 0.f;
	if (!ADDR_SQUAD_ORDER || !ADDR_SQUADS_PTR || !ptr_ok(scn)) return 0;
	n = (s32)M32(scn + 0x240); defs = M32(scn + 0x244); ns = (s32)M32(scn + 0x160); squads = M32(scn + 0x164);
	if (n <= 0 || n > 2048 || !ptr_ok(defs) || ns <= 0 || ns > 1024 || !ptr_ok(squads)) return 0;
	for (i = 0; i < n; i++)
	{
		u32 def = defs + (u32)i * 0x7Cu;
		s32 f = (M32(def + 0x24) & 0x10u) != 0;
		float d2 = os_order_near(scn, def, at);
		if (!f && !(d2 < 1.0e29f)) continue;                                  /* not a follow order, and no areas to follow among */
		/* 6i.185: an order that is NOT a follow one is preferred now - the mod keeps the squad's areas round you itself,
		 * from every zone (os_follow_keep); under a follow order the game would redo them every two seconds from that
		 * order's own areas only. A follow order is the fallback (left to the game). */
		if (best >= 0 && (f > best_f || (f == best_f && !(d2 < bd)))) continue;
		best = i; best_f = f; bd = d2;
	}
	if (best < 0) return 0;
	if (g_os_fw_squad >= 0 && g_os_fw_scn == scn && (rt = os_rt_squad(g_os_fw_squad)) != 0 && (s16)M16(rt + 0x0A) > 0 && (s32)(s16)M16(rt + 0x2A) == g_os_fw_order)
	{ *squad = g_os_fw_squad; *order = g_os_fw_order; *manual = g_os_fw_manual; return 1; }   /* the squad the last ally got: still someone in it, still that order */
	for (i = 0; i < ns; i++)
	{
		s32 bare;
		if (!(rt = os_rt_squad(i)) || M16(rt + 0x08) || M16(rt + 0x0A)) continue;
		bare = (s32)M32(squads + (u32)i * 0x74u + 0x48) <= 0;
		if (sq >= 0 && sq_bare && !bare) continue;
		sq = i; sq_bare = bare;
	}
	if (sq < 0) return 0;
	*squad = sq; *order = best; *manual = !best_f;
	return 1;
}
static u32 os_my_unit(void)                       /* your unit, as the game names it (not its address) */
{
	u32 lp = M32(ADDR_LOCAL_PLAYERS_PTR), pl;
	if (!ptr_ok(lp) || !(pl = pool_entry(ADDR_PLAYERS_PTR, (s32)M32(lp + 0xC), 0x30))) return 0xFFFFFFFFu;
	return M32(pl + 0x2C);
}
/* 6i.185: YOUR 6i.184 TEST - "follows you" in the message, and they stand where they are. The log: "an ally in its
 * own squad 85, order 114 (not a follow order: the squad's target is set to you by hand every 2 s)". What 6i.182 did by
 * hand was run the game's areas-around-the-target routine with ORDER 114 - and that routine only ever chooses among
 * that order's own areas, which are where the ally already was: wherever you went, the nearest of them was still there.
 * (The allies the level gives you follow because its script keeps handing their squad new orders as you go.)
 * Now the routine is given an order of the mod's own making, which exists only as 0x7C bytes in the mod's memory: its
 * areas are the 24 nearest YOU, from every zone of the level, found again every two seconds; its follow radius 6 units.
 * The game's routine (function_204450) reads an order as "the scenario's orders + number x 0x7C" with nothing checking
 * the number, so the number passed is the one that lands on the mod's block; it reads only the order's flags (+0x24:
 * 0 here), radius (+0x50) and area list (+0x54 count, +0x58 address; 8 bytes an entry, zone +4, area +6) - checked in
 * the decomp and the routine's own code. It then does everything it does for a real follow order: the squad's areas
 * become those round you (10 at most), and each actor standing outside them is sent to find a place inside.
 * Still true: where the level has no areas at all, there is nowhere to send them. */
#define OS_FW_CAND 24
static u8 g_os_fw_def[0x7C * 2] __attribute__((aligned(4)));
static u8 g_os_fw_list[OS_FW_CAND * 8] __attribute__((aligned(4)));
#ifdef HOST
#define OS_FW_DEF_AT 0xE9C00u                     /* (the tests: the block and its list live in the tests' own memory) */
#define OS_FW_LIST_AT 0xE9D00u
static u32 g_host_os_fw_def_at, g_host_os_fw_n;
u32 host_os_fw_def(u32 *n) { *n = g_host_os_fw_n; return g_host_os_fw_def_at; }
#else
#define OS_FW_DEF_AT ((u32)g_os_fw_def)
#define OS_FW_LIST_AT ((u32)g_os_fw_list)
#endif
/* the mod's own order for the routine: the areas nearest `at`; its number for the routine, or 0xFFFFFFFF */
static u32 os_follow_def(u32 scn, const float *at)
{
	u32 base = M32(scn + 0x244), zones = M32(scn + 0x16C), buf = OS_FW_DEF_AT, list = OS_FW_LIST_AT, du, r, fake;
	s32 nz = (s32)M32(scn + 0x168), z, a, n = 0, k, q;
	float cd[OS_FW_CAND];
	s16 cz[OS_FW_CAND], ca[OS_FW_CAND];
	if (!ptr_ok(base) || nz <= 0 || nz > 512 || !ptr_ok(zones)) return 0xFFFFFFFFu;
	for (z = 0; z < nz; z++)
	{
		u32 zone = zones + (u32)z * 0x38u, areas = M32(zone + 0x34);
		s32 na = (s32)M32(zone + 0x30);
		if (na <= 0 || na > 1024 || !ptr_ok(areas)) continue;
		for (a = 0; a < na; a++)
		{
			u32 area = areas + (u32)a * 0x88u;
			float d2 = 0.f;
			if ((s16)M16(area + 0x30) != -1) continue;                         /* placed on something that moves: left out */
			for (q = 0; q < 3; q++) { float d = MF(area + 0x24 + 4u * (u32)q) - at[q]; d2 += d * d; }
			if (n == OS_FW_CAND && !(d2 < cd[n - 1])) continue;
			k = n < OS_FW_CAND ? n++ : n - 1;                                  /* kept in order, nearest first */
			while (k > 0 && d2 < cd[k - 1]) { cd[k] = cd[k - 1]; cz[k] = cz[k - 1]; ca[k] = ca[k - 1]; k--; }
			cd[k] = d2; cz[k] = (s16)z; ca[k] = (s16)a;
		}
	}
	if (!n) return 0xFFFFFFFFu;
	du = buf - base; r = du % 0x7Cu;
	if (r) { if (du > 0xFFFFFFFFu - 0x7Cu) return 0xFFFFFFFFu; du += 0x7Cu - r; }
	fake = base + du;                                                        /* inside g_os_fw_def, a whole number of orders from the scenario's */
	for (k = 0; k < 0x7C; k++) gw8(fake + (u32)k, 0);
	for (k = 0; k < n; k++) { gw32(list + (u32)k * 8u, 0u); gw32(list + (u32)k * 8u + 4u, (u32)(u16)cz[k] | ((u32)(u16)ca[k] << 16)); }
	/* 6i.186: your 6i.185 test - they follow, but only once you are 200 ft or more away, and keep 40 ft or more back.
	 * (A world unit is 10 ft.) The order's follow radius was 6 units: an area stayed "round you" while you were within its
	 * own radius + 6 of its centre, so an ally left its area only when you were some 20 units gone. Now 1 unit: only the
	 * areas you are in or right beside count, or, if none, the single nearest one - so they move up as soon as you leave
	 * the area they stand in. And it is done every second, not every two. How close they then stand is the area's size
	 * and the place the game picks for each of them in it - that part is the game's. */
	{ union { float f; u32 u; } rad; rad.f = 1.f; gw32(fake + 0x50, rad.u); }
	gw32(fake + 0x54, (u32)n);
	gw32(fake + 0x58, list);
#ifdef HOST
	g_host_os_fw_def_at = fake; g_host_os_fw_n = (u32)n;
#endif
	return du / 0x7Cu;
}
static void os_follow_keep(u32 tick)              /* (3): every two seconds, the squad's target set to you and its areas rebuilt round you */
{
	u32 scn = ADDR_SCENARIO ? M32(ADDR_SCENARIO) : 0, rt, me, num, obj, root, par;
	float at[3];
	s32 up, q;
	if (g_os_fw_squad < 0 || !g_os_fw_manual) return;
	if (scn != g_os_fw_scn || !(rt = os_rt_squad(g_os_fw_squad)) || (s16)M16(rt + 0x0A) <= 0 || (s32)(s16)M16(rt + 0x2A) != g_os_fw_order) { g_os_fw_squad = -1; return; }   /* another level, nobody left, or another order since: let go */
	if (tick - g_os_fw_tick < 30u) return;                                   /* 6i.186: every second (was two) */
	g_os_fw_tick = tick;
	me = os_my_unit();
	if (me == 0xFFFFFFFFu || !(obj = player_object())) return;
	root = obj;                                                              /* where you are in the world (riding something: where it is) */
	for (up = 0; up < 4 && (s32)(par = M32(root + 0x14)) != -1; up++) { u32 po = object_from_datum((s32)par); if (!po) break; root = po; }
	for (q = 0; q < 3; q++) at[q] = MF(root + 0x64 + 4u * (u32)q);
	num = os_follow_def(scn, at);
	if (num == 0xFFFFFFFFu) return;                                          /* no areas in the level: nothing to send them to */
	gw32(rt + 0x80, me);
	os_follow_call(num, (u32)g_os_fw_squad);
}
static void os_follow_tick(void) { u32 t = M32(ADDR_GAME_TIME_PTR); if (ptr_ok(t)) os_follow_keep(M32(t + 8)); }

/* ---- 6i.183: A SPAWNED FLIER FLIES (your note: Buggers do not fly) ----
 * The game does not make a flying character fly by itself. Where it places one from a squad (function_1e0850), the
 * starting location carries an "initial movement mode" (+0x3E); for 2, flying, and a character whose movement settings
 * (the character tag's block at +0x5C, first byte) have 0x20 set, it sends the new actor's unit the request 0x2C with
 * the byte after it 1 (function_e6900: the unit in eax, the request in edi) - "take off". The mod made the actor and
 * never asked. Now, after the actor is made: a character that can fly (that same 0x20) gets that same request. ---- */
#ifdef HOST
static u32 g_host_os_req[4];                      /* unit, type, flag, calls */
static void os_request_call(u32 unit, void *request) { g_host_os_req[0] = unit; memcpy(&g_host_os_req[1], request, 4); g_host_os_req[2] = ((u8 *)request)[4]; g_host_os_req[3]++; }
u32 host_os_fly(s32 k) { return g_host_os_req[k]; }
void host_os_fly_reset(void) { g_host_os_req[3] = 0; }
#else
void os_request_call(u32 unit, void *request);    /* hooks.S: function_e6900 */
#endif
static s32 os_take_off(u32 character, u32 unit)   /* 1: a flier, told to fly */
{
	u32 mv = os_char_block(character, 0x5C);
	u8 req[0x20] __attribute__((aligned(4)));
	s32 k;
	if (!ADDR_UNIT_REQUEST || !mv || !(M8(mv) & 0x20u)) return 0;
	for (k = 0; k < 0x20; k++) req[k] = 0;
	req[0] = 0x2C; req[4] = 1;
	os_request_call(unit, req);
	return 1;
}
/* the dropped biped given its brain: the words for the message (what it got, or why not) */
static const char *os_brain(s32 d, u32 unit, const float *at)
{
	static char said[80];
	u32 character, ai = ADDR_AI_GLOBALS_PTR ? M32(ADDR_AI_GLOBALS_PTR) : 0, general, actor;
	s_os_squad sq = { -1, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u };
	s32 in_squad, k = 0, v, always;
	u32 w1, w2, usual;
	const char *p;
	if (!ADDR_AI_GLOBALS_PTR || !ptr_ok(ai) || !M8(ai + 1)) return "no AI: AI is off here";
	s32 side = o_os_ally ? 1 : o_os_enemy ? 2 : 0, same = 1, fw = 0, fw_squad = -1, fw_order = -1, fw_manual = 0, fly = 0;
	u32 team = 0xFFFFu;
	if (side == 1 && os_follow_pick(at, &fw_squad, &fw_order, &fw_manual))  /* 6i.182: an ally - its own squad, with an order to follow you */
	{
		u32 me;
		fw = 1; in_squad = 1;
		sq.squad = fw_squad; sq.character = g_os_char[d]; sq.weapon = sq.weapon2 = 0xFFFFFFFFu; sq.flag = 0u;
		team = os_my_team(&me);
	}
	else if (side)                                                         /* 6i.171: a squad of the side asked for, or none */
	{
		in_squad = os_side_squad(side == 1, g_os_tag[d], at, &sq, &same, &team);
		if (!in_squad || side == 1) team = os_side_team(side == 1);        /* 6i.172: an ally is on YOUR team, squad or none (your 6i.171 test: on its squad's team, your other allies went for it) */
		if (in_squad && !same) { sq.character = g_os_char[d]; sq.weapon = sq.weapon2 = 0xFFFFFFFFu; }   /* another kind's squad: its own character, its own usual weapon */
	}
	else in_squad = os_squad_find(g_os_tag[d], at, &sq, g_os_char[d]);
	character = in_squad ? sq.character : g_os_char[d];
	if (character == 0xFFFFFFFFu || !os_char_def(character)) return "no AI: no character in this level";
	general = os_char_block(character, 0x34);
	if (!general) return "no AI: its character has no settings";
	if (M8(general) & 1) return "no AI: a swarm character";
	w1 = in_squad ? sq.weapon : 0xFFFFFFFFu; w2 = in_squad ? sq.weapon2 : 0xFFFFFFFFu;
	usual = os_usual_weapon(d, &always, character);                        /* 6i.170 */
	if (w1 != 0xFFFFFFFFu && !os_char_has_weapon(character, w1)) w1 = 0xFFFFFFFFu;   /* 6i.171: a squad's weapon this character has no settings for: not that one */
	if (usual != 0xFFFFFFFFu && (always || w1 == 0xFFFFFFFFu)) { w1 = usual; if (always) w2 = 0xFFFFFFFFu; }
	os_equip_call(w1, character, unit, w2);
	actor = os_actor_call(in_squad ? (u32)sq.squad : 0xFFFFFFFFu, character, unit, in_squad ? sq.flag : 0u);
	if (actor == 0xFFFFFFFFu) return "no AI: the game made no actor";
	if (!in_squad) os_actor_on_call(actor); else os_squads_refresh();
	if (side && team != 0xFFFFu) os_set_team(unit, actor, team);           /* 6i.170: after the squad has given it its team */
	fly = os_take_off(character, unit);                                    /* 6i.183 */
	if (fw)                                                                /* 6i.182: the squad's order (once: later allies find it set) */
	{
		u32 rt = os_rt_squad(fw_squad), scn = M32(ADDR_SCENARIO), tm = M32(ADDR_GAME_TIME_PTR);
		s32 at2;
		if (rt && (s32)(s16)M16(rt + 0x2A) != fw_order) os_order_call((u32)fw_squad, (u32)fw_order);
		g_os_fw_squad = fw_squad; g_os_fw_order = fw_order; g_os_fw_manual = fw_manual; g_os_fw_scn = scn;
		g_os_fw_tick = ptr_ok(tm) ? M32(tm + 8) - 30u : 0u;                 /* (by hand: at the next tick) */
		at2 = lput(0, "object spawning: an ally in its own squad "); at2 = lnum(at2, (u32)fw_squad); at2 = lput(at2, ", order "); at2 = lnum(at2, (u32)fw_order);
		lput(at2, fw_manual ? " (the mod keeps the squad's areas round you, from every zone, every second)" : " (a follow-the-player order, the only kind here: the game keeps it, among that order's areas)");
		log_write();
	}
	for (p = fw ? "with AI, follows you, squad " : in_squad ? "with AI, squad " : side ? "with AI, no squad of that side: stays put" : "with AI, no squad of its kind: stays put"; *p; p++) said[k++] = *p;
	if (in_squad)
	{
		v = sq.squad;
		if (v >= 100) said[k++] = (char)('0' + v / 100 % 10);
		if (v >= 10) said[k++] = (char)('0' + v / 10 % 10);
		said[k++] = (char)('0' + v % 10);
	}
	for (p = w1 == 0xFFFFFFFFu ? ", unarmed" : ""; *p; p++) said[k++] = *p;
	for (p = fly ? ", flying" : ""; *p; p++) said[k++] = *p;
	for (p = side == 1 ? ", ally" : side == 2 ? ", enemy" : ""; *p; p++) said[k++] = *p;
	said[k] = 0;
	return said;
}
/* 6i.177: the map file's string table, found in the header (see the notes at g_os_var_id); a string by its id */
static u32 os_le32(const u8 *b) { return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24); }
static void os_strings_find(u32 hdr, u32 fsize)
{
	u32 p;
	g_os_str_n = 0;
	for (p = 0x100u; p + 16u <= 0x2BCu && !g_os_str_n; p += 4u)
	{
		u32 c = M32(hdr + p), size = M32(hdr + p + 4), io = M32(hdr + p + 8), bo = M32(hdr + p + 12), k, prev = 0;
		s32 good = 1;
		u8 *b;
		if (c < 256u || c > 0x40000u || size < c || size > 0x1000000u || io < 0x800u || bo < 0x800u || io == bo || io >= fsize || bo >= fsize || c * 4u > fsize - io || size > fsize - bo) continue;
		if (os_read(io, &b, 32) != 32u) continue;
		for (k = 0; k < 8u && good; k++) { u32 v = os_le32(b + 4u * k); if ((!k && v) || (k && v <= prev) || v >= size) good = 0; prev = v; }
		if (good) { g_os_str_n = c; g_os_str_io = io; g_os_str_bo = bo; g_os_str_size = size; }
	}
}
static s32 os_string(u32 id, char *out, u32 cap)  /* 1: read, and as long as the id says */
{
	u32 idx = id & 0xFFFFu, len = id >> 24, off, k;
	u8 *b;
	if (!g_os_str_n || idx >= g_os_str_n || !len || len + 1u > cap) return 0;
	if (os_read(g_os_str_io + 4u * idx, &b, 4) != 4u) return 0;
	off = os_le32(b);
	if (off >= g_os_str_size || len + 1u > g_os_str_size - off) return 0;
	if (os_read(g_os_str_bo + off, &b, len + 1u) != len + 1u || b[len]) return 0;
	for (k = 0; k < len; k++) { if (b[k] < 0x20 || b[k] > 0x7E) return 0; out[k] = (char)b[k]; }
	out[len] = 0;
	return 1;
}
static void os_variants(u32 inst, u32 count, u32 hdr, u32 fsize)   /* the Warthog's model variants other than its default (the file open) */
{
	u32 tag, def, mdl, m, vp, dflt, i;
	s32 n, d, named = 0, at;
	g_os_var_n = 0; g_os_var_seen = 0;
	if (g_os_var_def < 0) for (d = 0; d < OS_DEFS; d++) if (os_defs[d].group == OSG_VEHI && str_eq(os_defs[d].label, "Warthog")) g_os_var_def = d;
	if (g_os_var_def < 0 || (tag = g_os_tag[g_os_var_def]) == 0xFFFFFFFFu || (tag & 0xFFFFu) >= count) return;
	def = M32(inst + (tag & 0xFFFFu) * 16u + 8);
	if (!ptr_ok(def)) return;
	mdl = M32(def + 0x38);
	if (mdl == 0xFFFFFFFFu || (mdl & 0xFFFFu) >= count || !ptr_ok(m = M32(inst + (mdl & 0xFFFFu) * 16u + 8))) return;
	n = (s32)M32(m + 0x50); vp = M32(m + 0x54);
	if (n <= 0 || n > 32 || !ptr_ok(vp)) return;
	g_os_var_seen = n; dflt = M32(def + 0x30);
	os_strings_find(hdr, fsize);
	for (i = 0; i < (u32)n && g_os_var_n < OS_VAR_MAX; i++)
	{
		u32 id = M32(vp + i * 0x38u);
		char nm[28], *to = g_os_var_label[g_os_var_n];
		const char *p;
		s32 k = 0, q, gauss = 0;
		if (!id || id == dflt || (!dflt && !i)) continue;                     /* the default: "Warthog" itself spawns that */
		for (p = "Warthog ("; *p; p++) to[k++] = *p;
		if (os_string(id, nm, sizeof(nm)))
		{
			for (q = 0; nm[q]; q++) if (nm[q] == 'd' && nm[q + 1] == 'a' && nm[q + 2] == 'm' && nm[q + 3] == 'a' && nm[q + 4] == 'g') break;
			if (nm[q]) continue;                                               /* 6i.185: your note - "(damaged)" is not wanted */
			named++;
			for (q = 0; nm[q]; q++) if (nm[q] == 'g' && nm[q + 1] == 'a' && nm[q + 2] == 'u' && nm[q + 3] == 's' && nm[q + 4] == 's') gauss = 1;
			for (p = gauss ? "Gauss" : nm; *p; p++) to[k++] = *p;
		}
		else { for (p = "variant "; *p; p++) to[k++] = *p; if (i + 1u >= 10u) to[k++] = (char)('0' + (i + 1u) / 10u); to[k++] = (char)('0' + (i + 1u) % 10u); }
		to[k++] = ')'; to[k] = 0;
		g_os_var_id[g_os_var_n++] = id;
	}
	at = lput(0, "object spawning: the Warthog's model has "); at = lnum(at, (u32)n); at = lput(at, " variant(s); listed beside it: "); at = lnum(at, (u32)g_os_var_n);
	lput(at, !g_os_var_n ? "" : named == g_os_var_n ? " (names read)" : g_os_str_n ? " (some names not read)" : " (the string table was not found: numbered)");
	log_write();
}
static void os_build(void)
{
	u32 th = M32(ADDR_CACHE_TAGS_PTR), hdr = ADDR_CACHE_HEADER, inst, count, scn, fsize, p;
	s32 k, ok = 0, tries = 0, at;
	for (k = 0; k < 0xFF && (g_os_map[k] = (char)M8(ADDR_MAP_NAME + (u32)k)); k++) ;
	g_os_map[k] = 0;
	g_os_scn = ptr_ok(th) ? M32(th + 0xC) : 0xFFFFFFFFu;
	g_os_state = -1;
	for (k = 0; k < OS_DEFS; k++) g_os_tag[k] = 0xFFFFFFFFu;
	g_os_found = 0;
	inst = M32(ADDR_TAG_INSTANCES);
	if (hdr && ptr_ok(th) && ptr_ok(inst) && g_os_map[0])
	{
		count = M32(th + 0x18); scn = g_os_scn; fsize = M32(hdr + 8);
		if (count && count <= 0x10000u && (scn & 0xFFFFu) < count && os_open())
		{
			s32 step;
			for (step = -1; step < (0x800 - 0x10 - 16) / 4 + 1 && !ok && tries < 6; step++)   /* where it should be (+0x2BC), then anywhere in the header */
			{
				u32 c, off, size, io;
				p = step < 0 ? 0x2BCu : 0x10u + 4u * (u32)step;
				if (step >= 0 && p == 0x2BCu) continue;
				c = M32(hdr + p); off = M32(hdr + p + 4); size = M32(hdr + p + 8); io = M32(hdr + p + 12);
				if (c != count || off < 0x800u || size < count * 2u || size > 0x400000u || off >= fsize || size > fsize - off) continue;
				if (io < 0x800u || io >= fsize || count * 4u > fsize - io) continue;
				tries++;
				ok = os_names_walk(off, size, count, inst, scn);
			}
			if (ok) os_variants(inst, count, hdr, fsize);                    /* 6i.177 (the file still open) */
			os_close();
		}
	}
	for (k = 0; k < OS_DEFS; k++) { g_os_char[k] = 0xFFFFFFFFu; g_os_char_base[k] = 0; }
	if (ok) { g_os_state = 1; os_chars(inst, M32(th + 0x18)); }
	else { for (k = 0; k < OS_DEFS; k++) g_os_tag[k] = 0xFFFFFFFFu; g_os_found = 0; g_os_var_n = 0; }
	os_menus_fill(ok);
	at = lput(0, "object spawning: "); at = lput(at, g_os_map);
	if (ok) { at = lput(at, " - "); at = lnum(at, (u32)g_os_found); at = lput(at, " of the list's "); at = lnum(at, (u32)OS_DEFS); lput(at, " in this level"); }
	else lput(at, " - the tag names could not be read");
	log_write();
}
static s32 os_current(void)                      /* the lists are this level's */
{
	u32 th = M32(ADDR_CACHE_TAGS_PTR);
	s32 k;
	if (!g_os_state || !ptr_ok(th) || M32(th + 0xC) != g_os_scn) return 0;
	for (k = 0; k < 0xFF; k++) { char c = (char)M8(ADDR_MAP_NAME + (u32)k); if (c != g_os_map[k]) return 0; if (!c) break; }
	return 1;
}
static void os_check(void)                       /* an Object Spawning menu is open: its lists made for this level, once */
{
	if (os_current()) return;
	os_build();
	g_menu = OS_MENU;                                                      /* (a level changed under an open list: back to the headings) */
}
#ifdef HOST
void host_os_rebuild(void) { g_os_state = 0; os_check(); }
#endif
static void os_run(void)                         /* the tick hook: the chosen object, in front of you */
{
	s32 d = g_os_req - 1, k, at;
	u32 obj, tag = 0xFFFFFFFFu, made, group = 0, variant = 0;
	const char *label = "";
	float f[3], len, dist, lift, p[3];
	u8 rec[0xC4] __attribute__((aligned(4)));
	char msg[104];
	g_os_req = 0;
	if (d < 0) return;
	at = 0;
	/* 6i.177: what was chosen - an entry of the lists (as ever), or a Warthog variant (1900 + n: the Warthog's own entry
	 * with that variant's name in the placement) */
	if (d >= 1900) { k = d - 1900; d = g_os_var_def; if (k >= g_os_var_n || d < 0) return; variant = g_os_var_id[k]; label = g_os_var_label[k]; group = os_defs[d].group; tag = g_os_tag[d]; }
	else if (d < OS_DEFS) { label = os_defs[d].label; group = os_defs[d].group; tag = g_os_tag[d]; }
	else return;
#define OS_SAY(t) do { const char *q = (t); while (*q && at < 102) msg[at++] = *q++; msg[at] = 0; } while (0)
	if (on_main_menu() || g_os_state != 1 || !os_current() || tag == 0xFFFFFFFFu) { toast("Object Spawning: not in this level"); return; }
	{ s32 cam = o_free_cam && g_fc_on;                                     /* 6i.160: flying the free camera - in front of IT, along where it looks */
	float look[3], rt[3], up3[3];
	obj = cam ? 0 : player_object();
	if (!cam && !obj) { toast("Object Spawning: you have no body to spawn in front of"); return; }
	if (cam) { fc_dirs(g_fc_yaw, g_fc_pitch, look, rt, up3); f[0] = look[0]; f[1] = look[1]; }
	else
	{
		/* 6i.163: your 6i.162 test (a bit off to the side) - along where you LOOK (the unit's looking vector, +0x168),
		 * not where the body faces (+0x70): the legs only follow the view once it has turned far enough. The body's
		 * facing only if you look straight up or down */
		f[0] = MF(obj + 0x168); f[1] = MF(obj + 0x16C);
		if (f[0] * f[0] + f[1] * f[1] < 0.0001f) { f[0] = MF(obj + 0x70); f[1] = MF(obj + 0x74); }
	}
	f[2] = 0.f;
	len = fsqrt(f[0] * f[0] + f[1] * f[1]);
	if (len > 0.001f) { f[0] /= len; f[1] /= len; }
	else if (cam) { f[0] = -rt[1]; f[1] = rt[0]; }                         /* looking straight up or down: the way the camera is turned */
	else { f[0] = 1.f; f[1] = 0.f; }
	/* how far ahead and how far above your feet, by kind: a vehicle clear of you, the rest within reach */
	if (group == OSG_VEHI) { dist = 4.0f; lift = 1.0f; }
	else if (group == OSG_BIPD) { dist = 1.5f; lift = 0.2f; }
	else if (group == OSG_BLOC) { dist = 1.5f; lift = 0.5f; }
	else { dist = 1.0f; lift = 0.6f; }
	if (cam) for (k = 0; k < 3; k++) p[k] = g_fc_p[k] + look[k] * dist;      /* the same distances, along the look itself (no lift: the camera is not at your feet) */
	else
	{
		/* 6i.214: YOUR NOTE - spawned from a vehicle, the object is nowhere to be seen. A unit sitting in a seat has a
		 * parent (+0x14), and its own position (+0x64) is then measured FROM THE SEAT, not in the level: a driver's reads a
		 * fraction of a unit from nothing. The object was made that far from the level's origin - wherever that is on the
		 * map - not in front of you. Where you ARE while seated is your root part's place (the first of the body's part
		 * matrices, +0x28: the level's own co-ordinates - what the in-between frames already go by for a seat). From
		 * there, along where you look as before, and 3 units further out so it lands clear of what you are sitting in. */
		float base[3];
		s32 seated = (s32)M32(obj + 0x14) != -1;
		for (k = 0; k < 3; k++) base[k] = MF(obj + 0x64 + 4 * (u32)k);
		if (seated)
		{
			u32 nb = M16(obj + 0x114), noff = M16(obj + 0x116);
			if (nb < 0x34u || !noff) { toast("Object Spawning: could not tell where you are in this seat"); return; }
			for (k = 0; k < 3; k++) base[k] = MF(obj + noff + 0x28 + 4 * (u32)k);
			dist += 3.0f;
		}
		for (k = 0; k < 3; k++) p[k] = base[k] + f[k] * dist;
		p[2] += lift;
	}
	}
	os_place_call(rec, tag);
	if (variant) memcpy(rec + 0xC, &variant, 4);                           /* 6i.177: the model variant's name (none: the tag's own default) */
	memcpy(rec + 0x1C, p, 12);
	memcpy(rec + 0x28, f, 12);
	{ float up[3] = { 0.f, 0.f, 1.f }; memcpy(rec + 0x34, up, 12); }
	made = os_new_call(rec);
	OS_SAY(made == 0xFFFFFFFFu ? "Could not spawn: " : "Spawned: "); OS_SAY(label);
	if (made == 0xFFFFFFFFu) OS_SAY(" (the game refused)");
	else if (d >= 0 && group == OSG_BIPD && o_os_ai)                      /* 6i.159: its brain, or why it has none (6i.162: with Spawn with AI on) */
	{
		OS_SAY(" ("); OS_SAY(os_brain(d, made, p)); OS_SAY(")");
	}
	else if (d >= 0 && group == OSG_BIPD && (o_os_ally || o_os_enemy))    /* 6i.170 / 6i.171: a body with no brain: its team all the same */
	{
		u32 t = os_side_team(o_os_ally);
		if (t != 0xFFFFu) { os_set_team(made, 0xFFFFFFFFu, t); OS_SAY(o_os_ally ? " (ally)" : " (enemy)"); }
	}
	toast(msg);
	at = lput(0, "object spawning: "); at = lput(at, label); at = lput(at, " tag "); at = lhex(at, tag); at = lput(at, " -> object "); at = lhex(at, made);
	at = lput(at, ", at x100 "); at = lsnum(at, fti(p[0] * 100.f)); at = lput(at, " "); at = lsnum(at, fti(p[1] * 100.f)); at = lput(at, " "); lsnum(at, fti(p[2] * 100.f));   /* 6i.214: where */
	log_write();
#undef OS_SAY
}
#ifdef HOST
s32 host_os_state(void) { return g_os_state; }
s32 host_os_found(void) { return g_os_found; }
u32 host_os_tag(s32 d) { return g_os_tag[d]; }
u32 host_os_usual(s32 d, s32 *always, u32 character) { return os_usual_weapon(d, always, character); }
void host_os_set_tag(s32 d, u32 tag) { g_os_tag[d] = tag; }
void host_set_os_ally(s32 on) { o_os_ally = on; if (on) o_os_enemy = 0; }
void host_set_os_enemy(s32 on) { o_os_enemy = on; if (on) o_os_ally = 0; }
s32 host_os_def(const char *label) { s32 d; for (d = 0; d < OS_DEFS; d++) if (str_eq(label, os_defs[d].label)) return d; return -1; }
void host_os_req(s32 d) { g_os_req = d + 1; }
void host_os_run(void) { if (g_os_req) os_run(); }
void host_os_made_set(u32 n) { g_host_os_new_n = n; }
u32 host_os_placed(float *rec9, u32 *made_n) { memcpy(rec9, g_host_os_rec, sizeof(g_host_os_rec)); *made_n = g_host_os_new_n; return g_host_os_place_tag; }
void host_os_result(u32 r) { g_host_os_result = r; }
u32 host_os_ai(s32 k) { return k < 0 ? g_host_os_ai_n : g_host_os_ai[k]; }
u32 host_os_sq(s32 k) { return g_host_os_sq[k]; }
void host_os_actor(u32 a) { g_host_os_actor = a; }
void host_set_os_ai(s32 on) { o_os_ai = on; }
s32 host_os_ai_on(void) { return o_os_ai; }
u32 host_os_char(s32 d) { return g_os_char[d]; }
void host_os_activate(s32 m, s32 i) { g_menu = m; g_sel[m] = i; change(0, 1); }   /* A on a menu's item */
s32 host_menu_n(s32 m) { return g_menus[m].n; }
const char *host_menu_label(s32 m, s32 i) { return g_menus[m].items[i].label; }
#endif

/* ---- 6i.156: the Campaign Timer (Other Tools) - a run timer that starts and splits by itself, timed as Halo 2 Classic
 * runs are: REAL TIME FROM START TO FINISH, NEVER STOPPED - not for a pause, a cutscene, a load or the load between two
 * levels.
 * - The clock is the console's own millisecond count (the kernel's KeTickCount, which the game's GetTickCount at v1.0
 *   0x3314b0 reads through its import slot 0x411690): the run's time is "now - the count at the start", so nothing the
 *   game does can hold it.
 * - START (6i.161, 6i.163): the moment you have control in the run's FIRST level - the first tick with no cinematic in
 *   progress (the cinematic globals, *0x510c50 +5: what the game's own test 0x13cb40 reads), where the fade from white
 *   begins - not the level's first tick, where its opening cutscene begins. A campaign level's first tick (the game's
 *   tick count 0 - a new level or Restart Level; a level resumed from a save starts later than 0 and does nothing)
 *   arms the timer (ct_armed / ct_trial, below, have the rule). A level that is all cutscene (The Heretic) never starts it: the
 *   next level arms it again. From the main menu that begins a new run; Restart Level on the run's first level arms
 *   it again from 0; on a later level nothing changes - and nothing after the start waits for a cutscene.
 * - SPLIT: only when the level's script ends the mission (game_won -> game_finish, v1.0 0x1388e0; its one call in the
 *   script function at 0x2a99b0 goes through hooks.S won_hook -> ct_won): the time since the last split (or the start)
 *   is that level's. The timer runs on through the load into the next level, whose first tick only changes the name
 *   shown. (A level change that never passed game_won is split when the next level starts.)
 * - END: the game's main menu. After a finished level the run's time is the last split's (complete); reached part-way
 *   through a level it is the time of arriving there (not finished). Nothing is shown there (6i.169). ---- */
#define CT_MAX 20
static s32 g_ct_state;                           /* 0 waiting for a level's start; 1 running, in a level; 2 running, level finished (the next not started); 3 ended (main menu); 4 armed: the first level started, waiting for you to have control */
static s32 g_ct_seen;                            /* armed: a cinematic has been in progress since the level began */
static u32 g_ct_trial;                           /* running: the ticks left in which a cinematic starting re-arms the timer (ct_trial) */
static char g_ct_map[48];                        /* the level being played */
static u32 g_ct_start;                           /* the console's millisecond count at the run's start */
static u32 g_ct_split_at;                        /* the run's time at the last split (0: none yet) */
static u32 g_ct_end_ms;                          /* ended: the run's time */
static u32 g_ct_last;                            /* the game's tick count at the last tick seen */
static s32 g_ct_n;                               /* the finished levels (splits) */
static u32 g_ct_split_ms[CT_MAX];                /* each one's own time (from the split before it) */
static char g_ct_split_map[CT_MAX][48];
#ifdef HOST
static u32 g_host_ct_clock;
static u32 ct_now(void) { return g_host_ct_clock; }
#else
static u32 ct_now(void) { return ADDR_KE_TICK_SLOT ? *(volatile u32 *)(*(u32 *)ADDR_KE_TICK_SLOT) : 0u; }
#endif
static u32 ct_run_ms(void) { return g_ct_state == 3 ? g_ct_end_ms : (g_ct_state == 1 || g_ct_state == 2 ? ct_now() - g_ct_start : 0u); }   /* the run's time */
static void ct_copy(char *to, const char *from) { s32 i; for (i = 0; i < 47 && from[i]; i++) to[i] = from[i]; to[i] = 0; }
static void ct_log(const char *what, const char *map, u32 ms)
{
	s32 at = lput(0, "campaign timer: "); at = lput(at, what); at = lput(at, " "); at = lput(at, map);
	at = lput(at, ", run ms "); at = lnum(at, ms); at = lput(at, ", finished levels "); at = lnum(at, (u32)g_ct_n); at = lput(at, " (last split at ms "); at = lnum(at, g_ct_split_at); lput(at, ")");
	log_write();
}
static void ct_split(void)                       /* the level being played is finished: the time since the last split is its */
{
	u32 run = ct_run_ms();
	if (g_ct_n == CT_MAX)                        /* (the campaign has 15: only a run that never ends gets here) the oldest dropped */
	{
		s32 k;
		for (k = 1; k < CT_MAX; k++) { g_ct_split_ms[k - 1] = g_ct_split_ms[k]; ct_copy(g_ct_split_map[k - 1], g_ct_split_map[k]); }
		g_ct_n--;
	}
	g_ct_split_ms[g_ct_n] = run - g_ct_split_at; ct_copy(g_ct_split_map[g_ct_n], g_ct_map); g_ct_n++;
	g_ct_split_at = run;
}
static void ct_begin(const char *map)            /* a level's first tick */
{
	if (g_ct_state == 1 && str_eq(map, g_ct_map) && g_ct_n > 0) { ct_log("level restarted (the run goes on)", map, ct_run_ms()); return; }
	if (g_ct_state == 1 && !str_eq(map, g_ct_map)) { ct_split(); ct_log("split (the level changed)", g_ct_map, g_ct_split_at); }
	else if (g_ct_state != 2)                                              /* from the main menu, or the run's first level (re)started: a new run - armed, started when you have control */
	{
		s32 again = g_ct_state == 4 && str_eq(map, g_ct_map);               /* (the tick count held at 0: armed already) */
		g_ct_n = 0; g_ct_split_at = 0; g_ct_seen = 0; g_ct_trial = 0;
		ct_copy(g_ct_map, map); g_ct_state = 4;
		if (!again) ct_log("armed (waiting for control)", map, 0);
		return;
	}
	ct_copy(g_ct_map, map); g_ct_state = 1;
	ct_log("level started", map, ct_run_ms());
}
/* 6i.163: your 6i.162 test (Regret: the timer stood at 0:00 through the fade and then snapped to the right time) - the
 * start is the first tick with no cinematic in progress, the moment the fade from white begins; the letterbox flag is
 * not waited for, so the time runs from that tick with nothing to snap. One thing can still be wrong about that tick:
 * a level's script takes a few ticks to start its opening cutscene, and until it does no cinematic is in progress.
 * So a start made before any cinematic was seen is on trial - for 1 s (5 s while the letterbox flag is still set, as
 * the game sets it when a campaign level loads): a cinematic beginning in that time is the opening cutscene, and the
 * timer goes back to armed, 0:00, to start when it ends. */
static void ct_armed(void)                       /* armed, each tick: started once you have control */
{
	u32 cg = M32(ADDR_ASPECT_FLAGS_PTR);                                   /* the cinematic globals */
	s32 cinematic = ptr_ok(cg) && M8(cg + 5), bars = ptr_ok(cg) && M8(cg + 4);
	if (cinematic) { g_ct_seen = 1; return; }
	g_ct_start = ct_now();
	g_ct_state = 1;
	g_ct_trial = g_ct_seen ? 0u : (bars ? 150u : 30u);
	ct_log(g_ct_trial ? "run started (no cutscene yet: on trial)" : "run started (you have control)", g_ct_map, 0);
}
static void ct_trial(void)                       /* running, on trial, each tick */
{
	u32 cg = M32(ADDR_ASPECT_FLAGS_PTR);
	if (g_ct_n || g_ct_state != 1) { g_ct_trial = 0; return; }
	if (ptr_ok(cg) && M8(cg + 5)) { g_ct_trial = 0; g_ct_seen = 1; g_ct_state = 4; ct_log("armed again (the opening cutscene began)", g_ct_map, 0); return; }
	g_ct_trial--;
}
static void ct_tick(void)                        /* before each tick the game runs */
{
	u32 time = M32(ADDR_GAME_TIME_PTR), gt;
	char map[48];
	if (!o_ctimer || !is_campaign() || !ptr_ok(time)) return;
	gt = M32(time + 8);
	if (gt == g_ct_last && g_ct_state == 1) return;                        /* the game's time did not move (its first tick seen once) */
	g_ct_last = gt;
	if (gt == 0)
	{
		if (o_speed != 10) { o_speed = 10; toast("Game Speed: back to 1.0 (Campaign Timer)"); }   /* 6i.169: a campaign mission starts at the game's own speed */
		current_map(map); if (map[0]) ct_begin(map);
	}
	if (g_ct_state == 4) ct_armed();
	else if (g_ct_trial) ct_trial();
}
void ct_won(void)                                /* hooks.S won_hook: the script's game_won, before the game's game_finish */
{
	if (!o_ctimer || g_ct_state != 1 || !is_campaign()) return;
	ct_split(); g_ct_state = 2;
	ct_log("split (mission done)", g_ct_map, g_ct_split_at);
}
static void ct_frame(s32 main_menu)              /* every frame */
{
	if (!o_ctimer) { g_ct_state = 0; g_ct_n = 0; g_ct_split_at = 0; return; }   /* off: forgotten */
	if (main_menu && g_ct_state == 4) g_ct_state = 0;                      /* left before it started: nothing to show */
	if (main_menu && (g_ct_state == 1 || g_ct_state == 2))
	{
		u32 end = g_ct_state == 2 ? g_ct_split_at : ct_run_ms();          /* after a finished level: the last split's time */
		ct_log(g_ct_state == 2 ? "run ended (complete)" : "run ended (level left unfinished)", g_ct_map, end);
		g_ct_end_ms = end; g_ct_state = 3;
	}
}
/* the campaign's level names, by map (any other map shows its own name) */
static const char *ct_name(const char *map)
{
	static const char *const names[15][2] = {
		{ "00a_introduction", "The Heretic" }, { "01a_tutorial", "The Armory" }, { "01b_spacestation", "Cairo Station" },
		{ "03a_oldmombasa", "Outskirts" }, { "03b_newmombasa", "Metropolis" }, { "04a_gasgiant", "The Arbiter" },
		{ "04b_floodlab", "The Oracle" }, { "05a_deltaapproach", "Delta Halo" }, { "05b_deltatowers", "Regret" },
		{ "06a_sentinelwalls", "Sacred Icon" }, { "06b_floodzone", "Quarantine Zone" }, { "07a_highcharity", "Gravemind" },
		{ "08a_deltacliffs", "Uprising" }, { "07b_forerunnership", "High Charity" }, { "08b_deltacontrol", "The Great Journey" } };
	s32 k;
	for (k = 0; k < 15; k++) if (str_eq(map, names[k][0])) return names[k][1];
	return map;
}
static s32 put_2(s32 at, s32 v) { if (v < 10) at = put(at, "0"); return put_int(at, v); }
static s32 put_time(s32 at, u32 ms)              /* m:ss.cc, h:mm:ss.cc from an hour */
{
	u32 cs = ms / 10u, h = cs / 360000u, m = cs / 6000u % 60u, sec = cs / 100u % 60u;
	if (h) { at = put_int(at, (s32)h); at = put(at, ":"); at = put_2(at, (s32)m); }
	else at = put_int(at, (s32)m);
	at = put(at, ":"); at = put_2(at, (s32)sec); at = put(at, "."); return put_2(at, (s32)(cs % 100u));
}
static s32 ct_put(const char *map, u32 ms) { s32 at = put(0, ct_name(map)); at = put(at, "  "); return put_time(at, ms); }
/* the run's total: always hh:mm:ss.cc */
static s32 put_time_full(s32 at, u32 ms)
{
	u32 cs = ms / 10u;
	at = put_2(at, (s32)(cs / 360000u)); at = put(at, ":"); at = put_2(at, (s32)(cs / 6000u % 60u)); at = put(at, ":");
	at = put_2(at, (s32)(cs / 100u % 60u)); at = put(at, "."); return put_2(at, (s32)(cs % 100u));
}
/* 6i.169: the timer in the bottom-right corner, as far from it as the FPS Counter is from the top-left (9 across, 10
 * down), right-justified, a list that grows upward: "Total Time: hh:mm:ss.cc" - the whole run - on the bottom line;
 * above it the level being played and its time so far, in white; above that every finished level with its time, in
 * light grey, the first played at the top. Nothing on the game's main menu (6i.157's list there is gone, and with it
 * the 5-second green line for a split: the finished level joins the list at once). */
/* 6i.170: your 6i.169 photo (Text Scale 0.6) - the last digits cut off at the screen's edge; fine from 0.9 up. The
 * game's own right-justification places a line by its measured width (v1.0 0x13f0e0: the box's right edge less the
 * width at this scale), yet what it draws runs further right the smaller the scale and the longer the line - about
 * 9 to 12 pixels for these lines at 0.6 by the photo, nothing at 1.0. I have not found where in its glyph drawing the
 * difference comes from, so this is a correction fitted to that one photo: below 1.0 the box's right edge moves left by
 * 0.15 pixel for each character and each 0.1 of scale under 1.0 (23 characters at 0.6: 14 pixels). Lines of
 * different lengths then end close to one another; exact it is not. */
static const color4f c_ct_done = { 1.0f, 0.60f, 0.60f, 0.60f };          /* 6i.170: a finished level - plainly grey beside the white lines (0.85 read as white) */
static void ct_line(s32 n, s32 y, const color4f *color)
{
	s32 right = 640 - 9;
	if (o_scale < 10) right -= (n * (10 - o_scale) * 3 + 10) / 20;
	line_at(right - 420, y, 420, color, 1);
}
static void ct_draw(s32 height)
{
	s32 y = 480 - 10 - height, k, n;
	u32 run = ct_run_ms();
	if (g_ct_state == 3 || on_main_menu() || !is_campaign()) return;      /* (ended: nothing until a level starts) */
	k = put(0, "Total Time: "); n = put_time_full(k, run);
	ct_line(n, y, g_ct_state ? &c_speed : &c_uncharged);                   /* grey while waiting for a level's start */
	if (g_ct_state == 1 || g_ct_state == 4)                                /* the level being played (armed: at 0 until you have control) */
	{
		y -= height;
		n = ct_put(g_ct_map, g_ct_state == 1 ? run - g_ct_split_at : 0u); ct_line(n, y, &c_speed);
	}
	for (k = g_ct_n - 1; k >= 0 && y - height >= 40; k--)                  /* the finished levels, the latest nearest */
	{
		y -= height;
		n = ct_put(g_ct_split_map[k], g_ct_split_ms[k]); ct_line(n, y, &c_ct_done);
	}
}
#ifdef HOST
void host_set_ctimer(s32 on) { o_ctimer = on; }
void host_ct_tick(void) { ct_tick(); }
void host_ct_clock(u32 ms) { g_host_ct_clock = ms; }
s32 host_ct(u32 *ms, s32 *n, u32 *done) { *ms = ct_run_ms(); *n = g_ct_n; *done = g_ct_split_at; return g_ct_state; }   /* the run's time, the splits, the last split's time */
u32 host_ct_split(s32 k) { return g_ct_split_ms[k]; }
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
		if (g_ps_state == 5 && ip_give_back("the Monster/Dreadnought spots need it"))   /* 6i.2: once more with that memory */
		{ ps_unload(); g_ps_state = 0; if (guarded_call(ps_load_body)) { ps_unload(); g_ps_state = 4; } }
		ps_say();
	}
	if (g_ps_state != 1 || (o_unlock_fps ? (!g_steps || (g_hud30 & 3)) : (g_count_hud & 3))) return;   /* 6i: 30 Hz steps when unlocked */
	view_camera(p, 0, 0);
	{
		/* every spot within the range of your view (the Range setting); more than PS_NEAR of them: the nearest PS_NEAR -
		 * counted into 64 distance bands, then taken band by band from the nearest (two passes, no sorting) */
		const float R = o_pred_range > 0 ? (float)o_pred_range : 2000.f, R2 = R * R;   /* 0: Max (every BSP's whole) */
		s32 q[3], qr = fti(R * 16.f), band[64], k, cut = 64, need = PS_NEAR, n_in = 0;
		float bd = 1e30f;
		for (k = 0; k < 64; k++) band[k] = 0;
		g_ps_near_n = 0; g_ps_best = -1;
		if (qr > 65535) qr = 65535;
		for (i = 0; i < 3; i++) q[i] = fti((p[i] - g_ps_org[i]) * 16.f);   /* 6i.220: the audit found this one once the code around it moved */
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
				b = fti(d2 / R2 * 64.f); if (b > 63) b = 63;
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
		{ ps_label_launch(r); line_at_s(fti(sx) + g_vp_left - 150, fti(sy) + g_vp_top - height * 3 / 4, 300, r[6] >= 140 ? &c_dread : &c_mon, 2, 0.75f); }
		p[0] = w[0]; p[1] = w[1]; p[2] = w[2] + 0.02f;                       /* the approach's: beside its tip (the spot) */
		project(p, &sx, &sy, &dep);
		if (dep > 0.05f && sx > -100.f && sx < (float)g_vp_w + 100.f && sy > 0.f && sy < (float)g_vp_h)
		{ ps_label_entry(r); line_at_s(fti(sx) + g_vp_left + 8, fti(sy) + g_vp_top - height * 3 / 8, 300, &c_in, 0, 0.75f); }
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
		/* 6h.13: an element anchored bottom-left (the motion tracker's frame, the bar on it): its right edge further out -
		 * the frame's right side showed cut off. 2 pixels did it on the 360; on the original Xbox a few more stayed cut
		 * (6i.36: 4) - the rest of the HUD as it was */
		float cx = 0.25f * (q[0] + q[2] + q[4] + q[6]), x0 = q[0], x1 = q[0], y0 = q[1], y1 = q[1];
		/* 6i.43: 6h.13 / 6i.36's extra room on the right (2, then 4 pixels) gone - it made the ring wider than the bar and
		 * its tracker, and never cured the cut-off; the ring's width a setting instead */
		for (i = 1; i < 4; i++)
		{
			if (q[2 * i] < x0) x0 = q[2 * i];
			if (q[2 * i] > x1) x1 = q[2 * i];
			if (q[2 * i + 1] < y0) y0 = q[2 * i + 1];
			if (q[2 * i + 1] > y1) y1 = q[2 * i + 1];
		}
		/* 6i.39: the frame ring (the squarish one - not the flat bar on it) moved to sit on the tracker's dot and pulses:
		 * your photos - it sat a little up and right of them, on the 360 too. By the Radar frame settings (tests) */
		if (y1 - y0 > 0.6f * (x1 - x0))
			for (i = 0; i < 4; i++)
			{
				q[2 * i] = cx + (q[2 * i] - cx) * (float)o_trk_w * 0.01f + (float)o_trk_dx;   /* its width about its middle */
				q[2 * i + 1] += (float)o_trk_dy;
			}
		else
			for (i = 0; i < 4; i++)                                         /* 6i.41 / 6i.45: the health (shields) bar - the same three */
			{
				q[2 * i] = cx + (q[2 * i] - cx) * (float)o_bar_w * 0.01f + (float)o_bar_dx;
				q[2 * i + 1] += (float)o_bar_dy;
			}
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
float host_hsq_quad_y(u32 corners, s32 i) { u32 q = hsq_quad(corners); return q == corners ? MF(corners + 8 * (u32)i + 4) : g_hsq_quad[(g_hsq_qi - 1) & 3][2 * i + 1]; }
#endif
/* the game's main menu: the game options' state (+8) is 3 - 1 is the campaign, 2 multiplayer (the window manager's own
 * main menu code tests it so). 6f.3 took "no player record" - the main menu has one too: the hint never showed. */
static s32 on_main_menu(void)
{
	u32 g = M32(ADDR_GAME_OPTIONS_PTR);
	return ptr_ok(g) && M32(g + 8) == 3;
}
static void spooky_frame_body(void);
static void ct_frame(s32 main_menu);
static void os_check(void);
static void ct_draw(s32 height);
/* ---- 6i.124: Trigger Volume Overlay (Other Tools) - HCM's, on the Xbox. The level's trigger volumes are the scenario's
 * (g_4e0350, v1.0 0x4e0350: count +0x108, array +0x10c, 0x44 bytes each - +0 name (string id), +4 the object it rides
 * (-1: none), +8 its node, +0xC forward, +0x18 up, +0x24 a corner, +0x30 extents); the game places one with
 * function_11c380 (eax the volume, ebx a transform4x3f: scale, forward, left, up, position), on its object's node when it
 * has one, and a point is inside when 0 < local < extents along forward, left and up (function_11c470). Drawn as yellow
 * boxes - within 200 units. A CHECK is what HCM counts: a script testing the volume against you - volume_test_object,
 * volume_test_objects(_all), the players' test and volume_return_objects all call function_11c470 with an object's
 * centre (+0x30) in edi; hooks.S tvt_hook, at those four calls, notes each test of your centre: the tick, and whether you
 * were inside (any test that tick). Settings:
 *   Trigger names                   the volume's name, from the level's map file (below), else its number
 *   Trigger timer bars              a bar under the name: the time left until the next check - the ticks between your
 *                                   last two checks, counting down
 *   Flash volumes on trigger        red at each check (the bar's 0), fading back to yellow over half a second
 *   Highlight volumes when activated  red while the last check found you inside - kept red while checks keep finding you
 * NAMES: string ids aren't kept in memory by the retail game, so at the first draw in a level the map file
 * (d:\maps\<map>.map) is opened and its string table found from the header - a pair of offsets whose strings are
 * exactly as long as the volumes' string ids say (their top byte); none found: numbers. Logged either way. ---- */
#define TV_MAX 512
#define TV_NAME 24
static u32 g_tv_last[TV_MAX], g_tv_scn;          /* each volume's last check: its tick + 1 (0: never) */
static u16 g_tv_gap[TV_MAX];                      /* the ticks between its last two checks (0: one seen) */
static u8 g_tv_hit[TV_MAX];                       /* the last check found you inside */
static char *g_tv_names;                           /* run-time memory: TV_MAX names, then a 0x800 header buffer */
static u8 g_tv_act[TV_MAX];                       /* 6i.126: 0 not known, 1 in the loaded BSP, 2 not (dark gray) */
static u32 g_tv_bsp;                              /* ... for this collision BSP */
static s32 g_tv_names_state, g_tv_h = 16;         /* 0 not tried this level, 1 read, -1 none */
static void tv_reset(u32 scn)
{
	memset(g_tv_last, 0, sizeof(g_tv_last)); memset(g_tv_gap, 0, sizeof(g_tv_gap)); memset(g_tv_hit, 0, sizeof(g_tv_hit)); memset(g_tv_act, 0, sizeof(g_tv_act));
	g_tv_scn = scn; g_tv_names_state = 0;
}
void tvt_note(u32 index, u32 point, u32 result)  /* hooks.S tvt_hook: right after a script's test (the game's answer) */
{
	u32 obj, t, cur;
	if (!o_trig || index >= TV_MAX || !ADDR_SCENARIO || M32(ADDR_SCENARIO) != g_tv_scn) return;
	obj = player_object();
	if (!obj || point != obj + 0x30) return;                                      /* only tests of you, as HCM's */
	t = M32(ADDR_GAME_TIME_PTR);
	if (!ptr_ok(t)) return;
	cur = M32(t + 8) + 1u;
	if (cur == g_tv_last[index]) { if (result & 0xFFu) g_tv_hit[index] = 1; return; }   /* the same tick: inside if any test was */
	if (g_tv_last[index] && cur > g_tv_last[index]) { u32 gap = cur - g_tv_last[index]; g_tv_gap[index] = (u16)(gap > 65535u ? 65535u : gap); }
	g_tv_last[index] = cur;
	g_tv_hit[index] = (u8)((result & 0xFFu) != 0);
}
static s32 tv_state(s32 i, u32 cur, float *flash)   /* 2 activated (red), 1 flashing (*flash 1 -> 0), 0 yellow */
{
	u32 last = g_tv_last[i];
	*flash = 0.f;
	if (!last) return 0;
	if (o_trig_hl && g_tv_hit[i]) return 2;
	if (o_trig_flash && cur >= last && cur - last < 15u) { *flash = 1.f - (float)(cur - last) / 15.f; return 1; }
	return 0;
}
static float tv_bar(s32 i, u32 cur)                 /* the time left to the next check: 1 just checked, 0 due; -1 not known */
{
	u32 last = g_tv_last[i], gap = g_tv_gap[i];
	float f;
	if (!last || !gap || cur < last) return -1.f;
	f = 1.f - (float)(cur - last) / (float)gap;
	return f < 0.f ? 0.f : f;
}
#ifdef HOST
void host_tv_level(u32 scn) { gw32(ADDR_SCENARIO, scn); tv_reset(scn); }
void host_cam_pf(float *p, float *f) { s32 k; for (k = 0; k < 3; k++) { p[k] = g_cam_p[k]; f[k] = g_cam_f[k]; } }
void host_set_trig(s32 on, s32 flash, s32 hl) { o_trig = on; o_trig_flash = flash; o_trig_hl = hl; }
s32 host_tv_state(s32 i, u32 cur, float *fl) { return tv_state(i, cur, fl); }
float host_tv_bar(s32 i, u32 cur) { return tv_bar(i, cur); }
s32 tv_matrix_call(s32 index, float *m)          /* host: a volume in the world, from its tag (scale 1) */
{
	u32 v = M32(M32(ADDR_SCENARIO) + 0x10C) + (u32)index * 0x44;
	float f[3], u[3];
	s32 k;
	m[0] = 1.f;
	for (k = 0; k < 3; k++) { f[k] = MF(v + 0xC + 4 * (u32)k); u[k] = MF(v + 0x18 + 4 * (u32)k); m[1 + k] = f[k]; m[7 + k] = u[k]; m[10 + k] = MF(v + 0x24 + 4 * (u32)k); }
	m[4] = u[1] * f[2] - u[2] * f[1]; m[5] = u[2] * f[0] - u[0] * f[2]; m[6] = u[0] * f[1] - u[1] * f[0];
	return 1;
}
#else
s32 tv_matrix_call(s32 index, float *m);          /* hooks.S: function_11c380 */
static u32 tv_rd32(const u8 *p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }
static s32 tv_name_read(void *h, u32 io, u32 to, u32 id, char *out)   /* 1: a string exactly as long as the id says */
{
	u32 len = id >> 24, index = id & 0xFFFFFFu, soff = 0, got = 0, k;
	char b[72];
	if (!id || !len || len > 64) return 0;
	if (SetFilePointer_x(h, (s32)(io + index * 4u), 0, 0) == 0xFFFFFFFFu || !ReadFile_x(h, &soff, 4, &got, 0) || got != 4) return 0;
	if (SetFilePointer_x(h, (s32)(to + soff), 0, 0) == 0xFFFFFFFFu || !ReadFile_x(h, b, len + 1, &got, 0) || got != len + 1) return 0;
	if (b[len]) return 0;
	for (k = 0; k < len; k++) if (b[k] < 0x20 || b[k] > 0x7E) return 0;
	if (out) { for (k = 0; k < len && k < TV_NAME - 1; k++) out[k] = b[k]; out[k] = 0; }
	return 1;
}
#endif
static void tv_names_load(u32 arr, s32 n)
{
#ifdef HOST
	(void)arr; (void)n;
	g_tv_names_state = -1;
#else
	char path[72], map[48];
	u8 *hdr;
	void *h;
	u32 got = 0, fsize, io = 0, to = 0, p, q;
	s32 i, k, found = 0, named = 0, at;
	g_tv_names_state = -1;
	if (!ADDR_SETFILEPOINTER) return;
	if (!g_tv_names) { g_tv_names = (char *)VirtualAlloc_x(0, TV_MAX * TV_NAME + 0x800, 0x3000, 4); if (!g_tv_names) return; }
	memset(g_tv_names, 0, TV_MAX * TV_NAME);
	hdr = (u8 *)g_tv_names + TV_MAX * TV_NAME;
	current_map(map);
	{ const char *a = "d:\\maps\\"; for (k = 0; a[k]; k++) path[k] = a[k]; for (i = 0; map[i] && k < 60; i++) path[k++] = map[i]; path[k++] = '.'; path[k++] = 'm'; path[k++] = 'a'; path[k++] = 'p'; path[k] = 0; }
	h = CreateFileA_x(path, 0x80000000, 1, 0, 3, 0x80, 0);
	if (h == (void *)-1) { at = lput(0, "trigger names: couldn't open "); lput(at, path); log_write(); return; }
	if (ReadFile_x(h, hdr, 0x800, &got, 0) && got == 0x800 && tv_rd32(hdr) == 0x68656164u)       /* 'head' */
	{
		fsize = tv_rd32(hdr + 8);
		/* the string table: two offsets near each other in the header - the index (4 bytes a string id) and the strings -
		   tried both ways round; right when the volumes' names read back exactly as long as their ids say */
		for (p = 0x10; p + 16 <= 0x800 && !found; p += 4)
			for (q = 4; q <= 12 && !found; q += 4)
			{
				s32 w;
				for (w = 0; w < 2 && !found; w++)
				{
					u32 a = tv_rd32(hdr + p + (w ? q : 0)), b = tv_rd32(hdr + p + (w ? 0 : q)), tried = 0, ok = 0;
					if (a < 0x800 || b < 0x800 || a >= fsize || b >= fsize || a == b) continue;
					if ((a > b ? a - b : b - a) > 0x1000000u) continue;
					for (i = 0; i < n && tried < 3; i++)
					{
						u32 id = M32(arr + (u32)i * 0x44);
						if (!id || !(id >> 24)) continue;
						tried++;
						if (!tv_name_read(h, a, b, id, 0)) break;
						ok++;
					}
					if (tried && ok == tried) { found = 1; io = a; to = b; }
				}
			}
		if (found) for (i = 0; i < n; i++) if (tv_name_read(h, io, to, M32(arr + (u32)i * 0x44), g_tv_names + i * TV_NAME)) named++;
	}
	CloseHandle_x(h);
	g_tv_names_state = found ? 1 : -1;
	at = lput(0, "trigger names: "); at = lput(at, path);
	if (found) { at = lput(at, " - "); at = lnum(at, (u32)named); at = lput(at, " of "); at = lnum(at, (u32)n); at = lput(at, " named (index "); at = lhex(at, io); at = lput(at, ", strings "); at = lhex(at, to); lput(at, ")"); }
	else lput(at, " - no string table found: shown by number");
	log_write();
#endif
}
/* 6i.126: is a point in solid in the loaded level's collision BSP - its 3D nodes walked from the root to a leaf, as the
 * game's leaf search does (bsp_ray's walk, for one point). Outside the loaded BSP's space is solid. -1: can't tell */
static s32 tv_point_solid(const float *p)
{
	u32 b = M32(ADDR_COLLISION_BSP), nn, nodes, np, planes;
	s32 c = 0, steps = 0, j;
	if (!ptr_ok(b)) return -1;
	nn = M32(b); nodes = M32(b + 4); np = M32(b + 8); planes = M32(b + 0xC);
	if (!nn || nn > 0x7fffff || !ptr_ok(nodes) || !np || np > 0xffff || !ptr_ok(planes)) return -1;
	for (;;)
	{
		u32 node, pi, pa;
		float s = 0.f;
		if (c == -1) return 1;
		if (c < 0) return 0;
		if ((u32)c >= nn || ++steps > 4096) return -1;
		node = nodes + (u32)c * 8;
		pi = (u32)(s32)(s16)M16(node);
		if (pi >= np) return -1;
		pa = planes + pi * 16;
		for (j = 0; j < 3; j++) s += MF(pa + 4 * (u32)j) * p[j];
		s -= MF(pa + 12);
		if (!(s > -1e7f && s < 1e7f)) return -1;
		c = s >= 0.f ? bsp_child(node + 5) : bsp_child(node + 2);
	}
}
/* 6i.126: a volume in the loaded BSP (1) or not (2, drawn dark gray): its centre or any corner in open space - worked out
 * once for each volume while the same BSP is loaded */
static s32 tv_active(s32 i, float c[8][3], const float *ctr)
{
	u32 b = M32(ADDR_COLLISION_BSP);
	s32 k, open = 0, known = 0;
	if (b != g_tv_bsp) { memset(g_tv_act, 0, sizeof(g_tv_act)); g_tv_bsp = b; }
	if (g_tv_act[i]) return g_tv_act[i];
	for (k = -1; k < 8 && !open; k++) { s32 r = tv_point_solid(k < 0 ? ctr : c[k]); if (r == 0) open = 1; if (r >= 0) known = 1; }
	if (!known) return 1;                                                        /* no BSP to tell by: as active */
	g_tv_act[i] = (u8)(open ? 1 : 2);
	return g_tv_act[i];
}
#ifdef HOST
s32 host_tv_point_solid(float x, float y, float z) { float p[3]; p[0] = x; p[1] = y; p[2] = z; return tv_point_solid(p); }
s32 host_camera_read_fov(float fov_rad, float *tan_v)
{
	s32 r, o = g_fov_over;
	float u = g_fov_used;
	g_fov_over = 1; g_fov_used = fov_rad;
	r = camera_read();
	*tan_v = g_tan_v;
	g_fov_over = o; g_fov_used = u;
	return r;
}
#endif

/* ---- 6i.178: HAVOK ISLAND VIEWER (Other Tools, under Sleep Timer) ----
 * What H2EK's havok_debug_start shows in Havok's external viewer, drawn in the game. The retail game has none of the
 * Visual Debugger's code, so nothing is sent anywhere: the same data is read from Havok's world and drawn as an overlay.
 * As that viewer draws it (its two viewers' code, in the H2EK halo2.exe you sent: the broadphase viewer pushes
 * 0xFFFF0000; the simulation island viewer 0xFF0000FF for active islands and 0xFF00FF00 for inactive ones, each island's
 * box the union of its bodies' boxes):
 *   RED    each body's box - the box Havok's broadphase holds for it
 *   GREEN  an island at rest (inactive): the box around its bodies
 *   BLUE   an island awake (active)
 * and two things the external viewer does not have: the island's body count over its box, and the island's box WHITE
 * for a sixth of a second when islands merge or one splits (a body whose island last tick differs from another body's in
 * the same island now: a merge; from a body's still in its old island: a split).
 * Where it is read (v1.0; the layouts from the halo2-decompiled sources, the box decode confirmed against the game's
 * own function 0x2e0020):
 *   the world (*ADDR_HAVOK_WORLD_PTR): active islands +0x08 (data) / +0x0C (count); inactive +0x14 / +0x18; the
 *     broadphase +0xC4. The FIXED island (+0x2C: everything that never moves) is not walked - its bodies are not drawn.
 *   an island: its bodies +0x3C / +0x40.       a body: its broadphase handle at +0x1C, whose first word is its box's
 *     number in the broadphase; trusted only if that box's owner (+0xC of the 16-byte element) points back at the handle.
 *   the broadphase: lower offset +0x10, upper offset +0x20, scale +0x30 (four floats each), the boxes +0x40, the three
 *     axes' sorted endpoints +0x4C (x) / +0x58 (y) / +0x64 (z), 4 bytes an endpoint, its 16-bit value first. A box holds
 *     where its six ends sit: words 0 / 1 lower y / z, 2 / 3 upper y / z, 4 / 5 lower / upper x. A coordinate =
 *     value / scale - offset.
 * NOT THROUGH WALLS: the overlay is drawn flat over the finished picture, so a box is drawn only if the straight line
 * from the camera to its centre is clear of the level (bsp_ray - the walk of the level's collision the third person
 * camera uses). HIV_LOS of these a tick, in turn, each answer kept until its turn comes round; a body just seen starts
 * hidden. An island's box is drawn if any of its bodies is. Level geometry only: a vehicle or a crate hides nothing,
 * and a box is all drawn or not at all.
 * Kept in check: bodies within HIV_RANGE of the camera, HIV_BODIES at most (the nearest islands first come first
 * served), HIV_ISLANDS islands. ---- */
#define HIV_BODIES 96
#define HIV_ISLANDS 32
#define HIV_RANGE 40.f
#define HIV_LOS 16
#define HIV_FLASH 5                              /* ticks: a sixth of a second (6i.179: was 15 - your note: shorter) */
/* 6i.186: your note - the boxes stutter as you walk while the Ghost Viewer's capsule stays with you. The boxes are read
 * once a tick (30 a second) and were drawn where the tick left them; the picture between two ticks is the in-between
 * frames' blend of the tick before and this one. So each box now keeps where it was the tick before (pmn / pmx) and is
 * drawn at the same blend the world is (g_ip_a, when a frame is in-between; else where the tick left it). */
typedef struct { u32 e, isl; float mn[3], mx[3], pmn[3], pmx[3]; u8 vis, known; } s_hiv_body;
typedef struct { u32 isl; float mn[3], mx[3], pmn[3], pmx[3]; s32 bodies; u8 awake, flash, vis; } s_hiv_isl;
static s_hiv_body g_hiv_b[HIV_BODIES], g_hiv_pb[HIV_BODIES];   /* this tick's; last tick's (for the merges and the answers kept) */
static s_hiv_isl g_hiv_i[HIV_ISLANDS], g_hiv_pi[HIV_ISLANDS];   /* this tick's; last tick's (for where each was) */
static s32 g_hiv_npi;
static u32 g_hiv_fl_isl[8]; static u8 g_hiv_fl_t[8];           /* islands flashing: which, and the ticks left */
static s32 g_hiv_nb, g_hiv_npb, g_hiv_ni, g_hiv_los_at, g_hiv_seen, g_hiv_bad, g_hiv_logged;
static float bsp_ray(const float *p0, const float *p1);
#ifdef HOST
static s32 g_host_hiv_wall_on; static float g_host_hiv_wall;             /* the tests' wall: everything beyond this x is behind it */
void host_hiv_wall(s32 on, float x) { g_host_hiv_wall_on = on; g_host_hiv_wall = x; }
#define HIV_RAY(c) (g_host_hiv_wall_on ? ((c)[0] > g_host_hiv_wall ? 0.5f : 2.f) : bsp_ray(g_cam_p, (c)))
#else
#define HIV_RAY(c) bsp_ray(g_cam_p, (c))
#endif
static s32 hiv_box(u32 bp, u32 e, float *mn, float *mx)       /* a body's box out of the broadphase: 1 if it is there and is this body's */
{
	u32 id = M32(e + 0x1C), els = M32(bp + 0x40), n = M32(bp + 0x44), el, ax[3], k;
	static const u8 lo[3] = { 8, 0, 2 }, hi[3] = { 10, 4, 6 };            /* the element's words for x, y, z: lower, upper */
	if (!ptr_ok(els) || id >= n || n > 0x10000u) return 0;
	el = els + id * 16u;
	if (M32(el + 12) != e + 0x1Cu) return 0;                              /* not this body's box */
	ax[0] = M32(bp + 0x4C); ax[1] = M32(bp + 0x58); ax[2] = M32(bp + 0x64);
	for (k = 0; k < 3; k++)
	{
		float sc = MF(bp + 0x30 + 4u * k);
		if (!ptr_ok(ax[k]) || !(sc > 1.0e-6f && sc < 1.0e9f)) return 0;
		mn[k] = (float)M16(ax[k] + (u32)M16(el + lo[k]) * 4u) / sc - MF(bp + 0x10 + 4u * k);
		mx[k] = (float)M16(ax[k] + (u32)M16(el + hi[k]) * 4u) / sc - MF(bp + 0x20 + 4u * k);
		if (!(mn[k] > -1.0e6f && mx[k] < 1.0e6f && mx[k] >= mn[k])) return 0;
	}
	return 1;
}
static void hiv_flash(u32 isl)
{
	s32 k, free_k = -1;
	for (k = 0; k < 8; k++) { if (g_hiv_fl_t[k] && g_hiv_fl_isl[k] == isl) { g_hiv_fl_t[k] = HIV_FLASH; return; } if (!g_hiv_fl_t[k] && free_k < 0) free_k = k; }
	if (free_k >= 0) { g_hiv_fl_isl[free_k] = isl; g_hiv_fl_t[free_k] = HIV_FLASH; }
}
static void hiv_tick(void)                       /* the tick hook: the islands and bodies near the camera, as they are now */
{
	u32 world = M32(ADDR_HAVOK_WORLD_PTR), bp, list, cnt;
	s32 pass, i, j, k, nb = 0, ni = 0;
	g_hiv_nb = 0; g_hiv_ni = 0; g_hiv_seen = 0; g_hiv_bad = 0;
	if (!o_hiv) { g_hiv_npb = 0; g_hiv_npi = 0; for (k = 0; k < 8; k++) g_hiv_fl_t[k] = 0; return; }
	if (!ptr_ok(world) || !ptr_ok(bp = M32(world + 0xC4))) return;
	for (k = 0; k < 8; k++) if (g_hiv_fl_t[k]) g_hiv_fl_t[k]--;
	for (pass = 0; pass < 2; pass++)             /* the active islands, then the inactive */
	{
		list = M32(world + (pass ? 0x14u : 0x08u)); cnt = M32(world + (pass ? 0x18u : 0x0Cu));
		if (!ptr_ok(list) || cnt > 4096u) continue;
		for (i = 0; i < (s32)cnt; i++)
		{
			u32 isl = M32(list + 4u * (u32)i), ents, n;
			s_hiv_isl I;
			s32 first = nb, near = 0, got = 0;
			if (!ptr_ok(isl)) continue;
			ents = M32(isl + 0x3C); n = M32(isl + 0x40);
			if (!ptr_ok(ents) || !n || n > 4096u) continue;
			I.isl = isl; I.bodies = (s32)n; I.awake = (u8)!pass; I.flash = 0; I.vis = 0;
			for (j = 0; j < (s32)n; j++)
			{
				u32 e = M32(ents + 4u * (u32)j);
				float mn[3], mx[3], d2 = 0.f;
				if (!ptr_ok(e)) continue;
				g_hiv_seen++;
				if (!hiv_box(bp, e, mn, mx)) { g_hiv_bad++; continue; }
				for (k = 0; k < 3; k++)
				{
					float c = (mn[k] + mx[k]) * 0.5f - g_cam_p[k];
					d2 += c * c;
					if (!got || mn[k] < I.mn[k]) I.mn[k] = mn[k];
					if (!got || mx[k] > I.mx[k]) I.mx[k] = mx[k];
				}
				got = 1;
				if (d2 > HIV_RANGE * HIV_RANGE || nb >= HIV_BODIES) continue;
				g_hiv_b[nb].e = e; g_hiv_b[nb].isl = isl; g_hiv_b[nb].vis = 0; g_hiv_b[nb].known = 0;
				for (k = 0; k < 3; k++) { g_hiv_b[nb].mn[k] = mn[k]; g_hiv_b[nb].mx[k] = mx[k]; }
				nb++; near = 1;
			}
			if (!near) { nb = first; continue; }                             /* none of it near: not shown */
			if (ni < HIV_ISLANDS) g_hiv_i[ni++] = I; else nb = first;
		}
	}
	/* each body against last tick: the answer kept (seen or not), and its island then - for the merges and splits */
	{
		static u32 now_of[HIV_BODIES];                                      /* last tick's bodies: the island each is in now (0: not listed now) */
		for (j = 0; j < g_hiv_npb; j++)
		{
			now_of[j] = 0;
			for (i = 0; i < nb; i++) if (g_hiv_b[i].e == g_hiv_pb[j].e)
			{
				now_of[j] = g_hiv_b[i].isl; g_hiv_b[i].vis = g_hiv_pb[j].vis; g_hiv_b[i].known = g_hiv_pb[j].known;
				for (k = 0; k < 3; k++) { g_hiv_b[i].pmn[k] = g_hiv_pb[j].mn[k]; g_hiv_b[i].pmx[k] = g_hiv_pb[j].mx[k]; }   /* 6i.186: where it was */
				g_hiv_b[i].known |= 2;
				break;
			}
		}
		for (j = 0; j < g_hiv_npb; j++)
		{
			u32 was = g_hiv_pb[j].isl, now = now_of[j];
			s32 q, merge = 0, split = 0;
			if (!now || now == was) continue;                                 /* gone, or where it was */
			for (q = 0; q < g_hiv_npb; q++)
			{
				if (q == j || !now_of[q]) continue;
				if (now_of[q] == now && g_hiv_pb[q].isl != was) merge = 1;     /* with it now, was elsewhere: islands merged */
				if (g_hiv_pb[q].isl == was && now_of[q] != now) split = 1;     /* was with it, is elsewhere now: an island split */
			}
			if (merge || split) hiv_flash(now);
		}
	}
	/* the line of sight: HIV_LOS bodies a tick, in turn */
	for (k = 0; k < HIV_LOS && nb > 0; k++)
	{
		s_hiv_body *b;
		float c[3];
		if (g_hiv_los_at >= nb) g_hiv_los_at = 0;
		b = &g_hiv_b[g_hiv_los_at++];
		for (j = 0; j < 3; j++) c[j] = (b->mn[j] + b->mx[j]) * 0.5f;
		b->vis = (u8)!(HIV_RAY(c) <= 1.f); b->known |= 1;
	}
	for (i = 0; i < ni; i++)
	{
		for (j = 0; j < nb; j++) if (g_hiv_b[j].isl == g_hiv_i[i].isl && g_hiv_b[j].vis) { g_hiv_i[i].vis = 1; break; }
		for (k = 0; k < 8; k++) if (g_hiv_fl_t[k] && g_hiv_fl_isl[k] == g_hiv_i[i].isl) g_hiv_i[i].flash = 1;
	}
	for (i = 0; i < nb; i++)                                                 /* 6i.186: one not seen last tick was where it is */
	{
		if (!(g_hiv_b[i].known & 2)) for (k = 0; k < 3; k++) { g_hiv_b[i].pmn[k] = g_hiv_b[i].mn[k]; g_hiv_b[i].pmx[k] = g_hiv_b[i].mx[k]; }
		g_hiv_b[i].known &= 1;
	}
	for (i = 0; i < ni; i++)
	{
		s32 was = -1;
		for (j = 0; j < g_hiv_npi; j++) if (g_hiv_pi[j].isl == g_hiv_i[i].isl) { was = j; break; }
		for (k = 0; k < 3; k++) { g_hiv_i[i].pmn[k] = was >= 0 ? g_hiv_pi[was].mn[k] : g_hiv_i[i].mn[k]; g_hiv_i[i].pmx[k] = was >= 0 ? g_hiv_pi[was].mx[k] : g_hiv_i[i].mx[k]; }
	}
	for (i = 0; i < ni; i++) g_hiv_pi[i] = g_hiv_i[i];
	g_hiv_npi = ni;
	for (i = 0; i < nb; i++) g_hiv_pb[i] = g_hiv_b[i];
	g_hiv_npb = nb; g_hiv_nb = nb; g_hiv_ni = ni;
	if (!g_hiv_logged && g_hiv_seen)             /* once: what it found, for the caps */
	{
		s32 at = lput(0, "havok island viewer: active islands "); at = lnum(at, M32(world + 0x0C)); at = lput(at, ", inactive "); at = lnum(at, M32(world + 0x18));
		at = lput(at, "; bodies in them "); at = lnum(at, (u32)g_hiv_seen); at = lput(at, " (boxes not read: "); at = lnum(at, (u32)g_hiv_bad);
		at = lput(at, "); within range and shown: bodies "); at = lnum(at, (u32)nb); at = lput(at, ", islands "); lnum(at, (u32)ni);
		log_write(); g_hiv_logged = 1;
	}
}
static void box3(const float *mn, const float *mx);
static s32 g_hiv_h = 16;
static void hiv_draw(void)                       /* the frame: red bodies, then the islands (blue awake, green at rest, white flashing), then the counts */
{
	static const color4f c_hiv = { 1.0f, 1.0f, 1.0f, 1.0f };
	s32 i, pass, k;
	float a = g_ip_frame ? g_ip_a : 1.f, lo[3], hi[3];                       /* 6i.186: between the tick before and this one, as the world is drawn */
	if (!g_hiv_nb && !g_hiv_ni) return;
	if (!(a >= 0.f && a <= 1.f)) a = 1.f;
#define HIV_AT(B) do { for (k = 0; k < 3; k++) { lo[k] = (B).pmn[k] + ((B).mn[k] - (B).pmn[k]) * a; hi[k] = (B).pmx[k] + ((B).mx[k] - (B).pmx[k]) * a; } } while (0)
	g_line_half = 0.5f;
	gfx_begin(1.f, 1.f, 0.f, 0.f);
	for (i = 0; i < g_hiv_nb; i++) if (g_hiv_b[i].vis) { HIV_AT(g_hiv_b[i]); box3(lo, hi); }
	gfx_end();
	for (pass = 0; pass < 3; pass++)
	{
		if (pass == 0) gfx_begin(1.f, 0.f, 1.f, 0.f); else if (pass == 1) gfx_begin(1.f, 0.f, 0.f, 1.f); else gfx_begin(1.f, 1.f, 1.f, 1.f);
		for (i = 0; i < g_hiv_ni; i++)
		{
			const s_hiv_isl *I = &g_hiv_i[i];
			if (!I->vis || (I->flash ? 2 : I->awake ? 1 : 0) != pass) continue;
			HIV_AT(*I); box3(lo, hi);
		}
		gfx_end();
	}
	g_line_half = 1.f;
	for (i = 0; i < g_hiv_ni; i++)               /* the body count, over the island's box */
	{
		const s_hiv_isl *I = &g_hiv_i[i];
		float top[3], sx, sy, sd;
		if (!I->vis) continue;
		HIV_AT(*I);
		top[0] = (lo[0] + hi[0]) * 0.5f; top[1] = (lo[1] + hi[1]) * 0.5f; top[2] = hi[2];
		if (view_depth(top) < 0.5f) continue;
		project(top, &sx, &sy, &sd);
		if (!(sx > -50.f && sx < 690.f && sy > -20.f && sy < 500.f)) continue;
		put_int(0, I->bodies);
		line_at((s32)sx + g_vp_left - 40, (s32)sy + g_vp_top - g_hiv_h, 80, &c_hiv, 2);
	}
}
#ifdef HOST
void host_set_hiv(s32 on) { o_hiv = on; }
void host_hiv_tick(void) { hiv_tick(); }
void host_hiv_draw(void) { hiv_draw(); }
void host_hiv_cam(float x, float y, float z) { g_cam_p[0] = x; g_cam_p[1] = y; g_cam_p[2] = z; }
s32 host_hiv(s32 what, s32 i) { return what == 0 ? g_hiv_nb : what == 1 ? g_hiv_ni : what == 2 ? g_hiv_b[i].vis : what == 3 ? g_hiv_i[i].awake : what == 4 ? g_hiv_i[i].flash : what == 5 ? g_hiv_i[i].bodies : what == 6 ? g_hiv_i[i].vis : what == 7 ? g_hiv_bad : g_hiv_seen; }
void host_hiv_box(s32 isl, s32 i, float *six) { const float *a = isl ? g_hiv_i[i].mn : g_hiv_b[i].mn, *b = isl ? g_hiv_i[i].mx : g_hiv_b[i].mx; s32 k; for (k = 0; k < 3; k++) { six[k] = a[k]; six[3 + k] = b[k]; } }
void host_hiv_was(s32 isl, s32 i, float *six) { const float *a = isl ? g_hiv_i[i].pmn : g_hiv_b[i].pmn, *b = isl ? g_hiv_i[i].pmx : g_hiv_b[i].pmx; s32 k; for (k = 0; k < 3; k++) { six[k] = a[k]; six[3 + k] = b[k]; } }
#endif
static void tv_draw(void)
{
	static const u8 E[12][2] = { {0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7} };
	static const color4f c_tv_name = { 1.0f, 1.0f, 0.95f, 0.45f }, c_tv_gray = { 1.0f, 0.45f, 0.45f, 0.45f };
	u32 scn = M32(ADDR_SCENARIO), arr, t, cur;
	s32 n, i, k, pass;
	if (!ADDR_SCENARIO || !ptr_ok(scn)) return;
	if (scn != g_tv_scn) tv_reset(scn);
	n = (s32)M32(scn + 0x108); arr = M32(scn + 0x10C);
	if (n <= 0 || n > 4096 || !ptr_ok(arr)) return;
	if (n > TV_MAX) n = TV_MAX;
	t = M32(ADDR_GAME_TIME_PTR); cur = ptr_ok(t) ? M32(t + 8) + 1u : 0u;
	if (o_trig_names && !g_tv_names_state) tv_names_load(arr, n);
	g_line_half = 0.5f;                                  /* 6i.126: 1 pixel wide (2 before - your note) */
	for (pass = 0; pass < 4; pass++)                     /* yellow ones, red ones, each flashing one, then the gray (not in the loaded BSP) */
	{
		if (pass == 0) gfx_begin(1.f, 1.f, 1.f, 0.2f);
		else if (pass == 1) gfx_begin(1.f, 1.f, 0.2f, 0.2f);
		else if (pass == 3) gfx_begin(1.f, 0.3f, 0.3f, 0.3f);              /* 6i.126: dark gray */
		for (i = 0; i < n; i++)
		{
			float m[13], ex[3], c[8][3], ctr[3], d2 = 0.f, r2, fl;
			u32 v = arr + (u32)i * 0x44;
			s32 st, q;
			st = tv_state(i, cur, &fl);
			if ((pass == 0 && st != 0) || (pass == 1 && st != 2) || (pass == 2 && st != 1) || (pass == 3 && st != 0)) continue;
			if (!tv_matrix_call(i, m) || !(m[0] > 0.001f && m[0] < 1000.f)) continue;
			for (k = 0; k < 3; k++) ex[k] = MF(v + 0x30 + 4 * (u32)k) * m[0];
			if (!(ex[0] > 0.f && ex[1] > 0.f && ex[2] > 0.f && ex[0] < 5000.f && ex[1] < 5000.f && ex[2] < 5000.f)) continue;
			for (k = 0; k < 8; k++)
				for (q = 0; q < 3; q++)
					c[k][q] = m[10 + q] + ((k & 1) ? ex[0] * m[1 + q] : 0.f) + ((k & 2) ? ex[1] * m[4 + q] : 0.f) + ((k & 4) ? ex[2] * m[7 + q] : 0.f);
			for (k = 0; k < 3; k++) { ctr[k] = (c[0][k] + c[7][k]) * 0.5f; d2 += (ctr[k] - g_cam_p[k]) * (ctr[k] - g_cam_p[k]); }
			r2 = (ex[0] * ex[0] + ex[1] * ex[1] + ex[2] * ex[2]) * 0.25f;
			if (fsqrt(d2) - fsqrt(r2) > 200.f) continue;                            /* farther than 200 units */
			if (st == 0 && (tv_active(i, c, ctr) == 2) != (pass == 3)) continue;     /* 6i.126: yellow in the loaded BSP, gray not */
			if (pass == 2) gfx_begin(1.f, 1.f, 1.f - 0.8f * fl, 0.2f);                /* red, fading back to yellow */
			for (k = 0; k < 12; k++) gfx_line3(c[E[k][0]], c[E[k][1]]);
			if (pass == 2) gfx_end();
		}
		if (pass != 2) gfx_end();
	}
	g_line_half = 1.f;
	if (!o_trig_names && !o_trig_bars) return;
	for (i = 0; i < n; i++)                               /* the names and the bars: within 100 units, in front */
	{
		float m[13], ex[3], ctr[3], d2 = 0.f, sx, sy, sd, b;
		u32 v = arr + (u32)i * 0x44;
		if (!tv_matrix_call(i, m) || !(m[0] > 0.001f && m[0] < 1000.f)) continue;
		for (k = 0; k < 3; k++) ex[k] = MF(v + 0x30 + 4 * (u32)k) * m[0];
		for (k = 0; k < 3; k++) { ctr[k] = m[10 + k] + 0.5f * (ex[0] * m[1 + k] + ex[1] * m[4 + k] + ex[2] * m[7 + k]); d2 += (ctr[k] - g_cam_p[k]) * (ctr[k] - g_cam_p[k]); }
		if (d2 > 100.f * 100.f || view_depth(ctr) < 0.5f) continue;
		project(ctr, &sx, &sy, &sd);
		if (!(sx > -200.f && sx < 840.f && sy > -100.f && sy < 580.f)) continue;
		if (o_trig_names)
		{
			if (g_tv_names_state == 1 && g_tv_names && g_tv_names[i * TV_NAME]) put(0, g_tv_names + i * TV_NAME);
			else { s32 at = put(0, "trigger "); put_int(at, i); }
			line_at(fti(sx) + g_vp_left - 150, fti(sy) + g_vp_top - g_tv_h, 300, g_tv_act[i] == 2 ? &c_tv_gray : &c_tv_name, 2);
		}
		b = o_trig_bars ? tv_bar(i, cur) : -1.f;
		if (b >= 0.f)
		{
			s32 x0 = fti(sx) - 30, y0 = fti(sy) + 2;
			gfx_begin(0.6f, 0.f, 0.f, 0.f); gfx_rect(x0 - 1, y0 - 1, x0 + 61, y0 + 5); gfx_end();
			gfx_begin(1.f, 1.f, 0.95f, 0.45f); gfx_rect(x0, y0, x0 + (s32)(60.f * b + 0.5f), y0 + 4); gfx_end();
		}
	}
}
void spooky_frame(void)                         /* 6i.3: timed (the mod's own share of the frame) */
{
	u32 t = tsc_lo();
	spooky_frame_body();
	g_pf_mod += tsc_lo() - t;
}
/* ---- 6i.167: the menu behind the game's pause menu. The game draws its windows (the pause menu, its dialogs) after
 * the HUD text pass where the mod draws: in the pass's function, v1.0 0x2c86e is the HUD text (hud_hook), 0x2c884 the
 * window manager's render (function_147af1), 0x2c889 the pass's closing call (function_13c20, no arguments). While a
 * window of the game's is in use the menu is now drawn from that last call's hook (hooks.S late_hook ->
 * spooky_after_windows), after the windows - in front of them; otherwise where it always was. The menu still works
 * while the game is paused, as before. menu_draw is the menu's drawing as it stood in spooky_frame_body. ---- */
static s32 g_menu_late, g_menu_late_h, g_menu_late_cam;   /* drawn after the game's windows this frame; with this line height and camera flag */
static void menu_draw(s32 height, s32 cam)
{
		s32 x = 60, y = 48, i, rows, first, last, shown;
		const s_menu *mn = &g_menus[g_menu];
		u32 t_menu = tsc_lo();                                             /* 6i.150: the open menu's own drawing time (the 2-5 fps it costs) */
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
				gfx_rect(pl, y - 8 - g_vp_top, g_vp_w - pl, y + height * (shown + 4 + (g_menu == 13 || g_menu == LC_MENU)) - g_vp_top);   /* 6i.137 / 6i.139: + the extra lines */
			}
			gfx_end();
		}
		put(0, mn->title);
		line(x, y, height, &c_title);
		y += height + height / 2;
		/* 6i.152: the memory check's five lines (6i.139-6i.149) are gone from the page - the 128 MB work is done; the fit still
		 * decides each step (the toast says) */
		if (g_menu == 13 || g_menu == LC_MENU)                               /* 6i.137: the restart question - what this start has; 6i.201: Larger Caches' too */
		{
			if (g_menu == LC_MENU) put(0, o_big_cache ? "Larger Caches is currently: on" : "Larger Caches is currently: off");
			else
			put(0, spooky_triple ? "Triple Buffering is currently: on" : "Triple Buffering is currently: off");
			line(x, y, height, &c_item);
			y += height + height / 2;
		}
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
			if (it->kind == K_TOGGLE) at = put(at, it->value == &o_big_cache && !big_cache_can() ? ": not available (64 MB)" : *it->value ? ": on" : ": off");   /* 6i.192: Larger Caches says so where it cannot be had */
			else if (it->kind == K_INT || it->kind == K_INT10)
			{
				at = put(at, ": ");
				if (it->value == &o_scale || it->value == &o_speed) { at = put_int(at, *it->value / 10); at = put(at, "."); at = put_int(at, *it->value % 10); }
				else if (it->kind == K_INT10 && *it->value == 0) at = put(at, "Max");       /* 6h.18: the range's Max */
				else if (it->value == &o_fx_copy) at = put(at, *it->value ? "fast" : "game's");   /* 6i.60 */
				else if (it->value == &o_water_refr) at = put(at, *it->value >= 3 ? "off" : *it->value == 2 ? "late" : *it->value ? "fast" : "game's");   /* 6i.59 */
				else if (it->value == &o_rscale) { at = put_int(at, *it->value * 10); at = put(at, "%"); }   /* 6i.61 */
				else if (it->value == &o_lights) at = put(at, *it->value ? "fast" : "game's");   /* 6i.77 */
				else if (it->value == &o_lightf) at = put(at, *it->value >= 2 ? "faster" : *it->value ? "fast" : "game's");   /* 6i.85; 6i.90 */
				else if (it->value == &o_particles) at = put(at, *it->value == 1 ? "fast" : *it->value == 2 ? "faster" : "game's");   /* 6i.73 */
				else if (it->value == &o_water_rate) at = put(at, *it->value == 1 ? "half rate" : *it->value == 2 ? "off" : "every frame");   /* 6i.73 */
				else if (it->value == &o_rs_point) at = put(at, *it->value == 1 ? "1 before 2nd UI" : *it->value == 2 ? "2 after 2nd UI" : *it->value == 3 ? "3 before 13870" : *it->value == 4 ? "4 before 13630" : "5 HUD view");   /* 6i.71 */
				else if (it->value == &o_aniso) at = put(at, *it->value >= 3 ? "full" : *it->value == 2 ? "2x" : *it->value ? "2x fast" : "off");   /* 6i.53 */
				else if (it->value == &o_wire) at = put(at, *it->value == 2 ? "xray" : *it->value ? "on" : "off");   /* 6i.52 */
				else if (it->value == &o_vsync) at = put(at, *it->value == 2 ? "30 fps" : *it->value ? "on" : "off");   /* 6i.176 */
				else if (it->value == &o_trate) at = put_int(at, g_tick_rates[*it->value < 0 ? 0 : *it->value > 2 ? 2 : *it->value]);   /* 6i.120 */
				else if (it->value == &o_fov) { if (*it->value <= 0) at = put(at, "game's"); else { at = put_int(at, *it->value); if (*it->value == g_fov_def) at = put(at, "*");
					else if (g_fov_why) at = put(at, g_fov_why == 1 ? "  (not in use: no camera)" : g_fov_why == 2 ? "  (not in use: the game's angle unreadable)" : g_fov_why == 3 ? "  (not in use: on foot only)" : g_fov_why == 4 ? "  (not in use: the game's own not read yet)" : "  (not in use: out of range ");
					if (*it->value != g_fov_def && g_fov_why == 5) at = put(at, ")");   /* (6i.187: the four numbers that followed are gone - making them spoiled the angle on the 360) */
					} }   /* 6i.120: the game's own marked; 6i.172: why not, if not (6i.224: the legs' report after it is gone) */
				else if (it->value == &o_fps_cap) at = put_int(at, g_fps_caps[*it->value < 0 ? 0 : *it->value > 12 ? 12 : *it->value]);   /* 6i.46 */
				else at = put_int(at, *it->value);
			}
			line(x, y, height, i == g_selected ? &c_selected : &c_item);
			y += height;
		}
		y += height / 2;
		put(0, g_menu ? "D-pad: select/change   A: toggle/enter   B: back" : "D-pad: select/change   A: toggle/enter   B or Back+Up: close");
		line(x, y, height, &c_hint);
		if (g_toast_frames > 0)                                              /* 6h.5: an action that leaves the menu open: its result - the line under the help line (6i.155: one row down, not one and a half: that half row hung below the panel) */
		{
			put(0, g_toast);
			line(x, y + height, height, &c_selected);
			toast_step();
		}
		g_menu_us = (tsc_lo() - t_menu) / 733u;
}
#ifdef HOST
static s32 g_host_late;
void host_set_late(s32 on) { g_host_late = on; }
static s32 menu_late_ok(void) { return g_host_late; }
#else
static s32 menu_late_ok(void) { return ADDR_AFTER_WINDOWS != 0; }
#endif
void spooky_after_windows(void)                  /* hooks.S late_hook: after the game's windows are drawn */
{
	u8 saved[0x64];
	if (!g_menu_late) return;
	g_menu_late = 0;
	if (!g_open || g_peek || g_dlg) return;
	memcpy(saved, DRAW_STRING_GLOBALS, sizeof(saved));
	text_style_clear();
	*(s32 *)(DRAW_STRING_GLOBALS + 0x00) = 9;
	*(u32 *)(DRAW_STRING_GLOBALS + 0x04) = 1;
	*(s32 *)(DRAW_STRING_GLOBALS + 0x08) = -1;
	*(s32 *)(DRAW_STRING_GLOBALS + 0x0c) = 0;
	menu_draw(g_menu_late_h, g_menu_late_cam);
	memcpy(DRAW_STRING_GLOBALS, saved, sizeof(saved));
}
static void spooky_frame_body(void)
{
	s32 menu = on_main_menu();                                            /* (6i.198: the menu is no longer closed on reaching the game's main menu) */
	g_frames++;
	lc_default();                                                          /* v2.0: Larger Caches' default, once the console's memory is known */
	g_count_hud++;
	g_dlg = g_open ? reconnect_dialog_up() : 0;                           /* 6i.160: (also with the menu's own pad unplugged - the usual reason for the dialog - when no read of it comes) */
	if (g_open && g_menu >= OS_MENU && g_menu <= OS_MENU_LAST) os_check();   /* 6i.158: Object Spawning's lists are this level's */
	ct_frame(menu);                                                       /* 6i.156: the Campaign Timer's run ends at the main menu */
	if (o_unlock_fps) g_hud30 += (u32)g_steps;   /* 6i: the search and spots schedules' clock while unlocked */
	s32 hint = o_hint && on_main_menu();         /* the "Back + D-pad Up" hint: on the game's main menu only (its state, 3) -
	                                                 never in a level, dead or alive (6f.2 showed it while dead); the launcher's
	                                                 "Hide hint line" hides it entirely. 6h.6: on every screen of it (lobbies
	                                                 too), drawn as low as the info line in a game, below the menus' panels */
	s32 hud = o_sb_hud && g_sb_charged >= 0;     /* in a level, with a player */
	s32 marker = o_marker && g_sb_object && g_sb_charged >= 0;
	s32 seams = o_sb_hud && g_sb_object && g_sb_charged >= 0;
	{ static s32 was_on; if (seams && !was_on) { g_trace_until = g_t30 + 90; g_trace_on = 1; } was_on = seams; }   /* turned on: 3 s of trace, from this very search */     /* the Superbounce Viewer: the search, the landing, the trajectory -
	                                                                  "Seams near you" only draws the seams themselves (5e.2 hid all of it) */
#if SPOOKY_PROBE
	rs_probe_step();                                                      /* 6i.130: Render Scale above 100 %: the memory it would need (the menu's change) */
#endif
	cache_log();                                                          /* 6i.183 */
	if (!g_open && !hint && !hud && !marker && !seams && !o_ghost && !o_oob && !o_sleep && !o_ctimer && !o_pred && !g_toast_frames && !o_fps_show && !(o_stored_vel && g_sv_pinned) && !o_trig && !o_hiv) return;   /* 6g.2: and a message pending (Launch's speed, Passion Check...): it showed only with another overlay on */   /* (6a.2 left out the Ghost Viewer and the
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
	if (o_fps_show > 1) o_fps_show = 1;                                    /* 6i.220: "detailed" is gone - a saved 2 is "on" */
	if (o_fps_show && !on_main_menu())                                     /* 6i.42: the FPS Counter, top-left (the menu's title starts lower); 6i.189: not on the game's main menu (your note) */
	{
		u32 t_hud = tsc_lo();                                              /* 6i.151: these four long lines' own time, next to the menu's (~25 short lines): per call or per glyph? */
		s32 a = put(0, "FPS: "), k;
		a = put_int(a, (s32)g_fps_shown);                                  /* 6i.57: just the figure */
		if (ad_on() && fps_unlocked()) { a = put(a, "  res "); a = put_int(a, g_rs_eff); a = put(a, "%"); }   /* 6i.130 */
		line(9, 10, height, &c_hint);
		g_hud_us = (tsc_lo() - t_hud) / 733u;
	}
	if (g_open && !g_peek && !g_dlg)                                       /* (6i.159: held aside with White - the game's screen, as with the menu closed) */
	{
		if (menu_late_ok() && (ui_window_in_use() || on_main_menu())) { g_menu_late = 1; g_menu_late_h = height; g_menu_late_cam = cam; g_menu_us = 0; }   /* 6i.167: a window of the game's is up - after it, in front */
		else { g_menu_late = 0; menu_draw(height, cam); }
	}
	else
	{
		s32 y_hint = 480 - 36 - height, y_charge = y_hint - height - 4, y_speed = y_charge - height;
		g_menu_us = 0;
		if (seams && cam)
		{
			u32 t0 = tsc_lo(), dt;
			/* 6i: with Unlock FPS on, the schedule runs in 30 Hz steps (one every other frame at 60), so the search costs
			 * what it did a second and steps with the ticks; frames between them only draw (the path still followed) */
			u32 hc = o_unlock_fps ? g_hud30 : g_count_hud;
			s32 fresh = !o_unlock_fps || g_steps > 0;
#ifdef HOST
			s32 search = fresh;
#else
			s32 search = fresh && !(hc % (u32)g_search_div);   /* every Nth frame (2, slower hardware more); drawn every frame */
#endif
			s32 step;                                        /* -1 none this frame; 9 all four; 0 seams / 1 landing / 2 rating / 3 view */
#ifdef HOST
			step = search ? 9 : -1;
			if (g_host_step_div && fresh) { u32 m = hc % (u32)g_host_step_div; step = g_host_step_div == 3 ? (m == 2 ? 23 : (s32)m) : m < 4 ? (s32)m : -1; }   /* as on a slow console */
#else
			if (g_search_div <= 2) step = search ? 9 : -1;
			else if (!fresh) step = -1;
			else { u32 m = hc % (u32)g_search_div; step = g_search_div == 3 ? (m == 2 ? 23 : (s32)m) : m < 4 ? (s32)m : -1; }   /* 3: rating and view together (6h.20) */
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
				land_follow();                                                /* 6h.37: the circle follows the path too */
			}
			else if (step == 0 && g_sb_airborne) land_follow();             /* (the landing step's own fresh path; its result is drawn 3 frames on) */
			if (o_seams) { u32 td = tsc_lo(); seams_draw_d8(); g_cyc_draw += tsc_lo() - td; }
			ld_swap(); arc_draw(); ld_swap();                 /* the landing drawn as last finished, with its seams */
			dt = tsc_lo() - t0;
			g_seam_cyc_sum += dt; g_seam_cyc_n++;
			if (dt > g_seam_cyc_max) g_seam_cyc_max = dt;
		}
		if (o_trig && cam && !on_main_menu())                               /* 6i.124: the Trigger Volume Overlay */
		{
			g_tv_h = height;
			if (guarded_call(tv_draw)) { o_trig = 0; toast("Trigger Volume Overlay: a bad read was caught - turned off"); }
		}
		if (o_hiv && cam && !on_main_menu())                                /* 6i.178: the Havok Island Viewer */
		{
			g_hiv_h = height;
			if (guarded_call(hiv_draw)) { o_hiv = 0; toast("Havok Island Viewer: a bad read was caught - turned off"); }
		}
		if (o_stored_vel && g_sv_pinned && !on_main_menu())                 /* 6i.120: Show Stored Velocity - on the line above your speed's */
		{
			s32 t10 = (s32)(g_sv_speed * 10.f + 0.5f), at = put(0, "STORED  ");
			at = put_int(at, t10 / 10); at = put(at, "."); at = put_int(at, t10 % 10); put(at, " u/s");
			line_at(170, y_speed - height, 300, &c_stored, 2);
			if (cam)                                                            /* 6i.181: and the way it points */
			{
				float a[3], b[3];
				if (sv_arrow(a, b)) { g_line_half = 1.5f; gfx_begin(c_stored.a, c_stored.r, c_stored.g, c_stored.b); ps_arrow(a, b, 0.2f); gfx_end(); g_line_half = 1.f; }
			}
		}
		if (cam && o_ghost) { trace("draw start"); ghost_draw(); trace("draw end"); }   /* the ghost pill */
		oob_update(); oob_draw(height);                                     /* the out-of-bounds timer */
		sleep_update(); sleep_draw(height);                                 /* the sleep timer */
		if (o_ctimer) ct_draw(height);                                      /* 6i.156: the Campaign Timer */
		pred_update(); pred_draw(height, cam != 0);                         /* the predicted monster / dreadnought spots */
		if (marker && cam) draw_marker(g_sb_object);
		if (hint)                                                            /* 6h.6: on the info line's row (the lowest); 6h.8: */
		{                                                                    /* in the bottom-left corner, as far from the screen's left
		                                                                        edge as its letters are from the bottom (~12 lines; a pixel
		                                                                        here is 4/3 as wide on a 16:9 TV - 9 px) */
			put(0, "Spooky Ghost Menu: Back + D-pad Up");   /* 6i.199: your wording */
			line(9, y_hint + height + 4, height, &c_hint);
			g_toast_frames = 0;                                              /* (a game's last message: not over it) */
		}
		if (g_toast_frames > 0)                                              /* a practice action's result, 2 s */
		{
			put(0, g_toast);
			line(60, y_hint + height + 4, height, &c_hint);
			toast_step();
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
				else put(at, g_ld_dir[2] > 0.7f ? " u/s UP" : " u/s SIDEWAYS");           /* (6h.32: a launch stops the fall - never down) */
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
					if (e->ph)
					{
						/* 6h.34 (HCM): the physics check's line; 6i.127: HCM's own wording now (Harc's Build, 2026-11-05) */
						static const color4f c_ph[4] = { { 1.0f, 0.31f, 0.90f, 0.35f }, { 1.0f, 0.40f, 0.80f, 1.00f }, { 1.0f, 0.72f, 0.38f, 1.00f }, { 1.0f, 1.0f, 0.31f, 0.27f } };
						s32 air = e->ph_cur > 0.5f, at = put(0, air ? "PHYSICS: " : "PHYSICS (JUMP from here): ");
						if (e->ph == 4) put(at, air ? "no forward speed (0-2.25) launches from this fall" : "no forward speed (0-2.25) launches from a jump here");
						else
						{
							if (e->ph == 1) at = put(at, air ? "launches at your forward speed (" : "launches holding forward (");
							else at = put(at, e->ph == 2 ? "SLOW DOWN - forward " : "SPEED UP - forward ");
							at = put_f(at, e->ph_lo, 2); at = put(at, "-"); at = put_f(at, e->ph_hi, 2);
							at = put(at, e->ph == 1 ? " u/s forward work) -> about " : e->ph == 2 ? " u/s launches (tap / fragment forward) -> about " : " u/s launches (max 2.25) -> about ");
							at = put_int(at, (s32)(e->ph_sp + 0.5f)); put(at, " u/s");
						}
						line_at((s32)g_lbl_x[li] + g_vp_left - 260, (s32)g_lbl_y[li] + g_vp_top, 520, &c_ph[e->ph - 1], 2);
					}
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
				at = put(at, " (late "); at = put_f(at, g_ld_have_depth ? g_ld_late : 0.f, 2); at = put(at, ")");   /* 6i.212: no "tilt" / "+ bend" - they were Cartographer's */
				line_at(60, y_speed - 2 * height, 560, &c_land[q == 3 ? 3 : q], 2);
				put(0, g_ld_why);
				line_at(60, y_speed - height, 560, &c_land[q == 3 ? 3 : q], 2);
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
u32 host_seam_searches(void) { return g_seam_searches; }   /* 6i */
u32 host_heartbeats(void) { return g_heartbeats; }
s32 host_toast_left(void) { return g_toast_frames; }
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

static void spooky_main_frame_body(void);
/* 6i.4: the frame-time, Unlock FPS and census lines - from the 10-second block, and after its first ~4 minutes on their
 * own every 10 s (up to 30 minutes in all) while Unlock FPS is on, for tests that take a while to set up */
static void timing_lines(void)
{
	s32 at;
	if (g_pf_n)                                                         /* 6i.3: where the frames' time went */
	{
		static const char *const nm[PF_N] = { "update ", ", drawing ", ", vblank wait ", ", present ", ", after ", "; the mod " };
		u32 k;
		at = lput(0, "  frame time ms, average / most ("); at = lnum(at, g_pf_n); at = lput(at, " frames): ");
		for (k = 0; k < PF_N; k++) { at = lput(at, nm[k]); at = lms(at, g_pf_sum[k] / g_pf_n); at = lput(at, " / "); at = lms(at, g_pf_max[k]); }
		at = lput(at, " (interpolation "); at = lms(at, g_pf_ip_sum / g_pf_n); lput(at, ")");
		log_write();
		if (g_ipt_ticks || g_ipt_objs)                                      /* 6i.182: the in-between frames, split */
		{
			at = lput(0, "  in-between frames ms: snapshot a tick "); at = lms(at, g_ipt_ticks ? g_ipt_snap / 733u / g_ipt_ticks : 0u);
			at = lput(at, "; a frame: save "); at = lms(at, g_ipt_save / 733u / g_pf_n); at = lput(at, ", blend "); at = lms(at, g_ipt_blend / 733u / g_pf_n);
			at = lput(at, ", restore "); at = lms(at, g_ipt_rest / 733u / g_pf_n);
			at = lput(at, "; tracked a frame: objects "); at = lnum(at, g_ipt_objs / g_pf_n); at = lput(at, ", parts "); at = lnum(at, g_ipt_parts / g_pf_n);
			at = lput(at, ", objects blended "); at = lnum(at, g_ipt_blended / g_pf_n);
			if (g_ip_fpw_n) { at = lput(at, "; a first-person weapon changed between two ticks (not blended) "); at = lnum(at, g_ip_fpw_n); g_ip_fpw_n = 0; }   /* 6i.204 */
			log_write();
		}
		g_ipt_snap = g_ipt_save = g_ipt_blend = g_ipt_rest = g_ipt_ticks = g_ipt_objs = g_ipt_parts = g_ipt_blended = 0;
		if (g_pf_slow_n)                                                /* the frames that missed 60: what they spent it on */
		{
			at = lput(0, "  frames over 17 ms ("); at = lnum(at, g_pf_slow_n); at = lput(at, "), average: ");
			for (k = 0; k < PF_N; k++) { at = lput(at, nm[k]); at = lms(at, g_pf_slow_sum[k] / g_pf_slow_n); }
			log_write();
		}
		if (!g_cap_frames)                                                    /* 6i.19: not while a capture runs */
		{                                                                     /* 6i.12: the render passes' drawing a frame */
			u32 k, us;
			at = lput(0, "  render passes ms a frame:");
			for (k = 0; k < 64; k++)
			{
				us = spooky_pass_cyc[k] / 733u / g_pf_n; spooky_pass_cyc[k] = 0;   /* (k < 64: below) */
				if (us >= 50u) { at = lput(at, " "); at = lnum(at, k); at = lput(at, ":"); at = lms(at, us); }
			}
			if (spooky_skip_pass) { at = lput(at, " (pass "); at = lnum(at, (u32)spooky_skip_pass); at = lput(at, " skipped)"); }
			log_write();
		}
		g_pf_n = 0; g_pf_ip_sum = 0; g_pf_slow_n = 0;
		for (k = 0; k < PF_N; k++) g_pf_sum[k] = g_pf_max[k] = g_pf_slow_sum[k] = 0;
	}
	if (g_vb_hist[0] | g_vb_hist[1] | g_vb_hist[2] | g_vb_hist[3])        /* 6i: Unlock FPS - how long the frames took */
	{
		at = lput(0, "  Unlock FPS: frames that took 1 vblank (60 fps) "); at = lnum(at, g_vb_hist[0]);
		at = lput(at, ", 2 (30) "); at = lnum(at, g_vb_hist[1]); at = lput(at, ", 3 (20) "); at = lnum(at, g_vb_hist[2]);
		at = lput(at, ", 4 or more "); at = lnum(at, g_vb_hist[3]);
		if (g_ip_mem > 0)                                              /* 6i.1: the last snapshot - how much it took */
		{
			at = lput(at, "; interpolated "); at = lnum(at, (u32)g_ip_n); at = lput(at, " objects, ");
			at = lnum(at, (u32)g_ip_nodes); at = lput(at, " of "); at = lnum(at, (u32)g_ip_node_cap); at = lput(at, " nodes");
			if (g_ip_body_n >= 0) { at = lput(at, ", your body "); at = lnum(at, (u32)g_ip_body_n); } else if (g_ip_body_n == -2) at = lput(at, ", YOUR BODY LEFT OUT");   /* 6i.27 */
			if (spooky_ptc_kept) { at = lput(at, "; particle systems kept between ticks "); at = lnum(at, spooky_ptc_kept); spooky_ptc_kept = 0; }   /* 6i.35 */
			if (g_ip_left_out) { at = lput(at, " ("); at = lnum(at, (u32)g_ip_left_out); lput(at, " near ones left out)"); }
		}
		else if (g_ip_mem < 0) { at = lput(at, "; interpolation: no memory (largest free block "); at = lnum(at, g_ip_largest / 1024u); lput(at, " KB)"); }
		log_write();
		g_vb_hist[0] = g_vb_hist[1] = g_vb_hist[2] = g_vb_hist[3] = 0;
	}
	g_log_hold = 0;                                                           /* 6i.10: none of them flushed (the next other line does) */
}
void spooky_main_frame(void)                    /* 6i.3: timed (the mod's own share of the frame) */
{
	u32 t;
	g_count_main++;
	frame_time_update();                                                  /* 6i: the frame clock; the game's pacing byte */
	t = tsc_lo();
	spooky_main_frame_body();
	g_pf_mod += tsc_lo() - t;
}
static void spooky_main_frame_body(void)
{
	if (g_ip_own_set || g_ip_blended) guarded_call(ip_unblend);             /* 6i.48: (none should be: the draw puts all back) */
	if (g_ip_mem > 0 && !o_unlock_fps && o_speed >= 10) ip_give_back("Unlock FPS off");   /* 6i.2 */
	g_fov_seen = 0;                                                       /* 6i.126: the coming frame's first view build sets the overlays' field of view */
	guarded_call(af_step);                                                /* 6i.128: the rocket launcher's animation fix */
	game_speed_frame();                                                   /* 6h.22: Game Speed / Pause Game, before the game's time update */
	if (g_rebuild_due)                                                    /* 6i.121: the tick rate changed - the collision at the new rate */
	{
		char t[64], *p;
		u32 tps = M32(ADDR_GAME_TIME_PTR);
		g_rebuild_due = 0;
		tps = ptr_ok(tps) ? (u32)(s32)(s16)M16(tps + 2) : 0;
		if (guarded_call(collision_rebuild)) { lput(0, "tick rate: the collision rebuild faulted (guarded)"); log_write(); }
		if (o_trate != g_trate_said)                                     /* said when you changed Tick Rate (a revert's rebuild: silently) */
		{
			g_trate_said = o_trate;
			p = say_s(t, "Tick Rate "); p = say_u(p, tps); p = say_s(p, ": collision rebuilt ("); p = say_u(p, g_rb_objects); say_s(p, " objects)");
			toast(t); lput(0, t); log_write();
		}
	}
	trace_frame();
	fc_frame();                                                           /* the free camera: flown once a frame */
	trace("main start");
	if (g_req && on_main_menu() && g_req != 14 && g_req != 20)               /* 6i.198: on the game's main menu only the restart (14) and Reset to defaults (20) run */
	{
		toast("Only in a level");
		g_req = 0;
	}
	if (g_req)                                                                /* a practice action, before the game's update */
	{
		g_prac_what = g_req; g_req = 0; g_prac_stage = "starting";
		if (guarded_call(practice_guarded))                                  /* 6h.34: guarded (the 360's Launch crash) */
		{
			s32 at = lput(0, "practice: a bad read was caught (guarded) in "); at = lput(at, prac_name(g_prac_what));
			at = lput(at, " while "); at = lput(at, g_prac_stage); lput(at, " - skipped");
			log_write();
			toast("Skipped: a bad read was caught (see the log)");
		}
	}
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
	if (g_t30 >= 30 && g_t30 - (u32)g_steps < 30)   /* about a second in: the game has mapped T: by now (6i: the 30 Hz clock) */
	{
		s32 at = lput(0, "Spooky Ghost Menu v2.0 running in Halo 2 " H2_VERSION_TEXT " (modified default.xbe); per-frame hook OK; log on ");
		lput(at, g_log_drive == 1 ? "U: (E:\\UDATA\\4D530064)" : g_log_drive == 2 ? "Z: (cache drive)" : "T: (E:\\TDATA\\4D530064)");
		log_write();
		guarded_call(mem_size_note);                                         /* 6i.171 */
#if SPOOKY_PROBE
		guarded_call(mem_probe);                                              /* 6i.130: the console's memory, for Render Scale above 100 % */
		at = lput(0, "memory: "); at = lnum(at, g_mem_mb); at = lput(at, " MB console, "); at = lnum(at, g_mem_free_kb); at = lput(at, " KB free, largest block ");
		at = lnum(at, g_mem_big_kb); at = lput(at, " KB; the XBE's 64 MB limit flag "); at = lput(at, (M32(0x10124u) & 4u) ? "set" : "clear");   /* 6i.132: the init flags are at +0x124 (6i.130 read +0x118, the certificate's address) */
		{
#ifdef HOST
			u32 pfb = 0x04000000u;
#else
			u32 pfb = *(volatile u32 *)0xFD10020Cu;                           /* 6i.131: the NV2A's memory size (NV_PFB_CSTATUS) - the hardware's own word on it */
#endif
			at = lput(at, "; RAM fitted (host bridge) "); at = lnum(at, g_mem_top_mb); at = lput(at, " MB");
			if (g_pfn) at = lput(at, g_pfn > 0 ? ", kernel cap raised to 128 MB" : ", kernel cap NOT found");
			at = lput(at, "; the GPU's memory register: "); at = lhex(at, pfb);
			if (pfb == 0x04000000u || pfb == 0x08000000u) { at = lput(at, " ("); at = lnum(at, pfb >> 20); at = lput(at, " MB)"); }
		}
		log_write();
#endif
	}
	{
		static u32 more;                                                      /* 6i.4: after those ~4 minutes, the test lines go on */
		if (g_steps && g_t30 / 300 != (g_t30 - (u32)g_steps) / 300 && g_heartbeats >= 24 && o_unlock_fps && more < 156) { more++; g_log_hold = 1; timing_lines(); }
	}
	if (g_steps && g_t30 / 300 != (g_t30 - (u32)g_steps) / 300 && g_heartbeats < 24)   /* about every 10 s (300 steps of the 30 Hz clock), for about 4 minutes */
	{
		g_heartbeats++;
		g_log_hold = 1;                                                   /* 6i.7: one flush for the block's lines (timing_lines' last) */
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
					at = lput(at, ", drawing "); at = lnum(at, (u32)((float)g_cyc_draw / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, ", exact "); at = lnum(at, (u32)((float)g_cyc_exact / (float)g_seam_cyc_n / 733.333f));
					at = lput(at, " (area rebuilds "); at = lnum(at, (u32)g_exa_builds); lput(at, ")"); g_exa_builds = 0;
				}
				log_write();
			}
			if (g_sm_n[0] || g_sm_n[1] || g_nlr_n)                              /* 6i.184: the search, on the ground and in the air */
			{
				s32 a;
				at = lput(0, "  superbounce search (us):");
				for (a = 0; a < 2; a++)
				{
					u32 n = g_sm_n[a] ? g_sm_n[a] : 1u;
					at = lput(at, a ? "; in the air " : " on the ground "); at = lnum(at, g_sm_n[a]); at = lput(at, " searches");
					if (!g_sm_n[a]) continue;
					at = lput(at, ", average "); at = lnum(at, g_sm_cyc[a] / 733u / n); at = lput(at, ", longest "); at = lnum(at, g_sm_max[a] / 733u);
					at = lput(at, ", level edges "); at = lnum(at, g_sm_edges[a] / n); at = lput(at, ", seams kept "); at = lnum(at, g_sm_kept[a] / n);
					at = lput(at, ", pieces near "); at = lnum(at, g_sm_pieces[a] / n); at = lput(at, ", objects near "); at = lnum(at, g_sm_objects[a] / n);
				}
				at = lput(at, "; grid "); at = lput(at, g_lg_ok ? "on" : "OFF"); at = lput(at, ", long edges "); at = lnum(at, (u32)g_lg_nlong);
				at = lput(at, ", longest half x10 "); at = lnum(at, (u32)(g_lg_half * 10.f));
				if (g_nlr_n)
				{
					at = lput(at, "; near list rebuilt "); at = lnum(at, g_nlr_n); at = lput(at, " times ("); at = lnum(at, g_nlr_flip);
					at = lput(at, " for leaving / touching the ground), average "); at = lnum(at, g_nlr_cyc / 733u / g_nlr_n); at = lput(at, " us, holds "); at = lnum(at, g_nlr_size);
					if (g_nlr_full) at = lput(at, " (FULL)");
				}
				log_write();
				for (a = 0; a < 2; a++) g_sm_n[a] = g_sm_cyc[a] = g_sm_max[a] = g_sm_edges[a] = g_sm_kept[a] = g_sm_pieces[a] = g_sm_objects[a] = 0;
				g_nlr_n = g_nlr_flip = g_nlr_cyc = g_nlr_full = 0;
			}
			g_fps_t0 = now; g_fps_f0 = g_count_hud;
			timing_lines();                                                     /* 6i.3 / 6i / 6i.4 */
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
			g_cyc_search = g_cyc_block = g_cyc_rate = g_cyc_draw = g_cyc_level = g_cyc_pieces = g_cyc_objects = g_cyc_land = 0; g_cyc_exact = 0;
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
s32 host_menu_peek(void) { return g_peek; }
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
