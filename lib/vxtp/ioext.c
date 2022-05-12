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

// ----------------------------------
//  THIS DEVICE IS WORK-IN-PROGRESS!
// ----------------------------------

#include <stdlib.h>
#include <stdio.h>

#include "vxtp.h"

VXT_PIREPHERAL(ioext, {
	vxt_byte ret_code;
    char buffer[0x10000];
})

static const char *read_string(struct ioext *io, vxt_system *s, vxt_pointer addr) {
    *io->buffer = 0;
    for (int i = 0; i < (int)sizeof(io->buffer) - 1; i++) { 
        if (!(io->buffer[i] = vxt_system_read_byte(s, addr + i)))
            break;
    }
    return io->buffer;
}

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    return (port == 0xB5) ? (VXT_GET_DEVICE(ioext, p))->ret_code : 0;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    (void)port; (void)data;
    VXT_DEC_DEVICE(io, ioext, p);
    vxt_system *s = VXT_GET_SYSTEM(ioext, p);
    struct vxt_registers *r = vxt_system_registers(s);

    if (port != 0xB4)
        return;

    switch (r->ax) {
        case 0x4400: // Handle is device or file
            printf("Handle is device or file!\n");
            break;
        case 0x4406: // Check for EOF
            printf("Check for EOF!\n");
            break;
        case 0x4407: // Check for busy
            printf("Check for busy!\n");
            break;
        case 0x440A: // Is file on network drv
            printf("Is file on network drv!\n");
            break;
        default:
            switch (r->ah) {
                case 0xE: // Select DOS default disk
                    printf("default disk: %c\n", r->al + 'A');
                    io->ret_code = 0xFF;
                    return;
                case 0x3B: // Set DOS default directory
                    printf("default dir: %s\n", read_string(io, s, VXT_POINTER(r->ds, r->dx)));
                    io->ret_code = 0xFF;
                    return;
                case 0x3C: // Open file
                    printf("create file: %s\n", read_string(io, s, VXT_POINTER(r->ds, r->dx)));
                    break;
                case 0x3D: // Open file
                    printf("open file: %s\n", read_string(io, s, VXT_POINTER(r->ds, r->dx)));
                    break;
                case 0x3E: // Close file
                    printf("close file!\n");
                    break;
                case 0x3F: // Read file
                    printf("Read file!\n");
                    break;
                case 0x40: // Write file
                    printf("Write file!\n");
                    break;
                case 0x41: // Delete file
                    printf("delete file: %s\n", read_string(io, s, VXT_POINTER(r->ds, r->dx)));
                    break;
                case 0x42: // Seek
                    printf("seek file!\n");
                    break;
                case 0x45: // Duplicate file handle
                    printf("Duplicate file handle!\n");
                    break;
                case 0x46: // Redirect file handle I/O
                    printf("Redirect file handle I/O\n");
                    break;
                case 0x5A: // Create unique temporary file
                    printf("temporary file: %s\n", read_string(io, s, VXT_POINTER(r->ds, r->dx)));
                    break;
                case 0x5B: // Create new file (must not exist)
                    printf("create new file: %s\n", read_string(io, s, VXT_POINTER(r->ds, r->dx)));
                    break;
                case 0x68: // Flush
                    printf("Flush file!\n");
                    break;
                default:
                    io->ret_code = 0xFF;
                    return;
            }
    }

    //r->flags &= VXT_CARRY;
    io->ret_code = 0xFF;//0;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    (void)p;
    vxt_system_install_io(s, p, 0xB4, 0xB5);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    (VXT_GET_DEVICE(ioext, p))->ret_code = 0;
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(ioext, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p; return "VirtualXT File IO Extension";
}

struct vxt_pirepheral *vxtp_ioext_create(vxt_allocator *alloc) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(ioext));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(ioext));

    p->install = &install;
    p->destroy = &destroy;
    p->reset = &reset;
    p->name = &name;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}
