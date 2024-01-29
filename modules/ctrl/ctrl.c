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

struct ctrl {
 	vxt_byte ret;
    vxt_byte (*callback)(enum frontend_ctrl_command,vxt_byte,void*);
    void *userdata;
    
    int state;
};

static vxt_byte in(struct ctrl *c, vxt_word port) {
    if (!c->callback) {
        return 0;
    }
	return (port == 0xB5) ? c->callback(FRONTEND_CTRL_READ_DATA, 0, c->userdata) : c->ret;
}

static void out(struct ctrl *c, vxt_word port, vxt_byte data) {
    (void)port;
    if (!c->callback) {
        return;
	}
	c->ret = (port == 0xB5) ? c->callback(FRONTEND_CTRL_WRITE_DATA, data, c->userdata) : c->callback((enum frontend_ctrl_command)data, 0, c->userdata);
}

static vxt_error install(struct ctrl *c, vxt_system *s) {
    vxt_system_install_io(s, VXT_GET_PIREPHERAL(c), 0xB4, 0xB5);
    return VXT_NO_ERROR;
}

static const char *name(struct ctrl *c) {
    (void)c; return "Emulator Control";
}

VXTU_MODULE_CREATE(ctrl, {
    if (FRONTEND) {
        struct frontend_interface *fi = (struct frontend_interface*)FRONTEND;
        DEVICE->callback = fi->ctrl.callback;
        DEVICE->userdata = fi->ctrl.userdata;
    }

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
