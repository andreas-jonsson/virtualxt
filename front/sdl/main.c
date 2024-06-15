// Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    This product make use of the VirtualXT software emulator.
//    Visit https://virtualxt.org for more information.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

/* The Mac OS X libc API selection is broken (tested with Xcode 15.0.1) */
#ifndef __APPLE__
#define _POSIX_C_SOURCE 2 /* select POSIX.2-1992 to expose popen & pclose */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <vxt/vxt.h>
#include <vxt/vxtu.h>

#include <modules.h>
#include <frontend.h>

#include <SDL.h>
#include <ini.h>
#include <microui.h>

#include "mu_renderer.h"
#include "window.h"
#include "keys.h"
#include "docopt.h"
#include "icons.h"

#if defined(_WIN32)
	#define EDIT_CONFIG "notepad "
#elif defined(__APPLE__)
	#define EDIT_CONFIG "open -e "
#else
	#define EDIT_CONFIG "xdg-open "
#endif

#define CONFIG_FILE_NAME "config.ini"
#define MIN_CLOCKS_PER_STEP 1
#define MAX_PENALTY_USEC 1000

FILE *trace_op_output = NULL;
FILE *trace_offset_output = NULL;

#define SYNC(...) {								   	\
	if (SDL_LockMutex(emu_mutex) == -1)			   	\
		printf("sync error: %s\n", SDL_GetError());	\
	{ __VA_ARGS__ ; }							   	\
	if (SDL_UnlockMutex(emu_mutex) == -1)		   	\
		printf("sync error: %s\n", SDL_GetError());	\
}

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

struct DocoptArgs args = {0};

Uint32 last_title_update = 0;
int num_cycles = 0;
double cpu_frequency = (double)VXT_DEFAULT_FREQUENCY / 1000000.0;
enum vxt_cpu_type cpu_type = VXT_CPU_8088;
bool cpu_paused = false;

int num_devices = 0;
struct vxt_pirepheral *devices[VXT_MAX_PIREPHERALS] = { NULL };
#define APPEND_DEVICE(d) { devices[num_devices++] = (d); }

// Needed for detecting turbo mode.
struct vxt_pirepheral *ppi_device = NULL;

SDL_atomic_t running = {1};
SDL_mutex *emu_mutex = NULL;
SDL_Thread *emu_thread = NULL;

SDL_Texture *framebuffer = NULL;
SDL_Point framebuffer_size = {640, 200};

char floppy_image_path[FILENAME_MAX] = {0};
char new_floppy_image_path[FILENAME_MAX] = {0};

bool config_updated = false;

#define EMUCTRL_BUFFER_SIZE 1024
vxt_byte emuctrl_buffer[EMUCTRL_BUFFER_SIZE] = {0};
int emuctrl_buffer_len = 0;
FILE *emuctrl_file_handle = NULL;
bool emuctrl_is_popen_handle = false;

int str_buffer_len = 0;
char *str_buffer = NULL;

#define AUDIO_FREQUENCY 44100
#define AUDIO_LATENCY 10

SDL_AudioDeviceID audio_device = 0;
SDL_AudioSpec audio_spec = {0};

#define MAX_AUDIO_ADAPTERS 8
int num_audio_adapters = 0;
struct frontend_audio_adapter audio_adapters[MAX_AUDIO_ADAPTERS] = {0};

struct frontend_video_adapter video_adapter = {0};
struct frontend_mouse_adapter mouse_adapter = {0};
struct frontend_disk_controller disk_controller = {0};
struct frontend_joystick_controller joystick_controller = {0};
struct frontend_keyboard_controller keyboard_controller = {0};

struct frontend_interface front_interface = {0};

static int text_width(mu_Font font, const char *text, int len) {
	(void)font;
	if (len == -1)
		len = (int)strlen(text);
	return mr_get_text_width(text, len);
}

