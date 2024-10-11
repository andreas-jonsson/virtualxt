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

struct a20 {
	bool available;
	vxt_byte port_92;
};

static vxt_byte in(struct a20 *c, vxt_word port) {
	(void)port;
	return c->port_92;
}

static void out(struct a20 *c, vxt_word port, vxt_byte data) {
	(void)port;
	bool enable_a20 = (data & 2) != 0;
	if ((c->port_92 & 2) != (data & 2))
		VXT_LOG(enable_a20 ? "Enable Fast-A20 line!" : "Disable Fast-A20 line!");
		
	vxt_system_set_a20(VXT_GET_SYSTEM(c), enable_a20);
	c->port_92 = data;
}

static vxt_error config(struct a20 *c, const char *section, const char *key, const char *value) {
	if (!strcmp(section, "args") && !strcmp(key, "a20"))
		c->available = atoi(value) != 0;
	return VXT_NO_ERROR;
}

static vxt_error install(struct a20 *c, vxt_system *s) {
	if (c->available)
		vxt_system_install_io_at(s, VXT_GET_PERIPHERAL(c), 0x92);
	return VXT_NO_ERROR;
}

static vxt_error reset(struct a20 *c) {
	c->port_92 = 0;
	return VXT_NO_ERROR;
}

static const char *name(struct a20 *c) {
	(void)c; return "Fast-A20";
}

struct vxt_peripheral *a20_create(vxt_allocator *alloc, void *frontend, const char *args) {
	(void)frontend; (void)args;
	
	struct VXT_PERIPHERAL(struct a20) *p;
	*(void**)&p = (void*)vxt_allocate_peripheral(alloc, sizeof(struct a20));

	p->install = &install;
	p->config = &config;
	p->reset = &reset;
	p->name = &name;
	p->io.in = &in;
	p->io.out = &out;
	return (struct vxt_peripheral*)p;
}
