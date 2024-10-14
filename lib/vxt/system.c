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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

#include "common.h"
#include "system.h"
#include "testing.h"

#if !defined(VXT_NO_LIBC) && !defined(VXT_NO_LOGGING)
   #include <stdarg.h>
   #include <stdio.h>
   static int libc_print(const char *fmt, ...) {
      va_list args;
      va_start(args, fmt);
      int ret = vprintf(fmt, args);
      va_end(args);
      return ret;
   }
   VXT_API int (*_vxt_logger)(const char*, ...) = &libc_print;
#else
   static int no_print(const char *fmt, ...) {
      UNUSED(fmt);
      return -1;
   }
   VXT_API int (*_vxt_logger)(const char*, ...) = &no_print;
#endif

static vxt_error update_timers(CONSTP(vxt_system) s, int ticks) {
    for (int i = 0; i < s->num_timers; i++) {
        struct timer *t = &s->timers[i];
        t->ticks += ticks;
        if (UNLIKELY(t->ticks >= (INT64)(t->interval * (double)s->frequency))) {
            vxt_error err = t->dev->timer(vxt_peripheral_device(t->dev), t->id, (int)t->ticks);
            if (err != VXT_NO_ERROR)
                return err;
            t->ticks = 0;
        }
    }
    return VXT_NO_ERROR;
}

VXT_API const char *vxt_error_str(vxt_error err) {
    #define ERROR_TEXT(id, name, text) case id: return text;
    switch (err) {
        _VXT_ERROR_CODES(ERROR_TEXT)
        case _VXT_NUM_ERRORS: break;
    }
    #undef ERROR_TEXT
    return "user error";
}

VXT_API const char *vxt_lib_version(void) { return VXT_VERSION; }
VXT_API int vxt_lib_version_major(void) { return VXT_VERSION_MAJOR; }
VXT_API int vxt_lib_version_minor(void) { return VXT_VERSION_MINOR; }
VXT_API int vxt_lib_version_patch(void) { return VXT_VERSION_PATCH; }

VXT_API void vxt_set_logger(int (*f)(const char*, ...)) {
    _vxt_logger = f;
}

VXT_API vxt_system *vxt_system_create(vxt_allocator *alloc, int frequency, struct vxt_peripheral * const devs[]) {
    vxt_system *s = (vxt_system*)alloc(NULL, sizeof(struct system));
    vxt_memclear(s, sizeof(struct system));
    s->alloc = alloc;
    s->frequency = frequency;
	s->cpu.s = s;

    int i = 1;
    for (; devs && devs[i-1]; i++) {
        s->devices[i] = devs[i-1];
        struct peripheral *internal = (struct peripheral*)s->devices[i];
        if (internal->sig != PERIPHERAL_SIGNATURE) {
			VXT_LOG("Peripheral was not correctly allocated!");
			return NULL;
		}
        
        internal->idx = i;
        internal->s = s;
    }
    s->num_devices = i;

    for (i = 0; i < MAX_TIMERS; i++)
        s->timers[i].id = VXT_INVALID_TIMER_ID;

    // Always init dummy device 0. Depends on memset!
    s->devices[0] = (struct vxt_peripheral*)&s->dummy;
    init_dummy_device(s);
    return s;
}

VXT_API vxt_error vxt_system_configure(vxt_system *s, const char *section, const char *key, const char *value) {
    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_peripheral) d = s->devices[i];
        if (d->config) {
            vxt_error err = d->config(vxt_peripheral_device(d), section, key, value);
            if (err) return err;
        }
    }
    return VXT_NO_ERROR;
}

