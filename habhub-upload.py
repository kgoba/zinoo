#!/usr/bin/env python

#    Copyright 2011, 2012 (C) Daniel Richman
#                    2017 (C) Karlis Goba
#
# habhub-upload is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# habhub-upload is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with habhub-upload.  If not, see <http://www.gnu.org/licenses/>.

import serial
import sys, time, math, struct, json
from datetime import datetime

import copy
import base64
import hashlib
import couchdbkit
import couchdbkit.exceptions
import restkit
import restkit.errors
import threading
import Queue
import strict_rfc3339


class UnmergeableError(Exception):
    """
    Couldn't merge a ``payload_telemetry`` CouchDB conflict after many tries.
    """
    pass


class Uploader(object):
    """
    An easy interface to insert documents into a habitat CouchDB.

    This class is intended for use by a listener.

    After having created an :class:`Uploader` object, call
    :meth:`payload_telemetry`, :meth:`listener_telemetry` or
    :meth:`listener_information` in any order. It is however recommended that
    :meth:`listener_information` and :meth:`listener_telemetry` are called once
    before any other uploads.

    :meth:`flights` returns a list of current flight documents.

    Each method that causes an upload accepts an optional kwarg, time_created,
    which should be the unix timestamp of when the doc was created, if it is
    different from the default 'now'. It will add time_uploaded, and turn both
    times into RFC3339 strings using the local offset.

    See the CouchDB schema for more information, both on
    validation/restrictions and data formats.
    """

    def __init__(self, callsign,
                       couch_uri="http://habitat.habhub.org/",
                       couch_db="habitat",
                       max_merge_attempts=20):

        self._lock = threading.RLock()
        self._callsign = callsign
        self._latest = {}
        self._max_merge_attempts = max_merge_attempts

        server = couchdbkit.Server(couch_uri)
        self._db = server[couch_db]

    def listener_telemetry(self, data, time_created=None):
        """
        Upload a ``listener_telemetry`` doc. The ``doc_id`` is returned

        A ``listener_telemetry`` doc contains information about the listener's
        current location, be it a rough stationary location or a constant
        feed of GPS points. In the former case, you may only need to call
        this function once, at startup. In the latter, you might want to
        call it constantly.

        The format of the document produced is described elsewhere;
        the actual document will be constructed by :class:`Uploader`.
        *data* must be a dict and should typically look something like
        this::

            data = {
                "time": "12:40:12",
                "latitude": -35.11,
                "longitude": 137.567,
                "altitude": 12
            }

        ``time`` is the GPS time for this point, ``latitude`` and ``longitude``
        are in decimal degrees, and ``altitude`` is in metres.

        ``latitude`` and ``longitude`` are mandatory.

        Validation will be performed by the CouchDB server. *data* must not
        contain the key ``callsign`` as that is added by
        :class:`Uploader`.
        """
        return self._listener_doc(data, "listener_telemetry", time_created)

    def listener_information(self, data, time_created=None):
        """
        Upload a listener_information doc. The doc_id is returned

        A listener_information document contains static human readable
        information about a listener.

        The format of the document produced is described elsewhere (TODO?);
        the actual document will be constructed by ``Uploader``.
        *data* must be a dict and should typically look something like
        this::

            data = {
                "name": "Adam Greig",
                "location": "Cambridge, UK",
                "radio": "ICOM IC-7000",
                "antenna": "9el 434MHz Yagi"
            }

        *data* must not contain the key ``callsign`` as that is added by
        :class:`Uploader`.
        """
        return self._listener_doc(data, "listener_information", time_created)

    def _listener_doc(self, data, doc_type, time_created=None):
        if time_created is None:
            time_created = time.time()

        assert "callsign" not in data

        data = copy.deepcopy(data)
        data["callsign"] = self._callsign

        doc = {
            "data": data,
            "type": doc_type
        }

        self._set_time(doc, time_created)
        self._db.save_doc(doc)

        doc_id = doc["_id"]
        with self._lock:
            self._latest[doc_type] = doc_id
        #print doc
        return doc_id

    def _set_time(self, thing, time_created):
        time_uploaded = int(round(time.time()))
        time_created = int(round(time_created))

        to_rfc3339 = strict_rfc3339.timestamp_to_rfc3339_localoffset
        thing["time_uploaded"] = to_rfc3339(time_uploaded)
        thing["time_created"] = to_rfc3339(time_created)

    def payload_telemetry(self, string, metadata=None, time_created=None):
        """
        Create or add to the ``payload_telemetry`` document for *string*.

        This function attempts to create a new ``payload_telemetry`` document
        for the provided string (a new document, with one receiver: you).
        If the document already exists in the database it instead downloads
        it, adds you to the list of receivers, and reuploads.

        *metadata* can contain extra information about your receipt of
        *string*. Nothing has been standardised yet (TODO), but here's an
        example of what you might be able to do in the future::

            metadata = {
                "frequency": 434075000,
                "signal_strength": 5
            }

        *metadata* must not contain the keys ``time_created``,
        ``time_uploaded``, ``latest_listener_information`` or
        ``latest_listener_telemetry``. These are added by :class:`Uploader`.
        """

        if metadata is None:
            metadata = {}

        if time_created is None:
            time_created = time.time()

        for key in ["time_created", "time_uploaded",
                "latest_listener_information", "latest_listener_telemetry"]:
            assert key not in metadata

        receiver_info = copy.deepcopy(metadata)

        with self._lock:
            for doc_type in ["listener_telemetry", "listener_information"]:
                if doc_type in self._latest:
                    receiver_info["latest_" + doc_type] = \
                            self._latest[doc_type]

        for i in xrange(self._max_merge_attempts):
            try:
                self._set_time(receiver_info, time_created)
                doc_id = self._payload_telemetry_update(string + '\n', receiver_info)
            except couchdbkit.exceptions.ResourceConflict:
                continue
            except restkit.errors.Unauthorized:
                raise UnmergeableError
            else:
                return doc_id
        else:
            raise UnmergeableError

    def _payload_telemetry_update(self, string, receiver_info):
        doc_id = hashlib.sha256(base64.b64encode(string)).hexdigest()
        doc_ish = {
            "data": {"_raw": base64.b64encode(string)},  # added newline
            "receivers": {self._callsign: receiver_info}
        }
        url = "_design/payload_telemetry/_update/add_listener/" + doc_id
        #print string, url, doc_ish
        self._db.res.put(url, payload=doc_ish).skip_body()
        return doc_id

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
    # parse listener telemetry
    (time, lat, lng, alt) = line[2:].split(',')[:4]
    return (float(lat), float(lng), float(alt))

