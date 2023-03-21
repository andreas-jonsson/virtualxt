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

#include <libretro.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define VXT_LIBC
#include <vxt/vxt.h>
#include <vxt/vxtu.h>

#include "keys.h"

#include "../../bios/pcxtbios.h"
#include "../../bios/vxtx.h"

#define AUDIO_FREQUENCY 16000

#define LOG(...) log_cb(RETRO_LOG_INFO, __VA_ARGS__)

retro_perf_get_time_usec_t get_time_usec = NULL;
retro_log_printf_t log_cb = NULL;
retro_video_refresh_t video_cb = NULL;
retro_audio_sample_t audio_cb = NULL;
retro_input_poll_t input_poll_cb = NULL;
retro_input_state_t input_state_cb = NULL;
retro_environment_t environ_cb = NULL;

long long last_update = 0;

vxt_byte disk_idx = 0;
FILE *booter = NULL; 

vxt_system *sys = NULL;
struct vxt_pirepheral *disk = NULL;
struct vxt_pirepheral *ppi = NULL;
struct vxt_pirepheral *cga = NULL;

static void no_log(enum retro_log_level level, const char *fmt, ...) {
    (void)level; (void)fmt;
}

static int log_wrapper(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char buffer[4096] = {0};
	int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    log_cb(RETRO_LOG_INFO, buffer);

	va_end(args);
	return n;
}

static int get_disk_size(FILE *fp) {
    long pos = ftell(fp);
    if (fseek(fp, 0, SEEK_END) != 0) return -1;
    int sz = (int)ftell(fp);
    if (fseek(fp, pos, SEEK_SET)) return -1;
    return sz;
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

static long long ustimer(void) {
    assert(get_time_usec);
    return (long long)get_time_usec();
}

static void audio_callback(void) {
    vxt_int16 sample = vxtu_ppi_generate_sample(ppi, AUDIO_FREQUENCY);
    audio_cb(sample, sample);
}

static void keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers) {
    (void)character; (void)key_modifiers;
    assert(ppi);
    if (keycode != RETROK_UNKNOWN)
        vxtu_ppi_key_event(ppi, retro_to_xt_scan(keycode) | (down ? 0 : VXTU_KEY_UP_MASK), false);
}

static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
    (void)userdata;
    video_cb(rgba, width, height, width * 4);
    return 0;
}

static void check_variables(void) {
}

static struct vxt_pirepheral *load_bios(const vxt_byte *data, int size, vxt_pointer base) {
	struct vxt_pirepheral *rom = vxtu_memory_create(&realloc, base, size, true);
	if (!vxtu_memory_device_fill(rom, data, size)) {
		log_cb(RETRO_LOG_ERROR, "vxtu_memory_device_fill() failed!\n");
		return NULL;
	}
	LOG("Loaded BIOS @ 0x%X-0x%X\n", base, base + size - 1);
	return rom;
}

void retro_init(void) {
	vxt_set_logger(&log_wrapper);

    struct vxtu_disk_interface interface = {
		&read_file, &write_file, &seek_file, &tell_file
	};
	
	disk = vxtu_disk_create(&realloc, &interface);
	ppi = vxtu_ppi_create(&realloc);
    cga = vxtu_cga_create(&realloc);

	struct vxt_pirepheral *devices[] = {
		vxtu_memory_create(&realloc, 0x0, 0x100000, false),
        load_bios(pcxtbios_bin, (int)pcxtbios_bin_len, 0xFE000),
        load_bios(vxtx_bin, (int)vxtx_bin_len, 0xE0000),
        vxtu_pic_create(&realloc),
	    vxtu_dma_create(&realloc),
        vxtu_pit_create(&realloc),
        ppi,
		cga,
        disk,
		NULL
	};

	assert(!sys);
	sys = vxt_system_create(&realloc, VXT_CPU_8088, VXT_DEFAULT_FREQUENCY, devices);
	vxt_system_initialize(sys);

	LOG("Installed pirepherals:\n");
	for (int i = 1; i < VXT_MAX_PIREPHERALS; i++) {
		struct vxt_pirepheral *device = vxt_system_pirepheral(sys, (vxt_byte)i);
		if (device)
			LOG("%d - %s\n", i, vxt_pirepheral_name(device));
	}
	vxt_system_reset(sys);
}

