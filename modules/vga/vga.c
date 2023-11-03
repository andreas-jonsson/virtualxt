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

/*
References: https://www.scs.stanford.edu/10wi-cs140/pintos/specs/freevga/vga/vga.htm
            http://www.osdever.net/FreeVGA/vga/vga.htm
            https://wiki.osdev.org/Video_Signals_And_Timing

*/

#include <vxt/vxtu.h>

#include <frontend.h>

#include "font.h"

#include <string.h>

#define PLANE_SIZE 0x10000
#define MEMORY_SIZE 0x40000
#define MEMORY_START 0xA0000
#define CGA_BASE 0x18000
#define CURSOR_TIMING 333333

#define MEMORY(p, i) ((p)[(i) & (MEMORY_SIZE - 1)])

#define ONE_PLANE(mask, i, ...) {                   \
    const int I = (i); const int M = 1 << I;        \
    if ((mask) & M) { __VA_ARGS__ ; }               \
}                                                   \

#define FOR_PLANES(mask, ...) {                     \
    ONE_PLANE((mask), 0, { __VA_ARGS__ ; });        \
    ONE_PLANE((mask), 1, { __VA_ARGS__ ; });        \
    ONE_PLANE((mask), 2, { __VA_ARGS__ ; });        \
    ONE_PLANE((mask), 3, { __VA_ARGS__ ; });        \
}                                                   \

#define ROTATE_OP(gc, v)                            \
	for (int i = 0; i < ((gc)[3] & 7); i++) {       \
		(v) = ((v) >> 1) | (((v) & 1) << 7);        \
	}                                               \

#define LOGIC_OP(gc, value, latch)                  \
	switch (((gc)[3] >> 3) & 3) {                   \
        case 1: (value) &= (latch); break;          \
        case 2: (value) |= (latch); break;          \
        case 3: (value) ^= (latch); break;          \
	}                                               \

struct snapshot {
    vxt_byte mem[MEMORY_SIZE];
    vxt_byte rgba_surface[720 * 480 * 4];
    vxt_dword palette[0x100];

    bool cursor_blink;
    bool cursor_visible;
    vxt_byte cursor_start;
    vxt_byte cursor_end;
    int cursor_offset;

    vxt_word width;
    vxt_word height;
    vxt_byte bpp;
    bool textmode;

    int video_page;
    int pixel_shift;
    bool plane_mode;
    vxt_byte mode_ctrl;
    vxt_byte color_select;
    vxt_byte pal_reg[16];
};

struct vga_video {
    vxt_byte mem[MEMORY_SIZE];
    bool is_dirty;
    struct snapshot snap;

    vxt_timer_id retrace_timer;
    vxt_timer_id scanline_timer;

    vxt_word width;
    vxt_word height;
    vxt_byte bpp;
    bool textmode;

    bool cursor_blink;
    bool cursor_visible;
    vxt_byte cursor_start;
    vxt_byte cursor_end;
    int cursor_offset;

    vxt_byte mem_latch[4];
    vxt_dword palette[0x100];

    bool (*set_video_adapter)(const struct frontend_video_adapter*);

    struct {
        vxt_byte feature_ctrl_reg;
        vxt_byte status_reg;
        bool flip_3C0;

        vxt_byte misc_output;
        vxt_byte vga_enable;
        vxt_byte pixel_mask;

        vxt_byte dac_state;
        vxt_dword pal_rgb;
        vxt_byte pal_read_index;
		vxt_byte pal_read_latch;
        vxt_byte pal_write_index;
		vxt_byte pal_write_latch;

        vxt_byte crt_addr;
        vxt_byte crt_reg[0x100];

        vxt_byte attr_addr;
        vxt_byte attr_reg[0x100];

        vxt_byte seq_addr;
        vxt_byte seq_reg[0x100];

        vxt_byte gfx_addr;
        vxt_byte gfx_reg[0x100];
    } reg;
};

#include "render.inl"

