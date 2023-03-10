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

#ifndef _MU_RENDERER_H_
#define _MU_RENDERER_H_

#include <SDL.h>
#include <microui.h>

typedef struct renderer mr_renderer;

mr_renderer *mr_init(SDL_Renderer *renderer);
void mr_destroy(mr_renderer *r);
void mr_draw_rect(mr_renderer *r, mu_Rect rect, mu_Color color);
void mr_draw_text(mr_renderer *r, const char *text, mu_Vec2 pos, mu_Color color);
void mr_draw_icon(mr_renderer *r, int id, mu_Rect rect, mu_Color color);
void mr_set_clip_rect(mr_renderer *r, mu_Rect rect);
void mr_clear(mr_renderer *r, mu_Color color);
void mr_present(mr_renderer *r);

int mr_get_text_width(const char *text, int len);
int mr_get_text_height(void);

#endif

