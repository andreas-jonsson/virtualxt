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

#ifdef VXT_NO_LIBC

vxt_byte *vxtu_read_file(vxt_allocator *alloc, const char *file, int *size) {
    (void)alloc; (void)file; (void)size;
    VXT_LOG("ERROR: libvxt is built with VXT_NO_LIBC!");
    return NULL;
}

#else

vxt_byte *vxtu_read_file(vxt_allocator *alloc, const char *file, int *size) {
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
