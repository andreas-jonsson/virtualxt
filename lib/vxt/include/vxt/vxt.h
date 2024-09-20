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

#ifndef _VXT_H_
#define _VXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VXT_VERSION_MAJOR 1
#define VXT_VERSION_MINOR 1
#define VXT_VERSION_PATCH 0

#ifdef VXT_VERSION_RELEASE
    #define _VXT_VERSION_SUFFIX
#else
    #define _VXT_VERSION_SUFFIX "-testing"
#endif

#define _VXT_STRINGIFY(x) #x
#define _VXT_EVALUATOR(M, ...) M(__VA_ARGS__)
#define VXT_VERSION _VXT_EVALUATOR(_VXT_STRINGIFY, VXT_VERSION_MAJOR) "." _VXT_EVALUATOR(_VXT_STRINGIFY, VXT_VERSION_MINOR) "." _VXT_EVALUATOR(_VXT_STRINGIFY, VXT_VERSION_PATCH) _VXT_VERSION_SUFFIX

#ifdef VXT_NO_LIBC
    #ifdef TESTING
        #error "Don't use VXT_NO_LIBC for tests!"
    #endif

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

    typedef char vxt_int8;
    typedef short vxt_int16;
    typedef int vxt_int32;

    typedef unsigned char vxt_byte;
    typedef unsigned short vxt_word;
    typedef unsigned int vxt_dword;
    typedef unsigned int vxt_pointer;
#else
    #ifndef __STDC_HOSTED__
        #error "Use VXT_NO_LIBC for builds without hosted environment."
    #endif

    #include <stddef.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>

    typedef int8_t vxt_int8;
    typedef int16_t vxt_int16;
    typedef int32_t vxt_int32;

    typedef uint8_t vxt_byte;
    typedef uint16_t vxt_word;
    typedef uint32_t vxt_dword;
    typedef uint32_t vxt_pointer;
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
    #error "libvxt require C11 support!"
#endif

#define vxt_memclear(p, s) ( memset((p), 0, (int)(s)) )

#ifdef _MSC_VER
    #define VXT_PACK(x) __pragma(pack(push, 1)) x __pragma(pack(pop))

    #define VXT_API_EXPORT __declspec(dllexport)
    #ifdef VXT_EXPORT
        #define VXT_API VXT_API_EXPORT
    #else
        #define VXT_API __declspec(dllimport)
    #endif
#else
    #define VXT_PACK(x) x __attribute__((__packed__))

    #define VXT_API_EXPORT
    #define VXT_API
#endif

// Extended memory in MB
#ifndef VXT_EXTENDED_MEMORY_SIZE
	#define VXT_EXTENDED_MEMORY_SIZE 1
#endif

#define VXT_POINTER(s, o) ( ((((vxt_pointer)(vxt_word)(s)) << 4) + (vxt_pointer)(vxt_word)(o)) )
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
#define VXT_MEM_MAP_SIZE 0x10000
#define VXT_MAX_PERIPHERALS 0xFF
#define VXT_MAX_MONITORS 0xFF
#define VXT_DEFAULT_FREQUENCY 4772726

enum vxt_segment {
	VXT_SEGMENT_ES,
	VXT_SEGMENT_CS,
	VXT_SEGMENT_SS,
	VXT_SEGMENT_DS,
	
	// For internal reference only.
	_VXT_REG_TR,
	_VXT_REG_IDTR,
	_VXT_REG_LDTR,
	_VXT_REG_GDTR
};

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
    bool halted, int28, invalid;
    vxt_error err;
};

enum vxt_pclass {
    VXT_PCLASS_GENERIC  = 0x01,
    VXT_PCLASS_DEBUGGER = 0x02,
    VXT_PCLASS_PIC      = 0x04,
    VXT_PCLASS_DMA      = 0x08,
    VXT_PCLASS_PPI      = 0x10,
    VXT_PCLASS_PIT      = 0x20,
    VXT_PCLASS_UART     = 0x40,
    VXT_PCLASS_VIDEO    = 0x80
};

