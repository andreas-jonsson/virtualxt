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

#include <gpiod.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define VXT_CLIB_IO
#include <vxt/vxt.h>
#include <vxt/utils.h>

#define NUM_INVALID_FETCHES 6
#define NUM_MEM_OPS 16
#define CONSUMER "pi8088"
#define PULSE_LENGTH 2000 // Microseconds

#define NL "\n"
#define DEBUG(...) log(LOG_DEBUG, __VA_ARGS__)
#define ERROR(...) { log(LOG_ERROR, "ERROR: " __VA_ARGS__); abort(); }
#define ENSURE(e) if (!(e)) ERROR("Ensure failed!")
#define ASSERT(e, ...) if (!(e)) ERROR(__VA_ARGS__)

enum validator_state {
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
	bool modregrm;
	bool discard;
	int num_nop;
	struct vxt_registers regs[2];

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

enum validator_state state = STATE_SETUP;
struct frame current_frame = {0};

const enum log_level {
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_SILENT
} level = LOG_DEBUG;

static int log(const enum log_level lv, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);

	int ret = 0;
	if (lv >= level)
		ret = vfprintf(lv == LOG_ERROR ? stderr : stdout, fmt, va);
	
	va_end(va);
	return ret;
}

static int check(int line) {
	ENSURE(line == 0 || line == 1);
	return line;
}

static void pulse_clock(int ticks) {
	for (int i = 0; i < ticks; i++) {
		check(gpiod_line_set_value(clock_line, 1));
		usleep(PULSE_LENGTH / 2);
		check(gpiod_line_set_value(clock_line, 0));
		usleep(PULSE_LENGTH / 2);
		cycle_count++;
	}

	rd_signal = !check(gpiod_line_get_value(rd_line));
	wr_signal = !check(gpiod_line_get_value(wr_line));
	iom_signal = check(gpiod_line_get_value(iom_line));
	ale_signal = check(gpiod_line_get_value(ale_line));

	ENSURE((rd_signal == 0) || (wr_signal == 0));
	ENSURE((rd_signal == 0) || (ale_signal == 0));

	if (!ale_signal) {
		cpu_memory_access = (enum access_type)(check(gpiod_line_get_value(a_8_19_line[9])) << 1) | check(gpiod_line_get_value(a_8_19_line[8]));
		cpu_interrupt_enabled = check(gpiod_line_get_value(a_8_19_line[10])) != 0;
	}
}

static void reset_sequence() {
	log(LOG_INFO, "Starting reset sequence..." NL);

	check(gpiod_line_set_value(reset_line, 1));
	pulse_clock(4);
	check(gpiod_line_set_value(reset_line, 0));

	log(LOG_INFO, "Waiting for ALE");

	do {
		log(LOG_INFO, ".");
		fflush(stdout);
		pulse_clock(1);
	} while (!ale_signal);

	log(LOG_INFO, NL "CPU is initialized!" NL);
}

static void latch_address() {
	ENSURE(ale_signal);

	address_latch = 0;
	for (int i = 0; i < 8; i++)
		address_latch |= ((uint32_t)check(gpiod_line_get_value(ad_0_7_line[i]))) << i;
	for (int i = 0; i < 12; i++)
		address_latch |= ((uint32_t)check(gpiod_line_get_value(a_8_19_line[i]))) << (i + 8);

	DEBUG("Address latched: 0x%X" NL, address_latch);
}

static void set_bus_direction_out() {
	for (int i = 0; i < 8; i++) {
		struct gpiod_line **ln = &ad_0_7_line[i];
		gpiod_line_release(*ln);
		*ln = gpiod_chip_get_line(chip, i);
		ENSURE(*ln);
		check(gpiod_line_request_input(*ln, CONSUMER));
	}
}

static void set_bus_direction_in(vxt_byte data) {
	ENSURE(rd_signal);
	for (int i = 0; i < 8; i++) {
		struct gpiod_line **ln = &ad_0_7_line[i]; 
		gpiod_line_release(*ln);
		*ln = gpiod_chip_get_line(chip, i);
		ENSURE(*ln);
		check(gpiod_line_request_output(*ln, CONSUMER, (data >> i) & 1));
	}
}

static vxt_byte read_cpu_pins() {
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

	ERROR("This is not a valid write!" NL);
}

static vxt_byte validate_data_read(vxt_pointer addr) {
	DEBUG("CPU read: [0x%X] -> ", addr);
	ENSURE(!iom_signal);

	switch (state) {
		case STATE_SETUP:
		{
			// Trigger state change?
			if (addr != current_frame.reads[0].addr) {
				vxt_byte data = scratchpad[addr];
				DEBUG("0x%X" NL, data);
				return data;
			}

			state = STATE_EXECUTE;
			DEBUG("Validator state change: 0x%X" NL, state);
		}
		case STATE_EXECUTE:
		{
			const struct vxt_registers *r = &current_frame.regs[1];
			if (addr != VXT_POINTER(r->cs, r->ip)) {
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

				// Allow for N invalid fetches that we assume is the prefetch.
				if (current_frame.num_nop++ < NUM_INVALID_FETCHES) {
					DEBUG("NOP" NL);
					return 0x90; // NOP
				}
			}

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
			DEBUG("NOP" NL);
			return 0x90; // NOP
		}
	}

	ERROR(NL);
	return 0;
}