VXT_API vxt_error _vxt_system_initialize(CONSTP(vxt_system) s, unsigned reg_size, int v_major, int v_minor) {
    if (sizeof(struct vxt_registers) != reg_size)
        return VXT_INVALID_REGISTER_PACKING;

    if (VXT_VERSION_MAJOR != v_major || VXT_VERSION_MINOR != v_minor)
        return VXT_INVALID_VERSION;

    vxt_system_install_monitor(s, NULL, "AX", &s->cpu.regs.ax, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "BX", &s->cpu.regs.bx, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "CX", &s->cpu.regs.cx, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "DX", &s->cpu.regs.dx, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);

    vxt_system_install_monitor(s, NULL, "CS", &s->cpu.regs.cs, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "SS", &s->cpu.regs.ss, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "DS", &s->cpu.regs.ds, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "ES", &s->cpu.regs.es, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);

    vxt_system_install_monitor(s, NULL, "SP", &s->cpu.regs.sp, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "BP", &s->cpu.regs.bp, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "SI", &s->cpu.regs.si, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "DI", &s->cpu.regs.di, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);

    vxt_system_install_monitor(s, NULL, "IP", &s->cpu.regs.ip, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, NULL, "Flags", &s->cpu.regs.ip, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_BINARY);

    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_peripheral) d = s->devices[i];
        if (d->install) {
            vxt_error err = d->install(vxt_peripheral_device(d), s);
            if (err) return err;
        }
        if (vxt_peripheral_class(d) == VXT_PCLASS_PIC)
            s->cpu.pic = d;
    }

    if (s->cpu.validator)
        s->cpu.validator->initialize(s, s->cpu.validator->userdata);
    return VXT_NO_ERROR;
}

TEST(system_initialize,
    CONSTP(vxt_system) sp = vxt_system_create(TALLOC, VXT_DEFAULT_FREQUENCY, NULL);
    TENSURE(sp);
    TENSURE_NO_ERR(vxt_system_initialize(sp));
    vxt_system_destroy(sp);
)

VXT_API vxt_error vxt_system_destroy(CONSTP(vxt_system) s) {
    if (s->cpu.validator) {
        vxt_error (*destroy)(void*) = s->cpu.validator->destroy;
        destroy(s->cpu.validator->userdata);
    }

    for (int i = 0; i < s->num_devices; i++) {
        struct vxt_peripheral *d = s->devices[i];
        if (d->destroy) {
            vxt_error (*destroy)(void*) = d->destroy;
            vxt_error err = destroy(vxt_peripheral_device(d));
            if (err) return err;
        } else {
            s->alloc(d, 0);
        }
    }
    s->alloc(s, 0);
    return VXT_NO_ERROR;
}

VXT_API struct vxt_registers *vxt_system_registers(CONSTP(vxt_system) s) {
    return &s->cpu.regs;
}

VXT_API vxt_allocator *vxt_system_allocator(vxt_system *s) {
    return s->alloc;
}

VXT_API struct vxt_peripheral *vxt_allocate_peripheral(vxt_allocator *alloc, size_t size) {
	size_t sz = sizeof(struct peripheral) + size;
	struct peripheral *p = (struct peripheral*)alloc(NULL, sz);
	
	vxt_memclear(p, sz);
    p->sig = PERIPHERAL_SIGNATURE;
    p->size = sz;
	return (struct vxt_peripheral*)p;
}

VXT_API void vxt_system_reset(CONSTP(vxt_system) s) {
    cpu_reset(&s->cpu);
    s->a20 = false;
    
    for (int i = 0; i < s->num_devices; i++) {
        CONSTSP(vxt_peripheral) d = s->devices[i];
        if (d->reset)
            d->reset(vxt_peripheral_device(d));
    }
}

VXT_API struct vxt_step vxt_system_step(CONSTP(vxt_system) s, int cycles) {
	int oldc = 0;
	struct vxt_step step = {0};
	cpu_reset_cycle_count(&s->cpu);

	for (;;) {
		int newc = cpu_step(&s->cpu);
		int c = newc - oldc;
		oldc = newc;
		step.cycles += c;
		step.halted = s->cpu.halt;
		step.interrupt = s->cpu.interrupt;
		step.int28 = s->cpu.int28;
		step.invalid = s->cpu.invalid;

		if (UNLIKELY((step.err = update_timers(s, c)) != VXT_NO_ERROR))
			return step;

		if (newc >= cycles)
			return step;
	}
}

