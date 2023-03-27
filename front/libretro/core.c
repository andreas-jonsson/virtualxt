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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdatomic.h>

#define VXT_LIBC
#include <vxt/vxt.h>
#include <vxt/vxtu.h>
#include <vxtp.h>

#include "keys.h"

#include "../../bios/pcxtbios.h"
#include "../../bios/vxtx.h"

#define AUDIO_FREQUENCY 44100

#define LOG(...) log_cb(RETRO_LOG_INFO, __VA_ARGS__)

#define SYNC(...) {								   	                \
    atomic_bool sys_lock = false;                                   \
    bool f = false;                                                 \
    while (!atomic_compare_exchange_weak(&sys_lock, &f, true)) {}   \
	{ __VA_ARGS__ ; }                                               \
    atomic_store(&sys_lock, false);                                 \
}

retro_perf_get_time_usec_t get_time_usec = NULL;
retro_log_printf_t log_cb = NULL;
retro_video_refresh_t video_cb = NULL;
retro_audio_sample_t audio_cb = NULL;
retro_input_poll_t input_poll_cb = NULL;
retro_input_state_t input_state_cb = NULL;
retro_environment_t environ_cb = NULL;

struct retro_vfs_interface *vfs = NULL;

enum vxtu_mouse_button mouse_state = 0;
long long last_update = 0;

bool floppy_ejected = false;
int num_disk_images = 0;
int current_disk_image = 0;
struct retro_vfs_file_handle *disk_image_files[256] = {NULL};
struct retro_vfs_file_handle *hd_image = NULL;

int cpu_frequency = VXT_DEFAULT_FREQUENCY;
enum vxt_cpu_type cpu_type = VXT_CPU_8088;

vxt_system *sys = NULL;
struct vxt_pirepheral *disk = NULL;
struct vxt_pirepheral *ppi = NULL;
struct vxt_pirepheral *cga = NULL;
struct vxt_pirepheral *mouse = NULL;
struct vxt_pirepheral *joystick = NULL;

static void no_log(enum retro_log_level level, const char *fmt, ...) {
    (void)level; (void)fmt;
}

static int log_wrapper(const char *fmt, ...) {
    assert(log_cb);
	va_list args;
	va_start(args, fmt);

	char buffer[4096] = {0};
	int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    log_cb(RETRO_LOG_INFO, buffer);

	va_end(args);
	return n;
}

static int read_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s;
    return (int)vfs->read((struct retro_vfs_file_handle*)fp, buffer, size);
}

static int write_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s;
    return (int)vfs->write((struct retro_vfs_file_handle*)fp, buffer, size);
}

static int seek_file(vxt_system *s, void *fp, int offset, enum vxtu_disk_seek whence) {
	(void)s;
	switch (whence) {
		case VXTU_SEEK_START: return (int)vfs->seek((struct retro_vfs_file_handle*)fp, offset, RETRO_VFS_SEEK_POSITION_START);
		case VXTU_SEEK_CURRENT: return (int)vfs->seek((struct retro_vfs_file_handle*)fp, offset, RETRO_VFS_SEEK_POSITION_CURRENT);
		case VXTU_SEEK_END: return (int)vfs->seek((struct retro_vfs_file_handle*)fp, offset, RETRO_VFS_SEEK_POSITION_END);
		default: return -1;
	}
}

static int tell_file(vxt_system *s, void *fp) {
	(void)s;
	return (int)vfs->tell((struct retro_vfs_file_handle*)fp);
}

static long long ustimer(void) {
    assert(get_time_usec);
    return (long long)get_time_usec();
}

static void audio_callback(void) {
    assert(ppi);
    vxt_int16 sample = 0;
    SYNC(sample = vxtu_ppi_generate_sample(ppi, AUDIO_FREQUENCY));
    audio_cb(sample, sample);
}

static void keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers) {
    (void)character; (void)key_modifiers;
    assert(ppi);
    if (keycode != RETROK_UNKNOWN)
        SYNC(vxtu_ppi_key_event(ppi, retro_to_xt_scan(keycode) | (down ? 0 : VXTU_KEY_UP_MASK), false));
}

static void joystick_event(unsigned port) {
    vxt_int16 x = (input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) / 256) + 128;
    vxt_int16 y = (input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y) / 256) + 128;
    enum vxtp_joystick_button btn = (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A ) ? VXTP_JOYSTICK_A : 0)
        | (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B ) ? VXTP_JOYSTICK_B : 0);

    struct vxtp_joystick_event ev = {
        (void*)(uintptr_t)port,
        btn, x, y
    };
    SYNC(vxtp_joystick_push_event(joystick, &ev));
}

