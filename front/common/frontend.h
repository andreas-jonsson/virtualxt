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

#ifndef _FRONTEND_H_
#define _FRONTEND_H_

#include <vxt/vxt.h>

struct frontend_video_adapter {
	struct vxt_pirepheral *device;
	vxt_dword (*border_color)(struct vxt_pirepheral *p);
	bool (*snapshot)(struct vxt_pirepheral *p);
	int (*render)(struct vxt_pirepheral *p, int (*f)(int,int,const vxt_byte*,void*), void *userdata);
};

struct frontend_audio_adapter {
	struct vxt_pirepheral *device;
	vxt_int16 (*generate_sample)(struct vxt_pirepheral *p, int freq);
};

enum frontend_ctrl_command {
	FRONTEND_CTRL_SHUTDOWN = 0x1
};

struct frontend_ctrl_interface {
    void *userdata;
	vxt_byte (*callback)(enum frontend_ctrl_command cmd, void *userdata);
};

struct frontend_interface {
    bool (*set_video_adapter)(const struct frontend_video_adapter *adapter);
    bool (*set_audio_adapter)(const struct frontend_audio_adapter *adapter);

    struct frontend_ctrl_interface ctrl;
};

#endif
