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

#include "vxtp.h"

#define CLOSE_DIR(p)

static bool rifs_exists(const char *path) {
    (void)path;
    return false;
}

static const char *rifs_copy_root(char *dest, const char *path) {
    (void)path;
    *dest = 0;
    return NULL;
}

static vxt_word rifs_findnext(struct dos_proc *proc, vxt_byte *data) {
    (void)proc; (void)data;
    time_t t = time(NULL);
    time_and_data(&t, NULL, NULL);
    return 0x16;
}

static vxt_word rifs_findfirst(struct dos_proc *proc, vxt_word attrib, const char *path, bool root, vxt_byte *data) {
    (void)proc; (void)attrib; (void)path; (void)root; (void)data;
    return 0x16;
}

static vxt_word rifs_openfile(struct dos_proc *proc, vxt_word attrib, const char *path, vxt_byte *data) {
    (void)proc; (void)attrib; (void)path; (void)data;
    return 0x16;
}

static vxt_word rifs_rmdir(const char *path) {
    (void)path;
    return 0x16;
}

static vxt_word rifs_mkdir(const char *path) {
    (void)path;
    return 0x16;
}

static vxt_word rifs_rename(const char *from, const char *to) {
    (void)from; (void)to;
    return 0x16;
}

static vxt_word rifs_delete(const char *path) {
    (void)path;
    return 0x16;
}
