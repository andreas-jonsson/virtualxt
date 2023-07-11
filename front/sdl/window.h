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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <stdbool.h>
#include <microui.h>

// This is used to prevent the emulator to
// capture the mouse when windows are open.
extern bool has_open_windows;

mu_Container *open_window(mu_Context *ctx, const char *name);
void open_error_window(mu_Context *ctx, const char *msg);

void help_window(mu_Context *ctx);
void error_window(mu_Context *ctx);
int config_window(mu_Context *ctx);
int eject_window(mu_Context *ctx, const char *path);
int mount_window(mu_Context *ctx, char *path);

#endif
