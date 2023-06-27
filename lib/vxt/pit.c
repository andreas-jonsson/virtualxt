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

// Single step the counter for maximum BIOS compatibility.
#ifndef VXTU_PIT_COUNTER_STEP
    #define VXTU_PIT_COUNTER_STEP 1
#endif

#define TOGGLE_HIGH(ch) ( ((ch)->mode == MODE_TOGGLE) && (ch)->toggle )
#define TOGGLE_LOW(ch) ( ((ch)->mode == MODE_TOGGLE) && !(ch)->toggle )

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
	vxt_word counter;
    vxt_word data;
	vxt_byte mode;
};

#define INT64 long long

struct pit {
 	struct channel channels[3];
    INT64 ticks;
    INT64 device_ticks;
    
    // TODO: This is part of the transition code.
    INT64 ticker;
};

static vxt_byte in(struct pit *c, vxt_word port) {
	if (port == 0x43)
		return 0;

    vxt_word ret = 0;
	struct channel *ch = &c->channels[port & 3];

	if ((ch->mode == MODE_LATCH_COUNT) || (ch->mode == MODE_LOW_BYTE) || TOGGLE_LOW(ch))
		ret = ch->counter & 0xFF;
	else if ((ch->mode == MODE_HIGH_BYTE) || TOGGLE_HIGH(ch))
		ret = ch->counter >> 8;

	if ((ch->mode == MODE_LATCH_COUNT) || (ch->mode == MODE_TOGGLE))
		ch->toggle = !ch->toggle;

	return (vxt_byte)ret;
}

static void out(struct pit *c, vxt_word port, vxt_byte data) {
    if (port == 0x43) { // Mode/Command register.
        struct channel *ch = &c->channels[(data >> 6) & 3];
        ch->mode = (data >> 4) & 3;
        if (ch->mode == MODE_TOGGLE)
            ch->toggle = false;
        return;
    }

    struct channel *ch = &c->channels[port & 3];
    ch->enabled = true;

    if ((ch->mode == MODE_LOW_BYTE) || TOGGLE_LOW(ch))
        ch->data = (ch->data & 0xFF00) | ((vxt_word)data);
    else if ((ch->mode == MODE_HIGH_BYTE) || TOGGLE_HIGH(ch))
        ch->data = (ch->data & 0x00FF) | (((vxt_word)data) << 8);

    vxt_dword effective = (vxt_dword)ch->data;
    if (!ch->data && TOGGLE_HIGH(ch)) {
        ch->data = 0xFFFF;
        effective = 0x10000;
    }
    
    if (effective)
        ch->frequency = 1193182.0 / (double)effective;

    if (ch->mode == MODE_TOGGLE)
        ch->toggle = !ch->toggle;
}

static vxt_error install(struct pit *c, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(c);
    vxt_system_install_io(s, p, 0x40, 0x43);
    vxt_system_install_timer(s, p, 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct pit *c) {
    vxt_memclear(c->channels, sizeof(c->channels));
    c->ticker = c->device_ticks = c->ticks = 0;
    return VXT_NO_ERROR;
}

static vxt_error step(struct pit *c, INT64 ticks) {
    if (vxt_system_registers(VXT_GET_SYSTEM(c))->debug) {
        c->ticks = c->device_ticks = ticks;
        return VXT_NO_ERROR;
    }

    struct channel *ch = c->channels;
	if (ch->enabled && (ch->frequency > 0.0)) {
		INT64 next = 1000000000ll / (INT64)ch->frequency;
		if (ticks >= (c->ticks + next)) {
			c->ticks = ticks;
            #ifndef PI8088
                vxt_system_interrupt(VXT_GET_SYSTEM(c), 0);
            #endif
		}
	}

	const INT64 step = VXTU_PIT_COUNTER_STEP;
	const INT64 next = 1000000000ll / (1193182ll / step);

	if (ticks >= (c->device_ticks + next)) {
        for (int i = 0; i < 3; i++) {
            ch = &c->channels[i];
			if (ch->enabled)
				ch->counter = (vxt_word)((ch->counter < step) ? ch->data : (ch->counter - step));
		}
		c->device_ticks = ticks;
	}
    return VXT_NO_ERROR;
}

static vxt_error timer(struct pit *c, vxt_timer_id id, int cycles) {
    (void)id;
    c->ticker += (INT64)((double)cycles / ((double)vxt_system_frequency(VXT_GET_SYSTEM(c)) / 1000000000.0));
    return step(c, c->ticker);
}

static const char *name(struct pit *c) {
    (void)c; return "PIT (Intel 8253)";
}

static enum vxt_pclass pclass(struct pit *c) {
    (void)c; return VXT_PCLASS_PIT;
}

VXT_API struct vxt_pirepheral *vxtu_pit_create(vxt_allocator *alloc) VXT_PIREPHERAL_CREATE(alloc, pit, {
    VXT_PIREPHERAL_SET_CALLBACK(install, install);
    VXT_PIREPHERAL_SET_CALLBACK(name, name);
    VXT_PIREPHERAL_SET_CALLBACK(pclass, pclass);
    VXT_PIREPHERAL_SET_CALLBACK(timer, timer);
    VXT_PIREPHERAL_SET_CALLBACK(reset, reset);
    VXT_PIREPHERAL_SET_CALLBACK(io.in, in);
    VXT_PIREPHERAL_SET_CALLBACK(io.out, out);
})

VXT_API double vxtu_pit_get_frequency(struct vxt_pirepheral *p, int channel) {
    if ((channel > 2) || (channel < 0))
        return 0.0;
    return (VXT_GET_DEVICE(pit, p))->channels[channel].frequency;
}