VXT_API void vxt_system_set_tracer(vxt_system *s, void (*tracer)(vxt_system*,vxt_pointer,vxt_byte)) {
    s->cpu.tracer = tracer;
}

VXT_API void vxt_system_set_validator(CONSTP(vxt_system) s, const struct vxt_validator *intrf) {
    s->cpu.validator = intrf;
}

VXT_API void vxt_system_set_userdata(CONSTP(vxt_system) s, void *data) {
    s->userdata = data;
}

VXT_API void *vxt_system_userdata(CONSTP(vxt_system) s) {
    return s->userdata;
}

VXT_API const vxt_byte *vxt_system_io_map(vxt_system *s) {
    return s->io_map;
}

VXT_API const vxt_byte *vxt_system_mem_map(vxt_system *s) {
    return s->mem_map;
}

VXT_API const struct vxt_monitor *vxt_system_monitor(vxt_system *s, vxt_byte idx) {
    struct vxt_monitor *m = &s->monitors[idx];
    return m->flags ? m : NULL;
}

VXT_API struct vxt_peripheral *vxt_system_peripheral(vxt_system *s, vxt_byte idx) {
    return s->devices[idx];
}

VXT_API struct vxt_peripheral *vxt_device_peripheral(void *dev) {
	struct vxt_peripheral *p = (struct vxt_peripheral*)((char*)(dev) - sizeof(struct peripheral));
	VERIFY_PERIPHERAL(p, NULL);
	return p;
}

VXT_API vxt_system *vxt_peripheral_system(const struct vxt_peripheral *p) {
	VERIFY_PERIPHERAL(p, NULL);
	return ((struct peripheral*)p)->s;
}

VXT_API void *vxt_peripheral_device(const struct vxt_peripheral *p) {
    VERIFY_PERIPHERAL(p, NULL);
    return (void*)((char*)p + sizeof(struct peripheral));
}

VXT_API const char *vxt_peripheral_name(struct vxt_peripheral *p) {
    VERIFY_PERIPHERAL(p, NULL);
    return p->name ? p->name(vxt_peripheral_device(p)) : "unknown device";
}

VXT_API enum vxt_pclass vxt_peripheral_class(struct vxt_peripheral *p) {
	VERIFY_PERIPHERAL(p, VXT_PCLASS_GENERIC);
    return p->pclass ? p->pclass(vxt_peripheral_device(p)) : VXT_PCLASS_GENERIC;
}

VXT_API void vxt_system_wait(CONSTP(vxt_system) s, int cycles) {
    s->cpu.cycles += cycles;
}

VXT_API void vxt_system_interrupt(CONSTP(vxt_system) s, int n) {
	s->cpu.halt = false;
    if (s->cpu.pic)
        s->cpu.pic->pic.irq(vxt_peripheral_device(s->cpu.pic), n);
}

VXT_API int vxt_system_frequency(CONSTP(vxt_system) s) {
    return s->frequency;
}

VXT_API void vxt_system_set_frequency(CONSTP(vxt_system) s, int freq) {
    s->frequency = freq;
}

VXT_API void vxt_system_set_a20(CONSTP(vxt_system) s, bool enable) {
	s->a20 = enable;
}

VXT_API void vxt_system_install_monitor(CONSTP(vxt_system) s, struct vxt_peripheral *dev, const char *name, void *reg, enum vxt_monitor_flag flags) {
	if (s->num_monitors < VXT_MAX_MONITORS)
		s->monitors[s->num_monitors++] = (struct vxt_monitor){ dev ? vxt_peripheral_name(dev) : "CPU", name, reg, flags };
}

