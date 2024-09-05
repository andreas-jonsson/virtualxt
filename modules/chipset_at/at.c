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

static struct vxt_peripheral *pic_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)frontend; (void)args;
    return vxtu_pic_create(alloc);
}

static struct vxt_peripheral *dma_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)frontend; (void)args;
    return vxtu_dma_create(alloc);
}

static struct vxt_peripheral *pit_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)frontend; (void)args;
    return vxtu_pit_create(alloc);
}

struct vxt_peripheral *kbc_create(vxt_allocator *alloc);
bool kbc_key_event(struct vxt_peripheral *p, enum vxtu_scancode key, bool force);
vxt_int16 kbc_generate_sample(struct vxt_peripheral *p, int freq);

static struct vxt_peripheral *kbc_setup(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)args;

    struct vxt_peripheral *p = kbc_create(alloc);
    if (!p) return NULL;

	if (frontend) {
		struct frontend_interface *fi = (struct frontend_interface*)frontend;
		if (fi->set_keyboard_controller) {
			struct frontend_keyboard_controller controller = { p, &kbc_key_event };
			fi->set_keyboard_controller(&controller);
		}
		if (fi->set_audio_adapter) {
			struct frontend_audio_adapter adapter = { p, &kbc_generate_sample };
			fi->set_audio_adapter(&adapter);
		}
	}

    return p;
}

VXTU_MODULE_ENTRIES(&pic_create, &dma_create, &pit_create, &kbc_setup)
