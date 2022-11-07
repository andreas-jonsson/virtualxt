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

#include <vxt/vxt.h>
#include <vxt/vxtu.h>
#include <vxtp.h>

#include <printf.h>

#include "main.h"

#define ALLOCATOR_SIZE (20 * 1024 * 1024)
vxt_byte allocator_data[ALLOCATOR_SIZE];
vxt_byte *allocator_ptr = allocator_data;

#define LOG(...) ( log_wrapper(__VA_ARGS__) )

vxt_system *sys = NULL;
struct vxt_pirepheral *disk = NULL;
struct vxt_pirepheral *ppi = NULL;
struct vxt_pirepheral *cga = NULL;

int log_wrapper(const char *fmt, ...) {
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
	allocator_ptr += sz;
	if (allocator_ptr >= (allocator_data + ALLOCATOR_SIZE)) {
		LOG("Allocator is out of memory!\n");
		return NULL;
	}
	return allocator_ptr;
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
    return 1;//(long long)(((double)clock() / CLOCKS_PER_SEC) * 1000000.0);
}
/*
static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
    (void)width; (void)height;
    *(const vxt_byte**)userdata = rgba;
    return 0;
}
*/
void *video_rgba_memory_pointer(void) {
	//vxtp_cga_snapshot(cga);
	/*
	const vxt_byte *video_mem_buffer = NULL;
	vxtp_cga_render(cga, &render_callback, &video_mem_buffer);
	*/
	static vxt_byte buffer[640*200*4];
	return buffer;//(void*)video_mem_buffer;
}

void step_emulation(int cycles) {
	(void)cycles;
}

void initialize_emulator(void) {
	vxt_set_logger(&log_wrapper);
	
	//struct vxt_pirepheral *disk = vxtp_disk_create(&vxt_clib_malloc, NULL);
	struct vxt_pirepheral *pit = vxtp_pit_create(&allocator, &ustimer);
	
	ppi = vxtp_ppi_create(&allocator, pit);
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
        //disk,
		NULL
	};

	sys = vxt_system_create(&allocator, devices);
	//vxt_system_initialize(sys);
	/*

	LOG("Installed pirepherals:\n");
	for (int i = 1; i < VXT_MAX_PIREPHERALS; i++) {
		struct vxt_pirepheral *device = vxt_system_pirepheral(sys, (vxt_byte)i);
		if (device)
			LOG("%d - %s\n", i, vxt_pirepheral_name(device));
	}

	vxt_system_reset(sys);
	*/
}
