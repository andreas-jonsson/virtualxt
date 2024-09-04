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

// Reference: https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/

#include <vxt/vxtu.h>

#ifdef __linux__

#include <stdio.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

struct serial {
	vxt_word base_port;
	char name[FILENAME_MAX];
    struct vxt_peripheral *uart;

	vxt_byte tx_data;
	vxt_byte rx_data;
	bool has_tx_data;
	bool has_rx_data;

	struct termios tty;
	int serial_port;
};

static void setup_serial(struct serial *c) {
	const struct vxtu_uart_registers *regs = vxtu_uart_internal_registers(c->uart);
	vxt_word div = regs->divisor;
	vxt_byte lcr = regs->lcr;

	if (tcgetattr(c->serial_port, &c->tty)) {
		VXT_LOG("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		return;
	}

	// Clear all bits that set the data size.
	c->tty.c_cflag &= ~CSIZE;
	switch (lcr & 3) {
		case 0: c->tty.c_cflag |= CS5; break;
		case 1: c->tty.c_cflag |= CS6; break;
		case 2: c->tty.c_cflag |= CS7; break;
		case 3: c->tty.c_cflag |= CS8; break;
	};

	// Clear stop field.
	c->tty.c_cflag &= ~CSTOPB;
	if (lcr & 4)
		c->tty.c_cflag |= CSTOPB;

	// Clear parity bit.
	c->tty.c_cflag &= ~(PARENB | PARODD);
	switch ((lcr >> 3) & 7) {
		case 1: c->tty.c_cflag |= PARENB | PARODD; break;
		case 2: c->tty.c_cflag |= PARENB; break;
	};

	//c->tty.c_cflag &= ~CRTSCTS; 		// Disable RTS/CTS hardware flow control (NOT POSIX)
	c->tty.c_cflag |= CREAD | CLOCAL; 	// Turn on READ & ignore ctrl lines (CLOCAL = 1)

	c->tty.c_lflag &= ~ICANON;
	c->tty.c_lflag &= ~ECHO; 												// Disable echo
	c->tty.c_lflag &= ~ECHOE; 												// Disable erasure
	c->tty.c_lflag &= ~ECHONL; 												// Disable new-line echo
	c->tty.c_lflag &= ~ISIG; 												// Disable interpretation of INTR, QUIT and SUSP
	c->tty.c_iflag &= ~(IXON | IXOFF | IXANY); 								// Turn off s/w flow ctrl
	c->tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); 	// Disable any special handling of received bytes

	c->tty.c_oflag &= ~OPOST; 		// Prevent special interpretation of output bytes (e.g. newline chars)
	c->tty.c_oflag &= ~ONLCR; 		// Prevent conversion of newline to carriage return/line feed
	//c->tty.c_oflag &= ~OXTABS; 	// Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	//c->tty.c_oflag &= ~ONOEOT; 	// Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	c->tty.c_cc[VTIME] = c->tty.c_cc[VMIN] = 0;    // Dont block.

	speed_t baud = (speed_t)(115200 / (div ? div : 1));
	cfsetispeed(&c->tty, baud);
	cfsetospeed(&c->tty, baud);

	if (tcsetattr(c->serial_port, TCSANOW, &c->tty) != 0) {
		VXT_LOG("Error %i from tcsetattr: %s\n", errno, strerror(errno));
		return;
	}
}

static void uart_config_cb(struct vxt_peripheral *uart, const struct vxtu_uart_registers *regs, int idx, void *udata) {
	(void)uart; (void)regs;
    struct serial *c = (struct serial*)udata;
	if (idx == 0 || idx == 3)
		setup_serial(c);
}

static void write_data(struct serial *c) {
	int ret = write(c->serial_port, &c->tx_data, 1);
	if (ret == 1) {
		c->has_tx_data = false;
	} else if (ret != 0) {
		VXT_LOG("Error writing: %s", strerror(errno));
		vxtu_uart_set_error(c->uart, 0xFF);
		c->has_tx_data = false;
	}
}

static void uart_data_cb(struct vxt_peripheral *uart, vxt_byte data, void *udata) {
	struct serial *c = (struct serial*)udata; (void)uart;
	while (c->has_tx_data)
		write_data(c);

	c->has_tx_data = true;
	c->tx_data = data;
}

static vxt_error timer(struct serial *c, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;

	int ret = read(c->serial_port, &c->rx_data, 1);
	if (ret == 1) {
		c->has_rx_data = true;
	} else if (ret != 0) {
		VXT_LOG("Error reading: %s", strerror(errno));
		vxtu_uart_set_error(c->uart, 0xFF);
	}

	if (c->has_tx_data)
		write_data(c);
	return VXT_NO_ERROR;
}

static vxt_error install(struct serial *c, vxt_system *s) {
    for (int i = 0; i < VXT_MAX_PERIPHERALS; i++) {
        struct vxt_peripheral *ip = vxt_system_peripheral(s, (vxt_byte)i);
        if (ip && (vxt_peripheral_class(ip) == VXT_PCLASS_UART) && (vxtu_uart_address(ip) == c->base_port)) {
            struct vxtu_uart_interface intrf = {
                .config = &uart_config_cb,
				.data = &uart_data_cb,
                .ready = NULL,
                .udata = (void*)c
            };
            vxtu_uart_set_callbacks(ip, &intrf);
            c->uart = ip;
            break;
        }
    }
    if (!c->uart) {
        VXT_LOG("Could not find UART with base address: 0x%X", c->base_port);
        return VXT_USER_ERROR(0);
    }

	c->serial_port = open(c->name, O_RDWR);
	if (c->serial_port == -1) {
		VXT_LOG("Error %i from open serial device: %s\n", errno, strerror(errno));
		return VXT_USER_ERROR(1);
	}

	vxt_system_install_timer(s, VXT_GET_PERIPHERAL(c), 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct serial *c) {
	setup_serial(c);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct serial *c) {
    close(c->serial_port);
    vxt_system_allocator(VXT_GET_SYSTEM(c))(VXT_GET_PERIPHERAL(c), 0);
    return VXT_NO_ERROR;
}

static const char *name(struct serial *c) {
    (void)c; return "Serial Passthrough";
}

VXTU_MODULE_CREATE(serial, {
    if (sscanf(ARGS, "%hx", &DEVICE->base_port) != 1) {
		VXT_LOG("Invalid UART address: %s", ARGS);
		return NULL;
    }

	const char *dev = strchr(ARGS, ',');
	if (!dev) {
		VXT_LOG("Invalid device name: %s", ARGS);
		return NULL;
	}
	strncpy(DEVICE->name, dev + 1, FILENAME_MAX - 1);

    PERIPHERAL->install = &install;
	PERIPHERAL->destroy = &destroy;
	PERIPHERAL->timer = &timer;
	PERIPHERAL->reset = &reset;
    PERIPHERAL->name = &name;
})

#else

struct serial { int _; };
VXTU_MODULE_CREATE(serial, { return NULL; })

#endif
