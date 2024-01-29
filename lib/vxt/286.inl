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

static void ext_286_F(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	VALIDATOR_DISCARD(p);

	switch (read_opcode8(p)) {
		case 0:
			read_modregrm(p);
			switch (p->mode.reg) {
				case 0: // SLDT - Store Local Descriptor Table Register
					VXT_LOG("SLDT - Store Local Descriptor Table Register");
					return;
				case 1: // STR - Store Task Register
					VXT_LOG("STR - Store Task Register");
					return;
				case 2: // LDTR - Load Local Descriptor Table Register
					VXT_LOG("LDTR - Load Local Descriptor Table Register");
					return;
				case 3: // LTR - Load Task Register
					VXT_LOG("LTR - Load Task Register");
					return;
				case 4: // VERR - Verify a Segment for Reading
					VXT_LOG("VERR - Verify a Segment for Reading");
					return;
				case 5: // VERW - Verify a Segment for Writing
					VXT_LOG("VERW - Verify a Segment for Writing");
					return;
			}
			return;
		case 1:
			read_modregrm(p);
			switch (p->mode.reg) {
				case 0: // SGDT - Store Global Descriptor Table Register
					VXT_LOG("SGDT - Store Global Descriptor Table Register");
					return;
				case 1: // SIDT - Store Interrupt Descriptor Table Register
					VXT_LOG("SIDT - Store Interrupt Descriptor Table Register");
					return;
				case 2: // LGDT - Load Global Descriptor Table Register
					VXT_LOG("LGDT - Load Global Descriptor Table Register");
					return;
				case 3: // LIDT - Load Interrupt Descriptor Table Register
					VXT_LOG("LIDT - Load Interrupt Descriptor Table Register");
					return;
				case 4: // SMSW - Store Machine Status Word
					VXT_LOG("SMSW - Store Machine Status Word");
					return;
				case 6: // LMSW - Load Machine Status Word
					VXT_LOG("LMSW - Load Machine Status Word");
					return;
			}
			return;
		case 2: // LAR - Load Access Rights Byte
			VXT_LOG("LAR - Load Access Rights Byte");
			read_modregrm(p);
			return;
		case 3: // LSL - Load Segment Limit
			VXT_LOG("LSL - Load Segment Limit");
			read_modregrm(p);
			return;
		case 5: // LOADALL - Load All of the CPU Registers
			VXT_LOG("LOADALL - Load All of the CPU Registers");
			return;
		case 6: // CLTS - Clear Task-Switched Flag in CR0
			VXT_LOG("CLTS - Clear Task-Switched Flag in CR0");
			return;
	}

	p->regs.ip = p->inst_start;
	call_int(p, 6); 
}

#define X 1
#define INVALID "INVALID", false, X, &invalid_op

