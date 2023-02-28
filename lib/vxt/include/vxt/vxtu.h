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

#ifndef _VXTU_H_
#define _VXTU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vxt.h"

#define vxtu_static_allocator(name, size)                               \
    static vxt_byte name ## allocator_data[(size)];                     \
    static vxt_byte * name ## allocator_ptr = name ## allocator_data;   \
                                                                        \
    void * name (void *ptr, size_t sz) {                                \
        if (!sz)                                                        \
            return NULL;                                                \
        else if (ptr) /* Reallocation is not support! */                \
            return NULL;                                                \
                                                                        \
        vxt_byte *p = name ## allocator_ptr;                            \
        sz += 8 - (sz % 8);                                             \
        name ## allocator_ptr += sz;                                    \
                                                                        \
        if (name ## allocator_ptr >= (name ## allocator_data + (size))) \
            return NULL; /* Allocator is out of memory! */              \
        return p;                                                       \
    }                                                                   \

#if defined(VXT_LIBC) && defined(VXTU_LIBC_IO)
    #include <stdio.h>
    #include <string.h>

    static vxt_byte *vxtu_read_file(vxt_allocator *alloc, const char *file, int *size) {
        vxt_byte *data = NULL;
        FILE *fp = fopen(file, "rb");
        if (!fp)
            return data;

        if (fseek(fp, 0, SEEK_END))
            goto error;

        int sz = (int)ftell(fp);
        if (size)
            *size = sz;

        if (fseek(fp, 0, SEEK_SET))
            goto error;

        data = (vxt_byte*)alloc(NULL, sz);
        if (!data)
            goto error;

        memset(data, 0, sz);
        if (fread(data, 1, sz, fp) != (size_t)sz) {
            alloc(data, 0);
            goto error;
        }

    error:
        fclose(fp);
        return data;
    }
#endif

enum vxtu_scancode {
	VXTU_SCAN_INVALID,
	VXTU_SCAN_ESCAPE,
	VXTU_SCAN_1,
	VXTU_SCAN_2,
	VXTU_SCAN_3,
	VXTU_SCAN_4,
	VXTU_SCAN_5,
	VXTU_SCAN_6,
	VXTU_SCAN_7,
	VXTU_SCAN_8,
	VXTU_SCAN_9,
	VXTU_SCAN_0,
	VXTU_SCAN_MINUS,
	VXTU_SCAN_EQUAL,
	VXTU_SCAN_BACKSPACE,
	VXTU_SCAN_TAB,
	VXTU_SCAN_Q,
	VXTU_SCAN_W,
	VXTU_SCAN_E,
	VXTU_SCAN_R,
	VXTU_SCAN_T,
	VXTU_SCAN_Y,
	VXTU_SCAN_U,
	VXTU_SCAN_I,
	VXTU_SCAN_O,
	VXTU_SCAN_P,
	VXTU_SCAN_LBRACKET,
	VXTU_SCAN_RBRACKET,
	VXTU_SCAN_ENTER,
	VXTU_SCAN_CONTROL,
	VXTU_SCAN_A,
	VXTU_SCAN_S,
	VXTU_SCAN_D,
	VXTU_SCAN_F,
	VXTU_SCAN_G,
	VXTU_SCAN_H,
	VXTU_SCAN_J,
	VXTU_SCAN_K,
	VXTU_SCAN_L,
	VXTU_SCAN_SEMICOLON,
	VXTU_SCAN_QUOTE,
	VXTU_SCAN_BACKQUOTE,
	VXTU_SCAN_LSHIFT,
	VXTU_SCAN_BACKSLASH,
	VXTU_SCAN_Z,
	VXTU_SCAN_X,
	VXTU_SCAN_C,
	VXTU_SCAN_V,
	VXTU_SCAN_B,
	VXTU_SCAN_N,
	VXTU_SCAN_M,
	VXTU_SCAN_COMMA,
	VXTU_SCAN_PERIOD,
	VXTU_SCAN_SLASH,
	VXTU_SCAN_RSHIFT,
	VXTU_SCAN_PRINT,
	VXTU_SCAN_ALT,
	VXTU_SCAN_SPACE,
	VXTU_SCAN_CAPSLOCK,
	VXTU_SCAN_F1,
	VXTU_SCAN_F2,
	VXTU_SCAN_F3,
	VXTU_SCAN_F4,
	VXTU_SCAN_F5,
	VXTU_SCAN_F6,
	VXTU_SCAN_F7,
	VXTU_SCAN_F8,
	VXTU_SCAN_F9,
	VXTU_SCAN_F10,
	VXTU_SCAN_NUMLOCK,
	VXTU_SCAN_SCRLOCK,
	VXTU_SCAN_KP_HOME,
	VXTU_SCAN_KP_UP,
	VXTU_SCAN_KP_PAGEUP,
	VXTU_SCAN_KP_MINUS,
	VXTU_SCAN_KP_LEFT,
	VXTU_SCAN_KP_5,
	VXTU_SCAN_KP_RIGHT,
	VXTU_SCAN_KP_PLUS,
	VXTU_SCAN_KP_END,
	VXTU_SCAN_KP_DOWN,
	VXTU_SCAN_KP_PAGEDOWN,
	VXTU_SCAN_KP_INSERT,
	VXTU_SCAN_KP_DELETE
};

static const enum vxtu_scancode VXTU_KEY_UP_MASK = 0x80;

enum vxtu_mda_attrib {
    VXTU_MDA_UNDELINE       = 0x1,
    VXTU_MDA_HIGH_INTENSITY = 0x2,
    VXTU_MDA_BLINK          = 0x4,
    VXTU_MDA_INVERSE        = 0x8
};

enum vxtu_mouse_button {
    VXTU_MOUSE_RIGHT = 0x1,
	VXTU_MOUSE_LEFT  = 0x2    
};

struct vxtu_mouse_event {
	enum vxtu_mouse_button buttons;
	int xrel;
    int yrel;
};

enum vxtu_disk_seek {
    VXTU_SEEK_START		= 0x0,
	VXTU_SEEK_CURRENT 	= 0x1,
	VXTU_SEEK_END 		= 0x2
};

struct vxtu_disk_interface {
    int (*read)(vxt_system *s, void *fp, vxt_byte *buffer, int size);
	int (*write)(vxt_system *s, void *fp, vxt_byte *buffer, int size);
	int (*seek)(vxt_system *s, void *fp, int offset, enum vxtu_disk_seek whence);
	int (*tell)(vxt_system *s, void *fp);
};

struct vxtu_debugger_interface {
    bool (*pdisasm)(vxt_system*, vxt_pointer, int, int);
    const char *(*getline)(void);
    int (*print)(const char*, ...);
};

extern struct vxt_pirepheral *vxtu_debugger_create(vxt_allocator *alloc, const struct vxtu_debugger_interface *interface);
extern void vxtu_debugger_interrupt(struct vxt_pirepheral *dbg);

extern struct vxt_pirepheral *vxtu_memory_create(vxt_allocator *alloc, vxt_pointer base, int amount, bool read_only);
extern void *vxtu_memory_internal_pointer(const struct vxt_pirepheral *p);
extern bool vxtu_memory_device_fill(struct vxt_pirepheral *p, const vxt_byte *data, int size);

extern struct vxt_pirepheral *vxtu_pic_create(vxt_allocator *alloc);

extern struct vxt_pirepheral *vxtu_dma_create(vxt_allocator *alloc);

extern struct vxt_pirepheral *vxtu_pit_create(vxt_allocator *alloc);
extern double vxtu_pit_get_frequency(struct vxt_pirepheral *p, int channel);

extern struct vxt_pirepheral *vxtu_ppi_create(vxt_allocator *alloc);
extern bool vxtu_ppi_key_event(struct vxt_pirepheral *p, enum vxtu_scancode key, bool force);
extern bool vxtu_ppi_turbo_enabled(struct vxt_pirepheral *p);
extern vxt_int16 vxtu_ppi_generate_sample(struct vxt_pirepheral *p, int freq);
extern void vxtu_ppi_set_speaker_callback(struct vxt_pirepheral *p, void (*f)(struct vxt_pirepheral*,double,void*), void *userdata);
extern void vxtu_ppi_set_xt_switches(struct vxt_pirepheral *p, vxt_byte data);

extern struct vxt_pirepheral *vxtu_mda_create(vxt_allocator *alloc);
extern void vxtu_mda_invalidate(struct vxt_pirepheral *p);
extern int vxtu_mda_traverse(struct vxt_pirepheral *p, int (*f)(int,vxt_byte,enum vxtu_mda_attrib,int,void*), void *userdata);

extern struct vxt_pirepheral *vxtu_cga_create(vxt_allocator *alloc);
extern vxt_dword vxtu_cga_border_color(struct vxt_pirepheral *p);
extern bool vxtu_cga_snapshot(struct vxt_pirepheral *p);

// This function only operates on snapshot data and is threadsafe.
// The use of 'vxtu_cga_snapshot' and 'vxtu_cga_render' needs to be coordinated by the user.
extern int vxtu_cga_render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata);

extern struct vxt_pirepheral *vxtu_disk_create(vxt_allocator *alloc, const struct vxtu_disk_interface *interface);
extern void vxtu_disk_set_boot_drive(struct vxt_pirepheral *p, int num);
extern vxt_error vxtu_disk_mount(struct vxt_pirepheral *p, int num, void *fp);
extern bool vxtu_disk_unmount(struct vxt_pirepheral *p, int num);

extern struct vxt_pirepheral *vxtu_mouse_create(vxt_allocator *alloc, vxt_word base_port, int irq);
extern bool vxtu_mouse_push_event(struct vxt_pirepheral *p, const struct vxtu_mouse_event *ev);

#ifdef __cplusplus
}
#endif

#endif
