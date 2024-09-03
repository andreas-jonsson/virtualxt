#!/usr/bin/env python3

import os
import sys
import struct

size = 0
sum = 0

with open(sys.argv[1], "rb") as f:
	f.seek(0, os.SEEK_END)
	size = f.tell()
	f.seek(0, os.SEEK_SET)

	num = size
	while num > 1:
		sum += int(struct.unpack("B", f.read(1))[0])
		num -= 1

sum = (256 - (sum & 0xFF)) & 0xFF
print("BIOS Checksum: 0x{:02X}".format(sum))

with open(sys.argv[1], "rb+") as f:
	f.seek(size - 1, os.SEEK_SET)
	f.write(struct.pack("B", sum))
