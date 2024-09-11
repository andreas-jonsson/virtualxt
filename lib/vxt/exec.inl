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

#include "common.h"
#include "exec.h"

#include "shift.inl"
#include "rep.inl"

#define CARRY (((inst->opcode > 0xF) && (p->regs.flags & VXT_CARRY)) ? 1 : 0)

static void add_0_10(CONSTSP(cpu) p, INST(inst)) {
   rm_write8(p, op_add_adc8(&p->regs, rm_read8(p), reg_read8(&p->regs, p->mode.reg), CARRY));
}

static void add_1_11(CONSTSP(cpu) p, INST(inst)) {
   rm_write16(p, op_add_adc16(&p->regs, rm_read16(p), reg_read16(&p->regs, p->mode.reg), CARRY));
}

static void add_2_12(CONSTSP(cpu) p, INST(inst)) {
   reg_write8(&p->regs, p->mode.reg, op_add_adc8(&p->regs, reg_read8(&p->regs, p->mode.reg), rm_read8(p), CARRY));
}

static void add_3_13(CONSTSP(cpu) p, INST(inst)) {
   reg_write16(&p->regs, p->mode.reg, op_add_adc16(&p->regs, reg_read16(&p->regs, p->mode.reg), rm_read16(p), CARRY));
}

static void add_4_14(CONSTSP(cpu) p, INST(inst)) {
   p->regs.al = op_add_adc8(&p->regs, p->regs.al, read_opcode8(p), CARRY);
}

static void add_5_15(CONSTSP(cpu) p, INST(inst)) {
   p->regs.ax = op_add_adc16(&p->regs, p->regs.ax, read_opcode16(p), CARRY);
}

#undef CARRY

#define PUSH_POP(r)                                            \
   static void push_ ## r (CONSTSP(cpu) p, INST(inst)) {       \
      UNUSED(inst);                                            \
      push(p, p->regs.r);                                      \
   }                                                           \
                                                               \
   static void pop_ ## r (CONSTSP(cpu) p, INST(inst)) {        \
      UNUSED(inst);                                            \
      p->regs.r = pop(p);                                      \
   }                                                           \

PUSH_POP(es)
PUSH_POP(ss)
PUSH_POP(ds)
PUSH_POP(ax)
PUSH_POP(bx)
PUSH_POP(cx)
PUSH_POP(dx)
PUSH_POP(bp)
PUSH_POP(si)
PUSH_POP(di)
#undef PUSH_POP

static void push_sp(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.sp -= 2;
   cpu_segment_write_word(p, p->regs.ss, p->regs.sp, p->regs.sp);
}

static void pop_sp(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.sp = cpu_segment_read_word(p, p->regs.ss, p->regs.sp);
}

static void push_cs(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   push(p, p->regs.cs);
}

static void or_8(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, op_or8(&p->regs, rm_read8(p), reg_read8(&p->regs, p->mode.reg)));
}

static void or_9(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, op_or16(&p->regs, rm_read16(p), reg_read16(&p->regs, p->mode.reg)));
}

static void or_A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write8(&p->regs, p->mode.reg, op_or8(&p->regs, reg_read8(&p->regs, p->mode.reg), rm_read8(p)));
}

static void or_B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write16(&p->regs, p->mode.reg, op_or16(&p->regs, reg_read16(&p->regs, p->mode.reg), rm_read16(p)));
}

static void or_C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = op_or8(&p->regs, p->regs.al, read_opcode8(p));
}

static void or_D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = op_or16(&p->regs, p->regs.ax, read_opcode16(p));
}

static void invalid_op(CONSTSP(cpu) p, INST(inst)) {
   VALIDATOR_DISCARD(p);
   PRINT("invalid opcode: 0x%X", inst->opcode);
   p->regs.debug = true;
}

static void invalid_prefix(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p);
   PANIC("prefix (0x%X) should have been parsed", inst->opcode);
}

#define CARRY (((inst->opcode < 0x1F) && (p->regs.flags & VXT_CARRY)) ? 1 : 0)

static void sub_18_28(CONSTSP(cpu) p, INST(inst)) {
   rm_write8(p, op_sub_sbb8(&p->regs, rm_read8(p), reg_read8(&p->regs, p->mode.reg), CARRY));
}

static void sub_19_29(CONSTSP(cpu) p, INST(inst)) {
   rm_write16(p, op_sub_sbb16(&p->regs, rm_read16(p), reg_read16(&p->regs, p->mode.reg), CARRY));
}