enum vxt_monitor_flag {
    VXT_MONITOR_SIZE_BYTE       = 0x1,
    VXT_MONITOR_SIZE_WORD       = 0x2,
    VXT_MONITOR_SIZE_DWORD      = 0x4,
    VXT_MONITOR_SIZE_QWORD      = 0x8,
    VXT_MONITOR_FORMAT_HEX      = 0x10,
    VXT_MONITOR_FORMAT_DECIMAL  = 0x20,
    VXT_MONITOR_FORMAT_BINARY   = 0x40,
    VXT_MONITOR_FORMAT_REAL     = 0x80
};

struct vxt_monitor {
    const char *dev;
    const char *name;
    const void *reg;
    enum vxt_monitor_flag flags;
};

#define VXT_GET_DEVICE_PTR(pir) ( (void*)((char*)(pir) + sizeof(struct _vxt_peripheral)) )
#define VXT_GET_DEVICE(type, pir) ( (struct type*)VXT_GET_DEVICE_PTR(pir) )
#define VXT_GET_PERIPHERAL(dev) ( (struct vxt_peripheral*)((char*)(dev) - sizeof(struct _vxt_peripheral)) )
#define VXT_GET_SYSTEM(dev) ((struct _vxt_peripheral*)VXT_GET_PERIPHERAL(dev))->s

#define VXT_PERIPHERAL_SIZE(type) ( sizeof(struct _vxt_peripheral) + sizeof(struct type) )

#define VXT_PERIPHERAL_CREATE(alloc, type, body) {                          \
        struct VXT_PERIPHERAL(struct type) *PERIPHERAL;                     \
        *(void**)&PERIPHERAL = (alloc)(NULL, VXT_PERIPHERAL_SIZE(type));    \
        vxt_memclear(PERIPHERAL, VXT_PERIPHERAL_SIZE(type));                \
        struct type *DEVICE = VXT_GET_DEVICE(type, PERIPHERAL);             \
        { body ; }                                                          \
        (void)DEVICE;                                                       \
        return (struct vxt_peripheral*)PERIPHERAL;                          \
    }                                                                       \

#define VXT_PERIPHERAL(ty) {                                        \
	vxt_error (*install)(ty*, vxt_system*);                         \
    vxt_error (*config)(ty*,const char*,const char*,const char*);   \
    vxt_error (*destroy)(ty*);                                      \
    vxt_error (*reset)(ty*);                                        \
    vxt_error (*timer)(ty*,vxt_timer_id,int);                       \
    const char* (*name)(ty*);                                       \
    enum vxt_pclass (*pclass)(ty*);                                 \
                                                                    \
    struct {                                                        \
        vxt_byte (*in)(ty*,vxt_word);                               \
        void (*out)(ty*,vxt_word,vxt_byte);                         \
                                                                    \
        vxt_byte (*read)(ty*,vxt_pointer);                          \
        void (*write)(ty*,vxt_pointer,vxt_byte);                    \
    } io;                                                           \
                                                                    \
    struct {                                                        \
        void (*irq)(ty*,int);                                       \
        int (*next)(ty*);                                           \
    } pic;                                                          \
                                                                    \
    struct {                                                        \
        vxt_byte (*read)(ty*,vxt_byte);                             \
        void (*write)(ty*,vxt_byte,vxt_byte);                       \
    } dma;                                                          \
}                                                                   \

/// Interface for ISA bus devices.
struct vxt_peripheral VXT_PERIPHERAL(void);

/// @private
struct _vxt_peripheral {
    struct vxt_peripheral p;
    vxt_device_id id;
    vxt_system *s;
    // User device data is located at the end of this struct.
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
    VXT_API extern int (*_vxt_logger)(const char*, ...);

    #define VXT_PRINT(...) { _vxt_logger(__VA_ARGS__); }
    #define VXT_LOG(...) {                                                              \
        int l = 0; const char *f = __FILE__;                                            \
        while (f[l]) l++;                                                               \
        while (l && (f[l-1] != '/') && (f[l-1] != '\\')) l--;                           \
        _vxt_logger("[%s] ", &f[l]); _vxt_logger(__VA_ARGS__); _vxt_logger("\n"); }     \

#endif

#define vxt_logger() _vxt_logger

/// @private
VXT_API vxt_error _vxt_system_initialize(vxt_system *s, unsigned reg_size, int v_major, int v_minor);

