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

#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vxt.h"

struct vxtu_debugger_interface {
    const char *trace;
    bool (*pdisasm)(vxt_system*, const char*, vxt_pointer, int, int);
    const char *(*getline)(void);
    int (*print)(const char*, ...);
};

#if defined(VXT_LIBC) || defined(VXT_CLIB_IO)
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

extern struct vxt_pirepheral *vxtu_create_memory_device(vxt_allocator *alloc, vxt_pointer base, int amount, bool read_only);
extern void *vxtu_memory_internal_pointer(const struct vxt_pirepheral *p);
extern bool vxtu_memory_device_fill(struct vxt_pirepheral *p, const vxt_byte *data, int size);

extern struct vxt_pirepheral *vxtu_create_debugger(vxt_allocator *alloc, const struct vxtu_debugger_interface *interface);
extern void vxtu_debugger_interrupt(struct vxt_pirepheral *dbg);

extern struct vxt_pirepheral *vxtu_create_pic(vxt_allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif
