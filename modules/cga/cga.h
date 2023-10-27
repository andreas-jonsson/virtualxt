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

#ifndef _CGA_H_
#define _CGA_H_

#include <vxt/vxt.h>

struct vxt_pirepheral *cga_create(vxt_allocator *alloc);
vxt_dword cga_border_color(struct vxt_pirepheral *p);
bool cga_snapshot(struct vxt_pirepheral *p);

// This function only operates on snapshot data and is threadsafe.
// The use of 'vxtu_cga_snapshot' and 'vxtu_cga_render' needs to be coordinated by the user.
int cga_render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata);

#endif
