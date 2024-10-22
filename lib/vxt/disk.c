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

#include <vxt/vxtu.h>

#define SECTOR_SIZE 512
#define WAIT_STATES 1000

struct drive {
    void *fp;
    int size;
    bool is_hd;

    vxt_byte buffer[SECTOR_SIZE];

    vxt_word cylinders;
    vxt_word sectors;
    vxt_word heads;

    vxt_byte ah;
    vxt_word cf;
};

struct disk {
    struct vxtu_disk_interface intrf;

	vxt_byte boot_drive;
    vxt_byte num_hd;

    void (*activity_cb)(int,void*);
    void *activity_cb_data;

    struct drive disks[0x100];
};

static vxt_byte execute_operation(vxt_system *s, struct disk *c, vxt_byte disk, bool read, vxt_pointer addr,
    vxt_word cylinders, vxt_word sectors, vxt_word heads, vxt_byte count)
{
    if (!sectors)
	    return 0;

    struct vxtu_disk_interface *di = &c->intrf;
    struct drive *dev = &c->disks[disk];

    int lba = ((int)cylinders * (int)dev->heads + (int)heads) * (int)dev->sectors + (int)sectors - 1;
    if (di->seek(s, dev->fp, lba * SECTOR_SIZE, VXTU_SEEK_START))
        return 0;
    else if (c->activity_cb)
        c->activity_cb((int)disk, c->activity_cb_data);

    int num_sectors = 0;
    while (num_sectors < count) {
        if (read) {
            if (di->read(s, dev->fp, dev->buffer, SECTOR_SIZE) != SECTOR_SIZE)
                break;
            for (int i = 0; i < SECTOR_SIZE; i++)
                vxt_system_write_byte(s, addr++, dev->buffer[i]);
        } else {
            for (int i = 0; i < SECTOR_SIZE; i++)
                dev->buffer[i] = vxt_system_read_byte(s, addr++);
            if (di->write(s, dev->fp, dev->buffer, SECTOR_SIZE) != SECTOR_SIZE)
                break;
        }
        num_sectors++;
    }

    // NOTE: File interface must be flushed at this point!

    return num_sectors;
}

static void execute_and_set(vxt_system *s, struct disk *c, bool read) {
    struct vxt_registers *r = vxt_system_registers(s);
    struct drive *d = &c->disks[r->dl];
	if (!d->fp) {
        r->ah = 1;
        r->flags |= VXT_CARRY;
	} else {
        r->al = execute_operation(s, c, r->dl, read, VXT_POINTER(r->es, r->bx), (vxt_word)r->ch + (r->cl / 64) * 256, (vxt_word)r->cl & 0x3F, (vxt_word)r->dh, r->al);
        r->ah = 0;
        r->flags &= ~VXT_CARRY;
	}
}

static void bootstrap(vxt_system *s, struct disk *c) {
    struct drive *d = &c->disks[c->boot_drive];
    if (!d->fp) {
        VXT_LOG("No bootdrive!");
        return;
    }

    struct vxt_registers *r = vxt_system_registers(s);
    r->dl = c->boot_drive;
    r->al = execute_operation(s, c, c->boot_drive, true, VXT_POINTER(0x0, 0x7C00), 0, 1, 0, 1);
}

static vxt_byte in(struct disk *c, vxt_word port) {
    switch (port) {
        case 0xB0:
            return (c->boot_drive >= 0x80) ? 0 : 0xFF;
        case 0xB1:
            return c->disks[vxt_system_registers(VXT_GET_SYSTEM(c))->dl].fp ? 0 : 0xFF;
        default:
            return 0xFF;
    }
}

static void out(struct disk *c, vxt_word port, vxt_byte data) {
    (void)data;
    vxt_system *s = VXT_GET_SYSTEM(c);
    struct vxt_registers *r = vxt_system_registers(s);

    // Simulate delay for accessing disk controller.
    vxt_system_wait(s, WAIT_STATES);

    switch (port) {
        case 0xB0:
            bootstrap(s, c);
            break;
        case 0xB1:
        {
            struct drive *d = &c->disks[r->dl];
            switch (r->ah) {
                case 0: // Reset
                    r->ah = 0;
                    r->flags &= ~VXT_CARRY;
                    break;
                case 1: // Return status
                    r->ah = d->ah;
                    r->flags = d->cf ? (r->flags | VXT_CARRY) : (r->flags & ~VXT_CARRY);
                    return;
                case 2: // Read sector
                    execute_and_set(s, c, true);
                    break;
                case 3: // Write sector
                    execute_and_set(s, c, false);
                    break;
                case 4: // Format track
                case 5:
                    r->ah = 0;
                    r->flags &= ~VXT_CARRY;
                    break;
                case 8: // Drive parameters
                    if (!d->fp) {
                        r->ah = 0xAA;
                        r->flags |= VXT_CARRY;
                    } else {
                        r->ah = 0;
                        r->flags &= ~VXT_CARRY;
                        r->ch = (vxt_byte)d->cylinders - 1;
                        r->cl = (vxt_byte)((d->sectors & 0x3F) + (d->cylinders / 256) * 64);
                        r->dh = (vxt_byte)d->heads - 1;

                        if (r->dl < 0x80) {
                            r->bl = 4;
                            r->dl = 2;
                        } else {
                            r->dl = (vxt_byte)c->num_hd;
                        }
                    }
                    break;
                default:
                    r->flags |= VXT_CARRY;
            }

            d->ah = r->ah;
            d->cf = r->flags & VXT_CARRY;
            if (d->is_hd)
                vxt_system_write_byte(s, VXT_POINTER(0x40, 0x74), r->ah);
            break;
        }
    }
}