static int text_height(mu_Font font) {
	(void)font;
	return mr_get_text_height();
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

static void tracer(vxt_system *s, vxt_pointer addr, vxt_byte data) {
	(void)s; (void)addr;
	fwrite(&data, 1, 1, trace_op_output);
	fwrite(&addr, sizeof(vxt_pointer), 1, trace_offset_output);
}

static int emu_loop(void *ptr) {
	vxt_system *vxt = (vxt_system*)ptr;
	Sint64 penalty = 0;
	double frequency = cpu_frequency;
	int frequency_hz = (int)(cpu_frequency * 1000000.0);
	Uint64 start = SDL_GetPerformanceCounter();

	while (SDL_AtomicGet(&running)) {
		struct vxt_step res;
		SYNC(
			if (!cpu_paused) {
				res = vxt_system_step(vxt, MIN_CLOCKS_PER_STEP);
				if (res.err != VXT_NO_ERROR) {
					if (res.err == VXT_USER_TERMINATION)
						SDL_AtomicSet(&running, 0);
					else
						printf("step error: %s", vxt_error_str(res.err));
				} else if (!args.no_idle && (res.halted || res.int28)) { // Assume int28 is DOS waiting for input.
					SDL_Delay(1); // Yield CPU time to other processes.
				}
				num_cycles += res.cycles;
			}
			
			frequency = vxtu_ppi_turbo_enabled(ppi_device) ? cpu_frequency : ((double)VXT_DEFAULT_FREQUENCY / 1000000.0);
			frequency_hz = (int)(frequency * 1000000.0);
			vxt_system_set_frequency(vxt, frequency_hz);
		);

		for (;;) {
			const Uint64 f = SDL_GetPerformanceFrequency() / (Uint64)frequency_hz;
			if (!f) {
				penalty = 0;
				break;
			}

			const Sint64 max_penalty = (Sint64)(frequency * (double)MAX_PENALTY_USEC);
			const Sint64 c = (Sint64)((SDL_GetPerformanceCounter() - start) / f) + penalty;
			const Sint64 d = c - (Uint64)res.cycles;
			if (d >= 0) {
				penalty = (d > max_penalty) ? max_penalty : d;
				break;
			}
		}
		start = SDL_GetPerformanceCounter();
	}
	return 0;
}

void monitors_window(mu_Context *ctx, vxt_system *s) {
	if (mu_begin_window_ex(ctx, "Monitors", mu_rect(20, 20, 340, 440), MU_OPT_CLOSED)) SYNC(
		has_open_windows = true;

		mu_layout_row(ctx, 2, (int[]){ 120, -1 }, 0);
		if (mu_button_ex(ctx, cpu_paused ? "Resume" : "Pause", 0, MU_OPT_ALIGNCENTER))
			cpu_paused = !cpu_paused;
		
		mu_label(ctx, cpu_paused ? "Halted!" : "Running...");

		for (vxt_byte i = 0; i < VXT_MAX_MONITORS;) {
			const struct vxt_monitor *d = vxt_system_monitor(s, i);
			if (!d) break;

			int open = mu_header_ex(ctx, d->dev, 0);
			for (vxt_byte j = i; j < VXT_MAX_MONITORS; j++) {
				const struct vxt_monitor *m = vxt_system_monitor(s, j);
				if (!m || strcmp(m->dev, d->dev)) {
					i = j;
					break;
				} else if (!open) {
					continue;
				}
	
				mu_layout_row(ctx, 2, (int[]){ mr_get_text_width(m->name, (int)strlen(m->name)) + 10, -1 }, 0);
				mu_label(ctx, m->name);

				uint64_t reg = 0;
				char buf[128] = {0};

				if (m->flags & VXT_MONITOR_SIZE_BYTE) reg = *(vxt_byte*)m->reg;
				else if (m->flags & VXT_MONITOR_SIZE_WORD) reg = *(vxt_word*)m->reg;
				else if (m->flags & VXT_MONITOR_SIZE_DWORD) reg = *(vxt_dword*)m->reg;
				else if (m->flags & VXT_MONITOR_SIZE_QWORD) reg = *(uint64_t*)m->reg;

				if (m->flags & VXT_MONITOR_FORMAT_DECIMAL) SDL_ulltoa(reg, buf, 10);
				else if (m->flags & VXT_MONITOR_FORMAT_HEX) { buf[0] = '0'; buf[1] = 'x'; SDL_ulltoa(reg, &buf[2], 16); }
				else if (m->flags & VXT_MONITOR_FORMAT_BINARY) { buf[0] = '0'; buf[1] = 'b'; SDL_ulltoa(reg, &buf[2], 2); }

				mu_textbox_ex(ctx, buf, sizeof(buf), MU_OPT_NOINTERACT);
			}
		}
		mu_end_window(ctx);
	);
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
		framebuffer = SDL_CreateTexture(userdata, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
		if (!framebuffer) {
			printf("SDL_CreateTexture() failed with error %s\n", SDL_GetError());
			return -1;
		}
		framebuffer_size = (SDL_Point){width, height};
	}

	int status = SDL_UpdateTexture(framebuffer, NULL, rgba, width * 4);
	if (status != 0)
		printf("SDL_UpdateTexture() failed with error %s\n", SDL_GetError());
	return status;
}

static void audio_callback(void *udata, uint8_t *stream, int len) {
	(void)udata;
	len /= 2;
	
	SYNC(
		for (int i = 0; i < len; i++) {
			vxt_word sample = 0;
			for (int j = 0; j < num_audio_adapters; j++) {
				struct frontend_audio_adapter *a = &audio_adapters[j];
				sample += a->generate_sample(a->device, audio_spec.freq);
			}

			((vxt_int16*)stream)[i] = sample;
			if (audio_spec.channels > 1)
				((vxt_int16*)stream)[++i] = sample;
		}
	);
}

static void disk_activity_cb(int disk, void *data) {
	(void)disk;
	SDL_AtomicSet((SDL_atomic_t*)data, (int)SDL_GetTicks() + 0xFF);
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

static int seek_file(vxt_system *s, void *fp, int offset, enum vxtu_disk_seek whence) {
	(void)s;
	switch (whence) {
		case VXTU_SEEK_START: return (int)fseek((FILE*)fp, (long)offset, SEEK_SET);
		case VXTU_SEEK_CURRENT: return (int)fseek((FILE*)fp, (long)offset, SEEK_CUR);
		case VXTU_SEEK_END: return (int)fseek((FILE*)fp, (long)offset, SEEK_END);
		default: return -1;
	}
}

static int tell_file(vxt_system *s, void *fp) {
	(void)s;
	return (int)ftell((FILE*)fp);
}

static bool file_exist(const char *path) {
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return false;
	fclose(fp);
	return true;
}

// Note that this fuction uses static memory. Only call this from the
// frontend interface or before the emulator thread is started.
static const char *resolve_path(enum frontend_path_type type, const char *path) {
	static char buffer[FILENAME_MAX + 1] = {0};
	strncpy(buffer, path, FILENAME_MAX);

	if (file_exist(buffer))
		return buffer;
	
	const char *bp = SDL_GetBasePath();
	bp = bp ? bp : ".";

	static char unix_path[FILENAME_MAX];
	sprintf(unix_path, "%s/../share/virtualxt", bp);

	static char dev_path[FILENAME_MAX];
	sprintf(dev_path, "%s/../../", bp);

	const char *env = SDL_getenv("VXT_SEARCH_PATH");
	env = env ? env : ".";

	const char *search_paths[] = { env, bp, args.config, unix_path, dev_path, NULL };

	for (int i = 0; search_paths[i]; i++) {
		const char *sp = search_paths[i];

		sprintf(buffer, "%s/%s", sp, path);
		if (file_exist(buffer))
			return buffer;

		switch (type) {
			case FRONTEND_BIOS_PATH: sprintf(buffer, "%s/bios/%s", sp, path); break;
			case FRONTEND_MODULE_PATH: sprintf(buffer, "%s/modules/%s", sp, path); break;
			default: break;
		};
		
		if (file_exist(buffer))
			return buffer;
	}

	strncpy(buffer, path, FILENAME_MAX);
	return buffer;
}

static vxt_byte emu_control(enum frontend_ctrl_command cmd, vxt_byte data, void *userdata) {
	vxt_system *s = (vxt_system*)userdata;
	switch (cmd) {
		case FRONTEND_CTRL_RESET:
			emuctrl_buffer_len = 0;
			if (emuctrl_file_handle) {
				if (emuctrl_is_popen_handle)
					#ifdef _WIN32
						_pclose
					#else
						pclose
					#endif
					(emuctrl_file_handle);
				else
					fclose(emuctrl_file_handle);
			}
			emuctrl_file_handle = NULL;
			emuctrl_is_popen_handle = false;
			break;
		case FRONTEND_CTRL_SHUTDOWN:
			printf("Guest OS shutdown!\n");
			SDL_AtomicSet(&running, 0);
			break;
		case FRONTEND_CTRL_HAS_DATA:
			if (emuctrl_file_handle) {
				int ch = fgetc(emuctrl_file_handle);
				if (ch & ~0xFF) {
					return 0;
				} else {
					ungetc(ch, emuctrl_file_handle);
					return 1;
				}
			}
			return emuctrl_buffer_len ? 1 : 0;
		case FRONTEND_CTRL_READ_DATA:
			if (emuctrl_file_handle)
				return (vxt_byte)fgetc(emuctrl_file_handle);
			else if (emuctrl_buffer_len)
				return emuctrl_buffer[--emuctrl_buffer_len];
			break;
		case FRONTEND_CTRL_WRITE_DATA:
			if (emuctrl_file_handle && !emuctrl_is_popen_handle)
				return (fputc(data, emuctrl_file_handle) == data) ? 0 : 1;
			if (emuctrl_buffer_len >= EMUCTRL_BUFFER_SIZE)
				return 1;
			emuctrl_buffer[emuctrl_buffer_len++] = data;
			break;
		case FRONTEND_CTRL_POPEN:
			if (!(emuctrl_file_handle = 
				#ifdef _WIN32
					_popen
				#else
					popen
				#endif
					((char*)emuctrl_buffer, "r"))
			) {
				printf("FRONTEND_CTRL_POPEN: popen failed!\n");
				return 1;
			}
			printf("FRONTEND_CTRL_POPEN: \"%s\"\n", (char*)emuctrl_buffer);
			emuctrl_buffer_len = 0;
			emuctrl_is_popen_handle = true;
			break;
		case FRONTEND_CTRL_PCLOSE:
			if (!emuctrl_is_popen_handle)
				return 1;
			if (emuctrl_file_handle)
				pclose(emuctrl_file_handle);
			emuctrl_file_handle = NULL;
			emuctrl_is_popen_handle = false;
			break;
		case FRONTEND_CTRL_FCLOSE:
			if (emuctrl_is_popen_handle)
				return 1;
			if (emuctrl_file_handle)
				fclose(emuctrl_file_handle);
			emuctrl_file_handle = NULL;
			break;
		case FRONTEND_CTRL_PUSH:
		case FRONTEND_CTRL_PULL:
			if (!(emuctrl_file_handle = fopen((char*)emuctrl_buffer, (cmd == FRONTEND_CTRL_PUSH) ? "wb" : "rb"))) {
				printf("FRONTEND_CTRL_PUSH/PULL: fopen failed!\n");
				return 1;
			}
			emuctrl_buffer_len = 0;
			break;
		case FRONTEND_CTRL_DEBUG:
			vxt_system_registers(s)->debug = true;
			break;
	}
	return 0;
}

static bool set_audio_adapter(const struct frontend_audio_adapter *adapter) {
	if (num_audio_adapters >= MAX_AUDIO_ADAPTERS)
		return false;
	memcpy(&audio_adapters[num_audio_adapters++], adapter, sizeof(struct frontend_audio_adapter));
	return true;
}

static bool set_video_adapter(const struct frontend_video_adapter *adapter) {
	if (video_adapter.device)
		return false;
	video_adapter = *adapter;
	return true;
}

static bool set_mouse_adapter(const struct frontend_mouse_adapter *adapter) {
	if (mouse_adapter.device)
		return false;
	mouse_adapter = *adapter;
	return true;
}

static bool set_disk_controller(const struct frontend_disk_controller *controller) {
	if (disk_controller.device)
		return false;
	disk_controller = *controller;
	return true;
}

static bool set_joystick_controller(const struct frontend_joystick_controller *controller) {
	if (joystick_controller.device)
		return false;
	joystick_controller = *controller;
	return true;
}

static bool set_keyboard_controller(const struct frontend_keyboard_controller *controller) {
	if (keyboard_controller.device)
		return false;
	keyboard_controller = *controller;
	return true;
}

static int load_config(void *user, const char *section, const char *name, const char *value) {
	(void)user; (void)section; (void)name; (void)value;
	if (!strcmp("args", section)) {
		if (!strcmp("hdboot", name))
			args.hdboot |= atoi(value);
		else if (!strcmp("halt", name))
			args.halt |= atoi(value);
		else if (!strcmp("mute", name))
			args.mute |= atoi(value);
		else if (!strcmp("cpu", name))
		{
			if (!strcmp("v20", value)) args.cpu = "v20";
			else if (!strcmp("286", value)) args.cpu = "286";
		}
		else if (!strcmp("no-activity", name))
			args.no_activity |= atoi(value);
		else if (!strcmp("no-idle", name))
			args.no_idle |= atoi(value);
		else if (!strcmp("harddrive", name) && !args.harddrive) {
			static char harddrive_image_path[FILENAME_MAX + 2] = {0};
			strncpy(harddrive_image_path, resolve_path(FRONTEND_ANY_PATH, value), FILENAME_MAX + 1);
			args.harddrive = harddrive_image_path;
		}
	}
	return 1;
}

static int load_modules(void *user, const char *section, const char *name, const char *value) {
	if (!strcmp("modules", section)) {
		for (
			int i = 0;
			#ifdef VXTU_STATIC_MODULES
				true;
			#else
				i < 1;
			#endif
			i++
		) {
			vxtu_module_entry_func *(*constructors)(int(*)(const char*, ...)) = NULL;

			#ifdef VXTU_STATIC_MODULES
				const struct vxtu_module_entry *e = &vxtu_module_table[i];
				if (!e->name)
					break;
				else if (strcmp(e->name, name))
					continue;

				constructors = e->entry;
			#else
				char buffer[FILENAME_MAX];
				sprintf(buffer, "%s.vxt", name);
				strcpy(buffer, resolve_path(FRONTEND_MODULE_PATH, buffer));

				void *lib = SDL_LoadObject(buffer);
				if (!lib) {
					printf("ERROR: Could not load module: %s\n", name);
					printf("ERROR: %s\n", SDL_GetError());
					continue;
				}

				sprintf(buffer, "_vxtu_module_%s_entry", name);
				*(void**)(&constructors) = SDL_LoadFunction(lib, buffer);

				if (!constructors) {
					printf("ERROR: Could not load module entry: %s\n", SDL_GetError());
					SDL_UnloadObject(lib);
					continue;
				}
			#endif

			vxtu_module_entry_func *const_func = constructors(vxt_logger());
			if (!const_func) {
				printf("ERROR: Module '%s' does not return entries!\n", name);
				continue;
			}

			for (vxtu_module_entry_func *f = const_func; *f; f++) {
				struct vxt_pirepheral *p = (*f)(VXTU_CAST(user, void*, vxt_allocator*), (void*)&front_interface, value);
				if (!p)
					continue; // Assume the module chose not to be loaded.
				APPEND_DEVICE(p);
			}

			#ifdef VXTU_STATIC_MODULES
				printf("%d - %s", i + 1, name);
			#else
				printf("- %s", name);
			#endif

			if (*value) printf(" = %s\n", value);
			else putc('\n', stdout);
		}
	}
	return 1;
}

static int configure_pirepherals(void *user, const char *section, const char *name, const char *value) {
	return (vxt_system_configure(user, section, name, value) == VXT_NO_ERROR) ? 1 : 0;
}

static void print_memory_map(vxt_system *s)	{
	int prev_idx = 0;
	const vxt_byte *map = vxt_system_io_map(s);

	printf("IO map:\n");
	for (int i = 0; i < VXT_IO_MAP_SIZE; i++) {
		int idx = map[i];
		if (idx && (idx != prev_idx)) {
			printf("[0x%.4X-", i);
			while (map[++i] == idx);
			printf("0x%.4X] %s\n", i - 1, vxt_pirepheral_name(vxt_system_pirepheral(s, idx)));
			prev_idx = idx;
		}			
	}

	prev_idx = 0;
	map = vxt_system_mem_map(s);

	printf("Memory map:\n");
	for (int i = 0; i < VXT_MEM_MAP_SIZE; i++) {
		int idx = map[i];
		if (idx != prev_idx) {
			printf("[0x%.5X-", i << 4);
			while (map[++i] == idx);
			printf("0x%.5X] %s\n", (i << 4) - 1, vxt_pirepheral_name(vxt_system_pirepheral(s, idx)));
			prev_idx = idx;
			i--;
		}			
	}
}

static bool write_default_config(const char *path, bool clean) {
	FILE *fp;
	if (!clean && (fp = fopen(path, "r"))) {
		fclose(fp);
		return false;
	}

	printf("WARNING: No config file found. Creating new default: %s\n", path);
	if (!(fp = fopen(path, "w"))) {
		printf("ERROR: Could not create config file: %s\n", path);
		return false;
	}

	fprintf(fp,
		"[modules]\n"
		"chipset=xt\n"
		"bios=0xFE000,GLABIOS.ROM\n"
		"uart=0x3F8,4 \t;COM1\n"
		"uart=0x2F8,3 \t;COM2\n"
		"cga=\n"
		";vga=vgabios.bin\n"
		"disk=\n"
		"bios=0xE0000,vxtx.bin \t;DISK\n"
		"rtc=0x240\n"
		"bios=0xC8000,GLaTICK_0.8.4_AT.ROM \t;RTC\n"
		"adlib=\n"
		"rifs=\n"
		"ctrl=\n"
		"joystick=0x201\n"
		"ems=lotech_ems\n"
		"mouse=0x3F8\n"
		";serial=0x2F8,/dev/ttyUSB0\n"
		";covox=0x378,disney\n"
		";network=eth0\n"
		";arstech_isa=\n"
		";ch36x_isa=/dev/ch36xpci0\n"
		";lua=lua/serial_debug.lua,0x3F8\n"
		"\n; GDB server module should always be loadad after the others.\n"
		"; Otherwise you might have trouble with hardware breakpoints.\n"
		";gdb=1234\n"
		"\n[args]\n"
		";halt=1\n"
		";cpu=v20\n"
		";hdboot=1\n"
		";harddrive=boot/freedos_hd.img\n"
		"\n[ch36x_isa]\n"
		"port=0x201\n"
		"\n[arstech_isa]\n"
		"port=0x201\n"
		"\n[lotech_ems]\n"
		"memory=0xD0000\n"
		"port=0x260\n"
		"\n[rifs]\n"
		"port=0x178\n"
	);

	fclose(fp);
	return true;
}

int main(int argc, char *argv[]) {
	// This is a hack because there seems to be a bug in DocOpt
	// that prevents us from adding trailing parameters.
	char *rifs_path = NULL;
	if ((argc == 2) && (*argv[1] != '-')) {
		rifs_path = argv[1];
		argc = 1;
	}

	args = docopt(argc, argv, true, vxt_lib_version());
	args.rifs = rifs_path ? rifs_path : args.rifs;

	if (!args.config) {
		args.config = SDL_GetPrefPath("virtualxt", "VirtualXT-SDL2-" VXT_VERSION);
		if (!args.config) {
			printf("No config path!\n");
			return -1;
		}
	}

	if (args.locate) {
		puts(args.config);
		return 0;
	}

	printf("Config path: %s\n", args.config);

	const char *base_path = SDL_GetBasePath();
	base_path = base_path ? base_path : "./";
	printf("Base path: %s\n", base_path);

	{
		const char *path = sprint("%s/" CONFIG_FILE_NAME, args.config);
		config_updated = write_default_config(path, args.clean != 0);

		if (ini_parse(path, &load_config, NULL)) {
			printf("Can't open, or parse: %s\n", path);
			return -1;
		}
	}

	if (args.edit) {
		printf("Open config file!\n");
		return system(sprint(EDIT_CONFIG "%s/" CONFIG_FILE_NAME, args.config));
	}

	if (!args.harddrive && !args.floppy) {
		args.harddrive = SDL_getenv("VXT_DEFAULT_HD_IMAGE");
		if (!args.harddrive) {
			static char harddrive_image_path_buffer[FILENAME_MAX + 2];
			strncpy(harddrive_image_path_buffer, resolve_path(FRONTEND_ANY_PATH, "boot/freedos_hd.img"), FILENAME_MAX + 1);
			args.harddrive = harddrive_image_path_buffer;
		}
	}

	if (!strcmp(args.cpu, "v20")) {
		cpu_type = VXT_CPU_V20;
		args.cpu = "V20";
	} else if (!strcmp(args.cpu, "286")) {
		cpu_type = VXT_CPU_286;
	} else {
		args.cpu = "8088";
	}
	printf("CPU type: %s\n", args.cpu);

	if (args.frequency)
		cpu_frequency = strtod(args.frequency, NULL);
	printf("CPU frequency: %.2f MHz\n", cpu_frequency);

	#if !defined(_WIN32) && !defined(__APPLE__)
		SDL_setenv("SDL_VIDEODRIVER", "x11", 1);
	#endif

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init() failed with error %s\n", SDL_GetError());
		return -1;
	}

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
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		printf("SDL_CreateRenderer() failed with error %s\n", SDL_GetError());
		return -1;
	}

	framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, framebuffer_size.x, framebuffer_size.y);
	if (!framebuffer) {
		printf("SDL_CreateTexture() failed with error %s\n", SDL_GetError());
		return -1;
	}

	SDL_RWops *rwops = SDL_RWFromConstMem(disk_activity_icon, sizeof(disk_activity_icon));
	SDL_Surface *surface = SDL_LoadBMP_RW(rwops, 1);
	if (!surface) {
		printf("SDL_LoadBMP_RW() failed with error %s\n", SDL_GetError());
		return -1;
	}

	SDL_Texture *disk_icon_texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!disk_icon_texture) {
		printf("SDL_CreateTextureFromSurface() failed with error %s\n", SDL_GetError());
		return -1;
	}
	SDL_FreeSurface(surface);

	mr_renderer *mr = mr_init(renderer);
	mu_Context *ctx = SDL_malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
	ctx->text_height = text_height;

	int num_sticks = 0;
	SDL_Joystick *sticks[2] = {NULL};
	
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

	vxt_set_logger(&printf);

	APPEND_DEVICE(vxtu_memory_create(&realloc, 0x0, 0x100000, false));

	struct vxtu_disk_interface disk_interface = {
		&read_file, &write_file, &seek_file, &tell_file
	};

	front_interface.interface_version = FRONTEND_INTERFACE_VERSION;
	front_interface.set_video_adapter = &set_video_adapter;
	front_interface.set_audio_adapter = &set_audio_adapter;
	front_interface.set_mouse_adapter = &set_mouse_adapter;
	front_interface.set_disk_controller = &set_disk_controller;
	front_interface.set_joystick_controller = &set_joystick_controller;
	front_interface.set_keyboard_controller = &set_keyboard_controller;
	front_interface.resolve_path = &resolve_path;
	front_interface.ctrl.callback = &emu_control;
	front_interface.disk.di = disk_interface;
	
	SDL_atomic_t icon_fade = {0};
	if (!args.no_activity) {
		front_interface.disk.activity_callback = &disk_activity_cb;
		front_interface.disk.userdata = &icon_fade;
	}

	#ifdef VXTU_STATIC_MODULES
		printf("Modules are staticlly linked!\n");
	#endif
	printf("Loaded modules:\n");

	if (ini_parse(sprint("%s/" CONFIG_FILE_NAME, args.config), &load_modules, VXTU_CAST(&realloc, vxt_allocator*, void*))) {
		printf("ERROR: Could not load all modules!\n");
		return -1;
	}

	vxt_system *vxt = vxt_system_create(&realloc, cpu_type, (int)(cpu_frequency * 1000000.0), devices);
	if (!vxt) {
		printf("Could not create system!\n");
		return -1;
	}

	// Initializing this here is a bit late. But it works.
	front_interface.ctrl.userdata = vxt;

	if (ini_parse(sprint("%s/" CONFIG_FILE_NAME, args.config), &configure_pirepherals, vxt)) {
		printf("ERROR: Could not configure all pirepherals!\n");
		return -1;
	}

	#ifdef VXTU_MODULE_RIFS
		if (args.rifs)
			vxt_system_configure(vxt, "rifs", "root", args.rifs);
		else
			vxt_system_configure(vxt, "rifs", "readonly", "1");
	#endif

	#ifdef VXTU_MODULE_GDB
		if (args.halt)
			vxt_system_configure(vxt, "gdb", "halt", "1");
	#endif

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
		if (device) {
			printf("%d - %s\n", i, vxt_pirepheral_name(device));

			if (vxt_pirepheral_class(device) == VXT_PCLASS_PPI)
				ppi_device = device;
		}
	}

	if (!ppi_device) {
		printf("No PPI device!\n");
		return -1;
	}

	if (!disk_controller.device) {
		printf("No disk controller!\n");
		return -1;
	}

	if (args.floppy) {
		strncpy(floppy_image_path, args.floppy, sizeof(floppy_image_path) - 1);

		FILE *fp = fopen(floppy_image_path, "rb+");
		if (fp && (disk_controller.mount(disk_controller.device, 0, fp) == VXT_NO_ERROR))
			printf("Floppy image: %s\n", floppy_image_path);
	}

	if (args.harddrive) {
		FILE *fp = fopen(args.harddrive, "rb+");
		if (fp && (disk_controller.mount(disk_controller.device, 128, fp) == VXT_NO_ERROR)) {
			printf("Harddrive image: %s\n", args.harddrive);
			if (args.hdboot || !args.floppy)
				disk_controller.set_boot(disk_controller.device, 128);
		}
	}

	print_memory_map(vxt);

	vxt_system_reset(vxt);
	vxt_system_registers(vxt)->debug = args.halt != 0;

	if (!(emu_mutex = SDL_CreateMutex())) {
		printf("SDL_CreateMutex failed!\n");
		return -1;
	}

	if (!(emu_thread = SDL_CreateThread(&emu_loop, "emulator loop", vxt))) {
		printf("SDL_CreateThread failed!\n");
		return -1;
	}

	if (args.mute) {
		printf("Audio is muted!\n");
	} else {
		SDL_AudioSpec spec = {0};
		spec.freq = AUDIO_FREQUENCY;
		spec.format = AUDIO_S16;
		spec.channels = 1;
		spec.samples = pow2((AUDIO_FREQUENCY / 1000) * AUDIO_LATENCY);
		spec.callback = &audio_callback;

		int allowed_changes = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE;

		if (!(audio_device = SDL_OpenAudioDevice(NULL, false, &spec, &audio_spec, allowed_changes))) {
			printf("SDL_OpenAudioDevice() failed with error %s\n", SDL_GetError());
			return -1;
		}
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
					if (mouse_adapter.device && SDL_GetRelativeMouseMode() && !has_open_windows) {
						Uint32 state = SDL_GetMouseState(NULL, NULL);
						struct frontend_mouse_event ev = {0, e.motion.xrel, e.motion.yrel};
						if (state & SDL_BUTTON_LMASK)
							ev.buttons |= FRONTEND_MOUSE_LEFT;
						if (state & SDL_BUTTON_RMASK)
							ev.buttons |= FRONTEND_MOUSE_RIGHT;
						SYNC(mouse_adapter.push_event(mouse_adapter.device, &ev));
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (mouse_adapter.device && !has_open_windows) {
						if (e.button.button == SDL_BUTTON_MIDDLE) {
							SDL_SetRelativeMouseMode(false);
							break;
						}
						SDL_SetRelativeMouseMode(true);

						struct frontend_mouse_event ev = {0};
						if (e.button.button == SDL_BUTTON_LEFT)
							ev.buttons |= FRONTEND_MOUSE_LEFT;
						if (e.button.button == SDL_BUTTON_RIGHT)
							ev.buttons |= FRONTEND_MOUSE_RIGHT;
						SYNC(mouse_adapter.push_event(mouse_adapter.device, &ev));
					}
					break;
				case SDL_DROPFILE:
					strncpy(new_floppy_image_path, e.drop.file, sizeof(new_floppy_image_path) - 1);
					open_window(ctx, "Mount");
					break;
				case SDL_JOYAXISMOTION:
				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
				{
					assert(e.jaxis.which == e.jbutton.which);
					SDL_Joystick *js = SDL_JoystickFromInstanceID(e.jaxis.which);
					if (js && ((sticks[0] == js) || (sticks[1] == js))) {
						struct frontend_joystick_event ev = {
							(sticks[0] == js) ? FRONTEND_JOYSTICK_1 : FRONTEND_JOYSTICK_2,
							(SDL_JoystickGetButton(js, 0) ? FRONTEND_JOYSTICK_A : 0) | (SDL_JoystickGetButton(js, 1) ? FRONTEND_JOYSTICK_B : 0),
							SDL_JoystickGetAxis(js, 0),
							SDL_JoystickGetAxis(js, 1)
						};
						if (joystick_controller.device) SYNC(
							joystick_controller.push_event(joystick_controller.device, &ev)
						)
					}
					break;
				}
				case SDL_KEYDOWN:
					if (!has_open_windows && keyboard_controller.device && (e.key.keysym.sym != SDLK_F11) && (e.key.keysym.sym != SDLK_F12))
						SYNC(keyboard_controller.push_event(keyboard_controller.device, sdl_to_xt_scan(e.key.keysym.scancode), false));
					break;
				case SDL_KEYUP:
					if (e.key.keysym.sym == SDLK_F11) {
						if (e.key.keysym.mod & KMOD_ALT) {
							printf("Toggle turbo!\n");
							SYNC(
								vxt_byte data = ppi_device->io.in(VXT_GET_DEVICE_PTR(ppi_device), 0x61);
								ppi_device->io.out(VXT_GET_DEVICE_PTR(ppi_device), 0x61, data ^ 4);
							);
						} else if (e.key.keysym.mod & KMOD_CTRL) {
							open_window(ctx, "Eject");
						} else {
							if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
								SDL_SetWindowFullscreen(window, 0);
								SDL_SetRelativeMouseMode(false);
							} else {
								SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
								SDL_SetRelativeMouseMode(true);
							}
						}
						break;
					} else if (e.key.keysym.sym == SDLK_F12) {
						open_window(ctx, (e.key.keysym.mod & KMOD_CTRL) ? "Monitors" : "Help");
						break;
					}

					if (!has_open_windows && keyboard_controller.device)
						SYNC(keyboard_controller.push_event(keyboard_controller.device, sdl_to_xt_scan(e.key.keysym.scancode) | VXTU_KEY_UP_MASK, false));
					break;
			}
		}

		// Update titlebar.
		Uint32 ticks = SDL_GetTicks();
		if ((ticks - last_title_update) > 500) {
			last_title_update = ticks;

			char buffer[100];
			double mhz;
			bool turbo;

			SYNC(
				mhz = (double)num_cycles / 500000.0;
				num_cycles = 0;
				turbo = vxtu_ppi_turbo_enabled(ppi_device);
			);
			
			if (ticks > 10000) {
				snprintf(buffer, sizeof(buffer), "VirtualXT - %s@%.2f MHz%s", args.cpu, mhz, turbo ? " (Turbo)" : "");
			} else {
				snprintf(buffer, sizeof(buffer), "VirtualXT - <Press F12 for help>");
			}
			SDL_SetWindowTitle(window, buffer);
		}

		{ // Update all windows and there functions.
			has_open_windows = false;
			mu_begin(ctx);

			help_window(ctx);
			error_window(ctx);
			monitors_window(ctx, vxt);

			if (config_updated) {
				config_updated = false;
				open_window(ctx, "Config Update");
			}

			if (config_window(ctx)) {
				if (system(sprint(EDIT_CONFIG "%s/" CONFIG_FILE_NAME, args.config)))
					return -1;
			}

			if (eject_window(ctx, (*floppy_image_path != 0) ? floppy_image_path: NULL)) {
				SYNC(disk_controller.unmount(disk_controller.device, 0));
				*floppy_image_path = 0;
			}

			if (mount_window(ctx, new_floppy_image_path)) {
				FILE *fp = fopen(new_floppy_image_path, "rb+");
				if (!fp) {
					open_error_window(ctx, "Could not open floppy image file!");
				} else {
					vxt_error err = VXT_NO_ERROR;
					SYNC(err = disk_controller.mount(disk_controller.device, 0, fp));
					if (err != VXT_NO_ERROR) {
						open_error_window(ctx, "Could not mount floppy image file!");
						fclose(fp);
					} else {
						strncpy(floppy_image_path, new_floppy_image_path, sizeof(floppy_image_path));
					}
				}
			}

			mu_end(ctx);
			if (has_open_windows)
				SDL_SetRelativeMouseMode(false);
		}

		{ // Final rendering.
			vxt_dword bg = 0;
			SDL_Rect rect = {0};

			if (video_adapter.device) {
				SYNC(
					bg = video_adapter.border_color(video_adapter.device);
					video_adapter.snapshot(video_adapter.device);
				);
				video_adapter.render(video_adapter.device, &render_callback, renderer);
			}

			mr_clear(mr, mu_color(bg & 0x0000FF, (bg & 0x00FF00) >> 8, (bg & 0xFF0000) >> 16, 0xFF));
			if (SDL_RenderCopy(renderer, framebuffer, NULL, target_rect(window, &rect)) != 0)
				printf("SDL_RenderCopy() failed with error %s\n", SDL_GetError());

			mu_Command *cmd = NULL;
			while (mu_next_command(ctx, &cmd)) {
				switch (cmd->type) {
					case MU_COMMAND_TEXT: mr_draw_text(mr, cmd->text.str, cmd->text.pos, cmd->text.color); break;
					case MU_COMMAND_RECT: mr_draw_rect(mr, cmd->rect.rect, cmd->rect.color); break;
					case MU_COMMAND_ICON: mr_draw_icon(mr, cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
					case MU_COMMAND_CLIP: mr_set_clip_rect(mr, cmd->clip.rect); break;
				}
			}

			int fade = SDL_AtomicGet(&icon_fade) - (int)SDL_GetTicks();
			if (fade > 0) {
				SDL_Rect dst = { 4, 2, 20, 20 };
				SDL_SetTextureAlphaMod(disk_icon_texture, (fade > 0xFF) ? 0xFF : fade);
				SDL_RenderCopy(renderer, disk_icon_texture, NULL, &dst);
			}

			mr_present(mr);
		}
	}

	SDL_WaitThread(emu_thread, NULL);

	if (audio_device)
		SDL_CloseAudioDevice(audio_device);

	SDL_DestroyMutex(emu_mutex);

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

	mr_destroy(mr);
	SDL_free(ctx);

	SDL_DestroyTexture(disk_icon_texture); 
	SDL_DestroyTexture(framebuffer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
