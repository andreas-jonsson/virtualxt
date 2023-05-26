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

#ifndef _VXT_H_
#define _VXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VXT_VERSION_MAJOR 0
#define VXT_VERSION_MINOR 9
#define VXT_VERSION_PATCH 0

#define _VXT_STRINGIFY(x) #x
#define _VXT_EVALUATOR(M, ...) M(__VA_ARGS__)
#define VXT_VERSION _VXT_EVALUATOR(_VXT_STRINGIFY, VXT_VERSION_MAJOR) "." _VXT_EVALUATOR(_VXT_STRINGIFY, VXT_VERSION_MINOR) "." _VXT_EVALUATOR(_VXT_STRINGIFY, VXT_VERSION_PATCH)

#if !defined(VXT_LIBC) && defined(TESTING)
    #define VXT_LIBC
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
    #error libvxt require C11 support
#endif

typedef char vxt_int8;
typedef short vxt_int16;
typedef int vxt_int32;

typedef unsigned char vxt_byte;
typedef unsigned short vxt_word;
typedef unsigned int vxt_dword;
typedef unsigned int vxt_pointer;

#ifndef bool
    typedef _Bool bool;
    #define true ((bool)1)
    #define false ((bool)0)
#endif

#ifndef size_t
    typedef __SIZE_TYPE__ size_t;
#endif

#ifndef intptr_t
    #ifdef __INTPTR_TYPE__
        typedef __INTPTR_TYPE__ intptr_t;
    #else
        typedef long long int intptr_t;
    #endif
#endif

#ifndef uintptr_t
    #ifdef __UINTPTR_TYPE__
        typedef __UINTPTR_TYPE__ uintptr_t;
    #else
        typedef unsigned long long int uintptr_t;
    #endif
#endif

#ifndef NULL
    #define NULL ((void*)0)
#endif

#ifndef memmove
    void *memmove(void*, const void*, size_t);
#endif

#ifndef memcpy
    void *memcpy(void*, const void*, size_t);
#endif

#ifndef memset
    void *memset(void*, int, size_t);
#endif

#define vxt_memclear(p, s) ( memset((p), 0, (int)(s)) )

#ifdef _MSC_VER
   #define VXT_PACK(x) __pragma(pack(push, 1)) x __pragma(pack(pop))
#else
   #define VXT_PACK(x) x __attribute__((__packed__))
#endif

#define VXT_POINTER(s, o) ( (((((vxt_pointer)(vxt_word)(s)) << 4) + (vxt_pointer)(vxt_word)(o))) & 0xFFFFF )
#define VXT_INVALID_POINTER ((vxt_pointer)0xFFFFFFFF)
#define VXT_INVALID_DEVICE_ID ((vxt_device_id)0xFF)
#define VXT_INVALID_TIMER_ID ((vxt_timer_id)-1)

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
typedef int vxt_timer_id;

typedef struct system vxt_system;

typedef void *vxt_allocator(void*, size_t);

enum vxt_cpu_type {
    VXT_CPU_8088,
    VXT_CPU_V20
};

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
#define VXT_DEFAULT_FREQUENCY 4772726

#define VXT_PIREPHERAL_SIZE(type) sizeof(struct _ ## type)
#define VXT_GET_DEVICE_DATA(type, pir) ((struct _ ## type*)(void*)(pir))->d
#define VXT_GET_SYSTEM(type, pir) ((struct _ ## type*)(void*)(pir))->p.s
#define VXT_GET_DEVICE(type, pir) &((struct _ ## type*)(void*)(pir))->u
#define VXT_DEC_DEVICE(var, type, pir) struct type* const var = VXT_GET_DEVICE(type, pir)

#define VXT_PIREPHERAL_CREATE(alloc, type, body) {              \
        struct vxt_pirepheral *PIREPHERAL = (struct vxt_pirepheral*)(alloc)(NULL, VXT_PIREPHERAL_SIZE(type)); \
        vxt_memclear(PIREPHERAL, VXT_PIREPHERAL_SIZE(type));    \
        VXT_DEC_DEVICE(DEVICE, type, PIREPHERAL);               \
        { body ; }                                              \
        (void)DEVICE;                                           \
        return PIREPHERAL;                                      \
    }                                                           \

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
    VXT_PCLASS_PPI      = 0x10,
    VXT_PCLASS_PIT      = 0x20,
    VXT_PCLASS_VIDEO    = 0x40
};

