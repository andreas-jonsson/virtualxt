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

#ifndef _VXT_H_
#define _VXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VXT_VERSION_MAJOR 0
#define VXT_VERSION_MINOR 7
#define VXT_VERSION_PATCH 0

#define _VXT_STRINGIFY(x) #x
#define _VXT_VERSION(A, B, C) _VXT_STRINGIFY(A) "." _VXT_STRINGIFY(B) "." _VXT_STRINGIFY(C)
#define VXT_VERSION _VXT_VERSION(VXT_VERSION_MAJOR, VXT_VERSION_MINOR, VXT_VERSION_PATCH)

#if !defined(VXT_LIBC) && defined(TESTING)
    #define VXT_LIBC
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
    #error libvxt require C11 support
#endif

#ifdef VXT_LIBC
    #include <stdint.h>
    #include <stdbool.h>

    typedef int8_t vxt_int8;
    typedef int16_t vxt_int16;
    typedef int32_t vxt_int32;

    typedef uint8_t vxt_byte;
    typedef uint16_t vxt_word;
    typedef uint32_t vxt_dword;
    typedef uint32_t vxt_pointer;

    #define vxt_memclear(p, s) ( memset((p), 0, (int)(s)) )
#else
    typedef char vxt_int8;
    typedef short vxt_int16;
    typedef int vxt_int32;

    typedef unsigned char vxt_byte;
    typedef unsigned short vxt_word;
    typedef unsigned int vxt_dword;
    typedef unsigned int vxt_pointer;

    #ifndef bool
        typedef _Bool bool;
        static const bool true = 1;
        static const bool false = 0;
    #endif

    #ifndef NULL
        #define NULL ((void*)0)
    #endif

    #define vxt_memclear(p, s) {            \
        vxt_byte *bp = (vxt_byte*)(p);      \
        for (int i = 0; i < (int)(s); i++)  \
            bp[i] = 0;                      \
    }                                       \

#endif

#if defined(VXT_LIBC) && defined(VXT_LIBC_ALLOCATOR) 
    #include <stdlib.h>
    static void *vxt_clib_malloc(void *p, int s) {
        return realloc(p, s);
    }
#endif

#ifdef _MSC_VER
   #define VXT_PACK(x) __pragma(pack(push, 1)) x __pragma(pack(pop))
#else
   #define VXT_PACK(x) x __attribute__((__packed__))
#endif

#ifdef VXT_CPU_286
    #define VXT_POINTER(s, o) ( (((vxt_pointer)(vxt_word)(s)) << 4) + (vxt_pointer)(vxt_word)(o) )
#else
    #define VXT_POINTER(s, o) ( (((((vxt_pointer)(vxt_word)(s)) << 4) + (vxt_pointer)(vxt_word)(o))) & 0xFFFFF )
#endif

#ifndef VXT_EXTENDED_MEMORY
    #define VXT_EXTENDED_MEMORY (0x1000000 - 0x100000) // 15MB
    #define VXT_EXTENDED_MEMORY_MASK 0xF00000
#endif

#define VXT_INVALID_POINTER ((vxt_pointer)0xFFFFFFFF)
#define VXT_INVALID_DEVICE_ID ((vxt_device_id)0xFF)

#define _VXT_ERROR_CODES(x)                                                     \
    x(0, VXT_NO_ERROR,                  "no error")                             \
    x(1, VXT_INVALID_VERSION,           "invalid version")                      \
    x(2, VXT_INVALID_REGISTER_PACKING,  "invalid register size or packing")     \
    x(3, VXT_USER_TERMINATION,          "user requested termination")           \
    x(4, VXT_NO_PIC,                    "could not find interrupt controller")  \
    x(5, VXT_NO_DMA,                    "could not find dma controller")        \

#define _VXT_ERROR_ENUM(id, name, text) name = id,
typedef enum {_VXT_ERROR_CODES(_VXT_ERROR_ENUM) _VXT_NUM_ERRORS} vxt_error;
#undef _VXT_ERROR_ENUM

