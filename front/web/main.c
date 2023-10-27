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

#include <stdarg.h>
#include <stdio.h>

#include <vxt/vxt.h>
#include <vxt/vxtu.h>

#include <frontend.h>
#include <modules.h>

#include "../../bios/glabios.h"
#include "../../bios/vxtx.h"

#include "js.h"

#define ALLOCATOR_SIZE (20 * 1024 * 1024)
vxtu_static_allocator(ALLOCATOR, ALLOCATOR_SIZE)

#define LOG(...) ( log_wrapper(__VA_ARGS__) )

int num_devices = 0;
struct vxt_pirepheral *devices[VXT_MAX_PIREPHERALS] = { NULL };
#define APPEND_DEVICE(d) { devices[num_devices++] = (d); }

int disk_head = 0;

int cga_width = -1;
int cga_height = -1;
vxt_dword cga_border = 0;
struct frontend_video_adapter video_adapter = {0};

vxt_system *sys = NULL;
struct vxt_pirepheral *disk = NULL;
struct vxt_pirepheral *ppi = NULL;
struct vxt_pirepheral *mouse = NULL;

static int log_wrapper(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char buffer[1024] = {0};
	int n = vsnprintf_(buffer, sizeof(buffer), fmt, args);
    js_puts(buffer, n);

	va_end(args);
	return n;
}

static int read_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s; (void)fp;

	int disk_sz = (int)js_disk_size();
	if ((disk_head + size) > disk_sz)
		size = disk_sz - disk_head;

	js_disk_read(buffer, size, disk_head);
	disk_head += size;
	return size;
}

static int write_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s; (void)fp;

	int disk_sz = (int)js_disk_size();
	if ((disk_head + size) > disk_sz)
		size = disk_sz - disk_head;

	js_disk_write(buffer, size, disk_head);
	disk_head += size;
	return size;
}

static int seek_file(vxt_system *s, void *fp, int offset, enum vxtu_disk_seek whence) {
	(void)s; (void)fp;

	int disk_sz = (int)js_disk_size();
	int pos = -1;

	switch (whence) {
		case VXTU_SEEK_START:
			if ((pos = offset) > disk_sz)
				return -1;
			break;
		case VXTU_SEEK_CURRENT:
			pos = disk_head + offset;
			if ((pos < 0) || (pos > disk_sz))
				return -1;
			break;
		case VXTU_SEEK_END:
			pos = disk_sz - offset;
			if ((pos < 0) || (pos > disk_sz))
				return -1;
			break;
		default:
			LOG("Invalid seek!");
			return -1;
	}

	disk_head = pos;
	return 0;
}

static int tell_file(vxt_system *s, void *fp) {
	(void)s; (void)fp;
	return disk_head;
}

static struct vxt_pirepheral *load_bios(const vxt_byte *data, int size, vxt_pointer base) {
	struct vxt_pirepheral *rom = vxtu_memory_create(&ALLOCATOR, base, size, true);
	if (!vxtu_memory_device_fill(rom, data, size)) {
		LOG("vxtu_memory_device_fill() failed!\n");
		return NULL;
	}
	LOG("Loaded BIOS @ 0x%X-0x%X\n", base, base + size - 1);
	return rom;
}

static bool set_video_adapter(const struct frontend_video_adapter *adapter) {
	if (video_adapter.device)
		return false;
	video_adapter = *adapter;
	return true;
}

static vxt_byte emu_control(enum frontend_ctrl_command cmd, void *userdata) {
	(void)userdata;
	if (cmd == FRONTEND_CTRL_SHUTDOWN) {
		LOG("Guest OS shutdown!");
		js_shutdown();
	}
	return 0;
}

static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
    cga_width = width;
	cga_height = height;
    *(const vxt_byte**)userdata = rgba;
    return 0;
}

int wasm_video_width(void) {
	return cga_width;
}

int wasm_video_height(void) {
	return cga_height;
}

const void *wasm_video_rgba_memory_pointer(void) {
	if (!video_adapter.device)
		return NULL;

	video_adapter.snapshot(video_adapter.device);

	vxt_dword color = video_adapter.border_color(video_adapter.device);
	if (color != cga_border)
		js_set_border_color((cga_border = color));

	const vxt_byte *video_mem_buffer = NULL;
	video_adapter.render(video_adapter.device, &render_callback, &video_mem_buffer);
	return (void*)video_mem_buffer;
}

