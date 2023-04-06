#pragma once

#include <stdint.h>
#include <stdbool.h>

// Internal types and stuff that is needed in the header for declarations,
// but is not a part of the public API.

/** Boot Sector structure */
typedef struct __attribute__((packed))
{
	// Fields loaded directly from disk:

	// 13 bytes skipped
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t root_entries;
	// 3 bytes skipped
	uint16_t fat_size_sectors;
	// 8 bytes skipped
	uint32_t total_sectors; // if "short size sectors" is used, it's copied here too
	// 7 bytes skipped
	char volume_label[11]; // space padded, no terminator

	// Added fields:

	uint32_t bytes_per_cluster;

}
Fat16BootSector;


/** FAT filesystem handle */
typedef struct __attribute__((packed))
{
	// Backing block device
	const BLOCKDEV* dev;

	// Root directory sector start
	uint32_t rd_addr;

	// Start of first cluster (number "2")
	uint32_t data_addr;

	// Start of fat table
	uint32_t fat_addr;

	// Boot sector data struct
	Fat16BootSector bs;
}
FAT16;


/**
 * File Attributes (bit flags)
 * Accessible using file->attribs
 */
#define FA_READONLY 0x01 // read only file
#define FA_HIDDEN   0x02 // hidden file
#define FA_SYSTEM   0x04 // system file
#define FA_LABEL    0x08 // volume label entry, found only in root directory.
#define FA_DIR      0x10 // subdirectory
#define FA_ARCHIVE  0x20 // archive flag
