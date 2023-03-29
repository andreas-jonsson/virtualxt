/*
 * ch365/ch367/ch368 PCI/PCIE application library
 *
 * Copyright (C) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
 * Web: http://wch.cn
 * Author: WCH@TECH39 <tech@wch.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I /path/to/cross-kernel/include
 *
 * Version: V1.11
 *
 * Update Log:
 * V1.0 - initial version
 * V1.1 - modified APIs related to interrupt
 * V1.11 - added APIs related to SPI transfer
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "ch36x_lib.h"

static const char *device = "/dev/ch36xpci0";

/**
 * ch36x_open - open ch36x device
 * @devname: the device name to open
 *
 * The function return the new file descriptor, or -1 if an error occurred
 */
int ch36x_open(const char *devname)
{
    int fd;

    fd = open(devname, O_RDWR);
    if (fd > 0) {
        fcntl(fd, F_SETOWN, getpid());
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | FASYNC);
    }

    return fd;
}

/**
 * ch36x_close - close ch36x device
 * @fd: the device handle
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_close(int fd)
{
    return close(fd);
}

/**
 * ch36x_get_chiptype - get chip model
 * @fd: file descriptor of ch36x device
 * @chiptype: pointer to chiptype
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_get_chiptype(int fd, enum CHIP_TYPE *chiptype)
{
    return ioctl(fd, CH36x_GET_CHIPTYPE, (unsigned long)chiptype);
}

/**
 * ch36x_get_version - get driver version
 * @fd: file descriptor of ch36x device
 * @version: pointer to version string
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_get_version(int fd, char *version)
{
    return ioctl(fd, CH36x_GET_VERSION, (unsigned long)version);
}

/**
 * ch36x_get_ioaddr - get io base address of ch36x
 * @fd: file descriptor of ch36x device
 * @ioaddr: pointer to io base address
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_get_ioaddr(int fd, void *ioaddr)
{
    return ioctl(fd, CH36x_GET_IO_BASE_ADDR, (unsigned long)ioaddr);
}

/**
 * ch36x_get_memaddr - get io memory address of ch36x
 * @fd: file descriptor of ch36x device
 * @memaddr: pointer to memory base address
 *
 * The function return 0 if success, others if fail.
 */

int ch36x_get_memaddr(int fd, void *memaddr)
{
    return ioctl(fd, CH36x_GET_MEM_BASE_ADDR, (unsigned long)memaddr);
}

