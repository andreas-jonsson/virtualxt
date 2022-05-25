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

VXT_PIREPHERAL(dma, {
 	int _;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    (void)p; (void)port;
	return 0;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    (void)p; (void)port; (void)data;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io(s, p, 0x0, 0xF);
    vxt_system_install_io(s, p, 0x81, 0x8F);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(dma, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "Dmmy DMA Controller";
}

struct vxt_pirepheral *vxtp_dma_create(vxt_allocator *alloc) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(dma));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(dma));

    p->install = &install;
    p->destroy = &destroy;
    p->name = &name;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}
