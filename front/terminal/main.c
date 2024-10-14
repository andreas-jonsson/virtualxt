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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

/* The Mac OS X libc API selection is broken (tested with Xcode 15.0.1) */
#ifndef __APPLE__
#define _POSIX_C_SOURCE 2 /* select POSIX.2-1992 to expose popen & pclose */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <vxt/vxt.h>
#include <vxt/vxtu.h>

#include <modules.h>
#include <frontend.h>

#define TB_IMPL
#include <termbox2.h>

#include <ini.h>
#include "keys.h"
#include "docopt.h"

#if defined(_WIN32)
	#define EDIT_CONFIG "notepad "
#elif defined(__APPLE__)
	#define EDIT_CONFIG "open -e "
#else
	#define EDIT_CONFIG "xdg-open "
#endif

#define CONFIG_FILE_NAME ".vxterm.ini"
#define MIN_CLOCKS_PER_STEP 1000

#ifdef PI8088
	extern struct vxt_validator *pi8088_validator(void);
#endif

double cpu_frequency = (double)VXT_DEFAULT_FREQUENCY / 1000000.0;

struct DocoptArgs args = {0};

bool running = true;
FILE *log_fp = NULL;
bool update_screen = true;

int num_devices = 0;
struct vxt_peripheral *devices[VXT_MAX_PERIPHERALS] = { NULL };
#define APPEND_DEVICE(d) { devices[num_devices++] = (d); }

#define EMUCTRL_BUFFER_SIZE 1024
vxt_byte emuctrl_buffer[EMUCTRL_BUFFER_SIZE] = {0};
int emuctrl_buffer_len = 0;
FILE *emuctrl_file_handle = NULL;
bool emuctrl_is_popen_handle = false;

int str_buffer_len = 0;
char *str_buffer = NULL;

char floppy_image_path[FILENAME_MAX] = {0};

struct frontend_disk_controller disk_controller = {0};
struct frontend_keyboard_controller keyboard_controller = {0};
struct frontend_interface front_interface = {0};

static const char *sprint(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int size = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	if (str_buffer_len < size) {
		str_buffer_len = size;
		str_buffer = realloc(str_buffer, size);
	}

	va_start(args, fmt);
	vsnprintf(str_buffer, size, fmt, args);
	va_end(args);
	return str_buffer;
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

	// TODO: Fix this!
	const char *bp = NULL;//SDL_GetBasePath();
	bp = bp ? bp : ".";

	static char unix_path[FILENAME_MAX];
	sprintf(unix_path, "%s/../share/virtualxt", bp);

	static char dev_path[FILENAME_MAX];
	sprintf(dev_path, "%s/../../", bp);

	const char *env = getenv("VXT_SEARCH_PATH");
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
			VXT_LOG("Guest OS shutdown!");
			running = false;
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
				VXT_LOG("FRONTEND_CTRL_POPEN: popen failed!");
				return 1;
			}
			VXT_LOG("FRONTEND_CTRL_POPEN: \"%s\"", (char*)emuctrl_buffer);
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
				VXT_LOG("FRONTEND_CTRL_PUSH/PULL: fopen failed!");
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

static bool set_disk_controller(const struct frontend_disk_controller *controller) {
	if (disk_controller.device)
		return false;
	disk_controller = *controller;
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
					VXT_LOG("ERROR: Could not load module: %s", name);
					VXT_LOG("ERROR: %s", SDL_GetError());
					continue;
				}

				sprintf(buffer, "_vxtu_module_%s_entry", name);
				*(void**)(&constructors) = SDL_LoadFunction(lib, buffer);

				if (!constructors) {
					VXT_LOG("ERROR: Could not load module entry: %s", SDL_GetError());
					SDL_UnloadObject(lib);
					continue;
				}
			#endif

			vxtu_module_entry_func *const_func = constructors(vxt_logger());
			if (!const_func) {
				VXT_LOG("ERROR: Module '%s' does not return entries!", name);
				continue;
			}

			for (vxtu_module_entry_func *f = const_func; *f; f++) {
				struct vxt_peripheral *p = (*f)(VXTU_CAST(user, void*, vxt_allocator*), (void*)&front_interface, value);
				if (!p)
					continue; // Assume the module chose not to be loaded.
				APPEND_DEVICE(p);
			}

			#ifdef VXTU_STATIC_MODULES
				VXT_PRINT("%d - %s", i + 1, name);
			#else
				VXT_PRINT("- %s", name);
			#endif

			if (*value) {
				VXT_LOG(" = %s", value);
			} else {
				VXT_LOG("");
			}
		}
	}
	return 1;
}

static int configure_peripherals(void *user, const char *section, const char *name, const char *value) {
	return (vxt_system_configure(user, section, name, value) == VXT_NO_ERROR) ? 1 : 0;
}

