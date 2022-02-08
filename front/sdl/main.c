// Copyright (c) 2019-2022 Andreas T Jonsson
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#define VXT_CLIB_ALLOCATOR
#define VXT_CLIB_IO
#include <vxt/vxt.h>
#include <vxt/utils.h>

#ifdef _WIN32
	#include <SDL.h>
#else
	#include <SDL2/SDL.h>
#endif

#include <microui.h>

#include "main.h"
#include "mu_renderer.h"

static const char button_map[256] = {
	[ SDL_BUTTON_LEFT   & 0xff ] =  MU_MOUSE_LEFT,
	[ SDL_BUTTON_RIGHT  & 0xff ] =  MU_MOUSE_RIGHT,
	[ SDL_BUTTON_MIDDLE & 0xff ] =  MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
	[ SDLK_LSHIFT       & 0xff ] = MU_KEY_SHIFT,
	[ SDLK_RSHIFT       & 0xff ] = MU_KEY_SHIFT,
	[ SDLK_LCTRL        & 0xff ] = MU_KEY_CTRL,
	[ SDLK_RCTRL        & 0xff ] = MU_KEY_CTRL,
	[ SDLK_LALT         & 0xff ] = MU_KEY_ALT,
	[ SDLK_RALT         & 0xff ] = MU_KEY_ALT,
	[ SDLK_RETURN       & 0xff ] = MU_KEY_RETURN,
	[ SDLK_BACKSPACE    & 0xff ] = MU_KEY_BACKSPACE,
};

static mu_Color style_colors[MU_COLOR_MAX] = {
    { 200, 200, 200, 255 }, /* MU_COLOR_TEXT */
    { 70,  70,  70,  255 }, /* MU_COLOR_BORDER */
    { 100, 100, 100, 255 }, /* MU_COLOR_WINDOWBG */
    { 50,  50,  50,  255 }, /* MU_COLOR_TITLEBG */
    { 220, 220, 220, 255 }, /* MU_COLOR_TITLETEXT */
    { 0,   0,   0,   0   }, /* MU_COLOR_PANELBG */
    { 75,  75,  75,  255 }, /* MU_COLOR_BUTTON */
    { 85,  85,  85,  255 }, /* MU_COLOR_BUTTONHOVER */
    { 115, 100, 100, 255 }, /* MU_COLOR_BUTTONFOCUS */
    { 50,  50,  50,  255 }, /* MU_COLOR_BASE */
    { 60,  60,  60,  255 }, /* MU_COLOR_BASEHOVER */
    { 80,  60,  60,  255 }, /* MU_COLOR_BASEFOCUS */
    { 65,  65,  65,  255 }, /* MU_COLOR_SCROLLBASE */
    { 50,  50,  50,  255 }  /* MU_COLOR_SCROLLTHUMB */
};

static void set_style(mu_Style *style) {
	for (int i = 0; i < MU_COLOR_MAX; i++)
		style->colors[i] = style_colors[i];
}

static int text_width(mu_Font font, const char *text, int len) {
	(void)font;
	if (len == -1)
		len = strlen(text);
	return r_get_text_width(text, len);
}

static int text_height(mu_Font font) {
	(void)font;
	return r_get_text_height();
}

static char logbuf[160*1024] = {0};
static bool logbuf_updated = false;

static char membuf[160*1024] = {0};
//static bool membuf_updated = false;

static int write_log(const char *fmt, ...) {
	va_list args;
    va_start(args, fmt);

	logbuf_updated = true;

    int ln = strlen(logbuf);
    if (sizeof(logbuf) - (ln+1) < 160) {
        memmove(logbuf, logbuf+160, sizeof(logbuf)-160);
    }
    int ret = vsnprintf(logbuf+ln, 160, fmt, args);

    va_end(args);
    return ret;
}

static void update_debug_memory_view(vxt_system *vxt, const char *memaddr) {
	bool valid = true;
	int seg = 0, offset = 0;

	if (sscanf(memaddr, "%X:%X", &seg, &offset) != 2)
		valid = false;

	if (seg < 0 || seg > 0xFFFF || offset < 0 || offset > 0xFFFF)
		valid = false;
	
	if (valid) {
		*membuf = 0;
		vxt_pointer start = (vxt_pointer)seg * 16 + (vxt_pointer)offset;
		for (vxt_pointer i = start, num = 0; i < start+256 && i < 0xFFFFF; i++, num++) {
			vxt_byte data = vxt_system_read_byte(vxt, i);
			snprintf(membuf + num * 3, sizeof(membuf) - num * 3 + 1, "%0*X%s", 2, data, ((num+1) % 16) ? " " : "\n");
		}
	}
}

