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

#include "common.h"
#include "system.h"
#include "testing.h"

#if defined(VXT_LIBC) && !defined(VXT_NO_LOGGING)
   #include <stdarg.h>
   static int libc_print(const char *fmt, ...) {
      va_list args;
      va_start(args, fmt);
      int ret = vprintf(fmt, args);
      va_end(args);
      return ret;
   }
   int (*_vxt_logger)(const char*, ...) = &libc_print;
#else
   static int no_print(const char *fmt, ...) {
      UNUSED(fmt);
      return -1;
   }
   int (*_vxt_logger)(const char*, ...) = &no_print;
#endif

static void no_breakpoint(void) {}
void (*breakpoint)(void) = &no_breakpoint;

const char *vxt_error_str(vxt_error err) {
    #define ERROR_TEXT(id, name, text) case id: return text;
    switch (err) {
        _VXT_ERROR_CODES(ERROR_TEXT)
        case _VXT_NUM_ERRORS: break;
    }
    #undef ERROR_TEXT
    return "user error";
}

const char *vxt_cpu_name(void) {
    #if defined(VXT_CPU_V20)
        return "V20";
    #elif defined(VXT_CPU_286)
        return "i80286";
    #else
        return "i8088";
    #endif
}

const char *vxt_lib_version(void) { return VXT_VERSION; }
int vxt_lib_version_major(void) { return VXT_VERSION_MAJOR; }
int vxt_lib_version_minor(void) { return VXT_VERSION_MINOR; }
int vxt_lib_version_patch(void) { return VXT_VERSION_PATCH; }

void vxt_set_logger(int (*f)(const char*, ...)) {
    _vxt_logger = f;
}

void vxt_set_breakpoint(void (*f)(void)) {
    breakpoint = f;
}

vxt_system *vxt_system_create(vxt_allocator *alloc, struct vxt_pirepheral * const devs[]) {
    vxt_system *s = (vxt_system*)alloc(NULL, sizeof(struct system));
    vxt_memclear(s, sizeof(struct system));
    s->alloc = alloc;
    s->cpu.s = s;

    int i = 1;
    for (; devs && devs[i-1]; i++) {
        s->devices[i] = devs[i-1];
        struct _vxt_pirepheral *internal = (struct _vxt_pirepheral*)s->devices[i];
        internal->id = (vxt_device_id)i;
        internal->s = s;
    }
    s->num_devices = i;

    // Always init dummy device 0. Depends on memset!
    s->devices[0] = (struct vxt_pirepheral*)&s->dummy;
    init_dummy_device(s);
    return s;
}

vxt_error _vxt_system_initialize(CONSTP(vxt_system) s, unsigned reg_size, int v_major, int v_minor) {
    if (sizeof(struct vxt_registers) != reg_size)
        return VXT_INVALID_REGISTER_PACKING;

    //if (VXT_VERSION_MAJOR != v_major || VXT_VERSION_MINOR < v_minor)
    if (VXT_VERSION_MAJOR != v_major || VXT_VERSION_MINOR != v_minor)
        return VXT_INVALID_VERSION;

    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_pirepheral) d = s->devices[i];
        if (d->install) {
            vxt_error err = d->install(s, (struct vxt_pirepheral*)d);
            if (err) return err;
        }
        if (vxt_pirepheral_class(d) == VXT_PCLASS_PIC)
            s->cpu.pic = d;
    }

    if (s->cpu.validator)
        s->cpu.validator->initialize(s, s->cpu.validator->userdata);
    return VXT_NO_ERROR;
}

TEST(system_initialize,
    CONSTP(vxt_system) sp = vxt_system_create(TALLOC, NULL);
    TENSURE(sp);
    TENSURE_NO_ERR(vxt_system_initialize(sp));
    vxt_system_destroy(sp);
)

vxt_error vxt_system_destroy(CONSTP(vxt_system) s) {
    if (s->cpu.validator) {
        vxt_error (*destroy)(void*) = s->cpu.validator->destroy;
        destroy(s->cpu.validator->userdata);
    }

    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_pirepheral) d = s->devices[i];
        if (d->destroy) {
            vxt_error (*destroy)(struct vxt_pirepheral*) = d->destroy;
            vxt_error err = destroy(d);
            if (err) return err;
        }
    }
    s->alloc(s, 0);
    return VXT_NO_ERROR;
}

struct vxt_registers *vxt_system_registers(CONSTP(vxt_system) s) {
    return &s->cpu.regs;
}

vxt_allocator *vxt_system_allocator(vxt_system *s) {
    return s->alloc;
}

void vxt_system_reset(CONSTP(vxt_system) s) {
    cpu_reset(&s->cpu);
    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_pirepheral) d = s->devices[i];
        if (d->reset)
            d->reset(d);
    }
}

struct vxt_step vxt_system_step(CONSTP(vxt_system) s, int cycles) {
    int oldc = 0;
    struct vxt_step step = {0};
    cpu_reset_cycle_count(&s->cpu);

