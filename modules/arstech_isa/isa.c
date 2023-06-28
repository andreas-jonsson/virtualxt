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

#include <vxt/vxtu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct arstech_isa {
    struct {
        unsigned char (*in)(unsigned short port);
        void (*out)(unsigned short port, unsigned char data);
        unsigned char (*read)(unsigned long phadr);
        void (*write)(unsigned long phadr, unsigned char data);
        unsigned long (*get_irq_dma)(void);
        int (*initialize)(void);
        void (*shutdown)(void);
    } api;

    struct {
        vxt_word io_start;
        vxt_word io_end;
        vxt_pointer mem_start;
        vxt_pointer mem_end;
        int irq_poll;
    } config;
};

static vxt_byte in(struct arstech_isa *d, vxt_word port) {
    return d->api.in(port);
}

static void out(struct arstech_isa *d, vxt_word port, vxt_byte data) {
    d->api.out(port, data);
}

static vxt_byte read(struct arstech_isa *d, vxt_pointer addr) {
    return d->api.read(addr);
}

static void write(struct arstech_isa *d, vxt_pointer addr, vxt_byte data) {
    d->api.write(addr, data);
}

static vxt_error install(struct arstech_isa *d, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(d);
    if (d->config.io_start || d->config.io_end)
        vxt_system_install_io(s, p, d->config.io_start, d->config.io_end);
    if (d->config.mem_start || d->config.mem_end)
        vxt_system_install_mem(s, p, d->config.mem_start, d->config.mem_end);
    if (d->config.irq_poll >= 0)
        vxt_system_install_timer(s, p, (unsigned int)d->config.irq_poll);

    if (!d->api.initialize()) {
        VXT_LOG("ERROR: Could not initialize Arstech USB library!");
        return VXT_USER_ERROR(0);
    }
    return VXT_NO_ERROR;
}

static vxt_error reset(struct arstech_isa *d) {
    d->api.shutdown();
    return d->api.initialize() ? VXT_NO_ERROR : VXT_USER_ERROR(0);
}

static vxt_error destroy(struct arstech_isa *d) {
    d->api.shutdown();
    vxt_system_allocator(VXT_GET_SYSTEM(d))(VXT_GET_PIREPHERAL(d), 0);
    return VXT_NO_ERROR;
}

static vxt_error timer(struct arstech_isa *d, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    unsigned long irq = d->api.get_irq_dma();
    for (int i = 0; i < 8; i++) {
        if (irq & (1 << i))
            vxt_system_interrupt(VXT_GET_SYSTEM(d), i);
    }
    return VXT_NO_ERROR;
}

static const char *name(struct arstech_isa *d) {
    (void)d; return "ISA Passthrough (Arstech USB)";
}

static vxt_error config(struct arstech_isa *d, const char *section, const char *key, const char *value) {
    if (!strcmp("arstech_isa", section)) {
        if (!strcmp("memory", key)) {
            if (sscanf(value, "%x,%x", &d->config.mem_start, &d->config.mem_end) < 2)
                d->config.mem_end = d->config.mem_start + 1;
        } else if (!strcmp("port", key)) {
            if (sscanf(value, "%hx,%hx", &d->config.io_start, &d->config.io_end) < 2)
                d->config.io_end = d->config.io_start + 1;
        } else if (!strcmp("irq-poll", key)) {
            d->config.irq_poll = atoi(value);
        }
    }
    return VXT_NO_ERROR;
}

#ifdef _WIN32
    #include <windows.h>

    static void print_error(void) {
        char buffer[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
        VXT_LOG("ERROR: %s", buffer);
    }

    #define LOADLIB(name) HMODULE lib = LoadLibraryA(name);     \
        if (!lib) { print_error(); return NULL; }               \

    #define LOADSYM(func, sym) DEVICE->api.func = (void*)GetProcAddress(lib, sym);  \
        if (!DEVICE->api.func) { print_error(); FreeLibrary(lib); return NULL; }    \

#else
    #include <dlfcn.h>

    #define LOADLIB(name) void *lib = dlopen(ARGS, RTLD_LAZY);          \
        if (!lib) { VXT_LOG("ERROR: %s", dlerror()); return NULL; }     \

    #define LOADSYM(func, sym) DEVICE->api.func = dlsym(lib, sym);                                  \
        if (!DEVICE->api.func) { VXT_LOG("ERROR: %s", dlerror()); dlclose(lib); return NULL; }      \

#endif

VXTU_MODULE_CREATE(arstech_isa, {
    LOADLIB(ARGS);

    LOADSYM(initialize, "ArsInit");
    LOADSYM(shutdown, "ArsExit");
    LOADSYM(in, "in8");
    LOADSYM(out, "out8");
    LOADSYM(read, "rd8");
    LOADSYM(write, "wr8");
    LOADSYM(get_irq_dma, "GetIrqDma");

    DEVICE->config.irq_poll = -1;

    PIREPHERAL->install = &install;
    PIREPHERAL->destroy = &destroy;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->config = &config;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->name = &name;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
    PIREPHERAL->io.read = &read;
    PIREPHERAL->io.write = &write;
})