static void update_video_mode(struct vga_video *v) {
    const vxt_byte* const crt = v->reg.crt_reg;

    vxt_byte dots = (v->reg.seq_reg[1] & 1) ? 8 : 9;
	v->width = ((crt[1] + 1) - ((crt[5] & 0x60) >> 5)) * dots;
	v->height = (crt[0x12] | ((crt[7] & 2) ? 0x100 : 0) | ((crt[7] & 0x40) ? 0x200 : 0)) + 1;

    int htotal = (int)crt[0] + 5;
    int vtotal = (int)crt[6] | ((crt[7] & 1) ? 0x100 : 0) | ((crt[7] & 0x20) ? 0x200 : 0);
    double pixel_clock = (v->reg.misc_output & 4) ? 28322000.0 : 25175000.0;

    if (crt[9] & 0x80)
        v->height >>= 1;

    v->bpp = 4;
    v->textmode = true;

    if (v->reg.attr_reg[0x10] & 1) {
        v->textmode = false;

		switch ((v->reg.gfx_reg[5] >> 5) & 3) {
            case 0:
                v->bpp = ((v->reg.attr_reg[0x12] & 0xF) == 1) ? 1 : 4;
                break;
            case 1:
                v->bpp = 2;
                break;
            case 2:
            case 3:
                v->bpp = 8;

                // TODO: We should NOT have to adjust this here.
                v->width = 320;
                v->height = 200;
                break;
		}
    }

    if (v->width < 160) v->width = 160;
    else if (v->width > 720) v->width = 720;

    if (v->height < 100) v->height = 100;
    else if (v->height > 480) v->height = 480;

    int freq_div = htotal * vtotal * (int)dots;
    unsigned int interval_us = (freq_div > 0) ? (unsigned int)(500000.0 / (pixel_clock / (double)freq_div)) : 0;
    vxt_system_set_timer_interval(VXT_GET_SYSTEM(v), v->retrace_timer, interval_us);

    freq_div = htotal * (int)dots;
    interval_us = (freq_div > 0) ? (unsigned int)(500000.0 / (pixel_clock / (double)freq_div)) : 0;
    vxt_system_set_timer_interval(VXT_GET_SYSTEM(v), v->scanline_timer, interval_us);

	//VXT_LOG("Video Mode: %dx%d %dbpp @ %.02fHz%s", v->width, v->height, v->bpp, pixel_clock / (double)(htotal * vtotal * (int)dots), v->textmode ? " (textmode)" : "");
}

static vxt_byte read(struct vga_video *v, vxt_pointer addr) {
    addr -= MEMORY_START;

	if (v->textmode || (v->bpp != 4))
        return MEMORY(v->mem, addr);

    if (v->reg.seq_reg[4] & 8)
        return MEMORY(v->mem, addr);

    v->mem_latch[0] = MEMORY(v->mem, addr);
    v->mem_latch[1] = MEMORY(v->mem, addr + PLANE_SIZE);
    v->mem_latch[2] = MEMORY(v->mem, addr + PLANE_SIZE * 2);
    v->mem_latch[3] = MEMORY(v->mem, addr + PLANE_SIZE * 3);

    vxt_byte data = 0;
    if (v->reg.seq_reg[5] & 8) { // Readmode 1
        vxt_byte map_mask = v->reg.seq_reg[2] & 0xF;
        FOR_PLANES(map_mask,
            if (v->reg.gfx_reg[7] & M) {
                if (MEMORY(v->mem, addr + PLANE_SIZE * I) == (v->reg.gfx_reg[2] & 0xF))
                    data |= M;
            }
        );
    } else { // Readmode 0
        data = v->mem_latch[v->reg.gfx_reg[4] & 3];
    }
    return data;
}