// next_bus_cycle executes Tw, T4.
static void next_bus_cycle() {
	DEBUG("Wait for bus cycle");
	while (!ale_signal) {
		DEBUG(".");
		fflush(stdout);
		pulse_clock(1);
	}
	DEBUG(NL);
}

// execute_bus_cycle executes T1, T2, T3.
static void execute_bus_cycle() {
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

static void init_pins() {
	log(LOG_INFO, "Initialize pins..." NL);

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

static void begin(const char *name, vxt_byte opcode, bool modregrm, struct vxt_registers *regs, void *userdata) {
	(void)userdata;

	current_frame.name = name;
	current_frame.opcode = opcode;
	current_frame.modregrm = modregrm;
	current_frame.num_nop = 0;
	current_frame.discard = false;
	current_frame.regs[0] = *regs;

	// Correct for fetch.
	current_frame.regs->ip--;
	
	for (int i = 0; i < NUM_MEM_OPS; i++)
		current_frame.reads[i].flags = current_frame.writes[i].flags = MOF_UNUSED;
	current_frame.reads[0] = (struct mem_op){VXT_POINTER(current_frame.regs->cs, current_frame.regs->ip), opcode, MOF_EMULATOR};

	// We must discard the first instruction after boot.
	if (current_frame.reads[0].addr == 0xFFFF0)
		current_frame.discard = true;
}

static void validate_mem_op(struct mem_op *op) {
	ENSURE(state == STATE_FINISHED);
	if (op->flags && op->flags != (MOF_EMULATOR | MOF_PI8088)) {
		//print_debug_info();
		ERROR("Operations do not match!" NL);
	}
}

static void validate_registers() {
	ENSURE(state == STATE_FINISHED);
	struct vxt_registers *r = &current_frame.regs[1];
	bool err = false;
	int offset = 0;

	#define TEST(reg) {																		\
		vxt_word v = *(vxt_word*)&scratchpad[offset];										\
		offset += 2;																		\
		if (r->reg != v) { 																	\
			ERROR("'" #reg "' do not match! EMU: 0x%X != CPU: 0x%X\n", r->reg, v); 			\
			err = true;																		\
		}																					\
	}																						\
		
	TEST(flags);
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

	ENSURE(!err);
}

static void end(int cycles, struct vxt_registers *regs, void *userdata) {
	(void)cycles; (void)userdata;

	current_frame.regs[1] = *regs;
	if (current_frame.discard)
		return;

	log(LOG_INFO, "Validate instruction: %s (0x%X)" NL, current_frame.name, current_frame.opcode);

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

	state = STATE_SETUP;
	cycle_count = 0;

	reset_sequence();
	while (state != STATE_FINISHED) {
		execute_bus_cycle();
		next_bus_cycle();
	}

	for (int i = 0; i < NUM_MEM_OPS; i++) {
		validate_mem_op(&current_frame.reads[i]);
		validate_mem_op(&current_frame.writes[i]);
	}
	validate_registers();
}

static void read_byte(vxt_pointer addr, vxt_byte data, void *userdata) {
	(void)userdata;
	for (int i = 0; i < NUM_MEM_OPS; i++) {
		struct mem_op *op = &current_frame.reads[i];
		if (!op->flags) {
			*op = (struct mem_op){addr, data, MOF_EMULATOR};
			return;
		}
	}
	ERROR(NL);
}

static void write_byte(vxt_pointer addr, vxt_byte data, void *userdata) {
	(void)userdata;
	for (int i = 0; i < NUM_MEM_OPS; i++) {
		struct mem_op *op = &current_frame.writes[i];
		if (!op->flags) {
			*op = (struct mem_op){addr, data, MOF_EMULATOR};
			return;
		}
	}
	ERROR(NL);
}

static void discard(void *userdata) {
	(void)userdata;
	current_frame.discard = true;
}

static vxt_error initialize(vxt_system *s, void *userdata) {
	(void)s; (void)userdata;
	if (!(chip = gpiod_chip_open_by_name("gpiochip0"))) {
		log(LOG_ERROR, "Could not connect GPIO chip!" NL);
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
	log(LOG_INFO, "Cleanup..." NL);

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

	log(LOG_INFO, "Cleanup done!" NL);
	return VXT_NO_ERROR;
}

struct vxt_validator pi8088 = {0};

struct vxt_validator *pi8088_validator() {
	pi8088.initialize = &initialize;
	pi8088.destroy = &destroy;
	pi8088.begin = &begin;
	pi8088.end = &end;
	pi8088.read = &read_byte;
	pi8088.write = &write_byte;
	pi8088.discard = &discard;
	return &pi8088;
}
