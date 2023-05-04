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

#include <time.h>

#define CMOS_SIZE 50

VXT_PIREPHERAL(rtc, {
    vxt_byte addr;
 	vxt_byte cmos[CMOS_SIZE];

    bool enable_interrupt;
    vxt_byte update_progress;
    vxt_byte rate;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(c, rtc, p);
    if (!(port & 1))
        return c->addr;

    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    vxt_byte data = 0;
    switch (c->addr) {
        case 0x00:
            data = lt->tm_sec;
            break;
        case 0x02:
            data = lt->tm_min;
            break;
        case 0x04:
            data = lt->tm_hour;
            break;
        case 0x06:
            data = lt->tm_wday + 1;
            break;
        case 0x07:
            data = lt->tm_mday;
            break;
        case 0x08:
            data = lt->tm_mon + 1;
            break;
        case 0x09:
            data = lt->tm_year - 10;
            break;
        case 0x32:
            data = 19 + ((lt->tm_year - 10) / 100);
            break;
        case 0x0A: // Status A
            data = (c->update_progress ^= 0x80) | c->rate;
            break;
        case 0x0B: // Status B
            data = 0x6; // 24h format in binary.
            break;
        case 0x0C: // Status C
            // TODO: Cause of interrupt
            break;
        default:
            data = c->cmos[c->addr];
    }

    c->addr = 0xD;
    return data;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(c, rtc, p);
    if (!(port & 1)) {
        c->addr = data & 0x7F;
        if (c->addr >= CMOS_SIZE)
            c->addr = 0xD;
        return;
    }
    
    switch (c->addr) {
        case 0xA: // Status A
            c->rate = data & 0xF;
            break;
        case 0xB: // Status B
            c->enable_interrupt = (data & 0x40) != 0;
            break;
        default:
            c->cmos[c->addr] = data;
    }
    c->addr = 0xD;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io(s, p, 0x240, 0x241);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(c, rtc, p);
    c->addr = 0xD;
    c->rate = 5; // 1024 Hz
    c->enable_interrupt = false;
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "Real Time Clock";
}

VXTU_MODULE_CREATE(rtc, {
    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
