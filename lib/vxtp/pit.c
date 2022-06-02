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

#include "vxtp.h"

enum chmode {
    MODE_LATCH_COUNT,
    MODE_LOW_BYTE,
    MODE_HIGH_BYTE,
    MODE_TOGGLE
};

struct channel {
	bool enabled;
    bool toggle;
	double frequency;
	vxt_dword effective;
	vxt_word counter;
    vxt_word data;
	vxt_byte mode;
};

#define INT64 long long

VXT_PIREPHERAL(pit, {
 	struct channel channels[3];
    INT64 ticks;
    INT64 device_ticks;

    INT64 (*get_ticks)(void);
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(c, pit, p);
	if (port == 0x43)
		return 0;

    vxt_word ret = 0;
	struct channel *ch = &c->channels[port & 3];

	if ((ch->mode == MODE_LATCH_COUNT) || (ch->mode == MODE_LOW_BYTE) || ((ch->mode == MODE_TOGGLE) && !ch->toggle))
		ret = ch->counter & 0xFF;
	else if ((ch->mode == MODE_HIGH_BYTE) || ((ch->mode == MODE_TOGGLE) && ch->toggle))
		ret = ch->counter >> 8;

	if ((ch->mode == MODE_LATCH_COUNT) || (ch->mode == MODE_TOGGLE))
		ch->toggle = !ch->toggle;

	return (vxt_byte)ret;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(c, pit, p);
    if (port == 0x43) { // Mode/Command register.
        struct channel *ch = &c->channels[(data >> 6) & 3];
        ch->mode = (data >> 4) & 3;
        if (ch->mode == MODE_TOGGLE)
            ch->toggle = false;
        return;
    }

    struct channel *ch = &c->channels[port & 3];
    ch->enabled = true;

    if ((ch->mode == MODE_LOW_BYTE) || ((ch->mode == MODE_TOGGLE) && !ch->toggle))
        ch->data = (ch->data & 0xFF00) | ((vxt_word)data);
    else if ((ch->mode == MODE_HIGH_BYTE) || ((ch->mode == MODE_TOGGLE) && ch->toggle))
        ch->data = (ch->data & 0x00FF) | (((vxt_word)data) << 8);

    ch->effective = ch->data ? (vxt_dword)ch->data : 65536;
    ch->frequency = 1193182.0 / (double)ch->effective;

    if (ch->mode == MODE_TOGGLE)
        ch->toggle = !ch->toggle;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io(s, p, 0x40, 0x43);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(pit, p))(p, 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(c, pit, p);
    vxt_memclear(c->channels, sizeof(c->channels));
    c->device_ticks = c->ticks = 0;
    return VXT_NO_ERROR;
}

static vxt_error step(struct vxt_pirepheral *p, int cycles) {
    (void)cycles;
    VXT_DEC_DEVICE(c, pit, p);

    INT64 ticks = c->get_ticks();
    if (vxt_system_registers(VXT_GET_SYSTEM(pit, p))->debug) {
        c->ticks = c->device_ticks = ticks;
        return VXT_NO_ERROR;
    }

    struct channel *ch = c->channels;
	if (ch->enabled && (ch->frequency > 0.0)) {
		INT64 next = 1000000ll / (INT64)ch->frequency;
		if (ticks >= (c->ticks + next)) {
			c->ticks = ticks;
            #ifndef PI8088
                vxt_system_interrupt(VXT_GET_SYSTEM(pit, p), 0);
            #endif
		}
	}

	const INT64 step = 10;
	const INT64 next = 1000000 / (1193182 / step);

	if (ticks >= (c->device_ticks + next)) {
        for (int i = 0; i < 3; i++) {
            ch = &c->channels[i];
			if (ch->enabled)
				ch->counter = (ch->counter < step) ? ch->data : (ch->counter - step);
		}
		c->device_ticks = ticks;
	}
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "PIT (Intel 8253)";
}

struct vxt_pirepheral *vxtp_pit_create(vxt_allocator *alloc, INT64 (*ustics)(void)) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(pit));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(pit));
    (VXT_GET_DEVICE(pit, p))->get_ticks = ustics;

    p->install = &install;
    p->destroy = &destroy;
    p->name = &name;
    p->reset = &reset;
    p->step = &step;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

double vxtp_pit_get_frequency(struct vxt_pirepheral *p, int channel) {
    if ((channel > 2) || (channel < 0))
        return 0.0;
    return (VXT_GET_DEVICE(pit, p))->channels[channel].frequency;
}
