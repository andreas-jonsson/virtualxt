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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define VXT_LIBC
#include <vxt/vxt.h>
#include <vxt/vxtu.h>

#define TEMP_FILE "__scrambler.tmp"

extern struct vxt_validator *pi8088_validator(void);

int main(int argc, char *argv[]) {
	(void)argc; (void)argv;

	vxt_set_logger(&printf);
	srand((unsigned)time(NULL));

	struct vxt_pirepheral *ram = vxtu_memory_create(&realloc, 0x0, 0x100000, false);
	for (int i = 0; i < 0x100000; i++)
		ram->io.write(ram, (vxt_pointer)i, (vxt_byte)rand());

	struct vxt_pirepheral *devices[] = {
		ram,
		NULL
	};

	vxt_system *vxt = vxt_system_create(&realloc, devices);
	vxt_system_set_validator(vxt, pi8088_validator());

	vxt_error err = vxt_system_initialize(vxt);
	if (err != VXT_NO_ERROR) {
		printf("vxt_system_initialize() failed with error: %s\n", vxt_error_str(err));
		return -1;
	}

	struct vxt_registers *r = vxt_system_registers(vxt);
	for (;;) {
		printf("----------------------------------------------------------------\n");

		for (unsigned i = 0; i < sizeof(struct vxt_registers); i++)
			((unsigned char*)r)[i] = (unsigned char)rand();

		vxt_system_reset(vxt);
		r->cs = (vxt_word)rand();
		r->ip = (vxt_word)rand();
		
		{
			vxt_pointer inst = VXT_POINTER(r->cs, r->ip);
			FILE *fp = fopen(TEMP_FILE, "wb");
			fwrite((vxt_byte*)vxtu_memory_internal_pointer(ram) + inst, 1, 6, fp);
			fclose(fp);
			system("ndisasm -b 16 -o 0 " TEMP_FILE " | head -1");
		}

		struct vxt_step res = vxt_system_step(vxt, 0);
		if (res.err != VXT_NO_ERROR) {
			printf("step error: %s\n", vxt_error_str(res.err));
			return -1;
		}
	}

	vxt_system_destroy(vxt);
	remove(TEMP_FILE);
	return 0;
}
