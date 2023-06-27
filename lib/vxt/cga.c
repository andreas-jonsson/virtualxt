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
#include "cga_font.h"

#define MEMORY_SIZE 0x10000
#define MEMORY_START 0xB0000
#define CGA_BASE 0x8000
#define SCANLINE_TIMING 16
#define CURSOR_TIMING 333333

#define MEMORY(p, i) ((p)[(i) & (MEMORY_SIZE - 1)])

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

    // This is the CGA (black/cyan/red/white) palette.
    0x000000,
    0x000000,
    0x00AAAA,
    0x000000,
    0xAA0000,
    0x000000,
    0xAAAAAA,
    0x000000,
    0x000000,
    0x000000,
    0x55FFFF,
    0x000000,
    0xFF5555,
    0x000000,
    0xFFFFFF,
    0x000000
};

struct snapshot {
    vxt_byte mem[MEMORY_SIZE];
    vxt_byte rgba_surface[720 * 350 * 4];

    bool cursor_blink;
    bool cursor_visible;
    vxt_byte cursor_start;
    vxt_byte cursor_end;
    int cursor_offset;

    bool hgc_mode;
    int hgc_base;
    int video_page;
    vxt_byte mode_ctrl_reg;
    vxt_byte color_ctrl_reg;
};

struct cga_video {
    vxt_byte mem[MEMORY_SIZE];
    bool is_dirty;
    struct snapshot snap;

    bool hgc_mode;
    int hgc_base;
    vxt_byte hgc_enable;

    bool cursor_blink;
    bool cursor_visible;
    vxt_byte cursor_start;
    vxt_byte cursor_end;
    int cursor_offset;

    vxt_timer_id scanline_timer;
    int scanline;
    int retrace;

    vxt_byte mode_ctrl_reg;
    vxt_byte color_ctrl_reg;
    vxt_byte status_reg;
    vxt_byte crt_addr;
    vxt_byte crt_reg[0x100];
};

static vxt_byte read(struct cga_video *c, vxt_pointer addr) {
    return MEMORY(c->mem, addr - MEMORY_START);
}

static void write(struct cga_video *c, vxt_pointer addr, vxt_byte data) {
    MEMORY(c->mem, addr - MEMORY_START) = data;
    c->is_dirty = true;
}

static vxt_byte in(struct cga_video *c, vxt_word port) {
    switch (port) {
        case 0x3B0:
        case 0x3B2:
        case 0x3B4:
        case 0x3B6:
        case 0x3D0:
        case 0x3D2:
        case 0x3D4:
        case 0x3D6:
            return c->crt_addr;
        case 0x3B1:
        case 0x3B3:
        case 0x3B5:
        case 0x3B7:
	    case 0x3D1:
        case 0x3D3:
        case 0x3D5:
        case 0x3D7:
            return c->crt_reg[c->crt_addr];
        case 0x3D8:
            return c->mode_ctrl_reg;
        case 0x3D9:
		    return c->color_ctrl_reg;
        case 0x3BA:
        case 0x3DA:
            return c->hgc_mode ? 0xFF : c->status_reg;
        default:
            return 0;
    }
}

static void out(struct cga_video *c, vxt_word port, vxt_byte data) {
    c->is_dirty = true;
    switch (port) {
        case 0x3B0:
        case 0x3B2:
        case 0x3B4:
        case 0x3B6:
        case 0x3D0:
        case 0x3D2:
        case 0x3D4:
        case 0x3D6:
            c->crt_addr = data;
            break;
        case 0x3B1:
        case 0x3B3:
        case 0x3B5:
        case 0x3B7:
        case 0x3D1:
        case 0x3D3:
        case 0x3D5:
        case 0x3D7:
        	c->crt_reg[c->crt_addr] = data;
            switch (c->crt_addr) {
                case 0xA:
                    c->cursor_start = data & 0x1F;
                    c->cursor_visible = !(data & 0x20) && (c->cursor_start < 8);
                    break;
                case 0xB:
                    c->cursor_end = data;
                    break;
                case 0xE:
                    c->cursor_offset = (c->cursor_offset & 0x00FF) | ((vxt_word)data << 8);
                    break;
                case 0xF:
                    c->cursor_offset = (c->cursor_offset & 0xFF00) | (vxt_word)data;
                    break;
            }
            break;
        case 0x3B8: // Bit 7 is HGC page and bit 1 is gfx select.
            c->hgc_mode = ((c->hgc_enable & 1) & ((data & 2) >> 1)) != 0;
            c->hgc_base = 0;
            if ((c->hgc_enable >> 1) & (data >> 7))
                c->hgc_base = CGA_BASE;
            break;
        case 0x3D8:
            c->mode_ctrl_reg = data;
            break;
        case 0x3D9:
            c->color_ctrl_reg = data;
            break;
        case 0x3BF: // Enable HGC.
            // Set bit 0 to enable bit 1 of 3B8.
            // Set bit 1 to enable bit 7 of 3B8 and the second 32k of RAM ("Full" mode).
            #ifndef VXTU_CGA_NO_HGC
                c->hgc_enable = data & 3;
            #endif
            break;
        }
}