static void mouse_event(void) {
    int x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    int y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    int l = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
    int r = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

    enum vxtu_mouse_button btn = (enum vxtu_mouse_button)((l << 1) | r);
    if ((btn != mouse_state) || x || y) {
        mouse_state = btn;
        struct vxtu_mouse_event state = {btn, x, y};
        SYNC(vxtu_mouse_push_event(mouse, &state));
    }
}

static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
    (void)userdata;
    video_cb(rgba, width, height, width * 4);
    return 0;
}

static struct retro_vfs_file_handle *load_disk_image(const char *path) {
    struct retro_vfs_file_handle *fp = vfs->open(path, RETRO_VFS_FILE_ACCESS_READ_WRITE | RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING, RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS);
    if (!fp) {
        // Try open as read-only.
        fp = vfs->open(path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS);
        if (!fp) {
            log_cb(RETRO_LOG_ERROR, "Could not open: %s\n", path);
            return NULL;
        }
    }
    return fp;
}

static bool set_eject_state(bool ejected) {
    if (ejected == floppy_ejected)
        return true;

    if (ejected) {
        vxtu_disk_unmount(disk, 0);
        floppy_ejected = true;
    } else {
        if (!num_disk_images)
            return false;
        struct retro_vfs_file_handle *fp = disk_image_files[current_disk_image];
        if (vxtu_disk_mount(disk, 0, (void*)fp) != VXT_NO_ERROR) {
            log_cb(RETRO_LOG_ERROR, "Could not mount disk image at index: %d\n", current_disk_image);
            return false;
        }
        floppy_ejected = false;
    }
    return true;
}

static bool get_eject_state(void) {
    return floppy_ejected;
}

static unsigned get_image_index(void) {
    return current_disk_image;
}

static bool set_image_index(unsigned index) {
    current_disk_image = index;
    return true;
}

static unsigned get_num_images(void) {
    return num_disk_images;
}

static bool replace_image_index(unsigned index, const struct retro_game_info *info) {
    if (!info)
        return false;

    assert(!disk_image_files[index]);
    struct retro_vfs_file_handle *fp = disk_image_files[index] = load_disk_image(info->path);
    if (!fp)
        return false;

    if ((int)vfs->size(fp) > 1474560) {
        disk_image_files[index] = NULL;
        vfs->close(fp);
        log_cb(RETRO_LOG_ERROR, "Can not mount harddrive images at runtime!\n");
        return false;
    }
    return true;
}

static bool add_image_index(void) {
    if (num_disk_images == 256)
        return false;
    num_disk_images++;
    return true;
}

static void check_variables(void) {
    struct retro_variable var = { .key = "virtualxt_cpu_frequency" };
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        double freq;
        if (sscanf(var.value, "%lf", &freq) != 1)
            log_cb(RETRO_LOG_ERROR, "Invalid frequency: %s\n", var.value);
        cpu_frequency = (int)(freq * 1000000.0);
        if (sys)
            SYNC(vxt_system_set_frequency(sys, cpu_frequency));
    }

    var = (struct retro_variable){ .key = "virtualxt_v20" };
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        cpu_type = strcmp(var.value, "false") ? VXT_CPU_V20 : VXT_CPU_8088;
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
    assert(!sys);
    check_variables();
    SYNC(
        vxt_set_logger(&log_wrapper);

        struct vxtu_disk_interface interface = {
            &read_file, &write_file, &seek_file, &tell_file
        };
        
        disk = vxtu_disk_create(&realloc, &interface);
        ppi = vxtu_ppi_create(&realloc);
        cga = vxtu_cga_create(&realloc);
        mouse = vxtu_mouse_create(&realloc, 0x3F8, 4); // COM1
        joystick = vxtp_joystick_create(&realloc, (void*)0, (void*)1);

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
            mouse,
            joystick,
            NULL
        };

        sys = vxt_system_create(&realloc, cpu_type, cpu_frequency, devices);
        vxt_system_initialize(sys);

        LOG("CPU Type: %s\n", (cpu_type == VXT_CPU_8088) ? "Intel 8088" : "NEC V20");
        LOG("CPU Frequency: %.2fMHz\n", (double)cpu_frequency / 1000000.0);
        LOG("Installed pirepherals:\n");
        for (int i = 1; i < VXT_MAX_PIREPHERALS; i++) {
            struct vxt_pirepheral *device = vxt_system_pirepheral(sys, (vxt_byte)i);
            if (device)
                LOG("%d - %s\n", i, vxt_pirepheral_name(device));
        }
        vxt_system_reset(sys);
    )
}