static void write(struct vga_video *v, vxt_pointer addr, vxt_byte data) {
    addr -= MEMORY_START;
    v->is_dirty = true;

	if (v->textmode || (v->bpp != 4)) {
        MEMORY(v->mem, addr) = data;
        return;
    }

	if (v->reg.seq_reg[4] & 8) {
        MEMORY(v->mem, addr) = data;
        return;
    }

    vxt_byte *gr = v->reg.gfx_reg;
    vxt_byte bit_mask = gr[8];
    vxt_byte map_mask = v->reg.seq_reg[2] & 0xF;

    switch (gr[5] & 3) {
        case 0:
            ROTATE_OP(gr, data);
            FOR_PLANES(map_mask,
                vxt_byte value = data;
                if (gr[1] & M)
                    value = (gr[0] & M) ? 0xFF : 0x0;
                else
                    ROTATE_OP(gr, value);
                LOGIC_OP(gr, value, v->mem_latch[I]);
                MEMORY(v->mem, addr + PLANE_SIZE * I) = (bit_mask & value) | (~bit_mask & v->mem_latch[I]);
            );
            break;
        case 1:
            FOR_PLANES(map_mask, MEMORY(v->mem, addr + PLANE_SIZE * I) = v->mem_latch[I]);
            break;
        case 2:
            FOR_PLANES(map_mask,
                vxt_byte value = (data & M) ? 0xFF : 0x0;
                LOGIC_OP(gr, value, v->mem_latch[I]);
                MEMORY(v->mem, addr + PLANE_SIZE * I) = (bit_mask & value) | (~bit_mask & v->mem_latch[I]);
            );
            break;
        case 3:
        {
            vxt_byte value = data;
            ROTATE_OP(gr, value);
            value &= bit_mask;

            FOR_PLANES(map_mask,
                vxt_byte set_reset = (gr[0] & M) ? 0xFF : 0x0;
                MEMORY(v->mem, addr + PLANE_SIZE * I) = (value & set_reset) | (~value & v->mem_latch[I]);
            );
            break;
        }
    }
}

static vxt_byte in(struct vga_video *v, vxt_word port) {
    switch (port) {
        case 0x3C0:
            return v->reg.attr_addr;
        case 0x3C1:
            return v->reg.attr_reg[v->reg.attr_addr];
        case 0x3C2:
            break; // Should be status 0?
        case 0x3C3:
            return v->reg.vga_enable;
        case 0x3C4:
            return v->reg.seq_addr;
        case 0x3C5:
            return v->reg.seq_reg[v->reg.seq_addr];
        case 0x3C6:
            return v->reg.pixel_mask;
        case 0x3C7:
            return v->reg.dac_state;
        case 0x3C8:
            return v->reg.pal_read_index;
        case 0x3C9:
            switch (v->reg.pal_read_latch++) {
                case 0:
                    return (v->palette[v->reg.pal_read_index] >> 18) & 0x3F;
                case 1:
                    return (v->palette[v->reg.pal_read_index] >> 10) & 0x3F;
                default:
                    v->reg.pal_read_latch = 0;
                    return (v->palette[v->reg.pal_read_index++] >> 2) & 0x3F;
            }
        case 0x3CA:
            return v->reg.feature_ctrl_reg;
        case 0x3CC:
            return v->reg.misc_output;
        case 0x3CE:
            return v->reg.gfx_addr;
        case 0x3CF:
            return v->reg.gfx_reg[v->reg.gfx_addr];
        case 0x3B4:
        case 0x3D4:
            return v->reg.crt_addr;
        case 0x3B5:
        case 0x3D5:
            return v->reg.crt_reg[v->reg.crt_addr];
        case 0x3BA:
        case 0x3DA:
            break;
        case 0xAFFF:
            return v->mem_latch[v->reg.gfx_addr & 3];
        default:
            return 0;
    }

    // 0x3BA, 0x3C2, 0x3DA
    v->reg.flip_3C0 = 0;
    return v->reg.status_reg;
}

