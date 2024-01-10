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
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    This product make use of the VirtualXT software emulator.
//    Visit https://virtualxt.org for more information.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <vxt/vxtu.h>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <alloca.h>
#elif defined(__NetBSD__)
	#define alloca __builtin_alloca
#endif

#include <dirent.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#define CLOSE_DIR(p) { if ((p)->dir_it) closedir((DIR*)(p)->dir_it); (p)->dir_it = NULL; }
#define COPY_VALUE(ptr, src, ty) { ty v = (src); memcpy((ptr), &v, sizeof(ty)); }

static bool case_path(char const *path, char *new_path) {
    #ifdef _WIN32
        strcpy(new_path, path);
    #else
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
    #endif
    return true;
}

static bool pattern_comp(const char *pattern, const char *name, bool root) {
    // TODO: Improve this! Might only work in FreeDOS.
    // Reference: https://devblogs.microsoft.com/oldnewthing/20071217-00/?p=24143

    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        if (root)
            return false;
    } else if (*name == '.') {
        return false;
    }

    if (!strcmp(pattern, "????????.???"))
        return true;
    return !strcasecmp(pattern, name);
}

static bool rifs_exists(const char *path) {
    char *new_path = alloca(strlen(path) + 2);
    if (case_path(path, new_path))
        return access(new_path, F_OK) == 0;
    return false;
}

static const char *rifs_copy_root(char *dest, const char *path) {
    struct stat s;
    if (!stat(path, &s)) {
        if (!S_ISDIR(s.st_mode)) {
            const char *file_name;
            if ((file_name = strrchr(path, '/'))) {
                file_name++; // Remove leading '/'
                assert(*file_name);
            } else {
                file_name = path;
            }

            memcpy(dest, path, strlen(path) - strlen(file_name));
            if (*dest == 0)
                strcpy(dest, "./");
            return file_name;
        }
    }
    strncpy(dest, path, MAX_PATH_LEN - 1);
    return NULL;
}

static vxt_word rifs_findnext(struct dos_proc *proc, vxt_byte *data) {
    if (proc->dir_it) {
        char *full_path = alloca(MAX_PATH_LEN + 257);

        for (struct dirent *dire = readdir((DIR*)proc->dir_it); dire; dire = readdir((DIR*)proc->dir_it)) {
            if (pattern_comp(proc->dir_pattern, dire->d_name, proc->is_root)) {
                memset(data, 0, 43);
                snprintf(full_path, MAX_PATH_LEN + 256, "%s/%s", proc->dir_path, dire->d_name);

                // Reference: https://www.stanislavs.org/helppc/int_21-4e.html
            
                struct stat stbuf;
                stat(full_path, &stbuf);
                bool is_dir = S_ISDIR(stbuf.st_mode);

                if (is_dir && !(proc->find_attrib & 0x10))
                    continue;

                data[0x15] = is_dir ? 0x10 : 0x0;
                time_and_data(&stbuf.st_mtime, (vxt_word*)&data[0x16], (vxt_word*)&data[0x18]);

                snprintf(full_path, MAX_PATH_LEN + 256, "%s/%s", proc->dir_path, dire->d_name);
                *(vxt_word*)&data[0x1A] = (vxt_word)(((stbuf.st_size > 0xFFFF) || !S_ISREG(stbuf.st_mode)) ? 0xFFFF : stbuf.st_size);

                char *dst = (char*)&data[0x1E];
                int ln = (int)strlen(dire->d_name);
                int max_ln = is_dir ? 8 : 12;
                int cap_ln = (ln > max_ln) ? max_ln : ln;
                memset(dst, 0, 13);

                for (int i = 0; i < cap_ln; i++)
                    dst[i] = (vxt_byte)toupper(dire->d_name[i]);
                if (ln > max_ln) {
                    dst[7] = '~';
                    if (!is_dir) {
                        dst[8] = '.';
                        dst[9] = (char)toupper(dire->d_name[ln - 3]);
                        dst[10] = (char)toupper(dire->d_name[ln - 2]);
                        dst[11] = (char)toupper(dire->d_name[ln - 1]);
                    }
                }
                return 0;
            }
        }
        CLOSE_DIR(proc);
    }
    return 12; // No more files
}

static vxt_word rifs_findfirst(struct dos_proc *proc, vxt_word attrib, const char *path, bool root, vxt_byte *data) {
    CLOSE_DIR(proc);
    char *new_path = alloca(strlen(path) + 2);

    if (case_path(path, new_path)) {
        const char *pattern = strrchr(path, '/'); // Use path because dirname may modify new_path.
        const char *dir_path = dirname(new_path);
        
        if ((proc->dir_it = opendir(dir_path))) {
            proc->is_root = root;
            proc->find_attrib = attrib;
            strncpy(proc->dir_path, dir_path, sizeof(proc->dir_path) - 1);
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

                struct stat stbuf;
                stat(new_path, &stbuf);
                vxt_word time, date;
                time_and_data(&stbuf.st_mtime, &time, &date);

                *(vxt_word*)data = time; data += 2; // Time
                *(vxt_word*)data = date; data += 2; // Date
                COPY_VALUE(data, (stbuf.st_size > 0xFFFF) ? 0 : (vxt_dword)stbuf.st_size, vxt_dword); // Size
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
        #ifdef _WIN32
            if (!mkdir(new_path))
        #else
            if (!mkdir(new_path, S_IRWXU))
        #endif
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
