#!/usr/bin/env python

import sys

off_fp = open(sys.argv[1], 'rb')

while True:
	data = off_fp.read(4)
	if len(data) != 4:
		break
	print('0x{:05X}'.format(int.from_bytes(data, byteorder='little')))
