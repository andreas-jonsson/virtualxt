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

#include <opl3.h>

VXT_PIREPHERAL(adlib, {
	opl3_chip chip;
    int freq;
    vxt_byte index;
    vxt_byte reg4;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(a, adlib, p);
    if (port == 0x388) {
        vxt_byte status = ((a->reg4 & 2) << 4) | ((a->reg4 & 1) << 6);
        status |= status ? 0x80 : 0;
        return status;
    }
    return 0xFF;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(a, adlib, p);
    switch (port) {
        case 0x388:
            a->index = data;
            break;
        case 0x389:
            if (port == 4)
                a->reg4 = data;
            OPL3_WriteRegBuffered(&a->chip, a->index, data);
            break;
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io(s, p, 0x388, 0x389);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(a, adlib, p);
    OPL3_Reset(&a->chip, a->freq);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(adlib, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p; return "AdLib Music Synthesizer";
}

struct vxt_pirepheral *vxtp_adlib_create(vxt_allocator *alloc) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(adlib));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(adlib));
    (VXT_GET_DEVICE(adlib, p))->freq = 48000;

    p->install = &install;
    p->destroy = &destroy;
    p->reset = &reset;
    p->name = &name;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

vxt_int16 vxtp_adlib_sample(struct vxt_pirepheral *p, int freq) {
    VXT_DEC_DEVICE(a, adlib, p);
    if (a->freq != freq) {
        a->freq = freq;
        OPL3_Reset(&a->chip, freq);
    }

    int16_t sample[2] = {0};
    OPL3_GenerateResampled(&a->chip, sample);
    return sample[0];
}
