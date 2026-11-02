/* Host test for spooky.c: scripted controller input, a recording stand-in for the game's text renderer. */
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef signed short s16; typedef signed int s32;
typedef struct { u32 packet; u16 buttons; u8 analog[8]; s16 lx, ly, rx, ry; } xinput_state;
typedef struct { s16 top, left, bottom, right; } rect16;
typedef struct { float a, r, g, b; } color4f;

u8 host_draw_string_globals[0x64];
void host_w32(u32 a, u32 v) { (void)a; (void)v; }
static u32 g_state = 3;                                  /* the game options' state: 3 the game's main menu, 1 a campaign game */
u32 host_mem32(u32 a)                                    /* no game memory: no player, so no HUD - but the game options: */
{                                                        /* the main menu (3: the hint line), or in a game (1: the menu opens) */
	if (a == 0x1D10) return 0x20000;
	if (a == 0x20008) return g_state;
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
	assert(*(s32 *)(host_draw_string_globals + 0x0c) == 0);       /* left-justified */
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

	/* 6h: on the game's main menu Back + Up doesn't open it - the game gets the pad; the hint line stays */
	press(0x20 | 0x01, 0, 0); frame();
	assert(!host_menu_open() && g_game.buttons == 0x21 && g_frame_lines == 1 && strstr(g_last[0], "Back + D-pad Up opens the menu in game."));
	assert(g_last_top[0] == 448 && g_last_bottom[0] <= 480 && g_last_left[0] == 9);   /* 6h.6: the info line's row; 6h.8: 9 px in */
	release(); frame();
	g_state = 1;                                                      /* in a game from here on */
	printf("main menu: Back + Up doesn't open the menu there; the game gets the pad; the hint line stays\n");

	/* 2. Back alone does nothing; the game sees it */
	press(0x20, 0, 0); frame();
	assert(!host_menu_open() && g_game.buttons == 0x20);
	/* Back + Up opens; the game sees a neutral pad from that read on */
	press(0x20 | 0x01, 0, 0); frame();
	assert(host_menu_open() && game_neutral());
	assert(g_frame_lines == 14 && strstr(g_last[0], "Spooky Ghost Menu - by Fortune090") && strstr(g_last[1], "> Cheats  >") && strstr(g_last[2], "Game Speed: 1.0") && strstr(g_last[3], "Pause Game: off") && strstr(g_last[4], "Checkpoint Settings  >") && strstr(g_last[5], "Launch / Teleport") && strstr(g_last[8], "Ghost Viewer: off") && strstr(g_last[9], "Ghost Viewer Settings") && strstr(g_last[10], "Camera Settings  >") && strstr(g_last[11], "Timers  >") && strstr(g_last[12], "Text scale: 1.0"));   /* 6h.22 */
	{ int z; for (z = 0; z < g_frame_lines; z++) assert(!strstr(g_last[z], "Passion Check") && !strstr(g_last[z], "Close menu")); }   /* 6h.22: to Cheats; gone */
	assert(g_last_color[1].b < 0.5f && g_last_color[2].b > 0.5f);   /* the selected line is yellow, the others grey */
	assert(!memcmp(host_draw_string_globals, marker, sizeof(marker)));
	printf("open: 14 lines (title, 12 items, footer), selected line highlighted, game sees a neutral pad\n");

	/* holding the combo doesn't toggle it again or move the selection */
	for (i = 0; i < 20; i++) frame();
	assert(host_menu_open() && host_menu_selected() == 0);

	/* 3. Down selects the next item; up wraps to the bottom */
	release(); frame(); press(0x02, 0, 0); frame(); release(); frame();
	assert(host_menu_selected() == 1);
	press(0x01, 0, 0); frame(); release(); frame(); press(0x01, 0, 0); frame(); release(); frame();
	assert(host_menu_selected() == 11);
	printf("navigation: down moves, up wraps from the top to the bottom\n");

	/* 4. A toggles "Superbounce Viewer" (item 5): select it, press A */
	while (host_menu_selected() != 5) { press(0x02, 0, 0); frame(); release(); frame(); }
	assert(host_value(5) == 0);                                        /* off by default */
	press(0, 255, 0); frame(); assert(host_value(5) == 1 && game_neutral());
	frame(); frame(); assert(host_value(5) == 1);                    /* holding A doesn't toggle again */
	release(); frame();
	printf("A toggles once per press; the game never sees A\n");

	/* 5. Text scale: holding right repeats after 12 reads, then every 3, and stops at the top (14) */
	while (host_menu_selected() != 11) { press(0x02, 0, 0); frame(); release(); frame(); }
	assert(host_menu_selected() == 11 && host_value(11) == 10);
	press(0x08, 0, 0); frame(); assert(host_value(11) == 11);
	for (i = 0; i < 11; i++) frame();
	assert(host_value(11) == 11);
	frame(); assert(host_value(11) == 12);
	frame(); frame(); assert(host_value(11) == 12); frame(); assert(host_value(11) == 13);
	for (i = 0; i < 30; i++) frame();
	assert(host_value(11) == 14);
	release(); frame();
	press(0x04, 0, 0); frame(); release(); frame(); assert(host_value(11) == 13);
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
	/* 6h: a game ending with the menu open (back on the game's main menu): closed at the next read, the game's menu gets
	 * the pad once everything is released, the hint line shows */
	press(0x21, 0, 0); frame(); release(); frame(); assert(host_menu_open());
	g_state = 3;
	press(0x02, 0, 0); frame();
	assert(!host_menu_open());
	release(); frame(); press(0x02, 0, 0); frame(); assert(g_game.buttons == 0x02);
	release(); frame(); assert(g_frame_lines == 1 && strstr(g_last[0], "Back + D-pad Up"));
	g_state = 1;
	printf("game over with the menu open: closed on the game's main menu, its pad back, the hint line shown\n");
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
	while (host_menu_selected() != 11) { press(0x02, 0, 0); frame(); release(); frame(); }
	assert(strstr(g_last[12], "> Text scale"));                       /* the last item now */
	press(0, 0, 255); frame(); assert(!host_menu_open());
	release(); frame();
	printf("Back + Up and B both close it; no \"Close menu\" item (the footer says B closes)\n");
	assert(logged("menu opened") >= 3 && logged("menu closed") >= 2);
	{ void spooky_main_frame(void); for (i = 0; i < 9000; i++) spooky_main_frame(); }
	assert(logged("Spooky Ghost step 6h.29 running") == 1);             /* at main-loop frame 30 */
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
	/* submenus (as Cartographer's tree nodes): A on "Launch / Teleport  >" enters it, B returns to the top, B closes;
	 * "Back" returns too */
	{
		int z, ok;
		press(0x21, 0, 0); frame(); release(); frame();
		assert(host_menu_open() && host_menu() == 0);
		while (host_menu_selected() != 4) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 1 && strstr(g_last[0], "Launch / Teleport") && strstr(g_last[1], "Launch (Black + Up)"));
		assert(g_frame_lines == 9);                                          /* title, 7 items, footer */
		for (ok = 0, z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "B: back")) ok = 1;
		assert(ok);
		press(0, 0, 255); frame(); release(); frame();                       /* B: back to the top, still open */
		assert(host_menu_open() && host_menu() == 0 && strstr(g_last[0], "Spooky Ghost Menu - by Fortune090"));
		while (host_menu_selected() != 6) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();                       /* Superbounce Viewer Settings */
		assert(host_menu() == 2 && strstr(g_last[1], "Maintain charge state") && strstr(g_last[2], "Force Charged state") && strstr(g_last[3], "Seam info text") && g_frame_lines == 12 && strstr(g_last[7], "Monster/Dreadnought Viewer: off") && strstr(g_last[8], "Monster/Dreadnought Viewer: 35+ only: off") && strstr(g_last[9], "Monster/Dreadnought range (units): Max"));   /* 6h.18 */
		{
			/* 6d.6: the range in steps of 10 - right at the top stays 2000, left 1990, 1980 */
			s32 host_pred_range(void); void host_set_pred_range(s32 r);
			while (host_menu_selected() != 8) { press(0x02, 0, 0); frame(); release(); frame(); }
			/* 6h.18: Max, then right 10, 20 ... 200 (stops there); left back down to Max (stops there) */
			int z;
			press(0x04, 0, 0); frame(); release(); frame(); assert(host_pred_range() == 0 && strstr(g_last[9], "range (units): Max"));
			press(0x08, 0, 0); frame(); release(); frame(); assert(host_pred_range() == 10 && strstr(g_last[9], "Monster/Dreadnought range (units): 10"));
			press(0x08, 0, 0); frame(); release(); frame(); assert(host_pred_range() == 20 && strstr(g_last[9], "range (units): 20"));
			for (z = 0; z < 25; z++) { press(0x08, 0, 0); frame(); release(); frame(); }
			assert(host_pred_range() == 200 && strstr(g_last[9], "range (units): 200"));
			for (z = 0; z < 25; z++) { press(0x04, 0, 0); frame(); release(); frame(); }
			assert(host_pred_range() == 0 && strstr(g_last[9], "range (units): Max"));
			host_set_pred_range(0);
		}
		while (host_menu_selected() != 9) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();                       /* its "Back" */
		assert(host_menu() == 0 && host_menu_open());
		/* 6h: Ghost Viewer Settings - the offset save state's three items on top; Rewind poses, Backstep hold, Max believed
		 * poses and Offset threshold gone */
		while (host_menu_selected() != 8) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 5 && strstr(g_last[0], "Ghost Viewer Settings") && strstr(g_last[1], "> Save offsets (Black + Left)")
		       && strstr(g_last[2], "Revert offsets (Black + Right)") && strstr(g_last[3], "Restore offsets after natural reverts: off")
		       && strstr(g_last[4], "Believed poses: on") && strstr(g_last[5], "Travel regions: on") && strstr(g_last[6], "Object believed poses: on")
		       && strstr(g_last[7], "Object travel regions: off") && strstr(g_last[8], "Back") && g_frame_lines == 10);
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
		printf("submenus: A enters Launch / Teleport and Superbounce Viewer Settings; B or Back returns; B at the top closes\n");
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
	/* step 6c.1: Camera Settings - the cameras first, then the third person camera's own settings */
	{
		void host_set_menu_open(s32 open, s32 m);
		host_set_menu_open(1, 3); frame();
		assert(strstr(g_last[0], "Camera Settings") && strstr(g_last[1], "Third person camera (Back + Down): off") && strstr(g_last[2], "Third Person Settings  >")
		       && strstr(g_last[3], "Free camera (Back + Black): off") && strstr(g_last[4], "Disable player movement: on") && strstr(g_last[5], "Lock camera pitch: on")
		       && strstr(g_last[6], "Teleport to camera (Back + White)") && strstr(g_last[7], "Back"));
		host_set_menu_open(0, 0); frame();
		/* Third Person Settings, within it: A on it enters; B and its Back go up to Camera Settings (still on that item),
		 * B there to the top, B at the top closes */
		release(); frame();
		press(0x20, 0, 0); press(0x21, 0, 0); frame(); release(); frame();                    /* Back + Up: open */
		assert(host_menu_open() && host_menu() == 0);
		while (host_menu_selected() != 9) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 3);              /* Camera Settings */
		press(0x02, 0, 0); frame(); release(); frame();
		press(0, 255, 0); frame(); release(); frame();                                         /* Third Person Settings */
		assert(host_menu() == 7 && strstr(g_last[0], "Third Person Settings") && strstr(g_last[1], "Distance") && strstr(g_last[2], "Height")
		       && strstr(g_last[3], "Side") && strstr(g_last[4], "Ignore Collision: on") && strstr(g_last[5], "Back"));   /* 6h.11: on by default */
		press(0, 0, 255); frame(); release(); frame();                                         /* B: up to Camera Settings */
		assert(host_menu() == 3 && host_menu_selected() == 1);
		press(0, 255, 0); frame(); release(); frame();                                         /* in again; its Back item */
		while (host_menu_selected() != 4) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame(); assert(host_menu() == 3);
		press(0, 0, 255); frame(); release(); frame(); assert(host_menu() == 0 && host_menu_selected() == 9);   /* B: the top */
		while (host_menu_selected() != 6) { press(0x01, 0, 0); frame(); release(); frame(); }   /* (left where the later tests expect) */
		press(0, 0, 255); frame(); release(); frame(); assert(!host_menu_open());
		printf("Camera Settings: third person, its settings (a submenu), free camera, disable player movement, lock camera pitch (on), teleport; B / Back go up one level\n");
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
		assert(host_menu_count() == 10);  /* 6e: + Skulls (within Cheats); 6h.12: + Checkpoint Settings */
		press(0x21, 0, 0); frame(); release(); frame();
		assert(host_menu_open() && host_menu() == 0);
		while (host_menu_selected() != 10) { press(0x02, 0, 0); frame(); release(); frame(); }
		sel2 = host_sel_of(2); first0 = host_first_of(0);
		press(0, 255, 0); frame(); release(); frame();                       /* A: into Timers */
		assert(host_menu() == 6 && strstr(g_last[0], "Timers") && strstr(g_last[1], "Out of bounds timer: off") && strstr(g_last[2], "Sleep timer: off") && strstr(g_last[3], "Back"));
		for (z = 0; z < 7; z++)
		{
			press(0x02, 0, 0); frame(); release(); frame();                   /* down, again and again */
			assert(host_menu() == 6 && host_menu_selected() == (z + 1) % 3);
			assert(host_sel_of(0) == 10 && host_first_of(0) == first0 && host_sel_of(2) == sel2);   /* nothing else moved */
		}
		press(0, 0, 255); frame(); release(); frame();                       /* B: back to the top, still on Timers */
		assert(host_menu() == 0 && host_menu_selected() == 10 && strstr(g_last[0], "Spooky Ghost Menu - by Fortune090"));
		while (host_menu_selected() != 6) { press(0x01, 0, 0); frame(); release(); frame(); }   /* left where the next test expects it */
		press(0, 0, 255); frame(); release(); frame();
		assert(!host_menu_open());
		printf("Timers menu: down moves only its own 3 items (7 times), the main menu's selection and scroll untouched\n");
	}
	/* scrolling: only 5 rows fit - the 10 items scroll with the selection, "more" lines marking each end that has more */
	{
		int z, sel_seen, above, below;
		host_set_rows_cap(5);
		press(0x21, 0, 0); frame(); release(); frame();                       /* reopens where it was left (item 6) */
		assert(g_frame_lines == 7 && host_menu_selected() == 6 && strstr(g_last[1], "more above") && strstr(g_last[4], "> Superbounce Viewer Settings"));
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
		assert(g_frame_lines == 7 && strstr(g_last[5], "more below") && !strstr(g_last[1], "more above"));   /* title, 5 rows, footer */
		for (z = 0; z < 11; z++)
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
			if (host_menu_selected() == 11) assert(above && !below && strstr(g_last[5], "> Text scale"));
			if (host_menu_selected() >= 4 && host_menu_selected() <= 7) assert(above && below);
		}
		press(0x02, 0, 0); frame(); release(); frame();                       /* wraps to the top */
		assert(host_menu_selected() == 0 && strstr(g_last[1], "> Cheats") && strstr(g_last[5], "more below"));
		press(0, 0, 255); frame(); release(); frame();
		host_set_rows_cap(0);
		printf("scrolling: 12 items in 5 rows - the selection always shown, \"more\" lines at each end with more\n");
	}
	/* the Cheats submenu: Invulnerability, Disable AI, Infinite Ammo, Bottomless Clip */
	{
		press(0x21, 0, 0); frame(); release(); frame();
		while (host_menu_selected() != 0) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 4 && strstr(g_last[0], "Cheats") && strstr(g_last[1], "Invulnerability: off") && strstr(g_last[2], "Infinite Ammo: off") &&
			strstr(g_last[3], "Bottomless Clip: off") && strstr(g_last[4], "Disable AI: off") && strstr(g_last[5], "Bump Possession: off") &&
			strstr(g_last[6], "Cycle Possession") && strstr(g_last[7], "Revert to player possession") && strstr(g_last[8], "Invincible Vehicles: off") &&
			strstr(g_last[9], "HUD Widescreen Fix: on") && strstr(g_last[10], "Passion Check") && strstr(g_last[11], "Skulls  >") && strstr(g_last[12], "Back"));   /* 6h.22: Passion Check back, above Skulls */
		{
			/* 6h.5: Cycle Possession and Revert to player possession leave the menu open; their message shows under it */
			void spooky_main_frame(void);
			int q, found = 0;
			while (host_menu_selected() != 5) { press(0x02, 0, 0); frame(); release(); frame(); }   /* item 5 (line 6) */
			press(0, 255, 0); frame(); release();
			spooky_main_frame(); frame();
			assert(host_menu_open() && host_menu() == 4);
			for (q = 0; q < g_frame_lines; q++) if (strstr(g_last[q], "Cycle Possession: no game in progress")) found = 1;
			assert(found);
			press(0x02, 0, 0); frame(); release(); frame();
			assert(host_menu_selected() == 6);
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
			while (host_menu_selected() != 10) { press(0x02, 0, 0); frame(); release(); frame(); }
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
		press(0, 0, 255); frame(); release(); frame(); press(0, 0, 255); frame(); release(); frame();
		assert(!host_menu_open());
		printf("cheats: a submenu with Invulnerability, Disable AI, Infinite Ammo and Bottomless Clip\n");
	}
	/* 6h.22: Game Speed (tenths, 0.0 to 5.0, from 1.0) and Pause Game (off) under Cheats; right / left step 0.1 with the
	 * auto-repeat and stop at the ends, A wraps from 5.0 to 0.0; A toggles Pause Game */
	{
		int z;
		press(0x21, 0, 0); frame(); release(); frame();
		while (host_menu_selected() != 1) { press(0x01, 0, 0); frame(); release(); frame(); }
		assert(strstr(g_last[2], "> Game Speed: 1.0") && host_value(1) == 10);
		press(0x08, 0, 0); frame(); release(); frame(); assert(host_value(1) == 11 && strstr(g_last[2], "Game Speed: 1.1"));
		press(0x04, 0, 0); frame(); release(); frame(); press(0x04, 0, 0); frame(); release(); frame();
		assert(host_value(1) == 9 && strstr(g_last[2], "Game Speed: 0.9"));
		press(0x04, 0, 0); for (z = 0; z < 60; z++) frame(); release(); frame();
		assert(host_value(1) == 0 && strstr(g_last[2], "Game Speed: 0.0"));      /* held: down to 0.0, stops */
		press(0x08, 0, 0); for (z = 0; z < 200; z++) frame(); release(); frame();
		assert(host_value(1) == 50 && strstr(g_last[2], "Game Speed: 5.0"));     /* held: up to 5.0, stops */
		press(0, 255, 0); frame(); release(); frame(); assert(host_value(1) == 0); /* A: wraps to 0.0 */
		for (z = 0; z < 10; z++) { press(0, 255, 0); frame(); release(); frame(); }
		assert(host_value(1) == 10 && strstr(g_last[2], "Game Speed: 1.0"));
		press(0x02, 0, 0); frame(); release(); frame();
		assert(host_menu_selected() == 2 && strstr(g_last[3], "> Pause Game: off") && host_value(2) == 0);
		press(0, 255, 0); frame(); release(); frame(); assert(host_value(2) == 1 && strstr(g_last[3], "Pause Game: on") && host_menu_open());
		press(0, 255, 0); frame(); release(); frame(); assert(host_value(2) == 0 && strstr(g_last[3], "Pause Game: off"));
		press(0, 0, 255); frame(); release(); frame(); assert(!host_menu_open());
		printf("Game Speed: 1.0 by default, 0.0 to 5.0 in steps of 0.1 (held: repeats, stops at the ends; A wraps); Pause Game: off, A toggles\n");
	}
	/* 6h.12: Checkpoint Settings (from the top, under Cheats): Create / Revert Checkpoint, Disable natural checkpoints and
	 * Disable reverts (both off); A toggles; B back to the top, still on it */
	{
		press(0x21, 0, 0); frame(); release(); frame();
		while (host_menu_selected() != 3) { press(0x01, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame();
		assert(host_menu() == 9 && strstr(g_last[0], "Checkpoint Settings") && strstr(g_last[1], "Create Checkpoint (Back + Left)")
		       && strstr(g_last[2], "Revert Checkpoint (Back + Right)") && strstr(g_last[3], "Disable natural checkpoints: off")
		       && strstr(g_last[4], "Disable reverts: off") && strstr(g_last[5], "Back") && g_frame_lines >= 7);   /* (+ a message still up) */
		while (host_menu_selected() != 2) { press(0x02, 0, 0); frame(); release(); frame(); }
		press(0, 255, 0); frame(); release(); frame(); assert(strstr(g_last[3], "Disable natural checkpoints: on"));
		press(0x02, 0, 0); frame(); release(); frame(); press(0, 255, 0); frame(); release(); frame(); assert(strstr(g_last[4], "Disable reverts: on"));
		press(0, 255, 0); frame(); release(); frame(); press(0x01, 0, 0); frame(); release(); frame(); press(0, 255, 0); frame(); release(); frame();   /* both off again */
		assert(strstr(g_last[3], "Disable natural checkpoints: off") && strstr(g_last[4], "Disable reverts: off"));
		press(0, 0, 255); frame(); release(); frame();
		assert(host_menu() == 0 && host_menu_selected() == 3);
		press(0, 0, 255); frame(); release(); frame(); assert(!host_menu_open());
		printf("Checkpoint Settings: Create / Revert Checkpoint, Disable natural checkpoints, Disable reverts (off); B back to the top\n");
	}
	/* the practice shortcut (menu closed): Back + D-pad Left asks for a checkpoint - no game here, so it says so */
	{
		int z, ok = 0;
		void spooky_main_frame(void);
		press(0x0024, 0, 0); frame(); release();
		spooky_main_frame();                                                  /* the game's frame: the request carried out */
		frame();
		for (z = 0; z < g_frame_lines; z++) if (strstr(g_last[z], "Checkpoints: no game in progress")) ok = 1;
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
	printf("ALL MENU CHECKS PASSED (%d lines drawn)\n", g_draws);
	return 0;
}
