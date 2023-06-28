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

// Reference: http://www.fysnet.net/joystick.htm

#include <vxt/vxtu.h>
#include <frontend.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define POS_TO_OHM(axis) ((((double)((axis) + 1)) / ((double)UINT16_MAX)) * 60000.0)
#define AXIS_TIMEOUT(axis) ((24.2 + 0.011 * POS_TO_OHM(axis)) * 1000.0)

struct gameport_joystick {
    vxt_int16 axis[2];
    double timeouts[2];
    vxt_byte buttons;
};

struct joystick {
    double time_stamp;
    double ticker;
    struct gameport_joystick joysticks[2];

    vxt_word port;
    bool (*set_joystick_controller)(const struct frontend_joystick_controller*);
};

static vxt_byte in(struct joystick *g, vxt_word port) {
    (void)port;
    vxt_byte data = 0xF0;
    double d = g->ticker - g->time_stamp;

    for (int i = 0; i < 2; i++) {
        struct gameport_joystick *js = &g->joysticks[i];
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

static void out(struct joystick *g, vxt_word port, vxt_byte data) {
    (void)port; (void)data;
    g->time_stamp = g->ticker = 0.0;

    for (int i = 0; i < 2; i++) {
        struct gameport_joystick *js = &g->joysticks[i];
        js->timeouts[0] = AXIS_TIMEOUT(((int)js->axis[0]) - INT16_MIN);
        js->timeouts[1] = AXIS_TIMEOUT(((int)js->axis[1]) - INT16_MIN);
    }
}

static vxt_error timer(struct joystick *g, vxt_timer_id id, int cycles) {
    (void)id;
    if (g->ticker < 1000000.0)
        g->ticker += (double)cycles / ((double)vxt_system_frequency(VXT_GET_SYSTEM(g)) / 1000000.0);
    return VXT_NO_ERROR;
}

static bool push_event(struct vxt_pirepheral *p, const struct frontend_joystick_event *ev) {
    struct joystick *g = VXT_GET_DEVICE(joystick, p);
    struct gameport_joystick *js = &g->joysticks[ev->id];
    js->buttons = (vxt_byte)ev->buttons;
    js->axis[0] = ev->xaxis;
    js->axis[1] = ev->yaxis;
    return true;
}

static vxt_error install(struct joystick *g, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(g);
    if (g->set_joystick_controller) {
        struct frontend_joystick_controller jc = { p, &push_event };
        g->set_joystick_controller(&jc);
    }

    vxt_system_install_io_at(s, p, g->port);
    vxt_system_install_timer(s, p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct joystick *g) {
    (void)g; return "Gameport Joystick(s)";
}

static struct vxt_pirepheral *create(vxt_allocator *alloc, void *frontend, const char *args) VXT_PIREPHERAL_CREATE(alloc, joystick, {
    if (sscanf(args, "%hx", &DEVICE->port) != 1)
        DEVICE->port = 0x201;

    if (frontend)
        DEVICE->set_joystick_controller = ((struct frontend_interface*)frontend)->set_joystick_controller;

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
VXTU_MODULE_ENTRIES(&create)
