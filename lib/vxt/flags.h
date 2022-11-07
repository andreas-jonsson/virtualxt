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

#ifndef _FLAGS_H_
#define _FLAGS_H_

#include "common.h"
#include "cpu.h"

#define ALL_FLAGS (VXT_CARRY | VXT_PARITY | VXT_AUXILIARY | VXT_ZERO | VXT_SIGN | VXT_TRAP | VXT_INTERRUPT | VXT_DIRECTION | VXT_OVERFLOW)
#define FLAGS(r, f) ( ((r) & (f)) == (f) )
#define ANY_FLAGS(r, f) ( ((r) & (f)) != 0 )
#define SET_FLAG(r, f, v) ( (r) = ((r) & ~(f)) | ((v) & (f)) )
#define SET_FLAG_IF(r, f, c) ( SET_FLAG((r), (f), (c) ? (f) : ~(f)) )

static const bool parity_table[0x100] = {
	true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true,
   false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
   true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
   true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true,
	true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true,
   false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
   true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true,
	true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true,
   false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true,
   false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true,
   true, false, false, true, false, true, true, false,
   true, false, false, true, false, true, true, false,
   false, true, true, false, true, false, false, true
};

static void flag_szp8(CONSTSP(vxt_registers) r, vxt_byte v) {
   SET_FLAG_IF(r->flags, VXT_ZERO, !v);
   SET_FLAG_IF(r->flags, VXT_SIGN, v & 0x80);
	SET_FLAG_IF(r->flags, VXT_PARITY, parity_table[v & 0xFF]);
}

static void flag_szp16(CONSTSP(vxt_registers) r, vxt_word v) {
   SET_FLAG_IF(r->flags, VXT_ZERO, !v);
   SET_FLAG_IF(r->flags, VXT_SIGN, v & 0x8000);
   SET_FLAG_IF(r->flags, VXT_PARITY, parity_table[v & 0xFF]);
}

static void flag_add_adc8(CONSTSP(vxt_registers) r, vxt_byte a, vxt_byte b, vxt_byte c) {
   vxt_word d = (vxt_word)a + (vxt_word)b + (vxt_word)c;
   flag_szp8(r, (vxt_byte)d);
   SET_FLAG_IF(r->flags, VXT_CARRY, d & 0xFF00);
   SET_FLAG_IF(r->flags, VXT_AUXILIARY, ((a ^ b ^ d) & 0x10) == 0x10);
   SET_FLAG_IF(r->flags, VXT_OVERFLOW, ((d ^ a) & (d ^ b) & 0x80) == 0x80);
}

static void flag_add_adc16(CONSTSP(vxt_registers) r, vxt_word a, vxt_word b, vxt_word c) {
   vxt_dword d = (vxt_dword)a + (vxt_dword)b + (vxt_dword)c;
   flag_szp16(r, (vxt_word)d);
   SET_FLAG_IF(r->flags, VXT_CARRY, d & 0xFFFF0000);
   SET_FLAG_IF(r->flags, VXT_AUXILIARY, ((a ^ b ^ d) & 0x10) == 0x10);
   SET_FLAG_IF(r->flags, VXT_OVERFLOW, ((d ^ a) & (d ^ b) & 0x8000) == 0x8000);
}

static void flag_sub_sbb8(CONSTSP(vxt_registers) r, vxt_byte a, vxt_byte b, vxt_byte c) {
   vxt_word d = (vxt_word)a - ((vxt_word)b + (vxt_word)c);
   flag_szp8(r, (vxt_byte)d);
   SET_FLAG_IF(r->flags, VXT_CARRY, d & 0xFF00);
   SET_FLAG_IF(r->flags, VXT_AUXILIARY, (a ^ b ^ d) & 0x10);
   SET_FLAG_IF(r->flags, VXT_OVERFLOW, ((d ^ a) & (a ^ b) & 0x80) == 0x80);
}

static void flag_sub_sbb16(CONSTSP(vxt_registers) r, vxt_word a, vxt_word b, vxt_word c) {
   vxt_dword d = (vxt_dword)a - ((vxt_dword)b + (vxt_dword)c);
   flag_szp16(r, (vxt_word)d);
   SET_FLAG_IF(r->flags, VXT_CARRY, d & 0xFFFF0000);
   SET_FLAG_IF(r->flags, VXT_AUXILIARY, (a ^ b ^ d) & 0x10);
   SET_FLAG_IF(r->flags, VXT_OVERFLOW, (d ^ a) & (a ^ b) & 0x8000);
}

static void flag_logic8(CONSTSP(vxt_registers) r, vxt_byte v) {
	flag_szp8(r, v);
   SET_FLAG(r->flags, VXT_CARRY|VXT_OVERFLOW, 0);
}

static void flag_logic16(CONSTSP(vxt_registers) r, vxt_word v) {
	flag_szp16(r, v);
   SET_FLAG(r->flags, VXT_CARRY|VXT_OVERFLOW, 0);
}

static vxt_byte op_add_adc8(CONSTSP(vxt_registers) r, vxt_byte a, vxt_byte b, vxt_byte c) {
	vxt_byte d = a + b + c;
	flag_add_adc8(r, a, b, c);
   return d;
}

static vxt_word op_add_adc16(CONSTSP(vxt_registers) r, vxt_word a, vxt_word b, vxt_word c) {
	vxt_word d = a + b + c;
	flag_add_adc16(r, a, b, c);
   return d;
}

static vxt_byte op_sub_sbb8(CONSTSP(vxt_registers) r, vxt_byte a, vxt_byte b, vxt_byte c) {
	vxt_byte d = a - (b + c);
	flag_sub_sbb8(r, a, b, c);
   return d;
}

static vxt_word op_sub_sbb16(CONSTSP(vxt_registers) r, vxt_word a, vxt_word b, vxt_word c) {
	vxt_word d = a - (b + c);
	flag_sub_sbb16(r, a, b, c);
   return d;
}

#define NARROW(name, op, f) f(name, op, byte, 8)
#define WIDE(name, op, f) f(name, op, word, 16)

#define AND(f) f(and, &)
#define OR(f) f(or, |)
#define XOR(f) f(xor, ^)

#define LOGIC_OP_FUNC(name, op, width, bits)                                                                 \
static vxt_ ## width op_ ## name ## bits (CONSTSP(vxt_registers) r, vxt_ ## width a, vxt_ ## width b) {      \
   vxt_ ## width d = a op b;                                                                                 \
	flag_logic ## bits (r, d);                                                                                \
   return d;                                                                                                 \
}                                                                                                            \

#define LOGIC_OP(name, op)             \
   NARROW(name, op, LOGIC_OP_FUNC)     \
   WIDE(name, op, LOGIC_OP_FUNC)       \

AND(LOGIC_OP)
OR(LOGIC_OP)
XOR(LOGIC_OP)

#undef LOGIC_OP
#undef LOGIC_OP_FUNC

#undef AND
#undef OR
#undef XOR

#undef NARROW
#undef WIDE

#endif
