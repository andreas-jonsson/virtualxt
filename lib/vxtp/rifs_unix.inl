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
#include <libgen.h>
#include <sys/stat.h>

#define CLOSE_DIR(p) { if ((p)->dir_it) closedir((DIR*)(p)->dir_it); (p)->dir_it = NULL; }
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

static vxt_dword get_fp_size(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return (size > 0xFFFF) ? 0 : (vxt_word)size;
}
/*
static vxt_dword get_file_size(const char *name) {
    FILE *fp = fopen(name, "r");
    if (!fp)
        return 0;
    vxt_dword size = get_fp_size(fp);
    fclose(fp);
    return size;
}
*/
static bool rifs_exists(const char *path) {
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path))
        return access(new_path, F_OK) == 0;
    return false;
}

static const char *rifs_copy_root(char *dest, const char *path) {
    const char *file_name = NULL;
    const char *abs_path = realpath(path, NULL);

    if (abs_path) {
        FILE *fp = fopen(abs_path, "r");
        if (fp) {
            fclose(fp);
            file_name = strrchr(path, '/');
            assert(file_name);

            file_name++; // Remove leading '/'
            assert(*file_name);

            // TODO: Modify path and remove filename.
        }
        strncpy(dest, abs_path, MAX_PATH_LEN);
        free(abs_path);
        return file_name;
    }

    fprintf(stderr, "WARNING: Could not resolve absolut path: %s\n", path);
    strncpy(dest, path, MAX_PATH_LEN);
    return NULL;
}

static bool pattern_comp(const char *pattern, const char *name) {
    // TODO: Improve this! Might only work in FreeDOS.
    // Reference: https://devblogs.microsoft.com/oldnewthing/20071217-00/?p=24143
    if (!strcmp(name, ".") || !strcmp(name, ".."))
        return false;
    if (!strcmp(pattern, "????????.???"))
        return true;
    return !strcasecmp(pattern, name);
}

static vxt_word rifs_findnext(struct dos_proc *proc, vxt_byte *data) {
    if (proc->dir_it) {
        for (struct dirent *dire = readdir((DIR*)proc->dir_it); dire; dire = readdir((DIR*)proc->dir_it)) {
            if (pattern_comp(proc->dir_pattern, dire->d_name)) {
                // Reference: https://www.stanislavs.org/helppc/int_21-4e.html
                memset(data, 0, 43);
                // TODO: Need to provide full path.
                //*(vxt_word*)&data[0x1A] = get_file_size(dire->d_name);
                strncpy((char*)&data[0x1E], dire->d_name, 13);
                return 0;
            }
        }
        CLOSE_DIR(proc);
    }
    return 12; // No more files
}

static vxt_word rifs_findfirst(struct dos_proc *proc, const char *path, vxt_byte *data) {
    CLOSE_DIR(proc);
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path)) {
        const char *pattern = strrchr(path, '/'); // Use path because dirname may modify new_path.
        if ((proc->dir_it = opendir(dirname(new_path)))) {
            pattern = pattern ? pattern + 1 : "????????.???";
            strncpy(proc->dir_pattern, pattern, sizeof(proc->dir_pattern));
            return rifs_findnext(proc, data);
        }
    }
    return 3; // Path not found
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
                COPY_VALUE(data, get_fp_size(fp), vxt_dword); // Size
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
