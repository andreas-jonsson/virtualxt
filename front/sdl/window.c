// Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <stdlib.h>
#include <stdbool.h>
#include <microui.h>

#include "window.h"

bool has_open_windows = false;

void help_window(mu_Context *ctx) {
	if (mu_begin_window_ex(ctx, "Help", mu_rect(120, 40, 400, 400), MU_OPT_CLOSED|MU_OPT_NORESIZE)) {
		has_open_windows = true;

		mu_layout_row(ctx, 1, (int[]){-1}, 80);
		mu_text(ctx,
			"VirtualXT is an IBM PC/XT emulator that runs on modern hardware and\n"
			"operating systems. It is designed to be simple and lightweight yet\n"
			"still capable enough to run a large library of old application and games.");

		const char *text_l =
			"<F11>\n"
			"<F12>\n"
			"<Alt+F12>\n"
			"<Ctrl+F12>\n"
			"<Drag & Drop>\n"
			"<Middle Mouse>";

		const char *text_r =
			"Toggle fullscreen\n"
			"Show this help screen\n"
			"Debug break, if started with '--debug'\n"
			"Eject floppy disk in A: drive\n"
			"Drop floppy image file on window to mount\n"
			"Release or capture mouse";

		mu_layout_row(ctx, 1, (int[]){-1}, -1);
		mu_begin_panel(ctx, "Controls");
		mu_layout_row(ctx, 3, (int[]){10, 120, -1}, -1);
		mu_text(ctx, "");
		mu_text(ctx, text_l);
		mu_text(ctx, text_r);
		mu_end_panel(ctx);

		mu_end_window(ctx);
	}
}
