/* Host test for spooky.c: scripted controller input, a recording stand-in for the game's text renderer. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef signed short s16; typedef signed int s32;
typedef struct { u32 packet; u16 buttons; u8 analog[8]; s16 lx, ly, rx, ry; } xinput_state;
typedef struct { s16 top, left, bottom, right; } rect16;
typedef struct { float a, r, g, b; } color4f;

u8 host_draw_string_globals[0x64];
void host_w32(u32 a, u32 v) { (void)a; (void)v; }
static u32 g_state = 3;                                  /* the game options' state: 3 the game's main menu, 1 a campaign game */
static u32 g_vblank = 1000;                              /* 6i */
double g_yaw_in, g_yaw_out;                              /* 6i.54 */
u32 host_mem32(u32 a)                                    /* no game memory: no player, so no HUD - but the game options: */
{                                                        /* the main menu (3: the hint line), or in a game (1: the menu opens) */
	if (a == 0x1D10) return 0x20000;
	if (a == 0x20008) return g_state;
	if (a == 0x1DC0) return g_vblank;                    /* 6i: the vblank count (the refresh rate reads 0: 60 taken) */
	if (a == 0x1800 + 0x7C || a == 0x1800 + 0x80 || a == 0x1800 + 0x20 || a == 0x1800 + 0x24)   /* 6i.54: the cameras' forward (x, y) */
	{
		extern double g_yaw_in, g_yaw_out;
		double y = (a == 0x1800 + 0x7C || a == 0x1800 + 0x80) ? g_yaw_in : g_yaw_out;
		float f = (float)((a == 0x1800 + 0x7C || a == 0x1800 + 0x20) ? cos(y) : sin(y)); u32 v; memcpy(&v, &f, 4); return v;
	}
	return 0;
}
u8 host_mem8(u32 a) { (void)a; return 0; }
u32 host_read_file(const char *n, char *b, u32 s) { (void)n; (void)b; (void)s; return 0; }
void host_write_file(const char *n, const char *d, u32 s) { (void)n; (void)d; (void)s; }
void host_gfx_quad(u32 color, const s16 *xy) { (void)color; (void)xy; }
u32 host_font_handles[16];
static xinput_state g_pad;          /* what the controller reports */
static int g_draws, g_frame_lines;
static char g_last[16][100];
static color4f g_last_color[16];
static int g_last_shadow;
static FILE *g_log;
static char g_loglines[64][160]; static int g_nlog;
void host_log(const char *line) { if (g_nlog < 64) strncpy(g_loglines[g_nlog++], line, 159); }
static int logged(const char *needle) { int i, n = 0; for (i = 0; i < g_nlog; i++) if (strstr(g_loglines[i], needle)) n++; return n; }

u32 host_xinput_get_state(void *handle, xinput_state *st) { (void)handle; *st = g_pad; return 0; }
void host_text_style_clear(void) { memset(host_draw_string_globals, 0, sizeof(host_draw_string_globals)); }
static s16 g_font_header[8] = { 0, 0, 12, 4, 2, 0, 0, 0 };      /* +4/+6/+8: ascending 12, descending 4, leading 2 */
void *host_font_get(u32 handle) { (void)handle; return g_font_header; }
static int g_min_room = 1000;       /* smallest (rectangle height - full font height) seen */
static int g_last_top[16], g_last_bottom[16], g_last_left[16];
void host_draw_text(const u16 *text, const rect16 *b, float scale)
{
	const color4f *color = (const color4f *)(host_draw_string_globals + 0x10);
	int shadow = host_draw_string_globals[0x20];
	if (b->bottom - b->top - 18 < g_min_room) g_min_room = b->bottom - b->top - 18;
	char s[100]; int i;
	for (i = 0; text[i] && i < 99; i++) s[i] = (char)text[i];
	s[i] = 0;
	assert(*(s32 *)(host_draw_string_globals + 0) == 9);          /* the menu's font is set while drawing */
	assert(*(s32 *)(host_draw_string_globals + 0x0c) == 0 || !strncmp(s, "Total Time: ", 12));       /* left-justified (6i.169: but the Campaign Timer's line, bottom-right) */
	if (g_frame_lines < 16) { strcpy(g_last[g_frame_lines], s); g_last_color[g_frame_lines] = *color; g_last_top[g_frame_lines] = b->top; g_last_bottom[g_frame_lines] = b->bottom; g_last_left[g_frame_lines] = b->left; }
	g_frame_lines++; g_draws++;
	assert(shadow == 0 || shadow == 1);
	g_last_shadow = shadow;
	if (g_log) fprintf(g_log, "%d %d %d %d %.2f %.2f %.2f %.2f %d %s\n", b->top, b->left, b->bottom, b->right, scale, color->r, color->g, color->b, shadow != 0, s);
}

u32 spooky_input(void *handle, xinput_state *st);
void spooky_frame(void);
s32 host_menu_open(void), host_menu_selected(void), host_value(s32), host_suppressing(void);

static xinput_state g_game;          /* what the game received */
static void *P1 = (void *)0x1111;
static void frame(void) { spooky_input(P1, &g_game); memset(g_last, 0, sizeof(g_last)); g_frame_lines = 0; spooky_frame(); }
static void press(u16 buttons, u8 a, u8 b) { memset(&g_pad, 0, sizeof(g_pad)); g_pad.buttons = buttons; g_pad.analog[0] = a; g_pad.analog[1] = b; }
static void release(void) { memset(&g_pad, 0, sizeof(g_pad)); }
static int game_neutral(void) { return !g_game.buttons && !g_game.analog[0] && !g_game.analog[1]; }

