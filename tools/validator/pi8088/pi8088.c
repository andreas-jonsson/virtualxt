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
#include <assert.h>
#include <unistd.h>

#include <vxt/vxt.h>
#include <vxt/utils.h>

#define NUM_INVALID_FETCHES 6
#define NUM_MEM_OPS 16
#define CONSUMER "pi8088"
#define PULSE_LENGTH 1000000 //50000 // Microseconds

enum validator_state {
	STATE_SETUP,
	STATE_EXECUTE,
	STATE_READBACK,
	STATE_FINISHED
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

enum validator_state state = STATE_SETUP;
struct frame current_frame = {0};

static void pulse_clock(int ticks) {
	for (int i = 0; i < ticks; i++) {
		gpiod_line_set_value(clock_line, 1);
		usleep(PULSE_LENGTH / 2);
		gpiod_line_set_value(clock_line, 0);
		usleep(PULSE_LENGTH / 2);
		cycle_count++;
	}

	rd_signal = !gpiod_line_get_value(rd_line);
	wr_signal = !gpiod_line_get_value(wr_line);
	iom_signal = gpiod_line_get_value(iom_line);
	ale_signal = gpiod_line_get_value(ale_line);

	assert((rd_signal == 0) || (wr_signal == 0));
	assert((rd_signal == 0) || (ale_signal == 0));
}

static void reset_sequence() {
	printf("Starting reset sequence...\n");

	gpiod_line_set_value(reset_line, 1);
	pulse_clock(4);
	gpiod_line_set_value(reset_line, 0);

	printf("Waiting for ALE");

	do {
		printf(".");
		fflush(stdout);
		pulse_clock(1);
	} while (!ale_signal);

	printf("\nCPU is initialized!\n");
}

static void latch_address() {
	assert(ale_signal);

	address_latch = 0;
	for (int i = 0; i < 8; i++)
		address_latch |= ((uint32_t)gpiod_line_get_value(ad_0_7_line[i])) << i;
	for (int i = 0; i < 12; i++)
		address_latch |= ((uint32_t)gpiod_line_get_value(a_8_19_line[i])) << (i + 8);

	printf("Address latched: 0x%X\n", address_latch);
}

static void set_bus_direction_out() {
	for (int i = 0; i < 8; i++)
		gpiod_line_request_input(ad_0_7_line[i], CONSUMER);
}

static void set_bus_direction_in() {
	assert(rd_signal);
	for (int i = 0; i < 8; i++)
		gpiod_line_request_output(ad_0_7_line[i], CONSUMER, 0);
}

static void write_cpu_pins(vxt_byte data) {
	for (int i = 0; i < 8; i++)
		gpiod_line_set_value(ad_0_7_line[i], (data >> i) & 1);
}

static vxt_byte read_cpu_pins() {
	vxt_byte data = 0;
	for (int i = 0; i < 8; i++)
		data |= gpiod_line_get_value(ad_0_7_line[i]) << i;
	return data;
}

static void validate_data_write(vxt_pointer addr, vxt_byte data) {
	printf("CPU write: [0x%X] <- 0x%X\n", addr, data);
	assert(state != STATE_SETUP);
	
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

	fprintf(stderr, "ERROR: This is not a valid write!\n");
	assert(0);
}

static vxt_byte validate_data_read(vxt_pointer addr) {
	printf("CPU read: [0x%X] -> ", addr);

	switch (state) {
		case STATE_SETUP:
		{
			vxt_byte data = scratchpad[addr];
			printf("0x%X\n", data);

			// Trigger state change.
			if (addr != current_frame.reads[0].addr)
				return data;

			state = STATE_EXECUTE;
			printf("Validator state change: 0x%X\n", state);
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
							printf("0x%X\n", op->data);
							return op->data;
						}
					}
				}

				// Allow for N invalid fetches that we assume is the prefetch.
				if (current_frame.num_nop++ < NUM_INVALID_FETCHES)
					return 0x90; // NOP
			}
			state = STATE_READBACK;
		}
		case STATE_READBACK:
		{
			state = STATE_FINISHED;
			// TODO
			assert(0);
		}
	}

	assert(0);
	return 0;
}

// next_bus_cycle executes Tw, T4.
static void next_bus_cycle() {
	printf("Wait for bus cycle");
	while (!ale_signal) {
		printf(".");
		fflush(stdout);
		pulse_clock(1);
	}
	printf("\n");
}

