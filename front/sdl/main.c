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
#include <string.h>
#include <assert.h>

#define VXT_LIBC
#define VXT_LIBC_ALLOCATOR
#define VXTU_LIBC_IO
#include <vxt/vxt.h>
#include <vxt/vxtu.h>
#include <vxtp.h>

#ifdef _WIN32
	#include <SDL.h>
#else
	#include <SDL2/SDL.h>
#endif

#include "main.h"
#include "keys.h"
#include "docopt.h"

struct video_adapter {
	struct vxt_pirepheral *device;
	vxt_dword (*border_color)(struct vxt_pirepheral *p);
	bool (*snapshot)(struct vxt_pirepheral *p);
	int (*render)(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata);
};

FILE *trace_op_output = NULL;
FILE *trace_offset_output = NULL;

#if defined(VXT_CPU_286)
	#define CPU_NAME "i286"
#elif defined(VXT_CPU_V20)
	#define CPU_NAME "V20"
#else
	#define CPU_NAME "8088"
#endif

#define SYNC(...) {						\
	while (SDL_LockMutex(emu_mutex));	\
	{ __VA_ARGS__ ; }					\
	SDL_UnlockMutex(emu_mutex); }		\

#ifdef PI8088
	extern struct vxt_validator *pi8088_validator(void);
#endif

Uint32 last_title_update = 0;
int num_cycles = 0;
double cpu_frequency = 4.772726;

SDL_atomic_t running;
SDL_mutex *emu_mutex = NULL;
SDL_Thread *emu_thread = NULL;

SDL_Texture *framebuffer = NULL;
SDL_Point framebuffer_size = {640, 200};

#define AUDIO_FREQUENCY 48000
#define AUDIO_LATENCY 10

SDL_AudioDeviceID audio_device = 0;

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

static int open_url(const char *url) {
	int ret = 0;
	char *buffer = SDL_malloc(strlen(url) + 32);
	if (!buffer)
		return -1;

	#if defined(_WIN32) || defined(__CYGWIN__)
		strcpy(buffer, "cmd /c start ");
	#elif defined(__APPLE__) && defined(__MACH__)
		strcpy(buffer, "open ");
	#else
		strcpy(buffer, "xdg-open ");
	#endif

	ret = system(strcat(buffer, url));
	SDL_free(buffer);
	return ret;
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
	fwrite(&addr, sizeof(vxt_pointer), 1, trace_offset_output);
	data = vxt_system_isr_flag(s) ? 1 : 0;
	fwrite(&data, 1, 1, trace_offset_output);
}

static int emu_loop(void *ptr) {
	vxt_system *vxt = (vxt_system*)ptr;
	Uint64 start = SDL_GetPerformanceCounter();

	while (SDL_AtomicGet(&running)) {
		struct vxt_step res;
		SYNC(
			res = vxt_system_step(vxt, 100);
			if (res.err != VXT_NO_ERROR) {
				if (res.err == VXT_USER_TERMINATION)
					SDL_AtomicSet(&running, 0);
				else
					printf("step error: %s", vxt_error_str(res.err));
			}
			num_cycles += res.cycles;
		);

		if (cpu_frequency > 0.0)
			while (((SDL_GetPerformanceCounter() - start) / (SDL_GetPerformanceFrequency() / (Uint64)(cpu_frequency * 1000000.0))) < (Uint64)res.cycles);
		start = SDL_GetPerformanceCounter();
	}
	return 0;
}

static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
	if ((framebuffer_size.x != width) || (framebuffer_size.y != height)) {
		SDL_DestroyTexture(framebuffer);
		framebuffer = SDL_CreateTexture(userdata, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
		if (!framebuffer)
			return -1;
		framebuffer_size = (SDL_Point){width, height};
	}
	return SDL_UpdateTexture(framebuffer, NULL, rgba, width * 4);
}

#ifdef VXTP_NETWORK
	static Uint32 network_callback(Uint32 interval, void *param) {
		SYNC(vxtp_network_poll(param));
		return interval;
	}
#endif

static Uint32 audio_callback(Uint32 interval, void *param) {
	int num_bytes = 0;
	SDL_AudioSpec *spec = (SDL_AudioSpec*)param;

	static vxt_byte buffer[48000 * 2]; // TODO: Resize this dynamically.
	assert((unsigned)(spec->channels * spec->channels) <= sizeof(buffer));

	SYNC(num_bytes = vxtp_ppi_write_audio(spec->userdata, buffer, spec->freq, spec->channels, spec->samples));

	if (num_bytes) {
		SDL_LockAudioDevice(audio_device);
		SDL_QueueAudio(audio_device, buffer, num_bytes);
		SDL_UnlockAudioDevice(audio_device);
	}
	return interval;
}