static void sub_1A_2A(CONSTSP(cpu) p, INST(inst)) {
   reg_write8(&p->regs, p->mode.reg, op_sub_sbb8(&p->regs, reg_read8(&p->regs, p->mode.reg), rm_read8(p), CARRY));
}

static void sub_1B_2B(CONSTSP(cpu) p, INST(inst)) {
   reg_write16(&p->regs, p->mode.reg, op_sub_sbb16(&p->regs, reg_read16(&p->regs, p->mode.reg), rm_read16(p), CARRY));
}

static void sub_1C_2C(CONSTSP(cpu) p, INST(inst)) {
   p->regs.al = op_sub_sbb8(&p->regs, p->regs.al, read_opcode8(p), CARRY);
}

static void sub_1D_2D(CONSTSP(cpu) p, INST(inst)) {
   p->regs.ax = op_sub_sbb16(&p->regs, p->regs.ax, read_opcode16(p), CARRY);
}

#undef CARRY

static void das_2F(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte al = p->regs.al;
   vxt_word af = p->regs.flags & VXT_AUXILIARY;
   vxt_word cf = p->regs.flags & VXT_CARRY;
   p->regs.flags &= ~VXT_CARRY;

   if (((al & 0xF) > 9) || FLAGS(p->regs.flags, VXT_AUXILIARY)) {
      p->regs.al -= 6;
   	p->regs.flags |= VXT_AUXILIARY;
	} else {
      p->regs.flags &= ~VXT_AUXILIARY;
	}

   if ((al > (af ? 0x9F : 0x99)) || cf) {
      p->regs.al -= 0x60;
      p->regs.flags |= VXT_CARRY;
   } else {
      p->regs.flags &= ~VXT_CARRY;
   }

   flag_szp8(&p->regs, p->regs.al);
}

static void and_20(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, op_and8(&p->regs, rm_read8(p), reg_read8(&p->regs, p->mode.reg)));
}

static void and_21(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, op_and16(&p->regs, rm_read16(p), reg_read16(&p->regs, p->mode.reg)));
}

static void and_22(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write8(&p->regs, p->mode.reg, op_and8(&p->regs, reg_read8(&p->regs, p->mode.reg), rm_read8(p)));
}

static void and_23(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write16(&p->regs, p->mode.reg, op_and16(&p->regs, reg_read16(&p->regs, p->mode.reg), rm_read16(p)));
}

static void and_24(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = op_and8(&p->regs, p->regs.al, read_opcode8(p));
}

static void and_25(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = op_and16(&p->regs, p->regs.ax, read_opcode16(p));
}

static void daa_27(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte al = p->regs.al;
   vxt_word af = p->regs.flags & VXT_AUXILIARY;
   vxt_word cf = p->regs.flags & VXT_CARRY;

   if (((al & 0xF) > 9) || FLAGS(p->regs.flags, VXT_AUXILIARY)) {
      p->regs.al += 6;
      p->regs.flags |= VXT_AUXILIARY;
	} else {
      p->regs.flags &= ~VXT_AUXILIARY;
	}

   if ((al > (af ? 0x9F : 0x99)) || cf) {
      p->regs.al += 0x60;
      p->regs.flags |= VXT_CARRY;
   } else {
      p->regs.flags &= ~VXT_CARRY;
   }

   flag_szp8(&p->regs, p->regs.al);
}

static void xor_30(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, op_xor8(&p->regs, rm_read8(p), reg_read8(&p->regs, p->mode.reg)));
}

static void xor_31(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, op_xor16(&p->regs, rm_read16(p), reg_read16(&p->regs, p->mode.reg)));
}

static void xor_32(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write8(&p->regs, p->mode.reg, op_xor8(&p->regs, reg_read8(&p->regs, p->mode.reg), rm_read8(p)));
}

static void xor_33(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write16(&p->regs, p->mode.reg, op_xor16(&p->regs, reg_read16(&p->regs, p->mode.reg), rm_read16(p)));
}

static void xor_34(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = op_xor8(&p->regs, p->regs.al, read_opcode8(p));
}

static void xor_35(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = op_xor16(&p->regs, p->regs.ax, read_opcode16(p));
}

#define ASCII(name, op)                                                          \
   static void name (CONSTSP(cpu) p, INST(inst)) {                               \
      UNUSED(inst);                                                              \
      if (((p->regs.al & 0xF) > 9) || FLAGS(p->regs.flags, VXT_AUXILIARY)) {     \
         p->regs.al = p->regs.al op 6;                                           \
         p->regs.ah = p->regs.ah op 1;                                           \
         p->regs.flags |= VXT_AUXILIARY | VXT_CARRY;                             \
      } else {                                                                   \
         p->regs.flags &= ~(VXT_AUXILIARY | VXT_CARRY);                          \
      }                                                                          \
      p->regs.al &= 0xF;                                                         \
   }                                                                             \

