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

// Reference: https://archive.org/stream/dss-programmers-guide/dss-programmers-guide_djvu.txt

#include <vxt/vxtu.h>
#include <frontend.h>

#include <stdio.h>

enum device_type {
	COVOX,
	DISNEY
};

struct covox {
	vxt_word base_port;
	enum device_type type;

	vxt_int8 sample;
	vxt_byte data;
	vxt_byte control;

	int fifo_len;
	int fifo_max;
	vxt_byte fifo[16];

	bool (*set_audio_adapter)(const struct frontend_audio_adapter *adapter);
};

static void push_data(struct covox *c, vxt_byte data) {
    if (c->fifo_len == c->fifo_max)
        return;
    c->fifo[c->fifo_len++] = data;
}

static vxt_int16 generate_sample(struct vxt_peripheral *p, int freq) {
	(void)freq;
    return VXT_GET_DEVICE(covox, p)->sample * 64;
}

static vxt_byte in(struct covox *c, vxt_word port) {
	switch (port - c->base_port) {
		case 0: return c->data;
		case 1: return (c->fifo_len == c->fifo_max) ? 0x40 : 0x0;
		default: return 0;
	};
}

static void out(struct covox *c, vxt_word port, vxt_byte data) {
	switch (port - c->base_port) {
		case 0:
			push_data(c, data);
			c->data = data;
			break;
		case 2:
			if ((data & 8) && !(c->control & 8))
				push_data(c, c->data);
			c->control = data;
			break;
	};
}

static vxt_error timer(struct covox *c, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    c->sample = (vxt_int8)(c->fifo_len ? *c->fifo : 0);
	if (c->fifo_len)
		memmove(c->fifo, &c->fifo[1], --c->fifo_len);
    return VXT_NO_ERROR;
}

static vxt_error install(struct covox *c, vxt_system *s) {
	struct vxt_peripheral *p = VXT_GET_PERIPHERAL(c);
    if (c->set_audio_adapter) {
        struct frontend_audio_adapter adapter = { p, &generate_sample };
        c->set_audio_adapter(&adapter);
    }

    vxt_system_install_io(s, p, c->base_port, c->base_port + 3);
	vxt_system_install_timer(s, p, 142); // ~7kHz
    return VXT_NO_ERROR;
}

static vxt_error reset(struct covox *c) {
	c->fifo_len = 0;
    return VXT_NO_ERROR;
}

static const char *name(struct covox *c) {
    return (c->type == COVOX) ? "Covox Speech Thing" : "Disney Sound Source";
}

VXTU_MODULE_CREATE(covox, {
    if (sscanf(ARGS, "%hx", &DEVICE->base_port) != 1) {
		VXT_LOG("Invalid LPT address: %s", ARGS);
		return NULL;
    }

	const char *type = strchr(ARGS, ',');
	if (!type) {
		VXT_LOG("Invalid type parameter: %s", ARGS);
		return NULL;
	}

	type = type + 1;
	if (!strcmp(type, "disney")) {
		DEVICE->type = DISNEY;
		DEVICE->fifo_max = 16;
	//} else if (!strcmp(type, "covox")) {
	//	DEVICE->type = COVOX;
	//	DEVICE->fifo_max = 1;
	} else {
		VXT_LOG("Invalid device type: %s", type);
		return NULL;
	}

	if (FRONTEND)
        DEVICE->set_audio_adapter = ((struct frontend_interface*)FRONTEND)->set_audio_adapter;

    PERIPHERAL->install = &install;
	PERIPHERAL->reset = &reset;
	PERIPHERAL->timer = &timer;
    PERIPHERAL->name = &name;
    PERIPHERAL->io.in = &in;
    PERIPHERAL->io.out = &out;
})
