/*
Copyright (c) 2019-2021 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _REGISTERS_H_
#define _REGISTERS_H_

#include <stdbool.h>

#define DefulatQueueSize  0x400      // 1KB
#define DefaultBufferSize 0x40000000 // 1GB

enum {
	AL,
	AH,
	AX,
	CL,
	CH,
	CX,
	DL,
	DH,
	DX,
	BL,
	BH,
	BX,
	SP,
	BP,
	SI,
	DI,
	ES,
	CS,
	SS,
	DS
};

#endif
