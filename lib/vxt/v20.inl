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
#include "exec.h"
#include "rep.h"

#define X 1
#define INVALID "INVALID", false, X, &invalid_op

static struct instruction const opcode_table[0x100] = {
   {0x0, "ADD Eb Gb", true, X, &add_0_10},
   {0x1, "ADD Ev Gv", true, X, &add_1_11},
   {0x2, "ADD Gb Eb", true, X, &add_2_12},
   {0x3, "ADD Gv Ev", true, X, &add_3_13},
   {0x4, "ADD AL Ib", false, 4, &add_4_14},
   {0x5, "ADD AX Iv", false, 4, &add_5_15},
   {0x6, "PUSH ES", false, 10, &push_es},
   {0x7, "POP ES", false, 8, &pop_es},
   {0x8, "OR Eb Gb", true, X, &or_8},
   {0x9, "OR Ev Gv", true, X, &or_9},
   {0xA, "OR Gb Eb", true, X, &or_A},
   {0xB, "OR Gv Ev", true, X, &or_B},
   {0xC, "OR AL Ib", false, 4, &or_C},
   {0xD, "OR AX Iv", false, 4, &or_D},
   {0xE, "PUSH CS", false, 10, &push_cs},
   {0xF, INVALID},
   {0x10, "ADC Eb Gb", true, X, &add_0_10},
   {0x11, "ADC Ev Gv", true, X, &add_1_11},
   {0x12, "ADC Gb Eb", true, X, &add_2_12},
   {0x13, "ADC Gv Ev", true, X, &add_3_13},
   {0x14, "ADC AL Ib", false, 4, &add_4_14},
   {0x15, "ADC AX Iv", false, 4, &add_5_15},
   {0x16, "PUSH SS", false, 10, &push_ss},
   {0x17, "POP SS", false, 8, &pop_ss},
   {0x18, "SBB Eb Gb", true, X, &sub_18_28},
   {0x19, "SBB Ev Gv", true, X, &sub_19_29},
   {0x1A, "SBB Gb Eb", true, X, &sub_1A_2A},
   {0x1B, "SBB Gv Ev", true, X, &sub_1B_2B},
   {0x1C, "SBB AL Ib", false, 4, &sub_1C_2C},
   {0x1D, "SBB AX Iv", false, 4, &sub_1D_2D},
   {0x1E, "PUSH DS", false, 10, &push_ds},
   {0x1F, "POP DS", false, 8, &pop_ds},
   {0x20, "AND Eb Gb", true, X, &and_20},
   {0x21, "AND Ev Gv", true, X, &and_21},
   {0x22, "AND Gb Eb", true, X, &and_22},
   {0x23, "AND Gv Ev", true, X, &and_23},
   {0x24, "AND AL Ib", false, 4, &and_24},
   {0x25, "AND AX Iv", false, 4, &and_25},
   {0x26, "PREFIX", false, X, &invalid_prefix},
   {0x27, "DAA", false, 4, daa_27},
   {0x28, "SUB Eb Gb", true, X, &sub_18_28},
   {0x29, "SUB Ev Gv", true, X, &sub_19_29},
   {0x2A, "SUB Gb Eb", true, X, &sub_1A_2A},
   {0x2B, "SUB Gv Ev", true, X, &sub_1B_2B},
   {0x2C, "SUB AL Ib", false, 4, &sub_1C_2C},
   {0x2D, "SUB AX Iv", false, 4, &sub_1D_2D},
   {0x2E, "PREFIX", false, X, &invalid_prefix},
   {0x2F, "DAS", false, 4, das_2F},
   {0x30, "XOR Eb Gb", true, X, &xor_30},
   {0x31, "XOR Ev Gv", true, X, &xor_31},
   {0x32, "XOR Gb Eb", true, X, &xor_32},
   {0x33, "XOR Gv Ev", true, X, &xor_33},
   {0x34, "XOR AL Ib", false, 4, &xor_34},
   {0x35, "XOR AX Iv", false, 4, &xor_35},
   {0x36, "PREFIX", false, X, &invalid_prefix},
   {0x37, "AAA", false, 8, aaa_37},
   {0x38, "CMP Eb Gb", true, X, cmp_38},
   {0x39, "CMP Ev Gv", true, X, cmp_39},
   {0x3A, "CMP Gb Eb", true, X, cmp_3A},
   {0x3B, "CMP Gv Ev", true, X, cmp_3B},
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
   {0x88, "MOV Eb Gb", true, X, &mov_88},
   {0x89, "MOV Ev Gv", true, X, &mov_89},
   {0x8A, "MOV Gb Eb", true, X, &mov_8A},
   {0x8B, "MOV Gv Ev", true, X, &mov_8B},
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
   {0xD6, "SALC", false, X, &salc_D6},
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
