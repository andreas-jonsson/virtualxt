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

// References: http://www.brokenthorn.com/Resources/OSDev20.html
//             https://wiki.osdev.org/Floppy_Disk_Controller

#include "vxtp.h"

#include <string.h>
#include <stdio.h>

#define FIFO_SIZE 16
#define SECTOR_SIZE 512

#if 1
    #define LOG(fmt, ...) printf("FDC: " fmt "\n", __VA_ARGS__)
    #define LOGS(str) LOG("%s", (str))
#else
    #define LOG(...)
    #define LOGS(...)
#endif

const vxt_byte disk_command_size[16] = {
    0, 0, 9, 3, 2, 9, 9, 2, 1, 9, 2, 0, 9, 6, 0, 3
};

enum disk_command {
    CMD_READ_TRACK	    = 0x2,
    CMD_SPECIFY		    = 0x3,
    CMD_CHECK_STAT	    = 0x4,
    CMD_WRITE_SECT	    = 0x5,
    CMD_READ_SECT	    = 0x6,
    CMD_CALIBRATE	    = 0x7,
    CMD_CHECK_INT	    = 0x8,
    CMD_WRITE_DEL_S	    = 0x9,
    CMD_READ_ID_S	    = 0xA,
    CMD_READ_DEL_S	    = 0xC,
    CMD_FORMAT_TRACK	= 0xD,
    CMD_SEEK		    = 0xF
};

enum {
	DOR_MASK_DRIVE0			= 0x00,
	DOR_MASK_DRIVE1			= 0x01,
	DOR_MASK_DRIVE2			= 0x02,
	DOR_MASK_DRIVE3			= 0x03,
	DOR_MASK_RESET			= 0x04,
	DOR_MASK_DMA			= 0x08,
	DOR_MASK_DRIVE0_MOTOR	= 0x10,
	DOR_MASK_DRIVE1_MOTOR	= 0x20,
	DOR_MASK_DRIVE2_MOTOR	= 0x40,
	DOR_MASK_DRIVE3_MOTOR	= 0x80
};

enum {
	MSR_MASK_DRIVE1_POS_MODE	= 0x01,
	MSR_MASK_DRIVE2_POS_MODE	= 0x02,
	MSR_MASK_DRIVE3_POS_MODE	= 0x04,
	MSR_MASK_DRIVE4_POS_MODE	= 0x08,
	MSR_MASK_BUSY			    = 0x10,
	MSR_MASK_DMA			    = 0x20,
	MSR_MASK_DATAIO			    = 0x40,
	MSR_MASK_DATAREG		    = 0x80
};

enum status0 {
    STATUS0_INT_NORMAL			= 0x00,
    STATUS0_HD					= 0x04,
    STATUS0_NR					= 0x08,
    STATUS0_UC					= 0x10,
    STATUS0_SE					= 0x20,
    STATUS0_INT_ABNORMAL		= 0x40,
    STATUS0_INT_INVALID			= 0x80,
    STATUS0_INT_ABNORMAL_POLL	= 0xC0
};

enum status1 {
    STATUS1_NID		= 0x01,
    STATUS1_NW		= 0x02,
    STATUS1_NDAT	= 0x04,
    STATUS1_TO		= 0x10,
    STATUS1_DE		= 0x20,
    STATUS1_EN		= 0x80
};

enum status2 {
    STATUS2_NDAM    = 0x01,
    STATUS2_BCYL    = 0x02,
    STATUS2_SERR    = 0x04,
    STATUS2_SEQ	    = 0x08,
    STATUS2_WCYL    = 0x10,
    STATUS2_CRCE    = 0x20,
    STATUS2_DADM    = 0x40
};

enum status3 {
    STATUS3_HDDR    = 0x04,
    STATUS3_DSDR    = 0x08,
    STATUS3_TRK0    = 0x10,
    STATUS3_RDY     = 0x20,
    STATUS3_WPDR    = 0x40,
    STATUS3_ESIG    = 0x80
};

