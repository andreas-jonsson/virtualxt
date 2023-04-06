#pragma once

//
// Block device interface, somewhat akin to stream.h
// Used for filesystem implementations.
//

#include <stdint.h>

/** Abstract block device interface
 *
 * Populate an instance of this with pointers to your I/O functions.
 */
typedef struct
{
	/** Sequential read at cursor
	 * @param dest destination memory structure
	 * @param len  number of bytes to load and store in {dest}
	 */
	void (*load)(void* dest, const uint16_t len);


	/** Sequential write at cursor
	 * @param src source memory structure
	 * @param len number of bytes to write
	 */
	void (*store)(const void* src, const uint16_t len);


	/** Write one byte at cursor
	 * @param b byte to write
	 */
	void (*write)(const uint8_t b);


	/** Read one byte at cursor
	 * @return the read byte
	 */
	uint8_t (*read)(void);


	/** Absolute seek - set cursor
	 * @param addr new cursor address
	 */
	void (*seek)(const uint32_t addr);


	/** Relative seek - move cursor
	 * @param offset cursor address change
	 */
	void (*rseek)(const int16_t offset);



	/** Flush the data buffer if it's dirty.
	 *
	 * Should be called after each sequence of writes,
	 * to avoid data loss.
	 *
	 * Tmplementations that do not need this should provide
	 * a no-op function.
	 */
	void (*flush)(void);

} BLOCKDEV;

