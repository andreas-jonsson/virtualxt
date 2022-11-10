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

#ifndef _MAIN_H_
#define _MAIN_H_

extern const unsigned char *get_pcxtbios_data(void);
extern unsigned int get_pcxtbios_size(void);
extern const unsigned char *get_vxtx_data(void);
extern unsigned int get_vxtx_size(void);
extern const unsigned char *get_freedos_hd_data(void);
extern unsigned int get_freedos_hd_size(void);

extern void step_emulation(int);
extern void initialize_emulator(void);
extern const void *video_rgba_memory_pointer(void);
extern int video_width(void);
extern int video_height(void);
extern void send_key(int);

extern void js_puts(const char*, int);
extern unsigned int js_ustimer(void);
extern void js_speaker_callback(double);

#endif