static void out(struct vga_video *v, vxt_word port, vxt_byte data) {
    v->is_dirty = true;
    switch (port) {
        case 0x3C0:
        case 0x3C1:
            if (v->reg.flip_3C0)
                v->reg.attr_reg[v->reg.attr_addr] = data;
            else
                v->reg.attr_addr = data;
            v->reg.flip_3C0 = !v->reg.flip_3C0;
            break;
        case 0x3C2:
            v->reg.misc_output = data;
            break;
        case 0x3C3:
            v->reg.vga_enable = data;
            break;
        case 0x3C4:
            v->reg.seq_addr = data;
            break;
        case 0x3C5:
            v->reg.seq_reg[v->reg.seq_addr] = data;
            update_video_mode(v);
            break;
        case 0x3C7:
            v->reg.pal_read_index = data;
			v->reg.pal_read_latch = 0;
            v->reg.dac_state = 0;
            break;
        case 0x3C8:
            v->reg.pal_write_index = data;
			v->reg.pal_write_latch = 0;
			v->reg.dac_state = 3;
            break;
        case 0x3C9:
        {
            vxt_dword value = data & 0x3F;
            switch (v->reg.pal_write_latch++) {
                case 0:
                    v->reg.pal_rgb = value << 18;
                    break;
                case 1:
                    v->reg.pal_rgb |= value << 10;
                    break;
                case 2:
                    v->reg.pal_rgb |= value << 2;
                    v->reg.pal_write_latch = 0;
                    v->palette[v->reg.pal_write_index++] = v->reg.pal_rgb;
                    break;
            }
            break;
        }
        case 0x3CE:
            v->reg.gfx_addr = data;
            break;
        case 0x3CF:
            v->reg.gfx_reg[v->reg.gfx_addr] = data;
            update_video_mode(v);
            break;
        case 0x3B4:
        case 0x3D4:
            v->reg.crt_addr = data;
            break;
        case 0x3B5:
        case 0x3D5:
        	v->reg.crt_reg[v->reg.crt_addr] = data;
            switch (v->reg.crt_addr) {
                case 0xA:
                    v->cursor_start = data & 0x1F;
                    v->cursor_visible = !(data & 0x20) && (v->cursor_start < 16);
                    break;
                case 0xB:
                    v->cursor_end = data;
                    break;
                case 0xE:
                    v->cursor_offset = (v->cursor_offset & 0x00FF) | ((vxt_word)data << 8);
                    break;
                case 0xF:
                    v->cursor_offset = (v->cursor_offset & 0xFF00) | (vxt_word)data;
                    break;
                default:
                    update_video_mode(v);
            }
            break;
        case 0x3D8:
        case 0x3D9:
            VXT_LOG("WARNING: CGA register manipulation is not supported on VGA. Switch to the CGA module.");
            break;
        case 0x3BA:
        case 0x3DA:
            v->reg.feature_ctrl_reg = data;
            break;
        case 0xAFFF:
            v->mem_latch[v->reg.gfx_addr & 3] = data;
            break;
    }
}

static vxt_error reset(struct vga_video *v) {
    v->reg.status_reg = 0;
    v->is_dirty = true;
    return VXT_NO_ERROR;
}

static const char *name(struct vga_video *v) {
    (void)v; return "VGA Compatible Device";
}

static enum vxt_pclass pclass(struct vga_video *v) {
    (void)v; return VXT_PCLASS_VIDEO;
}

static vxt_error timer(struct vga_video *v, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;

    if (v->scanline_timer == id) {
        v->reg.status_reg ^= 1;
    } else if (v->retrace_timer == id) {
        v->reg.status_reg ^= 8;
    } else {
        v->cursor_blink = !v->cursor_blink;
        v->is_dirty = true;
    }

    v->reg.status_reg |= 6;
    return VXT_NO_ERROR;
}

static vxt_dword border_color(struct vxt_pirepheral *p) {
    (void)p;
    return 0;
}

