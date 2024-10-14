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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

#include <gpiod.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <vxt/vxt.h>
#include <vxt/vxtu.h>

#include "udmask.h"

#define NUM_INVALID_FETCHES 6
#define NUM_MEM_OPS (0x20000 + 16)
#define CONSUMER "pi8088"
#define HW_REVISION 1

#define NL "\n"
#define LOG print_log
#define DEBUG(...) LOG(LOG_DEBUG, __VA_ARGS__)
#define ERROR(...) { LOG(LOG_ERROR, "ERROR: " __VA_ARGS__); abort(); }
#define ENSURE(e) if (!(e)) ERROR("Ensure failed!")
#define ASSERT(e, ...) if (!(e)) ERROR(__VA_ARGS__)

enum validator_state {
	STATE_RESET,
	STATE_SETUP,
	STATE_EXECUTE,
	STATE_READBACK,
	STATE_FINISHED
};

enum access_type {
	ACC_ALTERNATE_DATA 		= 0x0,
	ACC_STACK 				= 0x1,
	ACC_CODE_OR_NONE  		= 0x2,
	ACC_DATA 				= 0x4
};

enum mem_op_flag {
	MOF_UNUSED 		= 0x0,
	MOF_EMULATOR 	= 0x1,
	MOF_PI8088  	= 0x2
};

struct mem_op {
	vxt_pointer addr;
	vxt_byte data;
	enum mem_op_flag flags;
};

struct frame {
	const char *name;
	vxt_byte opcode;
	vxt_byte ext_opcode;
	bool modregrm;
	bool discard;
	bool next_fetch;
	int num_nop;
	struct vxt_registers regs[2];

	vxt_pointer prefetch_addr[NUM_INVALID_FETCHES];
	struct mem_op reads[NUM_MEM_OPS];
	struct mem_op writes[NUM_MEM_OPS];
};

struct gpiod_chip *chip = NULL;

struct gpiod_line *clock_line = NULL;
struct gpiod_line *reset_line = NULL;
struct gpiod_line *test_line = NULL;

struct gpiod_line *ss0_line = NULL;
struct gpiod_line *rd_line = NULL;
struct gpiod_line *wr_line = NULL;
struct gpiod_line *iom_line = NULL;
struct gpiod_line *ale_line = NULL;

struct gpiod_line *ad_0_7_line[8] = {NULL};
struct gpiod_line *a_8_19_line[12] = {NULL};

enum access_type cpu_memory_access = 0;
bool cpu_interrupt_enabled = false;

int cycle_count = 0;
int executed_cycles = 0;
int rd_signal = 0;
int wr_signal = 0;
int iom_signal = 0;
int ale_signal = 0;
uint32_t address_latch = 0;

vxt_allocator *allocator = NULL;
vxt_byte *load_code = NULL;
int load_code_size = 0;
vxt_byte *store_code = NULL;
int store_code_size = 0;
vxt_byte scratchpad[0x100000] = {0};
int readback_ptr = 0;

// TODO: Fix this issue.
bool code_as_data_skip = false;

const bool visit_once = false;
bool visited[0x100000] = {false};

//vxt_pointer trigger_addr = VXT_POINTER(0x0, 0x7C00);
vxt_pointer trigger_addr = VXT_INVALID_POINTER;

enum validator_state state = STATE_RESET;
struct frame current_frame = {0};

const enum log_level {
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_SILENT
} level = LOG_INFO;

static int print_log(const enum log_level lv, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);

	int ret = 0;
	if (lv >= level)
		ret = vfprintf(lv == LOG_ERROR ? stderr : stdout, fmt, va);

	if ((level == LOG_DEBUG) || (lv == LOG_ERROR)) {
		fflush(stdout);
		fflush(stderr);
	}
	
	va_end(va);
	return ret;
}

static int check(int line) {
	if ((line != 0) && (line != 1))
		ERROR("%s" NL, strerror(errno));
	return line;
}

