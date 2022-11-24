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

#include <ini.h>

#ifdef _WIN32
	#include <SDL.h>
#else
	#include <SDL2/SDL.h>
#endif

#include <microui.h>

#include "main.h"
#include "mu_renderer.h"
#include "window.h"
#include "keys.h"
#include "docopt.h"

struct video_adapter {
	struct vxt_pirepheral *device;
	vxt_dword (*border_color)(struct vxt_pirepheral *p);
	bool (*snapshot)(struct vxt_pirepheral *p);
	int (*render)(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata);
};

struct ini_config {
	struct DocoptArgs *args;
};
struct ini_config config = {0};

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

static const char button_map[256] = {
	[ SDL_BUTTON_LEFT   & 0xFF ] = MU_MOUSE_LEFT,
	[ SDL_BUTTON_RIGHT  & 0xFF ] = MU_MOUSE_RIGHT,
	[ SDL_BUTTON_MIDDLE & 0xFF ] = MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
	[ SDLK_LSHIFT       & 0xFF ] = MU_KEY_SHIFT,
	[ SDLK_RSHIFT       & 0xFF ] = MU_KEY_SHIFT,
	[ SDLK_LCTRL        & 0xFF ] = MU_KEY_CTRL,
	[ SDLK_RCTRL        & 0xFF ] = MU_KEY_CTRL,
	[ SDLK_LALT         & 0xFF ] = MU_KEY_ALT,
	[ SDLK_RALT         & 0xFF ] = MU_KEY_ALT,
	[ SDLK_RETURN       & 0xFF ] = MU_KEY_RETURN,
	[ SDLK_BACKSPACE    & 0xFF ] = MU_KEY_BACKSPACE,
};

Uint32 last_title_update = 0;
int num_cycles = 0;
double cpu_frequency = 4.772726;

SDL_atomic_t running;
SDL_mutex *emu_mutex = NULL;
SDL_Thread *emu_thread = NULL;

SDL_Texture *framebuffer = NULL;
SDL_Point framebuffer_size = {640, 200};

int str_buffer_len = 0;
char *str_buffer = NULL;

#define AUDIO_FREQUENCY 48000
#define AUDIO_LATENCY 10

SDL_AudioDeviceID audio_device = 0;
SDL_AudioSpec audio_spec = {0};

static void trigger_breakpoint(void) {
	SDL_TriggerBreakpoint();
}

static long long ustimer(void) {
	return SDL_GetPerformanceCounter() / (SDL_GetPerformanceFrequency() / 1000000);
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

static const char *sprint(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int size = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	if (str_buffer_len < size) {
		str_buffer_len = size;
		str_buffer = SDL_realloc(str_buffer, size);
	}

	va_start(args, fmt);
	vsnprintf(str_buffer, size, fmt, args);
	va_end(args);
	return str_buffer;
}

static const char *mgetline(void) {
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
	#ifdef __APPLE__
		int fh;
		char name[128] = {0};
		strncpy(name, "disasm.XXXXXX", sizeof(name) - 1);
		fh = mkstemp(name);
		if (fh == -1)
			return false;
		FILE *tmpf = fdopen(fh, "wb");
	#else
		char *name = tmpnam(NULL);
		if (!name)
			return false;
		FILE *tmpf = fopen(name, "wb");
	#endif

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

	bool ret = system(sprint("ndisasm -i -b 16 -o %d \"%s\" | head -%d", start, name, lines)) == 0;
	remove(name);
	return ret;
}

static void tracer(vxt_system *s, vxt_pointer addr, vxt_byte data) {
	(void)s; (void)addr;
	fwrite(&data, 1, 1, trace_op_output);
	fwrite(&addr, sizeof(vxt_pointer), 1, trace_offset_output);
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

		while (cpu_frequency > 0.0) {
			Uint64 f = SDL_GetPerformanceFrequency() / (Uint64)(cpu_frequency * 1000000.0);
			if (!f || (((SDL_GetPerformanceCounter() - start) / f) >= (Uint64)res.cycles))
				break;
		}
		start = SDL_GetPerformanceCounter();
	}
	return 0;
}

