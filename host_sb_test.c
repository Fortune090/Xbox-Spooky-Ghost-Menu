#include <math.h>
/* Host test for the superbounce HUD: spooky.c's real per-tick code against simulated game memory. */
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef signed short s16; typedef signed int s32;
typedef struct { u32 packet; u16 buttons; u8 analog[8]; s16 lx, ly, rx, ry; } xinput_state;
typedef struct { s16 top, left, bottom, right; } rect16;
typedef struct { float a, r, g, b; } color4f;

/* ---- simulated game memory: 1 MB at address 0 ---- */
static u8 MEM[0x100000];
void host_w32(u32 a, u32 v) { if (a + 4 <= sizeof(MEM)) memcpy(MEM + a, &v, 4); }
u32 host_mem32(u32 a) { u32 v = 0; if (a + 4 <= sizeof(MEM)) memcpy(&v, MEM + a, 4); return v; }
u8 host_mem8(u32 a) { return a < sizeof(MEM) ? MEM[a] : 0; }
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

int main(void)
{
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
		assert(drawn("Bounces in a row: 1") && drawn("Last Bounce: 18."));
		host_req(3); frame();                                                   /* launched again ... */
		lv[0] = M32f(OBJECT + 0x88); lv[1] = M32f(OBJECT + 0x8C);
		W32(TIME + 8, 117); WF(OBJECT + 0x90, 25.f); frame();                  /* ... and the very next tick a bounce */
		sp = sqrtf(lv[0] * lv[0] + lv[1] * lv[1] + 25.f * 25.f);
		snprintf(want, sizeof(want), "Last Bounce: %d.%du/s", (int)(sp * 10.f + 0.5f) / 10, (int)(sp * 10.f + 0.5f) % 10);
		assert(drawn("Bounces in a row: 2") && drawn(want));
		{ void host_clear_toast(void); host_clear_toast(); }                     /* (Launch's message) */
		printf("launches: a launch alone is no bounce; a bounce the very tick after one counts (%s)\n", want);
	}
	{
		/* 6h.19: a launch the game didn't apply (your speed unchanged) is no bounce either (6h.18 took one for a 2.2 u/s
		 * "bounce"); a bounce under 5 u/s never counts; Last Bounce keeps the peak of the 5 ticks after a bounce */
		void host_req(s32 r); const char *host_toast(void);
		int b0;
		char line[64];
		state(130, -10.f, 1, 1); frame();
		b0 = drawn("Bounces in a row: 2") ? 2 : -1; assert(b0 == 2);
		host_req(3); frame();                                                   /* launched ... */
		W32(TIME + 8, 131); WF(OBJECT + 0x88, 3.f); WF(OBJECT + 0x8C, 4.f); WF(OBJECT + 0x90, -10.14f); frame();   /* ... it didn't take */
		assert(drawn("Bounces in a row: 2"));
		W32(TIME + 8, 132); WF(OBJECT + 0x90, -10.f); frame();
		W32(TIME + 8, 133); WF(OBJECT + 0x88, 1.f); WF(OBJECT + 0x8C, 1.f); WF(OBJECT + 0x90, 4.5f); frame();   /* 4.7 u/s: too slow */
		assert(drawn("Bounces in a row: 2"));
		state(134, -10.f, 1, 1); frame();
		state(135, 10.f, 1, 1); frame();                                       /* a bounce: 11.2 u/s ... */
		assert(drawn("Bounces in a row: 3") && drawn("Last Bounce: 11.2u/s"));
		state(136, 30.f, 1, 1); frame();                                       /* ... rising to 30.4 the next tick */
		assert(drawn("Bounces in a row: 3") && drawn("Last Bounce: 30.4u/s"));
		state(137, 29.f, 1, 1); frame(); assert(drawn("Last Bounce: 30.4u/s")); /* the peak kept */
		state(138, 28.f, 1, 1); frame(); state(139, 27.f, 1, 1); frame(); state(140, 26.f, 1, 1); frame();
		assert(logged("superbounce: bounce 3 peak speed x10 304") == 1);       /* 6h.20: the peak in the log, after its 5 ticks */
		{ void host_clear_toast(void); host_clear_toast(); }
		(void)line; (void)host_toast;
		printf("bounces: a launch that didn't take - none; under 5 u/s - none; Last Bounce - the peak over 5 ticks (30.4)\n");
	}
	/* a revert (the tick goes back) ends the chain */
	state(90, -3.f, 1, 1); frame();
	assert(drawn("Bounces in a row: 0"));
	printf("a pause changes nothing; a revert ends the chain\n");

	/* another level: its own record; back again: this one's */
	WS(0x1100, "scenarios\\solo\\zz_other\\zz_other"); state(91, -3.f, 1, 1); frame();
	assert(drawn("Level record: 7") && drawn("Last Bounce: --"));            /* 6h.16: a new level: none yet */
	WS(0x1100, "scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa"); state(92, -3.f, 1, 1); frame();
	assert(drawn("Level record: 3"));                                        /* (3 in a row in the 6h.19 test) */
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
			assert(!drawn("Back + D-pad Up") && !drawn("OUT OF BOUNDS"));                   /* 6f.3: no hint (6f.2 showed it) */
			{
				/* 6g: the game's main menu - its options' state 3 (6f.3 took "no player record": the main menu has one) */
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
		s32 host_sleep(s32 *ts, s32 *ta, s32 *tb, s32 *r); s32 host_sleep_near(s32 i, s32 *ts, float *dist);
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
		assert(g_q == 7 && host_sleep_near(0, &ts, &dist) && ts == 17 && dist > 2.99f && dist < 3.01f && drawn("island 1 body, 3.0 wu: 17 ticks (4/5)"));
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
		assert(ts == -1 && host_sleep_awake() == 1 && drawn("SLEEP: kept awake - the game keeps it awake"));
		{ int q, full = 0; for (q = 0; q < g_q && q < 64; q++) if (g_qxy[q][0] == 164 && g_qxy[q][2] == 476) full = 1; assert(full); }   /* not counting down: full */
		host_set_veto(0); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == 26 && host_sleep_awake() == 0);   /*    not vetoed: Havok's tests */
		W32(DEA1, VT_FAKE); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == -1 && host_sleep_awake() == 3);  /* 8. the fake one */
		W32(ENTITY + 0x54, 0); frame(); host_sleep(&ts, &ta, &tb, &r); assert(ts == -1 && host_sleep_awake() == 2);  /*    none */
		W32(ENTITY + 0x54, DEA1); W32(DEA1, VT_SPATIAL);
		W32(WORLD + 0x23C, 0); frame(); st = host_sleep(&ts, &ta, &tb, &r); assert(st == 2 && drawn("SLEEP: off"));
		host_set_sleep(0); frame(); assert(!drawn("SLEEP"));
		W32(WORLD + 0x08, 0); W32(WORLD + 0x0C, 0); W32(ISLAND + 0x2C, 0); W32(ENTITY + 0x3C, 0); W32(ENTITY + 0x54, 0);
		printf("sleep: all still -> 26 (the long check); long off -> 49; short off -> 26; both -> 50 (the second long check); turned -> 26; at the limit not still; vetoed / fake / none -> kept awake; near island 4/5 -> 17\n");
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
		assert(!drawn("no seam here") && host_landing_q() == 0);              /* landing text off by default: verdict only on the cursor */
		{
			/* the seam's colour is the reticle's verdict for landing ON it: a flat seam along y, falling at 6 u/s, contact 6.2
			 * u/s (over the 5.45 needed), depth 0.4 - so the fall alone says likely; the drift across it decides the rest */
			s32 host_rate_as_verdict(const float *pa, const float *pb, s32 kind, float down, float walk, float dw, const float *pv, s32 charged, const char **why);
			float sa[3] = { 0, -2, 0 }, sb[3] = { 0, 2, 0 };
			float across_fast[3] = { 2.f, 0.f, 0.f }, along[3] = { 0.f, 3.f, 0.f }, few[3] = { 0.42f, 3.f, 0.f }, edge8[3] = { 0.95f, 3.f, 0.f };
			const char *why = 0;
			s32 q;
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, across_fast, 1, &why);    /* ~18 deg across: too fast */
			assert(q == 1 && why && !strcmp(why, "ANGLE: drifting too fast across"));
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, along, 1, &why);          /* along it, flat: too little */
			assert(q == 1 && why && !strcmp(why, "ANGLE: too little drift across"));
			q = host_rate_as_verdict(sa, sb, 1, 6.f, 6.2f, 0.4f, along, 1, &why);          /* along it, a valley: its bend may push - not downgraded */
			assert(q == 2 && !why);
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, few, 1, &why);            /* ~4 deg across: likely */
			assert(q == 2 && !why);
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, edge8, 1, &why);          /* ~9 deg: an edge */
			assert(q == 3 && why && !strcmp(why, "EDGE (about 1 in 2)"));
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.15f, few, 1, &why);           /* shallow (0.15 < 0.21): an edge */
			assert(q == 3);
			q = host_rate_as_verdict(sa, sb, 0, 6.f, 6.2f, 0.4f, few, 0, &why);            /* not charged: red */
			assert(q == 0 && why && !strcmp(why, "NOT CHARGED"));
			printf("rating as the verdict: too fast / too little (flat) yellow, a valley along it kept, ~4 deg green, ~9 deg or shallow amber, uncharged red\n");
		}
		{
			/* the landing drawn with its seams, as last finished: on the slow schedule, after the landing step the drawn
			 * landing is still the old one; after the cycle, the new (5e.17 drew the live landing against the old seams) */
			void host_set_step_div(s32 d); u32 host_count_hud(void); s32 host_drawn_landing(float *dp);
			float old_p[3], d_p[3], live[3];
			s32 ff;
			assert(host_drawn_landing(old_p) && old_p[0] > 2.6f && old_p[0] < 2.95f);
			host_set_step_div(6);
			for (ff = 0; ff < 12 && (host_count_hud() % 6) != 5; ff++) { state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 4.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame(); }
			state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 3.5f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f);
			frame();                                                               /* step 0: the landing */
			assert(host_landing(live) && live[0] < old_p[0] - 0.05f);              /* the live landing moved */
			assert(host_drawn_landing(d_p) && d_p[0] == old_p[0]);                 /* the drawn one: still the old */
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
				assert(host_landing(live) && live[0] < before[0] - 0.05f && host_drawn_landing(d_p) && d_p[0] == before[0]);
				for (ff = 0; ff < 2; ff++) { state(401, 0.f, 1, 0); WF(OBJECT + 0x88, 3.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame(); }
				assert(host_drawn_landing(d_p) && host_landing(live) && d_p[0] == live[0]);   /* rating + view ran: the new */
				printf("every 3rd frame: the landing, the seams, then rating and view together - the drawn landing follows\n");
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
		assert(drawn("no seam here"));                                        /* Cartographer's verdict text: this floor has no seam - red */
		printf("landing: found on the floor quad at (%.2f, %.2f, %.2f), HUD says so\n", p[0], p[1], p[2]);
		state(402, 0.f, 1, 0); WF(OBJECT + 0x88, 1.f); frame();
		state(403, 0.f, 1, 0); WF(OBJECT + 0x88, 1.f); frame();
		state(404, 0.f, 1, 0); WF(OBJECT + 0x88, 1.f); g_q = 0; frame();
		assert(!host_landing(p));
		printf("landing: none when the arc passes beside the floor\n");
	}
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
		assert(host_landing_q() == 1 && drawn("LOTTERY"));
		printf("landing verdict: on the diagonal seam at (%.2f, %.2f, %.2f) - %d, LOTTERY, as Cartographer's rules give\n", p[0], p[1], p[2], host_landing_q());
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
	/* the seam labels ("Seam info text"): standing over the synthetic seams 1 below at rest - Cartographer's words */
	{
		W32(0x1B04, 0); W32(0x1B00, 0x80000);
		WF(OBJECT + 0x64, 0.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, 0.f);
		host_set_land_text(1);
		state(800, 0.f, 0, 1); WF(OBJECT + 0x88, 0.f); WF(OBJECT + 0x8C, 0.f); WF(OBJECT + 0x90, 0.f); frame();
		assert(drawn("drop 1.0: too slow (walk "));
		printf("seam labels: in Cartographer's words (drop 1.0: too slow ...)\n");
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
		assert(drawn[0] == 1 && drawn[1] == 0);
		W32(0x1B00, 0);
		printf("drawn only if reachable: the seam at your level drawn with a clear walk, hidden behind a wall\n");
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
			assert(fabsf(fx - ex) < 1e-4f && fabsf(fy) < 1e-6f && fabsf(fz - ez) < 1e-4f && fz < 0.f);
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
		assert(moved != cam && fabsf(M32f(moved) + 4.f) < 1e-4f && fabsf(fx - ex) < 1e-4f && fabsf(fz - ez) < 1e-4f);
		{
			/* eased in 1 / distance: from 500 out to the wall (3), a third of the way a frame - 1 / (1/500 + (1/3 - 1/500) x 0.35) */
			float aim, gx, gz, lz;
			host_set_third_person(0, 10, 2, 0); host_tp_camera(cam, 0);
			host_set_third_person(1, 10, 2, 0);
			WF(cam + 0x20, -1.f); host_tp_camera(cam, 0);                         /* looking away from the wall: 500 out */
			WF(cam + 0x20, 1.f); moved = host_tp_camera(cam, 0);                  /* turned to it: eased */
			aim = 1.f / (1.f / 500.f + (1.f / 3.f - 1.f / 500.f) * 0.35f); gx = aim + 1.f; gz = -0.2f; lz = sqrtf(gx * gx + gz * gz);
			assert(fabsf(M32f(moved + 0x20) - gx / lz) < 1e-3f && fabsf(M32f(moved + 0x28) - gz / lz) < 1e-3f);
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
			gx = -51.f; gy = -1.f; gz = -0.2f; lz = sqrtf(gx * gx + gy * gy + gz * gz);
			assert(fabsf(M32f(moved + 0x20) - gx / lz) < 1e-4f && fabsf(M32f(moved + 0x24) - gy / lz) < 1e-4f && fabsf(M32f(moved + 0x28) - gz / lz) < 1e-4f);
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
			/* no shake: a one-frame bump of the eye (0.1 up) moves the camera a quarter of it; a steady fall (vertical
			 * speed -6 u/s, the eye 0.2 lower a frame) is followed exactly */
			u32 c2 = 0x48000, m;
			float z0, z1;
			memset(MEM + c2, 0, 0x358);
			WF(c2, -3.f); WF(c2 + 4, 0.f); WF(c2 + 8, -3.5f); WF(c2 + 0x20, 1.f); WF(c2 + 0x34, 1.f);
			WF(OBJECT + 0x64, -3.f); WF(OBJECT + 0x68, 0.f); WF(OBJECT + 0x6C, -4.1f); WF(OBJECT + 0x90, 0.f);
			host_set_third_person(0, 10, 2, 0); host_tp_camera(c2, 0);
			host_set_third_person(1, 10, 2, 0);
			m = host_tp_camera(c2, 0); z0 = M32f(m + 8);
			WF(c2 + 8, -3.4f); m = host_tp_camera(c2, 0); z1 = M32f(m + 8);          /* the bump */
			assert(z1 - z0 > 0.02f && z1 - z0 < 0.03f);
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
		W32(W + 0x224 + 8, 30); W32(W + 0x234 + 8, 12); WF(W + 0x180, age);
		host_set_ammo(0, 1);
		bc_before(WD);
		W32(W + 0x224 + 8, 29); W32(W + 0x234 + 8, 11); WF(W + 0x180, 0.6f);    /* the shot */
		bc_after();
		assert((M32h(W + 0x224 + 8) & 0xffff) == 30 && (M32h(W + 0x234 + 8) & 0xffff) == 12 && M32f(W + 0x180) == age);
		W32(OBJECT + 0x13C, 0xFFFFFFFFu);                                      /* an AI's unit */
		bc_before(WD);
		W32(W + 0x224 + 8, 29); bc_after();
		assert((M32h(W + 0x224 + 8) & 0xffff) == 29);
		host_set_ammo(0, 0); W32(OBJ_DATA + 4 * 12, 0); W32(OBJ_DATA + 4 * 12 + 8, 0); W32(OBJECT + 0x13C, 0);
		printf("bottomless clip: a player's weapon's rounds and battery put back after its shot; an AI's weapon untouched\n");
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
		/* 6f: Bump Possession - a biped no player controls, 0.5 from you: on the 3rd tick of the touch, player_set_unit(you,
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
			/* 6f.1: Revert to player - the game has you in the vehicle now: back into the body you started in; in your own body,
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
			/* 6h.13: an element anchored bottom-left (kind 2): its right edge 2 pixels further out; its left edge, and any other
			 * kind's edges, as ever */
			void host_hsq_type(s32 t);
			host_hsq_type(2); hsq_anchor(a);
			assert(host_hsq_quad_x(Q, 0) == 85.f && host_hsq_quad_x(Q, 1) == 162.f && host_hsq_quad_x(Q, 2) == 162.f && host_hsq_quad_x(Q, 3) == 85.f);
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
		WF(OBJECT + 0x30, 200.f); W32(TIME + 8, 1000); ip_tick_before(); assert(host_ip_snapped() == 0); WF(OBJECT + 0x30, 1.f);   /* far: not taken */
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
			/* the legs: body facing +x, the view along +y - turned +90 degrees about the eye (10 - 0.45, 0), back after the build */
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
		}
		memcpy(MEM + OBJECT, save, sizeof(save)); memcpy(MEM + TIME, tsave, sizeof(tsave)); memcpy(MEM + 0x1800, csave, sizeof(csave));
		W32(0x1D10, oopt);
		frame();
		#undef NEAR
		#undef SETPOS
		#undef NPOS
		printf("smooth slow motion: node matrices drawn between the two ticks (0.25 -> 1.15, 0.75 -> 1.45) and put back exactly; none at 1.0, paused, after a 5-unit jump, two ticks on, for another object, still or far; the eye moved as its unit's root (first and third person), the first-person legs turned to the view (any pitch) and put back\n");
	}
	printf("ALL SUPERBOUNCE CHECKS PASSED\n");
	return 0;
}
