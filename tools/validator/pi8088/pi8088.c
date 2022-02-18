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

#define NUM_MEM_OPS 16
#define CONSUMER "pi8088"
#define PULSE_LENGTH 1000000 //50000 // Microseconds

enum mem_op_flag {
	MOF_UNUSED 		= 0x0,
	MOF_EMULATOR 	= 0x1,
	MOF_PI8088  	= 0x2
};

struct mem_op {
	vxt_pointer addr;
	vxt_byte data;
	enum mem_op_flag flags;
}

struct frame {
	const char *name;
	vxt_byte opcode;
	bool modregrm;
	struct vxt_registers regs[2];

	struct mem_op reads[NUM_MEM_OPS];
	struct mem_op writes[NUM_MEM_OPS];
}

struct gpiod_chip *chip = NULL;

struct gpiod_line *clock_line = NULL;
struct gpiod_line *reset_line = NULL;

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

struct frame current_frame = {0};

static void pulse_clock(int ticks) {
	for (int i = 0; i < ticks; i++) {
		gpiod_line_set_value(clock_line, 1);
		usleep(PULSE_LENGTH / 2);
		gpiod_line_set_value(clock_line, 0);
		usleep(PULSE_LENGTH / 2);
		cycle_count++;
	}

	rd_signal = gpiod_line_get_value(rd_line);
	wr_signal = gpiod_line_get_value(wr_line);
	iom_signal = gpiod_line_get_value(iom_line);
	ale_signal = gpiod_line_get_value(ale_line);

	assert((rd_signal == 0) || (wr_signal == 0));
	assert((wr_line == 0) || (ale_signal == 0));
}

static void reset_sequence() {
	printf("Starting reset sequence...\n");

	gpiod_line_set_value(reset_line, 1);
	pulse_clock(4);
	gpiod_line_set_value(reset_line, 0);

	printf("Waiting for ALE...\n");

	do {
		pulse_clock(1);
	} while (!ale_signal);

	printf("CPU is initialized!\n");
}

static void latch_address() {
	assert(ale_signal);

	address_latch = 0;
	for (int i = 0; i < 8; i++)
		address_latch |= ((uint32_t)gpiod_line_get_value(ad_0_7_line[i]) << i);
	for (int i = 0; i < 12; i++)
		address_latch |= ((uint32_t)gpiod_line_get_value(a_8_19_line[i]) << (i + 8));

	printf("Address latched: 0x%X\n", address_latch);
}

static void set_bus_direction_in() {
	for (int i = 0; i < 8; i++)
		gpiod_line_request_input(ad_0_7_line[i], CONSUMER);
}

static void set_bus_direction_out() {
	assert(rd_signal);
	for (int i = 0; i < 8; i++)
		gpiod_line_request_output(ad_0_7_line[i], CONSUMER, 0);
}

static void write_data(vxt_byte data) {
	for (int i = 0; i < 8; i++)
		gpiod_line_set_value(ad_0_7_line[i], (data >> i) & 1);
}

static vxt_byte read_data() {
	vxt_byte data = 0;
	for (int i = 0; i < 8; i++)
		data |= gpiod_line_get_value(ad_0_7_line[i]) << i;
	return data;
}

static void validate_data_write(vxt_pointer addr, vxt_byte data) {
	printf("CPU write: [0x%X] <- 0x%X\n", addr, data);
	
	for (int i = 0; i < NUM_MEM_OPS; i++) {
		mem_op *op = &current_frame.writes[i];
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
	printf("CPU read: 0x%X <- [0x%X]\n", data, addr);

	for (int i = 0; i < NUM_MEM_OPS; i++) {
		struct mem_op *op = &current_frame.reads[i];
		if (!(op->flags & MOF_PI8088) && (op->flags & MOF_EMULATOR)) {
			if (op->addr == addr) { // YES: This is a valid read!
				op->flags |= MOF_PI8088;
				return op->data;
			}
		}
	}

	fprintf(stderr, "ERROR: This is not a valid read!\n");
	assert(0);
	return 0;
}

static void execute_cycle() {
	printf("Execute cycle...\n");
	if (ale_signal) {
		latch_address();
		pulse_clock(1);
		
		if (wr_signal) { // CPU is writing data to the bus.
			assert(!iom_line);
			validate_data_write(address_latch, read_data());
			pulse_clock(2);
		} else if (rd_line) { // CPU is reading data from the bus.
			assert(!iom_line);
			set_bus_direction_out();
			write_data(validate_data_read(address_latch));
			pulse_clock(2);
			set_bus_direction_in();
		}
	} else {
		pulse_clock(1);
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

	// Input pins.

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
	current_frame.name = name;
	current_frame.opcode = opcode;
	current_frame.modregrm = modregrm;
	current_frame.regs[0] = *regs;
	
	for (int i = 0; i < NUM_MEM_OPS; i++)
		current_frame.reads[i].flags = current_frame.writes[i].flags = MOF_UNUSED;
	current_frame.reads[0] = (struct mem_op){(vxt_pointer)((regs->cs << 4) + (regs->ip - 1)), opcode, MOF_EMULATOR};
}

static void validate_mem_op(struct mem_op *op) {
	if (op->flags && op->flags != (MOF_EMULATOR | MOF_PI8088)) {
		fprintf(stderr, "ERROR: Operations do not match!\n");
		//print_debug_info();
		assert(0);
	}
}

static void end(int cycles, struct vxt_registers *regs, void *userdata) {
	current_frame.regs[1] = *regs;

	cycle_count = 0;
	while (cycle_count < cycles)
		execute_cycle();

	for (int i = 0; i < NUM_MEM_OPS; i++) {
		validate_mem_op(&current_frame.reads[i]);
		validate_mem_op(&current_frame.writes[i]);
	}
}

static void read(vxt_pointer addr, vxt_byte data, void *userdata) {
	for (int i = 0; i < NUM_MEM_OPS; i++) {
		struct mem_op *op = &current_frame.reads[i];
		if (!op->flags) {
			*op = (struct mem_op){addr, data, MOF_EMULATOR};
			return;
		}
	}
	assert(0);
}

static void write(vxt_pointer addr, vxt_byte data, void *userdata) {
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
	fprintf(stderr, "WARNING: This validator can not discard instructions.\n");
}

static vxt_error initialize(vxt_system *s, void *userdata) {
	if (!(chip = gpiod_chip_open_by_name("gpiochip0"))) {
		fprintf(stderr, "Could not connect GPIO chip!\n");
		return -1;
	}

	init_pins();	
	reset_sequence();
	return VXT_NO_ERROR;
}

static vxt_error destroy(void *userdata) {
	printf("Cleanup...\n");
	
	// Output pins.
	gpiod_line_release(clock_line);
	gpiod_line_release(reset_line);

	// Input pins.
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
	return &pi8088;
}