static void pulse_clock(int ticks) {
	for (int i = 0; i < ticks; i++) {
		check(gpiod_line_set_value(clock_line, 1));
		check(gpiod_line_set_value(clock_line, 0));
		cycle_count++;
	}

	ale_signal = check(gpiod_line_get_value(ale_line));
	if (ale_signal && (state == STATE_RESET))
		state = STATE_SETUP;

	// On some chips these lines float during reset.
	if (state != STATE_RESET) {
		rd_signal = !check(gpiod_line_get_value(rd_line));
		wr_signal = !check(gpiod_line_get_value(wr_line));
		iom_signal = check(gpiod_line_get_value(iom_line));

		ENSURE((rd_signal == 0) || (wr_signal == 0));
		ENSURE((rd_signal == 0) || (ale_signal == 0));
	}

	if (!ale_signal) {
		cpu_memory_access = (enum access_type)(check(gpiod_line_get_value(a_8_19_line[9])) << 1) | check(gpiod_line_get_value(a_8_19_line[8]));
		cpu_interrupt_enabled = check(gpiod_line_get_value(a_8_19_line[10])) != 0;
	}
}

static void reset_sequence(void) {
	DEBUG("Starting reset sequence..." NL);

	state = STATE_RESET;

	check(gpiod_line_set_value(reset_line, 1));
	pulse_clock(4);
	check(gpiod_line_set_value(reset_line, 0));

	DEBUG("Waiting for ALE");
	do {
		DEBUG(".");
		pulse_clock(1);
	} while (!ale_signal);
	DEBUG(NL "CPU is initialized!" NL);
}

static void latch_address(void) {
	ENSURE(ale_signal);

	address_latch = 0;
	for (int i = 0; i < 8; i++)
		address_latch |= ((uint32_t)check(gpiod_line_get_value(ad_0_7_line[i]))) << i;
	for (int i = 0; i < 12; i++)
		address_latch |= ((uint32_t)check(gpiod_line_get_value(a_8_19_line[i]))) << (i + 8);

	DEBUG("Address latched: 0x%X" NL, address_latch);
}

static void set_bus_direction_out(void) {
	for (int i = 0; i < 8; i++)
		check(gpiod_line_set_config(ad_0_7_line[i], GPIOD_LINE_REQUEST_DIRECTION_INPUT, 0, 0));
}

static void set_bus_direction_in(vxt_byte data) {
	ENSURE(rd_signal);
	for (int i = 0; i < 8; i++)
		check(gpiod_line_set_config(ad_0_7_line[i], GPIOD_LINE_REQUEST_DIRECTION_OUTPUT, 0, (data >> i) & 1));
}

static vxt_byte read_cpu_pins(void) {
	vxt_byte data = 0;
	for (int i = 0; i < 8; i++)
		data |= check(gpiod_line_get_value(ad_0_7_line[i])) << i;
	return data;
}

static void validate_data_write(vxt_pointer addr, vxt_byte data) {
	DEBUG("CPU write: [0x%X] <- 0x%X" NL, addr, data);
	ENSURE(state == STATE_EXECUTE || state == STATE_READBACK);

	if (state == STATE_READBACK) {
		if (iom_line) {
			if ((addr == 0xFF) && (data == 0xFF)) {
				state = STATE_FINISHED;
				DEBUG("Validator state change: 0x%X" NL, state);
				return;
			}
		} else {
			ENSURE((addr == 0) || (addr == 1));
		}
		scratchpad[addr] = data;
		return;
	}

	ENSURE(!iom_signal);

	for (int i = 0; i < NUM_MEM_OPS; i++) {
		struct mem_op *op = &current_frame.writes[i];
		if (!(op->flags & MOF_PI8088) && (op->flags & MOF_EMULATOR)) {
			if (op->addr == addr) { // YES: This is a valid write!
				op->flags |= MOF_PI8088;
				op->data = data;
				return;
			}
		}
	}
	
	ERROR("This is not a valid write! ([0x%X] <- 0x%X)" NL, addr, data);
}

