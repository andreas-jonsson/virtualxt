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
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    This product make use of the VirtualXT software emulator.
//    Visit https://virtualxt.org for more information.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <vxt/vxtu.h>

struct sampler {
	int frequency;
	float (*generate)(int freq);

	int buffer_size;
	int buffer_len;
	float *buffer;
};

static vxt_error timer(struct sampler *a, vxt_timer_id id, int cycles) {
	(void)id; (void)cycles;
	
	float sample = a->generate(a->frequency);
	if (a->buffer_len >= a->buffer_size)
		return VXT_NO_ERROR;

	a->buffer[a->buffer_len++] = sample;
    return VXT_NO_ERROR;
}

static vxt_error install(struct sampler *a, vxt_system *s) {
    vxt_system_install_timer(s, VXT_GET_PIREPHERAL(a), 1000000 / a->frequency);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct sampler *a) {
	vxt_allocator *alloc = vxt_system_allocator(VXT_GET_SYSTEM(a));	
    alloc(a->buffer, 0);
    alloc(VXT_GET_PIREPHERAL(a), 0);
    return VXT_NO_ERROR;
}

static const char *name(struct sampler *a) {
    (void)a; return "Audio Sampler";
}

float *sampler_get_buffer(struct vxt_pirepheral *p) {
	struct sampler *a = VXT_GET_DEVICE(sampler, p);
	while (a->buffer_len < a->buffer_size)
		a->buffer[a->buffer_len++] = a->generate(a->frequency);
	a->buffer_len = 0;
	return a->buffer;
}

struct vxt_pirepheral *sampler_create(vxt_allocator *alloc, int frequency, int num_samples, float (*generate)(int freq)) VXT_PIREPHERAL_CREATE(alloc, sampler, {
	DEVICE->frequency = frequency;
	DEVICE->generate = generate;
	
	DEVICE->buffer_len = 0;
	DEVICE->buffer_size = num_samples;
	DEVICE->buffer = (float*)alloc(NULL, num_samples * sizeof(float));

    PIREPHERAL->install = &install;
    PIREPHERAL->destroy = &destroy;
    PIREPHERAL->name = &name;
    PIREPHERAL->timer = &timer;
})
