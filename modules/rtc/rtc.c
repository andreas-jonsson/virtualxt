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
#include <stdio.h>

#define CMOS_SIZE 50

struct rtc {
    vxt_byte addr;
    vxt_byte busy;

    struct tm lt;
 	vxt_byte cmos[CMOS_SIZE];
    vxt_word base_port;
};

static vxt_byte to_bcd(struct rtc *c, vxt_byte data) {
    if (!(c->cmos[0xB] & 4)) {
		vxt_byte rh, rl;
		rh = (data / 10) % 10;
		rl = data % 10;
		data = (rh << 4) | rl;
    }
    return data;
}

static vxt_byte in(struct rtc *c, vxt_word port) {
    if (!(port & 1))
        return c->addr;

    vxt_byte data = 0;    
    switch (c->addr) {
        case 0x0:
            data = to_bcd(c, (vxt_byte)c->lt.tm_sec);
            break;
        case 0x2:
            data = to_bcd(c, (vxt_byte)c->lt.tm_min);
            break;
        case 0x4:
            data = to_bcd(c, (vxt_byte)c->lt.tm_hour);
            break;
        case 0x6:
            data = to_bcd(c, (vxt_byte)c->lt.tm_wday + 1);
            break;
        case 0x7:
            data = to_bcd(c, (vxt_byte)c->lt.tm_mday);
            break;
        case 0x8:
            data = to_bcd(c, (vxt_byte)c->lt.tm_mon + 1);
            break;
        case 0x9:
            data = to_bcd(c, (vxt_byte)(c->lt.tm_year - 100));
            break;
        case 0xA: // Status A
            data = c->busy | (c->cmos[c->addr] & 0x7F);
            c->busy = 0;
            break;
        case 0xB: // Status B
            data = c->cmos[c->addr] & 0xFD; // 24h format only.
            break;
        case 0xD: // Status D
            // CMOS battery power good
            data = 0x80;
            break;
        case 0x32:
            data = to_bcd(c, 20);
            break;
        default:
            data = c->cmos[c->addr];
    }

    c->addr = 0xD;
    return data;
}

static void out(struct rtc *c, vxt_word port, vxt_byte data) {
    if (!(port & 1)) {
        c->addr = (c->addr >= CMOS_SIZE) ? 0xD : data;
        return;
    }

    c->cmos[c->addr] = data;
    c->addr = 0xD;
}

static vxt_error timer(struct rtc *c, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    time_t t = time(NULL);
    c->lt = *localtime(&t);
    c->busy = 0x80;
    return VXT_NO_ERROR;
}

static vxt_error install(struct rtc *c, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(c);
    vxt_system_install_io(s, p, c->base_port, c->base_port + 1);
    vxt_system_install_timer(s, p, 1000000);
    return VXT_NO_ERROR;
}

static const char *name(struct rtc *c) {
    (void)c; return "RTC (Motorola MC146818)";
}

VXTU_MODULE_CREATE(rtc, {
    if (sscanf(ARGS, "%hx", &DEVICE->base_port) != 1)
        DEVICE->base_port = 0x240;

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