static vxt_byte validate_data_read(vxt_pointer addr) {
	DEBUG("CPU read: [0x%X] -> ", addr);
	ENSURE(!iom_signal);

	switch (state) {
		case STATE_RESET:
			ERROR("Should not validate read during reset!" NL);
		case STATE_SETUP:
		{
			// Trigger state change?
			if (addr != current_frame.reads[0].addr) {
				vxt_byte data = scratchpad[addr];
				DEBUG("0x%X" NL, data);
				return data;
			}

			cycle_count = 0;
			state = STATE_EXECUTE;
			DEBUG("Validator state change: 0x%X" NL, state);
		}
		case STATE_EXECUTE:
		{
			vxt_pointer next_inst_addr = VXT_POINTER(current_frame.regs[1].cs, current_frame.regs[1].ip);

			// TODO: This is a bug in the validator! The emulator needs to indicate data or code fetch to avoid this.
			if ((cpu_memory_access != ACC_CODE_OR_NONE) && (abs((int)addr - (int)next_inst_addr) < 6)) {
				LOG(LOG_INFO, "Fetching next instruction as data is not supported!" NL);
				code_as_data_skip = true;
				state = STATE_FINISHED;
				return 0;
			}

			for (int i = 0; i < NUM_MEM_OPS; i++) {
				struct mem_op *op = &current_frame.reads[i];
				if (!(op->flags & MOF_PI8088) && (op->flags & MOF_EMULATOR)) {
					if (op->addr == addr) { // YES: This is a valid read!
						op->flags |= MOF_PI8088;
						DEBUG("0x%X" NL, op->data);
						return op->data;
					}
				}
			}

			if (cpu_memory_access == ACC_CODE_OR_NONE) {
				if (addr == next_inst_addr)
					current_frame.next_fetch = true;

				// Allow for N invalid fetches that we assume is the prefetch.
				// This is intended to fill the prefetch queue so the instruction can finish.
				if (current_frame.num_nop < NUM_INVALID_FETCHES) {
					current_frame.prefetch_addr[current_frame.num_nop++] = addr;
					DEBUG("NOP" NL);
					return 0x90; // NOP
				}
			}

			executed_cycles = cycle_count;
			state = STATE_READBACK;
			DEBUG("Validator state change: 0x%X" NL, state);

			// Clear scratchpad.
			memset(scratchpad, 0, sizeof(scratchpad));
			readback_ptr = 2;
		}
		case STATE_READBACK:
		{
			// Assume this is prefetch.
			if (readback_ptr >= store_code_size) {
				ENSURE(cpu_memory_access == ACC_CODE_OR_NONE);
				DEBUG("NOP" NL);
				return 0x90; // NOP
			}

			// We assume store code fetches linear memory.
			vxt_byte data = store_code[readback_ptr++];
			DEBUG("0x%X" NL, data);
			return data;
		}
		case STATE_FINISHED:
		{
			ENSURE(cpu_memory_access == ACC_CODE_OR_NONE);
			DEBUG("NOP" NL);
			return 0x90; // NOP
		}
	}

	ERROR(NL);
	return 0;
}

// next_bus_cycle executes Tw, T4.
static void next_bus_cycle(void) {
	DEBUG("Wait for bus cycle");
	while (!ale_signal) {
		DEBUG(".");
		pulse_clock(1);
	}
	DEBUG(NL);
}

// execute_bus_cycle executes T1, T2, T3.
static void execute_bus_cycle(void) {
	DEBUG("Execute bus cycle..." NL);

	ENSURE(ale_signal);
	latch_address();
	pulse_clock(1);

	DEBUG("Memory access type: 0x%X" NL, cpu_memory_access);
	
	if (wr_signal) { // CPU is writing data to the bus.
		validate_data_write(address_latch, read_cpu_pins());
		pulse_clock(2);
	} else if (rd_signal) { // CPU is reading data from the bus.
		set_bus_direction_in(validate_data_read(address_latch));
		pulse_clock(2);
		set_bus_direction_out();
	}
}