#define vxt_system_initialize(s) _vxt_system_initialize((s), sizeof(struct vxt_registers), VXT_VERSION_MAJOR, VXT_VERSION_MINOR)

VXT_API const char *vxt_error_str(vxt_error err);
VXT_API const char *vxt_lib_version(void);
VXT_API void vxt_set_logger(int (*f)(const char*, ...));
VXT_API int vxt_lib_version_major(void);
VXT_API int vxt_lib_version_minor(void);
VXT_API int vxt_lib_version_patch(void);

VXT_API const char *vxt_peripheral_name(struct vxt_peripheral *p);
VXT_API enum vxt_pclass vxt_peripheral_class(struct vxt_peripheral *p);

VXT_API vxt_system *vxt_system_create(vxt_allocator *alloc, int frequency, struct vxt_peripheral * const devs[]);
VXT_API vxt_error vxt_system_configure(vxt_system *s, const char *section, const char *key, const char *value);
VXT_API vxt_error vxt_system_destroy(vxt_system *s);
VXT_API struct vxt_step vxt_system_step(vxt_system *s, int cycles);
VXT_API void vxt_system_reset(vxt_system *s);

VXT_API void vxt_system_reload_segments(vxt_system *s);
VXT_API struct vxt_registers *vxt_system_registers(vxt_system *s);
VXT_API bool vxt_system_cpu_protected(vxt_system *s);
VXT_API void vxt_system_set_a20(vxt_system *s, bool enable);

VXT_API int vxt_system_frequency(vxt_system *s);
VXT_API void vxt_system_set_frequency(vxt_system *s, int freq);
VXT_API void vxt_system_set_tracer(vxt_system *s, void (*tracer)(vxt_system*,vxt_pointer,vxt_byte));
VXT_API void vxt_system_set_validator(vxt_system *s, const struct vxt_validator *intrf);
VXT_API void vxt_system_set_userdata(vxt_system *s, void *data);
VXT_API void *vxt_system_userdata(vxt_system *s);
VXT_API vxt_allocator *vxt_system_allocator(vxt_system *s);

VXT_API const vxt_byte *vxt_system_io_map(vxt_system *s);
VXT_API const vxt_byte *vxt_system_mem_map(vxt_system *s);
VXT_API const struct vxt_monitor *vxt_system_monitor(vxt_system *s, vxt_byte idx);
VXT_API struct vxt_peripheral *vxt_system_peripheral(vxt_system *s, vxt_byte idx);
VXT_API vxt_system *vxt_peripheral_system(const struct vxt_peripheral *p);
VXT_API vxt_device_id vxt_peripheral_id(const struct vxt_peripheral *p);

VXT_API void vxt_system_interrupt(vxt_system *s, int n);
VXT_API void vxt_system_wait(vxt_system *s, int cycles);

VXT_API void vxt_system_install_io_at(vxt_system *s, struct vxt_peripheral *dev, vxt_word addr);
VXT_API void vxt_system_install_io(vxt_system *s, struct vxt_peripheral *dev, vxt_word from, vxt_word to);
VXT_API void vxt_system_install_mem(vxt_system *s, struct vxt_peripheral *dev, vxt_pointer from, vxt_pointer to);
VXT_API vxt_timer_id vxt_system_install_timer(vxt_system *s, struct vxt_peripheral *dev, unsigned int us);
VXT_API bool vxt_system_set_timer_interval(vxt_system *s, vxt_timer_id id, unsigned int us);
VXT_API void vxt_system_install_monitor(vxt_system *s, struct vxt_peripheral *dev, const char *name, void *reg, enum vxt_monitor_flag flags);

VXT_API vxt_byte vxt_system_read_byte(vxt_system *s, vxt_pointer addr);
VXT_API void vxt_system_write_byte(vxt_system *s, vxt_pointer addr, vxt_byte data);
VXT_API vxt_word vxt_system_read_word(vxt_system *s, vxt_pointer addr);
VXT_API void vxt_system_write_word(vxt_system *s, vxt_pointer addr, vxt_word data);

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

/// @private
_Static_assert((VXT_EXTENDED_MEMORY_SIZE >= 0) && (VXT_EXTENDED_MEMORY_SIZE <= 16), "invalid memory size");

#ifdef __cplusplus
}
#endif

#endif
