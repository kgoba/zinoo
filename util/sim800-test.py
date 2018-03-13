#!/usr/bin/env python

import sys
import time
import serial
import codecs
import argparse

GPRS_APN = 'internet.tele2.lv'
HABHUB_TEST1 = '''PUT /habitat/_design/payload_telemetry/_update/add_listener/319ae509b72e0da5676c20f7f59ba344acd6aadce462259a14a3a96c3f9712a1 HTTP/1.1
Host: habitat.habhub.org
User-Agent: couchdbkit/0.6.5
Accept-Encoding: identity
Accept: application/json
Content-Length: 227
Content-Type: application/json

{"data": {"_raw": "JCRzcGFjZXRlY2gyMDE3LDIwMCwyMTQxNTQsNTguMzY2Mzk3LDI2LjY5MDc0MCw3MCwzLDcqREEyMQo="}, "receivers": {"CrazyLatvians": {"time_created": "2018-02-24T21:41:54+02:00", "time_uploaded": "2018-02-24T21:41:54+02:00"}}}
'''

"""
AT+CPIN="PIN_"
AT+CLCK="SC",0,"PIN_"

AT+CREG?
+CREG: 0,1

OK
AT+COPS?
+COPS: 0,0,"TELE2"

OK
AT+CSTT="internet.tele2.lv","",""
OK
AT+CIICR
OK
AT+CIFSR
10.30.242.68
AT+CIPSTART="TCP","exploreembedded.com",80
OK

CONNECT OK
AT+CIPSEND=44
> GET /wiki/images/1/15/Hello.txt HTTP/1.0


SEND OK

AT+CIPRXGET=2,1460
"""

"""
AT+SAPBR=3,1,"APN","internet.tele2.lv"
OK
AT+SAPBR=1,1
OK
AT+HTTPPARA="URL","http://exploreembedded.com/wiki/images/1/15/Hello.txt"
OK
AT+HTTPACTION=0

+HTTPACTION: 0,200,28
AT+HTTPREAD
+HTTPREAD: 28
Welcome to Explore Embedded!
OK
"""

class ATCommandError(Exception):
    pass

class ATCommandTimeout(Exception):
    pass