struct reg_entry {
	const char *title;
	uintptr_t offset;
	char buffer[5];
	bool new_row;
};

#define REG_NAME_SPACE "   "
static struct vxt_registers reg_ui_base;
static struct reg_entry reg_ui_layout[] = {
	{REG_NAME_SPACE "CS:", (uintptr_t)&reg_ui_base.cs, {0}, true},
	{REG_NAME_SPACE "SS:", (uintptr_t)&reg_ui_base.ss, {0}, false},
	{REG_NAME_SPACE "DS:", (uintptr_t)&reg_ui_base.ds, {0}, false},
	{REG_NAME_SPACE "ES:", (uintptr_t)&reg_ui_base.es, {0}, false},
	{REG_NAME_SPACE "AX:", (uintptr_t)&reg_ui_base.ax, {0}, true},
	{REG_NAME_SPACE "BX:", (uintptr_t)&reg_ui_base.bx, {0}, false},
	{REG_NAME_SPACE "CX:", (uintptr_t)&reg_ui_base.cx, {0}, false},
	{REG_NAME_SPACE "DX:", (uintptr_t)&reg_ui_base.dx, {0}, false},
	{REG_NAME_SPACE "SP:", (uintptr_t)&reg_ui_base.sp, {0}, true},
	{REG_NAME_SPACE "BP:", (uintptr_t)&reg_ui_base.bp, {0}, false},
	{REG_NAME_SPACE "SI:", (uintptr_t)&reg_ui_base.si, {0}, false},
	{REG_NAME_SPACE "DI:", (uintptr_t)&reg_ui_base.di, {0}, false},
	{0}
};
#undef REG_NAME_SPACE

static void debug_window(mu_Context *ctx, vxt_system *vxt) {
	struct vxt_registers *regs = vxt_system_registers(vxt);

	if (mu_begin_window_ex(ctx, "Debugger", mu_rect(20, 550, 800, 400), MU_OPT_NOCLOSE)) {
		// Registers & Trace
		mu_layout_row(ctx, 2, (int[]){435, -1}, 90);

		mu_begin_panel_ex(ctx, "Registers", MU_OPT_NOSCROLL);
		mu_layout_row(ctx, 2, (int[]){270, 150}, -1);
		
		mu_begin_panel_ex(ctx, "register panel", MU_OPT_NOTITLE|MU_OPT_NOSCROLL|MU_OPT_NOFRAME);

		for (int i = 0;; i++) {
			struct reg_entry *reg = &reg_ui_layout[i];
			if (!reg->title)
				break;

			if (reg->new_row)
				mu_layout_row(ctx, 0, NULL, 0);
			
			mu_layout_width(ctx, 25);
			mu_text(ctx, reg->title);
			mu_layout_width(ctx, 32);

			vxt_word r = *(vxt_word*)(((uintptr_t)regs) + (reg->offset - (uintptr_t)&reg_ui_base));
			snprintf(reg->buffer, sizeof(reg->buffer), "%0*X", 4, r);

			mu_textbox_ex(ctx, reg->buffer, sizeof(reg->buffer), MU_OPT_ALIGNCENTER);
		}

		mu_end_panel(ctx);

		mu_begin_panel_ex(ctx, "flags panel", MU_OPT_NOTITLE|MU_OPT_NOSCROLL|MU_OPT_NOFRAME);
		mu_layout_set_next(ctx, mu_rect(50, -1, -1, -1), 1);
		mu_text(ctx, "Flags:");
		mu_layout_row(ctx, 0, NULL, 15);
		mu_layout_width(ctx, 12);

		#define FLAG_BTN(f, t) mu_push_id(ctx, t, sizeof(t)); if (mu_button(ctx, (regs->flags & f) ? t : "") == MU_RES_SUBMIT) regs->flags = (regs->flags & ~f) | (~regs->flags & f); mu_pop_id(ctx);
		FLAG_BTN(VXT_OVERFLOW, "O");
		FLAG_BTN(VXT_DIRECTION, "D");
		FLAG_BTN(VXT_INTERRUPT, "I");
		FLAG_BTN(VXT_TRAP, "T");
		FLAG_BTN(VXT_SIGN, "S");
		FLAG_BTN(VXT_ZERO, "Z");
		FLAG_BTN(VXT_AUXILIARY, "A");
		FLAG_BTN(VXT_PARITY, "P");
		FLAG_BTN(VXT_CARRY, "C");
		#undef FLAG_BTN

		mu_end_panel(ctx);

		mu_end_panel(ctx);

		mu_begin_panel_ex(ctx, "Trace", MU_OPT_NOSCROLL);
		mu_layout_row(ctx, 1, (int[]){100}, 0);
		static int trace = 0;
		mu_checkbox(ctx, "Enable", &trace);
		mu_button(ctx, "Dump");
		mu_end_panel(ctx);

		// Disassembly & Memory
		mu_layout_row(ctx, 2, (int[]){435, -1}, -25);

		mu_begin_panel_ex(ctx, "Disassembly", MU_OPT_NOSCROLL);

		static char disaddr[10];
		snprintf(disaddr, sizeof(disaddr), "%0*X:%0*X", 4, regs->cs, 4, regs->ip);

		mu_layout_set_next(ctx, mu_rect(15, -1, 50, -1), 1);
		mu_text(ctx, "Address:");
		mu_textbox_ex(ctx, disaddr, 10, MU_OPT_ALIGNCENTER);

		mu_layout_row(ctx, 1, (int[]){-1}, -1);
		mu_begin_panel_ex(ctx, "disassembly view", MU_OPT_NOTITLE);
		mu_layout_row(ctx, 1, (int[]){-1}, -1);
		mu_text(ctx, logbuf);
		mu_end_panel(ctx);

		mu_end_panel(ctx);

		mu_begin_panel_ex(ctx, "Memory", MU_OPT_NOSCROLL);

		static char memaddr[10] = {"0000:0000"};
		mu_layout_set_next(ctx, mu_rect(15, -1, 50, -1), 1);
		mu_text(ctx, "Address:");
		mu_textbox_ex(ctx, memaddr, 10, MU_OPT_ALIGNCENTER);

		mu_layout_row(ctx, 1, (int[]){-1}, -1);
		mu_begin_panel_ex(ctx, "memory view", MU_OPT_NOTITLE);
		mu_layout_row(ctx, 1, (int[]){-1}, -1);
		update_debug_memory_view(vxt, memaddr);
		mu_text(ctx, membuf);
		mu_end_panel(ctx);

		mu_end_panel(ctx);

		// Controls
		mu_layout_row(ctx, 0, NULL, 0);
		mu_layout_width(ctx, 80);
		mu_button(ctx, "Step");
		mu_button(ctx, "Continue");

		mu_end_window(ctx);
	}
}