struct drive_position {
    vxt_byte track;
	vxt_byte sector;
    vxt_byte head;

    vxt_byte request_track;

    bool is_reading;
    bool is_seeking;
    bool is_transfer;
};

struct diskette {
    vxt_byte tracks;
	vxt_byte sectors;
	vxt_byte heads;

    FILE *fp;
    int size;
};

VXT_PIREPHERAL(fdc, {
    vxt_system *s;
    vxt_word base;
    int irq;

    vxt_byte fifo[FIFO_SIZE];
    int fifo_len;

    struct drive_position position[4];
    struct diskette floppy[4];

    vxt_byte sector_buffer[SECTOR_SIZE];
    int sector_read_index;

    vxt_byte command[9];
    int command_len;

    enum status0 status_reg0;
    enum status1 status_reg1;
    enum status2 status_reg2;
    enum status3 status_reg3;

    vxt_byte drive_num;
    vxt_byte dor_reg;
    bool is_busy;
    bool use_dma;
})

static vxt_byte fifo_read(struct fdc *c) {
    if (c->fifo_len) {
        vxt_byte data = c->fifo[--c->fifo_len];
        memmove(c->fifo, &c->fifo[1], c->fifo_len);
        return data;
    }
    LOGS("Reading empty FIFO!");
    return 0;
}

static void fifo_write(struct fdc *c, vxt_byte data) {
    if (c->fifo_len < FIFO_SIZE)
        c->fifo[c->fifo_len++] = data;
}

static void execute_command(struct fdc *c) {
    LOG("Execute command: %d", *c->command & 0xF);

    c->fifo_len = 0;
    c->is_busy = false;
    c->status_reg0 = STATUS0_INT_NORMAL | (c->floppy[c->drive_num].fp ? 0 : STATUS0_NR) | (c->position[c->drive_num].head << 2) | c->drive_num;

	switch (*c->command & 0xF) {
        case CMD_READ_TRACK:
            break;
        case CMD_SPECIFY:
            c->use_dma = (c->command[2] & 1) != 0;
            break;
        case CMD_CHECK_STAT:
            break;
        case CMD_WRITE_SECT:
            break;
        case CMD_READ_SECT:
        {
            struct drive_position *drive = &c->position[c->command[1] & 3];
            drive->request_track = c->command[2];
            drive->head = c->command[3];
            drive->sector = c->command[4];
            drive->is_reading = drive->is_transfer = false;
            drive->is_seeking = c->is_busy = true;
            break;
        }
        case CMD_CALIBRATE:
        {
            struct drive_position *drive = &c->position[c->command[1] & 3];
            drive->request_track = 0;
            drive->is_seeking = c->is_busy = true;
            break;
        }
        case CMD_CHECK_INT:
            fifo_write(c, (vxt_byte)c->status_reg0);
            fifo_write(c, c->position[c->drive_num].track);
            break;
        case CMD_WRITE_DEL_S:
            break;
        case CMD_READ_ID_S:
            break;
        case CMD_READ_DEL_S:
            break;
        case CMD_FORMAT_TRACK:
            break;
        case CMD_SEEK:
        {
            struct drive_position *drive = &c->position[c->command[1] & 3];
            drive->head = (c->command[1] >> 2) & 1;
            drive->request_track = c->command[2];
            drive->is_seeking = c->is_busy = true;
            break;
        }
        default:
            c->status_reg0 = STATUS0_INT_INVALID;
            fifo_write(c, (vxt_byte)c->status_reg0);
	}

	c->command_len = 0;
}

static void push_command_byte(struct fdc *c, vxt_byte data) {
    if (c->is_busy) return;
    if (c->command_len < 9) c->command[c->command_len++] = data;
	if (c->command_len < (disk_command_size[*c->command & 0xF])) return;
    execute_command(c);
}

