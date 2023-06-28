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
#include <stdio.h>
#include <string.h>

struct serial_dbg {
    char section_name[32];
    vxt_word base_port;
};

static vxt_byte in(struct serial_dbg *d, vxt_word port) {
    vxt_word reg = port - d->base_port;
    if (reg == 5)
        return 0x20; // Set transmission holding register empty (THRE); data can be sent.
    return 0;
}

static void out(struct serial_dbg *d, vxt_word port, vxt_byte data) {
    (void)d; (void)port;
    VXT_PRINT("%c", data);
}

static vxt_error install(struct serial_dbg *d, vxt_system *s) {
    vxt_system_install_io(s, VXT_GET_PIREPHERAL(d), d->base_port, d->base_port + 7);
    return VXT_NO_ERROR;
}

static vxt_error config(struct serial_dbg *d, const char *section, const char *key, const char *value) {
    if (!strcmp(d->section_name, section)) {
        if (strcmp("port", key))
            return VXT_NO_ERROR;
        sscanf(value, "%hx", &d->base_port);
    }
    return VXT_NO_ERROR;
}

static const char *name(struct serial_dbg *d) {
    (void)d; return "Serial Debug Printer";
}

VXTU_MODULE_CREATE(serial_dbg, {
    strncpy(DEVICE->section_name, ARGS, sizeof(DEVICE->section_name) - 1);

    PIREPHERAL->install = &install;
    PIREPHERAL->config = &config;
    PIREPHERAL->name = &name;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
