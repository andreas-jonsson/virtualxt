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

#include <alloca.h>
#include <strings.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define COPY_VALUE(ptr, src, ty) { ty v = (src); memcpy((ptr), &v, sizeof(ty)); }

static bool case_path(char const *path, char *new_path) {
    char *p = strcpy(alloca(strlen(path) + 1), path);
    int new_path_ln = 0;
    
    DIR *dir;
    if (p[0] == '/') {
        dir = opendir("/");
        p = p + 1;
    } else {
        dir = opendir(".");
        new_path[0] = '.';
        new_path[1] = 0;
        new_path_ln = 1;
    }
    
    bool tail = false;
    p = strtok(p, "/");
    while (p) {
        if (!dir)
            return false;
        
        if (tail) {
            closedir(dir);
            return false;
        }
        
        new_path[new_path_ln++] = '/';
        new_path[new_path_ln] = 0;
        
        struct dirent *dire = readdir(dir);
        while (dire) {
            if (!strcasecmp(p, dire->d_name)) {
                strcpy(new_path + new_path_ln, dire->d_name);
                new_path_ln += strlen(dire->d_name);

                closedir(dir);
                dir = opendir(new_path);
                break;
            }
            dire = readdir(dir);
        }
        
        if (!dire) {
            strcpy(new_path + new_path_ln, p);
            new_path_ln += strlen(p);
            tail = true;
        }
        p = strtok(NULL, "/");
    }
    
    if (dir)
        closedir(dir);
    return true;
}

static bool rifs_exists(const char *path) {
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path))
        return access(new_path, F_OK) == 0;
    return false;
}

static vxt_dword get_file_size(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return (size > 0xFFFF) ? 0 : (vxt_word)size;
}

static vxt_word rifs_openfile(struct dos_proc *proc, vxt_word attrib, const char *path, vxt_byte *data) {
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path)) {
        for (vxt_word i = 0; i < MAX_OPEN_FILES; i++) {
            FILE *fp = proc->files[i];
            if (!fp) {
                const char *mode = "rb+";
                if (attrib == 0)
                    mode = "rb";
                else if (attrib == 1)
                    mode = "wb";
                fp = proc->files[i] = fopen(new_path, mode);
                if (!fp)
                    return 2; // File not found

                *(vxt_word*)data = i; data += 2; // FP
                *(vxt_word*)data = attrib; data += 2; // Attribute
                *(vxt_word*)data = 0; data += 2; // Time
                *(vxt_word*)data = 0; data += 2; // Date
                COPY_VALUE(data, get_file_size(fp), vxt_dword); // Size
                return 0;
            }
        }
        return 4; // Too many open files
    }
    return 3; // Path not found
}

static vxt_word rifs_rmdir(const char *path) {
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path)) {
        if (!rmdir(new_path))
            return 0;
    }
    return 3; // Path not found
}

static vxt_word rifs_mkdir(const char *path) {
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path)) {
        if (!mkdir(new_path, S_IRWXU))
            return 0;
    }
    return 3; // Path not found
}

static vxt_word rifs_rename(const char *from, const char *to) {
    char *new_path = alloca(strlen(from) + 2);
    if (case_path(from, new_path)) {
        if (!rename(new_path, to))
            return 0;
        return 2; // File not found
    }
    return 3; // Path not found
}

static vxt_word rifs_delete(const char *path) {
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path)) {
        if (!remove(new_path))
            return 0;
        return 2; // File not found
    }
    return 3; // Path not found
}
