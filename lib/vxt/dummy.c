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

#include "common.h"

static vxt_byte in(void *d, vxt_word port) {
    UNUSED(d);
    LOG("reading unmapped IO port: %X", port);
    return 0xFF;
}

static void out(void *d, vxt_word port, vxt_byte data) {
    UNUSED(d); UNUSED(data);
    LOG("writing unmapped IO port: %X", port);
}

static vxt_byte read(void *d, vxt_pointer addr) {
    UNUSED(d);
    LOG("reading unmapped memory: %X", addr);
    return 0xFF;
}

static void write(void *d, vxt_pointer addr, vxt_byte data) {
    UNUSED(d); UNUSED(data);
    LOG("writing unmapped memory: %X", addr);
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *d) {
    vxt_system_install_io(s, d, 0x0, 0xFFFF);
    vxt_system_install_mem(s, d, 0x0, 0xFFFFF);
    return VXT_NO_ERROR;
}

void init_dummy_device(vxt_system *s, struct vxt_pirepheral *d) {
    UNUSED(s);
    d->install = &install;
    d->io.in = &in;
    d->io.out = &out;
    d->io.read = &read;
    d->io.write = &write;
}
