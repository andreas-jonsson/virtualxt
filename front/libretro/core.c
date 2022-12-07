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

#include <libretro.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define VXT_LIBC
#define VXT_LIBC_ALLOCATOR
#include <vxt/vxt.h>
#include <vxt/vxtu.h>
#include <vxtp.h>

extern const vxt_byte *get_pcxtbios_data(void);
extern vxt_dword get_pcxtbios_size(void);
extern const vxt_byte *get_vxtx_data(void);
extern vxt_dword get_vxtx_size(void);

#define LOG(...) log_cb(RETRO_LOG_INFO, __VA_ARGS__)

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 200

retro_perf_get_time_usec_t time_usec_cb = NULL;
retro_log_printf_t log_cb = NULL;
retro_video_refresh_t video_cb = NULL;
retro_audio_sample_t audio_cb = NULL;
retro_audio_sample_batch_t audio_batch_cb = NULL;
retro_input_poll_t input_poll_cb = NULL;
retro_input_state_t input_state_cb = NULL;
retro_environment_t environ_cb = NULL;

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

static long long ustimer(void) {
    return (long long)(((double)clock() / CLOCKS_PER_SEC) * 1000000.0);
    //return (long long)time_usec_cb();
}

static struct vxt_pirepheral *load_bios(const vxt_byte *data, int size, vxt_pointer base) {
	struct vxt_pirepheral *rom = vxtu_memory_create(&vxt_clib_malloc, base, size, true);
	if (!vxtu_memory_device_fill(rom, data, size)) {
		printf("vxtu_memory_device_fill() failed!\n");
		return NULL;
	}
	LOG("Loaded BIOS @ 0x%X-0x%X\n", base, base + size - 1);
	return rom;
}

void retro_init(void) {
	vxt_set_logger(&log_wrapper);
	
	//struct vxt_pirepheral *disk = vxtp_disk_create(&vxt_clib_malloc, NULL);
	struct vxt_pirepheral *pit = vxtp_pit_create(&vxt_clib_malloc, &ustimer);

	ppi = vxtp_ppi_create(&vxt_clib_malloc, pit);
    cga = vxtp_cga_create(&vxt_clib_malloc, &ustimer);

	struct vxt_pirepheral *devices[] = {
		vxtu_memory_create(&vxt_clib_malloc, 0x0, 0x100000, false),
        load_bios(get_pcxtbios_data(), (int)get_pcxtbios_size(), 0xFE000),
        load_bios(get_vxtx_data(), (int)get_vxtx_size(), 0xE0000),
        vxtp_pic_create(&vxt_clib_malloc),
	    vxtp_dma_create(&vxt_clib_malloc),
        pit,
        ppi,
		cga,
        //disk,
		NULL
	};

	assert(!sys);
	sys = vxt_system_create(&vxt_clib_malloc, devices);
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
	if (sys) {
		vxt_system_destroy(sys);
		sys = NULL;
	}
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
    info->library_version = "0.8.0";
    info->valid_extensions = "img";
    info->need_fullpath = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    info->geometry.base_width    = VIDEO_WIDTH;
    info->geometry.base_height  = VIDEO_HEIGHT;
    info->geometry.max_width     = VIDEO_WIDTH;
    info->geometry.max_height    = VIDEO_HEIGHT;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
}

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    struct retro_log_callback logging;
    log_cb = cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging) ? logging.log : no_log;

    struct retro_perf_callback perf;
    if (!cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf) || !perf.get_time_usec)
        log_cb(RETRO_LOG_ERROR, "Need perf timers!\n");
    time_usec_cb = perf.get_time_usec;

    static const struct retro_controller_description controllers[] = {
        { "Gamepad", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
    };

    static const struct retro_controller_info ports[] = {
        { controllers, 1 },
        { NULL, 0 },
    };

    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

    bool no_rom = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
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
}

/*
static void audio_callback(void) {
    audio_cb();
}

static void audio_set_state(bool enable) {
  (void)enable;
}
*/

static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
    (void)userdata;
    video_cb(rgba, width, height, width * 4);
    return 0;
}

static void check_variables(void) {
}

void retro_run(void) {
    bool updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        check_variables();
    }

	vxt_system_step(sys, 4770000 / 60);

    vxtp_cga_snapshot(cga);
    vxtp_cga_render(cga, &render_callback, NULL);
}

void retro_keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers) {
    (void)down; (void)keycode; (void)character; (void)key_modifiers;
}

bool retro_load_game(const struct retro_game_info *info) {

    LOG("Load game! Size: %d\n", info ? info->size : -1);

     /*
    struct retro_input_descriptor desc[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 0 },
    };
    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
    */

    struct retro_keyboard_callback kbdesk = {&retro_keyboard_event};
    environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kbdesk);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log_cb(RETRO_LOG_ERROR, "XRGB8888 is not supported!\n");
        return false;
    }

    //struct retro_audio_callback audio_cb = { audio_callback, audio_set_state };
    //use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

    check_variables();
    return true;
}

void retro_unload_game(void) {
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
