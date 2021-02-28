#!/usr/bin/env python3
import sys
h = 2166136261
for x in sys.argv[1]:
    h ^= ord(x)
    h *= 16777619
    h &= 0xFFFFFFFF # simulate overflow
print(h, end='')
