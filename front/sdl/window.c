// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <microui.h>

#include "window.h"
#include "mu_renderer.h"

bool has_open_windows = false;
char window_error_message[1024] = {0};

mu_Container *open_window(mu_Context *ctx, const char *name) {
	mu_Container *cont = mu_get_container(ctx, name);
	if (cont) {
		cont->open = 1;
		mu_bring_to_front(ctx, cont);
	}
	return cont;
}

void open_error_window(mu_Context *ctx, const char *msg) {
	if (open_window(ctx, "Error"))
		strncpy(window_error_message, msg, sizeof(window_error_message) - 1);
}

void help_window(mu_Context *ctx) {
	if (mu_begin_window_ex(ctx, "Help", mu_rect(120, 40, 400, 400), MU_OPT_CLOSED|MU_OPT_NORESIZE)) {
		has_open_windows = true;

		mu_layout_row(ctx, 1, (int[]){-1}, 80);
		mu_text(ctx,
			"VirtualXT is a Turbo PC/XT emulator that runs on modern hardware and\n"
			"operating systems. It is designed to be simple and lightweight yet\n"
			"still capable enough to run a large library of old application and games.");

		const char *text_l =
			"<F11>\n"
			"<Ctrl+F11>\n"
			"<F12>\n"
			"<Alt+F12>\n"
			"<Ctrl+F12>\n"
			"<Drag & Drop>\n"
			"<Middle Mouse>";

		const char *text_r =
			"Toggle fullscreen\n"
			"Eject floppy disk image\n"
			"Show this help screen\n"
			"Debug break, if GDB module is loaded\n"
			"Show debug monitors\n"
			"Drop floppy image file on window to mount\n"
			"Release or capture mouse";

		mu_layout_row(ctx, 1, (int[]){-1}, -30);
		mu_begin_panel(ctx, "Controls");
		mu_layout_row(ctx, 3, (int[]){10, 120, -1}, -1);
		mu_text(ctx, "");
		mu_text(ctx, text_l);
		mu_text(ctx, text_r);
		mu_end_panel(ctx);

		mu_layout_row(ctx, 1, (int[]){-1}, -1);
		mu_label(ctx, "Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>");

		mu_end_window(ctx);
	}
}

void error_window(mu_Context *ctx) {
	if (mu_begin_window_ex(ctx, "Error", mu_rect(120, 40, 400, 100), MU_OPT_CLOSED|MU_OPT_NORESIZE)) {
		has_open_windows = true;

		int text_width = mr_get_text_width(window_error_message, (int)strlen(window_error_message));
		mu_layout_set_next(ctx, mu_rect(200 - text_width / 2, 0, text_width + 10, 25), 1);
		mu_label(ctx, window_error_message);

		mu_layout_set_next(ctx, mu_rect(170, 35, 60, 25), 1);
		if (mu_button(ctx, "OK"))
			mu_get_current_container(ctx)->open = 0;

		mu_end_window(ctx);
	}
}

int eject_window(mu_Context *ctx, const char *path) {
	int eject = 0;
	char buf[512] = {0};

	if (mu_begin_window_ex(ctx, "Eject", mu_rect(120, 40, 400, 60), MU_OPT_CLOSED|MU_OPT_NORESIZE)) {
		has_open_windows = true;
		mu_layout_row(ctx, 3, (int[]){20, -100, -1}, 25);
	
		mu_label(ctx, "A:");
		
		strncpy(buf, path ? path : "EMPTY", sizeof(buf) - 1);
		mu_textbox_ex(ctx, buf, sizeof(buf), MU_OPT_NOINTERACT);

		if ((eject = mu_button_ex(ctx, "Eject", 0, MU_OPT_ALIGNCENTER | (path ? 0 : MU_OPT_NOINTERACT))))
			mu_get_current_container(ctx)->open = 0;

		mu_end_window(ctx);
	}
	return eject;
}

int mount_window(mu_Context *ctx, char *path) {
	int mount = 0;
	if (mu_begin_window_ex(ctx, "Mount", mu_rect(120, 40, 400, 60), MU_OPT_CLOSED|MU_OPT_NORESIZE)) {
		has_open_windows = true;
		mu_layout_row(ctx, 3, (int[]){20, -100, -1}, 25);
	
		mu_label(ctx, "A:");

		mu_textbox(ctx, path, FILENAME_MAX);

		if ((mount = mu_button_ex(ctx, "Mount", 0, MU_OPT_ALIGNCENTER | (path ? 0 : MU_OPT_NOINTERACT))))
			mu_get_current_container(ctx)->open = 0;

		mu_end_window(ctx);
	}
	return mount;
}

int config_window(mu_Context *ctx) {
	int open_cfg = 0;
	if (mu_begin_window_ex(ctx, "Config Update", mu_rect(120, 40, 340, 120), MU_OPT_CLOSED|MU_OPT_NORESIZE)) {
		has_open_windows = true;
	
		mu_layout_set_next(ctx, mu_rect(60, 5, 250, 25), 1);
		mu_label(ctx, "Do you want to inspect the config file?");

		mu_layout_set_next(ctx, mu_rect(80, 45, 60, 25), 1);
		if ((open_cfg = mu_button_ex(ctx, "Yes", 0, MU_OPT_ALIGNCENTER)))
			mu_get_current_container(ctx)->open = 0;

		mu_layout_set_next(ctx, mu_rect(180, 45, 60, 25), 1);
		if (mu_button_ex(ctx, "No", 0, MU_OPT_ALIGNCENTER))
			mu_get_current_container(ctx)->open = 0;

		mu_end_window(ctx);
	}
	return open_cfg;
}
