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
   int (*logger)(const char*, ...) = &libc_print;
#else
   static int no_print(const char *fmt, ...) {
      (void)fmt;
      return -1;
   }
   int (*logger)(const char*, ...) = &no_print;
#endif

const char *vxt_error_str(vxt_error err) {
    #define ERROR_TEXT(id, name, text) case id: return text;
    switch (err) {
        _VXT_ERROR_CODES(ERROR_TEXT)
    }
    return "unknown error";
}

const char *vxt_lib_version(void) { return VXT_VERSION; }
int vxt_lib_version_major(void) { return VXT_VERSION_MAJOR; }
int vxt_lib_version_minor(void) { return VXT_VERSION_MINOR; }
int vxt_lib_version_patch(void) { return VXT_VERSION_PATCH; }

int _vxt_system_register_size(void) {
    return sizeof(struct vxt_registers);
}

void vxt_set_logger(int (*f)(const char*, ...)) {
    logger = f;
}

vxt_system *vxt_system_create(vxt_allocator *alloc, struct vxt_pirepheral *devs[]) {
    vxt_system *s = (vxt_system*)alloc(NULL, sizeof(struct system));
    memclear(s, sizeof(struct system));
    s->alloc = alloc;
    s->cpu.s = s;

    int i = 1;
    for (; devs && devs[i-1]; i++) {
        s->devices[i] = *devs[i-1];
        s->devices[i].id = (vxt_device_id)i;
    }
    s->num_devices = i;

    // Always init dummy device 0. Depends on memset!
    init_dummy_device(s, &s->devices[0]);
    return s;
}

vxt_error _vxt_system_initialize(CONSTP(vxt_system) s) {
    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_pirepheral) d = &s->devices[i];
        if (d->install) {
            vxt_error err = d->install(s, d);
            if (err) return err;
        }
    }
    return VXT_NO_ERROR;
}

TEST(system_initialize, {
    CONSTP(vxt_system) sp = vxt_system_create(TEST_ALLOC, NULL);
    TENSURE(sp);
    TENSURE_NO_ERR(vxt_system_initialize(sp));
    vxt_system_destroy(sp);
})

vxt_error vxt_system_destroy(CONSTP(vxt_system) s) {
    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_pirepheral) d = &s->devices[i];
        if (d->destroy) {
            vxt_error err = d->destroy(d->userdata);
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
        CONSTSP(vxt_pirepheral) d = &s->devices[i];
        if (d->reset)
            d->reset(d->userdata);
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
            CONSTSP(vxt_pirepheral) d = &s->devices[i];
            if (d->step) {
                if ((step.err = d->step(d->userdata, c)) != VXT_NO_ERROR)
                    return step;

            }
        }

        if (newc >= cycles)
            return step;
    }
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

const struct vxt_pirepheral *vxt_system_pirepheral(vxt_system *s, vxt_byte idx) {
    return &s->devices[idx];
}

void vxt_system_install_io_at(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_word addr) {
    s->io_map[addr] = (vxt_byte)dev->id;
}

void vxt_system_install_mem_at(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_pointer addr) {
    s->mem_map[addr & 0xFFFFF] = (vxt_byte)dev->id;
}

void vxt_system_install_io(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_word from, vxt_word to) {
    int i = (int)from;
    while (i <= (int)to)
        s->io_map[i++] = (vxt_byte)dev->id;
}

void vxt_system_install_mem(CONSTP(vxt_system) s, struct vxt_pirepheral *dev, vxt_pointer from, vxt_pointer to) {
    int i = (int)(from & 0xFFFFF);
    to &= 0xFFFFF;
    while (i <= (int)to)
        s->mem_map[i++] = (vxt_byte)dev->id;
}

vxt_byte vxt_system_read_byte(CONSTP(vxt_system) s, vxt_pointer addr) {
    addr &= 0xFFFFF;
    CONSTSP(vxt_pirepheral) dev = &s->devices[s->mem_map[addr]];
    return dev->io.read(dev->userdata, addr);
}

void vxt_system_write_byte(CONSTP(vxt_system) s, vxt_pointer addr, vxt_byte data) {
    addr &= 0xFFFFF;
    CONSTSP(vxt_pirepheral) dev = &s->devices[s->mem_map[addr]];
    dev->io.write(dev->userdata, addr, data);
}

vxt_word vxt_system_read_word(CONSTP(vxt_system) s, vxt_pointer addr) {
    vxt_pointer p = addr & 0xFFFFF;
    CONSTSP(vxt_pirepheral) devl = &s->devices[s->mem_map[p]];
    vxt_byte low = devl->io.read(devl->userdata, p);
    p = (addr+1) & 0xFFFFF;
    CONSTSP(vxt_pirepheral) devh = &s->devices[s->mem_map[p]];
    vxt_byte high = devh->io.read(devh->userdata, (addr+1) & 0xFFFFF);
    return WORD(high, low);
}

void vxt_system_write_word(CONSTP(vxt_system) s, vxt_pointer addr, vxt_word data) {
    vxt_pointer p = addr &= 0xFFFFF;
    CONSTSP(vxt_pirepheral) devh = &s->devices[s->mem_map[p]];
    devh->io.write(devh->userdata, p, LBYTE(data));
    p = (addr+1) & 0xFFFFF;
    CONSTSP(vxt_pirepheral) devl = &s->devices[s->mem_map[p]];
    devl->io.write(devl->userdata, p, HBYTE(data));
}

vxt_byte system_in(CONSTP(vxt_system) s, vxt_word port) {
    CONSTSP(vxt_pirepheral) dev = &s->devices[s->io_map[port]];
    return dev->io.in(dev->userdata, port);
}

void system_out(CONSTP(vxt_system) s, vxt_word port, vxt_byte data) {
    CONSTSP(vxt_pirepheral) dev = &s->devices[s->io_map[port]];
    dev->io.out(dev->userdata, port, data);
}
