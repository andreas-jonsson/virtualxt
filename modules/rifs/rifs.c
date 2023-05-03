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
#include "crc32.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 0x10000
#define MAX_PATH_LEN 1024
#define MAX_DOS_PROC 256
#define MAX_OPEN_FILES 256

VXT_PACK(struct rifs_packet {
    vxt_byte packetID[2];   // 'K', 'Y': Request from server.
                            // 'L', 'Y': Reply from server.
                            // 'R', 'P': Resent last packet, either direction.

    vxt_word length;        // Total number of bytes in this block, incl header.
    vxt_word notlength;     // ~Length (used for checking).
    vxt_word cmd;           // Command to execute / result.
    vxt_word _;             // Machine ID of block sender. NOT USED!
    vxt_word __;            // Machine ID of intended reciever. NOT USED!
    vxt_word process_id;    // ID of sending process. (for closeall)
    vxt_dword crc32;        // CRC-32 of entire block. (calculated with crc32 set to zero)
    vxt_byte data[];        // Payload data.
});

enum {
    IFS_RMDIR,
    IFS_MKDIR,
    IFS_CHDIR,
    IFS_CLOSEFILE,
    IFS_COMMITFILE,
    IFS_READFILE,
    IFS_WRITEFILE,
    IFS_LOCKFILE,
    IFS_UNLOCKFILE,
    IFS_GETSPACE,
    IFS_SETATTR,
    IFS_GETATTR,
    IFS_RENAMEFILE,
    IFS_DELETEFILE,
    IFS_OPENFILE,
    IFS_CREATEFILE,
    IFS_FINDFIRST,
    IFS_FINDNEXT,
    IFS_CLOSEALL,
    IFS_EXTOPEN
};

struct dos_proc {
    bool active;
    vxt_word id;
    FILE *files[MAX_OPEN_FILES];
    
    bool is_root;
    vxt_word find_attrib;

    void *dir_it;
    char dir_pattern[13];
    char dir_path[MAX_PATH_LEN];
};

static void time_and_data(time_t *mod_time, vxt_word *time_out, vxt_word *date_out) {
    struct tm *timeinfo = localtime(mod_time);
    if (time_out) {
        *time_out = (timeinfo->tm_hour & 0x1F) << 11;
        *time_out |= (timeinfo->tm_min & 0x3F) << 5;
        *time_out |= (timeinfo->tm_sec / 2) & 0x1F;
    }
    if (date_out) {
        *date_out = ((timeinfo->tm_year - 80) & 0x7F) << 9;
        *date_out |= ((timeinfo->tm_mon + 1) & 0xF) << 5;
        *date_out |= timeinfo->tm_mday & 0x1F;
    }
}

#if defined(_WIN32) || defined(__linux__)
    #include "rifs_win_nix.inl"
#else
    #include "rifs_dummy.inl"
#endif

VXT_PIREPHERAL(rifs, {
    vxt_word base_port;
    vxt_byte registers[8];
    char root_path[MAX_PATH_LEN];
    bool dlab;
    bool readonly;

    vxt_byte buffer_input[BUFFER_SIZE];
    int buffer_input_len;

    vxt_byte buffer_output[BUFFER_SIZE];
    int buffer_output_len;

    char path_scratchpad[MAX_PATH_LEN];
    struct dos_proc processes[MAX_DOS_PROC];
})

// Expects the 'cmd' and 'process_id' to be set by caller.
static void server_response(struct rifs *fs, const struct rifs_packet *pk, int payload_size) {
    assert(!fs->buffer_input_len);

    int size = payload_size + sizeof(struct rifs_packet);
    assert(size <= BUFFER_SIZE);
    fs->buffer_input_len = size;

    struct rifs_packet *dest = (struct rifs_packet*)fs->buffer_input;
    memcpy(dest, pk, size);
    dest->length = (vxt_word)size;
    dest->notlength = ~dest->length;

    dest->packetID[0] = 'L'; dest->packetID[1] = 'Y';

    dest->crc32 = 0;
    dest->crc32 = crc32(fs->buffer_input, dest->length);
}