    for (;;) {
        int newc = cpu_step(&s->cpu);
        int c = newc - oldc;
        oldc = newc;
        step.cycles += c;
        step.halted = s->cpu.halt;

        for (int i = 0; i < s->num_devices; i++) {
            CONSTSP(vxt_pirepheral) d = s->devices[i];
            if (d->step) {
                if ((step.err = d->step(d, c)) != VXT_NO_ERROR)
                    return step;
            }
        }

        if (newc >= cycles)
            return step;
    }
}

void vxt_system_set_tracer(vxt_system *s, void (*tracer)(vxt_system*,vxt_pointer,vxt_byte)) {
    s->cpu.tracer = tracer;
}

void vxt_system_set_validator(CONSTP(vxt_system) s, const struct vxt_validator *interface) {
    s->cpu.validator = interface;
}

void vxt_system_set_userdata(CONSTP(vxt_system) s, void *data) {
    s->userdata = data;
}

void *vxt_system_userdata(CONSTP(vxt_system) s) {
    return s->userdata;
}

const vxt_byte *vxt_system_io_map(vxt_system *s) {
    return s->io_map;
}

const vxt_byte *vxt_system_mem_map(vxt_system *s) {
    return s->mem_map;
}

struct vxt_pirepheral *vxt_system_pirepheral(vxt_system *s, vxt_byte idx) {
    return s->devices[idx];
}

vxt_system *vxt_pirepheral_system(const struct vxt_pirepheral *p) {
    return ((struct _vxt_pirepheral*)p)->s;
}

vxt_device_id vxt_pirepheral_id(const struct vxt_pirepheral *p) {
    return ((struct _vxt_pirepheral*)p)->id;
}

const char *vxt_pirepheral_name(struct vxt_pirepheral *p) {
    return p->name ? p->name(p) : "unknown device";
}

enum vxt_pclass vxt_pirepheral_class(struct vxt_pirepheral *p) {
    return p->pclass ? p->pclass(p) : VXT_PCLASS_GENERIC;
}

void vxt_system_interrupt(CONSTP(vxt_system) s, int n) {
    if (s->cpu.pic)
        s->cpu.pic->pic.irq(s->cpu.pic, n);
}

void vxt_system_install_io_at(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_word addr) {
    s->io_map[addr] = (vxt_byte)vxt_pirepheral_id(dev);
}

void vxt_system_install_mem_at(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_pointer addr) {
    s->mem_map[addr & 0xFFFFF] = (vxt_byte)vxt_pirepheral_id(dev);
}

void vxt_system_install_io(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_word from, vxt_word to) {
    int i = (int)from;
    while (i <= (int)to)
        s->io_map[i++] = (vxt_byte)vxt_pirepheral_id(dev);
}

void vxt_system_install_mem(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_pointer from, vxt_pointer to) {
    int i = (int)(from & 0xFFFFF);
    to &= 0xFFFFF;
    while (i <= (int)to)
        s->mem_map[i++] = (vxt_byte)vxt_pirepheral_id(dev);
}

vxt_byte vxt_system_read_byte(CONSTP(vxt_system) s, vxt_pointer addr) {
    addr &= 0xFFFFF;
    CONSTSP(vxt_pirepheral) dev = s->devices[s->mem_map[addr]];
    vxt_byte data = dev->io.read(dev, addr);
    VALIDATOR_READ(&s->cpu, addr, data);
    return data;
}

void vxt_system_write_byte(CONSTP(vxt_system) s, vxt_pointer addr, vxt_byte data) {
    addr &= 0xFFFFF;
    CONSTSP(vxt_pirepheral) dev = s->devices[s->mem_map[addr]];
    dev->io.write(dev, addr, data);
    VALIDATOR_WRITE(&s->cpu, addr, data);
}

vxt_word vxt_system_read_word(CONSTP(vxt_system) s, vxt_pointer addr) {
    return WORD(vxt_system_read_byte(s, addr + 1), vxt_system_read_byte(s, addr));
}

void vxt_system_write_word(CONSTP(vxt_system) s, vxt_pointer addr, vxt_word data) {
    vxt_system_write_byte(s, addr, LBYTE(data));
    vxt_system_write_byte(s, addr + 1, HBYTE(data));
}

vxt_byte system_in(CONSTP(vxt_system) s, vxt_word port) {
    CONSTSP(vxt_pirepheral) dev = s->devices[s->io_map[port]];
    VALIDATOR_DISCARD(&s->cpu);
    return dev->io.in(dev, port);
}

void system_out(CONSTP(vxt_system) s, vxt_word port, vxt_byte data) {
    CONSTSP(vxt_pirepheral) dev = s->devices[s->io_map[port]];
    VALIDATOR_DISCARD(&s->cpu);
    dev->io.out(dev, port, data);
}