void retro_deinit(void) {
	assert(sys);
	vxt_system_destroy(sys);
	sys = NULL;
}

unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
    LOG("Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info) {
    vxt_memclear(info, sizeof(struct retro_system_info));
    info->library_name = "VirtualXT";
    info->library_version = vxt_lib_version();
    info->valid_extensions = "img";
    info->need_fullpath = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    info->geometry.base_width = 640;
    info->geometry.base_height = 200;
    info->geometry.max_width = 720;
    info->geometry.max_height = 350;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.sample_rate = (double)AUDIO_FREQUENCY;
    info->timing.fps = 60.0;
}

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    struct retro_log_callback logging;
    log_cb = cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging) ? logging.log : no_log;

    struct retro_perf_callback perf;
    if (!cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf) || !perf.get_time_usec)
        log_cb(RETRO_LOG_ERROR, "Need perf timers!\n");
    else
        get_time_usec = perf.get_time_usec;

    static const struct retro_controller_description controllers[] = {
        { "Gamepad", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
    };

    static const struct retro_controller_info ports[] = {
        { controllers, 1 },
        { NULL, 0 },
    };
    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    (void)cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}

void retro_reset(void) {
    assert(sys);
    vxt_system_reset(sys);
}

void retro_run(void) {
    long long now = ustimer();
    double delta = (double)(now - last_update);
    last_update = now;

    bool updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
        check_variables();

    const double freq = (double)VXT_DEFAULT_FREQUENCY / 1000000.0;
    int clocks = (int)(freq * delta);
    if (clocks > VXT_DEFAULT_FREQUENCY) {
        clocks = VXT_DEFAULT_FREQUENCY;
    }

	struct vxt_step s = vxt_system_step(sys, clocks);
	if (s.err != VXT_NO_ERROR)
		log_cb(RETRO_LOG_ERROR, vxt_error_str(s.err));
    last_update -= (s.cycles - clocks) * freq;

    vxtu_cga_snapshot(cga);
    vxtu_cga_render(cga, &render_callback, NULL);
}

bool retro_load_game(const struct retro_game_info *info) {
    booter = fopen(info->path, "rb+");
    if (!booter) {
        // Try open as read-only.
        booter = fopen(info->path, "rb");
        if (!booter) {
            log_cb(RETRO_LOG_ERROR, "Could not open: %s\n", info->path);
            return false;
        }
    }

    disk_idx = (get_disk_size(booter) > 1474560) ? 128 : 0;
    if (vxtu_disk_mount(disk, disk_idx, (void*)booter) != VXT_NO_ERROR) {
        log_cb(RETRO_LOG_ERROR, "Could not mount floppy image: %s\n", info->path);
        fclose(booter);
        booter = NULL;
        return false;
    }
    vxtu_disk_set_boot_drive(disk, disk_idx);

     /*
    struct retro_input_descriptor desc[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 0 },
    };
    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
    */

    struct retro_keyboard_callback kbdesk = { &keyboard_event };
    environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kbdesk);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log_cb(RETRO_LOG_ERROR, "XRGB8888 is not supported!\n");
        return false;
    }

    struct retro_audio_callback audio_cb = { &audio_callback, NULL };
    environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

    check_variables();
    last_update = ustimer();
    return true;
}

void retro_unload_game(void) {
    if (booter) {
        vxtu_disk_unmount(disk, disk_idx);
        fclose(booter);
        booter = NULL;
    }
}

unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    (void)type; (void)info; (void)num;
    return false;
}

size_t retro_serialize_size(void) {
    return 0;
}

bool retro_serialize(void *data, size_t size) {
    (void)data; (void)size;
    return false;
}

bool retro_unserialize(const void *data, size_t size) {
    (void)data; (void)size;
    return false;
}

void *retro_get_memory_data(unsigned id) {
    (void)id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id) {
    (void)id;
    return 0;
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    (void)index; (void)enabled; (void)code;
}