static bool verify_packet(struct rifs_packet *pk) {
    vxt_word len = ~pk->notlength;
    if (pk->length != len) {
        fprintf(stderr, "RIFS packet of invalid size!\n");
        return false;
    }

    vxt_dword crc = pk->crc32;
    pk->crc32 = 0;

    if (crc != crc32((vxt_byte*)pk, len)) {
        fprintf(stderr, "RIFS packet CRC failed!\n");
        return false;
    }

    pk->crc32 = crc; // Restore the CRC if needed later.
    return true;
}

static const char *host_path(struct rifs *fs, const char *path) {
    strncpy(fs->path_scratchpad, fs->root_path, sizeof(fs->path_scratchpad));

    // Remove drive letter.
    if ((strlen(path) >= 2) && (path[1] == ':'))
        path = path + 2;
    else
        fprintf(stderr, "RIFS path is not absolute!\n");
       
    strncat(fs->path_scratchpad, path, sizeof(fs->path_scratchpad) - 1);

    // Convert slashes.
    int ln = strlen(fs->path_scratchpad);
    for (int i = 0; i < ln; i++) {
        if (fs->path_scratchpad[i] == '\\')
            fs->path_scratchpad[i] = '/';
    }
    return fs->path_scratchpad;
}

static struct dos_proc *get_proc(struct rifs *fs, vxt_word id) {
    struct dos_proc *inactive = NULL;
    for (int i = 0; i < MAX_DOS_PROC; i++) {
        struct dos_proc *proc = &fs->processes[i];
        if (!inactive && !proc->active)
            inactive = proc;
        if (proc->id == id && proc->active)
            return proc;
    }

    // Create new proc.
    inactive->id = id;
    inactive->active = true;
    memset(inactive->files, 0, sizeof(inactive->files));
    return inactive;
}

