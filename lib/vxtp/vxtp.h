// Copyright (c) 2019-2022 Andreas T Jonsson
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
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _VXTP_H_
#define _VXTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vxt/vxtu.h>
#include <stdio.h>

enum vxtp_scancode {
	VXTP_SCAN_INVALID,
	VXTP_SCAN_ESCAPE,
	VXTP_SCAN_1,
	VXTP_SCAN_2,
	VXTP_SCAN_3,
	VXTP_SCAN_4,
	VXTP_SCAN_5,
	VXTP_SCAN_6,
	VXTP_SCAN_7,
	VXTP_SCAN_8,
	VXTP_SCAN_9,
	VXTP_SCAN_0,
	VXTP_SCAN_MINUS,
	VXTP_SCAN_EQUAL,
	VXTP_SCAN_BACKSPACE,
	VXTP_SCAN_TAB,
	VXTP_SCAN_Q,
	VXTP_SCAN_W,
	VXTP_SCAN_E,
	VXTP_SCAN_R,
	VXTP_SCAN_T,
	VXTP_SCAN_Y,
	VXTP_SCAN_U,
	VXTP_SCAN_I,
	VXTP_SCAN_O,
	VXTP_SCAN_P,
	VXTP_SCAN_LBRACKET,
	VXTP_SCAN_RBRACKET,
	VXTP_SCAN_ENTER,
	VXTP_SCAN_CONTROL,
	VXTP_SCAN_A,
	VXTP_SCAN_S,
	VXTP_SCAN_D,
	VXTP_SCAN_F,
	VXTP_SCAN_G,
	VXTP_SCAN_H,
	VXTP_SCAN_J,
	VXTP_SCAN_K,
	VXTP_SCAN_L,
	VXTP_SCAN_SEMICOLON,
	VXTP_SCAN_QUOTE,
	VXTP_SCAN_BACKQUOTE,
	VXTP_SCAN_LSHIFT,
	VXTP_SCAN_BACKSLASH,
	VXTP_SCAN_Z,
	VXTP_SCAN_X,
	VXTP_SCAN_C,
	VXTP_SCAN_V,
	VXTP_SCAN_B,
	VXTP_SCAN_N,
	VXTP_SCAN_M,
	VXTP_SCAN_COMMA,
	VXTP_SCAN_PERIOD,
	VXTP_SCAN_SLASH,
	VXTP_SCAN_RSHIFT,
	VXTP_SCAN_PRINT,
	VXTP_SCAN_ALT,
	VXTP_SCAN_SPACE,
	VXTP_SCAN_CAPSLOCK,
	VXTP_SCAN_F1,
	VXTP_SCAN_F2,
	VXTP_SCAN_F3,
	VXTP_SCAN_F4,
	VXTP_SCAN_F5,
	VXTP_SCAN_F6,
	VXTP_SCAN_F7,
	VXTP_SCAN_F8,
	VXTP_SCAN_F9,
	VXTP_SCAN_F10,
	VXTP_SCAN_NUMLOCK,
	VXTP_SCAN_SCRLOCK,
	VXTP_SCAN_KP_HOME,
	VXTP_SCAN_KP_UP,
	VXTP_SCAN_KP_PAGEUP,
	VXTP_SCAN_KP_MINUS,
	VXTP_SCAN_KP_LEFT,
	VXTP_SCAN_KP_5,
	VXTP_SCAN_KP_RIGHT,
	VXTP_SCAN_KP_PLUS,
	VXTP_SCAN_KP_END,
	VXTP_SCAN_KP_DOWN,
	VXTP_SCAN_KP_PAGEDOWN,
	VXTP_SCAN_KP_INSERT,
	VXTP_SCAN_KP_DELETE
};

static const enum vxtp_scancode VXTP_KEY_UP_MASK = 0x80;

enum vxtp_mda_attrib {
    VXTP_MDA_UNDELINE       = 0x1,
    VXTP_MDA_HIGH_INTENSITY = 0x2,
    VXTP_MDA_BLINK          = 0x4,
    VXTP_MDA_INVERSE        = 0x8
};

enum vxtp_mouse_button {
    VXTP_MOUSE_RIGHT = 0x1,
	VXTP_MOUSE_LEFT  = 0x2    
};

struct vxtp_mouse_event {
	enum vxtp_mouse_button buttons;
	int xrel;
    int yrel;
};

extern struct vxt_pirepheral *vxtp_pic_create(vxt_allocator *alloc);

extern struct vxt_pirepheral *vxtp_pit_create(vxt_allocator *alloc, long long (*ustics)(void));
extern double vxtp_pit_get_frequency(struct vxt_pirepheral *p, int channel);

extern struct vxt_pirepheral *vxtp_ppi_create(vxt_allocator *alloc, struct vxt_pirepheral *pit, void (*enable_spk)(bool), void *userdata);
extern bool vxtp_ppi_key_event(struct vxt_pirepheral *p, enum vxtp_scancode key, bool force);
extern int vxtp_ppi_write_audio(struct vxt_pirepheral *p, vxt_byte *buffer, int freq, int channels, int samples);

extern struct vxt_pirepheral *vxtp_mda_create(vxt_allocator *alloc);
extern void vxtp_mda_invalidate(struct vxt_pirepheral *p);
extern int vxtp_mda_traverse(struct vxt_pirepheral *p, int (*f)(int,vxt_byte,enum vxtp_mda_attrib,int,void*), void *userdata);

extern struct vxt_pirepheral *vxtp_cga_create(vxt_allocator *alloc, long long (*ustics)(void));
extern vxt_dword vxtp_cga_border_color(struct vxt_pirepheral *p);
extern bool vxtp_cga_snapshot(struct vxt_pirepheral *p);

// This function only operates on snapshot data and is threadsafe.
// The use of 'vxtp_cga_snapshot' and 'vxtp_cga_render' needs to be coordinated by the user.
extern int vxtp_cga_render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata);

extern struct vxt_pirepheral *vxtp_disk_create(vxt_allocator *alloc, const char *files[]);
extern void vxtp_disk_set_boot_drive(struct vxt_pirepheral *p, int num);
extern vxt_error vxtp_disk_mount(struct vxt_pirepheral *p, int num, FILE *fp);
extern bool vxtp_disk_unmount(struct vxt_pirepheral *p, int num);

extern struct vxt_pirepheral *vxtp_dma_create(vxt_allocator *alloc);

extern struct vxt_pirepheral *vxtp_mouse_create(vxt_allocator *alloc, vxt_word base_port, int irq);
extern bool vxtp_mouse_push_event(struct vxt_pirepheral *p, const struct vxtp_mouse_event *ev);

#ifdef __cplusplus
}
#endif

#endif
