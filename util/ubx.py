#!/usr/bin/env python3

import sys
import struct
import serial

def u1(data, offset):
    return struct.unpack('<B', data[offset:offset + 1])[0]

def u2(data, offset):
    return struct.unpack('<H', data[offset:offset + 2])[0]

def u4(data, offset):
    return struct.unpack('<I', data[offset:offset + 4])[0]

def i4(data, offset):
    return struct.unpack('<i', data[offset:offset + 4])[0]

def i8(data, offset):
    return struct.unpack('<q', data[offset:offset + 8])[0]

class Parser:
    UBX_SYS = ['GPS', 'SBS', 'GAL', 'CMP', 'QZS', 'GLO']
    MINPRNSBS = 120
    CLIGHT    = 299792458.0                 # speed of light (m/s)
    P2_10     = 0.0009765625                # 2^-10
    P2_31     = 4.656612873077393E-10
    P2_32     = 2.328306436538696E-10       # 2^-32
    P2_43     = 1.136868377216160E-13
    P2_55     = 2.775557561562891E-17       # 2^-55
    PRN_RANGES = (
        ('GPS', 1, 32),
        ('GLO', 1, 24)
    )
    PRN_GPS = (1, 32)
    PRN_GLO = (1, 24)

    def __init__(self):
        self.state = 0
        self.subfrm = dict()
        pass

    def ubx_sys(self, value):
        if value >= 0 and value < len(self.UBX_SYS):
            return self.UBX_SYS[value]
        return '[%d]' % value

    def satno(self, prn, sys):
        return '%s:%03d' % (sys, prn)
        if prn <= 0: return 0
        prn_base = 0
        for (test_sys, prn_min, prn_max) in self.PRN_RANGES:
            if sys == test_sys:
                if prn < prn_min or prn > prn_max:
                    return 0
                return prn_base + (prn - prn_min) + 1
            prn_base += (prn_max - prn_min) + 1
        return 0

    def decode_subfrm1(self, frm):
        tow = int(frm[24:24+17], 2) * 6.0
        week = int(frm[48:58],  2)  # 10 bits
        code = int(frm[58:60],  2)  # 2  bits
        sva  = int(frm[60:64],  2)  # 4  bits
        svh  = int(frm[64:70],  2)  # 6  bits
        iodc0 = int(frm[70:72], 2) # 2  bits
        flag = int(frm[72:73],  2)  # 1  bit
        tgd  = int(frm[160:168], 2) # 8 bits
        iodc1  = int(frm[168:176], 2) # 8 bits
        toc  = int(frm[176:192], 2) * 16.0 # 16 bits
        f2   = int(frm[192:200], 2) * self.P2_55    # 8 bits
        f1   = int(frm[200:216], 2) * self.P2_43    # 16 bits
        f0   = int(frm[216:238], 2) * self.P2_31    # 22 bits
        iodc = (iodc0 << 8) | iodc1
        print code, sva, svh, flag, f2, f1, f0, tgd, iodc
        return
    def decode_subfrm2(self, frm):
        return
    def decode_subfrm3(self, frm):
        return

    def decode_nav(self, sat, block):
        ''' decode gps and qzss navigation data '''
        if len(block) < 40:
            print "ERROR (block length %d)" % len(block)
            return
        words = [u4(block, i * 4) >> 6 for i in range(10)]
        frm = ''.join([format(x & 0xFFFFFF, '024b') for x in words])

        sf_id = (words[1] >> 2) & 0x0007
        if (sf_id < 1) or (sf_id > 5):
            print "ERROR (subframe id = %d)" % sf_id
            return

        #key = (sat, sf_id)
        #self.subfrm[key] = frm

        print "sat: %s sf_id: %d (%s)" % (sat, sf_id, frm[43:46])
        if sf_id == 1:
            self.decode_subfrm1(frm)
        if sf_id == 2:
            self.decode_subfrm2(frm)
        if sf_id == 3:
            self.decode_subfrm3(frm)
        pass

    def decode_gnav(self, sat, block, frq):
        pass

    def parse_trk_sfrbx(self, payload):
        sys = self.ubx_sys(u1(payload, 1))
        prn = u1(payload, 2)
        if sys == 'QZS': 
            prn += 192

        sat = self.satno(prn, sys)
        #print 'sat: %3d prn: %3d sys: %s' % (sat, prn, sys)
        if sys == 'GPS':
            self.decode_nav(sat, payload[13:])
        elif sys == 'GLO':
            frq = u1(payload, 4)
            self.decode_gnav(sat, payload[13:], frq)
        print

    def parse_trk_trkd5(self, payload):
        msg_type = u1(payload, 0)
        #print 'msg_type:', msg_type

        if msg_type == 6:
            block_offset = 80
            block_length = 64

        #print "len: %d = 80 + %d * 64 + %d" % (len(payload), (len(payload) - 80)/64, (len(payload) - 80) % 64)
        tr = -1.0
        for offset in range(block_offset, len(payload) - block_length + 1, block_length):
            qi = u1(payload, offset + 41) & 7
            if qi < 4: continue
            t = i8(payload, offset)*self.P2_32/1000.0
            if msg_type == 6:
                sys = self.ubx_sys(u1(payload, offset + 56))
                if sys=='GLO': 
                    t -= 10800.0
            if sys != 'GPS':
                continue
            if t > tr: tr = t

        if tr < 0:
            return

        tr = 0.1 * int(0.5 + (tr + 0.08)/0.1)

        obs_data = list()
        for offset in range(block_offset, len(payload) - block_length + 1, block_length):
            #print 'offset:', offset
            qi = u1(payload, offset + 41) & 7
            #print "qi:", qi
            if (qi < 4) or (qi > 7):
                continue
            if msg_type == 6:
                sys = self.ubx_sys(u1(payload, offset + 56))
                prn = u1(payload, offset + 57)
                if sys == 'QZS': 
                    prn += 192
                #frq = u1(payload, offset + 59) - 7
                #print 'frq:', frq
            else:
                prn = u1(payload, offset + 34)
                if prn < self.MINPRNSBS:
                    sys = 'GPS'
                else:
                    sys = 'SBS'

            sat = self.satno(prn, sys)

            ts = i8(payload, offset)*self.P2_32/1000.0
            #if sys == 'GLO': 
            #    ts -= 10800.0

            if qi >= 4:
                tau = tr-ts
                if      (tau<-302400.0): tau+=604800.0
                elif    (tau> 302400.0): tau-=604800.0
            else:
                tau = 0
            
            flag = u1(payload, offset + 54)
            if qi < 6:
                adr = 0
            else:
                adr = i8(payload, offset + 8) * self.P2_32
                if flag & 0x01:
                    adr += 0.5
            D = i4(payload, offset + 16) * self.P2_10 / 4.0
            snr = u2(payload, offset + 32) / 256.0

            P = tau * self.CLIGHT

            entry = {
                'qi': qi,
                'sat': sat,
                'prn': prn,
                'sys': sys,
                'snr': snr,
                'tau': tau,
                'ts' : ts,
                'P': P,
                'D': D,
                'L': -adr
            }
            obs_data.append(entry)
        
        for entry in sorted(obs_data, key = lambda x: x['sat']):
            line1 = 'sat: %s qi: %1d snr: %4.1f' % (entry['sat'], entry['qi'], entry['snr'])
            line2 = 'ts: %.6f P: %.1f D: %8.1f L: %11.1f' % (entry['ts'], entry['P'], entry['D'], entry['L'])
            print line1, line2

        print
        #print ' '.join(['%02X' % x for x in payload])


    def parse_frame(self, frame):
        #print ' '.join(['%02X' % x for x in frame])
        (m_cls, m_id) = frame[0:2]
        payload = frame[4:-2]
        if m_cls == 0x03 and m_id == 0x0A:
            print "TRK-TRKD5"
            self.parse_trk_trkd5(payload)
        if m_cls == 0x03 and m_id == 0x0F:
            print "TRK-SFRBX"
            self.parse_trk_sfrbx(payload)

    def parse_byte(self, b):
        if self.state == 0:
            if b == 0xB5:
                self.state = 1
        elif self.state == 1:
            if b == 0x62:
                self.state = 2
                self.frame_data = bytearray()
            else:
                self.state = 0
        elif self.state == 2:
            self.frame_data.append(b)
            if len(self.frame_data) == 4:
                self.payload_length = self.frame_data[-2] + 256 * self.frame_data[-1]
            if len(self.frame_data) >= 4:
                if len(self.frame_data) == self.payload_length + 6:
                    self.parse_frame(self.frame_data)
                    self.state = 0
        return

port_name = sys.argv[1]
port_speed = 115200

input = serial.Serial(port_name, port_speed)
parser = Parser()

while True:
    data = ''
    while True:
        data += input.read(1)
        if data[-1] == ' ':
            break
    parser.parse_byte(int(data[:-1], 16))
