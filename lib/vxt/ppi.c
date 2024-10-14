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

struct ppi {
	vxt_byte data_port;
    vxt_byte port_61;
    vxt_byte xt_switches;

    bool kb_reset;
    bool turbo_enabled;

    int queue_size;
    enum vxtu_scancode queue[MAX_EVENTS];

    INT64 spk_sample_index;
	bool spk_enabled;

    void (*speaker_callback)(struct vxt_peripheral*,double,void*);
    void *speaker_callback_data;

    struct vxt_peripheral *pit;
};

static vxt_byte in(struct ppi *c, vxt_word port) {
	switch (port) {
        case 0x60:
        {
            vxt_byte data = c->data_port;
            if (c->kb_reset) {
				c->kb_reset = false;
				c->data_port = 0;
			}
            return data;
        }
        case 0x61:
			c->port_61 ^= 0x10; // Toggle refresh bit.
            return c->port_61;
        case 0x62:
            return (c->port_61 & 8) ? (c->xt_switches >> 4) : (c->xt_switches & 0xF);
	}
	return 0;
}

static void out(struct ppi *c, vxt_word port, vxt_byte data) {
	if (port == 0x61) {
        bool spk_enable = (data & 3) == 3;
        if (spk_enable != c->spk_enabled) {
            c->spk_enabled = spk_enable;
            c->spk_sample_index = 0;

            if (c->speaker_callback)
                c->speaker_callback(VXT_GET_PERIPHERAL(c), spk_enable ? vxtu_pit_get_frequency(c->pit, 2) : 0.0, c->speaker_callback_data);
        }

        bool turbo_enabled = (data & 4) != 0;
        if (turbo_enabled != c->turbo_enabled) {
            c->turbo_enabled = turbo_enabled;
            VXT_LOG("Turbo mode %s!", turbo_enabled ? "on" : "off");
        }

		bool do_reset = !(c->port_61 & 0xC0) && (data & 0xC0);
		c->kb_reset = c->kb_reset || do_reset;
		
        if (c->kb_reset && (c->data_port != 0xAA)) {
            c->queue_size = 0;
            c->data_port = 0xAA;
            vxt_system_interrupt(VXT_GET_SYSTEM(c), 1);
            VXT_LOG("Keyboard reset!");
        }
        
        c->port_61 = data;
    }
}

static vxt_error install(struct ppi *c, vxt_system *s) {
    struct vxt_peripheral *p = VXT_GET_PERIPHERAL(c);
    vxt_system_install_io(s, p, 0x60, 0x63);
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

static enum vxt_pclass pclass(struct ppi *c) {
    (void)c; return VXT_PCLASS_PPI;
}

static vxt_error timer(struct ppi *c, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
	if (c->queue_size && !c->kb_reset) {
		c->data_port = (vxt_byte)c->queue[0];
		memmove(c->queue, &c->queue[1], --c->queue_size);
		vxt_system_interrupt(VXT_GET_SYSTEM(c), 1);
	}
    return VXT_NO_ERROR;
}

static vxt_error reset(struct ppi *c) {
    c->data_port = 0;
    c->port_61 = 14;
    c->turbo_enabled = false;

	c->spk_sample_index = 0;
	c->spk_enabled = false;

    if (c->speaker_callback)
        c->speaker_callback(VXT_GET_PERIPHERAL(c), 0.0, c->speaker_callback_data);

    c->kb_reset = false;
    c->queue_size = 0;

    return VXT_NO_ERROR;
}

static const char *name(struct ppi *c) {
    (void)c; return "PPI (Intel 8255)";
}

VXT_API struct vxt_peripheral *vxtu_ppi_create(vxt_allocator *alloc) VXT_PERIPHERAL_CREATE(alloc, ppi, {
    // Reference: https://bochs.sourceforge.io/techspec/PORTS.LST
    //            https://github.com/skiselev/8088_bios/blob/master/bios.asm
    DEVICE->xt_switches = 0x2E; // 640K ram, 80 column CGA, 1 floppy drive, no fpu.

    PERIPHERAL->install = &install;
    PERIPHERAL->reset = &reset;
    PERIPHERAL->timer = &timer;
    PERIPHERAL->name = &name;
    PERIPHERAL->pclass = &pclass;
    PERIPHERAL->io.in = &in;
    PERIPHERAL->io.out = &out;
})

VXT_API bool vxtu_ppi_key_event(struct vxt_peripheral *p, enum vxtu_scancode key, bool force) {
    struct ppi *c = VXT_GET_DEVICE(ppi, p);
    if (c->queue_size < MAX_EVENTS) {
        c->queue[c->queue_size++] = key;
        return true;
    } else if (force) {
        c->queue[MAX_EVENTS - 1] = key;
    }
    return false;
}

VXT_API bool vxtu_ppi_turbo_enabled(struct vxt_peripheral *p) {
    return (VXT_GET_DEVICE(ppi, p))->turbo_enabled;
}

VXT_API void vxtu_ppi_set_speaker_callback(struct vxt_peripheral *p, void (*f)(struct vxt_peripheral*,double,void*), void *userdata) {
    struct ppi *c = VXT_GET_DEVICE(ppi, p);
    c->speaker_callback = f;
    c->speaker_callback_data = userdata;
}

VXT_API vxt_int16 vxtu_ppi_generate_sample(struct vxt_peripheral *p, int freq) {
    struct ppi *c = VXT_GET_DEVICE(ppi, p);

    double tone_hz = vxtu_pit_get_frequency(c->pit, 2);
    if (!c->spk_enabled || (tone_hz <= 0.0))
        return 0;

    INT64 square_wave_period = (INT64)((double)freq / tone_hz);
    INT64 half_square_wave_period = square_wave_period / 2;

    if (!half_square_wave_period)
        return 0;
    return ((++c->spk_sample_index / half_square_wave_period) % 2) ? VXTU_PPI_TONE_VOLUME : -VXTU_PPI_TONE_VOLUME;
}

VXT_API void vxtu_ppi_set_xt_switches(struct vxt_peripheral *p, vxt_byte data) {
    (VXT_GET_DEVICE(ppi, p))->xt_switches = data;
}

VXT_API vxt_byte vxtu_ppi_xt_switches(struct vxt_peripheral *p) {
    return (VXT_GET_DEVICE(ppi, p))->xt_switches;
}
