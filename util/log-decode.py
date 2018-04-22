#!/usr/bin/env python

import sys
import serial
import struct

def play_log(port_name, port_speed = 115200):
    stream = serial.Serial(port_name, port_speed, timeout = 1.0)
    stream.write('play_log\n')
    stream.flush()

    for line in stream:
        try:
            line = line.rstrip()
            print line
        except SerialTimeoutException:
            break
    return

def decode_start(data):
    timestamp = struct.unpack('<L', data[1:5])[0]
    print timestamp, 'START'
    return 5

def decode_baro(data):
    (timestamp, pressure2, temp) = struct.unpack('<LHb', data[1:8])
    pressure = pressure2 * 2
    print timestamp, 'BARO', pressure, temp
    return 8

def decode_mag(data):
    (timestamp, mag, temp) = struct.unpack('<Lhb', data[1:8])
    print timestamp, 'MAG', mag, temp
    return 8

def decode_gps_time(data):
    (timestamp, hour, minute, second) = struct.unpack('<LBBB', data[1:8])
    time = '%02d:%02d:%02d' % (hour, minute, second)
    print timestamp, 'TIME', time
    return 8

def decode_gps_pos(data):
    (timestamp, alt, lat, lon) = struct.unpack('<Lhll', data[1:15])
    lat /= 1.0E7
    lon /= 1.0E7
    print timestamp, 'POS', alt, lat, lon
    return 15

def decode_log():
    stream = sys.stdin
    data = bytearray()

    for line in stream:
        line = line.rstrip()
        fields = line.split()
        fragment = [int(x, 16) for x in fields]
        data += bytearray(fragment)

    sys.stderr.write('Read %d bytes\n' % len(data))

    idx = 0
    while idx < len(data):
        if data[idx] == 0:
            idx += decode_start(data[idx:])
        elif data[idx] == 1:
            idx += decode_baro(data[idx:])
        elif data[idx] == 2:
            idx += decode_mag(data[idx:])
        elif data[idx] == 3:
            idx += decode_gps_time(data[idx:])
        elif data[idx] == 4:
            idx += decode_gps_pos(data[idx:])
        else:
            print 'UNK'
            idx += 1

    return


#play_log(sys.argv[1])
decode_log()