static void init_pins(void) {
	LOG(LOG_INFO, "Initialize pins..." NL);

	// Output pins.

	clock_line = gpiod_chip_get_line(chip, 20);
	ENSURE(clock_line);
	check(gpiod_line_request_output(clock_line, CONSUMER, 0));
	
	reset_line = gpiod_chip_get_line(chip, 21);
	ENSURE(reset_line);
	check(gpiod_line_request_output(reset_line, CONSUMER, 1));

	test_line = gpiod_chip_get_line(chip, 23);
	ENSURE(test_line);
	check(gpiod_line_request_output(test_line, CONSUMER, 0));

	// Input pins.

	ss0_line = gpiod_chip_get_line(chip, 22);
	ENSURE(ss0_line);
	check(gpiod_line_request_input(ss0_line, CONSUMER));

	rd_line = gpiod_chip_get_line(chip, 24);
	ENSURE(rd_line);
	check(gpiod_line_request_input(rd_line, CONSUMER));

	wr_line = gpiod_chip_get_line(chip, 25);
	ENSURE(wr_line);
	check(gpiod_line_request_input(wr_line, CONSUMER));

	iom_line = gpiod_chip_get_line(chip, 26);
	ENSURE(iom_line);
	check(gpiod_line_request_input(iom_line, CONSUMER));

	ale_line = gpiod_chip_get_line(chip, 27);
	ENSURE(ale_line);
	check(gpiod_line_request_input(ale_line, CONSUMER));

	// Address and data pins.

	for (int i = 0; i < 8; i++) {
		ad_0_7_line[i] = gpiod_chip_get_line(chip, i);
		ENSURE(ad_0_7_line[i]);
		check(gpiod_line_request_input(ad_0_7_line[i], CONSUMER));
	}
	
	for (int i = 0; i < 12; i++) {
		a_8_19_line[i] = gpiod_chip_get_line(chip, i + 8);
		ENSURE(a_8_19_line[i]);
		check(gpiod_line_request_input(a_8_19_line[i], CONSUMER));
	}
}

static void begin(struct vxt_registers *regs, void *userdata) {
	(void)userdata;

	current_frame.discard = false;
	current_frame.regs[0] = *regs;

	vxt_pointer ip_addr = VXT_POINTER(regs->cs, regs->ip);
	if (trigger_addr == ip_addr)
		trigger_addr = VXT_INVALID_POINTER;

	if ((trigger_addr != VXT_INVALID_POINTER) || (visit_once && visited[ip_addr])) {
		current_frame.discard = true;
		return;
	}
	
	memset(current_frame.reads, 0, sizeof(current_frame.reads));
	memset(current_frame.writes, 0, sizeof(current_frame.writes));
}

static void validate_mem_op(struct mem_op *op) {
	ENSURE(state == STATE_FINISHED);
	if (op->flags && op->flags != (MOF_EMULATOR | MOF_PI8088)) {
		// TODO: Print more info.
		ERROR("Memory operations do not match!" NL);
	}
}

static void mask_undefined_flags(vxt_word *flags) {
	*flags &= 0xCD5; // Ignore I and T.
	for (int i = 0; flag_mask_lookup[i].opcode != -1; i++) {
		int iop = (int)current_frame.opcode;
		if (flag_mask_lookup[i].opcode == iop) {
			if (flag_mask_lookup[i].ext != -1) {
				ENSURE(current_frame.modregrm);
				for (; flag_mask_lookup[i].opcode == iop; i++) {
					if (flag_mask_lookup[i].ext == (int)current_frame.ext_opcode)
						goto mask;
				}

				// Nothing to mask!
				return;
			}

mask:
			*flags &= ~((vxt_word)flag_mask_lookup[i].mask);
			return;
		}
	}
}