def extract_payload_position(line):
    # parse payload telemetry: 
    # $$Z72,123,152251,56.95790,24.13918,21,11,13*CRC16
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

def test_telemetry(sentence_id = 0):
    def crc16(data, poly = 0x1021, crc = 0xFFFF):
        ''' CRC-16-CCITT Algorithm '''
        for b in bytearray(data):
            crc = crc ^ (b << 8)
            for _ in range(8):
                if (crc & 0x8000):
                    crc = (crc << 1) ^ poly
                else:
                    crc <<= 1
        return crc & 0xFFFF

    raw_sentence = "Z72,%d,152251,56.95790,24.13918,21,11,13" % sentence_id
    return "$$%s*%04X" % (raw_sentence, crc16(raw_sentence))

def test():
    callsign = "TEST7"
    u = Uploader(callsign)

    data1 = {
        #"time": "12:40:12",
        "latitude": 51.15,
        "longitude": 3.733333,
        "altitude": 12
    }

    data2 = {
        #"name": "Test Receiver",
        #"location": "Belgium",
        "radio": "rtlsdr",
        "antenna": "fake 434MHz Yagi"
    }

    string = test_telemetry(501)
                
    #u.listener_telemetry(data1)
    #u.listener_information(data2)

    try:
        print "Uploading telemetry..."
        u.payload_telemetry(string)
    except Exception as e:
        print "Error:", type(e)

    return

def main():
    receiver_callsign = sys.argv[1]
    port_name = sys.argv[2]
    
    if port_name == '-f':
        #input = sys.stdin # not a good idea, as stdin is normally buffered
        input = open(sys.argv[3])
        delay = 10
    else:
        port_speed = 9600
        input = serial.Serial(port_name, port_speed)
        delay = None

    logfile = open("log-" + datetime.utcnow().isoformat("T") + ".txt", "w")

    u = Uploader(receiver_callsign)

    data = {
        #"name": "Receiver",
        "radio": "LoRa",
        "antenna": "434MHz Yagi"
    }
    u.listener_information(data)

    my_position = None  # (lat, lng, alt)
    for line in input:
        log(logfile, line)

        line = line.rstrip()
        if line.startswith('$$'):
            for nTry in reversed(range(10)):
                try:
                    u.payload_telemetry(line)
                    log(logfile, "  Uploaded to Habitat\n")
                    break
                except UnmergeableError:
                    log(logfile, "  Failed to upload to Habitat (cannot merge, data already exists)", error=True)
                    break
                except Exception as e:
                    if nTry == 0:
                        log(logfile, "  Failed to upload to Habitat (%s)\n" % str(e), error=True)
                    else:
                        log(logfile, "  Failed to upload to Habitat (%s), retrying...\n" % str(e), error=True)
            try:
                pos = extract_payload_position(line)
                if my_position and pos:
                    d_km = distance_straight(my_position, pos) / 1000.0
                    log(logfile, "  Distance: %.3f km\n" % (d_km,))
            except Exception as e:
                log(logfile, "  Failed to extract payload position (%s)\n" % str(e), error=True)
        
        if line.startswith('**'):
            try:
                my_position = extract_listener_position(line)
                data = {
                    "latitude": my_position[0],
                    "longitude": my_position[1],
                    "altitude": my_position[2]
                }
                u.listener_telemetry(data)
            except:
                log(logfile, "  Failed to extract listener position\n", error=True)
        
        if delay:
            time.sleep(delay)
    return

if __name__ == "__main__":
    try:
        if sys.argv[1] == 'test':
            test()
        else:
            main()    
    except KeyboardInterrupt:
        sys.exit(0)
