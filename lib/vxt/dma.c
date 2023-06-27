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

struct dma {
    bool flip;
    bool mem_to_mem;

	struct {
        bool masked;
        bool auto_init;
        bool request;

        vxt_byte operation;
        vxt_byte mode;		

        vxt_dword count;
        vxt_dword reload_count;
		vxt_dword addr;
        vxt_dword reload_addr;
		vxt_dword addr_inc;
        vxt_dword page;
	} channel[4];
};

static vxt_error reset(struct dma *c) {
    vxt_memclear(c, sizeof(struct dma));
    for (int i = 0; i < 4; i++)
        c->channel[i].masked = true;
    return VXT_NO_ERROR;
}

static vxt_byte in(struct dma *c, vxt_word port) {
    if (port >= 0x80) {
        vxt_byte ch;
        switch (port & 0xF) {
            case 0x1:
                ch = 2;
                break;
            case 0x2:
                ch = 3;
                break;
            case 0x3:
                ch = 1;
                break;
            case 0x7:
                ch = 0;
                break;
            default:
                return 0xFF;
        }
	    return (vxt_byte)(c->channel[ch].page >> 16);
    } else {
        port &= 0xF;
        if (port < 8) {
            vxt_byte ch = (port >> 1) & 3;
            vxt_dword *target = (port & 1) ? &c->channel[ch].count : &c->channel[ch].addr;
            vxt_byte res = (vxt_byte)(*target >> (c->flip ? 8 : 0));
            c->flip = !c->flip;
            return res;
        } else if (port == 8) { // Status Register
            return 0xF;
        }
        return 0xFF;
    }
}

static void out(struct dma *c, vxt_word port, vxt_byte data) {
    if (port >= 0x80) {
        vxt_byte ch;
        switch (port & 0xF) {
            case 0x1:
                ch = 2;
                break;
            case 0x2:
                ch = 3;
                break;
            case 0x3:
                ch = 1;
                break;
            case 0x7:
                ch = 0;
                break;
            default:
                return;
        }
        c->channel[ch].page = (vxt_dword)data << 16;
    } else {
        switch ((port &= 0xF)) {
            case 0x0:
            case 0x1:
            case 0x2:
            case 0x3:
            case 0x4:
            case 0x5:
            case 0x6:
            case 0x7:
            {
                vxt_byte ch = (port >> 1) & 3;
                vxt_dword *target = (port & 1) ? &c->channel[ch].count : &c->channel[ch].addr;
                if (c->flip)
                    *target = (*target & 0xFF) | ((vxt_word)data << 8);
                else
                    *target = (*target & 0xFF00) | (vxt_word)data;

                *((port & 1) ? &c->channel[ch].reload_count : &c->channel[ch].reload_addr) = *target;
                c->flip = !c->flip;
                break;
            }
            case 0x8: // Command register
                c->mem_to_mem = (data & 1) != 0;
                break;
            case 0x9: // Request register
                c->channel[data & 3].request = ((data >> 2) & 1) != 0;
                break;
            case 0xA: // Mask register
                c->channel[data & 3].masked = ((data >> 2) & 1) != 0;
                break;
            case 0xB: // Mode register
                c->channel[data & 3].operation = (data >> 2) & 3;
                c->channel[data & 3].mode = (data >> 6) & 3;
                c->channel[data & 3].auto_init = ((data >> 4) & 1) != 0;
                c->channel[data & 3].addr_inc = (data & 0x20) ? 0xFFFFFFFF : 0x1;
                break;
            case 0xC: // Clear flip flop
                c->flip = false;
                break;
            case 0xD: // Master reset
                reset(c);
                break;
            case 0xF: // Write mask register
                for (int i = 0; i < 4; i++)
                    c->channel[i].masked = ((data >> i) & 1) != 0;
                break;
        }
    }
}

static vxt_error install(struct dma *c, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(c);
    vxt_system_install_io(s, p, 0x0, 0xF);
    vxt_system_install_io(s, p, 0x80, 0x8F);
    return VXT_NO_ERROR;
}

static const char *name(struct dma *c) {
    (void)c;
    return "DMA (Intel 8237)";
}

static enum vxt_pclass pclass(struct dma *c) {
    (void)c; return VXT_PCLASS_DMA;
}

static void update_count(struct dma *c, vxt_byte ch) {
    c->channel[ch].addr += c->channel[ch].addr_inc;
    c->channel[ch].count--;

    if ((c->channel[ch].count == 0xFFFF) && (c->channel[ch].auto_init)) {
        c->channel[ch].count = c->channel[ch].reload_count;
        c->channel[ch].addr = c->channel[ch].reload_addr;
    }
}

static vxt_byte dma_read(struct dma *c, vxt_byte ch) {
    ch &= 3;
    vxt_byte res = vxt_system_read_byte(VXT_GET_SYSTEM(c), c->channel[ch].page + c->channel[ch].addr);
    update_count(c, ch);
    return res;
}

static void dma_write(struct dma *c, vxt_byte ch, vxt_byte data) {
    ch &= 3;
    vxt_system_write_byte(VXT_GET_SYSTEM(c), c->channel[ch].page + c->channel[ch].addr, data);
    update_count(c, ch);
}

VXT_API struct vxt_pirepheral *vxtu_dma_create(vxt_allocator *alloc) VXT_PIREPHERAL_CREATE(alloc, dma, {
    VXT_PIREPHERAL_SET_CALLBACK(install, install);
    VXT_PIREPHERAL_SET_CALLBACK(name, name);
    VXT_PIREPHERAL_SET_CALLBACK(pclass, pclass);
    VXT_PIREPHERAL_SET_CALLBACK(reset, reset);
    VXT_PIREPHERAL_SET_CALLBACK(dma.read, dma_read);
    VXT_PIREPHERAL_SET_CALLBACK(dma.write, dma_write);
    VXT_PIREPHERAL_SET_CALLBACK(io.in, in);
    VXT_PIREPHERAL_SET_CALLBACK(io.out, out);
})
