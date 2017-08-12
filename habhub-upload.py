#!/usr/bin/env python

import serial
import sys, time, struct, json, socket, httplib
from base64 import b64encode
from hashlib import sha256
from datetime import datetime

# Habitat Upload Functions
def habitat_upload_payload_telemetry(sentence, receiver_callsign):
    sentence_b64 = b64encode(sentence + '\n')
    date = datetime.utcnow().isoformat("T") + "Z"

    data = {
        "type": "payload_telemetry",
        "data": {
            "_raw": sentence_b64
            },
        "receivers": {
            receiver_callsign: {
                "time_created": date,
                "time_uploaded": date,
                },
            },
    }
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
    
receiver = sys.argv[1]
port_name = sys.argv[2]
port_speed = 9600
input = serial.Serial(port_name, port_speed)
#input = sys.stdin # not a good idea, as stdin is normally buffered

logfile = open("log-" + datetime.utcnow().isoformat("T") + ".txt", "w")

try:
    for line in input:
        log(logfile, line)

        line = line.rstrip()
        if line.startswith('$$'):
            for nTry in reversed(range(10)):
                try:
                    habitat_upload_payload_telemetry(line, receiver)
                    log(logfile, "  Uploaded to Habitat\n")
                    break
                except Exception as e:
                    if nTry == 0:
                        log(logfile, "  Failed to upload to Habitat (%s)\n" % str(e), error=True)
                    else:
                        log(logfile, "  Failed to upload to Habitat (%s), retrying...\n" % str(e), error=True)
            #try:
            #    pos = extractPayloadPosition(line)
            #except:
            #    sys.stderr.write("  Failed to extract position\n")
        
        #if line.startswith('**'):
            #try:
            #    myPos = extractReceiverPosition(line)
            #except:
            #    sys.stderr.write("  Failed to extract receiver position")

except KeyboardInterrupt:
    sys.exit(0)
