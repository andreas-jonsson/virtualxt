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

#ifndef _KEYS_H_
#define _KEYS_H_

#include <libretro.h>
#include <vxt/vxtu.h>

static enum vxtu_scancode retro_to_xt_scan(enum retro_key key) {
	switch (key) {
		case RETROK_ESCAPE:
			return VXTU_SCAN_ESCAPE;
		case RETROK_1:
			return VXTU_SCAN_1;
		case RETROK_2:
			return VXTU_SCAN_2;
		case RETROK_3:
			return VXTU_SCAN_3;
		case RETROK_4:
			return VXTU_SCAN_4;
		case RETROK_5:
			return VXTU_SCAN_5;
		case RETROK_6:
			return VXTU_SCAN_6;
		case RETROK_7:
			return VXTU_SCAN_7;
		case RETROK_8:
			return VXTU_SCAN_8;
		case RETROK_9:
			return VXTU_SCAN_9;
		case RETROK_0:
			return VXTU_SCAN_0;
		case RETROK_MINUS:
			return VXTU_SCAN_MINUS;
		case RETROK_EQUALS:
			return VXTU_SCAN_EQUAL;
		case RETROK_BACKSPACE:
			return VXTU_SCAN_BACKSPACE;
		case RETROK_TAB:
			return VXTU_SCAN_TAB;
		case RETROK_q:
			return VXTU_SCAN_Q;
		case RETROK_w:
			return VXTU_SCAN_W;
		case RETROK_e:
			return VXTU_SCAN_E;
		case RETROK_r:
			return VXTU_SCAN_R;
		case RETROK_t:
			return VXTU_SCAN_T;
		case RETROK_y:
			return VXTU_SCAN_Y;
		case RETROK_u:
			return VXTU_SCAN_U;
		case RETROK_i:
			return VXTU_SCAN_I;
		case RETROK_o:
			return VXTU_SCAN_O;
		case RETROK_p:
			return VXTU_SCAN_P;
		case RETROK_LEFTBRACKET:
			return VXTU_SCAN_LBRACKET;
		case RETROK_RIGHTBRACKET:
			return VXTU_SCAN_RBRACKET;
		case RETROK_RETURN:
		case RETROK_KP_ENTER:
			return VXTU_SCAN_ENTER;
		case RETROK_LCTRL:
		case RETROK_RCTRL:
			return VXTU_SCAN_CONTROL;
		case RETROK_a:
			return VXTU_SCAN_A;
		case RETROK_s:
			return VXTU_SCAN_S;
		case RETROK_d:
			return VXTU_SCAN_D;
		case RETROK_f:
			return VXTU_SCAN_F;
		case RETROK_g:
			return VXTU_SCAN_G;
		case RETROK_h:
			return VXTU_SCAN_H;
		case RETROK_j:
			return VXTU_SCAN_J;
		case RETROK_k:
			return VXTU_SCAN_K;
		case RETROK_l:
			return VXTU_SCAN_L;
		case RETROK_SEMICOLON:
			return VXTU_SCAN_SEMICOLON;
		case RETROK_QUOTE:
			return VXTU_SCAN_QUOTE;
		case RETROK_BACKQUOTE:
			return VXTU_SCAN_BACKQUOTE;
		case RETROK_LSHIFT:
			return VXTU_SCAN_LSHIFT;
		case RETROK_BACKSLASH:
			return VXTU_SCAN_BACKSLASH;
		case RETROK_z:
			return VXTU_SCAN_Z;
		case RETROK_x:
			return VXTU_SCAN_X;
		case RETROK_c:
			return VXTU_SCAN_C;
		case RETROK_v:
			return VXTU_SCAN_V;
		case RETROK_b:
			return VXTU_SCAN_B;
		case RETROK_n:
			return VXTU_SCAN_N;
		case RETROK_m:
			return VXTU_SCAN_M;
		case RETROK_COMMA:
			return VXTU_SCAN_COMMA;
		case RETROK_PERIOD:
			return VXTU_SCAN_PERIOD;
		case RETROK_SLASH:
			return VXTU_SCAN_SLASH;
		case RETROK_RSHIFT:
			return VXTU_SCAN_RSHIFT;
		case RETROK_PRINT:
			return VXTU_SCAN_PRINT;
		case RETROK_LALT:
		case RETROK_RALT:
			return VXTU_SCAN_ALT;
		case RETROK_SPACE:
			return VXTU_SCAN_SPACE;
		case RETROK_CAPSLOCK:
			return VXTU_SCAN_CAPSLOCK;
		case RETROK_F1:
			return VXTU_SCAN_F1;
		case RETROK_F2:
			return VXTU_SCAN_F2;
		case RETROK_F3:
			return VXTU_SCAN_F3;
		case RETROK_F4:
			return VXTU_SCAN_F4;
		case RETROK_F5:
			return VXTU_SCAN_F5;
		case RETROK_F6:
			return VXTU_SCAN_F6;
		case RETROK_F7:
			return VXTU_SCAN_F7;
		case RETROK_F8:
			return VXTU_SCAN_F8;
		case RETROK_F9:
			return VXTU_SCAN_F9;
		case RETROK_F10:
			return VXTU_SCAN_F10;
		case RETROK_NUMLOCK:
			return VXTU_SCAN_NUMLOCK;
		case RETROK_SCROLLOCK:
			return VXTU_SCAN_SCRLOCK;
		case RETROK_KP7: case RETROK_HOME:
			return VXTU_SCAN_KP_HOME;
		case RETROK_KP8: case RETROK_UP:
			return VXTU_SCAN_KP_UP;
		case RETROK_KP9: case RETROK_PAGEUP:
			return VXTU_SCAN_KP_PAGEUP;
		case RETROK_KP_MINUS:
			return VXTU_SCAN_KP_MINUS;
		case RETROK_KP4: case RETROK_LEFT:
			return VXTU_SCAN_KP_LEFT;
		case RETROK_KP5:
			return VXTU_SCAN_KP_5;
		case RETROK_KP6: case RETROK_RIGHT:
			return VXTU_SCAN_KP_RIGHT;
		case RETROK_KP_PLUS:
			return VXTU_SCAN_KP_PLUS;
		case RETROK_KP1: case RETROK_END:
			return VXTU_SCAN_KP_END;
		case RETROK_KP2: case RETROK_DOWN:
			return VXTU_SCAN_KP_DOWN;
		case RETROK_KP3: case RETROK_PAGEDOWN:
			return VXTU_SCAN_KP_PAGEDOWN;
		case RETROK_KP0: case RETROK_INSERT:
			return VXTU_SCAN_KP_INSERT;
		case RETROK_KP_PERIOD: case RETROK_DELETE:
			return VXTU_SCAN_KP_DELETE;
		default:
			return VXTU_SCAN_INVALID;
	}
}

#endif