void host_set_linear_velocity(u32 motion, const float *v) { (void)motion; (void)v; }
s32 host_menu(void);
void host_set_rows_cap(s32 n);
int main(int argc, char **argv)
{
	u8 marker[0x64]; int i;
	for (i = 0; i < 0x64; i++) marker[i] = (u8)(0xA0 ^ i);
	memcpy(host_draw_string_globals, marker, sizeof(marker));

	/* 1. closed: the hint line, and the game's text state is left exactly as it was */
	release(); frame();
	assert(g_frame_lines == 1 && strstr(g_last[0], "Back + D-pad Up"));
	assert(g_last_shadow == 1);                                       /* shadows on by default */
	assert(!memcmp(host_draw_string_globals, marker, sizeof(marker)));
	printf("closed: hint line drawn, text state restored\n");
	assert(logged("controller hook running") == 1 && logged("HUD-text hook running") == 1 && logged("line height 18"));

	/* 6i.198: on the game's main menu the hint line shows, and Back is treated as with the game's pause menu up: held
	 * back from the game while it is down (so the game's own "back" does not fire under a Back + Up), a tap of it alone
	 * given to the game on release; Back + Up opens the menu there (6h had it off there); the game gets a neutral pad
	 * while it is open; B closes it; an action that needs a level says so */
	frame();
	assert(!host_menu_open() && g_frame_lines == 1 && strstr(g_last[0], "Spooky Ghost Menu: Back + D-pad Up"));
	assert(g_last_top[0] == 448 && g_last_bottom[0] <= 480 && g_last_left[0] == 9);   /* 6h.6: the info line's row; 6h.8: 9 px in */
	press(0x20, 0, 0); frame(); assert(!host_menu_open() && !(g_game.buttons & 0x20));          /* Back held: the game does not see it */
	release(); frame(); assert(g_game.buttons & 0x20);                                           /* let go alone: the game gets a tap of it */
	frame(); frame(); frame(); assert(!(g_game.buttons & 0x20));
	press(0x20, 0, 0); frame(); press(0x20 | 0x01, 0, 0); frame();                               /* Back, then Up with it */
	assert(host_menu_open() && game_neutral());
	release(); frame(); frame(); frame(); assert(host_menu_open() && !(g_game.buttons & 0x20));  /* used in the combination: the game never sees Back */
	press(0x02, 0, 0); frame(); assert(game_neutral() && host_menu_selected() == 1);             /* the pad is the menu's */
	release(); frame();
	press(0x01, 0, 0); frame(); release(); frame(); assert(host_menu_selected() == 0);
	{
		/* 6i.202: on the game's main menu an action that needs a level says "Only in a level" and the menu stays open
		 * where it is - Checkpoint Settings' Create checkpoint, Launch / Teleport's Launch and Teleport, the Superbounce
		 * Viewer Settings' Passion Check, the Ghost Viewer Settings' Save / Revert offsets */
		void host_set_menu_open(s32 open, s32 m); const char *host_toast(void); void host_clear_toast(void); void spooky_main_frame(void);
		static const struct { int menu; const char *item; } act[] = { { 9, "Create Checkpoint" }, { 9, "Revert Checkpoint" }, { 9, "Force Double Revert" }, { 1, "Launch (Black + Up)" },
			{ 1, "Teleport (Black + Down)" }, { 2, "Passion Check" }, { 5, "Save offsets" }, { 5, "Revert offsets" } };
		int q, z, guard;
		for (q = 0; q < (int)(sizeof(act) / sizeof(act[0])); q++)
		{
			int found = 0;
			host_set_menu_open(1, act[q].menu); frame(); host_clear_toast();
			for (guard = 0; guard < 40 && !found; guard++)
			{
				for (z = 0; z < g_frame_lines; z++) if (g_last[z][0] == '>' && strstr(g_last[z], act[q].item)) found = 1;
				if (!found) { press(0x02, 0, 0); frame(); release(); frame(); }
			}
			if (!found) { printf("no item \"%s\" in menu %d\n", act[q].item, act[q].menu); assert(0); }
			press(0, 255, 0); frame(); release(); frame(); spooky_main_frame();
			assert(host_menu_open() && host_menu() == act[q].menu && !strcmp(host_toast(), "Only in a level"));
			while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }   /* (each menu left as it was found) */
		}
		host_clear_toast(); host_set_menu_open(1, 0); frame();
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
	}
	press(0, 0, 255); frame(); assert(!host_menu_open()); release(); frame(); frame();           /* B closes it */
	g_state = 1;                                                      /* in a game from here on */
	printf("main menu: the hint line; Back held back from the game, a tap of it passed on release; Back + Up opens the menu; B closes it\n");

	/* 2. Back alone does nothing; the game sees it */
	press(0x20, 0, 0); frame();
	assert(!host_menu_open() && g_game.buttons == 0x20);
	/* Back + Up opens; the game sees a neutral pad from that read on */
	press(0x20 | 0x01, 0, 0); frame();
	assert(host_menu_open() && game_neutral());
	assert(g_frame_lines == 13 && strstr(g_last[0], "Spooky Ghost Menu - by Fortune090") && strstr(g_last[1], "> Cheats  >") && strstr(g_last[2], "Checkpoint Settings  >") && strstr(g_last[3], "Launch / Teleport") && strstr(g_last[6], "Ghost Viewer: off") && strstr(g_last[7], "Ghost Viewer Settings") && strstr(g_last[8], "Other Tools  >") && strstr(g_last[9], "Camera Settings  >") && strstr(g_last[10], "Game Hacks  >") && strstr(g_last[11], "Text scale: 1.0"));   /* 6i.61: Passion Check back to Cheats */   /* 6h.22; 6i.40: Game Speed / Pause Game to Cheats, Game Hacks; 6i.57: Passion Check above Text scale */
	{ int z; for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Close menu") && !strstr(g_last[z], "Game Speed") && !strstr(g_last[z], "Pause Game")); }   /* 6h.22: gone; 6i.57: Passion Check back in this menu */
	assert(g_last_color[1].b < 0.5f && g_last_color[2].b > 0.5f);   /* the selected line is yellow, the others grey */
	assert(!memcmp(host_draw_string_globals, marker, sizeof(marker)));
	printf("open: 13 lines (title, 11 items, footer), selected line highlighted, game sees a neutral pad\n");

	/* holding the combo doesn't toggle it again or move the selection */
	for (i = 0; i < 20; i++) frame();
	assert(host_menu_open() && host_menu_selected() == 0);

	/* 3. Down selects the next item; up wraps to the bottom */
	release(); frame(); press(0x02, 0, 0); frame(); release(); frame();
	assert(host_menu_selected() == 1);
	press(0x01, 0, 0); frame(); release(); frame(); press(0x01, 0, 0); frame(); release(); frame();
	assert(host_menu_selected() == 10);                                 /* 6i.61: 11 items again */
	printf("navigation: down moves, up wraps from the top to the bottom\n");

	/* 4. A toggles "Superbounce Viewer" (item 3): select it, press A */
	while (host_menu_selected() != 3) { press(0x02, 0, 0); frame(); release(); frame(); }
	assert(host_value(3) == 0);                                        /* off by default */
	press(0, 255, 0); frame(); assert(host_value(3) == 1 && game_neutral());
	frame(); frame(); assert(host_value(3) == 1);                    /* holding A doesn't toggle again */
	release(); frame();
	printf("A toggles once per press; the game never sees A\n");

	/* 5. Text scale: holding right repeats after 12 reads, then every 3, and stops at the top (14) */
	while (host_menu_selected() != 10) { press(0x02, 0, 0); frame(); release(); frame(); }
	assert(host_menu_selected() == 10 && host_value(10) == 10);
	press(0x08, 0, 0); frame(); assert(host_value(10) == 11);
	for (i = 0; i < 11; i++) frame();
	assert(host_value(10) == 11);
	frame(); assert(host_value(10) == 12);
	frame(); frame(); assert(host_value(10) == 12); frame(); assert(host_value(10) == 13);
	for (i = 0; i < 30; i++) frame();
	assert(host_value(10) == 14);
	release(); frame();
	press(0x04, 0, 0); frame(); release(); frame(); assert(host_value(10) == 13);
	printf("right/left change values with Blam-style auto-repeat and stop at the limits\n");

	/* 6. B closes; the game stays neutral until everything is released */
	press(0, 0, 255); frame();
	assert(!host_menu_open() && game_neutral() && host_suppressing());
	press(0x08, 0, 255); frame(); assert(game_neutral());           /* still holding B (and right): still neutral */
	release(); frame(); assert(!host_suppressing());
	press(0x08, 0, 0); frame(); assert(g_game.buttons == 0x08);      /* now the game sees input again */
	release(); g_state = 3; frame();
	assert(g_frame_lines == 1 && strstr(g_last[0], "Back + D-pad Up"));   /* the hint stays while its option is on (main menu) */
	g_state = 1;
	/* 6i.198: a game ending with the menu open (back on the game's main menu): it stays open there and still has the pad;
	 * B closes it and the game's menu gets the pad once everything is released */
	press(0x21, 0, 0); frame(); release(); frame(); assert(host_menu_open());
	g_state = 3;
	press(0x02, 0, 0); frame();
	assert(host_menu_open() && game_neutral());
	release(); frame();
	press(0, 0, 255); frame(); assert(!host_menu_open() && host_suppressing());
	release(); frame(); assert(!host_suppressing());
	press(0x02, 0, 0); frame(); assert(g_game.buttons == 0x02); release(); frame();
	g_state = 1;
	printf("game over with the menu open: still open on the game's main menu; B closes it and the pad is the game's again\n");
	{
		/* 6i.198: the soft lock of 6i.36 / 6i.37 - a controller whose X rests at 5 (never exactly 0): after the menu is
		 * closed with B the pad is given back as soon as everything reads released by the buttons' threshold, X at 5
		 * included; on the game's main menu as in a game */
		void host_set_free_cam(s32 on, s32 block); s32 host_free_cam(void);
		press(0x21, 0, 0); frame(); release(); frame(); assert(host_menu_open());
		g_state = 3;
		press(0, 0, 255); g_pad.analog[2] = 5; frame(); assert(!host_menu_open() && host_suppressing());
		release(); g_pad.analog[2] = 5; frame(); assert(!host_suppressing());
		press(0x02, 0, 0); g_pad.analog[2] = 5; frame(); assert(g_game.buttons == 0x02);
		release(); frame();
		/* the free camera flying (player movement off) when the game ends: the main menu still gets the pad */
		g_state = 1; release(); frame(); host_set_free_cam(1, 1);
		press(0x02, 0, 0); frame(); assert(game_neutral());                     /* in a game: its flight */
		g_state = 3; press(0x02, 0, 0); frame(); assert(g_game.buttons == 0x02);   /* the main menu: the game's */
		release(); frame(); host_set_free_cam(0, 1); g_state = 1; release(); frame();
		printf("an X resting at 5 no longer holds the pad back after a close; on the main menu the free camera's flight lets go\n");
	}
	printf("B closes, input stays blocked until every button is released, then passes through again\n");
	/* (the hint line's option is gone: it shows on the game's main menu only - tested in the Superbounce suite) */

	/* 7. Back + Up toggles it closed too; 6h.22: no "Close menu" item - B closes at the top, as the footer says */
	press(0x21, 0, 0); frame(); assert(host_menu_open());
	release(); frame(); press(0x21, 0, 0); frame(); assert(!host_menu_open());
	release(); frame(); frame();
	press(0x21, 0, 0); frame(); release(); frame();
	{
		int z, foot = 0;
		for (z = 0; z < g_frame_lines; z++) { assert(!strstr(g_last[z], "Close menu")); if (strstr(g_last[z], "B or Back+Up: close")) foot = 1; }
		assert(foot);
	}
	while (host_menu_selected() != 10) { press(0x02, 0, 0); frame(); release(); frame(); }
	assert(strstr(g_last[11], "> Text scale"));                       /* the last item now */
	press(0, 0, 255); frame(); assert(!host_menu_open());
	release(); frame();
	printf("Back + Up and B both close it; no \"Close menu\" item (the footer says B closes)\n");
	assert(logged("menu opened") >= 3 && logged("menu closed") >= 2);
	{ void spooky_main_frame(void); for (i = 0; i < 9000; i++) spooky_main_frame(); }
	assert(logged("Spooky Ghost Menu v2.0 running") == 1);
	{	/* 6i.152: one settings file - every menu setting with a value, Triple Buffering first; written when the menu closes,
		 * read at the start; the Cheats / Skulls menus and the transient toggles are not kept */
		extern char g_host_settings[4096]; extern s32 g_host_settings_len;
		extern void host_set_aniso(s32 on); extern void host_set_fps_show(s32 on); extern void host_settings_reload(void), host_settings_save(void), host_tb_reload(void); extern s32 host_triple(void);
		extern s32 host_aniso(void);
		host_set_aniso(3); host_set_fps_show(1);
		host_settings_save();                                                              /* what the menu's close does */
		assert(g_host_settings_len > 0 && strstr(g_host_settings, "Triple Buffering=") == g_host_settings && g_host_settings[17] == (host_triple() ? '1' : '0'));
		assert(strstr(g_host_settings, "\r\nAnisotropic Filtering=3\r\n") && strstr(g_host_settings, "\r\nFPS Counter=1\r\n"));
		assert(!strstr(g_host_settings, "Pause Game=") && !strstr(g_host_settings, "Invulnerability=") && !strstr(g_host_settings, "Cow Bell="));
		assert(!strstr(g_host_settings, "Superbounce Viewer=") && !strstr(g_host_settings, "Ghost Viewer=") && strstr(g_host_settings, "\r\nText scale="));
		assert(!strstr(g_host_settings, "Performance Mode=") && !strstr(g_host_settings, "Tick Rate=") && strstr(g_host_settings, "\r\nHUD Widescreen Fix="));
		assert(strstr(g_host_settings, "\r\nField of View=") && !strstr(g_host_settings, "Hide HUD=") && !strstr(g_host_settings, "Third person camera") && strstr(g_host_settings, "\r\nDistance (tenths)="));
		assert(!strstr(g_host_settings, "Force Charged state=") && strstr(g_host_settings, "\r\nSeam info text="));
		assert(!strstr(g_host_settings, "Disable natural checkpoints=") && !strstr(g_host_settings, "Out of bounds timer=") && !strstr(g_host_settings, "Trigger names=") && strstr(g_host_settings, "\r\nLaunch forward u/s="));
		host_set_aniso(0); host_set_fps_show(0);
		strcpy(g_host_settings, "Triple Buffering=1\r\nAnisotropic Filtering=99\r\nFPS Counter=1\r\nNo Such Setting=5\r\nPause Game=1\r\n"); g_host_settings_len = (s32)strlen(g_host_settings);
		host_settings_reload();
		assert(host_aniso() == 3 && host_triple() == 1);                                   /* 99 clamped to the top; the triple line read */
		host_set_aniso(1); host_set_fps_show(0); g_host_settings_len = -1; host_settings_reload(); host_tb_reload();
	}
	/* v2.0: Larger Caches is on at EVERY start where the console has 128 MB. Unset at start, it is settled the first time
	 * the memory is known - on above 96 MB, off otherwise, nothing while the memory is not yet measured. It is never in
	 * the settings file, whatever it is set to, and a line for it in an old file is ignored: turned off, it is on again
	 * at the next start */
	{
		void host_set_mem_mb(u32 mb); u32 host_get_mem_mb(void); void host_set_big_cache(s32 on); s32 host_get_big_cache(void); void host_lc_default(void);
		void host_settings_save(void); void host_settings_reload(void); void host_tb_reload(void);
		extern char g_host_settings[4096]; extern s32 g_host_settings_len;
		u32 mb0 = host_get_mem_mb(); s32 bc0 = host_get_big_cache();
		host_set_big_cache(-1); host_set_mem_mb(0); host_lc_default(); assert(host_get_big_cache() == -1);        /* not measured yet: still unset */
		host_set_mem_mb(127); host_lc_default(); assert(host_get_big_cache() == 1);                              /* 128 MB: on */
		host_set_big_cache(-1); host_set_mem_mb(64); host_lc_default(); assert(host_get_big_cache() == 0);        /* 64 MB: off */
		host_set_big_cache(-1); host_set_mem_mb(127); frame(); assert(host_get_big_cache() == 1);                /* ... and settled by the frame itself */
		host_settings_save(); g_host_settings[g_host_settings_len] = 0; assert(!strstr(g_host_settings, "Larger Caches"));   /* on: not in the file */
		host_set_big_cache(0); host_settings_save(); g_host_settings[g_host_settings_len] = 0; assert(!strstr(g_host_settings, "Larger Caches"));   /* turned off: not in the file either */
		strcpy(g_host_settings, "Triple Buffering=1\r\nLarger Caches (128 MB)=0\r\n"); g_host_settings_len = (s32)strlen(g_host_settings);
		host_set_big_cache(-1); host_settings_reload(); assert(host_get_big_cache() == -1);                      /* the next start: an old file's line is ignored */
		host_lc_default(); assert(host_get_big_cache() == 1);                                                     /* ... and it is on again */
		host_set_mem_mb(mb0); host_set_big_cache(bc0 < 0 ? 0 : bc0); g_host_settings_len = -1; host_settings_reload(); host_tb_reload();
		printf("Larger Caches: on at every start with 128 MB, off with 64; never saved\n");
	}
	{	/* 6i.150: Skip Hidden Light Draws stays on in a busy frame - 300 camera draws (contiguous runs merge), then a light
		 * draw over the 290th is still seen (skipped); 6i.147's 256-entry table answered "seen" for nothing past 256 */
		extern void cam_range_add(u32 ip, u32 n); extern void cam_range_reset(void); extern s32 cam_seen(u32 a, u32 n);
		u32 k;
		cam_range_reset();
		for (k = 0; k < 300u; k++) cam_range_add(0x100000u + k * 0x1000u, 6u);                 /* 300 separate ranges */
		assert(cam_seen(0x100000u + 290u * 0x1000u + 4u, 2u) == 1 && cam_seen(0x100000u + 290u * 0x1000u + 0x100u, 2u) == 0);
		cam_range_reset();
		for (k = 0; k < 2000u; k++) cam_range_add(0x200000u + k * 12u, 6u);                     /* 2000 contiguous draws: one entry */
		assert(cam_seen(0x200000u + 1999u * 12u, 1u) == 1 && cam_seen(0x200000u + 2000u * 12u, 1u) == 0 && cam_seen(0x1FFFF0u, 1u) == 0);
		cam_range_reset();
	}
	{	/* 6i.150: the overlay colours are quantised to 16 steps a channel before a pixel-shader slot is chosen */
		extern u32 host_gfx_colour(float a, float r, float g, float b);
		assert(host_gfx_colour(1.f, 1.f, 1.f, 1.f) == 0xFFFFFFFFu);
		assert(host_gfx_colour(1.f, 0.5f, 0.25f, 0.f) == 0xFF773300u);                            /* 127 -> 0x77, 63 -> 0x33 */
		assert(host_gfx_colour(0.6f, 0.f, 0.f, 0.f) == 0x99000000u);
	}             /* at main-loop frame 30 */
	assert(logged("status: main-loop frames") == 24);              /* every 300 frames, at most 24 */
	assert(logged("controller hook running") == 1 && logged("HUD-text hook running") == 1);
	printf("log: first runs logged once, menu opens/closes logged, trainer-loaded line at frame 30, status every 300 frames (24 at most)\n");
	for (i = 0; i < g_nlog && i < 5; i++) printf("   log: %s\n", g_loglines[i]);

	/* 8. a frame to preview: open, "Text scale" selected */
	if (argc > 1)
	{
		press(0x21, 0, 0); frame(); release(); frame();
		press(0x02, 0, 0); frame(); release(); frame(); press(0x02, 0, 0); frame(); release(); frame();
		g_log = fopen(argv[1], "w"); spooky_frame(); fclose(g_log); g_log = 0;
	}
	assert(g_min_room >= 6);                                          /* every rectangle fits the full font height + 6 */
	printf("every line's rectangle holds the full font height at every scale (room %d)\n", g_min_room);
	/* submenus (as Cartographer's tree nodes): A on "Launch / Teleport  >" enters it, B returns to the top, B closes
	 * (6i.36: no "Back" items - the help line's B) */
	{
		int z, ok;
		press(0x21, 0, 0); frame(); release(); frame();
		assert(host_menu_open() && host_menu() == 0);
		while (host_menu_selected() != 2) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 1 && strstr(g_last[0], "Launch / Teleport") && strstr(g_last[1], "Launch (Black + Up)"));
		assert(g_frame_lines == 8);                                          /* title, 6 items, footer (6i.36: no Back item) */
		for (ok = 0, z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "B: back")) ok = 1;
		assert(ok);
		press(0, 0, 255); frame(); release(); frame();                       /* B: back to the top, still open */
		assert(host_menu_open() && host_menu() == 0 && strstr(g_last[0], "Spooky Ghost Menu - by Fortune090"));
		while (host_menu_selected() != 4) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();                       /* Superbounce Viewer Settings */
		assert(host_menu() == 2 && strstr(g_last[1], "Maintain charge state") && strstr(g_last[2], "Force Charged state") && strstr(g_last[3], "Seam info text") && strstr(g_last[4], "Speed above the charge indicator") && g_frame_lines == 12 && strstr(g_last[6], "Seams near you") && strstr(g_last[7], "Passion Check") && strstr(g_last[8], "Monster/Dreadnought Viewer: off") && strstr(g_last[9], "Monster/Dreadnought Viewer: 35+ only: off") && strstr(g_last[10], "Monster/Dreadnought range (units): Max"));   /* 6i.170: Passion Check under Seams near you */   /* 6h.18; 6h.34; 6h.42: the circle toggle gone */
		{
			/* 6d.6: the range in steps of 10 - right at the top stays 2000, left 1990, 1980 */
			s32 host_pred_range(void); void host_set_pred_range(s32 r);
			while (host_menu_selected() != 9) { press(0x02, 0, 0); frame(); release(); frame(); }
			/* 6h.18: Max, then right 10, 20 ... 200 (stops there); left back down to Max (stops there) */
			int z;
			press(0x04, 0, 0); frame(); release(); frame(); assert(host_pred_range() == 0 && strstr(g_last[10], "range (units): Max"));
			press(0x08, 0, 0); frame(); release(); frame(); assert(host_pred_range() == 10 && strstr(g_last[10], "Monster/Dreadnought range (units): 10"));
			press(0x08, 0, 0); frame(); release(); frame(); assert(host_pred_range() == 20 && strstr(g_last[10], "range (units): 20"));
			for (z = 0; z < 25; z++) { press(0x08, 0, 0); frame(); release(); frame(); }
			assert(host_pred_range() == 200 && strstr(g_last[10], "range (units): 200"));
			for (z = 0; z < 25; z++) { press(0x04, 0, 0); frame(); release(); frame(); }
			assert(host_pred_range() == 0 && strstr(g_last[10], "range (units): Max"));
			host_set_pred_range(0);
		}
		press(0, 0, 255); frame(); release(); frame();                       /* B (6i.36: the Back items gone) */
		assert(host_menu() == 0 && host_menu_open());
		/* 6h: Ghost Viewer Settings - the offset save state's three items on top; Rewind poses, Backstep hold, Max believed
		 * poses and Offset threshold gone */
		while (host_menu_selected() != 6) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 5 && strstr(g_last[0], "Ghost Viewer Settings") && strstr(g_last[1], "> Save offsets (Black + Left)")
		       && strstr(g_last[2], "Revert offsets (Black + Right)") && strstr(g_last[3], "Restore offsets after natural reverts: off")
		       && strstr(g_last[4], "Believed poses: on") && strstr(g_last[5], "Travel regions: on") && strstr(g_last[6], "Object believed poses: on")
		       && strstr(g_last[7], "Object travel regions: off") && g_frame_lines == 9);
		{
			int z;
			for (z = 0; z < g_frame_lines; z++)
				assert(!strstr(g_last[z], "Rewind") && !strstr(g_last[z], "Backstep") && !strstr(g_last[z], "Max believed") && !strstr(g_last[z], "threshold") && !strstr(g_last[z], " too"));
		}
		press(0, 0, 255); frame(); release(); frame();                       /* B: back to the top */
		assert(host_menu() == 0 && host_menu_open());
		printf("Ghost Viewer Settings: Save offsets, Revert offsets, Restore offsets after natural reverts on top; the four removed\n");
		press(0, 0, 255); frame(); release(); frame();                       /* B at the top: closed */
		assert(!host_menu_open());
		printf("submenus: A enters Launch / Teleport and Superbounce Viewer Settings; B returns (no Back items); B at the top closes\n");
	}
	/* 6h: Black + D-pad Left / Right (6h.27: was White) = Save / Revert offsets - only with the Ghost Viewer on (as
	 * Cartographer's shortcut); Back + D-pad Left / Right stay Create / Revert Checkpoint */
	{
		void host_set_ghost(s32 on); const char *host_toast(void); void spooky_main_frame(void); void host_practice(s32 what);
		release(); frame();
		press(0x04, 0, 0); g_pad.analog[4] = 255; frame(); spooky_main_frame(); release(); frame();      /* viewer off: nothing */
		assert(!strstr(host_toast(), "offsets") && !strstr(host_toast(), "Offsets"));
		host_set_ghost(1);
		g_pad.analog[4] = 255; frame(); press(0x04, 0, 0); g_pad.analog[4] = 255; frame(); spooky_main_frame(); release(); frame();
		assert(strstr(host_toast(), "Offsets: no game in progress"));                  /* the save asked for (no game here) */
		host_practice(1);                                                                /* (any other toast in between) */
		press(0x08, 0, 0); g_pad.analog[4] = 255; frame(); spooky_main_frame(); release(); frame();
		assert(strstr(host_toast(), "Offsets: no game in progress"));                  /* Black + Right: the revert */
		host_set_ghost(0);
		printf("Black + D-pad Left / Right: save / revert offsets, with the Ghost Viewer on only\n");
	}
	/* 6h.27: White + D-pad Left / Right (menu closed) - Game Speed down / up 0.1, held: again after 12 reads then every 3,
	 * stopping at 0.0 and 5.0; White + D-pad Down - Pause Game on / off (once a press); each said on screen. With the Ghost
	 * Viewer on, White + Left / Right is still Game Speed. The free camera's D-pad speed isn't touched by White / Black */
	{
		void host_set_speed(s32 tenths, s32 pause); s32 host_speed_get(void); s32 host_pause_get(void); s32 host_req_get(void);
		void host_set_ghost(s32 on); const char *host_toast(void); void host_set_free_cam(s32 on, s32 block); s32 host_fc_steps(void);
		void host_req(s32 r);
		int z, st;
		release(); frame();
		host_set_speed(10, 0);
		press(0x08, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();
		assert(host_speed_get() == 11 && !strcmp(host_toast(), "Game Speed: 1.1"));
		press(0x04, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();
		press(0x04, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();
		assert(host_speed_get() == 9 && !strcmp(host_toast(), "Game Speed: 0.9"));
		press(0x04, 0, 0); g_pad.analog[5] = 255; frame(); assert(host_speed_get() == 8);          /* held */
		for (z = 0; z < 11; z++) frame();
		assert(host_speed_get() == 8);
		frame(); assert(host_speed_get() == 7);
		frame(); frame(); assert(host_speed_get() == 7); frame(); assert(host_speed_get() == 6);
		for (z = 0; z < 60; z++) frame();
		assert(host_speed_get() == 0 && !strcmp(host_toast(), "Game Speed: 0.0"));               /* stops at 0.0 */
		release(); frame();
		press(0x08, 0, 0); g_pad.analog[5] = 255; for (z = 0; z < 300; z++) frame(); release(); frame();
		assert(host_speed_get() == 50 && !strcmp(host_toast(), "Game Speed: 5.0"));              /* stops at 5.0 */
		host_set_speed(10, 0);
		/* 6i.169: the Campaign Timer on, in a campaign game - Game Speed cannot be changed, by the shortcut or in the menu,
		 * and says why; the timer off: as ever */
		{
			void host_set_ctimer(s32 on); void host_set_menu_open(s32 open, s32 m); int q, found = 0;
			host_set_ctimer(1);
			press(0x08, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();              /* White + Right */
			assert(host_speed_get() == 10 && !strcmp(host_toast(), "Game Speed: 1.0 while the Campaign Timer is on"));
			{ int z, at = -1; for (z = 0; z < g_frame_lines; z++) if (!strncmp(g_last[z], "Total Time: 00:00:00.00", 23)) at = z;   /* the timer's line: bottom-right - its box ends 9 from the right edge, the text 10 above the bottom */
			  assert(at >= 0 && g_last_left[at] == 640 - 9 - 420 && g_last_top[at] >= 440 && g_last_top[at] <= 462); }
			press(0x04, 0, 0); g_pad.analog[5] = 255; for (q = 0; q < 40; q++) frame(); release(); frame();   /* White + Left, held */
			assert(host_speed_get() == 10);
			host_set_menu_open(1, 4); frame();                                                 /* Cheats > Game Speed */
			for (q = 0; q < 12 && !found; q++)
			{
				int z; for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "> Game Speed")) found = 1;
				if (!found) { press(0x02, 0, 0); frame(); release(); frame(); }
			}
			assert(found);
			press(0x08, 0, 0); frame(); release(); frame(); assert(host_speed_get() == 10);       /* right: no */
			press(0x04, 0, 0); frame(); release(); frame(); assert(host_speed_get() == 10);       /* left: no */
			press(0, 255, 0); frame(); release(); frame(); assert(host_speed_get() == 10);        /* A: no */
			host_set_ctimer(0);
			press(0x08, 0, 0); frame(); release(); frame(); assert(host_speed_get() == 11);       /* the timer off: it changes */
			host_set_speed(10, 0); host_set_menu_open(0, 0); release(); frame(); frame();
			press(0x08, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame(); assert(host_speed_get() == 11);
			host_set_speed(10, 0);
		}
		press(0x02, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();
		assert(host_pause_get() == 1 && host_speed_get() == 10 && !strcmp(host_toast(), "Pause Game: on"));
		press(0x02, 0, 0); g_pad.analog[5] = 255; frame(); frame(); frame(); release(); frame();  /* held: once */
		assert(host_pause_get() == 0 && !strcmp(host_toast(), "Pause Game: off"));
		press(0x04, 0, 0); frame(); release(); frame(); assert(host_speed_get() == 10);         /* the D-pad alone: nothing */
		host_set_ghost(1); host_req(0);
		press(0x04, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();
		assert(host_speed_get() == 9 && host_req_get() != 10);                                    /* not the offsets */
		host_set_ghost(0); host_set_speed(10, 0);
		/* the menu open: White + Left is just the menu's left */
		press(0x21, 0, 0); frame(); release(); frame(); assert(host_menu_open());
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }   /* on Cheats */
		press(0x04, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();
		assert(host_speed_get() == 10 && host_menu_open() && host_menu() == 0);                   /* no shortcut with the menu open */
		press(0, 0, 255); frame(); release(); frame(); assert(!host_menu_open());
		host_set_speed(10, 0);
		/* the free camera flying: D-pad Down is its speed; White + Down and Black + Down leave it */
		host_set_free_cam(1, 1); st = host_fc_steps();
		press(0x02, 0, 0); frame(); release(); frame(); assert(host_fc_steps() == st - 1);
		press(0x02, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame();
		assert(host_fc_steps() == st - 1 && host_pause_get() == 1);                               /* paused, the speed left */
		press(0x02, 0, 0); g_pad.analog[5] = 255; frame(); release(); frame(); assert(host_pause_get() == 0);
		press(0x02, 0, 0); g_pad.analog[4] = 255; frame(); release(); frame(); assert(host_fc_steps() == st - 1);   /* Black + Down too */
		host_req(0);
		press(0x01, 0, 0); frame(); release(); frame(); assert(host_fc_steps() == st);
		host_set_free_cam(0, 1);
		printf("White + D-pad Left / Right: Game Speed -/+ 0.1 (held: repeats, stops at 0.0 and 5.0), White + Down: Pause Game, said on screen; the free camera's speed untouched\n");
	}
	/* step 6c: Back + Black toggles the free camera; while it flies the game sees a neutral pad (Start passes); with
	 * "Disable player movement" off the game gets the pad */
	{
		void host_set_free_cam(s32 on, s32 block); s32 host_free_cam(void); void spooky_main_frame(void);
		release(); frame();
		press(0x20, 0, 0); g_pad.analog[4] = 255; frame(); spooky_main_frame();           /* Back + Black */
		release(); frame();
		assert(host_free_cam() == 1);
		memset(&g_pad, 0, sizeof(g_pad)); g_pad.lx = 20000; g_pad.ry = -12000; frame();
		assert(g_game.lx == 0 && g_game.ry == 0 && g_game.buttons == 0);                 /* flying: the game sees nothing */
		press(0x10, 0, 0); frame(); assert(g_game.buttons == 0x10);                      /* Start passes */
		{
			/* step 6c.4: stick drift (8500 / -8000 - inside the player controls' +-9000) flies nothing, two seconds of it */
			void host_fc_pose(float *p, float *yaw, float *pitch);
			float p0[3], p1[3], y0, y1, q0, q1;
			int k;
			release(); frame(); spooky_main_frame();
			host_fc_pose(p0, &y0, &q0);
			memset(&g_pad, 0, sizeof(g_pad)); g_pad.lx = 8500; g_pad.ly = -8000; g_pad.rx = -8500; g_pad.ry = 8000; g_pad.analog[6] = 30; g_pad.analog[7] = 30;
			for (k = 0; k < 60; k++) { frame(); spooky_main_frame(); }
			host_fc_pose(p1, &y1, &q1);
			assert(p1[0] == p0[0] && p1[1] == p0[1] && p1[2] == p0[2] && y1 == y0 && q1 == q0);
			memset(&g_pad, 0, sizeof(g_pad));
			printf("free camera: stick drift inside the player controls' dead zone (+-9000) moves nothing\n");
		}
		release(); host_set_free_cam(1, 0);
		memset(&g_pad, 0, sizeof(g_pad)); g_pad.lx = 20000; frame();
		assert(g_game.lx == 20000);                                                    /* player movement on: the game's */
		host_set_free_cam(1, 1); release(); frame();
		press(0x20, 0, 0); g_pad.analog[4] = 255; frame(); spooky_main_frame(); release(); frame();
		assert(host_free_cam() == 0);
		printf("free camera: Back + Black toggles it; flying, the game sees a neutral pad (Start passes); player movement on: the game's pad\n");
	}
	/* 6i.120 / 6i.122: the free camera's zoom - the right stick's click zooms in and out; D-pad Right / Left set the level,
	 * 2x - 20x, held: again and again, quickly; the level kept while zoomed out */
	{
		void host_set_free_cam(s32 on, s32 block); s32 host_fc_zoom(void);
		int k, was = g_state;
		g_state = 1; host_set_free_cam(1, 1); release(); frame();
		assert(host_fc_zoom() == 0);
		press(0x80, 0, 0); frame(); release(); frame(); assert(host_fc_zoom() == 2);
		press(0x08, 0, 0); frame(); release(); frame(); assert(host_fc_zoom() == 3);
		press(0x08, 0, 0); for (k = 0; k < 60; k++) frame(); release(); frame(); assert(host_fc_zoom() == 20);
		press(0x04, 0, 0); frame(); release(); frame(); assert(host_fc_zoom() == 19);
		press(0x80, 0, 0); frame(); release(); frame(); assert(host_fc_zoom() == 0);
		press(0x80, 0, 0); frame(); release(); frame(); assert(host_fc_zoom() == 19);
		press(0x04, 0, 0); for (k = 0; k < 60; k++) frame(); release(); frame(); assert(host_fc_zoom() == 2);
		press(0x80, 0, 0); frame(); release(); frame(); assert(host_fc_zoom() == 0);
		host_set_free_cam(0, 1); release(); frame(); g_state = was;
		printf("free camera zoom: the right stick's click in / out; D-pad Right / Left the level 2x - 20x, held: quickly\n");
	}
	/* step 6c.1: Camera Settings - the cameras first, then the third person camera's own settings */
	{
		void host_set_menu_open(s32 open, s32 m);
		host_set_menu_open(1, 3); frame();
		assert(strstr(g_last[0], "Camera Settings") && strstr(g_last[1], "Third person camera (Back + Down): off") && strstr(g_last[2], "Third Person Settings  >")
		       && strstr(g_last[3], "Free camera (Back + Black): off") && strstr(g_last[4], "Disable freecam player input: on") && strstr(g_last[5], "Lock camera pitch: on")
		       && strstr(g_last[6], "Teleport to camera (Back + White)") && strstr(g_last[7], "Hide HUD: off") && strstr(g_last[8], "Field of View: game's") && strstr(g_last[9], "B: back"));   /* 6i.120 */   /* 6i.36: Unlock FPS (now Performance Mode, in Cheats), its settings, the tests and Back gone - the footer next (a message may follow) */
		host_set_menu_open(0, 0); frame();
		/* Third Person Settings, within it: A on it enters; B goes up to Camera Settings (still on that item), B there to
		 * the top, B at the top closes */
		release(); frame();
		press(0x20, 0, 0); press(0x21, 0, 0); frame(); release(); frame();                    /* Back + Up: open */
		assert(host_menu_open() && host_menu() == 0);
		while (host_menu_selected() != 8) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 3);              /* Camera Settings */
		press(0x02, 0, 0); frame(); release(); frame();
		press(0, 255, 0); frame(); release(); frame();                                         /* Third Person Settings */
		assert(host_menu() == 7 && strstr(g_last[0], "Third Person Settings") && strstr(g_last[1], "Distance") && strstr(g_last[2], "Height")
		       && strstr(g_last[3], "Side") && strstr(g_last[4], "Ignore Collision: on") && strstr(g_last[5], "B: back"));   /* 6h.11: on by default; 6i.36: no Back - the footer next */
		press(0, 0, 255); frame(); release(); frame();                                         /* B: up to Camera Settings */
		assert(host_menu() == 3 && host_menu_selected() == 1);
		press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 7);               /* in again */
		press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 3);               /* B up again */
		press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 0 && host_menu_selected() == 8);   /* B: the top */
		while (host_menu_selected() != 4) { press(0x01, 0, 0); frame(); release(); frame(); }   /* (left where the later tests expect) */
		press(0, 0, 255); frame(); release(); frame(); assert(!host_menu_open());
		printf("Camera Settings: third person, its settings (a submenu), free camera, disable player movement, lock camera pitch (on), teleport; B goes up one level\n");
	}
	/* 6i: Unlock FPS - 6i.36: Performance Mode, in Cheats under HUD Widescreen Fix; A toggles it; with it on, a held
	 * D-pad's repeat is counted in 30 Hz steps - at 60 (a vblank a frame) the second move comes 24 frames on, not 12 (the
	 * same ~0.4 s), then every 6 */
	{
		void host_set_menu_open(s32 open, s32 m); s32 host_unlock_fps(void); void spooky_main_frame(void);
		int f, fires, second, last;
		host_set_menu_open(1, 10); frame();
		assert(strstr(g_last[0], "Game Hacks") && strstr(g_last[1], "HUD Widescreen Fix: on") && strstr(g_last[2], "Performance Mode: off") && strstr(g_last[3], "Performance Mode Settings  >") && strstr(g_last[4], "Tick Rate: 30")
		       && strstr(g_last[5], "Toggle Triple Buffering (restarts)") && strstr(g_last[6], "Larger Caches (128 MB): not available (64 MB)") && strstr(g_last[7], "B: back"));   /* (6i.219: the water test setting gone again) */   /* 6i.197: Toggle Triple Buffering here, above Larger Caches */   /* 6i.191: Larger Caches; 6i.192: says so on a 64 MB console */
		{
			/* 6i.192: on a 64 MB console it cannot be turned on (a note says why); with the memory it reads off / on and toggles */
			void host_set_mem_mb(u32 mb); u32 host_get_mem_mb(void); s32 host_get_big_cache(void); const char *host_toast(void); void host_clear_toast(void);
			u32 mb = host_get_mem_mb(); int sel0 = host_menu_selected();
			while (host_menu_selected() != 5) { press(0x02, 0, 0); frame(); release(); frame(); }
			press(0, 255, 0); frame(); release(); frame();
			assert(host_get_big_cache() == 0 && !strcmp(host_toast(), "Larger Caches: needs a 128 MB console") && strstr(g_last[6], "> Larger Caches (128 MB): not available (64 MB)"));
			host_clear_toast(); host_set_mem_mb(127); frame(); assert(strstr(g_last[6], "> Larger Caches (128 MB): off"));
			{
				/* 6i.201: with a level loaded it asks first - "This will restart the level, are you sure?", what it is now, Yes /
				 * No with No selected; No goes back with nothing changed; Yes changes it, saves, sets the game's Restart Level
				 * flag and closes the menu. On the game's main menu (no level) it just changes, as before */
				extern u32 g_host_level_reset; void spooky_main_frame(void); u32 r0 = g_host_level_reset;
				press(0, 255, 0); frame(); release(); frame();
				assert(host_menu() == 26 && strstr(g_last[0], "This will restart the level, are you sure?") && strstr(g_last[1], "Larger Caches is currently: off")
				       && strstr(g_last[2], "  Yes") && strstr(g_last[3], "> No") && host_get_big_cache() == 0);
				press(0, 255, 0); frame(); release(); frame();
				assert(host_menu() == 10 && host_get_big_cache() == 0 && g_host_level_reset == r0 && strstr(g_last[6], "> Larger Caches (128 MB): off"));
				press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 26);
				press(0x01, 0, 0); frame(); release(); frame(); assert(strstr(g_last[2], "> Yes"));
				press(0, 255, 0); frame(); release(); frame(); spooky_main_frame();
				assert(host_get_big_cache() == 1 && g_host_level_reset == r0 + 1 && !host_menu_open() && !strcmp(host_toast(), "Larger Caches: on - restarting the level"));
				host_clear_toast(); host_set_menu_open(1, 10); frame();
				while (host_menu_selected() != 5) { press(0x02, 0, 0); frame(); release(); frame(); }
				assert(strstr(g_last[6], "> Larger Caches (128 MB): on"));
				g_state = 3;                                                          /* the game's main menu: no question, no restart */
				press(0, 255, 0); frame(); release(); frame();
				assert(host_menu() == 10 && host_get_big_cache() == 0 && g_host_level_reset == r0 + 1 && strstr(g_last[6], "> Larger Caches (128 MB): off"));
				g_state = 1; frame();
			}
			host_set_mem_mb(mb);
			while (host_menu_selected() != sel0) { press(0x01, 0, 0); frame(); release(); frame(); }
		}   /* 6i.121: Tick Rate under Performance Mode Settings */   /* 6i.40; 6i.47: the HUD tests set and gone; 6i.57: Passion Check to the main menu */
		release(); frame();
		while (host_menu_selected() != 1) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_unlock_fps() == 1 && strstr(g_last[2], "> Performance Mode: on"));
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
		/* held down, unlocked at 60: a step every other frame */
		release(); g_vblank++; spooky_main_frame(); frame();
		last = host_menu_selected(); fires = 0; second = -1;
		for (f = 0; f < 60; f++)
		{
			press(0x02, 0, 0); g_vblank++; spooky_main_frame(); frame();
			if (host_menu_selected() != last) { fires++; if (fires == 2) second = f; last = host_menu_selected(); }
		}
		assert(second >= 23 && second <= 25 && fires >= 7 && fires <= 8);    /* the press, 24, then every 6: 0 24 30 36 42 48 54 */
		release(); g_vblank++; spooky_main_frame(); frame();
		/* off: a frame each, as ever (the vblanks don't matter) */
		while (host_menu_selected() != 1) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame(); assert(host_unlock_fps() == 0 && strstr(g_last[2], "> Performance Mode: off"));
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
		g_vblank++; spooky_main_frame(); frame();
		last = host_menu_selected(); fires = 0; second = -1;
		for (f = 0; f < 30; f++)
		{
			press(0x02, 0, 0); g_vblank++; spooky_main_frame(); frame();
			if (host_menu_selected() != last) { fires++; if (fires == 2) second = f; last = host_menu_selected(); }
		}
		assert(second == 12 && fires == 7);                                   /* 0 12 15 18 21 24 27 */
		release(); frame();
		host_set_menu_open(0, 0); frame();
		printf("Performance Mode: in Game Hacks under HUD Widescreen Fix, A toggles it; a held D-pad repeats after ~0.4 s, then every ~0.1 s, at 60 as at 30\n");
	}
	/* step 6d.1: the Monster/Dreadnought Viewer's two items in Superbounce Viewer Settings (before Back), none in Ghost Viewer Settings */
	{
		void host_set_menu_open(s32 open, s32 m); int q, found = 0, ghost = 0;
		host_set_menu_open(1, 2); frame();
		for (q = 0; q < g_frame_lines; q++) if (strstr(g_last[q], "Monster/Dreadnought Viewer: off") || strstr(g_last[q], "Monster/Dreadnought Viewer: 35+ only: off")) found++;
		host_set_menu_open(1, 5); frame();
		for (q = 0; q < g_frame_lines; q++) if (strstr(g_last[q], "Predicted") || strstr(g_last[q], "Monster")) ghost++;
		host_set_menu_open(0, 0); frame();
		assert(found == 2 && ghost == 0);
		printf("Superbounce Viewer Settings: Monster/Dreadnought Viewer and its 35+ only (both off); none in Ghost Viewer Settings\n");
	}
	/* the Timers menu (the 7th): down inside it moves only its own selection - 6a.1 kept 6 menus' state, so its selection was
	 * the main menu's scroll (and its scroll past the end): down there moved other menus' state, then crashed */
	{
		s32 host_sel_of(s32 m); s32 host_first_of(s32 m); s32 host_menu_count(void);
		s32 z, sel2, first0;
		assert(host_menu_count() == 27);   /* 6i.158: + Object Spawning and its 11 */   /* 6e: + Skulls (within Cheats); 6h.12: + Checkpoint Settings; 6i.40: + Game Hacks, Performance Mode Settings (6i.42: Tests gone) */
		press(0x21, 0, 0); frame(); release(); frame();
		assert(host_menu_open() && host_menu() == 0);
		while (host_menu_selected() != 7) { press(0x02, 0, 0); frame(); release(); frame(); }
		sel2 = host_sel_of(2); first0 = host_first_of(0);
		press(0, 255, 0); frame(); release(); frame();                       /* A: into Timers */
		assert(host_menu() == 6 && strstr(g_last[0], "Other Tools") && strstr(g_last[1], "Trigger Volume Overlay: off") && strstr(g_last[2], "Trigger Volume Overlay Settings  >") && strstr(g_last[3], "Out of Bounds Timer: off") && strstr(g_last[4], "Sleep Timer: off") && strstr(g_last[5], "Havok Island Viewer: off") && strstr(g_last[6], "Show Stored Velocity: off") && strstr(g_last[7], "Crouch Turbo: off") && strstr(g_last[8], "Wireframe: off") && strstr(g_last[9], "Campaign Timer: off") && strstr(g_last[10], "Object Spawning  >") && g_frame_lines == 12);   /* 6i.156: + Campaign Timer; 6i.158: + Object Spawning */   /* 6i.124 */   /* 6i.121: Tick Rate to Game Hacks */   /* 6i.120: Timers is Other Tools */   /* 6i.36: no Back */
		for (z = 0; z < 10; z++)
		{
			press(0x02, 0, 0); frame(); release(); frame();                   /* down, again and again */
			assert(host_menu() == 6 && host_menu_selected() == (z + 1) % 10);
			assert(host_sel_of(0) == 7 && host_first_of(0) == first0 && host_sel_of(2) == sel2);   /* nothing else moved (6i.40: Timers is 8) */
		}
		press(0, 0, 255); frame(); release(); frame();                       /* B: back to the top, still on Timers */
		assert(host_menu() == 0 && host_menu_selected() == 7 && strstr(g_last[0], "Spooky Ghost Menu - by Fortune090"));
		while (host_menu_selected() != 4) { press(0x01, 0, 0); frame(); release(); frame(); }   /* left where the next test expects it */
		press(0, 0, 255); frame(); release(); frame();
		assert(!host_menu_open());
		printf("Other Tools menu (was Timers): down moves only its own 9 items (9 times), the main menu's selection and scroll untouched\n");
	}
	/* scrolling: only 5 rows fit - the 12 items scroll with the selection, "more" lines marking each end that has more */
	{
		int z, sel_seen, above, below;
		host_set_rows_cap(5);
		press(0x21, 0, 0); frame(); release(); frame();                       /* reopens where it was left (item 4) */
		assert(g_frame_lines == 7 && host_menu_selected() == 4 && strstr(g_last[1], "more above") && strstr(g_last[4], "> Superbounce Viewer Settings"));
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
		assert(g_frame_lines == 7 && strstr(g_last[5], "more below") && !strstr(g_last[1], "more above"));   /* title, 5 rows, footer */
		for (z = 0; z < 10; z++)                                              /* 6i.61: 11 items */
		{
			int k;
			press(0x02, 0, 0); frame(); release(); frame();
			for (sel_seen = 0, above = 0, below = 0, k = 1; k < g_frame_lines - 1; k++)
			{
				if (g_last[k][0] == '>') sel_seen = 1;
				if (strstr(g_last[k], "more above")) above = 1;
				if (strstr(g_last[k], "more below")) below = 1;
			}
			assert(sel_seen && g_frame_lines == 7);                         /* the selection always on screen */
			if (host_menu_selected() == 10) assert(above && !below && strstr(g_last[5], "> Text scale"));
			if (host_menu_selected() >= 4 && host_menu_selected() <= 6) assert(above && below);
		}
		press(0x02, 0, 0); frame(); release(); frame();                       /* wraps to the top */
		assert(host_menu_selected() == 0 && strstr(g_last[1], "> Cheats") && strstr(g_last[5], "more below"));
		press(0, 0, 255); frame(); release(); frame();
		host_set_rows_cap(0);
		printf("scrolling: 11 items in 5 rows - the selection always shown, \"more\" lines at each end with more\n");
	}
	/* the Cheats submenu: Invulnerability, Disable AI, Infinite Ammo, Bottomless Clip */
	{
		press(0x21, 0, 0); frame(); release(); frame();
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 4 && strstr(g_last[0], "Cheats") && strstr(g_last[1], "Invulnerability: off") && strstr(g_last[2], "Infinite Ammo: off") &&
			strstr(g_last[3], "Bottomless Clip: off") && strstr(g_last[4], "One Shot Kills: off") && strstr(g_last[5], "Disable AI: off") && strstr(g_last[6], "Game Speed: 1.0") && strstr(g_last[7], "Pause Game: off") &&
			strstr(g_last[8], "Bump Possession: off") && strstr(g_last[9], "Cycle Possession") && strstr(g_last[10], "Revert to player possession") && strstr(g_last[11], "Invincible Vehicles: off") &&
			strstr(g_last[12], "Skulls  >") && strstr(g_last[13], "B: back"));   /* 6i.170: One Shot Kills under Bottomless Clip; Passion Check to Superbounce Viewer Settings */   /* 6i.120: Wireframe to Other Tools */   /* 6i.61: Passion Check above Skulls */   /* 6i.40: Game Speed / Pause Game in; 6i.44: Wireframe (Passion Check to Game Hacks) */
		{
			/* 6h.5: Cycle Possession and Revert to player possession leave the menu open; their message shows under it */
			void spooky_main_frame(void);
			int q, found = 0;
			while (host_menu_selected() != 8) { press(0x02, 0, 0); frame(); release(); frame(); }   /* item 8 (line 9; 6i.170: One Shot Kills above) */
			press(0, 255, 0); frame(); release();
			spooky_main_frame(); frame();
			assert(host_menu_open() && host_menu() == 4);
			for (q = 0; q < g_frame_lines; q++) if (strstr(g_last[q], "Cycle Possession: no game in progress")) found = 1;
			assert(found);
			press(0x02, 0, 0); frame(); release(); frame();
			assert(host_menu_selected() == 9);
			press(0, 255, 0); frame(); release();
			spooky_main_frame(); frame();
			assert(host_menu_open() && host_menu() == 4);
			for (q = 0, found = 0; q < g_frame_lines; q++) if (strstr(g_last[q], "Revert to player possession: no game in progress")) found = 1;
			assert(found);
			printf("cycle / revert to player possession: the menu stays open, their message under it\n");
		}
		{
			/* 6e: Skulls - in the order they're found; Whuppopotamus on sets the game's skull 14; B back to Cheats */
			int q, ok = 1;
			static const char *const order[] = { "Cow Bell", "Thunderstorm", "Blind", "I Would Have Been Your Daddy", "Catch", "Grunt Birthday Party", "Famine" };
			while (host_menu_selected() != 11) { press(0x02, 0, 0); frame(); release(); frame(); }   /* 6i.40; 6i.61: under Passion Check; 6i.120: 11 */
			press(0, 255, 0); frame(); release(); frame();
			assert(host_menu() == 8 && strstr(g_last[0], "Skulls"));
			for (q = 0; q < 7 && q + 1 < g_frame_lines; q++) if (!strstr(g_last[q + 1], order[q])) ok = 0;
			assert(ok);
			{
				s32 host_skull(s32 i);
				press(0, 255, 0); frame(); release(); frame();                 /* A on Cow Bell (the game's Whuppopotamus): skull 14 */
				assert(host_skull(14) == 1 && host_skull(3) == 0);
				press(0, 255, 0); frame(); release(); frame();                 /* off again */
				assert(host_skull(14) == 0);
			}
			press(0, 0, 255); frame(); release(); frame();
			assert(host_menu() == 4);
			printf("skulls: in the campaign's order; Whuppopotamus toggles the game's skull 14; B back to Cheats\n");
		}
		{
			/* 6i.40 / 6i.42: Game Hacks > Performance Mode Settings - the FPS Counter off, VSync on, Late Frames on, FPS Limit 0,
			 * Anisotropic Filtering off, the three split-screen levers off (Tests gone with the UDMA mode) */
			void host_set_menu_open(s32 open, s32 m);
			host_set_menu_open(1, 10); frame();
			while (host_menu_selected() != 2) { press(0x02, 0, 0); frame(); release(); frame(); }
			press(0, 255, 0); frame(); release(); frame();
			assert(host_menu() == 11 && strstr(g_last[0], "Performance Mode Settings") && strstr(g_last[1], "FPS Counter: off") && strstr(g_last[2], "VSync: on")
			       && strstr(g_last[3], "Allow Tearing: on") && strstr(g_last[4], "FPS Limit (0 none): 0") && strstr(g_last[5], "Render Scale: 100%")
			       && strstr(g_last[6], "Adaptive Resolution Scaling: off") && strstr(g_last[7], "Anisotropic Filtering: 2x fast") && strstr(g_last[8], "Light Filtering: faster") && strstr(g_last[9], "World Mip Blending: off")   /* 6i.189: Allow Tearing on by default; Anisotropic Filtering under Adaptive Resolution Scaling */
			       && strstr(g_last[10], "Lights & Shadows") && strstr(g_last[11], "Object Shadows: on") && strstr(g_last[12], "Dynamic Lighting"));   /* 6i.154: the two Supersampling items gone (Render Scale does it) */
			{ int z; for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Supersampling")); }
			{	/* 6i.61: Render Scale 100 % down to 50 % in tens, and back up */
				int k;
				while (host_menu_selected() != 4) { press(0x02, 0, 0); frame(); release(); frame(); }
				assert(strstr(g_last[5], "> Render Scale: 100%"));
				for (k = 9; k >= 4; k--) { press(0x04, 0, 0); frame(); release(); frame(); assert(strstr(g_last[5], k >= 5 ? (k == 9 ? "> Render Scale: 90%" : k == 8 ? "> Render Scale: 80%" : k == 7 ? "> Render Scale: 70%" : k == 6 ? "> Render Scale: 60%" : "> Render Scale: 50%") : "> Render Scale: 50%")); }
				for (k = 0; k < 5; k++) { press(0x08, 0, 0); frame(); release(); frame(); }
				assert(strstr(g_last[5], "> Render Scale: 100%"));
				/* 6i.130: on up to 150 %; 6i.149v: to 200 % (and no further); each step above 100 % measures the memory it would
				 * need and says so (the toast; 6i.152: short, and the page shows no memory lines any more) - the host's console:
				 * 64 MB, 160 KB free, a 624 KB block: no room */
				for (k = 0; k < 5; k++) { press(0x08, 0, 0); frame(); release(); frame(); }
				{ const char *host_toast(void); assert(strstr(g_last[5], "> Render Scale: 150%") && !strcmp(host_toast(), "Render Scale 150%: no room - stays at 100%")); }
				for (k = 0; k < 6; k++) { press(0x08, 0, 0); frame(); release(); frame(); }
				{ const char *host_toast(void); assert(strstr(g_last[5], "> Render Scale: 200%") && !strcmp(host_toast(), "Render Scale 200%: no room - stays at 100%")); }
				for (k = 0; k < 5; k++) { press(0x04, 0, 0); frame(); release(); frame(); }
				assert(strstr(g_last[5], "> Render Scale: 150%"));
				{ int z; for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Memory: kernel") && !strstr(g_last[z], "RAM fitted") && !strstr(g_last[z], "Above 64 MB")); }   /* 6i.152: gone */
				for (k = 0; k < 5; k++) { press(0x04, 0, 0); frame(); release(); frame(); }
				assert(strstr(g_last[5], "> Render Scale: 100%"));
			}   /* 6i.41: Lower Max LoD gone; 6i.42: the FPS Counter on top; 6i.55: the limit and the status line gone; 6i.58: the water tests */
			{
				int f2, z;
				for (f2 = 0; f2 < 200; f2++) frame();                         /* 6i.55: no status line under it, however long */
				for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Interpolation"));
			}
			{
				/* 6i.53: Anisotropic Filtering - off, 2x fast, 2x, full (the top), and back */
				static const char *const lv[] = { "> Anisotropic Filtering: off", "> Anisotropic Filtering: 2x fast", "> Anisotropic Filtering: 2x", "> Anisotropic Filtering: full", "> Anisotropic Filtering: full" };
				int k;
				while (host_menu_selected() != 6) { press(0x02, 0, 0); frame(); release(); frame(); }
				assert(strstr(g_last[7], lv[1]));                                                 /* 6i.151: 2x fast by default */
				press(0x04, 0, 0); frame(); release(); frame(); assert(strstr(g_last[7], lv[0]));
				for (k = 1; k < 5; k++) { press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[7], lv[k])); }
				for (k = 0; k < 3; k++) { press(0x04, 0, 0); frame(); release(); frame(); }
				assert(strstr(g_last[7], lv[0]));
				press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[7], lv[1]));   /* back to the default for the tests that follow */
			}
			{
				/* 6i.120: Reset to defaults - the last item; A puts every setting in the menu back, the menu left open. Coop
				 * Dynamic Lighting gone (Coop Render Passes does both), Scale Down Object Detail named Lower Shader Detail */
				void host_set_vsync(s32 on, s32 cap); void host_set_aniso(s32 on);
				int k, seen = 0, gone = 1;
				host_set_vsync(0, 5); host_set_aniso(3); frame();
				while (host_menu_selected() != 14) { press(0x02, 0, 0); frame(); release(); frame(); }   /* 6i.154: two items fewer; 6i.198: Toggle Triple Buffering gone to Game Hacks */
				for (k = 0; k < g_frame_lines; k++) { if (strstr(g_last[k], "> Reset to defaults")) seen |= 1; if (strstr(g_last[k], "Lower Shader Detail: off")) seen |= 2; if (strstr(g_last[k], "Coop Dynamic")) gone = 0; }
				assert(seen == 3 && gone);
				press(0, 255, 0); frame(); release(); frame();
				assert(host_menu_open() && host_menu() == 11);
				press(0x02, 0, 0); frame(); release(); frame();                     /* down from the last: the first */
				assert(host_menu_selected() == 0 && strstr(g_last[2], "VSync: on") && strstr(g_last[4], "FPS Limit (0 none): 0") && strstr(g_last[7], "Anisotropic Filtering: 2x fast"));
				/* 6i.176: VSync has three settings - off, on, 30 fps (right at the top stays) */
				press(0x02, 0, 0); frame(); release(); frame(); assert(host_menu_selected() == 1);
				press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[2], "> VSync: 30 fps"));
				press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[2], "> VSync: 30 fps"));
				press(0x04, 0, 0); frame(); release(); frame(); assert(strstr(g_last[2], "> VSync: on"));
				press(0x04, 0, 0); frame(); release(); frame(); assert(strstr(g_last[2], "> VSync: off"));
				press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[2], "> VSync: on"));
				press(0x01, 0, 0); frame(); release(); frame(); assert(host_menu_selected() == 0);
				while (host_menu_selected() != 2) { press(0x02, 0, 0); frame(); release(); frame(); }
			}
			{
				/* 6i.135: Toggle Triple Buffering (restarts) - shows this start's state; A asks "This will force the game to
				 * restart, are you sure?" with No selected; No goes back; Yes saves the other way and relaunches the game; the
				 * next start reads it back */
				extern s32 g_host_tb_file; extern u32 g_host_relaunch; s32 host_triple(void); void host_tb_reload(void);
				void host_set_menu_open(s32 open, s32 m);
				int k, seen = 0;
				press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 10);                 /* 6i.197: it is in Game Hacks now, above Larger Caches */
				while (host_menu_selected() != 4) { press(0x02, 0, 0); frame(); release(); frame(); }
				for (k = 0; k < g_frame_lines; k++) if (strstr(g_last[k], "> Toggle Triple Buffering (restarts)")) seen = strstr(g_last[k], "restarts):") ? 2 : 1;
				assert(seen == 1);                                                 /* 6i.136: a button - no on / off after it */
				press(0, 255, 0); frame(); release(); frame();
				assert(host_menu() == 13 && strstr(g_last[0], "This will force the game to restart, are you sure?") && strstr(g_last[1], "Triple Buffering is currently: on")
				       && strstr(g_last[2], "  Yes") && strstr(g_last[3], "> No"));                      /* 6i.137: the state under the question */
				press(0, 255, 0); frame(); release(); frame();                     /* No: back, nothing done */
				assert(host_menu() == 10 && g_host_relaunch == 0 && g_host_tb_file == -1);
				press(0, 255, 0); frame(); release(); frame();
				assert(host_menu() == 13 && strstr(g_last[3], "> No"));           /* asks on No again */
				press(0x01, 0, 0); frame(); release(); frame();
				assert(strstr(g_last[2], "> Yes"));
				press(0, 255, 0); frame(); release(); frame();
				{ void spooky_main_frame(void); spooky_main_frame(); }                /* the action runs before the game's update */
				assert(g_host_relaunch == 1 && g_host_tb_file == 0);               /* saved off, the game relaunched */
				host_tb_reload(); assert(host_triple() == 0);                      /* the next start: off */
				host_set_menu_open(1, 13); frame();                                  /* the state line follows: off now */
				{ int q, off = 0; for (q = 0; q < g_frame_lines; q++) if (strstr(g_last[q], "Triple Buffering is currently: off")) off = 1; assert(off); }
				g_host_tb_file = 1; host_tb_reload(); assert(host_triple() == 1); g_host_tb_file = -1;
				{
					/* 6i.197: the restart stays on the 360 too (6i.196 had taken it out there): saved and relaunched */
					void host_nv2a_reset(u32 v); u32 before = g_host_relaunch;
					host_nv2a_reset(0u);
					host_set_menu_open(1, 13); frame();
					while (!strstr(g_last[2], "> Yes")) { press(0x01, 0, 0); frame(); release(); frame(); }
					press(0, 255, 0); frame(); release(); frame();
					{ void spooky_main_frame(void); spooky_main_frame(); }
					assert(g_host_relaunch == before + 1 && g_host_tb_file == 0);
					host_nv2a_reset(0x00011C01u); g_host_tb_file = -1; host_tb_reload();
					host_set_menu_open(0, 0); frame();
				}
				host_set_menu_open(1, 10); frame();                                    /* (back where the tests that follow expect: Performance Mode Settings, from Game Hacks' third line) */
				while (host_menu_selected() != 2) { press(0x01, 0, 0); frame(); release(); frame(); }
				press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 11);
				host_set_menu_open(1, 11); frame();
				while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
			}
			press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 10 && host_menu_selected() == 2);
			press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 0);   /* up to the main menu */
			host_set_menu_open(1, 4); frame();
			printf("Game Hacks: HUD Widescreen Fix, Performance Mode, Performance Mode Settings (8 - Render Scale 100 - 50 %, no water or distortion settings, no status line; Anisotropic Filtering off / 2x fast / 2x / full); B back up\n");
		}
		press(0, 0, 255); frame(); release(); frame(); press(0, 0, 255); frame(); release(); frame();
		assert(!host_menu_open());
		printf("cheats: a submenu with Invulnerability, Disable AI, Infinite Ammo and Bottomless Clip\n");
	}
	/* 6h.22: Game Speed (tenths, 0.0 to 5.0, from 1.0) and Pause Game (off) under Cheats; right / left step 0.1 with the
	 * auto-repeat and stop at the ends, A wraps from 5.0 to 0.0; A toggles Pause Game */
	{
		int z;
		press(0x21, 0, 0); frame(); release(); frame();
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 4);                   /* 6i.40: in Cheats */
		while (host_menu_selected() != 5) { press(0x02, 0, 0); frame(); release(); frame(); }
		assert(strstr(g_last[6], "> Game Speed: 1.0") && host_value(5) == 10);
		press(0x08, 0, 0); frame(); release(); frame(); assert(host_value(5) == 11 && strstr(g_last[6], "Game Speed: 1.1"));
		press(0x04, 0, 0); frame(); release(); frame(); press(0x04, 0, 0); frame(); release(); frame();
		assert(host_value(5) == 9 && strstr(g_last[6], "Game Speed: 0.9"));
		press(0x04, 0, 0); for (z = 0; z < 60; z++) frame(); release(); frame();
		assert(host_value(5) == 0 && strstr(g_last[6], "Game Speed: 0.0"));      /* held: down to 0.0, stops */
		press(0x08, 0, 0); for (z = 0; z < 200; z++) frame(); release(); frame();
		assert(host_value(5) == 50 && strstr(g_last[6], "Game Speed: 5.0"));     /* held: up to 5.0, stops */
		press(0, 255, 0); frame(); release(); frame(); assert(host_value(5) == 0); /* A: wraps to 0.0 */
		for (z = 0; z < 10; z++) { press(0, 255, 0); frame(); release(); frame(); }
		assert(host_value(5) == 10 && strstr(g_last[6], "Game Speed: 1.0"));
		press(0x02, 0, 0); frame(); release(); frame();
		assert(host_menu_selected() == 6 && strstr(g_last[7], "> Pause Game: off") && host_value(6) == 0);
		press(0, 255, 0); frame(); release(); frame(); assert(host_value(6) == 1 && strstr(g_last[7], "Pause Game: on") && host_menu_open());
		press(0, 255, 0); frame(); release(); frame(); assert(host_value(6) == 0 && strstr(g_last[7], "Pause Game: off"));
		press(0, 0, 255); frame(); release(); frame(); press(0, 0, 255); frame(); release(); frame(); assert(!host_menu_open());
		printf("Game Speed: 1.0 by default, 0.0 to 5.0 in steps of 0.1 (held: repeats, stops at the ends; A wraps); Pause Game: off, A toggles\n");
	}
	/* 6h.12: Checkpoint Settings (from the top, under Cheats): Create / Revert Checkpoint, Disable natural checkpoints and
	 * Disable reverts (both off); A toggles; B back to the top, still on it */
	{
		press(0x21, 0, 0); frame(); release(); frame();
		while (host_menu_selected() != 1) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 9 && strstr(g_last[0], "Checkpoint Settings") && strstr(g_last[1], "Create Checkpoint (Back + Left)")
		       && strstr(g_last[2], "Revert Checkpoint (Back + Right)") && strstr(g_last[3], "Force Double Revert") && strstr(g_last[4], "Disable natural checkpoints: off")
		       && strstr(g_last[5], "Disable reverts: off") && g_frame_lines >= 7);   /* 6i.121: Force Double Revert */   /* (+ a message still up); 6i.36: no Back */
		while (host_menu_selected() != 3) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame(); assert(strstr(g_last[4], "Disable natural checkpoints: on"));
		press(0x02, 0, 0); frame(); release(); frame(); press(0, 255, 0); frame(); release(); frame(); assert(strstr(g_last[5], "Disable reverts: on"));
		press(0, 255, 0); frame(); release(); frame(); press(0x01, 0, 0); frame(); release(); frame(); press(0, 255, 0); frame(); release(); frame();   /* both off again */
		assert(strstr(g_last[4], "Disable natural checkpoints: off") && strstr(g_last[5], "Disable reverts: off"));
		press(0, 0, 255); frame(); release(); frame();
		assert(host_menu() == 0 && host_menu_selected() == 1);
		press(0, 0, 255); frame(); release(); frame(); assert(!host_menu_open());
		printf("Checkpoint Settings: Create / Revert Checkpoint, Force Double Revert, Disable natural checkpoints, Disable reverts (off); B back to the top\n");
	}
	/* the practice shortcut (menu closed): Back + D-pad Left asks for a checkpoint - a game with no body here, so it says so */
	{
		int z, ok = 0;
		void spooky_main_frame(void);
		press(0x0024, 0, 0); frame(); release();
		spooky_main_frame();                                                  /* the game's frame: the request carried out */
		frame();
		for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "Checkpoint: not while you are dead")) ok = 1;   /* 6i.165: a campaign game with no body - Create refused (Revert would go through) */
		assert(ok);
		printf("Back + D-pad Left (menu closed): Create Checkpoint asked for, and its result shown\n");
	}
	/* Black + D-pad Up / Down (menu closed): launch / teleport asked for (no player here: refused, and said so);
	 * LS click + D-pad Up no longer does anything */
	{
		int z, ok = 0;
		void spooky_main_frame(void);
		for (z = 0; z < 70; z++) { spooky_main_frame(); frame(); }           /* the last message gone */
		press(0x0041, 0, 0); frame(); release(); spooky_main_frame(); frame();   /* LS click + Up */
		for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "Launch / Teleport")) ok = 1;
		assert(!ok);
		press(0x0001, 0, 0); g_pad.analog[4] = 255; frame(); release(); spooky_main_frame(); frame();   /* Black + Up */
		for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "Launch / Teleport: no player unit")) ok = 1;
		assert(ok);
		ok = 0;
		for (z = 0; z < 70; z++) { spooky_main_frame(); frame(); }
		press(0x0002, 0, 0); g_pad.analog[4] = 255; frame(); release(); spooky_main_frame(); frame();   /* Black + Down */
		for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "Launch / Teleport: no player unit")) ok = 1;
		assert(ok);
		printf("Black + D-pad Up / Down (menu closed): launch / teleport asked for; LS click + Up does nothing\n");
	}
	/* Back + D-pad Down (menu closed): the third person camera on, and off again - said so each time */
	{
		int z, on = 0, off = 0;
		void spooky_main_frame(void);
		for (z = 0; z < 70; z++) { spooky_main_frame(); frame(); }
		press(0x0022, 0, 0); frame(); release(); spooky_main_frame(); frame();
		for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "Third person camera: on")) on = 1;
		for (z = 0; z < 70; z++) { spooky_main_frame(); frame(); }
		press(0x0022, 0, 0); frame(); release(); spooky_main_frame(); frame();
		for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "Third person camera: off")) off = 1;
		assert(on && off && !host_menu_open());
		printf("Back + D-pad Down (menu closed): the third person camera toggled on and off, said so each time\n");
	}
	{
		/* 6i.42: the FPS Counter - on: "FPS: N" the first line drawn (top-left), menu closed or open; a vblank a frame for
		 * a second: 60; two a frame: 30; off: not drawn */
		void host_set_fps_show(s32 on); u32 host_fps_shown(void); void spooky_main_frame(void); void host_set_menu_open(s32 open, s32 m);
		int f;
		release(); host_set_menu_open(0, 0); host_set_fps_show(1);
		for (f = 0; f < 130; f++) { g_vblank++; spooky_main_frame(); frame(); }
		assert(host_fps_shown() == 60 && strstr(g_last[0], "FPS: 60"));
		for (f = 0; f < 70; f++) { g_vblank++; spooky_main_frame(); frame(); }
		assert(host_fps_shown() == 60 && !strcmp(g_last[0], "FPS: 60"));   /* 6i.57: just the figure (the spinning readout gone) */
		for (f = 0; f < 70; f++) { g_vblank += 2; g_yaw_in -= 6.0 * 3.14159265358979 / 180.0; spooky_main_frame(); frame(); }
		assert(host_fps_shown() == 30 && !strcmp(g_last[0], "FPS: 30"));
		host_set_menu_open(1, 11); frame(); assert(strstr(g_last[0], "FPS: 30") && strstr(g_last[1], "Performance Mode Settings"));
		/* 6i.189: not on the game's main menu - gone there, back in a game */
		{ u32 was = g_state; int z, seen = 0; host_set_menu_open(0, 0); g_state = 3; frame(); for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "FPS:")) seen = 1; assert(!seen);
		  g_state = was; frame(); assert(strstr(g_last[0], "FPS: 30")); }
		host_set_menu_open(0, 0); host_set_fps_show(0); frame();
		{ int z; for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "FPS:")); }
		printf("FPS Counter: \"FPS: 60\" / \"FPS: 30\" top-left, menu open or closed, just the figure (6i.57); off: gone\n");
	}
	{
		/* 6i.44: Wireframe: off, right - on, right - xray (the top); 6i.120: in Other Tools, Tick Rate under it */
		void host_set_menu_open(s32 open, s32 m); void host_set_wire(s32 v);
		release(); host_set_menu_open(1, 6); frame();
		while (host_menu_selected() != 7) { press(0x02, 0, 0); frame(); release(); frame(); }
		assert(strstr(g_last[8], "> Wireframe: off"));
		press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[8], "> Wireframe: on"));
		press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[8], "> Wireframe: xray"));
		press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[8], "> Wireframe: xray"));
		host_set_wire(0);
		/* 6i.159: White held with the menu open - the menu not drawn and the pad the game's (all but White itself, the
		 * game's flashlight) for as long as it is held; let go: the menu back on the same item, the pad the menu's again,
		 * and a button still held at that moment not taken as a press */
		{
			s32 host_menu_peek(void); int z, sel;
			host_set_menu_open(1, 6); frame();
			while (host_menu_selected() != 3) { press(0x02, 0, 0); frame(); release(); frame(); }
			sel = host_menu_selected();
			assert(strstr(g_last[0], "Other Tools") && game_neutral());
			press(0x02, 255, 0); g_pad.analog[5] = 255; g_pad.lx = 20000; frame();          /* White + D-pad Down + A + the stick */
			assert(host_menu_open() && host_menu_peek() && host_menu() == 6 && host_menu_selected() == sel);   /* nothing of the menu moved or was chosen */
			assert(g_game.buttons == 0x02 && g_game.analog[0] == 255 && g_game.lx == 20000 && g_game.analog[5] == 0);   /* the game has the pad, without White */
			for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Other Tools") && !strstr(g_last[z], "Sleep Timer"));   /* the menu not drawn */
			for (z = 0; z < 40; z++) { frame(); assert(host_menu_peek() && host_menu_selected() == sel); }   /* held: no repeat reaches the menu */
			g_pad.analog[5] = 0; frame();                                                    /* White let go, Down and A still held */
			assert(host_menu_open() && !host_menu_peek() && host_menu() == 6 && host_menu_selected() == sel && game_neutral() && g_game.lx == 0);   /* back, on the same item; no press taken; the pad the menu's */
			assert(strstr(g_last[0], "Other Tools"));
			release(); frame(); press(0x02, 0, 0); frame(); release(); frame();
			assert(host_menu_selected() == sel + 1);                                         /* and it works as before */
			press(0, 0, 0); g_pad.analog[5] = 255; frame(); press(0x21, 0, 0); g_pad.analog[5] = 255; frame();   /* Back + Up while held aside: closed */
			assert(!host_menu_open() && !host_menu_peek());
			release(); frame(); frame();
			press(0, 0, 0); g_pad.analog[5] = 255; frame(); assert(!host_menu_peek() && g_game.analog[5] == 255);   /* menu closed: White is the game's (and the shortcuts') as ever */
			release(); frame();
		}
		/* 6i.163: (a) White held with the menu open while the free camera flies - the pad is the CAMERA's (the game gets a
		 * neutral pad), not the player's; let go, the menu is back. (b) Black + White, either pressed while the other is
		 * held, in free camera mode only: "Disable freecam player input" on / off, said; neither button reaches the game
		 * then; not in first person, not with Back held */
		{
			void host_set_free_cam(s32 on, s32 block); s32 host_fc_block(void); float host_fc_in(s32 k); s32 host_menu_peek(void); const char *host_toast(void); void host_clear_toast(void);
			int sel;
			host_set_free_cam(1, 1); host_set_menu_open(1, 6); frame();
			sel = host_menu_selected();
			press(0, 0, 0); g_pad.analog[5] = 255; g_pad.lx = 20000; g_pad.ly = 20000; frame();   /* White + the left stick */
			assert(host_menu_open() && host_menu_peek() && host_menu_selected() == sel);
			assert(game_neutral() && g_game.lx == 0 && g_game.ly == 0 && g_game.analog[5] == 0);  /* the game: nothing - you stay put */
			assert(host_fc_in(0) > 0.3f && host_fc_in(1) > 0.3f);                              /* the camera: the stick */
			host_set_free_cam(1, 0); frame();                                                  /* "Disable freecam player input" off: held aside, the pad moves you */
			assert(host_menu_peek() && g_game.lx == 20000 && g_game.analog[5] == 0);
			host_set_free_cam(1, 1);
			release(); frame(); assert(!host_menu_peek() && strstr(g_last[0], "Other Tools") && host_menu_selected() == sel);
			host_set_menu_open(0, 0); release(); frame(); frame(); host_clear_toast();
			press(0, 0, 0); g_pad.analog[5] = 255; frame(); assert(host_fc_block() == 1);          /* White alone: nothing */
			g_pad.analog[4] = 255; frame();                                                    /* ... then Black: off, said */
			assert(host_fc_block() == 0 && strstr(host_toast(), "Disable freecam player input: off") && g_game.analog[4] == 0 && g_game.analog[5] == 0);
			{ int z; for (z = 0; z < 31; z++) { frame(); assert(host_fc_block() == 0 && g_game.analog[4] == 0 && g_game.analog[5] == 0); } }   /* held, 31 more reads: once (6i.163 toggled on every read) */
			g_pad.analog[4] = 0; frame(); g_pad.analog[4] = 255; frame(); assert(host_fc_block() == 1);   /* Black let go and pressed again, White still held: a new press */
			g_pad.analog[4] = 0; frame(); g_pad.analog[4] = 255; frame(); assert(host_fc_block() == 0);
			release(); frame(); press(0, 0, 0); g_pad.analog[4] = 255; frame(); assert(host_fc_block() == 0);   /* Black alone: nothing */
			g_pad.analog[5] = 255; frame();                                                    /* ... then White: on again */
			assert(host_fc_block() == 1 && strstr(host_toast(), "Disable freecam player input: on"));
			release(); frame(); press(0x20, 0, 0); g_pad.analog[4] = 255; g_pad.analog[5] = 255; frame(); frame();   /* with Back: not this shortcut */
			release(); frame(); host_set_free_cam(1, 1);                                       /* (whatever Back's own shortcuts did, the camera back on for the next check) */
			host_set_free_cam(0, 1); press(0, 0, 0); g_pad.analog[4] = 255; g_pad.analog[5] = 255; frame();   /* first person: nothing, and both reach the game */
			assert(host_fc_block() == 1 && g_game.analog[4] == 255 && g_game.analog[5] == 255);
			release(); frame(); host_clear_toast();
		}
		/* 6i.165: Back + Black toggles the free camera in either order - Back held then Black, or Black held then Back -
		 * once a press however long both are held */
		{
			void host_set_free_cam(s32 on, s32 block); s32 host_free_cam(void); void spooky_main_frame(void); int z;
			host_set_menu_open(0, 0); release(); frame(); spooky_main_frame(); frame(); host_set_free_cam(0, 1);   /* (any request an earlier check left waiting: carried out first) */
			press(0, 0, 0); g_pad.analog[4] = 255; frame(); spooky_main_frame(); assert(host_free_cam() == 0);        /* Black alone: nothing */
			press(0x20, 0, 0); g_pad.analog[4] = 255; frame(); spooky_main_frame(); assert(host_free_cam() == 1);     /* ... then Back: on */
			for (z = 0; z < 20; z++) { frame(); spooky_main_frame(); assert(host_free_cam() == 1); }                  /* held: once */
			release(); frame(); spooky_main_frame();
			press(0x20, 0, 0); frame(); spooky_main_frame(); assert(host_free_cam() == 1);                            /* Back alone: nothing */
			g_pad.analog[4] = 255; frame(); spooky_main_frame(); assert(host_free_cam() == 0);                         /* ... then Black: off (as before) */
			for (z = 0; z < 20; z++) { frame(); spooky_main_frame(); assert(host_free_cam() == 0); }
			release(); frame(); spooky_main_frame(); host_set_free_cam(0, 1);
			{ void host_clear_toast(void); host_clear_toast(); }
		}
		/* 6i.167: a window of the game's up (the pause menu) with the menu open - the menu is not drawn in the HUD text
		 * pass (where the window would cover it) but after the game's windows, whole and on the same item, and still
		 * takes the pad; no window: drawn where it always was, and nothing more after */
		{
			void host_set_late(s32 on); void host_set_ui_window(s32 on); void spooky_after_windows(void);
			int z, sel, n_before;
			host_set_late(1); host_set_menu_open(1, 6); frame();
			sel = host_menu_selected(); n_before = g_frame_lines;
			assert(strstr(g_last[0], "Other Tools") && n_before == 12);
			spooky_after_windows(); assert(g_frame_lines == n_before);                       /* no window: nothing drawn late */
			host_set_ui_window(1); frame();
			for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Other Tools") && !strstr(g_last[z], "Sleep Timer"));   /* not in the HUD pass */
			z = g_frame_lines; spooky_after_windows();
			assert(g_frame_lines == z + n_before && strstr(g_last[z], "Other Tools") && strstr(g_last[z + 1], "Trigger Volume Overlay"));   /* after the windows: all of it */
			spooky_after_windows(); assert(g_frame_lines == z + n_before);                   /* once a frame */
			press(0x02, 0, 0); frame(); release(); frame(); spooky_after_windows();           /* the menu still works behind... in front of the pause menu */
			assert(host_menu_selected() == sel + 1 && game_neutral());
			host_set_ui_window(0); frame(); assert(strstr(g_last[0], "Other Tools"));         /* the window gone: where it always was */
			z = g_frame_lines; spooky_after_windows(); assert(g_frame_lines == z);
			host_set_late(0); host_set_ui_window(1); frame(); assert(strstr(g_last[0], "Other Tools"));   /* no late hook (v1.5): drawn in the HUD pass as before */
			host_set_ui_window(0); host_set_menu_open(0, 0); frame();
		}
		/* 6i.170: Back with a window of the game's up (the pause menu) - held, the game does not see it; with D-pad Up the
		 * menu opens and the game never saw Back or the D-pad; let go alone, the game gets a short press of it then
		 * (its own "back"); no window: Back reaches the game at once, as ever */
		{
			void host_set_ui_window(s32 on); void host_set_menu_open(s32 open, s32 m); int z;
			host_set_menu_open(0, 0); release(); frame(); frame();
			press(0x20, 0, 0); frame(); assert(g_game.buttons == 0x20);                         /* no window: at once */
			release(); frame(); assert(g_game.buttons == 0); frame(); assert(g_game.buttons == 0);   /* ... and no press made up afterwards */
			host_set_ui_window(1);
			press(0x20, 0, 0); for (z = 0; z < 10; z++) { frame(); assert(g_game.buttons == 0 && !host_menu_open()); }   /* held: kept from the game */
			press(0x21, 0, 0); frame(); assert(host_menu_open() && game_neutral());              /* then D-pad Up: the menu, the pause menu none the wiser */
			release(); frame(); frame(); frame(); assert(host_menu_open() && game_neutral());    /* let go: no press passed on (it was a combination) */
			host_set_menu_open(0, 0); release(); frame(); frame(); assert(game_neutral());
			press(0x20, 0, 0); for (z = 0; z < 5; z++) { frame(); assert(g_game.buttons == 0); }  /* Back alone, held ... */
			release(); frame(); assert(g_game.buttons == 0x20);                                  /* ... let go: the game's "back", now */
			frame(); assert(g_game.buttons == 0x20); frame(); assert(g_game.buttons == 0);       /* two reads long, then nothing */
			press(0x20, 0, 0); frame(); press(0x20, 255, 0); frame(); assert(g_game.buttons == 0 && g_game.analog[0] == 255);   /* Back held, then A: A goes through, Back does not ... */
			release(); frame(); frame(); assert(g_game.buttons == 0);                            /* ... and no press of it after */
			host_set_ui_window(0); release(); frame(); frame();
		}
		/* 6i.170: the Campaign Timer's line below Text Scale 1.0 - its box ends further left, by the line's length */
		{
			void host_set_ctimer(s32 on); void host_set_scale(s32 s); int z, at = -1;
			host_set_ctimer(1); frame();
			for (z = 0; z < g_frame_lines; z++) if (!strncmp(g_last[z], "Total Time: 00:00:00.00", 23)) at = z;
			assert(at >= 0 && g_last_left[at] == 640 - 9 - 420);                                  /* 1.0: where it was */
			host_set_scale(6); frame(); at = -1;
			for (z = 0; z < g_frame_lines; z++) if (!strncmp(g_last[z], "Total Time: 00:00:00.00", 23)) at = z;
			assert(at >= 0 && g_last_left[at] == 640 - 9 - 420 - 14);                             /* 0.6, 23 characters: 14 further left */
			host_set_scale(12); frame(); at = -1;
			for (z = 0; z < g_frame_lines; z++) if (!strncmp(g_last[z], "Total Time: 00:00:00.00", 23)) at = z;
			assert(at >= 0 && g_last_left[at] == 640 - 9 - 420);                                  /* above 1.0: where it was */
			host_set_scale(10); host_set_ctimer(0); frame();
		}
		/* 6i.160: the game's Reconnect Controller dialog up while the menu is open - the menu not drawn and the pad the
		 * game's so its "A to continue" gets through; the dialog gone: the menu back where it was */
		{
			void host_set_reconnect(s32 on); s32 host_menu_peek(void); int z, sel;
			host_set_menu_open(1, 6); frame();
			sel = host_menu_selected();
			press(0, 255, 0); host_set_reconnect(1); frame();                                /* A, with the dialog up */
			assert(host_menu_open() && host_menu_peek() && host_menu() == 6 && host_menu_selected() == sel && g_game.analog[0] == 255);   /* A is the game's; the menu chose nothing */
			for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Other Tools"));
			release(); frame(); press(0x02, 0, 0); frame();
			assert(g_game.buttons == 0x02 && host_menu_selected() == sel);                   /* the D-pad too: the game's, not the menu's */
			release(); host_set_reconnect(0); frame();                                       /* the dialog gone */
			assert(host_menu_open() && !host_menu_peek() && host_menu() == 6 && host_menu_selected() == sel && strstr(g_last[0], "Other Tools") && game_neutral());
			press(0x02, 0, 0); frame(); release(); frame(); assert(host_menu_selected() == (sel + 1) % 9);
			host_set_reconnect(1); frame(); host_set_menu_open(0, 0); frame(); host_set_reconnect(0);   /* menu closed: nothing of this applies */
			press(0, 255, 0); frame(); assert(g_game.analog[0] == 255); release(); frame();
		}
		/* 6i.158: Other Tools > Object Spawning, the last item - with no level's tags to read (this harness has none) the
		 * menu says so in one line; A on it goes back up, as B does */
		host_set_menu_open(1, 6); frame();
		while (host_menu_selected() != 9) { press(0x02, 0, 0); frame(); release(); frame(); }
		assert(strstr(g_last[10], "> Object Spawning  >"));
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 14 && strstr(g_last[0], "Object Spawning") && strstr(g_last[1], "> This level's tag names could not be read") && g_frame_lines == 3);
		press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 6 && host_menu_selected() == 9);
		press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 14);
		press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 6);
		/* 6i.124: Other Tools > Trigger Volume Overlay Settings: names, timer bars, flash, highlight - all on; B back up */
		host_set_menu_open(1, 6); frame();
		while (host_menu_selected() != 1) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 12 && strstr(g_last[0], "Trigger Volume Overlay Settings") && strstr(g_last[1], "Trigger names: on") && strstr(g_last[2], "Trigger timer bars: on")
		       && strstr(g_last[3], "Flash volumes on trigger: on") && strstr(g_last[4], "Highlight volumes when activated: on"));
		press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 6 && host_menu_selected() == 1);
		host_set_menu_open(1, 10); frame();                        /* 6i.121: Tick Rate in Game Hacks, under Performance Mode Settings */
		while (host_menu_selected() != 3) { press(0x02, 0, 0); frame(); release(); frame(); }
		assert(strstr(g_last[5 - 1], "> Tick Rate: 30") && strstr(g_last[3], "Performance Mode Settings  >"));
		press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[4], "> Tick Rate: 60"));
		press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[4], "> Tick Rate: 60"));
		press(0x04, 0, 0); frame(); release(); frame(); assert(strstr(g_last[4], "> Tick Rate: 30"));
		press(0x04, 0, 0); frame(); release(); frame(); assert(strstr(g_last[4], "> Tick Rate: 25"));
		press(0x04, 0, 0); frame(); release(); frame(); assert(strstr(g_last[4], "> Tick Rate: 25"));
		press(0x08, 0, 0); frame(); release(); frame(); assert(strstr(g_last[4], "> Tick Rate: 30"));
		host_set_menu_open(1, 2); frame();
		while (host_menu_selected() != 6) { press(0x02, 0, 0); frame(); release(); frame(); }
		assert(strstr(g_last[6], "Seams near you") && strstr(g_last[7], "> Passion Check"));   /* 6i.170: Superbounce Viewer Settings, under Seams near you */
		{ int z; host_set_menu_open(1, 4); frame(); for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Passion Check")); }   /* ... and gone from Cheats */
		host_set_menu_open(0, 0); release(); frame();
		printf("Wireframe: off / on / xray in Other Tools; Tick Rate 25 / 30 / 60 in Game Hacks; Passion Check in Superbounce Viewer Settings under Seams near you\n");
	}
	{
		/* 6i.46: FPS Limit shows the rate: 0, 10, 20, 25, 30, 40 ... 70, 75, 80 ... 120 - stepped by right */
		void host_set_menu_open(s32 open, s32 m);
		static const char *want[13] = { ": 0", ": 10", ": 20", ": 30", ": 40", ": 50", ": 60", ": 70", ": 80", ": 90", ": 100", ": 110", ": 120" };
		int k2;
		release(); host_set_menu_open(1, 11); frame();
		while (host_menu_selected() != 3) { press(0x02, 0, 0); frame(); release(); frame(); }
		for (k2 = 0; k2 < 13; k2++)
		{
			char line_want[64];
			snprintf(line_want, sizeof line_want, "> FPS Limit (0 none)%s", want[k2]);
			assert(strstr(g_last[4], line_want) && strlen(g_last[4]) == strlen(line_want));
			press(0x08, 0, 0); frame(); release(); frame();
		}
		while (host_menu_selected() == 3 && !strstr(g_last[4], "(0 none): 0")) { press(0x04, 0, 0); frame(); release(); frame(); }
		host_set_menu_open(0, 0); release(); frame();
		printf("FPS Limit: 0 to 120 by 10\n");
	}
	printf("ALL MENU CHECKS PASSED (%d lines drawn)\n", g_draws);
	return 0;
}
