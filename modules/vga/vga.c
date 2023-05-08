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

// Reference: https://www.scs.stanford.edu/10wi-cs140/pintos/specs/freevga/vga/vga.htm

#define VXT_LIBC
#define VXTU_LIBC_IO
#include <vxt/vxtu.h>

#include <frontend.h>

#include "vga_font.h"
#include "vga_palette.h"

#include <string.h>

#define PLANE_SIZE 0x10000
#define MEMORY_SIZE 0x40000
#define MEMORY_START 0xA0000
#define CGA_BASE 0x18000
#define SCANLINE_TIMING 16
#define CURSOR_TIMING 333333
#define VIDEO_MODE_BDA_ADDRESS (VXT_POINTER(0x40, 0x49))

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

static const vxt_dword cga_palette[] = {
	0x000000,
	0x0000AA,
	0x00AA00,
	0x00AAAA,
	0xAA0000,
	0xAA00AA,
	0xAA5500,
	0xAAAAAA,
	0x555555,
	0x5555FF,
	0x55FF55,
	0x55FFFF,
	0xFF5555,
	0xFF55FF,
	0xFFFF55,
	0xFFFFFF,
};

struct snapshot {
    vxt_byte mem[MEMORY_SIZE];
    vxt_byte rgba_surface[640 * 480 * 4];
    vxt_dword palette[0x100];

    bool cursor_blink;
    bool cursor_visible;
    int cursor_offset;

    int video_page;
    int pixel_shift;
    bool plane_mode;
    vxt_byte video_mode;
    vxt_byte mode_ctrl_reg;
    vxt_byte color_ctrl_reg;
};

