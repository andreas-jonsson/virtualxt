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

#include <stdlib.h>
#include <string.h>

#include "mu_renderer.h"
#include "mu_atlas.inl"

#define BUFFER_SIZE 16384

struct renderer {
	float tex_buf[BUFFER_SIZE * 8];
	float vert_buf[BUFFER_SIZE * 8];
	uint8_t color_buf[BUFFER_SIZE * 16];
	uint32_t index_buf[BUFFER_SIZE * 6];

	int buf_idx;

	SDL_Renderer *renderer;
	SDL_Texture *texture;
};

mr_renderer *mr_init(SDL_Renderer *renderer) {
	mr_renderer *r = (mr_renderer*)SDL_malloc(sizeof(struct renderer));
	SDL_memset(r, 0, sizeof(struct renderer));

	r->renderer = renderer;
	r->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, ATLAS_WIDTH, ATLAS_HEIGHT);
	
	if (!r->texture) {
		SDL_Log("Error creating texture: %s", SDL_GetError());
		SDL_free(r);
		return NULL;
	}

	{
		Uint8 *ptr = SDL_malloc(4 * ATLAS_WIDTH * ATLAS_HEIGHT);
		for (int x = 0; x < ATLAS_WIDTH * ATLAS_HEIGHT; x++) {
			ptr[4 * x] = 255;
			ptr[4 * x + 1] = 255;
			ptr[4 * x + 2] = 255;
			ptr[4 * x + 3] = atlas_texture[x];
		}
		SDL_UpdateTexture(r->texture, NULL, ptr, 4 * ATLAS_WIDTH);
		SDL_free(ptr);
	}
	SDL_SetTextureBlendMode(r->texture, SDL_BLENDMODE_BLEND);
	return r;
}

void mr_destroy(mr_renderer *r) {
	SDL_DestroyTexture(r->texture);
	SDL_memset(r, 0, sizeof(struct renderer));
	SDL_free(r);
}

static void flush(mr_renderer *r) {
	if (!r->buf_idx)
		return;

	int xy_stride = 2 * sizeof(float);
	float *xy = r->vert_buf;

	int uv_stride = 2 * sizeof(float);
	float *uv = r->tex_buf;

	int col_stride = 4 * sizeof(Uint8);
	int *color = (int*)r->color_buf;

	SDL_RenderGeometryRaw(r->renderer, r->texture,
		xy, xy_stride, (const SDL_Color*)color,
		col_stride,
		uv, uv_stride,
		r->buf_idx * 4,
		r->index_buf, r->buf_idx * 6, sizeof(int));

	r->buf_idx = 0;
}

static void push_quad(mr_renderer *r, mu_Rect dst, mu_Rect src, mu_Color color) {
	if (r->buf_idx == BUFFER_SIZE)
		flush(r);

	int texvert_idx = r->buf_idx *  8;
	int   color_idx = r->buf_idx * 16;
	int element_idx = r->buf_idx *  4;
	int   index_idx = r->buf_idx *  6;
	r->buf_idx++;

	/* update texture buffer */
	float x = src.x / (float) ATLAS_WIDTH;
	float y = src.y / (float) ATLAS_HEIGHT;
	float w = src.w / (float) ATLAS_WIDTH;
	float h = src.h / (float) ATLAS_HEIGHT;
	r->tex_buf[texvert_idx + 0] = x;
	r->tex_buf[texvert_idx + 1] = y;
	r->tex_buf[texvert_idx + 2] = x + w;
	r->tex_buf[texvert_idx + 3] = y;
	r->tex_buf[texvert_idx + 4] = x;
	r->tex_buf[texvert_idx + 5] = y + h;
	r->tex_buf[texvert_idx + 6] = x + w;
	r->tex_buf[texvert_idx + 7] = y + h;

	/* update vertex buffer */
	r->vert_buf[texvert_idx + 0] = (float)dst.x;
	r->vert_buf[texvert_idx + 1] = (float)dst.y;
	r->vert_buf[texvert_idx + 2] = (float)dst.x + dst.w;
	r->vert_buf[texvert_idx + 3] = (float)dst.y;
	r->vert_buf[texvert_idx + 4] = (float)dst.x;
	r->vert_buf[texvert_idx + 5] = (float)dst.y + dst.h;
	r->vert_buf[texvert_idx + 6] = (float)dst.x + dst.w;
	r->vert_buf[texvert_idx + 7] = (float)dst.y + dst.h;

	/* update color buffer */
	memcpy(r->color_buf + color_idx +  0, &color, 4);
	memcpy(r->color_buf + color_idx +  4, &color, 4);
	memcpy(r->color_buf + color_idx +  8, &color, 4);
	memcpy(r->color_buf + color_idx + 12, &color, 4);

	/* update index buffer */
	r->index_buf[index_idx + 0] = element_idx + 0;
	r->index_buf[index_idx + 1] = element_idx + 1;
	r->index_buf[index_idx + 2] = element_idx + 2;
	r->index_buf[index_idx + 3] = element_idx + 2;
	r->index_buf[index_idx + 4] = element_idx + 3;
	r->index_buf[index_idx + 5] = element_idx + 1;
}

void mr_draw_rect(mr_renderer *r, mu_Rect rect, mu_Color color) {
	push_quad(r, rect, atlas[ATLAS_WHITE], color);
}

void mr_draw_text(mr_renderer *r, const char *text, mu_Vec2 pos, mu_Color color) {
	mu_Rect dst = { pos.x, pos.y, 0, 0 };
	for (const char *p = text; *p; p++) {
		if ((*p & 0xc0) == 0x80)
			continue;

		int chr = mu_min((unsigned char) *p, 127);
		mu_Rect src = atlas[ATLAS_FONT + chr];
		dst.w = src.w;
		dst.h = src.h;
		push_quad(r, dst, src, color);
		dst.x += dst.w;
	}
}

void mr_draw_icon(mr_renderer *r, int id, mu_Rect rect, mu_Color color) {
	mu_Rect src = atlas[id];
	int x = rect.x + (rect.w - src.w) / 2;
	int y = rect.y + (rect.h - src.h) / 2;
	push_quad(r, mu_rect(x, y, src.w, src.h), src, color);
}

int mr_get_text_width(const char *text, int len) {
	int res = 0;
	for (const char *p = text; *p && len--; p++) {
		if ((*p & 0xc0) == 0x80)
			continue;

		int chr = mu_min((unsigned char) *p, 127);
		res += atlas[ATLAS_FONT + chr].w;
	}
	return res;
}

int mr_get_text_height(void) {
	return 18;
}

void mr_set_clip_rect(mr_renderer *r, mu_Rect rect) {
	flush(r);
	SDL_Rect rc;
	rc.x = rect.x;
	rc.y = rect.y;
	rc.w = rect.w;
	rc.h = rect.h;
	SDL_RenderSetClipRect(r->renderer, &rc);
}

void mr_clear(mr_renderer *r, mu_Color clr) {
	flush(r);
	SDL_SetRenderDrawColor(r->renderer, clr.r, clr.g, clr.b, clr.a);
	SDL_RenderClear(r->renderer);
}

void mr_present(mr_renderer *r) {
	flush(r);
	SDL_RenderPresent(r->renderer);
}
