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
#include "vga_font.h"
#include "vga_palette.h"

#include <assert.h>
#include <string.h>

#define PLANE_SIZE 0x10000
#define MEMORY_SIZE 0x40000
#define MEMORY_START 0xA0000
#define CGA_BASE 0x18000
#define SCANLINE_TIMING 31469

#define INT64 long long
#define MEMORY(p, i) ((p)[(i) & (MEMORY_SIZE - 1)])

#define ONE_PLANE(mask, i, p) {                     \
    const int I = (i); const int M = 1 << I;        \
    if ((mask) & M) { p }                           \
}                                                   \

#define ALL_PLANES(mask, a, b, c, d) {              \
    ONE_PLANE((mask), 0, a)                         \
    ONE_PLANE((mask), 1, b)                         \
    ONE_PLANE((mask), 2, c)                         \
    ONE_PLANE((mask), 3, d)                         \
}                                                   \

#define FOR_PLANES(mask, ...) ALL_PLANES((mask),    \
    { __VA_ARGS__ ; },                              \
    { __VA_ARGS__ ; },                              \
    { __VA_ARGS__ ; },                              \
    { __VA_ARGS__ ; }                               \
)                                                   \

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

extern vxt_dword cga_palette[];

struct snapshot {
    vxt_byte mem[MEMORY_SIZE];
    vxt_byte rgba_surface[640 * 480 * 4];

    bool cursor_visible;
    int cursor_x;
    int cursor_y;

    int video_page;
    vxt_byte video_mode;
    vxt_byte mode_ctrl_reg;
    vxt_byte color_ctrl_reg;
};

VXT_PIREPHERAL(vga_video, {
    vxt_byte mem[MEMORY_SIZE];
    bool is_dirty;
    struct snapshot snap;

    INT64 last_scanline;
	int current_scanline;

    INT64 (*ustics)(void);
    INT64 application_start;

    vxt_byte video_mode;
    vxt_byte mem_latch[4];

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

        vxt_byte crt_addr;
        vxt_byte crt_reg[0x100];

        vxt_byte attr_addr;
        vxt_byte attr_reg[0x100];

        vxt_byte seq_addr;
        vxt_byte seq_reg[0x100];

        vxt_byte gfx_addr;
        vxt_byte gfx_reg[0x100];

        vxt_byte dac_addr;
        vxt_byte dac_reg[0x100];
    } reg;
})

