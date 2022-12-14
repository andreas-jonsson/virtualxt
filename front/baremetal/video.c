// Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include "video.h"
#include "mbox.h"

#define COLOR_DEPTH 32
#define ALPHA_MODE 0
#define PIXEL_ORDER 0 // RGB

int width, height, pitch;
vxt_byte *frame_buffer;

bool video_init(int w, int h) {
	mbox[0] = 39 * 4; 			// Length of message in bytes
	mbox[1] = MBOX_REQUEST;

	mbox[2] = MBOX_TAG_SETPHYWH; // Set physical width-height
	mbox[3] = 8;				 // Value size in bytes
	mbox[4] = 0;				 // REQUEST CODE = 0
	mbox[5] = w;
	mbox[6] = h;

	mbox[7] = MBOX_TAG_SETVIRTWH; // Set virtual width and height
	mbox[8] = 8;
	mbox[9] = 0;
	mbox[10] = w;
	mbox[11] = h;

	mbox[12] = MBOX_TAG_SETVIRTOFF; // Set virtual offset
	mbox[13] = 8;
	mbox[14] = 0;
	mbox[15] = 0; // X offset
	mbox[16] = 0; // Y offset

	mbox[17] = MBOX_TAG_SETDEPTH; // Set color depth
	mbox[18] = 4;
	mbox[19] = 0;
	mbox[20] = COLOR_DEPTH; // Bits per pixel

	mbox[21] = MBOX_TAG_SETPXLORDR; // Set pixel order
	mbox[22] = 4;
	mbox[23] = 0;
	mbox[24] = PIXEL_ORDER;

	mbox[25] = MBOX_TAG_GETFB; // Get frame buffer
	mbox[26] = 8;
	mbox[27] = 0;
	mbox[28] = 16; 		// Alignment in 16 bytes will return Frame Buffer size in bytes
	mbox[29] = 0;

	mbox[30] = MBOX_TAG_GETPITCH; // Get pitch
	mbox[31] = 4;
	mbox[32] = 0;
	mbox[33] = 0; // Will get pitch value here

	mbox[34] = MBOX_TAG_SETALPHA;
	mbox[35] = 4;
	mbox[36] = 0;
	mbox[37] = ALPHA_MODE;

	mbox[38] = MBOX_TAG_LAST;

	bool res = mbox_call(ADDR(mbox), MBOX_CH_PROP);
	if (res	&& (mbox[20] == COLOR_DEPTH) && (mbox[24] == PIXEL_ORDER) && mbox[28]) {
		frame_buffer = (vxt_byte*)((unsigned long)(mbox[28] & 0x3FFFFFFF));
		width = mbox[5];  // Physical width
		height = mbox[6]; // Physical height
		pitch = mbox[33];
		return true;
	}
	return false;
}

void video_put_pixel(int x, int y, vxt_dword col) {
	int offset = (y * pitch) + (x * 4);
	*((vxt_dword*)(frame_buffer + offset)) = col;
}

int video_width(void) {
	return width;
}

int video_height(void) {
	return height;
}
