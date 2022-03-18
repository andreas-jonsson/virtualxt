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

#include <pirepheral.h>

#ifdef _WIN32
	#include <SDL.h>
#else
	#include <SDL2/SDL.h>
#endif

#include "main.h"
#include "font.h"
#include "keys.h"
#include "docopt.h"

//const char *bb_test = "tools/testdata/mul.bin";
const char *bb_test = NULL;
FILE *trace_op_output = NULL;

#define CPU_NAME "8088"

#define SET_WHITE(r) ( SDL_SetRenderDrawColor((r), 0xFF, 0xFF, 0xFF, 0xFF) )
#define SET_BLACK(r) ( SDL_SetRenderDrawColor((r), 0x0, 0x0, 0x0, 0xFF) )
#define SET_COLOR(r, c) ( c ? SET_WHITE((r)) : SET_BLACK((r)) )

#define SYNC(...) {						\
	while (SDL_LockMutex(emu_mutex));	\
	{ __VA_ARGS__ ; }					\
	SDL_UnlockMutex(emu_mutex); }		\

#ifdef PI8088
	extern struct vxt_validator *pi8088_validator();
#endif

Uint32 last_title_update = 0;
int num_cycles = 0;

SDL_atomic_t running;
SDL_mutex *emu_mutex = NULL;
SDL_Thread *emu_thread = NULL;

static void trigger_breakpoint(void) {
	SDL_TriggerBreakpoint();
}

long long ustimer(void) {
	return SDL_GetPerformanceCounter() / (SDL_GetPerformanceFrequency() / 1000000);
}

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

static bool pdisasm(vxt_system *s, vxt_pointer start, int size, int lines) {
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

	char *buffer = NULL;
	const char *cmd = "ndisasm -i -b 16 -o %d \"%s\" | head -%d";
	buffer = (char*)malloc(strlen(name) + strlen(cmd));
	sprintf(buffer, cmd, start, name, lines);

	bool ret = system(buffer) == 0;
	free(buffer);
	remove(name);
	return ret;
}

static void tracer(vxt_system *s, vxt_pointer addr, vxt_byte data) {
	(void)s; (void)addr;
	fwrite(&data, 1, 1, trace_op_output);
}

static int emu_loop(void *ptr) {
	vxt_system *vxt = (vxt_system*)ptr;
	while (SDL_AtomicGet(&running)) {
		struct vxt_step res;
		Uint64 start = SDL_GetPerformanceCounter();

		SYNC(
			res = vxt_system_step(vxt, 0);
			if (res.err != VXT_NO_ERROR) {
				if (res.err == VXT_USER_TERMINATION)
					SDL_AtomicSet(&running, 0);
				else
					printf("step error: %s", vxt_error_str(res.err));
			}
			num_cycles += res.cycles;
		);

		//const Uint64 freq = 4772726ul; // 4.77 Mhz
		const Uint64 freq = 0;
		if (freq > 0)
			while (((SDL_GetPerformanceCounter() - start) / (SDL_GetPerformanceFrequency() / freq)) < (Uint64)res.cycles);
	}
	return 0;
}