static void log_window(mu_Context *ctx) {
	if (mu_begin_window_ex(ctx, "Log Output", mu_rect(850, 550, 400, 400), MU_OPT_NOCLOSE)) {
		mu_layout_row(ctx, 1, (int[]) { -1 }, -1);

		mu_begin_panel_ex(ctx, "log output panel", MU_OPT_NOTITLE);
		mu_Container *panel = mu_get_current_container(ctx);
		mu_layout_row(ctx, 1, (int[]) { -1 }, -1);
		mu_text(ctx, logbuf);
		mu_end_panel(ctx);

		if (logbuf_updated) {
			panel->scroll.y = panel->content_size.y;
			logbuf_updated = false;
		}

		mu_end_window(ctx);
	}
}

static void emu_window(mu_Context *ctx) {
	if (mu_begin_window_ex(ctx, "Emulator", mu_rect(20, 20, 640, 480), MU_OPT_NORESIZE|MU_OPT_NOCLOSE)) {
		mu_draw_icon(ctx, -1, mu_get_current_container(ctx)->body, mu_color(0, 0, 0, 255));
		mu_end_window(ctx);
	}
}

static void process_frame(mu_Context *ctx, vxt_system *vxt) {
	mu_begin(ctx);
	log_window(ctx);
	debug_window(ctx, vxt);
	emu_window(ctx);
	mu_end(ctx);
}

static int err_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = vfprintf(stderr, fmt, args);
	va_end(args);
	return ret;
}

static const char *getline() {
	static char buffer[1024] = {0};
	return fgets(buffer, sizeof(buffer), stdin);
}

