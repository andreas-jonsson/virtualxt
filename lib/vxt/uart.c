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

#include <vxt/vxtu.h>

#define DLAB ( (u->regs.lcr >> 7) != 0 )
#define CONFIGURE(r) { if (u->callbacks.config) u->callbacks.config(VXT_GET_PIREPHERAL(u), &u->regs, (r), u->callbacks.udata); }
#define PROCESS(d) { if (u->callbacks.data) u->callbacks.data(VXT_GET_PIREPHERAL(u), (d), u->callbacks.udata); }
#define READY { if (u->callbacks.ready) u->callbacks.ready(VXT_GET_PIREPHERAL(u), u->callbacks.udata); }

enum pendig_irq {
    PENDING_RX = 0x1,
    PENDING_TX = 0x2,
    PENDING_MSR = 0x4,
    PENDING_LSR = 0x8
};

enum {
    IEN_ENABLE_RX = 0x1,
    IEN_ENABLE_TX = 0x2,
    IEN_ENABLE_LSR = 0x4,
    IEN_ENABLE_MSR = 0x8
};

struct uart {
    struct vxtu_uart_registers regs;
    struct vxtu_uart_interface callbacks;

    vxt_word base;
    int irq;

    enum pendig_irq pending;
    vxt_byte prev_msr;

    bool has_rx_data;
    vxt_byte rx_data;
    vxt_byte tx_data;
};

const vxt_byte data_bits_size[4] = { 5, 6, 7, 8 };
const vxt_byte data_bits_mask[4] = { 0x1F, 0x3F, 0x7F, 0xFF };

static vxt_byte in(struct uart *u, vxt_word port) {
	switch (port - u->base) {
        case 0x0: // Data
            if (DLAB) {
                return (vxt_byte)u->regs.divisor;
            } else {
                u->has_rx_data = false;
                u->pending &= ~PENDING_RX;

                if (u->regs.ien & IEN_ENABLE_LSR) {
                    u->pending |= PENDING_LSR;
                    vxt_system_interrupt(VXT_GET_SYSTEM(u), u->irq);
                }

                vxt_byte data = u->rx_data;
                READY;
                return data;
            }
        case 0x1: // IEN
            return DLAB ? (vxt_byte)(u->regs.divisor >> 8) : u->regs.ien;
        case 0x2: // IIR
        {
            vxt_byte ret = u->pending ? 0 : 1;
            if (u->pending & PENDING_TX) {
                ret |= 0x2;
                u->pending &= ~PENDING_TX;
            } else if (u->pending & PENDING_RX) {
                ret |= 0x4;
            } else if (u->pending & PENDING_LSR) {
                ret |= 0x6;
            }

            if (u->pending)
                vxt_system_interrupt(VXT_GET_SYSTEM(u), u->irq);
            return ret;
        }
        case 0x3: // LCR
            return u->regs.lcr;
        case 0x4: // MCR
            return u->regs.mcr;
        case 0x5: // LSR
            u->pending &= ~PENDING_LSR;
            return u->has_rx_data ? 0x61 : 0x60;
        case 0x6: // MSR
        {
            vxt_byte ret = u->regs.msr & 0xF0;
            if ((u->regs.msr & 0x10) != (u->prev_msr & 0x10))
                ret |= 0x1;
            if ((u->regs.msr & 0x20) != (u->prev_msr & 0x20))
                ret |= 0x2;
            if ((u->regs.msr & 0x80) != (u->prev_msr & 0x80))
                ret |= 0x8;

            u->prev_msr = u->regs.msr;
            u->pending &= ~PENDING_MSR;
            return ret;
        }
        default:
            return 0xFF;
	}
}

