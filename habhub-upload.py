#!/usr/bin/env python

import serial
import sys, time, math, struct, json, socket, httplib
from base64 import b64encode
from hashlib import sha256
from datetime import datetime

# Habitat Upload Functions
def habitat_upload_payload_telemetry(sentence, listener_callsign, listener_telemetry = None):
    sentence_b64 = b64encode(sentence + '\n')
    date = datetime.utcnow().isoformat("T") + "Z"

    data = {
        "type": "payload_telemetry",
        "data": {
            "_raw": sentence_b64
            },
        "receivers": {
            listener_callsign: {
                "time_created": date,
                "time_uploaded": date,
                },
            },
    }

    if listener_telemetry:
        data["receivers"][listener_callsign]["latest_listener_telemetry"] = listener_telemetry

    c = httplib.HTTPConnection("habitat.habhub.org",timeout=4)
    c.request(
        "PUT",
        "/habitat/_design/payload_telemetry/_update/add_listener/%s" % sha256(sentence_b64).hexdigest(),
        json.dumps(data),  # BODY
        {"Content-Type": "application/json"}  # HEADERS
        )

    response = c.getresponse()
    c.close()
    return response
    
def log(logfile, line, error=False):
    logfile.write(line)
    if error:
        sys.stderr.write(line)
    else:
        sys.stdout.write(line)    

RADIUS_EARTH = 6378137
DEG2RAD = math.pi / 180

def geo2eci(position):
    (lat, lng, alt) = position
    
    r = alt + RADIUS_EARTH
    
    r_xy = r * math.cos(lat * DEG2RAD)
    X = r_xy * math.cos(lng * DEG2RAD)
    Y = r_xy * math.sin(lng * DEG2RAD)
    Z = r * math.sin(lat * DEG2RAD)
    return (X, Y, Z)

def distance_straight(position1, position2):
    (x1, y1, z1) = geo2eci(position1)
    (x2, y2, z2) = geo2eci(position2)
    dx = x2 - x1
    dy = y2 - y1
    dz = z2 - z1
    d = math.sqrt(dx*dx + dy*dy + dz*dz)
    return d

def extract_listener_position(line):
    (time, lat, lng, alt, n_sats) = line[2:].split(',')
    return (float(lat), float(lng), float(alt))

def extract_payload_position(line):
    fields = line[2:].split(',')
    lat = fields[3]
    lng = fields[4]
    alt = fields[5]
    return (float(lat), float(lng), float(alt))

def test_distance_straight():
    pos1 = (51.05, 3.733333, 0)
    pos2 = (56.948889, 24.106389, 0)
    pos3 = (51.05, 3.733333, 500)
    print "pos1-pos2 =", distance_straight(pos1, pos2)
    print "pos1-pos3 =", distance_straight(pos1, pos3)

receiver_callsign = sys.argv[1]
port_name = sys.argv[2]
port_speed = 9600
input = serial.Serial(port_name, port_speed)
#input = sys.stdin # not a good idea, as stdin is normally buffered

logfile = open("log-" + datetime.utcnow().isoformat("T") + ".txt", "w")

myPos = None  # (lat, lng, alt)
try:
    for line in input:
        log(logfile, line)

        line = line.rstrip()
        if line.startswith('$$'):
            for nTry in reversed(range(10)):
                try:
                    habitat_upload_payload_telemetry(line, receiver_callsign)
                    log(logfile, "  Uploaded to Habitat\n")
                    break
                except Exception as e:
                    if nTry == 0:
                        log(logfile, "  Failed to upload to Habitat (%s)\n" % str(e), error=True)
                    else:
                        log(logfile, "  Failed to upload to Habitat (%s), retrying...\n" % str(e), error=True)
            try:
                pos = extract_payload_position(line)
                if myPos and pos:
                    d_km = distance_straight(myPos, pos) / 1000.0
                    log(logfile, "  Distance: %.3f km\n" % (d_km,))
            except Exception as e:
                log(logfile, "  Failed to extract payload position (%s)\n" % str(e), error=True)
        
        if line.startswith('**'):
            try:
                myPos = extract_listener_position(line)
            except:
                log(logfile, "  Failed to extract listener position\n", error=True)

except KeyboardInterrupt:
    sys.exit(0)
