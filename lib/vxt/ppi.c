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

VXT_PIREPHERAL(ppi, {
	vxt_byte data_port;
    vxt_byte command_port;
    vxt_byte port_61;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(c, ppi, p);
	switch (port) {
        case 0x60:
            c->command_port = 0;
            return c->data_port;
        case 0x61:
            return c->port_61;
        case 0x62:
            // Reference: https://bochs.sourceforge.io/techspec/PORTS.LST
            //            https://github.com/skiselev/8088_bios/blob/master/bios.asm

            LOG("reading DIP switches!");
            return 0x3; // TODO: Return other then MDA video bits.
        case 0x64:
            return c->command_port;
	}
	return 0;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    if (port == 0x61)
        (VXT_GET_DEVICE(ppi, p))->port_61 = data;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io_at(s, p, 0x60);
    vxt_system_install_io_at(s, p, 0x61);
    vxt_system_install_io_at(s, p, 0x62);
    vxt_system_install_io_at(s, p, 0x64);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(c, ppi, p);
    c->command_port = c->data_port = 0;
    c->port_61 = 4;
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(ppi, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    UNUSED(p); return "PPI (Intel 8255)";
}

struct vxt_pirepheral *vxtu_create_ppi(vxt_allocator *alloc) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(ppi));
    memclear(p, VXT_PIREPHERAL_SIZE(ppi));

    p->install = &install;
    p->destroy = &destroy;
    p->reset = &reset;
    p->name = &name;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

bool vxtu_ppi_key_event(struct vxt_pirepheral *p, enum vxtu_scancode key, bool force) {
    VXT_DEC_DEVICE(c, ppi, p);
    bool has_scan = c->command_port & 2;
    if (force || !has_scan) {
    	c->command_port |= 2;
		c->data_port = (vxt_byte)key;
        if (!has_scan)
            vxt_system_interrupt(VXT_GET_SYSTEM(ppi, p), 1);
        return true;
    }
    return false;
}
