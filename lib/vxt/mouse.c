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

#define BUFFER_SIZE 128

struct serial_mouse {
    int irq;
    vxt_word base_port;
    vxt_byte registers[8];

    vxt_byte buffer[BUFFER_SIZE];
    int buffer_len;
};

static bool push_data(struct serial_mouse *m, vxt_byte data) {
    if (m->buffer_len == BUFFER_SIZE)
        return false;
    else if (!m->buffer_len)
        vxt_system_interrupt(VXT_GET_SYSTEM(m), m->irq);
    m->buffer[m->buffer_len++] = data;
    return true;
}

static vxt_byte pop_data(struct serial_mouse *m) {
    vxt_byte data = *m->buffer;
    memmove(m->buffer, &m->buffer[1], --m->buffer_len);
    return data;
}

static vxt_byte in(struct serial_mouse *m, vxt_word port) {
	vxt_word reg = port & 7;
	switch (reg) {
        case 0: // Serial Data Register
        {
            vxt_byte data = 0;
            if (m->buffer_len) {
                data = pop_data(m);
                vxt_system_interrupt(VXT_GET_SYSTEM(m), m->irq);
            }
            return data;
        }
        case 5: // Line Status Register
            if (m->buffer_len)
                return 0x61;
            return 0x60;
	}
	return m->registers[reg];
}

static void out(struct serial_mouse *m, vxt_word port, vxt_byte data) {
	vxt_word reg = port & 7;
	m->registers[reg] = data;

    // Modem Control Register
	if ((reg == 4) && (data & 2)) {
        m->buffer_len = 0;
        push_data(m, 'M');
        VXT_LOG("Reset!");
	}
}

static vxt_error install(struct serial_mouse *m, vxt_system *s) {
    vxt_system_install_io(s, VXT_GET_PIREPHERAL(m), m->base_port, m->base_port + 7);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct serial_mouse *m) {
    vxt_memclear(m->registers, sizeof(m->registers));
    return VXT_NO_ERROR;
}

static const char *name(struct serial_mouse *m) {
    (void)m;
    return "Microsoft Serial Mouse";
}

VXT_API struct vxt_pirepheral *vxtu_mouse_create(vxt_allocator *alloc, vxt_word base_port, int irq) VXT_PIREPHERAL_CREATE(alloc, serial_mouse, {
    DEVICE->base_port = base_port;
    DEVICE->irq = irq;

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})

VXT_API bool vxtu_mouse_push_event(struct vxt_pirepheral *p, const struct vxtu_mouse_event *ev) {
    struct serial_mouse *m = VXT_GET_DEVICE(serial_mouse, p);
    vxt_byte upper = 0;
    if (ev->xrel < 0)
        upper = 0x3;
    if (ev->yrel < 0)
        upper |= 0xC;

    return push_data(m, 0x40 | ((ev->buttons & 3) << 4) | upper) &&
        push_data(m, (vxt_byte)(ev->xrel & 0x3F)) &&
        push_data(m, (vxt_byte)(ev->yrel & 0x3F));
}