static vxt_error install(struct vga_video *v, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(v);
    if (v->set_video_adapter) {
        struct frontend_video_adapter a = { p, &border_color, &snapshot, &render };
        v->set_video_adapter(&a);
    }

    // Try to set XT switches to VGA.
    for (int i = 0; i < VXT_MAX_PIREPHERALS; i++) {
        struct vxt_pirepheral *ip = vxt_system_pirepheral(s, (vxt_byte)i);
        if (vxt_pirepheral_class(ip) == VXT_PCLASS_PPI) {
            vxtu_ppi_set_xt_switches(ip, vxtu_ppi_xt_switches(ip) & 0xCF);
            break;
        }
    }

    vxt_system_install_monitor(s, p, "Color Select", &v->reg.attr_reg[0x14], VXT_MONITOR_SIZE_BYTE|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, p, "Mode Control", &v->reg.attr_reg[0x10], VXT_MONITOR_SIZE_BYTE|VXT_MONITOR_FORMAT_BINARY);
    vxt_system_install_monitor(s, p, "Offset Register", &v->reg.attr_reg[0x13], VXT_MONITOR_SIZE_BYTE|VXT_MONITOR_FORMAT_DECIMAL);

    vxt_system_install_mem(s, p, MEMORY_START, (MEMORY_START + 0x20000) - 1);

    vxt_system_install_timer(s, p, CURSOR_TIMING);
    v->retrace_timer = vxt_system_install_timer(s, p, 0);
    v->scanline_timer = vxt_system_install_timer(s, p, 0);

    vxt_system_install_io_at(s, p, 0x3B4); // --
    vxt_system_install_io_at(s, p, 0x3D4); // R/W: CRT Index

    vxt_system_install_io_at(s, p, 0x3B5); // --
    vxt_system_install_io_at(s, p, 0x3D5); // R/W: CRT Data

    vxt_system_install_io_at(s, p, 0x3C0); // R/W: Attribute Controller Index
    vxt_system_install_io_at(s, p, 0x3C1); // R/W: Attribute Data
    vxt_system_install_io_at(s, p, 0x3C2); // W: Misc Output, R: Input Status 0
    vxt_system_install_io_at(s, p, 0x3C3); // R/W: VGA Enable
    vxt_system_install_io_at(s, p, 0x3C4); // R/W: Sequencer Index
    vxt_system_install_io_at(s, p, 0x3C5); // R/W: Sequencer Data
    vxt_system_install_io_at(s, p, 0x3C6); // R: DAC State Register or Pixel Mask
    vxt_system_install_io_at(s, p, 0x3C7); // R: DAC State
    vxt_system_install_io_at(s, p, 0x3C8); // R/W: Pixel Address
    vxt_system_install_io_at(s, p, 0x3C9); // R/W: Color Data
    vxt_system_install_io_at(s, p, 0x3CA); // R: Feature Control
    vxt_system_install_io_at(s, p, 0x3CC); // R: Misc Output
    vxt_system_install_io_at(s, p, 0x3CE); // R/W: Graphics Controller Index
    vxt_system_install_io_at(s, p, 0x3CF); // R/W: Graphics Data
    
    vxt_system_install_io_at(s, p, 0x3D8); // R/W: Mode Control
    vxt_system_install_io_at(s, p, 0x3D9); // R/W: Color Control

    vxt_system_install_io_at(s, p, 0x3BA); // --
    vxt_system_install_io_at(s, p, 0x3DA); // W: Feature Control

    vxt_system_install_io_at(s, p, 0xAFFF); // R/W: Plane System Latch
    return VXT_NO_ERROR;
}

static struct vxt_pirepheral *vga_create(vxt_allocator *alloc, void *frontend, const char *args) VXT_PIREPHERAL_CREATE(alloc, vga_video, {
    (void)args;
    if (frontend)
        DEVICE->set_video_adapter = ((struct frontend_interface*)frontend)->set_video_adapter;

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->pclass = &pclass;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->io.read = &read;
    PIREPHERAL->io.write = &write;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})

static struct vxt_pirepheral *bios_create(vxt_allocator *alloc, void *frontend, const char *args) {
    struct frontend_interface *fi = (struct frontend_interface*)frontend;
    if (fi && fi->resolve_path)
        args = fi->resolve_path(FRONTEND_BIOS_PATH, args);

    int size = 0;
    vxt_byte *data = vxtu_read_file(alloc, args, &size);
    if (!data) {
        VXT_LOG("Could not load VGA BIOS: %s", args);
        return NULL;
    }

    struct vxt_pirepheral *p = vxtu_memory_create(alloc, 0xC0000, size, true);
    vxtu_memory_device_fill(p, data, size);
    alloc(data, 0);
    return p;
}

VXTU_MODULE_ENTRIES(&vga_create, &bios_create)
