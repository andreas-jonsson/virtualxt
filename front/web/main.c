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

#include <stdarg.h>
#include <string.h>

#include <vxt/vxt.h>
#include <vxt/vxtu.h>
#include <vxtp.h>

#include <printf.h>

#include "main.h"

#define ALLOCATOR_SIZE (20 * 1024 * 1024)
vxt_byte allocator_data[ALLOCATOR_SIZE];
vxt_byte *allocator_ptr = allocator_data;

#define LOG(...) ( log_wrapper(__VA_ARGS__) )

int disk_head = 0;

int cga_width = -1;
int cga_height = -1;
vxt_dword cga_border = 0;

vxt_system *sys = NULL;
struct vxt_pirepheral *disk = NULL;
struct vxt_pirepheral *ppi = NULL;
struct vxt_pirepheral *cga = NULL;

static int log_wrapper(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char buffer[1024] = {0};
	int n = vsnprintf_(buffer, sizeof(buffer), fmt, args);
    js_puts(buffer, n);

	va_end(args);
	return n;
}

static void *allocator(void *ptr, int sz) {
	if (!sz) {
		return NULL;
	} else if (ptr) {
		LOG("Reallocation is not support!\n");
		return NULL;
	}

	vxt_byte *p = allocator_ptr;
	sz += 8 - (sz % 8);
	allocator_ptr += sz;
	
	if (allocator_ptr >= (allocator_data + ALLOCATOR_SIZE)) {
		LOG("Allocator is out of memory!\n");
		return NULL;
	}
	return p;
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

static int seek_file(vxt_system *s, void *fp, int offset, enum vxtp_disk_seek whence) {
	(void)s; (void)fp;

	int disk_sz = (int)js_disk_size();
	int pos = -1;

	switch (whence) {
		case VXTP_SEEK_START:
			if ((pos = offset) > disk_sz)
				return -1;
			break;
		case VXTP_SEEK_CURRENT:
			pos = disk_head + offset;
			if ((pos < 0) || (pos > disk_sz))
				return -1;
			break;
		case SEEK_END:
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
	struct vxt_pirepheral *rom = vxtu_memory_create(&allocator, base, size, true);
	if (!vxtu_memory_device_fill(rom, data, size)) {
		LOG("vxtu_memory_device_fill() failed!\n");
		return NULL;
	}
	LOG("Loaded BIOS @ 0x%X-0x%X\n", base, base + size - 1);
	return rom;
}

static long long ustimer(void) {
    return (long long)js_ustimer();
}

static void speaker_callback(struct vxt_pirepheral *p, double freq, void *userdata) {
	(void)p; (void)userdata;
	js_speaker_callback(freq);
}

static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
    cga_width = width;
	cga_height = height;
    *(const vxt_byte**)userdata = rgba;
    return 0;
}

int video_width(void) {
	return cga_width;
}

int video_height(void) {
	return cga_height;
}

const void *video_rgba_memory_pointer(void) {
	vxtp_cga_snapshot(cga);

	vxt_dword color = vxtp_cga_border_color(cga);
	if (color != cga_border)
		js_set_border_color((cga_border = color));

	const vxt_byte *video_mem_buffer = NULL;
	vxtp_cga_render(cga, &render_callback, &video_mem_buffer);
	return (void*)video_mem_buffer;
}

void send_key(int scan) {
	vxtp_ppi_key_event(ppi, (enum vxtp_scancode)scan, false);
}

void step_emulation(int cycles) {
	struct vxt_step s = vxt_system_step(sys, cycles);
	if (s.err != VXT_NO_ERROR)
		LOG(vxt_error_str(s.err));
}

void initialize_emulator(void) {
	vxt_set_logger(&log_wrapper);

	struct vxtp_disk_interface interface = {
		&read_file, &write_file, &seek_file, &tell_file
	};
	
	struct vxt_pirepheral *disk = vxtp_disk_create(&allocator, &interface);
	struct vxt_pirepheral *pit = vxtp_pit_create(&allocator, &ustimer);
	
	ppi = vxtp_ppi_create(&allocator, pit);
	vxtp_ppi_set_speaker_callback(ppi, &speaker_callback, NULL);

    cga = vxtp_cga_create(&allocator, &ustimer);

	struct vxt_pirepheral *devices[] = {
		vxtu_memory_create(&allocator, 0x0, 0x100000, false),
        load_bios(get_pcxtbios_data(), (int)get_pcxtbios_size(), 0xFE000),
        load_bios(get_vxtx_data(), (int)get_vxtx_size(), 0xE0000),
        vxtp_pic_create(&allocator),
	    vxtp_dma_create(&allocator),
        pit,
        ppi,
		cga,
        disk,
		NULL
	};

	sys = vxt_system_create(&allocator, devices);
	vxt_system_initialize(sys);
	
	LOG("Installed pirepherals:\n");
	for (int i = 1; i < VXT_MAX_PIREPHERALS; i++) {
		struct vxt_pirepheral *device = vxt_system_pirepheral(sys, (vxt_byte)i);
		if (device)
			LOG("%d - %s\n", i, vxt_pirepheral_name(device));
	}

	int drive_num = (js_disk_size() > 1474560) ? 128 : 0;
	LOG("Disk num: %d", drive_num);

	vxt_error err = vxtp_disk_mount(disk, drive_num, (void*)1);
	if (err != VXT_NO_ERROR)
		LOG("%s (0x%X)", vxt_error_str(err), err);

	vxtp_disk_set_boot_drive(disk, drive_num);
	vxt_system_reset(sys);
}
