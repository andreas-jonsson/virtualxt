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

#define PIT_FREQUENCY 1.193182
#define TOGGLE_HIGH(ch) ( ((ch)->mode == MODE_LATCH_COUNT || (ch)->mode == MODE_TOGGLE) && (ch)->toggle )
#define TOGGLE_LOW(ch) ( ((ch)->mode == MODE_LATCH_COUNT || (ch)->mode == MODE_TOGGLE) && !(ch)->toggle )

enum chmode {
    MODE_LATCH_COUNT,
    MODE_LOW_BYTE,
    MODE_HIGH_BYTE,
    MODE_TOGGLE
};

struct channel {
	bool enabled;
    bool toggle;

	vxt_word counter;
    vxt_word latch;
    vxt_word data;
	vxt_byte mode;
};

struct pit {
 	struct channel channels[3];
    double ticker;
};

static vxt_byte in(struct pit *c, vxt_word port) {
	if (port == 0x43)
		return 0;

    vxt_word ret = 0;
	struct channel *ch = &c->channels[port & 3];

    if (ch->mode != MODE_LATCH_COUNT) {
        ch->latch = ch->counter;
    }

	if ((ch->mode == MODE_LOW_BYTE) || TOGGLE_LOW(ch))
		ret = ch->latch & 0xFF;
	else if ((ch->mode == MODE_HIGH_BYTE) || TOGGLE_HIGH(ch))
		ret = ch->latch >> 8;

	if ((ch->mode == MODE_LATCH_COUNT) || (ch->mode == MODE_TOGGLE))
		ch->toggle = !ch->toggle;

	return (vxt_byte)ret;
}

static void out(struct pit *c, vxt_word port, vxt_byte data) {
    // Mode/Command register.
    if (port == 0x43) {
        struct channel *ch = &c->channels[(data >> 6) & 3];
        
        ch->toggle = false;
        ch->mode = (data >> 4) & 3;
        if (ch->mode == MODE_LATCH_COUNT)
            ch->latch = ch->counter;
        
        return;
    }

    struct channel *ch = &c->channels[port & 3];
    if ((ch->mode == MODE_LOW_BYTE) || TOGGLE_LOW(ch)) {
        ch->data = (ch->data & 0xFF00) | ((vxt_word)data);
        ch->enabled = false;
    } else if ((ch->mode == MODE_HIGH_BYTE) || TOGGLE_HIGH(ch)) {
        ch->data = (ch->data & 0x00FF) | (((vxt_word)data) << 8);
        ch->enabled = true;
        if (!ch->data)
            ch->data = 0xFFFF;
    }

    if ((ch->mode == MODE_TOGGLE) || (ch->mode == MODE_LATCH_COUNT))
        ch->toggle = !ch->toggle;
}

static vxt_error install(struct pit *c, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(c);
    vxt_system_install_io(s, p, 0x40, 0x43);
    vxt_system_install_timer(s, p, 0);

    enum vxt_monitor_flag flags = VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_DECIMAL;
    vxt_system_install_monitor(s, p, "Channel 0", &c->channels[0].counter, flags);
    vxt_system_install_monitor(s, p, "Channel 1", &c->channels[1].counter, flags);
    vxt_system_install_monitor(s, p, "Channel 1", &c->channels[2].counter, flags);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct pit *c) {
    vxt_memclear(c, sizeof(struct pit));
    return VXT_NO_ERROR;
}

static vxt_error timer(struct pit *c, vxt_timer_id id, int cycles) {
    (void)id;

    // Elapsed time in microseconds.
    c->ticker += (double)cycles * (1000000.0 / (double)vxt_system_frequency(VXT_GET_SYSTEM(c)));

    const double interval = 1.0 / PIT_FREQUENCY;
    while (c->ticker >= interval) {
        for (int i = 0; i < 3; i++) {
            struct channel *ch = &c->channels[i];
            if (!ch->enabled)
                continue;

            if (ch->counter-- == 0) {
                ch->counter = ch->data;
                if (i == 0)
                    vxt_system_interrupt(VXT_GET_SYSTEM(c), 0);
            }
        }
        c->ticker -= interval;
    }
    return VXT_NO_ERROR;
}

static const char *name(struct pit *c) {
    (void)c; return "PIT (Intel 8253)";
}

static enum vxt_pclass pclass(struct pit *c) {
    (void)c; return VXT_PCLASS_PIT;
}

VXT_API struct vxt_pirepheral *vxtu_pit_create(vxt_allocator *alloc) VXT_PIREPHERAL_CREATE(alloc, pit, {
    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->pclass = &pclass;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})

VXT_API double vxtu_pit_get_frequency(struct vxt_pirepheral *p, int channel) {
    if ((channel > 2) || (channel < 0))
        return 0.0;

    double fd = (double)(VXT_GET_DEVICE(pit, p))->channels[channel].data;
    if (fd == 0.0) fd = (double)0x10000;
    return (PIT_FREQUENCY * 1000000.0) / (double)fd;
}