static bool in_chained_mode(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(v, vga_video, p);
    v->video_mode = vxt_system_read_byte(VXT_GET_SYSTEM(vga_video, p), VXT_POINTER(0x40, 0x49));
    bool vm = (v->video_mode != 0xD) && (v->video_mode != 0x10) && (v->video_mode != 0x12);

    if ((v->video_mode != 0x13) && vm)
        return true;
    return !(v->reg.seq_reg[4] & 6) && vm;
}

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    VXT_DEC_DEVICE(v, vga_video, p);
    addr -= MEMORY_START;

    if (in_chained_mode(p))
        return MEMORY(v->mem, addr);

    v->mem_latch[0] = MEMORY(v->mem, addr);
    v->mem_latch[1] = MEMORY(v->mem, addr + PLANE_SIZE);
    v->mem_latch[2] = MEMORY(v->mem, addr + PLANE_SIZE * 2);
    v->mem_latch[3] = MEMORY(v->mem, addr + PLANE_SIZE * 3);

    vxt_byte map_mask = v->reg.seq_reg[2];
    FOR_PLANES(map_mask, return v->mem_latch[I]);
    return 0;
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    VXT_DEC_DEVICE(v, vga_video, p);
    addr -= MEMORY_START;
    v->is_dirty = true;

    if (in_chained_mode(p)) {
        MEMORY(v->mem, addr) = data;
        return;
    }

    vxt_byte *gr = v->reg.gfx_reg;
    vxt_byte *sr = v->reg.seq_reg;
    vxt_byte map_mask = sr[2];
    vxt_byte bit_mask = gr[8];

    switch (sr[5] & 3) {
        case 0:
            ROTATE_OP(gr, data);
            FOR_PLANES(map_mask,
                vxt_byte value = data;
                if (gr[1] & M)
                    value = (gr[0] & M) ? 0xFF : 0x0;

                LOGIC_OP(gr, value, v->mem_latch[I]);
                MEMORY(v->mem, addr + PLANE_SIZE * I) = (~bit_mask & value) | (bit_mask & v->mem_latch[I]);
            );
            break;
        case 1:
            FOR_PLANES(map_mask, MEMORY(v->mem, addr + PLANE_SIZE * I) = v->mem_latch[I]);
            break;
        case 2:
            FOR_PLANES(map_mask,
                vxt_byte value = data;
                if (gr[1] & M)
                    value = (data & M) ? 0xFF : 0x0;

                LOGIC_OP(gr, value, v->mem_latch[I]);
                MEMORY(v->mem, addr + PLANE_SIZE * I) = (~bit_mask & value) | (bit_mask & v->mem_latch[I]);
            );
            break;
        case 3:
            ROTATE_OP(gr, data);
            data &= bit_mask;
            FOR_PLANES(map_mask,
                vxt_byte value = (gr[0] & M) ? 0xFF : 0x0;
                MEMORY(v->mem, addr + PLANE_SIZE * I) = (~data & value) | (data & v->mem_latch[I]);
            );
            break;
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
            return v->reg.dac_addr;
        case 0x3C9:
            return v->reg.dac_reg[v->reg.dac_addr];
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
        default:
            return 0;
    }

    // 0x3BA, 0x3C2, 0x3DA
    vxt_byte status = v->reg.status_reg;
    v->reg.status_reg &= 0xFE;
    return status;
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
        case 0x3C8:
            v->reg.dac_addr = data;
            break;
        case 0x3C9:
            v->reg.dac_reg[v->reg.dac_addr] = data;
            break;
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
            // TODO
            break;
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_mem(s, p, MEMORY_START, (MEMORY_START + 0x20000) - 1);

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
    vxt_system_install_io_at(s, p, 0x3C6); // R: DAC State Register
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

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(vga_video, p))(p, 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(v, vga_video, p);

    v->last_scanline = v->ustics() * 1000ll;
    v->current_scanline = 0;
    v->is_dirty = true;

    v->reg.mode_ctrl_reg = 1;
    v->reg.color_ctrl_reg = 0x20;
    v->reg.status_reg = 0;
    v->reg.misc_output = 0x80; // CGA emulation

    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "VGA Compatible Device";
}

static enum vxt_pclass pclass(struct vxt_pirepheral *p) {
    (void)p; return VXT_PCLASS_VIDEO;
}

static vxt_error step(struct vxt_pirepheral *p, int cycles) {
    (void)cycles;
    VXT_DEC_DEVICE(v, vga_video, p);

	INT64 t = v->ustics() * 1000ll;
	INT64 d = t - v->last_scanline;
	INT64 scanlines = d / SCANLINE_TIMING;

	if (scanlines > 0) {
		INT64 offset = d % SCANLINE_TIMING;
		v->last_scanline = t - offset;
        v->current_scanline = (v->current_scanline + (int)scanlines) % 525;
		if (v->current_scanline > 479)
			v->reg.status_reg = 88;
		else
			v->reg.status_reg = 0;
		v->reg.status_reg |= 1;
	}
    return VXT_NO_ERROR;
}

static bool blink_tick(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(v, vga_video, p);
    return (((v->ustics() - v->application_start) / 500000) % 2) == 0;
}

static void blit32(vxt_byte *pixels, int offset, vxt_dword color) {
    pixels[offset++] = 0xFF;
    pixels[offset++] = (vxt_byte)(color & 0x0000FF);
    pixels[offset++] = (vxt_byte)((color & 0x00FF00) >> 8);
    pixels[offset] = (vxt_byte)((color & 0xFF0000) >> 16);
}

