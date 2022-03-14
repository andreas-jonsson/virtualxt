/*
Copyright (c) 2019-2022 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <vxt/utils.h>
#include "common.h"

VXT_PIREPHERAL(pic, {
	vxt_byte mask_reg;
    vxt_byte request_reg;
    vxt_byte service_reg;
    vxt_byte icw_step;
    vxt_byte read_mode;
	vxt_byte icw[5];
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(c, pic, p);
	switch (port) {
        case 0x20:
            return c->read_mode ? c->service_reg : c->request_reg;
        case 0x21:
            return c->mask_reg;
	}
	return 0;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(c, pic, p);
	switch (port) {
        case 0x20:
            if (data & 0x10) {
                c->icw_step = 1;
                c->mask_reg = 0;
                c->icw[c->icw_step] = data;
                c->icw_step++;
                return;
            }

            if (((data & 0x98) == 8) && (data & 2))
                c->read_mode = data & 2;

            if (data & 0x20) {
                for (int i = 0; i < 8; i++) {
                    if ((c->service_reg >> i) & 1) {
                        c->service_reg ^= (1 << i);
                        if (!i)
                            c->request_reg |= 1;
                        return;
                    }
                }
            }
            break;
        case 0x21:
            if ((c->icw_step == 3) && (c->icw[1] & 2))
                c->icw_step = 4;

            if (c->icw_step < 5) {
                c->icw[c->icw_step] = data;
                c->icw_step++;
                return;
            }

            c->mask_reg = data;
            break;
	}
}

static int next(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(c, pic, p);
    vxt_byte has = c->request_reg & (~c->mask_reg);
	if (has) {
        for (vxt_byte i = 0; i < 8; i++) {
            if ((has >> i) & 1) {
                c->request_reg ^= (1 << i);
                c->service_reg |= (1 << i);
                return (int)c->icw[2] + i;
            }
        }
    }
    return -1;
}

static void irq(struct vxt_pirepheral *p, int n) {
    VXT_DEC_DEVICE(c, pic, p);
    c->request_reg |= (vxt_byte)(1 << n);
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io(s, p, 0x20, 0x21);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(c, pic, p);
    memclear(c, sizeof(struct pic));
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(pic, p))(p, 0);
    return VXT_NO_ERROR;
}

static enum vxt_pclass pclass(struct vxt_pirepheral *p) {
    UNUSED(p); return VXT_PCLASS_PIC;
}

static const char *name(struct vxt_pirepheral *p) {
    UNUSED(p); return "PIC (Intel 8259)";
}

struct vxt_pirepheral *vxtu_create_pic(vxt_allocator *alloc) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(pic));
    memclear(p, VXT_PIREPHERAL_SIZE(pic));

    p->install = &install;
    p->destroy = &destroy;
    p->reset = &reset;
    p->name = &name;
    p->pclass = &pclass;
    p->io.in = &in;
    p->io.out = &out;
    p->pic.next = &next;
    p->pic.irq = &irq;
    return p;
}
