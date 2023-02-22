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

#include <vxt/vxt.h>
#include <vxt/vxtu.h>
#include <vxtp.h>
#include <printf.h>

#include "main.h"
#include "uart.h"
#include "timer.h"
#include "video.h"

int disk_head = 0;
vxt_byte *disk_data = NULL;

struct timer microsec;// = {0};

#define ALLOCATOR_SIZE (32 * 1024 * 1024)
vxtu_static_allocator(ALLOCATOR, ALLOCATOR_SIZE)

#define LOG(...) { printf(__VA_ARGS__);	_putchar('\n'); }

static int read_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s; (void)fp;

	int disk_sz = (int)get_disk_size();
	if ((disk_head + size) > disk_sz)
		size = disk_sz - disk_head;

	memcpy(buffer, disk_data + disk_head, size);
	disk_head += size;
	return size;
}

static int write_file(vxt_system *s, void *fp, vxt_byte *buffer, int size) {
	(void)s; (void)fp;

	int disk_sz = (int)get_disk_size();
	if ((disk_head + size) > disk_sz)
		size = disk_sz - disk_head;

	memcpy(disk_data + disk_head, buffer, size);
	disk_head += size;
	return size;
}

static int seek_file(vxt_system *s, void *fp, int offset, enum vxtu_disk_seek whence) {
	(void)s; (void)fp;

	int disk_sz = (int)get_disk_size();
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

static long long ustimer(void) {
    return (long long)timer_read(&microsec);
}

static const char *mgetline(void) {
	static char buffer[1024];
	int i = 0;
	for (*buffer = 0; buffer[i] != '\n'; i++)
		buffer[i] = uart_read_byte();
	buffer[i] = 0;
	return buffer;
}

static struct vxt_pirepheral *load_bios(const vxt_byte *data, int size, vxt_pointer base) {
	struct vxt_pirepheral *rom = vxtu_memory_create(&ALLOCATOR, base, size, true);
	if (!vxtu_memory_device_fill(rom, data, size)) {
		LOG("vxtu_memory_device_fill() failed!");
		return NULL;
	}
	LOG("Loaded BIOS @ 0x%X-0x%X", base, base + size - 1);
	return rom;
}

static int render_callback(int width, int height, const vxt_byte *rgba, void *userdata) {
	(void)userdata;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			const vxt_byte *offset = &rgba[(y * width + x) * 4];
			vxt_dword pixel = 0xFF000000 | ((vxt_dword)offset[3] << 16) | ((vxt_dword)offset[2] << 8) | (vxt_dword)offset[1];
			video_put_pixel(x, y, pixel);
		}
	}
    return 0;
}

/*
#define LED_GPFSEL      2
#define LED_GPFBIT      27
#define LED_GPSET       7
#define LED_GPCLR       10
#define LED_GPIO_BIT    29

void blink_act(void) {
	volatile unsigned int* gpio = (unsigned int*)0x3F200000UL;
    gpio[LED_GPFSEL] |= (1 << LED_GPFBIT);
    for (;;) {
        for (int tim = 0; tim < 500000; tim++) {}
        gpio[LED_GPCLR] = (1 << LED_GPIO_BIT);
        for (int tim = 0; tim < 500000; tim++) {}
        gpio[LED_GPSET] = (1 << LED_GPIO_BIT);
    }
}
*/

int ENTRY(int argc, char *argv[]) {
	(void)argc; (void)argv;

	//blink_act();

	uart_init();
	timer_init(&microsec, 1000000);
	if (!video_init(SCREEN_WIDTH, SCREEN_HEIGHT))
		LOG("Could not initialize video!");

	/*
	printf("hello world!\n");

	//vxt_dword col = 0;
	for (;;) {
		for (int y = 0; y < video_height(); y++) {
			for (int x = 0; x < video_width(); x++) {
				video_put_pixel(x, y, 0x555555);
			}
		}
		printf("one frame!\n");
	}
	for (;;) {};
	*/

	vxt_set_logger(&printf);

	disk_data = (vxt_byte*)ALLOCATOR(NULL, get_disk_size());
	memcpy(disk_data, get_disk_data(), get_disk_size());

	struct vxtu_disk_interface interface = {
		&read_file, &write_file, &seek_file, &tell_file
	};
	
	struct vxt_pirepheral *disk = vxtu_disk_create(&ALLOCATOR, &interface);
	struct vxt_pirepheral *pit = vxtu_pit_create(&ALLOCATOR, &ustimer);
	
	struct vxt_pirepheral *ppi = vxtu_ppi_create(&ALLOCATOR, pit);
	//vxtu_ppi_set_speaker_callback(ppi, &speaker_callback, NULL);

    struct vxt_pirepheral *cga = vxtu_cga_create(&ALLOCATOR, &ustimer);

	struct vxtu_debugger_interface dbgif = {NULL, &mgetline, &printf};
	struct vxt_pirepheral *dbg = vxtu_debugger_create(&ALLOCATOR, &dbgif);

	struct vxt_pirepheral *devices[] = {
		vxtu_memory_create(&ALLOCATOR, 0x0, 0x100000, false),
        load_bios(get_pcxtbios_data(), (int)get_pcxtbios_size(), 0xFE000),
        load_bios(get_vxtx_data(), (int)get_vxtx_size(), 0xE0000),
        vxtu_pic_create(&ALLOCATOR),
	    vxtu_dma_create(&ALLOCATOR),
		//vxtp_ctrl_create(&ALLOCATOR, &emu_control, NULL),
        pit,
        ppi,
		cga,
        disk,
		dbg,
		NULL
	};

	vxt_system *sys = vxt_system_create(&ALLOCATOR, VXT_DEFAULT_FREQUENCY, devices);
	vxt_system_initialize(sys);
	
	LOG("Installed pirepherals:");
	for (int i = 1; i < VXT_MAX_PIREPHERALS; i++) {
		struct vxt_pirepheral *device = vxt_system_pirepheral(sys, (vxt_byte)i);
		if (device)
			LOG("%d - %s", i, vxt_pirepheral_name(device));
	}

	int drive_num = (get_disk_size() > 1474560) ? 128 : 0;
	LOG("Disk num: %d", drive_num);

	vxt_error err = vxtu_disk_mount(disk, drive_num, (void*)1);
	if (err != VXT_NO_ERROR)
		LOG("%s (0x%X)", vxt_error_str(err), err);

	vxtu_disk_set_boot_drive(disk, drive_num);
	vxt_system_reset(sys);

	//struct vxt_registers *reg = vxt_system_registers(sys);
	//reg->debug = true;

	for (;;) {
		// TODO: Fix propper timing.
		struct vxt_step s = vxt_system_step(sys, VXT_DEFAULT_FREQUENCY / 60);
		if (s.err != VXT_NO_ERROR)
			LOG(vxt_error_str(s.err));
		
		vxtu_cga_snapshot(cga);
		//vxt_dword color = vxtu_cga_border_color(cga);
		//if (color != cga_border)
		//	js_set_border_color((cga_border = color));
		vxtu_cga_render(cga, &render_callback, NULL);
	}
	return 0;
}

void exception_handler(unsigned int n) {
	LOG("Exception: 0x%X", n);
}