static void process_request(struct rifs *fs, struct rifs_packet *pk) {
    int data_size = pk->length - sizeof(struct rifs_packet);
    struct dos_proc *proc = get_proc(fs, pk->process_id);
    if (!proc) {
        fprintf(stderr, "To many RIFS procs!\n");
        return;
    }

    #define BREAK_RO(size) {                    \
        if (fs->readonly) {                     \
            pk->cmd = 5;                        \
            server_response(fs, pk, (size));    \
            break;                              \
        }                                       \
    }                                           \

    switch (pk->cmd) {
        case IFS_RMDIR: BREAK_RO(0)
            pk->cmd = rifs_rmdir(host_path(fs, (char*)pk->data));
            server_response(fs, pk, 0);
            break;
        case IFS_MKDIR: BREAK_RO(0)
            pk->cmd = rifs_mkdir(host_path(fs, (char*)pk->data));
            server_response(fs, pk, 0);
            break;
        case IFS_CHDIR:
            pk->cmd = rifs_exists(host_path(fs, (char*)pk->data)) ? 0 : 3;
            server_response(fs, pk, 0);
            break;
        case IFS_CLOSEFILE:
        {
            vxt_word idx = *(vxt_word*)pk->data;
            if (idx >= MAX_OPEN_FILES) {
                pk->cmd = 6; // Invalid handle
            } else {
                pk->cmd = 0;
                fclose(proc->files[idx]);
            }
            proc->files[idx] = NULL;
            server_response(fs, pk, 0);
            break;
        }
        case IFS_READFILE:
        {
            vxt_word *dest = (vxt_word*)pk->data;
            vxt_word idx = *dest; dest++;
            vxt_dword pos = (((vxt_dword)dest[1]) << 16) | (vxt_dword)dest[0]; dest += 2;
            vxt_word ln = *dest;

            *(vxt_word*)pk->data = 0;

            if (idx >= MAX_OPEN_FILES) {
                pk->cmd = 6; // Invalid handle
                server_response(fs, pk, 2);
                break;
            }

            FILE *fp = proc->files[idx];
            if (!fp || fseek(fp, (long)pos, SEEK_SET)) {
                pk->cmd = 0x19; // Seek error
                server_response(fs, pk, 2);
                break;
            }

            assert((ln + 2) <= (vxt_word)(BUFFER_SIZE - sizeof(struct rifs_packet)));
            size_t res = fread(pk->data + 2, 1, (size_t)ln, fp);
            if ((res != (size_t)ln) && !feof(fp)) {
                pk->cmd = 0x1E; // Read fault
                server_response(fs, pk, 2);
                break;
            }

            pk->cmd = 0;
            *(vxt_word*)pk->data = (vxt_word)res;
            server_response(fs, pk, res + 2);
            break;
        }
        case IFS_WRITEFILE: BREAK_RO(2)
        {
            vxt_word *src = (vxt_word*)pk->data;
            vxt_word idx = *src; src++;
            vxt_dword pos = (((vxt_dword)src[1]) << 16) | (vxt_dword)src[0]; src += 2;
            vxt_word ln = *src; src++;

            if (idx >= MAX_OPEN_FILES) {
                pk->cmd = 6; // Invalid handle
                *(vxt_word*)pk->data = 0;
                server_response(fs, pk, 2);
                break;
            }
            
            FILE *fp = proc->files[idx];
            //if (fp && (ln == 0)) {
            //    pos = 0;
            //    fp = freopen(NULL, "wb+", fp);
            //}

            if (!fp || fseek(fp, (long)pos, SEEK_SET)) {
                pk->cmd = 0x19; // Seek error
                *(vxt_word*)pk->data = 0;
                server_response(fs, pk, 2);
                break;
            }

            size_t res = fwrite(src, 1, (size_t)ln, fp);
            *(vxt_word*)pk->data = (vxt_word)res;

            if (res != (size_t)ln) {
                pk->cmd = 0x1D; // Write fault
                server_response(fs, pk, 2);
                break;
            }

            pk->cmd = 0;
            server_response(fs, pk, 2);
            break;
        }
        case IFS_GETSPACE:
        {
            // TODO: We just fake this and say we always have 63Mb of free space. :D

            vxt_word *dest = (vxt_word*)pk->data;
            dest[0] = 63; // Available clusters
            dest[1] = 64; // Total clusters
            dest[2] = 1024; // Bytes per sector
            dest[3] = 1024; // Sectors per cluster

            pk->cmd = 0;
            server_response(fs, pk, 8);
            break;
        }
        case IFS_SETATTR: BREAK_RO(0)
            pk->cmd = rifs_exists(host_path(fs, (char*)pk->data + 2)) ? 0 : 2;
            server_response(fs, pk, 0);
            break;
        case IFS_GETATTR:
            if (rifs_exists(host_path(fs, (char*)pk->data))) {
                pk->cmd = 0;
                *(vxt_word*)pk->data = 0;
                server_response(fs, pk, 2);
            } else {
                pk->cmd = 2; // File not found
                server_response(fs, pk, 0);
            }
            break;
        case IFS_RENAMEFILE: BREAK_RO(0)
        {
            const char *new_name = (char*)pk->data + strlen((char*)pk->data) + 1;
            pk->cmd = rifs_rename(host_path(fs, (char*)pk->data), new_name);
            server_response(fs, pk, 0);
            break;
        }
        case IFS_DELETEFILE: BREAK_RO(0)
            pk->cmd = rifs_delete(host_path(fs, (char*)pk->data));
            server_response(fs, pk, 0);
            break;
        case IFS_OPENFILE:
            if (fs->readonly && (*(vxt_word*)pk->data == 1)) {
                pk->cmd = 5;
                server_response(fs, pk, 0);
                break;
            }
            pk->cmd = rifs_openfile(proc, *(vxt_word*)pk->data, host_path(fs, (char*)pk->data + 2), pk->data);
            server_response(fs, pk, pk->cmd ? 0 : 12);
            break;
        case IFS_CREATEFILE: BREAK_RO(0)
            pk->cmd = rifs_openfile(proc, 1, host_path(fs, (char*)pk->data + 2), pk->data);
            server_response(fs, pk, pk->cmd ? 0 : 12);
            break;
        case IFS_FINDFIRST:
            pk->cmd = rifs_findfirst(proc, *(vxt_word*)pk->data, host_path(fs, (char*)pk->data + 2), strcmp("Z:\\????????.???", (char*)pk->data + 2) == 0, pk->data);
            server_response(fs, pk, pk->cmd ? 0 : 43);
            break;
        case IFS_FINDNEXT:
            pk->cmd = rifs_findnext(proc, pk->data);
            server_response(fs, pk, pk->cmd ? 0 : 43);
            break;
        case IFS_CLOSEALL:
            CLOSE_DIR(proc);
            for (int i = 0; i < MAX_DOS_PROC; i++) {
                if (proc->id == fs->processes[i].id) {
                    proc->active = false;
                    for (int j = 0; j < MAX_OPEN_FILES; j++) {
                        FILE **fp = &proc->files[j];
                        if (*fp) {
                            fclose(*fp);
                            *fp = NULL;
                        }
                    }
                    break;
                }
            }
            pk->cmd = 0;
            server_response(fs, pk, 0);
            break;
        //case IFS_EXTOPEN:
        //    break;
        case IFS_COMMITFILE:
        case IFS_LOCKFILE:
        case IFS_UNLOCKFILE:
            pk->cmd = 0;
            server_response(fs, pk, 0);
            break;
        default:
            fprintf(stderr, "Unknown RIFS command: 0x%X (payload size %d)\n", pk->cmd, data_size);
            pk->cmd = 0x16; // Unknown command
            server_response(fs, pk, 0);
    }

    #undef BREAK_RO
}