static vxt_error install(struct cga_video *c, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(c);
    vxt_system_install_mem(s, p, MEMORY_START, (MEMORY_START + MEMORY_SIZE) - 1);
    vxt_system_install_io(s, p, 0x3B0, 0x3BF);
    vxt_system_install_io(s, p, 0x3D0, 0x3DF);
    vxt_system_install_timer(s, p, CURSOR_TIMING);
    c->scanline_timer = vxt_system_install_timer(s, p, SCANLINE_TIMING);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct cga_video *c) {
    c->cursor_visible = true;
    c->cursor_start = 6;
    c->cursor_end = 7;
    c->cursor_offset = 0;
    c->is_dirty = true;

    c->mode_ctrl_reg = 1;
    c->color_ctrl_reg = 0x20;
    c->status_reg = 0;
    
    c->hgc_enable = 0;
    c->hgc_base = 0;
    c->hgc_mode = false;

    return VXT_NO_ERROR;
}

static const char *name(struct cga_video *c) {
    (void)c;
    return "CGA"
        #ifndef VXTU_CGA_NO_HGC
            "/HGC"
        #endif
        " Compatible Device";
}

static enum vxt_pclass pclass(struct cga_video *c) {
    (void)c; return VXT_PCLASS_VIDEO;
}

static vxt_error timer(struct cga_video *c, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;

    if (c->scanline_timer == id) {
        c->status_reg = 6;
        c->status_reg |= (c->retrace == 3) ? 1 : 0;
        c->status_reg |= (c->scanline >= 224) ? 8 : 0;
        
        if (++c->retrace == 4) {
            c->retrace = 0;
            c->scanline++;
        }
        if (c->scanline == 256)
            c->scanline = 0;
    } else {
        c->cursor_blink = !c->cursor_blink;
        c->is_dirty = true;
    }
    return VXT_NO_ERROR;
}

static void blit32(vxt_byte *pixels, int offset, vxt_dword color) {
    pixels[offset + VXTU_CGA_RED] = (vxt_byte)((color & 0xFF0000) >> 16);
    pixels[offset + VXTU_CGA_GREEN] = (vxt_byte)((color & 0x00FF00) >> 8);
    pixels[offset + VXTU_CGA_BLUE] = (vxt_byte)(color & 0x0000FF);
    pixels[offset + VXTU_CGA_ALPHA] = VXTU_CGA_ALPHA_FILL;
}

static void blit_char(struct cga_video *c, int ch, vxt_byte attr, int x, int y) {
    struct snapshot *snap = &c->snap;

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
    int start = 0;
    int end = 7;

    if (ch & ~0xFF) {
        ch = 0xDB;
        start = (int)snap->cursor_start;
        end = (int)snap->cursor_end;
    }

	for (int i = start; true; i++) {
        int n = i % 8;
        vxt_byte glyph_line = cga_font[ch * 8 + n];

		for (int j = 0; j < 8; j++) {
            vxt_byte mask = 0x80 >> j;
			vxt_dword color = (glyph_line & mask) ? fg_color : bg_color;
			int offset = (width * (y + n) + x + j) * 4;
            blit32(snap->rgba_surface, offset, color);
		}

        if (n == end)
            break;
	}
}

VXT_API struct vxt_pirepheral *vxtu_cga_create(vxt_allocator *alloc) VXT_PIREPHERAL_CREATE(alloc, cga_video, {
    vxtu_randomize(DEVICE->mem, MEMORY_SIZE, (intptr_t)PIREPHERAL);

    VXT_PIREPHERAL_SET_CALLBACK(install, install);
    VXT_PIREPHERAL_SET_CALLBACK(name, name);
    VXT_PIREPHERAL_SET_CALLBACK(pclass, pclass);
    VXT_PIREPHERAL_SET_CALLBACK(reset, reset);
    VXT_PIREPHERAL_SET_CALLBACK(timer, timer);
    VXT_PIREPHERAL_SET_CALLBACK(io.read, read);
    VXT_PIREPHERAL_SET_CALLBACK(io.write, write);
    VXT_PIREPHERAL_SET_CALLBACK(io.in, in);
    VXT_PIREPHERAL_SET_CALLBACK(io.out, out);
})