#define VXT_USER_ERROR(e) ((vxt_error)(e) + _VXT_NUM_ERRORS)
#define VXT_GET_USER_ERROR(e) ((vxt_error)(e) - _VXT_NUM_ERRORS)

typedef vxt_byte vxt_device_id;

typedef struct system vxt_system;

typedef void *vxt_allocator(void*,int);

enum {
    VXT_CARRY     = 0x001,
    VXT_PARITY    = 0x004,
    VXT_AUXILIARY = 0x010,
    VXT_ZERO      = 0x040,
    VXT_SIGN      = 0x080,
    VXT_TRAP      = 0x100,
    VXT_INTERRUPT = 0x200,
    VXT_DIRECTION = 0x400,
    VXT_OVERFLOW  = 0x800
};

#define VXT_IO_MAP_SIZE 0x10000
#define VXT_MEM_MAP_SIZE 0x100000
#define VXT_MAX_PIREPHERALS 0xFF

#define VXT_PIREPHERAL_SIZE(type) sizeof(struct _ ## type)
#define VXT_GET_DEVICE_DATA(type, pir) ((struct _ ## type*)(pir))->d
#define VXT_GET_SYSTEM(type, pir) ((struct _ ## type*)(pir))->p.s
#define VXT_GET_DEVICE(type, pir) &((struct _ ## type*)(pir))->u
#define VXT_DEC_DEVICE(var, type, pir) struct type* const var = VXT_GET_DEVICE(type, pir)

#define VXT_PIREPHERAL(name, body)                      \
    struct name                                         \
        body;                                           \
    struct _ ## name {                                  \
        struct _vxt_pirepheral p;                       \
        struct name u;                                  \
    };                                                  \

#define VXT_PIREPHERAL_WITH_DATA(name, type, body)      \
    struct name                                         \
        body;                                           \
    struct _ ## name {                                  \
        struct _vxt_pirepheral p;                       \
        struct name u;                                  \
        type d[];                                       \
    };                                                  \

#define _VXT_REG(r) VXT_PACK(union {VXT_PACK(struct {vxt_byte r ## l; vxt_byte r ## h;}); vxt_word r ## x;})
struct vxt_registers {
    _VXT_REG(a);
    _VXT_REG(b);
    _VXT_REG(c);
    _VXT_REG(d);

    vxt_word cs, ss, ds, es;
    vxt_word sp, bp, si, di;
    vxt_word ip, flags;

    bool debug;
};
#undef _VXT_REG

struct vxt_step {
    int cycles;
    bool halted;
    vxt_error err;
};

enum vxt_pclass {
    VXT_PCLASS_GENERIC  = 0x01,
    VXT_PCLASS_DEBUGGER = 0x02,
    VXT_PCLASS_PIC      = 0x04,
    VXT_PCLASS_DMA      = 0x08,
    VXT_PCLASS_VIDEO    = 0x10
};

struct vxt_pirepheral;

/// Interface for ISA bus devices.
struct vxt_pirepheral {
	vxt_error (*install)(vxt_system*,struct vxt_pirepheral*);
    vxt_error (*destroy)(struct vxt_pirepheral*);
    vxt_error (*reset)(struct vxt_pirepheral*);

    const char* (*name)(struct vxt_pirepheral*);
    enum vxt_pclass (*pclass)(struct vxt_pirepheral*);
    vxt_error (*step)(struct vxt_pirepheral*,int);

    struct {
        vxt_byte (*in)(struct vxt_pirepheral*,vxt_word);
        void (*out)(struct vxt_pirepheral*,vxt_word,vxt_byte);

        vxt_byte (*read)(struct vxt_pirepheral*,vxt_pointer);
        void (*write)(struct vxt_pirepheral*,vxt_pointer,vxt_byte);
    } io;

    struct {
        void (*irq)(struct vxt_pirepheral*,int);
        int (*next)(struct vxt_pirepheral*);
    } pic;

    struct {
        vxt_byte (*read)(struct vxt_pirepheral*,vxt_byte);
        void (*write)(struct vxt_pirepheral*,vxt_byte,vxt_byte);
    } dma;
};