static void controller_reset(struct fdc *c) {
    LOGS("Controller reset!");
    c->fifo_len = c->command_len = 0;
    c->status_reg0 = (c->status_reg0 & 3) | STATUS0_INT_ABNORMAL_POLL;
    vxt_system_interrupt(c->s, c->irq);
}

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(c, fdc, p);
    switch (port - c->base) {
        case 4: // Main Status Register (MSR)
        {
            vxt_byte status = MSR_MASK_DATAREG;
            if (c->is_busy) {
                LOGS("BUSY!");
                status = MSR_MASK_BUSY;
            } else if (c->fifo_len) {
                status = MSR_MASK_DATAREG | MSR_MASK_DATAIO | MSR_MASK_BUSY;
            //} else if (c->command_len && (c->command_len < disk_command_size[*c->command & 0xF])) {
            //    status = MSR_MASK_DATAIO;
            }

            //if (c->command_len)
            //    status |= MSR_MASK_BUSY;

            status |= c->use_dma ? 0 : MSR_MASK_DMA;
            for (int i = 0; i < 4; i++) {
                if (c->position[i].is_seeking)
                    status |= 1 << i;
            }

            LOG("STATUS: 0x%X", status);
            return status;
        }
        case 5: // Data Register
        {
            vxt_byte data = fifo_read(c);
            LOG("FIFO Read: 0x%X", data);
            return data;
        }
    }
    return 0;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(c, fdc, p);
    switch (port - c->base) {
        case 2: // Digital Output Register (DOR)
            LOG("DOR: 0x%X", data);
        	c->drive_num = data & 0x3;
            c->status_reg0 = (c->status_reg0 & 0xFC) | c->drive_num;
            c->status_reg3 = (enum status3)c->status_reg0;//(c->status_reg0 & 0xFC) | c->drive_num;
            c->use_dma = (data & DOR_MASK_DMA) != 0;
            //if (!(c->dor_reg & DOR_MASK_RESET) && (data & DOR_MASK_RESET))
            if (data & DOR_MASK_RESET)
                controller_reset(c);
            c->dor_reg = data;
            break;
        case 5:	// Data Register
            LOG("Push command: 0x%X", data);
            push_command_byte(c, data);
            break;
    }
}
/*
static void update_seek(struct fdc *c) {
	for (int i = 0; i < 4; i++) {
        struct drive_position *drive = &c->position[i];
        if (!drive->is_seeking)
            continue;

        if (drive->track < drive->request_track) {
            drive->track++;
        } else if (drive->track > drive->request_track) {
            drive->track--;
        } else {
            drive->is_seeking = false;
            switch (*c->command & 0x0F) {
                case CMD_READ_SECT:
                    drive->is_reading = true;
                    drive->is_transfer = false;
                    break;
                case CMD_CALIBRATE:
                case CMD_SEEK:
                    c->is_busy = false;
                    c->status_reg0 = STATUS0_INT_NORMAL | STATUS0_SE | (drive->head << 2) | i;
                    vxt_system_interrupt(c->s, c->irq);
                    break;
            }
            return;
        }

        if ((drive->track >= c->floppy[i].tracks) || !c->floppy[i].fp) {
            drive->is_seeking = c->is_busy = false;
            c->status_reg0 = STATUS0_UC | STATUS0_INT_ABNORMAL | (drive->head << 2) | i;
            vxt_system_interrupt(c->s, c->irq);
            return;
        }
	}
}

static void update_transfer(struct fdc *c) {
	for (int i = 0; i < 4; i++) {
        struct drive_position *drive = &c->position[i];
		if (drive->is_transfer) {
			if (c->sector_read_index < SECTOR_SIZE) {
				if (c->use_dma) {
					// TODO!!!
                    fprintf(stderr, "No DMA implemented!\n");
				} else {
					if (!c->fifo_len) {
						//c->fifo_len = 0;
						fifo_write(c, c->sector_buffer[c->sector_read_index++]);
						vxt_system_interrupt(c->s, c->irq);
                        return;
					}
				}
			} else if (c->sector_read_index == SECTOR_SIZE) {
				c->status_reg0 = STATUS0_INT_NORMAL | STATUS0_SE | (drive->head << 2) | i;
				c->status_reg1 = 0;
                c->status_reg2 = 0;
                c->is_busy = drive->is_reading = drive->is_transfer = false;

                c->fifo_len = 0;
				fifo_write(c, (vxt_byte)c->status_reg0);
				fifo_write(c, (vxt_byte)c->status_reg1);
				fifo_write(c, (vxt_byte)c->status_reg2);
				fifo_write(c, drive->track);
				fifo_write(c, drive->head);
				fifo_write(c, drive->sector);
				fifo_write(c, 2);

				vxt_system_interrupt(c->s, c->irq);
				return;
			}
		} else if (drive->is_reading) {
            FILE *fp = c->floppy[i].fp;
            int sz = c->floppy[i].sectors * SECTOR_SIZE;
			int lba = drive->track * sz * 2 + drive->head * sz + (drive->sector - 1) * SECTOR_SIZE;

			fseek(fp, SEEK_SET, lba);
			fread(c->sector_buffer, 1, SECTOR_SIZE, fp);

			drive->is_transfer = true;
            c->sector_read_index = c->fifo_len = 0;
		}
	}
}

static vxt_error step(struct vxt_pirepheral *p, int cycles) {
    (void)cycles;
    VXT_DEC_DEVICE(c, fdc, p);
    // TODO: Don't do this every step!
    update_seek(c);
    update_transfer(c);
    return VXT_NO_ERROR;
}
*/
static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(c, fdc, p);
    c->s = s;
    vxt_system_install_io(s, p, c->base, c->base + 5);
    vxt_system_install_io_at(s, p, c->base + 7);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(fdc, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "FDC (NEC 765)";
}