static struct instruction const opcode_table_286[0x100] = {
   {0x0, "ADD Eb Gb", true, X, &add_0_10},
   {0x1, "ADD Ev Gv", true, X, &add_1_11},
   {0x2, "ADD Gb Eb", true, X, &add_2_12},
   {0x3, "ADD Gv Ev", true, X, &add_3_13},
   {0x4, "ADD AL Ib", false, X, &add_4_14},
   {0x5, "ADD AX Iv", false, X, &add_5_15},
   {0x6, "PUSH ES", false, X, &push_es},
   {0x7, "POP ES", false, X, &pop_es},
   {0x8, "OR Eb Gb", true, X, &or_8},
   {0x9, "OR Ev Gv", true, X, &or_9},
   {0xA, "OR Gb Eb", true, X, &or_A},
   {0xB, "OR Gv Ev", true, X, &or_B},
   {0xC, "OR AL Ib", false, X, &or_C},
   {0xD, "OR AX Iv", false, X, &or_D},
   {0xE, "PUSH CS", false, X, &push_cs},
   {0xF, "EXTENDED 0xF", false, X, &ext_286_F},
   {0x10, "ADC Eb Gb", true, X, &add_0_10},
   {0x11, "ADC Ev Gv", true, X, &add_1_11},
   {0x12, "ADC Gb Eb", true, X, &add_2_12},
   {0x13, "ADC Gv Ev", true, X, &add_3_13},
   {0x14, "ADC AL Ib", false, X, &add_4_14},
   {0x15, "ADC AX Iv", false, X, &add_5_15},
   {0x16, "PUSH SS", false, X, &push_ss},
   {0x17, "POP SS", false, X, &pop_ss},
   {0x18, "SBB Eb Gb", true, X, &sub_18_28},
   {0x19, "SBB Ev Gv", true, X, &sub_19_29},
   {0x1A, "SBB Gb Eb", true, X, &sub_1A_2A},
   {0x1B, "SBB Gv Ev", true, X, &sub_1B_2B},
   {0x1C, "SBB AL Ib", false, X, &sub_1C_2C},
   {0x1D, "SBB AX Iv", false, X, &sub_1D_2D},
   {0x1E, "PUSH DS", false, X, &push_ds},
   {0x1F, "POP DS", false, X, &pop_ds},
   {0x20, "AND Eb Gb", true, X, &and_20},
   {0x21, "AND Ev Gv", true, X, &and_21},
   {0x22, "AND Gb Eb", true, X, &and_22},
   {0x23, "AND Gv Ev", true, X, &and_23},
   {0x24, "AND AL Ib", false, X, &and_24},
   {0x25, "AND AX Iv", false, X, &and_25},
   {0x26, "PREFIX", false, X, &invalid_prefix},
   {0x27, "DAA", false, X, daa_27},
   {0x28, "SUB Eb Gb", true, X, &sub_18_28},
   {0x29, "SUB Ev Gv", true, X, &sub_19_29},
   {0x2A, "SUB Gb Eb", true, X, &sub_1A_2A},
   {0x2B, "SUB Gv Ev", true, X, &sub_1B_2B},
   {0x2C, "SUB AL Ib", false, X, &sub_1C_2C},
   {0x2D, "SUB AX Iv", false, X, &sub_1D_2D},
   {0x2E, "PREFIX", false, X, &invalid_prefix},
   {0x2F, "DAS", false, X, das_2F},
   {0x30, "XOR Eb Gb", true, X, &xor_30},
   {0x31, "XOR Ev Gv", true, X, &xor_31},
   {0x32, "XOR Gb Eb", true, X, &xor_32},
   {0x33, "XOR Gv Ev", true, X, &xor_33},
   {0x34, "XOR AL Ib", false, X, &xor_34},
   {0x35, "XOR AX Iv", false, X, &xor_35},
   {0x36, "PREFIX", false, X, &invalid_prefix},
   {0x37, "AAA", false, X, aaa_37},
   {0x38, "CMP Eb Gb", true, X, cmp_38},
   {0x39, "CMP Ev Gv", true, X, cmp_39},
   {0x3A, "CMP Gb Eb", true, X, cmp_3A},
   {0x3B, "CMP Gv Ev", true, X, cmp_3B},
   {0x3C, "CMP AL Ib", false, X, &cmp_3C},
   {0x3D, "CMP AX Iv", false, X, &cmp_3D},
   {0x3E, "PREFIX", false, X, &invalid_prefix},
   {0x3F, "AAS", false, X, aas_3F},
   {0x40, "INC AX", false, X, &inc_reg},
   {0x41, "INC CX", false, X, &inc_reg},
   {0x42, "INC DX", false, X, &inc_reg},
   {0x43, "INC BX", false, X, &inc_reg},
   {0x44, "INC SP", false, X, &inc_reg},
   {0x45, "INC BP", false, X, &inc_reg},
   {0x46, "INC SI", false, X, &inc_reg},
   {0x47, "INC DI", false, X, &inc_reg},
   {0x48, "DEC AX", false, X, &dec_reg},
   {0x49, "DEC CX", false, X, &dec_reg},
   {0x4A, "DEC DX", false, X, &dec_reg},
   {0x4B, "DEC BX", false, X, &dec_reg},
   {0x4C, "DEC SP", false, X, &dec_reg},
   {0x4D, "DEC BP", false, X, &dec_reg},
   {0x4E, "DEC SI", false, X, &dec_reg},
   {0x4F, "DEC DI", false, X, &dec_reg},
   {0x50, "PUSH AX", false, X, &push_ax},
   {0x51, "PUSH CX", false, X, &push_cx},
   {0x52, "PUSH DX", false, X, &push_dx},
   {0x53, "PUSH BX", false, X, &push_bx},
   {0x54, "PUSH SP", false, X, &push_sp},
   {0x55, "PUSH BP", false, X, &push_bp},
   {0x56, "PUSH SI", false, X, &push_si},
   {0x57, "PUSH DI", false, X, &push_di},
   {0x58, "POP AX", false, X, &pop_ax},
   {0x59, "POP CX", false, X, &pop_cx},
   {0x5A, "POP DX", false, X, &pop_dx},
   {0x5B, "POP BX", false, X, &pop_bx},
   {0x5C, "POP SP", false, X, &pop_sp},
   {0x5D, "POP BP", false, X, &pop_bp},
   {0x5E, "POP SI", false, X, &pop_si},
   {0x5F, "POP DI", false, X, &pop_di},
   {0x60, "PUSHA", false, X, &pusha_60},
   {0x61, "POPA", false, X, &popa_61},
   {0x62, "BOUND", true, X, &bound_62},
   {0x63, INVALID},
   {0x64, INVALID},
   {0x65, INVALID},
   {0x66, INVALID},
   {0x67, INVALID},
   {0x68, "PUSH Iv", false, X, &push_68},
   {0x69, "IMUL Eb Ib", true, X, &imul_69_6B},
   {0x6A, "PUSH Ib", false, X, &push_6A},
   {0x6B, "IMUL Ew Iv", true, X, &imul_69_6B},
   {0x6C, "INSB", false, X, &insb_6C},
   {0x6D, "INSW", false, X, &insw_6D},
   {0x6E, "OUTSB", false, X, &outsb_6E},
   {0x6F, "OUTSW", false, X, &outsw_6F},
   {0x70, "JO Jb", false, X, &jump_jo},
   {0x71, "JNO Jb", false, X, &jump_jno},
   {0x72, "JB Jb", false, X, &jump_jb},
   {0x73, "JNB Jb", false, X, &jump_jnb},
   {0x74, "JZ Jb", false, X, &jump_jz},
   {0x75, "JNZ Jb", false, X, &jump_jnz},
   {0x76, "JBE Jb", false, X, &jump_jbe},
   {0x77, "JA Jb", false, X, &jump_ja},
   {0x78, "JS Jb", false, X, &jump_js},
   {0x79, "JNS Jb", false, X, &jump_jns},
   {0x7A, "JPE Jb", false, X, &jump_jpe},
   {0x7B, "JPO Jb", false, X, &jump_jpo},
   {0x7C, "JL Jb", false, X, &jump_jl},
   {0x7D, "JGE Jb", false, X, &jump_jge},
   {0x7E, "JLE Jb", false, X, &jump_jle},
   {0x7F, "JG Jb", false, X, &jump_jg},
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
   {0x8D, "LEA Gv M", true, X, &lea_8D},
   {0x8E, "MOV Sw Ew", true, X, &mov_8E},
   {0x8F, "POP Ev", true, X, &pop_8F},
   {0x90, "NOP", false, X, &nop_90},
   {0x91, "XCHG CX AX", false, X, &xchg_cx_ax},
   {0x92, "XCHG DX AX", false, X, &xchg_dx_ax},
   {0x93, "XCHG BX AX", false, X, &xchg_bx_ax},
   {0x94, "XCHG SP AX", false, X, &xchg_sp_ax},
   {0x95, "XCHG BP AX", false, X, &xchg_bp_ax},
   {0x96, "XCHG SI AX", false, X, &xchg_si_ax},
   {0x97, "XCHG DI AX", false, X, &xchg_di_ax},
   {0x98, "CBW", false, X, &cbw_98},
   {0x99, "CWD", false, X, &cwd_99},
   {0x9A, "CALL Ap", false, X, &call_9A},
   {0x9B, "WAIT", false, X, &wait_9B},
   {0x9C, "PUSHF", false, X, &pushf_9C},
   {0x9D, "POPF", false, X, &popf_9D},
   {0x9E, "SAHF", false, X, &sahf_9E},
   {0x9F, "LAHF", false, X, &lahf_9F},
   {0xA0, "MOV AL Ob", false, X, &mov_A0},
   {0xA1, "MOV AX Ov", false, X, &mov_A1},
   {0xA2, "MOV Ob AL", false, X, &mov_A2},
   {0xA3, "MOV Ov AX", false, X, &mov_A3},
   {0xA4, "MOVSB", false, X, &movsb_A4},
   {0xA5, "MOVSW", false, X, &movsw_A5},
   {0xA6, "CMPSB", false, X, &cmpsb_A6},
   {0xA7, "CMPSW", false, X, &cmpsw_A7},
   {0xA8, "TEST AL Ib", false, X, &test_A8},
   {0xA9, "TEST AX Iv", false, X, &test_A9},
   {0xAA, "STOSB", false, X, &stosb_AA},
   {0xAB, "STOSW", false, X, &stosw_AB},
   {0xAC, "LODSB", false, X, &lodsb_AC},
   {0xAD, "LODSW", false, X, &lodsw_AD},
   {0xAE, "SCASB", false, X, &scasb_AE},
   {0xAF, "SCASW", false, X, &scasw_AF},
   {0xB0, "MOV AL Ib", false, X, &mov_reg8},
   {0xB1, "MOV CL Ib", false, X, &mov_reg8},
   {0xB2, "MOV DL Ib", false, X, &mov_reg8},
   {0xB3, "MOV BL Ib", false, X, &mov_reg8},
   {0xB4, "MOV AH Ib", false, X, &mov_reg8},
   {0xB5, "MOV CH Ib", false, X, &mov_reg8},
   {0xB6, "MOV DH Ib", false, X, &mov_reg8},
   {0xB7, "MOV BH Ib", false, X, &mov_reg8},
   {0xB8, "MOV AX Iv", false, X, &mov_reg16},
   {0xB9, "MOV CX Iv", false, X, &mov_reg16},
   {0xBA, "MOV DX Iv", false, X, &mov_reg16},
   {0xBB, "MOV BX Iv", false, X, &mov_reg16},
   {0xBC, "MOV SP Iv", false, X, &mov_reg16},
   {0xBD, "MOV BP Iv", false, X, &mov_reg16},
   {0xBE, "MOV SI Iv", false, X, &mov_reg16},
   {0xBF, "MOV DI Iv", false, X, &mov_reg16},
   {0xC0, "SHL Eb Ib", true, X, &shl_C0},
   {0xC1, "SHL Ev Iv", true, X, &shl_C1},
   {0xC2, "RET Iw", false, X, &ret_C2},
   {0xC3, "RET", false, X, &ret_C3},
   {0xC4, "LES Gv Mp", true, X, &les_C4},
   {0xC5, "LDS Gv Mp", true, X, &lds_C5},
   {0xC6, "MOV Eb Ib", true, X, &mov_C6},
   {0xC7, "MOV Ev Iv", true, X, &mov_C7},
   {0xC8, "ENTER", false, X, &enter_C8},
   {0xC9, "LEAVE", false, X, &leave_C9},
   {0xCA, "RETF Iw", false, X, &retf_CA},
   {0xCB, "RETF", false, X, &retf_CB},
   {0xCC, "INT 3", false, X, &int_CC},
   {0xCD, "INT Ib", false, X, &int_CD},
   {0xCE, "INTO", false, X, &int_CE},
   {0xCF, "IRET", false, X, &iret_CF},
   {0xD0, "GRP2 Eb 1", true, X, &grp2_D0},
   {0xD1, "GRP2 Ev 1", true, X, &grp2_D1},
   {0xD2, "GRP2 Eb CL", true, X, &grp2_D2},
   {0xD3, "GRP2 Ev CL", true, X, &grp2_D3},
   {0xD4, "AAM I0", false, X, &aam_D4},
   {0xD5, "AAD I0", false, X, &aad_D5},
   {0xD6, "SALC", false, X, &salc_D6},
   {0xD7, "XLAT", false, X, &xlat_D7},
   {0xD8, "ESC", true, X, &fpu_dummy},
   {0xD9, "ESC", true, X, &fpu_dummy},
   {0xDA, "ESC", true, X, &fpu_dummy},
   {0xDB, "ESC", true, X, &fpu_dummy},
   {0xDC, "ESC", true, X, &fpu_dummy},
   {0xDD, "ESC", true, X, &fpu_dummy},
   {0xDE, "ESC", true, X, &fpu_dummy},
   {0xDF, "ESC", true, X, &fpu_dummy},
   {0xE0, "LOOPNZ Jb", false, X, &loopnz_E0},
   {0xE1, "LOOPZ Jb", false, X, &loopz_E1},
   {0xE2, "LOOP Jb", false, X, &loop_E2},
   {0xE3, "JCXZ Jb", false, X, &jcxz_E3},
   {0xE4, "IN AL Ib", false, X, &in_E4},
   {0xE5, "IN AX Ib", false, X, &in_E5},
   {0xE6, "OUT Ib AL", false, X, &out_E6},
   {0xE7, "OUT Ib AX", false, X, &out_E7},
   {0xE8, "CALL Jv", false, X, &call_E8},
   {0xE9, "JMP Jv", false, X, &jmp_E9},
   {0xEA, "JMP Ap", false, X, &jmp_EA},
   {0xEB, "JMP Jb", false, X, &jmp_EB},
   {0xEC, "IN AL DX", false, X, &in_EC},
   {0xED, "IN AX DX", false, X, &in_ED},
   {0xEE, "OUT DX AL", false, X, &out_EE},
   {0xEF, "OUT DX AX", false, X, &out_EF},
   {0xF0, "LOCK", false, X, &lock_F0},
   {0xF1, INVALID},
   {0xF2, "PREFIX", false, X, &invalid_prefix},
   {0xF3, "PREFIX", false, X, &invalid_prefix},
   {0xF4, "HLT", false, X, &hlt_F4},
   {0xF5, "CMC", false, X, &cmc_F5},
   {0xF6, "GRP3a Eb", true, X, &grp3_F6},
   {0xF7, "GRP3b Ev", true, X, &grp3_F7},
   {0xF8, "CLC", false, X, &clc_F8},
   {0xF9, "STC", false, X, &stc_F9},
   {0xFA, "CLI", false, X, &cli_FA},
   {0xFB, "STI", false, X, &sti_FB},
   {0xFC, "CLD", false, X, &cld_FC},
   {0xFD, "STD", false, X, &std_FD},
   {0xFE, "GRP4 Eb", true, X, &grp4_FE},
   {0xFF, "GRP5 Ev", true, X, &grp5_FF}
};

#undef INVALID
#undef X