static vxt_byte pop_data(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(fs, rifs, p);
    assert(fs->buffer_input_len > 0);
    vxt_byte data = *fs->buffer_input;
    memmove(fs->buffer_input, &fs->buffer_input[1], --fs->buffer_input_len);
    return data;
}

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(fs, rifs, p);
	vxt_word reg = port & 7;
	switch (reg) {
        case 0: // Serial Data Register
            if (fs->dlab)
                return fs->registers[reg];
            return fs->buffer_input_len ? pop_data(p) : 0;
        case 5: // Line Status Register
        {
            vxt_byte res = 0x60; // Always assume transmition buffer is empty.
            if (fs->buffer_input_len)
                res |= 1;
            return res;
        }
	}
	return fs->registers[reg];
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(fs, rifs, p);
	vxt_word reg = port & 7;
	fs->registers[reg] = data;

	switch (reg) {
        case 0: // Serial Data Register
            if (fs->dlab)
                return;

            if (fs->buffer_output_len >= BUFFER_SIZE) {
                fprintf(stderr, "Invalid RIFS buffer state!\n");
                fs->buffer_output_len = 0;
                return;
            }

            fs->buffer_output[fs->buffer_output_len++] = data;
            if (fs->buffer_output_len >= (int)sizeof(struct rifs_packet)) {
                struct rifs_packet *p = (struct rifs_packet*)fs->buffer_output;
                if (p->length == fs->buffer_output_len) { // Is packet ready?
                    fs->buffer_output_len = 0;
                    if (verify_packet(p))
                        process_request(fs, p);
                }
            }
            break;
        case 3: // Line Control Register
            {
                bool dlab = data & 0x80;
                if (fs->dlab != dlab) {
                    fs->dlab = dlab;
                    fs->buffer_output_len = fs->buffer_input_len = 0;
                    fprintf(stderr, "DLAB change! Assume RIFS state reset.\n");
                }
            }
            break;
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(fs, rifs, p);
    vxt_system_install_io(s, p, fs->base_port, fs->base_port + 7);
    if (!fs->readonly)
        VXT_LOG("WARNING: '%s' is writable from guest!", fs->root_path);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(fs, rifs, p);
    for (int i = 0; i < MAX_DOS_PROC; i++) {
        struct dos_proc *proc = &fs->processes[i];
        if (proc->active) {
            for (int j = 0; j < MAX_OPEN_FILES; j++) {
                if (proc->files[j])
                    fclose(proc->files[j]);
            }
            CLOSE_DIR(proc);
        }
    }
    vxt_memclear(fs->registers, sizeof(fs->registers));
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "RIFS Server";
}

static vxt_error config(struct vxt_pirepheral *p, const char *section, const char *key, const char *value) {
   VXT_DEC_DEVICE(fs, rifs, p);
    if (!strcmp("rifs", section)) {
        if (!strcmp("port", key)) {
            sscanf(value, "%hx", &fs->base_port);
        } else if (!strcmp("writable", key)) {
            fs->readonly = atoi(value) == 0;
        } else if (!strcmp("root", key)) {
            rifs_copy_root(fs->root_path, value);
        }
    }
    return VXT_NO_ERROR;
}

VXTU_MODULE_CREATE(rifs, {
    DEVICE->readonly = true;
    DEVICE->base_port = 0x178;
    rifs_copy_root(DEVICE->root_path, ".");
    
    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->config = &config;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