static void validate_registers(void) {
	ENSURE(state == STATE_FINISHED);
	ASSERT(current_frame.next_fetch, "Next instruction was never fetched! Possible bad jump.");

	struct vxt_registers *r = &current_frame.regs[1];
	int offset = 2;

	#define TEST(reg) {																			\
		vxt_word v = *(vxt_word*)&scratchpad[offset];											\
		offset += 2;																			\
		ASSERT(r->reg == v, "'" #reg "' do not match! EMU: 0x%X != CPU: 0x%X" NL, r->reg, v);	\
	}																							\
	
	TEST(ax);
	TEST(bx);
	TEST(cx);
	TEST(dx);
	TEST(ss);
	TEST(sp);
	TEST(cs);
	TEST(ds);
	TEST(es);
	TEST(bp);
	TEST(si);
	TEST(di);
	
	#undef TEST

	vxt_word cpu_flags = *(vxt_word*)scratchpad; 
	mask_undefined_flags(&cpu_flags);
	vxt_word emu_flags = r->flags;
	mask_undefined_flags(&emu_flags);

	if (cpu_flags != emu_flags)
		ERROR("Flags error! EMU: 0x%X (0x%X) != CPU: 0x%X (0x%X)" NL, emu_flags, r->flags, cpu_flags, *(vxt_word*)scratchpad);
}

static void end(const char *name, vxt_byte opcode, bool modregrm, int cycles, struct vxt_registers *regs, void *userdata) {
	(void)cycles; (void)userdata;

	vxt_pointer ip_addr = VXT_POINTER(current_frame.regs->cs, current_frame.regs->ip);
	if ((trigger_addr != VXT_INVALID_POINTER) || (visit_once && visited[ip_addr]))
		return;

	visited[ip_addr] = true;

	current_frame.name = name;
	current_frame.opcode = opcode;
	current_frame.ext_opcode = 0xFF;
	current_frame.modregrm = modregrm;
	current_frame.num_nop = 0;
	current_frame.next_fetch = false;
	current_frame.regs[1] = *regs;

	if (modregrm) {
		for (int i = 0; i < (NUM_MEM_OPS - 1); i++) {
			if (current_frame.reads[i].data == opcode) {
				current_frame.ext_opcode = (current_frame.reads[i + 1].data >> 3) & 7;
				break;
			}
		}
	}

	// We must discard the first instructions after boot.
	if ((ip_addr >= 0xFFFF0) || (ip_addr <= 0xFF))
		current_frame.discard = true;

	LOG(LOG_INFO, "%s: %s (0x%X) @ %0*X:%0*X" NL, current_frame.discard ? "DISCARD" : "VALIDATE", name, opcode, 4, current_frame.regs->cs, 4, current_frame.regs->ip);
	if (current_frame.discard)
		return;

	memset(current_frame.prefetch_addr, 0, sizeof(current_frame.prefetch_addr));

	// Create scratchpad.
	memset(scratchpad, 0, sizeof(scratchpad));
	for (int i = 0; i < load_code_size; i++)
		scratchpad[i] = load_code[i];

	const struct vxt_registers *r = current_frame.regs;

	#define WRITE_WORD(o, w) { scratchpad[(o)] = (vxt_byte)((w)&0xFF); scratchpad[(o)+1] = (vxt_byte)((w)>>8); }
	WRITE_WORD(0x00, r->flags);

	WRITE_WORD(0x0B, r->bx);
	WRITE_WORD(0x0E, r->cx);
	WRITE_WORD(0x11, r->dx);

	WRITE_WORD(0x14, r->ss);
	WRITE_WORD(0x19, r->ds);
	WRITE_WORD(0x1E, r->es);
	WRITE_WORD(0x23, r->sp);
	WRITE_WORD(0x28, r->bp);
	WRITE_WORD(0x2D, r->si);
	WRITE_WORD(0x32, r->di);
	
	WRITE_WORD(0x37, r->ax);
	
	WRITE_WORD(0x3A, r->ip);
	WRITE_WORD(0x3C, r->cs);
	#undef WRITE_WORD

	// JMP 0:2
	const vxt_byte jmp[5] = {0xEA, 0x2, 0x0, 0x0, 0x0};
	for (unsigned i = 0; i < sizeof(jmp); i++)
		scratchpad[0xFFFF0 + i] = jmp[i];

	code_as_data_skip = false;

	reset_sequence();
	for (state = STATE_SETUP; state != STATE_FINISHED;) {
		execute_bus_cycle();
		next_bus_cycle();
	}

	if (code_as_data_skip)
		return;

	for (int i = 0; i < NUM_MEM_OPS; i++) {
		validate_mem_op(&current_frame.reads[i]);
		validate_mem_op(&current_frame.writes[i]);
	}
	validate_registers();

	//if (executed_cycles != cycles)
	//	LOG(LOG_WARNING, "WARNING: Unexpected cycle timing! EMU: %d, CPU: %d" NL, cycles, executed_cycles);
}

