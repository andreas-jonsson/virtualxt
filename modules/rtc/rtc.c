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

struct rtc {
    vxt_byte addr;
    vxt_byte busy;

    struct tm lt;
 	vxt_byte cmos[CMOS_SIZE];
};

static vxt_byte in(struct rtc *c, vxt_word port) {
    if (!(port & 1))
        return c->addr;

    vxt_byte data = 0;    
    switch (c->addr) {
        case 0x0:
            data = c->lt.tm_sec;
            break;
        case 0x2:
            data = c->lt.tm_min;
            break;
        case 0x4:
            data = c->lt.tm_hour;
            break;
        case 0x6:
            data = c->lt.tm_wday + 1;
            break;
        case 0x7:
            data = c->lt.tm_mday;
            break;
        case 0x8:
            data = c->lt.tm_mon + 1;
            break;
        case 0x9:
            data = c->lt.tm_year - 10;
            break;
        case 0xA: // Status A
        {
            vxt_byte busy = c->busy;
            c->busy = 0;
            data = busy | (c->cmos[c->addr] & 0x7F);
            break;
        }
        case 0xB: // Status B
            // 24h format in binary
            data = 0x6;
            break;
        case 0xC: // Status C
            // Cause of interrupt
            break;
        case 0xD: // Status D
            // CMOS battery power good
            data = 0x80;
            break;
        default:
            data = c->cmos[c->addr];
    }

    c->addr = 0xD;
    return data;
}

static void out(struct rtc *c, vxt_word port, vxt_byte data) {
    if (!(port & 1)) {
        c->addr = data & 0x7F;
        if (c->addr >= CMOS_SIZE)
            c->addr = 0xD;
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
    vxt_system_install_io(s, p, 0x240, 0x241);
    vxt_system_install_timer(s, p, 1000000);
    return VXT_NO_ERROR;
}

static const char *name(struct rtc *c) {
    (void)c; return "RTC (Motorola MC146818)";
}

static struct vxt_pirepheral *rtc_create(vxt_allocator *alloc, void *frontend, const char *args) VXT_PIREPHERAL_CREATE(alloc, rtc, {
    (void)args; (void)frontend;

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})

static struct vxt_pirepheral *bios_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)frontend;
    if (!args[0])
        return NULL;

    int size = 0;
    vxt_byte *data = vxtu_read_file(alloc, args, &size);
    if (!data) {
        VXT_LOG("Could not load RTC BIOS: %s", args);
        return NULL;
    }

    struct vxt_pirepheral *p = vxtu_memory_create(alloc, 0xC8000, size, true);
    vxtu_memory_device_fill(p, data, size);
    alloc(data, 0);
    return p;
}

VXTU_MODULE_ENTRIES(&rtc_create, &bios_create)