class SIM800:
    def __init__(self, port, baud = 9600):
        self.tty = serial.Serial(port, baud, inter_byte_timeout = 0.5, timeout = 10.0)

    def _sendline(self, line, newline = True, echo = True):
        print "-->", line
        self.tty.write(line)
        if echo:
            self.tty.read(len(line))
        if newline:
            self.tty.write('\x0A')
            if echo:
                self.tty.read(2)    # 0x0D 0x0A

    def _send_data(self, data, echo = True):
        self.tty.write(data)
        if echo:
            self.tty.read(len(data))

    def _readline(self):
        line = self.tty.readline()
        if len(line) == 0:
            raise ATCommandTimeout()
        #print "<<<", codecs.encode(line, 'hex'), line.rstrip()
        print "<--", line.rstrip()
        return line.rstrip()

    def _read(self, size):
        data = self.tty.read(size)
        #print "<<<", line.rstrip()
        if len(data) < size:
            raise ATCommandTimeout()
        return data

    def _send_command(self, cmd, args = None, read_lines = 1, expected_status = 'OK'):
        self._sendline('AT%s' % cmd)
        response = []
        lines_left = read_lines
        while lines_left > 0:
            response.append( self._readline() )
            lines_left -= 1
        if read_lines > 0 and expected_status:
            if response[-1] != expected_status:
                raise ATCommandError('Error while executing %s' % cmd)
        return response

    def check(self):
        self._send_command('')

    def battery_voltage(self):
        response = self._send_command("+CBC", read_lines = 3)
        if not response[0].startswith('+CBC: '):
            raise ATCommandError('Error while executing %s' % '+CBC')
        fields = response[0][6:].split(',')
        state = int(fields[0])
        charge = int(fields[1])
        voltage = float(fields[2]) / 1000
        return (state, charge, voltage)

    def signal_quality(self):
        response = self._send_command("+CSQ", read_lines = 3)
        if response[0].startswith('+CSQ: '):
            fields = response[0][6:].split(',')
            rssi_level = int(fields[0])
            ber = int(fields[1])

            if rssi_level == 0:
                rssi = -115
            elif rssi_level == 1:
                rssi = -111
            else:
                rssi = -110 + (rssi_level - 2) * 2

            return (rssi, ber)
        return None

    def gprs_start(self, apn, user = '', password = ''):
        self._send_command('+CIPSHUT', expected_status = 'SHUT OK')
        self._send_command('+CGATT=1')
        response = self._send_command('+CIPRXGET?', read_lines = 3)
        if response[0] != '+CIPRXGET: 1':
            self._send_command('+CIPRXGET=1')
        self._send_command('+CIPSPRT=1')
        self._send_command('+CSTT="%s","%s","%s"' % (apn, user, password))
        self._send_command('+CIICR')
        response = self._send_command('+CIFSR', read_lines = 1, expected_status = None)
        print "IP:", response[0]

    def gprs_stop(self):
        self._send_command('+CIPSHUT', expected_status = 'SHUT OK')
        self._send_command('+CGATT=0')

    def tcp_connect(self, host, port = 80):
        self._send_command('+CIPSTART="TCP","%s",%d' % (host, port), read_lines = 3, expected_status = 'CONNECT OK')
    
    def tcp_send(self, data):
        self._send_command('+CIPSEND=%d' % len(data), read_lines = 0)

        if self._read(2) != '> ':
            raise ATCommandError('Error while sending data')
        self._send_data(data)

        for i in range(2):
            response = self._readline()
        if response != 'SEND OK':
            raise ATCommandError('Error while sending data')
        
        self._readline()
        response = self._readline()
        if response != '+CIPRXGET: 1':
            raise ATCommandError('Error while sending data')

    def tcp_receive(self, max_chunk = 1460):
        data = ''
        while True:
            response = self._send_command(
                '+CIPRXGET=2,%d' % max_chunk, 
                read_lines = 1, expected_status = None
            )
            fields = response[0].split(',')
            if fields[0] != '+CIPRXGET: 2':
                raise ATCommandError('Error while reading data')

            (length, remaining) = (int(fields[1]), int(fields[2]))
            data += self.tty.read(length)

            self._readline() # ''
            response = self._readline() # 'OK'
            if response != 'OK':
                raise ATCommandError('Error while reading data')

            if remaining == 0: break
        #self._readline()    # ''
        #self._readline()    # 'CLOSED'
        return data

if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help = "Serial port name")
    parser.add_argument("--baud", help = "Serial port baudrate (9600)", default=9600, type=int)
    args = parser.parse_args()

    gsm = SIM800(args.port, args.baud)
    
    try:
        gsm.check()
        (state, charge, voltage) = gsm.battery_voltage()
        print 'STATE: %d CHARGE: %d%% VOLTAGE: %.3fV' % (state, charge, voltage)
        (rssi, ber) = gsm.signal_quality()
        print 'RSSI: %ddBm BER: %d%%' % (rssi, ber)
    except ATCommandError:
        sys.stderr.write('Modem not responding')
        sys.exit(0)

    try:
        gsm.gprs_start(GPRS_APN)
    except ATCommandError as e:
        sys.stderr.write('Error while setting up GPRS (%s)\n' % str(e))
        sys.exit(0)

    try:
        gsm.tcp_connect('exploreembedded.com')
        request = 'GET /wiki/images/1/15/Hello.txt HTTP/1.0\n\n'
        #gsm.tcp_send('GET /wiki/images/1/15/Hello2.txt HTTP/1.0\r\n\r\n')
        #gsm.tcp_connect('habitat.habhub.org')
        #request = HABHUB_TEST1
        print request
        gsm.tcp_send(request)
        response = gsm.tcp_receive()
        print response
    except ATCommandError as e:
        sys.stderr.write('Error while connecting to host (%s)\n' % str(e))
        sys.exit(0)

    try:
        gsm.gprs_stop()
    except ATCommandError as e:
        sys.stderr.write('Error while shutting down GPRS')
 
    sys.exit(0)

    while True:
        line = []
        bat_report = gsm.battery_voltage()
        if bat_report:
            (state, charge, voltage) = bat_report
            line.append('STATE: %d CHARGE: %d%% VOLTAGE: %.3fV' % (state, charge, voltage))
        sig_report = gsm.signal_quality()
        if sig_report:
            (rssi, ber) = sig_report
            line.append('RSSI: %ddBm BER: %d%%' % (rssi, ber))

        if line:
            print ' '.join(line)
        time.sleep(5)