int ENTRY(int argc, char *argv[]) {
	printf("Application arguments: ");
	for (int i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");

	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_SetHint(SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4, "1");
	SDL_Window *window = SDL_CreateWindow(
			"VirtualXT", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			1280, 960, SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_RESIZABLE
		);

	if (window == NULL) {
		SDL_Log("SDL_CreateWindow() failed with error %s\n", SDL_GetError());
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		SDL_Log("SDL_CreateRenderer() failed with error %s\n", SDL_GetError());
		return -1;
	}
	//SDL_RenderSetLogicalSize(renderer, 640, 480);

	r_init(renderer);

	mu_Context *ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	set_style(ctx->style);
	ctx->text_width = text_width;
	ctx->text_height = text_height;

	int size = 0;
	//vxt_byte *data = vxtu_read_file(&vxt_clib_malloc, "bios/pcxtbios.bin", &size);
	vxt_byte *data = vxtu_read_file(&vxt_clib_malloc, "tools/testdata/bitwise.bin", &size);
	if (!data) {
		write_log("vxtu_read_file() failed!\n");
		return -1;
	}
	
	struct vxt_pirepheral ram = vxtu_create_memory_device(&vxt_clib_malloc, 0x0, 0x100000, false);
	//struct vxt_pirepheral rom = vxtu_create_memory_device(&vxt_clib_malloc, 0xFE000, size, true);
	struct vxt_pirepheral rom = vxtu_create_memory_device(&vxt_clib_malloc, 0xF0000, size, true);

	struct vxtu_debugger_interface dbgif = {true, &getline, &printf};
	struct vxt_pirepheral dbg = vxtu_create_debugger_device(&vxt_clib_malloc, &dbgif);

	if (!vxtu_memory_device_fill(&rom, data, size)) {
		write_log("vxtu_memory_device_fill() failed!\n");
		return -1;
	}

	struct vxt_pirepheral *devices[] = {
		&ram, &rom,
		&dbg, // Must be the last device in list.
		NULL
	};

	vxt_set_logger(&err_printf);
	//vxt_set_logger(&write_log);
	vxt_system *vxt = vxt_system_create(&vxt_clib_malloc, devices);

	vxt_error err = vxt_system_initialize(vxt);
	if (err != VXT_NO_ERROR) {
		write_log("vxt_system_initialize() failed with error %s\n", vxt_error_str(err));
		return -1;
	}

	vxt_system_reset(vxt);

	// For running testdata.
	struct vxt_registers *r = vxt_system_registers(vxt);
	r->cs = 0xF000;
    r->ip = 0xFFF0;

	for (bool run = true; run;) {
		for (SDL_Event e; SDL_PollEvent(&e);) {
			switch (e.type) {
				case SDL_QUIT: run = false; break;
				case SDL_MOUSEMOTION: mu_input_mousemove(ctx, e.motion.x, e.motion.y); break;
				case SDL_MOUSEWHEEL: mu_input_scroll(ctx, 0, e.wheel.y * -30); break;
				case SDL_TEXTINPUT: mu_input_text(ctx, e.text.text); break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: {
					int b = button_map[e.button.button & 0xff];
					if (b && e.type == SDL_MOUSEBUTTONDOWN) { mu_input_mousedown(ctx, e.button.x, e.button.y, b); }
					if (b && e.type ==   SDL_MOUSEBUTTONUP) { mu_input_mouseup(ctx, e.button.x, e.button.y, b);   }
					break;
				}

				case SDL_KEYDOWN:
					write_log("SDL_KEYDOWN: %s\n", SDL_GetKeyName(e.key.keysym.sym));
				case SDL_KEYUP: {
					int c = key_map[e.key.keysym.sym & 0xff];
					if (c && e.type == SDL_KEYDOWN) { mu_input_keydown(ctx, c); }
					if (c && e.type ==   SDL_KEYUP) { mu_input_keyup(ctx, c);   }
					break;
				}
			}
		}

		process_frame(ctx, vxt);

		struct vxt_step res = vxt_system_step(vxt, 0);
		if (res.err != VXT_NO_ERROR) {
			if (res.err == VXT_USER_TERMINATION)
				run = false;
			else
				write_log("step error: %s", vxt_error_str(res.err));
		}

		r_clear(mu_color(0, 0, 0, 255));
		mu_Command *cmd = NULL;
		while (mu_next_command(ctx, &cmd)) {
			switch (cmd->type) {
				case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
				case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
				case MU_COMMAND_ICON:
					{
						if (cmd->icon.id == -1) {
							r_draw_rect(cmd->rect.rect, mu_color((int)((SDL_GetTicks() / 10) % 255), 0, 0, 255));
						} else {
							r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
						}
						break;
					}
				case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
			}
		}
		r_present();
	}

	vxt_system_destroy(vxt);
	r_destroy();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
