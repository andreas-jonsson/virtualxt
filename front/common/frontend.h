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

#ifndef _FRONTEND_H_
#define _FRONTEND_H_

#include <vxt/vxtu.h>

#define FRONTEND_INTERFACE_VERSION 0

#ifndef FRONTEND_VIDEO_RED
	#define FRONTEND_VIDEO_RED 0
#endif
#ifndef FRONTEND_VIDEO_GREEN
	#define FRONTEND_VIDEO_GREEN 1
#endif
#ifndef FRONTEND_VIDEO_BLUE
	#define FRONTEND_VIDEO_BLUE 2
#endif
#ifndef FRONTEND_VIDEO_ALPHA
	#define FRONTEND_VIDEO_ALPHA 3
#endif
#ifndef FRONTEND_VIDEO_ALPHA_FILL
	#define FRONTEND_VIDEO_ALPHA_FILL 0xFF
#endif

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

enum frontend_mouse_button {
    FRONTEND_MOUSE_RIGHT = 0x1,
	FRONTEND_MOUSE_LEFT  = 0x2
};

struct frontend_mouse_event {
	enum frontend_mouse_button buttons;
	int xrel;
    int yrel;
};

struct frontend_mouse_adapter {
	struct vxt_pirepheral *device;
	bool (*push_event)(struct vxt_pirepheral *p, const struct frontend_mouse_event *ev);
};

struct frontend_disk_controller {
	struct vxt_pirepheral *device;
	vxt_error (*mount)(struct vxt_pirepheral *p, int num, void *fp);
	bool (*unmount)(struct vxt_pirepheral *p, int num);
	void (*set_boot)(struct vxt_pirepheral *p, int num);
};

enum frontend_path_type {
	FRONTEND_CONFIG_PATH,
	FRONTEND_BIOS_PATH,
	FRONTEND_MODULE_PATH,
	FRONTEND_ANY_PATH
};

enum frontend_joystick_id {
	FRONTEND_JOYSTICK_1,
	FRONTEND_JOYSTICK_2
};

enum frontend_joystick_button {
    FRONTEND_JOYSTICK_A	= 0x1,
	FRONTEND_JOYSTICK_B = 0x2
};

struct frontend_joystick_event {
	enum frontend_joystick_id id;
	enum frontend_joystick_button buttons;
	vxt_int16 xaxis;
    vxt_int16 yaxis;
};

struct frontend_joystick_controller {
    struct vxt_pirepheral *device;
	bool (*push_event)(struct vxt_pirepheral *p, const struct frontend_joystick_event *ev);
};

struct frontend_keyboard_controller {
    struct vxt_pirepheral *device;
	bool (*push_event)(struct vxt_pirepheral *p, enum vxtu_scancode key, bool force);
};

enum frontend_ctrl_command {
	FRONTEND_CTRL_RESET,
	FRONTEND_CTRL_SHUTDOWN,
	FRONTEND_CTRL_HAS_DATA,
	FRONTEND_CTRL_READ_DATA,
	FRONTEND_CTRL_WRITE_DATA,
	FRONTEND_CTRL_POPEN,
	FRONTEND_CTRL_PCLOSE,
	FRONTEND_CTRL_FCLOSE,
	FRONTEND_CTRL_PUSH,
	FRONTEND_CTRL_PULL,
	FRONTEND_CTRL_DEBUG
};

struct frontend_ctrl_interface {
    void *userdata;
	vxt_byte (*callback)(enum frontend_ctrl_command cmd, vxt_byte data, void *userdata);
};

struct frontend_disk_interface {
    void *userdata;
	void (*activity_callback)(int num, void *userdata);
	struct vxtu_disk_interface di;
};

struct frontend_interface {
	int interface_version;

    bool (*set_video_adapter)(const struct frontend_video_adapter *adapter);
    bool (*set_audio_adapter)(const struct frontend_audio_adapter *adapter);
	bool (*set_mouse_adapter)(const struct frontend_mouse_adapter *adapter);
	bool (*set_keyboard_controller)(const struct frontend_keyboard_controller *controller);
	bool (*set_disk_controller)(const struct frontend_disk_controller *controller);
	bool (*set_joystick_controller)(const struct frontend_joystick_controller *controller);

	const char *(*resolve_path)(enum frontend_path_type type, const char *path);

    struct frontend_ctrl_interface ctrl;
	struct frontend_disk_interface disk;
};

#endif