static void out(struct uart *u, vxt_word port, vxt_byte data) {
	switch (port - u->base) {
        case 0x0: // Data
            if (DLAB) {
                u->regs.divisor = (u->regs.divisor & 0xFF00) | data;
                CONFIGURE(0);
                return;
            }

            u->tx_data = data_bits_mask[u->regs.lcr & 3] & data;
            if (u->regs.mcr & 0x10) {
                vxtu_uart_write(VXT_GET_PIREPHERAL(u), u->tx_data);
                return;
            }

            if (u->regs.ien & IEN_ENABLE_TX) {
                u->pending |= PENDING_TX;
                vxt_system_interrupt(VXT_GET_SYSTEM(u), u->irq);
            }

            if (u->regs.ien & IEN_ENABLE_LSR) {
                u->pending |= PENDING_LSR;
                vxt_system_interrupt(VXT_GET_SYSTEM(u), u->irq);
            }

            PROCESS(u->tx_data);
            return;
        case 0x1: // IEN
            if (DLAB) {
                u->regs.divisor = (u->regs.divisor & 0xFF) | ((vxt_word)data << 8);
                CONFIGURE(0);
            } else {
                u->regs.ien = data;
                CONFIGURE(1);
            }
            return;
        case 0x3: // LCR
            u->regs.lcr = data;
            CONFIGURE(3);
            return;
        case 0x4: // MCR
            u->regs.mcr = data;
            CONFIGURE(4);
            return;
	}
}

static vxt_error install(struct uart *u, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(u);
    vxt_system_install_io(s, p, u->base, u->base + 7);

    vxt_system_install_monitor(s, p, "Address", &u->base, VXT_MONITOR_SIZE_WORD|VXT_MONITOR_FORMAT_HEX);
    vxt_system_install_monitor(s, p, "IRQ", &u->irq, VXT_MONITOR_SIZE_DWORD|VXT_MONITOR_FORMAT_DECIMAL);

    enum vxt_monitor_flag flags = VXT_MONITOR_SIZE_BYTE|VXT_MONITOR_FORMAT_BINARY;
    vxt_system_install_monitor(s, p, "Interrupt Enable", &u->regs.ien, flags);
    vxt_system_install_monitor(s, p, "Interrupt Identification", &u->regs.iir, flags);
    vxt_system_install_monitor(s, p, "Line Control", &u->regs.lcr, flags);
    vxt_system_install_monitor(s, p, "Modem Control", &u->regs.mcr, flags);
    vxt_system_install_monitor(s, p, "Line Status", &u->regs.lsr, flags);
    vxt_system_install_monitor(s, p, "Modem Status", &u->regs.msr, flags);

    flags = VXT_MONITOR_SIZE_BYTE|VXT_MONITOR_FORMAT_HEX;
    vxt_system_install_monitor(s, p, "RX Data", &u->rx_data, flags);
    vxt_system_install_monitor(s, p, "TX Data", &u->tx_data, flags);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct uart *u) {
    vxt_memclear(&u->regs, sizeof(struct vxtu_uart_registers));
    u->regs.divisor = 12;
    u->regs.msr = 0x30;    
    u->pending = 0;
    u->has_rx_data = false;
    u->prev_msr = 0;
    return VXT_NO_ERROR;
}

static const char *name(struct uart *u) {
    (void)u; return "UART (National Semiconductor 8250)";
}

static enum vxt_pclass pclass(struct uart *u) {
    (void)u; return VXT_PCLASS_UART;
}

VXT_API struct vxt_pirepheral *vxtu_uart_create(vxt_allocator *alloc, vxt_word base_port, int irq) VXT_PIREPHERAL_CREATE(alloc, uart, {
    DEVICE->base = base_port;
    DEVICE->irq = irq;

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->pclass = &pclass;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})

VXT_API const struct vxtu_uart_registers *vxtu_uart_internal_registers(struct vxt_pirepheral *p) {
    return &VXT_GET_DEVICE(uart, p)->regs;
}

VXT_API void vxtu_uart_set_callbacks(struct vxt_pirepheral *p, struct vxtu_uart_interface *intrf) {
    VXT_GET_DEVICE(uart, p)->callbacks = *intrf;
}

VXT_API bool vxtu_uart_ready(struct vxt_pirepheral *p) {
    return !VXT_GET_DEVICE(uart, p)->has_rx_data;
}

VXT_API void vxtu_uart_write(struct vxt_pirepheral *p, vxt_byte data) {
    struct uart *u = VXT_GET_DEVICE(uart, p);
    u->rx_data = data;
    u->has_rx_data = true;
    if (u->regs.ien & IEN_ENABLE_RX) {
        u->pending |= PENDING_RX;
        vxt_system_interrupt(vxt_pirepheral_system(p), u->irq);
    }
}

VXT_API vxt_word vxtu_uart_address(struct vxt_pirepheral *p) {
    return VXT_GET_DEVICE(uart, p)->base;
}