struct vxt_pirepheral;

/// Interface for ISA bus devices.
struct vxt_pirepheral {
	vxt_error (*install)(vxt_system*,struct vxt_pirepheral*);
    vxt_error (*config)(struct vxt_pirepheral*,const char*,const char*,const char*);
    vxt_error (*destroy)(struct vxt_pirepheral*);
    vxt_error (*reset)(struct vxt_pirepheral*);
    vxt_error (*timer)(struct vxt_pirepheral*,vxt_timer_id,int);
    const char* (*name)(struct vxt_pirepheral*);
    enum vxt_pclass (*pclass)(struct vxt_pirepheral*);

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

// Use VXT_NO_LOG to disable logging in specific compile units
// or use VXT_NO_LOGGING to disable all logging.
#ifdef VXT_NO_LOG
    #define VXT_PRINT(...) {}
    #define VXT_LOG(...) {}
#else

    /// @private
    extern int (*_vxt_logger)(const char*, ...);

    #define VXT_PRINT(...) { _vxt_logger(__VA_ARGS__); }
    #define VXT_LOG(...) {                                                              \
        int l = 0; const char *f = __FILE__;                                            \
        while (f[l]) l++;                                                               \
        while (l && (f[l-1] != '/') && (f[l-1] != '\\')) l--;                           \
        _vxt_logger("[%s] ", &f[l]); _vxt_logger(__VA_ARGS__); _vxt_logger("\n"); }     \

#endif

#define vxt_logger() _vxt_logger

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

extern vxt_system *vxt_system_create(vxt_allocator *alloc, enum vxt_cpu_type ty, int frequency, struct vxt_pirepheral * const devs[]);
extern vxt_error vxt_system_configure(vxt_system *s, const char *section, const char *key, const char *value);
extern vxt_error vxt_system_destroy(vxt_system *s);
extern struct vxt_step vxt_system_step(vxt_system *s, int cycles);
extern void vxt_system_reset(vxt_system *s);
extern struct vxt_registers *vxt_system_registers(vxt_system *s);

extern int vxt_system_frequency(vxt_system *s);
extern void vxt_system_set_frequency(vxt_system *s, int freq);
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

extern void vxt_system_install_io_at(vxt_system *s, struct vxt_pirepheral *dev, vxt_word addr);
extern void vxt_system_install_mem_at(vxt_system *s, struct vxt_pirepheral *dev, vxt_pointer addr);
extern void vxt_system_install_io(vxt_system *s, struct vxt_pirepheral *dev, vxt_word from, vxt_word to);
extern void vxt_system_install_mem(vxt_system *s, struct vxt_pirepheral *dev, vxt_pointer from, vxt_pointer to);
extern vxt_timer_id vxt_system_install_timer(vxt_system *s, struct vxt_pirepheral *dev, unsigned int us);

extern vxt_byte vxt_system_read_byte(vxt_system *s, vxt_pointer addr);
extern void vxt_system_write_byte(vxt_system *s, vxt_pointer addr, vxt_byte data);
extern vxt_word vxt_system_read_word(vxt_system *s, vxt_pointer addr);
extern void vxt_system_write_word(vxt_system *s, vxt_pointer addr, vxt_word data);

/// @private
_Static_assert(sizeof(vxt_pointer) == 4 && sizeof(vxt_int32) == 4, "invalid integer size");

/// @private
_Static_assert(sizeof(long long) == 8, "invalid size of 'long long'");

/// @private
_Static_assert(sizeof(vxt_word) == 2, "invalid size of 'short'");

/// @private
_Static_assert(sizeof(intptr_t) == sizeof(void*), "invalid intptr_t size");

/// @private
_Static_assert(sizeof(uintptr_t) == sizeof(void*), "invalid uintptr_t size");

#ifdef __cplusplus
}
#endif

#endif
