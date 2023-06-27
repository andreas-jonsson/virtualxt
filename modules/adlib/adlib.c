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
#include <frontend.h>
#include "nuked-opl3/opl3.h"

struct adlib {
	opl3_chip chip;
    int freq;
    vxt_byte index;
    vxt_byte reg4;
    
    bool (*set_audio_adapter)(const struct frontend_audio_adapter *adapter);
};

static vxt_byte in(struct adlib *a, vxt_word port) {
    if (port == 0x388) {
        vxt_byte status = ((a->reg4 & 2) << 4) | ((a->reg4 & 1) << 6);
        status |= status ? 0x80 : 0;
        return status;
    }
    return 0xFF;
}

static void out(struct adlib *a, vxt_word port, vxt_byte data) {
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

static vxt_int16 generate_sample(struct vxt_pirepheral *p, int freq) {
    struct adlib *a = VXT_GET_DEVICE(adlib, p);
    if (a->freq != freq) {
        a->freq = freq;
        OPL3_Reset(&a->chip, freq);
    }

    int16_t sample[2] = {0};
    OPL3_GenerateResampled(&a->chip, sample);
    return sample[0];
}

static vxt_error install(struct adlib *a, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(a);
    if (a->set_audio_adapter) {
        struct frontend_audio_adapter adapter = { p, &generate_sample };
        a->set_audio_adapter(&adapter);
    }

    vxt_system_install_io(s, p, 0x388, 0x389);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct adlib *a) {
    OPL3_Reset(&a->chip, a->freq);
    return VXT_NO_ERROR;
}

static const char *name(struct adlib *a) {
    (void)a; return "AdLib Music Synthesizer";
}

VXTU_MODULE_CREATE(adlib, {
    DEVICE->freq = 48000;
    if (FRONTEND)
        DEVICE->set_audio_adapter = ((struct frontend_interface*)FRONTEND)->set_audio_adapter;

    VXT_PIREPHERAL_SET_CALLBACK(install, install);
    VXT_PIREPHERAL_SET_CALLBACK(name, name);
    VXT_PIREPHERAL_SET_CALLBACK(reset, reset);
    VXT_PIREPHERAL_SET_CALLBACK(io.in, in);
    VXT_PIREPHERAL_SET_CALLBACK(io.out, out);
})
