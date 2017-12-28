#!/usr/bin/env python

import sys

"""
    This program converts EEPROM dump files from MicroPeak flight computer
    to equivalent serial dump files to be used with the MicroPeak software
"""

POLY = 0x8408

def usage():
    sys.stderr.write(
        "Usage: %s INPUT [OUTPUT]\n" % sys.argv[0]
    )

def update_crc(crc, byte):
    for i in range(8):
        if (crc & 0x0001) ^ (byte & 0x0001):
            crc = (crc >> 1) ^ POLY
        else:
            crc = crc >> 1
        byte >>= 1
    return crc

def dump_mp(log, data):
    n_entries = data[8] + data[9] * 256
    n_bytes = 10 + 2 * n_entries

    log.write('MP')
    crc = 0xFFFF
    for b in range(n_bytes):
        if (b & 0x0F) == 0:
            log.write('\r\n')
        log.write('%02x' % data[b])
        crc = update_crc(crc, data[b])

    log.write('\r\n')
    crc = crc ^ 0xFFFF
    log.write('%02x%02x' % (crc >> 8, crc & 0xFF))
    log.write('\r\n')



if len(sys.argv) < 2:
    usage()
    sys.exit(0)

path_hex = sys.argv[1]
hexfile = open(path_hex)
data = [int(x, 16) for x in hexfile.read().split(',')]

if len(sys.argv) > 2:
    path_log = sys.argv[2]
    log = open(path_log, 'wb')
else:
    log = sys.stdout

dump_mp(log, data)
