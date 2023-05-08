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

#ifndef _VXTP_H_
#define _VXTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vxt/vxtu.h>

enum vxtp_joystick_button {
    VXTP_JOYSTICK_A	= 0x1,
	VXTP_JOYSTICK_B = 0x2
};

struct vxtp_joystick_event {
	void *id;
	enum vxtp_joystick_button buttons;
	vxt_int16 xaxis;
    vxt_int16 yaxis;
};

extern struct vxt_pirepheral *vxtp_vga_create(vxt_allocator *alloc);
extern vxt_dword vxtp_vga_border_color(struct vxt_pirepheral *p);
extern bool vxtp_vga_snapshot(struct vxt_pirepheral *p);
extern int vxtp_vga_render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata);

extern struct vxt_pirepheral *vxtp_joystick_create(vxt_allocator *alloc, void *stick_a, void *stick_b);
extern bool vxtp_joystick_push_event(struct vxt_pirepheral *p, const struct vxtp_joystick_event *ev);

#ifdef __cplusplus
}
#endif

#endif
