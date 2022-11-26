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

#include "vxtp.h"

VXT_PIREPHERAL(ctrl, {
 	vxt_byte ret;
    vxt_byte (*callback)(enum vxtp_ctrl_command,void*);
    void *userdata;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(c, ctrl, p);
    (void)port;
    vxt_byte r = c->ret;
    c->ret = 0;
	return r;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(c, ctrl, p);
    (void)port;
    switch (data) {
        case 0: // Reset controller
            c->ret = 0;
            break;
        case 1: // Shutdown
            c->ret = c->callback(VXTP_CTRL_SHUTDOWN, c->userdata);
            break;
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io_at(s, p, 0xB4);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(ctrl, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "Emulator Control";
}

struct vxt_pirepheral *vxtp_ctrl_create(vxt_allocator *alloc, vxt_byte (*f)(enum vxtp_ctrl_command,void*), void *userdata) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(ctrl));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(ctrl));

    VXT_DEC_DEVICE(c, ctrl, p);
    c->callback = f;
    c->userdata = userdata;

    p->install = &install;
    p->destroy = &destroy;
    p->name = &name;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}