VXT_API vxt_dword vxtu_cga_border_color(struct vxt_pirepheral *p) {
    return cga_palette[(VXT_GET_DEVICE(cga_video, p))->color_ctrl_reg & 0xF];
}

VXT_API bool vxtu_cga_snapshot(struct vxt_pirepheral *p) {
    struct cga_video *c = VXT_GET_DEVICE(cga_video, p);
    if (!c->is_dirty)
        return false;

    memcpy(c->snap.mem, c->mem, MEMORY_SIZE);

    c->snap.hgc_mode = c->hgc_mode;
    c->snap.hgc_base = c->hgc_base;
    c->snap.cursor_blink = c->cursor_blink;
    c->snap.cursor_visible = c->cursor_visible;
    c->snap.cursor_start = c->cursor_start;
    c->snap.cursor_end = c->cursor_end;
    c->snap.cursor_offset = c->cursor_offset;
    c->snap.mode_ctrl_reg = c->mode_ctrl_reg;
    c->snap.color_ctrl_reg = c->color_ctrl_reg;
    c->snap.video_page = ((int)c->crt_reg[0xC] << 8) + (int)c->crt_reg[0xD];

    c->is_dirty = false;
    return true;
}

VXT_API int vxtu_cga_render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata) {
    struct snapshot *snap = &(VXT_GET_DEVICE(cga_video, p))->snap;

    if (snap->hgc_mode) {
        for (int y = 0; y < 348; y++) {
            for (int x = 0; x < 720; x++) {
                int addr = ((y & 3) << 13) + ( y >> 2) * 90 + (x >> 3);
                vxt_byte pixel = (MEMORY(snap->mem, snap->hgc_base + addr) >> (7 - (x & 7))) & 1;
                vxt_dword color = cga_palette[pixel * 15];
                int offset = (y * 720 + x) * 4;
                blit32(snap->rgba_surface, offset, color);
            }
        }

        for (int i = 720 * 348; i < 720 * 350; i++)
            blit32(snap->rgba_surface, i * 4, cga_palette[0]);

        return f(720, 350, snap->rgba_surface, userdata);
    } else if (snap->mode_ctrl_reg & 2) { // In CGA graphics mode?
        int border_color = snap->color_ctrl_reg & 0xF;

        // In high-resolution mode?
        if (snap->mode_ctrl_reg & 0x10) {
            for (int y = 0; y < 200; y++) {
                for (int x = 0; x < 640; x++) {
                    int addr = (y >> 1) * 80 + (y & 1) * 8192 + (x >> 3);
                    vxt_byte pixel = (MEMORY(snap->mem, CGA_BASE + addr) >> (7 - (x & 7))) & 1;
                    vxt_dword color = cga_palette[pixel * border_color];
                    int offset = (y * 640 + x) * 4;
                    blit32(snap->rgba_surface, offset, color);
                }
            }
            return f(640, 200, snap->rgba_surface, userdata);
        } else {
            int intensity = ((snap->color_ctrl_reg >> 4) & 1) << 3;
            int pal5 = snap->mode_ctrl_reg & 4;
            int color_index = pal5 ? 16 : ((snap->color_ctrl_reg >> 5) & 1);

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

                    vxt_dword color = cga_palette[pixel ? (pixel * 2 + color_index + intensity) : border_color];
                    blit32(snap->rgba_surface, (y * 320 + x) * 4, color);
                }
            }
            return f(320, 200, snap->rgba_surface, userdata);
        }
    } else { // We are in text mode.
        int num_col = (snap->mode_ctrl_reg & 1) ? 80 : 40;
        int num_char = num_col * 25;

        for (int i = 0; i < num_char * 2; i += 2) {
            int idx = i / 2;
            int cell_offset = CGA_BASE + snap->video_page + i;
            vxt_byte ch = MEMORY(snap->mem, cell_offset);
            vxt_byte attr = MEMORY(snap->mem, cell_offset + 1);
            blit_char(VXT_GET_DEVICE(cga_video, p), ch, attr, (idx % num_col) * 8, (idx / num_col) * 8);
        }

        if (snap->cursor_blink && snap->cursor_visible) {
            int x = snap->cursor_offset % num_col;
            int y = snap->cursor_offset / num_col;
            if (x < num_col && y < 25) {
                vxt_byte attr = (MEMORY(snap->mem, CGA_BASE + snap->video_page + (num_col * 2 * y + x * 2 + 1)) & 0x70) | 0xF;
                blit_char(VXT_GET_DEVICE(cga_video, p), -1, attr, x * 8, y * 8);
            }
        }
        return f(num_col * 8, 200, snap->rgba_surface, userdata);
    }
    return -1;
}