ASCII(aaa_37, +)
ASCII(aas_3F, -)
#undef ASCII

static void cmp_38(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb8(&p->regs, rm_read8(p), reg_read8(&p->regs, p->mode.reg), 0);
}

static void cmp_39(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb16(&p->regs, rm_read16(p), reg_read16(&p->regs, p->mode.reg), 0);
}

static void cmp_3A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb8(&p->regs, reg_read8(&p->regs, p->mode.reg), rm_read8(p), 0);
}

static void cmp_3B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb16(&p->regs, reg_read16(&p->regs, p->mode.reg), rm_read16(p), 0);
}

static void cmp_3C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb8(&p->regs, p->regs.al, read_opcode8(p), 0);
}

static void cmp_3D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb16(&p->regs, p->regs.ax, read_opcode16(p), 0);
}

static void inc_reg(CONSTSP(cpu) p, INST(inst)) {
   vxt_word c = p->regs.flags & VXT_CARRY;
   vxt_word r = inst->opcode - 0x40;
   reg_write16(&p->regs, r, op_add_adc16(&p->regs, reg_read16(&p->regs, r), 1, 0));
   SET_FLAG(p->regs.flags, VXT_CARRY, c);
}

static void dec_reg(CONSTSP(cpu) p, INST(inst)) {
   vxt_word c = p->regs.flags & VXT_CARRY;
   vxt_word r = inst->opcode - 0x48;
   reg_write16(&p->regs, r, op_sub_sbb16(&p->regs, reg_read16(&p->regs, r), 1, 0));
   SET_FLAG(p->regs.flags, VXT_CARRY, c);
}

static void pusha_60(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word sp = p->regs.sp;
   push(p, p->regs.ax);
   push(p, p->regs.cx);
   push(p, p->regs.dx);
   push(p, p->regs.bx);
   push(p, sp);
   push(p, p->regs.bp);
   push(p, p->regs.si);
   push(p, p->regs.di);
}

static void popa_61(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.di = pop(p);
   p->regs.si = pop(p);
   p->regs.bp = pop(p);
   pop(p);
   p->regs.bx = pop(p);
   p->regs.dx = pop(p);
   p->regs.cx = pop(p);
   p->regs.ax = pop(p);
}

static void bound_62(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_dword idx = sign_extend32(reg_read16(&p->regs, p->mode.reg));
   vxt_word offset = get_ea_offset(p);

   if ((idx < sign_extend32(cpu_segment_read_word(p, p->seg, offset))) || (idx > sign_extend32(cpu_segment_read_word(p, p->seg, offset + 2)))) {
      p->regs.ip = p->inst_start;
      call_int(p, 5);
   }
}

static void arpl_63(CONSTSP(cpu) p, INST(inst)) {
   VALIDATOR_DISCARD(p);
   UNUSED(inst);

   // TODO: Implement
   VXT_LOG("ARPL is not implemented!");
   p->regs.flags &= ~VXT_ZERO;
}

static void push_68(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   push(p, read_opcode16(p));
}

static void imul_69_6B(CONSTSP(cpu) p, INST(inst)) {
   vxt_int32 a = sign_extend32(rm_read16(p));
   vxt_int32 b = (inst->opcode == 69) ? sign_extend32(sign_extend16(read_opcode8(p))) : sign_extend32(read_opcode16(p));

   vxt_int32 res = a * b;
   vxt_word res16 = (vxt_word)(res & 0xFFFF);

   flag_szp16(&p->regs, res16);
   SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, res != ((vxt_int16)res));
   rm_write16(p, res16);
}

static void push_6A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   push(p, (vxt_word)read_opcode8(p));
}

static void insb_6C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   cpu_write_byte(p, VXT_POINTER(p->regs.ds, p->regs.si), system_in(p->s, p->regs.dx));
   update_di_si(p, 1);
}

static void insw_6D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   cpu_segment_write_word(p, p->regs.ds, p->regs.si, WORD(system_in(p->s, p->regs.dx + 1), system_in(p->s, p->regs.dx)));
   update_di_si(p, 2);
}

static void outsb_6E(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, p->regs.dx, cpu_read_byte(p, VXT_POINTER(p->regs.ds, p->regs.si)));
   update_di_si(p, 1);
}