// execute_bus_cycle executes T1, T2, T3.
static void execute_bus_cycle() {
	printf("Execute bus cycle...\n");

	assert(ale_signal);
	latch_address();
	pulse_clock(1);
	
	if (wr_signal) { // CPU is writing data to the bus.
		assert(!iom_signal);
		validate_data_write(address_latch, read_cpu_pins());
		pulse_clock(2);
	} else if (rd_signal) { // CPU is reading data from the bus.
		assert(!iom_signal);
		set_bus_direction_in();
		write_cpu_pins(validate_data_read(address_latch));
		pulse_clock(2);
		set_bus_direction_out();
	}
}

static void init_pins() {
	printf("Initialize pins...\n");

	// Output pins.

	clock_line = gpiod_chip_get_line(chip, 20);
	assert(clock_line);
	gpiod_line_request_output(clock_line, CONSUMER, 0);
	
	reset_line = gpiod_chip_get_line(chip, 21);
	assert(reset_line);
	gpiod_line_request_output(reset_line, CONSUMER, 1);

	test_line = gpiod_chip_get_line(chip, 23);
	assert(test_line);
	gpiod_line_request_output(test_line, CONSUMER, 0);

	// Input pins.

	ss0_line = gpiod_chip_get_line(chip, 22);
	assert(ss0_line);
	gpiod_line_request_input(ss0_line, CONSUMER);

	rd_line = gpiod_chip_get_line(chip, 24);
	assert(rd_line);
	gpiod_line_request_input(rd_line, CONSUMER);

	wr_line = gpiod_chip_get_line(chip, 25);
	assert(wr_line);
	gpiod_line_request_input(wr_line, CONSUMER);

	iom_line = gpiod_chip_get_line(chip, 26);
	assert(iom_line);
	gpiod_line_request_input(iom_line, CONSUMER);

	ale_line = gpiod_chip_get_line(chip, 27);
	assert(ale_line);
	gpiod_line_request_input(ale_line, CONSUMER);

	// Address and data pins.

	for (int i = 0; i < 8; i++) {
		ad_0_7_line[i] = gpiod_chip_get_line(chip, i);
		assert(ad_0_7_line[i]);
		gpiod_line_request_input(ad_0_7_line[i], CONSUMER);
	}
	
	for (int i = 0; i < 12; i++) {
		a_8_19_line[i] = gpiod_chip_get_line(chip, i + 8);
		assert(a_8_19_line[i]);
		gpiod_line_request_input(a_8_19_line[i], CONSUMER);
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
	
	for (int i = 0; i < NUM_MEM_OPS; i++)
		current_frame.reads[i].flags = current_frame.writes[i].flags = MOF_UNUSED;
	current_frame.reads[0] = (struct mem_op){VXT_POINTER(regs->cs, regs->ip), opcode, MOF_EMULATOR};

	if (current_frame.reads[0].addr == 0xFFFF0)
		current_frame.discard = true;
}

static void validate_mem_op(struct mem_op *op) {
	if (op->flags && op->flags != (MOF_EMULATOR | MOF_PI8088)) {
		fprintf(stderr, "ERROR: Operations do not match!\n");
		//print_debug_info();
		assert(0);
	}
}

static void end(int cycles, struct vxt_registers *regs, void *userdata) {
	(void)cycles; (void)userdata;

	current_frame.regs[1] = *regs;
	if (current_frame.discard)
		return;

	// Create scratchpad.
	memset(scratchpad, 0, sizeof(scratchpad));
	for (int i = 0; i < load_code_size; i++)
		scratchpad[i] = load_code[i];

	const struct vxt_registers *r = current_frame.regs;

	#define WRITE_WORD(o, w) { scratchpad[(o)] = ((w)&0xF); scratchpad[(o)+1] = ((w)>>8); }
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
	const vxt_byte jmp[5] = {0xEB, 0x2, 0x0, 0x0, 0x0};
	for (int i = 0; i < sizeof(jmp); i++)
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
	assert(0);
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
	assert(0);
}

static void discard(void *userdata) {
	(void)userdata;
	current_frame.discard = true;
}

static vxt_error initialize(vxt_system *s, void *userdata) {
	(void)s; (void)userdata;
	if (!(chip = gpiod_chip_open_by_name("gpiochip0"))) {
		fprintf(stderr, "Could not connect GPIO chip!\n");
		return -1;
	}

	allocator = vxt_system_allocator(s);
	load_code = vxtu_read_file(allocator, "tools/validator/pi8088/load", &load_code_size);
	aseert(load_code);

	store_code = vxtu_read_file(allocator, "tools/validator/pi8088/load", &store_code_size);
	aseert(store_code);

	init_pins();
	return VXT_NO_ERROR;
}

static vxt_error destroy(void *userdata) {
	(void)userdata;
	printf("Cleanup...\n");

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

	printf("Cleanup done!\n");
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