static void enable_speaker(bool b) {
	SDL_LockAudioDevice(audio_device);
	SDL_ClearQueuedAudio(audio_device);
	SDL_PauseAudioDevice(audio_device, b ? 0 : 1);
	SDL_UnlockAudioDevice(audio_device);
}

static vxt_word pow2(vxt_word v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	return ++v;
}

static struct vxt_pirepheral *load_bios(const char *path, vxt_pointer base) {
	int size = 0;
	vxt_byte *data = vxtu_read_file(&vxt_clib_malloc, path, &size);
	if (!data) {
		printf("vxtu_read_file() failed!\n");
		return NULL;
	}
	
	struct vxt_pirepheral *rom = vxtu_memory_create(&vxt_clib_malloc, base, size, true);
	if (!vxtu_memory_device_fill(rom, data, size)) {
		printf("vxtu_memory_device_fill() failed!\n");
		return NULL;
	}
	vxt_clib_malloc(data, 0);

	printf("Loaded BIOS @ 0x%X: %s\n", base, path);
	return rom;
}

int ENTRY(int argc, char *argv[]) {
	struct DocoptArgs args = docopt(argc, argv, true, vxt_lib_version());

	if (!args.bios) {
		args.bios = SDL_getenv("VXT_DEFAULT_BIOS_PATH");
		if (!args.bios) args.bios = "bios/pcxtbios.bin";
	}

	if (!args.extension) {
		args.extension = SDL_getenv("VXT_DEFAULT_VXTX_BIOS_PATH");
		if (!args.extension) args.extension = "bios/vxtx.bin";
	}

	if (!args.harddrive && !args.floppy) {
		args.harddrive = SDL_getenv("VXT_DEFAULT_HD_IMAGE");
		if (!args.harddrive) args.harddrive = "boot/freedos_hd.img";
	}

	if (args.manual) {
		const char *path = SDL_getenv("VXT_DEFAULT_MANUAL_INDEX");
		return open_url(path ? path : "tools/manual/index.md.html");
	}

	#ifdef VXTP_NETWORK
		if (args.list) {
			vxtp_network_list(NULL);
			return 0;
		}
	#endif

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

	args.debug |= args.halt;
	if (args.debug)
		printf("Internal debugger enabled!\n");

	cpu_frequency = strtod(args.frequency, NULL);
	if (cpu_frequency <= 0.0)
		printf("No CPU frequency lock!\n");
	else
		printf("CPU frequency: %.2f MHz\n", cpu_frequency);

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

	framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, framebuffer_size.x, framebuffer_size.y);
	if (!framebuffer) {
		printf("SDL_CreateTexture() failed with error %s\n", SDL_GetError());
		return -1;
	}
	
	if (SDL_RenderSetLogicalSize(renderer, 640, 480)) {
		printf("SDL_RenderSetLogicalSize() failed with error %s\n", SDL_GetError());
		return -1;
	}

	vxt_set_logger(&printf);
	vxt_set_breakpoint(&trigger_breakpoint);

	struct vxt_pirepheral *dbg = NULL;
	if (args.debug) {
		struct vxtu_debugger_interface dbgif = {&pdisasm, &getline, &printf};
		dbg = vxtu_debugger_create(&vxt_clib_malloc, &dbgif);
	}

	struct vxt_pirepheral *rom = load_bios(args.bios, 0xFE000);
	if (!rom) return -1;

	struct vxt_pirepheral *rom_ext = load_bios(args.extension, 0xE0000);
	if (!rom_ext) return -1;

	struct vxt_pirepheral *disk = vxtp_disk_create(&vxt_clib_malloc, NULL);
	struct vxt_pirepheral *pit = vxtp_pit_create(&vxt_clib_malloc, &ustimer);
	struct vxt_pirepheral *ppi = vxtp_ppi_create(&vxt_clib_malloc, pit, &enable_speaker, NULL);
	struct vxt_pirepheral *mouse = vxtp_mouse_create(&vxt_clib_malloc, 0x3F8, 4);

	int i = 2;
	struct vxt_pirepheral *devices[16] = {vxtu_memory_create(&vxt_clib_malloc, 0x0, 0x100000, false), rom};

	struct video_adapter video = {0};
	if (args.vga) {
		video.device = vxtp_vga_create(&vxt_clib_malloc, &ustimer);
		video.border_color = &vxtp_vga_border_color;
		video.snapshot = &vxtp_vga_snapshot;
		video.render = &vxtp_vga_render;

		vxtp_ppi_set_xt_switches(ppi, 0);
		struct vxt_pirepheral *rom = load_bios(args.vga, 0xC0000); // Tested with ET4000 VGA BIOS.
		if (!rom) return -1;

		devices[i++] = rom;
	} else {
		video.device = vxtp_cga_create(&vxt_clib_malloc, &ustimer);
		video.border_color = &vxtp_cga_border_color;
		video.snapshot = &vxtp_cga_snapshot;
		video.render = &vxtp_cga_render;
	}

	devices[i++] = rom_ext;
	devices[i++] = vxtp_pic_create(&vxt_clib_malloc);
	devices[i++] = vxtp_dma_create(&vxt_clib_malloc);
	//devices[i++] = vxtp_ioext_create(&vxt_clib_malloc);
	devices[i++] = pit;
	devices[i++] = mouse;
	devices[i++] = disk;
	devices[i++] = ppi;
	devices[i++] = video.device;

	SDL_TimerID network_timer = 0;
	if (args.network) {
		#ifdef VXTP_NETWORK
			struct vxt_pirepheral *net = vxtp_network_create(&vxt_clib_malloc, atoi(args.network));
			if (net) {
				devices[i++] = net;
				if (!(network_timer = SDL_AddTimer(10, &network_callback, net))) {
					printf("SDL_AddTimer failed!\n");
					return -1;
				}
			}
		#else
			printf("No network support in this build!\n");
			return -1;
		#endif
	}

	devices[i++] = dbg;
	devices[i] = NULL;

	vxt_system *vxt = vxt_system_create(&vxt_clib_malloc, devices);

	if (args.trace) {
		if (!(trace_op_output = fopen(args.trace, "wb"))) {
			printf("Could not open: %s\n", args.trace);
			return -1;
		}
		static char buffer[512] = {0};
		snprintf(buffer, sizeof(buffer), "%s.offset", args.trace);
		if (!(trace_offset_output = fopen(buffer, "wb"))) {
			printf("Could not open: %s\n", buffer);
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

	if (args.floppy) {
		FILE *fp = fopen(args.floppy, "rb+");
		if (fp && (vxtp_disk_mount(disk, 0, fp) == VXT_NO_ERROR))
			printf("Floppy image: %s\n", args.floppy);
	}

	if (args.harddrive) {
		FILE *fp = fopen(args.harddrive, "rb+");
		if (fp && (vxtp_disk_mount(disk, 128, fp) == VXT_NO_ERROR)) {
			printf("Harddrive image: %s\n", args.harddrive);
			if (args.hdboot || !args.floppy)
				vxtp_disk_set_boot_drive(disk, 128);
		}
	}

	vxt_system_reset(vxt);
	vxt_system_registers(vxt)->debug = (bool)args.halt;

	SDL_AtomicSet(&running, 1);
	if (!(emu_mutex = SDL_CreateMutex())) {
		printf("SDL_CreateMutex failed!\n");
		return -1;
	}

	if (!(emu_thread = SDL_CreateThread(&emu_loop, "emulator loop", vxt))) {
		printf("SDL_CreateThread failed!\n");
		return -1;
	}

	SDL_TimerID audio_timer = 0;
	if (args.mute) {
		printf("Audio is muted!\n");
	} else {
		SDL_AudioSpec desired = {.freq = AUDIO_FREQUENCY, .format = AUDIO_U8, .channels = 1, .samples = pow2((AUDIO_FREQUENCY / 1000) * AUDIO_LATENCY)};
		SDL_AudioSpec obtained = {0};
		audio_device = SDL_OpenAudioDevice(NULL, false, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
		SDL_PauseAudioDevice(audio_device, true);

		obtained.userdata = ppi;
		if (!(audio_timer = SDL_AddTimer(1000 / (obtained.freq / obtained.samples), &audio_callback, &obtained))) {
			printf("SDL_AddTimer failed!\n");
			return -1;
		}
	}

	while (SDL_AtomicGet(&running)) {
		for (SDL_Event e; SDL_PollEvent(&e);) {
			switch (e.type) {
				case SDL_QUIT:
					SDL_AtomicSet(&running, 0);
					break;
				case SDL_MOUSEMOTION:
					if (mouse && SDL_GetRelativeMouseMode()) {
						Uint32 state = SDL_GetMouseState(NULL, NULL);
						struct vxtp_mouse_event ev = {0, e.motion.xrel, e.motion.yrel};
						if (state & SDL_BUTTON_LMASK)
							ev.buttons |= VXTP_MOUSE_LEFT;
						if (state & SDL_BUTTON_RMASK)
							ev.buttons |= VXTP_MOUSE_RIGHT;
						SYNC(vxtp_mouse_push_event(mouse, &ev));
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (mouse) {
						if (e.button.button == SDL_BUTTON_MIDDLE) {
							SDL_SetRelativeMouseMode(false);
							break;
						}
						SDL_SetRelativeMouseMode(true);

						struct vxtp_mouse_event ev = {0};
						if (e.button.button == SDL_BUTTON_LEFT)
							ev.buttons |= VXTP_MOUSE_LEFT;
						if (e.button.button == SDL_BUTTON_RIGHT)
							ev.buttons |= VXTP_MOUSE_RIGHT;
						SYNC(vxtp_mouse_push_event(mouse, &ev));
					}
					break;
				case SDL_DROPFILE:
					if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Insert Floppy", "Mount floppy image in A: drive?", window)) {
						FILE *fp = fopen(e.drop.file, "rb+");
						if (!fp) {
							SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Insert Floppy", "Could not open floppy image file!", window);
						} else {
							vxt_error err = VXT_NO_ERROR;
							SYNC(err = vxtp_disk_mount(disk, 0, fp));
							if (err != VXT_NO_ERROR)
								SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Insert Floppy", "Could not mount floppy image!", window);
						}
					}
					break;
				case SDL_KEYDOWN:
					SYNC(vxtp_ppi_key_event(ppi, sdl_to_xt_scan(e.key.keysym.scancode), false));
					break;
				case SDL_KEYUP:
					//if ((e.key.keysym.sym == SDLK_RETURN) && (e.key.keysym.mod & KMOD_ALT)) {
					if (e.key.keysym.sym == SDLK_F11) {
						if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
							SDL_SetWindowFullscreen(window, 0);
							SDL_SetRelativeMouseMode(false);
						} else {
							SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
							SDL_SetRelativeMouseMode(true);
						}
						break;
					} else if (e.key.keysym.sym == SDLK_F12) {
						if (args.debug && (e.key.keysym.mod & KMOD_ALT)) {
							SDL_SetWindowFullscreen(window, 0);
							SDL_SetRelativeMouseMode(false);
							SYNC(vxtu_debugger_interrupt(dbg));					
						} else if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Eject Floppy", "Eject mounted floppy image in A: drive?", window)) {
							SYNC(vxtp_disk_unmount(disk, 0));
						}
						break;
					}
					SYNC(vxtp_ppi_key_event(ppi, sdl_to_xt_scan(e.key.keysym.scancode) | VXTP_KEY_UP_MASK, false));
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
				snprintf(buffer, sizeof(buffer), "VirtualXT - " CPU_NAME "@%d MHz", (int)mhz);
			else
				snprintf(buffer, sizeof(buffer), "VirtualXT - " CPU_NAME "@%.2f MHz", mhz);
			SDL_SetWindowTitle(window, buffer);
		}

		vxt_dword bg = 0;
		SYNC(
			bg = video.border_color(video.device);
			video.snapshot(video.device)
		);

		SDL_SetRenderDrawColor(renderer, bg & 0x0000FF, (bg & 0x00FF00) >> 8, (bg & 0xFF0000) >> 16, 0xFF);
		SDL_RenderClear(renderer);

		video.render(video.device, &render_callback, renderer);

		SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	SDL_WaitThread(emu_thread, NULL);
	SDL_DestroyMutex(emu_mutex);

	if (network_timer)
		SDL_RemoveTimer(network_timer);

	if (audio_timer) {
		SDL_RemoveTimer(audio_timer);
		SDL_CloseAudioDevice(audio_device);
	}

	vxt_system_destroy(vxt);

	if (trace_op_output)
		fclose(trace_op_output);
	if (trace_offset_output)
		fclose(trace_offset_output);

	SDL_DestroyTexture(framebuffer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