static void outsw_6F(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word data = cpu_segment_read_word(p, p->regs.ds, p->regs.si);
   system_out(p->s, p->regs.dx, (vxt_byte)(data & 0xFF));
   system_out(p->s, p->regs.dx + 1, (vxt_byte)(data >> 8));
   update_di_si(p, 2);
}

#define JUMP(name, cond)                                       \
   static void jump_ ##name (CONSTSP(cpu) p, INST(inst)) {     \
      UNUSED(inst);                                            \
      vxt_word offset = sign_extend16(read_opcode8(p));        \
      if (cond) {                                              \
         p->regs.ip += offset;                                 \
         p->cycles += 12;                                      \
         p->inst_queue_dirty = true;                           \
      }                                                        \
   }                                                           \

JUMP(jo, FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jno, !FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jb, FLAGS(p->regs.flags, VXT_CARRY))
JUMP(jnb, !FLAGS(p->regs.flags, VXT_CARRY))
JUMP(jz, FLAGS(p->regs.flags, VXT_ZERO))
JUMP(jnz, !FLAGS(p->regs.flags, VXT_ZERO))
JUMP(jbe, ANY_FLAGS(p->regs.flags, VXT_CARRY|VXT_ZERO))
JUMP(ja, !ANY_FLAGS(p->regs.flags, VXT_CARRY|VXT_ZERO))
JUMP(js, FLAGS(p->regs.flags, VXT_SIGN))
JUMP(jns, !FLAGS(p->regs.flags, VXT_SIGN))
JUMP(jpe, FLAGS(p->regs.flags, VXT_PARITY))
JUMP(jpo, !FLAGS(p->regs.flags, VXT_PARITY))
JUMP(jl, FLAGS(p->regs.flags, VXT_SIGN) != FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jge, FLAGS(p->regs.flags, VXT_SIGN) == FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jle, (FLAGS(p->regs.flags, VXT_SIGN) != FLAGS(p->regs.flags, VXT_OVERFLOW)) || FLAGS(p->regs.flags, VXT_ZERO))
JUMP(jg, (FLAGS(p->regs.flags, VXT_SIGN) == FLAGS(p->regs.flags, VXT_OVERFLOW)) && !FLAGS(p->regs.flags, VXT_ZERO))
#undef JUMP

static void grp1_80_82(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte a = rm_read8(p);
   vxt_byte b = read_opcode8(p);
   vxt_byte res = 0;

   switch (p->mode.reg) {
      case 0:
         res = op_add_adc8(&p->regs, a, b, 0);
         break;
      case 1:
         res = op_or8(&p->regs, a, b);
         break;
      case 2:
         res = op_add_adc8(&p->regs, a, b, (p->regs.flags & VXT_CARRY) ? 1 : 0);
         break;
      case 3:
         res = op_sub_sbb8(&p->regs, a, b, (p->regs.flags & VXT_CARRY) ? 1 : 0);
         break;
      case 4:
         res = op_and8(&p->regs, a, b);
         break;
      case 5:
         res = op_sub_sbb8(&p->regs, a, b, 0);
         break;
      case 6:
         res = op_xor8(&p->regs, a, b);
         break;
      case 7:
         flag_sub_sbb8(&p->regs, a, b, 0);
         break;
      default:
         UNREACHABLE();
   }

   if (p->mode.reg != 7)
      rm_write8(p, res);
}

static void grp1_81_83(CONSTSP(cpu) p, INST(inst)) {
   vxt_word a = rm_read16(p);
   vxt_word b = (inst->opcode == 0x81) ? read_opcode16(p) : sign_extend16(read_opcode8(p));
   vxt_word res = 0;

   switch (p->mode.reg) {
      case 0:
         res = op_add_adc16(&p->regs, a, b, 0);
         break;
      case 1:
         res = op_or16(&p->regs, a, b);
         break;
      case 2:
         res = op_add_adc16(&p->regs, a, b, (p->regs.flags & VXT_CARRY) ? 1 : 0);
         break;
      case 3:
         res = op_sub_sbb16(&p->regs, a, b, (p->regs.flags & VXT_CARRY) ? 1 : 0);
         break;
      case 4:
         res = op_and16(&p->regs, a, b);
         break;
      case 5:
         res = op_sub_sbb16(&p->regs, a, b, 0);
         break;
      case 6:
         res = op_xor16(&p->regs, a, b);
         break;
      case 7:
         flag_sub_sbb16(&p->regs, a, b, 0);
         break;
      default:
         UNREACHABLE();
   }

   if (p->mode.reg != 7)
      rm_write16(p, res);
}