/**
 * ch36x_read_config_byte - read one byte from config space
 * @fd: file descriptor of ch36x device
 * @offset: config space register offset
 * @obyte: pointer to read byte
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_config_byte(int fd, uint8_t offset, uint8_t *obyte)
{
    struct {
        uint8_t offset;
        uint8_t *obyte;
    } ch36x_read_config_t;

    ch36x_read_config_t.offset = offset;
    ch36x_read_config_t.obyte = obyte;

    return ioctl(fd, CH36x_READ_CONFIG_BYTE, (unsigned long)&ch36x_read_config_t);
}

/**
 * ch36x_read_config_word - read one word from config space
 * @fd: file descriptor of ch36x device
 * @offset: config space register offset
 * @oword: pointer to read word
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_config_word(int fd, uint8_t offset, uint16_t *oword)
{
    struct {
        uint8_t offset;
        uint16_t *oword;
    } ch36x_read_config_t;

    ch36x_read_config_t.offset = offset;
    ch36x_read_config_t.oword = oword;

    return ioctl(fd, CH36x_READ_CONFIG_WORD, (unsigned long)&ch36x_read_config_t);
}

/**
 * ch36x_read_config_dword - read one dword from config space
 * @fd: file descriptor of ch36x device
 * @offset: config space register offset
 * @oword: pointer to read dword
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_config_dword(int fd, uint8_t offset, uint32_t *odword)
{
    struct {
        uint8_t offset;
        uint32_t *odword;
    } ch36x_read_config_t;

    ch36x_read_config_t.offset = offset;
    ch36x_read_config_t.odword = odword;

    return ioctl(fd, CH36x_READ_CONFIG_DWORD, (unsigned long)&ch36x_read_config_t);
}

/**
 * ch36x_write_config_byte - write one byte to config space
 * @fd: file descriptor of ch36x device
 * @offset: config space register offset
 * @ibyte: byte to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_config_byte(int fd, uint8_t offset, uint8_t ibyte)
{
    struct {
        uint8_t offset;
        uint8_t ibyte;
    } ch36x_write_config_t;

    ch36x_write_config_t.offset = offset;
    ch36x_write_config_t.ibyte = ibyte;

    return ioctl(fd, CH36x_WRITE_CONFIG_BYTE, (unsigned long)&ch36x_write_config_t);
}

/**
 * ch36x_write_config_word - write one word to config space
 * @fd: file descriptor of ch36x device
 * @offset: config space register offset
 * @iword: word to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_config_word(int fd, uint8_t offset, uint16_t iword)
{
    struct {
        uint8_t offset;
        uint16_t iword;
    } ch36x_write_config_t;

    ch36x_write_config_t.offset = offset;
    ch36x_write_config_t.iword = iword;

    return ioctl(fd, CH36x_WRITE_CONFIG_WORD, (unsigned long)&ch36x_write_config_t);
}

/**
 * ch36x_write_config_dword - write one dword to config space
 * @fd: file descriptor of ch36x device
 * @offset: config space register offset
 * @dword: dword to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_config_dword(int fd, uint8_t offset, uint32_t idword)
{
    struct {
        uint8_t offset;
        uint32_t idword;
    } ch36x_write_config_t;

    ch36x_write_config_t.offset = offset;
    ch36x_write_config_t.idword = idword;

    return ioctl(fd, CH36x_WRITE_CONFIG_DWORD, (unsigned long)&ch36x_write_config_t);
}

/**
 * ch36x_read_io_byte - read one byte from io space
 * @fd: file descriptor of ch36x device
 * @ioaddr: io address
 * @obyte: pointer to read byte
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_io_byte(int fd, unsigned long ioaddr, uint8_t *obyte)
{
    struct {
        unsigned long ioaddr;
        uint8_t *obyte;
    } ch36x_read_io_t;

    ch36x_read_io_t.ioaddr = ioaddr;
    ch36x_read_io_t.obyte = obyte;

    return ioctl(fd, CH36x_READ_IO_BYTE, (unsigned long)&ch36x_read_io_t);
}

/**
 * ch36x_read_io_word - read one byte from io word
 * @fd: file descriptor of ch36x device
 * @ioaddr: io address
 * @oword: pointer to read word
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_io_word(int fd, unsigned long ioaddr, uint16_t *oword)
{
    struct {
        unsigned long ioaddr;
        uint16_t *oword;
    } ch36x_read_io_t;

    ch36x_read_io_t.ioaddr = ioaddr;
    ch36x_read_io_t.oword = oword;

    return ioctl(fd, CH36x_READ_IO_WORD, (unsigned long)&ch36x_read_io_t);
}

/**
 * ch36x_read_io_dword - read one dword from io space
 * @fd: file descriptor of ch36x device
 * @ioaddr: io address
 * @odword: pointer to read dword
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_io_dword(int fd, unsigned long ioaddr, uint32_t *odword)
{
    struct {
        unsigned long ioaddr;
        uint32_t *odword;
    } ch36x_read_io_t;

    ch36x_read_io_t.ioaddr = ioaddr;
    ch36x_read_io_t.odword = odword;

    return ioctl(fd, CH36x_READ_IO_DWORD, (unsigned long)&ch36x_read_io_t);
}

/**
 * ch36x_write_io_byte - write one byte to io space
 * @fd: file descriptor of ch36x device
 * @ioaddr: io address
 * @ibyte: byte to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_io_byte(int fd, unsigned long ioaddr, uint8_t ibyte)
{
    struct {
        unsigned long ioaddr;
        uint8_t ibyte;
    } ch36x_write_io_t;

    ch36x_write_io_t.ioaddr = ioaddr;
    ch36x_write_io_t.ibyte = ibyte;

    return ioctl(fd, CH36x_WRITE_IO_BYTE, (unsigned long)&ch36x_write_io_t);
}

/**
 * ch36x_write_io_word - write one word to io space
 * @fd: file descriptor of ch36x device
 * @ioaddr: io address
 * @iword: word to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_io_word(int fd, unsigned long ioaddr, uint16_t iword)
{
    struct {
        unsigned long ioaddr;
        uint16_t iword;
    } ch36x_write_io_t;

    ch36x_write_io_t.ioaddr = ioaddr;
    ch36x_write_io_t.iword = iword;

    return ioctl(fd, CH36x_WRITE_IO_WORD, (unsigned long)&ch36x_write_io_t);
}

/**
 * ch36x_write_io_dword - write one dword to io space
 * @fd: file descriptor of ch36x device
 * @ioaddr: io address
 * @idword: dword to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_io_dword(int fd, unsigned long ioaddr, uint32_t idword)
{
    struct {
        unsigned long ioaddr;
        uint32_t idword;
    } ch36x_write_io_t;

    ch36x_write_io_t.ioaddr = ioaddr;
    ch36x_write_io_t.idword = idword;

    return ioctl(fd, CH36x_WRITE_IO_DWORD, (unsigned long)&ch36x_write_io_t);
}

/**
 * ch36x_read_mem_byte - read one byte from memory space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @obyte: pointer to read byte
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_mem_byte(int fd, unsigned long memaddr, uint8_t *obyte)
{
    struct {
        unsigned long memaddr;
        uint8_t *obyte;
    } ch36x_read_mem_t;

    ch36x_read_mem_t.memaddr = memaddr;
    ch36x_read_mem_t.obyte = obyte;

    return ioctl(fd, CH36x_READ_MEM_BYTE, (unsigned long)&ch36x_read_mem_t);
}

/**
 * ch36x_read_mem_word - read one word from memory space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @oword: pointer to read word
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_mem_word(int fd, unsigned long memaddr, uint16_t *oword)
{
    struct {
        unsigned long memaddr;
        uint16_t *oword;
    } ch36x_read_mem_t;

    ch36x_read_mem_t.memaddr = memaddr;
    ch36x_read_mem_t.oword = oword;

    return ioctl(fd, CH36x_READ_MEM_WORD, (unsigned long)&ch36x_read_mem_t);
}

/**
 * ch36x_read_mem_dword - read one dword from memory space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @odword: pointer to read dword
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_mem_dword(int fd, unsigned long memaddr, uint32_t *odword)
{
    struct {
        unsigned long memaddr;
        uint32_t *odword;
    } ch36x_read_mem_t;

    ch36x_read_mem_t.memaddr = memaddr;
    ch36x_read_mem_t.odword = odword;

    return ioctl(fd, CH36x_READ_MEM_DWORD, (unsigned long)&ch36x_read_mem_t);
}

/**
 * ch36x_write_mem_byte - write one byte to mem space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @ibyte: byte to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_mem_byte(int fd, unsigned long memaddr, uint8_t ibyte)
{
    struct {
        unsigned long memaddr;
        uint8_t ibyte;
    } ch36x_write_mem_t;

    ch36x_write_mem_t.memaddr = memaddr;
    ch36x_write_mem_t.ibyte = ibyte;

    return ioctl(fd, CH36x_WRITE_MEM_BYTE, (unsigned long)&ch36x_write_mem_t);
}

/**
 * ch36x_write_mem_word - write one word to mem space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @iword: word to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_mem_word(int fd, unsigned long memaddr, uint16_t iword)
{
    struct {
        unsigned long memaddr;
        uint16_t iword;
    } ch36x_write_mem_t;

    ch36x_write_mem_t.memaddr = memaddr;
    ch36x_write_mem_t.iword = iword;

    return ioctl(fd, CH36x_WRITE_MEM_WORD, (unsigned long)&ch36x_write_mem_t);
}

/**
 * ch36x_write_mem_dword - write one dword to mem space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @idword: dword to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_mem_dword(int fd, unsigned long memaddr, uint32_t idword)
{
    struct {
        unsigned long memaddr;
        uint32_t idword;
    } ch36x_write_mem_t;

    ch36x_write_mem_t.memaddr = memaddr;
    ch36x_write_mem_t.idword = idword;

    return ioctl(fd, CH36x_WRITE_MEM_DWORD, (unsigned long)&ch36x_write_mem_t);
}

/**
 * ch36x_read_mem_block - read bytes from mem space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @obuffer: pointer to read buffer
 * @len: length to read
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_read_mem_block(int fd, unsigned long memaddr, uint8_t *obuffer, unsigned long len)
{
    struct {
        unsigned long memaddr;
        uint8_t *obuffer;
        unsigned long len;
    } ch36x_read_mem_t;

    if ((len <= 0) || (len > sizeof(mCH368_MEM_REG))) {
        return -1;
    }
    ch36x_read_mem_t.memaddr = memaddr;
    ch36x_read_mem_t.obuffer = obuffer;
    ch36x_read_mem_t.len = len;

    return ioctl(fd, CH36x_READ_MEM_BLOCK, (unsigned long)&ch36x_read_mem_t);
}

/**
 * ch36x_write_mem_block - write bytes to mem space
 * @fd: file descriptor of ch36x device
 * @memaddr: memory address
 * @ibuffer: pointer to write buffer
 * @len: length to write
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_write_mem_block(int fd, unsigned long memaddr, uint8_t *ibuffer, unsigned long len)
{
    struct {
        unsigned long memaddr;
        uint8_t *ibuffer;
        unsigned long len;
    } ch36x_write_mem_t;

    if ((len <= 0) || (len > sizeof(mCH368_MEM_REG))) {
        return -1;
    }
    ch36x_write_mem_t.memaddr = memaddr;
    ch36x_write_mem_t.ibuffer = ibuffer;
    ch36x_write_mem_t.len = len;

    return ioctl(fd, CH36x_WRITE_MEM_BLOCK, (unsigned long)&ch36x_write_mem_t);
}

/**
 * ch36x_enable_isr - enable ch36x interrupt
 * @fd: file descriptor of ch36x device
 * @mode: interrupt mode
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_enable_isr(int fd, enum INTMODE mode)
{
    return ioctl(fd, CH36x_ENABLE_INT, (unsigned long)&mode);
}

/**
 * ch36x_disable_isr - disable ch36x interrupt
 * @fd: file descriptor of ch36x device
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_disable_isr(int fd)
{
    return ioctl(fd, CH36x_DISABLE_INT, NULL);
}

/**
 * ch36x_set_int_routine - set interrupt handler
 * @fd: file descriptor of ch36x device
 * @isr_handler: handler to call when interrupt occurs
 *
 */
