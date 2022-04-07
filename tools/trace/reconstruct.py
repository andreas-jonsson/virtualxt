#!/usr/bin/env python

import sys

def main():
    src_fp = open(sys.argv[1], 'r')
    off_fp = open(sys.argv[2], 'rb')

    next_instruction = 0

    while 1:
        parts = src_fp.readline().split()
        if len(parts) == 0:
            return

        size = int(len(parts[1]) / 2)
        new_offset = int.from_bytes(off_fp.read(4), byteorder='little')
        flags = off_fp.read(1)[0]
        off_fp.read((size-1)*4+(size-1)) # Discard

        if flags == 0:
            print(end = ' ')
        else:
            print(end = 'i')

        if next_instruction != new_offset:
            print(end = '*')
        else:
            print(end = ' ')
        next_instruction = new_offset + size

        if new_offset < 0xE0000:
            print(end = 'RAM> ')
        else:
            print(end = 'ROM> ')

        print('{:X}'.format(new_offset), end = '\t')
        for v in parts[1:]:
            print(v, end = '\t')
        print()
      
main()