static void test_84(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_logic8(&p->regs, reg_read8(&p->regs, p->mode.reg) & rm_read8(p));
}

static void test_85(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_logic16(&p->regs, reg_read16(&p->regs, p->mode.reg) & rm_read16(p));
}

static void xchg_86(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte v = rm_read8(p);
   rm_write8(p, reg_read8(&p->regs, p->mode.reg));
   reg_write8(&p->regs, p->mode.reg, v);
}

static void xchg_87(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word v = rm_read16(p);
   rm_write16(p, reg_read16(&p->regs, p->mode.reg));
   reg_write16(&p->regs, p->mode.reg, v);
}

static void mov_88(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, reg_read8(&p->regs, p->mode.reg));
}

static void mov_89(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, reg_read16(&p->regs, p->mode.reg));
}

static void mov_8A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write8(&p->regs, p->mode.reg, rm_read8(p));
}

static void mov_8B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write16(&p->regs, p->mode.reg, rm_read16(p));
}

static void mov_8C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, seg_read16(p));
}

static void lea_8D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write16(&p->regs, p->mode.reg, VXT_POINTER(p->seg, get_ea_offset(p)) - (p->seg << 4));
}

static void mov_8E(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   seg_write16(p, rm_read16(p));
}

static void pop_8F(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, pop(p));
}

static void nop_90(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p); UNUSED(inst);
}

#define XCHG(a, b)                                                   \
   static void xchg_ ## a ## _ ## b (CONSTSP(cpu) p, INST(inst)) {   \
      UNUSED(inst);                                                  \
      vxt_word tmp = p->regs.a;                                      \
      p->regs.a = p->regs.b;                                         \
      p->regs.b = tmp;                                               \
   }                                                                 \

XCHG(cx, ax)
XCHG(dx, ax)
XCHG(bx, ax)
XCHG(sp, ax)
XCHG(bp, ax)
XCHG(si, ax)
XCHG(di, ax)
#undef XCHG

static void cbw_98(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ah = (p->regs.al & 0x80) ? 0xFF : 0;
}

static void cwd_99(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.dx = (p->regs.ax & 0x8000) ? 0xFFFF : 0;
}

static void call_9A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word ip = read_opcode16(p);
   vxt_word cs = read_opcode16(p);
   push(p, p->regs.cs);
   push(p, p->regs.ip);
   p->regs.ip = ip;
   p->regs.cs = cs;
   p->inst_queue_dirty = true;
}

static void wait_9B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p); UNUSED(inst);
   VALIDATOR_DISCARD(p);
}

static void pushf_9C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   push(p, p->regs.flags);
}

static void popf_9D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags = (pop(p) & ALL_FLAGS) | 2;
   #ifdef FLAG8086
      p->regs.flags |= 0xF000; // 8086 flags
   #else
      p->regs.flags &= 0x0FFF; // 286 flags
   #endif
}

static void sahf_9E(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags = (p->regs.flags & 0xFF00) | (vxt_word)(p->regs.ah & ALL_FLAGS) | 2;
}

static void lahf_9F(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ah = (vxt_byte)(p->regs.flags & 0xFF);
}

static void mov_A0(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = cpu_read_byte(p, VXT_POINTER(p->seg, read_opcode16(p)));
}

static void mov_A1(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = cpu_segment_read_word(p, p->seg, read_opcode16(p));
}

static void mov_A2(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   cpu_write_byte(p, VXT_POINTER(p->seg, read_opcode16(p)), p->regs.al);
}

static void mov_A3(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   cpu_segment_write_word(p, p->seg, read_opcode16(p), p->regs.ax);
}

static void test_A8(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_logic8(&p->regs, p->regs.al & read_opcode8(p));
}

static void test_A9(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_logic16(&p->regs, p->regs.ax & read_opcode16(p));
}

static void mov_reg8(CONSTSP(cpu) p, INST(inst)) {
   ENSURE(inst->opcode >= 0xB0 && inst->opcode <= 0xB7)
   reg_write8(&p->regs, inst->opcode - 0xB0, read_opcode8(p));
}

static void mov_reg16(CONSTSP(cpu) p, INST(inst)) {
   ENSURE(inst->opcode >= 0xB8 && inst->opcode <= 0xBF)
   reg_write16(&p->regs, inst->opcode - 0xB8, read_opcode16(p));
}

static void shl_C0(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, bitshift_8(p, rm_read8(p), read_opcode8(p)));
}