void ch36x_set_int_routine(int fd, void *isr_handler)
{
    if (isr_handler != NULL) {
        signal(SIGIO, isr_handler);
    }
}

/**
 * ch36x_set_stream - set spi mode
 * @fd: file descriptor of ch36x device
 * @mode: bit0 on SPI Freq, 0->31.3MHz, 1->15.6MHz
 * 		  bit1 on SPI I/O Pinout, 0->SPI3(SCS/SCL/SDX), 1->SPI4(SCS/SCL/SDX/SDI)
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_set_stream(int fd, unsigned long mode)
{
    return ioctl(fd, CH36x_SET_STREAM, (unsigned long)&mode);
}

/**
 * ch36x_stream_spi - spi transfer
 * @fd: file descriptor of ch36x device
 * @ibuffer: spi buffer to write
 * @len: length to xfer
 * @obuffer: pointer to read buffer
 *
 * The function return 0 if success, others if fail.
 */
int ch36x_stream_spi(int fd, uint8_t *ibuffer, unsigned long ilen, uint8_t *obuffer, unsigned long olen)
{
    struct {
        uint8_t *ibuffer;
        unsigned long ilen;
        uint8_t *obuffer;
        unsigned long olen;
    } ch36x_stream_spi_t;

    if ((ilen < 0) || (ilen > mMAX_BUFFER_LENGTH)) {
        return -1;
    }
    if ((olen < 0) || (olen > mMAX_BUFFER_LENGTH)) {
        return -1;
    }
    ch36x_stream_spi_t.ibuffer = ibuffer;
    ch36x_stream_spi_t.ilen = ilen;
    ch36x_stream_spi_t.obuffer = obuffer;
    ch36x_stream_spi_t.olen = olen;

    return ioctl(fd, CH36x_STREAM_SPI, (unsigned long)&ch36x_stream_spi_t);
}