static void print_memory_map(vxt_system *s)	{
	int prev_idx = 0;
	const vxt_byte *map = vxt_system_io_map(s);

	VXT_LOG("IO map:");
	for (int i = 0; i < VXT_IO_MAP_SIZE; i++) {
		int idx = map[i];
		if (idx && (idx != prev_idx)) {
			VXT_PRINT("[0x%.4X-", i);
			while (map[++i] == idx) {}
			VXT_LOG("0x%.4X] %s", i - 1, vxt_peripheral_name(vxt_system_peripheral(s, idx)));
			prev_idx = idx;
		}
	}

	prev_idx = 0;
	map = vxt_system_mem_map(s);

	VXT_LOG("Memory map:");
	for (int i = 0; i < VXT_MEM_MAP_SIZE; i++) {
		int idx = map[i];
		if (idx != prev_idx) {
			VXT_PRINT("[0x%.5X-", i << 4);
			while (map[++i] == idx) {}
			VXT_LOG("0x%.5X] %s", (i << 4) - 1, vxt_peripheral_name(vxt_system_peripheral(s, idx)));
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

	VXT_LOG("WARNING: No config file found. Creating new default: %s", path);
	if (!(fp = fopen(path, "w"))) {
		VXT_LOG("ERROR: Could not create config file: %s", path);
		return false;
	}

	fprintf(fp,
		"[modules]\n"
		"chipset=xt\n"
		"bios=0xFE000,GLABIOS.ROM\n"
		"uart=0x3F8,4 \t;COM1\n"
		"uart=0x2F8,3 \t;COM2\n"
		"disk=\n"
		"bios=0xE0000,vxtx.bin \t;DISK\n"
		"rtc=0x240\n"
		"bios=0xC8000,GLaTICK_0.8.4_AT.ROM \t;RTC\n"
		"rifs=\n"
		"ctrl=\n"
		"ems=lotech_ems\n"
		"\n; GDB server module should always be loadad after the others.\n"
		"; Otherwise you might have trouble with hardware breakpoints.\n"
		";gdb=1234\n"
		"\n[lotech_ems]\n"
		"memory=0xD0000\n"
		"port=0x260\n"
		"\n[rifs]\n"
		"port=0x178\n"
	);

	fclose(fp);
	return true;
}

static void close_log_file(void) {
	if (log_fp)
		fclose(log_fp);
	log_fp = NULL;
}

static void termbox_shutdown(void) {
	tb_shutdown();
}

static int log_to_file(const char *fmt, ...) {
	va_list alist;
	va_start(alist, fmt);

	int ret = 0;
	if (args.log) {
		if (!log_fp) {
			if (!(log_fp = fopen(args.log, "w")))
				return 0;
			atexit(&close_log_file);
		}
		ret = vfprintf(log_fp, fmt, alist);
	}

	va_end(alist);
	return ret;
}

static int mda_render(int offset, vxt_byte ch, enum vxtu_mda_attrib attr, int cur, void *userdata) {
	ch = (isgraph(ch) || (ch == ' ')) ? ch : '?';

	if (!update_screen) {
		update_screen = true;
		tb_clear();
	}

	uintattr_t fg = TB_DEFAULT;
	fg |= (attr & VXTU_MDA_UNDELINE) ? TB_UNDERLINE : 0;
	fg |= (attr & VXTU_MDA_HIGH_INTENSITY) ? TB_BRIGHT : 0;
	fg |= (attr & VXTU_MDA_BLINK) ? TB_BLINK : 0;
	fg |= (attr & VXTU_MDA_INVERSE) ? TB_REVERSE : 0;

	tb_set_cell(offset % 80, offset / 80, ch, fg, TB_DEFAULT);

	if (cur >= 0 )
		tb_set_cursor(cur % 80, cur / 80);
	else
		tb_hide_cursor();
	return 0;
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

	vxt_set_logger(&log_to_file);

	{
		const char *home = getenv("HOME");
		if (!home || !strcmp(home, "")) {
			VXT_LOG("ERROR: Can't find home directory!");
			return -1;
		}

		const char *path = args.config;
		if (!path)
			path = sprint("%s/" CONFIG_FILE_NAME, home);

		if (args.locate) {
			puts(path);
			return 0;
		}

		VXT_LOG("Config file path: %s", path);

		if (args.edit) {
			VXT_LOG("Open config file!");
			return system(sprint(EDIT_CONFIG "%s", path));
		}

		write_default_config(path, args.clean != 0);

		if (ini_parse(path, &load_config, NULL)) {
			VXT_LOG("Can't open, or parse: %s", path);
			return -1;
		}
	}

	if (!args.harddrive && !args.floppy) {
		args.harddrive = getenv("VXT_DEFAULT_HD_IMAGE");
		if (!args.harddrive) {
			static char harddrive_image_path_buffer[FILENAME_MAX + 2];
			strncpy(harddrive_image_path_buffer, resolve_path(FRONTEND_ANY_PATH, "boot/freedos_hd.img"), FILENAME_MAX + 1);
			args.harddrive = harddrive_image_path_buffer;
		}
	}

	if (args.frequency)
		cpu_frequency = strtod(args.frequency, NULL);
	VXT_LOG("CPU frequency: %.2f MHz", cpu_frequency);

	APPEND_DEVICE(vxtu_memory_create(&realloc, 0x0, 0x100000, false));
	struct vxt_peripheral *mda_device = vxtu_mda_create(&realloc);
	APPEND_DEVICE(mda_device);

	struct vxtu_disk_interface disk_interface = {
		&read_file, &write_file, &seek_file, &tell_file
	};

	front_interface.interface_version = FRONTEND_INTERFACE_VERSION;
	front_interface.set_disk_controller = &set_disk_controller;
	front_interface.set_keyboard_controller = &set_keyboard_controller;
	front_interface.resolve_path = &resolve_path;
	front_interface.ctrl.callback = &emu_control;
	front_interface.disk.di = disk_interface;

	#ifdef VXTU_STATIC_MODULES
		VXT_LOG("Modules are staticlly linked!");
	#endif
	VXT_LOG("Loaded modules:");

	if (ini_parse(sprint("%s/" CONFIG_FILE_NAME, getenv("HOME")), &load_modules, VXTU_CAST(&realloc, vxt_allocator*, void*))) {
		VXT_LOG("ERROR: Could not load all modules!");
		return -1;
	}

	vxt_system *vxt = vxt_system_create(&realloc, (int)(cpu_frequency * 1000000.0), devices);
	if (!vxt) {
		VXT_LOG("Could not create system!");
		return -1;
	}

	// Initializing this here is a bit late. But it works.
	front_interface.ctrl.userdata = vxt;

	if (ini_parse(sprint("%s/" CONFIG_FILE_NAME, getenv("HOME")), &configure_peripherals, vxt)) {
		VXT_LOG("ERROR: Could not configure all peripherals!");
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

	#ifdef PI8088
		vxt_system_set_validator(vxt, pi8088_validator());
	#endif

	vxt_error err = vxt_system_initialize(vxt);
	if (err != VXT_NO_ERROR) {
		VXT_LOG("vxt_system_initialize() failed with error %s", vxt_error_str(err));
		return -1;
	}

	VXT_LOG("Installed peripherals:");
	for (int i = 1; i < VXT_MAX_PERIPHERALS; i++) {
		struct vxt_peripheral *device = vxt_system_peripheral(vxt, (vxt_byte)i);
		if (device) {
			VXT_LOG("%d - %s", i, vxt_peripheral_name(device));

			// Setup MDA video
			if (vxt_peripheral_class(device) == VXT_PCLASS_PPI)
        		vxtu_ppi_set_xt_switches(device, vxtu_ppi_xt_switches(device) | 0x30);
        }
	}

	if (!disk_controller.device) {
		VXT_LOG("No disk controller!");
		return -1;
	}

	if (args.floppy) {
		strncpy(floppy_image_path, args.floppy, sizeof(floppy_image_path) - 1);

		FILE *fp = fopen(floppy_image_path, "rb+");
		if (fp && (disk_controller.mount(disk_controller.device, 0, fp) == VXT_NO_ERROR))
			VXT_LOG("Floppy image: %s", floppy_image_path);
	}

	if (args.harddrive) {
		FILE *fp = fopen(args.harddrive, "rb+");
		if (fp && (disk_controller.mount(disk_controller.device, 128, fp) == VXT_NO_ERROR)) {
			VXT_LOG("Harddrive image: %s", args.harddrive);
			if (args.hdboot || !args.floppy)
				disk_controller.set_boot(disk_controller.device, 128);
		}
	}

	print_memory_map(vxt);

	vxt_system_reset(vxt);
	vxt_system_registers(vxt)->debug = args.halt != 0;

	if (tb_init())
		return -1;
	atexit(&termbox_shutdown);

	while (running) {
		struct tb_event ev;
		while (tb_peek_event(&ev, 0) == TB_OK) {
	        switch (ev.type) {
		        case TB_EVENT_KEY:
		        	VXT_LOG("Ch %c", ev.ch);
		        	VXT_LOG("Key %X", ev.key);
		        	VXT_LOG("Mod %X", ev.mod);

		            if (ev.key == TB_KEY_F12)
		            	running = false;

					enum vxtu_scancode scan = ev.key ? tb_key_to_xt_scan(ev.key) : tb_ch_to_xt_scan(ev.ch);
		            keyboard_controller.push_event(keyboard_controller.device, scan, false);
		            keyboard_controller.push_event(keyboard_controller.device, scan | VXTU_KEY_UP_MASK, false);
		            break;
		        case TB_EVENT_RESIZE:
		        	if (mda_device)
						vxtu_mda_invalidate(mda_device);
		            break;
		        case TB_EVENT_MOUSE:
		            break;
		        default:
		            break;
	        }
	    }

		if (mda_device) {
			vxtu_mda_traverse(mda_device, &mda_render, NULL);
			if (update_screen) {
				update_screen = false;
				tb_present();
			}
		}

		vxt_system_step(vxt, MIN_CLOCKS_PER_STEP);
	}

	vxt_system_destroy(vxt);
	if (str_buffer)
		free(str_buffer);
	return 0;
}