VXT_API vxt_timer_id vxt_system_install_timer(CONSTP(vxt_system) s, struct vxt_peripheral *dev, unsigned int us) {
    VERIFY_PERIPHERAL(dev, VXT_INVALID_TIMER_ID);
    if (s->num_timers >= MAX_TIMERS)
        return VXT_INVALID_TIMER_ID;
        
    struct timer *t = &s->timers[s->num_timers];
    t->ticks = 0;
    t->interval = (double)us / 1000000.0;
    t->dev = dev;
    t->id = (vxt_timer_id)s->num_timers++;
    return t->id;
}

VXT_API bool vxt_system_set_timer_interval(vxt_system *s, vxt_timer_id id, unsigned int us) {
    if (s->num_timers >= MAX_TIMERS)
        return false;

    struct timer *t = &s->timers[id];
    if (t->id == VXT_INVALID_TIMER_ID)
        return false;

    t->ticks = 0;
    t->interval = (double)us / 1000000.0;
    return true;
}

VXT_API void vxt_system_install_io_at(CONSTP(vxt_system) s, struct vxt_peripheral *dev, vxt_word addr) {
	VERIFY_PERIPHERAL(dev,);
	s->io_map[addr] = ((struct peripheral*)dev)->idx;
}

VXT_API void vxt_system_install_io(CONSTP(vxt_system) s, struct vxt_peripheral *dev, vxt_word from, vxt_word to) {
	VERIFY_PERIPHERAL(dev,);
	int i = (int)from;
	while (i <= (int)to)
		s->io_map[i++] = ((struct peripheral*)dev)->idx;
}

VXT_API void vxt_system_install_mem(CONSTP(vxt_system) s, struct vxt_peripheral *dev, vxt_pointer from, vxt_pointer to) {
	VERIFY_PERIPHERAL(dev,);
	if ((from | to) & ~0xFFFFF)
		VXT_LOG("ERROR: Trying to register memory above 1MB!");
	
	if ((from | (to + 1)) & 0xF)
		VXT_LOG("ERROR: Trying to register unaligned address!");

	from = from >> 4;
	to = to >> 4;
	while (from <= to)
		s->mem_map[from++] = ((struct peripheral*)dev)->idx;
}

VXT_API vxt_byte vxt_system_read_byte(CONSTP(vxt_system) s, vxt_pointer addr) {
	if (!s->a20)
		addr &= 0xEFFFFF;
		
	if (addr >= 0x100000) {
		addr -= 0x100000;
		return (addr >= EXT_MEM_SIZE) ? 0xFF : s->ext_mem[addr];
	}

	CONSTSP(vxt_peripheral) dev = s->devices[s->mem_map[addr >> 4]];
	return dev->io.read(vxt_peripheral_device(dev), addr);
}

VXT_API void vxt_system_write_byte(CONSTP(vxt_system) s, vxt_pointer addr, vxt_byte data) {
	if (!s->a20)
		addr &= 0xEFFFFF;
		
	if (addr >= 0x100000) {
		addr -= 0x100000;
		if (addr < EXT_MEM_SIZE)
			s->ext_mem[addr] = data;
		return;
	}
	
	CONSTSP(vxt_peripheral) dev = s->devices[s->mem_map[addr >> 4]];
	dev->io.write(vxt_peripheral_device(dev), addr, data);
}

vxt_byte system_in(CONSTP(vxt_system) s, vxt_word port) {
    CONSTSP(vxt_peripheral) dev = s->devices[s->io_map[port]];
    s->cpu.bus_transfers++;
    VALIDATOR_DISCARD(&s->cpu);
    return dev->io.in(vxt_peripheral_device(dev), port);
}

void system_out(CONSTP(vxt_system) s, vxt_word port, vxt_byte data) {
    CONSTSP(vxt_peripheral) dev = s->devices[s->io_map[port]];
    s->cpu.bus_transfers++;
    VALIDATOR_DISCARD(&s->cpu);
    dev->io.out(vxt_peripheral_device(dev), port, data);
}
