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

#ifndef _VXTU_H_
#define _VXTU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vxt.h"

// TODO: Fix this!
#ifdef _MSC_VER
	#define VXTU_CAST(in, tin, tout) ((tout)in)
#else
	#define VXTU_CAST(in, tin, tout) ( ((VXT_PACK(union { tin from; tout to; })){ .from = (in) }).to )
#endif

#define vxtu_randomize(ptr, size, seed) {					\
    int s = (int)(seed);									\
    for (int i = 0; i < (int)(size); i++) {					\
        const int m = 2147483647;							\
        s = (16807 * s) % m;								\
        float r = (float)s / m;								\
        ((vxt_byte*)(ptr))[i] = (vxt_byte)(255.0 * r);		\
    }														\
}															\

typedef struct vxt_pirepheral *(*vxtu_module_entry_func)(vxt_allocator*,void*,const char*);

#ifdef VXTU_MODULES
    #define _VXTU_MODULE_ENTRIES(n, ...) VXT_API_EXPORT vxtu_module_entry_func *_vxtu_module_ ## n ## _entry(int (*f)(const char*, ...)) {		\
		vxt_set_logger(f);																										\
		static vxtu_module_entry_func _vxtu_module_ ## n ## _entries[] = { __VA_ARGS__, NULL };									\
		return _vxtu_module_ ## n ## _entries;																					\
	}																															\
    
	#define VXTU_MODULE_ENTRIES(...) _VXT_EVALUATOR(_VXTU_MODULE_ENTRIES, VXTU_MODULE_NAME, __VA_ARGS__)
    #define VXTU_MODULE_CREATE(name, body) static struct vxt_pirepheral *_vxtu_pirepheral_create(vxt_allocator *ALLOC, void *FRONTEND, const char *ARGS)	\
		VXT_PIREPHERAL_CREATE(ALLOC, name, { body ; (void)FRONTEND; (void)ARGS; }) VXTU_MODULE_ENTRIES(&_vxtu_pirepheral_create)							\
    
	#define VXTU_MODULE_NAME_STRING _VXT_EVALUATOR(_VXT_STRINGIFY, VXTU_MODULE_NAME)
#else
    #define VXTU_MODULE_ENTRIES(...)
    #define VXTU_MODULE_CREATE(name, body)
    #define VXTU_MODULE_NAME_STRING ""
#endif

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

struct vxtu_uart_registers {
	vxt_word divisor; // Baud Rate Divisor
	vxt_byte ien; // Interrupt Enable
	vxt_byte iir; // Interrupt Identification
	vxt_byte lcr; // Line Control
	vxt_byte mcr; // Modem Control
	vxt_byte lsr; // Line Status
	vxt_byte msr; // Modem Status
};

struct vxtu_uart_interface {
    void (*config)(struct vxt_pirepheral *p, const struct vxtu_uart_registers *regs, int idx, void *udata);
	void (*data)(struct vxt_pirepheral *p, vxt_byte data, void *udata);
	void (*ready)(struct vxt_pirepheral *p, void *udata);
	void *udata;
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

VXT_API vxt_byte *vxtu_read_file(vxt_allocator *alloc, const char *file, int *size);

VXT_API struct vxt_pirepheral *vxtu_memory_create(vxt_allocator *alloc, vxt_pointer base, int amount, bool read_only);
VXT_API void *vxtu_memory_internal_pointer(struct vxt_pirepheral *p);
VXT_API bool vxtu_memory_device_fill(struct vxt_pirepheral *p, const vxt_byte *data, int size);

VXT_API struct vxt_pirepheral *vxtu_pic_create(vxt_allocator *alloc);

VXT_API struct vxt_pirepheral *vxtu_dma_create(vxt_allocator *alloc);

VXT_API struct vxt_pirepheral *vxtu_pit_create(vxt_allocator *alloc);
VXT_API double vxtu_pit_get_frequency(struct vxt_pirepheral *p, int channel);

VXT_API struct vxt_pirepheral *vxtu_ppi_create(vxt_allocator *alloc);
VXT_API bool vxtu_ppi_key_event(struct vxt_pirepheral *p, enum vxtu_scancode key, bool force);
VXT_API bool vxtu_ppi_turbo_enabled(struct vxt_pirepheral *p);
VXT_API vxt_int16 vxtu_ppi_generate_sample(struct vxt_pirepheral *p, int freq);
VXT_API void vxtu_ppi_set_speaker_callback(struct vxt_pirepheral *p, void (*f)(struct vxt_pirepheral*,double,void*), void *userdata);
VXT_API void vxtu_ppi_set_xt_switches(struct vxt_pirepheral *p, vxt_byte data);
VXT_API vxt_byte vxtu_ppi_xt_switches(struct vxt_pirepheral *p);

VXT_API struct vxt_pirepheral *vxtu_mda_create(vxt_allocator *alloc);
VXT_API void vxtu_mda_invalidate(struct vxt_pirepheral *p);
VXT_API int vxtu_mda_traverse(struct vxt_pirepheral *p, int (*f)(int,vxt_byte,enum vxtu_mda_attrib,int,void*), void *userdata);

VXT_API struct vxt_pirepheral *vxtu_disk_create(vxt_allocator *alloc, const struct vxtu_disk_interface *intrf);
VXT_API void vxtu_disk_set_activity_callback(struct vxt_pirepheral *p, void (*cb)(int,void*), void *ud);
VXT_API void vxtu_disk_set_boot_drive(struct vxt_pirepheral *p, int num);
VXT_API vxt_error vxtu_disk_mount(struct vxt_pirepheral *p, int num, void *fp);
VXT_API bool vxtu_disk_unmount(struct vxt_pirepheral *p, int num);

VXT_API struct vxt_pirepheral *vxtu_uart_create(vxt_allocator *alloc, vxt_word base_port, int irq);
VXT_API const struct vxtu_uart_registers *vxtu_uart_internal_registers(struct vxt_pirepheral *p);
VXT_API void vxtu_uart_set_callbacks(struct vxt_pirepheral *p, struct vxtu_uart_interface *intrf);
VXT_API void vxtu_uart_set_error(struct vxt_pirepheral *p, vxt_byte err);
VXT_API void vxtu_uart_write(struct vxt_pirepheral *p, vxt_byte data);
VXT_API bool vxtu_uart_ready(struct vxt_pirepheral *p);
VXT_API vxt_word vxtu_uart_address(struct vxt_pirepheral *p);

#ifdef __cplusplus
}
#endif

#endif
