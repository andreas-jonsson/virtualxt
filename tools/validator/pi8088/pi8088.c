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

#define PULSE_LENGTH 1000000 //50000 // Microseconds

struct gpiod_chip *chip = NULL;

struct gpiod_line *clock_line = NULL;
struct gpiod_line *reset_line = NULL;

struct gpiod_line *ale_line = NULL;

struct gpiod_line *ad_0_7_line[8] = {NULL};
struct gpiod_line *a_8_19_line[12] = {NULL};

uint32_t address_latch = 0;

static void pulse_clock(int ticks) {
	for (int i = 0; i < ticks; i++) {
		gpiod_line_set_value(clock_line, 1);
		usleep(PULSE_LENGTH / 2);
		gpiod_line_set_value(clock_line, 0);
		usleep(PULSE_LENGTH / 2);
	}
}

static void reset_sequence() {
	printf("Starting reset sequence...\n");

	gpiod_line_set_value(reset_line, 1);
	pulse_clock(4);
	gpiod_line_set_value(reset_line, 0);

	printf("Waiting for ALE...\n");

	do {
		pulse_clock(1);
	} while (!gpiod_line_get_value(ale_line));

	printf("CPU is initialized!\n");
}

static void latch_address() {
	assert(gpiod_line_get_value(ale_line));
	address_latch = 0;

	for (int i = 0; i < 8; i++)
		address_latch |= ((uint32_t)gpiod_line_get_value(ad_0_7_line[i]) << i);
	for (int i = 0; i < 12; i++)
		address_latch |= ((uint32_t)gpiod_line_get_value(a_8_19_line[i]) << (i + 8));

	printf("Address latched: 0x%X\n", address_latch);
}

static void init_pins() {
	printf("Initialize pins...\n");

	struct gpiod_line *t1 = gpiod_chip_get_line(chip, 27);
	assert(t1);
	gpiod_line_request_output(t1, "pi8088", 1);

	struct gpiod_line *t2 = gpiod_chip_get_line(chip, 28);
	assert(t2);
	gpiod_line_request_output(t2, "pi8088", 1);

	// Output pins.

	clock_line = gpiod_chip_get_line(chip, 20);
	assert(clock_line);
	gpiod_line_request_output(clock_line, "pi8088", 0);
	
	reset_line = gpiod_chip_get_line(chip, 21);
	assert(reset_line);
	gpiod_line_request_output(reset_line, "pi8088", 0);

	// Output pins.

	ale_line = gpiod_chip_get_line(chip, 27);
	assert(ale_line);
	gpiod_line_request_input(ale_line, "pi8088");

	// Address and data pins.

	for (int i = 2; i < 8; i++) {
		ad_0_7_line[i] = gpiod_chip_get_line(chip, i);
		assert(ad_0_7_line[i]);
		gpiod_line_request_input(ad_0_7_line[i], "pi8088");
	}

	// This is a rev 2 hack, switching GPIO 0/1 to 22/23. 
	
	ad_0_7_line[0] = gpiod_chip_get_line(chip, 22);
	assert(ad_0_7_line[0]);
	gpiod_line_request_input(ad_0_7_line[0], "pi8088");

	ad_0_7_line[1] = gpiod_chip_get_line(chip, 23);
	assert(ad_0_7_line[1]);
	gpiod_line_request_input(ad_0_7_line[1], "pi8088");
	
	for (int i = 0; i < 12; i++) {
		a_8_19_line[i] = gpiod_chip_get_line(chip, i + 8);
		assert(a_8_19_line[i]);
		gpiod_line_request_input(a_8_19_line[i], "pi8088");
	}	
}

static void cleanup() {
	printf("Cleanup...\n");
	
	// Output pins.
	gpiod_line_release(clock_line);
	gpiod_line_release(reset_line);

	// Input pins.
	gpiod_line_release(ale_line);

	// Address and data pins.
	for (int i = 0; i < 8; i++)
		gpiod_line_release(ad_0_7_line[i]);

	for (int i = 0; i < 12; i++)
		gpiod_line_release(a_8_19_line[i]);
	
	gpiod_chip_close(chip);

	printf("Cleanup done!\n");
}

int main(int argc, char *argv[]) {
	(void)argc; (void)argv;
	
	if (!(chip = gpiod_chip_open_by_name("gpiochip0"))) {
		fprintf(stderr, "Could not connect GPIO chip!\n");
		return -1;
	}

	init_pins();	
	atexit(&cleanup);

	reset_sequence();
	latch_address();

	printf("Program loop...\n");
	//for (;;) {
	//	pulse_clock(1);
	//}

	return 0;
}
