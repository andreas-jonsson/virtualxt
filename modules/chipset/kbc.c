// Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

#include <vxt/vxtu.h>

#ifndef VXTU_PPI_TONE_VOLUME
    #define VXTU_PPI_TONE_VOLUME 8192
#endif

#define MAX_EVENTS 16
#define INT64 long long

#define DATA_READY 1
#define COMMAND_READY 2

#define DISABLE_KEYBOARD_CMD 0xAD
#define ENABLE_KEYBOARD_CMD 0xAE
#define READ_INPUT_CMD 0xD0
#define WRITE_OUTPUT_CMD 0xD1

struct kbc {
	vxt_byte data_port;
    vxt_byte command_port;
    vxt_byte port_61;

    vxt_byte command;
    bool keyboard_enable;

    int queue_size;
    enum vxtu_scancode queue[MAX_EVENTS];

    INT64 spk_sample_index;
	bool spk_enabled;

    struct vxt_peripheral *pit;
};

static vxt_byte in(struct kbc *c, vxt_word port) {
	switch (port) {
        case 0x60:
        {
            vxt_byte data = c->data_port;
            c->command_port = c->data_port = 0; // Not sure if the data port should be cleared on read?
            return data;
        }
        case 0x61:
            return c->port_61;
        case 0x64:
            return c->command_port;
	}
	return 0;
}

static void out(struct kbc *c, vxt_word port, vxt_byte data) {
    if (port == 0x61) {
        bool spk_enable = (data & 3) == 3;
        if (spk_enable != c->spk_enabled) {
            c->spk_enabled = spk_enable;
            c->spk_sample_index = 0;
        }

        bool kb_reset = !(c->port_61 & 0xC0) && (data & 0xC0);
        if (kb_reset) {
            c->queue_size = 0;
            c->data_port = 0xAA;
            c->command_port = COMMAND_READY | DATA_READY;
            vxt_system_interrupt(VXT_GET_SYSTEM(c), 1);
        }

        c->port_61 = data;
    }
}

static vxt_error install(struct kbc *c, vxt_system *s) {
    struct vxt_peripheral *p = VXT_GET_PERIPHERAL(c);
    vxt_system_install_io(s, p, 0x60, 0x62);
    vxt_system_install_io_at(s, p, 0x64);
    vxt_system_install_timer(s, p, 1000);

    for (int i = 0; i < VXT_MAX_PERIPHERALS; i++) {
        struct vxt_peripheral *ip = vxt_system_peripheral(s, (vxt_byte)i);
        if (ip && (vxt_peripheral_class(ip) == VXT_PCLASS_PIT)) {
            c->pit = ip;
            break;
        }
    }
    return c->pit ? VXT_NO_ERROR : VXT_USER_ERROR(0);
}

static vxt_error timer(struct kbc *c, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    c->command_port |= COMMAND_READY;
    if (c->keyboard_enable) {
        if (c->queue_size && !(c->command_port & DATA_READY)) {
            c->command_port |= DATA_READY;
            c->data_port = (vxt_byte)c->queue[0];
            memmove(c->queue, &c->queue[1], --c->queue_size);
            vxt_system_interrupt(VXT_GET_SYSTEM(c), 1);
        }
    } else if (c->command == READ_INPUT_CMD) {
        c->command_port |= DATA_READY;
    }
    return VXT_NO_ERROR;
}

static vxt_error reset(struct kbc *c) {
    c->command_port = c->data_port = 0;
    c->port_61 = 14;

	c->spk_sample_index = 0;
	c->spk_enabled = false;

    c->keyboard_enable = true;
    c->queue_size = 0;

    return VXT_NO_ERROR;
}

static const char *name(struct kbc *c) {
    (void)c; return "Keyboard Controller (Intel 8042)";
}

struct vxt_peripheral *kbc_create(vxt_allocator *alloc) VXT_PERIPHERAL_CREATE(alloc, kbc, {
    PERIPHERAL->install = &install;
    PERIPHERAL->reset = &reset;
    PERIPHERAL->timer = &timer;
    PERIPHERAL->name = &name;
    PERIPHERAL->io.in = &in;
    PERIPHERAL->io.out = &out;
})

bool kbc_key_event(struct vxt_peripheral *p, enum vxtu_scancode key, bool force) {
    struct kbc *c = VXT_GET_DEVICE(kbc, p);
    if (c->queue_size < MAX_EVENTS) {
        c->queue[c->queue_size++] = key;
        return true;
    } else if (force) {
        c->queue[MAX_EVENTS - 1] = key;
    }
    return false;
}

vxt_int16 kbc_generate_sample(struct vxt_peripheral *p, int freq) {
    struct kbc *c = VXT_GET_DEVICE(kbc, p);

    double tone_hz = vxtu_pit_get_frequency(c->pit, 2);
    if (!c->spk_enabled || (tone_hz <= 0.0))
        return 0;

    INT64 square_wave_period = (INT64)((double)freq / tone_hz);
    INT64 half_square_wave_period = square_wave_period / 2;

    if (!half_square_wave_period)
        return 0;
    return ((++c->spk_sample_index / half_square_wave_period) % 2) ? VXTU_PPI_TONE_VOLUME : -VXTU_PPI_TONE_VOLUME;
}
