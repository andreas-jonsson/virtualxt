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

// Reference: http://www.fysnet.net/joystick.htm

#include "vxtp.h"

#include <stdint.h>

#define POS_TO_OHM(axis) ((((double)((axis) + 1)) / ((double)UINT16_MAX)) * 60000.0)
#define AXIS_TIMEOUT(axis) ((24.2 + 0.011 * POS_TO_OHM(axis)) * 1000.0)

#define INT64 long long

struct joystick {
    void *id;
    vxt_int16 axis[2];
    double timeouts[2];
    vxt_byte buttons;
};

VXT_PIREPHERAL(gameport, {
    INT64 time_stamp;
    struct joystick joysticks[2];
    INT64 (*ustics)(void);
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    (void)port;
    VXT_DEC_DEVICE(g, gameport, p);
    vxt_byte data = 0xF0;
    double d = (double)(g->ustics() - g->time_stamp);

    for (int i = 0; i < 2; i++) {
        struct joystick *js = &g->joysticks[i];
        if (!js->id)
            continue;

        int shift = i * 2;
        if ((js->timeouts[0] -= d) > 0.0) // X-Axis
            data |= 1 << shift;
        else
            js->timeouts[0] = 0.0;

        if ((js->timeouts[1] -= d) > 0.0) // Y-Axis
            data |= 2 << shift;
        else
            js->timeouts[1] = 0.0;

        data ^= js->buttons << (4 + shift);
    }
	return data;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    (void)port; (void)data;
    VXT_DEC_DEVICE(g, gameport, p);
    g->time_stamp = g->ustics();

    for (int i = 0; i < 2; i++) {
        struct joystick *js = &g->joysticks[i];
        js->timeouts[0] = AXIS_TIMEOUT(((int)js->axis[0]) - INT16_MIN);
        js->timeouts[1] = AXIS_TIMEOUT(((int)js->axis[1]) - INT16_MIN);
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io_at(s, p, 0x201);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(gameport, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "Gameport Joystick Controller";
}

struct vxt_pirepheral *vxtp_joystick_create(vxt_allocator *alloc, INT64 (*ustics)(void), void *stick_a, void *stick_b) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(gameport));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(gameport));
    VXT_DEC_DEVICE(g, gameport, p);

    g->ustics = ustics;
    g->joysticks[0].id = stick_a;
    g->joysticks[1].id = stick_b;

    p->install = &install;
    p->destroy = &destroy;
    p->name = &name;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

bool vxtp_joystick_push_event(struct vxt_pirepheral *p, const struct vxtp_joystick_event *ev) {
    VXT_DEC_DEVICE(g, gameport, p);
    for (int i = 0; i < 2; i++) {
        struct joystick *js = &g->joysticks[i];
        if (js->id == ev->id) {
            js->buttons = (vxt_byte)ev->buttons;
            js->axis[0] = ev->xaxis;
            js->axis[1] = ev->yaxis;
            return true;
        }
    }
    return false;
}
