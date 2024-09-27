#!/usr/bin/env python

import sys

op_fp = open(sys.argv[1], 'rb')
off_fp = open(sys.argv[2], 'rb')

buffer = bytearray(0x100000)

while True:
	opcode = op_fp.read(1)
	if len(opcode) != 1:
		break
	
	data = off_fp.read(4)
	if len(data) != 4:
		break
	offset = int.from_bytes(data, byteorder='little')
	
	print('\rOpcode {:02X} @ 0x{:05X}'.format(opcode[0], offset), end='')
	buffer[offset] = opcode[0]

print('')

open(sys.argv[3], 'wb').write(buffer)
