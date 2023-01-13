// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <vxt/vxt.h>
#include <printf.h>

#include "uart.h"
#include "gpio.h"

void nop(void);

// Set baud rate 115200 8N1 and map to GPIO
void uart_init(void) {
    register unsigned int r;

    *AUX_ENABLE |= 1;       // Enable UART1, AUX mini uart (AUXENB)
    *AUX_MU_CNTL = 0;		// Stop transmitter and receiver
    *AUX_MU_LCR  = 3;       // 8-bit mode
    *AUX_MU_MCR  = 0;		// RTS (request to send)
    *AUX_MU_IER  = 0;		// Disable interrupts
    *AUX_MU_IIR  = 0xC6;    // Clear FIFOs
    *AUX_MU_BAUD = 270;    	// 115200 baud

    // Map UART1 to GPIO pins
    r = *GPFSEL1;
    r &= ~( (7 << 12)|(7 << 15) ); // Clear bits 12-17 (gpio14, gpio15)
    r |=    (2 << 12)|(2 << 15);   // Set value 2 (select ALT5: UART1)
    *GPFSEL1 = r;

    // Enable GPIO 14, 15
    *GPPUD = 0;                         // No pull up/down control
    r = 150; while(r--) { nop(); }      // Waiting 150 cycles
    *GPPUDCLK0 = (1 << 14)|(1 << 15);   // Enable clock for GPIO 14, 15
    r = 150; while(r--) { nop(); }      // Waiting 150 cycles
    *GPPUDCLK0 = 0;                     // Flush GPIO setup

    *AUX_MU_CNTL = 3;      //Enable transmitter and receiver (Tx, Rx)
}

void uart_write_byte(vxt_byte b) {
    do {
    	nop();
    } while (!(*AUX_MU_LSR & 0x20));
    *AUX_MU_IO = b;
}

void _putchar(char character) {
    uart_write_byte(character);
}

bool uart_peak(void) {
	nop();
    return (*AUX_MU_LSR & 0x01) != 0;
}

vxt_byte uart_read_byte(void) {
	while (!uart_peak()) {};
    return (vxt_byte)(*AUX_MU_IO);
}
