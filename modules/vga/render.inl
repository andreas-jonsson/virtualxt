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

static void blit32(vxt_byte *pixels, int offset, vxt_dword color) {
    pixels[offset + FRONTEND_VIDEO_RED] = (vxt_byte)((color & 0xFF0000) >> 16);
    pixels[offset + FRONTEND_VIDEO_GREEN] = (vxt_byte)((color & 0x00FF00) >> 8);
    pixels[offset + FRONTEND_VIDEO_BLUE] = (vxt_byte)(color & 0x0000FF);
    pixels[offset + FRONTEND_VIDEO_ALPHA] = FRONTEND_VIDEO_ALPHA_FILL;
}

static vxt_dword color_lookup(struct snapshot *snap, vxt_byte index) {
    index = (snap->pal_reg[index] & 0x3F) | ((snap->color_select & 0xC) << 4);
    if (snap->mode_ctrl & 0x80)
        index = (index & 0xCF) | ((snap->color_select & 3) << 4);
    return snap->palette[index];
}

static void blit_char(struct vga_video *v, int ch, vxt_byte attr, int x, int y) {
    struct snapshot * const snap = &v->snap;

    int bg_color_index = (attr & 0x70) >> 4;
	int fg_color_index = attr & 0xF;

	if (attr & 0x80) {
		if (snap->mode_ctrl & 8) {
			if (snap->cursor_blink)
				fg_color_index = bg_color_index;
		} else {
			// High intensity!
			bg_color_index += 8;
		}
	}

	vxt_dword bg_color = color_lookup(snap, bg_color_index);
	vxt_dword fg_color = color_lookup(snap, fg_color_index);
    int font = (attr & 8) ? snap->font_b : snap->font_a;

    // TODO: Fix this! We only run at 640 in textmode.
    int width = (snap->width >= 640) ? 640 : 320;
    int start = 0;
    int end = 15;

    if (ch & ~0xFF) {
        ch = 0xDB;
        start = (int)snap->cursor_start;
        end = (int)snap->cursor_end;
    }

	for (int i = start; true; i++) {
        int n = i % 16;
		vxt_byte glyph_line = snap->mem[font + ch * 32 + n];

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

static bool snapshot(struct vxt_pirepheral *p) {
    struct vga_video *v = VXT_GET_DEVICE(vga_video, p);
    if (!v->is_dirty)
        return false;

    memcpy(v->snap.mem, v->mem, MEMORY_SIZE);
    memcpy(v->snap.palette, v->palette, sizeof(v->snap.palette));
    memcpy(v->snap.pal_reg, v->reg.attr_reg, 16);
    
    v->snap.width = v->width;
    v->snap.height = v->height;
    v->snap.bpp = v->bpp;
    v->snap.textmode = v->textmode;

    vxt_byte char_map_reg = v->reg.seq_reg[0x3];
    vxt_byte fa = ((char_map_reg >> 3) & 4) | ((char_map_reg >> 2) & 3);
    vxt_byte fb = ((char_map_reg >> 2) & 4) | (char_map_reg & 3);

    const int font_offsets[] = { 0x0000, 0x4000, 0x8000, 0xC000, 0x2000, 0x6000, 0xA000, 0xE000 };
    v->snap.font_a = font_offsets[fa];
    v->snap.font_b = font_offsets[fb];

    v->snap.video_page = ((int)v->reg.crt_reg[0xC] << 8) + (int)v->reg.crt_reg[0xD];
    v->snap.plane_mode = !(v->reg.seq_reg[0x4] & 6);
    v->snap.mode_ctrl = v->reg.attr_reg[0x10];
    v->snap.pixel_shift = v->reg.attr_reg[0x13] & 15;
    v->snap.color_select = v->reg.attr_reg[0x14];

    v->snap.cursor_offset = v->cursor_offset;
    v->snap.cursor_visible = v->cursor_visible;
    v->snap.cursor_start = v->cursor_start;
    v->snap.cursor_end = v->cursor_end;
    v->snap.cursor_blink = v->cursor_blink;

    v->is_dirty = false;
    return true;
}

static int render(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata) {
    struct vga_video *v = VXT_GET_DEVICE(vga_video, p);
    struct snapshot * const snap = &v->snap;

    if (snap->textmode) {
        int num_col = (snap->width < 640) ? 40 : 80;
        int num_char = num_col * 25;

        for (int i = 0; i < num_char * 2; i += 2) {
            int idx = i / 2;
            int cell_offset = CGA_BASE + snap->video_page + i;
            vxt_byte ch = MEMORY(snap->mem, cell_offset);
            vxt_byte attr = MEMORY(snap->mem, cell_offset + 1);
            blit_char(v, ch, attr, (idx % num_col) * 8, (idx / num_col) * 16);
        }

        if (snap->cursor_blink && snap->cursor_visible) {
            int x = snap->cursor_offset % num_col;
            int y = snap->cursor_offset / num_col;
            if (x < num_col && y < 25) {
                vxt_byte attr = (MEMORY(snap->mem, CGA_BASE + snap->video_page + (num_col * 2 * y + x * 2 + 1)) & 0x70) | 0xF;
                blit_char(v, -1, attr, x * 8, y * 16);
            }
        }
        return f(num_col * 8, 400, snap->rgba_surface, userdata);
    }

    if (snap->bpp == 1) {
        for (int y = 0; y < snap->height; y++) {
            for (int x = 0; x < snap->width; x++) {
                int addr = (y >> 1) * 80 + (y & 1) * 8192 + (x >> 3);
                vxt_byte index = (MEMORY(snap->mem, CGA_BASE + addr) >> (7 - (x & 7))) & 1;
                int offset = (y * snap->width + x) * 4;
                blit32(snap->rgba_surface, offset, color_lookup(snap, index));
            }
        }
        return f(snap->width, snap->height, snap->rgba_surface, userdata);
    }

    if (snap->bpp == 2) {
        for (int y = 0; y < snap->height; y++) {
            for (int x = 0; x < snap->width; x++) {
                int addr = (y >> 1) * 80 + (y & 1) * 8192 + (x >> 2);
                vxt_byte index = MEMORY(snap->mem, CGA_BASE + addr);

                switch (x & 3) {
                    case 0:
                        index = (index >> 6) & 3;
                        break;
                    case 1:
                        index = (index >> 4) & 3;
                        break;
                    case 2:
                        index = (index >> 2) & 3;
                        break;
                    case 3:
                        index = index & 3;
                        break;
                }

                blit32(snap->rgba_surface, (y * 320 + x) * 4, color_lookup(snap, index));
            }
        }
        return f(snap->width, snap->height, snap->rgba_surface, userdata);
    }

    if (snap->bpp == 4) {
        for (int y = 0; y < snap->height; y++) {
            for (int x = 0; x < snap->width; x++) {
                int addr = (y * (snap->width >> 3) + (x >> 3)) + snap->video_page;
                int shift = 7 - (x & 7);

                vxt_byte index = (MEMORY(snap->mem, addr) >> shift) & 1;
                index |= ((MEMORY(snap->mem, PLANE_SIZE + addr) >> shift) & 1) << 1;
                index |= ((MEMORY(snap->mem, PLANE_SIZE * 2 + addr) >> shift) & 1) << 2;
                index |= ((MEMORY(snap->mem, PLANE_SIZE * 3 + addr) >> shift) & 1) << 3;

                blit32(snap->rgba_surface, (y * snap->width + x) * 4, color_lookup(snap, index));
            }
        }
        return f(snap->width, snap->height, snap->rgba_surface, userdata);
    }

    if (snap->bpp == 8) {
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
    
    if (snap->bpp == 0) // Assume uninitialized. Render junk.
        return f(640, 400, snap->rgba_surface, userdata);

    VXT_LOG("Unsupported video mode: %dx%d %dbpp", snap->width, snap->height, snap->bpp);
    return -1;
}
