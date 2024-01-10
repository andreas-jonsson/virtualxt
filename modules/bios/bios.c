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

#include <vxt/vxtu.h>
#include <frontend.h>

#include <stdio.h>
#include <string.h>

static struct vxt_pirepheral *bios_create(vxt_allocator *alloc, void *frontend, const char *args) {
    vxt_pointer base;
    if (sscanf(args, "%x", &base) != 1) {
		VXT_LOG("Could not read BIOS address: %s", args);
		return NULL;
    }

	const char *path = strchr(args, ',');
	if (!path) {
		VXT_LOG("Invalid BIOS image path: %s", args);
		return NULL;
	}
    path += 1;

    struct frontend_interface *fi = (struct frontend_interface*)frontend;
    if (fi && fi->resolve_path)
        path = fi->resolve_path(FRONTEND_BIOS_PATH, path);

    int size = 0;
    vxt_byte *data = vxtu_read_file(alloc, path, &size);
    if (!data) {
        VXT_LOG("Could not load BIOS image: %s", path );
        return NULL;
    }

    struct vxt_pirepheral *p = vxtu_memory_create(alloc, base, size, true);
    vxtu_memory_device_fill(p, data, size);
    alloc(data, 0);
    return p;
}

VXTU_MODULE_ENTRIES(&bios_create)
