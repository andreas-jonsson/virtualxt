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

struct post {
	int code;
};

static vxt_byte in(struct post *c, vxt_word port) {
    (void)port;
    return c->code;
}

static void out(struct post *c, vxt_word port, vxt_byte data) {
    (void)port;
    c->code = data;
    VXT_LOG("0x%01X", data);
}

static vxt_error install(struct post *c, vxt_system *s) {
	struct vxt_peripheral *p = VXT_GET_PERIPHERAL(c);
    vxt_system_install_io_at(s, p, 0x80);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct post *c) {
    c->code = 0;
    return VXT_NO_ERROR;
}

static const char *name(struct post *c) {
    (void)c; return "Post Card";
}

VXTU_MODULE_CREATE(post, {
    PERIPHERAL->install = &install;
    PERIPHERAL->reset = &reset;
    PERIPHERAL->name = &name;
    PERIPHERAL->io.in = &in;
    PERIPHERAL->io.out = &out;
})
