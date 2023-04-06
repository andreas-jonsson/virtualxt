#pragma once

//
// Simple FAT16 library.
//
// To use it, implement BLOCKDEV functions
// and attach them to it's instance.
//

#include <stdint.h>
#include <stdbool.h>

#include "blockdev.h"


// -------------------------------

/**
 * File types (values can be used for debug printing).
 * Accessible using file->type
 */
typedef enum
{
	FT_NONE = '-',
	FT_DELETED = 'x',
	FT_SUBDIR = 'D',
	FT_PARENT = 'P',
	FT_LABEL = 'L',
	FT_LFN = '~',
	FT_INVALID = '?', // not recognized weird file
	FT_SELF = '.',
	FT_FILE = 'F'
} FAT16_FT;


/** "File address" for saving and restoring file */
typedef struct
{
	uint16_t clu;
	uint16_t num;
	uint32_t cur_rel;
} FSAVEPOS;


// Include definitions of fully internal structs
#include "fat16_internal.h"


/**
 * File handle struct.
 *
 * File handle contains cursor, file name, type, size etc.
 * Everything (files, dirs) is accessed using this.
 */
typedef struct __attribute__((packed))
{
	/**
	 * Raw file name. Starting 0x05 was converted to 0xE5.
	 * To get PRINTABLE file name, use fat16_dispname()
	 */
	uint8_t name[11];

	/**
	 * File attributes - bit field composed of FA_* flags
	 * (internal)
	 */
	uint8_t attribs;

	// 14 bytes skipped (10 reserved, date, time)

	/** First cluster of the file. (internal) */
	uint16_t clu_start;

	/**
	 * File size in bytes.
	 * This is the current allocated and readable file size.
	 */
	uint32_t size;


	// --- the following fields are added when reading ---

	/** File type. */
	FAT16_FT type;


	// --- INTERNAL FIELDS ---

	// Cursor variables. (internal)
	uint32_t cur_abs; // absolute position in device
	uint32_t cur_rel; // relative position in file
	uint16_t cur_clu; // cluster where the cursor is
	uint16_t cur_ofs; // offset within the active cluster

	// File position in the directory. (internal)
	uint16_t clu; // first cluster of directory
	uint16_t num; // file entry number

	// Pointer to the FAT16 handle. (internal)
	const FAT16* fat;
}
FFILE;


/**
 * Store modified file metadata and flush it to disk.
 */
void ff_flush_file(FFILE* file);


/**
 * Save a file "position" into a struct, for later restoration.
 * Cursor is also saved.
 */
FSAVEPOS ff_savepos(const FFILE* file);

/**
 * Restore a file from a saved position.
 */
void ff_reopen(FFILE* file, const FSAVEPOS* pos);


/**
 * Initialize the file system - store into "fat"
 */
bool ff_init(const BLOCKDEV* dev, FAT16* fat);


/**
 * Open the first file of the root directory.
 * The file may be invalid (eg. a volume label, deleted etc),
 * or blank (type FT_NONE) if the filesystem is empty.
 */
void ff_root(const FAT16* fat, FFILE* file);


/**
 * Resolve the disk label.
 * That can be in the Boot Sector, or in the first root directory entry.
 *
 * @param fat       the FAT handle
 * @param label_out string to store the label in. Should have at least 12 bytes.
 */
char* ff_disk_label(const FAT16* fat, char* label_out);


// ----------- FILE I/O -------------


/**
 * Move file cursor to a position relative to file start
 * Returns false on I/O error (bad file, out of range...)
 */
bool ff_seek(FFILE* file, uint32_t addr);


/**
 * Read bytes from file into memory
 * Returns number of bytes read, 0 on error.
 */
uint16_t ff_read(FFILE* file, void* target, uint16_t len);


/**
 * Write into file at a "seek" position.
 */
bool ff_write(FFILE* file, const void* source, uint32_t len);


/**
 * Store a 0-terminated string at cursor.
 */
bool ff_write_str(FFILE* file, const char* source);


/**
 * Create a new file in given folder
 *
 * file ... open directory; new file is opened into this handle.
 * name ... name of the new file, including extension
 */
bool ff_newfile(FFILE* file, const char* name);


/**
 * Create a sub-directory of given name.
 * Directory is allocated and populated with entries "." and ".."
 */
bool ff_mkdir(FFILE* file, const char* name);


/**
 * Set new file size.
 * Allocates / frees needed clusters, does NOT erase them.
 *
 * Useful mainly for shrinking.
 */
void set_file_size(FFILE* file, uint32_t size);


/**
 * Delete a *FILE* and free it's clusters.
 */
bool ff_rmfile(FFILE* file);


/**
 * Delete an empty *DIRECTORY* and free it's clusters.
 */
bool ff_rmdir(FFILE* file);


/**
 * Delete a file or directory, even FT_LFN and FT_INVALID.
 * Directories are deleted recursively (!)
 */
bool ff_delete(FFILE* file);



// --------- NAVIGATION ------------


/** Go to previous file in the directory (false = no prev file) */
bool ff_prev(FFILE* file);


/** Go to next file in directory (false = no next file) */
bool ff_next(FFILE* file);


/**
 * Open a subdirectory denoted by the file.
 * Provided handle changes to the first entry of the directory.
 */
bool ff_opendir(FFILE* dir);


/**
 * Open a parent directory. Fails in root.
 * Provided handle changes to the first entry of the parent directory.
 */
bool ff_parent(FFILE* file);


/** Jump to first file in this directory */
void ff_first(FFILE* file);


/**
 * Find a file with given "display name" in this directory, and open it.
 * If file is found, "file" will contain it's handle.
 * Otherwise, the handle is unchanged.
 */
bool ff_find(FFILE* file, const char* name);


// -------- FILE INSPECTION -----------

/** Check if file is a valid entry, or long-name/label/deleted */
bool ff_is_regular(const FFILE* file);


/**
 * Resolve a file name, trim spaces and add null terminator.
 * Returns the passed char*, or NULL on error.
 */
char* ff_dispname(const FFILE* file, char* disp_out);


/**
 * Convert filename to zero-padded fixed length one
 * Returns the passed char*.
 */
char* ff_rawname(const char* disp_in, char* raw_out);

