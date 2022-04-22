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

#define TONE_VOLUME 32
#define UINT64 unsigned long long

VXT_PIREPHERAL(ppi, {
	vxt_byte data_port;
    vxt_byte command_port;
    vxt_byte port_61;

    UINT64 spk_sample_index;
	bool spk_enabled;
    void (*spk_enable_cb)(bool);
    void *spk_enable_ud;

    struct vxt_pirepheral *pit;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(c, ppi, p);
	switch (port) {
        case 0x60:
            c->command_port = 0;
            return c->data_port;
        case 0x61:
            return c->port_61;
        case 0x62:
            // Reference: https://bochs.sourceforge.io/techspec/PORTS.LST
            //            https://github.com/skiselev/8088_bios/blob/master/bios.asm
            return 0x2; // TODO: Return other then CGA video bits.
        case 0x64:
            return c->command_port;
	}
	return 0;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(c, ppi, p);
    if (port == 0x61) {
        c->port_61 = data;
        bool enabled = (data & 3) == 3;
        if (enabled != c->spk_enabled) {
            c->spk_enabled = enabled;
            c->spk_sample_index = 0;
            if (c->spk_enable_cb)
                c->spk_enable_cb(enabled);
        }
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io(s, p, 0x60, 0x62);
    vxt_system_install_io_at(s, p, 0x64);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(c, ppi, p);
    c->command_port = c->data_port = 0;
    c->port_61 = 4;

	c->spk_sample_index = 0;
	c->spk_enabled = false;

    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(ppi, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p; return "PPI (Intel 8255)";
}

struct vxt_pirepheral *vxtp_ppi_create(vxt_allocator *alloc, struct vxt_pirepheral *pit, void (*enable_spk)(bool), void *userdata) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(ppi));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(ppi));
    VXT_DEC_DEVICE(c, ppi, p);

    c->pit = pit;
    c->spk_enable_cb = enable_spk;
    c->spk_enable_ud = userdata;

    p->install = &install;
    p->destroy = &destroy;
    p->reset = &reset;
    p->name = &name;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

bool vxtp_ppi_key_event(struct vxt_pirepheral *p, enum vxtp_scancode key, bool force) {
    VXT_DEC_DEVICE(c, ppi, p);
    bool has_scan = c->command_port & 2;
    if (force || !has_scan) {
    	c->command_port |= 3;
		c->data_port = (vxt_byte)key;
        if (!has_scan)
            vxt_system_interrupt(VXT_GET_SYSTEM(ppi, p), 1);
        return true;
    }
    return false;
}

int vxtp_ppi_write_audio(struct vxt_pirepheral *p, vxt_byte *buffer, int freq, int channels, int samples) {
    VXT_DEC_DEVICE(c, ppi, p);

    int num_bytes = 0;
    double tone_hz = vxtp_pit_get_frequency(c->pit, 2);

    if (!c->spk_enabled || (tone_hz <= 0.0))
        return num_bytes;

    UINT64 square_wave_period = (UINT64)((double)freq / tone_hz);
    UINT64 half_square_wave_period = square_wave_period / 2;

    if (!half_square_wave_period)
        return num_bytes;

    for (int i = 0; i < samples; i++) {
        char sample_value = ((++c->spk_sample_index / half_square_wave_period) % 2) ? TONE_VOLUME : -TONE_VOLUME;
        for (int j = 0; j < channels; j++)
            buffer[num_bytes++] = (vxt_byte)sample_value;
    }
    return num_bytes;
}