static void shl_C1(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, bitshift_16(p, rm_read16(p), (vxt_byte)read_opcode16(p)));
}

static void ret_C2(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word ip = pop(p);
   p->regs.sp += read_opcode16(p);
   p->regs.ip = ip;
   p->inst_queue_dirty = true;
}

static void ret_C3(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip = pop(p);
   p->inst_queue_dirty = true;
}

#define LOAD(name, r)                                                				\
   static void name (CONSTSP(cpu) p, INST(inst)) {                   				\
      UNUSED(inst);                                                  				\
      vxt_word offset = get_ea_offset(p);                     						\
      reg_write16(&p->regs, p->mode.reg, cpu_segment_read_word(p, p->seg, offset));	\
      p->regs.r = cpu_segment_read_word(p, p->seg, offset + 2);						\
   }                                                                 				\

LOAD(les_C4, es)
LOAD(lds_C5, ds)
#undef LOAD

static void mov_C6(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, read_opcode8(p));
}

static void mov_C7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, read_opcode16(p));
}

static void enter_C8(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	vxt_word size = read_opcode16(p);
	vxt_byte level = read_opcode8(p) & 0x1F;

	push(p, p->regs.bp);
	vxt_word bp = p->regs.bp;
	vxt_word sp = p->regs.sp;

	if (level > 0) {
		while (--level) {
			bp -= 2;
			push(p, cpu_segment_read_word(p, p->regs.ss, bp));
		}
		push(p, sp);
	}

	p->regs.sp -= size;
	p->regs.bp = sp;
}

static void leave_C9(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.sp = p->regs.bp;
   p->regs.bp = pop(p);
}

static void retf_CA(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word sp = read_opcode16(p);
   p->regs.ip = pop(p);
   p->regs.cs = pop(p);
   p->regs.sp += sp;
   p->inst_queue_dirty = true;
}

static void retf_CB(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip = pop(p);
   p->regs.cs = pop(p);
   p->inst_queue_dirty = true;
}

static void int_CC(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   call_int(p, 3);
}

static void int_CD(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   call_int(p, (int)read_opcode8(p));
}

static void int_CE(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   if (p->regs.flags & VXT_OVERFLOW) {
      p->cycles += 69;
      call_int(p, 4);
   }
}

static void iret_CF(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   VALIDATOR_DISCARD(p);
   p->inst_queue_dirty = true;
   p->regs.ip = pop(p);
   p->regs.cs = pop(p);
   p->regs.flags = (pop(p) & ALL_FLAGS) | 2;
   #ifdef FLAG8086
      p->regs.flags |= 0xF000; // 8086 flags
   #else
      p->regs.flags &= 0x0FFF; // 286 flags
   #endif
}

static void grp2_D0(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, bitshift_8(p, rm_read8(p), 1));
}

static void grp2_D1(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, bitshift_16(p, rm_read16(p), 1));
}

static void grp2_D2(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write8(p, bitshift_8(p, rm_read8(p), p->regs.cl));
}

static void grp2_D3(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   rm_write16(p, bitshift_16(p, rm_read16(p), p->regs.cl));
}

static void aam_D4(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte a = p->regs.al;
   vxt_byte b = read_opcode8(p);
   if (!b) {
      divZero(p);
      return;
   }
   p->regs.al = a % b;
   p->regs.ah = a / b;
   flag_szp8(&p->regs, p->regs.al);
}

static void aad_D5(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word imm = (vxt_word)read_opcode8(p);
   p->regs.ax = ((vxt_word)p->regs.ah * imm + (vxt_word)p->regs.al) & 0xFF;
   flag_szp8(&p->regs, p->regs.al);
}

static void xlat_D7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = cpu_read_byte(p, VXT_POINTER(p->seg, p->regs.bx + p->regs.al));
}

static void salc_D6(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = (p->regs.flags & VXT_CARRY) ? 0xFF : 0x0;
}

static void fpu_dummy(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p); UNUSED(inst);
   VALIDATOR_DISCARD(p);
}

static void in_E4(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = system_in(p->s, read_opcode8(p));
}

static void in_E5(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = (vxt_word)system_in(p->s, read_opcode8(p)) | 0xFF00;
}

static void out_E6(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, read_opcode8(p), p->regs.al);
}

static void out_E7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word port = read_opcode8(p);
   system_out(p->s, port, LBYTE(p->regs.ax));
   system_out(p->s, port + 1, HBYTE(p->regs.ax));
}

static void call_E8(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word offset = read_opcode16(p);
   push(p, p->regs.ip);
   p->regs.ip += offset;
   p->inst_queue_dirty = true;
}

