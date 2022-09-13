/*
Copyright (c) 2019-2022 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "vxtp.h"

static bool rifs_exists(const char *path) {
    (void)path;
    return false;
}

static vxt_word rifs_rmdir(const char *path) {
    (void)path;
    return 0;
}

static vxt_word rifs_mkdir(const char *path) {
    (void)path;
    return 0;
}

static vxt_word rifs_rename(const char *from, const char *to) {
    (void)from; (void)to;
    return 0;
}

static vxt_word rifs_delete(const char *path) {
    (void)path;
    return 0;
}