void wasm_send_key(int scan) {
	vxtu_ppi_key_event(ppi, (enum vxtu_scancode)scan, false);
}

void wasm_send_mouse(int xrel, int yrel, unsigned int buttons) {
	struct vxtu_mouse_event ev = {0, xrel, yrel};
	if (buttons & 1)
		ev.buttons |= VXTU_MOUSE_LEFT;
	if (buttons & 2)
		ev.buttons |= VXTU_MOUSE_RIGHT;
	vxtu_mouse_push_event(mouse, &ev);
}

int wasm_step_emulation(int cycles) {
	struct vxt_step s = vxt_system_step(sys, cycles);
	if (s.err != VXT_NO_ERROR)
		LOG(vxt_error_str(s.err));
	return s.cycles;
}

double wasm_generate_sample(int freq) {
	return (double)vxtu_ppi_generate_sample(ppi, freq) / 32767.0;
}

void wasm_initialize_emulator(int v20, int freq) {
	vxt_set_logger(&log_wrapper);

	struct vxtu_disk_interface intrf = {
		&read_file, &write_file, &seek_file, &tell_file
	};
	struct vxt_pirepheral *disk = vxtu_disk_create(&ALLOCATOR, &intrf);
	vxtu_disk_set_activity_callback(disk, &js_disk_activity, NULL);
	
	ppi = vxtu_ppi_create(&ALLOCATOR);
	mouse = vxtu_mouse_create(&ALLOCATOR, 0x3F8); // COM1

	APPEND_DEVICE(vxtu_memory_create(&ALLOCATOR, 0x0, 0x100000, false));
	APPEND_DEVICE(load_bios(glabios_bin, (int)glabios_bin_len, 0xFE000));
	APPEND_DEVICE(load_bios(vxtx_bin, (int)vxtx_bin_len, 0xE0000));
	APPEND_DEVICE(vxtu_uart_create(&ALLOCATOR, 0x3F8, 4));
	APPEND_DEVICE(vxtu_pic_create(&ALLOCATOR));
	APPEND_DEVICE(vxtu_dma_create(&ALLOCATOR));
	APPEND_DEVICE(vxtu_pit_create(&ALLOCATOR));
	APPEND_DEVICE(ppi);
	APPEND_DEVICE(disk);
	APPEND_DEVICE(mouse);

	#ifndef VXTU_STATIC_MODULES
		#error The web frontend requires all modules to be staticlly linked!
	#endif

	vxtu_module_entry_func *e = _vxtu_module_ctrl_entry(&log_wrapper);
	if (e) {
		static struct frontend_interface fi = { .interface_version = FRONTEND_INTERFACE_VERSION };
		fi.ctrl.callback = &emu_control;
		fi.set_video_adapter = &set_video_adapter;
		APPEND_DEVICE((*e)(&ALLOCATOR, &fi, ""));
	}

	sys = vxt_system_create(&ALLOCATOR, v20 ? VXT_CPU_V20 : VXT_CPU_8088, freq, devices);
	vxt_system_initialize(sys);
	
	LOG("Installed pirepherals:\n");
	for (int i = 1; i < VXT_MAX_PIREPHERALS; i++) {
		struct vxt_pirepheral *device = vxt_system_pirepheral(sys, (vxt_byte)i);
		if (device)
			LOG("%d - %s\n", i, vxt_pirepheral_name(device));
	}

	int drive_num = (js_disk_size() > 1474560) ? 128 : 0;
	LOG("Disk num: %d", drive_num);

	vxt_error err = vxtu_disk_mount(disk, drive_num, (void*)1);
	if (err != VXT_NO_ERROR)
		LOG("%s (0x%X)", vxt_error_str(err), err);

	vxtu_disk_set_boot_drive(disk, drive_num);
	vxt_system_reset(sys);
}

void _putchar(char ch) {
    (void)ch;
}

int getch_(void) {
	return 0;
}

void ungetch_(int ch) {
	(void)ch;
}