static void jmp_E9(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip += read_opcode16(p);
   p->inst_queue_dirty = true;
}

static void jmp_EA(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word ip = read_opcode16(p);
   p->regs.cs = read_opcode16(p);
   p->regs.ip = ip;
   p->inst_queue_dirty = true;
}

static void jmp_EB(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip += sign_extend16(read_opcode8(p));
   p->inst_queue_dirty = true;
}

static void in_EC(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = system_in(p->s, p->regs.dx);
}

static void in_ED(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = WORD(system_in(p->s, p->regs.dx + 1), system_in(p->s, p->regs.dx));
}

static void out_EE(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, p->regs.dx, p->regs.al);
}

static void out_EF(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, p->regs.dx, LBYTE(p->regs.ax));
   system_out(p->s, p->regs.dx + 1, HBYTE(p->regs.ax));
}

static void lock_F0(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p); UNUSED(inst);
   VALIDATOR_DISCARD(p);
}

static void hlt_F4(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   VALIDATOR_DISCARD(p);
   p->halt = true;
}

static void cmc_F5(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   SET_FLAG_IF(p->regs.flags, VXT_CARRY, !FLAGS(p->regs.flags, VXT_CARRY));
}

static void grp3_F6(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte v = rm_read8(p);
   switch (p->mode.reg) {
      case 0: // TEST Eb Ib
      case 1:
         flag_logic8(&p->regs, v & read_opcode8(p));
         break;
      case 2: // NOT
         rm_write8(p, ~v);
         break;
      case 3: // NEG
      {
         vxt_byte res = ~v + 1;
         flag_sub_sbb8(&p->regs, 0, v, 0);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY, res);
         rm_write8(p, res);
         break;
      }
      case 4: // MUL
      {
         p->regs.ax = ((vxt_word)v) * ((vxt_word)p->regs.al);
         flag_szp8(&p->regs, p->regs.al);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, p->regs.ah);
         p->regs.flags &= ~VXT_ZERO;
         break;
      }
      case 5: // IMUL
      {
         vxt_int8 a = p->regs.al;
         vxt_int8 b = v;
         vxt_int16 res = a * b;
         vxt_byte res8 = (res & 0xFF);

         p->regs.ax = res;
         flag_szp8(&p->regs, res8);
         p->regs.flags &= ~VXT_ZERO;
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, res != ((vxt_int8)res));
         break;
      }
      case 6: // DIV
      {
         if (!v) {
            divZero(p);
            return;
         }

         vxt_word a = p->regs.ax;
         vxt_word q = a / v;
         vxt_byte r = a % v;
         vxt_byte q8 = q & 0xFF;

         if (q != q8) {
            divZero(p);
            return;
         }

         p->regs.ah = (vxt_byte)r;
         p->regs.al = (vxt_byte)q8;
         break;
      }
      case 7: // IDIV
      {
         vxt_int16 a = p->regs.ax;
         if (a == ((vxt_int16)0x8000)) {
            divZero(p);
            return;
         }

         vxt_int8 b = v;
         if (!b) {
            divZero(p);
            return;
         }

         vxt_int16 q = a / b;
         vxt_int8 r = a % b;
         vxt_word q8 = (q & 0xFF);

         if (q != q8) {
            divZero(p);
            return;
         }

         p->regs.ah = (vxt_byte)r;
         p->regs.al = (vxt_byte)q8;
         break;
      }
   }
}

