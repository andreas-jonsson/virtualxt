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

#define VXT_LIBC
#define VXTU_LIBC_IO
#include <vxt/vxtu.h>
#include <frontend.h>

#include <stdlib.h>

static struct vxt_pirepheral *disk_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)args;
    struct frontend_interface *fi = (struct frontend_interface*)frontend;
    struct vxt_pirepheral *p = vxtu_disk_create(alloc, &fi->disk.di);
    if (!p)
        return NULL;

    if (fi->set_disk_controller) {
		struct frontend_disk_controller c = {
			.device = p,
			.mount = &vxtu_disk_mount,
			.unmount = &vxtu_disk_unmount,
			.set_boot = &vxtu_disk_set_boot_drive
		};
		fi->set_disk_controller(&c);
    }

    vxtu_disk_set_activity_callback(p, fi->disk.activity_callback, fi->disk.userdata);
    return p;
}

static struct vxt_pirepheral *bios_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)frontend;
    if (!args[0]) {
		args = getenv("VXT_DEFAULT_VXTX_BIOS_PATH");
		if (!args) args = "bios/vxtx.bin";
	}

    int size = 0;
    vxt_byte *data = vxtu_read_file(alloc, args, &size);
    if (!data) {
        VXT_LOG("Could not load VirtualXT BIOS extension: %s", args);
        return NULL;
    }

    struct vxt_pirepheral *p = vxtu_memory_create(alloc, 0xE0000, size, true);
    vxtu_memory_device_fill(p, data, size);
    alloc(data, 0);
    return p;
}

VXTU_MODULE_ENTRIES(&disk_create, &bios_create)