int mda_render(int offset, vxt_byte ch, enum vxtu_mda_attrib attrib, int cursor, void *userdata) {
	int x = offset % 80;
	int y = offset / 80;
	int num_pixels = 0;
	bool blink = ((SDL_GetTicks() / 500) % 2) != 0;
	SDL_Point pixels[64];
	
	if ((attrib & VXTU_MDA_BLINK) && blink)
		ch = ' ';

	for (int i = 0; i < 8; i++) {
		vxt_byte glyphLine = cga_font[ch * 8 + i];
		if (attrib & VXTU_MDA_INVERSE)
			glyphLine = ~glyphLine;
		
		for (int j = 0; j < 8; j++) {
			vxt_byte mask = 0x80 >> j;
			if (glyphLine & mask)
				pixels[num_pixels++] = (SDL_Point){x * 8 + j, y * 8 + i};
		}
	}

	// Draw character.
	int err = SDL_RenderDrawPoints((SDL_Renderer*)userdata, pixels, num_pixels);
	if (err) return err;

	// Render blinking CRT cursor and underlines.
	bool is_cursor = offset == cursor;
	if (is_cursor || (attrib & VXTU_MDA_UNDELINE)) {
		SET_COLOR((SDL_Renderer*)userdata, is_cursor && blink);
		for (int i = 0; i < 8; i++)
			pixels[i] = (SDL_Point){x * 8 + i, y * 8 + 7};
		err = SDL_RenderDrawPoints((SDL_Renderer*)userdata, pixels, 8);
		SET_WHITE((SDL_Renderer*)userdata);
	}
	return err;
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

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_SetHint(SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4, "1");
	SDL_Window *window = SDL_CreateWindow(
			"VirtualXT", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			640, 480, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE
		);

	if (!window) {
		printf("SDL_CreateWindow() failed with error %s\n", SDL_GetError());
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		printf("SDL_CreateRenderer() failed with error %s\n", SDL_GetError());
		return -1;
	}

	//if (SDL_RenderSetLogicalSize(renderer, 640, 200)) {
	//	printf("SDL_RenderSetLogicalSize() failed with error %s\n", SDL_GetError());
	//	return -1;
	//}

	vxt_set_logger(&printf);
	vxt_set_breakpoint(&trigger_breakpoint);

	struct vxt_pirepheral *dbg = NULL;
	if (args.debug) {
		struct vxtu_debugger_interface dbgif = {&pdisasm, &getline, &printf};
		dbg = vxtu_create_debugger(&vxt_clib_malloc, &dbgif);
	}

	int size = 0;
	vxt_byte *data = vxtu_read_file(&vxt_clib_malloc, bb_test ? bb_test : "bios/pcxtbios.bin", &size);
	if (!data) {
		printf("vxtu_read_file() failed!\n");
		return -1;
	}
	
	struct vxt_pirepheral *rom = vxtu_create_memory_device(&vxt_clib_malloc, bb_test ? 0xF0000 : 0xFE000, size, true);
	if (!vxtu_memory_device_fill(rom, data, size)) {
		printf("vxtu_memory_device_fill() failed!\n");
		return -1;
	}

	data = vxtu_read_file(&vxt_clib_malloc, "bios/vxtx.bin", &size);
	if (!data) {
		printf("vxtu_read_file() failed!\n");
		return -1;
	}
	
	struct vxt_pirepheral *rom_ext = vxtu_create_memory_device(&vxt_clib_malloc, 0xE0000, size, bb_test == NULL);
	if (!bb_test && !vxtu_memory_device_fill(rom_ext, data, size)) {
		printf("vxtu_memory_device_fill() failed!\n");
		return -1;
	}

	struct vxt_pirepheral *mda = vxtu_create_mda(&vxt_clib_malloc);
	struct vxt_pirepheral *ppi = vxtu_create_ppi(&vxt_clib_malloc);

	struct vxt_pirepheral *devices[16] = {vxtu_create_memory_device(&vxt_clib_malloc, 0x0, 0x100000, false), rom};

	const char *files[] = {
		"0*boot/freedos.img",
		NULL
	};
	
	int i = 2;
	if (!bb_test) {
		devices[i++] = rom_ext;
		devices[i++] = vxtu_create_pic(&vxt_clib_malloc);
		devices[i++] = vxtu_create_pit(&vxt_clib_malloc, &ustimer);
		devices[i++] = create_disk_controller(&vxt_clib_malloc, files);
		devices[i++] = ppi;
		devices[i++] = mda;
	} else {
		devices[i++] = vxtu_create_pic(&vxt_clib_malloc);
	}
	devices[i++] = dbg;
	devices[i] = NULL;

	vxt_system *vxt = vxt_system_create(&vxt_clib_malloc, devices);

	if (args.trace) {
		if (!(trace_op_output = fopen(args.trace, "wb"))) {
			printf("Could not open: %s\n", args.trace);
			return -1;
		}
		vxt_system_set_tracer(vxt, &tracer);
	}

	#ifdef PI8088
		vxt_system_set_validator(vxt, pi8088_validator());
	#endif

	vxt_error err = vxt_system_initialize(vxt);
	if (err != VXT_NO_ERROR) {
		printf("vxt_system_initialize() failed with error %s\n", vxt_error_str(err));
		return -1;
	}

	printf("Installed pirepherals:\n");
	for (int i = 1; i < VXT_MAX_PIREPHERALS; i++) {
		struct vxt_pirepheral *device = vxt_system_pirepheral(vxt, (vxt_byte)i);
		if (device)
			printf("%d - %s\n", i, vxt_pirepheral_name(device));
	}

	vxt_system_reset(vxt);
	vxt_system_registers(vxt)->debug = (bool)args.halt;

	if (bb_test) {
		struct vxt_registers *r = vxt_system_registers(vxt);
		r->cs = 0xF000;
    	r->ip = 0xFFF0;
	}

	SDL_AtomicSet(&running, 1);
	if (!(emu_mutex = SDL_CreateMutex())) {
		printf("SDL_CreateMutex failed!\n");
		return -1;
	}

	if (!(emu_thread = SDL_CreateThread(&emu_loop, "emulator loop", vxt))) {
		printf("SDL_CreateThread failed!\n");
		return -1;
	}

	while (SDL_AtomicGet(&running)) {
		for (SDL_Event e; SDL_PollEvent(&e);) {
			switch (e.type) {
				case SDL_QUIT:
					SDL_AtomicSet(&running, 0);
					break;
				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_MOUSEBUTTONUP:
					break;
				case SDL_KEYDOWN:
					if (args.debug && (e.key.keysym.sym == SDLK_F12) && (e.key.keysym.mod & KMOD_ALT))
						SYNC(vxtu_debugger_interrupt(dbg));
					for (bool success = false; !success;)
						SYNC(success = vxtu_ppi_key_event(ppi, sdl_to_xt_scan(e.key.keysym.scancode), false));
					break;
				case SDL_KEYUP:
					for (bool success = false; !success;)
						SYNC(success = vxtu_ppi_key_event(ppi, sdl_to_xt_scan(e.key.keysym.scancode) | VXTU_KEY_UP_MASK, false));
					break;
			}
		}

		Uint32 ticks = SDL_GetTicks();
		if ((ticks - last_title_update) > 1000) {
			last_title_update = ticks;

			char buffer[100];
			double mhz;

			SYNC(
				mhz = (double)num_cycles / 1000000.0;
				num_cycles = 0;
			);

			if (mhz > 10.0)
				snprintf(buffer, sizeof(buffer), "VirtualXT - " CPU_NAME "@%d Mhz", (int)mhz);
			else
				snprintf(buffer, sizeof(buffer), "VirtualXT - " CPU_NAME "@%.2f Mhz", mhz);
			SDL_SetWindowTitle(window, buffer);
		}

		SET_BLACK(renderer);
		SDL_RenderClear(renderer);
		SET_WHITE(renderer);

		SYNC(
			vxtu_mda_invalidate(mda);
			vxtu_mda_traverse(mda, &mda_render, renderer);
		);

		SDL_RenderPresent(renderer);
	}

	SDL_WaitThread(emu_thread, NULL);
	SDL_DestroyMutex(emu_mutex);

	vxt_system_destroy(vxt);
	if (trace_op_output)
		fclose(trace_op_output);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
