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

#include <vxt/vxtu.h>
#include <frontend.h>

#include <stdio.h>

static struct vxt_pirepheral *mouse_create(vxt_allocator *alloc, void *frontend, const char *args) {
    vxt_word addr = 0x3F8;
    int irq = 4;
    sscanf(args, "%hx,%d", &addr, &irq);

    struct vxt_pirepheral *p = vxtu_mouse_create(alloc, addr, irq);
    if (!p)
        return NULL;

    struct frontend_interface *fi = (struct frontend_interface*)frontend;
    if (fi->set_mouse_adapter) {
		struct frontend_mouse_adapter a = { p, &vxtu_mouse_push_event };
		fi->set_mouse_adapter(&a);
    }
    return p;
}

VXTU_MODULE_ENTRIES(&mouse_create)
