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

#ifndef _KEYS_H_
#define _KEYS_H_

#include <SDL.h>
#include <vxt/vxtu.h>

static enum vxtu_scancode sdl_to_xt_scan(SDL_Scancode scan) {
	switch ((int)scan) {
		case SDL_SCANCODE_ESCAPE:
			return VXTU_SCAN_ESCAPE;
		case SDL_SCANCODE_1:
			return VXTU_SCAN_1;
		case SDL_SCANCODE_2:
			return VXTU_SCAN_2;
		case SDL_SCANCODE_3:
			return VXTU_SCAN_3;
		case SDL_SCANCODE_4:
			return VXTU_SCAN_4;
		case SDL_SCANCODE_5:
			return VXTU_SCAN_5;
		case SDL_SCANCODE_6:
			return VXTU_SCAN_6;
		case SDL_SCANCODE_7:
			return VXTU_SCAN_7;
		case SDL_SCANCODE_8:
			return VXTU_SCAN_8;
		case SDL_SCANCODE_9:
			return VXTU_SCAN_9;
		case SDL_SCANCODE_0:
			return VXTU_SCAN_0;
		case SDL_SCANCODE_MINUS:
			return VXTU_SCAN_MINUS;
		case SDL_SCANCODE_EQUALS:
			return VXTU_SCAN_EQUAL;
		case SDL_SCANCODE_BACKSPACE:
			return VXTU_SCAN_BACKSPACE;
		case SDL_SCANCODE_TAB:
			return VXTU_SCAN_TAB;
		case SDL_SCANCODE_Q:
			return VXTU_SCAN_Q;
		case SDL_SCANCODE_W:
			return VXTU_SCAN_W;
		case SDL_SCANCODE_E:
			return VXTU_SCAN_E;
		case SDL_SCANCODE_R:
			return VXTU_SCAN_R;
		case SDL_SCANCODE_T:
			return VXTU_SCAN_T;
		case SDL_SCANCODE_Y:
			return VXTU_SCAN_Y;
		case SDL_SCANCODE_U:
			return VXTU_SCAN_U;
		case SDL_SCANCODE_I:
			return VXTU_SCAN_I;
		case SDL_SCANCODE_O:
			return VXTU_SCAN_O;
		case SDL_SCANCODE_P:
			return VXTU_SCAN_P;
		case SDL_SCANCODE_LEFTBRACKET:
			return VXTU_SCAN_LBRACKET;
		case SDL_SCANCODE_RIGHTBRACKET:
			return VXTU_SCAN_RBRACKET;
		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:
			return VXTU_SCAN_ENTER;
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
			return VXTU_SCAN_CONTROL;
		case SDL_SCANCODE_A:
			return VXTU_SCAN_A;
		case SDL_SCANCODE_S:
			return VXTU_SCAN_S;
		case SDL_SCANCODE_D:
			return VXTU_SCAN_D;
		case SDL_SCANCODE_F:
			return VXTU_SCAN_F;
		case SDL_SCANCODE_G:
			return VXTU_SCAN_G;
		case SDL_SCANCODE_H:
			return VXTU_SCAN_H;
		case SDL_SCANCODE_J:
			return VXTU_SCAN_J;
		case SDL_SCANCODE_K:
			return VXTU_SCAN_K;
		case SDL_SCANCODE_L:
			return VXTU_SCAN_L;
		case SDL_SCANCODE_SEMICOLON:
			return VXTU_SCAN_SEMICOLON;
		case SDL_SCANCODE_APOSTROPHE:
			return VXTU_SCAN_QUOTE;
		case SDL_SCANCODE_GRAVE:
			return VXTU_SCAN_BACKQUOTE;
		case SDL_SCANCODE_LSHIFT:
			return VXTU_SCAN_LSHIFT;
		case SDL_SCANCODE_BACKSLASH:
			return VXTU_SCAN_BACKSLASH;
		case SDL_SCANCODE_Z:
			return VXTU_SCAN_Z;
		case SDL_SCANCODE_X:
			return VXTU_SCAN_X;
		case SDL_SCANCODE_C:
			return VXTU_SCAN_C;
		case SDL_SCANCODE_V:
			return VXTU_SCAN_V;
		case SDL_SCANCODE_B:
			return VXTU_SCAN_B;
		case SDL_SCANCODE_N:
			return VXTU_SCAN_N;
		case SDL_SCANCODE_M:
			return VXTU_SCAN_M;
		case SDL_SCANCODE_COMMA:
			return VXTU_SCAN_COMMA;
		case SDL_SCANCODE_PERIOD:
			return VXTU_SCAN_PERIOD;
		case SDL_SCANCODE_SLASH:
			return VXTU_SCAN_SLASH;
		case SDL_SCANCODE_RSHIFT:
			return VXTU_SCAN_RSHIFT;
		case SDL_SCANCODE_PRINTSCREEN:
			return VXTU_SCAN_PRINT;
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
			return VXTU_SCAN_ALT;
		case SDL_SCANCODE_SPACE:
			return VXTU_SCAN_SPACE;
		case SDL_SCANCODE_CAPSLOCK:
			return VXTU_SCAN_CAPSLOCK;
		case SDL_SCANCODE_F1:
			return VXTU_SCAN_F1;
		case SDL_SCANCODE_F2:
			return VXTU_SCAN_F2;
		case SDL_SCANCODE_F3:
			return VXTU_SCAN_F3;
		case SDL_SCANCODE_F4:
			return VXTU_SCAN_F4;
		case SDL_SCANCODE_F5:
			return VXTU_SCAN_F5;
		case SDL_SCANCODE_F6:
			return VXTU_SCAN_F6;
		case SDL_SCANCODE_F7:
			return VXTU_SCAN_F7;
		case SDL_SCANCODE_F8:
			return VXTU_SCAN_F8;
		case SDL_SCANCODE_F9:
			return VXTU_SCAN_F9;
		case SDL_SCANCODE_F10:
			return VXTU_SCAN_F10;
		case SDL_SCANCODE_NUMLOCKCLEAR:
			return VXTU_SCAN_NUMLOCK;
		case SDL_SCANCODE_SCROLLLOCK:
			return VXTU_SCAN_SCRLOCK;
		case SDL_SCANCODE_KP_7:
			return VXTU_SCAN_KP_HOME;
		case SDL_SCANCODE_KP_8:
		case SDL_SCANCODE_UP:
			return VXTU_SCAN_KP_UP;
		case SDL_SCANCODE_KP_9:
			return VXTU_SCAN_KP_PAGEUP;
		case SDL_SCANCODE_KP_MINUS:
			return VXTU_SCAN_KP_MINUS;
		case SDL_SCANCODE_KP_4:
		case SDL_SCANCODE_LEFT:
			return VXTU_SCAN_KP_LEFT;
		case SDL_SCANCODE_KP_5:
			return VXTU_SCAN_KP_5;
		case SDL_SCANCODE_KP_6:
		case SDL_SCANCODE_RIGHT:
			return VXTU_SCAN_KP_RIGHT;
		case SDL_SCANCODE_KP_PLUS:
			return VXTU_SCAN_KP_PLUS;
		case SDL_SCANCODE_KP_1:
			return VXTU_SCAN_KP_END;
		case SDL_SCANCODE_KP_2:
		case SDL_SCANCODE_DOWN:
			return VXTU_SCAN_KP_DOWN;
		case SDL_SCANCODE_KP_3:
			return VXTU_SCAN_KP_PAGEDOWN;
		case SDL_SCANCODE_KP_0:
			return VXTU_SCAN_KP_INSERT;
		case SDL_SCANCODE_KP_COMMA:
			return VXTU_SCAN_KP_DELETE;
		default:
			return VXTU_SCAN_INVALID;
	}
}

#endif