static void blit_char(struct vxt_pirepheral *p, vxt_byte ch, vxt_byte attr, int x, int y) {
    VXT_DEC_DEVICE(v, vga_video, p);
    struct snapshot *snap = &v->snap;

    int bg_color_index = (attr & 0x70) >> 4;
	int fg_color_index = attr & 0xF;

	if (attr & 0x80) {
		if (snap->mode_ctrl_reg & 0x20) {
			if (blink_tick(p))
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

struct vxt_pirepheral *vxtp_vga_create(vxt_allocator *alloc, INT64 (*ustics)(void)) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(vga_video));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(vga_video));
    VXT_DEC_DEVICE(v, vga_video, p);

    v->ustics = ustics;
    v->application_start = ustics();

    p->install = &install;
    p->destroy = &destroy;
    p->name = &name;
    p->pclass = &pclass;
    p->reset = &reset;
    p->step = &step;
    p->io.read = &read;
    p->io.write = &write;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

vxt_dword vxtp_vga_border_color(struct vxt_pirepheral *p) {
    return cga_palette[(VXT_GET_DEVICE(vga_video, p))->reg.color_ctrl_reg & 0xF];
}

bool vxtp_vga_snapshot(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(v, vga_video, p);
    if (!v->is_dirty)
        return false;

    vxt_system *s = VXT_GET_SYSTEM(vga_video, p);
    memcpy(v->snap.mem, v->mem, MEMORY_SIZE);
    
    // This is also set in in_chained_mode.
    v->snap.video_mode = v->video_mode = vxt_system_read_byte(VXT_GET_SYSTEM(vga_video, p), VXT_POINTER(0x40, 0x49));
    v->snap.video_page = ((int)v->reg.crt_reg[0xC] << 8) + (int)v->reg.crt_reg[0xD];
    //assert(v->snap.video_page == vxt_system_read_byte(s, VXT_POINTER(0x40, 0x62)));

    vxt_pointer cursor_addr = VXT_POINTER(0x40, 0x50 + v->snap.video_page * 2);
    v->snap.cursor_x = vxt_system_read_byte(s, cursor_addr);
    v->snap.cursor_y = vxt_system_read_byte(s, cursor_addr + 1);
    v->snap.cursor_visible = !(v->reg.crt_reg[0xE] & 0x20);

    v->snap.mode_ctrl_reg = v->reg.mode_ctrl_reg;
    v->snap.color_ctrl_reg = v->reg.color_ctrl_reg;

    v->is_dirty = false;
    return true;
}

int vxtp_vga_render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata) {
    struct snapshot *snap = &(VXT_GET_DEVICE(vga_video, p))->snap;
    int num_col = 80;

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

            if (blink_tick(p) && snap->cursor_visible) {
                const int x = snap->cursor_x;
                const int y = snap->cursor_y;
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
            num_col = 40;

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
        case 0xD:
            num_col = 40;
            snap->mode_ctrl_reg &= ~1;
        case 0xE:
        {
            int width = num_col * 8;
            for (int y = 0; y < 200; y++) {
                for (int x = 0; x < width; x++) {
                    int addr = y * num_col + (x >> 3);
                    int shift = 7 - (x & 7);

                    vxt_byte pixel = (MEMORY(snap->mem, addr) >> shift) & 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE + addr) >> shift) & 1) << 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 2 + addr) >> shift) & 1) << 2;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 3 + addr) >> shift) & 1) << 3;

                    blit32(snap->rgba_surface, (y * width + x) * 4, vga_palette[pixel]);
                }
            }
            return f(width, 200, snap->rgba_surface, userdata);
        }
        case 0x10:
            for (int y = 0; y < 350; y++) {
                for (int x = 0; x < 640; x++) {
                    int addr = y * 80 + (x >> 3);
                    int shift = 7 - (x & 7);

                    vxt_byte pixel = (MEMORY(snap->mem, addr) >> shift) & 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE + addr) >> shift) & 1) << 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 2 + addr) >> shift) & 1) << 2;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 3 + addr) >> shift) & 1) << 3;

                    blit32(snap->rgba_surface, (y * 640 + x) * 4, vga_palette[pixel]);
                }
            }
            return f(640, 350, snap->rgba_surface, userdata);
        case 0x12:
            for (int y = 0; y < 480; y++) {
                for (int x = 0; x < 640; x++) {
                    int addr = y * 80 + (x / 8);
                    int shift = ~x & 7;

                    vxt_byte pixel = (MEMORY(snap->mem, addr) >> shift) & 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE + addr) >> shift) & 1) << 1;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 2 + addr) >> shift) & 1) << 2;
                    pixel += ((MEMORY(snap->mem, PLANE_SIZE * 3 + addr) >> shift) & 1) << 3;

                    blit32(snap->rgba_surface, (y * 640 + x) * 4, vga_palette[pixel]);
                }
            }
            return f(640, 480, snap->rgba_surface, userdata);
    }
    
    fprintf(stderr, "Unsupported video mode: 0x%X\n", snap->video_mode);
    return -1;
}
