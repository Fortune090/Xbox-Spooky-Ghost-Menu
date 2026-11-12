#include <math.h>
/* Host test for the superbounce HUD: spooky.c's real per-tick code against simulated game memory. */
#include <stdio.h>
#include <string.h>
#include "sb_exact.c"                 /* 6h.34: the separately verified exact core (sbx_), for an independent check */
#include <assert.h>
float host_hsq_quad_y(unsigned int corners, int i);   /* 6i.47: the bar's test reads it before the ring's block */

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef signed short s16; typedef signed int s32;
typedef struct { u32 packet; u16 buttons; u8 analog[8]; s16 lx, ly, rx, ry; } xinput_state;
typedef struct { s16 top, left, bottom, right; } rect16;
typedef struct { float a, r, g, b; } color4f;

/* ---- simulated game memory: 1 MB at address 0 ---- */
static u8 MEM[0x100000];
void host_w32(u32 a, u32 v) { if (a + 4 <= sizeof(MEM)) memcpy(MEM + a, &v, 4); }
u32 host_mem32(u32 a) { u32 v = 0; if (a + 4 <= sizeof(MEM)) memcpy(&v, MEM + a, 4); return v; }
u8 host_mem8(u32 a) { return a < sizeof(MEM) ? MEM[a] : 0; }
static u8 g_osfile[0x20000]; static u32 g_osfile_off, g_osfile_size;   /* 6i.158: the stand-in map file's tag names, at this file offset */
u32 host_os_read(u32 offset, u8 *to, u32 bytes)
{
	u32 n;
	if (offset < g_osfile_off || offset - g_osfile_off >= g_osfile_size) return 0;
	n = g_osfile_size - (offset - g_osfile_off); if (n > bytes) n = bytes;
	memcpy(to, g_osfile + (offset - g_osfile_off), n);
	return n;
}
static void W32(u32 a, u32 v) { memcpy(MEM + a, &v, 4); }
static void WF(u32 a, float f) { memcpy(MEM + a, &f, 4); }
static float RF(u32 a) { float f; memcpy(&f, MEM + a, 4); return f; }
static void WS(u32 a, const char *s) { strcpy((char *)MEM + a, s); }

/* ---- files ---- */
static char g_records[4096]; static u32 g_records_len; static int g_saves;
u32 host_read_file(const char *name, char *buffer, u32 size) { (void)name; u32 n = g_records_len < size ? g_records_len : size; memcpy(buffer, g_records, n); return n; }
void host_write_file(const char *name, const char *data, u32 size) { (void)name; memcpy(g_records, data, size); g_records_len = size; g_records[size] = 0; g_saves++; }

/* ---- the rest of the host interface ---- */
u8 host_draw_string_globals[0x64];
u32 host_font_handles[16];
static s16 g_font_header[8] = { 0, 0, 12, 4, 2, 0, 0, 0 };
static char g_lines[16][100]; static int g_nlines; static color4f g_colors[16]; static int g_just[16]; static rect16 g_rects[16];
u32 host_xinput_get_state(void *h, xinput_state *st) { (void)h; memset(st, 0, sizeof(*st)); return 0; }
void host_text_style_clear(void) { memset(host_draw_string_globals, 0, sizeof(host_draw_string_globals)); }
void *host_font_get(u32 h) { (void)h; return g_font_header; }
void host_draw_text(const u16 *text, const rect16 *b, float scale)
{
	(void)b; (void)scale; int i;
	if (g_nlines >= 16) return;
	for (i = 0; text[i] && i < 99; i++) g_lines[g_nlines][i] = (char)text[i];
	g_lines[g_nlines][i] = 0;
	g_colors[g_nlines] = *(const color4f *)(host_draw_string_globals + 0x10);
	g_just[g_nlines] = *(const s32 *)(host_draw_string_globals + 0x0c);
	g_rects[g_nlines] = *b;
	g_nlines++;
}
static char g_loglines[256][200]; static int g_nlog;            /* the latest 256 log lines */
void host_log(const char *line) { strncpy(g_loglines[g_nlog % 256], line, 199); g_loglines[g_nlog % 256][199] = 0; g_nlog++; }
static int logged(const char *needle) { int i, n = 0, m = g_nlog < 256 ? g_nlog : 256; for (i = 0; i < m; i++) if (strstr(g_loglines[i], needle)) n++; return n; }
void host_marker_reset(void);
void host_set_widescreen(u32 ws, u32 flag5);
void host_set_marker(s32 on);
void host_set_seams(s32 on);
s32 host_landing(float *p);
s32 host_landing_q(void);
void host_set_land_text(s32 on);
void host_set_force_charge(s32 on);
void host_set_keep_charge(s32 on);
void host_practice(s32 what);
void host_set_sb_viewer(s32 on);
void host_set_disable_ai(s32 on);
void host_apply_ai(void);
void host_set_ammo(s32 inf, s32 bottomless);
s32 ia_answer(u32 weapon_index);
const char *host_toast(void);
void bc_before(u32 weapon_index);
void bc_after(void);
void host_set_third_person(s32 on, s32 dist, s32 height, s32 side);
u32 host_tp_camera(u32 cam, s32 user);
void host_tp_fp_after(void);
void host_set_invuln(s32 on);
void host_apply_invuln(void);
void host_set_level_grid(s32 on);
s32 host_seams_drawn(void);
float host_seam_sum(void);
s32 host_blockers(void);
const char *host_toast(void);
void host_set_practice(s32 lf, s32 lu, s32 tf, s32 tu);
static u32 g_lv_motion; static float g_lv[3]; static int g_lv_n;
void host_set_linear_velocity(u32 motion, const float *v) { g_lv_motion = motion; memcpy(g_lv, v, 12); g_lv_n++; }
#define ADDR_SAVE_MAP_T 0x1D00
s32 host_ghost(const float *a, const float *b, s32 add, s32 launched);
s32 host_rated_seams(void);
s32 host_reach_ok(const float *feet, const float *vel, const float *q);
s32 host_air_near(const float *f, const float *v, s32 land_ok, const float *lp, float x, float y, float r);
s32 host_seam_count(s32 kind);
s32 host_object_bsps(void);
static int g_q; static s16 g_qxy[64][8]; static u32 g_qcol[64];
void host_gfx_quad(u32 color, const s16 *xy) { if (g_q < 64) { memcpy(g_qxy[g_q], xy, 16); g_qcol[g_q] = color; } g_q++; }

void spooky_main_frame(void);
void spooky_frame(void);

/* addresses of the simulated structures */
enum { TIME = 0x30000, OBJ_POOL = 0x31000, OBJ_DATA = 0x31100, LOCALS = 0x32000, PL_POOL = 0x33000, PL_DATA = 0x33100,
       HC_POOL = 0x34000, HC_DATA = 0x34100, BODIES = 0x35000, ENTITY = 0x36000, WORLD = 0x37000, ISLAND = 0x38000,
       ENTS = 0x38100, PAIRS = 0x38200, SHAPE = 0x39000, OBJECT = 0x40000, NULL_AGENT = 0x20500, AGENT = 0x42000 };
#define UNIT 0x00050002u
#define HANDLE (ENTITY + 0x20)

static void build(void)
{
	W32(0x1000, TIME); W32(0x1004, OBJ_POOL); W32(0x1008, LOCALS); W32(0x100C, PL_POOL); W32(0x1010, HC_POOL); W32(0x1014, WORLD);
	WS(0x1100, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa");
	W32(OBJ_POOL + 0x20, 10); W32(OBJ_POOL + 0x24, 12); W32(OBJ_POOL + 0x44, OBJ_DATA);
	W32(OBJ_DATA + 2 * 12, 0x0005); W32(OBJ_DATA + 2 * 12 + 8, OBJECT);        /* salt 5, object */
	W32(LOCALS + 0xC, 0x00010000);                                             /* local player 0 -> player 0 */
	W32(PL_POOL + 0x20, 4); W32(PL_POOL + 0x24, 0x21C); W32(PL_POOL + 0x44, PL_DATA);
	W32(PL_DATA + 0x2C, UNIT);
	W32(HC_POOL + 0x20, 8); W32(HC_POOL + 0x24, 0x98); W32(HC_POOL + 0x44, HC_DATA);
	W32(HC_DATA + 3 * 0x98 + 0x70, BODIES); W32(HC_DATA + 3 * 0x98 + 0x74, 1);
	W32(OBJECT + 0xB4, 3);
	W32(BODIES + 0x40, ENTITY);
	W32(ENTITY + 0x08, WORLD); W32(ENTITY + 0x44, ISLAND);
	W32(HANDLE + 0x10, ENTITY); W32(HANDLE - 0x10, SHAPE);                      /* the handle inside the body */
	W32(ISLAND + 0x3C, ENTS); W32(ISLAND + 0x40, 1); W32(ENTS, ENTITY);
	W32(ISLAND + 0x68, PAIRS); W32(ISLAND + 0x6C, 3);
	W32(PAIRS + 0, HANDLE); W32(PAIRS + 4, 0x50000); W32(PAIRS + 8, NULL_AGENT); W32(PAIRS + 12, (u32)-3);     /* null agent */
	W32(PAIRS + 16, 0x50010); W32(PAIRS + 20, 0x60000); W32(PAIRS + 24, 0x41000); W32(PAIRS + 28, (u32)-3);   /* not ours */
	W32(PAIRS + 32, 0x50020); W32(PAIRS + 36, HANDLE); W32(PAIRS + 40, AGENT); W32(PAIRS + 44, 0);           /* ours */
}
static void state(u32 tick, float vz, int airborne, int charged)
{
	W32(TIME + 8, tick); WF(OBJECT + 0x88, 3.f); WF(OBJECT + 0x8C, 4.f); WF(OBJECT + 0x90, vz);
	MEM[OBJECT + 0x348] = (u8)(airborne ? 1 : 0); W32(PAIRS + 44, charged ? (u32)-3 : 0);
}
void spooky_view(u32 view);
/* a frame: the main loop, then route A handing the HUD pass the view record (its camera block at +0xC), then HUD text */
static u32 M32h(u32 a) { return host_mem32(a); }
static float M32f(u32 a) { float f; u32 v = host_mem32(a); memcpy(&f, &v, 4); return f; }
void host_shown_reset(void);
s32 host_shown_count(void);
static int g_keep_shown;
/* each frame measured alone (the shown seams' 1.5 s hold is tested on its own) */
static void frame(void) { g_nlines = 0; if (!g_keep_shown) host_shown_reset(); spooky_main_frame(); spooky_view(0x120C); spooky_frame(); }
static int drawn(const char *s) { int i; for (i = 0; i < g_nlines; i++) if (strstr(g_lines[i], s)) return i + 1; return 0; }

float ftanf_host(float x);
const void *host_live_page(void);
int main(void)
{
	{
		/* 6h.35: the exact circle is the default now (your 6h.34 log: it called 24 of 24 bounces); the tests below were
		 * written for both - most of the early ones for Cartographer's, so they start with it and switch where they say */
		s32 host_exact_circle(void); void host_set_exact_circle(s32 on);
		assert(host_exact_circle() == 1);
		host_set_exact_circle(0);
	}
	build();
	host_set_sb_viewer(1);                    /* off by default; this suite tests it */
	{ void host_set_oob(s32 on); void host_set_sleep(s32 on); host_set_oob(0); host_set_sleep(0); }   /* the timers (on by default) are tested on their own: off for the quad counts */
	strcpy(g_records, "03a_oldmombasa 1\r\nzz_other 7\r\n"); g_records_len = (u32)strlen(g_records);

	/* falling, not charged: the null agent's -3 pair and a pair of other bodies at -3 must not count */
	state(100, -10.f, 1, 0); frame();
	assert(drawn("not charged") && drawn("11.2 u/s") && drawn("Level record: 1") && drawn("Bounces in a row: 0"));
	assert(drawn("Last Bounce: --"));                                        /* 6h.16: none yet */
	{ int i = drawn("not charged") - 1; assert(g_just[i] == 2); }
	printf("not charged while only the null agent's and other bodies' pairs are at -3; record 1 loaded for this map\n");

	/* charged */
	state(101, -10.f, 1, 1); frame();
	assert(drawn("CHARGED"));
	{ int i = drawn("CHARGED") - 1; assert(g_colors[i].g < 0.5f); }
	/* launched: 20 u/s up after falling at 10 -> a bounce; 1 does not beat the record of 1 */
	state(102, 20.f, 1, 1); frame();
	assert(drawn("Bounces in a row: 1") && drawn("Level record: 1") && g_saves == 0);
	assert(logged("superbounce: bounce 1 in a row, first tick speed x10 206") == 1);   /* 6h.18: in the log */
	{
		/* 6h.16: the bounce's speed (3, 4, 20 -> 20.6) on the charge state's line, right-aligned, ending left of its text */
		int i = drawn("Last Bounce: 20.6u/s") - 1, c = drawn("CHARGED") - 1;
		assert(i >= 0 && c >= 0 && g_just[i] == 1 && g_rects[i].top == g_rects[c].top && g_rects[i].right <= 320 - 55 - 12);
	}
	/* fall, bounce again: 2 beats the record: saved */
	state(103, -12.f, 1, 1); frame(); state(104, 15.f, 1, 1); frame();
	assert(drawn("Bounces in a row: 2") && drawn("Level record: 2") && g_saves == 1 && drawn("Last Bounce: 15.8u/s"));
	assert(strstr(g_records, "03a_oldmombasa 2") && strstr(g_records, "zz_other 7"));
	printf("bounces counted (launch > 4 u/s up after falling > 2 u/s, charged); record 2 saved, other levels kept\n");

	/* not charged: a launch does not count */
	state(105, -12.f, 1, 0); frame(); state(106, 15.f, 1, 0); frame();
	assert(drawn("Bounces in a row: 2"));
	/* a bounce touches down for one tick: the chain survives one or two ground ticks, ends on the third */
	state(107, -6.f, 1, 1); frame(); state(108, 0.f, 0, 1); frame(); state(109, 8.f, 1, 1); frame();
	assert(drawn("Bounces in a row: 2"));
	state(110, 0.f, 0, 1); frame(); state(111, 0.f, 0, 1); frame();
	assert(drawn("Bounces in a row: 2"));
	state(112, 0.f, 0, 1); frame();
	assert(drawn("Bounces in a row: 0") && drawn("Level record: 2"));
	printf("the chain survives a one-tick touchdown and ends on the third tick on the ground\n");

	/* paused (same tick): nothing changes */
	state(113, -10.f, 1, 1); frame(); state(114, 18.f, 1, 1); frame();
	assert(drawn("Bounces in a row: 1"));
	frame(); frame(); frame();
	assert(drawn("Bounces in a row: 1"));
	{
		/* 6h.17: a Launch (practice action 3) while charged and falling is not a bounce - its push is added to what the bounce
		 * test compares with; and a bounce on the very tick after a launch (a monster / dreadnought launched into) counts,
		 * at its own speed */
		void host_req(s32 r);
		float lv[3], sp;
		char want[64];
		state(115, -10.f, 1, 1); frame();
		host_req(3); frame();                                                   /* (same tick) the launch's push applied */
		lv[0] = M32f(OBJECT + 0x88); lv[1] = M32f(OBJECT + 0x8C); lv[2] = M32f(OBJECT + 0x90);
		assert(fabsf(lv[0] - 3.f) + fabsf(lv[1] - 4.f) > 5.f);                 /* it pushed (10 u/s forward) */
		W32(TIME + 8, 116); WF(OBJECT + 0x90, lv[2] - 0.14f); frame();          /* its next tick, falling on: not a bounce */
		{ const char *host_toast(void); assert(strstr(host_toast(), "Bounce streak reset: Launch used")); }   /* 6h.42: the streak reset */
		assert(drawn("Bounces in a row: 0") && drawn("Last Bounce: 18.") && logged("superbounce: bounce streak of 1 reset - Launch used") == 1);
		host_req(3); frame();                                                   /* launched again ... */
		lv[0] = M32f(OBJECT + 0x88); lv[1] = M32f(OBJECT + 0x8C);
		W32(TIME + 8, 117); WF(OBJECT + 0x90, 25.f); frame();                  /* ... and the very next tick a bounce */
		sp = sqrtf(lv[0] * lv[0] + lv[1] * lv[1] + 25.f * 25.f);
		snprintf(want, sizeof(want), "Last Bounce: %d.%du/s", (int)(sp * 10.f + 0.5f) / 10, (int)(sp * 10.f + 0.5f) % 10);
		assert(drawn("Bounces in a row: 1") && drawn(want));
		{ void host_clear_toast(void); host_clear_toast(); }                     /* (Launch's message) */
		printf("launches: a launch alone is no bounce and resets the streak (6h.42: \"Bounce streak reset: Launch used\"); a bounce the very tick after one counts, from 0 (%s)\n", want);
	}
	{
		/* 6h.19: a launch the game didn't apply (your speed unchanged) is no bounce either (6h.18 took one for a 2.2 u/s
		 * "bounce"); a bounce under 5 u/s never counts; Last Bounce keeps the peak of the 5 ticks after a bounce */
		void host_req(s32 r); const char *host_toast(void);
		int b0;
		char line[64];
		state(130, -10.f, 1, 1); frame();
		b0 = drawn("Bounces in a row: 1") ? 1 : -1; assert(b0 == 1);
		host_req(3); frame();                                                   /* launched (the streak reset, 6h.42) ... */
		W32(TIME + 8, 131); WF(OBJECT + 0x88, 3.f); WF(OBJECT + 0x8C, 4.f); WF(OBJECT + 0x90, -10.14f); frame();   /* ... it didn't take */
		assert(drawn("Bounces in a row: 0"));
		W32(TIME + 8, 132); WF(OBJECT + 0x90, -10.f); frame();
		W32(TIME + 8, 133); WF(OBJECT + 0x88, 1.f); WF(OBJECT + 0x8C, 1.f); WF(OBJECT + 0x90, 4.5f); frame();   /* 4.7 u/s: too slow */
		assert(drawn("Bounces in a row: 0"));
		state(134, -10.f, 1, 1); frame();
		state(135, 10.f, 1, 1); frame();                                       /* a bounce: 11.2 u/s ... */
		assert(drawn("Bounces in a row: 1") && drawn("Last Bounce: 11.2u/s"));
		state(136, 30.f, 1, 1); frame();                                       /* ... rising to 30.4 the next tick */
		assert(drawn("Bounces in a row: 1") && drawn("Last Bounce: 30.4u/s"));
		state(137, 29.f, 1, 1); frame(); assert(drawn("Last Bounce: 30.4u/s")); /* the peak kept */
		state(138, 28.f, 1, 1); frame(); state(139, 27.f, 1, 1); frame(); state(140, 26.f, 1, 1); frame();
		assert(logged("superbounce: bounce 1 peak speed x10 304") >= 1);       /* 6h.20: the peak in the log, after its 5 ticks */
		{ void host_clear_toast(void); host_clear_toast(); }
		(void)line; (void)host_toast;
		printf("bounces: a launch that didn't take - none; under 5 u/s - none; Last Bounce - the peak over 5 ticks (30.4)\n");
	}
	{
		/* 6h.42: a streak only counts at the game's own speed: slowing it (Game Speed under 1.0) or pausing resets a streak, a
		 * bounce while it's slowed isn't counted, and Launch / Teleport reset it - each said on screen and in the log */
		void host_set_speed(s32 tenths, s32 pause); const char *host_toast(void); void host_req(s32 r); void host_clear_toast(void);
		state(141, -10.f, 1, 1); frame(); state(142, 10.f, 1, 1); frame();
		assert(drawn("Bounces in a row: 2"));
		host_set_speed(5, 0); state(143, 8.f, 1, 1); frame();                  /* Game Speed 0.5 */
		assert(drawn("Bounces in a row: 0") && strstr(host_toast(), "Bounce streak reset: the game was slowed") && logged("superbounce: bounce streak of 2 reset - the game was slowed") == 1);
		state(144, -10.f, 1, 1); frame(); state(145, 10.f, 1, 1); frame();    /* a bounce at half speed: not counted */
		assert(drawn("Bounces in a row: 0") && strstr(host_toast(), "Bounce not counted: the game is slowed"));
		host_set_speed(10, 0);
		state(146, -10.f, 1, 1); frame(); state(147, 10.f, 1, 1); frame();    /* back to 1.0: counted */
		assert(drawn("Bounces in a row: 1"));
		host_set_speed(20, 0);
		state(148, -10.f, 1, 1); frame(); state(149, 10.f, 1, 1); frame();    /* faster than the game's own: counted */
		assert(drawn("Bounces in a row: 2"));
		host_set_speed(10, 1); frame();                                          /* paused: no new tick - still reset */
		assert(drawn("Bounces in a row: 0") && strstr(host_toast(), "Bounce streak reset: the game was paused"));
		host_set_speed(10, 0);
		state(150, -10.f, 1, 1); frame(); state(151, 10.f, 1, 1); frame();
		assert(drawn("Bounces in a row: 1"));
		host_req(4); state(152, 8.f, 1, 1); frame();                            /* Teleport */
		assert(drawn("Bounces in a row: 0") && strstr(host_toast(), "Bounce streak reset: Teleport used"));
		host_clear_toast();
		printf("bounce streaks: reset by Game Speed under 1.0, Pause, Launch and Teleport (said on screen and logged); not counted while slowed; 1.0 and faster count\n");
	}
	/* a revert (the tick goes back) ends the chain */
	state(90, -3.f, 1, 1); frame();
	assert(drawn("Bounces in a row: 0"));
	printf("a pause changes nothing; a revert ends the chain\n");

	/* another level: its own record; back again: this one's */
	WS(0x1100, "scenarios\\solo\\zz_other\\zz_other"); state(91, -3.f, 1, 1); frame();
	assert(drawn("Level record: 7") && drawn("Last Bounce: --"));            /* 6h.16: a new level: none yet */
	WS(0x1100, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa"); state(92, -3.f, 1, 1); frame();
	assert(drawn("Level record: 2"));                                        /* (6h.42: the Launch tests reset their streaks - 2 stands) */
	printf("records are per level (loaded from the file, kept in memory)\n");

	/* no player (main menu): no HUD */
	W32(PL_DATA + 0x2C, (u32)-1); frame();
	assert(!drawn("charged") && !drawn("CHARGED") && !drawn("u/s"));
	/* a stale object index (wrong salt): no HUD */
	W32(PL_DATA + 0x2C, 0x00060002u); frame();
	assert(!drawn("CHARGED") && !drawn("charged"));
	printf("no HUD without a player, or with a stale unit index\n");
	/* step 3a: the frame camera and the marker. Camera at the origin facing +x, z up, fov 1.0 rad, 640 x 480 */
	host_set_marker(1);
	W32(PL_DATA + 0x2C, UNIT); state(200, 0.f, 0, 0);          /* on the ground: the arc is for falls */
	WF(0x1200 + 0x18, 0); WF(0x1200 + 0x1C, 0); WF(0x1200 + 0x20, 0);
	WF(0x1200 + 0x24, 1); WF(0x1200 + 0x28, 0); WF(0x1200 + 0x2C, 0);
	WF(0x1200 + 0x30, 0); WF(0x1200 + 0x34, 0); WF(0x1200 + 0x38, 1);
	WF(0x1200 + 0x40, 1.0f);
	/* local player 0's view source (g_4e9c8c[0]): position +0x00, forward +0x20, up +0x2C, angle +0x4C
	 * (x 0.785 = the view's field of view: 1.0 rad here) */
	WF(0x1800 + 0x00, 0); WF(0x1800 + 0x04, 0); WF(0x1800 + 0x08, 0);
	WF(0x1800 + 0x20, 1); WF(0x1800 + 0x24, 0); WF(0x1800 + 0x28, 0);
	WF(0x1800 + 0x2C, 0); WF(0x1800 + 0x30, 0); WF(0x1800 + 0x34, 1);
	WF(0x1800 + 0x4C, 1.0f / 0.785f);
	WF(0x1200 + 0x40, 0.5f);                                     /* the HUD record's own angle: must NOT be used */
	W32(0x1200 + 0x48, 0x00000000); W32(0x1200 + 0x4C, (640u << 16) | 480u);   /* top 0, left 0, bottom 480, right 640 */
	WF(OBJECT + 0x64, 5.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -1.f);
	g_q = 0; frame();
	assert(g_q == 5);                                                     /* 4 square edges + the post */
	{
		const s16 *post = g_qxy[4];                                       /* the post: (5,0,-0.97) -> (5,0,0.03) */
		int i, xmin = 9999, xmax = -9999;
		for (i = 0; i < 8; i += 2) { if (post[i] < xmin) xmin = post[i]; if (post[i] > xmax) xmax = post[i]; }
		assert(xmin >= 318 && xmax <= 322);                               /* on the vertical centre line */
		assert(post[1] > post[3] || post[7] > post[5]);                   /* its foot is lower on screen than its top */
		/* the square's edge at y = -0.5 (the camera's right) lies right of centre */
		const s16 *e0 = g_qxy[0];                                         /* (4.5,-0.5) -> (5.5,-0.5) */
		assert(e0[0] > 322 && e0[2] > 322);
		/* the field of view is the source angle x 0.785 (1.0 rad), not the HUD record's (0.5): the corner
		 * (6.5, -0.5) lands at x = 320 + (0.5 / 6.5) / (tan 0.5 x 4/3) x 320 = 353.8 (0.5 rad would give ~392) */
		assert(e0[0] >= 351 && e0[0] <= 357);
		/* widescreen (16:9 into the 4:3 buffer): aspect x 4/3 -> x = 320 + (0.5 / 6.5) / (tan 0.5 x 16/9) x 320 = 345.3 */
		{
			s16 y43 = g_qxy[4][1];                                         /* the post's foot, 4:3 (~61 px below centre) */
			host_set_widescreen(1, 0); g_q = 0; frame(); assert(g_q == 5);
			assert(g_qxy[0][0] >= 343 && g_qxy[0][0] <= 348);
			assert((g_qxy[4][1] - y43) * (g_qxy[4][1] - y43) <= 1);       /* vertical unchanged */
			/* widescreen + g_510c50[5]: tangent x 0.75 -> horizontal back to 353.8, vertical scaled by 4/3 (the foot ~301 -> ~321) */
			host_set_widescreen(1, 1); g_q = 0; frame(); assert(g_q == 5);
			assert(g_qxy[0][0] >= 351 && g_qxy[0][0] <= 357);
			assert(g_qxy[4][1] >= y43 + 15 && g_qxy[4][1] <= y43 + 25);
			host_set_widescreen(0, 0); g_q = 0; frame();
			assert(g_qxy[0][0] >= 351 && g_qxy[0][0] <= 357 && (g_qxy[4][1] - y43) * (g_qxy[4][1] - y43) <= 1);
		}
	}
	/* the world frame's own projection (frame storage +0x8c), when it is the player's view: an off-centre frustum
	 * L/R/B/T = -0.80/0.60/-0.50/0.60 moves the post (on the view's centre line) to x = 0.8 / 1.4 x 640 = 365.7 */
	{
		const u32 C = 0x1200 + 0x8C + 0x34;                              /* the camera transform in the projection */
		WF(C + 0x04, 0); WF(C + 0x08, -1); WF(C + 0x0C, 0);             /* right */
		WF(C + 0x10, 0); WF(C + 0x14, 0); WF(C + 0x18, 1);              /* up */
		WF(C + 0x1C, -1); WF(C + 0x20, 0); WF(C + 0x24, 0);             /* backward */
		WF(C + 0x28, 0); WF(C + 0x2C, 0); WF(C + 0x30, 0);              /* position: the player's camera */
		WF(0x1200 + 0x8C + 0x68, -0.80f); WF(0x1200 + 0x8C + 0x6C, 0.60f); WF(0x1200 + 0x8C + 0x70, -0.50f); WF(0x1200 + 0x8C + 0x74, 0.60f);
		g_q = 0; frame(); assert(g_q == 5);
		{ int i, xmin = 9999, xmax = -9999; for (i = 0; i < 8; i += 2) { if (g_qxy[4][i] < xmin) xmin = g_qxy[4][i]; if (g_qxy[4][i] > xmax) xmax = g_qxy[4][i]; }
		  assert(xmin >= 362 && xmax <= 369); }
		/* not the player's view (camera 3 units away): ignored, back on the centre line */
		WF(C + 0x28, 3.f); g_q = 0; frame(); assert(g_q == 5);
		{ int i, xmin = 9999, xmax = -9999; for (i = 0; i < 8; i += 2) { if (g_qxy[4][i] < xmin) xmin = g_qxy[4][i]; if (g_qxy[4][i] > xmax) xmax = g_qxy[4][i]; }
		  assert(xmin >= 318 && xmax <= 322); }
		/* bounds far from this field of view (a different view's): ignored too */
		WF(C + 0x28, 0); WF(0x1200 + 0x8C + 0x68, -3.f); g_q = 0; frame();
		{ int i, xmin = 9999, xmax = -9999; for (i = 0; i < 8; i += 2) { if (g_qxy[4][i] < xmin) xmin = g_qxy[4][i]; if (g_qxy[4][i] > xmax) xmax = g_qxy[4][i]; }
		  assert(xmin >= 318 && xmax <= 322); }
		WF(0x1200 + 0x8C + 0x68, 0); WF(0x1200 + 0x8C + 0x6C, 0); WF(0x1200 + 0x8C + 0x70, 0); WF(0x1200 + 0x8C + 0x74, 0);
		printf("world frame projection: used when it is the player's view (off-centre frustum honoured), ignored when not\n");
	}
	/* fixed in the world: the player moves, the marker stays (same quads) */
	{
		s16 before[8]; memcpy(before, g_qxy[4], 16);
		WF(OBJECT + 0x64, 4.f); g_q = 0; frame(); assert(g_q == 5 && !memcmp(before, g_qxy[4], 16));
	}
	/* dropped again (menu closed) while the camera faces -x: it lands at x = -7; facing +x again, it is behind */
	WF(OBJECT + 0x64, -5.f); host_marker_reset();
	WF(0x1800 + 0x20, -1); g_q = 0; frame();
	WF(0x1800 + 0x20, 1); g_q = 0; frame(); assert(g_q == 0);
	printf("world camera projection: the marker dropped 2 units ahead - post on the centre line, right edge right of centre, fixed in the world, nothing behind\n");
	printf("aspect: 4:3, widescreen (x 4/3 horizontally) and widescreen + flag (tangent x 0.75) each project where the game's function_2fb70 would\n");
	/* step 3b: a synthetic collision BSP at 0x80000 (blocks {count, pointer}: planes +0x08, surfaces +0x28,
	 * edges +0x30, vertices +0x38). Floors at z = -1, seams 2-4 units ahead of the camera. */
	{
		const u32 B = 0x80000, PL = B + 0x100, SU = B + 0x200, ED = B + 0x300, VE = B + 0x400;
		const float sn = 0.1736482f, cs = 0.9848078f, h = 2.f * 0.1763270f;            /* 10 degrees */
		float v[10][3] = { { 3, -1, -1 }, { 3, 1, -1 }, { 3, 2, -1 }, { 3, 4, -1 }, { 1, -1, -1 }, { 1, 1, -1 },
			{ 5, -1, -1 + h }, { 1, 2, -1 }, { 5, 2, -1 - h }, { 1, -3, -1 } };
		float pl[5][4] = { { 0, 0, 1, -1 }, { -sn, 0, cs, 0 }, { sn, 0, cs, 0 }, { 1, 0, 0, 1 }, { 0, 0, -1, 1 } };
		/* surfaces: plane (0x8000 = flipped), first edge */
		u16 su[6][2] = { { 0, 2 }, { 1, 4 }, { 0, 5 }, { 2, 6 }, { 3, 2 }, { 0x8004, 7 } };
		/* edges: start, end, forward, reverse, left, right */
		u16 ed[8][6] = { { 0, 1, 0xffff, 0xffff, 0, 1 },        /* valley: flat S0 | S1 rising beyond x = 3 */
			{ 2, 3, 0xffff, 0xffff, 2, 3 },                       /* ridge: flat S2 | S3 falling beyond x = 3 */
			{ 4, 5, 0xffff, 0xffff, 0, 4 },                       /* floor | wall: not a floor seam */
			{ 4, 0, 0xffff, 0xffff, 0, 5 },                       /* flat: S0 | S5 (the same plane, used flipped) */
			{ 0, 6, 0xffff, 0xffff, 1, 0xffff }, { 7, 2, 0xffff, 0xffff, 2, 0xffff },
			{ 2, 8, 0xffff, 0xffff, 3, 0xffff }, { 9, 4, 0xffff, 0xffff, 5, 0xffff } };
		int i, j;
		for (i = 0; i < 5; i++) for (j = 0; j < 4; j++) WF(PL + i * 16 + j * 4, pl[i][j]);
		for (i = 0; i < 6; i++) { W32(SU + i * 8, (u32)su[i][0] | ((u32)su[i][1] << 16)); W32(SU + i * 8 + 4, 0); }
		for (i = 0; i < 8; i++) for (j = 0; j < 6; j += 2) W32(ED + i * 12 + j * 2, (u32)ed[i][j] | ((u32)ed[i][j + 1] << 16));
		for (i = 0; i < 10; i++) { for (j = 0; j < 3; j++) WF(VE + i * 16 + j * 4, v[i][j]); W32(VE + i * 16 + 12, 0); }
		W32(B + 0x08, 5); W32(B + 0x0C, PL); W32(B + 0x28, 6); W32(B + 0x2C, SU); W32(B + 0x30, 8); W32(B + 0x34, ED); W32(B + 0x38, 10); W32(B + 0x3C, VE);
		W32(0x1B00, B);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);   /* feet 1 above the floor */
		host_set_marker(0); host_set_seams(1);
		g_q = 0; frame();
		assert(host_seam_count(-1) == 3 && host_seam_count(1) == 1 && host_seam_count(2) == 1 && host_seam_count(0) == 1);
		assert(g_q >= 3);                                                    /* three lines; the wall edge excluded */
		/* a broken layout (an edge's vertex out of range) fails the check: nothing drawn */
		W32(B + 0x38, 9); g_q = 0; frame(); assert(g_q == 0);
		W32(B + 0x38, 10); g_q = 0; frame(); assert(g_q >= 3);
		/* out of reach (9 units across): none */
		WF(OBJECT + 0x64, -9.f); g_q = 0; frame(); assert(g_q == 0);
		WF(OBJECT + 0x64, 0.f);
		/* over the cap: 200 extra flat seams 6-7 units away come FIRST in the BSP; the 3 near ones (2-4 units) come last
		 * and must still be kept - the nearest 160, not the first 160 */
		{
			const u32 B2 = 0x90000, ED2 = B2 + 0x300, VE2 = B2 + 0x2000;
			int k;
			for (i = 0; i < 0x100; i += 4) W32(B2 + i, M32h(B + i));
			for (k = 0; k < 200; k++)                                          /* far edges: vertices 0..399 */
			{
				float y0 = -6.f + (float)(k % 12), x0 = 6.f + 0.004f * (float)k;
				WF(VE2 + (2 * k) * 16, x0); WF(VE2 + (2 * k) * 16 + 4, y0); WF(VE2 + (2 * k) * 16 + 8, -1);
				WF(VE2 + (2 * k + 1) * 16, x0); WF(VE2 + (2 * k + 1) * 16 + 4, y0 + 0.5f); WF(VE2 + (2 * k + 1) * 16 + 8, -1);
				W32(ED2 + k * 12, (u32)(2 * k) | ((u32)(2 * k + 1) << 16)); W32(ED2 + k * 12 + 4, 0xffffffffu); W32(ED2 + k * 12 + 8, 0u | (5u << 16));
			}
			for (k = 0; k < 10; k++) for (j = 0; j < 3; j++) WF(VE2 + (400 + k) * 16 + j * 4, v[k][j]);
			for (k = 0; k < 8; k++)                                            /* the original edges, after the far ones */
				for (j = 0; j < 6; j += 2)
				{
					u32 a0 = ed[k][j], a1 = ed[k][j + 1];
					if (j < 2) { a0 += 400; a1 += 400; }                           /* vertex indices moved up */
					W32(ED2 + (200 + k) * 12 + j * 2, a0 | (a1 << 16));
				}
			W32(B2 + 0x30, 208); W32(B2 + 0x34, ED2); W32(B2 + 0x38, 410); W32(B2 + 0x3C, VE2);
			/* surfaces' first edges point at the original edges, now 200 later */
			for (k = 0; k < 6; k++) W32(B2 + 0x200 + k * 8, (u32)su[k][0] | ((u32)(su[k][1] + 200) << 16));
			W32(B2 + 0x2C, B2 + 0x200); W32(B2 + 0x0C, PL);
			W32(0x1B00, B2);
			g_q = 0; frame();
			assert(host_seam_count(-1) == 160);
			assert(host_seam_count(1) == 1 && host_seam_count(2) == 1);        /* the near valley and ridge kept */
			W32(0x1B00, B);
		}
		printf("collision seams: valley, ridge and flat found (flipped plane honoured), wall edge excluded, a broken layout refused, reach honoured, nearest kept when over the cap\n");
		/* step 3c: the pieces. A structure BSP at 0xA0000: definitions (0xC8 bytes; sphere +0x60/+0x6C, collision
		 * BSP +0x70) at 0xA2000, instances (0x58 bytes) at 0xA1000. Definition 0 = the synthetic BSP above;
		 * definition 1 = the same with an edge vertex out of range. Instance 0: definition 0 turned 90 degrees
		 * about z and moved 6 ahead (world = (6 - y, x, z): its seams 3.6-7.1 units away, in front of the camera);
		 * instance 1: definition 0 100 units away; instance 2: definition 1. */
		{
			const u32 SB = 0xA0000, IN = 0xA1000, DF = 0xA2000;
			int k;
			for (k = 0; k < 2; k++)
			{
				u32 d = DF + k * 0xC8;
				WF(d + 0x60, 3); WF(d + 0x64, 0); WF(d + 0x68, -1); WF(d + 0x6C, 10);
				for (i = 0; i < 0x40; i += 4) W32(d + 0x70 + i, M32h(B + i));
			}
			W32(DF + 0xC8 + 0x70 + 0x38, 9);                                    /* definition 1: 9 vertices - broken */
			{
				float xf[3][13] = {
					{ 1, 0, 1, 0, -1, 0, 0, 0, 0, 1, 6, 0, 0 },                      /* scale, forward, left, up, position */
					{ 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 100, 0, 0 },
					{ 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 } };
				u16 dfi[3] = { 0, 0, 1 };
				for (k = 0; k < 3; k++)
				{
					for (i = 0; i < 13; i++) WF(IN + k * 0x58 + i * 4, xf[k][i]);
					W32(IN + k * 0x58 + 0x34, dfi[k]);
				}
			}
			W32(SB + 0x138, 2); W32(SB + 0x13C, DF); W32(SB + 0x140, 3); W32(SB + 0x144, IN);
			W32(0x1B04, SB);
			g_q = 0; frame();
			assert(host_seam_count(-1) == 6 && host_seam_count(-2) == 3);          /* 3 from the level, 3 from the piece */
			assert(host_seam_count(1) == 2 && host_seam_count(2) == 2 && host_seam_count(0) == 2);
			assert(host_seam_count(-3) == 1);                                       /* definition 1 refused */
			assert(g_q >= 5);                                                         /* one seam is wholly off the screen's left: not drawn (5e.9 clips to the screen) */
			{ s32 qi, qc; for (qi = 0; qi < g_q && qi < 64; qi++) for (qc = 0; qc < 8; qc++) assert(g_qxy[qi][qc] >= 0); }   /* no corner negative (the 360 read one as far right) */
			/* the piece's valley lies along x at y = 3 after the turn: one of the drawn lines must sit there.
			 * (Checked through the seam count instead of screen positions: the level's own seams are unchanged.) */
			/* a big piece: definition 0 x5 with its origin 12 units behind you - its seams (x = 3 x 5 - 12 = 3) are
			 * 3 units ahead. Its definition's +0x60/+0x6C hold a bogus tiny sphere far away: no longer trusted. */
			WF(DF + 0x60, 100); WF(DF + 0x64, 100); WF(DF + 0x68, 100); WF(DF + 0x6C, 0.5f);
			{
				float big[13] = { 5, 1, 0, 0, 0, 1, 0, 0, 0, 1, -12, 0, 0 };
				for (i = 0; i < 13; i++) WF(IN + 0x58 + i * 4, big[i]);       /* instance 1 becomes the big piece */
			}
			W32(SB + 0x140, 2);                                                   /* instances 0 and 1 only */
			g_q = 0; frame();
			assert(host_seam_count(-2) == 5);   /* the turned piece: 3; the big piece: valley (3 away) + flat (5 away), its ridge 10.4 away is out of reach. The old sphere would have rejected it: 3 */
			W32(0x1B04, 0); g_q = 0; frame(); assert(host_seam_count(-1) == 3 && g_q >= 3);
			/* step 3c.3: a scenery object in the object pool (slot 3): tag 0 = its definition, whose +0x38 -> tag 1
			 * ('hlmt'), whose +0x0C -> tag 2 ('coll'): one region, permutation 0, one bsp (node 0) = the synthetic
			 * BSP. Its node matrix (at +0x200, as node_matrices_offset +0x116 says) turns it 90 degrees and puts it
			 * 6 ahead - the same placement as the turned piece: 3 seams. */
			{
				const u32 TT = 0xB0000, DEF = 0xB1000, HL = 0xB2000, CO = 0xB3000, SO = 0xB5000;
				float nm[13] = { 1, 0, 1, 0, -1, 0, 0, 0, 0, 1, 6, 0, 0 };
				W32(TT + 0x00, 0x7363656E); W32(TT + 0x08, DEF);                  /* 'scen' */
				W32(TT + 0x10, 0x686C6D74); W32(TT + 0x18, HL);                   /* 'hlmt' */
				W32(TT + 0x20, 0x636F6C6C); W32(TT + 0x28, CO);                   /* 'coll' */
				W32(0x1B08, TT);
				W32(DEF + 0x38, 1); W32(HL + 0x0C, 2);
				W32(CO + 0x1C, 1); W32(CO + 0x20, CO + 0x100); W32(CO + 0x2C, 1);
				W32(CO + 0x100 + 4, 1); W32(CO + 0x100 + 8, CO + 0x200);          /* region 0: one permutation */
				W32(CO + 0x200 + 4, 1); W32(CO + 0x200 + 8, CO + 0x300);          /* permutation 0: one bsp */
				W32(CO + 0x300, 0);                                                 /* node 0 */
				for (i = 0; i < 0x40; i += 4) W32(CO + 0x300 + 4 + i, M32h(B + i));
				W32(SO + 0x00, 0); WF(SO + 0x30, 8); WF(SO + 0x34, 3); WF(SO + 0x38, -1); WF(SO + 0x3C, 10);
				MEM[SO + 0xAA] = 6;                                                 /* scenery */
				W32(SO + 0x114, 0x200u << 16);                                      /* node matrices offset (+0x116) */
				for (i = 0; i < 13; i++) WF(SO + 0x200 + i * 4, nm[i]);
				W32(OBJ_DATA + 3 * 12, 7); W32(OBJ_DATA + 3 * 12 + 8, SO);           /* slot 3, salt 7 */
				g_q = 0; frame();
				assert(host_object_bsps() == 1 && host_seam_count(-4) == 0 && host_seam_count(-1) == 3 && g_q >= 3);   /* found, no seams of its own */
				W32(TT + 0x10, 0x6D6F6465); g_q = 0; frame(); assert(host_object_bsps() == 0);   /* not 'hlmt': refused */
				W32(TT + 0x10, 0x686C6D74);
				MEM[SO + 0xAA] = 2; g_q = 0; frame(); assert(host_object_bsps() == 0);           /* a weapon: not looked at */
				MEM[SO + 0xAA] = 6;
				WF(SO + 0x30, 40); WF(SO + 0x3C, 2); WF(SO + 0x64, 60);                       /* sphere AND origin far */
				g_q = 0; frame(); assert(host_object_bsps() == 0);                             /* out of reach */
				/* scenery whose sphere is stale (far away) but whose origin is beside you: still looked at */
				WF(SO + 0x64, 0); g_q = 0; frame(); assert(host_object_bsps() == 1 && host_seam_count(-4) == 0);
				/* a ramp-like object: its collision one square floor QUAD (x 2-4, y -1..1, z -1), 4 edges in a loop. Its
				 * borders are no floor seams; its fan diagonal (corner 0 - corner 2) is the one seam, flat. */
				{
					const u32 Q = 0xB6000, QP = Q + 0x100, QS = Q + 0x200, QE = Q + 0x300, QV = Q + 0x400;
					float qv[4][3] = { { 2, -1, -1 }, { 4, -1, -1 }, { 4, 1, -1 }, { 2, 1, -1 } };
					int k2, j2;
					WF(QP, 0); WF(QP + 4, 0); WF(QP + 8, 1); WF(QP + 12, -1);
					W32(QS, 0u | (0u << 16)); W32(QS + 4, 0);                               /* plane 0, first edge 0 */
					for (k2 = 0; k2 < 4; k2++)
					{
						W32(QE + k2 * 12, (u32)k2 | ((u32)((k2 + 1) % 4) << 16));             /* start, end */
						W32(QE + k2 * 12 + 4, (u32)((k2 + 1) % 4) | (0xffffu << 16));         /* forward, reverse */
						W32(QE + k2 * 12 + 8, 0u | (0xffffu << 16));                           /* left S0, right none */
						for (j2 = 0; j2 < 3; j2++) WF(QV + k2 * 16 + j2 * 4, qv[k2][j2]);
					}
					W32(Q + 0x08, 1); W32(Q + 0x0C, QP); W32(Q + 0x28, 1); W32(Q + 0x2C, QS); W32(Q + 0x30, 4); W32(Q + 0x34, QE); W32(Q + 0x38, 4); W32(Q + 0x3C, QV);
					for (i = 0; i < 0x40; i += 4) W32(CO + 0x300 + 4 + i, M32h(Q + i));
					WF(SO + 0x30, 8); WF(SO + 0x3C, 10); g_q = 0; frame();
					assert(host_object_bsps() == 1 && host_seam_count(-4) == 0);       /* found (a surface near you), no seam drawn */
				}
				W32(OBJ_DATA + 3 * 12, 0); W32(OBJ_DATA + 3 * 12 + 8, 0);
				printf("objects: found through their tags and node matrices as surfaces near you (blocking, landing), but no seams of their own; a wrong tag group refused, a weapon ignored, a far object skipped\n");
			}
			printf("pieces: a turned piece's valley, ridge and flat found in world space, a far piece rejected by its measured reach, a broken definition refused, a big piece found though its origin is 12 units away\n");
		}
	}
	/* 6i.158: Object Spawning - the menus list what the level's tag table holds of H2EK's list (the names read from the
	 * map file, a tag counted only in its own group, the read trusted only if the scenario's tag reads back as the
	 * scenario's path); a chosen item is made by the game's two calls at the next tick, in front of the player */
	{
		s32 host_os_state(void); s32 host_os_found(void); u32 host_os_tag(s32 d); s32 host_os_def(const char *label); void host_os_req(s32 d); void host_os_run(void);
		u32 host_os_placed(float *rec9, u32 *made_n); void host_os_result(u32 r); u32 host_os_ai(s32 k); void host_os_actor(u32 a); u32 host_os_char(s32 d); u32 host_os_sq(s32 k); s32 host_menu_n(s32 m); const char *host_menu_label(s32 m, s32 i);
		void host_os_activate(s32 m, s32 i); void host_set_menu_open(s32 open, s32 m); s32 host_menu(void); const char *host_toast(void); void host_clear_toast(void);
		static const struct { u32 group; const char *name; } tags[15] = {
			{ 0x73636E72u, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa" },
			{ 0x76656869u, "objects\\vehicles\\warthog\\warthog" },
			{ 0x686C6D74u, "objects\\vehicles\\warthog\\warthog" },                    /* its model, the same name: not a vehicle */
			{ 0x77656170u, "objects\\weapons\\rifle\\battle_rifle\\battle_rifle" },
			{ 0x76656869u, "objects\\vehicles\\pelican\\pelican" },                    /* in the level, not in the list */
			{ 0x62697064u, "objects\\characters\\elite\\elite" },
			{ 0x65716970u, "objects\\weapons\\grenade\\frag_grenade\\frag_grenade" },
			{ 0x626C6F63u, "objects\\gear\\human\\military\\fusion_coil\\fusion_coil" },
			{ 0x77656170u, 0 },                                                         /* a 40,000-character name (across the 32 KB reads) */
			{ 0x76656869u, "objects\\vehicles\\ghost\\ghost" },
			{ 0x62697064u, "objects\\characters\\masterchief\\masterchief" },
			{ 0x77656170u, "objects\\weapons\\multiplayer\\flag\\flag" },
			{ 0x63686172u, "objects\\characters\\elite\\ai\\elite_major" },             /* 6i.159: characters - a major (its parent the plain elite), */
			{ 0x63686172u, "objects\\characters\\elite\\ai\\elite" },                   /* the plain elite (no parent; the settings and the weapon are its), */
			{ 0x63686172u, "objects\\characters\\masterchief\\ai\\swarm" } };            /* and a swarm character driving the Chief's biped */
		const u32 HDR = 0xE0000, TH = 0xE1000, INST = 0xE2000, FOFF = 0x5000, CH = 0xE3000, AIG = 0xE5000;   /* CH: the three characters' tag data, 0x400 apart */
		u32 old_inst = M32h(0x1B08), old_ai = 0, size = 0, made = 0, k, tag;
		float rec[9];
		s32 i;
		static char os_log_keep[256][200]; int os_nlog_keep = g_nlog;                            /* the log's ring put back after (later tests count its lines) */
#define OS_HUD() do { g_nlines = 0; spooky_view(0x120C); spooky_frame(); } while (0)
		memcpy(os_log_keep, g_loglines, sizeof(os_log_keep));
		g_osfile_off = FOFF;
		for (i = 0; i < 15; i++)
		{
			W32(INST + 16 * (u32)i, tags[i].group); W32(INST + 16 * (u32)i + 4, ((0xA000u + (u32)i) << 16) | (u32)i);
			if (tags[i].name) { strcpy((char *)g_osfile + size, tags[i].name); size += (u32)strlen(tags[i].name) + 1; }
			else { memset(g_osfile + size, 'x', 40000); g_osfile[size + 40000] = 0; size += 40001; }
		}
		g_osfile_size = size;
		memset(MEM + HDR, 0, 0x800);
		W32(HDR + 8, FOFF + size + 0x1000); W32(HDR + 0x2BC, 15); W32(HDR + 0x2C0, FOFF); W32(HDR + 0x2C4, size); W32(HDR + 0x2C8, FOFF + size);
		W32(TH + 0xC, 0xA0000000u); W32(TH + 0x18, 15); W32(0x1F28, TH); W32(0x1B08, INST);
		/* the characters' tag data: parent +8, unit +0x10, general settings +0x34 / +0x38 (first byte's bit 0: a swarm), weapons +0xCC / +0xD0 (the weapon's tag at +8) */
		memset(MEM + CH, 0, 0xC00);
		W32(INST + 16 * 12 + 8, CH); W32(CH + 8, 0xA00D000Du); W32(CH + 0x10, 0xA0050005u);                                   /* the major: no blocks of its own */
		W32(INST + 16 * 13 + 8, CH + 0x400); W32(CH + 0x400 + 8, 0xFFFFFFFFu); W32(CH + 0x400 + 0x10, 0xA0050005u);
		W32(CH + 0x400 + 0x34, 1); W32(CH + 0x400 + 0x38, CH + 0x700); W32(CH + 0x400 + 0xCC, 2); W32(CH + 0x400 + 0xD0, CH + 0x710); W32(CH + 0x710 + 8, 0xA0030003u); W32(CH + 0x710 + 0xCC + 8, 0xA00B000Bu);   /* (6i.171: settings for two weapons) */
		W32(INST + 16 * 14 + 8, CH + 0x800); W32(CH + 0x800 + 8, 0xFFFFFFFFu); W32(CH + 0x800 + 0x10, 0xA00A000Au);
		W32(CH + 0x800 + 0x34, 1); W32(CH + 0x800 + 0x38, CH + 0xB00); MEM[CH + 0xB00] = 1;
		old_ai = M32h(0x1D20); W32(0x1D20, AIG); MEM[AIG + 1] = 1;                                                                                 /* AI running */
		WF(OBJECT + 0x64, 10.f); WF(OBJECT + 0x68, 20.f); WF(OBJECT + 0x6C, 3.f); WF(OBJECT + 0x70, 1.f); WF(OBJECT + 0x74, 0.f); WF(OBJECT + 0x78, 0.f); WF(OBJECT + 0x168, 0.f); WF(OBJECT + 0x16C, 2.f); WF(OBJECT + 0x170, -1.5f);   /* 6i.163: the body faces +x, you look along +y and down: along the look, flattened */
		host_set_menu_open(1, 14); OS_HUD();                                                   /* the menu opened: this level read */
		assert(host_os_state() == 1 && host_os_found() == 8 && host_menu() == 14);
		assert(host_menu_n(14) == 6 && !strcmp(host_menu_label(14, 0), "Human Weapons  >") && !strcmp(host_menu_label(14, 1), "Human Vehicles  >") && !strcmp(host_menu_label(14, 2), "Covenant Vehicles  >")
		       && !strcmp(host_menu_label(14, 3), "Bipeds  >") && !strcmp(host_menu_label(14, 4), "Objects  >") && !strcmp(host_menu_label(14, 5), "Misc  >"));   /* only the headings with something; H2EK's order */
		assert(host_menu_n(20) == 5 && !strcmp(host_menu_label(20, 0), "Spawn with AI") && !strcmp(host_menu_label(20, 1), "Spawn as Ally") && !strcmp(host_menu_label(20, 2), "Spawn as Enemy") && !strcmp(host_menu_label(20, 3), "Human  >") && !strcmp(host_menu_label(20, 4), "Covenant  >"));   /* 6i.170 / 6i.171: the two sides after the AI toggle */   /* 6i.162: the toggle first */
		assert(host_menu_n(15) == 2 && !strcmp(host_menu_label(15, 0), "Frag Grenade") && !strcmp(host_menu_label(15, 1), "Battle Rifle"));
		assert(host_menu_n(18) == 1 && !strcmp(host_menu_label(18, 0), "Warthog") && host_menu_n(19) == 1 && !strcmp(host_menu_label(19, 0), "Ghost"));   /* no Pelican */
		assert(host_menu_n(21) == 1 && !strcmp(host_menu_label(21, 0), "Chief") && host_menu_n(22) == 1 && !strcmp(host_menu_label(22, 0), "Elite") && !strcmp(host_menu_label(24, 0), "Fusion Coil") && !strcmp(host_menu_label(25, 0), "MP Flag"));
		assert(host_os_tag(host_os_def("Warthog")) == 0xA0010001u && host_os_tag(host_os_def("Scorpion")) == 0xFFFFFFFFu);   /* the vehicle's tag, not its model's */
		assert(drawn("Object Spawning") && drawn("Human Weapons  >") && drawn("Misc  >"));
		host_clear_toast();
		W32(OBJECT + 0x14, 0xFFFFFFFFu);                                                           /* (on foot: no parent, as the game has it) */
		host_os_activate(18, 0); tag = host_os_placed(rec, &made); assert(made == 0);              /* A on Warthog: nothing yet - at the next tick */
		host_os_run(); tag = host_os_placed(rec, &made); OS_HUD();
		assert(made == 1 && tag == 0xA0010001u && rec[0] == 10.f && rec[1] == 24.f && rec[2] == 4.f && rec[3] == 0.f && rec[4] == 1.f && rec[5] == 0.f && rec[6] == 0.f && rec[7] == 0.f && rec[8] == 1.f);   /* 4 ahead along the facing (flattened), 1 up; facing as you do, upright */
		assert(strstr(host_toast(), "Spawned: Warthog") && drawn("Spawned: Warthog") && host_menu() == 18);   /* said; the menu left open */
		host_os_run(); host_os_placed(rec, &made); assert(made == 1);                              /* once */
		host_os_activate(15, 1); host_os_run(); tag = host_os_placed(rec, &made);                  /* a weapon: 1 ahead, 0.6 up */
		assert(made == 2 && tag == 0xA0030003u && rec[0] == 10.f && rec[1] == 21.f && rec[2] > 3.59f && rec[2] < 3.61f);
		assert(host_os_ai(-1) == 0);                                                               /* a vehicle, a weapon: no AI calls */
		{
			/* 6i.214: spawned from a seat - the unit has a parent and its own position (+0x64) is measured from the seat, so
			 * the object goes in front of where the body's root part is in the level (its first part matrix, +0x28), 3
			 * units further out to clear the vehicle; no part matrices to go by: refused, and said; on foot again: as before */
			unsigned char keep[0x40], kn[4]; float sx = RF(OBJECT + 0x64), sy = RF(OBJECT + 0x68), sz = RF(OBJECT + 0x6C); u32 par = M32h(OBJECT + 0x14);
			const u32 NO = 0x700;
			memcpy(keep, MEM + OBJECT + NO, 0x40); memcpy(kn, MEM + OBJECT + 0x114, 4);
			W32(OBJECT + 0x14, 0xE1230007u); WF(OBJECT + 0x64, 0.2f); WF(OBJECT + 0x68, 0.1f); WF(OBJECT + 0x6C, 0.3f);
			MEM[OBJECT + 0x114] = 0x34; MEM[OBJECT + 0x115] = 0; MEM[OBJECT + 0x116] = (unsigned char)(NO & 0xFF); MEM[OBJECT + 0x117] = (unsigned char)(NO >> 8);
			WF(OBJECT + NO + 0x28, 50.f); WF(OBJECT + NO + 0x2C, 60.f); WF(OBJECT + NO + 0x30, 7.f);
			host_os_activate(15, 1); host_os_run(); tag = host_os_placed(rec, &made);
			assert(made == 3 && rec[0] == 50.f && rec[1] == 64.f && rec[2] > 7.59f && rec[2] < 7.61f);      /* 1 + 3 ahead of the root part, 0.6 up */
			assert(logged(", at x100 5000 6400 7") == 1);
			MEM[OBJECT + 0x114] = 0; host_clear_toast();
			host_os_activate(15, 1); host_os_run(); host_os_placed(rec, &made);
			assert(made == 3 && strstr(host_toast(), "could not tell where you are in this seat"));
			memcpy(MEM + OBJECT + NO, keep, 0x40); memcpy(MEM + OBJECT + 0x114, kn, 4);
			W32(OBJECT + 0x14, par); WF(OBJECT + 0x64, sx); WF(OBJECT + 0x68, sy); WF(OBJECT + 0x6C, sz);
			host_os_activate(15, 1); host_os_run(); host_os_placed(rec, &made);
			assert(made == 4 && rec[0] == 10.f && rec[1] == 21.f);                                           /* on foot: as before */
			{ void host_os_made_set(u32 n); host_os_made_set(2); }                                           /* (the count as the checks below expect it) */
			host_clear_toast();
		}
		host_os_result(0xFFFFFFFFu); host_os_activate(22, 0); host_os_run(); host_os_placed(rec, &made);   /* the game makes none */
		assert(made == 3 && strstr(host_toast(), "Could not spawn: Elite (the game refused)") && host_os_ai(-1) == 0);
		host_os_result(0xE1230007u);
		/* 6i.159: a brain for a dropped biped. The Elite's character: the plain one (no parent) though the major comes
		 * first; the Chief's: the swarm one; the weapon and the settings read from the character or its parent */
		assert(host_os_char(host_os_def("Elite")) == 0xA00D000Du && host_os_char(host_os_def("Chief")) == 0xA00E000Eu && host_os_char(host_os_def("Grunt")) == 0xFFFFFFFFu && host_os_char(host_os_def("Warthog")) == 0xFFFFFFFFu);
		host_os_activate(22, 0); host_os_run(); host_os_placed(rec, &made);                        /* an Elite: made, armed, given its actor, switched on - in that order */
		assert(made == 4 && host_os_ai(-1) == 3 && host_os_ai(7) == 0 && host_os_ai(0) == 0xA0030003u && host_os_ai(1) == 0xA00D000Du && host_os_ai(2) == 0xE1230007u   /* 6i.163: no squad of this level uses it - in no squad; 6i.171: armed with the first weapon its character has settings for that the level has */
		       && host_os_ai(3) == 0xA00D000Du && host_os_ai(4) == 0xE1230007u && host_os_ai(5) == 0xAC700003u && host_os_sq(1) == 0xFFFFFFFFu && host_os_sq(3) == 0 && !strcmp(host_toast(), "Spawned: Elite (with AI, no squad of its kind: stays put)"));
		host_os_activate(21, 0); host_os_run(); host_os_placed(rec, &made);                        /* the Chief: a swarm character - a body */
		assert(made == 5 && host_os_ai(-1) == 3 && strstr(host_toast(), "Spawned: Chief (no AI: a swarm character)"));
		MEM[CH + 0xB00] = 0; host_os_actor(0xFFFFFFFFu); host_os_activate(21, 0); host_os_run();   /* not a swarm, no weapons block: armed with none; the game makes no actor - said, not switched on */
		assert(host_os_ai(-1) == 5 && host_os_ai(0) == 0xFFFFFFFFu && host_os_ai(1) == 0xA00E000Eu && strstr(host_toast(), "Spawned: Chief (no AI: the game made no actor)"));   /* 6i.171: its character has settings for no weapon: none (the Battle Rifle is on the Chief's list, but it could not fire it) */
		host_os_actor(0xAC700003u); MEM[CH + 0xB00] = 1;
		MEM[AIG + 1] = 0; host_os_activate(22, 0); host_os_run();                                  /* AI not running (multiplayer): a body */
		assert(host_os_ai(-1) == 5 && strstr(host_toast(), "Spawned: Elite (no AI: AI is off here)"));
		MEM[AIG + 1] = 1;
		W32(CH + 0x400 + 0x10, 0xA0010001u); W32(TH + 0xC, 0xA1000000u); OS_HUD();                 /* a level where only the major drives the Elite: it is used; its weapon and settings are its parent's */
		assert(host_os_char(host_os_def("Elite")) == 0xA00C000Cu);
		host_os_activate(22, 0); host_os_run();
		assert(host_os_ai(-1) == 8 && host_os_ai(0) == 0xA0030003u && host_os_ai(1) == 0xA00C000Cu && host_os_ai(3) == 0xA00C000Cu && strstr(host_toast(), "Spawned: Elite (with AI, no squad"));   /* 6i.171: the weapon its PARENT has settings for */
		W32(CH + 8, 0xFFFFFFFFu); host_os_activate(22, 0); host_os_run();                          /* a character with no settings of its own or a parent's: a body */
		assert(host_os_ai(-1) == 8 && strstr(host_toast(), "Spawned: Elite (no AI: its character has no settings)"));
		W32(CH + 8, 0xA00D000Du); W32(CH + 0x400 + 0x10, 0xA0050005u);
		W32(INST + 16 * 12, 0x686C6D74u); W32(INST + 16 * 13, 0x686C6D74u); W32(TH + 0xC, 0xA2000000u); OS_HUD();   /* a level with no character for the Elite: a body, and why */
		host_os_activate(22, 0); host_os_run();
		assert(host_os_ai(-1) == 8 && strstr(host_toast(), "Spawned: Elite (no AI: no character in this level)"));
		W32(INST + 16 * 12, 0x63686172u); W32(INST + 16 * 13, 0x63686172u); W32(TH + 0xC, 0xA3000000u); host_set_menu_open(1, 14); OS_HUD();
		made = 0; host_os_placed(rec, &made); assert(made == 10);
		/* the next level: the table's place in the header moved (found by its count), the Ghost not a vehicle here */
		W32(HDR + 0x2BC, 0); W32(HDR + 0x400, 15); W32(HDR + 0x404, FOFF); W32(HDR + 0x408, size); W32(HDR + 0x40C, FOFF + size);
		W32(INST + 16 * 9, 0x686C6D74u); W32(TH + 0xC, 0xB0000000u);
		host_set_menu_open(1, 19); OS_HUD();                                                   /* its list was open: back to the headings, made again */
		assert(host_menu() == 14 && host_os_state() == 1 && host_os_found() == 7 && host_menu_n(14) == 5 && !strcmp(host_menu_label(14, 2), "Bipeds  >"));
		host_os_req(host_os_def("Ghost")); host_os_run(); host_os_placed(rec, &made);              /* asked for what isn't here: said, nothing made */
		assert(made == 10 && strstr(host_toast(), "not in this level"));
		W32(INST + 16 * 9, 0x76656869u);
		for (i = 1; i < 15; i++) W32(INST + 16 * (u32)i, 0x686C6D74u);                             /* a level with nothing of the list */
		W32(TH + 0xC, 0xC0000000u); OS_HUD();
		assert(host_os_state() == 1 && host_os_found() == 0 && host_menu_n(14) == 1 && !strcmp(host_menu_label(14, 0), "Nothing from the list is in this level") && drawn("Nothing from the list is in this level"));
		for (i = 0; i < 15; i++) W32(INST + 16 * (u32)i, tags[i].group);
		g_osfile[0] = 'S'; W32(TH + 0xC, 0xD0000000u); OS_HUD();                                   /* the scenario's tag doesn't read back as the scenario's path: not trusted */
		assert(host_os_state() == -1 && host_os_found() == 0 && host_menu_n(14) == 1 && !strcmp(host_menu_label(14, 0), "This level's tag names could not be read") && host_os_tag(host_os_def("Warthog")) == 0xFFFFFFFFu);
		host_os_req(host_os_def("Warthog")); host_os_run(); host_os_placed(rec, &made); assert(made == 10);
		g_osfile[0] = 's'; g_osfile_size = size - 5; W32(TH + 0xC, 0xE0000000u); OS_HUD();        /* the file ends early: not trusted */
		assert(host_os_state() == -1 && host_menu_n(14) == 1);
		g_osfile_size = size; W32(TH + 0xC, 0xF0000000u); OS_HUD(); assert(host_os_state() == 1 && host_os_found() == 8);   /* and whole again */
		/* 6i.163: the level's squads. The dropped biped joins the squad whose character drives it and whose first starting
		 * location is nearest: armed with that squad's weapons (a location's own override first), given its actor IN that
		 * squad with the squad's flag, the squads refreshed as ai_attach does - not switched on by the mod, not armed from
		 * the character's weapon list */
		{
			const u32 SC = 0xE7000, SQ = 0xE7400, PAL = 0xE7800, WP = 0xE7900, LOC = 0xE7A00;
			u32 old_scn = M32h(0x1F20), n0, r0;
			memset(MEM + SC, 0, 0x1000);
			W32(0x1F20, SC); W32(SC + 0x160, 3); W32(SC + 0x164, SQ); W32(SC + 0x178, 2); W32(SC + 0x17C, PAL); W32(SC + 0x98, 2); W32(SC + 0x9C, WP);
			W32(PAL + 4, 0xA00D000Du); W32(PAL + 8 + 4, 0xA00E000Eu);                              /* the character palette: the plain elite, the Chief's (swarm) character */
			W32(WP + 4, 0xA0030003u); W32(WP + 0x28 + 4, 0xA00B000Bu);                             /* the weapon palette */
			for (i = 0; i < 3; i++)
			{
				u32 sq = SQ + 0x74 * (u32)i, loc = LOC + 0x80 * (u32)i;
				MEM[sq + 0x36] = 0; MEM[sq + 0x37] = 0; MEM[sq + 0x3C] = MEM[sq + 0x3D] = MEM[sq + 0x3E] = MEM[sq + 0x3F] = 0xFF;
				W32(sq + 0x48, 1); W32(sq + 0x4C, loc);
				MEM[loc + 0x10] = MEM[loc + 0x11] = 0xFF; MEM[loc + 0x20] = MEM[loc + 0x21] = MEM[loc + 0x22] = MEM[loc + 0x23] = MEM[loc + 0x24] = MEM[loc + 0x25] = 0xFF;
			}
			W32(SQ + 0x20, 1u << 10); MEM[SQ + 0x3C] = 0; MEM[SQ + 0x3D] = 0; WF(LOC + 4, 100.f); WF(LOC + 8, 100.f); WF(LOC + 12, 0.f);             /* squad 0: elites, far away, the flag set, weapon 0 */
			MEM[SQ + 0x74 + 0x3C] = 1; MEM[SQ + 0x74 + 0x3D] = 0; MEM[SQ + 0x74 + 0x3E] = 0; MEM[SQ + 0x74 + 0x3F] = 0;                          /* squad 1: elites, here, weapon 1 and second weapon 0 */
			WF(LOC + 0x80 + 4, 10.f); WF(LOC + 0x80 + 8, 22.f); WF(LOC + 0x80 + 12, 3.f);
			MEM[SQ + 0xE8 + 0x36] = 1; WF(LOC + 0x100 + 4, 10.f); WF(LOC + 0x100 + 8, 21.f); WF(LOC + 0x100 + 12, 3.f);                         /* squad 2: nearer still, but the other character */
			n0 = host_os_ai(-1); r0 = host_os_sq(3);
			host_os_activate(22, 0); host_os_run();                                                /* an Elite: squad 1 - the nearest of its kind */
			assert(host_os_ai(-1) == n0 + 2 && host_os_ai(0) == 0xA00B000Bu && host_os_sq(0) == 0xA0030003u && host_os_ai(1) == 0xA00D000Du && host_os_sq(1) == 1 && host_os_sq(2) == 0
			       && host_os_ai(3) == 0xA00D000Du && host_os_sq(3) == r0 + 1 && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 1)"));
			MEM[LOC + 0x80 + 0x22] = 0; MEM[LOC + 0x80 + 0x23] = 0;                                /* its starting location names its own weapon: that one */
			host_os_activate(22, 0); host_os_run(); assert(host_os_ai(0) == 0xA0030003u && host_os_sq(1) == 1);
			MEM[LOC + 0x80 + 0x20] = 1; MEM[LOC + 0x80 + 0x21] = 0;                                /* ... and its own character, not the Elite's: squad 0 then, far as it is - its flag passed on */
			host_os_activate(22, 0); host_os_run();
			assert(host_os_sq(1) == 0 && host_os_sq(2) == 1 && host_os_ai(0) == 0xA0030003u && host_os_sq(0) == 0xFFFFFFFFu && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 0)"));
			host_os_activate(21, 0); host_os_run();                                                /* the Chief: squads 1 and 2 use its character - a swarm one: a body */
			assert(strstr(host_toast(), "Spawned: Chief (no AI: a swarm character)"));
			W32(SC + 0x160, 0); n0 = host_os_ai(-1);
			host_os_activate(22, 0); host_os_run();                                                /* a level with no squads: 6i.159's way, unarmed, in no squad */
			assert(host_os_ai(-1) == n0 + 3 && host_os_ai(0) == 0xA0030003u && host_os_sq(1) == 0xFFFFFFFFu && strstr(host_toast(), "no squad of its kind: stays put"));   /* 6i.171: in no squad, armed with its character's first weapon */
			/* 6i.170: an armed squad before an unarmed one; the biped's own base character's squad before another's; a usual
			 * weapon when no squad names one (the first of its list the level has), a Jackal's always; Spawn as ally */
			{
				u32 host_os_usual(s32 d, s32 *always, u32 character); void host_os_set_tag(s32 d, u32 tag); void host_set_os_ally(s32 on);
				s32 always, dE = host_os_def("Elite"), dPR = host_os_def("Plasma Rifle"), dPP = host_os_def("Plasma Pistol"), dN = host_os_def("Needler"), dJ = host_os_def("Jackal");
				const u32 EO = 0xE8000, AP = 0xE8400, AD = 0xE8800;
				W32(SC + 0x160, 3);
				MEM[LOC + 0x80 + 0x20] = MEM[LOC + 0x80 + 0x21] = 0xFF;                                /* squad 1: the Elite's again, here ... */
				MEM[LOC + 0x80 + 0x22] = MEM[LOC + 0x80 + 0x23] = 0xFF; MEM[SQ + 0x74 + 0x3C] = MEM[SQ + 0x74 + 0x3D] = 0xFF;   /* ... but it names no weapon */
				host_os_activate(22, 0); host_os_run();
				assert(host_os_sq(1) == 0 && host_os_ai(0) == 0xA0030003u && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 0)"));   /* squad 0, far but armed */
				MEM[SQ + 0x3C] = MEM[SQ + 0x3D] = 0xFF;                                                /* no squad of its kind names one: the nearest - armed with the first weapon its character has settings for (6i.171) */
				host_os_activate(22, 0); host_os_run();
				assert(host_os_sq(1) == 1 && host_os_ai(0) == 0xA0030003u && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 1)"));
				/* a usual weapon: the first of the kind's list that the level has AND the character has settings for; none
				 * of the list: the character's own first; a character with settings for none: none */
				assert(host_os_usual(dE, &always, 0xA00D000Du) == 0xA0030003u && !always);
				host_os_set_tag(dN, 0xA0770077u); assert(host_os_usual(dE, &always, 0xA00D000Du) == 0xA0030003u);   /* a Needler in the level, but no settings for it: not that */
				W32(CH + 0x710 + 0xCC + 8, 0xA0770077u); assert(host_os_usual(dE, &always, 0xA00D000Du) == 0xA0770077u);   /* settings for it: third on an Elite's list */
				host_os_set_tag(dPR, 0xA0660066u); assert(host_os_usual(dE, &always, 0xA00D000Du) == 0xA0770077u);   /* a Plasma Rifle (first on the list) without settings: still the Needler */
				assert(host_os_usual(dE, &always, 0xA00C000Cu) == 0xA0770077u);                         /* the major: its parent's settings count */
				host_os_activate(22, 0); host_os_run();
				assert(host_os_sq(1) == 1 && host_os_ai(0) == 0xA0770077u && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 1)"));   /* armed with it */
				MEM[SQ + 0x74 + 0x3C] = 1; MEM[SQ + 0x74 + 0x3D] = 0;                                  /* squad 1 names the palette's second weapon - which this character now has no settings for: refused, the usual one */
				host_os_activate(22, 0); host_os_run(); assert(host_os_sq(1) == 1 && host_os_ai(0) == 0xA0770077u);
				MEM[SQ + 0x74 + 0x3C] = MEM[SQ + 0x74 + 0x3D] = 0xFF; MEM[SQ + 0x3C] = 0; MEM[SQ + 0x3D] = 0;
				host_os_activate(22, 0); host_os_run(); assert(host_os_sq(1) == 0 && host_os_ai(0) == 0xA0030003u);   /* a squad that names one it can fire: the squad's, not the usual */
				assert(host_os_usual(dJ, &always, 0xA00D000Du) == 0xA0770077u && always);               /* a Jackal: its own always - the Needler while it has no Plasma Pistol it can fire */
				host_os_set_tag(dPP, 0xA0880088u); assert(host_os_usual(dJ, &always, 0xA00D000Du) == 0xA0770077u && always);
				W32(CH + 0x710 + 8, 0xA0880088u); assert(host_os_usual(dJ, &always, 0xA00D000Du) == 0xA0880088u && always);   /* ... the Plasma Pistol once it has */
				W32(CH + 0x710 + 8, 0xA0030003u);
				assert(host_os_usual(host_os_def("Warthog"), &always, 0xA00E000Eu) == 0xFFFFFFFFu && host_os_usual(dE, &always, 0xA00E000Eu) == 0xFFFFFFFFu);   /* a character with settings for no weapon: nothing */
				assert(host_os_usual(host_os_def("Hunter"), &always, 0xA00D000Du) == 0xA0030003u);      /* a kind not on the list: its character's first */
				host_os_set_tag(dN, 0xFFFFFFFFu); host_os_set_tag(dPR, 0xFFFFFFFFu); host_os_set_tag(dPP, 0xFFFFFFFFu); W32(CH + 0x710 + 0xCC + 8, 0xA00B000Bu);
				W32(PAL + 8 + 4, 0xA00C000Cu); MEM[SQ + 0xE8 + 0x3C] = 0; MEM[SQ + 0xE8 + 0x3D] = 0;     /* squad 2 (nearest): the MAJOR's, armed; squad 1: the plain Elite's, armed again */
				MEM[SQ + 0x74 + 0x3C] = 1; MEM[SQ + 0x74 + 0x3D] = 0; MEM[SQ + 0x3C] = MEM[SQ + 0x3D] = 0xFF;
				{                                                                                       /* (the major drives the Elite's biped too) */
				  host_os_activate(22, 0); host_os_run();
				  assert(host_os_sq(1) == 1 && host_os_ai(1) == 0xA00D000Du);                           /* the base character's squad, not the nearer major's */
				  MEM[SQ + 0x74 + 0x3C] = MEM[SQ + 0x74 + 0x3D] = 0xFF; host_os_activate(22, 0); host_os_run();
				  assert(host_os_sq(1) == 2 && host_os_ai(1) == 0xA00C000Cu); }                         /* ... unless only the major's is armed */
				W32(PAL + 8 + 4, 0xA00E000Eu); MEM[SQ + 0xE8 + 0x3C] = MEM[SQ + 0xE8 + 0x3D] = 0xFF; MEM[SQ + 0x74 + 0x3C] = 1; MEM[SQ + 0x74 + 0x3D] = 0;
				/* 6i.170 / 6i.171: Spawn as Ally / Spawn as Enemy - a squad of that side (its team at peace with yours, or
				 * not): its own kind's first, then an armed one; the squad's team given to the unit and its actor; another
				 * kind's squad - its own character and usual weapon; no squad of that side - none, and a team of that side */
				{
					void host_set_os_enemy(s32 on); void host_set_os_ai(s32 on);
					const u32 PEACE = 0xE8C00; u8 swarm_was = MEM[CH + 0xB00];
					#define TEAM_OF(q) MEM[SQ + 0x74 * (q) + 0x24]
					memset(MEM + EO, 0, 0x200); memset(MEM + AP, 0, 0x100); memset(MEM + AD, 0, 0x888 * 4); memset(MEM + PEACE, 0, 0x100);
					W32(OBJ_DATA + 7 * 12, 0xE123); W32(OBJ_DATA + 7 * 12 + 8, EO);                    /* the made object: slot 7, salt 0xE123 */
					W32(0x1DF8, AP); W32(AP + 0x20, 4); W32(AP + 0x44, AD);                            /* the actors' pool; the actor made is 0xAC700003 */
					W32(0x1DFC, PEACE); W32(PEACE + 0xC4 + 4, 1u << 1);                                /* the peace table: team 2 (human) with team 1 (you) - bit 33 */
					MEM[OBJECT + 0x138] = 1; MEM[OBJECT + 0x139] = 0;
					TEAM_OF(0) = 3; TEAM_OF(1) = 3; TEAM_OF(2) = 2; MEM[CH + 0xB00] = 0;                /* squads 0, 1: Elites, covenant; squad 2: another kind (not a swarm now), human */
					MEM[SQ + 0x3C] = 0; MEM[SQ + 0x3D] = 0;                                            /* squad 0 armed (far), squad 1 armed (here) */
					MEM[EO + 0x138] = 9; MEM[AD + 3 * 0x888 + 0x24] = 9;
					host_os_activate(22, 0); host_os_run();                                            /* neither: its own kind's squad, the teams as the game made them */
					assert(host_os_sq(1) == 1 && MEM[EO + 0x138] == 9 && MEM[AD + 3 * 0x888 + 0x24] == 9 && !strstr(host_toast(), "ally") && !strstr(host_toast(), "enemy"));
					host_set_os_ally(1); host_os_activate(22, 0); host_os_run();                       /* Ally: the only squad at peace with you is the other kind's - joined, as an Elite, with an Elite's own weapon (not that squad's) */
					assert(host_os_sq(1) == 2 && host_os_ai(1) == 0xA00D000Du && host_os_ai(0) == 0xA0030003u && MEM[EO + 0x138] == 1 && MEM[AD + 3 * 0x888 + 0x24] == 1
					       && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 2, ally)"));   /* 6i.172: on YOUR team (1), not the squad's (2) */
					TEAM_OF(1) = 2; host_os_activate(22, 0); host_os_run();                            /* an Elite squad at peace with you too: its own kind's first, armed by it */
					assert(host_os_sq(1) == 1 && host_os_ai(0) == 0xA00B000Bu && MEM[EO + 0x138] == 1 && MEM[AD + 3 * 0x888 + 0x24] == 1 && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 1, ally)"));
					host_set_os_enemy(1); host_os_activate(22, 0); host_os_run();                      /* Enemy (Ally goes off): the squad not at peace - squad 0, far as it is */
					assert(host_os_sq(1) == 0 && host_os_ai(0) == 0xA0030003u && MEM[EO + 0x138] == 3 && MEM[AD + 3 * 0x888 + 0x24] == 3 && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 0, enemy)"));
					TEAM_OF(0) = 2; host_os_activate(22, 0); host_os_run();                            /* no squad at war with you: none - the first team at war with yours (3) */
					assert(host_os_sq(1) == 0xFFFFFFFFu && MEM[EO + 0x138] == 3 && MEM[AD + 3 * 0x888 + 0x24] == 3 && !strcmp(host_toast(), "Spawned: Elite (with AI, no squad of that side: stays put, enemy)"));
					TEAM_OF(0) = 3; TEAM_OF(1) = 3; TEAM_OF(2) = 3; host_set_os_ally(1); host_os_activate(22, 0); host_os_run();   /* Ally, no squad at peace: none - your own team */
					assert(host_os_sq(1) == 0xFFFFFFFFu && MEM[EO + 0x138] == 1 && MEM[AD + 3 * 0x888 + 0x24] == 1 && !strcmp(host_toast(), "Spawned: Elite (with AI, no squad of that side: stays put, ally)"));
					TEAM_OF(2) = 0; W32(PEACE + 0xC4, 0xFFFFFFFFu); host_os_activate(22, 0); host_os_run();   /* a squad with no team of its own (0): never a side's */
					assert(host_os_sq(1) == 0xFFFFFFFFu);
					W32(PEACE + 0xC4, 0);
					MEM[EO + 0x138] = 9; MEM[AD + 3 * 0x888 + 0x24] = 9; host_set_os_ai(0);
					host_os_activate(22, 0); host_os_run();                                            /* a body with no brain: its team all the same, no actor touched */
					assert(MEM[EO + 0x138] == 1 && MEM[AD + 3 * 0x888 + 0x24] == 9 && !strcmp(host_toast(), "Spawned: Elite (ally)"));
					host_set_os_enemy(1); host_os_activate(22, 0); host_os_run();
					assert(MEM[EO + 0x138] == 3 && !strcmp(host_toast(), "Spawned: Elite (enemy)"));
					host_set_os_ai(1); host_set_os_enemy(0);
					host_set_menu_open(1, 20); OS_HUD();                                               /* the menu: one or the other */
					host_os_activate(20, 1); OS_HUD(); assert(drawn("Spawn as Ally: on") && drawn("Spawn as Enemy: off"));
					host_os_activate(20, 2); OS_HUD(); assert(drawn("Spawn as Ally: off") && drawn("Spawn as Enemy: on"));
					host_os_activate(20, 1); OS_HUD(); assert(drawn("Spawn as Ally: on") && drawn("Spawn as Enemy: off"));
					host_os_activate(20, 1); OS_HUD(); assert(drawn("Spawn as Ally: off") && drawn("Spawn as Enemy: off"));
					/* 6i.182: an ally that follows - the level's "follow closest player" order (the one with the nearest area),
					 * an EMPTY squad, the ally's actor made in it, the squad given the order; the next ally joins the same squad
					 * (no second order); no follow order in the level - the order with the area nearest you, and every two
					 * seconds the squad's target set to you and the game's own areas-around-the-target run; nobody left in the
					 * squad - let go; no empty squad, or no order to use - as before */
					{
						u32 host_os_order(s32 k); u32 host_os_follow(s32 k); s32 host_os_fw(s32 k); void host_os_fw_reset(void); void host_os_follow_keep(u32 tick);
						const u32 ORD = 0xE8D00, ZN = 0xE8E00, AR = 0xE8E80, LST = 0xE9000, RP = 0xE9080, RD = 0xE9100;
						u32 scn = M32h(0x1F20), o240 = M32h(scn + 0x240), o244 = M32h(scn + 0x244), o168 = M32h(scn + 0x168), o16c = M32h(scn + 0x16C), me_unit = M32h(PL_DATA + 0x2C);
						int q;
						#define RTS(k) (RD + 0x98u * (u32)(k))
						memset(MEM + ORD, 0, 0x400 + 0x300);
						W32(scn + 0x240, 2); W32(scn + 0x244, ORD); W32(scn + 0x168, 1); W32(scn + 0x16C, ZN);
						W32(ZN + 0x30, 2); W32(ZN + 0x34, AR);
						WF(AR + 0x24, 10.f); WF(AR + 0x28, 21.f); WF(AR + 0x2C, 3.f); MEM[AR + 0x30] = 0xFF; MEM[AR + 0x31] = 0xFF;                       /* area 0: where you are */
						WF(AR + 0x88 + 0x24, 90.f); WF(AR + 0x88 + 0x28, 21.f); WF(AR + 0x88 + 0x2C, 3.f); MEM[AR + 0x88 + 0x30] = 0xFF; MEM[AR + 0x88 + 0x31] = 0xFF;   /* area 1: 80 away */
						W32(ORD + 0x54, 1); W32(ORD + 0x58, LST); MEM[LST + 6] = 0;                             /* order 0: not a follow order; its area the near one */
						W32(ORD + 0x7C + 0x24, 0x10); W32(ORD + 0x7C + 0x54, 1); W32(ORD + 0x7C + 0x58, LST + 8); MEM[LST + 8 + 6] = 1;   /* order 1: follow closest player; its area the far one */
						W32(0x1F40, RP); W32(RP + 0x20, 3); W32(RP + 0x44, RD);
						for (q = 0; q < 3; q++) { MEM[RTS(q) + 0x08] = (u8)(q < 2 ? 2 : 0); MEM[RTS(q) + 0x0A] = (u8)(q < 2 ? 2 : 0); MEM[RTS(q) + 0x2A] = 0xFF; MEM[RTS(q) + 0x2B] = 0xFF; W32(RTS(q) + 0x80, 0xFFFFFFFFu); }
						TEAM_OF(0) = 3; TEAM_OF(1) = 2; TEAM_OF(2) = 3; W32(PEACE + 0xC4 + 4, 1u << 1);
						/* 6i.185: the order chosen is one that is NOT a follow order (0, its area the near one) - the mod keeps the
						 * squad's areas round you itself; the follow order (1) is left alone */
						host_os_fw_reset(); host_set_os_ally(1); host_os_activate(22, 0); host_os_run();
						assert(host_os_sq(1) == 2 && host_os_fw(0) == 2 && host_os_fw(1) == 0 && host_os_fw(2) == 1 && host_os_order(2) == 1 && host_os_order(0) == 2 && host_os_order(1) == 0);   /* the empty squad (2: an enemy's by its own team), order 0, given once */
						assert(host_os_ai(1) == 0xA00D000Du && MEM[EO + 0x138] == 1 && MEM[AD + 3 * 0x888 + 0x24] == 1 && !strcmp(host_toast(), "Spawned: Elite (with AI, follows you, squad 2, ally)"));   /* its own character; on YOUR team */
						MEM[RTS(2) + 0x08] = 1; MEM[RTS(2) + 0x0A] = 1;                                           /* (the game's count once it is in) */
						{
							/* every two seconds: the squad's target is you, and the game's routine is run with an order of the mod's own -
							 * 0x7C bytes a whole number of orders away from the scenario's, no flags, a radius of 1, and for its areas the
							 * level's areas nearest you (here both, the near one first) */
							u32 host_os_fw_def(u32 *n); u32 n_c = 0, def;
							host_os_follow_keep(M32h(TIME + 8));
							def = host_os_fw_def(&n_c);
							assert(host_os_follow(2) == 1 && host_os_follow(1) == 2 && M32h(RTS(2) + 0x80) == me_unit);
							assert(def >= 0xE9C00 && def < 0xE9C00 + 0x7C && (def - ORD) % 0x7C == 0 && host_os_follow(0) == (def - ORD) / 0x7C);   /* the number the routine multiplies by 0x7C lands on it */
							assert(M32h(def + 0x24) == 0 && M32f(def + 0x50) == 1.f && M32h(def + 0x54) == 2 && n_c == 2 && M32h(def + 0x58) == 0xE9D00);
							assert(M32h(0xE9D00 + 4) == 0x00000000u && M32h(0xE9D00 + 12) == 0x00010000u);            /* zone 0 area 0 (where you are), then zone 0 area 1 */
							host_os_follow_keep(M32h(TIME + 8) + 15); assert(host_os_follow(2) == 1);                /* half a second later: not yet */
							W32(RTS(2) + 0x80, 0xFFFFFFFFu); host_os_follow_keep(M32h(TIME + 8) + 30); assert(host_os_follow(2) == 2 && M32h(RTS(2) + 0x80) == me_unit);   /* a second: again (6i.186: was two) */
							/* you have moved to the far area: it comes first now */
							{ float px = M32f(OBJECT + 0x64); WF(OBJECT + 0x64, 90.f); host_os_follow_keep(M32h(TIME + 8) + 60); WF(OBJECT + 0x64, px); }
							assert(host_os_follow(2) == 3 && M32h(0xE9D00 + 4) == 0x00010000u && M32h(0xE9D00 + 12) == 0x00000000u);
							/* an area placed on something that moves is left out */
							MEM[AR + 0x88 + 0x30] = 3; MEM[AR + 0x88 + 0x31] = 0; host_os_follow_keep(M32h(TIME + 8) + 90); host_os_fw_def(&n_c);
							assert(host_os_follow(2) == 4 && n_c == 1 && M32h(0xE9D00 + 4) == 0x00000000u);
							MEM[AR + 0x88 + 0x30] = 0xFF; MEM[AR + 0x88 + 0x31] = 0xFF;
						}
						host_os_activate(22, 0); host_os_run();
						assert(host_os_sq(1) == 2 && host_os_order(2) == 1 && !strcmp(host_toast(), "Spawned: Elite (with AI, follows you, squad 2, ally)"));   /* a second ally: the same squad, no second order */
						MEM[RTS(2) + 0x0A] = 0; host_os_follow_keep(M32h(TIME + 8) + 400); assert(host_os_follow(2) == 4 && host_os_fw(0) == -1);   /* nobody left alive in it: let go */
						/* only follow orders in the level: the one with the nearest area, and the game is left to keep it */
						W32(ORD + 0x24, 0x10); host_os_fw_reset(); MEM[RTS(2) + 0x08] = 0; MEM[RTS(2) + 0x0A] = 0; MEM[RTS(2) + 0x2A] = 0xFF; MEM[RTS(2) + 0x2B] = 0xFF;
						host_os_activate(22, 0); host_os_run();
						assert(host_os_sq(1) == 2 && host_os_fw(1) == 0 && host_os_fw(2) == 0 && host_os_order(2) == 1 && host_os_order(1) == 0);
						MEM[RTS(2) + 0x08] = 1; MEM[RTS(2) + 0x0A] = 1;
						host_os_follow_keep(M32h(TIME + 8) + 600); assert(host_os_follow(2) == 0);
						W32(ORD + 0x24, 0);
						/* no empty squad: as before - the squad at peace with you */
						MEM[RTS(2) + 0x08] = 3; MEM[RTS(2) + 0x0A] = 3; host_os_fw_reset();
						host_os_activate(22, 0); host_os_run();
						assert(host_os_sq(1) == 1 && host_os_order(2) == 0 && !strcmp(host_toast(), "Spawned: Elite (with AI, squad 1, ally)"));
						/* an empty squad, but the level has no order that helps (none flagged, none with areas): as before */
						MEM[RTS(2) + 0x08] = 0; MEM[RTS(2) + 0x0A] = 0; W32(ORD + 0x54, 0); W32(ORD + 0x7C + 0x54, 0); W32(ORD + 0x7C + 0x24, 0);
						host_os_activate(22, 0); host_os_run();
						assert(host_os_sq(1) == 1 && host_os_order(2) == 0);
						/* Enemy: never this way */
						W32(ORD + 0x54, 1); W32(ORD + 0x7C + 0x54, 1); W32(ORD + 0x7C + 0x24, 0x10); host_set_os_enemy(1); host_os_activate(22, 0); host_os_run();
						assert(host_os_order(2) == 0 && strstr(host_toast(), "enemy") && !strstr(host_toast(), "follows"));
						host_set_os_enemy(0); host_os_fw_reset();
						/* 6i.183: a character that can fly (its movement settings' 0x20) is told to take off once its actor is made -
						 * the game's own request (0x2C, 1) to its unit; one that cannot is not; with no AI nothing is asked */
						{
							u32 host_os_fly(s32 k); void host_os_fly_reset(void);
							u32 c5c = M32h(CH + 0x400 + 0x5C), c60 = M32h(CH + 0x400 + 0x60);
							W32(scn + 0x240, 0);                                                          /* (no orders: a plain spawn) */
							W32(CH + 0x400 + 0x5C, 1); W32(CH + 0x400 + 0x60, CH + 0x7C0); MEM[CH + 0x7C0] = 0x20;
							host_os_fly_reset(); host_os_activate(22, 0); host_os_run();
							assert(host_os_fly(3) == 1 && host_os_fly(0) == 0xE1230007u && host_os_fly(1) == 0x2C && host_os_fly(2) == 1 && strstr(host_toast(), ", flying)"));
							MEM[CH + 0x7C0] = 0x1F; host_os_activate(22, 0); host_os_run();
							assert(host_os_fly(3) == 1 && !strstr(host_toast(), "flying"));               /* cannot fly: not asked */
							MEM[CH + 0x7C0] = 0x20; host_set_os_ai(0); host_os_activate(22, 0); host_os_run();
							assert(host_os_fly(3) == 1 && !strstr(host_toast(), "flying"));               /* no AI: nothing to ask */
							host_set_os_ai(1);
							W32(CH + 0x400 + 0x5C, c5c); W32(CH + 0x400 + 0x60, c60);
						}
						/* 6i.183: the game's region and its caches in the log, once a level */
						{
							void host_cache_log(void); const u32 PH = 0xE9400;
							memset(MEM + PH, 0, 0x200); memset(MEM + 0x1F50, 0, 0x44);
							W32(0x1F50, 1); W32(0x1F50 + 0xC + 4, 0x01000000u); W32(0x1F50 + 0x20 + 4, 0x01400000u);
							W32(0x1F88, PH); W32(PH + 0x30, 0x1000); W32(PH + 0x34, 12);                 /* texture: 0x1000 pages of 4 KB */
							W32(0x1F8C, PH + 0x80); W32(PH + 0x80 + 0x30, 0x680); W32(PH + 0x80 + 0x34, 12);
							W32(0x1F90, PH + 0x100); W32(PH + 0x100 + 0x30, 0xC0); W32(PH + 0x100 + 0x34, 14);
							host_cache_log();
							assert(logged("memory: the game's region - stage 1, used to 0x01000000, ends 0x01400000, left 4096 KB"));
							assert(logged("memory: caches KB - texture 16384, geometry 6656, animation ") && logged(", sound 3072; the console: "));
							W32(0x1F88, 0); W32(0x1F8C, 0); W32(0x1F90, 0); memset(MEM + 0x1F50, 0, 0x44);
						}
						W32(scn + 0x240, o240); W32(scn + 0x244, o244); W32(scn + 0x168, o168); W32(scn + 0x16C, o16c); W32(0x1F40, 0);
						#undef RTS
					}
					TEAM_OF(0) = TEAM_OF(1) = TEAM_OF(2) = 0; MEM[CH + 0xB00] = swarm_was; W32(0x1DFC, 0);
					#undef TEAM_OF
				}
				host_set_os_ally(0); W32(0x1DF8, 0); W32(OBJ_DATA + 7 * 12, 0); W32(OBJ_DATA + 7 * 12 + 8, 0); MEM[OBJECT + 0x138] = 0;
			}
			W32(0x1F20, old_scn);
		}
		/* 6i.162: Bipeds > Spawn with AI (on to begin with) - A on it turns it off: the biped a body, no AI call, no reason given; A again: on */
		{
			void host_set_os_ai(s32 on); s32 host_os_ai_on(void); u32 m1, n1 = host_os_ai(-1);
			host_os_placed(rec, &m1);
			host_set_menu_open(1, 20); OS_HUD(); assert(host_os_ai_on() == 1 && drawn("Spawn with AI: on"));
			host_os_activate(20, 0); OS_HUD(); assert(host_os_ai_on() == 0 && drawn("Spawn with AI: off") && host_menu() == 20);
			host_os_activate(22, 0); host_os_run(); host_os_placed(rec, &made);
			assert(made == m1 + 1 && host_os_ai(-1) == n1 && !strcmp(host_toast(), "Spawned: Elite"));
			host_os_activate(20, 0); assert(host_os_ai_on() == 1);
			host_os_activate(22, 0); host_os_run(); host_os_placed(rec, &made);
			assert(made == m1 + 2 && host_os_ai(-1) == n1 + 3 && strstr(host_toast(), "Spawned: Elite (with AI, no squad"));
			host_set_menu_open(1, 14);
		}
		/* 6i.160: flying the free camera - in front of the camera, along where it looks (no lift), facing its heading */
		{
			void host_set_free_cam(s32 on, s32 block); void host_fc_set(float x, float y, float z, float yaw, float pitch); void host_fc_level_reset(void); s32 host_free_cam(void);
			u32 m0;
			host_os_placed(rec, &m0);
			host_set_free_cam(1, 1); host_fc_set(100.f, 200.f, 50.f, 0.f, 0.f);                    /* looking along +x, level */
			host_os_activate(18, 0); host_os_run(); host_os_placed(rec, &made);                    /* a Warthog: 4 along the look, at the camera's height */
			assert(made == m0 + 1 && rec[0] > 103.99f && rec[0] < 104.01f && rec[1] > 199.99f && rec[1] < 200.01f && rec[2] > 49.99f && rec[2] < 50.01f && rec[3] > 0.999f && rec[4] > -0.001f && rec[4] < 0.001f && rec[5] == 0.f && rec[8] == 1.f);
			host_fc_set(100.f, 200.f, 50.f, 1.5707963f, -0.5235988f);                              /* turned to +y, looking 30 degrees down */
			host_os_activate(15, 1); host_os_run(); host_os_placed(rec, &made);                    /* a weapon: 1 along the look - 0.866 ahead, 0.5 down; facing +y, upright */
			assert(rec[0] > 99.99f && rec[0] < 100.01f && rec[1] > 200.86f && rec[1] < 200.87f && rec[2] > 49.49f && rec[2] < 49.51f && rec[4] > 0.999f && rec[5] == 0.f && rec[8] == 1.f);
			host_fc_set(100.f, 200.f, 50.f, 0.f, -1.5707963f);                                     /* straight down: under the camera, facing its heading */
			host_os_activate(15, 1); host_os_run(); host_os_placed(rec, &made);
			assert(rec[2] > 48.99f && rec[2] < 49.01f && rec[3] > 0.999f && rec[5] == 0.f);
			W32(TIME + 8, 500); host_fc_level_reset(); assert(host_free_cam() == 1);               /* mid-level: the camera stays */
			W32(TIME + 8, 0); host_fc_level_reset();                                               /* a level's first tick: the free camera off, said */
			assert(host_free_cam() == 0 && strstr(host_toast(), "Free camera: off (the level started)"));
			host_os_activate(18, 0); host_os_run(); host_os_placed(rec, &made);                    /* and spawning is in front of you again */
			assert(rec[0] == 10.f && rec[1] == 24.f && rec[2] == 4.f);
			host_set_free_cam(0, 1); W32(TIME + 8, 500);
			made = 30 - 20;                                                                        /* (the count the next check starts from) */
			host_os_placed(rec, &m0); k = m0;
		}
		{ u32 before; host_os_placed(rec, &before); for (k = 0; k < 20; k++) { host_os_activate(18, 0); host_os_run(); } host_os_placed(rec, &made); assert(made == before + 20); }                     /* many: each one made, nothing kept */
		assert(logged("object spawning: scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa - 8 of the list's ") && logged("object spawning: Warthog tag 0xA0010001 -> object 0xE1230007") && logged(" - the tag names could not be read"));
		/* 6i.177 / 6i.178: the level's scenery and crates are COUNTED (the log), not listed - no heading, the lists as they
		 * were. The Warthog's model variants other than its default listed after it, named from the map file's string
		 * table, spawned with the variant's name in the placement; names unreadable or not as long as the id says:
		 * numbered; a single variant: nothing added */
		{
			void host_os_rebuild(void); u32 host_os_variant(void); s32 q; u32 placed;
			const u32 VD = 0xE9800, MD = 0xE9900, VP = 0xE9A00, NS = 300;
			u32 size0 = g_osfile_size, fsize0 = M32h(HDR + 8), vdata = M32h(INST + 16 * 1 + 8), mdata = M32h(INST + 16 * 2 + 8), bo, off;
			for (i = 0; i < 15; i++) W32(INST + 16 * (u32)i, tags[i].group);
			W32(TH + 0xC, 0xA0000000u); host_os_rebuild(); OS_HUD();
			assert(host_menu_n(14) == 6 && !strcmp(host_menu_label(14, 5), "Misc  >") && host_menu_n(24) == 1 && host_menu_n(18) == 1);   /* no new heading; the lists as they were (6i.224: the level's props are no longer counted) */
			W32(INST + 16 * 4, 0x7363656Eu); host_os_rebuild();                                   /* the Pelican's tag as scenery: two counted */
			assert(host_menu_n(14) == 6 && host_menu_n(24) == 1);
			host_set_menu_open(1, 24); OS_HUD(); assert(drawn("SPOOKY GHOST  Objects") && !drawn("level props"));   /* 6i.224: the title plain again - the prop count is gone */
			host_set_menu_open(1, 14); OS_HUD();
			W32(INST + 16 * 4, tags[4].group);
			/* the Warthog: its tag's default variant and model; the model's three variants; the file's string table */
			memset(MEM + VD, 0, 0x300);
			W32(INST + 16 * 1 + 8, VD); W32(VD + 0x30, 0x07000010u); W32(VD + 0x38, 0xA0020002u);
			W32(INST + 16 * 2 + 8, MD); W32(MD + 0x50, 3); W32(MD + 0x54, VP);
			W32(VP, 0x07000010u); W32(VP + 0x38, 0x05000011u); W32(VP + 0x70, 0x04000012u);
			bo = size0 + 4 * NS; off = 0;
			for (q = 0; q < (s32)NS; q++)
			{
				const char *sv = q == 0 ? "" : q == 0x10 ? "default" : q == 0x11 ? "gauss" : q == 0x12 ? "ptur" : q == 0x13 ? "damaged" : "x";
				memcpy(g_osfile + size0 + 4 * q, &off, 4); strcpy((char *)g_osfile + bo + off, sv); off += (u32)strlen(sv) + 1;
			}
			g_osfile_size = bo + off; W32(HDR + 8, FOFF + g_osfile_size + 0x1000);
			W32(HDR + 0x160, NS); W32(HDR + 0x164, off); W32(HDR + 0x168, FOFF + size0); W32(HDR + 0x16C, FOFF + bo);
			host_os_rebuild();
			assert(host_menu_n(18) == 3 && !strcmp(host_menu_label(18, 0), "Warthog") && !strcmp(host_menu_label(18, 1), "Warthog (Gauss)") && !strcmp(host_menu_label(18, 2), "Warthog (ptur)"));
			W32(VP + 0x70, 0x07000013u); host_os_rebuild();                                        /* 6i.185: a variant named "damaged" is not listed */
			assert(host_menu_n(18) == 2 && !strcmp(host_menu_label(18, 1), "Warthog (Gauss)"));
			W32(VP + 0x70, 0x04000012u); host_os_rebuild(); assert(host_menu_n(18) == 3);
			host_os_activate(18, 1); host_os_run(); placed = host_os_placed(rec, &made);
			assert(placed == 0xA0010001u && host_os_variant() == 0x05000011u && !strcmp(host_toast(), "Spawned: Warthog (Gauss)") && rec[1] == 24.f);   /* the Warthog's tag, the variant's name in the placement, a vehicle's distance */
			host_os_activate(18, 2); host_os_run(); assert(host_os_placed(rec, &made) == 0xA0010001u && host_os_variant() == 0x04000012u);
			host_os_activate(18, 0); host_os_run(); assert(host_os_placed(rec, &made) == 0xA0010001u && host_os_variant() == 0 && !strcmp(host_toast(), "Spawned: Warthog"));   /* the entry that was there: no variant named, as ever */
			W32(VP + 0x38, 0x06000011u); host_os_rebuild();                                        /* an id that says 6 letters for "gauss": not trusted - numbered */
			assert(host_menu_n(18) == 3 && !strcmp(host_menu_label(18, 1), "Warthog (variant 2)") && !strcmp(host_menu_label(18, 2), "Warthog (ptur)"));
			W32(VP + 0x38, 0x05000011u); W32(HDR + 0x160, 0); host_os_rebuild();                   /* no string table found: both numbered */
			assert(host_menu_n(18) == 3 && !strcmp(host_menu_label(18, 1), "Warthog (variant 2)") && !strcmp(host_menu_label(18, 2), "Warthog (variant 3)"));
			host_os_activate(18, 1); host_os_run(); host_os_placed(rec, &made); assert(host_os_variant() == 0x05000011u);
			W32(VD + 0x30, 0); host_os_rebuild(); assert(host_menu_n(18) == 3);                     /* the tag names no default: the model's first is it */
			W32(VD + 0x30, 0x05000011u); host_os_rebuild(); assert(host_menu_n(18) == 3 && host_os_variant() == 0x05000011u);   /* another default: the other two listed */
			host_os_activate(18, 1); host_os_run(); host_os_placed(rec, &made); assert(host_os_variant() == 0x07000010u);
			W32(VD + 0x30, 0x07000010u); W32(MD + 0x50, 1); host_os_rebuild(); assert(host_menu_n(18) == 1 && !strcmp(host_menu_label(18, 0), "Warthog"));   /* one variant: nothing added */
			assert(logged("object spawning: the Warthog's model has 3 variant(s); listed beside it: 2 (names read)") && logged("listed beside it: 2 (the string table was not found: numbered)") && logged("has 1 variant(s); listed beside it: 0"));
			W32(INST + 16 * 1 + 8, vdata); W32(INST + 16 * 2 + 8, mdata); W32(HDR + 0x160, 0); W32(HDR + 0x164, 0); W32(HDR + 0x168, 0); W32(HDR + 0x16C, 0);
			g_osfile_size = size0; W32(HDR + 8, fsize0); host_os_rebuild(); host_clear_toast();
		}
		memcpy(g_loglines, os_log_keep, sizeof(os_log_keep)); g_nlog = os_nlog_keep;
		host_set_menu_open(0, 0); host_clear_toast();
		W32(0x1F28, 0); W32(0x1D20, old_ai); W32(0x1B08, old_inst); g_osfile_size = 0;
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f); WF(OBJECT + 0x70, 0.f); WF(OBJECT + 0x74, 0.f); WF(OBJECT + 0x78, 0.f); WF(OBJECT + 0x168, 0.f); WF(OBJECT + 0x16C, 0.f); WF(OBJECT + 0x170, 0.f);
#undef OS_HUD
		printf("Object Spawning: the level's own list (by tag group and name from the map file, checked against the scenario), H2EK's order, only what is there; made at the next tick in front of you; a new level re-read; an unreadable or empty level said; a biped given its character's weapon and an actor (the plain rank first; not a swarm, not with AI off, not without a character)\n");
	}
	/* 6i.156: the Campaign Timer - Halo 2 Classic timing: real time (the console's millisecond count) from the first
	 * level's first tick, never stopped - not by a pause, a stretch with no ticks (a load, a cutscene) or the load between
	 * levels; split only at game_won (the level's time = since the last split); the next level's first tick changes the
	 * name only; ends at the main menu (complete: the last split's time; part-way: the time of getting there) */
	{
		void host_set_ctimer(s32 on); void host_ct_tick(void); void host_ct_clock(u32 ms); s32 host_ct(u32 *ms, s32 *n, u32 *done); u32 host_ct_split(s32 k); void ct_won(void);
		u32 ms, done, k, clk = 0xFFFFF000u; s32 n, st; u8 t0 = MEM[TIME], t1 = MEM[TIME + 1];    /* (the console's count wraps during the run: no matter) */
		static char log_keep[256][200]; int nlog_keep = g_nlog;                                  /* the log's ring put back after (later tests count its lines) */
		const u32 CTO = 0x47000;
#define CT_HUD() do { g_nlines = 0; spooky_view(0x120C); spooky_frame(); } while (0)   /* the HUD's frame alone: the main loop's frame count (the 10 s log lines' windows, tested later) left as it was */
#define CT_RUN(a, b) for (k = (a); k < (b); k++) { clk += 100; host_ct_clock(clk); W32(TIME + 8, k); host_ct_tick(); }   /* ticks a..b-1, 100 ms of real time each */
#define CT_WAIT(d) do { clk += (d); host_ct_clock(clk); } while (0)                     /* real time passing with no tick */
		memcpy(log_keep, g_loglines, sizeof(log_keep));
		W32(0x1D10, CTO); W32(CTO + 8, 1); W32(CTO + 0xC, 0); MEM[TIME] = 1; MEM[TIME + 1] = 0;
		host_set_ctimer(1); host_ct_clock(clk);
		W32(TIME + 8, 500); host_ct_tick(); st = host_ct(&ms, &n, &done); CT_HUD();                /* turned on part-way through a level: waits */
		assert(st == 0 && ms == 0 && drawn("Total Time: 00:00:00.00") && !drawn("Outskirts"));   /* 6i.169: the total alone, at 0 */
		CT_RUN(0, 30);                                                                             /* the level's start: its first tick is 0, 29 more = 2.9 s */
		st = host_ct(&ms, &n, &done); CT_HUD();
		assert(st == 1 && ms == 2900 && n == 0 && drawn("Outskirts  0:02.90") && drawn("Total Time: 00:00:02.90") && drawn("Total Time") < drawn("Outskirts"));   /* 6i.169: the total on the bottom line (drawn first), the level above it */
		CT_WAIT(250); CT_HUD(); assert(drawn("Outskirts  0:03.15"));                               /* no tick: the time runs all the same (read off the clock each frame) */
		MEM[TIME + 1] = 1; CT_WAIT(10000); host_ct_tick(); CT_HUD(); MEM[TIME + 1] = 0;            /* paused 10 s: counted */
		assert(drawn("Outskirts  0:13.15"));
		CT_WAIT(6850); W32(TIME + 8, 10); host_ct_tick(); CT_HUD();                                /* a revert's load, 6.85 s with no tick, the tick count back to 10: counted, no restart */
		st = host_ct(&ms, &n, &done); assert(st == 1 && ms == 20000 && n == 0 && drawn("Outskirts  0:20.00"));
		CT_WAIT(4000); CT_RUN(0, 10);                                                              /* Restart Level on the run's first level: the run again from 0 */
		st = host_ct(&ms, &n, &done); assert(st == 1 && ms == 900 && n == 0);
		ct_won(); st = host_ct(&ms, &n, &done);                                                    /* game_won: the split - and the timer goes on */
		assert(st == 2 && n == 1 && host_ct_split(0) == 900 && done == 900);
		ct_won(); CT_WAIT(8000); W32(TIME + 8, 11); host_ct_tick(); st = host_ct(&ms, &n, &done); CT_HUD();   /* 8 s of loading after it: no second split, the total running, the level's time kept in green */
		assert(st == 2 && n == 1 && done == 900 && ms == 8900 && drawn("Outskirts  0:00.90") && drawn("Total Time: 00:00:08.90"));
		WS(0x1100, "scenarios\\solo\\03b_newmombasa\\03b_newmombasa");
		CT_RUN(0, 20);                                                                             /* the next level's first tick: the name changes, the time does not - 2 s more */
		st = host_ct(&ms, &n, &done); CT_HUD();
		assert(st == 1 && ms == 10900 && n == 1 && done == 900 && drawn("Metropolis  0:10.00") && drawn("Total Time: 00:00:10.90") && drawn("Outskirts  0:00.90") && drawn("Total Time") < drawn("Metropolis") && drawn("Metropolis") < drawn("Outskirts"));   /* this level so far = since the split (its load included); 6i.169: the finished level listed above it */
		CT_WAIT(3000); CT_RUN(0, 10);                                                              /* Restart Level on a later level: nothing changes, the time runs on */
		st = host_ct(&ms, &n, &done); assert(st == 1 && ms == 14900 && n == 1 && done == 900);
		ct_won(); CT_WAIT(5000);                                                                   /* mission done: the second split, 14.0 s; then 5 s more */
		st = host_ct(&ms, &n, &done); assert(st == 2 && n == 2 && host_ct_split(1) == 14000 && done == 14900 && ms == 19900);
		WS(0x1100, "scenarios\\solo\\04a_gasgiant\\04a_gasgiant");
		CT_RUN(0, 11); CT_HUD();                                                                   /* the third level, 1.1 s in */
		assert(drawn("The Arbiter  0:06.10") && drawn("Total Time: 00:00:21.00") && drawn("Metropolis  0:14.00") && drawn("Outskirts  0:00.90") && drawn("The Arbiter") < drawn("Metropolis") && drawn("Metropolis") < drawn("Outskirts"));   /* 6i.169: every level played, the first at the top */
		W32(CTO + 8, 3); CT_HUD(); st = host_ct(&ms, &n, &done);                                   /* quit to the main menu: ended there, unfinished - the list */
		assert(st == 3 && ms == 21000 && !drawn("Outskirts") && !drawn("Metropolis") && !drawn("The Arbiter") && !drawn("Total") && !drawn("Run not finished"));   /* 6i.169: nothing on the main menu */
		CT_WAIT(60000); CT_HUD(); assert(!drawn("Total") && host_ct(&ms, &n, &done) == 3 && ms == 21000);                                 /* ended: stopped */
		W32(CTO + 8, 1); W32(TIME + 8, 700); host_ct_tick(); CT_HUD();                             /* a level resumed from a save: no start, nothing drawn */
		assert(host_ct(&ms, &n, &done) == 3 && !drawn("Total") && !drawn("0:00.00"));
		WS(0x1100, "scenarios\\solo\\08b_deltacontrol\\08b_deltacontrol");
		CT_RUN(0, 10);                                                                             /* a level started from the menu: a new run */
		st = host_ct(&ms, &n, &done); assert(st == 1 && ms == 900 && n == 0 && done == 0);
		ct_won(); CT_WAIT(30000); W32(CTO + 8, 3); CT_HUD(); st = host_ct(&ms, &n, &done);         /* mission done, the main menu 30 s later: complete, at the split's time */
		assert(st == 3 && n == 1 && ms == 900 && !drawn("The Great Journey") && !drawn("Total"));
		/* 6i.161 / 6i.163: the run's first level starts the timer when you have control - the first tick with no cinematic
		 * in progress after its opening cutscene (the letterbox not waited for) - not at its first tick; while armed the
		 * time shown runs from the first tick with no cinematic and a cinematic starting puts it back to 0; later
		 * levels' cutscenes never hold it */
		{
			const u32 CG = 0xE6000; u32 old_cg = M32h(0x1A04);
			memset(MEM + CG, 0, 0x24); W32(0x1A04, CG);                                            /* the cinematic globals: the letterbox flag +4, cinematic in progress +5 */
			W32(CTO + 8, 1); WS(0x1100, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa");
			MEM[CG + 4] = 1; CT_RUN(0, 10);                                                        /* the level loads with the letterbox on; 10 ticks before its script starts the cutscene: running, on trial */
			st = host_ct(&ms, &n, &done); CT_HUD();
			assert(st == 1 && ms == 900 && drawn("Outskirts  0:00.90") && drawn("Total Time: 00:00:00.90"));
			MEM[CG + 5] = 1; CT_RUN(10, 110); st = host_ct(&ms, &n, &done); CT_HUD();              /* the cutscene begins inside the trial, 10 s of it: armed again, at 0 */
			assert(st == 4 && ms == 0 && drawn("Outskirts  0:00.00") && drawn("Total Time: 00:00:00.00"));
			MEM[CG + 5] = 0; CT_RUN(110, 111); st = host_ct(&ms, &n, &done);                       /* over (the fade from white begins), the letterbox still on: you have control - started, at 0, at once */
			assert(st == 1 && ms == 0 && n == 0);
			CT_RUN(111, 131); CT_HUD(); assert(host_ct(&ms, &n, &done) == 1 && ms == 2000 && drawn("Outskirts  0:02.00"));   /* no standing at 0:00, no snap */
			MEM[CG + 4] = 0;
			ct_won(); assert(host_ct(&ms, &n, &done) == 2 && n == 1 && done == 2000);
			WS(0x1100, "scenarios\\solo\\03b_newmombasa\\03b_newmombasa");
			MEM[CG + 4] = 1; MEM[CG + 5] = 1; CT_WAIT(3000); CT_RUN(0, 50);                        /* the next level, with a cutscene of its own: the timer runs through the load and the cutscene */
			st = host_ct(&ms, &n, &done); assert(st == 1 && ms == 10000 && n == 1);
			MEM[CG + 4] = 0; MEM[CG + 5] = 0;
			W32(CTO + 8, 3); CT_HUD(); assert(host_ct(&ms, &n, &done) == 3);
			W32(CTO + 8, 1); WS(0x1100, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa");
			CT_RUN(0, 41); assert(host_ct(&ms, &n, &done) == 1 && ms == 4000);                     /* a new run on a level with no cutscene and no letterbox: running from its first tick (the 1 s trial passes) */
			MEM[CG + 5] = 1; CT_RUN(41, 51); assert(host_ct(&ms, &n, &done) == 1 && ms == 5000);   /* a cinematic after the trial: a mid-level cutscene - counted, as every later one */
			MEM[CG + 5] = 0;
			MEM[CG + 4] = 1; CT_WAIT(2000); CT_RUN(0, 3);                                          /* Restart Level on the first level: armed again */
			assert(host_ct(&ms, &n, &done) == 1 && ms == 200);
			MEM[CG + 5] = 1; CT_RUN(3, 50); ct_won(); st = host_ct(&ms, &n, &done);                /* a level that is all cutscene ends: armed again when it began, no start, no split */
			assert(st == 4 && n == 0 && ms == 0);
			WS(0x1100, "scenarios\\solo\\03b_newmombasa\\03b_newmombasa");
			CT_RUN(0, 20); assert(host_ct(&ms, &n, &done) == 4 && ms == 0);                        /* the next level, its cutscene still running: armed for it */
			MEM[CG + 5] = 0; CT_RUN(20, 31); CT_HUD();                                             /* control there (the letterbox still on): the run starts in that level, at that tick */
			assert(host_ct(&ms, &n, &done) == 1 && ms == 1000 && n == 0 && drawn("Metropolis  0:01.00"));
			W32(CTO + 8, 3); CT_HUD(); W32(CTO + 8, 1); WS(0x1100, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa");
			MEM[CG + 4] = 1; CT_RUN(0, 152); assert(host_ct(&ms, &n, &done) == 1 && ms == 15100);  /* no cutscene but the letterbox flag left on: running from the level's first tick */
			MEM[CG + 5] = 1; CT_RUN(152, 160); assert(host_ct(&ms, &n, &done) == 1 && ms == 15900); /* (its 5 s trial over: a cinematic now is counted) */
			MEM[CG + 5] = 0;
			W32(CTO + 8, 3); CT_HUD(); W32(CTO + 8, 1); MEM[CG + 5] = 1; CT_RUN(0, 3); assert(host_ct(&ms, &n, &done) == 4);
			W32(CTO + 8, 3); CT_HUD(); assert(host_ct(&ms, &n, &done) == 0);                       /* left for the main menu before it started: nothing kept */
			MEM[CG + 5] = 0;
			MEM[CG + 4] = 0; W32(0x1A04, old_cg);
		}
		W32(CTO + 8, 1); WS(0x1100, "scenarios\\solo\\some_other_map\\some_other_map");
		CT_RUN(0, 2); CT_WAIT(3661100);                                                            /* 3661.2 s; a map with no name of its own */
		CT_HUD(); assert(drawn("some_other_map  1:01:01.20") && drawn("Total Time: 01:01:01.20"));
		{
			void host_set_speed(s32 tenths, s32 pause); s32 host_speed_get(void); const char *host_toast(void);
			host_set_speed(5, 0); CT_RUN(5, 8); assert(host_speed_get() == 5);                     /* 6i.169: Game Speed 0.5 mid-level: left (it could not have been set with the timer on) */
			CT_RUN(0, 2); assert(host_speed_get() == 10 && strstr(host_toast(), "Game Speed: back to 1.0"));   /* a campaign mission starts: 1.0 */
			host_set_speed(20, 0); W32(CTO + 8, 2); CT_RUN(0, 3); assert(host_speed_get() == 20);  /* a multiplayer game: not touched */
			host_set_ctimer(0); W32(CTO + 8, 1); CT_RUN(0, 3); assert(host_speed_get() == 20);     /* the timer off: not touched */
			host_set_ctimer(1); host_set_speed(10, 0); W32(CTO + 8, 1);
		}
		W32(CTO + 8, 2); CT_RUN(0, 5); assert(host_ct(&ms, &n, &done) == 1 && n == 0);             /* multiplayer: no start, no split */
		assert(logged("campaign timer: armed (waiting for control) 03a_oldmombasa") && logged("campaign timer: run started (you have control) 03a_oldmombasa, run ms 0") && logged("campaign timer: level started 03b_newmombasa") && logged("campaign timer: split (mission done) 03a_oldmombasa, run ms 900") && logged("campaign timer: level restarted (the run goes on) 03b_newmombasa")
		       && logged("campaign timer: split (mission done) 03b_newmombasa, run ms 14900") && logged("campaign timer: run ended (level left unfinished) 04a_gasgiant, run ms 21000") && logged("campaign timer: run ended (complete) 08b_deltacontrol, run ms 900"));
		host_set_ctimer(0); CT_HUD(); st = host_ct(&ms, &n, &done);                                /* off: forgotten, nothing drawn */
		assert(st == 0 && n == 0 && !drawn("some_other_map"));
		memcpy(g_loglines, log_keep, sizeof(log_keep)); g_nlog = nlog_keep;
		W32(0x1D10, 0); W32(CTO + 8, 0); MEM[TIME] = t0; MEM[TIME + 1] = t1;
		WS(0x1100, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa");
#undef CT_HUD
#undef CT_RUN
#undef CT_WAIT
		printf("Campaign Timer: real time from the first level's first tick, never stopped (pause, no ticks, the load between levels), split only at game_won, the next level changes the name only, Restart Level (first level: from 0; later: runs on), ends at the main menu\n");
	}
	/* step 6a: the out-of-bounds timer, HCM's bars - the death bar while the player's counter (+0x3E) counts against
	 * round(30 x 0.5) = 15; the detection bar (ticks to the next (game time + index) % 16 == 0 check) while it doesn't */
	{
		s32 host_oob(s32 *left, s32 *next); void host_set_oob(s32 on);
		s32 left, next, st;
		host_set_oob(1);
		W32(0x1B00, 0);
		state(400, 0.f, 0, 0);
		W32(PL_DATA + 0x3C, 5u << 16); g_q = 0; frame();                               /* counted 5 ticks */
		assert(g_q == 6);                                                                /* the death bar: backdrop, fill, 4 edges */
		{
			/* 312 wide, centred: the backdrop 161 to 479 (3 past each end); the fill 11 of 16 ticks: 164 to 378 (312 x 11/16) */
			int q, back = 0, fill = 0;
			for (q = 0; q < g_q && q < 64; q++)
			{
				if (g_qxy[q][0] == 161 && g_qxy[q][2] == 479) back = 1;
				if (g_qxy[q][0] == 164 && g_qxy[q][2] == 378) fill = 1;
			}
			assert(back && fill);
		}
		st = host_oob(&left, &next);
		assert(st == 1 && left == 11 && drawn("OUT OF BOUNDS - 11 ticks"));
		W32(PL_DATA + 0x3C, 15u << 16); frame(); st = host_oob(&left, &next);
		assert(st == 1 && left == 1 && drawn("OUT OF BOUNDS - 1 tick"));
		W32(PL_DATA + 0x3C, 16u << 16); frame(); st = host_oob(&left, &next);
		assert(st == 1 && left == 0 && drawn("OUT OF BOUNDS - 0 ticks"));
		W32(PL_DATA + 0x3C, 20u << 16); frame();                                        /* past the limit, alive: blocked */
		assert(drawn("OUT OF BOUNDS (kill blocked) - 0 ticks"));
		W32(PL_DATA + 0x3C, 0); state(400, 0.f, 0, 0); g_q = 0; frame(); st = host_oob(&left, &next);   /* not counting: tick 400, player 0 */
		assert(g_q == 6);                                                                /* the detection bar, full (white) */
		assert(st == 2 && next == 0 && drawn("OOB CHECK NOW") && !drawn("OUT OF BOUNDS"));
		state(401, 0.f, 0, 0); g_q = 0; frame(); st = host_oob(&left, &next);
		assert(st == 2 && next == 15 && drawn("OOB check in 15 ticks") && g_q == 6);      /* the detection bar */
		W32(0x1D10, 0x47000); W32(0x47000 + 0xC, 4); W32(PL_DATA + 0x3C, 5u << 16); frame(); st = host_oob(&left, &next);
		assert(st == 0 && !drawn("OUT OF BOUNDS") && !drawn("OOB"));                     /* mode 4: the game doesn't check */
		W32(0x1D10, 0); W32(0x47000 + 0xC, 0);
		host_set_oob(0); frame(); assert(!drawn("OUT OF BOUNDS") && !drawn("OOB"));      /* switched off */
		{
			/* the timer on its own keeps the overlay drawing: the Superbounce Viewer off, the menu closed, in a level - the
			 * bars still drawn (6a.2 returned early: with the hint line and the viewer off they stopped); and the hint line
			 * never in a level, only on the main menu (no player) */
			void host_set_sb_viewer(s32 on);
			host_set_oob(1); host_set_sb_viewer(0);
			W32(PL_DATA + 0x3C, 5u << 16); g_q = 0; frame();
			assert(drawn("OUT OF BOUNDS - 11 ticks") && g_q == 6 && !drawn("Back + D-pad Up"));
			W32(PL_DATA + 0x2C, (u32)-1); frame();                                          /* dead in a level: no unit, a player */
			assert(!drawn("Back + D-pad Up") && !drawn("OUT OF BOUNDS"));                   /* 6.f.3: no hint (6.f.2 showed it) */
			{
				/* 6g: the game's main menu - its options' state 3 (6.f.3 took "no player record": the main menu has one) */
				u32 opp = M32h(0x1D10), ows = 0;
				if (!opp) { opp = 0x4F000; W32(0x1D10, opp); }
				ows = M32h(opp + 8);
				W32(opp + 8, 3); frame();
				assert(drawn("Back + D-pad Up") && !drawn("OUT OF BOUNDS"));
				W32(opp + 8, 1); frame(); assert(!drawn("Back + D-pad Up"));          /* the campaign, dead: none */
				W32(opp + 8, ows);
			}
			W32(PL_DATA + 0x2C, UNIT); host_set_sb_viewer(1); host_set_oob(0);
			printf("the timer alone keeps the overlay drawing (viewer off); the hint line on the main menu only\n");
		}
		W32(PL_DATA + 0x3C, 0);                                                          /* (left off for the rest) */
		printf("out of bounds: death bar 5 -> 11 ticks, 15 -> 1, 16 -> 0, 20 -> kill blocked; detection bar tick 400 -> CHECK NOW, 401 -> 15; mode 4 / off -> nothing\n");
	}
	/* step 6b.2: the menu's dark panel - its right edge mirrors its left margin: 48 to 592 on the 640-wide screen (6b.1:
	 * 48 to 660 - off the right) */
	{
		void host_set_menu_open(s32 open, s32 m);
		int q, found = 0;
		{
			/* step 6d.1: the Monster/Dreadnought Viewer in Superbounce Viewer Settings (moved from Ghost Viewer Settings) */
			int q, a = 0, b = 0, g = 0;
			host_set_menu_open(1, 2); frame();
			for (q = 0; q < g_nlines; q++) { if (strstr(g_lines[q], "Monster/Dreadnought Viewer: off")) a = 1; if (strstr(g_lines[q], "Monster/Dreadnought Viewer: 35+ only: off")) b = 1; }
			host_set_menu_open(1, 5); frame();
			for (q = 0; q < g_nlines; q++) if (strstr(g_lines[q], "Predicted") || strstr(g_lines[q], "Monster/Dreadnought")) g = 1;
			assert(a && b && !g);
			printf("Superbounce Viewer Settings: Monster/Dreadnought Viewer, 35+ only (no longer in Ghost Viewer Settings)\n");
		}
		host_set_menu_open(1, 0); g_q = 0; frame();
		for (q = 0; q < g_q && q < 64; q++)
		{
			int lo = g_qxy[q][0], hi = g_qxy[q][0], k;
			for (k = 0; k < 8; k += 2) { if (g_qxy[q][k] < lo) lo = g_qxy[q][k]; if (g_qxy[q][k] > hi) hi = g_qxy[q][k]; }
			if (lo == 48 && hi == 592) found = 1;
			assert(hi <= 640);                                                  /* nothing past the screen's right edge */
		}
		assert(found && drawn("Spooky Ghost Menu - by Fortune090"));
		host_set_menu_open(0, 0); frame();
		printf("menu panel: 48 to 592 - the same 48 margin both sides, nothing past the right edge\n");
	}
	/* step 6b.1: the sleep timer, exact - a step of 0.125, period A 1.0, B 3.0, both timers 0: quiet checks at ticks 9, 17,
	 * 25, ...; long checks at 26, then 50 (49 is a quiet check's tick). Your island's one body has a spatial deactivator
	 * (limits 0.5 moved / 0.5 turned short, 0.05 / 0.05 long): its references at the body's pose - still - or off. */
	{
		void host_set_sleep(s32 on); void host_set_veto(s32 v); s32 host_sleep_awake(void);
		s32 host_sleep(s32 *ts, s32 *ta, s32 *tb, s32 *r);
		enum { ISL2 = 0x48000, ENTS2 = 0x48100, ENT2 = 0x48200, MOT2 = 0x48300, LIST = 0x48400, MOT1 = 0x48500, DEA1 = 0x48600, DEA2 = 0x48700 };
		const u32 VT_SPATIAL = 0x1D20, VT_FAKE = 0x1D24, VT_HALO = 0x1D28;
		s32 ts, ta, tb, r, st, k;
		float dist, p0;
		host_set_sleep(1); host_set_veto(0);
		W32(0x1B00, 0);
		state(500, 0.f, 0, 0);
		W32(WORLD + 0x23C, 0x0100); WF(WORLD + 0x150, 0.125f); WF(WORLD + 0x30, 1.f); WF(WORLD + 0x34, 3.f);
		W32(WORLD + 0x08, LIST); W32(WORLD + 0x0C, 2); W32(LIST, ISLAND); W32(LIST + 4, ISL2);
		W32(ISLAND + 0x20, WORLD); W32(ISLAND + 0x2C, 0); WF(ISLAND + 0x34, 0.f); WF(ISLAND + 0x38, 0.f);
		W32(ISL2 + 0x20, WORLD); W32(ISL2 + 0x2C, 4); WF(ISL2 + 0x34, 0.f); WF(ISL2 + 0x38, 0.f);
		W32(ISL2 + 0x3C, ENTS2); W32(ISL2 + 0x40, 1); W32(ENTS2, ENT2); W32(ENT2 + 0x3C, MOT2); W32(ENT2 + 0x54, DEA2);
		p0 = RF(OBJECT + 0x64);
		WF(MOT2 + 0x70, p0 + 3.f); WF(MOT2 + 0x74, RF(OBJECT + 0x68)); WF(MOT2 + 0x78, RF(OBJECT + 0x6C));
		/* the bodies' poses (position +0xB0, rotation +0x30) and their deactivators' references at them: still */
		W32(ENTITY + 0x3C, MOT1); W32(ENTITY + 0x54, DEA1);
		{
			u32 mo[2] = { MOT1, MOT2 }, de[2] = { DEA1, DEA2 };
			for (k = 0; k < 2; k++)
			{
				WF(mo[k] + 0xB0, 1.f); WF(mo[k] + 0xB4, 2.f); WF(mo[k] + 0xB8, 3.f); WF(mo[k] + 0xBC, 0.f);
				WF(mo[k] + 0x30, 0.f); WF(mo[k] + 0x34, 0.f); WF(mo[k] + 0x38, 0.f); WF(mo[k] + 0x3C, 1.f);
				W32(de[k], VT_SPATIAL); WF(de[k] + 0x08, 1.f);
				WF(de[k] + 0x10, 1.f); WF(de[k] + 0x14, 2.f); WF(de[k] + 0x18, 3.f); WF(de[k] + 0x20, 0.f); WF(de[k] + 0x24, 0.f); WF(de[k] + 0x28, 0.f); WF(de[k] + 0x2C, 1.f);
				WF(de[k] + 0x30, 1.f); WF(de[k] + 0x34, 2.f); WF(de[k] + 0x38, 3.f); WF(de[k] + 0x40, 0.f); WF(de[k] + 0x44, 0.f); WF(de[k] + 0x48, 0.f); WF(de[k] + 0x4C, 1.f);
				WF(de[k] + 0x50, 0.5f); WF(de[k] + 0x54, 0.5f); WF(de[k] + 0x58, 0.05f); WF(de[k] + 0x5C, 0.05f);
			}
		}
		g_q = 0; frame(); st = host_sleep(&ts, &ta, &tb, &r);                             /* 1. all still: the long check at 26 */
		assert(st == 1 && r == 0 && ta == 9 && tb == 26 && ts == 26 && host_sleep_awake() == 0 && drawn("SLEEP in 26 ticks if still - quiet rounds 0/5"));
		assert(!drawn("island"));                                           /* 6i.48: the other islands' lines gone (one 4/5 there, 3 away) */
		{
			/* draining: 26 of a 54-tick cycle (6 checks of 9 ticks) - the fill 164 to 314 (312 x 26/54 = 150); the jump line
			 * at 3 ticks: 164 + 312 x 3/54 = 181, two pixels wide, past the bar's top and bottom */
			int q, fill = 0, line = 0;
			for (q = 0; q < g_q && q < 64; q++)
			{
				if (g_qxy[q][0] == 164 && g_qxy[q][2] == 314) fill = 1;
				if (g_qxy[q][0] == 180 && g_qxy[q][2] == 182) line = 1;
			}
			assert(fill && line);
		}
		WF(DEA1 + 0x30, 1.2f); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 49);  /* 2. long off: it fails at 26 - the quiet checks: 49 */
		WF(DEA1 + 0x30, 1.f); WF(DEA1 + 0x10, 1.6f); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 26);   /* 3. short off, long still: 26 */
		WF(DEA1 + 0x30, 1.2f); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 50);  /* 4. both off: the second long check, 50 */
		WF(DEA1 + 0x10, 1.f); WF(DEA1 + 0x30, 1.f);
		WF(DEA1 + 0x20, 0.8f); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 26);  /* 5. turned too far (0.64 > 0.25): short off -> 26 */
		WF(DEA1 + 0x20, 0.f); WF(DEA1 + 0x30, 1.2f);                                    /* (long off from here: the quiet checks decide) */
		WF(DEA1 + 0x10, 1.5f); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 50);  /* 6. moved exactly the limit (0.25 = 0.5^2): not still - both off: 50 */
		WF(DEA1 + 0x10, 1.25f); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 49); /*    half of it (0.0625): still */
		WF(DEA1 + 0x10, 1.f); WF(DEA1 + 0x30, 1.f);
		W32(DEA1, VT_HALO); host_set_veto(1); g_q = 0; frame(); host_sleep(&ts, &ta, &tb, &r);   /* 7. Halo's, vetoed: kept awake */
		assert(ts == -1 && host_sleep_awake() == 1 && drawn("ASLEEP") && !drawn("kept awake"));   /* 6i.48: just ASLEEP */
		{ int q, full = 0; for (q = 0; q < g_q && q < 64; q++) if (g_qxy[q][0] == 164 && g_qxy[q][2] == 476) full = 1; assert(full); }   /* not counting down: full */
		host_set_veto(0); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 26 && host_sleep_awake() == 0);   /*    not vetoed: Havok's tests */
		W32(DEA1, VT_FAKE); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == -1 && host_sleep_awake() == 3);  /* 8. the fake one */
		W32(ENTITY + 0x54, 0); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == -1 && host_sleep_awake() == 2);  /*    none */
		W32(ENTITY + 0x54, DEA1); W32(DEA1, VT_SPATIAL);
		W32(WORLD + 0x23C, 0); frame(); st = host_sleep(&ts, &ta, &tb, &r); assert(st == 2 && drawn("SLEEP: off"));
		host_set_sleep(0); frame(); assert(!drawn("SLEEP"));
		W32(WORLD + 0x08, 0); W32(WORLD + 0x0C, 0); W32(ISLAND + 0x2C, 0); W32(ENTITY + 0x3C, 0); W32(ENTITY + 0x54, 0);
		printf("sleep: all still -> 26 (the long check); long off -> 49; short off -> 26; both -> 50 (the second long check); turned -> 26; at the limit not still; vetoed / fake / none -> ASLEEP (no other islands' lines)\n");
	}
	/* step 3d.2: airborne, falling forward at 6 u/s from 1 above the synthetic floor: the arc is drawn ahead too */
	{
		int ground_q;
		W32(0x1B04, 0); W32(0x1B00, 0x80000);
		host_set_marker(0); host_set_seams(1);
		state(300, 0.f, 0, 0); g_q = 0; frame(); ground_q = g_q;
		WF(OBJECT + 0x88, 6.f); WF(OBJECT + 0x8C, 0.f);
		state(301, 0.f, 1, 0); WF(OBJECT + 0x88, 6.f); g_q = 0; frame();
		assert(g_q > ground_q + 5);
		printf("airborne: the arc drawn ahead (%d lines with the seams, %d on the ground)\n", g_q, ground_q);
	}
	/* step 3d.3: the landing. The level's floor = the square quad (x 2-4, y -1..1, z -1). Falling from z 0:
	 * forward at 4 u/s the feet reach z -1 at about x 2.77 - inside the quad: a landing there.
	 * At 1 u/s the arc passes beside the quad (x ~0.69): no landing. */
	{
		float p[3];
		W32(0x1B04, 0); W32(0x1B00, 0xB6000);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		/* air control's target is taken once two ticks agree (Cartographer): two ticks at the same drift */
		state(399, 0.f, 1, 0); WF(OBJECT + 0x88, 4.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame();
		state(400, 0.f, 1, 0); WF(OBJECT + 0x88, 4.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame();
		state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 4.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
		g_q = 0; frame();
		assert(host_landing(p) && p[0] > 2.6f && p[0] < 2.95f && p[1] > -0.05f && p[1] < 0.05f && p[2] > -1.01f && p[2] < -0.99f);
		/* 6i.212: (was a check of Cartographer's landing verdict - retired) */              /* landing text off by default: verdict only on the cursor */
		{
			/* the seam's colour is the reticle's verdict for landing ON it: a flat seam along y, falling at 6 u/s, contact 6.2
			 * u/s (over the 5.45 needed), depth 0.4 - so the fall alone says likely; the drift across it decides the rest */
			s32 host_rate_as_verdict(const float *pa, const float *pb, s32 kind, float down, float walk, float dw, const float *pv, s32 charged, const char **why);
			float sa[3] = { 0, -2, 0 }, sb[3] = { 0, 2, 0 };
			float across_fast[3] = { 2.f, 0.f, 0.f }, along[3] = { 0.f, 3.f, 0.f }, few[3] = { 0.42f, 3.f, 0.f }, edge8[3] = { 0.95f, 3.f, 0.f };
			const char *why = 0;
			s32 q;
			/* 6h.34: 6h.29's rule again - the colour is the verdict for landing ON it with your drift: too fast across (~18 deg)
			 * or too little (along a flat seam) yellow, a valley along it kept (its bend may push), ~4 deg green, ~9 deg or
			 * shallow amber, uncharged red (6h.30-6h.33 kept green at every angle: false greens) */
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, across_fast, 1, &why); assert(q == 1 && why && !strcmp(why, "ANGLE: drifting too fast across"));
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, along, 1, &why); assert(q == 1 && why && !strcmp(why, "ANGLE: too little drift across"));
			q = host_rate_as_verdict(sa, sb, 1, 6.f, 6.2f, 0.4f, along, 1, &why); assert(q == 2 && !why);
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, few, 1, &why); assert(q == 2 && !why);
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, edge8, 1, &why); assert(q == 3 && why && !strcmp(why, "EDGE (about 1 in 2)"));
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.15f, few, 1, &why); assert(q == 3);
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, few, 0, &why); assert(q == 0 && why && !strcmp(why, "NOT CHARGED"));
			{
				/* the margin: 13.2 deg across is amber (EDGE, under 13.5) if the seam was green last search, yellow (over 12.5) if
				 * it wasn't; 12.8 likewise - so drifting around 13 deg doesn't flip the line every search */
				void host_set_prev_q(const float *pa, const float *pb, s32 q);
				float a132[3], a128[3], down = 6.f;
				float t132 = 13.2f / 57.29578f, t128 = 12.8f / 57.29578f;
				a132[0] = down * ftanf_host(t132); a132[1] = 3.f; a132[2] = 0.f;
				a128[0] = down * ftanf_host(t128); a128[1] = 3.f; a128[2] = 0.f;
				host_set_prev_q(sa, sb, 2);
				q = host_rate_as_verdict(sa, sb, 0, down, 6.2f, 0.4f, a132, 1, &why); assert(q == 3);      /* was green: 13.2 < 13.5 */
				host_set_prev_q(sa, sb, 1);
				q = host_rate_as_verdict(sa, sb, 0, down, 6.2f, 0.4f, a132, 1, &why); assert(q == 1);      /* wasn't: 13.2 >= 12.5 */
				q = host_rate_as_verdict(sa, sb, 0, down, 6.2f, 0.4f, a128, 1, &why); assert(q == 1);      /* wasn't: 12.8 >= 12.5 still */
				host_set_prev_q(sa, sb, 2);
				q = host_rate_as_verdict(sa, sb, 0, down, 6.2f, 0.4f, a128, 1, &why); assert(q == 3);
				host_set_prev_q(sa, sb, -9);
			}
			printf("seams in the air: 6h.29's verdict-by-drift again (too fast / too little yellow, ~4 deg green, ~9 deg amber), with a half-degree margin so a line doesn't flicker on a limit\n");
		}
		{
			/* the landing drawn with its seams, as last finished (5e.17 drew the live landing against the old seams): on the slow
			 * schedule the verdict and seams change only when the cycle finishes; the circle itself follows the path every frame
			 * on the drawn landing's polygon (6h.37) */
			void host_set_step_div(s32 d); u32 host_count_hud(void); s32 host_drawn_landing(float *dp);
			float old_p[3], d_p[3], live[3];
			s32 ff;
			assert(host_drawn_landing(old_p) && old_p[0] > 2.6f && old_p[0] < 2.95f);
			host_set_step_div(6);
			for (ff = 0; ff < 12 && (host_count_hud() % 6) != 5; ff++) { state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 4.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame(); }
			state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 3.5f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			frame();                                                               /* step 0: the landing */
			assert(host_landing(live) && live[0] < old_p[0] - 0.05f);              /* the live landing moved */
			/* 6h.37: the drawn one follows the landing step's fresh path on its own polygon (the same floor here) - no longer
			 * waiting for the cycle to finish (its seams and verdict still do) */
			assert(host_drawn_landing(d_p) && fabsf(d_p[0] - live[0]) < 1e-3f && d_p[0] != old_p[0]);
			for (ff = 0; ff < 3; ff++) { state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 3.5f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame(); }
			assert(host_drawn_landing(d_p) && host_landing(live) && d_p[0] == live[0]);   /* after the cycle: the new */
			{
				/* 6h.20: every 3rd frame - the landing, the seams, then rating and view together: the drawn landing new after 3 */
				float before[3];
				host_set_step_div(3);
				for (ff = 0; ff < 6 && (host_count_hud() % 3) != 2; ff++) { state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 3.5f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame(); }
				assert(host_drawn_landing(before));
				state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 3.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
				frame();                                                           /* step 0: the landing */
				assert(host_landing(live) && live[0] < before[0] - 0.05f && host_drawn_landing(d_p) && fabsf(d_p[0] - live[0]) < 1e-3f);   /* 6h.37: followed */
				for (ff = 0; ff < 2; ff++) { state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 3.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame(); }
				assert(host_drawn_landing(d_p) && host_landing(live) && d_p[0] == live[0]);   /* rating + view ran: the new */
				printf("every 3rd frame: the landing, the seams, then rating and view together - the drawn landing follows (and between, the circle follows the path)\n");
			}
			host_set_step_div(0);
			state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 4.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame();
		}
		{
			/* "Seams near you" off: the landing still predicted, the trajectory and the cursor still drawn - only the seams
			 * themselves go (5e.2 hid all of it) */
			s32 with = g_q, without;
			float q2[3];
			host_set_seams(0);
			state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 4.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			g_q = 0; frame(); without = g_q;
			assert(host_landing(q2) && q2[0] > 2.6f && q2[0] < 2.95f && without > 0 && without <= with);
			host_set_seams(1);
		}
		host_set_land_text(1); g_q = 0; frame();
		/* 6i.212: (was a check of Cartographer's landing verdict - retired) */                                        /* Cartographer's verdict text (6h.34: the default circle again) */
		printf("landing: found on the floor quad at (%.2f, %.2f, %.2f), HUD says so\n", p[0], p[1], p[2]);
		state(402, 0.f, 1, 0); WF(OBJECT + 0x88, 1.f); frame();
		state(403, 0.f, 1, 0); WF(OBJECT + 0x88, 1.f); frame();
		state(404, 0.f, 1, 0); WF(OBJECT + 0x88, 1.f); g_q = 0; frame();
		assert(!host_landing(p));
		printf("landing: none when the arc passes beside the floor\n");
	}
	{ void host_set_exact_circle(s32 on); host_set_exact_circle(1); }            /* 6h.34: the exact circle's tests (an option now) */
	/* the verdict, checked against Cartographer's rules run independently: a square floor 5 down split into two triangles
	 * along its diagonal (a flat seam). Charged, drifting 0.3 u/s across the diagonal from a start that lands ON it:
	 * on-time depth 0.021 (too shallow), late 0.206 at 6.40 u/s, tilt 2.6 deg, late start -0.01 (no debounce) -> 1, LOTTERY */
	{
		const u32 T = 0xB8000, TP = T + 0x100, TS = T + 0x200, TE = T + 0x300, TV = T + 0x400;
		float tv[4][3] = { { 0, -2, -5 }, { 4, -2, -5 }, { 4, 2, -5 }, { 0, 2, -5 } }, p[3];
		u16 te[5][6] = { { 0, 1, 1, 0xffff, 0, 0xffff }, { 1, 2, 2, 0xffff, 0, 0xffff }, { 2, 0, 0, 3, 0, 1 },
			{ 2, 3, 4, 0xffff, 1, 0xffff }, { 3, 0, 2, 0xffff, 1, 0xffff } };
		int k2, j2, t;
		WF(TP, 0); WF(TP + 4, 0); WF(TP + 8, 1); WF(TP + 12, -5);
		W32(TS, 0u | (0u << 16)); W32(TS + 4, 0); W32(TS + 8, 0u | (2u << 16)); W32(TS + 12, 0);    /* S0 first edge 0, S1 first edge 2 */
		for (k2 = 0; k2 < 5; k2++) for (j2 = 0; j2 < 6; j2 += 2) W32(TE + k2 * 12 + j2 * 2, (u32)te[k2][j2] | ((u32)te[k2][j2 + 1] << 16));
		for (k2 = 0; k2 < 4; k2++) { for (j2 = 0; j2 < 3; j2++) WF(TV + k2 * 16 + j2 * 4, tv[k2][j2]); W32(TV + k2 * 16 + 12, 0); }
		W32(T + 0x08, 1); W32(T + 0x0C, TP); W32(T + 0x28, 2); W32(T + 0x2C, TS); W32(T + 0x30, 5); W32(T + 0x34, TE); W32(T + 0x38, 4); W32(T + 0x3C, TV);
		W32(0x1B00, T); W32(0x1B04, 0);
		for (t = 0; t < 3; t++)
		{
			state(500 + t, 0.f, 1, 1);
			WF(OBJECT + 0x64, 1.675065f); WF(OBJECT + 0x68, 0.324935f); WF(OBJECT + 0x6C, 0.f);
			WF(OBJECT + 0x88, 0.212132f); WF(OBJECT + 0x8C, -0.212132f); WF(OBJECT + 0x90, 0.f);
			g_q = 0; frame();
		}
		assert(host_landing(p) && p[0] > 1.99f && p[0] < 2.01f && p[1] > -0.01f && p[1] < 0.01f);
		{
			/* 6h.30: the exact physics on the same landing. On the landing tick the pill barely sinks in: the two triangles stop
			 * the fall (exit under 1 u/s) - no launch. A tick late (the contact Havok sometimes makes late) the pill straddles the
			 * diagonal deep enough that each triangle's shortest way out is sideways, opposite ways: the coupled pair launches
			 * ~12 u/s. So: LOTTERY, as before, now from the contacts and the solver rather than a depth rule. */
			s32 host_ex_q(void); double host_ex_speed(void); const char *host_ex_why(void);
			assert(host_landing_q() == 1 && host_ex_q() == 1 && !strncmp(host_ex_why(), "LOTTERY: a tick late", 20) && host_ex_speed() < 1.0 && strstr(host_ex_why(), "13.3"));   /* 6h.34: the capped solver: 13.3 (12.4 before) */
			printf("landing verdict (exact): on the diagonal flat seam at (%.2f, %.2f, %.2f) - %s\n", p[0], p[1], p[2], host_ex_why());
		}

		{
			s32 host_seam_src_at(float x, float y, s32 *q), host_exr_more(s32 *pending, s32 *held), host_exr_counts(s32 *rated, s32 *cached);
			s32 q0, q1, src, f0, f1, pend, held, rated, cached, t2;
			float vz = 0.f, z = 0.f;
			src = host_seam_src_at(2.f, 0.f, &q0);
			f0 = host_exr_more(&pend, &held);
			assert(q0 >= 0);
			{
				/* far from the landing (~45 ticks out) a change waits for a second search: uncharged now - the verdict is red,
				 * the circle still shows the LOTTERY until the next search agrees */
				s32 host_ex_q(void);
				vz -= 4.1712f / 30.f; z += vz / 30.f;
				state(503, vz, 1, 0); WF(OBJECT + 0x64, 1.675065f); WF(OBJECT + 0x68, 0.324935f); WF(OBJECT + 0x6C, z);
				WF(OBJECT + 0x88, 0.212132f); WF(OBJECT + 0x8C, -0.212132f); WF(OBJECT + 0x90, vz);
				frame();
				assert(host_ex_q() == 0 && host_landing_q() == 1);
			}
			for (t2 = 0; t2 < 4; t2++)
			{
				vz -= 4.1712f / 30.f; z += vz / 30.f;
				state(504 + t2, vz, 1, 1);
				WF(OBJECT + 0x64, 1.675065f); WF(OBJECT + 0x68, 0.324935f); WF(OBJECT + 0x6C, z);
				WF(OBJECT + 0x88, t2 & 1 ? 1.5f : -0.8f); WF(OBJECT + 0x8C, t2 & 1 ? 0.9f : -1.2f); WF(OBJECT + 0x90, vz);
				g_q = 0; frame();
				src = host_seam_src_at(2.f, 0.f, &q1); f1 = host_exr_more(&pend, &held); host_exr_counts(&rated, &cached);
				if (t2 == 0) { f0 = f1; q0 = q1; }                          /* the fall these ticks follow (gravity's step each tick) */
				assert(f1 == f0);
			}
			vz -= 4.1712f / 30.f; z += vz / 30.f;                             /* the same fall, uncharged: red */
			state(508, vz, 1, 0); WF(OBJECT + 0x64, 1.675065f); WF(OBJECT + 0x68, 0.324935f); WF(OBJECT + 0x6C, z); WF(OBJECT + 0x88, -0.8f); WF(OBJECT + 0x8C, -1.2f); WF(OBJECT + 0x90, vz);
			frame(); src = host_seam_src_at(2.f, 0.f, &q1); assert(q1 == 0);
			/* a jump in vertical speed: a new fall, re-rated */
			state(509, 6.f, 1, 1); WF(OBJECT + 0x64, 1.675065f); WF(OBJECT + 0x68, 0.324935f); WF(OBJECT + 0x6C, z); WF(OBJECT + 0x88, 0.212132f); WF(OBJECT + 0x8C, -0.212132f); WF(OBJECT + 0x90, 6.f);
			frame(); f1 = host_exr_more(&pend, &held);
			assert(f1 == f0 + 1);
			printf("seams in the air: a fall followed tick by tick (lottery seam %d), a new fall when the vertical speed jumps; uncharged red\n", q0);
		}
		{
			/* 6h.34 (HCM 2026-11-01): the seam colours on the validated physics. Above the diagonal seam 10 up, barely moving,
			 * your view across it: Cartographer's rule calls it yellow (too little drift across), so the physics check runs -
			 * forward speeds 0 - 2.25 along your view, the fall stepped tick by tick to the game's contact, the capped solver.
			 * Only 0.25 u/s forward lands you where it launches (10.1 u/s): purple, SPEED UP. Checked against the separately
			 * verified core (sb_exact.c, to 3e-13 of HCM's C++), stepped and capped here independently. */
			s32 host_ph_check(const float *f, const float *v, s32 air, const float *near, float *lo, float *hi, float *sp);
			s32 host_seam_ph_at(float x, float y, float *lo, float *hi, float *sp);
			float sa[3] = { 0, -2, -5 }, sb[3] = { 4, 2, -5 }, h[2] = { 0.70711f, -0.70711f }, f[3], v[3] = { 0.f, 0.f, 0.f }, lo, hi, sp;
			s32 ph, m;
			f[0] = 2.f - h[0] * 0.55f; f[1] = 0.f - h[1] * 0.55f; f[2] = 5.f;
			float nr[3] = { 2.f, 0.f, -5.f };                                 /* 6i.127: the seam's nearest point (toward it: h) */
			ph = host_ph_check(f, v, 1, nr, &lo, &hi, &sp);
			assert(ph == 3 && lo > 0.24f && lo < 0.26f && hi > 0.24f && hi < 0.26f && sp > 9.5f && sp < 10.7f);
			v[0] = h[0]; v[1] = h[1];                                       /* at 1.0 forward: slow down */
			ph = host_ph_check(f, v, 1, nr, &lo, &hi, &sp);
			assert(ph == 2 && lo > 0.24f && lo < 0.26f);
			{
				/* 6i.127: none launches - over the flat middle of the floor, far from the seam: RED (4; 6h.34 left the colour as it
				 * was); and on the ground the check runs too, as the jump from here (2.25 forward, your take-off speed) */
				float fl[3] = { 3.5f, -1.5f, 5.f }, nf[3] = { 3.5f, -1.5f, -5.f }, v0[3] = { 0.f, 0.f, 0.f }, fg[3], lo2, hi2, sp2;
				u32 host_ph_checks(void); u32 c0 = host_ph_checks();
				ph = host_ph_check(fl, v0, 1, nf, &lo2, &hi2, &sp2);
				assert(ph == 4 && sp2 == 0.f);
				fg[0] = 2.f - h[0] * 1.5f; fg[1] = 0.f - h[1] * 1.5f; fg[2] = -5.f;
				ph = host_ph_check(fg, v0, 0, nr, &lo2, &hi2, &sp2);
				assert(ph == 4 && host_ph_checks() == c0 + 2);                    /* checked (6h.34: never on the ground); a flat jump: none */
			}
			{
				/* the independent re-run: step the fall at 0.25 forward, find the first contact tick, solve capped */
				sbx_surf S[2];
				double tri[2][3][3] = { { { 0, -2, -5 }, { 4, -2, -5 }, { 4, 2, -5 } }, { { 4, 2, -5 }, { 0, 2, -5 }, { 0, -2, -5 } } };
				double px = f[0], py = f[1], z = f[2], vz = 0.0, vx = h[0] * 0.25, vy = h[1] * 0.25, launch = -1.0;
				s32 k3;
				for (m = 0; m < 2; m++) { S[m].n[0] = 0; S[m].n[1] = 0; S[m].n[2] = 1; S[m].nv = 3; S[m].soft = 0; S[m].key = m; for (k3 = 0; k3 < 3; k3++) { S[m].v[k3][0] = tri[m][k3][0]; S[m].v[k3][1] = tri[m][k3][1]; S[m].v[k3][2] = tri[m][k3][2]; } sbx_surf_bounds(&S[m]); }
				for (k3 = 0; k3 < 200; k3++)
				{
					double vz1 = vz - 4.1712 / 30.0, feet[3] = { px, py, z }, vpre[3] = { vx, vy, vz1 }, out[3], l, vc[3];
					sbx_row rows[32]; s32 n = sbx_landing_contacts(feet, vpre, 1.0 / 30.0, S, 2, rows, 32);
					if (n)
					{
						l = sqrt(vpre[0] * vpre[0] + vpre[1] * vpre[1] + vpre[2] * vpre[2]);
						vc[0] = vpre[0] * 4.085 / l; vc[1] = vpre[1] * 4.085 / l; vc[2] = vpre[2] * 4.085 / l;
						sbx_solve(rows, n, l > 4.085 ? vc : vpre, 1.0 / 30.0, out);
						launch = sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
						break;
					}
					px += vx / 30.0; py += vy / 30.0; z += vz1 / 30.0; vz = vz1;
				}
				assert(launch > 0.0 && fabs(launch - sp) < 0.05);
			}
			/* the whole pipeline: in the air above it, barely moving, your view across it - the seam drawn purple, its label's
			 * PHYSICS line says SPEED UP */
			{
				void host_ph_forget(void);
				host_ph_forget();
				float cam[14]; s32 c3; for (c3 = 0; c3 < 14; c3++) cam[c3] = M32f(0x1800 + 4 * (u32)c3);   /* (put back after) */
				/* your view: from you, across it, looking 60 deg down (forward and up kept square) */
				WF(0x1800 + 0x00, f[0]); WF(0x1800 + 0x04, f[1]); WF(0x1800 + 0x08, f[2] + 0.6f);
				WF(0x1800 + 0x20, h[0] * 0.5f); WF(0x1800 + 0x24, h[1] * 0.5f); WF(0x1800 + 0x28, -0.866f);
				WF(0x1800 + 0x2C, h[0] * 0.866f); WF(0x1800 + 0x30, h[1] * 0.866f); WF(0x1800 + 0x34, 0.5f);
				void host_set_sb_hud(s32 on); s32 host_sb_hud(void); s32 hud = host_sb_hud();
				host_set_land_text(1); host_set_sb_hud(1);
				for (m = 0; m < 2; m++)
				{
					state(600 + m, 0.f, 1, 1);
					WF(OBJECT + 0x64, f[0]); WF(OBJECT + 0x68, f[1]); WF(OBJECT + 0x6C, f[2] - 0.139f * (float)m / 30.f);
					WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -0.139f * (float)m);
					g_q = 0; frame();
				}
				ph = host_seam_ph_at(2.f, 0.f, &lo, &hi, &sp);
				assert(ph == 3 && drawn("PHYSICS: SPEED UP - forward 0.25-0.25 u/s launches (max 2.25) -> about 10 u/s"));   /* 6i.127: HCM's wording */
				host_set_land_text(0); host_set_sb_hud(hud);
				for (c3 = 0; c3 < 14; c3++) WF(0x1800 + 4 * (u32)c3, cam[c3]);
			}
			printf("physics seam colours: the diagonal seam 10 up - launches only at 0.25 forward (%.1f u/s, matched by the separate core): purple SPEED UP standing still, light blue SLOW DOWN at 1.0\n", sp);
		}	}
	/* 6h.30: a landing that LAUNCHES - the exact physics on the shape of a live dreadnought ("sink past floor 1", model.md):
	 * a 0.3-thick slab whose top (z -5.5, x -3..0.6) and underside (z -5.8, x -0.2..3) have offset edges. Charged, plunging
	 * at 25 u/s drifting 2 u/s in x, the swept pill passes the slab in one tick; its shortest way out of the top face is
	 * sideways (+x, tilted 0.08 up by the sweep) and out of the underside sideways the other way (-x): deepest first
	 * they are coupled, and the near-opposite pair turns 16 u/s of push-out into a 37 u/s launch straight up - a
	 * DREADNOUGHT. 0.5 u/s of drift either way keeps it (35-37): LIKELY. The same landing uncharged: red. */
	{
		const u32 T = 0xBE000, TP = T + 0x100, TS = T + 0x200, TE = T + 0x300, TV = T + 0x600;
		s32 host_ex_q(void), host_ex_shown_q(void); double host_ex_speed(void); const char *host_ex_why(void);
		float pn[2][4] = { { 0, 0, 1, -5.5f }, { 0, 0, -1, 5.8f } };
		/* vertices counter-clockwise about each normal */
		float tv[8][3] = { { -3, -3, -5.5f }, { 0.6f, -3, -5.5f }, { 0.6f, 3, -5.5f }, { -3, 3, -5.5f },
			{ -0.2f, 3, -5.8f }, { 3, 3, -5.8f }, { 3, -3, -5.8f }, { -0.2f, -3, -5.8f } };
		float p[3];
		s32 q, k2, j2, t;
		for (q = 0; q < 2; q++) for (j2 = 0; j2 < 4; j2++) WF(TP + q * 16 + j2 * 4, pn[q][j2]);
		for (q = 0; q < 2; q++)
		{
			W32(TS + q * 8, (u32)q | ((u32)(q * 4) << 16)); W32(TS + q * 8 + 4, 0);           /* plane q, first edge 4q, flags 0 */
			for (k2 = 0; k2 < 4; k2++)
			{
				u32 e = (u32)(q * 4 + k2), v0 = (u32)(q * 4 + k2), v1 = (u32)(q * 4 + (k2 + 1) % 4), nxt = (u32)(q * 4 + (k2 + 1) % 4);
				W32(TE + e * 12, v0 | (v1 << 16)); W32(TE + e * 12 + 4, nxt | (0xffffu << 16)); W32(TE + e * 12 + 8, (u32)q | (0xffffu << 16));
			}
		}
		for (k2 = 0; k2 < 8; k2++) { for (j2 = 0; j2 < 3; j2++) WF(TV + k2 * 16 + j2 * 4, tv[k2][j2]); W32(TV + k2 * 16 + 12, 0); }
		W32(T + 0x08, 2); W32(T + 0x0C, TP); W32(T + 0x28, 2); W32(T + 0x2C, TS); W32(T + 0x30, 8); W32(T + 0x34, TE); W32(T + 0x38, 8); W32(T + 0x3C, TV);
		W32(0x1B00, T); W32(0x1B04, 0);
		/* one tick before the landing tick: feet (-0.0667, 0, -4.6293) at (2, 0, -24.722) -> the landing tick starts at
		 * (0, 0, -5.458) with (2, 0, -25.0) and crosses the top face */
#define SLAB_STATE(tk, ch) do { state(tk, -24.722f, 1, ch); WF(OBJECT + 0x64, -0.0667f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -4.6293f); WF(OBJECT + 0x88, 2.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -24.722f); } while (0)
		for (t = 0; t < 3; t++) { SLAB_STATE(520 + t, 1); g_q = 0; frame(); }
		assert(host_landing(p) && p[2] > -5.51f && p[2] < -5.49f);
		printf("landing verdict (exact): the offset-edge slab - %d (%s), exit %.1f u/s\n", host_ex_q(), host_ex_why(), host_ex_speed());
		/* 6h.34: with the capped solver (it starts from 4.085 u/s, not the 25 u/s fall) the pair launches 40.6 u/s straight up
		 * (37.0 before), a DREADNOUGHT, and 0.5 u/s of drift either way keeps it (38.3 - 41.9): LIKELY */
		assert(host_ex_q() == 2 && host_landing_q() == 2 && host_ex_speed() >= 40.0 && host_ex_speed() <= 41.0 && !strncmp(host_ex_why(), "LIKELY: 40.6", 12) && strstr(host_ex_why(), "DREADNOUGHT"));
		host_set_land_text(1); g_q = 0; frame(); assert(drawn("LIKELY: ") && drawn("DREADNOUGHT")); host_set_land_text(0);
		/* two searches agree before the shown verdict changes: uncharged, the first search still shows green, the second red */
		SLAB_STATE(523, 0); frame();
		/* 6h.32: in the last 0.4 s before the landing there's no hold - the change shows at once (the hold far out: below) */
		assert(host_ex_q() == 0 && host_landing_q() == 0 && !strcmp(host_ex_why(), "NOT CHARGED"));
		SLAB_STATE(524, 0); frame();
		/* on the ground the hold resets: the next fall's first verdict shows at once */
		state(525, 0.f, 0, 1); frame();
		SLAB_STATE(526, 1); frame();
		assert(host_ex_q() == 2 && host_landing_q() == 2);
		printf("landing verdict (exact): far out a change shows once two searches agree, in the last 0.4 s at once; a new fall starts afresh\n");
		/* the landing recorder: landing (the fall stops) with the viewer on - one line with what was shown, then the result */
		{
			void host_set_sb_hud(s32 on); s32 host_sb_hud(void);
			s32 before = logged("landing: shown green \"LIKELY: 40.6"), hud_was = host_sb_hud();
			host_set_sb_hud(1);
			state(527, -3.f, 0, 1); WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -5.5f); WF(OBJECT + 0x88, 2.f); WF(OBJECT + 0x90, 30.f); frame();
			assert(logged("landing: shown green \"LIKELY: 40.6") == before + 1 && logged("predicted exit x10 406") >= 1);
			for (t = 0; t < 4; t++) { state(528 + t, 30.f, 1, 1); WF(OBJECT + 0x88, 2.f); WF(OBJECT + 0x90, 30.f - 0.14f * (float)t); frame(); }
			assert(logged("landing result: ") >= 1);
			assert(logged("(first tick x10 ") >= 1 && logged(" (up x10 ") >= 1);   /* (6i.212: the Cartographer part of the line is gone) */
			assert(logged(", Cartographer ") == 0 && logged("(Cartographer)") == 0 && logged("landing: shown ") >= 1);   /* 6i.212: one verdict in the log - the exact physics' */
			assert(logged(", tick 2 ") >= 1);                                 /* 6h.37: the tick after the predicted launch */
			/* 6h.31: a landing far from the last prediction (a launch or turn since) is logged as STALE, not scored */
			for (t = 0; t < 2; t++) { SLAB_STATE(540 + t, 1); frame(); }
			state(542, -3.f, 0, 1); WF(OBJECT + 0x64, 5.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -5.5f); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x90, 30.f); frame();
			assert(logged("landing: STALE prediction (it was green") >= 1 && logged("ticks old, x100 49") >= 1 && logged(" ticks from x100 ") >= 1);   /* 6h.37: from where */
			assert(logged(" of 4 next to it, fall x10 ") >= 1);
			/* 6h.34: the default circle is Cartographer's verdict; the recorder logs it AND the exact one, every landing */
			{
				void host_set_exact_circle(s32 on);
				host_set_exact_circle(0);
				for (t = 0; t < 2; t++) { SLAB_STATE(550 + t, 1); frame(); }
				state(552, -3.f, 0, 1); WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -5.5f); WF(OBJECT + 0x88, 2.f); WF(OBJECT + 0x90, 30.f); frame();
				/* 6i.212: (was a check of Cartographer's landing verdict - retired) */
				host_set_exact_circle(1);
			}
			host_set_sb_hud(hud_was);
			printf("landing recorder: a landing logs the shown verdict, predicted exit, place and seam colour, then the result\n");
		}
#undef SLAB_STATE
	}
	{ void host_set_exact_circle(s32 on); host_set_exact_circle(0); }
	/* 6h.38: the same slab, with a wall face just beside your fall (x = -0.25, facing +x, z -5 .. -3): on the tick before the
	 * crossing the pill grazes it - a sideways push, your velocity capped at 4.085 and still going down. The game solves
	 * that tick: the fall is gone, no 40 u/s launch. The verdict now takes the first tick with a contact like that */
	{
		const u32 T = 0xC6000, TP = T + 0x100, TS = T + 0x200, TE = T + 0x300, TV = T + 0x600;
		s32 host_ex_q(void); double host_ex_speed(void); const char *host_ex_why(void); void host_set_exact_circle(s32 on);
		float pn[3][4] = { { 0, 0, 1, -5.5f }, { 0, 0, -1, 5.8f }, { 1, 0, 0, -0.25f } };
		float tv[12][3] = { { -3, -3, -5.5f }, { 0.6f, -3, -5.5f }, { 0.6f, 3, -5.5f }, { -3, 3, -5.5f },
			{ -0.2f, 3, -5.8f }, { 3, 3, -5.8f }, { 3, -3, -5.8f }, { -0.2f, -3, -5.8f },
			{ -0.25f, -3, -5.f }, { -0.25f, 3, -5.f }, { -0.25f, 3, -3.f }, { -0.25f, -3, -3.f } };
		s32 q, k2, j2, t;
		for (q = 0; q < 3; q++) for (j2 = 0; j2 < 4; j2++) WF(TP + q * 16 + j2 * 4, pn[q][j2]);
		for (q = 0; q < 3; q++)
		{
			W32(TS + q * 8, (u32)q | ((u32)(q * 4) << 16)); W32(TS + q * 8 + 4, 0);
			for (k2 = 0; k2 < 4; k2++)
			{
				u32 e = (u32)(q * 4 + k2), v0 = (u32)(q * 4 + k2), v1 = (u32)(q * 4 + (k2 + 1) % 4), nxt = (u32)(q * 4 + (k2 + 1) % 4);
				W32(TE + e * 12, v0 | (v1 << 16)); W32(TE + e * 12 + 4, nxt | (0xffffu << 16)); W32(TE + e * 12 + 8, (u32)q | (0xffffu << 16));
			}
		}
		for (k2 = 0; k2 < 12; k2++) { for (j2 = 0; j2 < 3; j2++) WF(TV + k2 * 16 + j2 * 4, tv[k2][j2]); W32(TV + k2 * 16 + 12, 0); }
		W32(T + 0x08, 3); W32(T + 0x0C, TP); W32(T + 0x28, 3); W32(T + 0x2C, TS); W32(T + 0x30, 12); W32(T + 0x34, TE); W32(T + 0x38, 12); W32(T + 0x3C, TV);
		W32(0x1B00, T); W32(0x1B04, 0);
		host_set_exact_circle(1);
		for (t = 0; t < 3; t++)
		{
			state(2100 + t, -24.722f, 1, 1); WF(OBJECT + 0x64, -0.0667f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -3.8f);
			WF(OBJECT + 0x88, 2.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -24.722f);
			g_q = 0; frame();
		}
		printf("graze: the slab with a wall beside the fall - %d (%s)\n", host_ex_q(), host_ex_why());
		assert(host_ex_q() == 0 && !strncmp(host_ex_why(), "NO LAUNCH: you graze an edge first", 34));
		host_set_exact_circle(0);
		W32(0x1B00, 0);
	}
	/* Cartographer's Force Charged: not charged, the toggle on - the pair between you and the environment (its partner's
	 * owner body is in no Havok component) gets -3. A partner owned by an object's body is left alone. */
	{
		W32(0x1B00, 0); W32(0x1B04, 0);
		W32(0x50020 + 0x10, 0x51000);                                        /* the partner's owner: the level's body */
		host_set_force_charge(1);
		state(600, 0.f, 0, 0); frame();
		assert((s32)M32h(PAIRS + 44) == -3);
		W32(0x50020 + 0x10, ENTITY);                                        /* now an object's (a component's) body */
		state(601, 0.f, 0, 0); frame();
		assert((s32)M32h(PAIRS + 44) == 0);
		host_set_force_charge(0);
		state(602, 0.f, 0, 0); frame(); assert((s32)M32h(PAIRS + 44) == 0);
		printf("force charged: the pair with the environment set to -3; one with an object left alone; off: nothing\n");
	}
	/* Keep charge on reverts (Cartographer): (1) charged, then time goes back while not - restored. (2) not charged, a
	 * revert with no save to it - nothing. (3) charged when a checkpoint saves (main_globals.save_map set before the
	 * update), later not; a revert to that save - restored (the charge you had at the save). */
	{
		s32 t;
		W32(0x1B00, 0); W32(0x1B04, 0); W32(ADDR_SAVE_MAP_T, 0);
		W32(0x50020 + 0x10, 0x51000);                                        /* the partner: the level's body */
		host_set_keep_charge(1);
		for (t = 0; t < 3; t++) { state(1100 + t, 0.f, 0, 1); frame(); }
		state(1050, 0.f, 0, 0); frame();                                     /* (1) reverted, charge lost */
		assert((s32)M32h(PAIRS + 44) == -3);
		for (t = 0; t < 40; t++) { state(1200 + t, 0.f, 0, 0); frame(); }   /* (2) not charged for a while */
		state(1150, 0.f, 0, 0); frame();
		assert((s32)M32h(PAIRS + 44) == 0);
		for (t = 0; t < 3; t++) { state(1300 + t, 0.f, 0, 1); frame(); }   /* (3) charged... */
		W32(ADDR_SAVE_MAP_T, 1); state(1303, 0.f, 0, 1); frame();           /* ...a checkpoint saves */
		W32(ADDR_SAVE_MAP_T, 0);
		for (t = 0; t < 40; t++) { state(1304 + t, 0.f, 0, 0); frame(); }   /* the charge goes, a while passes */
		assert((s32)M32h(PAIRS + 44) == 0);
		state(1303, 0.f, 0, 0); frame();                                     /* revert to that save, not charged */
		assert((s32)M32h(PAIRS + 44) == -3);
		host_set_keep_charge(0);
		printf("keep charge: restored after a revert; not without a reason; restored at a save you were charged at\n");
	}
	/* ghost seams (sb_ghost_get / sb_ghost_impact): 2 launches in 5 lottery landings = 40%: amber; 2 in 7 = 29%: not;
	 * 3 in 8: again. The same seam either way round. */
	{
		float ga[3] = { 1.f, 2.f, -3.f }, gb[3] = { 4.f, 2.f, -3.f };
		s32 i;
		assert(!host_ghost(ga, gb, 0, 0));
		for (i = 0; i < 5; i++) host_ghost(ga, gb, 1, i < 2);
		assert(host_ghost(gb, ga, 0, 0));
		host_ghost(ga, gb, 1, 0); host_ghost(ga, gb, 1, 0);
		assert(!host_ghost(ga, gb, 0, 0));
		host_ghost(ga, gb, 1, 1);
		assert(host_ghost(ga, gb, 0, 0));
		printf("ghost seams: 2 of 5 amber, 2 of 7 not, 3 of 8 again; the seam known either way round\n");
	}
	/* covered seams (Cartographer's blockers): the two-triangle floor 5 down and its diagonal seam, standing 5 above it -
	 * rated; with a cover quad 2 down over all of it (in the same level BSP) - covered: not rated */
	{
		const u32 C = 0xBA000, CP = C + 0x100, CS = C + 0x200, CE = C + 0x300, CV = C + 0x400;
		float cv[8][3] = { { 0, -2, -5 }, { 4, -2, -5 }, { 4, 2, -5 }, { 0, 2, -5 }, { -1, -3, -2 }, { 5, -3, -2 }, { 5, 3, -2 }, { -1, 3, -2 } };
		u16 ce[9][6] = { { 0, 1, 1, 0xffff, 0, 0xffff }, { 1, 2, 2, 0xffff, 0, 0xffff }, { 2, 0, 0, 3, 0, 1 },
			{ 2, 3, 4, 0xffff, 1, 0xffff }, { 3, 0, 2, 0xffff, 1, 0xffff },
			{ 4, 5, 6, 0xffff, 2, 0xffff }, { 5, 6, 7, 0xffff, 2, 0xffff }, { 6, 7, 8, 0xffff, 2, 0xffff }, { 7, 4, 5, 0xffff, 2, 0xffff } };
		int k2, j2, with;
		WF(CP, 0); WF(CP + 4, 0); WF(CP + 8, 1); WF(CP + 12, -5);
		WF(CP + 16, 0); WF(CP + 20, 0); WF(CP + 24, 1); WF(CP + 28, -2);
		W32(CS, 0u | (0u << 16)); W32(CS + 4, 0); W32(CS + 8, 0u | (2u << 16)); W32(CS + 12, 0); W32(CS + 16, 1u | (5u << 16)); W32(CS + 20, 0);
		for (k2 = 0; k2 < 9; k2++) for (j2 = 0; j2 < 6; j2 += 2) W32(CE + k2 * 12 + j2 * 2, (u32)ce[k2][j2] | ((u32)ce[k2][j2 + 1] << 16));
		for (k2 = 0; k2 < 8; k2++) { for (j2 = 0; j2 < 3; j2++) WF(CV + k2 * 16 + j2 * 4, cv[k2][j2]); W32(CV + k2 * 16 + 12, 0); }
		W32(C + 0x08, 2); W32(C + 0x0C, CP); W32(C + 0x2C, CS); W32(C + 0x34, CE); W32(C + 0x3C, CV);
		W32(0x1B04, 0); host_set_force_charge(0);
		for (with = 0; with < 2; with++)
		{
			W32(C + 0x28, with ? 3 : 2); W32(C + 0x30, with ? 9 : 5); W32(C + 0x38, with ? 8 : 4);
			W32(0x1B00, C);
			state(700 + with, 0.f, 0, 1);
			WF(OBJECT + 0x64, 2.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
			WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			g_q = 0; frame();
			if (!with) assert(host_rated_seams() >= 1);
			else assert(host_rated_seams() == 0);
		}
		printf("covered seams: the diagonal rated in the open, not rated under a cover\n");
	}
	/* 6h.33: on the ground every seam that's drawn has a colour at once (no grey: pending is gone, and seams level with or
	 * above you - unrated - aren't drawn in a dim colour any more) */
	{
		s32 host_vis_uncoloured(void), host_exr_more(s32 *pending, s32 *held), pend, held;
		W32(0x1B00, 0xB8000); W32(0x1B04, 0);
		state(710, 0.f, 0, 1);
		WF(OBJECT + 0x64, 2.f); WF(OBJECT + 0x68, -0.7f); WF(OBJECT + 0x6C, 0.f);
		WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
		g_q = 0; frame();
		host_exr_more(&pend, &held);
		assert(host_rated_seams() >= 1 && pend == 0 && host_vis_uncoloured() == 0);
		W32(0x1B00, 0x80000);                                                /* the synthetic seams, some level with you */
		state(711, 0.f, 0, 1); WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -1.f); frame();
		assert(host_vis_uncoloured() == 0);
		printf("ground: every seam drawn has a colour at once - none grey\n");
		{
			/* the gather keeps the nearest when it's full (6h.32: a heap - 6h.31 rescanned for the farthest every time) */
			s32 host_ex_gather(const float *ctr, s32 cap, u32 *keys); u32 keys[4];
			float c0[3] = { -2.5f, 0.f, -5.5f }, c1[3] = { 2.5f, 0.f, -5.8f };              /* the offset-edge slab's top / underside */
			W32(0x1B00, 0xBE000); state(712, 0.f, 0, 1); WF(OBJECT + 0x6C, 0.f); frame();
			assert(host_ex_gather(c0, 1, keys) == 1 && keys[0] == 0u && host_ex_gather(c1, 1, keys) == 1 && keys[0] == 1u && host_ex_gather(c0, 4, keys) == 2);
			W32(0x1B00, 0xB8000);
		}
		{
			/* 6h.33: a launch: 6h.30's rule, and the contacts changed your velocity by half your entry speed (6 at least). Falling
			 * on through with a 4.8 u/s sideways nudge (6h.31's false 22 u/s MONSTERs) is not one; a bounce that still leaves
			 * you moving down a little on the landing tick is (6h.32 threw those away: no green verdicts in its log) */
			s32 host_ex_launched(double vx, double vy, double vz, double px, double py, double pz);
			/* 6h.34: the capped solver - a contact that barely touches a 22 u/s fall gives ~4 u/s out (the cap), not 22: no
			 * "launch" can be the fall carrying on; a flat floor 0.46 deep gives a hop of ~3 u/s up, not a launch */
			void host_ex_solve1(const double *n, double d, const double *vin, double *out);
			double up[3] = { 0, 0, 1 }, side[3] = { 0.6, 0, 0.8 }, vin[3] = { 0.0, 0, -22.3 }, out[3], l;
			host_ex_solve1(side, -0.01, vin, out); l = sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
			assert(l < 4.2 && !host_ex_launched(out[0], out[1], out[2], vin[0], vin[1], vin[2]));
			host_ex_solve1(up, -0.46, vin, out);
			assert(out[2] > 2.5 && out[2] < 4.0 && !host_ex_launched(out[0], out[1], out[2], vin[0], vin[1], vin[2]));
			assert(host_ex_launched(-1.3, 0, 37.0, 2.0, 0, -25.0) && host_ex_launched(7.0, 0, 0.5, 2.0, 0, -12.0) && !host_ex_launched(4.55, 0, 3.6, 2.0, 0, -25.0));
			printf("launch: the solver starts capped at 4.085 u/s - a barely-touching contact on a 22 u/s fall gives %.1f u/s out, a flat floor a %.1f u/s hop\n", l, out[2]);
		}
	}
	/* 6h.40: the frame order on the console - the landing check runs at the start of a frame (before the game's tick), the
	 * search at its end (after it). A bounce: the search of that frame sees you already launched and predicts the next
	 * fall; the recorder must keep the prediction made before, for the landing check of the next frame to score */
	{
		u32 host_lr_kept(void); void host_set_sb_hud(s32 on); s32 host_sb_hud(void); void host_set_exact_circle(s32 on);
		s32 hud = host_sb_hud(), shown0, stale0;
		u32 kept0 = host_lr_kept();
		W32(0x1B00, 0xB8000); W32(0x1B04, 0);
		host_set_sb_hud(1); host_set_exact_circle(1);
		state(3000, -6.f, 1, 1); WF(OBJECT + 0x64, 2.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -4.3f); WF(OBJECT + 0x88, 0.3f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -6.f);
		g_q = 0; frame();
		state(3001, -6.14f, 1, 1); WF(OBJECT + 0x64, 2.01f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -4.5f); WF(OBJECT + 0x88, 0.3f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -6.14f);
		g_q = 0; frame();                                                  /* the prediction: down onto the floor at -5 */
		shown0 = logged("landing: shown "); stale0 = logged("landing: STALE");
		/* the game's tick: you bounce (12 u/s up); this frame's search runs after it - and predicts the next fall */
		state(3002, 12.f, 1, 1); WF(OBJECT + 0x64, 2.02f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -4.6f); WF(OBJECT + 0x88, 0.3f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 12.f);
		g_nlines = 0; spooky_view(0x120C); spooky_frame();
		assert(host_lr_kept() == kept0 + 1);                                 /* the prediction from before the bounce kept */
		spooky_main_frame();                                                 /* the next frame: the landing check */
		assert(logged("landing: shown ") == shown0 + 1 && logged("landing: STALE") == stale0);
		host_set_sb_hud(hud); host_set_exact_circle(0);
		W32(0x1B00, 0);
		printf("recorder: a bounce seen by the search before the landing check - the prediction from before it kept and scored (not STALE)\n");
	}
	/* 6h.37: on a slow console (a search's steps spread over 4 frames) the landing circle follows the trajectory every frame
	 * between searches - the path's crossing with the drawn landing's polygon - instead of waiting for the next search.
	 * Over the diagonal floor 5 down, drifting faster each frame: the drawn circle moves every frame, onto the floor. */
	{
		void host_set_step_div(s32 d); u32 host_land_follows(void); s32 host_drawn_landing(float *p);
		float lp[3], prev_x = -100.f, vz = -2.f, z = 0.f, x = 0.8f;
		s32 f2, moved = 0, frames = 0;
		u32 f0;
		W32(0x1B00, 0xB8000); W32(0x1B04, 0);
		host_set_step_div(4);
		for (f2 = 0; f2 < 12; f2++)                                         /* a few searches first */
		{
			float vx = 0.6f + 0.05f * (float)f2;
			state(2000 + f2, vz, 1, 1); WF(OBJECT + 0x64, x); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, z); WF(OBJECT + 0x88, vx); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, vz);
			g_q = 0; frame();
			vz -= 4.1712f / 30.f; z += vz / 30.f; x += vx / 30.f;
		}
		f0 = host_land_follows();
		for (f2 = 12; f2 < 20; f2++)
		{
			float vx = 0.6f + 0.05f * (float)f2;
			state(2000 + f2, vz, 1, 1); WF(OBJECT + 0x64, x); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, z); WF(OBJECT + 0x88, vx); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, vz);
			g_q = 0; frame();
			assert(host_drawn_landing(lp) && lp[2] > -5.01f && lp[2] < -4.99f);
			if (lp[0] != prev_x) moved++;
			prev_x = lp[0]; frames++;
			vz -= 4.1712f / 30.f; z += vz / 30.f; x += vx / 30.f;
		}
		assert(host_land_follows() - f0 >= 7u && moved >= 7);
		{
			/* 6i.224: the circle no longer sticks at a seam - between searches it follows the path where it comes down
			 * inside the last search's polygon OR up to 0.75 of a unit over its edge (on that polygon's plane); further
			 * out it waits for the next search */
			s32 host_follow_ok(float (*pv)[3], s32 k, const float *wn, const float *hit);
			float sq[4][3] = { { 0.f, 0.f, 0.f }, { 2.f, 0.f, 0.f }, { 2.f, 2.f, 0.f }, { 0.f, 2.f, 0.f } }, up[3] = { 0.f, 0.f, 1.f };
			float in_[3] = { 1.f, 1.f, 0.f }, over[3] = { 2.5f, 1.f, 0.f }, edge[3] = { 2.74f, 1.f, 0.f }, far_[3] = { 2.8f, 1.f, 0.f }, corner[3] = { 2.5f, 2.5f, 0.f }, cfar[3] = { 2.6f, 2.6f, 0.f };
			assert(host_follow_ok(sq, 4, up, in_) && host_follow_ok(sq, 4, up, over) && host_follow_ok(sq, 4, up, edge));
			assert(!host_follow_ok(sq, 4, up, far_));
			assert(host_follow_ok(sq, 4, up, corner) && !host_follow_ok(sq, 4, up, cfar));     /* past a corner: 0.707 in, 0.849 out */
		}
		{
			/* 6h.38: a sudden change of drift (2 u/s sideways): between searches the circle never follows the path more than 1
			 * unit from where the last search put it */
			void host_drawn_landing_c(float *p);
			float c[3];
			for (f2 = 20; f2 < 28; f2++)
			{
				state(2000 + f2, vz, 1, 1); WF(OBJECT + 0x64, x); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, z); WF(OBJECT + 0x88, 1.f); WF(OBJECT + 0x8C, 2.f); WF(OBJECT + 0x90, vz);
				g_q = 0; frame();
				host_drawn_landing(lp); host_drawn_landing_c(c);
				assert((lp[0] - c[0]) * (lp[0] - c[0]) + (lp[1] - c[1]) * (lp[1] - c[1]) <= 1.0001f);
				vz -= 4.1712f / 30.f; z += vz / 30.f; x += 1.f / 30.f;
				if (z < -4.5f) break;
			}
		}
		host_set_step_div(0);
		W32(0x1B00, 0);
		printf("landing circle: on a 4-frame search schedule it moved %d of %d frames (followed the path %u times between searches)\n", moved, frames, host_land_follows() - f0);
	}
	/* 6h.35: a landing on an edge the pill reaches - a ledge (floor at 0, x -2..0) above a lower floor (at -3, x 0..3).
	 * Falling straight down 0.1 outside the ledge, the feet miss its polygon, but the pill (radius 0.175) catches its edge:
	 * the landing is on the ledge edge, not 3 units below. 0.3 outside, it misses and lands below. */
	{
		const u32 T = 0xC4000, TP = T + 0x100, TS = T + 0x200, TE = T + 0x300, TV = T + 0x600;
		float pn[3][4] = { { 0, 0, 1, 0 }, { 0, 0, 1, -3.f }, { 1, 0, 0, 0 } };
		float tv[12][3] = { { -2, -1, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { -2, 1, 0 },
			{ 0, -1, -3 }, { 3, -1, -3 }, { 3, 1, -3 }, { 0, 1, -3 },
			{ 0, -1, 0 }, { 0, -1, -3 }, { 0, 1, -3 }, { 0, 1, 0 } };
		float p[3], xs[2] = { 0.1f, 0.3f };
		s32 q, k2, j2, r;
		for (q = 0; q < 3; q++) for (j2 = 0; j2 < 4; j2++) WF(TP + q * 16 + j2 * 4, pn[q][j2]);
		for (q = 0; q < 3; q++)
		{
			W32(TS + q * 8, (u32)q | ((u32)(q * 4) << 16)); W32(TS + q * 8 + 4, 0);
			for (k2 = 0; k2 < 4; k2++)
			{
				u32 e = (u32)(q * 4 + k2), v0 = (u32)(q * 4 + k2), v1 = (u32)(q * 4 + (k2 + 1) % 4), nxt = (u32)(q * 4 + (k2 + 1) % 4);
				W32(TE + e * 12, v0 | (v1 << 16)); W32(TE + e * 12 + 4, nxt | (0xffffu << 16)); W32(TE + e * 12 + 8, (u32)q | (0xffffu << 16));
			}
		}
		for (k2 = 0; k2 < 12; k2++) { for (j2 = 0; j2 < 3; j2++) WF(TV + k2 * 16 + j2 * 4, tv[k2][j2]); W32(TV + k2 * 16 + 12, 0); }
		W32(T + 0x08, 3); W32(T + 0x0C, TP); W32(T + 0x28, 3); W32(T + 0x2C, TS); W32(T + 0x30, 12); W32(T + 0x34, TE); W32(T + 0x38, 12); W32(T + 0x3C, TV);
		W32(0x1B00, T); W32(0x1B04, 0);
		for (r = 0; r < 2; r++)
		{
			state(1900 + r * 2, -6.f, 1, 1); WF(OBJECT + 0x64, xs[r]); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 2.f); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -6.f);
			g_q = 0; frame();
			state(1901 + r * 2, -6.14f, 1, 1); WF(OBJECT + 0x64, xs[r]); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 1.8f); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -6.14f);
			g_q = 0; frame();
			assert(host_landing(p));
			if (r == 0) assert(p[2] > -0.01f && p[2] < 0.01f && p[0] > 0.09f && p[0] < 0.11f);     /* at the ledge's height, where your feet are (6h.38: not snapped onto the edge) */
			else assert(p[2] > -3.01f && p[2] < -2.99f && p[0] > 0.29f && p[0] < 0.31f);          /* below */
		}
		{
			/* 6h.36: an edge landing is named in the recorder's line */
			void host_set_sb_hud(s32 on); s32 host_sb_hud(void); s32 hud = host_sb_hud(), before = logged(", on an edge");
			host_set_sb_hud(1);
			state(1904, -6.f, 1, 1); WF(OBJECT + 0x64, 0.1f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 2.f); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, -6.f); frame();
			state(1905, -6.14f, 1, 1); WF(OBJECT + 0x6C, 1.8f); WF(OBJECT + 0x90, -6.14f); frame();
			state(1906, -6.28f, 1, 1); WF(OBJECT + 0x6C, 1.59f); WF(OBJECT + 0x90, -6.28f); frame();
			state(1907, 0.f, 0, 1); WF(OBJECT + 0x6C, 0.f); WF(OBJECT + 0x90, 0.f); frame();
			assert(logged(", on an edge") == before + 1);
			host_set_sb_hud(hud);
		}
		W32(0x1B00, 0);
		printf("edge landings: 0.1 outside a ledge the pill catches its edge (landing on it, named in the log), 0.3 outside it lands 3 below\n");
	}
	/* 6h.34 (HCM 2026-11-01): ledge edges are seams - a floor meeting a face that goes down from the edge (a post top, a
	 * ledge, a step down); a floor meeting a face going up (an inside corner) is still left out */
	{
		s32 ledge[2];
		{
			const u32 T = 0xC0000, TP = T + 0x100, TS = T + 0x200, TE = T + 0x300, TV = T + 0x600;
			float pn[2][4] = { { 0.000000f, 0.000000f, 1.000000f, 0.000000f }, { 1.000000f, 0.000000f, 0.000000f, 2.000000f } };
			float tv[6][3] = { { 0.f, -1.f, 0.f }, { 2.f, -1.f, 0.f }, { 2.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 2.f, -1.f, -2.f }, { 2.f, 1.f, -2.f } };
			u32 te[7][3] = { { 0x10000u, 0xFFFF0001u, 0xFFFF0000u }, { 0x20001u, 0x40002u, 0x10000u }, { 0x30002u, 0xFFFF0003u, 0xFFFF0000u }, { 0x3u, 0xFFFF0000u, 0xFFFF0000u }, { 0x40001u, 0xFFFF0005u, 0xFFFF0001u }, { 0x50004u, 0xFFFF0006u, 0xFFFF0001u }, { 0x20005u, 0xFFFF0001u, 0xFFFF0001u } };
			s32 k2, j2;
			for (k2 = 0; k2 < 2; k2++) for (j2 = 0; j2 < 4; j2++) WF(TP + k2 * 16 + j2 * 4, pn[k2][j2]);
			W32(TS, 0u | (0u << 16)); W32(TS + 4, 0); W32(TS + 8, 1u | (4u << 16)); W32(TS + 12, 0);
			for (k2 = 0; k2 < 7; k2++) for (j2 = 0; j2 < 3; j2++) W32(TE + k2 * 12 + j2 * 4, te[k2][j2]);
			for (k2 = 0; k2 < 6; k2++) { for (j2 = 0; j2 < 3; j2++) WF(TV + k2 * 16 + j2 * 4, tv[k2][j2]); W32(TV + k2 * 16 + 12, 0); }
			W32(T + 0x08, 2); W32(T + 0x0C, TP); W32(T + 0x28, 2); W32(T + 0x2C, TS); W32(T + 0x30, 7); W32(T + 0x34, TE); W32(T + 0x38, 6); W32(T + 0x3C, TV);
			W32(0x1B00, T); W32(0x1B04, 0);
			state(1800, 0.f, 0, 1); WF(OBJECT + 0x64, 0.5f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 1.0f); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			g_q = 0; frame();
			ledge[0] = host_seam_count(-1);
		}
		{
			const u32 T = 0xC2000, TP = T + 0x100, TS = T + 0x200, TE = T + 0x300, TV = T + 0x600;
			float pn[2][4] = { { 0.000000f, 0.000000f, 1.000000f, 0.000000f }, { 1.000000f, 0.000000f, 0.000000f, 2.000000f } };
			float tv[6][3] = { { 0.f, -1.f, 0.f }, { 2.f, -1.f, 0.f }, { 2.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 2.f, -1.f, 2.f }, { 2.f, 1.f, 2.f } };
			u32 te[7][3] = { { 0x10000u, 0xFFFF0001u, 0xFFFF0000u }, { 0x20001u, 0x20004u, 0x1u }, { 0x30002u, 0xFFFF0003u, 0xFFFF0000u }, { 0x3u, 0xFFFF0000u, 0xFFFF0000u }, { 0x50002u, 0xFFFF0005u, 0xFFFF0001u }, { 0x40005u, 0xFFFF0006u, 0xFFFF0001u }, { 0x10004u, 0xFFFF0001u, 0xFFFF0001u } };
			s32 k2, j2;
			for (k2 = 0; k2 < 2; k2++) for (j2 = 0; j2 < 4; j2++) WF(TP + k2 * 16 + j2 * 4, pn[k2][j2]);
			W32(TS, 0u | (0u << 16)); W32(TS + 4, 0); W32(TS + 8, 1u | (4u << 16)); W32(TS + 12, 0);
			for (k2 = 0; k2 < 7; k2++) for (j2 = 0; j2 < 3; j2++) W32(TE + k2 * 12 + j2 * 4, te[k2][j2]);
			for (k2 = 0; k2 < 6; k2++) { for (j2 = 0; j2 < 3; j2++) WF(TV + k2 * 16 + j2 * 4, tv[k2][j2]); W32(TV + k2 * 16 + 12, 0); }
			W32(T + 0x08, 2); W32(T + 0x0C, TP); W32(T + 0x28, 2); W32(T + 0x2C, TS); W32(T + 0x30, 7); W32(T + 0x34, TE); W32(T + 0x38, 6); W32(T + 0x3C, TV);
			W32(0x1B00, T); W32(0x1B04, 0);
			state(1801, 0.f, 0, 1); WF(OBJECT + 0x64, 0.5f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 1.0f); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			g_q = 0; frame();
			ledge[1] = host_seam_count(-1);
		}
		assert(ledge[0] == 1 && ledge[1] == 0);
		W32(0x1B00, 0);
		printf("ledge seams: a floor's edge where a wall goes down is a seam (flat); where one goes up (an inside corner) it isn't\n");
	}
	/* the seam labels ("Seam info text"): standing over the synthetic seams 1 below at rest - Cartographer's words */
	{
		W32(0x1B04, 0); W32(0x1B00, 0x80000);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		host_set_land_text(1);
		state(800, 0.f, 0, 1); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame();
		{
			/* 6h.30: the seams you stand over are rated exactly (a few a search, nearest first) - their labels say what the exact
			 * physics found; these synthetic seams' floor does not launch */
			s32 host_exr_counts(s32 *rated, s32 *cached), host_exr_area(void), rated, cached, n = host_exr_counts(&rated, &cached);
			/* these synthetic seams have no polygons under them: nothing to land on exactly - the empirical colour and words stay */
			assert(rated == 0 && n == 0 && host_exr_area() == 0 && drawn("drop 1.0: too slow (walk "));
			printf("seam labels: no polygons under the synthetic seams - the empirical words stay (drop 1.0: too slow ...)\n");
		}
	}
	/* steady: over 200 seams in reach, standing still - the lines drawn are the same frame after frame */
	{
		s32 f, q_first = -1, n_first = -1;
		W32(0x1B04, 0); W32(0x1B00, 0x90000);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		g_keep_shown = 1; host_shown_reset();
		for (f = 0; f < 6; f++)
		{
			state(900 + f, 0.f, 0, 1); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			g_q = 0; frame();
			if (f == 1) q_first = g_q;
			if (f >= 1) assert(g_q == q_first);
		}
		assert(q_first > 0); n_first = q_first;
		g_keep_shown = 0;
		printf("steady: %d lines drawn, the same for 5 frames running\n", q_first);
	}
	/* in the air, pieces / objects only where you can land: from 20 up at rest (3.1 s: air control ~8 units, capped at 10),
	 * landing straight below - 9 out kept, 30 out skipped; no landing found yet - 10 around your feet (+ the search's 8):
	 * 15 out kept, 30 out skipped (5e.10 and before kept up to 40 out - too many seams out of reach) */
	{
		float f[3] = { 0, 0, 20 }, v[3] = { 0, 0, 0 }, lp[3] = { 0, 0, 0 };
		assert(host_air_near(f, v, 1, lp, 9.f, 0.f, 0.f) && !host_air_near(f, v, 1, lp, 30.f, 0.f, 0.f));
		assert(!host_air_near(f, v, 0, lp, 7.f, 0.f, 0.f) && !host_air_near(f, v, 0, lp, 30.f, 0.f, 0.f));  /* no landing yet: nothing (the landing zone only) */
		{
			/* seams in the air with a landing: kept near the landing (2-3 out), not 6 out - within the 8 around your feet,
			 * but past what air control can reach (5e.13 kept it: seams well past the reticle) */
			s32 host_seam_kept_air(const float *pf, const float *pv, s32 land_ok, const float *plp, const float *pa, const float *pb);
			/* a short jump: 3 up at rest - ~1.2 s of air control, a reach of ~3.7 around the landing */
			float f3[3] = { 0.f, 0.f, 3.f }, na[3] = { 2.f, 0.f, 0.f }, nb[3] = { 3.f, 0.f, 0.f }, fa[3] = { 6.f, -0.5f, 0.f }, fb[3] = { 6.f, 0.5f, 0.f };
			assert(host_seam_kept_air(f3, v, 1, lp, na, nb) && !host_seam_kept_air(f3, v, 1, lp, fa, fb));
			assert(!host_seam_kept_air(f3, v, 0, lp, fa, fb) && !host_seam_kept_air(f3, v, 0, lp, na, nb));   /* no landing found: none */
			{
				/* at the landing's height only: the same seam 2 out but 3 above the landing - not kept (the arc meets that
				 * height elsewhere: 5e.17 kept such seams away from the reticle) */
				float ha[3] = { 2.f, 0.f, 3.f }, hb[3] = { 3.f, 0.f, 3.f };
				float f6[3] = { 0.f, 0.f, 6.f };
				assert(!host_seam_kept_air(f6, v, 1, lp, ha, hb) && host_seam_kept_air(f6, v, 1, lp, na, nb));
			}
		}
		printf("in the air: pieces / objects searched only where you can land (9 out yes, 30 out no)\n");
	}
	/* Cartographer's practice tools. Create / Revert Checkpoint: the main loop's save flag and the revert flag before
	 * it, campaign or multiplayer. Launch: 15 u/s forward on a (3, 4, 0) walk -> the body's velocity (18, 4, 0) through Havok's
	 * setter. Teleport: 5 forward -> the position, both swept centres and the transform moved by (5, 0, 0). */
	{
		enum { GOPT = 0x46000, MOTION = 0x44000, MSTATE = 0x45000 };
		s32 k;
		W32(0x1D10, GOPT); W32(GOPT + 8, 1);                                  /* campaign */
		W32(ENTITY + 0x3C, MOTION); W32(ENTITY + 0x18, MSTATE);               /* the motion; the handle's motion state */
		W32(OBJECT + 0x14, 0xFFFFFFFFu);                                       /* not seated */
		WF(0x1800 + 0x20, 1.f); WF(0x1800 + 0x24, 0.f); WF(0x1800 + 0x28, 0.f);   /* looking along +x */
		W32(0x1CFC, 0); W32(0x1D00, 0);
		host_practice(1); assert(MEM[0x1D00] == 1 && MEM[0x1CFF] == 0 && !strcmp(host_toast(), "Checkpoint created"));
		W32(0x1D00, 0);
		host_practice(2); assert(MEM[0x1CFF] == 1 && MEM[0x1D00] == 0 && !strcmp(host_toast(), "Reverted to the last checkpoint"));
		W32(0x1CFC, 0);
		W32(GOPT + 8, 2);                                                      /* multiplayer: allowed too (as Cartographer) */
		host_practice(1); assert(MEM[0x1D00] == 1 && !strcmp(host_toast(), "Checkpoint created"));
		W32(0x1D00, 0); W32(GOPT + 8, 1);
		host_set_practice(15, 0, 5, 0);
		state(1400, 0.f, 0, 0);                                                /* walking (3, 4, 0) */
		g_lv_n = 0; host_practice(3);
		assert(g_lv_n == 1 && g_lv_motion == MOTION && g_lv[0] == 18.f && g_lv[1] == 4.f && g_lv[2] == 0.f);
		assert(M32f(OBJECT + 0x88) == 18.f && !strcmp(host_toast(), "Launched: 15 u/s forward, 0 up"));
		WF(OBJECT + 0x64, 1.f); WF(OBJECT + 0x68, 2.f); WF(OBJECT + 0x6C, 3.f);
		for (k = 0; k < 3; k++) { WF(MOTION + 0x60 + 4 * k, 1.f); WF(MOTION + 0x70 + 4 * k, 1.f); WF(MSTATE + 0x50 + 4 * k, 1.f); }
		host_practice(4);
		assert(M32f(OBJECT + 0x64) == 6.f && M32f(OBJECT + 0x68) == 2.f && M32f(OBJECT + 0x6C) == 3.f);
		assert(M32f(MOTION + 0x60) == 6.f && M32f(MOTION + 0x70) == 6.f && M32f(MSTATE + 0x50) == 6.f && M32f(MOTION + 0x64) == 1.f);
		assert(!strcmp(host_toast(), "Teleported 5 forward, 0 up"));
		/* the crash trace: after a launch / teleport, each frame's stages logged */
		frame();
		{ const char *host_trace_last(void); assert(strstr(host_trace_last(), "frame ") && strlen(host_trace_last()) > 8); }
		/* a body out of Havok's world (as below the map after a ghost off): launch does nothing, teleport moves the
		 * position only - the body is never touched */
		W32(ENTITY + 0x08, 0);
		WF(OBJECT + 0x88, 3.f); WF(OBJECT + 0x8C, 4.f); WF(OBJECT + 0x90, 0.f);
		g_lv_n = 0; host_practice(3);                                              /* launch: the object's velocity only */
		assert(g_lv_n == 0 && M32f(OBJECT + 0x88) == 18.f && strstr(host_toast(), "the object only"));
		WF(MOTION + 0x60, 6.f); WF(OBJECT + 0x64, 6.f);
		host_practice(4);
		assert(M32f(OBJECT + 0x64) == 11.f && M32f(MOTION + 0x60) == 6.f && strstr(host_toast(), "position only"));
		assert(logged("practice: bodies in the physics world 0 of 1") >= 2);
		W32(ENTITY + 0x08, WORLD);
		W32(ENTITY + 0x3C, 0); W32(ENTITY + 0x18, 0); W32(0x1D10, 0);
		printf("practice: checkpoint / revert flags (campaign and multiplayer); launch through Havok's setter; teleport with the bodies\n");
	}
	/* "Superbounce Viewer" is the master switch (as Cartographer's): off - no seams drawn, though "Seams near you" is on */
	{
		W32(0x1B04, 0); W32(0x1B00, 0x80000);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		state(1500, 0.f, 0, 1); g_q = 0; frame(); assert(g_q > 0);
		host_set_sb_viewer(0);
		state(1501, 0.f, 0, 1); g_q = 0; frame(); assert(g_q == 0);
		host_set_sb_viewer(1); W32(0x1B00, 0);
		printf("Superbounce Viewer off: nothing of the viewer drawn (its master switch)\n");
	}
	/* Disable AI: in campaign the AI globals' "enabled" flag (+0, the AI update's pause switch) is held off, never
	 * "active" (+1, its bookkeeping - holding that off crashed in 4c); turned back off, the value it had is put back;
	 * in multiplayer nothing is touched */
	{
		enum { GOPT = 0x46000, AIG = 0x47000 };
		W32(0x1D10, GOPT); W32(GOPT + 8, 1); W32(0x1D20, AIG);
		MEM[AIG + 0] = 1; MEM[AIG + 1] = 1;
		host_set_disable_ai(1); host_apply_ai(); assert(MEM[AIG + 0] == 0 && MEM[AIG + 1] == 1);
		MEM[AIG + 0] = 1; host_apply_ai(); assert(MEM[AIG + 0] == 0);        /* the game (a script) turning it on: held off */
		host_set_disable_ai(0); host_apply_ai(); assert(MEM[AIG + 0] == 1 && MEM[AIG + 1] == 1);   /* what it was */
		MEM[AIG + 0] = 0; host_set_disable_ai(1); host_apply_ai(); host_set_disable_ai(0); host_apply_ai();
		assert(MEM[AIG + 0] == 0);                                          /* was off (a script's): stays off */
		W32(GOPT + 8, 2); MEM[AIG + 0] = 1;                                 /* multiplayer: untouched */
		host_set_disable_ai(1); host_apply_ai(); assert(MEM[AIG + 0] == 1);
		host_set_disable_ai(0); host_apply_ai();
		W32(0x1D10, 0); W32(0x1D20, 0);
		printf("disable AI: the AI update's pause switch (enabled) held off in campaign, put back as it was; active never touched\n");
	}
	/* the level's edges by area: the same seams found with the grid as with every edge - both test BSPs, on the ground
	 * and in the air, at several places */
	{
		static const float at[5][3] = { { 0, 0, 0 }, { 5, 3, 0 }, { 7, -2, 0 }, { 6.4f, 0.5f, 0 }, { -3, 4, 0 } };
		u32 bsps[2] = { 0x80000, 0x90000 };
		s32 bi, p, air, checks = 0;
		W32(0x1B04, 0);
		for (bi = 0; bi < 2; bi++)
			for (air = 0; air < 2; air++)
				for (p = 0; p < 5; p++)
				{
					s32 n[3], g;
					float sum[3];
					void host_force_grid_failed(s32 on);
					for (g = 0; g < 3; g++)
					{
						/* 0: every edge; 1: the grid; 2: the grid failed (as on the 360) - the near-edge list */
						host_set_level_grid(g >= 1); host_force_grid_failed(g == 2);
						W32(0x1B00, bsps[bi]);
						state(1600 + p, air ? -3.f : 0.f, air, 0);
						WF(OBJECT + 0x64, at[p][0]); WF(OBJECT + 0x68, at[p][1]); WF(OBJECT + 0x6C, at[p][2]);
						g_q = 0; frame();
						n[g] = host_seam_count(-1); sum[g] = host_seam_sum();
					}
					assert(n[0] == n[1] && sum[0] == sum[1] && n[0] == n[2] && sum[0] == sum[2]);
					host_force_grid_failed(0);
					checks++;
				}
		{
			/* the search spread over frames (as on a slow console: a step a frame, every 4th frame): after its first step the
			 * drawn seams are unchanged (never half-updated); after the third, the same as all at once */
			s32 host_drawn_n(void); float host_drawn_sum(void); void host_set_step_div(s32 d); u32 host_count_hud(void);
			s32 n_all, n_mid, n_end, f;
			float s_all, s_mid, s_end;
			W32(0x1B00, bsps[0]);
			state(1700, 0.f, 0, 0); WF(OBJECT + 0x64, at[0][0]); WF(OBJECT + 0x68, at[0][1]); WF(OBJECT + 0x6C, at[0][2]);
			frame(); n_all = host_drawn_n(); s_all = host_drawn_sum();                      /* all at once */
			host_set_step_div(4);
			host_set_step_div(6);
			for (f = 0; f < 12 && (host_count_hud() % 6) != 5; f++) frame();              /* to just before a cycle */
			W32(0x1B00, bsps[1]);                                                           /* another level: other seams (160, not 3) */
			frame(); n_mid = host_drawn_n(); s_mid = host_drawn_sum();                      /* step 1 (seams) only */
			frame(); assert(host_drawn_n() == n_mid && host_drawn_sum() == s_mid);          /* step 2 (landing): still the old */
			assert(n_mid == n_all && s_mid == s_all);                                       /* still the last finished: the old 3 */
			frame(); frame(); n_end = host_drawn_n(); s_end = host_drawn_sum();             /* steps 3 and 4 */
			assert(n_end != n_all && s_end != s_all);                                       /* now the new level's */
			host_set_step_div(0); frame();
			assert(host_drawn_n() == n_end && host_drawn_sum() == s_end);                   /* = all at once */
			{
				/* while slow, the nearest 64 seams, not 160: the same 64 nearest keys the full table has */
				void host_set_seam_cap(s32 c); s32 host_seam_keys(float *out, s32 n);
				float full[64], capped[64];
				s32 nf, nc, q;
				frame(); nf = host_seam_keys(full, 64);
				host_set_seam_cap(64); frame(); nc = host_seam_keys(capped, 64); host_set_seam_cap(160);
				assert(nf == 160 && nc == 64);
				for (q = 0; q < 64; q++) assert(capped[q] == full[q]);
			}
			checks++;
		}
		host_set_level_grid(1); W32(0x1B00, 0);
		printf("level edges by area: the same seams as every edge (%d checks, ground and air)\n", checks);
	}
	/* drawn only if reachable: standing level with the diagonal floor seam, it's drawn with a clear walk to it, hidden
	 * with a wall between */
	{
		const u32 C = 0xBC000, CP = C + 0x100, CS = C + 0x200, CE = C + 0x300, CV = C + 0x400;
		float cv[8][3] = { { 0, -2, -5 }, { 4, -2, -5 }, { 4, 2, -5 }, { 0, 2, -5 }, { -1, -3, -6 }, { -1, 3, -6 }, { -1, 3, -2 }, { -1, -3, -2 } };
		u16 ce[9][6] = { { 0, 1, 1, 0xffff, 0, 0xffff }, { 1, 2, 2, 0xffff, 0, 0xffff }, { 2, 0, 0, 3, 0, 1 },
			{ 2, 3, 4, 0xffff, 1, 0xffff }, { 3, 0, 2, 0xffff, 1, 0xffff },
			{ 4, 5, 6, 0xffff, 2, 0xffff }, { 5, 6, 7, 0xffff, 2, 0xffff }, { 6, 7, 8, 0xffff, 2, 0xffff }, { 7, 4, 5, 0xffff, 2, 0xffff } };
		int k2, j2, with, drawn[2];
		WF(CP, 0); WF(CP + 4, 0); WF(CP + 8, 1); WF(CP + 12, -5);
		WF(CP + 16, 1); WF(CP + 20, 0); WF(CP + 24, 0); WF(CP + 28, -1);
		W32(CS, 0u | (0u << 16)); W32(CS + 4, 0); W32(CS + 8, 0u | (2u << 16)); W32(CS + 12, 0); W32(CS + 16, 1u | (5u << 16)); W32(CS + 20, 0);
		for (k2 = 0; k2 < 9; k2++) for (j2 = 0; j2 < 6; j2 += 2) W32(CE + k2 * 12 + j2 * 2, (u32)ce[k2][j2] | ((u32)ce[k2][j2 + 1] << 16));
		for (k2 = 0; k2 < 8; k2++) { for (j2 = 0; j2 < 3; j2++) WF(CV + k2 * 16 + j2 * 4, cv[k2][j2]); W32(CV + k2 * 16 + 12, 0); }
		W32(C + 0x08, 2); W32(C + 0x0C, CP); W32(C + 0x2C, CS); W32(C + 0x34, CE); W32(C + 0x3C, CV);
		W32(0x1B04, 0);
		for (with = 0; with < 2; with++)
		{
			W32(C + 0x28, with ? 3 : 2); W32(C + 0x30, with ? 9 : 5); W32(C + 0x38, with ? 8 : 4);
			W32(0x1B00, C);
			state(1700 + with, 0.f, 0, 1);
			WF(OBJECT + 0x64, -2.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -5.f);
			WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			g_q = 0; frame();
			assert(host_seam_count(-1) == 1);
			drawn[with] = host_seams_drawn();
		}
		assert(drawn[0] == 0 && drawn[1] == 0);                             /* 6h.33: (6h.32 drew it dim when walkable: 1, 0) */
		W32(0x1B00, 0);
		printf("a seam level with you: no colour (nothing falls onto it from here), so not drawn - walkable or not\n");
	}
	/* in view: the rated diagonal 5 below you, the camera off to one side - drawn with a clear view; with a wall between
	 * the camera and the seam (your own paths to it still clear), hidden */
	{
		const u32 C = 0xBC000, CP = C + 0x100, CS = C + 0x200, CE = C + 0x300, CV = C + 0x400;
		float cv[8][3] = { { 0, -2, -5 }, { 4, -2, -5 }, { 4, 2, -5 }, { 0, 2, -5 }, { -1, -3, -6 }, { -1, 3, -6 }, { -1, 3, -2 }, { -1, -3, -2 } };
		u16 ce[9][6] = { { 0, 1, 1, 0xffff, 0, 0xffff }, { 1, 2, 2, 0xffff, 0, 0xffff }, { 2, 0, 0, 3, 0, 1 },
			{ 2, 3, 4, 0xffff, 1, 0xffff }, { 3, 0, 2, 0xffff, 1, 0xffff },
			{ 4, 5, 6, 0xffff, 2, 0xffff }, { 5, 6, 7, 0xffff, 2, 0xffff }, { 6, 7, 8, 0xffff, 2, 0xffff }, { 7, 4, 5, 0xffff, 2, 0xffff } };
		int k2, j2, with, drawn[2], rated[2];
		float camsave[3];
		WF(CP, 0); WF(CP + 4, 0); WF(CP + 8, 1); WF(CP + 12, -5);
		WF(CP + 16, 1); WF(CP + 20, 0); WF(CP + 24, 0); WF(CP + 28, -1);
		W32(CS, 0u | (0u << 16)); W32(CS + 4, 0); W32(CS + 8, 0u | (2u << 16)); W32(CS + 12, 0); W32(CS + 16, 1u | (5u << 16)); W32(CS + 20, 0);
		for (k2 = 0; k2 < 9; k2++) for (j2 = 0; j2 < 6; j2 += 2) W32(CE + k2 * 12 + j2 * 2, (u32)ce[k2][j2] | ((u32)ce[k2][j2 + 1] << 16));
		for (k2 = 0; k2 < 8; k2++) { for (j2 = 0; j2 < 3; j2++) WF(CV + k2 * 16 + j2 * 4, cv[k2][j2]); W32(CV + k2 * 16 + 12, 0); }
		W32(C + 0x08, 2); W32(C + 0x0C, CP); W32(C + 0x2C, CS); W32(C + 0x34, CE); W32(C + 0x3C, CV);
		for (k2 = 0; k2 < 3; k2++) memcpy(&camsave[k2], MEM + 0x1800 + 4 * k2, 4);
		W32(0x1B04, 0);
		for (with = 0; with < 2; with++)
		{
			W32(C + 0x28, with ? 3 : 2); W32(C + 0x30, with ? 9 : 5); W32(C + 0x38, with ? 8 : 4);
			W32(0x1B00, C);
			state(1800 + with, 0.f, 0, 1);
			WF(OBJECT + 0x64, 2.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);           /* above the seam */
			WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			WF(0x1800, -3.f); WF(0x1804, 0.f); WF(0x1808, -3.5f);                         /* the camera: behind x = -1 */
			g_q = 0; frame();
			rated[with] = host_rated_seams(); drawn[with] = host_seams_drawn();
		}
		assert(rated[0] == 1 && rated[1] == 1);                                 /* rated either way: your paths are clear */
		assert(drawn[0] == 1 && drawn[1] == 0);                                 /* in view only without the wall */
		for (k2 = 0; k2 < 3; k2++) memcpy(MEM + 0x1800 + 4 * k2, &camsave[k2], 4);
		W32(0x1B00, 0);
		printf("in view: the rated seam drawn with a clear view from the camera, hidden with a wall between (blockers %d)\n", host_blockers());
	}
	/* Invulnerability, as a switch: the game's cannot-take-damage (0x0080) and cannot-die (0x4000) bits of the damage
	 * flags (+0x10A) held set, other bits kept, health and shields never touched; off: the two bits as they were */
	{
		u32 o = OBJECT;
		state(2000, 0.f, 0, 0);
		W32(o + 0x108, 0x00020000u);                                         /* +0x10A flags: 0x0002 (another bit) */
		WF(o + 0xEC, 0.5f); WF(o + 0xF0, 0.25f);
		host_set_invuln(1); host_apply_invuln();
		assert((M32h(o + 0x108) >> 16) == 0x4082);                           /* both set, the other bit kept */
		W32(o + 0x108, 0x00020000u); host_apply_invuln();                     /* the game clearing them: held */
		assert((M32h(o + 0x108) >> 16) == 0x4082);
		assert(M32f(o + 0xEC) == 0.5f && M32f(o + 0xF0) == 0.25f);         /* health and shields untouched */
		host_set_invuln(0); host_apply_invuln();
		assert((M32h(o + 0x108) >> 16) == 0x0002);                           /* back as they were (clear) */
		W32(o + 0x108, 0x00820000u); host_set_invuln(1); host_apply_invuln(); host_set_invuln(0); host_apply_invuln();
		assert((M32h(o + 0x108) >> 16) == 0x0082);                           /* a script's cannot-take-damage kept */
		W32(o + 0x108, 0); WF(o + 0xEC, 0.f); WF(o + 0xF0, 0.f);
		printf("invulnerability: the game's cannot-take-damage and cannot-die flags held (others kept), health untouched, put back as they were\n");
	}
	/* third person: the view build gets a copy of the camera record moved behind the eye (Cartographer's placement), the
	 * view's first-person object / models cleared; a camera away from you (a cutscene), or off: the real record */
	{
		u32 cam = 0x48000, moved;                                              /* a camera record (0x358 bytes) */
		int k;
		memset(MEM + cam, 0, 0x358);
		state(2100, 0.f, 0, 0);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		WF(cam, 0.f); WF(cam + 4, 0.f); WF(cam + 8, 0.6f);                     /* the eye */
		WF(cam + 0x20, 1.f); WF(cam + 0x24, 0.f); WF(cam + 0x28, 0.f);        /* looking along +x */
		W32(0x1D30, 0x00050002u); W32(0x1D34, 2); W32(0x1D38, 0);             /* first-person object, 2 models; view of player 0 */
		MEM[OBJECT + 0x240] = 0xFF; MEM[OBJECT + 0x241] = 0xFF;               /* not zoomed */
		WF(cam + 0x2C, 0.f); WF(cam + 0x30, 0.f); WF(cam + 0x34, 1.f);        /* up */
		host_set_third_person(1, 10, 2, 0);
		moved = host_tp_camera(cam, 0);
		assert(moved != cam && M32f(moved) == -1.f && M32f(moved + 4) == 0.f && M32f(moved + 8) > 0.799f && M32f(moved + 8) < 0.801f);
		{
			/* aimed at what you aim at: nothing in the way, so 500 out from the eye - (500, 0, 0.6) - seen from (-1, 0, 0.8) */
			float fx = M32f(moved + 0x20), fy = M32f(moved + 0x24), fz = M32f(moved + 0x28), ux = M32f(moved + 0x2C), uz = M32f(moved + 0x34);
			float ex = 501.f / sqrtf(501.f * 501.f + 0.04f), ez = -0.2f / sqrtf(501.f * 501.f + 0.04f);
			assert(fabsf(fx - 1.f) < 1e-6f && fabsf(fy) < 1e-6f && fabsf(fz) < 1e-6f);   /* 6i.125: parallel to your view - nothing in the world moves it */
			assert(fabsf(fx * ux + fz * uz) < 1e-4f && uz > 0.99f);              /* up squared to it */
		}
		assert(M32f(cam + 0x20) == 1.f && M32f(cam) == 0.f);                   /* the real record untouched */
		host_tp_fp_after();
		assert(M32h(0x1D30) == 0xFFFFFFFFu && M32h(0x1D34) == 0);              /* body drawn, no first-person weapon */
		W32(0x1D30, 0x00050002u); W32(0x1D34, 2);
		MEM[OBJECT + 0x240] = 1; assert(host_tp_camera(cam, 0) == cam);       /* zoomed: first person */
		host_tp_fp_after(); assert(M32h(0x1D30) == 0x00050002u && M32h(0x1D34) == 2);
		MEM[OBJECT + 0x240] = 0xFF; MEM[OBJECT + 0x241] = 0;                  /* zooming in (desired): first person */
		assert(host_tp_camera(cam, 0) == cam);
		MEM[OBJECT + 0x241] = 0xFF;
		WF(cam, 10.f); assert(host_tp_camera(cam, 0) == cam);                 /* the camera away from you: left alone */
		host_tp_fp_after(); assert(M32h(0x1D30) == 0x00050002u && M32h(0x1D34) == 2);
		WF(cam, 0.f); host_set_third_person(0, 10, 2, 0);
		assert(host_tp_camera(cam, 0) == cam);                                 /* off */
		{
			/* step 6c: the free camera - started from your view (the eye (0, 0, 0.6), looking along +x), flown by the pad:
			 * full forward for 1 s at 3 u/s - eased, it's 2-3 units along +x; the right stick turns it (right: yaw down);
			 * the right trigger lifts it; with "Disable player movement" off the pad doesn't move it; off - your eye */
			void host_fc_input(float lx, float ly, float rx, float ry, float rt, float lt); void host_fc_pose(float *p, float *yaw, float *pitch);
			void host_set_free_cam(s32 on, s32 block); void spooky_main_frame(void);
			float fp[3], yaw, pitch, z0;
			int fr;
			unsigned char saved_cam[0x60];
			memcpy(saved_cam, MEM + 0x1800, 0x60);                               /* the player camera record: restored after */
			memset(MEM + 0x1800, 0, 0x60);
			WF(0x1800, 0.f); WF(0x1804, 0.f); WF(0x1808, 0.6f); WF(0x1820, 1.f); WF(0x182C, 0.f); WF(0x1834, 1.f);   /* the eye you had */
			host_set_free_cam(1, 1); host_fc_input(0, 0, 0, 0, 0, 0);
			spooky_main_frame();
			host_fc_pose(fp, &yaw, &pitch);
			assert(fp[0] == 0.f && fp[1] == 0.f && fabsf(fp[2] - 0.6f) < 1e-6f && fabsf(yaw) < 1e-6f && fabsf(pitch) < 1e-6f);
			moved = host_tp_camera(cam, 0);
			assert(moved != cam && fabsf(M32f(moved + 8) - 0.6f) < 1e-6f && fabsf(M32f(moved + 0x20) - 1.f) < 1e-6f && fabsf(M32f(moved + 0x34) - 1.f) < 1e-6f);
			/* 6i.179: the game's camera 6 units from your body - on foot that is not your eye (a cutscene): left alone; riding
			 * a vehicle it is the vehicle's own camera: the free camera all the same; a cutscene by the game's flag: left
			 * alone either way; 40 units off: left alone even in a vehicle */
			{
				float cx = M32f(cam); u32 par = M32h(OBJECT + 0x14), cgp = M32h(0x1A04); const u32 CGX = 0xE9F00;
				memset(MEM + CGX, 0, 0x20);
				WF(cam, cx + 6.f);
				W32(OBJECT + 0x14, 0xFFFFFFFFu); assert(host_tp_camera(cam, 0) == cam);
				W32(OBJECT + 0x14, 0x00120009u); moved = host_tp_camera(cam, 0); assert(moved != cam && fabsf(M32f(moved + 8) - 0.6f) < 1e-6f);
				W32(0x1A04, CGX); MEM[CGX + 5] = 1; assert(host_tp_camera(cam, 0) == cam);
				MEM[CGX + 5] = 0; assert(host_tp_camera(cam, 0) != cam);
				WF(cam, cx + 40.f); assert(host_tp_camera(cam, 0) == cam);
				WF(cam, cx); W32(OBJECT + 0x14, 0xFFFFFFFFu); MEM[CGX + 5] = 1; assert(host_tp_camera(cam, 0) == cam);   /* on foot at your eye, in a cutscene: left alone */
				/* 6i.185: a rider's own position is relative to what it rides - measured from the vehicle (the parent), which is
				 * where the camera is; from the rider's own numbers (here 500 units off) it would be refused */
				{
					const u32 VEH = 0xE9E00; u32 slot = M32h(OBJ_DATA + 9 * 0xC + 8), salt = M32h(OBJ_DATA + 9 * 0xC), cnt = M32h(OBJ_POOL + 0x20); float ox = M32f(OBJECT + 0x64);
					memset(MEM + VEH, 0, 0x80); W32(VEH + 0x14, 0xFFFFFFFFu); WF(VEH + 0x64, cx + 3.f); WF(VEH + 0x68, M32f(cam + 4)); WF(VEH + 0x6C, M32f(cam + 8));
					W32(OBJ_DATA + 9 * 0xC + 8, VEH); W32(OBJ_DATA + 9 * 0xC, 0x0012); if (cnt < 10) W32(OBJ_POOL + 0x20, 10); W32(OBJECT + 0x14, 0x00120009u); WF(OBJECT + 0x64, ox + 500.f); WF(cam, cx + 6.f); MEM[CGX + 5] = 0;
					moved = host_tp_camera(cam, 0); assert(moved != cam && fabsf(M32f(moved + 8) - 0.6f) < 1e-6f);
					WF(VEH + 0x64, cx + 60.f); assert(host_tp_camera(cam, 0) == cam);                       /* the vehicle itself far from the camera: a cutscene's - left alone */
					WF(OBJECT + 0x64, ox); W32(OBJ_DATA + 9 * 0xC + 8, slot); W32(OBJ_DATA + 9 * 0xC, salt); W32(OBJ_POOL + 0x20, cnt);
				}
				MEM[CGX + 5] = 0; W32(0x1A04, cgp); W32(OBJECT + 0x14, par); WF(cam, cx);
				moved = host_tp_camera(cam, 0); assert(moved != cam);
			}
			host_fc_input(0, 1.f, 0, 0, 0, 0);
			for (fr = 0; fr < 30; fr++) spooky_main_frame();
			host_fc_pose(fp, &yaw, &pitch);
			assert(fp[0] > 2.f && fp[0] < 3.f && fabsf(fp[1]) < 1e-4f && fabsf(fp[2] - 0.6f) < 1e-4f);
			moved = host_tp_camera(cam, 0);
			assert(fabsf(M32f(moved) - fp[0]) < 1e-6f);                        /* the view is where it's been flown */
			host_fc_input(0, 0, 1.f, 0, 0, 0);
			for (fr = 0; fr < 10; fr++) spooky_main_frame();
			host_fc_pose(fp, &yaw, &pitch);
			assert(yaw < -0.3f);                                                 /* turned right */
			z0 = fp[2]; host_fc_input(0, 0, 0, 0, 1.f, 0);
			for (fr = 0; fr < 30; fr++) spooky_main_frame();
			host_fc_pose(fp, &yaw, &pitch);
			assert(fp[2] > z0 + 2.f);                                           /* lifted */
			host_fc_input(0, 0, 0, 0, 0, 0); for (fr = 0; fr < 120; fr++) spooky_main_frame();
			host_fc_pose(fp, &yaw, &pitch); z0 = fp[0];
			host_set_free_cam(1, 0); host_fc_input(0, 1.f, 0, 0, 0, 0);
			for (fr = 0; fr < 30; fr++) spooky_main_frame();
			host_fc_pose(fp, &yaw, &pitch);
			assert(fabsf(fp[0] - z0) < 1e-3f);                                  /* player movement on: the camera stays put */
			{
				/* step 6c.1: your look inversion (the flying pad's port's preferences +0x1A); the pitch lock; no HUD in its
				 * view; teleport to the camera */
				void host_set_fc_lock(s32 on); void host_set_fc_pad(u32 h); void host_req(s32 r); s32 hud_skip(void);
				float host_fc_stick(s32 v); float host_fc_trigger(s32 v);
				/* step 6c.4: the player controls' dead zone - each axis 0 within +-9000, then stretched to full */
				assert(host_fc_stick(0) == 0.f && host_fc_stick(8500) == 0.f && host_fc_stick(9000) == 0.f && host_fc_stick(-9000) == 0.f && host_fc_stick(-8999) == 0.f);
				assert(host_fc_stick(9001) > 0.f && host_fc_stick(9001) < 0.001f);
				assert(fabsf(host_fc_stick(20883) - 0.5f) < 1e-4f && host_fc_stick(32767) == 1.f && host_fc_stick(-32768) == -1.f);
				assert(host_fc_trigger(32) == 0.f && host_fc_trigger(33) > 0.f && host_fc_trigger(255) == 1.f);
				float p0, py;
				host_set_free_cam(1, 1); host_set_fc_lock(1);
				W32(0x1D50, 0x111); W32(0x1D54, 0x222); W32(0x1D58, 0); W32(0x1D5C, 0);   /* ports 0 and 1 */
				host_set_fc_pad(0x222);                                            /* flown on port 1 */
				MEM[0x1D60 + 0x1C + 0x1A] = 0;
				host_fc_pose(fp, &yaw, &pitch); p0 = pitch;
				host_fc_input(0, 0, 0, 1.f, 0, 0); for (fr = 0; fr < 5; fr++) spooky_main_frame();
				host_fc_pose(fp, &yaw, &pitch); assert(pitch > p0);              /* not inverted: up looks up */
				host_fc_input(0, 0, 0, 0, 0, 0); for (fr = 0; fr < 60; fr++) spooky_main_frame();
				MEM[0x1D60 + 0x1C + 0x1A] = 1;
				host_fc_pose(fp, &yaw, &pitch); p0 = pitch;
				host_fc_input(0, 0, 0, 1.f, 0, 0); for (fr = 0; fr < 5; fr++) spooky_main_frame();
				host_fc_pose(fp, &yaw, &pitch); assert(pitch < p0);              /* inverted: up looks down */
				MEM[0x1D60 + 0x1C + 0x1A] = 0;
				host_fc_input(0, 0, 0, 1.f, 0, 0); for (fr = 0; fr < 90; fr++) spooky_main_frame();
				host_fc_pose(fp, &yaw, &pitch); assert(pitch <= 1.5501f && pitch > 1.4f);   /* locked: stops short of straight up */
				host_set_fc_lock(0); for (fr = 0; fr < 30; fr++) spooky_main_frame();
				host_fc_pose(fp, &yaw, &pitch); py = pitch; assert(py > 1.7f);    /* free: over the top */
				host_fc_input(0, 0, 0, 0, 0, 0); host_set_fc_lock(1); for (fr = 0; fr < 90; fr++) spooky_main_frame();
				host_fc_pose(fp, &yaw, &pitch); assert(pitch >= -1.5501f && pitch <= 1.5501f);   /* locked again: upright */
				host_set_fc_pad(0);
				moved = host_tp_camera(cam, 0); W32(0x1D38, 0);
				assert(moved != cam && hud_skip() == 1);                           /* its view: no HUD */
				host_req(7); spooky_main_frame();                                  /* teleport to the camera */
				host_fc_pose(fp, &yaw, &pitch);
				assert(RF(OBJECT + 0x64) == fp[0] && RF(OBJECT + 0x68) == fp[1] && RF(OBJECT + 0x6C) == fp[2]);
				host_set_free_cam(0, 1); spooky_main_frame(); host_tp_camera(cam, 0);
				assert(hud_skip() == 0);                                           /* off: the HUD */
				WF(OBJECT + 0x64, 1.f); host_req(7); spooky_main_frame(); assert(RF(OBJECT + 0x64) == 1.f);   /* off: no teleport */
				WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
				printf("free camera: look inversion from your port's settings; the pitch locked / over the top / upright again; no HUD in its view; teleport to the camera\n");
			}
			host_set_free_cam(0, 1); host_fc_input(0, 0, 0, 0, 0, 0); spooky_main_frame();
			assert(host_tp_camera(cam, 0) == cam);                              /* off: your eye */
			memcpy(MEM + 0x1800, saved_cam, 0x60);
			printf("free camera: started at your view; 1 s forward at 3 u/s -> %.2f units (eased); turns, lifts; held still with player movement on\n", z0);
		}
		for (k = 0; k < 3; k++) W32(0x1D30 + 4 * k, 0);
		printf("third person: the camera moved behind the eye (-1, 0, 0.8) and aimed at your aim point, first-person cleared; zoomed, away from you or off: untouched\n");
	}
	/* third person aimed at a wall: the wall BSP (floor 5 down, a wall at x = -1); standing at x = -3, eye 0.6 up, aiming
	 * along +x - the aim point is on the wall at x = -1, and the moved camera (x = -4, 0.2 higher) looks at it */
	{
		const u32 C = 0xBC000, cam = 0x48000;
		u32 moved;
		float fx, fz, ex, ez, dx = 4.f, dz = -0.2f, l = sqrtf(4.f * 4.f + 0.04f);   /* the wall 2 ahead: aimed 3 ahead (the least) */
		W32(C + 0x28, 3); W32(C + 0x30, 9); W32(C + 0x38, 8);                /* the floor and the wall */
		W32(0x1B04, 0); W32(0x1B00, C);
		state(2200, 0.f, 0, 0);
		WF(OBJECT + 0x64, -3.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -4.1f);
		WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
		g_q = 0; frame();                                                      /* the surfaces near you, built */
		memset(MEM + cam, 0, 0x358);
		WF(cam, -3.f); WF(cam + 4, 0.f); WF(cam + 8, -3.5f);
		WF(cam + 0x20, 1.f); WF(cam + 0x2C + 8, 1.f);
		MEM[OBJECT + 0x240] = 0xFF; MEM[OBJECT + 0x241] = 0xFF;
		host_set_third_person(1, 10, 2, 0);
		moved = host_tp_camera(cam, 0);
		fx = M32f(moved + 0x20); fz = M32f(moved + 0x28); ex = dx / l; ez = dz / l;
		assert(moved != cam && fabsf(M32f(moved) + 4.f) < 1e-4f && fabsf(fx - 1.f) < 1e-6f && fabsf(fz) < 1e-6f);   /* 6i.125: the wall 2 ahead doesn't bump it */
		{
			/* eased in 1 / distance: from 500 out to the wall (3), a third of the way a frame - 1 / (1/500 + (1/3 - 1/500) x 0.35) */
			float aim, gx, gz, lz;
			host_set_third_person(0, 10, 2, 0); host_tp_camera(cam, 0);
			host_set_third_person(1, 10, 2, 0);
			WF(cam + 0x20, -1.f); host_tp_camera(cam, 0);                         /* looking away from the wall: 500 out */
			WF(cam + 0x20, 1.f); moved = host_tp_camera(cam, 0);                  /* turned to it: eased */
			aim = 1.f / (1.f / 500.f + (1.f / 3.f - 1.f / 500.f) * 0.35f); gx = aim + 1.f; gz = -0.2f; lz = sqrtf(gx * gx + gz * gz);
			(void)aim; (void)gx; (void)gz; (void)lz;
			assert(fabsf(M32f(moved + 0x20) - 1.f) < 1e-6f && fabsf(M32f(moved + 0x28)) < 1e-6f);   /* 6i.125: turned to the wall: your view, exactly - no easing to a wall point */
		}
		{
			/* 6h.9: the level's collision BSP walked along your aim. One node: the plane x = -53 facing -x - its back (x above
			 * -53) open, its front solid. Standing at x = -3 (eye -3.5 up) looking along -x with the camera 1 unit to your right
			 * (+y), 1 back, 0.2 up - at (-2, 1, -3.3): the wall 50 ahead is far past the surfaces near you, and the camera looks
			 * at the point on it your aim meets, (-53, 0, -3.5) - the reticle where your shots go */
			float host_bsp_ray(const float *p0, const float *p1);
			const u32 CP = C + 0x100, N3 = C + 0x600;
			float q0[3] = { -3.f, 0.f, -3.5f }, q1[3] = { -103.f, 0.f, -3.5f }, q2[3] = { -3.f, 0.f, -3.5f }, q3[3] = { 0.f, 0.f, -3.5f }, tt;
			float gx, gy, gz, lz;
			WF(CP + 32, -1.f); WF(CP + 36, 0.f); WF(CP + 40, 0.f); WF(CP + 44, 53.f);
			W32(N3, 2u); W32(N3 + 4, 0xFFFFFF80u);                                      /* plane 2; back 0x800000 (leaf 0), front -1 */
			W32(C + 0x08, 3); W32(C + 0x00, 1); W32(C + 0x04, N3);
			tt = host_bsp_ray(q0, q1);
			assert(fabsf(tt - 0.5f) < 1e-5f);                                          /* -3 to -103: the wall at -53, half way */
			assert(host_bsp_ray(q2, q3) == 2.f);                                       /* toward +x: open all the way */
			host_set_third_person(0, 10, 2, 10); host_tp_camera(cam, 0);
			host_set_third_person(1, 10, 2, 10);
			WF(cam + 0x20, -1.f); moved = host_tp_camera(cam, 0);
			assert(fabsf(M32f(moved) + 2.f) < 1e-4f && fabsf(M32f(moved + 4) - 1.f) < 1e-4f && fabsf(M32f(moved + 8) + 3.3f) < 1e-4f);
			gx = -1.f; gy = 0.f; gz = 0.f; lz = 1.f;
			assert(fabsf(M32f(moved + 0x20) - gx / lz) < 1e-6f && fabsf(M32f(moved + 0x24) - gy / lz) < 1e-6f && fabsf(M32f(moved + 0x28) - gz / lz) < 1e-6f);   /* 6i.125: parallel, the wall 50 ahead ignored */
			W32(N3 + 4, 0x800000FFu);                                                   /* (front a leaf too: nothing solid) */
			assert(host_bsp_ray(q0, q1) == 2.f);
			W32(C + 0x00, 0); W32(C + 0x04, 0); W32(C + 0x08, 2);
			host_set_third_person(1, 10, 2, 0); WF(cam + 0x20, 1.f);
			printf("third person: the level's BSP walked along your aim - a wall 50 ahead, the camera 1 to the side looks at the point your shots hit\n");
		}
		{
			/* 6h.11: "Ignore Collision" off - a solid wall half a unit behind your eye (x below -3.5): the camera (set 1 back,
			 * 0.2 up) pulled in along the line from your eye to a quarter unit short of it - 0.5 - 0.25 / |(1, 0, 0.2)| of
			 * the way; the wall gone, eased back out a sixth of the way a frame. On (the default): through the wall. */
			void host_set_tp_nocoll(s32 on);
			const u32 CP = C + 0x100, N3 = C + 0x600;
			float len = sqrtf(1.04f), k0 = 0.5f - 0.25f / len, k1 = k0 + (1.f - k0) / 6.f;
			WF(CP + 32, -1.f); WF(CP + 36, 0.f); WF(CP + 40, 0.f); WF(CP + 44, 3.5f);
			W32(N3, 2u); W32(N3 + 4, 0xFFFFFF80u);
			W32(C + 0x08, 3); W32(C + 0x00, 1); W32(C + 0x04, N3);
			WF(cam + 0x20, 1.f);
			host_set_third_person(0, 10, 2, 0); host_tp_camera(cam, 0);
			host_set_third_person(1, 10, 2, 0); host_set_tp_nocoll(0);
			moved = host_tp_camera(cam, 0);
			assert(fabsf(M32f(moved) - (-3.f - k0)) < 1e-4f && fabsf(M32f(moved + 8) - (-3.5f + 0.2f * k0)) < 1e-4f);
			W32(N3 + 4, 0x800000FFu);                                                   /* the wall gone */
			moved = host_tp_camera(cam, 0);
			assert(fabsf(M32f(moved) - (-3.f - k1)) < 1e-4f);
			host_set_tp_nocoll(1); W32(N3 + 4, 0xFFFFFF80u);
			moved = host_tp_camera(cam, 0);
			assert(fabsf(M32f(moved) + 4.f) < 1e-4f);                                 /* ignored: 1 back, in the wall */
			W32(C + 0x00, 0); W32(C + 0x04, 0); W32(C + 0x08, 2);
			printf("third person collision: pulled in a quarter unit short of a wall behind you, eased back out; ignored by default\n");
		}
		{
			/* 6h.11: a possessed vehicle (your unit a vehicle, in no seat), third person on: the director told to use its following
			 * camera (answer 1, word 1) and Spooky Ghost's camera stays out of it; seated, a biped, third person off, or the
			 * game already following: the game's answer */
			s32 cam_mode_answer(s32 game, u32 unit, u32 out);
			const u32 WORD = 0x7F400;
			unsigned char t0 = MEM[OBJECT + 0xAA]; u32 par = M32h(OBJECT + 0x14);
			MEM[OBJECT + 0xAA] = 1; W32(OBJECT + 0x14, 0xFFFFFFFFu); W32(WORD, 0);
			host_set_third_person(1, 10, 2, 0);
			assert(cam_mode_answer(0, UNIT, WORD) == 1 && (M32h(WORD) & 0xffff) == 1);
			assert(host_tp_camera(cam, 0) == cam);                                     /* the game's own view of it */
			W32(WORD, 0); host_set_third_person(0, 10, 2, 0);
			assert(cam_mode_answer(0, UNIT, WORD) == 0 && M32h(WORD) == 0);              /* third person off */
			host_set_third_person(1, 10, 2, 0);
			W32(OBJECT + 0x14, 0x00090005u); assert(cam_mode_answer(0, UNIT, WORD) == 0);    /* in a seat */
			W32(OBJECT + 0x14, 0xFFFFFFFFu); MEM[OBJECT + 0xAA] = 0; assert(cam_mode_answer(2, UNIT, WORD) == 2);   /* a biped */
			MEM[OBJECT + 0xAA] = 1; assert(cam_mode_answer(1, UNIT, WORD) == 1);         /* already following */
			MEM[OBJECT + 0xAA] = t0; W32(OBJECT + 0x14, par);
			assert(host_tp_camera(cam, 0) != cam);                                     /* a biped again: Spooky Ghost's camera */
			printf("possessed vehicle in third person: the game's following camera (the vehicle's own view); a biped: Spooky Ghost's\n");
		}
		{
			/* effects: the player's weapon (held by the player's unit, its parent) gets "no" first-person marker while the
			 * view is moved; the game answers when third person is off */
			s32 fpm_answer(u32 object_index);
			const u32 W = 0x4B000, WD = 0x00070004u;
			memset(MEM + W, 0, 0x260);
			W32(OBJ_DATA + 4 * 12, 0x0007); W32(OBJ_DATA + 4 * 12 + 8, W);
			W32(W + 0x14, UNIT); W32(OBJECT + 0x14, 0xFFFFFFFFu);
			assert(fpm_answer(WD) == 0);
			host_set_third_person(0, 10, 2, 0); host_tp_camera(cam, 0);
			assert(fpm_answer(WD) == 2);
			W32(OBJ_DATA + 4 * 12, 0); W32(OBJ_DATA + 4 * 12 + 8, 0); W32(OBJECT + 0x14, 0);
		}
		{
			/* the perspective: "following" (3) for the player seen in third person; else the game's (-1) */
			s32 persp_answer(s32 user);
			host_set_third_person(1, 10, 2, 0); host_tp_camera(cam, 0);
			assert(persp_answer(0) == 3 && persp_answer(1) == -1 && persp_answer(-1) == -1);
			MEM[OBJECT + 0x240] = 1; host_tp_camera(cam, 0);                    /* zoomed: first person again */
			assert(persp_answer(0) == -1);
			MEM[OBJECT + 0x240] = 0xFF; host_tp_camera(cam, 0); assert(persp_answer(0) == 3);
			host_set_third_person(0, 10, 2, 0); host_tp_camera(cam, 0);
			assert(persp_answer(0) == -1);
		}
		{
			/* step 6c.2: the free camera's view (third person off) the same as third person's - no first-person markers for
			 * your weapon, "following" (6c.1: neither - its particle effects drawn at the screen) */
			void host_set_free_cam(s32 on, s32 block); void spooky_main_frame(void); s32 fpm_answer(u32 object_index); s32 persp_answer(s32 user);
			const u32 W = 0x4B000, WD = 0x00070004u;
			memset(MEM + W, 0, 0x260);
			W32(OBJ_DATA + 4 * 12, 0x0007); W32(OBJ_DATA + 4 * 12 + 8, W);
			W32(W + 0x14, UNIT); W32(OBJECT + 0x14, 0xFFFFFFFFu);
			host_set_third_person(0, 10, 2, 0); host_set_free_cam(1, 1); spooky_main_frame();
			assert(host_tp_camera(cam, 0) != cam);
			assert(fpm_answer(WD) == 0 && persp_answer(0) == 3);
			host_set_free_cam(0, 1); spooky_main_frame(); host_tp_camera(cam, 0);
			assert(fpm_answer(WD) == 2 && persp_answer(0) == -1);                /* off: the game's own */
			W32(OBJ_DATA + 4 * 12, 0); W32(OBJ_DATA + 4 * 12 + 8, 0); W32(OBJECT + 0x14, 0);
			printf("free camera: no first-person effects for your weapon, perspective \"following\" - as third person\n");
		}
		W32(0x1B00, 0);
		{
			/* 6i.123: no shake - the camera's height follows the eye exactly, as your body is drawn: a bump of the eye (0.1 up)
			 * moves it 0.1 (6h - 6i.122 eased it a quarter, which shook the body against it on slopes and seams); a steady
			 * fall (vertical speed -6 u/s, the eye 0.2 lower a frame) is followed exactly */
			u32 c2 = 0x48000, m;
			float z0, z1;
			memset(MEM + c2, 0, 0x358);
			WF(c2, -3.f); WF(c2 + 4, 0.f); WF(c2 + 8, -3.5f); WF(c2 + 0x20, 1.f); WF(c2 + 0x34, 1.f);
			WF(OBJECT + 0x64, -3.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -4.1f); WF(OBJECT + 0x90, 0.f);
			host_set_third_person(0, 10, 2, 0); host_tp_camera(c2, 0);
			host_set_third_person(1, 10, 2, 0);
			m = host_tp_camera(c2, 0); z0 = M32f(m + 8);
			WF(c2 + 8, -3.4f); m = host_tp_camera(c2, 0); z1 = M32f(m + 8);          /* the bump */
			assert(z1 - z0 > 0.099f && z1 - z0 < 0.101f);
			WF(c2 + 8, -3.5f); host_tp_camera(c2, 0); host_tp_camera(c2, 0); host_tp_camera(c2, 0); host_tp_camera(c2, 0);
			host_tp_camera(c2, 0); host_tp_camera(c2, 0); host_tp_camera(c2, 0); host_tp_camera(c2, 0); host_tp_camera(c2, 0);
			m = host_tp_camera(c2, 0); z0 = M32f(m + 8);                             /* settled */
			WF(OBJECT + 0x90, -6.f);
			WF(c2 + 8, -3.7f); WF(OBJECT + 0x6C, -4.3f); m = host_tp_camera(c2, 0); z1 = M32f(m + 8);   /* falling */
			assert(z0 - z1 > 0.199f && z0 - z1 < 0.201f);
			WF(OBJECT + 0x90, 0.f); host_set_third_person(0, 10, 2, 0); host_tp_camera(c2, 0);
		}
		printf("third person aimed at a wall 2 ahead (3, the least); eased from 30 to it a fifth a frame; the perspective reported as following\n");
	}
	/* Infinite Ammo (a call hook): on, a weapon in a player's inventory -> yes (1); an AI's, one not in an inventory, or
	 * off -> the game's own answer (2) */
	{
		const u32 W = 0x4B000, WD = 0x00070004u;
		memset(MEM + W, 0, 0x260);
		W32(OBJ_DATA + 4 * 12, 0x0007); W32(OBJ_DATA + 4 * 12 + 8, W);
		W32(W + 0x154, UNIT); MEM[W + 0x12C] = 1; W32(OBJECT + 0x13C, 0x00010000u);
		host_set_ammo(1, 0);
		assert(ia_answer(WD) == 1);
		W32(OBJECT + 0x13C, 0xFFFFFFFFu); assert(ia_answer(WD) == 2);         /* an AI's */
		W32(OBJECT + 0x13C, 0x00010000u); MEM[W + 0x12C] = 0; assert(ia_answer(WD) == 2);   /* not in an inventory */
		MEM[W + 0x12C] = 1; assert(ia_answer(0x00090004u) == 2);              /* a stale index */
		host_set_ammo(0, 0); assert(ia_answer(WD) == 2);                       /* off */
		W32(OBJ_DATA + 4 * 12, 0); W32(OBJ_DATA + 4 * 12 + 8, 0); W32(OBJECT + 0x13C, 0);
		printf("infinite ammo: yes for a player's weapon while on; an AI's, not carried, stale or off: the game answers\n");
	}
	/* Bottomless Clip: a player's weapon's loaded rounds and battery put back after its shot; an AI's weapon's not */
	{
		const u32 W = 0x4B000, WD = 0x00070004u;
		float age = 0.5f;
		memset(MEM + W, 0, 0x260);
		W32(OBJ_DATA + 4 * 12, 0x0007); W32(OBJ_DATA + 4 * 12 + 8, W);          /* the weapon, slot 4 */
		W32(W + 0x154, UNIT); W32(OBJECT + 0x13C, 0x00010000u);                 /* carried by the player's unit */
		W32(W + 0x224 + 8, 30); W32(W + 0x234 + 8, 12); WF(W + 0x180, age); WF(W + 0x184, 0.25f);
		host_set_ammo(0, 1);
		bc_before(WD);
		W32(W + 0x224 + 8, 29); W32(W + 0x234 + 8, 11); WF(W + 0x180, 0.6f); WF(W + 0x184, 0.26f);    /* the shot: a round, heat, and battery */
		bc_after();
		assert((M32h(W + 0x224 + 8) & 0xffff) == 30 && (M32h(W + 0x234 + 8) & 0xffff) == 12 && M32f(W + 0x180) == age && M32f(W + 0x184) == 0.26f);   /* 6i.168: rounds and heat put back (no overheat); the battery spent, as the game has it */
		W32(OBJECT + 0x13C, 0xFFFFFFFFu);                                      /* an AI's unit */
		bc_before(WD);
		W32(W + 0x224 + 8, 29); bc_after();
		assert((M32h(W + 0x224 + 8) & 0xffff) == 29);
		/* 6i.166 / 6i.167: Infinite Ammo - a carried weapon's battery (+0x184, its age; +0x180 is its heat and is left alone) neither drains nor refills: noted when first seen, put back
		 * each tick and before each draw whatever changed it (no shot involved); a dropped weapon forgotten and noted
		 * afresh when picked up; an AI's weapon and anything that is not a weapon untouched; off: nothing held */
		{
			void host_ia_hold(s32 scan); s32 host_ia_n(void);
			W32(OBJECT + 0x13C, 0x00010000u); W32(W + 0x224 + 8, 30); WF(W + 0x184, age);
			MEM[W + 0xAA] = 2; MEM[W + 0x12C] = 1;
			host_set_ammo(1, 0);
			bc_before(WD); W32(W + 0x224 + 8, 29); WF(W + 0x184, 0.55f); bc_after();           /* the shot's wrap is Bottomless Clip's alone again: nothing put back by it */
			assert((M32h(W + 0x224 + 8) & 0xffff) == 29 && M32f(W + 0x184) == 0.55f);
			WF(W + 0x184, age);
			host_ia_hold(1); assert(host_ia_n() == 1 && M32f(W + 0x184) == age);              /* a tick: noted at 0.5 */
			WF(W + 0x184, 0.9f); host_ia_hold(1); assert(M32f(W + 0x184) == age);              /* drained (a sword's hit, a shot - anything): put back */
			WF(W + 0x184, 0.1f); host_ia_hold(1); assert(M32f(W + 0x184) == age);              /* refilled: put back too */
			WF(W + 0x184, 0.9f); host_ia_hold(0); assert(M32f(W + 0x184) == age);              /* before a draw: put back, the table not gone through */
			assert((M32h(W + 0x224 + 8) & 0xffff) == 29);                                     /* the rounds: not its business */
			MEM[W + 0x12C] = 0; WF(W + 0x184, 0.7f); host_ia_hold(1);                          /* dropped: forgotten, left as it is */
			assert(host_ia_n() == 0 && M32f(W + 0x184) == 0.7f);
			host_ia_hold(0); assert(M32f(W + 0x184) == 0.7f);
			MEM[W + 0x12C] = 1; host_ia_hold(1); WF(W + 0x184, 0.2f); host_ia_hold(1);         /* picked up again: noted at what it has now */
			assert(host_ia_n() == 1 && M32f(W + 0x184) == 0.7f);
			W32(OBJECT + 0x13C, 0xFFFFFFFFu); host_ia_hold(1); WF(W + 0x184, 0.3f); host_ia_hold(1);   /* an AI's: not held */
			assert(host_ia_n() == 0 && M32f(W + 0x184) == 0.3f);
			W32(OBJECT + 0x13C, 0x00010000u); MEM[W + 0xAA] = 3; host_ia_hold(1); WF(W + 0x184, 0.4f); host_ia_hold(1);   /* not a weapon: not held */
			assert(host_ia_n() == 0 && M32f(W + 0x184) == 0.4f);
			MEM[W + 0xAA] = 2; host_ia_hold(1); assert(host_ia_n() == 1);
			WF(W + 0x180, 0.33f); WF(W + 0x184, 0.9f); host_ia_hold(1); assert(M32f(W + 0x180) == 0.33f);   /* the heat: not held */
			host_set_ammo(0, 0); WF(W + 0x184, 0.8f); host_ia_hold(1); host_ia_hold(0);        /* off: forgotten, nothing held */
			assert(host_ia_n() == 0 && M32f(W + 0x184) == 0.8f);
			MEM[W + 0xAA] = 0; MEM[W + 0x12C] = 0;
		}
		host_set_ammo(0, 0); W32(OBJ_DATA + 4 * 12, 0); W32(OBJ_DATA + 4 * 12 + 8, 0); W32(OBJECT + 0x13C, 0);
		printf("bottomless clip: a player's weapon's rounds and heat put back after its shot (its battery drains); an AI's weapon untouched\n");
	}
	/* 6i.165: dead (no unit) in a campaign game - Revert still works, Create does not (and says why); no game: neither */
	{
		u32 unit0 = M32h(PL_DATA + 0x2C), opt0 = M32h(0x1D10);
		const char *host_toast(void); void host_clear_toast(void);
		W32(0x1D10, 0x47000); W32(0x47000 + 8, 1); W32(PL_DATA + 0x2C, 0xFFFFFFFFu);
		MEM[0x1D00] = 0; MEM[0x1D00 - 1] = 0; host_clear_toast();
		host_practice(2); assert(MEM[0x1D00 - 1] == 1 && MEM[0x1D00] == 0 && strstr(host_toast(), "Reverted to the last checkpoint"));
		MEM[0x1D00 - 1] = 0;
		host_practice(1); assert(MEM[0x1D00] == 0 && MEM[0x1D00 - 1] == 0 && strstr(host_toast(), "Checkpoint: not while you are dead"));
		W32(0x47000 + 8, 3); host_practice(2); assert(MEM[0x1D00 - 1] == 0 && strstr(host_toast(), "Checkpoints: no game in progress"));
		W32(0x47000 + 8, 1); W32(PL_DATA + 0x2C, unit0);
		host_practice(1); assert(MEM[0x1D00] == 1 && strstr(host_toast(), "Checkpoint created"));      /* alive: as ever */
		MEM[0x1D00] = 0; W32(0x1D10, opt0); W32(0x47000 + 8, 0); host_clear_toast();
		printf("checkpoints: dead in a campaign game - Revert works, Create refused; alive - both; no game - neither\n");
	}
	/* 6i.170: One Shot Kills - the damage event's "kill" flag (bit 2 at +4) set for the one call when its owner is a
	 * player and the object hit is not a player's unit, not the owner and not what the owner rides; taken off after;
	 * a flag the game set itself left alone; off: nothing; nested calls each undone in turn */
	{
		void ohk_before(u32 data, u32 object_index); void ohk_after(void); void host_set_ohk(s32 on);
		const u32 DM = 0xE9000, DM2 = 0xE9100, TG = 0xE9200, VH = 0xE9400;
		memset(MEM + DM, 0, 0x200); memset(MEM + TG, 0, 0x400);
		W32(OBJ_DATA + 8 * 12, 0x0011); W32(OBJ_DATA + 8 * 12 + 8, TG); W32(TG + 0x13C, 0xFFFFFFFFu); W32(TG + 0x14, 0xFFFFFFFFu);   /* an enemy: 0x00110008 */
		W32(OBJ_DATA + 9 * 12, 0x0012); W32(OBJ_DATA + 9 * 12 + 8, VH); W32(VH + 0x13C, 0xFFFFFFFFu); W32(VH + 0x14, 0xFFFFFFFFu);   /* a vehicle: 0x00120009 */
		W32(DM + 4, 0x40); W32(DM + 8, 0x00010000u); W32(DM + 0xC, UNIT);                          /* your damage: owner player 0, your unit */
		{ u32 par = M32h(OBJECT + 0x14), pl = M32h(OBJECT + 0x13C);
		W32(OBJECT + 0x13C, 0x00010000u); W32(OBJECT + 0x14, 0xFFFFFFFFu);
		ohk_before(DM, 0x00110008u); assert(M32h(DM + 4) == 0x40); ohk_after();                    /* off: nothing */
		host_set_ohk(1);
		ohk_before(DM, 0x00110008u); assert(M32h(DM + 4) == 0x44);                                 /* an enemy: this damage kills */
		ohk_after(); assert(M32h(DM + 4) == 0x40);                                                 /* ... and the event is as it was for whatever it reaches next */
		ohk_before(DM, UNIT); assert(M32h(DM + 4) == 0x40); ohk_after();                           /* yourself (your own grenade): never */
		W32(TG + 0x13C, 0x00020001u); ohk_before(DM, 0x00110008u); assert(M32h(DM + 4) == 0x40); ohk_after(); W32(TG + 0x13C, 0xFFFFFFFFu);   /* another player's unit: never */
		W32(OBJECT + 0x14, 0x00120009u); ohk_before(DM, 0x00120009u); assert(M32h(DM + 4) == 0x40); ohk_after();   /* what you ride: never */
		W32(OBJECT + 0x14, 0xFFFFFFFFu); ohk_before(DM, 0x00120009u); assert(M32h(DM + 4) == 0x44); ohk_after();   /* an empty vehicle you shoot: it dies of it */
		W32(DM + 8, 0xFFFFFFFFu); ohk_before(DM, 0x00110008u); assert(M32h(DM + 4) == 0x40); ohk_after(); W32(DM + 8, 0x00010000u);   /* not a player's damage: as the game has it */
		W32(DM + 4, 0x44); ohk_before(DM, 0x00110008u); ohk_after(); assert(M32h(DM + 4) == 0x44); W32(DM + 4, 0x40);   /* the game's own kill flag: not taken off */
		memcpy(MEM + DM2, MEM + DM, 0x100);
		ohk_before(DM, 0x00110008u); ohk_before(DM2, 0x00110008u); ohk_before(DM, UNIT);            /* nested: an enemy, an enemy by another event, yourself */
		assert(M32h(DM + 4) == 0x44 && M32h(DM2 + 4) == 0x44);
		ohk_after(); assert(M32h(DM + 4) == 0x44 && M32h(DM2 + 4) == 0x44);
		ohk_after(); assert(M32h(DM + 4) == 0x44 && M32h(DM2 + 4) == 0x40);
		ohk_after(); assert(M32h(DM + 4) == 0x40);
		{ int q; for (q = 0; q < 12; q++) ohk_before(DM, 0x00110008u); for (q = 0; q < 12; q++) ohk_after(); assert(M32h(DM + 4) == 0x40); ohk_after(); }   /* deeper than the stack holds, and one "after" too many: nothing left set, nothing broken */
		host_set_ohk(0); W32(OBJECT + 0x14, par); W32(OBJECT + 0x13C, pl); }
		W32(OBJ_DATA + 8 * 12, 0); W32(OBJ_DATA + 8 * 12 + 8, 0); W32(OBJ_DATA + 9 * 12, 0); W32(OBJ_DATA + 9 * 12 + 8, 0);
		printf("One Shot Kills: a player's damage kills what it reaches - not a player, not the owner or its ride; the event restored after each call\n");
	}
	/* 6i.171: what counts as a pointer follows the console's memory - up to 64 MB on a 64 MB console, up to 128 MB on
	 * one that reports more than 96 MB (the 360's emulator): the game's own data sits up there on it */
	{
		void host_mem_size_note(void); s32 host_ptr_ok(u32 p); extern u32 g_host_mem_pages; u32 pages_was = g_host_mem_pages;
		g_host_mem_pages = 16384; host_mem_size_note();
		assert(host_ptr_ok(0x83FFFFFCu) && !host_ptr_ok(0x84000000u) && !host_ptr_ok(0x87D80000u) && !host_ptr_ok(0xD0001000u) && !host_ptr_ok(0x80000002u) && !host_ptr_ok(0x8000u));
		g_host_mem_pages = 32768; host_mem_size_note(); g_host_mem_pages = pages_was;
		assert(host_ptr_ok(0x83FFFFFCu) && host_ptr_ok(0x84000000u) && host_ptr_ok(0x87D80000u) && !host_ptr_ok(0x88000000u) && !host_ptr_ok(0xD0001000u));
		printf("pointers: up to 64 MB on a 64 MB console, up to 128 MB on a 128 MB one\n");
	}
	/* 6i.172: the Field of View's own arctangent and tangent (plain arithmetic) against the library's */
	{
		float host_fov_atan(float t); float host_fov_tan(float x); float t; double worst = 0, worst_t = 0;
		for (t = 0.f; t < 12.f; t += 0.0137f) { double e = fabs((double)host_fov_atan(t) - atan((double)t)); if (e > worst) worst = e; }
		for (t = 0.05f; t < 1.45f; t += 0.0091f) { double e = fabs((double)host_fov_tan(t) - tan((double)t)) / tan((double)t); if (e > worst_t) worst_t = e; }
		assert(worst < 3.0e-5 && worst_t < 1.0e-4 && host_fov_atan(0.f) == 0.f && fabs(host_fov_atan(1.f) - 0.78539816f) < 1.0e-5f);
		{ float fp_tan_host(float x); double w3 = 0; float tt;                                    /* 6i.185: the first-person body's own tangent (no test in it), over the angles it is given */
		  for (tt = 0.05f; tt < 0.9f; tt += 0.0037f) { double e = fabs((double)fp_tan_host(tt) - tan((double)tt)) / tan((double)tt); if (e > w3) w3 = e; }
		  assert(w3 < 2.0e-6 && fabsf(fp_tan_host(0.3925f * 1.9198622f) / fp_tan_host(0.3925f * 0.9599311f) - (float)(tan(0.3925 * 1.9198622) / tan(0.3925 * 0.9599311))) < 1.0e-5f);
		  printf("first-person body's tangent: within %.8f (relative) of the library's over 0.05..0.9; the factor at 110 from 55: %.4f\n", w3, (double)(fp_tan_host(0.3925f * 1.9198622f) / fp_tan_host(0.3925f * 0.9599311f))); }
		{ float ftanf_host(float x); double w2 = 0;                                                /* 6i.174: every tangent is this one - the whole range, both signs, near pi/2 and beyond it */
		  for (t = -1.56f; t < 1.56f; t += 0.00317f) { double e = fabs((double)ftanf_host(t) - tan((double)t)) / (1.0 + fabs(tan((double)t))); if (e > w2) w2 = e; }
		  assert(w2 < 2.0e-5 && ftanf_host(0.f) == 0.f && fabs(ftanf_host(0.7853982f) - 1.f) < 1.0e-5f && fabs(ftanf_host(2.5f) - (float)tan(2.5)) < 1.0e-4f && fabs(ftanf_host(-4.0f) - (float)tan(-4.0)) < 1.0e-4f);
		  printf("tangent (all uses): within %.7f (relative) of the library's over -1.56..1.56\n", w2); }
		printf("field of view: arctangent within %.6f rad, tangent within %.5f %% of the library's\n", worst, worst_t * 100.0);
	}
	/* 6i.178: the Havok Island Viewer - the world's active and inactive islands and their bodies' boxes out of the
	 * broadphase (the fixed island not walked); only what is near the camera; each island's box the union of its bodies';
	 * a body whose broadphase box does not point back at it skipped; hidden behind the level (the tests' wall), HIV_LOS
	 * answers a tick; a merge and a split flash the island for 15 ticks; the caps; off: nothing */
	{
		void host_set_hiv(s32 on); void host_hiv_tick(void); void host_hiv_draw(void); s32 host_hiv(s32 what, s32 i); void host_hiv_box(s32 isl, s32 i, float *six);
		void host_hiv_cam(float x, float y, float z); void host_hiv_wall(s32 on, float x);
		enum { HW = 0xEA000, HBP = 0xEA200, HACT = 0xEA400, HINA = 0xEA500, HISL = 0xEA600, HENT_LIST = 0xEB000, HEL = 0xEC000, HAX = 0xED000, HENT = 0xF0000 };   /* (the three axes: 0xED000, 0xEE000, 0xEF000) */
		u32 world_was = M32h(0x1014);
		float six[6];
		int q, nbody = 0;
		#define ISL(k) (HISL + 0x80u * (u32)(k))
		#define ENT(k) (HENT + 0x40u * (u32)(k))
		/* a body k with this box: its handle's number k, element k owned by its handle, its six ends at endpoints 2k / 2k+1 of each axis */
		#define BODY(k, x0, y0, z0, x1, y1, z1) do { float lo_[3] = { x0, y0, z0 }, hi_[3] = { x1, y1, z1 }; int a_; \
			static const int lw[3] = { 8, 0, 2 }, hw[3] = { 10, 4, 6 }; \
			W32(ENT(k) + 0x1C, (u32)(k)); W32(HEL + 16u * (u32)(k) + 12, ENT(k) + 0x1C); \
			for (a_ = 0; a_ < 3; a_++) { u16 vl = (u16)((lo_[a_] + 100.f) * 10.f), vh = (u16)((hi_[a_] + 100.f) * 10.f); \
				memcpy(MEM + HAX + 0x1000u * (u32)a_ + 4u * (2u * (u32)(k)), &vl, 2); memcpy(MEM + HAX + 0x1000u * (u32)a_ + 4u * (2u * (u32)(k) + 1u), &vh, 2); \
				{ u16 s0 = (u16)(2 * (k)), s1 = (u16)(2 * (k) + 1); memcpy(MEM + HEL + 16u * (u32)(k) + lw[a_], &s0, 2); memcpy(MEM + HEL + 16u * (u32)(k) + hw[a_], &s1, 2); } } } while (0)
		memset(MEM + HW, 0, 0xA000);
		W32(HW + 0x08, HACT); W32(HW + 0x14, HINA); W32(HW + 0xC4, HBP);
		for (q = 0; q < 3; q++) { WF(HBP + 0x10 + 4 * q, 100.f); WF(HBP + 0x20 + 4 * q, 100.f); WF(HBP + 0x30 + 4 * q, 10.f); }
		W32(HBP + 0x40, HEL); W32(HBP + 0x44, 240); W32(HBP + 0x4C, HAX); W32(HBP + 0x58, HAX + 0x1000); W32(HBP + 0x64, HAX + 0x2000);
		/* island 0 (awake): bodies 0, 1; island 1 (at rest): body 2; island 2 (at rest), 300 away: body 3 */
		BODY(0, 1.f, 1.f, 0.f, 2.f, 2.f, 1.f); BODY(1, 4.f, -1.f, 0.5f, 5.f, 0.f, 2.f); BODY(2, 10.f, 3.f, 0.f, 11.f, 4.f, 1.f); BODY(3, 300.f, 0.f, 0.f, 301.f, 1.f, 1.f);
		W32(HACT, ISL(0)); W32(HW + 0x0C, 1); W32(HINA, ISL(1)); W32(HINA + 4, ISL(2)); W32(HW + 0x18, 2);
		W32(ISL(0) + 0x3C, HENT_LIST); W32(ISL(0) + 0x40, 2); W32(HENT_LIST, ENT(0)); W32(HENT_LIST + 4, ENT(1));
		W32(ISL(1) + 0x3C, HENT_LIST + 0x100); W32(ISL(1) + 0x40, 1); W32(HENT_LIST + 0x100, ENT(2));
		W32(ISL(2) + 0x3C, HENT_LIST + 0x200); W32(ISL(2) + 0x40, 1); W32(HENT_LIST + 0x200, ENT(3));
		W32(0x1014, HW); host_hiv_cam(0.f, 0.f, 1.f); host_hiv_wall(1, 1000.f);
		host_set_hiv(0); host_hiv_tick(); assert(host_hiv(0, 0) == 0 && host_hiv(1, 0) == 0);        /* off: nothing */
		host_set_hiv(1); host_hiv_tick();
		assert(host_hiv(0, 0) == 3 && host_hiv(1, 0) == 2 && host_hiv(8, 0) == 4 && host_hiv(7, 0) == 0);   /* three bodies and two islands near; four bodies seen in all, every box read */
		host_hiv_box(0, 0, six); assert(fabsf(six[0] - 1.f) < 0.11f && fabsf(six[1] - 1.f) < 0.11f && fabsf(six[2]) < 0.11f && fabsf(six[3] - 2.f) < 0.11f && fabsf(six[4] - 2.f) < 0.11f && fabsf(six[5] - 1.f) < 0.11f);   /* body 0's box, to the broadphase's step */
		host_hiv_box(0, 1, six); assert(fabsf(six[0] - 4.f) < 0.11f && fabsf(six[1] + 1.f) < 0.11f && fabsf(six[2] - 0.5f) < 0.11f && fabsf(six[5] - 2.f) < 0.11f);
		host_hiv_box(1, 0, six); assert(fabsf(six[0] - 1.f) < 0.11f && fabsf(six[1] + 1.f) < 0.11f && fabsf(six[2]) < 0.11f && fabsf(six[3] - 5.f) < 0.11f && fabsf(six[4] - 2.f) < 0.11f && fabsf(six[5] - 2.f) < 0.11f);   /* island 0: the box around both */
		assert(host_hiv(3, 0) == 1 && host_hiv(5, 0) == 2 && host_hiv(3, 1) == 0 && host_hiv(5, 1) == 1);     /* awake with 2; at rest with 1 */
		assert(host_hiv(2, 0) && host_hiv(2, 1) && host_hiv(2, 2) && host_hiv(6, 0) && host_hiv(6, 1) && !host_hiv(4, 0));   /* nothing in the way: all drawn; no flash */
		host_hiv_draw();
		/* 6i.186: each box keeps where it was the tick before (drawn between the two, as the world is); one new this tick was
		 * where it is */
		{
			void host_hiv_was(s32 isl, s32 i, float *six); float was[6];
			host_hiv_was(0, 0, was); assert(fabsf(was[0] - 1.f) < 0.11f && fabsf(was[3] - 2.f) < 0.11f);       /* first seen: where it is */
			BODY(0, 1.5f, 1.f, 0.f, 2.5f, 2.f, 1.f); host_hiv_tick();
			host_hiv_box(0, 0, six); host_hiv_was(0, 0, was);
			assert(fabsf(six[0] - 1.5f) < 0.11f && fabsf(six[3] - 2.5f) < 0.11f && fabsf(was[0] - 1.f) < 0.11f && fabsf(was[3] - 2.f) < 0.11f);
			host_hiv_box(1, 0, six); host_hiv_was(1, 0, was);
			assert(fabsf(six[0] - 1.5f) < 0.11f && fabsf(was[0] - 1.f) < 0.11f && fabsf(was[3] - 5.f) < 0.11f);   /* its island's box too */
			host_hiv_draw();
			BODY(0, 1.f, 1.f, 0.f, 2.f, 2.f, 1.f); host_hiv_tick(); host_hiv_tick();
		}
		host_hiv_wall(1, 8.f); host_hiv_tick();                                                          /* a wall at x = 8: body 2 and its island behind it */
		assert(host_hiv(2, 0) && host_hiv(2, 1) && !host_hiv(2, 2) && host_hiv(6, 0) && !host_hiv(6, 1));
		host_hiv_wall(1, 3.f); host_hiv_tick();                                                          /* at x = 3: body 1 too - its island still drawn for body 0 */
		assert(host_hiv(2, 0) && !host_hiv(2, 1) && host_hiv(6, 0));
		host_hiv_wall(1, 1000.f); host_hiv_tick();
		W32(ENT(1) + 0x1C, 0); host_hiv_tick();                                                          /* body 1's handle names body 0's box: not its own - skipped */
		assert(host_hiv(0, 0) == 2 && host_hiv(7, 0) == 1 && host_hiv(5, 0) == 2);
		W32(ENT(1) + 0x1C, 1); host_hiv_tick(); assert(host_hiv(0, 0) == 3);
		/* a merge: body 2 joins island 0 (island 1 gone) - island 0 flashes for 5 ticks */
		W32(HENT_LIST + 8, ENT(2)); W32(ISL(0) + 0x40, 3); W32(HINA, ISL(2)); W32(HW + 0x18, 1);
		host_hiv_tick(); assert(host_hiv(1, 0) == 1 && host_hiv(5, 0) == 3 && host_hiv(4, 0) == 1);
		for (q = 0; q < 3; q++) { host_hiv_tick(); assert(host_hiv(4, 0) == 1); }
		host_hiv_tick(); host_hiv_tick(); assert(host_hiv(4, 0) == 0);   /* 6i.179: 5 ticks (was 15) */
		/* a split: body 2 leaves into island 1 again - the island it is in now flashes; island 0 does not */
		W32(ISL(0) + 0x40, 2); W32(HINA, ISL(1)); W32(HINA + 4, ISL(2)); W32(HW + 0x18, 2);
		host_hiv_tick(); assert(host_hiv(1, 0) == 2 && host_hiv(4, 1) == 1 && host_hiv(4, 0) == 0);
		for (q = 0; q < 20; q++) host_hiv_tick();
		assert(host_hiv(4, 1) == 0);
		/* many: 200 bodies near in one island - HIV_BODIES listed, and their answers come HIV_LOS a tick */
		for (q = 4; q < 204; q++) { BODY(q, 1.f + 0.05f * (float)q, 5.f, 0.f, 1.5f + 0.05f * (float)q, 6.f, 1.f); W32(HENT_LIST + 0x400 + 4u * (u32)(q - 4), ENT(q)); }
		W32(ISL(3) + 0x3C, HENT_LIST + 0x400); W32(ISL(3) + 0x40, 200); W32(HACT, ISL(3)); W32(HW + 0x0C, 1); W32(HW + 0x18, 0);
		host_hiv_tick();
		assert(host_hiv(0, 0) == 96 && host_hiv(1, 0) == 1 && host_hiv(5, 0) == 200 && host_hiv(8, 0) == 200);
		for (q = 0, nbody = 0; q < 96; q++) nbody += host_hiv(2, q);
		assert(nbody == 16);                                                                             /* the first tick: 16 answers */
		for (q = 0; q < 5; q++) host_hiv_tick();
		for (q = 0, nbody = 0; q < 96; q++) nbody += host_hiv(2, q);
		assert(nbody == 96);                                                                             /* six ticks: all of them */
		host_hiv_draw();
		W32(0x1014, 0); host_hiv_tick(); assert(host_hiv(0, 0) == 0);                                    /* no world: nothing, nothing read */
		host_set_hiv(0); host_hiv_tick(); host_hiv_wall(0, 0.f); W32(0x1014, world_was);
		#undef ISL
		#undef ENT
		#undef BODY
		printf("Havok Island Viewer: bodies' boxes from the broadphase, islands awake / at rest, behind the level hidden, merges and splits flash, 96 bodies and 16 sight lines a tick at most\n");
	}
	/* 6i.191: Larger Caches - the geometry cache's own block: only with the setting on and a console of more than 96 MB;
	 * one 16 MB block, taken once and kept; none to be had: the game's own (said once); the setting off again: given back */
	{
		void host_set_big_cache(s32 on); void host_set_mem_mb(u32 mb); u32 host_get_mem_mb(void); u32 host_cache_geo(void); u32 cache_geo_take(u32 game_block, u32 game_size);
		extern u32 cache_geo_pages, cache_geo_bytes, g_host_mem_big;
		u32 mb_was = host_get_mem_mb(), big_was = g_host_mem_big;
		host_set_big_cache(0); host_set_mem_mb(127); g_host_mem_big = 0x4000000u;
		assert(cache_geo_take(0x80C00000u, 0x680000u) == 0 && !host_cache_geo());                       /* off */
		host_set_big_cache(1); host_set_mem_mb(63);
		assert(cache_geo_take(0x80C00000u, 0x680000u) == 0 && !host_cache_geo());                       /* a 64 MB console: never */
		{ s32 host_get_big_cache(void); assert(host_get_big_cache() == 0); }                             /* 6i.192: ... and the setting itself is put off there (a saved "on") */
		host_set_big_cache(1); host_set_mem_mb(127); g_host_mem_big = 0x800000u;
		assert(cache_geo_take(0x80C00000u, 0x680000u) == 0 && !host_cache_geo() && logged("caches: no 16 MB block for the geometry cache") == 1);
		assert(cache_geo_take(0x80C00000u, 0x680000u) == 0 && logged("caches: no 16 MB block for the geometry cache") == 1);   /* said once */
		g_host_mem_big = 0x4000000u;
		assert(cache_geo_take(0x80C00000u, 0x680000u) == 0x100000u && host_cache_geo() == 0x100000u && cache_geo_pages == 0x1000u && cache_geo_bytes == 0x1000000u);
		assert(logged("caches: the geometry cache is in the mod's block at 0x00100000, 16384 KB (the game had taken 6656 KB at 0x80C00000"));
		g_host_mem_big = 0u; assert(cache_geo_take(0x80D00000u, 0x700000u) == 0x100000u);                 /* the next level: the same block, nothing new taken */
		host_set_big_cache(0);
		assert(cache_geo_take(0x80C00000u, 0x680000u) == 0 && !host_cache_geo() && logged("caches: Larger Caches off - the geometry cache's block given back"));
		/* 6i.194: the animation cache the same way - its own 4 MB block, the game's size kept; not for a request larger
		 * than the block; given back when off */
		{
			u32 cache_anim_take(u32 game_block, u32 game_size); u32 host_cache_anim(void);
			host_set_big_cache(0); host_set_mem_mb(127); g_host_mem_big = 0x4000000u;
			assert(cache_anim_take(0x80A00000u, 0x300000u) == 0 && !host_cache_anim());
			host_set_big_cache(1);
			assert(cache_anim_take(0x80A00000u, 0x500000u) == 0 && !host_cache_anim());                    /* more than the block holds: the game's own */
			g_host_mem_big = 0x200000u;
			assert(cache_anim_take(0x80A00000u, 0x300000u) == 0 && logged("caches: no 4 MB block for the animation cache") == 1);
			g_host_mem_big = 0x4000000u;
			assert(cache_anim_take(0x80A00000u, 0x300000u) == 0x100000u && host_cache_anim() == 0x100000u);
			assert(logged("caches: the animation cache is in the mod's block at 0x00100000 (the game had taken 3072 KB at 0x80A00000"));
			assert(cache_anim_take(0x80B00000u, 0x400000u) == 0x100000u);                                    /* the next level, the larger size: the same block */
			host_set_mem_mb(63); assert(cache_anim_take(0x80A00000u, 0x300000u) == 0 && !host_cache_anim());   /* (a 64 MB console: the setting put off, the block back) */
			host_set_big_cache(0);
		}
		host_set_mem_mb(mb_was); g_host_mem_big = big_was;
		printf("Larger Caches: the geometry cache's own 16 MB block - on and above 96 MB only, taken once, the game's own when there is none, given back when off\n");
	}
	/* 6i.193: a level's screen effect above 100 % - while the larger targets stand in, the draw is handed a copy of its
	 * grid with the corners scaled from the camera's rectangle to the larger target (the texture's 0..1 untouched), and the
	 * rectangle is the larger size for the length of the call; then put back. Not in use, an odd rectangle, too large a
	 * grid: the caller's grid, nothing touched */
	{
		u32 se_before(u32 grid, u32 columns, u32 rows); void se_after(void); void host_bg_state(s32 on, u32 w, u32 h); float host_se_grid(s32 i, s32 k);
		const u32 RECT = 0x7EA30, G = 0xE9A00; u32 r0 = M32h(RECT), r1 = M32h(RECT + 4), got; int i;
		static const float g4[4][4] = { { 0, 0, 0, 0 }, { 640, 0, 1, 0 }, { 0, 480, 0, 1 }, { 640, 480, 1, 1 } };
		for (i = 0; i < 16; i++) WF(G + 4 * (u32)i, g4[i / 4][i % 4]);
		W32(RECT, 0); W32(RECT + 4, 480u | (640u << 16));
		host_bg_state(0, 832, 624);
		assert(se_before(G, 2, 2) == G && M32h(RECT + 4) == (480u | (640u << 16))); se_after(); assert(M32h(RECT + 4) == (480u | (640u << 16)));   /* not in use */
		host_bg_state(1, 832, 624);
		got = se_before(G, 2, 2);
		assert(got != G && M32h(RECT) == 0 && M32h(RECT + 4) == (624u | (832u << 16)));
		assert(host_se_grid(0, 0) == 0.f && host_se_grid(0, 1) == 0.f && fabsf(host_se_grid(1, 0) - 832.f) < 1e-3f && host_se_grid(1, 1) == 0.f && host_se_grid(1, 2) == 1.f && host_se_grid(1, 3) == 0.f);
		assert(host_se_grid(2, 0) == 0.f && fabsf(host_se_grid(2, 1) - 624.f) < 1e-3f && host_se_grid(2, 3) == 1.f && fabsf(host_se_grid(3, 0) - 832.f) < 1e-3f && fabsf(host_se_grid(3, 1) - 624.f) < 1e-3f && host_se_grid(3, 2) == 1.f && host_se_grid(3, 3) == 1.f);
		assert(M32f(G + 16) == 640.f);                                                                  /* the caller's own grid untouched */
		se_after(); assert(M32h(RECT) == 0 && M32h(RECT + 4) == (480u | (640u << 16)));                  /* the rectangle back */
		se_after(); assert(M32h(RECT + 4) == (480u | (640u << 16)));                                    /* (a second time: nothing) */
		W32(RECT, 10u | (20u << 16)); assert(se_before(G, 2, 2) == G && M32h(RECT) == (10u | (20u << 16))); se_after();   /* not from 0, 0: left */
		W32(RECT, 0);
		assert(se_before(G, 5, 5) == G && M32h(RECT + 4) == (480u | (640u << 16))); se_after();        /* more points than the copy holds: left */
		assert(se_before(0, 2, 2) == 0); se_after();
		assert(logged("render scale: a level's screen effect drawn at the larger size (832 x 624)") == 1);
		host_bg_state(0, 0, 0); W32(RECT, r0); W32(RECT + 4, r1);
		printf("a level's screen effect above 100%%: its grid's corners and the camera's rectangle at the larger targets' size for that draw only\n");
	}
	/* 6i.195: the Field of View and the first-person arms and weapon - after the first-person set-up, the arms' model (0)
	 * and each weapon's are widened about the camera's line of sight by the legs' factor; the body's model (the unit's
	 * own, after the first) is left; a slot with no sane matrix is left; nothing when the legs' fix did not run */
	{
		void host_fp_models(void); s32 host_fp_models_done(void); void host_fp_state(s32 user, float s, s32 why);
		const u32 ARR = 0xEB000, MB = 0xD0C, CAM = 0x1DEC; int i, k;
		u32 cnt0 = M32h(0x1D34), obj0 = M32h(0x1D30), usr0 = M32h(0x1D38); float cam0[6];
		for (k = 0; k < 6; k++) cam0[k] = M32f(CAM + 4 * (u32)k);
		memset(MEM + ARR, 0, 4 * MB);
		WF(CAM, 10.f); WF(CAM + 4, 20.f); WF(CAM + 8, 5.f); WF(CAM + 0xC, 1.f); WF(CAM + 0x10, 0.f); WF(CAM + 0x14, 0.f);   /* the camera at (10, 20, 5) looking along +x */
		W32(0x1D38, 0); W32(0x1D30, 0xE1230007u); W32(0x1D34, 3);
		for (i = 0; i < 3; i++)                                                 /* 0 the arms (the unit's), 1 a weapon, 2 the body (the unit's) */
		{
			u32 m = ARR + (u32)i * MB, nd = m + 12;
			W32(m, 0x00050005u + (u32)i); W32(m + 4, i == 1 ? 0xE2220009u : 0xE1230007u);
			WF(nd, 1.f); WF(nd + 4, 1.f); WF(nd + 0x14, 1.f); WF(nd + 0x24, 1.f);            /* scale 1; forward x, left y, up z */
			WF(nd + 0x28, 10.5f); WF(nd + 0x2C, 20.2f); WF(nd + 0x30, 4.9f);                 /* half a unit ahead, 0.2 left, 0.1 down */
		}
		host_fp_state(0, 1.5f, 0);
		host_fp_models();
		assert(host_fp_models_done() == 2);
		for (i = 0; i < 2; i++)                                                 /* the arms and the weapon: across the view x 1.5, along it unchanged */
		{
			u32 nd = ARR + (u32)i * MB + 12;
			assert(fabsf(M32f(nd + 0x28) - 10.5f) < 1e-5f && fabsf(M32f(nd + 0x2C) - 20.3f) < 1e-5f && fabsf(M32f(nd + 0x30) - 4.85f) < 1e-5f);
			assert(fabsf(M32f(nd + 4) - 1.f) < 1e-6f && fabsf(M32f(nd + 0x14) - 1.5f) < 1e-6f && fabsf(M32f(nd + 0x24) - 1.5f) < 1e-6f);
			assert(M32f(nd + 0x34) == 0.f && M32f(nd + 0x34 + 0x2C) == 0.f);     /* the next slot (no matrix in it): untouched */
		}
		{ u32 nd = ARR + 2 * MB + 12; assert(M32f(nd + 0x2C) == 20.2f && M32f(nd + 0x14) == 1.f); }   /* the body: left */
		host_fp_state(0, 1.5f, 2); WF(ARR + 12 + 0x2C, 20.2f); host_fp_models(); assert(host_fp_models_done() == 0 && M32f(ARR + 12 + 0x2C) == 20.2f);   /* the legs' fix did not run: nothing */
		host_fp_state(0, 1.f, 0); host_fp_models(); assert(host_fp_models_done() == 0);                 /* the game's own view: nothing */
		host_fp_state(0, 1.5f, 0); W32(0x1D34, 0); host_fp_models(); assert(host_fp_models_done() == 0); /* no models drawn */
		W32(0x1D34, 9); host_fp_models(); assert(host_fp_models_done() == 0);                           /* a count that cannot be: nothing */
		host_fp_state(0, 1.f, 0);
		W32(0x1D34, cnt0); W32(0x1D30, obj0); W32(0x1D38, usr0); for (k = 0; k < 6; k++) WF(CAM + 4 * (u32)k, cam0[k]);
		printf("Field of View: the first-person arms' and weapons' models widened as the legs are (the body's model left; only when the legs' fix ran)\n");
	}
	/* 6i.198: the patchy fog above 100 % - while the larger targets stand in, the pattern's scale (the game's 0.5) is
	 * 320 / the larger width for the length of the fog's draw, and 0.5 again after; not in use, at 640 wide, or a value
	 * that is not the game's 0.5: left alone */
	{
		void fog_before(void); void fog_after(void); void host_bg_state(s32 on, u32 w, u32 h);
		const u32 FS = 0xE9F80;
		WF(FS, 0.5f);
		host_bg_state(0, 1280, 960); fog_before(); assert(M32f(FS) == 0.5f); fog_after(); assert(M32f(FS) == 0.5f);   /* not in use */
		host_bg_state(1, 1280, 960); fog_before(); assert(M32f(FS) == 0.25f); fog_after(); assert(M32f(FS) == 0.5f);   /* 200 %: a quarter */
		host_bg_state(1, 832, 624); fog_before(); assert(fabsf(M32f(FS) - 320.f / 832.f) < 1e-6f); fog_after(); assert(M32f(FS) == 0.5f);
		fog_after(); assert(M32f(FS) == 0.5f);                                                          /* a second time: nothing */
		host_bg_state(1, 640, 480); fog_before(); assert(M32f(FS) == 0.5f); fog_after();                 /* the game's own size */
		WF(FS, 0.75f); host_bg_state(1, 1280, 960); fog_before(); assert(M32f(FS) == 0.75f); fog_after(); assert(M32f(FS) == 0.75f);   /* not the game's 0.5: left */
		assert(logged("render scale: the patchy fog's pattern scaled for the larger picture (1280 x 960)") == 1);
		host_bg_state(0, 0, 0); WF(FS, 0.f);
		printf("the patchy fog above 100%%: its pattern's scale is 320 / the larger width for the length of its draw, the game's 0.5 after\n");
	}
	/* 6i.209: with Performance Mode on, a cutscene plays as if VSync were "30 fps" (the game's own pacing, one frame a
	 * tick) - the setting itself untouched, and yours again when it ends; not with Performance Mode off; a VSync of
	 * "30 fps" needs nothing; said in the log at each end */
	{
		s32 host_vsync_now(void); s32 host_cut30(void); void host_set_vsync(s32 on, s32 cap); void host_set_unlock_fps(s32 on);
		const u32 CGP = 0x1A04, CG = 0xE9FA0, GOF = 0x7D800; u32 cg0 = M32h(CGP), gop0 = M32h(0x1D10), go8 = M32h(GOF + 8); int was5 = MEM[CG + 5];
		W32(CGP, CG); MEM[CG + 5] = 0; W32(0x1D10, GOF); W32(GOF + 8, 1);
		host_set_unlock_fps(1); host_set_vsync(1, 0);
		assert(host_vsync_now() == 1 && !host_cut30());                                          /* in play: your setting */
		MEM[CG + 5] = 1; assert(host_vsync_now() == 2 && host_cut30() && logged("cutscene: held to 30 fps while it plays (Performance Mode)") == 1);
		assert(host_vsync_now() == 2 && logged("cutscene: held to 30 fps while it plays") == 1);   /* said once */
		host_set_vsync(0, 6); assert(host_vsync_now() == 2);                                     /* VSync off with an FPS Limit: 30 all the same */
		MEM[CG + 5] = 0; assert(host_vsync_now() == 0 && !host_cut30() && logged("cutscene over: your VSync setting again") == 1);
		MEM[CG + 5] = 1; host_set_unlock_fps(0); assert(host_vsync_now() == 0 && !host_cut30());   /* Performance Mode off: nothing */
		host_set_unlock_fps(1); host_set_vsync(2, 0); assert(host_vsync_now() == 2 && !host_cut30());   /* already "30 fps": nothing to do */
		MEM[CG + 5] = (unsigned char)was5; W32(CGP, cg0); W32(GOF + 8, go8); W32(0x1D10, gop0); host_set_vsync(1, 0); host_set_unlock_fps(0); (void)host_vsync_now();
		printf("cutscenes: held to 30 fps while Performance Mode is on; your VSync setting again when they end\n");
	}
	/* 6i.222 / 6i.223: the flicker after the intro video on the 360 - the loading code's last "keep alive" swap with the
	 * video over is followed by the same frame drawn and swapped three more times, and said; with the video still open
	 * nothing is asked for; once a check */
	{
		void swap_tail_check(void); void swap_settle(void); u32 host_settle_calls(u32 *swaps); u32 sw = 0, dr;
		memset(MEM + 0x1E00, 0, 0x20);
		MEM[0x1E00] = 1; W32(0x1E00 + 0xC, 0x12345);                         /* a video open */
		swap_tail_check(); swap_settle(); dr = host_settle_calls(&sw); assert(dr == 0 && sw == 0);
		MEM[0x1E00 + 4] = 1; W32(0x1E00 + 0xC, 0);                           /* ... over */
		swap_tail_check(); swap_settle();
		dr = host_settle_calls(&sw); assert(dr == 3 && sw == 3 && logged("after the video: the same frame drawn and swapped 3 more times") == 1);
		swap_settle(); assert(host_settle_calls(&sw) == 3);                  /* nothing more without another check */
		memset(MEM + 0x1E00, 0, 0x20);
		printf("after the intro video: the last frame drawn and swapped three more times\n");
	}
	/* Infinite Ammo includes grenades: the player's two counts held at 4; off: left alone */
	{
		void host_apply_grenades(void);
		MEM[OBJECT + 0x23E] = 1; MEM[OBJECT + 0x23F] = 0;
		host_set_ammo(1, 0); host_apply_grenades();
		assert(MEM[OBJECT + 0x23E] == 4 && MEM[OBJECT + 0x23F] == 4);
		MEM[OBJECT + 0x23E] = 2; host_set_ammo(0, 0); host_apply_grenades();
		assert(MEM[OBJECT + 0x23E] == 2);
		MEM[OBJECT + 0x23E] = 0; MEM[OBJECT + 0x23F] = 0;
		printf("infinite ammo: the player's grenades held at 4 each; off: left alone\n");
	}
	/* the landing seam keeps its own rating's colour (the verdict is on the circle and arrow): found either way round;
	 * not among the seams -> -1 (drawn white) */
	{
		s32 host_landing_seam_q(const float *a, const float *b);
		void host_put_seam(s32 i, const float *a, const float *b, s32 q);
		float a[3] = { 1, 2, -3 }, b[3] = { 4, 2, -3 }, c[3] = { 9, 9, 9 };
		host_put_seam(0, a, b, 2);
		assert(host_landing_seam_q(a, b) == 2 && host_landing_seam_q(b, a) == 2 && host_landing_seam_q(a, c) == -1);
		host_put_seam(0, a, b, 1); assert(host_landing_seam_q(b, a) == 1);
		printf("landing seam: drawn in its own rating's colour (found either way round), white if not among the seams\n");
	}
	/* the ghost pill (Ghost Viewer stage 1): the player's body's pair with the level; its capsule (getType read from its
	 * code: "mov eax, 7; ret"); pink at S 0, red at -3, lime on a rewind (the previous swept centre jumping back) and
	 * for 8 ticks after; off: nothing */
	{
		void host_set_ghost(s32 on);
		s32 host_ghost_state(void);
		s32 host_ghost_rewind(float *from, float *to);
		void host_ghost_cap(float *a, float *b, float *r);
		enum { VT = 0x4C000, FN = 0x1F800, MS = 0x4D000, MO = 0x4E000 };
		static const u8 get7[6] = { 0xB8, 0x07, 0x00, 0x00, 0x00, 0xC3 };
		float a[3], b[3], r, from[3], to[3];
		int k, t;
		W32(SHAPE, VT); W32(VT + 0x14, FN); memcpy(MEM + FN, get7, 6);
		WF(SHAPE + 0x0C, 0.2f); WF(SHAPE + 0x10, 0.f); WF(SHAPE + 0x14, 0.f); WF(SHAPE + 0x18, 0.2f);
		WF(SHAPE + 0x20, 0.f); WF(SHAPE + 0x24, 0.f); WF(SHAPE + 0x28, 0.5f);
		W32(HANDLE - 8, MS); memset(MEM + MS, 0, 0x60);
		WF(MS + 0x20, 1.f); WF(MS + 0x34, 1.f); WF(MS + 0x48, 1.f);                    /* identity, at the origin */
		W32(ENTITY + 0x3C, MO); memset(MEM + MO, 0, 0x80); WF(MO + 0x7C, 1.f);
		W32(0x50020 + 0x10, 0x51000);                                                   /* the partner: the level */
		W32(AGENT, 0x4F100); W32(AGENT + 0x10, 0);                                     /* its agent: the welder (no children yet) */
		W32(0x1B00, 0); W32(0x1B04, 0);
		host_set_ghost(1);
		state(3000, 0.f, 0, 0); WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f); frame();
		assert(host_ghost_state() == 0);
		host_ghost_cap(a, b, &r);
		assert(r == 0.2f && a[2] == 0.2f && b[2] == 0.5f && a[0] == 0.f);
		assert(logged("ghost pill: found - radius x1000 200, core length x1000 300") == 1);
		state(3001, 0.f, 0, 1); frame(); assert(host_ghost_state() == 2);              /* stuck: red */
		state(3002, 0.f, 0, 0); WF(MO + 0x60, 0.f); WF(MO + 0x70, 1.f); frame();         /* current at x = 1 */
		assert(host_ghost_state() == 0);
		state(3003, 0.f, 0, 0); WF(MO + 0x60, 0.5f); WF(MO + 0x70, 0.5f); frame();       /* previous now x = 0.5: rewound */
		assert(host_ghost_state() == 1 && host_ghost_rewind(from, to) && from[0] == 1.f && to[0] == 0.5f);
		assert(logged("ghost rewind: tick 3003, x1000 distance 500 from 1000 0 0 to 500 0 0, S 0") == 1);
		for (t = 3004; t <= 3011; t++) { state((u32)t, 0.f, 0, 0); frame(); }          /* the hold: 8 ticks */
		assert(host_ghost_state() == 1);
		state(3012, 0.f, 0, 0); frame(); assert(host_ghost_state() == 0);              /* over */
		{
			/* the TIMs: the pair agent's children - a GSK leaf skipping on the floor triangle 5 below with TIM 6 (the real
			 * gap about 5: an offset, the pill lime, logged), one in contact, one of another class (counted, logged) */
			s32 host_kids(s32 *offsets, s32 *leaves, s32 *other);
			s32 host_kid(s32 i, u32 *key, s32 *state, float *tim, float *over, s32 *corners);
			enum { ARR = 0x4F200, L1 = 0x4F400, L2 = 0x4F480, OT = 0x4F500 };
			const u32 C = 0xBC000;
			s32 offs, leaves, other, n, st, corners;
			u32 key; float tim, over;
			W32(C + 0x28, 3); W32(C + 0x30, 9); W32(C + 0x38, 8); W32(0x1B00, C);       /* the level: floor 5 below, a wall */
			W32(AGENT, 0x4F100); W32(AGENT + 0x0C, ARR); W32(AGENT + 0x10, 3);
			W32(ARR + 0, 0x20000000u); W32(ARR + 4, 0); W32(ARR + 8, L1);
			W32(ARR + 12, 0x20000001u); W32(ARR + 16, 0); W32(ARR + 20, L2);
			W32(ARR + 24, 0x20000002u); W32(ARR + 28, 0); W32(ARR + 32, OT);
			memset(MEM + L1, 0, 0x80); W32(L1, 0x4F000); W32(L1 + 0x18, 0); WF(L1 + 0x28, 1.f); WF(L1 + 0x2C, 6.f);
			memset(MEM + L2, 0, 0x80); W32(L2, 0x4F010); W32(L2 + 0x18, 1); WF(L2 + 0x28, 1.f); WF(L2 + 0x2C, 0.f);   /* the GSK subclass */
			/* another class whose +0x0C / +0x10 look like a list holding a leaf: must not be followed */
			memset(MEM + 0x4F600, 0, 0x40); W32(0x4F600, 0x20000003u); W32(0x4F608, L1);
			W32(OT, 0x4F300); W32(OT + 0x0C, 0x4F600); W32(OT + 0x10, 1);
			/* the level's welder: a child that is a BV-tree agent (0x412ADC's stand-in) holding a TIM leaf on the floor (category
			 * 1) and one with a key of another category (2) - the first read, the second logged as not placed */
			memset(MEM + 0x4F640, 0, 0x40); W32(0x4F640, 0x4F060); W32(0x4F64C, 0x4F680); W32(0x4F650, 2);
			memset(MEM + 0x4F680, 0, 0x20); W32(0x4F680, 0x20000001u); W32(0x4F688, L2); W32(0x4F68C, 0x40000005u); W32(0x4F694, L1);
			W32(ARR + 24, 0x20000002u); W32(ARR + 32, 0x4F640); W32(AGENT + 0x10, 3);
			state(3099, 0.f, 0, 0); frame();
			{ s32 bo2, bl2, bt2; host_kids(&bo2, &bl2, &bt2); assert(bl2 == 4 && bt2 == 1); }   /* its two leaves read (the BV-tree itself counted as a non-leaf) */
			assert(logged("ghost TIMs: a key of category 2 not placed - e.g. key 0x40000005") == 1);
			W32(ARR + 24, 0x20000002u); W32(ARR + 32, OT);
			/* the level's welder: a child that is a transform agent around a TIM leaf - read as a leaf */
			memset(MEM + 0x4F7C0, 0, 0x10); W32(0x4F7C0, 0x4F030); W32(0x4F7CC, L2); W32(ARR + 20, 0x4F7C0);
			state(3100, 0.f, 0, 0); frame();
			n = host_kids(&offs, &leaves, &other);
			assert(n == 2 && leaves == 2 && other == 1 && offs == 1);
			host_kid(0, &key, &st, &tim, &over, &corners); assert(key == 0x20000000u && st == 2 && corners == 3 && over > 0.5f);
			host_kid(1, &key, &st, &tim, &over, &corners); assert(st == 1);
			assert(host_ghost_state() == 1);                                            /* an offset: lime */
			assert(logged("ghost offset: key") >= 1 && logged("TIM leaves 2, other children 1") >= 1);   /* not followed: still 2 */
			{
				/* believed poses: one (the skipping child over-credited; the one in contact gives none), its over-credit and
				 * its separating normal (straight up); travel regions: one (the skipping child) */
				s32 host_believed(float *over0, float *dir0);
				s32 host_travel(void);
				float bo, bd[3];
				assert(host_believed(&bo, bd) == 1 && bo > 0.5f && bd[2] > 0.99f && host_travel() == 1);
			}
			{
				/* a key past the level's surfaces (as after a teleport): no polygon, no read past the end */
				s32 n2;
				W32(ARR + 0, 0x2000FFF0u); state(3100, 0.f, 0, 0); frame();
				n2 = host_kids(&offs, &leaves, &other); host_kid(0, &key, &st, &tim, &over, &corners);
				assert(n2 == 2 && corners == 0 && st != 2);
				W32(ARR + 0, 0x20000000u); state(3100, 0.f, 0, 0); frame();
			}
			WF(L1 + 0x2C, 4.f); state(3101, 0.f, 0, 0); frame();                       /* TIM under the gap: honest */
			host_kids(&offs, &leaves, &other); host_kid(0, &key, &st, &tim, &over, &corners);
			assert(offs == 0 && st == 0 && host_ghost_state() == 0);
			{
				s32 host_believed(float *over0, float *dir0);
				s32 host_travel(void);
				float bo, bd[3];
				assert(host_believed(&bo, bd) == 0 && host_travel() == 1);              /* honest: no believed pose, its zone still */
			}
			{
				/* 6h: the offset save state (Cartographer's) - Save offsets snapshots the skipping TIM (the one in contact
				 * left out) and asks for a checkpoint; 3 ticks on it is written back over the rebuilt pair (normal and TIM);
				 * Revert offsets asks for a revert and writes it back once time has gone back; not where it was saved:
				 * nothing written; natural reverts restore it only with the option on; the Ghost Viewer off: refused */
				s32 host_ofs(s32 *phase, s32 *restored);
				void host_set_ofs_natural(s32 on);
				s32 ph, rs;
				W32(0x1CFC, 0); W32(0x1D00, 0);
				host_practice(11); assert(MEM[0x1CFF] == 0 && !strcmp(host_toast(), "Revert offsets: nothing saved yet"));
				host_set_ghost(0); host_practice(10);
				assert(MEM[0x1D00] == 0 && !strcmp(host_toast(), "Save offsets: turn the Ghost Viewer on first"));
				host_set_ghost(1);
				WF(L1 + 0x20, 0.f); WF(L1 + 0x24, 0.f); WF(L1 + 0x28, 1.f); WF(L1 + 0x2C, 6.f);   /* skipping, TIM 6 */
				state(3102, 0.f, 0, 0); frame();
				host_practice(10);
				assert(MEM[0x1D00] == 1);                                             /* the checkpoint, asked for at once */
				state(3103, 0.f, 0, 0); frame();                                      /* this frame's update: the snapshot */
				assert(host_ofs(&ph, &rs) == 1 && ph == 1 && !strcmp(host_toast(), "Offsets saved: 1 TIMs - checkpoint made"));
				W32(0x1D00, 0);                                                       /* saved: the pair rebuilt, its TIM fresh */
				WF(L1 + 0x20, 1.f); WF(L1 + 0x28, 0.f); WF(L1 + 0x2C, 0.5f);
				state(3104, 0.f, 0, 0); frame(); state(3105, 0.f, 0, 0); frame();
				assert(M32f(L1 + 0x2C) == 0.5f);                                     /* not before 3 ticks on */
				state(3106, 0.f, 0, 0); frame();
				assert(M32f(L1 + 0x2C) == 6.f && M32f(L1 + 0x28) == 1.f && M32f(L1 + 0x20) == 0.f);
				assert(host_ofs(&ph, &rs) == 1 && ph == 0 && rs == 1 && !strcmp(host_toast(), "Offsets kept: 1 of 1 TIMs"));
				assert(logged("offset save state: Offsets kept: 1 of 1 TIMs") == 1);
				/* played on, the TIM spent; Revert offsets: the revert flag, then time back -> written back */
				WF(L1 + 0x2C, 0.3f); state(3150, 0.f, 0, 0); frame();
				host_practice(11);
				assert(MEM[0x1CFF] == 1 && host_ofs(&ph, &rs) == 1 && ph == 1 && !strcmp(host_toast(), "Revert offsets: reverting"));
				W32(0x1CFC, 0);
				state(3151, 0.f, 0, 0); frame(); assert(M32f(L1 + 0x2C) == 0.3f);   /* time still going forward */
				WF(L1 + 0x2C, 0.4f); state(3103, 0.f, 0, 0); frame();                /* reverted */
				assert(M32f(L1 + 0x2C) == 6.f && !strcmp(host_toast(), "Offsets restored: 1 of 1 TIMs"));
				/* in contact when it comes back: never written (then given up after 60 frames, 0 of 1) */
				WF(L1 + 0x2C, 0.3f); state(3160, 0.f, 0, 0); frame();
				host_practice(11); W32(0x1CFC, 0); W32(L1 + 0x18, 1);
				state(3103, 0.f, 0, 0); frame(); assert(M32f(L1 + 0x2C) == 0.3f);
				for (t = 0; t < 60; t++) { state((u32)(3104 + t), 0.f, 0, 0); frame(); }
				assert(M32f(L1 + 0x2C) == 0.3f && host_ofs(&ph, &rs) == 1 && ph == 0 && !strcmp(host_toast(), "Offsets restored: 0 of 1 TIMs (the rest had no pair)"));
				W32(L1 + 0x18, 0);
				/* reverted somewhere else (a newer checkpoint): nothing written */
				state(3170, 0.f, 0, 0); frame();
				host_practice(11); W32(0x1CFC, 0);
				WF(OBJECT + 0x64, 0.5f); WF(L1 + 0x2C, 0.4f); state(3120, 0.f, 0, 0); frame();
				assert(M32f(L1 + 0x2C) == 0.4f && !strcmp(host_toast(), "Revert offsets: not where they were saved (nothing restored)"));
				WF(OBJECT + 0x64, 0.f); state(3121, 0.f, 0, 0); frame();
				/* a natural revert (death, the pause menu): left alone with the option off, restored with it on */
				state(3180, 0.f, 0, 0); frame(); WF(L1 + 0x2C, 0.2f); state(3110, 0.f, 0, 0); frame();
				assert(M32f(L1 + 0x2C) == 0.2f);
				host_set_ofs_natural(1);
				state(3190, 0.f, 0, 0); frame(); state(3115, 0.f, 0, 0); frame();
				assert(M32f(L1 + 0x2C) == 6.f && !strcmp(host_toast(), "Offsets restored: 1 of 1 TIMs"));
				host_set_ofs_natural(0);
				WF(L1 + 0x2C, 4.f); state(3116, 0.f, 0, 0); frame();               /* (as the tests below expect it) */
				printf("offset save state: saved + checkpoint, kept 3 ticks on; Revert offsets restores; contact / moved: nothing; natural reverts only with the option\n");
			}
			{
				/* past a triangle's plane but beside it - not through: the wall at x = -1 (its top 2 below); the TIM first
				 * seen from x = -2 (honest), then you at x = 0, above the wall: no offset (5c.3 called it "through": ~1.7) */
				W32(AGENT + 0x10, 1);
				W32(ARR + 0, 0x20000002u); W32(ARR + 4, 0); W32(ARR + 8, L1);
				W32(L1 + 0x18, 0); WF(L1 + 0x20, -1.f); WF(L1 + 0x24, 0.f); WF(L1 + 0x28, 0.f); WF(L1 + 0x2C, 0.5f);
				WF(MS + 0x50, -2.f); WF(OBJECT + 0x64, -2.f); WF(MO + 0x60, -2.f); WF(MO + 0x70, -2.f);
				state(3200, 0.f, 0, 0); frame();
				host_kid(0, &key, &st, &tim, &over, &corners);
				assert(key == 0x20000002u && corners == 4 && st == 0);
				WF(MS + 0x50, 0.f); WF(OBJECT + 0x64, 0.f); WF(MO + 0x60, 0.f); WF(MO + 0x70, 0.f);
				state(3201, 0.f, 0, 0); frame();
				host_kid(0, &key, &st, &tim, &over, &corners);
				assert(st != 2 && over < 0.08f);
				/* a slant's TIM first seen from beside it (not over it: its side not kept), then you over it on its other side
				 * with an honest TIM: no offset (5c.7 kept the first side and called it "through": the pose in the floor) */
				W32(AGENT + 0x10, 0); state(3297, 0.f, 0, 0); frame(); state(3298, 0.f, 0, 0); frame(); state(3299, 0.f, 0, 0); frame();
				W32(AGENT + 0x10, 1);                                                     /* the earlier TIM gone: a new weld */
				WF(MS + 0x50, 0.f); WF(MS + 0x58, 0.f); WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x6C, 0.f);
				state(3300, 0.f, 0, 0); frame();
				WF(MS + 0x50, -2.f); WF(MS + 0x58, -4.7f); WF(OBJECT + 0x64, -2.f); WF(OBJECT + 0x6C, -4.7f);
				WF(MO + 0x60, -2.f); WF(MO + 0x70, -2.f);
				state(3301, 0.f, 0, 0); frame();
				host_kid(0, &key, &st, &tim, &over, &corners);
				assert(corners == 4 && st != 2 && over < 0.08f);
				/* a real ghost: from there (over the wall, its side kept) through it to its other side at the same height -
				 * an offset (~1.7: through by 1, the capsule's half-size, the TIM still 0.5) */
				WF(MS + 0x50, 0.f); WF(OBJECT + 0x64, 0.f); WF(MO + 0x60, 0.f); WF(MO + 0x70, 0.f);
				state(3302, 0.f, 0, 0); frame();
				host_kid(0, &key, &st, &tim, &over, &corners);
				assert(st == 2 && over > 1.6f && over < 1.8f);
				WF(MS + 0x50, 0.f); WF(MS + 0x58, 0.f); WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x6C, 0.f); WF(MO + 0x60, 0.f); WF(MO + 0x70, 0.f);
				{
					/* objects, stage 0: a pair of your body with an object's body (a GSK leaf agent) - logged once */
					memset(MEM + 0x4F700, 0, 0x40); W32(0x4F700, 0x4F000); WF(0x4F72C, 0.3f);
					W32(PAIRS + 8, 0x4F700); W32(0x50000 + 0x10, ENTITY); W32(0x50000 - 0x10, SHAPE);
					state(3303, 0.f, 0, 0); frame(); state(3304, 0.f, 0, 0); frame();
					assert(logged("ghost object pair (component): shape type 7") == 1 && logged("TIM leaves 1, other children 0") >= 1);
					{
						/* objects: the GJK distance against known answers, then a type-23 sweep around a box 2 ahead with TIM 2
						 * (the real gap ~1.3): an object offset (~0.74), the pill lime, an object believed pose */
						float host_gjk_box(const float *ga, const float *gb, const float *gc, const float *ghe, const float *gsw);
						s32 host_objects(s32 *offsets, float *over0, float *gap0);
						s32 host_believed(float *over0, float *dir0);
						float sa[3] = { 0, 0, 0.2f }, sb[3] = { 0, 0, 0.5f }, he[3] = { 0.5f, 0.5f, 0.5f }, z0[3] = { 0, 0, 0 };
						float c1[3] = { 2, 0, 0.35f }, c2[3] = { 0, 0, -1 }, c3[3] = { 2, 2, 0.35f }, c4[3] = { 0, 0, 0.35f }, swp[3] = { -1, 0, 0 };
						float oo, og, bo, bd[3];
						s32 offs2;
						static const u8 get23[6] = { 0xB8, 23, 0, 0, 0, 0xC3 }, get6[6] = { 0xB8, 6, 0, 0, 0, 0xC3 };
						assert(fabsf(host_gjk_box(sa, sb, c1, he, z0) - 1.5f) < 1e-3f);
						assert(fabsf(host_gjk_box(sa, sb, c1, he, swp) - 0.5f) < 1e-3f);
						assert(fabsf(host_gjk_box(sa, sb, c2, he, z0) - 0.7f) < 1e-3f);
						assert(fabsf(host_gjk_box(sa, sb, c3, he, z0) - 2.1213f) < 1e-3f);
						assert(host_gjk_box(sa, sb, c4, he, z0) < 1e-4f);
						memcpy(MEM + 0x1F810, get23, 6); memcpy(MEM + 0x1F820, get6, 6);
						W32(0x4FA00 + 0x14, 0x1F810); W32(0x4FA40 + 0x14, 0x1F820);
						memset(MEM + 0x4F800, 0, 0x40); W32(0x4F800, 0x4FA00); W32(0x4F830, 0x4F900);                 /* the sweep, no sweep vector */
						memset(MEM + 0x4F900, 0, 0x40); W32(0x4F900, 0x4FA40); WF(0x4F910, 0.5f); WF(0x4F914, 0.5f); WF(0x4F918, 0.5f);   /* the box */
						memset(MEM + 0x4FB00, 0, 0x60); WF(0x4FB20, 1.f); WF(0x4FB34, 1.f); WF(0x4FB48, 1.f);
						WF(0x4FB50, 2.f); WF(0x4FB54, 0.f); WF(0x4FB58, 0.35f);                                         /* its pose */
						memset(MEM + 0x4F700, 0, 0x40); W32(0x4F700, 0x4F000); WF(0x4F720, 1.f); WF(0x4F72C, 2.f);   /* TIM 2 */
						W32(PAIRS + 8, 0x4F700); W32(0x50000 + 0x10, ENTITY); W32(0x50000 - 0x10, 0x4F800); W32(0x50000 - 8, 0x4FB00);
						state(3305, 0.f, 0, 0); frame();
						assert(host_objects(&offs2, &oo, &og) == 1 && offs2 == 1 && fabsf(og - 1.3f) < 0.01f && oo > 0.7f && oo < 0.78f);
						assert(host_ghost_state() == 1 && host_believed(&bo, bd) >= 1);
						{
							/* object believed poses on their own: the level's setting off, the object's pose still there */
							void host_set_believed(s32 on);
							host_set_believed(0); state(3305, 0.f, 0, 0); frame();
							assert(host_believed(&bo, bd) >= 1);
							host_set_believed(1);
						}
						assert(logged("ghost offset: object agent") >= 1);
						assert(logged("ghost object measured: box, box x100 100 x 100 x 100, gap x1000 1300") >= 1);
						{
							void host_set_obj_travel(s32 on);
							s32 host_otravel(void);
							host_set_obj_travel(1); g_q = 0; state(3305, 0.f, 0, 0); frame();
							assert(host_otravel() == 1 && g_q > 0);                                      /* drawn */
							host_set_obj_travel(0);
						}
						WF(0x4F72C, 1.f); state(3306, 0.f, 0, 0); frame();                                             /* TIM 1: honest */
						assert(host_objects(&offs2, &oo, &og) == 1 && offs2 == 0);
						{
							/* a box as the root shape, its pair agent a transform agent around the TIM leaf: stepped through, measured */
							static const u8 get8[6] = { 0xB8, 8, 0, 0, 0, 0xC3 };
							s32 vi;
							WF(0x4F72C, 2.f);
							memset(MEM + 0x4F780, 0, 0x10); W32(0x4F780, 0x4F020); W32(0x4F78C, 0x4F700);
							W32(PAIRS + 8, 0x4F780); W32(0x50000 - 0x10, 0x4F900);
							state(3307, 0.f, 0, 0); frame();
							assert(host_objects(&offs2, &oo, &og) == 1 && offs2 == 1 && fabsf(og - 1.3f) < 0.01f);
							/* a convex hull (type 8): a unit cube in two blocks of four - the same gap */
							memcpy(MEM + 0x1F830, get8, 6); W32(0x4FA80 + 0x14, 0x1F830);
							memset(MEM + 0x4FC00, 0, 0x40); W32(0x4FC00, 0x4FA80); W32(0x4FC30, 0x4FD00); W32(0x4FC34, 2);
							for (vi = 0; vi < 8; vi++)
							{
								u32 bb = 0x4FD00 + (u32)(vi / 4) * 0x30, li = (u32)(vi % 4) * 4;
								WF(bb + li, (vi & 1) ? 0.5f : -0.5f); WF(bb + 0x10 + li, (vi & 2) ? 0.5f : -0.5f); WF(bb + 0x20 + li, (vi & 4) ? 0.5f : -0.5f);
							}
							W32(0x50000 - 0x10, 0x4FC00);
							state(3308, 0.f, 0, 0); frame();
							assert(host_objects(&offs2, &oo, &og) == 1 && offs2 == 1 && fabsf(og - 1.3f) < 0.01f);
							/* static scenery: a body in no component (as the level's), its agent a TIM leaf, a box - listed before the
							 * level's welder pair: measured as an object; the welder pair is still the level */
							W32(PAIRS + 8, 0x4F700); W32(0x50000 + 0x10, 0x51000); W32(0x50000 - 0x10, 0x4F900);
							state(3309, 0.f, 0, 0); frame();
							assert(host_objects(&offs2, &oo, &og) == 1 && offs2 == 1 && fabsf(og - 1.3f) < 0.01f);
							assert(logged("ghost object pair (static): shape type 6") == 1);
							{ s32 lo, ll, lt; host_kids(&lo, &ll, &lt); assert(ll >= 1); }                 /* the level's TIMs still read */
							/* no pair with the level right now (its partner made a component body): the pill from your body all the
							 * same, the static box still measured (5d.2 stopped: nothing near you while it lasted) */
							W32(0x50020 + 0x10, ENTITY);
							state(3310, 0.f, 0, 0); frame();
							assert(host_ghost_state() >= 0 && host_objects(&offs2, &oo, &og) == 1 && offs2 == 1 && fabsf(og - 1.3f) < 0.01f);
							assert(logged("ghost pill: from your body") >= 1);
							W32(0x50020 + 0x10, 0x51000);
							{
								/* crowding: 20 far objects (weapons, say: boxes 10-29 out) listed before the near box (2 ahead) - the
								 * nearest kept, so the near box is still measured; and a line with a NaN end draws nothing */
								float host_obj_min_gap(void);
								void host_line3(const float *la, const float *lb);
								s32 wi;
								float nanv = 0.f, pa[3] = { 2.f, 0.f, 0.f }, pb[3];
								for (wi = 0; wi < 20; wi++)
								{
									u32 hd = 0x52000 + (u32)wi * 0x40, ms = 0x54000 + (u32)wi * 0x60, pp = PAIRS + 48 + (u32)wi * 16;
									memset(MEM + ms, 0, 0x60); WF(ms + 0x20, 1.f); WF(ms + 0x34, 1.f); WF(ms + 0x48, 1.f);
									WF(ms + 0x50, 10.f + (float)wi); WF(ms + 0x58, 0.35f);
									W32(hd + 0x10, ENTITY); W32(hd - 0x10, 0x4F900); W32(hd - 8, ms);
									W32(pp, HANDLE); W32(pp + 4, hd); W32(pp + 8, 0x4F700); W32(pp + 12, 0);
								}
								/* the near box last */
								W32(PAIRS + 48 + 20 * 16, HANDLE); W32(PAIRS + 48 + 20 * 16 + 4, 0x50000); W32(PAIRS + 48 + 20 * 16 + 8, 0x4F700); W32(PAIRS + 48 + 20 * 16 + 12, 0);
								W32(PAIRS + 8, NULL_AGENT);
								W32(ISLAND + 0x6C, 24);
								state(3311, 0.f, 0, 0); frame();
								assert(fabsf(host_obj_min_gap() - 1.3f) < 0.01f);
								W32(ISLAND + 0x6C, 3); W32(PAIRS + 8, 0x4F700);
								memcpy(&nanv, "\x00\x00\xc0\x7f", 4);
								pb[0] = nanv; pb[1] = 0.f; pb[2] = 0.f;
								g_q = 0; host_line3(pa, pb); assert(g_q == 0);
								{
									/* an edge passing beside the camera (0.15 ahead, 1 to the side) to 3 ahead: cut at 0.25 from the camera, so it
									 * stays near the screen (5e.3's 0.05 drew its near end ~2900 px out - the band across the screen) */
									void host_cam(float *cp, float *cf, float *cr);
									float cp[3], cf[3], cr[3], e0[3], e1[3];
									s32 qi, w;
									host_cam(cp, cf, cr);
									for (w = 0; w < 3; w++) { e0[w] = cp[w] + cf[w] * 0.15f + cr[w] * 1.f; e1[w] = cp[w] + cf[w] * 3.f; }
									g_q = 0; host_line3(e0, e1);
									assert(g_q == 1);
									for (qi = 0; qi < 8; qi += 2) assert(g_qxy[0][qi] > -2000 && g_qxy[0][qi] < 2000 + 640);
								}
							}
							{
								/* a list shape (type 10: the Cairo boxes) under a list agent: two entries (the box 2 ahead), child agents
								 * the TIM leaf and the shared empty agent (filtered out) - one piece measured, an offset */
								static const u8 get10[6] = { 0xB8, 10, 0, 0, 0, 0xC3 };
								memcpy(MEM + 0x1F840, get10, 6); W32(0x4FAC0 + 0x14, 0x1F840);
								memset(MEM + 0x4FE00, 0, 0x100);
								W32(0x4FE00, 0x4FAC0); W32(0x4FE0C, 0x4FE40);                           /* the list shape, its entries */
								W32(0x4FE40, 0x4F900); W32(0x4FE48, 0x4F900);
								W32(0x4FE80, 0x4F040); W32(0x4FE8C, 0x4FEC0); W32(0x4FE90, 2);           /* the list agent, its children */
								W32(0x4FEC0, 0x4F700); W32(0x4FEC4, NULL_AGENT);
								W32(PAIRS + 8, 0x4FE80); W32(0x50000 - 0x10, 0x4FE00); W32(0x50000 + 0x10, ENTITY);
								state(3312, 0.f, 0, 0); frame();
								assert(host_objects(&offs2, &oo, &og) == 1 && offs2 == 1 && fabsf(og - 1.3f) < 0.01f);
								{
									/* a wrapper (type 19) around the list, under a BV-tree agent: its entry's key 0 picks piece 0 (the box) */
									static const u8 get19[6] = { 0xB8, 19, 0, 0, 0, 0xC3 };
									memcpy(MEM + 0x1F850, get19, 6); W32(0x4FB80 + 0x14, 0x1F850);
									memset(MEM + 0x4FF00, 0, 0x20); W32(0x4FF00, 0x4FB80); W32(0x4FF0C, 0x4FE00);
									W32(0x4FE10, 2);                                                    /* the list's entry count */
									memset(MEM + 0x4FF40, 0, 0x40); W32(0x4FF40, 0x4F060); W32(0x4FF4C, 0x4FF60); W32(0x4FF50, 1);
									W32(0x4FF60, 0); W32(0x4FF68, 0x4F700);
									W32(PAIRS + 8, 0x4FF40); W32(0x50000 - 0x10, 0x4FF00);
									state(3313, 0.f, 0, 0); frame();
									assert(host_objects(&offs2, &oo, &og) == 1 && offs2 == 1 && fabsf(og - 1.3f) < 0.01f);
								}
								W32(PAIRS + 8, 0x4F700); W32(0x50000 - 0x10, 0x4F900);
							}
							W32(0x50000 + 0x10, ENTITY);
						}
						W32(0x50000 - 8, 0);
					}
					W32(PAIRS + 8, NULL_AGENT); W32(0x50000 + 0x10, 0); W32(0x50000 - 0x10, 0);
				}
			}
			{
				/* baked scenery (key category 4): scenery index 5 -> its object (slot 9) -> definition -> 'hlmt' -> 'coll' ->
				 * region 0 / permutation 0 -> one BSP (the wall level, identity node). The level welder's entry: a transform
				 * agent around the scenery's welder, a TIM leaf on its surface 2 (the wall quad) - read, placed (4 corners) */
				const u32 TT2 = 0xB8000, DEF2 = 0xB8100, HL2 = 0xB8200, CO2 = 0xB8400, SO2 = 0xB8800, TBL = 0xB8C00, SW = 0xB8D00, XA = 0xB8E00;
				u32 kk; s32 st2, cr2, nk, j;
				float tm2, ov2;
				W32(TT2 + 0x00, 0x7363656E); W32(TT2 + 0x08, DEF2);
				W32(TT2 + 0x10, 0x686C6D74); W32(TT2 + 0x18, HL2);
				W32(TT2 + 0x20, 0x636F6C6C); W32(TT2 + 0x28, CO2);
				W32(0x1B08, TT2);
				W32(DEF2 + 0x38, 1); W32(HL2 + 0x0C, 2);
				W32(HL2 + 0x70, 1); W32(HL2 + 0x74, HL2 + 0x100);                    /* model region 0 */
				MEM[HL2 + 0x100 + 4] = 0; W32(HL2 + 0x100 + 8, 1); W32(HL2 + 0x100 + 0xC, HL2 + 0x180);
				MEM[HL2 + 0x180 + 5] = 0;                                               /* permutation 0 -> collision permutation 0 */
				W32(CO2 + 0x1C, 1); W32(CO2 + 0x20, CO2 + 0x100);
				W32(CO2 + 0x100 + 4, 1); W32(CO2 + 0x100 + 8, CO2 + 0x200);
				W32(CO2 + 0x200 + 4, 1); W32(CO2 + 0x200 + 8, CO2 + 0x300);
				W32(CO2 + 0x300, 0);
				for (j = 0; j < 0x40; j += 4) W32(CO2 + 0x300 + 4 + (u32)j, M32h(C + (u32)j));    /* the wall level's BSP */
				memset(MEM + SO2, 0, 0x300); W32(SO2 + 0x114, 0x200u << 16);
				WF(SO2 + 0x200, 1.f); WF(SO2 + 0x204, 1.f); WF(SO2 + 0x214, 1.f); WF(SO2 + 0x228, 1.f);   /* identity node */
				W32(OBJ_DATA + 9 * 12, 0x11); W32(OBJ_DATA + 9 * 12 + 8, SO2);
				W32(0x1F900, TBL); W32(TBL + 4 * 5, 0x00110009u);
				memset(MEM + SW, 0, 0x40); W32(SW, 0x4F100); W32(SW + 0x0C, SW + 0x20); W32(SW + 0x10, 1);
				W32(SW + 0x20, 2); W32(SW + 0x28, L1);                                  /* surface 2, the TIM leaf */
				memset(MEM + XA, 0, 0x10); W32(XA, 0x4F020); W32(XA + 0x0C, SW);
				W32(ARR + 0, 0x80000005u); W32(ARR + 8, XA);                            /* the level welder's entry */
				W32(ARR + 12, 0x60000005u); W32(ARR + 20, XA);                          /* and a category-3 one */
				W32(AGENT + 0x10, 2);
				state(3400, 0.f, 0, 0); frame();
				nk = host_kids(&offs, &leaves, &other);
				for (j = 0, st2 = -1; j < nk; j++) { host_kid(j, &kk, &st2, &tm2, &ov2, &cr2); if (kk & 0x80000000u) break; }
				assert(j < nk && cr2 == 4);
				assert(logged("ghost: baked scenery found (category 4)") == 1 && logged("without a physics model") == 1);
				{
					/* category 3: the model's physics model - region 0 / permutation 0 -> rigid body 0 (node 1, 2 ahead), its shape
					 * the box; the entry's agent a transform agent around a TIM leaf - measured as an object (gap 1.3) */
					float host_obj_min_gap(void);
					const u32 PH = 0xB9000, XB = 0xB8E40;
					W32(TT2 + 0x30, 0x70686D6F); W32(TT2 + 0x38, PH);                        /* 'phmo' (tag 3) */
					W32(HL2 + 0x24, 3); MEM[HL2 + 0x100 + 5] = 0; MEM[HL2 + 0x180 + 6] = 0;
					memset(MEM + PH, 0, 0x400);
					W32(PH + 0xC0, 1); W32(PH + 0xC4, PH + 0x100);
					W32(PH + 0x100 + 4, 1); W32(PH + 0x100 + 8, PH + 0x140);
					W32(PH + 0x140 + 4, 1); W32(PH + 0x140 + 8, PH + 0x180); W32(PH + 0x180, 0);
					W32(PH + 0x38, 1); W32(PH + 0x3C, PH + 0x200);
					W32(PH + 0x200, 1); W32(PH + 0x200 + 0x38, 0x4F900);                   /* node 1, the box */
					WF(SO2 + 0x234, 1.f); WF(SO2 + 0x238, 1.f); WF(SO2 + 0x248, 1.f); WF(SO2 + 0x25C, 1.f);   /* node 1: identity ... */
					WF(SO2 + 0x234 + 0x28, 2.f); WF(SO2 + 0x234 + 0x30, 0.35f);                /* ... at (2, 0, 0.35) */
					memset(MEM + XB, 0, 0x10); W32(XB, 0x4F020); W32(XB + 0x0C, L1);
					WF(L1 + 0x2C, 2.f); W32(L1 + 0x18, 0);
					W32(ARR + 12, 0x60000005u); W32(ARR + 20, XB);
					state(3401, 0.f, 0, 0); frame();
					assert(logged("ghost: baked scenery found (category 3, physics model)") == 1);
					assert(fabsf(host_obj_min_gap() - 1.3f) < 0.01f);
				}
				W32(ARR + 0, 0x20000000u); W32(ARR + 8, L1); W32(ARR + 12, 0x20000001u); W32(ARR + 20, L2);
			}
			{
				/* the ground probe: your feet on the wall level's floor (two triangles at z -5 sharing a diagonal) - the level's
				 * surface found, z -5, straight up; its shared edge a floor seam */
				void host_probe(const float *pf);
				float pf[3] = { 3.f, -1.f, -5.f };
				host_probe(pf);
				assert(logged("ground probe: the LEVEL's surface") >= 1 && logged("z x100 -500, normal x100 0 0 100") >= 1 && logged(" floor seam]") >= 1);
			}
			W32(AGENT, 0); W32(AGENT + 0x0C, 0); W32(AGENT + 0x10, 0); W32(0x1B00, 0);
		}
		MEM[FN] = 0x90; state(3013, 0.f, 0, 0); frame(); assert(host_ghost_state() == -1);   /* not a capsule's code: none */
		assert(logged("ghost pill: none - the capsule shape not found") == 1);
		MEM[FN] = 0xB8;
		host_set_ghost(0); state(3014, 0.f, 0, 0); frame(); assert(host_ghost_state() == -1);
		for (k = 0; k < 0x60; k += 4) W32(MS + (u32)k, 0);
		W32(HANDLE - 8, 0); W32(ENTITY + 0x3C, 0); W32(SHAPE, 0);
		{
			/* Cartographer's wireframe: 3 rings (48), 8 lines along it, 4 half-circles over each pole (64) = 120 lines */
			void host_draw_cap(const float *ca, const float *cb, float cr);
			float ca[3] = { 2.f, 0.f, -0.2f }, cb[3] = { 2.f, 0.f, 0.2f };
			g_q = 0; host_draw_cap(ca, cb, 0.174f);
			assert(g_q >= 110 && g_q <= 120);
		}
		printf("ghost pill: the capsule from the body's level pair (Cartographer's 120-line wireframe); pink, red when stuck, lime on a rewind (logged) and its hold; a bad shape or off: none\n");
	}
	{
		/* step 6d: the predicted spots - files in the converter's format, matched to the level by geometry (the test level's
		 * floor: the square z -1, x 2..4, y -1..1), read into run-time memory, the nearby ones listed, the nearest described */
		void host_set_spots_dir(const char *d); void host_set_pred(s32 on, s32 dread);
		s32 host_pred(s32 *n, s32 *near_n, s32 *best, float *best_d);
		FILE *f; int e, k, pn, pnear, pbest, st;
		float pd;
		unsigned char ix[8 + 3 * 168];
		system("mkdir -p /tmp/sgtest /tmp/sgtest_nomatch");
		memset(ix, 0, sizeof(ix)); memcpy(ix, "SGI1", 4); { unsigned int c = 3; memcpy(ix + 4, &c, 4); }
		for (e = 0; e < 3; e++)
		{
			unsigned char *r = ix + 8 + e * 168;
			unsigned short num = (unsigned short)e; unsigned int cnt = e == 1 ? 3 : 1;
			float bb[6], sm[24];
			memcpy(r, &num, 2); memcpy(r + 4, &cnt, 4);
			if (e == 0) { bb[0] = 500; bb[1] = 500; bb[2] = 0; bb[3] = 510; bb[4] = 510; bb[5] = 1; }   /* outside the level */
			else { bb[0] = 2.2f; bb[1] = -0.8f; bb[2] = -1.f; bb[3] = 3.8f; bb[4] = 0.8f; bb[5] = -1.f; }
			memcpy(r + 8, bb, 24);
			for (k = 0; k < 8; k++) { sm[3 * k] = 2.2f + 0.2f * k; sm[3 * k + 1] = -0.7f + 0.2f * k; sm[3 * k + 2] = e == 2 ? 3.f : -1.f; }   /* 2: in the air */
			memcpy(r + 32, sm, 96);
			snprintf((char *)r + 128, 40, e == 1 ? "Test floor" : "Elsewhere");
		}
		f = fopen("/tmp/sgtest/index.bin", "wb"); fwrite(ix, 1, sizeof(ix), f); fclose(f);
		{
			/* file 01: a dreadnought 40 u/s at (3, 0, -1), a monster 25 u/s at (2.5, 0.5, -1), one 40 units off */
			unsigned char b[20 + 3 * 12]; float org[3] = { 0.f, 0.f, -2.f }; unsigned int c = 3;
			short pos[3][3] = { { 48, 0, 16 }, { 40, 8, 16 }, { 688, 0, 16 } };
			unsigned char ceil4[3] = { 160, 100, 160 };
			memcpy(b, "SGP1", 4); memcpy(b + 4, &c, 4); memcpy(b + 8, org, 12);
			for (k = 0; k < 3; k++) { unsigned char *r = b + 20 + 12 * k; memcpy(r, pos[k], 6); r[6] = ceil4[k]; r[7] = 51; r[8] = k == 1 ? 0 : 26; r[9] = 30; r[10] = 0; r[11] = (unsigned char)(signed char)-22; }
			f = fopen("/tmp/sgtest/01.bin", "wb"); fwrite(b, 1, sizeof(b), f); fclose(f);
		}
		f = fopen("/tmp/sgtest_nomatch/index.bin", "wb"); { unsigned char ix2[8 + 168]; memcpy(ix2, ix, 8); ix2[4] = 1; memcpy(ix2 + 8, ix + 8 + 2 * 168, 168); fwrite(ix2, 1, sizeof(ix2), f); } fclose(f);
		W32(0x1B00, 0xB6000);
		WF(0x1800, 3.f); WF(0x1804, 0.f); WF(0x1808, 0.f);                          /* the view at (3, 0, 0) */
		{
			/* step 6d.2: loading them leaves the Superbounce Viewer's level grid alone (6d built it here, at the wrong point in
			 * the frame, and the viewer kept that grid: its seams broke) - with the viewer off, the grid stays unbuilt */
			u32 host_lg_for(void); void host_lg_forget(void); void host_set_sb_hud(s32 on); s32 host_sb_hud(void);
			s32 was = host_sb_hud();
			host_set_sb_hud(0); host_lg_forget();
			host_set_spots_dir("/tmp/sgtest"); host_set_pred(1, 0);
			for (k = 0; k < 4; k++) frame();
			assert(host_lg_for() == 0);
			host_set_sb_hud(was);
		}
		st = host_pred(&pn, &pnear, &pbest, &pd);
		assert(st == 1 && pn == 3 && pnear == 3 && pbest == 0 && fabsf(pd - 1.f) < 0.01f);   /* the default range (2000): all 3 */
		assert(!drawn("PREDICTED"));                                                 /* 6d.3: no line at the bottom */
		{
			/* the arrows' labels: the launch's u/s; the approach's speed - falling 30, drift (0, -2.2): 30.1 */
			void host_pred_labels(s32 i, char *a, char *b); char la[64], lb[64];
			host_pred_labels(0, la, lb);
			assert(!strcmp(la, "40.0 u/s launch") && !strcmp(lb, "in 30.1 u/s"));
			{
				/* 6d.5: the launch arrow 40% of the seam arrow's - 40 u/s: 0.2 x root(40^2 / 8.34) = 2.77, capped at 1.6; 20 u/s:
				 * 1.38. The approach arrow flat on the floor, 0.6 long whatever the speed, along the drift (0, -2.2): from
				 * (3, 0.6) to the spot (3, 0), 0.02 above it */
				s32 host_ps_in_arrow(s32 i, float *tail, float *tip); float host_ps_launch_len(float up);
				float ta[3], ti[3];
				assert(fabsf(host_ps_launch_len(40.f) - 1.6f) < 1e-5f && fabsf(host_ps_launch_len(20.f) - 1.385f) < 0.01f);
				assert(host_ps_in_arrow(0, ta, ti) == 1);
				assert(fabsf(ti[0] - 3.f) < 1e-4f && fabsf(ti[1]) < 1e-4f && fabsf(ti[2] - (-0.98f)) < 1e-4f);
				assert(fabsf(ta[0] - 3.f) < 1e-4f && fabsf(ta[1] - 0.6f) < 1e-4f && ta[2] == ti[2]);
			}
		}
		{
			/* with a camera (at (-6, 0, 1) looking along +x - the whole arrows in view, the launch ones capped at 4 units up):
			 * every spot in view gets its two arrows - 3 lines each - and the nearest their labels */
			int q, launch = 0, entry = 0;
			WF(0x1800, -6.f); WF(0x1804, 0.f); WF(0x1808, 1.f);
			WF(0x1820, 1.f); WF(0x1824, 0.f); WF(0x1828, 0.f); WF(0x182C, 0.f); WF(0x1830, 0.f); WF(0x1834, 1.f);
			for (k = 0; k < 4; k++) { g_q = 0; frame(); }
			for (q = 0; q < g_nlines; q++) { if (strstr(g_lines[q], "u/s launch")) launch++; if (!strncmp(g_lines[q], "in ", 3) && strstr(g_lines[q], " u/s")) entry++; }
			fprintf(stderr, "arrows: quads %d, launch labels %d, approach labels %d\n", g_q, launch, entry);
			assert(g_q == 18 && launch == 3 && entry == 3);   /* 6d.6: all 3 in the default range (2000) - the one 49 ahead too */
			{
				/* 6d.4: every spot labelled, not the 3 nearest - four spots in view, four of each label */
				unsigned char b4[20 + 4 * 12]; float org[3] = { 0.f, 0.f, -2.f }; unsigned int c4 = 4; FILE *f4;
				short pos4[4][3] = { { 48, 0, 16 }, { 40, 8, 16 }, { 56, -8, 16 }, { 64, 8, 16 } };
				memcpy(b4, "SGP1", 4); memcpy(b4 + 4, &c4, 4); memcpy(b4 + 8, org, 12);
				for (q = 0; q < 4; q++) { unsigned char *r = b4 + 20 + 12 * q; memcpy(r, pos4[q], 6); r[6] = 120; r[7] = 51; r[8] = 0; r[9] = 30; r[10] = 0; r[11] = 0; }
				f4 = fopen("/tmp/sgtest/01.bin", "wb"); fwrite(b4, 1, sizeof(b4), f4); fclose(f4);
				{ void host_pred_reset(void); host_pred_reset(); }
				for (k = 0; k < 4; k++) { g_q = 0; frame(); }
				launch = entry = 0;
				for (q = 0; q < g_nlines; q++) { if (strstr(g_lines[q], "u/s launch")) launch++; if (!strncmp(g_lines[q], "in ", 3) && strstr(g_lines[q], " u/s")) entry++; }
				assert(launch == 4 && entry == 4);
				{
					/* 6d.6: 1500 spots in a row from the view - the nearest 1024 shown (the nearest itself among them), none
					 * farther than one distance band beyond any left out */
					static unsigned char bb[20 + 1500 * 12]; unsigned int cb = 1500; float ob[3] = { 0.f, 0.f, -2.f };
					s32 host_pred_near_d(s32 k, float *d); float dmax = 0.f, dmin_out = 1e30f, dd;
					int z, nn, pb2; float pdd;
					memcpy(bb, "SGP1", 4); memcpy(bb + 4, &cb, 4); memcpy(bb + 8, ob, 12);
					for (z = 0; z < 1500; z++) { unsigned char *r = bb + 20 + 12 * z; short pp[3] = { (short)(48 + z), 0, 16 }; memcpy(r, pp, 6); r[6] = 100; r[7] = 51; r[8] = 0; r[9] = 30; r[10] = 0; r[11] = 0; }
					f4 = fopen("/tmp/sgtest/01.bin", "wb"); fwrite(bb, 1, sizeof(bb), f4); fclose(f4);
					{ void host_pred_reset(void); host_pred_reset(); }
					for (k = 0; k < 4; k++) frame();
					host_pred(&nn, &pnear, &pb2, &pdd);
					assert(nn == 1500 && pnear == 1024 && pb2 == 0);
					for (z = 0; z < pnear; z++) { host_pred_near_d(z, &dd); if (dd > dmax) dmax = dd; }
					for (z = 0; z < 1500; z++) { float wx = 3.f + z / 16.f + 6.f; float d0 = sqrtf(wx * wx + 4.f);   /* from the view: (-6, 0, 1) */ int inl = 0, y2; for (y2 = 0; y2 < pnear; y2++) if (host_pred_near_d(y2, &dd) == z) { inl = 1; break; } if (!inl && d0 < dmin_out) dmin_out = d0; }
					fprintf(stderr, "range cap: 1500 spots -> %d shown, farthest shown %.2f, nearest left out %.2f\n", pnear, dmax, dmin_out);
					assert(dmax <= dmin_out + 0.01f);                                 /* nothing shown farther than anything left out */
				}
				f4 = fopen("/tmp/sgtest/01.bin", "wb"); { unsigned char b[20 + 3 * 12]; float o2[3] = { 0.f, 0.f, -2.f }; unsigned int c = 3;
				short pos[3][3] = { { 48, 0, 16 }, { 40, 8, 16 }, { 688, 0, 16 } }; unsigned char ceil4[3] = { 160, 100, 160 };
				memcpy(b, "SGP1", 4); memcpy(b + 4, &c, 4); memcpy(b + 8, o2, 12);
				for (q = 0; q < 3; q++) { unsigned char *r = b + 20 + 12 * q; memcpy(r, pos[q], 6); r[6] = ceil4[q]; r[7] = 51; r[8] = q == 1 ? 0 : 26; r[9] = 30; r[10] = 0; r[11] = (unsigned char)(signed char)-22; }
				fwrite(b, 1, sizeof(b), f4); } fclose(f4);
				{ void host_pred_reset(void); host_pred_reset(); }
				for (k = 0; k < 4; k++) frame();
			}
			WF(0x1800, 3.f); WF(0x1804, 0.f); WF(0x1808, 0.f);
		}
		{
			/* 6d.6: the range from your view - 10: the 2 near (1.0 and 1.2 off); 2000 (the default): all 3, the one 40 off too */
			void host_set_pred_range(s32 r);
			host_set_pred_range(10); for (k = 0; k < 4; k++) frame(); host_pred(&pn, &pnear, &pbest, &pd); assert(pnear == 2);
			host_set_pred_range(0); for (k = 0; k < 4; k++) frame(); host_pred(&pn, &pnear, &pbest, &pd); assert(pnear == 3 && pbest == 0);   /* 6h.18: Max */
		}
		host_set_pred(1, 1); for (k = 0; k < 4; k++) frame();                         /* 35+ only: the monster gone */
		host_pred(&pn, &pnear, &pbest, &pd); assert(pnear == 2 && pbest == 0);   /* both dreadnoughts (the default range), the monster gone */
		{ void host_pred_reset(void);
		host_set_spots_dir("/tmp/sgtest_none"); host_pred_reset(); host_set_pred(1, 0); frame();
		assert(host_pred(&pn, &pnear, &pbest, &pd) == 2);                            /* no folder */
		host_set_spots_dir("/tmp/sgtest_nomatch"); host_pred_reset(); frame();
		assert(host_pred(&pn, &pnear, &pbest, &pd) == 3);                            /* no file matches this level */
		host_set_pred(0, 0); host_pred_reset(); W32(0x1B00, 0); frame(); }
		printf("predicted spots: matched by geometry (8/8), 3 read, 2 near, nearest a dreadnought 1.0 wu; 35+ only; no folder / no match reported\n");
	}
	{
		/* 6e: the skulls - on: its byte set each frame (a new map clears them); off: cleared only if set here (one picked up
		 * in a level stays) */
		void host_set_skull(s32 i, s32 on); void host_apply_skulls(void);
		MEM[0x1D80 + 3] = 0; MEM[0x1D80 + 14] = 0; MEM[0x1D80 + 6] = 1;           /* Blind already picked up */
		host_set_skull(14, 1); host_apply_skulls(); assert(MEM[0x1D80 + 14] == 1 && MEM[0x1D80 + 3] == 0);
		MEM[0x1D80 + 14] = 0; host_apply_skulls(); assert(MEM[0x1D80 + 14] == 1);  /* a new map cleared it: set again */
		host_set_skull(14, 0); host_apply_skulls(); assert(MEM[0x1D80 + 14] == 0);
		host_set_skull(6, 0); host_apply_skulls(); assert(MEM[0x1D80 + 6] == 1);   /* not set here: left alone */
		MEM[0x1D80 + 6] = 0;
		printf("skulls: set while on (again after a new map), cleared when off, one picked up in a level left alone\n");
	}
	{
		/* 6e: Invincible Vehicles - every vehicle (type 1 in the object table) gets 0x4080 at +0x10A, the others nothing;
		 * off: each gets back what it had */
		void host_set_inv_veh(s32 on); void host_apply_inv_veh(void);
#define M16h(a) ((u32)MEM[(a)] | ((u32)MEM[(a) + 1] << 8))
		u32 pool = 0x4C000, data = 0x4C100, v1 = 0x4C400, v2 = 0x4C700, b1 = 0x4CA00;
		u32 opool = M32h(0x1004);
		memset(MEM + pool, 0, 0xD00);
		W32(pool + 0x44, data); W32(pool + 0x20, 3);
		W32(data + 0, 0x00000011); MEM[data + 3] = 1; W32(data + 8, v1);         /* a vehicle */
		W32(data + 12, 0x00000012); MEM[data + 15] = 0; W32(data + 20, b1);      /* a biped */
		W32(data + 24, 0x00000013); MEM[data + 27] = 1; W32(data + 32, v2);      /* a vehicle, already cannot-die (0x0080) */
		MEM[v2 + 0x10A] = 0x80;
		W32(0x1004, pool);
		host_set_inv_veh(1); host_apply_inv_veh();
		assert((M16h(v1 + 0x10A) & 0x4080) == 0x4080 && (M16h(v2 + 0x10A) & 0x4080) == 0x4080 && M16h(b1 + 0x10A) == 0);
		{
			/* 6e.1: the region damage skipped for the vehicles (their parts don't break), not for the biped; a stale or
			 * empty index: not skipped */
			s32 rdmg_skip(u32 object_index);
			assert(rdmg_skip(0x00110000u) == 1 && rdmg_skip(0x00130002u) == 1 && rdmg_skip(0x00120001u) == 0);
			assert(rdmg_skip(0x00990000u) == 0 && rdmg_skip(0xFFFFFFFFu) == 0 && rdmg_skip(0x00110007u) == 0);
		}
		host_set_inv_veh(0); host_apply_inv_veh();
		assert(M16h(v1 + 0x10A) == 0 && M16h(v2 + 0x10A) == 0x80);
		{ s32 rdmg_skip(u32 object_index); assert(rdmg_skip(0x00110000u) == 0); }   /* off: the game's own */
		W32(0x1004, opool);
		printf("invincible vehicles: both vehicles flagged, the biped not; off: each back as it was\n");
	}
	{
		/* 6h.29: Acrophobia (cheat_jetpack) - the jump is the game's (A pressed on the ground, held through it: nothing); A
		 * pressed in the air, then held: flying (looking along +x at rest: 9 u/s^2 for a tick, 0.3 u/s at 30 tick, gravity's
		 * tick back); let go or land: it ends, and only a new press in the air flies again; crouch in the air: hover (x -0.2,
		 * gravity back), crouch on the ground: nothing; crouch + a press in the air: flying; the menu and the free camera:
		 * nothing; once a tick */
		void host_set_acro(s32 on); void host_acro_tick(void); void host_set_p1(u16 b, u8 a);
		void host_set_menu_open(s32 open, s32 m); void host_set_free_cam(s32 on, s32 block);
		u32 obj = OBJECT, otime = M32h(0x1000), tk = 100;
		u8 oair = MEM[obj + 0x348];
		W32(0x1000, 0x4D000); memset(MEM + 0x4D000, 0, 16); MEM[0x4D002] = 30;                     /* game time: 30 tick */
		W32(obj + 0x14, 0xFFFFFFFFu);
		WF(0x1820, 1.f); WF(0x1824, 0.f); WF(0x1828, 0.f);                                           /* looking along +x */
		host_set_acro(1);
		#define ZERO() (WF(obj + 0x88, 0.f), WF(obj + 0x8C, 0.f), WF(obj + 0x90, 0.f))
		#define TICK(btn, a, air) (W32(0x4D008, ++tk), MEM[obj + 0x348] = (u8)(air), host_set_p1((btn), (a)), host_acro_tick())
		#define STILL() (RF(obj + 0x88) == 0.f && RF(obj + 0x90) == 0.f)
		#define FLEW() (fabsf(RF(obj + 0x88) - 0.3f) < 1e-4f && fabsf(RF(obj + 0x90) - 4.1712f / 30.f) < 1e-4f)
		TICK(0, 0, 0);                                                                                 /* (A up, on the ground) */
		ZERO(); TICK(0, 255, 0); assert(STILL());                                                      /* A on the ground: the game's jump */
		ZERO(); TICK(0, 255, 1); assert(STILL());                                                      /* in the air, A still held: a normal jump */
		ZERO(); TICK(0, 255, 1); assert(STILL());
		ZERO(); TICK(0, 0, 1); assert(STILL());                                                        /* let go: falling */
		ZERO(); TICK(0, 255, 1); assert(FLEW());                                                       /* pressed in the air: flying */
		W32(0x4D008, tk); ZERO(); host_acro_tick(); assert(STILL());                                   /* once a tick */
		ZERO(); TICK(0, 255, 1); assert(FLEW());                                                       /* held: flying on */
		ZERO(); TICK(0, 0, 1); assert(STILL());                                                        /* let go: it ends */
		ZERO(); TICK(0, 255, 1); assert(FLEW());                                                       /* pressed again: flying */
		ZERO(); TICK(0, 255, 0); assert(STILL());                                                      /* landed */
		ZERO(); TICK(0, 255, 1); assert(STILL());                                                      /* off the ground again, A still held: not flying */
		WF(obj + 0x88, 5.f); WF(obj + 0x8C, 0.f); WF(obj + 0x90, -3.f); TICK(0x0040, 0, 1);           /* crouch in the air: hover */
		assert(fabsf(RF(obj + 0x88) + 1.f) < 1e-4f && fabsf(RF(obj + 0x90) - (0.6f + 4.1712f / 30.f)) < 1e-4f);
		WF(obj + 0x88, 5.f); WF(obj + 0x90, -3.f); TICK(0x0040, 0, 0); assert(RF(obj + 0x88) == 5.f && RF(obj + 0x90) == -3.f);   /* crouch on the ground: a crouch */
		ZERO(); TICK(0x0040, 255, 1); assert(FLEW());                                                  /* crouch + a press in the air: flying */
		ZERO(); TICK(0, 0, 1);
		host_set_menu_open(1, 0); ZERO(); TICK(0, 255, 1); assert(STILL());                            /* the menu's A */
		ZERO(); TICK(0, 0, 1); host_set_menu_open(0, 0);
		host_set_free_cam(1, 1); ZERO(); TICK(0, 255, 1); assert(STILL());                             /* held by the free camera */
		host_set_free_cam(0, 1); ZERO(); TICK(0, 0, 1); ZERO(); TICK(0, 255, 1); assert(FLEW());
		#undef FLEW
		#undef STILL
		#undef TICK
		#undef ZERO
		host_set_acro(0); host_set_p1(0, 0); W32(0x1000, otime); MEM[obj + 0x348] = oair;
		printf("acrophobia: the jump the game's; a press in the air then held flies (9 u/s^2 along the view, gravity back), let go or land ends it; crouch in the air hovers, on the ground a crouch; not in the menu or the free camera\n");
	}
	{
		/* 6.f: Bump Possession - a biped no player controls, 0.5 from you: on the 3rd tick of the touch, player_set_unit(you,
		 * it); then a second's rest; the body you left not taken back while it's within 3; a dead or a player's biped never;
		 * a vehicle whose bounding sphere you're well inside, taken */
		void host_set_bump(s32 on); void host_bump_tick(void); s32 host_set_unit_calls(u32 *player, u32 *unit);
		const u32 NPC = 0x4E000, VEH = 0x4E400;
		u32 sp, su, otick = M32h(TIME + 8);
		unsigned char otps = MEM[TIME + 2];
		s32 n0, tk;
		memset(MEM + NPC, 0, 0x200); memset(MEM + VEH, 0, 0x200);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f); W32(OBJECT + 0x14, 0xFFFFFFFFu);
		W32(OBJ_DATA + 4 * 12, 0x0007); MEM[OBJ_DATA + 4 * 12 + 3] = 0; W32(OBJ_DATA + 4 * 12 + 8, NPC);
		WF(NPC + 0x64, 0.5f); WF(NPC + 0x68, 0.f); WF(NPC + 0x6C, 0.f); W32(NPC + 0x13C, 0xFFFFFFFFu);
		MEM[TIME + 2] = 30;
		host_set_bump(1);
		n0 = host_set_unit_calls(&sp, &su);
		W32(TIME + 8, 1000); host_bump_tick(); W32(TIME + 8, 1001); host_bump_tick();
		assert(host_set_unit_calls(&sp, &su) == n0);                             /* two ticks: not yet */
		W32(TIME + 8, 1002); host_bump_tick();
		assert(host_set_unit_calls(&sp, &su) == n0 + 1 && sp == 0x00010000u && su == 0x00070004u);
		/* (the game would now have you in it: here the player still has its old unit - the rest and the left body hold) */
		for (tk = 1003; tk < 1032; tk++) { W32(TIME + 8, (u32)tk); host_bump_tick(); }
		assert(host_set_unit_calls(&sp, &su) == n0 + 1);                         /* a second's rest */
		MEM[NPC + 0x10A] = 4; for (tk = 1032; tk < 1040; tk++) { W32(TIME + 8, (u32)tk); host_bump_tick(); }
		assert(host_set_unit_calls(&sp, &su) == n0 + 1);                         /* dead: never */
		MEM[NPC + 0x10A] = 0; W32(NPC + 0x13C, 0x00020001u); for (tk = 1040; tk < 1048; tk++) { W32(TIME + 8, (u32)tk); host_bump_tick(); }
		assert(host_set_unit_calls(&sp, &su) == n0 + 1);                         /* a player's: never */
		W32(NPC + 0x13C, 0xFFFFFFFFu); WF(NPC + 0x64, 9.f);                     /* the biped away */
		W32(OBJ_DATA + 5 * 12, 0x0009); MEM[OBJ_DATA + 5 * 12 + 3] = 1; W32(OBJ_DATA + 5 * 12 + 8, VEH);
		W32(VEH + 0x13C, 0xFFFFFFFFu); WF(VEH + 0x30, 1.f); WF(VEH + 0x34, 0.f); WF(VEH + 0x38, 0.5f); WF(VEH + 0x3C, 2.f);
		for (tk = 1048; tk < 1051; tk++) { W32(TIME + 8, (u32)tk); host_bump_tick(); }
		assert(host_set_unit_calls(&sp, &su) == n0 + 2 && su == 0x00090005u);  /* the vehicle: inside 75% of its sphere */
		{
			/* 6.f.1: Revert to player - the game has you in the vehicle now: back into the body you started in; in your own body,
			 * nothing. Passion Check: Invulnerability's flags cleared, the game's kill on the unit you're in */
			void host_revert_to_player(void); void host_passion_check(void); u32 host_bp_home(void); s32 host_kill_calls(u32 *unit);
			u32 ku; s32 k0 = host_kill_calls(&ku);
			W32(PL_DATA + 0x2C, 0x00090005u);
			assert(host_bp_home() == UNIT);
			host_revert_to_player();
			assert(host_set_unit_calls(&sp, &su) == n0 + 3 && sp == 0x00010000u && su == UNIT && host_bp_home() == 0xFFFFFFFFu);
			W32(PL_DATA + 0x2C, UNIT); host_revert_to_player();
			assert(host_set_unit_calls(&sp, &su) == n0 + 3);                        /* in your own body: nothing */
			MEM[OBJECT + 0x10A] = 0x80; MEM[OBJECT + 0x10B] = 0x40;               /* Invulnerability's 0x4080 */
			{
				/* 6h.12: a roll - 1 in 7 passes; over 700, the passes about 100 and each says so; a failed one kills as ever */
				const char *host_toast(void);
				int tries, kills0 = host_kill_calls(&ku), passes = 0;
				for (tries = 0; tries < 700; tries++)
				{
					s32 before = host_kill_calls(&ku);
					MEM[OBJECT + 0x10A] = 0x80; MEM[OBJECT + 0x10B] = 0x40;
					host_passion_check();
					if (host_kill_calls(&ku) == before) { passes++; assert(strstr(host_toast(), "Passion Check passed") && (M16h(OBJECT + 0x10A) & 0x4080) == 0x4080); }
					else assert(ku == UNIT && (M16h(OBJECT + 0x10A) & 0x4080) == 0 && strstr(host_toast(), "Passion Check failed"));
				}
				assert(passes >= 60 && passes <= 140 && host_kill_calls(&ku) == kills0 + 700 - passes);
				printf("passion check: a roll - %d of 700 passed (1 in 7 is 100), each said so; the rest killed you\n", passes);
				/* (6h.34: the roll comes from the cycle counter, so the last of the 700 can be either; end on a kill for the check
				 * below, whatever the counter did) */
				{
					s32 b2, g2 = 0;
					do { b2 = host_kill_calls(&ku); MEM[OBJECT + 0x10A] = 0x80; MEM[OBJECT + 0x10B] = 0x40; host_passion_check(); } while (host_kill_calls(&ku) == b2 && ++g2 < 100);
				}
				k0 = host_kill_calls(&ku) - 1;
			}
			assert(host_kill_calls(&ku) == k0 + 1 && ku == UNIT && (M16h(OBJECT + 0x10A) & 0x4080) == 0);
			printf("revert to player: back into the body you started in (nothing in your own); passion check: invulnerability cleared, the game's kill on your unit\n");
		}
		{
			/* 6h.4: Cycle Possession - the next biped / vehicle no player controls, alive, in the table's order after the
			 * last one taken, wrapping round; "Revert to player possession" keeps your own body */
			void host_cycle_possession(void); u32 host_bp_home(void); const char *host_toast(void);
			u32 first;
			W32(PL_DATA + 0x2C, UNIT); MEM[NPC + 0x10A] = 0; W32(NPC + 0x13C, 0xFFFFFFFFu);
			host_cycle_possession();
			assert(host_set_unit_calls(&sp, &su) == n0 + 4 && (su == 0x00070004u || su == 0x00090005u) && strstr(host_toast(), " of 2"));
			assert(host_bp_home() == UNIT);
			first = su;
			host_cycle_possession();
			assert(host_set_unit_calls(&sp, &su) == n0 + 5 && su != first && (su == 0x00070004u || su == 0x00090005u));
			host_cycle_possession();
			assert(host_set_unit_calls(&sp, &su) == n0 + 6 && su == first);           /* round again */
			MEM[NPC + 0x10A] = 4; MEM[VEH + 0x10A] = 4; host_cycle_possession();
			assert(host_set_unit_calls(&sp, &su) == n0 + 6 && strstr(host_toast(), "nothing to possess"));
			MEM[NPC + 0x10A] = 0; MEM[VEH + 0x10A] = 0;
			printf("cycle possession: the next unit no player controls (biped / vehicle), round the level; your body kept to revert to\n");
		}
		{
			/* 6h.12: Disable natural checkpoints - the checkpoint logic's save asked for is skipped (said so); off, it goes on.
			 * Disable reverts - a revert flag raised by the revert-after-death call is lowered again; one already up (Revert
			 * Checkpoint, set before) stays; off, it stays */
			void host_set_cp(s32 natural, s32 reverts); void natural_save_check(void); void death_revert_before(void); void death_revert_after(void);
			extern s32 g_cp_skip; const char *host_toast(void);
			host_set_cp(1, 0); natural_save_check(); assert(g_cp_skip == 1 && strstr(host_toast(), "Natural checkpoint skipped"));
			host_set_cp(0, 0); natural_save_check(); assert(g_cp_skip == 0);
			MEM[0x1CFF] = 0; host_set_cp(0, 1);
			death_revert_before(); MEM[0x1CFF] = 1; death_revert_after(); assert(MEM[0x1CFF] == 0 && strstr(host_toast(), "Revert on death stopped"));
			MEM[0x1CFF] = 1; death_revert_before(); death_revert_after(); assert(MEM[0x1CFF] == 1);     /* yours, set before: kept */
			MEM[0x1CFF] = 0; host_set_cp(0, 0);
			death_revert_before(); MEM[0x1CFF] = 1; death_revert_after(); assert(MEM[0x1CFF] == 1);     /* off: the game reverts */
			MEM[0x1CFF] = 0;
			printf("checkpoints: natural saves skipped with the option on; the revert on death lowered, your own revert kept\n");
		}
		{
			/* 6h.12: a possessed vehicle's following camera gets its driver seat's camera - seat 1 here (flagged driver and third
			 * person camera, 0x14; seat 0 a gunner's) at seat +0x60, its seat word 1, and the marker step; seated / a biped /
			 * third person off: untouched; no seat flagged driver: seat 0, no marker step */
			void cam_target_fix(u32 rec); extern u32 g_host_marker_calls;
			const u32 REC = 0x7F700, TAGT = 0x7F800, DEF = 0x7FA00, SEATS = 0x7FC00;
			u32 otag = M32h(OBJECT), otab = M32h(0x1B08), m0;
			unsigned char t0 = MEM[OBJECT + 0xAA]; u32 par = M32h(OBJECT + 0x14);
			memset(MEM + SEATS, 0, 0x200); memset(MEM + DEF, 0, 0x200);
			W32(0x1B08, TAGT); W32(TAGT + 5 * 16 + 8, DEF); W32(OBJECT, 0xE0000005u);
			W32(DEF + 0x1C8, 2); W32(DEF + 0x1CC, SEATS); W32(SEATS, 0x8); W32(SEATS + 0xB0, 0x14);
			MEM[OBJECT + 0xAA] = 1; W32(OBJECT + 0x14, 0xFFFFFFFFu);
			host_set_third_person(1, 10, 2, 0);
			W32(REC, UNIT); W32(REC + 4, 0xFFFF); W32(REC + 8, 0x1234);
			m0 = g_host_marker_calls;
			cam_target_fix(REC);
			assert(M32h(REC + 8) == SEATS + 0xB0 + 0x60 && (M32h(REC + 4) & 0xffff) == 0xFFFF && g_host_marker_calls == m0);   /* 6h.14: settings only */
			{
				extern s32 g_bp_switching_c; s32 cam_mode_answer(s32 game, u32 unit, u32 out);
				W32(REC + 8, 0x1234); g_bp_switching_c = 1;                          /* while the game switches bodies: its own answers */
				cam_target_fix(REC); assert(M32h(REC + 8) == 0x1234 && cam_mode_answer(0, UNIT, REC + 0x40) == 0);
				g_bp_switching_c = 0;
			}
			W32(REC + 4, 0x0002); W32(REC + 8, 0x1234); cam_target_fix(REC); assert(M32h(REC + 8) == 0x1234);    /* seated */
			W32(REC + 4, 0xFFFF); MEM[OBJECT + 0xAA] = 0; cam_target_fix(REC); assert(M32h(REC + 8) == 0x1234);  /* a biped */
			MEM[OBJECT + 0xAA] = 1; host_set_third_person(0, 10, 2, 0); cam_target_fix(REC); assert(M32h(REC + 8) == 0x1234);   /* off */
			host_set_third_person(1, 10, 2, 0); W32(SEATS + 0xB0, 0x8); m0 = g_host_marker_calls;
			cam_target_fix(REC); assert(M32h(REC + 8) == SEATS + 0x60 && (M32h(REC + 4) & 0xffff) == 0xFFFF && g_host_marker_calls == m0);
			W32(OBJECT, otag); W32(0x1B08, otab); MEM[OBJECT + 0xAA] = t0; W32(OBJECT + 0x14, par);
			printf("possessed vehicle: the following camera takes its driver seat's camera settings (no seat number, no marker step); not while switching\n");
		}
		{
			/* 6h.14: "your own body" - only the body you were in before possessing; possessing while dead (no unit) keeps none,
			 * and the next possession from there doesn't make the possessed unit "home"; a home that isn't a living biped or
			 * vehicle any more (its slot now a weapon): "gone", nothing switched */
			void host_cycle_possession(void); u32 host_bp_home(void); void host_revert_to_player(void); const char *host_toast(void);
			s32 n1;
			W32(PL_DATA + 0x2C, UNIT); host_revert_to_player();                       /* (in your own body: settles the state) */
			W32(PL_DATA + 0x2C, 0xFFFFFFFFu);                                         /* dead: no unit */
			MEM[NPC + 0x10A] = 0; MEM[VEH + 0x10A] = 0;
			host_cycle_possession(); assert(host_bp_home() == 0xFFFFFFFFu);
			W32(PL_DATA + 0x2C, su);                                                  /* now in the possessed unit */
			host_cycle_possession(); assert(host_bp_home() == 0xFFFFFFFFu);
			host_revert_to_player(); assert(strstr(host_toast(), "you're in your own body"));   /* nothing to go back to */
			W32(PL_DATA + 0x2C, UNIT);
			host_cycle_possession(); assert(host_bp_home() == UNIT);                  /* from your own body: kept */
			W32(PL_DATA + 0x2C, su); host_cycle_possession(); assert(host_bp_home() == UNIT);   /* and not replaced */
			{
				u32 hk = UNIT & 0xffff; unsigned char ty = MEM[OBJ_DATA + hk * 12 + 3];
				MEM[OBJ_DATA + hk * 12 + 3] = 2; n1 = host_set_unit_calls(&sp, &su);   /* its slot now a weapon */
				host_revert_to_player();
				assert(host_set_unit_calls(&sp, &su) == n1 && strstr(host_toast(), "your own body is gone") && host_bp_home() == 0xFFFFFFFFu);
				MEM[OBJ_DATA + hk * 12 + 3] = ty;
			}
			W32(PL_DATA + 0x2C, UNIT);
			{ void host_clear_toast(void); host_clear_toast(); }
			printf("possession: home is only your own body - not set while dead, never a possessed unit; a home no longer a unit refused\n");
		}
		host_set_bump(0);
		memset(MEM + OBJ_DATA + 4 * 12, 0, 24);
		W32(TIME + 8, otick); MEM[TIME + 2] = otps;
		printf("bump possession: a biped taken on the 3rd tick of the touch (player_set_unit(you, it)); a second's rest; dead / a player's never; a vehicle you're inside, taken\n");
	}
	{
		/* 6g: HUD Scaling Fix - on: the anamorphic factor 4/3 becomes 1.0; off: the game's own back; a factor already 1.0
		 * (an HD build) left alone */
		void host_set_hud_fix(s32 on); void host_apply_hud_fix(void); extern float spooky_anamorphic;
		u32 bits;
		W32(0x1DB0, 0x3FAAAAABu);                                                    /* 4/3, as the game has it */
		memcpy(&bits, &spooky_anamorphic, 4); assert(bits == 0x3FAAAAABu);         /* before anything: the game's own */
		host_set_hud_fix(1); host_apply_hud_fix(); assert(spooky_anamorphic == 1.f && M32h(0x1DB0) == 0x3FAAAAABu);
		host_set_hud_fix(0); host_apply_hud_fix(); memcpy(&bits, &spooky_anamorphic, 4); assert(bits == 0x3FAAAAABu && M32h(0x1DB0) == 0x3FAAAAABu);
		printf("hud scaling fix: the mod's factor 4/3 -> 1.0 while on, the game's own 4/3 back when off; the game's constant never written\n");
	}
	{
		/* 6g.2: a message shows with every overlay off (it returned early before: Launch's speed, Passion Check - nothing) */
		void host_set_sb_viewer(s32 on); void host_set_pred(s32 on, s32 dread); void host_req(s32 r); void spooky_main_frame(void); void host_set_oob(s32 on);
		host_set_sb_viewer(0); host_set_oob(0); host_set_pred(0, 0);
		g_q = 0; frame(); assert(!drawn("Revert to player"));
		host_req(8); spooky_main_frame(); frame();                                    /* a practice action with a message */
		assert(drawn("Revert to player"));
		host_set_sb_viewer(1);
		printf("messages: shown with every overlay off\n");
	}
	{
		/* 6h.3: the HUD Widescreen Fix - an element's anchor noted only inside its drawer, in a game (never the main menu),
		 * in widescreen, with the option on; a quad's corners pulled toward it by 3/4 (from a copy); a shader widget's
		 * grid: every point moved so, its texture coordinates untouched; a text's rectangle moved with the squeeze (its
		 * centre), never past the anchor; the motion tracker's quad pulled toward the bottom-left anchor */
		void host_set_hud_ws(s32 on); s32 host_hsq_on(void); void host_hsq_in(s32 in); s32 host_trk_on(void);
		float host_hsq_quad_x(u32 corners, s32 i);
		void hsq_anchor(const float *out); void hsq_grid(u32 v); void hsq_text_before(u32 rect, u32 frame);
		void hsq_track_begin(void); void hsq_track_end(void); void hsq_trkv(float *x);
		enum { Q = 0x7E000, G = 0x7E100, R = 0x7E200, GO = 0x46000 };
		float a[2] = { 40.f, 30.f }, xs[4] = { 100.f, 200.f, 200.f, 100.f }, tx;
		int i, r;
		W32(0x1D10, GO); W32(GO + 8, 1);                                               /* campaign */
		host_hsq_in(1);
		MEM[0x1A00] = 0; hsq_anchor(a); assert(!host_hsq_on());                       /* 4:3: never */
		MEM[0x1A00] = 1; host_set_hud_ws(0); hsq_anchor(a); assert(!host_hsq_on());     /* the option off */
		host_set_hud_ws(1);
		W32(GO + 8, 3); hsq_anchor(a); assert(!host_hsq_on());                         /* the main menu: never */
		W32(GO + 8, 2); hsq_anchor(a); assert(host_hsq_on());                          /* multiplayer: yes */
		W32(GO + 8, 1); host_hsq_in(0); hsq_anchor(a); assert(!host_hsq_on());         /* outside a drawer: never */
		host_hsq_in(1); hsq_anchor(a); assert(host_hsq_on());
		for (i = 0; i < 4; i++) { WF(Q + 8 * i, xs[i]); WF(Q + 8 * i + 4, 7.f); }
		assert(host_hsq_quad_x(Q, 0) == 85.f && host_hsq_quad_x(Q, 1) == 160.f && M32f(Q) == 100.f);   /* the copy; the game's left */
		WF(Q, 101.f); assert(host_hsq_quad_x(Q, 0) == 85.75f); WF(Q, 100.f);           /* 6h.13: exact again (no snapping) */
		{
			/* 6h.13: an element anchored bottom-left (kind 2): its right edge further out (6i.36: 4); 6i.41: the flat one (the
			 * shields bar) 1 left - so its left edge 1 left too; any other kind's edges as ever */
			void host_hsq_type(s32 t);
			host_hsq_type(2); hsq_anchor(a);
			assert(fabsf(host_hsq_quad_x(Q, 0) - 83.375f) < 0.01f && fabsf(host_hsq_quad_x(Q, 1) - 163.625f) < 0.01f && fabsf(host_hsq_quad_x(Q, 3) - 83.375f) < 0.01f && host_hsq_quad_y(Q, 0) == 7.f);   /* 6i.47: the bar 107% about its middle (122.5), 1 right, 0 down */
			{
				/* 6i.39: the frame ring - a squarish bottom-left quad - moved 3 left and 2 down (the defaults) on top of its
				 * right side's 4; the flat bar (above: y all 7) only the 4 */
				float host_hsq_quad_y(u32 corners, s32 i); static const float ys[4] = { 10.f, 10.f, 90.f, 90.f };
				for (i = 0; i < 4; i++) WF(Q + 8 * i + 4, ys[i]);
				assert(fabsf(host_hsq_quad_x(Q, 0) - 85.75f) < 0.01f && fabsf(host_hsq_quad_x(Q, 1) - 159.25f) < 0.01f && host_hsq_quad_y(Q, 0) == 11.f && host_hsq_quad_y(Q, 2) == 91.f);   /* 6i.45: 98% about its middle (122.5), 0 across, 1 down */
				for (i = 0; i < 4; i++) WF(Q + 8 * i + 4, 7.f);
			}
			host_hsq_type(1); hsq_anchor(a);
			assert(host_hsq_quad_x(Q, 1) == 160.f && host_hsq_quad_x(Q, 0) == 85.f);
			host_hsq_type(0); hsq_anchor(a);
		}
		/* the grid: viewport 0..640, the element's middle at 500 (anchor 600 - top right): each point x -> 600 + (x - 600) x 0.75 */
		a[0] = 600.f; hsq_anchor(a);
		for (r = 0; r < 3; r++)
		{
			u32 row = G + 48 * r; float s = 0.5f / 640.f;
			WF(row, 0.f); WF(row + 16, 500.f); WF(row + 32, 640.f);
			WF(row + 8, 1.f - 500.f * s); WF(row + 24, 1.f); WF(row + 40, 1.f - 140.f * s); WF(row + 12, 0.25f);
		}
		hsq_grid(G);
		for (r = 0; r < 3; r++)
		{
			u32 row = G + 48 * r; float s = 0.5f / 640.f;
			assert(M32f(row) == 150.f && M32f(row + 16) == 525.f && M32f(row + 32) == 630.f);
			assert(M32f(row + 8) == 1.f - 500.f * s && M32f(row + 24) == 1.f && M32f(row + 40) == 1.f - 140.f * s && M32f(row + 12) == 0.25f);
		}
		/* text (6h.4): moved by its placement point (the drawer's [ebp-8]) - the anchor at the right (600), a 60-wide text placed
		 * by its left edge at 520: (600 - 520) x 0.25 = 20 -> 540..600; with no plausible point, by its centre (550 -> 13) */
		{
			enum { FR = 0x7E300 };
			{ u32 w0 = (u32)(10) | ((u32)520 << 16), w1 = (u32)(30) | ((u32)580 << 16); W32(R, w0); W32(R + 4, w1); }
			W32(0x1248, 0); W32(0x124C, (640u << 16) | 480u); WF(FR - 8, 520.f);
			hsq_text_before(R, FR);
			assert((s16)(host_mem32(R) >> 16) == 540 && (s16)(host_mem32(R + 4) >> 16) == 600 && (s16)(host_mem32(0x124C) >> 16) == 640);
			{ u32 w0 = (u32)(10) | ((u32)520 << 16), w1 = (u32)(30) | ((u32)580 << 16); W32(R, w0); W32(R + 4, w1); }
			hsq_text_before(R, 0);                                                      /* by its centre: centred, left of a right anchor - by its middle */
			assert((s16)(host_mem32(R) >> 16) == 533 && (s16)(host_mem32(R + 4) >> 16) == 593);
			a[0] = 40.f; hsq_anchor(a);                                                 /* left (40): placed at 100 -> -15 -> 85..145 */
			{ u32 w0 = (u32)(10) | ((u32)100 << 16), w1 = (u32)(30) | ((u32)160 << 16); W32(R, w0); W32(R + 4, w1); }
			WF(FR - 8, 100.f); hsq_text_before(R, FR);
			assert((s16)(host_mem32(R) >> 16) == 85 && (s16)(host_mem32(R + 4) >> 16) == 145);
			{ u32 w0 = (u32)(10) | ((u32)100 << 16), w1 = (u32)(30) | ((u32)160 << 16); W32(R, w0); W32(R + 4, w1); }   /* right-justified at 160 */
			WF(FR - 8, 160.f); hsq_text_before(R, FR);
			assert((s16)(host_mem32(R) >> 16) == 70 && (s16)(host_mem32(R + 4) >> 16) == 130);
			/* 6h.9: centred (placed by its middle, 130) right of a left anchor (40): squeezed by the point halfway to its left
			 * edge (115): -18.75 -> 81..141 (6h.4 by its middle: 77; 6h.5 by its edge: 85) */
			{ u32 w0 = (u32)(10) | ((u32)100 << 16), w1 = (u32)(30) | ((u32)160 << 16); W32(R, w0); W32(R + 4, w1); }
			WF(FR - 8, 130.f); hsq_text_before(R, FR);
			assert((s16)(host_mem32(R) >> 16) == 81 && (s16)(host_mem32(R + 4) >> 16) == 141);
			a[0] = 320.f; hsq_anchor(a);                                                /* centred around its anchor (the middle): stays */
			{ u32 w0 = (u32)(10) | ((u32)270 << 16), w1 = (u32)(30) | ((u32)370 << 16); W32(R, w0); W32(R + 4, w1); }
			WF(FR - 8, 320.f); hsq_text_before(R, FR);
			assert((s16)(host_mem32(R) >> 16) == 270 && (s16)(host_mem32(R + 4) >> 16) == 370);
			a[0] = 600.f; hsq_anchor(a);                                                /* 6h.10: centred left of a right anchor - by its middle (the right count, as 6h.4) */
			{ u32 w0 = (u32)(10) | ((u32)500 << 16), w1 = (u32)(30) | ((u32)560 << 16); W32(R, w0); W32(R + 4, w1); }
			WF(FR - 8, 530.f); hsq_text_before(R, FR);
			assert((s16)(host_mem32(R) >> 16) == 518 && (s16)(host_mem32(R + 4) >> 16) == 578);
			host_hsq_in(0); assert(!host_hsq_on());
			{ u32 w0 = (u32)(10) | ((u32)100 << 16); W32(R, w0); hsq_text_before(R, FR); assert((s16)(host_mem32(R) >> 16) == 100); }   /* over: nothing */
		}
		assert(host_hsq_quad_x(Q, 0) == 100.f);
		/* the motion tracker: its quad's x's toward the frame's left (32) while it draws; nothing after */
		W32(0x1250, 32u << 16);
		hsq_track_begin(); assert(host_trk_on());
		tx = 132.f; hsq_trkv(&tx); assert(tx == 107.f);
		hsq_track_end(); tx = 132.f; hsq_trkv(&tx); assert(tx == 132.f && !host_trk_on());
		MEM[0x1A00] = 0; hsq_track_begin(); assert(!host_trk_on());                    /* 4:3: never */
		printf("HUD Widescreen Fix: anchors only inside a drawer, in a game, in widescreen; quads and shader grids squeezed 3/4 around them (texture coordinates untouched), text moved with them, the tracker's quad too\n");
	}
	{
		/* 6h.19: the ghost seam file's text - whole lines only, never past its buffer, even with the longest lines (a 47-letter
		 * map name, numbers down to -2147483648); 96 short entries fit whole */
		u32 host_ghosts_text(char *buf, u32 cap); void host_ghost_add(const char *map, s32 tries); void host_ghosts_reset(void);
		static char buf[96 * 112 + 64];
		u32 n, cap = 96 * 112, k;
		int lines = 0;
		host_ghosts_reset();
		for (k = 0; k < 96; k++) host_ghost_add("scenarios_multi_headlong_headlong_xxxxxxxxxxxxxxxxxxxxxxxx", -2147483647 - 1);
		memset(buf, 0x5A, sizeof(buf));
		n = host_ghosts_text(buf, cap);
		for (k = 0; k < n; k++) if (buf[k] == '\n') lines++;
		assert(n <= cap && n + 160 >= cap && buf[n - 1] == '\n' && buf[cap] == 0x5A && strstr(buf, "-2147483648"));
		host_ghosts_reset();
		for (k = 0; k < 96; k++) host_ghost_add("headlong", 3);
		n = host_ghosts_text(buf, cap);
		for (lines = 0, k = 0; k < n; k++) if (buf[k] == '\n') lines++;
		assert(lines == 96 && n <= cap);
		host_ghosts_reset();
		printf("ghost seam file: built within its buffer (longest lines: %d whole lines, %u of %u bytes); 96 short ones whole\n", lines, n, cap);
	}
	{
		/* 6h.20: the search rate - every 2nd frame under 9 ms a search, 3rd over 9, 4th over 15, 6th over 30; faster again only
		 * under 70% of the bar that slowed it: 12-17 ms (the 360's) stays at every 4th once there, 8-14 (the Xbox's) at 3rd */
		s32 search_div_for(u32 per, s32 cur);
		assert(search_div_for(5000, 2) == 2 && search_div_for(9500, 2) == 3 && search_div_for(16000, 2) == 4 && search_div_for(31000, 2) == 6);
		assert(search_div_for(12000, 4) == 4 && search_div_for(13000, 4) == 4 && search_div_for(17000, 4) == 4);   /* the 360: no flipping */
		assert(search_div_for(10000, 4) == 3 && search_div_for(6000, 4) == 2);      /* clearly cheaper: faster */
		assert(search_div_for(8000, 3) == 3 && search_div_for(6000, 3) == 2 && search_div_for(14000, 3) == 3);
		assert(search_div_for(25000, 6) == 6 && search_div_for(20000, 6) == 4);
		printf("search rate: every 2nd / 3rd / 4th / 6th frame by cost, back faster only once clearly cheaper (no 4-2-4 flipping)\n");
	}
	{
		/* 6h.21: with the development lines off (as on the consoles): a launch, a bounce and its peak window, the ground probe /
		 * seam census and the crash trace write nothing; the bounce still counts and shows */
		void host_set_diag(s32 on); const char *host_trace_last(void); void host_req(s32 r); void host_clear_toast(void);
		int b0 = logged("superbounce: bounce"), p0 = logged("practice: bodies"), l0 = logged("practice: launch"), g0 = logged("ground probe"), c0 = logged("seam census");
		char tr[64];
		int k;
		strncpy(tr, host_trace_last(), 63); tr[63] = 0;
		host_set_diag(0);
		state(900, -10.f, 1, 1); frame();
		host_req(3); frame();                                                   /* a launch */
		state(901, -10.f, 1, 1); frame(); state(902, 20.f, 1, 1); frame();        /* a bounce */
		for (k = 0; k < 6; k++) { state(903 + k, 18.f - k, 1, 1); frame(); }      /* its peak window */
		assert(drawn("Last Bounce: 20.6u/s"));
		assert(logged("superbounce: bounce") == b0 && logged("practice: bodies") == p0 && logged("practice: launch") == l0);
		assert(logged("ground probe") == g0 && logged("seam census") == c0 && !strcmp(host_trace_last(), tr));
		host_set_diag(1); host_clear_toast();
		printf("development lines off (the consoles): launches, bounces, probes and the crash trace write nothing; the HUD as ever\n");
	}
	{
		/* 6h.22 / 6h.24: Game Speed / Pause Game - the game's time globals, each frame before its time update. Always 30
		 * ticks a second of 1/30 s; the speed sets how many run. A stale rate (a revert's, a 6h.23 checkpoint's 60 / 300
		 * ticks) goes; a script's rate and blend are the game's own */
		void host_set_speed(s32 tenths, s32 pause);
		const u32 GO = 0x4D000;
		u32 oopt = M32h(0x1D10), otick = M32h(TIME + 8), o0 = M32h(TIME), o18 = M32h(TIME + 0x18), o20 = M32h(TIME + 0x20);
		float sp, spt;
		int tps;
		#define RATE() (memcpy(&sp, MEM + TIME + 0xC, 4), memcpy(&spt, MEM + TIME + 4, 4), tps = (int)(short)M16h(TIME + 2))
		#define IS(T, S) (RATE(), tps == (T) && sp == (S) && spt == 1.f / (float)(T))
		W32(0x1D10, GO); W32(GO + 8, 1); MEM[GO + 0xE] = 30; MEM[GO + 0xF] = 0;      /* a campaign game, 30 ticks a second */
		MEM[TIME] = 1; MEM[TIME + 1] = 0; MEM[TIME + 2] = 30; MEM[TIME + 3] = 0; WF(TIME + 4, 1.f / 30.f); WF(TIME + 0xC, 1.f);
		WF(TIME + 0x18, 0.f); WF(TIME + 0x20, 0.f);
		frame(); assert(IS(30, 1.f));                                              /* 1.0, off: the game's own, untouched */
		host_set_speed(5, 0); frame(); assert(IS(30, 0.5f));                      /* 0.5: 30 ticks a second still, half as many run */
		assert(MEM[TIME] == 1 && MEM[TIME + 1] == 0);                             /* the flags beside the tick rate kept */
		host_set_speed(1, 0); frame(); assert(IS(30, 0.1f));                      /* 0.1 */
		host_set_speed(8, 0); frame(); assert(IS(30, 0.8f));                      /* 0.8 */
		host_set_speed(5, 1); frame(); assert(IS(30, 0.f));                       /* paused */
		frame(); frame(); assert(IS(30, 0.f));
		host_set_speed(5, 0); frame(); assert(IS(30, 0.5f));                      /* unpaused: back to 0.5 */
		WF(TIME + 0xC, 1.f); frame(); assert(IS(30, 0.5f));                       /* a level load's 1.0: ours again */
		MEM[TIME + 2] = 60; WF(TIME + 4, 1.f / 60.f); frame(); assert(IS(30, 0.5f));   /* a 6h.23 checkpoint's 60 ticks: 30 again */
		MEM[TIME + 2] = 44; MEM[TIME + 3] = 1; WF(TIME + 4, 1.f / 300.f); frame(); assert(IS(30, 0.5f));   /* its 300: 30 again */
		host_set_speed(20, 0); frame(); assert(IS(30, 2.f));                      /* 2.0: two 1/30 s ticks a frame */
		host_set_speed(50, 0); frame(); assert(IS(30, 5.f));                      /* 5.0 */
		host_set_speed(0, 0); frame(); assert(IS(30, 0.f));                       /* 0.0: stopped */
		host_set_speed(10, 0); frame(); assert(IS(30, 1.f));                      /* back to 1.0 */
		MEM[TIME + 2] = 60; WF(TIME + 4, 1.f / 60.f); WF(TIME + 0xC, 0.5f);        /* a revert brings 6h.23's 0.5 back ... */
		frame(); assert(IS(30, 1.f));                                              /* ... 30 ticks at 1.0 again */
		MEM[TIME + 2] = 60; WF(TIME + 4, 1.f / 60.f); WF(TIME + 0xC, 0.5f); WF(TIME + 0x20, 0.5f);   /* a script's 0.5 */
		frame(); assert(IS(60, 0.5f));                                             /* left: the game's own */
		WF(TIME + 0x20, 0.f);
		MEM[TIME + 2] = 30; WF(TIME + 4, 1.f / 30.f); WF(TIME + 0x18, 2.f); WF(TIME + 0xC, 0.7f);
		frame(); RATE(); assert(sp == 0.7f);                                       /* a script's blend under way: left */
		host_set_speed(5, 0); frame(); RATE(); assert(sp == 0.7f && tps == 30);    /* ... even with 0.5 asked for */
		WF(TIME + 0x18, 0.f); frame(); assert(IS(30, 0.5f));                      /* blend over: 0.5 */
		host_set_speed(1, 1); frame(); assert(IS(30, 0.f));                       /* paused at 0.1 */
		W32(GO + 8, 3); frame(); assert(IS(30, 1.f));                             /* the game's main menu: the game's rate back */
		frame(); assert(IS(30, 1.f));
		W32(GO + 8, 2); frame(); assert(IS(30, 0.f));                             /* a multiplayer game: applies */
		W32(GO + 8, 1);
		host_set_speed(10, 0); frame(); assert(IS(30, 1.f));
		MEM[TIME] = 0; host_set_speed(5, 0); frame(); assert(IS(30, 1.f));        /* no game time yet: nothing */
		MEM[TIME] = 1; W32(0x1D10, 0); frame(); assert(IS(30, 1.f));              /* no game options: nothing */
		host_set_speed(10, 0);
		W32(0x1D10, oopt); W32(TIME + 8, otick); W32(TIME, o0); MEM[TIME + 2] = 30; MEM[TIME + 3] = 0;
		WF(TIME + 0xC, 1.f); W32(TIME + 0x18, o18); W32(TIME + 0x20, o20);
		frame();
		#undef IS
		#undef RATE
		printf("game speed: 30 ticks of 1/30 s at every speed (0.1 .. 5.0, paused, stopped); a stale tick rate (6h.23's 60 / 300) put back to 30; a script's rate and blend left alone; nothing on the main menu or before a game\n");
	}
	{
		/* 6h.25: smooth slow motion at 30 ticks - an object's node matrices snapshotted before a tick, drawn between the two
		 * ticks by leftover_ticks, the real ones back after the draw; the camera's eye the same way; none of it at full
		 * speed, paused, after a jump, two ticks on, or for another object */
		void host_set_speed(s32 tenths, s32 pause); void ip_tick_before(void); void ip_draw_before(void); void ip_draw_after(void);
		s32 host_ip_blended(void); s32 host_ip_snapped(void); u32 tp_camera(u32 cam, s32 user);
		const u32 GO = 0x4D000, NODES = OBJECT + 0x500;
		static unsigned char save[0x600], tsave[0x40], csave[0x40];
		u32 oopt = M32h(0x1D10), b;
		float v;
		#define NPOS(n, i) (memcpy(&v, MEM + NODES + 0x34 * (n) + 0x28 + 4 * (i), 4), v)
		#define SETPOS(n, x, y, z) (WF(NODES + 0x34 * (n) + 0x28, x), WF(NODES + 0x34 * (n) + 0x2C, y), WF(NODES + 0x34 * (n) + 0x30, z))
		#define NEAR(a, b) ((a) - (b) < 1e-4f && (b) - (a) < 1e-4f)
		memcpy(save, MEM + OBJECT, sizeof(save)); memcpy(tsave, MEM + TIME, sizeof(tsave)); memcpy(csave, MEM + 0x1800, sizeof(csave));
		W32(0x1D10, GO); W32(GO + 8, 1); MEM[GO + 0xE] = 30; MEM[GO + 0xF] = 0;
		MEM[TIME] = 1; MEM[TIME + 1] = 0; MEM[TIME + 2] = 30; MEM[TIME + 3] = 0; WF(TIME + 4, 1.f / 30.f); WF(TIME + 0x18, 0.f); WF(TIME + 0x20, 0.f);
		WF(0x1800, 0.f); WF(0x1804, 0.f); WF(0x1808, 0.f);                          /* the camera at the origin */
		MEM[OBJECT + 0x114] = 0x68; MEM[OBJECT + 0x115] = 0; MEM[OBJECT + 0x116] = 0x00; MEM[OBJECT + 0x117] = 0x05;   /* 2 nodes at +0x500 */
		WF(OBJECT + 0x30, 1.f); WF(OBJECT + 0x34, 0.f); WF(OBJECT + 0x38, 0.f); WF(OBJECT + 0x3C, 1.f);   /* near the camera */
		memset(MEM + NODES, 0, 0x68); WF(NODES, 1.f); WF(NODES + 0x34, 1.f); SETPOS(0, 1.f, 2.f, 3.f); SETPOS(1, 4.f, 5.f, 6.f);
		W32(TIME + 8, 1000); WF(TIME + 0x10, 0.f);
		host_set_speed(5, 0);
		ip_tick_before(); assert(host_ip_snapped() >= 1);                        /* 0.5: snapshotted before the tick */
		SETPOS(0, 1.6f, 2.f, 3.f); SETPOS(1, 4.6f, 5.f, 6.f); W32(TIME + 8, 1001); WF(TIME + 0x10, 0.25f);   /* the tick */
		ip_draw_before(); assert(host_ip_blended() == 1 && NEAR(NPOS(0, 0), 1.15f) && NEAR(NPOS(1, 0), 4.15f) && NEAR(NPOS(0, 1), 2.f));
		ip_draw_after(); assert(NPOS(0, 0) == 1.6f && NPOS(1, 0) == 4.6f && host_ip_blended() == 0);   /* the real ones back, exactly */
		/* 6i.9: your first-person weapon's node matrices (camera-relative, not an object's) snapshotted and blended the
		 * same way, ahead of everything; an inactive or unarmed weapon left out */
		{
			const u32 FPU = 0x7E000, FW = FPU + 0xC, FN = FW + 0x308;
			s32 n0;
			#define FPOS(n, i) (memcpy(&v, MEM + FN + 0x34 * (n) + 0x28 + 4 * (i), 4), v)
			memset(MEM + FPU, 0, 0x2100); W32(0x1E90, FPU); W32(FW, 1); W32(FW + 4, 0x00050007); W32(FW + 0x304, 2);
			WF(FN + 0x28, 0.1f); WF(FN + 0x34 + 0x28, 0.3f);
			W32(TIME + 8, 1000); WF(TIME + 0x10, 0.f); ip_tick_before(); n0 = host_ip_snapped(); assert(n0 >= 2);
			WF(FN + 0x28, 0.5f); WF(FN + 0x34 + 0x28, 0.7f); W32(TIME + 8, 1001); WF(TIME + 0x10, 0.25f);
			ip_draw_before(); assert(host_ip_blended() >= 1 && NEAR(FPOS(0, 0), 0.2f) && NEAR(FPOS(1, 0), 0.4f));   /* a quarter of the way on */
			ip_draw_after(); assert(FPOS(0, 0) == 0.5f && FPOS(1, 0) == 0.7f);
			/* 6i.204: the hand takes another weapon on a tick (the block's weapon handle changes, the node count the same):
			 * its nodes of the tick before are the other weapon's - not blended, drawn as the game has them; the tick after,
			 * with the same weapon, blended again */
			WF(FN + 0x28, 0.5f); WF(FN + 0x34 + 0x28, 0.7f); W32(TIME + 8, 1001); ip_tick_before();
			W32(FW + 4, 0x00090011); WF(FN + 0x28, 2.5f); WF(FN + 0x34 + 0x28, 2.7f); W32(TIME + 8, 1002); WF(TIME + 0x10, 0.5f);
			ip_draw_before(); assert(FPOS(0, 0) == 2.5f && FPOS(1, 0) == 2.7f); ip_draw_after(); assert(FPOS(0, 0) == 2.5f);
			ip_tick_before();
			WF(FN + 0x28, 2.9f); WF(FN + 0x34 + 0x28, 3.1f); W32(TIME + 8, 1003); WF(TIME + 0x10, 0.5f);
			ip_draw_before(); assert(NEAR(FPOS(0, 0), 2.7f) && NEAR(FPOS(1, 0), 2.9f)); ip_draw_after(); assert(FPOS(0, 0) == 2.9f && FPOS(1, 0) == 3.1f);
			W32(FW + 4, 0x00050007); WF(FN + 0x28, 0.5f); WF(FN + 0x34 + 0x28, 0.7f);
			W32(FW, 0); W32(TIME + 8, 1000); ip_tick_before(); assert(host_ip_snapped() == n0 - 1);         /* inactive: left out */
			W32(FW, 1); W32(FW + 4, 0xFFFFFFFFu); ip_tick_before(); assert(host_ip_snapped() == n0 - 1);  /* unarmed */
			W32(FW + 4, 0x00050007); W32(FW + 0x304, 65); ip_tick_before(); assert(host_ip_snapped() == n0 - 1);   /* too many nodes */
			W32(0x1E90, 0);                                                                             /* the object's snapshot as it was */
			SETPOS(0, 1.f, 2.f, 3.f); SETPOS(1, 4.f, 5.f, 6.f); W32(TIME + 8, 1000); WF(TIME + 0x10, 0.f); ip_tick_before();
			SETPOS(0, 1.6f, 2.f, 3.f); SETPOS(1, 4.6f, 5.f, 6.f); W32(TIME + 8, 1001);
			#undef FPOS
		}
		WF(TIME + 0x10, 0.75f); ip_draw_before(); assert(NEAR(NPOS(0, 0), 1.45f)); ip_draw_after(); assert(NPOS(0, 0) == 1.6f);
		W32(TIME + 8, 1002); ip_draw_before(); assert(host_ip_blended() == 0 && NPOS(0, 0) == 1.6f); ip_draw_after();   /* two ticks on */
		W32(TIME + 8, 1001);
		host_set_speed(5, 1); ip_draw_before(); assert(host_ip_blended() == 0); ip_draw_after();                       /* paused */
		host_set_speed(10, 0); ip_draw_before(); assert(host_ip_blended() == 0); ip_draw_after();                      /* 1.0 */
		W32(TIME + 8, 1000); ip_tick_before(); assert(host_ip_snapped() == -1);                                        /* 1.0: no snapshot */
		host_set_speed(5, 0); W32(TIME + 8, 1000); ip_tick_before();
		SETPOS(0, 6.6f, 2.f, 3.f); W32(TIME + 8, 1001); ip_draw_before(); assert(host_ip_blended() == 0 && NPOS(0, 0) == 6.6f); ip_draw_after();   /* a 5-unit jump */
		W32(TIME + 8, 1000); ip_tick_before(); W32(TIME + 8, 1001);
		ip_draw_before(); assert(host_ip_blended() == 0); ip_draw_after();                                             /* still: nothing to blend */
		W32(TIME + 8, 1000); ip_tick_before(); SETPOS(0, 7.f, 2.f, 3.f); W32(TIME + 8, 1001);
		{ u32 hs = M32h(OBJ_DATA + 2 * 12); W32(OBJ_DATA + 2 * 12, (hs & 0xffff0000u) | 6); ip_draw_before(); assert(host_ip_blended() == 0); ip_draw_after(); W32(OBJ_DATA + 2 * 12, hs); }   /* another object there */
		WF(OBJECT + 0x30, 200.f); W32(TIME + 8, 1000); ip_tick_before(); assert(host_ip_snapped() == 1); WF(OBJECT + 0x30, 1.f);   /* far: 6i.1 - your own unit taken anyway (it carries the view; other objects far: not - the 6i.1 test) */
		/* 6h.26: the camera moves with its own unit's blend - the eye by what the unit's root node is moved by - in first
		 * person and behind it in third; the first-person legs turned to the view's heading about the eye (slowed, on foot,
		 * first person only) and put back after the first-person build */
		{
			const u32 REC = 0x6A000;
			void host_set_third_person(s32 on, s32 dist, s32 height, s32 side); void host_set_free_cam(s32 on, s32 block);
			void ip_fp_before(void); void ip_fp_after(void);
			float x1, x2, fx, fy;
			host_set_third_person(0, 10, 2, 0); host_set_free_cam(0, 1);       /* first person */
			memset(MEM + REC, 0, 0x358); WF(REC + 0x20, 1.f); WF(REC + 0x34, 1.f);  /* looking along +x, up +z */
			WF(OBJECT + 0x30, 1.f); MEM[OBJECT + 0x14] = 0xFF; MEM[OBJECT + 0x15] = 0xFF; MEM[OBJECT + 0x16] = 0xFF; MEM[OBJECT + 0x17] = 0xFF;   /* on foot */
			WF(OBJECT + 0x70, 1.f); WF(OBJECT + 0x74, 0.f); WF(OBJECT + 0x78, 0.f);   /* facing +x */
			host_set_speed(5, 0);
			SETPOS(0, 1.f, 2.f, 3.f); SETPOS(1, 4.f, 5.f, 6.f); W32(TIME + 8, 3000); WF(TIME + 0x10, 0.f);
			WF(NODES + 0x04, 1.f); WF(NODES + 0x08, 0.f); WF(NODES + 0x0C, 0.f);      /* node 0 faces +x */
			WF(OBJECT + 0x64, 1.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);   /* its origin */
			ip_tick_before();
			SETPOS(0, 1.6f, 2.07f, 3.f); SETPOS(1, 4.6f, 5.f, 6.f); W32(TIME + 8, 3001); WF(TIME + 0x10, 0.25f);   /* the pelvis bobs (y), the origin doesn't */
			WF(OBJECT + 0x64, 1.6f);
			WF(REC, 10.f); WF(REC + 4, 0.f); WF(REC + 8, 0.f);
			ip_draw_before();
			b = tp_camera(REC, 0); memcpy(&v, MEM + b, 4);
			assert(b != REC && NEAR(v, 10.f - 0.45f));                           /* the eye moved as the root: (1.0 - 1.6) x 0.75 */
			{ float r; memcpy(&r, MEM + REC, 4); assert(r == 10.f); }           /* the game's record untouched */
			ip_draw_after();
			b = tp_camera(REC, 0); assert(b == REC);                            /* outside the draw: as it is */
			WF(TIME + 0x10, 0.75f); ip_draw_before(); b = tp_camera(REC, 0); memcpy(&v, MEM + b, 4); assert(NEAR(v, 10.f - 0.15f)); ip_draw_after();
			/* the record moving within the tick (pitch, a shake) changes nothing: the offset is the root's */
			WF(REC, 10.3f); ip_draw_before(); b = tp_camera(REC, 0); memcpy(&v, MEM + b, 4); assert(NEAR(v, 10.3f - 0.15f)); ip_draw_after();
			/* third person: its camera from that eye, 1 unit behind it along the view */
			host_set_third_person(1, 10, 2, 0);
			MEM[OBJECT + 0xAA] = 0; MEM[OBJECT + 0x240] = 0xFF; MEM[OBJECT + 0x241] = 0xFF;
			WF(REC, 1.6f);                                                      /* the eye at your origin (the game's, roughly) */
			WF(TIME + 0x10, 0.25f); ip_draw_before(); b = tp_camera(REC, 0); memcpy(&x1, MEM + b, 4); ip_draw_after();
			WF(TIME + 0x10, 0.75f); ip_draw_before(); b = tp_camera(REC, 0); memcpy(&x2, MEM + b, 4); ip_draw_after();
			assert(NEAR(x1, 1.6f - 0.45f - 1.f) && NEAR(x2, 1.6f - 0.15f - 1.f));
			W32(0x1D38, 0); WF(TIME + 0x10, 0.25f); ip_draw_before(); tp_camera(REC, 0); ip_fp_before(); assert(NEAR(NPOS(0, 0), 1.15f) && NEAR(NPOS(0, 1), 2.0175f)); ip_fp_after(); ip_draw_after();   /* third person: no turn */
			host_set_third_person(0, 10, 2, 0); WF(REC, 10.f);
			/* 6i.15: two ticks begun with the view along +x (the views the nodes were made with) */
			{
				int q;
				for (q = 0; q < 2; q++)
				{
					WF(REC + 0x20, 1.f); WF(REC + 0x24, 0.f); WF(REC + 0x28, 0.f); WF(REC + 0x2C, 0.f); WF(REC + 0x30, 0.f); WF(REC + 0x34, 1.f);
					ip_draw_before(); tp_camera(REC, 0); ip_draw_after();
					W32(TIME + 8, 3000); SETPOS(0, 1.f, 2.f, 3.f); SETPOS(1, 4.f, 5.f, 6.f); WF(OBJECT + 0x64, 1.f); ip_tick_before();
					SETPOS(0, 1.6f, 2.07f, 3.f); SETPOS(1, 4.6f, 5.f, 6.f); WF(OBJECT + 0x64, 1.6f); W32(TIME + 8, 3001);
				}
			}
			/* the view along +x still: the legs as the nodes have them */
			WF(TIME + 0x10, 0.25f); ip_draw_before(); tp_camera(REC, 0); ip_fp_before(); assert(NEAR(NPOS(0, 0), 1.15f) && NEAR(NPOS(0, 1), 2.0175f)); ip_fp_after(); ip_draw_after();
			/* the view since turned to +y: the legs turned by that, +90 degrees about the eye (10 - 0.45, 0), back after the build */
			WF(REC + 0x20, 0.f); WF(REC + 0x24, 1.f); WF(REC + 0x28, 0.f); WF(REC + 0x2C, 0.f); WF(REC + 0x30, 0.f); WF(REC + 0x34, 1.f);
			WF(TIME + 0x10, 0.25f); ip_draw_before(); tp_camera(REC, 0);
			ip_fp_before();
			memcpy(&fx, MEM + NODES + 0x04, 4); memcpy(&fy, MEM + NODES + 0x08, 4);
			{
				/* node 0 blended to (1.15, 2.0175, 3); about the eye (9.55, 0): (-8.4, 2.0175) turned +90 -> (-2.0175, -8.4) */
				assert(NEAR(NPOS(0, 0), 9.55f - 2.0175f) && NEAR(NPOS(0, 1), -8.4f) && NPOS(0, 2) == 3.f);
				assert(NEAR(fx, 0.f) && NEAR(fy, 1.f));                           /* its forward (+x) turned to +y */
			}
			ip_fp_after(); assert(NEAR(NPOS(0, 0), 1.15f) && NEAR(NPOS(0, 1), 2.0175f));   /* back: the blended pose */
			ip_draw_after(); assert(NPOS(0, 0) == 1.6f && NPOS(0, 1) == 2.07f);   /* and the real one */
			/* looking straight down (forward -z, up along +y): the heading from up - the same turn */
			WF(REC + 0x20, 0.f); WF(REC + 0x24, 0.f); WF(REC + 0x28, -1.f); WF(REC + 0x2C, 0.f); WF(REC + 0x30, 1.f); WF(REC + 0x34, 0.f);
			ip_draw_before(); tp_camera(REC, 0); ip_fp_before(); memcpy(&fy, MEM + NODES + 0x08, 4); assert(NEAR(fy, 1.f)); ip_fp_after(); ip_draw_after();
			/* full speed: no turn */
			host_set_speed(10, 0); ip_draw_before(); tp_camera(REC, 0); ip_fp_before(); assert(NPOS(0, 0) == 1.6f && NPOS(0, 1) == 2.07f); ip_fp_after(); ip_draw_after();
			host_set_speed(5, 0);
			/* 6i.15 / 6i.30: the view turning as the ticks went - the frame after one tick looked along +x, the frame after the
			 * next (this one) along +y: halfway on, the nodes' view is 45 degrees round; the view now along +y - the legs
			 * turned the other 45 */
			{
				W32(TIME + 8, 3000); SETPOS(0, 1.f, 2.f, 3.f); SETPOS(1, 4.f, 5.f, 6.f); WF(OBJECT + 0x64, 1.f); ip_tick_before();
				SETPOS(0, 1.6f, 2.07f, 3.f); SETPOS(1, 4.6f, 5.f, 6.f); WF(OBJECT + 0x64, 1.6f); W32(TIME + 8, 3001);
				WF(REC + 0x20, 1.f); WF(REC + 0x24, 0.f); WF(REC + 0x28, 0.f); WF(REC + 0x2C, 0.f); WF(REC + 0x30, 0.f); WF(REC + 0x34, 1.f);
				ip_draw_before(); tp_camera(REC, 0); ip_draw_after();                   /* the frame after that tick: +x */
				W32(TIME + 8, 3000); SETPOS(0, 1.f, 2.f, 3.f); SETPOS(1, 4.f, 5.f, 6.f); WF(OBJECT + 0x64, 1.f); ip_tick_before();
				SETPOS(0, 1.6f, 2.07f, 3.f); SETPOS(1, 4.6f, 5.f, 6.f); WF(OBJECT + 0x64, 1.6f); W32(TIME + 8, 3001);
				WF(REC + 0x20, 0.f); WF(REC + 0x24, 1.f);                                /* the frame after the next: +y */
				WF(TIME + 0x10, 0.5f); ip_draw_before(); tp_camera(REC, 0); ip_fp_before();
				{ float px = NPOS(0, 0), py = NPOS(0, 1); assert(px - 2.3213f < 1e-3f && 2.3213f - px < 1e-3f && py + 4.5007f < 1e-3f && -4.5007f - py < 1e-3f); }
				ip_fp_after(); ip_draw_after();
				/* (the camera's state left as the tests before left it - a last draw at full speed - for the free camera's test) */
				WF(REC + 0x20, 0.f); WF(REC + 0x24, 0.f); WF(REC + 0x28, -1.f); WF(REC + 0x2C, 0.f); WF(REC + 0x30, 1.f); WF(REC + 0x34, 0.f);
				host_set_speed(10, 0); ip_draw_before(); tp_camera(REC, 0); ip_fp_before(); ip_fp_after(); ip_draw_after(); host_set_speed(5, 0);
			}
		}
		/* 6i.48: the frame's blend made before the director - with your unit's origin, facing, looking vector and crouch, and
		 * what it rides - so the game builds the camera from the same in-between state it draws. A stand-in director puts the
		 * eye half way from your origin to your pelvis (node 0), as the biped's camera does looking down. Over a tick the
		 * origin goes 1 -> 1.6 and the pelvis 1 -> 2.2: drawn at a, the eye must be 1 + 0.9 a - in step with the body on
		 * every frame. (6i.47 built it from the last tick's pelvis plus the origin's blend: 1.3 + 0.6 a - off by 0.3 (1 - a),
		 * changing every frame: the doubling. Built against 6i.47's spooky.c this fails at a = 0.25.) Yours back exactly for
		 * the draw, the nodes after it; what you ride blended too; a jump, or full speed with Performance Mode off: none. */
		{
			const u32 REC = 0x6A000, VEH = 0x6E000;
			void ip_dir_before(void) __attribute__((weak)); s32 host_ip_own(s32 *set, s32 *dir) __attribute__((weak));
			void host_set_third_person(s32 on, s32 dist, s32 height, s32 side); void host_set_free_cam(s32 on, s32 block);
			float ax[2] = { 0.25f, 0.75f }, e, lx, ly;
			s32 q, set, dir;
			#define TICK0() (W32(TIME + 8, 4000), SETPOS(0, 1.f, 2.f, 3.f), SETPOS(1, 4.f, 5.f, 6.f), WF(OBJECT + 0x64, 1.f), WF(OBJECT + 0x68, 2.f), WF(OBJECT + 0x6C, 3.f), \
				WF(OBJECT + 0x70, 1.f), WF(OBJECT + 0x74, 0.f), WF(OBJECT + 0x78, 0.f), WF(OBJECT + 0x7C, 0.f), WF(OBJECT + 0x80, 0.f), WF(OBJECT + 0x84, 1.f), \
				WF(OBJECT + 0x168, 1.f), WF(OBJECT + 0x16C, 0.f), WF(OBJECT + 0x170, 0.f), WF(OBJECT + 0x2C4, 0.f))
			#define TICK1(a) (SETPOS(0, 2.2f, 2.f, 3.f), SETPOS(1, 4.6f, 5.f, 6.f), WF(OBJECT + 0x64, 1.6f), WF(OBJECT + 0x70, 0.f), WF(OBJECT + 0x74, 1.f), \
				WF(OBJECT + 0x168, 0.f), WF(OBJECT + 0x16C, 1.f), WF(OBJECT + 0x2C4, 1.f), W32(TIME + 8, 4001), WF(TIME + 0x10, a))
			#define DIRECTOR() { float o_ = RF(OBJECT + 0x64), n_ = NPOS(0, 0); WF(REC, o_ + 0.5f * (n_ - o_)); WF(REC + 4, 2.f); WF(REC + 8, 3.f); }
			memset(MEM + REC, 0, 0x358); WF(REC + 0x28, -1.f); WF(REC + 0x30, 1.f);   /* looking straight down */
			host_set_third_person(0, 10, 2, 0); host_set_free_cam(0, 1); host_set_speed(5, 0);
			MEM[OBJECT + 0xAA] = 0; W32(OBJECT + 0x14, 0xFFFFFFFFu);                   /* a biped, on foot */
			for (q = 0; q < 2; q++)
			{
				TICK0(); ip_tick_before(); TICK1(ax[q]);
				if (ip_dir_before) ip_dir_before();
				DIRECTOR();
				if (ip_dir_before)
				{
					host_ip_own(&set, &dir);
					assert(set == 1 && dir == 1 && NEAR(RF(OBJECT + 0x64), 1.f + 0.6f * ax[q]) && NEAR(NPOS(0, 0), 1.f + 1.2f * ax[q]));
					lx = RF(OBJECT + 0x168); ly = RF(OBJECT + 0x16C);
					assert(NEAR(lx * lx + ly * ly, 1.f) && NEAR(ly / lx, ax[q] / (1.f - ax[q])) && NEAR(RF(OBJECT + 0x2C4), ax[q]));   /* normalised, between */
					assert(NEAR(RF(OBJECT + 0x74) / RF(OBJECT + 0x70), ax[q] / (1.f - ax[q])) && RF(OBJECT + 0x84) == 1.f);
				}
				ip_draw_before();
				if (ip_dir_before) assert(RF(OBJECT + 0x64) == 1.6f && RF(OBJECT + 0x168) == 0.f && RF(OBJECT + 0x16C) == 1.f && RF(OBJECT + 0x2C4) == 1.f && RF(OBJECT + 0x70) == 0.f && RF(OBJECT + 0x74) == 1.f);   /* yours back, exactly */
				b = tp_camera(REC, 0); memcpy(&e, MEM + b, 4);
				assert(NEAR(NPOS(0, 0), 1.f + 1.2f * ax[q]));                       /* the body drawn between */
				assert(NEAR(e, 1.f + 0.9f * ax[q]));                                /* and the eye with it */
				ip_draw_after(); assert(NPOS(0, 0) == 2.2f);
				if (ip_dir_before) { host_ip_own(&set, &dir); assert(set == 0 && dir == 0); }
			}
			/* what you ride: a vehicle (slot 6, salt 9) - its origin and facing blended for the director too, back after */
			memset(MEM + VEH, 0, 0x400); W32(OBJ_DATA + 6 * 12, 0x0009); W32(OBJ_DATA + 6 * 12 + 8, VEH); MEM[VEH + 0xAA] = 1;
			W32(OBJECT + 0x14, 0x00090006);
			TICK0(); WF(VEH + 0x64, 10.f); WF(VEH + 0x70, 1.f); WF(VEH + 0x84, 1.f); WF(VEH + 0x168, 1.f); ip_tick_before();
			TICK1(0.5f); WF(VEH + 0x64, 11.f); WF(VEH + 0x70, 0.f); WF(VEH + 0x74, 1.f);
			ip_dir_before(); host_ip_own(&set, &dir);
			assert(set == 2 && NEAR(RF(VEH + 0x64), 10.5f) && NEAR(RF(VEH + 0x70), 0.70710678f) && NEAR(RF(VEH + 0x74), 0.70710678f));
			ip_draw_before(); assert(RF(VEH + 0x64) == 11.f && RF(VEH + 0x70) == 0.f && RF(VEH + 0x74) == 1.f); ip_draw_after();
			/* 6i.50: the seat's camera follows the vehicle's physics body (0xb9ef0: its Havok component's camera body, the
			 * motion's centre +0x70) - snapshotted and blended for the director too, put back exactly for the draw */
			{
				s32 host_ip_hk(void);
				const u32 HC5 = HC_DATA + 5 * 0x98, BOD = 0x6E400, ENT3 = 0x6E500, MOT3 = 0x6E600;
				memset(MEM + BOD, 0, 0x300); memset(MEM + HC5, 0, 0x98);
				W32(VEH + 0xB4, 5); W32(VEH + 0x14, 0xFFFFFFFFu); MEM[HC5 + 0x18] = 0;   /* (the root: no parent) */
				 W32(HC5 + 0x70, BOD); W32(HC5 + 0x74, 1);
				W32(BOD + 0x40, ENT3); W32(ENT3 + 0x3C, MOT3);
				TICK0(); WF(VEH + 0x64, 10.f); WF(MOT3 + 0x70, 10.2f); WF(MOT3 + 0x74, 1.f); WF(MOT3 + 0x78, 2.f); ip_tick_before();
				assert(host_ip_hk() == 1);
				TICK1(0.25f); WF(VEH + 0x64, 11.f); WF(MOT3 + 0x70, 11.4f);
				ip_dir_before(); host_ip_own(&set, &dir);
				assert(NEAR(RF(MOT3 + 0x70), 10.2f + 1.2f * 0.25f) && RF(MOT3 + 0x74) == 1.f && RF(MOT3 + 0x78) == 2.f);   /* a quarter way */
				ip_draw_before(); assert(RF(MOT3 + 0x70) == 11.4f); ip_draw_after(); assert(RF(MOT3 + 0x70) == 11.4f);   /* back, exactly */
				MEM[HC5 + 0x18] = 3; TICK0(); ip_tick_before(); assert(host_ip_hk() == 0);   /* no such body: none */
				W32(VEH + 0xB4, 0xFFFFFFFFu); memset(MEM + HC5, 0, 0x98);
			}
			W32(OBJECT + 0x14, 0xFFFFFFFFu); W32(OBJ_DATA + 6 * 12, 0); W32(OBJ_DATA + 6 * 12 + 8, 0);
			/* 6i.50: Wireframe, Yelo's way - the frame setup's fill mode made wireframe from the start of each frame's draw
			 * until the HUD's pass begins; off, or on the main menu: solid */
			{
				extern volatile u8 spooky_fill_wire, spooky_wire_on; void wire_fill(void); void spooky_view(u32 view); void host_set_wire(s32 v);
				u32 gst = M32h(GO + 8);
				host_set_wire(2);                                                                 /* 6i.52: xray - the fill mode */
				ip_draw_before(); wire_fill(); assert(spooky_fill_wire == 1 && !spooky_wire_on); ip_draw_after();   /* the world's frames */
				spooky_view(0); wire_fill(); assert(spooky_fill_wire == 0);                      /* the HUD's frame: solid */
				ip_draw_before(); wire_fill(); assert(spooky_fill_wire == 1); ip_draw_after();   /* the next frame */
				W32(GO + 8, 3); wire_fill(); assert(spooky_fill_wire == 0); W32(GO + 8, gst);   /* the main menu */
				host_set_wire(1);                                                                 /* 6i.55: on - every world draw again as lines, until the HUD's frame */
				ip_draw_before(); wire_fill(); assert(spooky_fill_wire == 0 && spooky_wire_on == 1); spooky_view(0); assert(!spooky_wire_on); ip_draw_after();
				W32(GO + 8, 3); ip_draw_before(); assert(!spooky_wire_on); ip_draw_after(); W32(GO + 8, gst);
				host_set_wire(0); ip_draw_before(); wire_fill(); assert(spooky_fill_wire == 0 && !spooky_wire_on); ip_draw_after();
			}
			/* a jump of 5: yours not blended (shown at once) */
			TICK0(); ip_tick_before(); TICK1(0.5f); WF(OBJECT + 0x64, 6.f);
			ip_dir_before(); host_ip_own(&set, &dir); assert(set == 0 && RF(OBJECT + 0x64) == 6.f); ip_draw_before(); ip_draw_after();
			/* full speed, Performance Mode off: nothing blended, nothing taken */
			host_set_speed(10, 0); TICK0(); ip_tick_before(); TICK1(0.5f);
			ip_dir_before(); host_ip_own(&set, &dir); assert(set == 0 && dir == 0 && RF(OBJECT + 0x64) == 1.6f && NPOS(0, 0) == 2.2f);
			ip_draw_before(); ip_draw_after(); host_set_speed(5, 0);
			/* left blended (no draw came): put back before the next tick's snapshot, and before the mod's frame */
			TICK0(); ip_tick_before(); TICK1(0.25f); ip_dir_before(); host_ip_own(&set, &dir); assert(set == 1);
			W32(TIME + 8, 4001); ip_tick_before(); host_ip_own(&set, &dir); assert(set == 0 && dir == 0 && RF(OBJECT + 0x64) == 1.6f && NPOS(0, 0) == 2.2f);
			#undef TICK0
			#undef TICK1
			#undef DIRECTOR
		}
		memcpy(MEM + OBJECT, save, sizeof(save)); memcpy(MEM + TIME, tsave, sizeof(tsave)); memcpy(MEM + 0x1800, csave, sizeof(csave));
		W32(0x1D10, oopt);
		frame();
		#undef NEAR
		#undef SETPOS
		#undef NPOS
		printf("smooth slow motion: node matrices drawn between the two ticks (0.25 -> 1.15, 0.75 -> 1.45) and put back exactly; none at 1.0, paused, after a 5-unit jump, two ticks on, for another object, still or far; the eye moved as its unit's root (first and third person), the first-person legs turned to the view (any pitch) and put back; 6i.48: blended before the director - yours too (origin, facing, looking, crouch; what you ride; 6i.50: its physics body, the seat camera's) - the eye in step with the body (1 + 0.9 a), yours back exactly for the draw; 6i.50: Wireframe - xray: wireframe fill for the world's frames, solid from the HUD's; 6i.55: on - every world draw again as lines until the HUD's frame\n");
	}
	{
		/* 6i: Unlock FPS. The game's pacing byte: 1 (its own pacing) unless on and in a game - 0, a frame a vblank; the
		 * mod's 30 Hz clock from the vblanks (a step every other frame at 60, 30 a second at 50 Hz too) and this frame's
		 * seconds; off: a step and 1/30 s a frame, whatever the vblanks. Frame interpolation at full speed while on. Per
		 * second at any frame rate: messages (2 s), the Superbounce Viewer's searches, the free camera's flight and easing,
		 * the third person camera's easing, the 10-second log lines (with the frames' vblanks beside them). */
		void host_set_unlock_fps(s32 on); s32 host_pace_clamp(void); s32 host_steps(void); u32 host_t30(void); float host_fdt(void);
		float host_ease_k(float f); u32 host_seam_searches(void); s32 host_toast_left(void); u32 host_heartbeats(void);
		void host_set_speed(s32 tenths, s32 pause); void ip_tick_before(void); void ip_draw_before(void); void ip_draw_after(void);
		s32 host_ip_blended(void); s32 host_ip_snapped(void);
		void host_fc_input(float lx, float ly, float rx, float ry, float rt, float lt); void host_fc_pose(float *p, float *yaw, float *pitch);
		void host_set_free_cam(s32 on, s32 block); void host_fc_target(float *p);
		const u32 GO = 0x4D000;
		u32 oopt = M32h(0x1D10), vb = 70000, t0, n0;
		int k, steps, fr;
		#define VB(n) (vb += (n), W32(0x1DC0, vb))
		W32(0x1D10, GO); W32(GO + 8, 1); MEM[GO + 0xE] = 30; MEM[GO + 0xF] = 0;       /* in a game (campaign) */
		MEM[0x1DC4] = 60; MEM[0x1DC5] = 0;                                          /* a 60 Hz picture */
		host_set_unlock_fps(0); VB(2); frame();
		assert(host_pace_clamp() == 1 && host_steps() == 1 && host_fdt() == 1.f / 30.f && host_ease_k(0.35f) == 0.35f);
		t0 = host_t30(); VB(1); frame(); VB(1); frame(); VB(5); frame();
		assert(host_t30() == t0 + 3);                                               /* off: a step a frame, whatever the vblanks */
		host_set_unlock_fps(1); VB(2); frame(); assert(host_pace_clamp() == 0);    /* on, in a game: the raise to 2 skipped */
		t0 = host_t30(); steps = 0;
		for (k = 0; k < 60; k++) { VB(1); frame(); steps += host_steps(); assert(host_steps() <= 1 && host_fdt() > 0.01666f && host_fdt() < 0.01667f); }
		assert(steps == 30 && host_t30() == t0 + 30);                              /* a second at 60: 30 steps, 1/60 s a frame */
		VB(2); frame(); assert(host_steps() == 1 && host_fdt() > 0.0333f && host_fdt() < 0.0334f);   /* a frame of 2: a step */
		{ s32 a, b; VB(3); frame(); a = host_steps(); VB(3); frame(); b = host_steps(); assert(a + b == 3 && host_fdt() > 0.0499f && host_fdt() < 0.0501f); }   /* of 3: 1.5 each */
		VB(100); frame(); assert(host_fdt() < 0.134f);                             /* a long stall (a load): 8 vblanks at most */
		MEM[0x1DC4] = 50; t0 = host_t30();
		for (k = 0; k < 50; k++) { VB(1); frame(); }
		assert(host_t30() == t0 + 30 && host_fdt() > 0.0199f && host_fdt() < 0.0201f);   /* a 50 Hz picture: still 30 a second */
		MEM[0x1DC4] = 0; VB(1); frame(); assert(host_fdt() > 0.01666f && host_fdt() < 0.01667f);   /* no rate read: 60 */
		MEM[0x1DC4] = 60;
		W32(GO + 8, 3); VB(1); frame(); assert(host_pace_clamp() == 1); W32(GO + 8, 1);   /* the game's main menu: its own pacing */
		W32(0x1D10, 0); VB(1); frame(); assert(host_pace_clamp() == 1); W32(0x1D10, GO);   /* no game */
		VB(1); frame(); assert(host_pace_clamp() == 0);
		/* the easing: at 60 (a vblank a frame) two frames of it are one 30 Hz frame's; a frame of 2 vblanks: the same as off */
		VB(1); frame();
		{ float q = 1.f - host_ease_k(0.25f), q2 = 1.f - host_ease_k(0.1164f); assert(fabsf(q * q - 0.75f) < 1e-5f && fabsf(q2 - 0.94f) < 1e-5f); }   /* HCM's 6% a 60 Hz frame */
		VB(2); frame(); assert(fabsf(host_ease_k(0.35f) - 0.35f) < 1e-6f);
		host_set_unlock_fps(0); VB(1); frame(); assert(host_pace_clamp() == 1 && host_steps() == 1 && host_ease_k(0.25f) == 0.25f);   /* off again */
		/* frame interpolation at full speed: a snapshot before the tick, the frame drawn between the two ticks (half way:
		 * 1.0 -> 1.6 drawn at 1.3), the real one back; off at 1.0 - none, as ever; paused - none */
		{
			const u32 NODES = OBJECT + 0x500;
			static unsigned char save[0x600], tsave[0x40], csave[0x40];
			float v;
			memcpy(save, MEM + OBJECT, sizeof(save)); memcpy(tsave, MEM + TIME, sizeof(tsave)); memcpy(csave, MEM + 0x1800, sizeof(csave));
			MEM[TIME] = 1; MEM[TIME + 1] = 0; MEM[TIME + 2] = 30; MEM[TIME + 3] = 0; WF(TIME + 4, 1.f / 30.f); WF(TIME + 0x18, 0.f); WF(TIME + 0x20, 0.f);
			WF(0x1800, 0.f); WF(0x1804, 0.f); WF(0x1808, 0.f);
			MEM[OBJECT + 0x114] = 0x68; MEM[OBJECT + 0x115] = 0; MEM[OBJECT + 0x116] = 0x00; MEM[OBJECT + 0x117] = 0x05;   /* 2 nodes at +0x500 */
			WF(OBJECT + 0x30, 1.f); WF(OBJECT + 0x34, 0.f); WF(OBJECT + 0x38, 0.f); WF(OBJECT + 0x3C, 1.f);
			memset(MEM + NODES, 0, 0x68); WF(NODES, 1.f); WF(NODES + 0x34, 1.f);
			WF(NODES + 0x28, 1.f); WF(NODES + 0x2C, 2.f); WF(NODES + 0x30, 3.f); WF(NODES + 0x34 + 0x28, 4.f);
			W32(TIME + 8, 1000); WF(TIME + 0x10, 0.f);
			host_set_speed(10, 0);
			host_set_unlock_fps(0); ip_tick_before(); assert(host_ip_snapped() == -1);
			host_set_unlock_fps(1); ip_tick_before(); assert(host_ip_snapped() >= 1);
			WF(NODES + 0x28, 1.6f); W32(TIME + 8, 1001); WF(TIME + 0x10, 0.5f);       /* the tick; the next one half way */
			ip_draw_before(); memcpy(&v, MEM + NODES + 0x28, 4); assert(host_ip_blended() == 1 && fabsf(v - 1.3f) < 1e-4f);
			ip_draw_after(); memcpy(&v, MEM + NODES + 0x28, 4); assert(v == 1.6f && host_ip_blended() == 0);
			WF(TIME + 0x10, 0.f); ip_draw_before(); memcpy(&v, MEM + NODES + 0x28, 4); assert(fabsf(v - 1.f) < 1e-6f); ip_draw_after();   /* just after the tick: the tick before (one behind) */
			host_set_speed(10, 1); ip_draw_before(); assert(host_ip_blended() == 0); ip_draw_after(); host_set_speed(10, 0);   /* paused */
			host_set_unlock_fps(0); ip_draw_before(); assert(host_ip_blended() == 0); ip_draw_after();                       /* off */
			memcpy(MEM + OBJECT, save, sizeof(save)); memcpy(MEM + TIME, tsave, sizeof(tsave)); memcpy(MEM + 0x1800, csave, sizeof(csave));
		}
		/* messages: 2 s - 60 frames at the game's own 30, 120 unlocked at 60 */
		{
			int shown = 0;
			host_set_unlock_fps(0); VB(2); frame();
			host_practice(1); assert(host_toast_left() == 60);
			for (fr = 0; fr < 300 && host_toast_left() > 0; fr++) { VB(2); frame(); shown++; }
			assert(shown == 60);
			host_set_unlock_fps(1); VB(1); frame();
			host_practice(1); shown = 0;
			for (fr = 0; fr < 300 && host_toast_left() > 0; fr++) { VB(1); frame(); shown++; }
			assert(shown >= 119 && shown <= 121);
			MEM[0x1D00] = 0;
		}
		/* the Superbounce Viewer's search: a frame each at the game's own pacing (the host runs every step), every other
		 * frame at 60 - the same per second */
		{
			void host_set_sb_viewer(s32 on);
			host_set_sb_viewer(1); state(9000, 0.f, 0, 0);
			host_set_unlock_fps(0); VB(2); frame(); n0 = host_seam_searches();
			for (fr = 0; fr < 20; fr++) { VB(2); state(9001 + (u32)fr, 0.f, 0, 0); frame(); }
			assert(host_seam_searches() - n0 == 20);
			host_set_unlock_fps(1); VB(1); frame(); n0 = host_seam_searches();
			for (fr = 0; fr < 20; fr++) { VB(1); state(9100 + (u32)fr / 2, 0.f, 0, 0); frame(); }
			assert(host_seam_searches() - n0 == 10);
			host_set_sb_viewer(0);
		}
		/* the free camera: a second of full stick forward flies its target the same 3 units - 30 frames off, 60 at 60 -
		 * and, let go, the camera closes the same part of the gap to it in a second */
		{
			float a0[3], a1[3], b0[3], b1[3], p[3], yaw, pitch, ga, gb;
			unsigned char saved_cam[0x60];
			memcpy(saved_cam, MEM + 0x1800, 0x60); memset(MEM + 0x1800, 0, 0x60);
			WF(0x1800, 0.f); WF(0x1804, 0.f); WF(0x1808, 0.6f); WF(0x1820, 1.f); WF(0x182C, 0.f); WF(0x1834, 1.f);
			host_set_unlock_fps(0); host_set_free_cam(0, 1); VB(2); frame();
			host_set_free_cam(1, 1); host_fc_input(0, 0, 0, 0, 0, 0); VB(2); frame();
			host_fc_target(a0); host_fc_input(0, 1.f, 0, 0, 0, 0);
			for (fr = 0; fr < 30; fr++) { VB(2); frame(); }
			host_fc_target(a1); host_fc_pose(p, &yaw, &pitch); ga = a1[0] - p[0];
			host_fc_input(0, 0, 0, 0, 0, 0); for (fr = 0; fr < 30; fr++) { VB(2); frame(); }
			host_fc_pose(p, &yaw, &pitch); ga = (a1[0] - p[0]) / ga;
			host_set_free_cam(0, 1); VB(2); frame();
			host_set_unlock_fps(1); host_set_free_cam(1, 1); host_fc_input(0, 0, 0, 0, 0, 0); VB(1); frame();
			host_fc_target(b0); host_fc_input(0, 1.f, 0, 0, 0, 0);
			for (fr = 0; fr < 60; fr++) { VB(1); frame(); }
			host_fc_target(b1); host_fc_pose(p, &yaw, &pitch); gb = b1[0] - p[0];
			host_fc_input(0, 0, 0, 0, 0, 0); for (fr = 0; fr < 60; fr++) { VB(1); frame(); }
			host_fc_pose(p, &yaw, &pitch); gb = (b1[0] - p[0]) / gb;
			assert(a1[0] - a0[0] > 2.99f && a1[0] - a0[0] < 3.01f && fabsf((a1[0] - a0[0]) - (b1[0] - b0[0])) < 1e-3f);
			assert(ga > 0.02f && ga < 0.03f && fabsf(ga - gb) < 1e-3f);             /* 0.8836^30 = 0.94^60 = 0.024 */
			host_set_free_cam(0, 1); host_set_unlock_fps(0); VB(2); frame();
			memcpy(MEM + 0x1800, saved_cam, 0x60);
		}
		/* the third person camera: its eye height eased a quarter of the way a 30 Hz frame - unlocked at 60, two frames do
		 * what one did (0.6 -> 0.8: 0.65, the camera 0.2 above it) */
		{
			u32 cam = 0x48000, moved;
			float z1, z2;
			memset(MEM + cam, 0, 0x358);
			state(2100, 0.f, 0, 0); WF(OBJECT + 0x90, 0.f); MEM[OBJECT + 0xAA] = 0;
			WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
			WF(cam + 8, 0.6f); WF(cam + 0x20, 1.f); WF(cam + 0x34, 1.f);
			MEM[OBJECT + 0x240] = 0xFF; MEM[OBJECT + 0x241] = 0xFF;
			host_set_third_person(0, 10, 2, 0); host_tp_camera(cam, 0);             /* not applied: the next one starts afresh */
			host_set_third_person(1, 10, 2, 0);
			host_set_unlock_fps(0); VB(2); spooky_main_frame();
			host_tp_camera(cam, 0); host_tp_fp_after();                             /* taken: 0.6 */
			WF(cam + 8, 0.8f); VB(2); spooky_main_frame(); moved = host_tp_camera(cam, 0); host_tp_fp_after(); z1 = M32f(moved + 8);
			WF(cam + 8, 0.6f); host_set_third_person(0, 10, 2, 0); host_tp_camera(cam, 0); host_set_third_person(1, 10, 2, 0);
			host_set_unlock_fps(1); VB(1); spooky_main_frame();
			host_tp_camera(cam, 0); host_tp_fp_after();
			WF(cam + 8, 0.8f);
			VB(1); spooky_main_frame(); host_tp_camera(cam, 0); host_tp_fp_after();
			VB(1); spooky_main_frame(); moved = host_tp_camera(cam, 0); host_tp_fp_after(); z2 = M32f(moved + 8);
			assert(fabsf(z1 - 1.0f) < 1e-5f && fabsf(z2 - z1) < 1e-4f);   /* 6i.123: the eye exactly (0.8 + 0.2), at 30 and at 60 alike */
			host_set_third_person(0, 10, 2, 0); host_set_unlock_fps(0); VB(2); spooky_main_frame();
		}
		/* the 10-second lines: 300 steps of the clock - 600 frames at 60 - and the frames' vblanks logged beside them */
		{
			u32 h0 = host_heartbeats();
			int u0 = logged("Unlock FPS: frames that took 1 vblank (60 fps) ");
			host_set_unlock_fps(1);
			for (fr = 0; fr < 600; fr++) { VB(1); frame(); }
			if (h0 < 24) assert(host_heartbeats() == h0 + 1 && logged("Unlock FPS: frames that took 1 vblank (60 fps) ") > u0);
			host_set_unlock_fps(0); VB(2); frame();
		}
		#undef VB
		W32(0x1D10, oopt);
		frame();
		printf("Unlock FPS: the pacing byte (0 only on, in a game); the 30 Hz clock from the vblanks (60, 50 Hz, stalls), off a step a frame; interpolation at full speed; messages, searches, the free and third person cameras the same per second; the 10-second lines\n");
	}
	{
		/* 6i.1: the interpolation's memory sized to what's free - 2048 / 1024 / 512 / 256 nodes leaving 192 KB free beside it,
		 * else 128 if 16 KB stays free, else none (said once, tried again 30 s on); and taken in order: your unit, what it
		 * rides and holds (at any distance), then within 20 units, then out to 80 - the rest left out when it's full */
		u32 host_ip_nodes_for(u32 largest, u32 keep); void host_ip_mem(u32 largest); s32 host_ip_cap(void); s32 host_ip_left_out(void); u32 host_ip_datum(s32 k);
		s32 host_ip_give_back(void); void host_set_pred(s32 on, s32 dread);
		const u32 K = 192u * 1024u, K32 = 32u * 1024u;
		void host_set_unlock_fps(s32 on); void host_set_speed(s32 tenths, s32 pause); void ip_tick_before(void); s32 host_ip_snapped(void);
		void spooky_main_frame(void);
		const u32 GO = 0x4D000, O1 = 0x7A000, O3 = 0x7B000, O4 = 0x7C000;
		u32 oopt = M32h(0x1D10), osize = M32h(OBJECT + 0x114), opar = M32h(OBJECT + 0x14), k;
		static unsigned char tsave[0x40], csave[0x40], osave[0x40];
		float c0x = M32f(OBJECT + 0x30), c0y = M32f(OBJECT + 0x34), c0z = M32f(OBJECT + 0x38), c0r = M32f(OBJECT + 0x3C);
		int fr, i0;
		assert(host_ip_nodes_for(2048u * 1024u, K) == 2048 && host_ip_nodes_for(428032u, K) == 2048 && host_ip_nodes_for(428031u, K) == 1024);
		assert(host_ip_nodes_for(312320u, K) == 1024 && host_ip_nodes_for(307200u, K) == 512 && host_ip_nodes_for(254464u, K) == 512);
		assert(host_ip_nodes_for(235520u, K) == 256 && host_ip_nodes_for(225535u, K) == 128 && host_ip_nodes_for(32000u, K) == 128);
		assert(host_ip_nodes_for(31999u, K) == 0 && host_ip_nodes_for(0, K) == 0);
		/* 6i.2: 192 KB kept free only with the Monster/Dreadnought Viewer on; else 32 KB - your 360's 112 KB: 512 nodes, not 128 */
		assert(host_ip_nodes_for(112u * 1024u, K32) == 512 && host_ip_nodes_for(112u * 1024u, K) == 128 && host_ip_nodes_for(148480u, K32) == 1024 && host_ip_nodes_for(148479u, K32) == 512);
		memcpy(tsave, MEM + TIME, sizeof(tsave)); memcpy(csave, MEM + 0x1800, sizeof(csave)); memcpy(osave, MEM + OBJ_DATA, sizeof(osave));
		W32(0x1D10, GO); W32(GO + 8, 1);
		MEM[TIME] = 1; MEM[TIME + 1] = 0; MEM[TIME + 2] = 30; MEM[TIME + 3] = 0; WF(TIME + 0x18, 0.f); WF(TIME + 0x20, 0.f); W32(TIME + 8, 5000);
		WF(0x1800, 0.f); WF(0x1804, 0.f); WF(0x1808, 0.f);                          /* the view at the origin */
		host_set_speed(10, 0); host_set_unlock_fps(1);
		/* the console with 300 KB free and the spots viewer on: 512 nodes, said; off: 2048; your 360's 112 KB: 512 */
		host_set_pred(1, 0); host_ip_mem(300u * 1024u); ip_tick_before(); assert(host_ip_cap() == 512);
		assert(logged("frame interpolation: memory for 512 nodes (57 KB); the largest free block was 300 KB") == 1);
		host_set_pred(0, 0); host_ip_mem(300u * 1024u); ip_tick_before(); assert(host_ip_cap() == 2048);
		host_ip_mem(112u * 1024u); ip_tick_before(); assert(host_ip_cap() == 512);
		/* given back: Unlock FPS off at full speed (the next frame), and when asked (the spots) - taken again on the next use */
		{ int gb0 = logged("given back (Unlock FPS off)");
		  host_set_unlock_fps(0); W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame(); assert(host_ip_cap() == 0 && logged("given back (Unlock FPS off)") == gb0 + 1); }
		host_set_unlock_fps(1); W32(0x1DC0, M32h(0x1DC0) + 1); spooky_main_frame(); assert(host_ip_cap() == 0);   /* not until it's needed */
		ip_tick_before(); assert(host_ip_cap() == 512);
		assert(host_ip_give_back() == 1 && host_ip_cap() == 0 && host_ip_give_back() == 0); ip_tick_before(); assert(host_ip_cap() == 512);
		host_set_speed(5, 0); host_set_unlock_fps(0); W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame(); assert(host_ip_cap() == 512);   /* slowed: kept */
		host_set_speed(10, 0); host_set_unlock_fps(1); W32(0x1DC0, M32h(0x1DC0) + 1); spooky_main_frame();
		{
			/* 6i.55: Interpolation limit 128 (test) gone - frames go by and the room stays what fits */
			W32(0x1DC0, M32h(0x1DC0) + 1); spooky_main_frame(); ip_tick_before(); assert(host_ip_cap() == 512);
		}
		{
			/* 6i.38: the main menu no longer gives the interpolation's memory back (6i.36 did: the next level could leave
			 * none for it) - held across levels, as up to 6i.35 */
			assert(host_ip_cap() == 512);
			W32(GO + 8, 3); spooky_frame(); spooky_frame(); assert(host_ip_cap() == 512);
			W32(GO + 8, 1); spooky_frame(); ip_tick_before(); assert(host_ip_cap() == 512);
		}
		host_set_pred(1, 0);
		/* 20 KB free: none - said once, not tried again for 30 s, then tried (and got 1024 nodes with 400 KB free) */
		host_ip_mem(20u * 1024u); ip_tick_before(); assert(host_ip_cap() == -1 && host_ip_snapped() == -1);   /* (the spots viewer on: 192 KB kept) */
		assert(logged("frame interpolation: no memory for it (the largest free block 20 KB)") == 1);
		{ void host_set_ip_largest(u32 b); host_set_ip_largest(400u * 1024u); }
		for (fr = 0; fr < 899; fr++) { W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame(); }   /* 30 fps frames: a step each */
		ip_tick_before(); assert(host_ip_cap() == -1);                             /* 899 steps on: not yet */
		W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame(); ip_tick_before(); assert(host_ip_cap() == 1024 && host_ip_snapped() == 0);   /* got; ran (nothing with nodes here yet) */
		assert(logged("frame interpolation: no memory for it") == 1);              /* said once only */
		/* the order with room for 128 nodes: your unit (40 nodes, pool slot 2) and the weapon it holds (8, slot 4), then the
		 * object 5 units out (60, slot 3); the one 30 units out (40, slot 1) has no room left; one 200 out isn't counted */
		host_ip_mem(32000u); ip_tick_before(); assert(host_ip_cap() == 128);
		W32(OBJECT + 0x114, (40u * 0x34u) | (0x500u << 16)); W32(OBJECT + 0x14, 0xFFFFFFFFu);   /* 40 nodes at +0x500, on foot */
		WF(OBJECT + 0x30, 1.f); WF(OBJECT + 0x34, 0.f); WF(OBJECT + 0x38, 0.f); WF(OBJECT + 0x3C, 1.f);
		memset(MEM + O1, 0, 0x1000); memset(MEM + O3, 0, 0x1000); memset(MEM + O4, 0, 0x1000);
		W32(O1 + 0x114, (40u * 0x34u) | (0x100u << 16)); W32(O1 + 0x14, 0xFFFFFFFFu); WF(O1 + 0x30, 30.f); WF(O1 + 0x3C, 1.f);
		W32(O3 + 0x114, (60u * 0x34u) | (0x100u << 16)); W32(O3 + 0x14, 0xFFFFFFFFu); WF(O3 + 0x30, 5.f); WF(O3 + 0x3C, 1.f);
		W32(O4 + 0x114, (8u * 0x34u) | (0x100u << 16)); W32(O4 + 0x14, UNIT); WF(O4 + 0x30, 1.f); WF(O4 + 0x3C, 0.2f);   /* held: its parent is your unit */
		W32(OBJ_DATA + 1 * 12, 7); W32(OBJ_DATA + 1 * 12 + 8, O1);
		W32(OBJ_DATA + 3 * 12, 8); W32(OBJ_DATA + 3 * 12 + 8, O3);
		W32(OBJ_DATA + 4 * 12, 9); W32(OBJ_DATA + 4 * 12 + 8, O4);
		W32(TIME + 8, 5100); ip_tick_before();
		assert(host_ip_snapped() == 3 && host_ip_datum(0) == UNIT && host_ip_datum(1) == ((9u << 16) | 4u) && host_ip_datum(2) == ((8u << 16) | 3u));
		assert(host_ip_left_out() == 1);
		/* 6i.2: only what can move - the near one made scenery at rest: passed over (not counted); given a spin, taken;
		 * a machine (type 7) at rest: taken */
		MEM[O3 + 0xAA] = 6; W32(TIME + 8, 5103); ip_tick_before(); assert(host_ip_snapped() == 3 && host_ip_datum(2) == ((7u << 16) | 1u) && host_ip_left_out() == 0);
		WF(O3 + 0x9C, 0.3f); W32(TIME + 8, 5104); ip_tick_before(); assert(host_ip_snapped() == 3 && host_ip_datum(2) == ((8u << 16) | 3u) && host_ip_left_out() == 1);
		WF(O3 + 0x9C, 0.f); MEM[O3 + 0xAA] = 7; W32(TIME + 8, 5105); ip_tick_before(); assert(host_ip_datum(2) == ((8u << 16) | 3u));
		MEM[O3 + 0xAA] = 0; W32(TIME + 8, 5100); ip_tick_before();
		WF(O1 + 0x30, 200.f); W32(TIME + 8, 5101); ip_tick_before(); assert(host_ip_snapped() == 3 && host_ip_left_out() == 0);   /* far: not counted */
		WF(O3 + 0x30, 30.f); WF(O1 + 0x30, 5.f); W32(TIME + 8, 5102); ip_tick_before();     /* swapped: the 5-unit one in, 60 left out */
		assert(host_ip_snapped() == 3 && host_ip_datum(2) == ((7u << 16) | 1u) && host_ip_left_out() == 1);
		{
			/* 6i.27: your body first - a weapon you hold of 100 nodes in an earlier pool slot (1) than your unit (2): before,
			 * it took the room and your body (40) was left out; now your body goes in first and the weapon is left out */
			s32 host_ip_body(void);
			u32 s114 = M32h(O1 + 0x114), s14 = M32h(O1 + 0x14); float s30 = 0.f; memcpy(&s30, MEM + O1 + 0x30, 4);
			W32(O1 + 0x114, (100u * 0x34u) | (0x100u << 16)); W32(O1 + 0x14, UNIT); WF(O1 + 0x30, 1.f);
			W32(TIME + 8, 5106); ip_tick_before();
			assert(host_ip_datum(0) == UNIT && host_ip_body() == 40);
			{ s32 k2, w = 0; for (k2 = 0; k2 < host_ip_snapped(); k2++) if (host_ip_datum(k2) == ((7u << 16) | 1u)) w = 1; assert(!w); }   /* the weapon: no room */
			W32(O1 + 0x114, s114); W32(O1 + 0x14, s14); WF(O1 + 0x30, s30);
			W32(TIME + 8, 5102); ip_tick_before();
		}
		i0 = logged("; interpolated ");

		for (fr = 0; fr < 1200; fr++) { W32(0x1DC0, M32h(0x1DC0) + 1); frame(); }       /* the 10-second line: what it took */
		assert(logged("; interpolated 3 objects, 88 of 128 nodes, your body 40 (1 near ones left out)") >= 1 || logged("; interpolated ") > i0);
		/* back as it was: no extra objects, the room the other tests had */
		for (k = 1; k <= 4; k++) if (k != 2) { W32(OBJ_DATA + k * 12, 0); W32(OBJ_DATA + k * 12 + 8, 0); }
		W32(OBJECT + 0x114, osize); W32(OBJECT + 0x14, opar);
		WF(OBJECT + 0x30, c0x); WF(OBJECT + 0x34, c0y); WF(OBJECT + 0x38, c0z); WF(OBJECT + 0x3C, c0r);
		memcpy(MEM + TIME, tsave, sizeof(tsave)); memcpy(MEM + 0x1800, csave, sizeof(csave)); memcpy(MEM + OBJ_DATA, osave, sizeof(osave));
		host_set_pred(0, 0); host_ip_mem(4u * 1024u * 1024u);
		host_set_unlock_fps(0); W32(0x1D10, oopt); frame(); frame();
		printf("frame interpolation memory: 2048 / 1024 / 512 / 256 nodes leaving 192 KB free (32 KB without the spots viewer), else 128, else none (said once, tried again in 30 s); given back when Unlock FPS goes off or the spots need it; your unit, its seat and weapons first, then the nearest that can move - the rest left out and counted\n");
	}
	{
		/* 6i.3: Late frames at once - the display the game wants is its own constant's each frame, with byte 1 ("one
		 * vblank, or at once if late") set only while it's on with Unlock FPS in a game; a change is logged (the game
		 * re-applies it itself). The frame-time line: update / drawing / after drawing, the mod's own and the interpolation */
		void host_set_unlock_fps(s32 on); u32 host_display(s32 k); void spooky_main_frame(void);
		void ip_draw_before(void); void ip_draw_after(void);
		void prof_wait_before(void); void prof_wait_after(void); void prof_present_before(void); void prof_present_after(void);
		const u32 GO = 0x4D000;
		u32 oopt = M32h(0x1D10);
		int fr, l0;
		W32(0x1DC8, 0x00010000u); W32(0x1DCC, 3u); W32(0x1DD0, 1u);                 /* the game's constant */
		W32(0x1D10, GO); W32(GO + 8, 1);
		host_set_unlock_fps(0); spooky_main_frame();
		assert(host_display(0) == 0x00010000u && host_display(1) == 3u && host_display(2) == 1u);   /* Performance Mode off: the game's own */
		{ void host_set_tear(s32 on); host_set_unlock_fps(1); spooky_main_frame(); assert(host_display(0) == 0x00010100u);   /* 6i.189: Allow Tearing ON by default (the "or at once if late" byte) */   /* 6i.63: Allow Tearing off by default - held */
		  host_set_unlock_fps(0); spooky_main_frame(); host_set_tear(1); }
		l0 = logged("late frames: shown at once");
		host_set_unlock_fps(1); spooky_main_frame(); assert(host_display(0) == 0x00010100u && host_display(1) == 3u);
		assert(logged("late frames: shown at once") == l0 + 1);
		spooky_main_frame(); assert(logged("late frames: shown at once") == l0 + 1);   /* said once, not each frame */
		W32(GO + 8, 3); spooky_main_frame(); assert(host_display(0) == 0x00010000u && logged("late frames: held to the next vblank") >= 1);   /* the main menu */
		W32(GO + 8, 1); spooky_main_frame(); assert(host_display(0) == 0x00010100u);
		{
			/* 6i.35: the particle fix's hold - set by each frame's draw (6i.36: with Performance Mode), cleared by a tick: 1
			 * exactly while a frame's update runs with no tick since the last frame's */
			extern u8 spooky_ptc_hold; void ip_tick_before(void); void ip_draw_before(void); void ip_draw_after(void);
			host_set_unlock_fps(1);
			ip_draw_before(); ip_draw_after(); assert(spooky_ptc_hold == 1);
			ip_tick_before(); assert(spooky_ptc_hold == 0);
			ip_tick_before(); assert(spooky_ptc_hold == 0);                 /* two ticks in a frame */
			ip_draw_before(); ip_draw_after(); assert(spooky_ptc_hold == 1);
			host_set_unlock_fps(0); ip_draw_before(); ip_draw_after(); assert(spooky_ptc_hold == 0);
			host_set_unlock_fps(1); ip_tick_before();
		}
		host_set_unlock_fps(0); spooky_main_frame(); assert(host_display(0) == 0x00010000u);
		W32(0x1DD0, 7u); spooky_main_frame(); assert(host_display(2) == 7u);        /* whatever the game's constant says, kept */
		W32(0x1DD0, 1u);
		/* the frame-time parts, the clock held still and moved by hand: 4 ms to the draw; a draw of 12.5 ms holding the
		 * game's vblank wait (3 ms) and the present (0.5); 0.2 ms to the next frame - 16.7 in all, not a slow one. Then a
		 * 20 ms frame: counted slow. The 10-second lines: the parts in ms, and the slow frames' own */
		{
			void host_tsc(u32 v, u32 step); u32 host_pf_last(s32 k); u32 host_pf_slow(void);
			void prof_wait_before(void); void prof_wait_after(void); void prof_present_before(void); void prof_present_after(void);
			const u32 MS = 733000u;
			u32 c = 0x10000000u, s0;
			host_tsc(c, 0); W32(0x1DC0, M32h(0x1DC0) + 1); spooky_main_frame();
			c += 4u * MS; host_tsc(c, 0); ip_draw_before();
			c += 9u * MS; host_tsc(c, 0); prof_wait_before(); c += 3u * MS; host_tsc(c, 0); prof_wait_after();
			prof_present_before(); c += MS / 2u; host_tsc(c, 0); prof_present_after(); ip_draw_after();
			c += MS / 5u; host_tsc(c, 0); s0 = host_pf_slow(); W32(0x1DC0, M32h(0x1DC0) + 1); spooky_main_frame();
			assert(host_pf_last(0) == 4000 && host_pf_last(1) == 9000 && host_pf_last(2) == 3000 && host_pf_last(3) == 500 && host_pf_last(4) == 200 && host_pf_last(6) == 16700);
			assert(host_pf_slow() == s0);
			c += 2u * MS; host_tsc(c, 0); ip_draw_before(); c += 17u * MS; host_tsc(c, 0); ip_draw_after();
			c += MS; host_tsc(c, 0); W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();
			assert(host_pf_last(0) == 2000 && host_pf_last(1) == 17000 && host_pf_last(2) == 0 && host_pf_last(6) == 20000 && host_pf_slow() == s0 + 1);
			host_tsc(c, 1000);
		}
		l0 = logged("  frame time ms, average / most (");
		for (fr = 0; fr < 700; fr++) { W32(0x1DC0, M32h(0x1DC0) + 1); spooky_main_frame(); ip_draw_before(); prof_wait_before(); prof_wait_after(); prof_present_before(); prof_present_after(); ip_draw_after(); }
		assert(logged("  frame time ms, average / most (") > l0 && logged(", vblank wait 0.0 / 3.0, present 0.0 / 0.5, after 0.0 / 1.0; the mod ") >= 1 && logged(" (interpolation ") > 0);
		/* 6i.176: VSync "30 fps" - Performance Mode on with the game's own two-vblank pacing left on; a frame of two
		 * vblanks is one 30 Hz step and 1/30 s; back to "on": the raise skipped again; Performance Mode off: the game's own */
		{
			void host_set_vsync(s32 on, s32 cap); void host_set_unlock_fps(s32 on); s32 host_pace_clamp(void); s32 host_steps(void); float host_fdt(void);
			#define VB(n) W32(0x1DC0, M32h(0x1DC0) + (n))                                         /* the game's vblank count, n on */
			host_set_unlock_fps(1); host_set_vsync(1, 0); VB(1); spooky_main_frame(); assert(host_pace_clamp() == 0 && host_fdt() < 1.5f / 60.f);   /* Performance Mode on, VSync on: unlocked, a vblank a frame */
			host_set_vsync(2, 0); VB(2); spooky_main_frame(); assert(host_pace_clamp() == 1 && host_steps() == 1 && host_fdt() == 2.f / 60.f);
			VB(2); spooky_main_frame(); assert(host_pace_clamp() == 1 && host_steps() == 1);
			VB(3); spooky_main_frame(); assert(host_pace_clamp() == 1 && host_fdt() > 2.5f / 60.f && host_fdt() < 3.5f / 60.f);   /* a late frame: three vblanks, counted as such */
			host_set_vsync(1, 0); VB(1); spooky_main_frame(); assert(host_pace_clamp() == 0);
			host_set_vsync(0, 0); VB(1); spooky_main_frame(); assert(host_pace_clamp() == 0);                  /* VSync off: unpaced, as before */
			host_set_vsync(2, 0); host_set_unlock_fps(0); VB(2); spooky_main_frame(); assert(host_pace_clamp() == 1);
			host_set_unlock_fps(1); host_set_vsync(1, 0); VB(1); spooky_main_frame(); assert(host_pace_clamp() == 0);
			host_set_unlock_fps(0); VB(2); spooky_main_frame();                                    /* (as this part of the tests found it) */
			#undef VB
		}
		assert(logged("  frames over 17 ms (1), average: update 2.0, drawing 17.0, vblank wait 0.0, present 0.0, after 1.0; the mod 0.0") == 1);
		assert(logged("  render ms, ") == 0);                                              /* 6i.8: that line retired */
		host_set_unlock_fps(0); spooky_main_frame();
		W32(0x1D10, oopt);
		printf("Late frames at once: the game's display constant each frame, \"or at once if late\" only on with Unlock FPS in a game, a change said once; the frame-time parts (update, drawing, the game's vblank wait, the present, after the draw, the mod's own) to the microsecond, the slow frames' own\n");
	}
	{
		/* 6i.12 / 6i.16 (6i.23: the Direct3D probes' line retired): the render passes' and the missing textures' lines from
		 * a window of frames - 2 ms in pass 3, 0.04 in pass 1 (left out), pass 16 skipped; two textures missing */
		void ip_draw_before(void); void ip_draw_after(void); void spooky_main_frame(void);
		void host_set_unlock_fps(s32 on);
		int fr, l0 = logged("  render passes ms a frame: 3:2.0 (pass 16 skipped)");
		memset(MEM + 0x7E800, 0, 0x200);
		MEM[0x7E804] = 0x00; MEM[0x7E805] = 0x02; MEM[0x7E806] = 0x00; MEM[0x7E807] = 0x02; MEM[0x7E80A] = 2; W32(0x7E868, (1u << 16) | (18u << 8));   /* 512 x 512 cube, 1 mip */
		MEM[0x7E904] = 64; MEM[0x7E906] = 64; MEM[0x7E90A] = 0; W32(0x7E968, (7u << 16) | (12u << 8));                                   /* 64 x 64 2D, 7 mips */
		host_set_unlock_fps(1);
		for (fr = 0; fr < 1900 && logged("  render passes ms a frame: 3:2.0 (pass 16 skipped)") < l0 + 1; fr++)   /* a whole window of these frames */
		{
			W32(0x1DC0, M32h(0x1DC0) + 1); spooky_main_frame(); ip_draw_before();
			{ void host_pass(s32 k, u32 cyc, s32 skip); host_pass(3, 733u * 2000u, 16); host_pass(1, 733u * 40u, 16); }   /* 6i.12: 2 ms in pass 3, 0.04 in pass 1 (under 0.05: left out), pass 16 skipped */
			ip_draw_after();
		}
		assert(logged("  render passes ms a frame: 3:2.0 (pass 16 skipped)") >= 1);   /* 6i.12; 6i.13: the effect mode */
		{ void host_pass(s32 k, u32 cyc, s32 skip); host_pass(0, 0, 0); }
		host_set_unlock_fps(0); frame();
		printf("render passes line from a window of frames\n");
	}
	{
		/* 6i.17 / 6i.18: the passes timed in C - off: each pass start to end; GPU time by pass on: the frame cut at marks
		 * (the draw's start 29, each pass's start and end 28, the scene's calls 30+, the present), each part's time to the
		 * mark it began at */
		extern u32 spooky_pass_cyc[64]; void pass_in(u32 k); void pass_out(u32 k); void prof_present_before(void); void mark(u32 id);
		void host_set_gpu_time(s32 on); void host_tsc(u32 v, u32 step);
		u32 k;
		for (k = 0; k < 64; k++) spooky_pass_cyc[k] = 0;
		void host_cap_live(s32 live, s32 rec);
		host_set_gpu_time(1); host_cap_live(1, 0);                                     /* 6i.26: a capture's parts frame */
		host_tsc(100000, 0); mark(29);                                                  /* the draw starts */
		host_tsc(101000, 0); pass_in(3); host_tsc(104000, 0); pass_out(3);              /* 1000 before; pass 3: 3000 */
		host_tsc(104500, 0); mark(32); host_tsc(108500, 0); pass_in(16); host_tsc(109000, 0); pass_out(16);   /* 500 after it; part 32: 4000; pass 16: 500 */
		host_tsc(112000, 0); prof_present_before();                                     /* 3000 to the present */
		assert(spooky_pass_cyc[29] == 1000 && spooky_pass_cyc[3] == 3000 && spooky_pass_cyc[32] == 4000 && spooky_pass_cyc[16] == 500 && spooky_pass_cyc[28] == 3500);
		host_set_gpu_time(0); host_cap_live(0, 0);
		host_tsc(200000, 0); mark(30); pass_in(3); host_tsc(201000, 0); pass_out(3); host_tsc(205000, 0); prof_present_before();
		assert(spooky_pass_cyc[30] == 0 && spooky_pass_cyc[28] == 3500 && spooky_pass_cyc[3] == 4000);   /* off: no marks, the pass still timed */
		host_tsc(0, 1000);
		for (k = 0; k < 64; k++) spooky_pass_cyc[k] = 0;
		printf("render passes timed; GPU time by pass on - the frame cut at marks (the draw's start, the passes, the scene's calls, the present), each part to its mark\n");
	}
	{
		/* 6i.19: a capture - White + Up: 30 frames timed with the GPU drained at every mark (GPU time by pass off before
		 * and after), one line, the setting put back */
		extern u32 spooky_pass_cyc[64]; void pass_in(u32 k); void pass_out(u32 k); void prof_present_before(void); void prof_present_after(void); void mark(u32 id);
		void host_capture(void); void host_tsc(u32 v, u32 step); void spooky_main_frame(void); void host_set_gpu_time(s32 on);
		u32 T = 500000, k;
		int c0 = logged("capture 1, ms a frame:");
		host_set_gpu_time(0);
		host_capture(); W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();            /* the frame it was pressed in */
		for (k = 0; k < 48; k++)                                                        /* 6i.26: a warm-up, then 30 */
		{
			host_tsc(T, 0); mark(29); T += 733u * 1000u;
			host_tsc(T, 0); pass_in(3); T += 733u * 2000u; host_tsc(T, 0); pass_out(3); T += 733u * 500u;
			host_tsc(T, 0); prof_present_before(); prof_present_after();
			W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();
		}
		assert(logged("capture 1, ms a frame: 3:2.0 28:0.5 29:1.0") == c0 + 1);
		assert(logged("capture 1 as normal (14 frames): the CPU 3.5 ms a frame, drawing 3.5; the GPU's time not read; still busy at the present 0.0 ms") == 1);   /* 6i.90 */
		host_tsc(T, 0); mark(29); T += 733u * 1000u; host_tsc(T, 0); mark(30);           /* after it: GPU time by pass off again */
		assert(spooky_pass_cyc[29] == 0);
		host_tsc(0, 1000);
		printf("capture: White + Up - 30 frames GPU-timed, one line, the setting put back\n");
	}
	{
		/* 6i.23: a capture's shaders - each record's draw timed (rec_in / rec_out round it), summed by its shader and pass;
		 * the second line lists the costliest with their draws a frame; Skip shader n: the last capture's n-th not drawn */
		void rec_in(u32 record, u32 mode); void rec_out(void); s32 host_rec_skip(void); void host_set_skip_shader(s32 n);
		void host_capture(void); void host_tsc(u32 v, u32 step); void spooky_main_frame(void); void host_set_gpu_time(s32 on);
		u32 T = 900000, k, d;
		int c0;
		W32(0x7E810, 0x82A1B2C0); W32(0x7E830, 0x80123450);                         /* two records' shaders */
		/* 6i.24: what the draws are made with - stage 0 a 512 x 512 DXT1 (12) of 10 mips, linear / linear, anisotropy 4 */
		memset(MEM + 0x7F000, 0, 0x200); W32(0x1EC0, 0x7E900); W32(0x1EC4, 0); W32(0x1EC8, 0); W32(0x1ECC, 0);
		W32(0x7E90C, (10u << 16) | (12u << 8)); W32(0x1ED0, 512); W32(0x1EE0, 512);
		W32(0x7F010, 2); W32(0x7F014, 2); W32(0x7F018, 0); W32(0x7F020, 4);
		host_set_gpu_time(0);
		host_capture(); W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();
		for (k = 0; k < 48; k++)
		{
			for (d = 0; d < 4; d++) { host_tsc(T, 0); rec_in(0x7E800, 3); T += 733u * 500u; host_tsc(T, 0); rec_out(); }   /* 4 draws, 0.5 ms each */
			host_tsc(T, 0); rec_in(0x7E820, 1); T += 733u * 300u; host_tsc(T, 0); rec_out();                                /* 1 draw, 0.3 ms */
			W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();
		}
		c0 = logged("shaders, ms a frame (draws): 1) pass 3 0x82A1B2C0 2.0 (4) 2) pass 1 0x80123450 0.3 (1)");   /* 6i.82: textures */
		assert(c0 == 1);
		host_set_skip_shader(1); frame(); rec_in(0x7E800, 3); assert(host_rec_skip()); rec_in(0x7E820, 1); assert(!host_rec_skip());
		host_set_skip_shader(2); frame(); rec_in(0x7E820, 1); assert(host_rec_skip());
		host_set_skip_shader(0); frame(); rec_in(0x7E800, 3); assert(!host_rec_skip());
		{
			/* 6i.25: the selections locked - Skip shader 1 set after a capture whose 1st is A stays A through a capture whose
			 * 1st is B; set again (0, then 1) it picks afresh */
			void host_shader_locks(u32 *skip, u32 *info); u32 sk, inf, kk;
			host_set_skip_shader(0); frame(); host_set_skip_shader(1); frame();
			host_shader_locks(&sk, &inf); assert(sk == M32h(0x7E810) || sk == M32h(0x7E830));
			{
				u32 first = sk, other = first == 0x82A1B2C0u ? 0x80123450u : 0x82A1B2C0u;
				host_capture(); W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();
				for (kk = 0; kk < 48; kk++)                                         /* now the other one costs more */
				{
					host_tsc(T, 0); rec_in(other == 0x82A1B2C0u ? 0x7E800 : 0x7E820, 3); T += 733u * 900u; host_tsc(T, 0); rec_out();
					W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();
				}
				host_shader_locks(&sk, &inf); assert(sk == first);                 /* still the one chosen */
				host_set_skip_shader(0); frame(); host_set_skip_shader(1); frame();
				host_shader_locks(&sk, &inf); assert(sk == other);                 /* picked afresh: the new 1st */
				host_set_skip_shader(0); frame();
			}
		}
		{
			/* 6i.28: the water copy every other frame - 6i.59: no longer (it left the water a frame behind on alternate frames: the
			 * jitter); made every frame with Performance Mode too, one sample; Performance Mode off: as the game */
			extern volatile u8 spooky_copy_one;
			void host_copy_ran(void); s32 host_copy_keep(void); void host_set_unlock_fps(s32 on);
			#define CF(nv, ran) (W32(0x1DC0, M32h(0x1DC0) + (nv)), spooky_main_frame(), (ran) ? host_copy_ran() : (void)0)
			host_set_unlock_fps(1);
			CF(2, 1); CF(2, 0); assert(!host_copy_keep() && spooky_copy_one);             /* 6i.59: made every frame (keeping it every other frame was the jitter) */
			CF(2, 1); assert(!host_copy_keep());
			CF(1, 0); assert(!host_copy_keep());
			CF(1, 0); assert(!host_copy_keep()); CF(1, 0); assert(!host_copy_keep());
			host_set_unlock_fps(0); CF(2, 1); CF(2, 0); assert(!host_copy_keep() && !spooky_copy_one);   /* Performance Mode off: as the game */
			host_set_unlock_fps(1); CF(1, 0); assert(spooky_copy_one); host_set_unlock_fps(0); CF(1, 0);
			#undef CF
		}
		{
			/* 6i.25: No anisotropic filtering - after a material's sampler settings (3 stages), each anisotropic stage made
			 * linear, its max anisotropy 0; the 4th stage (not in the range) untouched */
			void aniso_clamp(u32 range); u32 st, lv; extern volatile u8 spooky_aniso;
			for (lv = 1; lv <= 3; lv++)
			{
				/* stages 0 - 2 anisotropic (stage 1: min linear, mag anisotropic), 4x, mip linear; stage 3 out of the range;
				 * then stage 2 made a plain linear layer: never touched by 2x / 2x fast */
				for (st = 0; st < 4; st++) { W32(0x7F00C + 0x80 * st, 3); W32(0x7F010 + 0x80 * st, st == 1 ? 2 : 3); W32(0x7F014 + 0x80 * st, 2); W32(0x7F020 + 0x80 * st, 4); }
				if (lv > 1) { W32(0x7F10C, 2); W32(0x7F110, 2); W32(0x7F120, 8); }
				MEM[0x7E700] = 0x00; MEM[0x7E701] = 3 << 1;                      /* the range: 3 stages (bits 9 - 15) */
				spooky_aniso = (u8)lv; aniso_clamp(0x7E700);
				if (lv == 1)                                                      /* off: linear, anisotropy 0 (6i.25) */
					for (st = 0; st < 3; st++) assert(M32h(0x7F00C + 0x80 * st) == 2 && M32h(0x7F010 + 0x80 * st) == 2 && M32h(0x7F020 + 0x80 * st) == 0 && M32h(0x7F014 + 0x80 * st) == 2);
				else                                                              /* 6i.53: 2x - still anisotropic, at most 2; fast: mip point */
				{
					for (st = 0; st < 2; st++) assert(M32h(0x7F00C + 0x80 * st) == 3 && M32h(0x7F010 + 0x80 * st) == (st == 1 ? 2u : 3u) && M32h(0x7F020 + 0x80 * st) == 2 && M32h(0x7F014 + 0x80 * st) == (lv == 3 ? 1u : 2u));
					assert(M32h(0x7F10C) == 2 && M32h(0x7F110) == 2 && M32h(0x7F120) == 8 && M32h(0x7F114) == 2);
				}
				assert(M32h(0x7F18C) == 3 && M32h(0x7F190) == 3 && M32h(0x7F1A0) == 4 && M32h(0x7F194) == 2);
			}
			W32(0x7F020, 1); spooky_aniso = 2; aniso_clamp(0x7E700); assert(M32h(0x7F020) == 1);   /* already under 2: kept */
			spooky_aniso = 0;
		}
		host_tsc(0, 1000);
		printf("capture shaders: each record's draw timed, summed by shader and pass, the costliest listed with its stages' textures and filters; Skip shader / Shader details locked to the material chosen; No anisotropic filtering puts a material's anisotropic stages to linear\n");
	}
	{
		/* 6h.39: the live log page - its header and the last lines, as the PC tool reads them (dumped for tools/test_live.py) */
		const unsigned char *live = (const unsigned char *)host_live_page();
		u32 seq, size, lines;
		FILE *fp;
		memcpy(&seq, live + 8, 4); memcpy(&size, live + 12, 4); memcpy(&lines, live + 24, 4);
		assert(!memcmp(live, "SPKYLIVE", 8) && size == 4096 - 32 && seq > 0 && lines > 0 && ((size_t)live & 4095) == 0);
		fp = fopen("/tmp/live_page.bin", "wb"); if (fp) { fwrite(live, 1, 4096, fp); fclose(fp); }
		fp = fopen("/tmp/live_last.txt", "w"); if (fp) { int k; for (k = g_nlog - 3; k < g_nlog; k++) if (k >= 0) fprintf(fp, "%s\n", g_loglines[k % 256]); fclose(fp); }
		printf("live log page: page-aligned, %u lines written, %u bytes through the ring\n", lines, seq);
	}
	{
		/* 6i.42: triple buffering's swap fix - after the present: both primary surfaces = the device's back buffer (+0x1A14),
		 * a reference each (the old two released), both primary textures' data = its data; without spooky_triple, or
		 * before the targets exist: nothing */
		extern volatile u8 spooky_triple; extern u32 g_host_addref, g_host_release; void prof_present_after(void);
		u32 dev = 0x30000, back = 0x31000, t0 = 0x32000, t1 = 0x32100, s0 = 0x33000, s1 = 0x33100, a0, r0;
		W32(0x1EB0, dev); W32(dev + 0x1A14, back); W32(back + 4, 0x00ABC000u);
		W32(0x1EE0, t0); W32(0x1EE4, t1); W32(0x1EE8, s0); W32(0x1EEC, s1); W32(t0 + 4, 0x111000u); W32(t1 + 4, 0x222000u);
		a0 = g_host_addref; r0 = g_host_release;
		spooky_triple = 0; prof_present_after();
		assert(M32h(0x1EE8) == s0 && M32h(0x1EEC) == s1 && M32h(t0 + 4) == 0x111000u && g_host_addref == a0 && g_host_release == r0);
		spooky_triple = 1; prof_present_after();
		assert(M32h(0x1EE8) == back && M32h(0x1EEC) == back && M32h(t0 + 4) == 0x00ABC000u && M32h(t1 + 4) == 0x00ABC000u);
		assert(g_host_addref == a0 + 2 && g_host_release == r0 + 2);
		prof_present_after(); assert(g_host_addref == a0 + 4 && g_host_release == r0 + 4);   /* every frame, balanced */
		W32(0x1EE8, 0); prof_present_after(); assert(g_host_addref == a0 + 4);               /* not set up: nothing */
		spooky_triple = 0; W32(0x1EB0, 0);
		printf("triple buffering: after each present both primary surfaces on the back buffer (references balanced), the textures' data with them\n");
	}
	{
		/* 6i.40: Performance Mode Settings - VSync off: the display's frame interval 0 (immediate, unpaced), on again: back;
		 * Late Frames: byte 1; Performance Mode off: the game's own whatever they say. FPS Limit (6 = 60, VSync off): a
		 * frame waits out its 60th; none, or VSync on: no wait. Anisotropic Filtering: off (the clamp) with Performance Mode;
		 * 6i.53: 2x fast / 2x / full */
		void host_set_unlock_fps(s32 on); void host_set_tear(s32 on); void host_set_vsync(s32 on, s32 cap); void host_set_aniso(s32 on); void prof_wait_before(void);
		u32 host_display(s32 k); void spooky_main_frame(void); void host_tsc(u32 v, u32 step); u32 host_tsc_now(void);
		extern volatile u8 spooky_aniso; u32 t1;
		host_set_unlock_fps(1); host_set_tear(1); host_set_vsync(1, 0); spooky_main_frame(); assert(host_display(0) == 0x00010100u);
		host_set_vsync(0, 0); spooky_main_frame(); assert(host_display(0) == 0x00000100u);
		host_set_tear(0); spooky_main_frame(); assert(host_display(0) == 0x00000000u);
		host_set_vsync(1, 0); spooky_main_frame(); assert(host_display(0) == 0x00010000u);
		host_set_tear(1); host_set_vsync(0, 0); host_set_unlock_fps(0); spooky_main_frame(); assert(host_display(0) == 0x00010000u);
		host_set_unlock_fps(1);
		host_set_vsync(0, 6); host_tsc(1000000, 1000000);                      /* 6i.47: index 6 = 60 */
		prof_wait_before(); t1 = host_tsc_now();
		prof_wait_before(); assert(host_tsc_now() - t1 >= 733333333u / 60u && host_tsc_now() - t1 < 733333333u / 60u + 3000000u);
		host_set_vsync(0, 0); t1 = host_tsc_now(); prof_wait_before(); assert(host_tsc_now() - t1 < 3000000u);
		host_set_vsync(0, 5); prof_wait_before(); t1 = host_tsc_now();        /* 6i.47: index 5 = 50 */
		prof_wait_before(); assert(host_tsc_now() - t1 >= 733333333u / 50u && host_tsc_now() - t1 < 733333333u / 50u + 3000000u);
		host_set_vsync(1, 6); t1 = host_tsc_now(); prof_wait_before(); assert(host_tsc_now() - t1 < 3000000u);
		host_set_vsync(1, 0); host_tsc(0, 1000); spooky_main_frame(); assert(spooky_aniso == 3);      /* 6i.151: 2x fast by default */
		host_set_aniso(0); spooky_main_frame(); assert(spooky_aniso == 1);                            /* off */
		host_set_aniso(1); spooky_main_frame(); assert(spooky_aniso == 3);                            /* 6i.53: 2x fast */
		host_set_aniso(2); spooky_main_frame(); assert(spooky_aniso == 2);                            /* 2x */
		host_set_aniso(3); spooky_main_frame(); assert(spooky_aniso == 0);                            /* full: the game's */
		host_set_aniso(1); host_set_unlock_fps(0); spooky_main_frame(); assert(spooky_aniso == 0);    /* Performance Mode off: the game's */
		host_set_aniso(0);
		{
			/* 6i.58: Water Surface (test) off - the plane hidden; 6i.59: Water Refraction game's (0) / fast (1, the default:
			 * the copy's target keeps the screen's depth) / late (2: the copy after the plane) / off (3: never made); with
			 * Performance Mode off, the game's own */
			extern volatile u8 spooky_water_hide, spooky_copy_keep, spooky_copy_keepz, spooky_copy_late; void host_set_water(s32 surface, s32 refr);
			host_set_unlock_fps(1); host_set_water(1, 1); spooky_main_frame(); assert(!spooky_water_hide && spooky_copy_keepz && !spooky_copy_late);
			host_set_water(0, 1); spooky_main_frame(); assert(spooky_water_hide == 1);
			host_set_water(1, 2); spooky_main_frame(); assert(!spooky_water_hide && !spooky_copy_keepz && spooky_copy_late);
			host_set_water(1, 0); spooky_main_frame(); assert(!spooky_copy_keepz && !spooky_copy_late);
			host_set_water(1, 3); spooky_main_frame(); assert(spooky_copy_keep == 1 && !spooky_copy_keepz && !spooky_copy_late);
			{	/* 6i.59: every other mode copies every frame - never kept (the every-other-frame keep was the jitter) */
				extern volatile u8 spooky_copy_ran; s32 m;
				for (m = 0; m < 3; m++) { host_set_water(1, m); spooky_copy_ran = 1; spooky_main_frame(); assert(!spooky_copy_keep); spooky_main_frame(); assert(!spooky_copy_keep); }
			}
			host_set_water(0, 3); host_set_unlock_fps(0); spooky_main_frame(); assert(!spooky_water_hide && !spooky_copy_keep && !spooky_copy_keepz && !spooky_copy_late);
			host_set_water(1, 1); host_set_unlock_fps(1); spooky_main_frame(); host_set_unlock_fps(0); spooky_main_frame();
			{	/* 6i.61 / 6i.63: Render Scale - at the main scene's start the frame storage's rectangle (every screen target's
				 * viewport) is cut to the share, top-left kept; before the interface (mark 49) put back as it was; 100 %,
				 * Performance Mode off, the main menu or a view not filling the screen: untouched */
				void rs_cut(void); void mark(u32 id); void rs_ui_point(void); void spooky_view(u32 view); void host_set_rscale(s32 v); s32 host_rs_on(void); void spooky_main_frame(void);
				u32 GOP = M32h(0x1D10), GOF = 0x7D800, FS = 0x1200 + 0x48, f0 = M32h(FS), f1 = M32h(FS + 4);
				#define RSF(t, l, b, r) (W32(FS, (t) | ((u32)(l) << 16)), W32(FS + 4, (b) | ((u32)(r) << 16)))
				W32(0x1D10, GOF); W32(GOF + 8, 1);
				#define NF() (W32(0x1DC0, M32h(0x1DC0) + 1), spooky_main_frame())   /* the next frame */
				host_set_unlock_fps(1); host_set_rscale(7); RSF(0, 0, 480, 640); NF();
				rs_cut(); assert(host_rs_on() && M32h(FS + 4) == (336u | (448u << 16)) && M32h(FS) == 0);
				RSF(0, 0, 480, 640); rs_cut(); assert(M32h(FS + 4) == (336u | (448u << 16)));   /* 6i.64: a second scene before the stretch: the same cut */
				mark(49); assert(host_rs_on());                                         /* 6i.69: no longer at the first interface step */
				rs_ui_point(); assert(host_rs_on());                                     /* 6i.70: not at the second interface step either */
				spooky_view(0); assert(!host_rs_on() && M32h(FS + 4) == (480u | (640u << 16)));   /* as the HUD's view begins: back */
				rs_cut(); assert(!host_rs_on() && M32h(FS + 4) == (480u | (640u << 16)));   /* 6i.64: after the stretch this frame: nothing */
				NF(); host_set_rscale(5); rs_cut(); assert(M32h(FS + 4) == (240u | (320u << 16))); spooky_view(0);
				NF(); host_set_rscale(10); rs_cut(); assert(!host_rs_on() && M32h(FS + 4) == (480u | (640u << 16)));
				NF(); host_set_rscale(7); RSF(0, 320, 480, 640); rs_cut(); assert(!host_rs_on());     /* not the whole screen */
				NF(); RSF(0, 0, 480, 640); host_set_unlock_fps(0); rs_cut(); assert(!host_rs_on());
				NF(); host_set_unlock_fps(1); W32(GOF + 8, 3); rs_cut(); assert(!host_rs_on());       /* the main menu */
				{	/* 6i.65: SetViewport while the cut is on - a full-screen viewport passed on cut (a copy), any other as it is;
					 * the cut off (after the stretch): as it is */
					u32 vp_filter(u32 vp); u32 host_vp_cut(s32 k); u32 VP = 0x7D900, r;
					W32(VP, 0); W32(VP + 4, 0); W32(VP + 8, 640); W32(VP + 12, 480);
					NF(); host_set_unlock_fps(1); W32(GOF + 8, 1); host_set_rscale(6); RSF(0, 0, 480, 640); rs_cut(); assert(host_rs_on());
					r = vp_filter(VP); assert(r != VP && host_vp_cut(0) == 0 && host_vp_cut(2) == 384 && host_vp_cut(3) == 288);
					W32(VP + 8, 320); W32(VP + 12, 240); assert(vp_filter(VP) == VP);      /* a smaller target's: as it is */
					W32(VP + 8, 640); W32(VP + 12, 480); W32(VP, 8); assert(vp_filter(VP) == VP); W32(VP, 0);
					spooky_view(0); assert(!host_rs_on() && vp_filter(VP) == VP);              /* after the stretch: as it is */
				}
				{	/* 6i.148: Render Scale above 100 % - drawn for real. At the scene's start the three big targets are made (the
					 * kernel's contiguous memory) and the game's screen-target header pointers turned to the mod's copies with the
					 * bigger memory and size, the light accumulation target's table entry edited in place, the view rectangle and
					 * the viewports made the big size, the size getter answering with it; at the stretch everything is put back
					 * (the picture's texture last, after the copy). No room: nothing turned, drawn at 100 %, said once. */
					extern u32 g_host_mem_big; s32 host_bg_on(void), host_bg_made(void); u32 host_bg_surf(s32), host_bg_tex(s32), host_bg_word(s32, s32);
					u32 rt_size(u32 index, u32 *w, u32 *h); const char *host_toast(void); u32 vp_filter(u32 vp); u32 host_vp_cut(s32 k);
					const u32 BBS = 0x5F700, BBS1 = 0x5F720, DEP = 0x5F740, T3S = 0x5F760, T3T = 0x5F780, T24 = 0x5F7A0, TX0 = 0x5F7C0, TX1 = 0x5F7E0, E1 = 0x7F400 + 0x98, VP = 0x7D900;
					u32 w = 0, h = 0, q;
					for (q = 0; q < 6; q++) { W32(BBS + 4 * q, 0x1000 + q); W32(BBS1 + 4 * q, 0x1100 + q); W32(DEP + 4 * q, 0x1200 + q); W32(T3S + 4 * q, 0x1300 + q); }
					for (q = 0; q < 5; q++) { W32(T3T + 4 * q, 0x1400 + q); W32(T24 + 4 * q, 0x1500 + q); W32(TX0 + 4 * q, 0x1600 + q); W32(TX1 + 4 * q, 0x1700 + q); }
					W32(BBS + 4, 0x03C00000u); W32(DEP + 4, 0x03A00000u); W32(T3S + 4, 0x03A00000u); W32(TX0 + 4, 0x03C00000u);   /* the game's Data: physical */
					W32(0x7F350, BBS); W32(0x7F354, BBS1); W32(0x7F364, DEP); W32(0x7F330, T3S); W32(0x7F334, T3T); W32(0x7F360, T24); W32(0x7F358, TX0); W32(0x7F35C, TX1); W32(0x7F368, 0);
					W32(E1 + 0x88, 0x80123000u); W32(E1 + 0x78, 1); MEM[E1 + 0x95] = 0; W32(E1 + 8, 0x03800000u); W32(E1 + 0x14, 0x271df27fu); W32(E1 + 0x1C, 0x03800000u); W32(E1 + 0x28, 0x271df27fu); W32(E1 + 0x80, 640); W32(E1 + 0x84, 480);
					W32(VP, 0); W32(VP + 4, 0); W32(VP + 8, 640); W32(VP + 12, 480);
					g_host_mem_big = 0x400000u;                                            /* room for 3 x 1.6 MB */
					NF(); host_set_unlock_fps(1); W32(GOF + 8, 1); host_set_rscale(12); RSF(0, 0, 480, 640); rs_cut();
					assert(host_rs_on() && host_bg_on() && host_bg_made() == 12 && M32h(FS + 4) == (576u | (768u << 16)));
					assert(M32h(0x7F350) == host_bg_surf(0) && M32h(0x7F364) == host_bg_surf(1) && M32h(0x7F330) == host_bg_surf(2));
					assert(M32h(0x7F334) == host_bg_tex(1) && M32h(0x7F360) == host_bg_tex(2) && M32h(0x7F358) == host_bg_tex(0) && M32h(0x7F35C) == host_bg_tex(0));
					assert(host_bg_word(0, 0) == 0x1000 && host_bg_word(0, 2) == 0x1002 && host_bg_word(0, 1) == 0x00200000u && host_bg_word(0, 4) == ((47u << 24) | (575u << 12) | 767u));   /* a copy: Common kept, Data the buffer (physical), Size 768 x 576 pitch 3072 */
					assert(host_bg_word(1, 1) == 0x00200000u && host_bg_word(2, 1) == 0x00200000u && host_bg_word(1, 4) == host_bg_word(0, 4));
					assert(M32h(E1 + 8) == 0x00200000u && M32h(E1 + 0x14) == host_bg_word(0, 4) && M32h(E1 + 0x1C) == 0x00200000u && M32h(E1 + 0x80) == 768 && M32h(E1 + 0x84) == 576);
					assert(rt_size(0, &w, &h) == 1 && w == 768 && h == 576); assert(rt_size(3, &w, &h) == 1 && rt_size(24, &w, &h) == 1 && rt_size(1, &w, &h) == 1 && rt_size(19, &w, &h) == 0);
					assert(vp_filter(VP) != VP && host_vp_cut(2) == 768 && host_vp_cut(3) == 576);
					RSF(0, 0, 480, 640); rs_cut(); assert(M32h(FS + 4) == (576u | (768u << 16)) && host_bg_on());   /* a second scene before the stretch: the same */
					spooky_view(0);                                                            /* the stretch: everything back */
					assert(!host_rs_on() && !host_bg_on() && M32h(FS + 4) == (480u | (640u << 16)));
					assert(M32h(0x7F350) == BBS && M32h(0x7F364) == DEP && M32h(0x7F330) == T3S && M32h(0x7F334) == T3T && M32h(0x7F360) == T24 && M32h(0x7F358) == TX0 && M32h(0x7F35C) == TX1);
					assert(M32h(E1 + 8) == 0x03800000u && M32h(E1 + 0x14) == 0x271df27fu && M32h(E1 + 0x1C) == 0x03800000u && M32h(E1 + 0x80) == 640 && M32h(E1 + 0x84) == 480);
					{
						/* 6i.199: while a cutscene's blur target exists (the table's entry 18) the larger targets stand down - the scene
						 * at the game's own size, nothing swapped, said once; gone again: the larger targets as before */
						const u32 E18 = 0x7F400 + 18 * 0x98;
						W32(E18 + 0x88, 0x80456000u); W32(E18 + 0x78, 1);
						NF(); RSF(0, 0, 480, 640); rs_cut();
						assert(!host_rs_on() && !host_bg_on() && M32h(FS + 4) == (480u | (640u << 16)) && M32h(0x7F350) == BBS);
						assert(rt_size(0, &w, &h) == 0 && logged("render scale: a cutscene is blurring (its extra target exists) - the scene at 640 x 480 until it has gone") == 1);
						spooky_view(0);
						NF(); RSF(0, 0, 480, 640); rs_cut(); assert(!host_bg_on() && logged("render scale: a cutscene is blurring") == 1); spooky_view(0);   /* said once */
						W32(E18 + 0x88, 0); W32(E18 + 0x78, 0);
						NF(); RSF(0, 0, 480, 640); rs_cut(); assert(host_rs_on() && host_bg_on() && M32h(FS + 4) == (576u | (768u << 16))); spooky_view(0);
						assert(!host_bg_on() && M32h(0x7F350) == BBS);
					}
					assert(rt_size(0, &w, &h) == 0 && vp_filter(VP) == VP);
					NF(); assert(host_bg_made() == 12);                                        /* the targets kept between frames */
					host_set_rscale(10); NF(); assert(host_bg_made() == 0);                     /* back to 100 %: freed at the next frame's start */
					NF(); host_set_rscale(11); W32(TX0 + 4, 0x83C00000u); rs_cut(); assert(host_bg_on() && host_bg_word(0, 1) == 0x00200000u && M32h(0x7F358) != TX0);
					{ extern u32 host_bg_texword(s32 which, s32 k); assert(host_bg_texword(0, 1) == 0xF0200000u); }   /* a Data with high bits: the virtual address form */
					spooky_view(0); assert(!host_bg_on() && M32h(0x7F358) == TX0);
					NF(); g_host_mem_big = 0x100000u; host_set_rscale(13); rs_cut();            /* no room: drawn at 100 %, said once */
					assert(!host_rs_on() && !host_bg_on() && host_bg_made() == 0 && M32h(FS + 4) == (480u | (640u << 16)) && strstr(host_toast(), "no room for its 3 x 2028 KB"));
					NF(); rs_cut(); assert(!host_rs_on());
					NF(); host_set_rscale(10); host_set_unlock_fps(1); g_host_mem_big = 0x9C000u;
					W32(0x7F350, 0); W32(0x7F354, 0); W32(0x7F364, 0); W32(0x7F330, 0); W32(0x7F334, 0); W32(0x7F360, 0); W32(0x7F358, 0); W32(0x7F35C, 0); W32(E1 + 0x88, 0);
				}
				{	/* 6i.66: the world view's projection - a clip box putting the whole view in the top-left share (x -1 ..
					 * -1 + 2/s, y 1 - 2/s .. 1); none when the game passes its own, at 100 %, not a full-screen view, or
					 * once stretched this frame */
					u32 proj_clip(u32 view, u32 clip); float host_rs_clip(s32 k); u32 PV = 0x7DA00, c;
					W32(PV + 0x30, 0); W32(PV + 0x34, 480u | (640u << 16));
					NF(); host_set_rscale(5); c = proj_clip(PV, 0);
					assert(c && host_rs_clip(0) == -1.f && host_rs_clip(1) > 2.99f && host_rs_clip(1) < 3.01f && host_rs_clip(2) < -2.99f && host_rs_clip(2) > -3.01f && host_rs_clip(3) == 1.f);
					{	/* 6i.67: the game's own box - the share's window cut from it */
						u32 CB = 0x7DB00; union { float f; u32 u; } cf;
						cf.f = -1.f; W32(CB, cf.u); cf.f = 1.f; W32(CB + 4, cf.u); cf.f = -1.f; W32(CB + 8, cf.u); cf.f = 1.f; W32(CB + 12, cf.u);
						c = proj_clip(PV, CB); assert(c && c != CB && host_rs_clip(0) == -1.f && host_rs_clip(1) > 2.99f && host_rs_clip(1) < 3.01f && host_rs_clip(2) < -2.99f && host_rs_clip(2) > -3.01f && host_rs_clip(3) == 1.f);
						cf.f = -0.5f; W32(CB, cf.u); cf.f = 0.5f; W32(CB + 4, cf.u);
						c = proj_clip(PV, CB); assert(host_rs_clip(0) == -0.5f && host_rs_clip(1) > 1.49f && host_rs_clip(1) < 1.51f);
						cf.f = 0.5f; W32(CB, cf.u); cf.f = -0.5f; W32(CB + 4, cf.u); assert(proj_clip(PV, CB) == CB);   /* not a box */
					}
					W32(PV + 0x34, 240u | (320u << 16)); assert(proj_clip(PV, 0) == 0); W32(PV + 0x34, 480u | (640u << 16));
					host_set_rscale(10); assert(proj_clip(PV, 0) == 0);
					host_set_rscale(5); RSF(0, 0, 480, 640); rs_cut(); spooky_view(0); assert(proj_clip(PV, 0) == 0);   /* stretched this frame */
				}
				{	/* 6i.71: Stretch Point (test) - the stretch at the chosen point only; the HUD's view always catches it */
					void rs_after_ui_point(void); void host_set_rs_point(s32 v);
					NF(); W32(GOF + 8, 1); host_set_unlock_fps(1); host_set_rscale(7); RSF(0, 0, 480, 640);
					host_set_rs_point(2); rs_cut(); rs_ui_point(); assert(host_rs_on()); rs_after_ui_point(); assert(!host_rs_on());
					NF(); host_set_rs_point(3); RSF(0, 0, 480, 640); rs_cut(); rs_after_ui_point(); mark(57); assert(host_rs_on()); mark(56); assert(!host_rs_on());
					NF(); host_set_rs_point(4); RSF(0, 0, 480, 640); rs_cut(); mark(56); assert(host_rs_on()); mark(57); assert(!host_rs_on());
					NF(); host_set_rs_point(1); RSF(0, 0, 480, 640); rs_cut(); rs_ui_point(); assert(!host_rs_on());
					NF(); host_set_rs_point(5); RSF(0, 0, 480, 640); rs_cut(); rs_ui_point(); rs_after_ui_point(); mark(56); mark(57); assert(host_rs_on()); spooky_view(0); assert(!host_rs_on());
				}
				{	/* 6i.73: Particles - around each sprite: the alpha test on (greater than 8 / 32) when the game has it off, then put
					 * back as it was; the game's own alpha test left alone; game's or Performance Mode off: untouched. Water
					 * Refraction: every frame / every other frame / never made (Performance Mode only) */
					void sprite_before(void); void sprite_after(void); void host_set_particles(s32 v); void host_set_water_rate(s32 v);
					extern volatile u8 spooky_copy_keep; u32 k, made;
					host_set_unlock_fps(1); W32(0x7E800, 0x203u); W32(0x7E808, 0); W32(0x7E80C, 77u);
					host_set_particles(0); sprite_before(); assert(M32h(0x7E808) == 0); sprite_after();
					host_set_particles(1); sprite_before(); assert(M32h(0x7E808) == 1 && M32h(0x7E800) == 0x204u && M32h(0x7E80C) == 8u);
					sprite_after(); assert(M32h(0x7E808) == 0 && M32h(0x7E800) == 0x203u && M32h(0x7E80C) == 77u);
					host_set_particles(2); sprite_before(); assert(M32h(0x7E80C) == 32u); sprite_after(); assert(M32h(0x7E80C) == 77u);
					W32(0x7E808, 1); W32(0x7E800, 0x204u); W32(0x7E80C, 0); sprite_before(); assert(M32h(0x7E80C) == 32u && M32h(0x7E808) == 1);   /* 6i.74: the game's own test on - its reference raised */
					sprite_after(); assert(M32h(0x7E80C) == 0 && M32h(0x7E808) == 1 && M32h(0x7E800) == 0x204u);
					W32(0x7E80C, 77u); sprite_before(); assert(M32h(0x7E80C) == 77u); sprite_after();                 /* already stricter: left */
					W32(0x7E800, 0x203u); W32(0x7E80C, 0); sprite_before(); assert(M32h(0x7E80C) == 0); sprite_after(); /* another kind: left */
					W32(0x7E808, 0); W32(0x7E800, 0x203u); W32(0x7E80C, 77u);
					host_set_unlock_fps(0); sprite_before(); assert(M32h(0x7E808) == 0); sprite_after(); host_set_particles(0);
					host_set_unlock_fps(1);
					host_set_water_rate(0); for (k = made = 0; k < 8; k++) { NF(); if (!spooky_copy_keep) made++; } assert(made == 8);
					host_set_water_rate(1); for (k = made = 0; k < 8; k++) { NF(); if (!spooky_copy_keep) made++; } assert(made == 4);
					host_set_water_rate(2); for (k = made = 0; k < 8; k++) { NF(); if (!spooky_copy_keep) made++; } assert(made == 0);
					host_set_unlock_fps(0); NF(); assert(!spooky_copy_keep); host_set_water_rate(0);
				}
				{	/* 6i.75: Object Shadows off - passes 10 and 11 in the skip mask; Smoke Pass (test) off - 12; Performance Mode only */
					extern volatile u32 spooky_pass_mask; void host_set_shadows(s32 v, s32 p12);
					u32 k, drawn;
					host_set_unlock_fps(1); host_set_shadows(0, 1); NF(); assert(spooky_pass_mask == 0);
					host_set_shadows(2, 1); NF(); assert(spooky_pass_mask == ((1u << 10) | (1u << 11)));
					host_set_shadows(0, 0); NF(); assert(spooky_pass_mask == (1u << 12));
					host_set_shadows(2, 0); host_set_unlock_fps(0); NF(); assert(spooky_pass_mask == 0);
					host_set_shadows(0, 1);
				}
				{	/* 6i.77: Dynamic Lights - a light's sphere to its screen rectangle from the scene's camera (forward +x, up +z,
					 * 240 px focal, 640 x 480); passes 11 / 12 clipped to it; the sphere at the camera, a light covering the
					 * screen, game's or Performance Mode off: left alone */
					void light_begin(u32 source, u32 tag, u32 shape); void light_end(void); void host_cam_snap(void); s32 host_light_rect(s32 k);
					void host_set_lights(s32 v); void host_light_pass(u32 k, s32 in); s32 host_light_set(void);
					union { float f; u32 u; } c; u32 CAM = 0x7EA00, L = 0x7EB00, k2; float cam[9] = { 0, 0, 0, 1, 0, 0, 0, 0, 1 };
					for (k2 = 0; k2 < 9; k2++) { c.f = cam[k2]; W32(CAM + 4 * k2, c.u); }
					c.f = 240.f; W32(0x7E900 + 0xB8, c.u); W32(0x7E900 + 0xBC, c.u);
					W32(0x1200 + 0x48, 0); W32(0x1200 + 0x4C, 480u | (640u << 16));
					host_set_unlock_fps(1); host_set_lights(1); host_cam_snap();
					c.f = 10.f; W32(L + 0x18, c.u); c.f = 0; W32(L + 0x1C, c.u); W32(L + 0x20, c.u); c.f = 1.f; W32(L + 0x24, c.u);
					light_begin(L + 0x84, 0, 0); assert(host_light_rect(-1) == 1);
					assert(host_light_rect(0) > 285 && host_light_rect(0) < 300 && host_light_rect(2) > 340 && host_light_rect(2) < 355);   /* about 320 +- 27 */
					assert(host_light_rect(1) > 205 && host_light_rect(1) < 220 && host_light_rect(3) > 260 && host_light_rect(3) < 275);   /* about 240 +- 27 */
					host_light_pass(12, 1); assert(host_light_set()); host_light_pass(12, 0); assert(!host_light_set());
					host_light_pass(3, 1); assert(!host_light_set());                                          /* other passes: not */
					c.f = 0; W32(L + 0x1C, c.u); c.f = 5.f; W32(L + 0x20, c.u);                                /* above the view's middle */
					light_begin(L + 0x84, 0, 0); assert(host_light_rect(-1) == 1 && host_light_rect(3) < 240);
					c.f = 0.5f; W32(L + 0x18, c.u); light_begin(L + 0x84, 0, 0); assert(host_light_rect(-1) == 0);   /* the sphere at the camera */
					c.f = 10.f; W32(L + 0x18, c.u); c.f = 20.f; W32(L + 0x24, c.u); light_begin(L + 0x84, 0, 0); assert(host_light_rect(-1) == 0);   /* reaches the camera */
					c.f = 1.f; W32(L + 0x24, c.u); host_set_lights(0); light_begin(L + 0x84, 0, 0); assert(host_light_rect(-1) == 0);
					{	/* 6i.96: the light's view is a local of the light loop - on the stack, above us: taken there (aligned, within
						 * 64 KB), not below us, not beyond, not unaligned - though ptr_ok turns stack addresses away */
						s32 view_in_stack(u32 p, u32 sp);
						assert(view_in_stack(0xD0001100u, 0xD0001000u) && view_in_stack(0xD000F000u, 0xD0001000u));
						assert(!view_in_stack(0xD0000FFCu, 0xD0001000u) && !view_in_stack(0xD0012000u, 0xD0001000u) && !view_in_stack(0xD0001102u, 0xD0001000u));
					}
					{	/* 6i.95: a beam light's frustum - the flashlight: carried by the first-person weapon (flags 0x20 without
						 * 0x40000), its frame in the source (position +0, beam +0x28, up +0x34), the view's type non-zero; slopes 0.2 /
						 * 0.1 (6i.97: the larger, 0.2, both ways), near 1, far 4, straight ahead from the camera: 0.2 x 240 px =
						 * 48: 320 +- 48, 240 +- 48 (and 4 more); the camera's own (0x40020), the beam behind the camera, a sphere
						 * light: the sphere */
						u32 TT = 0x7F600, DEF = 0x7F700, SRC = 0x7F780, IN = L + 0x84, q;
						host_set_lights(1);
						c.f = 10.f; W32(L + 0x18, c.u); c.f = 0; W32(L + 0x1C, c.u); W32(L + 0x20, c.u); c.f = 20.f; W32(L + 0x24, c.u);   /* its sphere holds the camera */
						for (q = 0; q < 3; q++) { c.f = 0.f; W32(IN + 4 * q, c.u); }                         /* at the camera (x ahead, z up) */
						c.f = 2.f; W32(IN + 0x28, c.u); c.f = 0; W32(IN + 0x2C, c.u); W32(IN + 0x30, c.u);    /* the beam, not normalized */
						c.f = 0; W32(IN + 0x34, c.u); W32(IN + 0x38, c.u); c.f = 1.f; W32(IN + 0x3C, c.u);   /* up */
						W32(0x7F304, TT); W32(TT + 5 * 16 + 8, DEF); W32(DEF, 0x20u); W32(SRC, 1);
						c.f = 0.2f; W32(SRC + 0x4C, c.u); c.f = 0.1f; W32(SRC + 0x50, c.u); c.f = 1.f; W32(SRC + 0x60, c.u); c.f = 4.f; W32(SRC + 0x64, c.u);
						light_begin(IN, 5, SRC); assert(host_light_rect(-1) == 1);
						assert(host_light_rect(0) >= 266 && host_light_rect(0) <= 270 && host_light_rect(2) >= 371 && host_light_rect(2) <= 375);   /* 6i.97: 0.2 x 240 = 48 */
						assert(host_light_rect(1) >= 186 && host_light_rect(1) <= 190 && host_light_rect(3) >= 291 && host_light_rect(3) <= 295);
						{	/* 6i.97: your flashlight - slopes 0.466 both ways, near 2, far 6, the gun a little ahead, right and below the
							 * eye: cut to about 0.47 x 240 = 112 round the centre, nudged by the gun's offset - not the whole screen */
							light_end();
							c.f = 0.466f; W32(SRC + 0x4C, c.u); W32(SRC + 0x50, c.u); c.f = 2.f; W32(SRC + 0x60, c.u); c.f = 6.f; W32(SRC + 0x64, c.u);
							c.f = 0.1f; W32(IN, c.u); c.f = -0.03f; W32(IN + 4, c.u); c.f = -0.03f; W32(IN + 8, c.u);
							light_begin(IN, 5, SRC); assert(host_light_rect(-1) == 1);
							assert(host_light_rect(0) > 160 && host_light_rect(0) < 215 && host_light_rect(2) > 425 && host_light_rect(2) < 480);
							assert(host_light_rect(1) > 80 && host_light_rect(1) < 135 && host_light_rect(3) > 345 && host_light_rect(3) < 400);
							c.f = 0.f; W32(IN, c.u); W32(IN + 4, c.u); W32(IN + 8, c.u);
							c.f = 0.2f; W32(SRC + 0x4C, c.u); c.f = 0.1f; W32(SRC + 0x50, c.u); c.f = 1.f; W32(SRC + 0x60, c.u); c.f = 4.f; W32(SRC + 0x64, c.u);
							light_begin(IN, 5, SRC);
						}
						host_light_pass(12, 1); assert(host_light_set()); host_light_pass(12, 0); light_end();
						c.f = -2.f; W32(IN + 0x28, c.u); light_begin(IN, 5, SRC); assert(host_light_rect(-1) == 0); light_end();   /* the beam behind: the sphere (holds the camera) */
						c.f = 2.f; W32(IN + 0x28, c.u); W32(SRC, 0); light_begin(IN, 5, SRC); assert(host_light_rect(-1) == 0); light_end();   /* a sphere light */
						W32(SRC, 1); W32(DEF, 0x40020u); light_begin(IN, 5, SRC); assert(host_light_rect(-1) == 0); light_end();   /* the camera's own: its sphere */
						W32(0x7F304, 0); W32(DEF, 0); W32(TT + 5 * 16 + 8, 0); for (q = 0; q < 0x70; q += 4) W32(SRC + q, 0); for (q = 0; q < 0x40; q += 4) W32(IN + q, 0);
						c.f = 1.f; W32(L + 0x24, c.u);
						printf("a beam light: its frustum's rectangle in its own frame (the weapon's flashlight); others their spheres\n");
					}
					host_set_lights(1); host_set_unlock_fps(0); light_begin(L + 0x84, 0, 0); assert(host_light_rect(-1) == 0); light_end();
				}
				{	/* 6i.91: the capture series - as you have it, no pixels drawn, no mip blending; nothing left on after */
					void host_capture_ab(void); void host_ab_state(s32 *ab, s32 *aniso, s32 *lightf); void host_set_lightf(s32 v);
					s32 ab, an, lf, seen[4] = { 0, 0, 0, 0 }, f;
					host_set_lightf(1);
					host_capture_ab(); host_ab_state(&ab, &an, &lf); assert(ab == 1 && lf == 1);
					for (f = 0; f < 600; f++)
					{
						W32(0x1DC0, M32h(0x1DC0) + 2); spooky_main_frame();
						host_ab_state(&ab, &an, &lf);
						if (ab >= 1 && ab <= 3) seen[ab] = 1; else break;
					}
					assert(seen[1] && seen[2] && seen[3] && ab == 0 && lf == 1);
					assert(logged("(as you have it,") >= 1 && logged("(no pixels drawn,") >= 1 && logged("(as you have it, again,") >= 1);   /* 6i.119 */
				}
				{	/* 6i.91: no pixels drawn - the world's and lights' passes into a 1 x 1 corner, put back after (texture
					 * accumulation for 1 and 14); others, the shadow texture's pass 10 among them, left alone */
					void pass_in(u32 k); void pass_out(u32 k); s32 host_px_set(void); void host_cap_flags(s32 nat, s32 pxoff, s32 nomip);
					host_cap_flags(0, 1, 0);
					pass_in(3); assert(host_px_set() == 1); pass_out(3); assert(host_px_set() == 0);
					pass_in(1); assert(host_px_set() == 2); pass_out(1); assert(host_px_set() == 0);
					pass_in(10); assert(host_px_set() == 0); pass_out(10);
					pass_in(21); assert(host_px_set() == 0); pass_out(21);
					host_cap_flags(0, 0, 0); pass_in(3); assert(host_px_set() == 0); pass_out(3);
				}
				{	/* 6i.91: no mip blending - any pass's draw (pass 3 here) unblends its mips, its anisotropy left as set, put back */
					void scr_pre(void); void scr_post(void); void pass_in(u32 k); void pass_out(u32 k); void host_cap_flags(s32 nat, s32 pxoff, s32 nomip);
					u32 TS = 0x7EF00, st;
					for (st = 0; st < 4u; st++) { W32(TS + 0x80 * st + 0x0C, 3); W32(TS + 0x80 * st + 0x10, 3); W32(TS + 0x80 * st + 0x14, 2); W32(TS + 0x80 * st + 0x20, 4); }
					host_cap_flags(0, 0, 1); pass_in(3); scr_pre();
					for (st = 0; st < 4u; st++) assert(M32h(TS + 0x80 * st + 0x14) == 1 && M32h(TS + 0x80 * st + 0x0C) == 3 && M32h(TS + 0x80 * st + 0x20) == 4);
					scr_post(); for (st = 0; st < 4u; st++) assert(M32h(TS + 0x80 * st + 0x14) == 2);
					pass_out(3); host_cap_flags(0, 0, 0); pass_in(3); scr_pre(); assert(M32h(TS + 0x14) == 2); scr_post(); pass_out(3);
					for (st = 0; st < 4u; st++) { W32(TS + 0x80 * st + 0x0C, 0); W32(TS + 0x80 * st + 0x10, 0); W32(TS + 0x80 * st + 0x14, 0); W32(TS + 0x80 * st + 0x20, 0); }
				}
				{	/* 6i.91: the GPU's timeline - each stamp's time to the next goes to the part it began (29 the draw's start, a
					 * pass's number, 28 after one, a mark's id), the first frame only setting the start; the draw's total too */
					void pass_in(u32 k); void pass_out(u32 k); void mark(u32 id); void prof_present_before(void); void prof_present_after(void);
					void host_cap_flags(s32 nat, s32 pxoff, s32 nomip); u32 host_nt_sum(s32 id); extern u32 g_host_vt_on, g_host_vt_t[128];
					static const u32 tm[7] = { 1000, 2000, 5000, 5500, 9500, 9800, 10000 };
					s32 fr, q;
					for (q = 0; q < 7; q++) g_host_vt_t[16 + q] = tm[q];
					g_host_vt_on = 1; host_cap_flags(1, 0, 0);
					for (fr = 0; fr < 2; fr++) { mark(29); pass_in(1); pass_out(1); pass_in(3); pass_out(3); mark(41); prof_present_before(); prof_present_after(); }
					assert(host_nt_sum(29) == 1000 && host_nt_sum(1) == 3000 && host_nt_sum(28) == 800 && host_nt_sum(3) == 4000 && host_nt_sum(41) == 200);
					assert(host_nt_sum(-1) == 9000);
					g_host_vt_on = 0; host_cap_flags(0, 0, 0);
					printf("capture series (as you have it / no pixels drawn / no mip blending) and the GPU's own timeline by part\n");
				}
				{	/* 6i.98: a draw's GPU cost fitted against its count - records costing 20 us + 2 ns a vertex come back as that;
					 * and the capture's draws' frames sum each draw's count into its record */
					void fit_line(const double *s, double *a, double *b); void scr_pre(void); void scr_post(void); extern u32 spooky_draw_n;
					void host_cap_live(s32 live, s32 rec); double s[5] = { 0, 0, 0, 0, 0 }, a, b; u32 v;
					for (v = 1000; v <= 9000; v += 1000) { double x = v, y = 20000.0 + 2.0 * x; s[0] += 1; s[1] += x; s[2] += y; s[3] += x * y; s[4] += x * x; }
					fit_line(s, &a, &b); assert(a > 19999.0 && a < 20001.0 && b > 1.999 && b < 2.001);
					s[0] = 1; s[1] = 500; s[2] = 9000; s[3] = 4500000; s[4] = 250000; fit_line(s, &a, &b); assert(b == 0.0 && a > 8999.0 && a < 9001.0);   /* one point: all fixed */
					printf("a draw's GPU cost: fixed part and per-vertex part fitted\n");
				}
				{	/* 6i.111: the light's draws classed by whether the camera's passes read an overlapping index range this frame;
					 * the unseen ones skipped with the setting on - seen ones never; too many camera draws: none skipped */
					void scr_pre(void); void scr_post(void); void pass_in(u32 k); void pass_out(u32 k); void rec_in(u32 key, u32 mode); void rec_out(void);
					void light_begin(u32 source, u32 tag, u32 shape); void light_end(void); void host_cap_live(s32 live, s32 rec); u32 host_un(s32 c);
					void host_set_light_skip(s32 v); s32 cam_seen(u32 a, u32 n); extern u32 spooky_draw_ip, spooky_draw_n; extern u8 spooky_skip_draw;
					u32 u0 = host_un(0), s0 = host_un(1);
					host_cap_live(1, 1);
					pass_in(1); spooky_draw_ip = 0x10000; spooky_draw_n = 100; scr_pre(); scr_post(); spooky_draw_ip = 0x20000; spooky_draw_n = 300; scr_pre(); scr_post(); pass_out(1);
					assert(cam_seen(0x20100, 10) && cam_seen(0x1FFF0, 16) && !cam_seen(0x1FFF0, 8) && !cam_seen(0x20258, 4) && cam_seen(0x200C8, 50));
					light_begin(0, 0, 0); pass_in(12);
					rec_in(0x8111u, 12); spooky_draw_ip = 0x20100; spooky_draw_n = 60; scr_pre(); assert(!spooky_skip_draw); scr_post(); rec_out();   /* inside the camera's: seen */
					host_set_light_skip(0);
					rec_in(0x8112u, 12); spooky_draw_ip = 0x30000; scr_pre(); assert(!spooky_skip_draw); scr_post(); rec_out();                       /* unseen - counted, not skipped (off) */
					host_set_light_skip(1);
					rec_in(0x8113u, 12); spooky_draw_ip = 0x40000; scr_pre(); assert(spooky_skip_draw == 1); spooky_skip_draw = 0; scr_post(); rec_out();   /* unseen, on: skipped */
					rec_in(0x8114u, 12); spooky_draw_ip = 0x20000; scr_pre(); assert(!spooky_skip_draw); scr_post(); rec_out();                       /* seen, on: drawn */
					pass_out(12); pass_in(11);
					rec_in(0x8115u, 11); spooky_draw_ip = 0x50000; spooky_draw_n = 60; scr_pre(); assert(spooky_skip_draw == 1); spooky_skip_draw = 0; scr_post(); rec_out();   /* 6i.112: the shadow pass too */
					pass_out(11); light_end();
					pass_in(7); spooky_draw_ip = 0x60000; scr_pre(); scr_post(); pass_out(7);   /* 6i.112: any pass outside a light is the camera's */
					light_begin(0, 0, 0); pass_in(12); rec_in(0x8116u, 12); spooky_draw_ip = 0x60010; spooky_draw_n = 4; scr_pre(); assert(!spooky_skip_draw); scr_post(); rec_out(); pass_out(12); light_end();
					host_set_light_skip(1);
					spooky_draw_ip = 0; spooky_draw_n = 0; host_cap_live(0, 0);
					assert(host_un(0) == u0 + 3 && host_un(1) == s0 + 3);
					printf("the light's draws: index ranges the camera read this frame or not; unseen ones skipped only with the setting on\n");
				}
				{	/* 6i.92: World Mip Blending off - the world's passes draw from the nearest mip level (Performance Mode only);
					 * a light's passes are Light Filtering's; on (the default) changes nothing */
					void scr_pre(void); void scr_post(void); void pass_in(u32 k); void pass_out(u32 k); void host_set_world_mip(s32 v);
					void host_set_unlock_fps(s32 on); void host_set_lightf(s32 v); u32 TS = 0x7EF00, st;
					for (st = 0; st < 4u; st++) { W32(TS + 0x80 * st + 0x0C, 2); W32(TS + 0x80 * st + 0x10, 2); W32(TS + 0x80 * st + 0x14, 2); W32(TS + 0x80 * st + 0x20, 0); }
					host_set_unlock_fps(1); host_set_lightf(0); host_set_world_mip(0);
					pass_in(3); scr_pre(); for (st = 0; st < 4u; st++) assert(M32h(TS + 0x80 * st + 0x14) == 1 && M32h(TS + 0x80 * st + 0x0C) == 2);
					scr_post(); for (st = 0; st < 4u; st++) assert(M32h(TS + 0x80 * st + 0x14) == 2); pass_out(3);
					pass_in(12); scr_pre(); assert(M32h(TS + 0x14) == 2); scr_post(); pass_out(12);
					host_set_unlock_fps(0); pass_in(3); scr_pre(); assert(M32h(TS + 0x14) == 2); scr_post(); pass_out(3);
					host_set_unlock_fps(1); host_set_world_mip(1); pass_in(3); scr_pre(); assert(M32h(TS + 0x14) == 2); scr_post(); pass_out(3);
					for (st = 0; st < 4u; st++) { W32(TS + 0x80 * st + 0x0C, 0); W32(TS + 0x80 * st + 0x10, 0); W32(TS + 0x80 * st + 0x14, 0); W32(TS + 0x80 * st + 0x20, 0); }
					host_set_lightf(2); host_set_unlock_fps(0); host_set_world_mip(0);   /* 6i.104: off is the default */
				}
				{	/* 6i.92: a render target switch starts the capture's part 27 (the GPU made to finish there), counted in its parts' frames */
					void rt_switch(u32 surf); extern u32 spooky_pass_cyc[64]; void pass_in(u32 k); void pass_out(u32 k); void mark(u32 id); void prof_present_before(void);
					void host_set_gpu_time(s32 on); void host_cap_live(s32 live, s32 rec); void host_tsc(u32 v, u32 step); u32 host_rt_n(void); u32 k, n0 = host_rt_n();
					for (k = 0; k < 64; k++) spooky_pass_cyc[k] = 0;
					host_set_gpu_time(1); host_cap_live(1, 0);
					host_tsc(100000, 0); mark(29);
					host_tsc(101000, 0); W32(0x7F900 + 0x10, 0x271DF13Fu); rt_switch(0x7F900u);   /* 6i.106: a 320-wide target -> part 23 */
					host_tsc(103000, 0); pass_in(3);
					host_tsc(104000, 0); pass_out(3);
					host_tsc(104500, 0); prof_present_before();
					assert(spooky_pass_cyc[29] == 1000 && spooky_pass_cyc[23] == 2000 && spooky_pass_cyc[3] == 1000 && spooky_pass_cyc[28] == 500 && host_rt_n() == n0 + 1);
					W32(0x7F900 + 0x10, 0);
					host_set_gpu_time(0); host_cap_live(0, 0); host_tsc(0, 1000);
					for (k = 0; k < 64; k++) spooky_pass_cyc[k] = 0;
					printf("World Mip Blending off: the world's passes from the nearest level; a render target switch is part 27\n");
				}
				{	/* 6i.90: Light Filtering faster - in a light's passes a stage blending mip levels draws from the nearest level
					 * (anisotropic off as fast), put back after; fast leaves the mips alone; outside those passes nothing changes */
					void scr_pre(void); void scr_post(void); void host_set_lightf(s32 v); void pass_in(u32 k); void pass_out(u32 k);
					u32 TS = 0x7EF00, st;
					for (st = 0; st < 4u; st++) { W32(TS + 0x80 * st + 0x0C, 2); W32(TS + 0x80 * st + 0x10, 2); W32(TS + 0x80 * st + 0x14, 0); W32(TS + 0x80 * st + 0x20, 0); }
					W32(TS + 0x80 + 0x14, 2); W32(TS + 0x100 + 0x14, 2); W32(TS + 0x100 + 0x0C, 3); W32(TS + 0x100 + 0x10, 3); W32(TS + 0x100 + 0x20, 4);
					host_set_lightf(2); pass_in(12); scr_pre();
					assert(M32h(TS + 0x80 + 0x14) == 1 && M32h(TS + 0x100 + 0x14) == 1 && M32h(TS + 0x14) == 0 && M32h(TS + 0x180 + 0x14) == 0);
					assert(M32h(TS + 0x100 + 0x0C) == 2 && M32h(TS + 0x100 + 0x10) == 2 && M32h(TS + 0x100 + 0x20) == 0);
					scr_post();
					assert(M32h(TS + 0x80 + 0x14) == 2 && M32h(TS + 0x100 + 0x14) == 2 && M32h(TS + 0x100 + 0x0C) == 3 && M32h(TS + 0x100 + 0x20) == 4);
					host_set_lightf(1); scr_pre();
					assert(M32h(TS + 0x80 + 0x14) == 2 && M32h(TS + 0x100 + 0x14) == 2 && M32h(TS + 0x100 + 0x0C) == 2);
					scr_post(); pass_out(12);
					host_set_lightf(2); scr_pre(); assert(M32h(TS + 0x80 + 0x14) == 2 && M32h(TS + 0x100 + 0x0C) == 3); scr_post();
					for (st = 0; st < 4u; st++) { W32(TS + 0x80 * st + 0x0C, 0); W32(TS + 0x80 * st + 0x10, 0); W32(TS + 0x80 * st + 0x14, 0); W32(TS + 0x80 * st + 0x20, 0); }
					host_set_lightf(1);
					printf("Light Filtering faster: the lights' mip levels unblended for their draws only, put back after\n");
				}
				{	/* 6i.88: anisotropic off in the world's passes only (the capture's third) - the material setup hook clamps in
					 * passes 1, 3, 5, 8, 20 and leaves every other pass's as the game set it */
					void aniso_clamp(u32 range); void host_set_aniso_world(s32 v); void pass_in(u32 k); void pass_out(u32 k);
					extern volatile u8 spooky_aniso; u32 R = 0x7F400, TB = 0x7F000; u8 was = spooky_aniso;
					W32(R, 1u << 9); spooky_aniso = 1; host_set_aniso_world(1);
					W32(TB + 0x0C, 3); W32(TB + 0x10, 3); W32(TB + 0x20, 4);
					pass_in(3); aniso_clamp(R); assert(M32h(TB + 0x10) == 2 && M32h(TB + 0x0C) == 2 && M32h(TB + 0x20) == 0); pass_out(3);
					W32(TB + 0x0C, 3); W32(TB + 0x10, 3); W32(TB + 0x20, 4);
					pass_in(12); aniso_clamp(R); assert(M32h(TB + 0x10) == 3 && M32h(TB + 0x20) == 4); pass_out(12);
					aniso_clamp(R); assert(M32h(TB + 0x10) == 3);                                   /* between passes: left */
					host_set_aniso_world(0); aniso_clamp(R); assert(M32h(TB + 0x10) == 2);          /* plain off: everywhere */
					spooky_aniso = was;
				}
				{	/* 6i.87: Light Filtering at the draw - in pass 11 or 12, an anisotropic stage linear with no anisotropy for
					 * the draw and put back after; other passes, other stages, or game's: left alone */
					void scr_pre(void); void scr_post(void); extern volatile u8 spooky_scr_set; void host_set_lightf(s32 v);
					void pass_in(u32 k); void pass_out(u32 k); u32 TS1 = 0x7EF00 + 0x80, TS2 = 0x7EF00 + 0x100;
					host_set_lightf(1);
					W32(0x7EE00, 0); W32(0x7EE04, 0); W32(0x7EE08, 0); W32(0x7EE0C, 0);
					W32(TS1 + 0xC, 3); W32(TS1 + 0x10, 3); W32(TS1 + 0x20, 4); W32(TS2 + 0xC, 2); W32(TS2 + 0x10, 2); W32(TS2 + 0x20, 0); W32(0x7F200, 0);
					pass_in(12);
					scr_pre(); assert(spooky_scr_set && M32h(TS1 + 0xC) == 2 && M32h(TS1 + 0x10) == 2 && M32h(TS1 + 0x20) == 0 && (M32h(0x7F200) & 2));
					assert(M32h(TS2 + 0xC) == 2 && M32h(TS2 + 0x10) == 2);                                   /* not anisotropic: left */
					scr_post(); assert(!spooky_scr_set && M32h(TS1 + 0xC) == 3 && M32h(TS1 + 0x10) == 3 && M32h(TS1 + 0x20) == 4);
					W32(TS1 + 0x10, 2); W32(TS1 + 0xC, 2); W32(TS1 + 0x20, 4);
					scr_pre(); assert(spooky_scr_set && M32h(TS1 + 0x20) == 0); scr_post(); assert(M32h(TS1 + 0x20) == 4);   /* max anisotropy alone */
					pass_out(12); W32(TS1 + 0x10, 3);
					scr_pre(); assert(!spooky_scr_set && M32h(TS1 + 0x10) == 3);                             /* between passes: left */
					pass_in(3); scr_pre(); assert(!spooky_scr_set); pass_out(3);                             /* the world's passes: left */
					pass_in(11); scr_pre(); assert(spooky_scr_set); scr_post(); pass_out(11);                /* the shadows' pass */
					host_set_lightf(0); pass_in(12); scr_pre(); assert(!spooky_scr_set); pass_out(12); host_set_lightf(1);   /* game's */
				}
				{	/* 6i.73: the projections' half width and height cut by the share with the rest, back at the stretch */
					union { float f; u32 u; } a, b; u32 HW = 0x7E900 + 0xB8, FW = 0x1200 + 0x8C + 0xB8;
					a.f = 320.f; W32(HW, a.u); W32(FW, a.u); a.f = 240.f; W32(HW + 4, a.u); W32(FW + 4, a.u);
					NF(); W32(GOF + 8, 1); host_set_unlock_fps(1); host_set_rscale(5); RSF(0, 0, 480, 640); rs_cut();
					b.u = M32h(HW); assert(b.f == 160.f); b.u = M32h(FW + 4); assert(b.f == 120.f);
					spooky_view(0); b.u = M32h(HW); assert(b.f == 320.f); b.u = M32h(FW + 4); assert(b.f == 240.f);
					host_set_rscale(10); host_set_unlock_fps(0);
				}
				#undef NF
				W32(0x1D10, GOP); W32(FS, f0); W32(FS + 4, f1); host_set_rscale(10); host_set_unlock_fps(0);
				#undef RSF
			}
		}
		host_set_unlock_fps(1); spooky_main_frame();
	}
	{
		/* 6i.40: the split-screen levers - after the game's own (one view: LoD 4, the flags on, 0.85 / 1.0): each on its
		 * own; with split-screen's values (LoD 3) or Performance Mode off: left as the game set them */
		void host_set_coop(s32 lod, s32 passes, s32 lights, s32 detail); void coop_apply(void); void host_set_unlock_fps(s32 on);
		u32 b;
#define W8(a, v) (MEM[(a)] = (u8)(v))
#define M8h(a) (MEM[(a)])
		host_set_unlock_fps(1);
#define COOP_ONE() do { W32(0x1EC0, 4); for (b = 0; b < 6; b++) W8(0x1EC4 + b, 1); W32(0x1ECC, 0x3F800000u); W32(0x1ED0, 0x3F59999Au); } while (0)
		COOP_ONE(); host_set_coop(0, 0, 0, 0); coop_apply();
		assert(M32h(0x1EC0) == 4 && M8h(0x1EC6) == 1 && M8h(0x1EC4) == 1 && M32h(0x1ED0) == 0x3F59999Au);
		COOP_ONE(); host_set_coop(1, 0, 0, 0); coop_apply(); assert(M32h(0x1EC0) == 3 && M8h(0x1EC6) == 1);
		COOP_ONE(); host_set_coop(0, 1, 0, 0); coop_apply();
		assert(M32h(0x1EC0) == 4 && M8h(0x1EC6) == 0 && M8h(0x1EC7) == 0 && M8h(0x1EC8) == 0 && M8h(0x1EC4) == 0 && M8h(0x1EC9) == 0 && M8h(0x1EC5) == 1);   /* 6i.120: one setting - all five */
		COOP_ONE(); host_set_coop(0, 0, 1, 0); coop_apply(); assert(M8h(0x1EC4) == 0 && M8h(0x1EC9) == 0 && M8h(0x1EC6) == 0);   /* 6i.120: the same one setting */
		COOP_ONE(); host_set_coop(0, 0, 0, 1); coop_apply(); assert(M32h(0x1ED0) == 0x3E99999Au && M32h(0x1ECC) == 0x3F4CCCCDu && M32h(0x1EC0) == 4);
		COOP_ONE(); W32(0x1EC0, 3); host_set_coop(1, 1, 1, 1); coop_apply(); assert(M32h(0x1EC0) == 3 && M8h(0x1EC6) == 1);   /* split-screen: its own */
		COOP_ONE(); host_set_unlock_fps(0); coop_apply(); assert(M32h(0x1EC0) == 4 && M8h(0x1EC6) == 1);                       /* Performance Mode off */
		host_set_coop(0, 0, 0, 0); host_set_unlock_fps(1);
#undef COOP_ONE
#undef W8
#undef M8h
		printf("Performance Mode Settings: VSync, Late Frames, FPS Limit, Anisotropic Filtering (6i.53: off / 2x fast / 2x / full); the four split-screen levers, each on its own\n");
	}
	{
		/* 6i.120: Field of View - the game's own read on foot (78 here) and marked; another value changes only the copy the
		 * view is built from, in tangent space (a zoomed view scaled the same); away from your eye (a cutscene) or at the
		 * game's own: the record as the game has it */
		void host_set_fov(s32 v); s32 host_fov_get(void); s32 host_fov_def(void); void host_set_free_cam(s32 on, s32 block);
		u32 cam = 0x48000, r, p14 = M32h(OBJECT + 0x14);
		float g = 1.3613568f, want;
		memset(MEM + cam, 0, 0x358);
		state(2100, 0.f, 0, 0);
		host_set_third_person(0, 10, 2, 0); host_set_free_cam(0, 1);
		W32(OBJECT + 0x14, 0xFFFFFFFFu);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		WF(cam, 0.f); WF(cam + 4, 0.f); WF(cam + 8, 0.6f); WF(cam + 0x20, 1.f); WF(cam + 0x34, 1.f); WF(cam + 0x4C, g);
		MEM[OBJECT + 0x240] = 0xFF; MEM[OBJECT + 0x241] = 0xFF;
		host_set_fov(0);
		r = host_tp_camera(cam, 0);
		assert(r == cam && host_fov_def() == 78 && host_fov_get() == 78);
		host_set_fov(90); r = host_tp_camera(cam, 0);
		assert(r != cam && fabsf(M32f(r + 0x4C) - 1.5707963f) < 2e-3f && M32f(cam + 0x4C) == g && M32f(r + 8) == 0.6f);
		MEM[OBJECT + 0x240] = 1; WF(cam + 0x4C, 0.5f); r = host_tp_camera(cam, 0);
		want = 2.f * atanf(tanf(0.25f) * tanf(0.7853982f) / tanf(g * 0.5f));
		assert(r != cam && fabsf(M32f(r + 0x4C) - want) < 2e-3f && host_fov_def() == 78);   /* zoomed: scaled alike, the default kept */
		MEM[OBJECT + 0x240] = 0xFF; WF(cam + 0x4C, g);
		WF(cam, 10.f); assert(host_tp_camera(cam, 0) == cam);                 /* away from your eye: the game's */
		WF(cam, 0.f); host_set_fov(78); assert(host_tp_camera(cam, 0) == cam); /* the game's own: untouched */
		{
			/* 6i.215: the game's own stays "55" for the retail angle's whole neighbourhood - 0.967 rad (55.4 degrees) and
			 * 0.975 (55.9, which rounding alone made 56) are both 55, so the menu's * does not move; 57 degrees is 57.
			 * 6i.219: the water's edges - in widescreen the view is 2.5 % wider at Field of View 55* and 2 % wider at 56,
			 * and untouched at every other setting; not in 4:3; not when the view is not your eye */
			void host_aspect_query(void);
			float a0, w56 = 2.f * atanf(tanf(28.f / 57.29578f)), w54 = 2.f * atanf(tanf(27.f / 57.29578f)), w57 = 2.f * atanf(tanf(28.5f / 57.29578f));
			WF(cam + 0x4C, 0.967f); host_set_fov(0); assert(host_tp_camera(cam, 0) == cam && host_fov_def() == 55 && host_fov_get() == 55);
			WF(cam + 0x4C, 0.975f); assert(host_tp_camera(cam, 0) == cam && host_fov_def() == 55);
			WF(cam + 0x4C, 0.9948f); host_tp_camera(cam, 0); assert(host_fov_def() == 57);
			WF(cam + 0x4C, 0.967f); host_set_fov(55); assert(host_tp_camera(cam, 0) == cam && host_fov_def() == 55);   /* 4:3: the game's own, untouched */
			host_set_widescreen(1, 0); host_aspect_query();
			r = host_tp_camera(cam, 0); memcpy(&a0, MEM + r + 0x4C, 4);
			assert(r != cam && fabsf(a0 - 0.967f * 1.025f) < 1e-5f && M32f(cam + 0x4C) == 0.967f);        /* 55*: 2.5 % wider */
			{
				/* 6i.225: in widescreen 56 and 57 trade angles so the view only widens as the number goes up - 56 gives 57
				 * degrees exactly, 57 gives 56 degrees 2 % wider; 55* < 56 < 57 < 58; 54 its own */
				float a55 = 0.967f * 1.025f, a56, a57, a58, w58 = 2.f * atanf(tanf(29.f / 57.29578f));
				host_set_fov(56); r = host_tp_camera(cam, 0); memcpy(&a56, MEM + r + 0x4C, 4);
				assert(r != cam && fabsf(a56 - w57) < 2e-3f);
				host_set_fov(57); r = host_tp_camera(cam, 0); memcpy(&a57, MEM + r + 0x4C, 4);
				assert(r != cam && fabsf(a57 - w56 * 1.02f) < 2e-3f);
				host_set_fov(58); r = host_tp_camera(cam, 0); memcpy(&a58, MEM + r + 0x4C, 4); assert(fabsf(a58 - w58) < 2e-3f);
				assert(a55 < a56 && a56 < a57 && a57 < a58);
				host_set_fov(54); r = host_tp_camera(cam, 0); memcpy(&a0, MEM + r + 0x4C, 4); assert(fabsf(a0 - w54) < 2e-3f && a0 < a55);
				host_set_widescreen(0, 0); host_aspect_query();                                           /* 4:3: nothing traded */
				host_set_fov(56); r = host_tp_camera(cam, 0); memcpy(&a0, MEM + r + 0x4C, 4); assert(fabsf(a0 - w56) < 2e-3f);
				host_set_fov(57); r = host_tp_camera(cam, 0); memcpy(&a0, MEM + r + 0x4C, 4); assert(fabsf(a0 - w57) < 2e-3f);
				host_set_widescreen(1, 0); host_aspect_query();
			}
			host_set_fov(55); WF(cam, 10.f); assert(host_tp_camera(cam, 0) == cam); WF(cam, 0.f);         /* not your eye: the game's */
			host_set_widescreen(0, 1); host_aspect_query(); r = host_tp_camera(cam, 0); assert(r != cam);   /* the other widescreen sign: also widened */
			host_set_widescreen(0, 0); host_aspect_query(); assert(host_tp_camera(cam, 0) == cam);
			{
				/* 6i.220: coming out of the scope the unit is "not zoomed" at once while the camera's angle eases back over
				 * some frames. Those half-way angles are NOT taken as the game's own: the settled 0.967 stands, so the view is
				 * scaled by the same factor all the way (here Field of View 90 - a factor of tan 45 / tan 27.7) instead of
				 * jumping straight to 90 degrees; once the angle has held still for the settling frames it is learned again;
				 * a new angle that holds (another body) is learned after the same wait */
				void host_set_fov_settle(s32 n); void host_frames_add(u32 n); int q; float kk = tanf(45.f / 57.29578f) / tanf(0.967f * 0.5f), ease[4] = { 0.55f, 0.70f, 0.85f, 0.95f };
				host_set_fov_settle(10);
				WF(cam + 0x4C, 0.967f); host_set_fov(90);
				for (q = 0; q < 12; q++) { host_frames_add(1); r = host_tp_camera(cam, 0); }
				assert(host_fov_def() == 55 && fabsf(M32f(r + 0x4C) - 1.5707963f) < 2e-3f);
				for (q = 0; q < 4; q++)
				{
					float want = 2.f * atanf(tanf(ease[q] * 0.5f) * kk);
					host_frames_add(1); WF(cam + 0x4C, ease[q]); r = host_tp_camera(cam, 0);
					assert(host_fov_def() == 55 && fabsf(M32f(r + 0x4C) - want) < 3e-3f && M32f(r + 0x4C) < 1.56f);   /* eased, not snapped to 90 */
				}
				WF(cam + 0x4C, 0.967f); for (q = 0; q < 12; q++) { host_frames_add(1); r = host_tp_camera(cam, 0); }
				assert(host_fov_def() == 55 && fabsf(M32f(r + 0x4C) - 1.5707963f) < 2e-3f);
				WF(cam + 0x4C, 1.2217f);                                                  /* 70 degrees, and it holds */
				for (q = 0; q < 9; q++) { host_frames_add(1); host_tp_camera(cam, 0); } assert(host_fov_def() == 55);
				for (q = 0; q < 3; q++) { host_frames_add(1); host_tp_camera(cam, 0); } assert(host_fov_def() == 70);
				host_set_fov_settle(0);
			}
			WF(cam + 0x4C, g); host_set_fov(0); host_tp_camera(cam, 0); host_set_fov(78);
		}
		{
			/* 6i.126: the overlays' field of view is the frame's first view build's (the world's): a second build in the same
			 * frame - not changed - leaves it; the next frame starts afresh */
			float host_fov_used(void); void host_fov_frame(void);
			host_fov_frame(); host_set_fov(90); host_tp_camera(cam, 0);
			assert(fabsf(host_fov_used() - 1.5707963f) < 2e-3f);
			WF(cam, 10.f); host_tp_camera(cam, 0); assert(fabsf(host_fov_used() - 1.5707963f) < 2e-3f);   /* a later build: kept */
			host_fov_frame(); host_tp_camera(cam, 0); assert(fabsf(host_fov_used() - g) < 1e-4f);          /* next frame: its own first */
			WF(cam, 0.f); host_set_fov(78); host_fov_frame();
		}
		W32(OBJECT + 0x14, p14);
		printf("Field of View: the game's own read on foot (78) and kept; 90 changes only the view's copy (tangent space, zoom scaled alike); cutscenes untouched\n");
	}
	{
		/* 6i.192: Show Stored Velocity - "STORED" is a body that is ASLEEP (its island on the world's inactive list) with a
		 * velocity of 0.5 u/s or more: shown at once, gone when it wakes or the velocity has been under 0.5 for 15 ticks.
		 * Awake, nothing shows whatever the body does - standing with a velocity, a turn-round, a jump, a landing (the
		 * movement rules of 6i.120 - 6i.190 only write to the log now). Its arrow: 0.08 units a u/s, 3 at most. */
		s32 host_sv_step(u32 obj, u32 tick, float px, float py, float pz, float vx, float vy, float vz); float host_sv_speed(void);
		void host_sv_asleep(s32 on); void host_sv_foot(float x, float y, float z); s32 host_sv_arrow(float *from, float *to);
		float fr[3], to[3];
		int k;
		/* awake: a velocity with no movement for six ticks - what the old rules took - is not shown; the log says so */
		host_sv_asleep(0);
		assert(!host_sv_step(1, 100, 0.f, 0.f, 0.f, 6.f, 0.f, 0.f));
		for (k = 101; k <= 106; k++) assert(!host_sv_step(1, (u32)k, 0.f, 0.f, k & 1 ? 0.03f : 0.f, 6.f, 0.f, 0.f));
		assert(logged("stored velocity: NOT shown (the body is awake) - the ") && !host_sv_arrow(fr, to));
		/* awake: a turn-round (your log: 3.25 u/s, then 1.13 with the body back near where it was) */
		assert(!host_sv_step(1, 300, 0.f, 0.f, 0.f, 3.25f, 0.f, 0.f) && !host_sv_step(1, 301, 0.108f, 0.f, 0.f, 3.25f, 0.f, 0.f) && !host_sv_step(1, 302, 0.032f, 0.f, 0.f, -0.56f, -0.98f, 0.f) && !host_sv_step(1, 303, 0.01f, -0.03f, 0.f, -0.56f, -0.98f, 0.f));
		/* awake: a jump - standing, the tick the velocity appears, rising, falling, landing */
		assert(!host_sv_step(1, 500, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f) && !host_sv_step(1, 501, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f) && !host_sv_step(1, 502, 0.f, 0.f, 0.f, 0.f, 0.f, 2.6f));
		assert(!host_sv_step(1, 503, 0.f, 0.f, 0.082f, 0.f, 0.f, 2.46f) && !host_sv_step(1, 504, 0.f, 0.f, 0.159f, 0.f, 0.f, 2.32f) && !host_sv_step(1, 505, 0.f, 0.f, 0.08f, 0.f, 0.f, -2.5f));
		assert(!host_sv_step(1, 506, 0.f, 0.f, 0.f, 0.f, 0.f, -2.6f) && !host_sv_step(1, 507, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f));
		/* asleep with a velocity: stored at once (the reading after the one that starts the row) */
		host_sv_asleep(1); host_sv_foot(10.f, 20.f, 5.f);
		assert(!host_sv_step(1, 700, 0.f, 0.f, 0.f, 0.f, 0.6f, 0.f));
		assert(host_sv_step(1, 701, 0.f, 0.f, 0.f, 0.f, 0.6f, 0.f) && fabsf(host_sv_speed() - 0.6f) < 1e-4f);
		assert(host_sv_arrow(fr, to) && fr[0] == 10.f && fr[1] == 20.f && fr[2] == 5.f && fabsf(to[1] - (20.f + 0.048f)) < 1e-4f && fabsf(to[0] - 10.f) < 1e-5f);   /* 0.6 u/s: 0.048 long */
		assert(host_sv_step(1, 702, 0.f, 0.f, 0.f, 6.f, 8.f, 0.f) && fabsf(host_sv_speed() - 10.f) < 1e-3f && host_sv_arrow(fr, to)
		       && fabsf(to[0] - (10.f + 0.6f * 0.8f)) < 1e-4f && fabsf(to[1] - (20.f + 0.8f * 0.8f)) < 1e-4f);   /* the velocity it holds changes: the figure and the arrow with it */
		assert(host_sv_step(1, 703, 0.f, 0.f, 0.f, 0.f, -90.f, 0.f) && host_sv_arrow(fr, to) && fabsf(to[1] - (20.f - 3.f)) < 1e-4f);   /* 90 u/s: 3 at most */
		/* it wakes: gone at once */
		host_sv_asleep(0);
		assert(!host_sv_step(1, 704, 0.f, 0.f, 0.f, 0.f, -90.f, 0.f) && !host_sv_arrow(fr, to));
		/* asleep with under 0.5 u/s: not stored */
		host_sv_asleep(1);
		assert(!host_sv_step(1, 900, 0.f, 0.f, 0.f, 0.f, 0.4f, 0.f) && !host_sv_step(1, 901, 0.f, 0.f, 0.f, 0.f, 0.4f, 0.f) && !host_sv_step(1, 902, 0.f, 0.f, 0.f, 0.f, 0.4f, 0.f));
		/* stored, then its velocity under 0.5 for 15 ticks: fizzled out, and not taken again while it stays under */
		assert(host_sv_step(1, 903, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f));
		for (k = 0; k < 14; k++) assert(host_sv_step(1, 904u + (u32)k, 0.f, 0.f, 0.f, 0.f, 0.1f, 0.f));
		assert(!host_sv_step(1, 918, 0.f, 0.f, 0.f, 0.f, 0.1f, 0.f) && !host_sv_step(1, 919, 0.f, 0.f, 0.f, 0.f, 0.1f, 0.f));
		assert(host_sv_step(1, 920, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f));
		/* a revert or a load (the tick jumps): starts over */
		assert(!host_sv_step(1, 2000, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f) && host_sv_step(1, 2001, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f));
		assert(!host_sv_step(1, 100, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f));
		host_sv_asleep(0); assert(!host_sv_step(1, 3000, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f));
		printf("Show Stored Velocity: a body asleep with 0.5 u/s or more - at once; gone when it wakes or fizzles; awake never; its arrow 0.08 a u/s, 3 at most\n");
	}
	{
		/* 6i.120 / 6i.121: Tick Rate - 60 / 25 in a game (the speed on top), the collision rebuilt once each time the rate in
		 * the time globals changes in a game (not every frame, not for a speed change, not on the main menu) */
		void host_set_trate(s32 v); u32 host_rebuilt(void); void host_clear_toast(void); void host_set_speed(s32 tenths, s32 pause);
		const u32 GO = 0x4D000;
		u32 oopt = M32h(0x1D10), o0 = M32h(TIME), o4 = M32h(TIME + 4), oc = M32h(TIME + 0xC), o18 = M32h(TIME + 0x18), o20 = M32h(TIME + 0x20), og8 = M32h(GO + 8), ogc = M32h(GO + 0xC), rb;
		float sp, spt;
		int tps;
		#define RATE2() (memcpy(&sp, MEM + TIME + 0xC, 4), memcpy(&spt, MEM + TIME + 4, 4), tps = (int)(short)M16h(TIME + 2))
		#define IS2(T, S) (RATE2(), tps == (T) && sp == (S) && spt == 1.f / (float)(T))
		W32(0x1D10, GO); W32(GO + 8, 1); MEM[GO + 0xE] = 30; MEM[GO + 0xF] = 0;
		MEM[TIME] = 1; MEM[TIME + 1] = 0; MEM[TIME + 2] = 30; MEM[TIME + 3] = 0; WF(TIME + 4, 1.f / 30.f); WF(TIME + 0xC, 1.f); WF(TIME + 0x18, 0.f); WF(TIME + 0x20, 0.f);
		host_set_speed(10, 0); frame(); assert(IS2(30, 1.f));
		rb = host_rebuilt();
		host_set_trate(2); frame(); assert(IS2(60, 1.f) && host_rebuilt() == rb + 1);
		frame(); frame(); assert(IS2(60, 1.f) && host_rebuilt() == rb + 1);
		host_set_speed(5, 0); frame(); assert(IS2(60, 0.5f) && host_rebuilt() == rb + 1);
		MEM[TIME + 2] = 30; WF(TIME + 4, 1.f / 30.f); frame(); assert(IS2(60, 0.5f) && host_rebuilt() == rb + 2);   /* a revert's 30: ours again, rebuilt */
		host_set_speed(10, 0); host_set_trate(0); frame(); assert(IS2(25, 1.f) && host_rebuilt() == rb + 3);
		W32(GO + 8, 3); frame(); assert(IS2(30, 1.f) && host_rebuilt() == rb + 3);   /* the main menu: the game's, nothing rebuilt */
		W32(GO + 8, 1); host_set_trate(1); frame(); assert(IS2(30, 1.f));
		host_clear_toast();
		W32(0x1D10, oopt); W32(TIME, o0); W32(TIME + 4, o4); W32(TIME + 0xC, oc); W32(TIME + 0x18, o18); W32(TIME + 0x20, o20); W32(GO + 8, og8); W32(GO + 0xC, ogc);
		#undef RATE2
		#undef IS2
		printf("Tick Rate: 60 / 25 in a game, the speed on top; the collision rebuilt once per rate change (a revert's 30 too), never on the main menu\n");
	}
	{
		/* 6i.121: Force Double Revert - the other slot, when both checkpoints exist (and its save was written); else refused,
		 * nothing changed (a revert the game can't do restarts the level) */
		void host_req(s32 r); const char *host_toast(void); void host_clear_toast(void);
		u32 o0 = M32h(0x1F00), o4 = M32h(0x1F04);
		MEM[0x1CFF] = 0;
		W32(0x1F00, 0x01010001u); W32(0x1F04, 0x0101u);                       /* slot 1, a checkpoint and the one before, both written */
		host_req(13); spooky_main_frame();
		assert(M32h(0x1F00) == 0x01010000u && MEM[0x1CFF] == 1 && strstr(host_toast(), "Double revert"));
		MEM[0x1CFF] = 0; host_clear_toast();
		W32(0x1F00, 0x00010001u);                                              /* only one checkpoint so far */
		host_req(13); spooky_main_frame();
		assert(M32h(0x1F00) == 0x00010001u && MEM[0x1CFF] == 0 && strstr(host_toast(), "no earlier checkpoint"));
		W32(0x1F00, 0x01010000u); W32(0x1F04, 0x0001u);                       /* slot 0; slot 1's save not written */
		host_req(13); spooky_main_frame();
		assert(M32h(0x1F00) == 0x01010000u && MEM[0x1CFF] == 0);
		W32(0x1F00, o0); W32(0x1F04, o4);
		printf("Force Double Revert: the other slot when both checkpoints exist; refused (nothing changed) when there's no earlier one\n");
	}
	{
		/* 6i.122: Crouch Turbo - while the crouch button is held it's shown released in the clock's other half (bit 22);
		 * other buttons and an unheld crouch untouched; an analog crouch button alike. And the free camera's pause test:
		 * the game's time flag, or a window of the game's UI in use */
		u16 host_turbo(u16 buttons, u8 *analog, s32 phys, u32 clock); s32 host_game_paused(void); void host_set_ui_window(s32 on);
		u8 an[8] = { 0 }, o0 = MEM[TIME], o1 = MEM[TIME + 1];
		assert(host_turbo(0x40, an, 14, 100u) == 0x40 && host_turbo(0x40, an, 14, 101u) == 0 && host_turbo(0x40, an, 14, 102u) == 0x40);   /* 6i.123: by tick */
		assert(host_turbo(0x41, an, 14, 103u) == 0x01 && host_turbo(0x01, an, 14, 101u) == 0x01);
		an[1] = 255; host_turbo(0, an, 1, 7u); assert(an[1] == 0);
		an[1] = 255; host_turbo(0, an, 1, 8u); assert(an[1] == 255);
		MEM[TIME] = 1; MEM[TIME + 1] = 0; host_set_ui_window(0); assert(!host_game_paused());
		MEM[TIME + 1] = 1; assert(host_game_paused());
		MEM[TIME + 1] = 0; host_set_ui_window(1); assert(host_game_paused());
		host_set_ui_window(0); MEM[TIME] = o0; MEM[TIME + 1] = o1;
		printf("Crouch Turbo: held crouch pressed on one tick, released the next, over and over; other buttons kept; pause: time flag or a UI window\n");
	}
	{
		/* 6i.122: third person in the air - the camera's height is exactly your eye's (drawn with your body), not predicted
		 * from your speed and eased (that ran a tick ahead of the body, which shook against the camera on a jump) */
		u32 cam = 0x48000, moved, p14 = M32h(OBJECT + 0x14);
		memset(MEM + cam, 0, 0x358);
		state(2200, 5.f, 1, 0);                                                  /* airborne, rising at 5 u/s */
		W32(OBJECT + 0x14, 0xFFFFFFFFu);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		WF(cam, 0.f); WF(cam + 4, 0.f); WF(cam + 8, 0.6f); WF(cam + 0x20, 1.f); WF(cam + 0x34, 1.f);
		MEM[OBJECT + 0x240] = 0xFF; MEM[OBJECT + 0x241] = 0xFF;
		host_set_third_person(1, 10, 2, 0);
		moved = host_tp_camera(cam, 0); assert(fabsf(M32f(moved + 8) - 0.8f) < 1e-4f);
		WF(cam + 8, 0.9f); moved = host_tp_camera(cam, 0); assert(fabsf(M32f(moved + 8) - 1.1f) < 1e-4f);   /* the eye + 0.2, exactly */
		WF(cam + 8, 0.95f); moved = host_tp_camera(cam, 0); assert(fabsf(M32f(moved + 8) - 1.15f) < 1e-4f);
		host_set_third_person(0, 10, 2, 0); W32(OBJECT + 0x14, p14); state(2201, 0.f, 0, 0);
		printf("third person in the air: the camera height exactly the eye's (no prediction, no lag behind the body)\n");
	}
	{
		/* 6i.123: the free camera's zoom eased - in, between levels and out, never at once, and settling exactly */
		float host_fc_zs(void); void host_fc_zoom_step(s32 zoom); void host_set_unlock_fps(s32 on);
		float prev = host_fc_zs(), z;
		int k;
		host_set_unlock_fps(0);                                                        /* a 30 Hz frame each step: 20 % of the way */
		assert(prev == 1.f);
		host_fc_zoom_step(4); z = host_fc_zs(); assert(z < 1.f && z > 0.5f);          /* on its way, not there at once */
		for (k = 0; k < 120; k++) { host_fc_zoom_step(4); assert(host_fc_zs() <= z + 1e-6f); z = host_fc_zs(); }
		assert(z == 0.25f);                                                            /* 4x: settled exactly */
		host_fc_zoom_step(0); assert(host_fc_zs() > 0.25f && host_fc_zs() < 1.f);
		for (k = 0; k < 120; k++) host_fc_zoom_step(0);
		assert(host_fc_zs() == 1.f);
		printf("free camera zoom: eased in, between levels and out (~0.15 s), settling exactly\n");
	}
	{
		/* 6i.124: Trigger Volume Overlay - a script's test of a volume against you is a check (its tick, inside or not); a
		 * test of anything else isn't; the gap between your last two checks makes the bar; a check flashes for 15 ticks;
		 * red while the last check found you inside; off: nothing noted */
		void host_tv_level(u32 scn); void tvt_note(u32 index, u32 point, u32 result); s32 host_tv_state(s32 i, u32 cur, float *fl);
		float host_tv_bar(s32 i, u32 cur); void host_set_trig(s32 on, s32 flash, s32 hl);
		float fl;
		u32 otick = M32h(TIME + 8);
		host_set_trig(1, 1, 1); host_tv_level(0x4E000);
		state(1000, 0.f, 0, 0); tvt_note(3, OBJECT + 0x30, 0);
		assert(host_tv_state(3, 1001, &fl) == 1 && fl == 1.f && host_tv_bar(3, 1005) < 0.f);         /* one check: a flash, no bar yet */
		assert(host_tv_state(3, 1009, &fl) == 1 && fl > 0.46f && fl < 0.47f && host_tv_state(3, 1016, &fl) == 0);
		state(1030, 0.f, 0, 0); tvt_note(3, OBJECT + 0x30, 0);
		assert(fabsf(host_tv_bar(3, 1046) - 0.5f) < 1e-4f && host_tv_bar(3, 1061) == 0.f);         /* 30 ticks apart: half way at 15 */
		tvt_note(3, OBJECT + 0x34, 1); assert(host_tv_state(3, 1031, &fl) == 1);                  /* another point: not you */
		tvt_note(3, OBJECT + 0x30, 1); assert(host_tv_state(3, 1031, &fl) == 2);                  /* the same tick, inside: activated */
		state(1060, 0.f, 0, 0); tvt_note(3, OBJECT + 0x30, 1); assert(host_tv_state(3, 1090, &fl) == 2);   /* kept active: kept red */
		state(1090, 0.f, 0, 0); tvt_note(3, OBJECT + 0x30, 0); assert(host_tv_state(3, 1091, &fl) == 1);   /* missed: a flash */
		host_set_trig(1, 1, 0); state(1120, 0.f, 0, 0); tvt_note(3, OBJECT + 0x30, 1); assert(host_tv_state(3, 1121, &fl) == 1);   /* no highlight */
		host_set_trig(0, 1, 1); state(1150, 0.f, 0, 0); tvt_note(5, OBJECT + 0x30, 1); assert(host_tv_state(5, 1151, &fl) == 0);   /* off */
		W32(TIME + 8, otick);
		printf("Trigger Volume Overlay: your checks noted (tick, inside), the gap makes the bar, flashes 15 ticks, red while active\n");
	}
	{
		/* 6i.124: the overlay drawn - two volumes 10 units ahead of the view: boxes drawn, and (no names on the host) their
		 * numbers as labels; off: none */
		void host_tv_level(u32 scn); void host_set_trig(s32 on, s32 flash, s32 hl); void host_cam_pf(float *p, float *f);
		const u32 SC = 0x4E000, AR = 0x4E200;
		float p[3], f[3];
		int k, q, seen = 0;
		frame(); host_cam_pf(p, f);
		memset(MEM + SC, 0, 0x110); memset(MEM + AR, 0, 0x88);
		W32(SC + 0x108, 2); W32(SC + 0x10C, AR);
		for (k = 0; k < 2; k++)
		{
			u32 v = AR + (u32)k * 0x44;
			W32(v, 0); MEM[v + 4] = 0xFF; MEM[v + 5] = 0xFF;
			WF(v + 0xC, 1.f); WF(v + 0x10, 0.f); WF(v + 0x14, 0.f); WF(v + 0x18, 0.f); WF(v + 0x1C, 0.f); WF(v + 0x20, 1.f);
			for (q = 0; q < 3; q++) WF(v + 0x24 + 4 * (u32)q, p[q] + f[q] * 10.f - 1.f + (q == 1 ? (float)k * 4.f - 2.f : 0.f));
			WF(v + 0x30, 2.f); WF(v + 0x34, 2.f); WF(v + 0x38, 2.f);
		}
		host_set_trig(1, 1, 1); host_tv_level(SC); frame();
		for (k = 0; k < g_nlines && k < 16; k++) if (strstr(g_lines[k], "trigger 0") || strstr(g_lines[k], "trigger 1")) seen++;
		assert(seen >= 1);
		host_set_trig(0, 1, 1); frame();
		for (k = 0, seen = 0; k < g_nlines && k < 16; k++) if (strstr(g_lines[k], "trigger ")) seen++;
		assert(!seen);
		printf("Trigger Volume Overlay drawn: two volumes ahead labelled (by number on the host); off: none\n");
	}
	{
		/* 6i.126: the projection at a zoomed view's narrow field of view (0.06 rad - past the free camera's 8x) is the view's
		 * own (6i.125 fell back to another one below 0.1: the overlays drew with the wrong perspective); and the loaded BSP's
		 * point test - one plane, x = 0: open in front (x > 0), solid behind; no BSP: can't tell */
		s32 host_camera_read_fov(float fov_rad, float *tan_v); s32 host_tv_point_solid(float x, float y, float z);
		const u32 BS = 0x5D000, ND = 0x5D100, PL = 0x5D200;
		float tv = 0.f;
		u32 obsp = M32h(0x1B00);
		frame();
		assert(host_camera_read_fov(0.06f, &tv) == 1 && fabsf(tv - tanf(0.06f * 0.785f * 0.5f)) < 2e-4f);
		memset(MEM + BS, 0, 0x300);
		W32(BS, 1); W32(BS + 4, ND); W32(BS + 8, 1); W32(BS + 0xC, PL);
		MEM[ND] = 0; MEM[ND + 1] = 0; MEM[ND + 2] = 0xFF; MEM[ND + 3] = 0xFF; MEM[ND + 4] = 0xFF; MEM[ND + 5] = 0x00; MEM[ND + 6] = 0x00; MEM[ND + 7] = 0x80;
		WF(PL, 1.f); WF(PL + 4, 0.f); WF(PL + 8, 0.f); WF(PL + 12, 0.f);
		W32(0x1B00, BS);
		assert(host_tv_point_solid(-1.f, 0.f, 0.f) == 1 && host_tv_point_solid(1.f, 0.f, 0.f) == 0);
		W32(0x1B00, 0); assert(host_tv_point_solid(1.f, 0.f, 0.f) == -1);
		W32(0x1B00, obsp);
		printf("zoomed view: the overlays' projection kept down to 0.06 rad; the loaded BSP's point test (solid / open / none)\n");
	}
	{
		/* 6i.128: Harc's FP Scale Fix - the first-person body widened about the line of sight by s (across x s, along it
		 * unchanged), axes too; and Harc's animation fix - the rocket launcher's animation found by its header in a finished
		 * load of the animation cache and its 51 pieces rewritten; one whose pieces read as neither original nor fixed is
		 * left alone; a load searched once */
		void host_fp_scale_nodes(u32 m, u32 n, float s, const float *c, const float *f);
		void host_af_step(void); u32 host_af_patched(void); u32 host_af_odd(void); const u8 *host_af_sig(void);
		void host_af_piece(s32 k, u32 *rel, const u8 **fix, const u8 **orig);
		const u32 ND = 0x5E000, POOL = 0x5F000, LD = 0x5F100, RES = 0x5F200, PHY = 0x5F300, BP = 0x5F400, BL = 0x5F500, MEMB = 0x60000;
		float c[3] = { 1.f, 2.f, 3.f }, f[3] = { 1.f, 0.f, 0.f };
		u32 rel; const u8 *fx, *og; s32 k, j;
		memset(MEM + ND, 0, 0x68);
		WF(ND + 0x28, 6.f); WF(ND + 0x2C, 4.f); WF(ND + 0x30, 3.f);           /* 5 ahead of the camera, 2 to the side */
		WF(ND + 0x04, 0.f); WF(ND + 0x08, 1.f); WF(ND + 0x0C, 0.f);            /* forward along y (across the view) */
		WF(ND + 0x10, 1.f); WF(ND + 0x14, 0.f); WF(ND + 0x18, 0.f);            /* left along x (the line of sight) */
		host_fp_scale_nodes(ND, 1, 1.5f, c, f);
		assert(fabsf(M32f(ND + 0x28) - 6.f) < 1e-5f && fabsf(M32f(ND + 0x2C) - 5.f) < 1e-5f && fabsf(M32f(ND + 0x30) - 3.f) < 1e-5f);
		assert(fabsf(M32f(ND + 0x08) - 1.5f) < 1e-5f && fabsf(M32f(ND + 0x10) - 1.f) < 1e-5f);
		/* the animation cache: one load (salt 1, done) of a resource 0x2000 long in block 0 (offset 0, page shift 12) */
		memset(MEM + POOL, 0, 0x600); memset(MEM + MEMB, 0, 0x4000);
		W32(POOL + 0x20, 2); W32(POOL + 0x24, 0x10); W32(POOL + 0x44, LD);
		MEM[LD] = 1; MEM[LD + 4] = 1; W32(LD + 0xC, RES);
		W32(RES + 4, 0x2000); W32(RES + 0xC, 0);
		W32(PHY + 0x34, 12); W32(PHY + 0x64, BP); W32(BP + 0x44, BL); W32(BL + 8, 0);
		W32(0x7F320, POOL); W32(0x7F324, MEMB); W32(0x7F328, PHY);
		memcpy(MEM + MEMB + 0x400, host_af_sig(), 40);
		for (k = 0; k < 51; k++) { host_af_piece(k, &rel, &fx, &og); memcpy(MEM + MEMB + 0x400 + rel, og, 8); }
		host_af_step();
		assert(host_af_patched() == 1);
		for (k = 0; k < 51; k++) { host_af_piece(k, &rel, &fx, &og); for (j = 0; j < 8; j++) assert(MEM[MEMB + 0x400 + rel + (u32)j] == fx[j]); }
		host_af_step(); assert(host_af_patched() == 1);                       /* searched once */
		MEM[LD] = 2;                                                           /* reloaded (a new salt), its data back to the original, one piece changed */
		for (k = 0; k < 51; k++) { host_af_piece(k, &rel, &fx, &og); memcpy(MEM + MEMB + 0x400 + rel, og, 8); }
		MEM[MEMB + 0x400 + 0x180] ^= 0x55;
		host_af_step();
		assert(host_af_patched() == 1 && host_af_odd() == 1);
		{ host_af_piece(1, &rel, &fx, &og); for (j = 0; j < 8; j++) assert(MEM[MEMB + 0x400 + rel + (u32)j] == og[j]); }   /* left alone */
		W32(0x7F320, 0); W32(0x7F324, 0); W32(0x7F328, 0);
		printf("FP Scale Fix: the body widened across the view only; animation fix: the rocket launcher's animation patched on load, once; an odd one left alone\n");
	}
	{
		/* 6i.129: a node that turned 180 deg in a tick (the rocket launcher's barrels at the end of their animation) keeps the
		 * tick's matrix - a straight blend halfway collapsed it to nothing; one that turned 60 deg is blended, and in the
		 * first-person weapon its axes keep their length (a straight blend: 0.87) */
		void host_ip_blend_nodes(u32 cur, const u32 *pv, const u32 *bk, u32 count, float a, s32 fp); u32 host_ip_snaps(void);
		const u32 NB = 0x5E200;
		float p0[13] = { 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0.5f, 0.2f, 0.1f }, p1[13] = { 1, -1, 0, 0, 0, -1, 0, 0, 0, 1, 0.5f, 0.2f, 0.1f };
		float c6 = 0.5f, s6 = 0.8660254f, q1[13] = { 1, 0.5f, 0.8660254f, 0, -0.8660254f, 0.5f, 0, 0, 0, 1, 0.6f, 0.2f, 0.1f };
		u32 pv[13], bk[13], k, sn = host_ip_snaps();
		float fl, ll;
		(void)c6; (void)s6;
		memcpy(pv, p0, 52); memcpy(bk, p1, 52);
		for (k = 0; k < 13; k++) W32(NB + 4 * k, bk[k]);
		host_ip_blend_nodes(NB, pv, bk, 1, 0.5f, 1);
		assert(host_ip_snaps() == sn + 1 && fabsf(M32f(NB + 4) + 1.f) < 1e-6f && fabsf(M32f(NB + 0x14) + 1.f) < 1e-6f);   /* this tick's, whole */
		memcpy(bk, q1, 52);
		for (k = 0; k < 13; k++) W32(NB + 4 * k, bk[k]);
		host_ip_blend_nodes(NB, pv, bk, 1, 0.5f, 1);
		fl = sqrtf(M32f(NB + 4) * M32f(NB + 4) + M32f(NB + 8) * M32f(NB + 8)); ll = sqrtf(M32f(NB + 0x10) * M32f(NB + 0x10) + M32f(NB + 0x14) * M32f(NB + 0x14));
		assert(host_ip_snaps() == sn + 1 && fabsf(fl - 1.f) < 1e-5f && fabsf(ll - 1.f) < 1e-5f && fabsf(M32f(NB + 0x28) - 0.55f) < 1e-6f);
		for (k = 0; k < 13; k++) W32(NB + 4 * k, bk[k]);
		host_ip_blend_nodes(NB, pv, bk, 1, 0.5f, 0);                         /* a world object's: blended straight, as before */
		fl = sqrtf(M32f(NB + 4) * M32f(NB + 4) + M32f(NB + 8) * M32f(NB + 8));
		assert(fabsf(fl - 0.8660254f) < 1e-5f);
		printf("blend: a node turned 180 deg in a tick keeps its tick (not collapsed); 60 deg blended, first-person axes kept at length 1\n");
	}
	{
		/* 6i.130 / 6i.131: Adaptive Resolution Scaling. Off: the setting. Late (3+ of 30 over 95 % of 16.7 ms): down 10 %;
		 * frames that don't get cheaper for it (CPU-bound): the drop undone, no drop for 10 s; a GPU-bound load: down until it
		 * fits, rises tried and backed off where the top step doesn't fit; the load gone: back to 100 % in about 3 s */
		s32 host_rs_adapt_feed(u32 busy_us, s32 frames); void host_set_rs_adapt(s32 on); void host_set_unlock_fps(s32 on); void host_set_rscale(s32 v);
		void host_set_vsync(s32 on, s32 cap); s32 host_rs_adapt_model(u32 cpu_us, u32 gpu_us, s32 frames, s32 *changes);
		s32 ch = 0, r;
		host_set_unlock_fps(1); host_set_vsync(1, 0); host_set_rscale(10);
		assert(host_rs_adapt_feed(18000, 30) == 10);                          /* off: 100 % */
		host_set_rs_adapt(1);
		r = host_rs_adapt_feed(18000, 60);                                    /* CPU-bound: a drop, no gain, undone */
		assert(r == 10 && host_rs_adapt_feed(18000, 540) == 10);              /* ... and none for 10 s */
		host_rs_adapt_feed(8000, 700);                                        /* (calm) */
		r = host_rs_adapt_model(6000, 14000, 1800, &ch);                      /* GPU-bound: 100 % = 20 ms, 80 % = 15 ms fits */
		assert(r >= 7 && r <= 9 && ch < 40);                                  /* settled low, few tries at the top in 30 s */
		ch = 0; r = host_rs_adapt_model(6000, 2000, 300, &ch);                /* the load gone: 100 % within 5 s */
		assert(r == 10);
		ch = 0; host_set_rscale(7); r = host_rs_adapt_model(3000, 2000, 300, &ch);
		assert(r == 7);                                                        /* the setting is the top */
		{
			/* 6i.133: a change glides 2 % a frame (60 -> 70 %: five frames), and under Adaptive the projection's half sizes
			 * stay full size through the cut (the detail levels don't move with the scale); the stretch leaves them be */
			s32 host_rs_eff(void); void rs_cut(void); void spooky_view(u32 view);
			host_set_rscale(10); host_rs_adapt_feed(18000, 30);              /* settle the window (a drop may come) */
			host_rs_adapt_feed(6000, 600);                                     /* light: up to 100 % */
			assert(host_rs_eff() == 100);
			host_rs_adapt_feed(18000, 30);                                     /* late: the target 90 % */
			{ s32 e = host_rs_eff(); assert(e >= 90 && e < 100); }
		}
		host_set_rs_adapt(0); host_set_rscale(10); assert(host_rs_adapt_feed(5000, 1) == 10);
		host_set_unlock_fps(0);
		printf("Adaptive Resolution Scaling: CPU-bound drops undone; GPU-bound settles with backed-off tries; back to 100%% when the load goes\n");
	}
	{
		/* 6i.133: 128 MB, Halo 2 HD's way: with the host bridge saying 128 and the XBE's limit clear, the kernel's
		 * contiguous cap is raised once (logged); with 64 MB fitted, or the flag set, untouched */
		extern u32 g_host_memtop, g_host_mem_big, g_host_mem_avail, g_host_mem_pages;
		void host_rs_probe_step(void); void host_set_rscale(s32 v); const char *host_toast(void);
		u32 fl = M32h(0x10124);
		host_set_rscale(10); host_rs_probe_step();
		g_host_memtop = 128u; W32(0x10124, fl & ~4u); g_host_mem_big = 0x4000000u; g_host_mem_avail = 16000u; g_host_mem_pages = 32768u;
		host_set_rscale(15); host_rs_probe_step();
		assert(strstr(host_toast(), "960x720 - drawn") != 0);                                  /* 6i.152: the short toast */
		host_set_rscale(10); host_rs_probe_step();
		g_host_memtop = 64u; g_host_mem_big = 0x9C000u; g_host_mem_avail = 160u; g_host_mem_pages = 16384u; W32(0x10124, fl);
		{
			/* 6i.200: the start-up search for the largest free block never asks for more than what is free less 16 MB (after a
			 * restart the whole upper half is one free run: it took 60 of 64 MB and the console stopped) - and reports what
			 * it found up to there; with 32 MB or less free (the original Xbox) it searches as it always did */
			extern u32 g_host_alloc_max; void host_mem_probe(void); u32 host_mem_big_kb(void);
			g_host_mem_pages = 32768u; g_host_mem_avail = 16384u; g_host_mem_big = 0x4000000u; g_host_alloc_max = 0;   /* 64 MB free, all of it one run */
			host_mem_probe();
			assert(host_mem_big_kb() == 49152u && g_host_alloc_max == 48u * 1024u * 1024u);            /* 48 MB asked for at most: 16 left alone */
			g_host_mem_big = 0x2000000u; g_host_alloc_max = 0; host_mem_probe();
			assert(host_mem_big_kb() == 32768u && g_host_alloc_max <= 48u * 1024u * 1024u);             /* a 32 MB run: found as before */
			g_host_mem_avail = 160u; g_host_mem_big = 0x90000u; g_host_alloc_max = 0; host_mem_probe();
			assert(host_mem_big_kb() == 576u);                                                          /* the original Xbox (640 KB free): searched as it always was */
			g_host_mem_big = 0x9C000u; g_host_mem_avail = 160u; g_host_mem_pages = 16384u; g_host_alloc_max = 0; host_mem_probe();
		}
		printf("128 MB (Halo 2 HD's way): the host bridge's 128 and a clear flag raise the kernel's cap; Render Scale 150%% fits then\n");
	}
	{
		/* 6i.147: the water mesh fitted to the view. The game sizes it for a 4:3 reach (tan(fov/2) x 640/480, x 1.13);
		 * in widescreen the view reaches 4/3 further - the sideways column grown by that; not widescreen: left alone */
		void host_set_widescreen(u32 ws, u32 flag5); void water_fit(u32 state);
		const u32 WM = 0x7F340, ST = 0x5F900, FSS = 0x1200;
		float fov = 1.0f, dist = 10.f, mesh_tan, game_w, col1;
		s32 i;
		u32 s40 = M32h(FSS + 0x40), s48 = M32h(FSS + 0x48), s4c = M32h(FSS + 0x4C);
		memset(MEM + ST, 0, 0x100); WF(ST + 0x7C, fov); WF(ST + 0x80, 1.f);
		WF(FSS + 0x40, fov); W32(FSS + 0x48, 0); W32(FSS + 0x4C, 480u | (640u << 16));
		mesh_tan = tanf(fov * 0.5f); game_w = (tanf(fov * 0.5f) * (640.f / 480.f) / mesh_tan) * 1.13f * dist;   /* as the game sizes it, 4:3 */
		for (i = 0; i < 3; i++) { WF(WM + 16 * i, i == 0 ? dist : 0.f); WF(WM + 16 * i + 4, i == 1 ? game_w : 0.f); WF(WM + 16 * i + 8, i == 2 ? 1.f : 0.f); }
		host_set_widescreen(0, 0); water_fit(ST);
		col1 = M32f(WM + 16 + 4); assert(fabsf(col1 - game_w) < 1e-4f);                              /* 4:3: as the game had it */
		host_set_widescreen(1, 0); water_fit(ST);
		col1 = M32f(WM + 16 + 4); assert(fabsf(col1 - game_w * 4.f / 3.f) < 1e-3f && fabsf(M32f(WM) - dist) < 1e-6f);   /* widescreen: 4/3 wider, the rest untouched */
		water_fit(ST); assert(fabsf(M32f(WM + 16 + 4) - game_w * 4.f / 3.f) < 1e-3f);                  /* again: no further growth */
		host_set_widescreen(0, 0); W32(FSS + 0x40, s40); W32(FSS + 0x48, s48); W32(FSS + 0x4C, s4c);
		printf("water mesh: widened 4/3 for widescreen, left alone at 4:3\n");
	}
	printf("ALL SUPERBOUNCE CHECKS PASSED\n");
	return 0;
}