static void grp3_F7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word v = rm_read16(p);
   switch (p->mode.reg) {
      case 0: // TEST Ev Iv
      case 1:
         flag_logic16(&p->regs, v & read_opcode16(p));
         break;
      case 2: // NOT
         rm_write16(p, ~v);
         break;
      case 3: // NEG
      {
         vxt_word res = ~v + 1;
         flag_sub_sbb16(&p->regs, 0, v, 0);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY, res);
         rm_write16(p, res);
         break;
      }
      case 4: // MUL
      {
         vxt_dword res = ((vxt_dword)v) * ((vxt_dword)p->regs.ax);
         p->regs.dx = (vxt_word)(res >> 16);
         p->regs.ax = (vxt_word)(res & 0xFFFF);
         flag_szp16(&p->regs, p->regs.ax);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, p->regs.dx);
         p->regs.flags &= ~VXT_ZERO;
         break;
      }
      case 5: // IMUL
      {
         vxt_int16 a = p->regs.ax;
         vxt_int16 b = v;

         vxt_int32 res = ((vxt_int32)a) * ((vxt_int32)b);
         p->regs.ax = (vxt_word)(res & 0xFFFF);
         p->regs.dx = (vxt_word)(res >> 16);

         flag_szp16(&p->regs, p->regs.ax);
         p->regs.flags &= ~VXT_ZERO;
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, res != ((vxt_int16)res));
         break;
      }
      case 6: // DIV
      {
         if (!v) {
            divZero(p);
            return;
         }

         vxt_dword a = (((vxt_dword)p->regs.dx) << 16) | ((vxt_dword)p->regs.ax);
         vxt_dword q = a / v;
         vxt_word r = a % v;
         vxt_word q16 = q & 0xFFFF;

         if (q != q16) {
            divZero(p);
            return;
         }

         p->regs.dx = r;
         p->regs.ax = q16;
         break;
      }
      case 7: // IDIV
      {
         vxt_int32 a = (((vxt_dword)p->regs.dx) << 16) | ((vxt_dword)p->regs.ax);
         if (a == ((vxt_int32)0x80000000)) {
            divZero(p);
            return;
         }

         vxt_int16 b = v;
         if (!b) {
            divZero(p);
            return;
         }

         vxt_int32 q = a / b;
         vxt_int16 r = a % b;
         vxt_int16 q16 = q & 0xFFFF;

         if (q != q16) {
            divZero(p);
            return;
         }

         p->regs.ax = (vxt_word)q16;
         p->regs.dx = (vxt_word)r;
         break;
      }
   }
}

static void clc_F8(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags &= ~VXT_CARRY;
}

static void stc_F9(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags |= VXT_CARRY;
}

static void cli_FA(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags &= ~VXT_INTERRUPT;
}

static void sti_FB(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags |= VXT_INTERRUPT;
}

static void cld_FC(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags &= ~VXT_DIRECTION;
}

static void std_FD(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags |= VXT_DIRECTION;
}

static void grp4_FE(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);

   vxt_byte v = rm_read8(p);
   vxt_word c = p->regs.flags & VXT_CARRY;
   switch (p->mode.reg) {
      case 0: // INC
         rm_write8(p, op_add_adc8(&p->regs, v, 1, 0));
         p->cycles += 3;
         break;
      case 1: // DEC
         rm_write8(p, op_sub_sbb8(&p->regs, v, 1, 0));
         p->cycles += 3;
         break;
      default:
         VALIDATOR_DISCARD(p);
         PRINT("TODO: Invalid opcode!");
         p->cycles++;
   }
   SET_FLAG(p->regs.flags, VXT_CARRY, c);
}

static void grp5_FF(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);

   vxt_word v = rm_read16(p);
   vxt_word c = p->regs.flags & VXT_CARRY;
   switch (p->mode.reg) {
      case 0: // INC
         rm_write16(p, op_add_adc16(&p->regs, v, 1, 0));
         SET_FLAG(p->regs.flags, VXT_CARRY, c);
         p->cycles += 3;
         break;
      case 1: // DEC
         rm_write16(p, op_sub_sbb16(&p->regs, v, 1, 0));
         SET_FLAG(p->regs.flags, VXT_CARRY, c);
         p->cycles += 3;
         break;
      case 2: // CALL
         push(p, p->regs.ip);
         p->regs.ip = v;
         p->inst_queue_dirty = true;
         p->cycles += 23;
         break;
      case 3: // CALL Mp
      {
         push(p, p->regs.cs);
         push(p, p->regs.ip);

         vxt_word offset = get_ea_offset(p);
         p->regs.ip = cpu_segment_read_word(p, p->seg, offset);
         p->regs.cs = cpu_segment_read_word(p, p->seg, offset + 2);
         p->inst_queue_dirty = true;
         p->cycles += 53;
         break;
      }
      case 4: // JMP
         p->regs.ip = v;
         p->inst_queue_dirty = true;
         p->cycles += 15;
         break;
      case 5: // JMP Mp
      {
         vxt_word offset = get_ea_offset(p);
         p->regs.ip = cpu_segment_read_word(p, p->seg, offset);
         p->regs.cs = cpu_segment_read_word(p, p->seg, offset + 2);
         p->inst_queue_dirty = true;
         p->cycles += 24;
         break;
      }
      case 6: // PUSH
      case 7:
         p->regs.sp -= 2;
         v = rm_read16(p);
         cpu_segment_write_word(p, p->regs.ss, p->regs.sp, v);
         p->cycles += 15;
         break;
      default:
         UNREACHABLE();
   }
}
