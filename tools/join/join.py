#!/usr/bin/env python

import io, sys

even = io.open(sys.argv[1], 'rb').read()
odd = io.open(sys.argv[2], 'rb').read()

for j in zip(even, odd):
    for i in j:
        sys.stdout.buffer.write(i.to_bytes(1, 'little'))