/// @private
struct _vxt_pirepheral {
    struct vxt_pirepheral p;
    vxt_device_id id;
    vxt_system *s;
};

struct vxt_validator {
    void *userdata;

    vxt_error (*initialize)(vxt_system *s, void *userdata);
    vxt_error (*destroy)(void *userdata);

    void (*begin)(struct vxt_registers *regs, void *userdata);
    void (*end)(const char *name, vxt_byte opcode, bool modregrm, int cycles, struct vxt_registers *regs, void *userdata);
    void (*read)(vxt_pointer addr, vxt_byte data, void *userdata);
    void (*write)(vxt_pointer addr, vxt_byte data, void *userdata);
    void (*discard)(void *userdata);
};

/// @private
extern vxt_error _vxt_system_initialize(vxt_system *s, unsigned reg_size, int v_major, int v_minor);

#define vxt_system_initialize(s) _vxt_system_initialize((s), sizeof(struct vxt_registers), VXT_VERSION_MAJOR, VXT_VERSION_MINOR)

extern const char *vxt_error_str(vxt_error err);
extern const char *vxt_lib_version(void);
extern void vxt_set_logger(int (*f)(const char*, ...));
extern void vxt_set_breakpoint(void (*f)(void));
extern int vxt_lib_version_major(void);
extern int vxt_lib_version_minor(void);
extern int vxt_lib_version_patch(void);

extern const char *vxt_pirepheral_name(struct vxt_pirepheral *p);
extern enum vxt_pclass vxt_pirepheral_class(struct vxt_pirepheral *p);

extern vxt_system *vxt_system_create(vxt_allocator *alloc, struct vxt_pirepheral * const devs[]);
extern vxt_error vxt_system_destroy(vxt_system *s);
extern struct vxt_step vxt_system_step(vxt_system *s, int cycles);
extern void vxt_system_reset(vxt_system *s);
extern struct vxt_registers *vxt_system_registers(vxt_system *s);

extern void vxt_system_set_tracer(vxt_system *s, void (*tracer)(vxt_system*,vxt_pointer,vxt_byte));
extern void vxt_system_set_validator(vxt_system *s, const struct vxt_validator *interface);
extern void vxt_system_set_userdata(vxt_system *s, void *data);
extern void *vxt_system_userdata(vxt_system *s);
extern vxt_allocator *vxt_system_allocator(vxt_system *s);

extern const vxt_byte *vxt_system_io_map(vxt_system *s);
extern const vxt_byte *vxt_system_mem_map(vxt_system *s);
extern struct vxt_pirepheral *vxt_system_pirepheral(vxt_system *s, vxt_byte idx);
extern vxt_system *vxt_pirepheral_system(const struct vxt_pirepheral *p);
extern vxt_device_id vxt_pirepheral_id(const struct vxt_pirepheral *p);
extern void vxt_system_interrupt(vxt_system *s, int n);
extern bool vxt_system_a20(vxt_system *s);
extern void vxt_system_set_a20(vxt_system *s, bool b);

extern void vxt_system_install_io_at(vxt_system *s, struct vxt_pirepheral *dev, vxt_word addr);
extern void vxt_system_install_mem_at(vxt_system *s, struct vxt_pirepheral *dev, vxt_pointer addr);
extern void vxt_system_install_io(vxt_system *s, struct vxt_pirepheral *dev, vxt_word from, vxt_word to);
extern void vxt_system_install_mem(vxt_system *s, struct vxt_pirepheral *dev, vxt_pointer from, vxt_pointer to);

extern vxt_byte vxt_system_read_byte(vxt_system *s, vxt_pointer addr);
extern void vxt_system_write_byte(vxt_system *s, vxt_pointer addr, vxt_byte data);
extern vxt_word vxt_system_read_word(vxt_system *s, vxt_pointer addr);
extern void vxt_system_write_word(vxt_system *s, vxt_pointer addr, vxt_word data);

/// @private
_Static_assert(sizeof(vxt_pointer) == 4 && sizeof(vxt_int32) == 4, "invalid integer size");

#ifdef __cplusplus
}
#endif

#endif