void retro_deinit(void) {
	assert(sys);
	SYNC(vxt_system_destroy(sys));
    sys = NULL;

    if (hd_image) {
        vfs->close(hd_image);
        hd_image = NULL;
    }

    while (num_disk_images)
        vfs->close(disk_image_files[--num_disk_images]);
    vxt_memclear(disk_image_files, sizeof(void*) * 256);
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
    vxt_memclear(info, sizeof(struct retro_system_av_info));
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

    static const struct retro_variable vars[] = {
        { "virtualxt_v20", "NEC V20; false|true" },
        { "virtualxt_cpu_frequency", "CPU Frequency; 4.77MHz|6MHz|8MHz|10MHz|12MHz|16MHz" },
        { NULL, NULL }
    };
    cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);

    static struct retro_disk_control_callback disk_control = {
        &set_eject_state,
        &get_eject_state,
        &get_image_index,
        &set_image_index,
        &get_num_images,
        &replace_image_index,
        &add_image_index
    };
    cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, (void*)&disk_control);

    struct retro_perf_callback perf;
    if (!cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, (void*)&perf) || !perf.get_time_usec)
        log_cb(RETRO_LOG_ERROR, "Need perf timers!\n");
    else
        get_time_usec = perf.get_time_usec;

    static struct retro_vfs_interface_info vfs_info = {0};
    if (!cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, (void*)&vfs_info) || !vfs_info.iface)
        log_cb(RETRO_LOG_ERROR, "No file system interface!\n");
    else
        vfs = vfs_info.iface;
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
    SYNC(vxt_system_reset(sys));
}

void retro_run(void) {
    assert(sys);
    long long now = ustimer();
    double delta = (double)(now - last_update);
    last_update = now;

    bool updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
        SYNC(check_variables());

    const double freq = (double)cpu_frequency / 1000000.0;
    int clocks = (int)(freq * delta);
    if (clocks > cpu_frequency) {
        clocks = cpu_frequency;
    }

    SYNC(
        struct vxt_step s = vxt_system_step(sys, clocks);
        if (s.err != VXT_NO_ERROR)
            log_cb(RETRO_LOG_ERROR, vxt_error_str(s.err));
        last_update -= (s.cycles - clocks) * freq;

        vxtu_cga_snapshot(cga);
    );

    vxtu_cga_render(cga, &render_callback, NULL);

    input_poll_cb();

    if (joystick) {
        joystick_event(0);
        joystick_event(1);
    }

    if (mouse)
        mouse_event();
}

bool retro_load_game(const struct retro_game_info *info) SYNC(
    assert(sys);

    struct retro_vfs_file_handle *fp = load_disk_image(info->path);
    int dos_idx = ((int)vfs->size(fp) > 1474560) ? 128 : 0;
    if (vxtu_disk_mount(disk, dos_idx, (void*)fp) != VXT_NO_ERROR) {
        log_cb(RETRO_LOG_ERROR, "Could not mount disk image: %s\n", info->path);
        vfs->close(fp);
        return false;
    }

    vxtu_disk_set_boot_drive(disk, dos_idx);
    if (dos_idx < 128) {
        LOG("Floppy image mounted!\n");
        disk_image_files[num_disk_images++] = fp;
    } else {
        LOG("Harddrive image mounted!\n");
        hd_image = fp;
        floppy_ejected = true;
    }

    struct retro_input_descriptor desc[] = {
        { 0, RETRO_DEVICE_ANALOG, 0, RETRO_DEVICE_ID_ANALOG_X,     "X Axis" },
        { 0, RETRO_DEVICE_ANALOG, 0, RETRO_DEVICE_ID_ANALOG_Y,     "Y Axis" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 1" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 2" },
        { 1, RETRO_DEVICE_ANALOG, 0, RETRO_DEVICE_ID_ANALOG_X,     "X Axis" },
        { 1, RETRO_DEVICE_ANALOG, 0, RETRO_DEVICE_ID_ANALOG_Y,     "Y Axis" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 1" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 2" },
        { 0 }
    };
    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    struct retro_keyboard_callback kbdesk = { &keyboard_event };
    environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kbdesk);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log_cb(RETRO_LOG_ERROR, "XRGB8888 is not supported!\n");
        return false;
    }

    struct retro_audio_callback audio_cb = { &audio_callback, NULL };
    environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

    last_update = ustimer();
    return true;
)

void retro_unload_game(void) {
    assert(sys);
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