static void rw_byte(struct mem_op *rw_op, vxt_pointer addr, vxt_byte data) {
	for (int i = 0; i < NUM_MEM_OPS; i++) {
		struct mem_op *op = &rw_op[i];
		if (!op->flags || ((op->flags & MOF_EMULATOR) && (op->addr == addr) && (op->data == data))) {
			*op = (struct mem_op){addr, data, MOF_EMULATOR};
			return;
		}
	}
	ERROR(NL);
}

static void read_byte(vxt_pointer addr, vxt_byte data, void *userdata) {
	(void)userdata;
	if (current_frame.discard)
		return;
	rw_byte(current_frame.reads, addr, data);
}

static void write_byte(vxt_pointer addr, vxt_byte data, void *userdata) {
	(void)userdata;
	visited[addr & 0xFFFFF] = false;
	if (current_frame.discard)
		return;
	rw_byte(current_frame.writes, addr, data);
}

static void discard(void *userdata) {
	(void)userdata;
	current_frame.discard = true;
}

static vxt_error initialize(vxt_system *s, void *userdata) {
	(void)s; (void)userdata;

	printf("VirtualXT Hardware Validator\nRev: %d\n\n", HW_REVISION);
	if (!(chip = gpiod_chip_open_by_name("gpiochip0"))) {
		LOG(LOG_ERROR, "Could not connect GPIO chip!" NL);
		return -1;
	}

	allocator = vxt_system_allocator(s);
	load_code = vxtu_read_file(allocator, "tools/validator/pi8088/load", &load_code_size);
	ENSURE(load_code);

	store_code = vxtu_read_file(allocator, "tools/validator/pi8088/store", &store_code_size);
	ENSURE(store_code);

	init_pins();
	return VXT_NO_ERROR;
}

static vxt_error destroy(void *userdata) {
	(void)userdata;
	LOG(LOG_INFO, "Cleanup..." NL);

	allocator(load_code, 0);
	allocator(store_code, 0);
	
	// Output pins.
	gpiod_line_release(clock_line);
	gpiod_line_release(reset_line);
	gpiod_line_release(test_line);

	// Input pins.
	gpiod_line_release(ss0_line);
	gpiod_line_release(rd_line);
	gpiod_line_release(wr_line);
	gpiod_line_release(iom_line);
	gpiod_line_release(ale_line);

	// Address and data pins.
	for (int i = 0; i < 8; i++)
		gpiod_line_release(ad_0_7_line[i]);

	for (int i = 0; i < 12; i++)
		gpiod_line_release(a_8_19_line[i]);
	
	gpiod_chip_close(chip);

	LOG(LOG_INFO, "Cleanup done!" NL);
	return VXT_NO_ERROR;
}

struct vxt_validator pi8088 = {0};

struct vxt_validator *pi8088_validator(void) {
	pi8088.initialize = &initialize;
	pi8088.destroy = &destroy;
	pi8088.begin = &begin;
	pi8088.end = &end;
	pi8088.read = &read_byte;
	pi8088.write = &write_byte;
	pi8088.discard = &discard;
	return &pi8088;
}