static SDL_Rect *target_rect(SDL_Window *window, SDL_Rect *rect) {
	const float crtAspect = 4.0f / 3.0f;
	SDL_Rect windowRect = {0};
	SDL_GetWindowSize(window, &windowRect.w, &windowRect.h);

	int targetWidth = (int)((float)windowRect.h * crtAspect);
	int targetHeight = windowRect.h;

	if (((float)windowRect.w / (float)windowRect.h) < crtAspect) {
		targetWidth = windowRect.w;
		targetHeight = (int)((float)windowRect.w / crtAspect);
	}

	*rect = (SDL_Rect){windowRect.w / 2 - targetWidth / 2, windowRect.h / 2 - targetHeight / 2, targetWidth, targetHeight};
	return rect;
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

static void audio_callback(void *udata, uint8_t *stream, int len) {
	struct vxt_pirepheral **audio_sources = (struct vxt_pirepheral**)udata;
	len /= 2;
	
	SYNC(
		for (int i = 0; i < len; i++) {
			vxt_word sample = vxtp_ppi_generate_sample(audio_sources[0], audio_spec.freq);
			if (audio_sources[1])
				sample += vxtp_adlib_generate_sample(audio_sources[1], audio_spec.freq);

			((vxt_int16*)stream)[i] = sample;
			if (audio_spec.channels > 1)
				((vxt_int16*)stream)[++i] = sample;
		}
	);
}

static vxt_word pow2(vxt_word v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	return ++v;
}

static int read_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s;
	return (int)fread(buffer, 1, (size_t)size, (FILE*)fp);
}

static int write_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s;
	return (int)fwrite(buffer, 1, (size_t)size, (FILE*)fp);
}

static int seek_file(vxt_system *s, void *fp, int offset, enum vxtp_disk_seek whence) {
	(void)s;
	switch (whence) {
		case VXTP_SEEK_START: return (int)fseek((FILE*)fp, (long)offset, SEEK_SET);
		case VXTP_SEEK_CURRENT: return (int)fseek((FILE*)fp, (long)offset, SEEK_CUR);
		case VXTP_SEEK_END: return (int)fseek((FILE*)fp, (long)offset, SEEK_END);
		default: return -1;
	}
}

static int tell_file(vxt_system *s, void *fp) {
	(void)s;
	return (int)ftell((FILE*)fp);
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

	printf("Loaded BIOS @ 0x%X-0x%X: %s\n", base, base + size - 1, path);
	return rom;
}

static int load_config(void *user, const char *section, const char *name, const char *value) {
	(void)user; (void)section; (void)name; (void)value;
	struct ini_config *config = (struct ini_config*)user;
	if (!strcmp("debug", section)) {
		if (!strcmp("debugger", name))
			config->args->debug = atoi(value);
		else if (!strcmp("halt", name))
			config->args->halt = atoi(value);
	} else if (!strcmp("rifs", section)) {
		if (!strcmp("detatch", name) && (atoi(value) != 0))
			config->args->rifs = NULL;
	}
	return 0;
}