static vxt_error install(struct disk *c, vxt_system *s) {
    struct vxt_peripheral *p = VXT_GET_PERIPHERAL(c);

    // IO 0xB0, 0xB1 to interrupt 0x19, 0x13
    vxt_system_install_io(s, p, 0xB0, 0xB1);
    c->boot_drive = 0;

    return VXT_NO_ERROR;
}

static vxt_error reset(struct disk *c, struct disk *state) {
    if (state)
        return VXT_CANT_RESTORE;
    for (int i = 0; i < 0x100; i++) {
        struct drive *d = &c->disks[i];
        d->ah = 0; d->cf = 0;
    }
    return VXT_NO_ERROR;
}

static const char *name(struct disk *c) {
    (void)c; return "Disk Controller";
}

VXT_API void vxtu_disk_set_activity_callback(struct vxt_peripheral *p, void (*cb)(int,void*), void *ud) {
    struct disk *c = VXT_GET_DEVICE(disk, p);
    c->activity_cb = cb;
    c->activity_cb_data = ud;
}

VXT_API void vxtu_disk_set_boot_drive(struct vxt_peripheral *p, int num) {
    (VXT_GET_DEVICE(disk, p))->boot_drive = num & 0xFF;
}

VXT_API bool vxtu_disk_unmount(struct vxt_peripheral *p, int num) {
    struct disk *c = VXT_GET_DEVICE(disk, p);
    struct drive *d = &c->disks[num & 0xFF];
    bool has_disk = d->fp != NULL;
    d->fp = NULL;
    if (d->is_hd)
        c->num_hd--;
    return has_disk;
}

VXT_API vxt_error vxtu_disk_mount(struct vxt_peripheral *p, int num, void *fp) {
    struct disk *c = VXT_GET_DEVICE(disk, p);
    vxt_system *s = VXT_GET_SYSTEM(c);

    if (!fp)
        return c->disks[num & 0xFF].fp ? VXT_USER_ERROR(0) : VXT_NO_ERROR;

    int size = 0;
    if (c->intrf.seek(s, fp, 0, VXTU_SEEK_END))
        return VXT_USER_ERROR(1);
    if ((size = c->intrf.tell(s, fp)) < 0)
        return VXT_USER_ERROR(2);
    if (c->intrf.seek(s, fp, 0, VXTU_SEEK_START))
        return VXT_USER_ERROR(3);

    if ((size > 1474560) && (num < 0x80)) {
        VXT_LOG("Invalid harddrive number, expected 128+");
        return VXT_USER_ERROR(4);
    }

    struct drive *d = &c->disks[num & 0xFF];
    if (d->fp) {
        vxtu_disk_unmount(p, num);
    }

	if (num >= 0x80) {
        d->cylinders = size / (63 * 16 * 512);
		d->sectors = 63;
		d->heads = 16;
        d->is_hd = true;
		c->num_hd++;
	} else {
		d->cylinders = 80;
		d->sectors = 18;
		d->heads = 2;
        d->is_hd = false;

		if (size <= 1228800) d->sectors = 15;
		if (size <= 737280) d->sectors = 9;
		if (size <= 368640) {
			d->cylinders = 40;
			d->sectors = 9;
		}
		if (size <= 163840) {
			d->cylinders = 40;
			d->sectors = 8;
			d->heads = 1;
		}
	}

	d->fp = fp;
    d->size = size;
    d->cf = d->ah = 0;
    return VXT_NO_ERROR;
}

VXT_API struct vxt_peripheral *vxtu_disk_create(vxt_allocator *alloc, const struct vxtu_disk_interface *intrf) VXT_PERIPHERAL_CREATE(alloc, disk, {
    DEVICE->intrf = *intrf;

    PERIPHERAL->install = &install;
    PERIPHERAL->reset = &reset;
    PERIPHERAL->name = &name;
    PERIPHERAL->io.in = &in;
    PERIPHERAL->io.out = &out;
})
