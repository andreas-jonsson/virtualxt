/*
Copyright (c) 2019-2022 Andreas T Jonsson

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

#include "common.h"
#include "system.h"
#include "cpu.h"

#define CARRY (((inst->opcode > 0xF) && (p->regs.flags & VXT_CARRY)) ? 1 : 0)

static void add_0_2_10_12(CONSTSP(cpu) p, INST(inst)) {
   write_dest8(p, op_add_adc8(&p->regs, read_dest8(p), read_source8(p), CARRY));
}

static void add_1_3_11_13(CONSTSP(cpu) p, INST(inst)) {
   write_dest16(p, op_add_adc16(&p->regs, read_dest16(p), read_source16(p), CARRY));
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
PUSH_POP(sp)
PUSH_POP(bp)
PUSH_POP(si)
PUSH_POP(di)
#undef PUSH_POP

static void push_cs(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   push(p, p->regs.cs);
}

static void or_8_A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest8(p, op_or8(&p->regs, read_dest8(p), read_source8(p)));
}

static void or_9_B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, op_or16(&p->regs, read_dest16(p), read_source16(p)));
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
   UNUSED(p); UNUSED(inst);
}

static void invalid_prefix(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p);
   PANIC("prefix (0x%X) should have been parsed", inst->opcode);
}

#define CARRY (((inst->opcode < 0x1F) && (p->regs.flags & VXT_CARRY)) ? 1 : 0)

static void sub_18_1A_28_2A(CONSTSP(cpu) p, INST(inst)) {
   write_dest8(p, op_sub_sbb8(&p->regs, read_dest8(p), read_source8(p), CARRY));
}

static void sub_19_1B_29_2B(CONSTSP(cpu) p, INST(inst)) {
   write_dest16(p, op_sub_sbb16(&p->regs, read_dest16(p), read_source16(p), CARRY));
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
   if (((p->regs.al & 0xF) > 9) || FLAGS(p->regs.flags, VXT_AUXILIARY)) {
      vxt_word v = ((vxt_word)p->regs.al) - 6;
      p->regs.al = (vxt_byte)(v & 0xFF);
      SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 0xFF00);
   	p->regs.flags |= VXT_AUXILIARY;
	} else {
      p->regs.flags &= ~VXT_AUXILIARY;
	}

   if (((p->regs.al & 0xF0) > 0x90) || FLAGS(p->regs.flags, VXT_CARRY) ) {
      p->regs.al -= 0x60;
      p->regs.flags |= VXT_CARRY;
   } else {
      p->regs.flags &= ~VXT_CARRY;
   }

   flag_szp8(&p->regs, p->regs.al);
}

static void and_20_22(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest8(p, op_and8(&p->regs, read_dest8(p), read_source8(p)));
}

static void and_21_23(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, op_and16(&p->regs, read_dest16(p), read_source16(p)));
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
   if (((p->regs.al & 0xF) > 9) || FLAGS(p->regs.flags, VXT_AUXILIARY)) {
      vxt_word v = ((vxt_word)p->regs.al) + 6;
      p->regs.al = (vxt_byte)(v & 0xFF);
      SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 0xFF00);
   	p->regs.flags |= VXT_AUXILIARY;
	} else {
      p->regs.flags &= ~VXT_AUXILIARY;
	}
   
   if ((p->regs.al > 0x9F) || FLAGS(p->regs.flags, VXT_CARRY)) {
      p->regs.al += 0x60;
      p->regs.flags |= VXT_CARRY;
   } else {
      p->regs.flags &= ~VXT_CARRY;
   }

   flag_szp8(&p->regs, p->regs.al);
}

static void xor_30_32(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest8(p, op_xor8(&p->regs, read_dest8(p), read_source8(p)));
}

static void xor_31_33(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, op_xor16(&p->regs, read_dest16(p), read_source16(p)));
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

static void cmp_38_3A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb8(&p->regs, read_dest8(p), read_source8(p), 0);
}

static void cmp_39_3B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb16(&p->regs, read_dest16(p), read_source16(p), 0);
}

static void cmp_3C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb16(&p->regs, p->regs.al, read_opcode8(p), 0);
}

static void cmp_3D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_sub_sbb16(&p->regs, p->regs.ax, read_opcode16(p), 0);
}

static void inc_reg(CONSTSP(cpu) p, INST(inst)) {
   vxt_word c = p->regs.flags & VXT_CARRY;
   op_add_adc16(&p->regs, reg_read16(&p->regs, inst->opcode - 0x40), 1, 0);
   SET_FLAG(p->regs.flags, VXT_CARRY, c);
}

static void dec_reg(CONSTSP(cpu) p, INST(inst)) {
   vxt_word c = p->regs.flags & VXT_CARRY;
   op_sub_sbb16(&p->regs, reg_read16(&p->regs, inst->opcode - 0x48), 1, 0);
   SET_FLAG(p->regs.flags, VXT_CARRY, c);
}

#define JUMP(name, cond)                                       \
   static void jump_ ##name (CONSTSP(cpu) p, INST(inst)) {     \
      UNUSED(inst);                                            \
      vxt_word offset = SIGNEXT16(read_opcode8(p));            \
      if (cond) {                                              \
         p->regs.ip += offset;                                 \
         p->cycles += 12;                                      \
      }                                                        \
   }                                                           \

JUMP(jo, FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jno, !FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jb, FLAGS(p->regs.flags, VXT_CARRY))
JUMP(jnb, !FLAGS(p->regs.flags, VXT_CARRY))
JUMP(jz, FLAGS(p->regs.flags, VXT_ZERO))
JUMP(jnz, !FLAGS(p->regs.flags, VXT_ZERO))
JUMP(jbe, ANY_FLAGS(p->regs.flags, VXT_CARRY|VXT_ZERO))
JUMP(ja, !FLAGS(p->regs.flags, VXT_CARRY|VXT_ZERO))
JUMP(js, FLAGS(p->regs.flags, VXT_SIGN))
JUMP(jns, !FLAGS(p->regs.flags, VXT_SIGN))
JUMP(jpe, FLAGS(p->regs.flags, VXT_PARITY))
JUMP(jpo, !FLAGS(p->regs.flags, VXT_PARITY))
JUMP(jl, FLAGS(p->regs.flags, VXT_SIGN) != FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jge, FLAGS(p->regs.flags, VXT_SIGN) == FLAGS(p->regs.flags, VXT_OVERFLOW))
JUMP(jle, (FLAGS(p->regs.flags, VXT_SIGN) != FLAGS(p->regs.flags, VXT_OVERFLOW)) || FLAGS(p->regs.flags, VXT_ZERO))
JUMP(jg, (!FLAGS(p->regs.flags, VXT_ZERO) && FLAGS(p->regs.flags, VXT_SIGN)) == FLAGS(p->regs.flags, VXT_OVERFLOW))
#undef JUMP

static void grp1_80_82(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte a = read_dest8(p);
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

   if (p->mode.reg < 7)
      write_dest8(p, res);
}

static void grp1_81_83(CONSTSP(cpu) p, INST(inst)) {
   vxt_word a = read_dest16(p);
   vxt_word b = (inst->opcode == 0x81) ? read_opcode16(p) : SIGNEXT16(read_opcode8(p));
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

   if (p->mode.reg < 7)
      write_dest16(p, res);
}

static void test_84(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_logic8(&p->regs, reg_read8(&p->regs, p->mode.reg) & read_source8(p));
}

static void test_85(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   flag_logic16(&p->regs, reg_read16(&p->regs, p->mode.reg) & read_source16(p));
}

static void xchg_86(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte v = reg_read8(&p->regs, p->mode.reg);
   reg_write8(&p->regs, p->mode.reg, read_dest8(p));
   write_dest8(p, v);
}

static void xchg_87(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word v = reg_read16(&p->regs, p->mode.reg);
   reg_write16(&p->regs, p->mode.reg, read_dest16(p));
   write_dest16(p, v);
}

static void mov_88_8A(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest8(p, read_source8(p));
}

static void mov_89_8B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, read_source16(p));
}

static void mov_8C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, seg_read16(p));
}

static void lea_8D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   reg_write16(&p->regs, p->mode.reg, get_effective_address(p) - (p->seg << 4));
}

static void mov_8E(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   seg_write16(p, read_source16(p));
}

static void pop_8F(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, pop(p));
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
   p->regs.ah = ((p->regs.al & 0x80) == 0x80) ? 0xFF : 0;
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
}

static void wait_9B(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p); UNUSED(inst);
}

static void pushf_9C(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   push(p, (p->regs.flags & ALL_FLAGS) | 2);
   //#ifdef VXT_CPU_286
   //   push(p, p->regs.flags | 0x0802);
   //#else
   //   push(p, p->regs.flags | 0xF802);
   //#endif
}

static void popf_9D(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags = (pop(p) & ALL_FLAGS) | 2;
   //#ifdef VXT_CPU_286
   //   p->regs.flags |= 0x0802;
   //#else
   //   p->regs.flags |= 0xF802;
   //#endif
}

static void sahf_9E(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.flags = (p->regs.flags & 0xFF00) | (vxt_word)(p->regs.ah & ALL_FLAGS) | 2;
}

static void lahf_9F(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ah = (vxt_byte)(p->regs.flags & 0xFF) | 2;
}

static void mov_A0(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = vxt_system_read_byte(p->s, POINTER(p->seg, read_opcode16(p)));
}

static void mov_A1(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = vxt_system_read_word(p->s, POINTER(p->seg, read_opcode16(p)));
}

static void mov_A2(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_system_write_byte(p->s, POINTER(p->seg, read_opcode16(p)), p->regs.al);
}

static void mov_A3(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_system_write_word(p->s, POINTER(p->seg, read_opcode16(p)), p->regs.ax);
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

static void ret_C2(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word ip = pop(p);
   p->regs.sp += read_opcode16(p);
   p->regs.ip = ip;
}

static void ret_C3(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip = pop(p);
}

#define LOAD(name, r)                                                         \
   static void name (CONSTSP(cpu) p, INST(inst)) {                            \
      UNUSED(inst);                                                           \
      vxt_pointer ea = get_effective_address(p);                              \
      reg_write16(&p->regs, p->mode.reg, vxt_system_read_word(p->s, ea));     \
      p->regs.r = vxt_system_read_word(p->s, ea + 2);                         \
   }                                                                          \

LOAD(les_C4, es)
LOAD(lds_C5, ds)
#undef LOAD

static void mov_C6(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest8(p, read_opcode8(p));
}

static void mov_C7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, read_opcode16(p));
}

static void retf_CA(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word sp = read_opcode16(p);
   p->regs.ip = pop(p);
   p->regs.cs = pop(p);
   p->regs.sp += sp;
}

static void retf_CB(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip = pop(p);
   p->regs.cs = pop(p);
}

static void int_CC(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   IRQ(p, 3);
}

static void int_CD(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   IRQ(p, (int)read_opcode8(p));
}

static void int_CE(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   if (p->regs.flags & VXT_OVERFLOW) {
      p->cycles += 69;
      IRQ(p, 4);
   }  
}

static void iret_CF(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip = pop(p);
   p->regs.cs = pop(p);
   p->regs.flags = pop(p);
}

static void grp2_D0(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest8(p, bitshift_8(p, read_source8(p), 1));
}

static void grp2_D1(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, bitshift_16(p, read_source16(p), 1));
}

static void grp2_D2(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest8(p, bitshift_8(p, read_source8(p), p->regs.cl));
}

static void grp2_D3(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   write_dest16(p, bitshift_16(p, read_source16(p), p->regs.cl));
}

static void aam_D4(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte a = p->regs.al;
   vxt_byte b = read_opcode8(p);
   if (!b) {
      IRQ(p, 0);
      return;
   } else {
      p->regs.al = a % b;
      p->regs.ah = a / b;
      flag_szp16(&p->regs, p->regs.ax);
   }
}

static void aad_D5(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = ((vxt_word)p->regs.ah * (vxt_word)read_opcode8(p) + (vxt_word)p->regs.al) & 0xFF;
   flag_szp16(&p->regs, p->regs.ax);
   SET_FLAG(p->regs.flags, VXT_ZERO, 0);
}

static void xlat_D7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = vxt_system_read_byte(p->s, POINTER(p->seg, p->regs.bx + p->regs.al));
}

static void fpu_dummy(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p); UNUSED(inst);
}

static void in_E4(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = (vxt_byte)system_in(p->s, read_opcode8(p));
}

static void in_E5(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = system_in(p->s, read_opcode8(p));
}

static void out_E6(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, read_opcode8(p), p->regs.al);
}

static void out_E7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, read_opcode8(p), p->regs.ax);
}

static void call_E8(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word offset = read_opcode16(p);
   push(p, p->regs.ip);
   p->regs.ip += offset;
}

static void jmp_E9(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip += read_opcode16(p);
}

static void jmp_EA(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word ip = read_opcode16(p);
   p->regs.cs = read_opcode16(p);
   p->regs.ip = ip;
}

static void jmp_EB(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ip += SIGNEXT16(read_opcode8(p));
}

static void in_EC(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.al = system_in(p->s, p->regs.dx);
}

static void in_ED(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->regs.ax = (system_in(p->s, p->regs.dx + 1) << 8) | system_in(p->s, p->regs.dx);
}

static void out_EE(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, p->regs.dx, p->regs.al);
}

static void out_EF(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   system_out(p->s, p->regs.dx, p->regs.ax);
}

static void lock_F0(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(p); UNUSED(inst);
}

static void hlt_F4(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   p->halt = p->regs.debug = true;
}

static void cmc_F5(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   SET_FLAG_IF(p->regs.flags, VXT_CARRY, !FLAGS(p->regs.flags, VXT_CARRY));
}

static void grp3_F6(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_byte v = read_dest8(p);
   switch (p->mode.reg) {
      case 0: // TEST Eb Ib
      case 1:
         flag_logic8(&p->regs, v & read_opcode8(p));
         break;
      case 2: // NOT
         write_dest8(p, ~v);
         break;
      case 3: // NEG
      {
         vxt_byte res = ~v + 1;
         flag_sub_sbb8(&p->regs, 0, v, 0);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY, res);
         write_dest8(p, res);
         break;
      }
      case 4: // MUL
      {
         vxt_dword res = (vxt_dword)v * (vxt_dword)p->regs.al;
         p->regs.ax = (vxt_word)(res & 0xFFFF);
         flag_szp8(&p->regs, (vxt_byte)res);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, p->regs.ah);
         p->regs.flags &= ~VXT_ZERO; // 8088 always clears zero flag.
         break;
      }
      case 5: // IMUL
         p->regs.ax = SIGNEXT16(v) * SIGNEXT16(p->regs.al);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, p->regs.ah);
         p->regs.flags &= ~VXT_ZERO; // 8088 always clears zero flag.
         break;
      case 6: // DIV
      {
         vxt_word ax = p->regs.ax;
         if (!v || (ax / (vxt_word)v) > 0xFF) {
            IRQ(p, 0);
            return;
         }

         p->regs.ah = (vxt_byte)(ax % (vxt_word)v);
         p->regs.al = (vxt_byte)(ax / (vxt_word)v);
         break;
      }
      case 7: // IDIV - reference: fake86's - cpu.c
      {
         if (!v) {
            IRQ(p, 0);
            return;
         }

         vxt_word a = p->regs.ax;
         vxt_word d = SIGNEXT16(v);
	      bool sign = ((a ^ d) & 0x8000) != 0;

         if (a >= 0x8000)
            a = ~a + 1;
         if (d >= 0x8000)
            d = ~d + 1;

         vxt_word res1 = a / d; 
         vxt_word res2 = a % d;
         if ((res1 & 0xFF00) != 0) {
            IRQ(p, 0);
            return;
         }

         if (sign) {
            res1 = (~res1 + 1) & 0xFF;
            res2 = (~res2 + 1) & 0xFF;
         }

         p->regs.al = (vxt_byte)res1;
         p->regs.ah = (vxt_byte)res2;
         break;
      }
   }
}

static void grp3_F7(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word v = read_dest16(p);
   switch (p->mode.reg) {
      case 0: // TEST Ev Iv
      case 1:
         flag_logic16(&p->regs, v & read_opcode16(p));
         break;
      case 2: // NOT
         write_dest16(p, ~v);
         break;
      case 3: // NEG
      {
         vxt_word res = ~v + 1;
         flag_sub_sbb16(&p->regs, 0, v, 0);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY, res);
         write_dest16(p, res);
         break;
      }
      case 4: // MUL
      {
         vxt_dword res = (vxt_dword)v * (vxt_dword)p->regs.ax;
         p->regs.dx = (vxt_word)(res >> 16);
         p->regs.ax = (vxt_word)(res & 0xFFFF);
         flag_szp16(&p->regs, (vxt_byte)res);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, p->regs.dx);
         p->regs.flags &= ~VXT_ZERO; // 8088 always clears zero flag.
         break;
      }
      case 5: // IMUL
      {
         vxt_dword res = SIGNEXT32(v) * SIGNEXT32(p->regs.ax);
         p->regs.ax = (vxt_word)(res & 0xFFFF);
         p->regs.dx = (vxt_word)(res >> 16);
         SET_FLAG_IF(p->regs.flags, VXT_CARRY|VXT_OVERFLOW, p->regs.dx);
         p->regs.flags &= ~VXT_ZERO; // 8088 always clears zero flag.
         break;
      }
      case 6: // DIV
      {
         vxt_dword a = (p->regs.dx << 16) + p->regs.ax;
         if (!v || (a / (vxt_dword)v) > 0xFFFF) {
            IRQ(p, 0);
            return;
         }

         p->regs.dx = (vxt_word)(a % (vxt_dword)v);
         p->regs.ax = (vxt_word)(a / (vxt_dword)v);
         break;
      }
      case 7: // IDIV - reference: fake86's - cpu.c
      {
         if (!v) {
            IRQ(p, 0);
            return;
         }

         vxt_dword a = (p->regs.dx << 16) + p->regs.ax;
         vxt_dword d = SIGNEXT32(v);
	      bool sign = ((a ^ d) & 0x80000000) != 0;

         if (a >= 0x80000000)
            a = ~a + 1;
         if (d >= 0x80000000)
            d = ~d + 1;

         vxt_dword res1 = a / d; 
         vxt_dword res2 = a % d;
         if ((res1 & 0xFFFF0000) != 0) {
            IRQ(p, 0);
            return;
         }

         if (sign) {
            res1 = (~res1 + 1) & 0xFFFF;
            res2 = (~res2 + 1) & 0xFFFF;
         }

         p->regs.ax = (vxt_word)res1;
         p->regs.dx = (vxt_word)res2;
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

   vxt_byte v = read_dest8(p);
   vxt_word c = p->regs.flags & VXT_CARRY;
   switch (p->mode.reg) {
      case 0: // INC
         write_dest8(p, op_add_adc8(&p->regs, v, 1, 0));
         break;
      case 1: // DEC
         write_dest8(p, op_sub_sbb8(&p->regs, v, 1, 0));
         break;
      default:
         LOG("TODO: Invalid opcode!");
   }
   SET_FLAG(p->regs.flags, VXT_CARRY, c);
}

static void grp5_FF(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);

   vxt_word v = read_dest16(p);
   vxt_word c = p->regs.flags & VXT_CARRY;
   switch (p->mode.reg) {
      case 0: // INC
         write_dest16(p, op_add_adc16(&p->regs, v, 1, 0));
         SET_FLAG(p->regs.flags, VXT_CARRY, c);
         break;
      case 1: // DEC
         write_dest16(p, op_sub_sbb16(&p->regs, v, 1, 0));
         SET_FLAG(p->regs.flags, VXT_CARRY, c);
         break;
      case 2: // CALL
         push(p, p->regs.ip);
         p->regs.ip = v;
         break;
      case 3: // CALL Mp
      {
         push(p, p->regs.cs);
         push(p, p->regs.ip);

         vxt_pointer ea = get_effective_address(p);
         p->regs.ip = vxt_system_read_word(p->s, ea);
         p->regs.cs = vxt_system_read_word(p->s, ea + 2);
         break;
      }
      case 4: // JMP
         p->regs.ip = v;
         break;
      case 5: // JMP Mp
      {
         vxt_pointer ea = get_effective_address(p);
         p->regs.ip = vxt_system_read_word(p->s, ea);
         p->regs.cs = vxt_system_read_word(p->s, ea + 2);
         break;
      }
      case 6: // PUSH
      case 7:
         push(p, v);
         break;
      default:
         UNREACHABLE();
   }
}

#define X 1
#define INVALID "INVALID", false, X, &invalid_op

// References: http://mlsite.net/8086/
//             http://aturing.umcs.maine.edu/~meadow/courses/cos335/80x86-Integer-Instruction-Set-Clocks.pdf

static struct instruction const opcode_table[0x100] = {
   {0x0, "ADD Eb Gb", true, X, &add_0_2_10_12},
   {0x1, "ADD Ev Gv", true, X, &add_1_3_11_13},
   {0x2, "ADD Gb Eb", true, X, &add_0_2_10_12},
   {0x3, "ADD Gv Ev", true, X, &add_1_3_11_13},
   {0x4, "ADD AL Ib", false, 4, &add_4_14},
   {0x5, "ADD AX Iv", false, 4, &add_5_15},
   {0x6, "PUSH ES", false, 10, &push_es},
   {0x7, "POP ES", false, 8, &pop_es},
   {0x8, "OR Eb Gb", true, X, &or_8_A},
   {0x9, "OR Ev Gv", true, X, &or_9_B},
   {0xA, "OR Gb Eb", true, X, &or_8_A},
   {0xB, "OR Gv Ev", true, X, &or_9_B},
   {0xC, "OR AL Ib", false, 4, &or_C},
   {0xD, "OR AX Iv", false, 4, &or_D},
   {0xE, "PUSH CS", false, 10, &push_es},
   {0xF, INVALID},
   {0x10, "ADC Eb Gb", true, X, &add_0_2_10_12},
   {0x11, "ADC Ev Gv", true, X, &add_1_3_11_13},
   {0x12, "ADC Gb Eb", true, X, &add_0_2_10_12},
   {0x13, "ADC Gv Ev", true, X, &add_1_3_11_13},
   {0x14, "ADC AL Ib", false, 4, &add_4_14},
   {0x15, "ADC AX Iv", false, 4, &add_5_15},
   {0x16, "PUSH SS", false, 10, &push_ss},
   {0x17, "POP SS", false, 8, &pop_ss},
   {0x18, "SBB Eb Gb", true, X, &sub_18_1A_28_2A},
   {0x19, "SBB Ev Gv", true, X, &sub_19_1B_29_2B},
   {0x1A, "SBB Gb Eb", true, X, &sub_18_1A_28_2A},
   {0x1B, "SBB Gv Ev", true, X, &sub_19_1B_29_2B},
   {0x1C, "SBB AL Ib", false, 4, &sub_1C_2C},
   {0x1D, "SBB AX Iv", false, 4, &sub_1D_2D},
   {0x1E, "PUSH DS", false, 10, &push_ds},
   {0x1F, "POP DS", false, 8, &pop_ds},
   {0x20, "AND Eb Gb", true, X, &and_20_22},
   {0x21, "AND Ev Gv", true, X, &and_21_23},
   {0x22, "AND Gb Eb", true, X, &and_20_22},
   {0x23, "AND Gv Ev", true, X, &and_21_23},
   {0x24, "AND AL Ib", false, 4, &and_24},
   {0x25, "AND AX Iv", false, 4, &and_25},
   {0x26, "PREFIX", false, X, &invalid_prefix},
   {0x27, "DAA", false, 4, daa_27},
   {0x28, "SUB Eb Gb", true, X, &sub_18_1A_28_2A},
   {0x29, "SUB Ev Gv", true, X, &sub_19_1B_29_2B},
   {0x2A, "SUB Gb Eb", true, X, &sub_18_1A_28_2A},
   {0x2B, "SUB Gv Ev", true, X, &sub_19_1B_29_2B},
   {0x2C, "SUB AL Ib", false, 4, &sub_1C_2C},
   {0x2D, "SUB AX Iv", false, 4, &sub_1D_2D},
   {0x2E, "PREFIX", false, X, &invalid_prefix},
   {0x2F, "DAS", false, 4, das_2F},
   {0x30, "XOR Eb Gb", true, X, &xor_30_32},
   {0x31, "XOR Ev Gv", true, X, &xor_31_33},
   {0x32, "XOR Gb Eb", true, X, &xor_30_32},
   {0x33, "XOR Gv Ev", true, X, &xor_31_33},
   {0x34, "XOR AL Ib", false, 4, &xor_34},
   {0x35, "XOR AX Iv", false, 4, &xor_35},
   {0x36, "PREFIX", false, X, &invalid_prefix},
   {0x37, "AAA", false, 8, aaa_37},
   {0x38, "CMP Eb Gb", true, X, cmp_38_3A},
   {0x39, "CMP Ev Gv", true, X, cmp_39_3B},
   {0x3A, "CMP Gb Eb", true, X, cmp_38_3A},
   {0x3B, "CMP Gv Ev", true, X, cmp_39_3B},
   {0x3C, "CMP AL Ib", false, 4, &cmp_3C},
   {0x3D, "CMP AX Iv", false, 4, &cmp_3D},
   {0x3E, "PREFIX", false, X, &invalid_prefix},
   {0x3F, "AAS", false, 8, aas_3F},
   {0x40, "INC AX", false, 2, &inc_reg},
   {0x41, "INC CX", false, 2, &inc_reg},
   {0x42, "INC DX", false, 2, &inc_reg},
   {0x43, "INC BX", false, 2, &inc_reg},
   {0x44, "INC SP", false, 2, &inc_reg},
   {0x45, "INC BP", false, 2, &inc_reg},
   {0x46, "INC SI", false, 2, &inc_reg},
   {0x47, "INC DI", false, 2, &inc_reg},
   {0x48, "DEC AX", false, 2, &dec_reg},
   {0x49, "DEC CX", false, 2, &dec_reg},
   {0x4A, "DEC DX", false, 2, &dec_reg},
   {0x4B, "DEC BX", false, 2, &dec_reg},
   {0x4C, "DEC SP", false, 2, &dec_reg},
   {0x4D, "DEC BP", false, 2, &dec_reg},
   {0x4E, "DEC SI", false, 2, &dec_reg},
   {0x4F, "DEC DI", false, 2, &dec_reg},
   {0x50, "PUSH AX", false, 11, &push_ax},
   {0x51, "PUSH CX", false, 11, &push_cx},
   {0x52, "PUSH DX", false, 11, &push_dx},
   {0x53, "PUSH BX", false, 11, &push_bx},
   {0x54, "PUSH SP", false, 11, &push_sp},
   {0x55, "PUSH BP", false, 11, &push_bp},
   {0x56, "PUSH SI", false, 11, &push_si},
   {0x57, "PUSH DI", false, 11, &push_di},
   {0x58, "POP AX", false, 8, &pop_ax},
   {0x59, "POP CX", false, 8, &pop_cx},
   {0x5A, "POP DX", false, 8, &pop_dx},
   {0x5B, "POP BX", false, 8, &pop_bx},
   {0x5C, "POP SP", false, 8, &pop_sp},
   {0x5D, "POP BP", false, 8, &pop_bp},
   {0x5E, "POP SI", false, 8, &pop_si},
   {0x5F, "POP DI", false, 8, &pop_di},
   {0x60, INVALID},
   {0x61, INVALID},
   {0x62, INVALID},
   {0x63, INVALID},
   {0x64, INVALID},
   {0x65, INVALID},
   {0x66, INVALID},
   {0x67, INVALID},
   {0x68, INVALID},
   {0x69, INVALID},
   {0x6A, INVALID},
   {0x6B, INVALID},
   {0x6C, INVALID},
   {0x6D, INVALID},
   {0x6E, INVALID},
   {0x6F, INVALID},
   {0x70, "JO Jb", false, 4, &jump_jo},
   {0x71, "JNO Jb", false, 4, &jump_jno},
   {0x72, "JB Jb", false, 4, &jump_jb},
   {0x73, "JNB Jb", false, 4, &jump_jnb},
   {0x74, "JZ Jb", false, 4, &jump_jz},
   {0x75, "JNZ Jb", false, 4, &jump_jnz},
   {0x76, "JBE Jb", false, 4, &jump_jbe},
   {0x77, "JA Jb", false, 4, &jump_ja},
   {0x78, "JS Jb", false, 4, &jump_js},
   {0x79, "JNS Jb", false, 4, &jump_jns},
   {0x7A, "JPE Jb", false, 4, &jump_jpe},
   {0x7B, "JPO Jb", false, 4, &jump_jpo},
   {0x7C, "JL Jb", false, 4, &jump_jl},
   {0x7D, "JGE Jb", false, 4, &jump_jge},
   {0x7E, "JLE Jb", false, 4, &jump_jle},
   {0x7F, "JG Jb", false, 4, &jump_jg},
   {0x80, "GRP1 Eb Ib", true, X, &grp1_80_82},
   {0x81, "GRP1 Ev Iv", true, X, &grp1_81_83},
   {0x82, "GRP1 Eb Ib", true, X, &grp1_80_82},
   {0x83, "GRP1 Ev Ib", true, X, &grp1_81_83},
   {0x84, "TEST Gb Eb", true, X, &test_84},
   {0x85, "TEST Gv Ev", true, X, &test_85},
   {0x86, "XCHG Gb Eb", true, X, &xchg_86},
   {0x87, "XCHG Gv Ev", true, X, &xchg_87},
   {0x88, "MOV Eb Gb", true, X, &mov_88_8A},
   {0x89, "MOV Ev Gv", true, X, &mov_89_8B},
   {0x8A, "MOV Gb Eb", true, X, &mov_88_8A},
   {0x8B, "MOV Gv Ev", true, X, &mov_89_8B},
   {0x8C, "MOV Ew Sw", true, X, &mov_8C},
   {0x8D, "LEA Gv M", true, 2, &lea_8D},
   {0x8E, "MOV Sw Ew", true, X, &mov_8E},
   {0x8F, "POP Ev", true, X, &pop_8F},
   {0x90, "NOP", false, 3, &nop_90},
   {0x91, "XCHG CX AX", false, 3, &xchg_cx_ax},
   {0x92, "XCHG DX AX", false, 3, &xchg_dx_ax},
   {0x93, "XCHG BX AX", false, 3, &xchg_bx_ax},
   {0x94, "XCHG SP AX", false, 3, &xchg_sp_ax},
   {0x95, "XCHG BP AX", false, 3, &xchg_bp_ax},
   {0x96, "XCHG SI AX", false, 3, &xchg_si_ax},
   {0x97, "XCHG DI AX", false, 3, &xchg_di_ax},
   {0x98, "CBW", false, 2, &cbw_98},
   {0x99, "CWD", false, 5, &cwd_99},
   {0x9A, "CALL Ap", false, 28, &call_9A},
   {0x9B, "WAIT", false, 4, &wait_9B},
   {0x9C, "PUSHF", false, 10, &pushf_9C},
   {0x9D, "POPF", false, 8, &popf_9D},
   {0x9E, "SAHF", false, 4, &sahf_9E},
   {0x9F, "LAHF", false, 4, &lahf_9F},
   {0xA0, "MOV AL Ob", false, 4, &mov_A0},
   {0xA1, "MOV AX Ov", false, 4, &mov_A1},
   {0xA2, "MOV Ob AL", false, 4, &mov_A2},
   {0xA3, "MOV Ov AX", false, 4, &mov_A3},
   {0xA4, "MOVSB", false, X, &movsb_A4},
   {0xA5, "MOVSW", false, X, &movsw_A5},
   {0xA6, "CMPSB", false, X, &cmpsb_A6},
   {0xA7, "CMPSW", false, X, &cmpsw_A7},
   {0xA8, "TEST AL Ib", false, 4, &test_A8},
   {0xA9, "TEST AX Iv", false, 4, &test_A9},
   {0xAA, "STOSB", false, X, &stosb_AA},
   {0xAB, "STOSW", false, X, &stosw_AB},
   {0xAC, "LODSB", false, 16, &lodsb_AC},
   {0xAD, "LODSW", false, 16, &lodsw_AD},
   {0xAE, "SCASB", false, X, &scasb_AE},
   {0xAF, "SCASW", false, X, &scasw_AF},
   {0xB0, "MOV AL Ib", false, 4, &mov_reg8},
   {0xB1, "MOV CL Ib", false, 4, &mov_reg8},
   {0xB2, "MOV DL Ib", false, 4, &mov_reg8},
   {0xB3, "MOV BL Ib", false, 4, &mov_reg8},
   {0xB4, "MOV AH Ib", false, 4, &mov_reg8},
   {0xB5, "MOV CH Ib", false, 4, &mov_reg8},
   {0xB6, "MOV DH Ib", false, 4, &mov_reg8},
   {0xB7, "MOV BH Ib", false, 4, &mov_reg8},
   {0xB8, "MOV AX Iv", false, 4, &mov_reg16},
   {0xB9, "MOV CX Iv", false, 4, &mov_reg16},
   {0xBA, "MOV DX Iv", false, 4, &mov_reg16},
   {0xBB, "MOV BX Iv", false, 4, &mov_reg16},
   {0xBC, "MOV SP Iv", false, 4, &mov_reg16},
   {0xBD, "MOV BP Iv", false, 4, &mov_reg16},
   {0xBE, "MOV SI Iv", false, 4, &mov_reg16},
   {0xBF, "MOV DI Iv", false, 4, &mov_reg16},
   {0xC0, INVALID},
   {0xC1, INVALID},
   {0xC2, "RET Iw", false, 24, &ret_C2},
   {0xC3, "RET", false, 20, &ret_C3},
   {0xC4, "LES Gv Mp", true, 24, &les_C4},
   {0xC5, "LDS Gv Mp", true, 24, &lds_C5},
   {0xC6, "MOV Eb Ib", true, X, &mov_C6},
   {0xC7, "MOV Ev Iv", true, X, &mov_C7},
   {0xC8, INVALID},
   {0xC9, INVALID},
   {0xCA, "RETF Iw", false, 33, &retf_CA},
   {0xCB, "RETF", false, 34, &retf_CB},
   {0xCC, "INT 3", false, 72, &int_CC},
   {0xCD, "INT Ib", false, 71, &int_CD},
   {0xCE, "INTO", false, 4, &int_CE},
   {0xCF, "IRET", false, 44, &iret_CF},
   {0xD0, "GRP2 Eb 1", true, X, &grp2_D0},
   {0xD1, "GRP2 Ev 1", true, X, &grp2_D1},
   {0xD2, "GRP2 Eb CL", true, X, &grp2_D2},
   {0xD3, "GRP2 Ev CL", true, X, &grp2_D3},
   {0xD4, "AAM I0", false, 83, &aam_D4},
   {0xD5, "AAD I0", false, 60, &aad_D5},
   {0xD6, INVALID},
   {0xD7, "XLAT", false, 11, &xlat_D7},
   {0xD8, "ESC", true, X, &fpu_dummy},
   {0xD9, "ESC", true, X, &fpu_dummy},
   {0xDA, "ESC", true, X, &fpu_dummy},
   {0xDB, "ESC", true, X, &fpu_dummy},
   {0xDC, "ESC", true, X, &fpu_dummy},
   {0xDD, "ESC", true, X, &fpu_dummy},
   {0xDE, "ESC", true, X, &fpu_dummy},
   {0xDF, "ESC", true, X, &fpu_dummy},
   {0xE0, "LOOPNZ Jb", false, 0, &loopnz_E0},
   {0xE1, "LOOPZ Jb", false, 0, &loopz_E1},
   {0xE2, "LOOP Jb", false, 0, &loop_E2},
   {0xE3, "JCXZ Jb", false, 6, &jcxz_E3},
   {0xE4, "IN AL Ib", false, 14, &in_E4},
   {0xE5, "IN AX Ib", false, 14, &in_E5},
   {0xE6, "OUT Ib AL", false, 14, &out_E6},
   {0xE7, "OUT Ib AX", false, 14, &out_E7},
   {0xE8, "CALL Jv", false, X, &call_E8},
   {0xE9, "JMP Jv", false, 15, &jmp_E9},
   {0xEA, "JMP Ap", false, 15, &jmp_EA},
   {0xEB, "JMP Jb", false, 15, &jmp_EB},
   {0xEC, "IN AL DX", false, 12, &in_EC},
   {0xED, "IN AX DX", false, 12, &in_ED},
   {0xEE, "OUT DX AL", false, 12, &out_EE},
   {0xEF, "OUT DX AX", false, 12, &out_EF},
   {0xF0, "LOCK", false, 2, &lock_F0},
   {0xF1, INVALID},
   {0xF2, "PREFIX", false, X, &invalid_prefix},
   {0xF3, "PREFIX", false, X, &invalid_prefix},
   {0xF4, "HLT", false, 2, &hlt_F4},
   {0xF5, "CMC", false, 2, &cmc_F5},
   {0xF6, "GRP3a Eb", true, X, &grp3_F6},
   {0xF7, "GRP3b Ev", true, X, &grp3_F7},
   {0xF8, "CLC", false, 2, &clc_F8},
   {0xF9, "STC", false, 2, &stc_F9},
   {0xFA, "CLI", false, 2, &cli_FA},
   {0xFB, "STI", false, 2, &sti_FB},
   {0xFC, "CLD", false, 2, &cld_FC},
   {0xFD, "STD", false, 2, &std_FD},
   {0xFE, "GRP4 Eb", true, 23, &grp4_FE},
   {0xFF, "GRP5 Ev", true, X, &grp5_FF}
};

#undef INVALID
#undef X