int ENTRY(int argc, char *argv[]) {
	// This is a hack because there seems to be a bug in DocOpt
	// that prevents us from adding trailing parameters.
	char *rifs_path = NULL;
	if ((argc == 2) && (*argv[1] != '-')) {
		rifs_path = argv[1];
		argc = 1;
	}

	struct DocoptArgs args = docopt(argc, argv, true, vxt_lib_version());
	args.rifs = rifs_path ? rifs_path : args.rifs;

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
			printf("No config path!\n");
			return -1;
		}
	}
	printf("Config path: %s\n", args.config);

	const char *base_path = SDL_GetBasePath();
	base_path = base_path ? base_path : "./";
	printf("Base path: %s\n", base_path);

	{
		const char *path = sprint("%s/config.ini", args.config);
		FILE *fp = fopen(path, "a");
		if (fp) fclose(fp);

		config.args = &args;
		if (ini_parse(path, &load_config, &config) < 0) {
			printf("Can't open: %s\n", path);
			return -1;
		}
	}

	if (!args.bios) {
		args.bios = SDL_getenv("VXT_DEFAULT_BIOS_PATH");
		if (!args.bios) {
			#ifdef VXT_CPU_286
				args.bios = "bios/vxt286.bin";
			#else
				args.bios = "bios/pcxtbios.bin";
			#endif
		}
	}

	if (!args.extension) {
		args.extension = SDL_getenv("VXT_DEFAULT_VXTX_BIOS_PATH");
		if (!args.extension) args.extension = "bios/vxtx.bin";
	}

	if (!args.harddrive && !args.floppy) {
		args.harddrive = SDL_getenv("VXT_DEFAULT_HD_IMAGE");
		if (!args.harddrive) args.harddrive = "boot/freedos_hd.img";
	}
	
	if (!args.rifs)
		args.rifs = ".";

	args.debug |= args.halt;
	if (args.debug)
		printf("Internal debugger enabled!\n");

	cpu_frequency = strtod(args.frequency, NULL);
	if (cpu_frequency <= 0.0)
		printf("No CPU frequency lock!\n");
	else
		printf("CPU frequency: %.2f MHz\n", cpu_frequency);

	#if !defined(_WIN32) && !defined(__APPLE__)
		SDL_setenv("SDL_VIDEODRIVER", "x11", 1);
	#endif

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

	r_init(renderer);

	mu_Context *ctx = SDL_malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
	ctx->text_height = text_height;

	int num_sticks = 0;
	SDL_Joystick *sticks[2] = {NULL};

	if (args.joystick) {
		printf("Initialize joysticks:\n");
		if (!(num_sticks = SDL_NumJoysticks())) {
			printf("No joystick found!\n");
		} else {
			for (int i = 0; i < num_sticks; i++) {
				const char *name = SDL_JoystickNameForIndex(i);
				if (!name) name = "Unknown Joystick";

				if ((i < 2) && (sticks[i] = SDL_JoystickOpen(i))) {
					printf("%d - *%s\n", i + 1, name);
					continue;
				}
				printf("%d - %s\n", i + 1, name);
			}
		}
	}

	vxt_set_logger(&printf);
	vxt_set_breakpoint(&trigger_breakpoint);

	struct vxt_pirepheral *dbg = NULL;
	if (args.debug) {
		struct vxtu_debugger_interface dbgif = {&pdisasm, &mgetline, &printf};
		dbg = vxtu_debugger_create(&vxt_clib_malloc, &dbgif);
	}

	struct vxt_pirepheral *rom = load_bios(args.bios, 0xFE000);
	if (!rom) return -1;

	struct vxt_pirepheral *rom_ext = load_bios(args.extension, 0xE0000);
	if (!rom_ext) return -1;

	struct vxtp_disk_interface interface = {
		&read_file, &write_file, &seek_file, &tell_file
	};

	struct vxt_pirepheral *disk = vxtp_disk_create(&vxt_clib_malloc, &interface);
	struct vxt_pirepheral *fdc = vxtp_fdc_create(&vxt_clib_malloc, 0x3F0, 6);
	struct vxt_pirepheral *pit = vxtp_pit_create(&vxt_clib_malloc, &ustimer);
	struct vxt_pirepheral *ppi = vxtp_ppi_create(&vxt_clib_malloc, pit);
	struct vxt_pirepheral *mouse = vxtp_mouse_create(&vxt_clib_malloc, 0x3F8, 4); // COM1
	struct vxt_pirepheral *adlib = args.no_adlib ? NULL : vxtp_adlib_create(&vxt_clib_malloc);
	struct vxt_pirepheral *joystick = NULL;

	int i = 2;
	struct vxt_pirepheral *devices[32] = {vxtu_memory_create(&vxt_clib_malloc, 0x0, 0x100000, false), rom};

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

	if (num_sticks)
		devices[i++] = joystick = vxtp_joystick_create(&vxt_clib_malloc, ustimer, sticks[0], sticks[1]);

	if (!args.no_adlib)
		devices[i++] = adlib;

	if (args.fdc) {
		devices[i++] = fdc;
	} else {
		devices[i++] = disk;
		devices[i++] = rom_ext;
	}

	if (args.rifs) {
		bool ro = *args.rifs != '*';
		const char *root = ro ? args.rifs : &args.rifs[1];
		devices[i++] = vxtp_rifs_create(&vxt_clib_malloc, 0x178, root, ro);
	}

	devices[i++] = vxtp_pic_create(&vxt_clib_malloc);
	devices[i++] = vxtp_dma_create(&vxt_clib_malloc);
	devices[i++] = vxtp_rtc_create(&vxt_clib_malloc);
	devices[i++] = pit;
	devices[i++] = mouse;
	devices[i++] = ppi;
	devices[i++] = video.device;

	#ifdef VXT_CPU_286
		devices[i++] = vxtp_postcard_create(&vxt_clib_malloc);
	#endif

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
		vxt_error (*mnt)(struct vxt_pirepheral*,int,void*) = args.fdc ? vxtp_fdc_mount : vxtp_disk_mount;
		if (fp && (mnt(args.fdc ? fdc : disk, 0, fp) == VXT_NO_ERROR))
			printf("Floppy image: %s\n", args.floppy);
	}

	if (args.harddrive && !args.fdc) {
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

	struct vxt_pirepheral *audio_sources[2] = {ppi, adlib};

	if (args.mute) {
		printf("Audio is muted!\n");
	} else {
		SDL_AudioSpec spec = {0};
		spec.freq = AUDIO_FREQUENCY;
		spec.format = AUDIO_S16;
		spec.channels = 1;
		spec.samples = pow2((AUDIO_FREQUENCY / 1000) * AUDIO_LATENCY);
		spec.userdata = audio_sources;
		spec.callback = &audio_callback;

		audio_device = SDL_OpenAudioDevice(NULL, false, &spec, &audio_spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
		SDL_PauseAudioDevice(audio_device, 0);
	}

	while (SDL_AtomicGet(&running)) {
		for (SDL_Event e; SDL_PollEvent(&e);) {
			if (has_open_windows) {
				switch (e.type) {
					case SDL_MOUSEMOTION: mu_input_mousemove(ctx, e.motion.x, e.motion.y); break;
					case SDL_MOUSEWHEEL: mu_input_scroll(ctx, 0, e.wheel.y * -30); break;
					case SDL_TEXTINPUT: mu_input_text(ctx, e.text.text); break;
					case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: {
						int b = button_map[e.button.button & 0xFF];
						if (b && e.type == SDL_MOUSEBUTTONDOWN) { mu_input_mousedown(ctx, e.button.x, e.button.y, b); }
						if (b && e.type == SDL_MOUSEBUTTONUP) { mu_input_mouseup(ctx, e.button.x, e.button.y, b); }
						break;
					}
					case SDL_KEYDOWN: case SDL_KEYUP: {
						int c = key_map[e.key.keysym.sym & 0xFF];
						if (c && e.type == SDL_KEYDOWN) { mu_input_keydown(ctx, c); }
						if (c && e.type == SDL_KEYUP) { mu_input_keyup(ctx, c); }
						break;
					}
				}
			}

			switch (e.type) {
				case SDL_QUIT:
					SDL_AtomicSet(&running, 0);
					break;
				case SDL_MOUSEMOTION:
					if (mouse && SDL_GetRelativeMouseMode() && !has_open_windows) {
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
					if (mouse && !has_open_windows) {
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
				case SDL_JOYAXISMOTION:
				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
				{
					assert(e.jaxis.which == e.jbutton.which);
					SDL_Joystick *js = SDL_JoystickFromInstanceID(e.jaxis.which);
					if (js && ((sticks[0] == js) || (sticks[0] == js))) {
						struct vxtp_joystick_event ev = {
							js,
							(SDL_JoystickGetButton(js, 0) ? VXTP_JOYSTICK_A : 0) | (SDL_JoystickGetButton(js, 1) ? VXTP_JOYSTICK_B : 0),
							SDL_JoystickGetAxis(js, 0),
							SDL_JoystickGetAxis(js, 1)
						};
						SYNC(vxtp_joystick_push_event(joystick, &ev));
					}
					break;
				}
				case SDL_KEYDOWN:
					if (!has_open_windows)
						SYNC(vxtp_ppi_key_event(ppi, sdl_to_xt_scan(e.key.keysym.scancode), false));
					break;
				case SDL_KEYUP:
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
						} else if ((e.key.keysym.mod & KMOD_CTRL) && !SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Eject Floppy", "Eject mounted floppy image in A: drive?", window)) {
							SYNC(vxtp_disk_unmount(disk, 0));
						} else {
							mu_Container *cont = mu_get_container(ctx, "Help");
							if (cont) {
								cont->open = 1;
								mu_bring_to_front(ctx, cont);
							}
						}
						break;
					}

					if (!has_open_windows)
						SYNC(vxtp_ppi_key_event(ppi, sdl_to_xt_scan(e.key.keysym.scancode) | VXTP_KEY_UP_MASK, false));
					break;
			}
		}

		// Update titlebar.
		Uint32 ticks = SDL_GetTicks();
		if ((ticks - last_title_update) > 1000) {
			last_title_update = ticks;

			char buffer[100];
			double mhz;

			SYNC(
				mhz = (double)num_cycles / 1000000.0;
				num_cycles = 0;
			);

			if (ticks > 10000) {
				if (mhz > 10.0)
					snprintf(buffer, sizeof(buffer), "VirtualXT - " CPU_NAME "@%d MHz", (int)mhz);
				else
					snprintf(buffer, sizeof(buffer), "VirtualXT - " CPU_NAME "@%.2f MHz", mhz);
			} else {
				snprintf(buffer, sizeof(buffer), "VirtualXT - <Press F12 for help>");
			}
			SDL_SetWindowTitle(window, buffer);
		}

		{ // Update all windows.
			has_open_windows = false;
			mu_begin(ctx);
			help_window(ctx);
			mu_end(ctx);
			if (has_open_windows)
				SDL_SetRelativeMouseMode(false);
		}

		{ // Final rendering.
			vxt_dword bg = 0;
			SDL_Rect rect = {0};

			SYNC(
				bg = video.border_color(video.device);
				video.snapshot(video.device)
			);
			video.render(video.device, &render_callback, renderer);

			r_clear(mu_color(bg & 0x0000FF, (bg & 0x00FF00) >> 8, (bg & 0xFF0000) >> 16, 0xFF));
			SDL_RenderCopy(renderer, framebuffer, NULL, target_rect(window, &rect));

			mu_Command *cmd = NULL;
			while (mu_next_command(ctx, &cmd)) {
				switch (cmd->type) {
					case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
					case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
					case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
					case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
				}
			}		
			r_present();
		}
	}

	SDL_WaitThread(emu_thread, NULL);
	SDL_DestroyMutex(emu_mutex);

	if (network_timer)
		SDL_RemoveTimer(network_timer);

	if (audio_device)
		SDL_CloseAudioDevice(audio_device);

	vxt_system_destroy(vxt);

	if (trace_op_output)
		fclose(trace_op_output);
	if (trace_offset_output)
		fclose(trace_offset_output);

	if (str_buffer)
		SDL_free(str_buffer);

	for (int i = 0; i < 2; i++) {
		if (sticks[i])
			SDL_JoystickClose(sticks[i]);
	}

	r_destroy();
	SDL_free(ctx);

	SDL_DestroyTexture(framebuffer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
