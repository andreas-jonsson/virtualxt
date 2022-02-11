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

#include "main.h"
#include "docopt.h"

static const char *getline() {
	static char buffer[1024] = {0};
	char *str = fgets(buffer, sizeof(buffer), stdin);
	for (char *p = str; *p; p++) {
		if (*p == '\n') {
			*p = 0;
			break;
		}
	}
	return str;
}

static bool pdisasm(vxt_system *s, vxt_pointer start, int size) {
	char *name = tmpnam(NULL);
	if (!name)
		return false;
	
	FILE *tmpf = fopen(name, "wb");
	if (!tmpf)
		return false;

	for (int i = 0; i < size; i++) {
		vxt_byte v = vxt_system_read_byte(s, start + i);
		if (fwrite(&v, 1, 1, tmpf) != 1) {
			fclose(tmpf);
			remove(name);
			return false;
		}
	}
	fclose(tmpf);

	const char *cmd = "ndisasm -i -b 16 -o %d \"%s\"";
	char *buffer = (char*)malloc(strlen(name) + strlen(cmd));
	sprintf(buffer, cmd, start, name);

	int ret = system(buffer) == 0;

	free(buffer);
	remove(name);
	return ret;
}

int ENTRY(int argc, char *argv[]) {
	struct DocoptArgs args = docopt(argc, argv, true, vxt_lib_version());
	args.debug |= args.halt;
	if (args.manual) {
		// TODO
		return 0;
	}

	if (!args.config) {
		args.config = SDL_GetPrefPath("virtualxt", "VirtualXT-SDL");
		if (!args.config) {
			printf("Could not initialize config!\n");
			return -1;		
		}
	}
	printf("Config path: %s\n", args.config);

	const char *base_path = SDL_GetBasePath();
	base_path = base_path ? base_path : "./";
	printf("Base path: %s\n", base_path);

	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_SetHint(SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4, "1");
	SDL_Window *window = SDL_CreateWindow(
			"VirtualXT", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			640, 480, SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_RESIZABLE
		);

	if (window == NULL) {
		printf("SDL_CreateWindow() failed with error %s\n", SDL_GetError());
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("SDL_CreateRenderer() failed with error %s\n", SDL_GetError());
		return -1;
	}
	SDL_RenderSetLogicalSize(renderer, 640, 200);

	int size = 0;
	//vxt_byte *data = vxtu_read_file(&vxt_clib_malloc, "bios/pcxtbios.bin", &size);
	vxt_byte *data = vxtu_read_file(&vxt_clib_malloc, "tools/testdata/bitwise.bin", &size);
	if (!data) {
		printf("vxtu_read_file() failed!\n");
		return -1;
	}
	
	struct vxt_pirepheral ram = vxtu_create_memory_device(&vxt_clib_malloc, 0x0, 0x100000, false);
	//struct vxt_pirepheral rom = vxtu_create_memory_device(&vxt_clib_malloc, 0xFE000, size, true);
	struct vxt_pirepheral rom = vxtu_create_memory_device(&vxt_clib_malloc, 0xF0000, size, true);

	struct vxtu_debugger_interface dbgif = {(bool)args.halt, &pdisasm, &getline, &printf};
	struct vxt_pirepheral dbg;
	if (args.debug)
		dbg = vxtu_create_debugger_device(&vxt_clib_malloc, &dbgif);

	if (!vxtu_memory_device_fill(&rom, data, size)) {
		printf("vxtu_memory_device_fill() failed!\n");
		return -1;
	}

	struct vxt_pirepheral *devices[] = {
		&ram, &rom,
		args.debug ? &dbg : NULL, // Must be the last device in list.
		NULL
	};

	vxt_set_logger(&printf);
	vxt_system *vxt = vxt_system_create(&vxt_clib_malloc, devices);

	vxt_error err = vxt_system_initialize(vxt);
	if (err != VXT_NO_ERROR) {
		printf("vxt_system_initialize() failed with error %s\n", vxt_error_str(err));
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
				case SDL_QUIT:
					run = false;
					break;
				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_MOUSEBUTTONUP:
					break;
				case SDL_KEYDOWN:
					if (args.debug && e.key.keysym.sym == SDLK_F12 && (e.key.keysym.mod & KMOD_ALT))
						vxtu_debugger_interrupt(&dbg);
					break;
				case SDL_KEYUP:
					break;
			}
		}

		struct vxt_step res = vxt_system_step(vxt, 0);
		if (res.err != VXT_NO_ERROR) {
			if (res.err == VXT_USER_TERMINATION)
				run = false;
			else
				printf("step error: %s", vxt_error_str(res.err));
		}

		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}

	vxt_system_destroy(vxt);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