VXT_PIREPHERAL(vga_video, {
    vxt_byte mem[MEMORY_SIZE];
    bool is_dirty;
    struct snapshot snap;

    vxt_timer_id scanline_timer;
    int scanline;
    int retrace;

    bool cursor_blink;
    bool cursor_visible;
    int cursor_offset;

    vxt_byte video_mode;
    vxt_byte mem_latch[4];

    vxt_dword palette[0x100];

    bool (*set_video_adapter)(const struct frontend_video_adapter*);

    struct {
        vxt_byte mode_ctrl_reg;
        vxt_byte color_ctrl_reg;
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
})

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    VXT_DEC_DEVICE(v, vga_video, p);
    if (addr == VIDEO_MODE_BDA_ADDRESS)
        return v->video_mode;
    addr -= MEMORY_START;

    if (v->reg.seq_reg[5] & 8) {
        VXT_LOG("Readmode 1 is unsupported!");
        return 0;
    }

	if ((v->video_mode != 0xD) && (v->video_mode != 0xE) && (v->video_mode != 0x10) && (v->video_mode != 0x12))
        return MEMORY(v->mem, addr);

    if (!(v->reg.seq_reg[4] & 6))
        return MEMORY(v->mem, addr);


    v->mem_latch[0] = MEMORY(v->mem, addr);
    v->mem_latch[1] = MEMORY(v->mem, addr + PLANE_SIZE);
    v->mem_latch[2] = MEMORY(v->mem, addr + PLANE_SIZE * 2);
    v->mem_latch[3] = MEMORY(v->mem, addr + PLANE_SIZE * 3);
    return v->mem_latch[v->reg.gfx_reg[4] & 3];
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    VXT_DEC_DEVICE(v, vga_video, p);
    v->is_dirty = true;

    if ((addr == VIDEO_MODE_BDA_ADDRESS) && (v->video_mode != data)) {
        VXT_LOG("Switch video mode: 0x%X", data);
        v->video_mode = data;
        v->reg.seq_reg[4] = 0; // Set chained mode.
        return;
    }

    addr -= MEMORY_START;

	if (((v->video_mode != 0xD) && (v->video_mode != 0xE) && (v->video_mode != 0x10) && (v->video_mode != 0x12)) || !(v->reg.seq_reg[4] & 6)) {
        MEMORY(v->mem, addr) = data;
        return;
    }

    vxt_byte *gr = v->reg.gfx_reg;
    vxt_byte bit_mask = gr[8];
    vxt_byte map_mask = v->reg.seq_reg[2];

    switch (gr[5] & 3) {
        case 0:
            ROTATE_OP(gr, data);
            FOR_PLANES(map_mask,
                vxt_byte value = (data & M) ? 0xFF : 0x0;
                if (gr[1] & M)
                    value = (gr[0] & M) ? 0xFF : 0x0;
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

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(v, vga_video, p);
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
        case 0x3D8:
            return v->reg.mode_ctrl_reg;
        case 0x3D9:
		    return v->reg.color_ctrl_reg;
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

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(v, vga_video, p);
    v->is_dirty = true;
    switch (port) {
        case 0x3C0:
            if (v->reg.flip_3C0)
                v->reg.attr_addr = data;
            else
                v->reg.attr_reg[v->reg.attr_addr] = data;
            v->reg.flip_3C0 = !v->reg.flip_3C0;
            break;
        case 0x3C1:
            v->reg.attr_reg[v->reg.attr_addr] = data;
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
                    v->cursor_visible = (data & 0x20) == 0;
                    break;
                case 0xE:
                    v->cursor_offset = (v->cursor_offset & 0x00FF) | ((vxt_word)data << 8);
                    break;
                case 0xF:
                    v->cursor_offset = (v->cursor_offset & 0xFF00) | (vxt_word)data;
                    break;
            }
            break;
        case 0x3D8:
            v->reg.mode_ctrl_reg = data;
            break;
        case 0x3D9:
            v->reg.color_ctrl_reg = data;
        case 0x3BA:
        case 0x3DA:
            v->reg.feature_ctrl_reg = data;
            break;
        case 0xAFFF:
            v->mem_latch[v->reg.gfx_addr & 3] = data;
            break;
    }
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(v, vga_video, p);

    v->reg.mode_ctrl_reg = 1;
    v->reg.color_ctrl_reg = 0x20;
    v->reg.status_reg = 0;

    v->is_dirty = true;
    memcpy(v->palette, vga_palette, sizeof(vga_palette));
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "VGA Compatible Device";
}

static enum vxt_pclass pclass(struct vxt_pirepheral *p) {
    (void)p; return VXT_PCLASS_VIDEO;
}

static vxt_error timer(struct vxt_pirepheral *p, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    VXT_DEC_DEVICE(v, vga_video, p);

    if (v->scanline_timer == id) {
        v->reg.status_reg = 6;
        v->reg.status_reg |= (v->retrace == 3) ? 1 : 0;
        v->reg.status_reg |= (v->scanline >= 224) ? 8 : 0;
        
        if (++v->retrace == 4) {
            v->retrace = 0;
            v->scanline++;
        }
        if (v->scanline == 256)
            v->scanline = 0;
    } else {
        v->cursor_blink = !v->cursor_blink;
        v->is_dirty = true;
    }
    return VXT_NO_ERROR;
}

static void blit32(vxt_byte *pixels, int offset, vxt_dword color) {
    // Let's use the CGA defined packing.
    pixels[offset + VXTU_CGA_RED] = (vxt_byte)((color & 0xFF0000) >> 16);
    pixels[offset + VXTU_CGA_GREEN] = (vxt_byte)((color & 0x00FF00) >> 8);
    pixels[offset + VXTU_CGA_BLUE] = (vxt_byte)(color & 0x0000FF);
    pixels[offset + VXTU_CGA_ALPHA] = VXTU_CGA_ALPHA_FILL;
}

static void blit_char(struct vxt_pirepheral *p, vxt_byte ch, vxt_byte attr, int x, int y) {
    VXT_DEC_DEVICE(v, vga_video, p);
    struct snapshot *snap = &v->snap;

    int bg_color_index = (attr & 0x70) >> 4;
	int fg_color_index = attr & 0xF;

	if (attr & 0x80) {
		if (snap->mode_ctrl_reg & 0x20) {
			if (snap->cursor_blink)
				fg_color_index = bg_color_index;
		} else {
			// High intensity!
			bg_color_index += 8;
		}
	}

	vxt_dword bg_color = cga_palette[bg_color_index];
	vxt_dword fg_color = cga_palette[fg_color_index];
    int width = (snap->mode_ctrl_reg & 1) ? 640 : 320;

	for (int i = 0; i < 16; i++) {
		vxt_byte glyph_line = vga_font[(int)ch * 16 + i];
		for (int j = 0; j < 8; j++) {
			vxt_byte mask = 0x80 >> j;
			vxt_dword color = (glyph_line & mask) ? fg_color : bg_color;
		
			int offset = (width * (y + i) + x + j) * 4;
            blit32(snap->rgba_surface, offset, color);
		}
	}
}

static vxt_dword border_color(struct vxt_pirepheral *p) {
    return cga_palette[(VXT_GET_DEVICE(vga_video, p))->reg.color_ctrl_reg & 0xF];
}

static bool snapshot(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(v, vga_video, p);
    if (!v->is_dirty)
        return false;

    memcpy(v->snap.mem, v->mem, MEMORY_SIZE);
    memcpy(v->snap.palette, v->palette, sizeof(v->snap.palette));
    
    v->snap.video_mode = v->video_mode;
    v->snap.video_page = ((int)v->reg.crt_reg[0xC] << 8) + (int)v->reg.crt_reg[0xD];
    v->snap.plane_mode = !(v->reg.seq_reg[0x4] & 6);
    v->snap.pixel_shift = v->reg.attr_reg[0x13] & 15;

    v->snap.cursor_offset = v->cursor_offset;
    v->snap.cursor_visible = v->cursor_visible;
    v->snap.cursor_blink = v->cursor_blink;

    v->snap.mode_ctrl_reg = v->reg.mode_ctrl_reg;
    v->snap.color_ctrl_reg = v->reg.color_ctrl_reg;

    v->is_dirty = false;
    return true;
}

static int render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata) {
    struct snapshot *snap = &(VXT_GET_DEVICE(vga_video, p))->snap;
    int num_col = 80;
    int height = 480;

    switch (snap->video_mode) {
        case 0x0:
        case 0x1:
            num_col = 40;
            snap->mode_ctrl_reg &= ~1;
        case 0x2:
        case 0x3:
        case 0x7:
        {
            int num_char = num_col * 25;
            for (int i = 0; i < num_char * 2; i += 2) {
                int idx = i / 2;
                int cell_offset = CGA_BASE + snap->video_page + i;
                vxt_byte ch = MEMORY(snap->mem, cell_offset);
                vxt_byte attr = MEMORY(snap->mem, cell_offset + 1);
                blit_char(p, ch, attr, (idx % num_col) * 8, (idx / num_col) * 16);
            }

            if (snap->cursor_blink && snap->cursor_visible) {
                int x = snap->cursor_offset % num_col;
                int y = snap->cursor_offset / num_col;
                if (x < num_col && y < 25) {
                    vxt_byte attr = (MEMORY(snap->mem, CGA_BASE + snap->video_page + (num_col * 2 * y + x * 2 + 1)) & 0x70) | 0xF;
                    blit_char(p, '_', attr, x * 8, y * 16);
                }
            }
            return f(num_col * 8, 400, snap->rgba_surface, userdata);
        }
        case 0x4:
        case 0x5: // CGA 320x200x4
        {
            int color_index = (snap->color_ctrl_reg >> 5) & 1;
            int bg_color_index = snap->color_ctrl_reg & 0xF;
            int intensity = ((snap->color_ctrl_reg >> 4) & 1) << 3;

            for (int y = 0; y < 200; y++) {
                for (int x = 0; x < 320; x++) {
                    int addr = (y >> 1) * 80 + (y & 1) * 8192 + (x >> 2);
                    vxt_byte pixel = MEMORY(snap->mem, CGA_BASE + addr);

                    switch (x & 3) {
                        case 0:
                            pixel = (pixel >> 6) & 3;
                            break;
                        case 1:
                            pixel = (pixel >> 4) & 3;
                            break;
                        case 2:
                            pixel = (pixel >> 2) & 3;
                            break;
                        case 3:
                            pixel = pixel & 3;
                            break;
                    }

                    vxt_dword color = cga_palette[pixel ? (pixel * 2 + color_index + intensity) : bg_color_index];
                    blit32(snap->rgba_surface, (y * 320 + x) * 4, color);
                }
            }
            return f(320, 200, snap->rgba_surface, userdata);
        }
        case 0x6: // CGA 640x200x2
            for (int y = 0; y < 200; y++) {
                for (int x = 0; x < 640; x++) {
                    int addr = (y >> 1) * 80 + (y & 1) * 8192 + (x >> 3);
                    vxt_byte pixel = (MEMORY(snap->mem, CGA_BASE + addr) >> (7 - (x & 7))) & 1;
                    vxt_dword color = cga_palette[pixel * 15];
                    int offset = (y * 640 + x) * 4;
                    blit32(snap->rgba_surface, offset, color);
                }
            }
            return f(640, 200, snap->rgba_surface, userdata);
        case 0xD: // EGA 320x200x16
            num_col = 40;
            snap->mode_ctrl_reg &= ~1;
        case 0xE: // EGA 640x200x16
        case 0x10: // EGA 640x350x16
            height = (snap->video_mode == 0x10) ? 350 : 200;
        case 0x12: // VGA 640x480x16
        {
            int width = num_col * 8;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int addr = y * num_col + (x >> 3);
                    int shift = 7 - (x & 7);
                    
                    vxt_byte pixel = (MEMORY(snap->mem, addr) >> shift) & 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE + addr) >> shift) & 1) << 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 2 + addr) >> shift) & 1) << 2;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 3 + addr) >> shift) & 1) << 3;

                    blit32(snap->rgba_surface, (y * width + x) * 4, snap->palette[pixel]);
                }
            }
            return f(width, height, snap->rgba_surface, userdata);
        }
        case 0x13: // VGA 320x200x256
            for (int y = 0; y < 200; y++) {
                for (int x = 0; x < 320; x++) {
                        int addr;
                        if (snap->plane_mode) {
                            addr = (y * 320 + x) / 4 + (x & 3) * PLANE_SIZE;
                            addr = addr + snap->video_page - snap->pixel_shift;
                        } else {
                            addr = (snap->video_page + y * 320 + x) & 0xFFFF;
                        }
                        blit32(snap->rgba_surface, (y * 320 + x) * 4, snap->palette[MEMORY(snap->mem, addr)]);
                }
            }
            return f(320, 200, snap->rgba_surface, userdata);
    }
    
    VXT_LOG("Unsupported video mode: 0x%X", snap->video_mode);
    return -1;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(v, vga_video, p);

    if (v->set_video_adapter) {
        struct frontend_video_adapter a = { p, &border_color, &snapshot, &render };
        v->set_video_adapter(&a);
    }

    vxt_system_install_mem(s, p, MEMORY_START, (MEMORY_START + 0x20000) - 1);
    vxt_system_install_mem_at(s, p, VIDEO_MODE_BDA_ADDRESS); // BDA video mode

    vxt_system_install_timer(s, p, CURSOR_TIMING);
    v->scanline_timer = vxt_system_install_timer(s, p, SCANLINE_TIMING);

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

// Tested with ET4000 VGA BIOS.
static struct vxt_pirepheral *bios_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)frontend;

    int size = 0;
    vxt_byte *data = vxtu_read_file(alloc, args, &size);
    if (!data) {
        VXT_LOG("Could not load VGA BIOS: %s", args);
        return NULL;
    }

    struct vxt_pirepheral *p = vxtu_memory_create(alloc, 0xC0000, size, true);
    alloc(data, 0);
    return p;
}

VXTU_MODULE_ENTRIES(&vga_create, &bios_create)
