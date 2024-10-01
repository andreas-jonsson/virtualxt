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

// AT kbc controller
struct vxt_peripheral *kbc_create(vxt_allocator *alloc);
bool kbc_key_event(struct vxt_peripheral *p, enum vxtu_scancode key, bool force);
vxt_int16 kbc_generate_sample(struct vxt_peripheral *p, int freq);

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

static vxt_error ppi_config(void *dev, const char *section, const char *key, const char *value) {
    struct vxt_peripheral *p = VXT_GET_PERIPHERAL(dev);
    vxt_byte v = 0xFF;
    vxt_byte sw = vxtu_ppi_xt_switches(p);

    #define READ if (sscanf(value, "%hhx", &v) != 1) { VXT_LOG("ERROR: [switch1] %s = %s", key, value); return VXT_USER_ERROR(0); } else { v = ~v; }
    if (!strcmp("switch1", section)) {
        if (!strcmp("ram", key)) {
            READ vxtu_ppi_set_xt_switches(p, (sw & 0xF3) | ((v & 3) << 2));
        } else if (!strcmp("video", key)) {
            READ vxtu_ppi_set_xt_switches(p, (sw & 0xCF) | ((v & 3) << 4));
        } else if (!strcmp("floppy", key)) {
            READ vxtu_ppi_set_xt_switches(p, (sw & 0x3F) | ((v & 3) << 6));
        } else {
            VXT_LOG("ERROR: [switch1] %s", key);
            return VXT_USER_ERROR(1);
        }
    }
    #undef READ

    return VXT_NO_ERROR;
}

static struct vxt_peripheral *ppi_kbc_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)args;
	
	struct vxt_peripheral *p = NULL;
	bool (*key_event)(struct vxt_peripheral*,enum vxtu_scancode,bool) = NULL;
	vxt_int16 (*generate_sample)(struct vxt_peripheral*,int) = NULL;
	
	if (!strcmp(args, "at")) {
		VXT_LOG("AT Chipset selected!");
		if (!(p = kbc_create(alloc)))
			return NULL;

		key_event = &kbc_key_event;
		generate_sample = &kbc_generate_sample;
	} else {
		VXT_LOG("PC/XT Chipset selected!");
		if (!(p = vxtu_ppi_create(alloc)))
			return NULL;
		
		p->config = &ppi_config;
		
		key_event = &vxtu_ppi_key_event;
		generate_sample = &vxtu_ppi_generate_sample;
	}
	
	if (frontend) {
		struct frontend_interface *fi = (struct frontend_interface*)frontend;
		if (fi->set_keyboard_controller) {
			struct frontend_keyboard_controller controller = { p, key_event };
			fi->set_keyboard_controller(&controller);
		}
		if (fi->set_audio_adapter) {
			struct frontend_audio_adapter adapter = { p, generate_sample };
			fi->set_audio_adapter(&adapter);
		}
	}
    return p;
}

VXTU_MODULE_ENTRIES(&pic_create, &dma_create, &pit_create, &ppi_kbc_create)