struct vxt_pirepheral *vxtp_fdc_create(vxt_allocator *alloc, vxt_word base, int irq) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(fdc));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(fdc));
    VXT_DEC_DEVICE(c, fdc, p);

    c->base = base;
    c->irq = irq;

    p->install = &install;
    p->destroy = &destroy;
    p->name = &name;
    //p->step = &step;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

bool vxtp_fdc_unmount(struct vxt_pirepheral *p, int num) {
    VXT_DEC_DEVICE(c, fdc, p);
    if (num > 3)
        return false;

    struct diskette *d = &c->floppy[num];
    bool has_disk = d->fp != NULL;
    d->fp = NULL;
    return has_disk;
}

vxt_error vxtp_fdc_mount(struct vxt_pirepheral *p, int num, FILE *fp) {
    VXT_DEC_DEVICE(c, fdc, p);

    if (num > 3)
        return VXT_USER_ERROR(0);

    if (fseek(fp, 0, SEEK_END))
        return VXT_USER_ERROR(1);

    long size = 0;
    if ((size = ftell(fp)) < 0)
        return VXT_USER_ERROR(2);

    if (fseek(fp, 0, SEEK_SET))
        return VXT_USER_ERROR(3);

    if (size > 1474560) {
        fprintf(stderr, "invalid floppy image size\n");
        return VXT_USER_ERROR(4);
    }

    struct diskette *d = &c->floppy[num];
    if (d->fp)
        vxtp_disk_unmount(p, num);

    d->tracks = 80;
    d->sectors = 18;
    d->heads = 2;

    if (size <= 1228800) d->sectors = 15;
    if (size <= 737280) d->sectors = 9;
    if (size <= 368640) {
        d->tracks = 40;
        d->sectors = 9;
    }
    if (size <= 163840) {
        d->tracks = 40;
        d->sectors = 8;
        d->heads = 1;
    }

	d->fp = fp;
    d->size = (int)size;
    return VXT_NO_ERROR;
}
