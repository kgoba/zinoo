#!/usr/bin/env python

import scipy.signal as spsig
import scipy.interpolate as spint
import numpy as np
import datetime
import math
import sys

T_TEMP = 60.0
T_SPEED = 10.0
T_SPEED_H = 10.0

def read_lora_log(path):
    series = { 
        'callsign': list(),
        'msg_id': list(),
        'timestamp' : list(),
        'time': list(), 
        'lat' : list(),
        'lng' : list(),
        'alt' : list(),
        'n_sat' : list(),
        't_ext_raw' : list()
    }
    for line in open(path):
        line = line.rstrip()
        if line.startswith('>>> '):
            line = line[4:]
        elif line.startswith('$$'):
            line = line[2:-5]
        fields = line.split(',')
        try:
            (callsign, msg_id, timestamp, latitude, longitude, altitude, n_sat, t_ext, status) = fields
            msg_id = int(msg_id)
            time = (((int(timestamp[0:2]) * 60) + int(timestamp[2:4])) * 60) + int(timestamp[4:6])
            latitude = float(latitude)
            longitude = float(longitude)
            altitude = float(altitude)
            n_sat = int(n_sat)
            t_ext = float(t_ext)
            
            series['callsign'].append(callsign)
            series['msg_id'].append(msg_id)
            series['time'].append(time)
            series['timestamp'].append(timestamp)
            series['lat'].append(latitude)
            series['lng'].append(longitude)
            series['alt'].append(altitude)
            series['t_ext_raw'].append(t_ext)
            series['n_sat'].append(n_sat)
        except Exception as e:
            sys.stderr.write("Couldn't parse line: %s (%s)\n" % (line, str(e)))
        
    return series

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

#EARTH_RADIUS = 6371009  # meters
def distance_ground(lat1, lng1, lat2, lng2):
    lat_mean = (lat1 + lat2) / 2 * DEG2RAD
    k_lat = (lat2 - lat1) * DEG2RAD
    k_lng = (lng2 - lng1) * DEG2RAD * math.cos(lat_mean)
    return RADIUS_EARTH * math.sqrt(k_lat * k_lat + k_lng * k_lng)

def interpolate(series, dt, key_time, keys_exclude = []):
    keys = set(series.keys()) - set([key_time]) - set(keys_exclude)
    sys.stderr.write("Interpolating %s\n" % str(keys))

    t = series[key_time]
    n_steps = int( 1E-6 + (np.max(t) - np.min(t)) / dt )
    t_full = np.linspace(np.min(t), np.max(t), n_steps + 1)
    sys.stderr.write('tmin = %s\n' % series['timestamp'][0])
    sys.stderr.write('tmax = %s\n' % series['timestamp'][-1])
 
    for name in keys:
        f_int = spint.interp1d(t, series[name], bounds_error=False, kind='cubic')
        series[name] = f_int(t_full)

    n_series = dict()
    for name in keys_exclude:
        n_series[name] = list()
    
    for i in range(len(t)):
        if (i + 1) >= len(t):
            n = 1
        else:
            n = int( 1E-6 + (t[i + 1] - t[i])/dt )
        for name in keys_exclude:
            for i_copy in range(n):
                n_series[name].append(series[name][i])
        
    for name in keys_exclude:
        series[name] = n_series[name]

    sys.stderr.write('len(t) = %d\n' % len(t_full))
    sys.stderr.write('len(x) = %d\n' % len(series[keys_exclude[0]]))
    series[key_time] = t_full

    sys.stderr.write('tmin = %s\n' % series['timestamp'][0])
    sys.stderr.write('tmax = %s\n' % series['timestamp'][-1])
    return series

def process(series, pos_launch):
    t = np.array(series['time'])
    dt = np.unique(np.diff(t))

    if (len(dt) != 1):
        sys.stderr.write("Non-regular time intervals! (%s)\n" % str(dt))
        dt = float(dt[0])
        sys.stderr.write("Assuming dt = %.1f\n" % dt)
        interpolate(series, dt, 'time', ['msg_id', 'callsign', 'n_sat', 'timestamp'])
        for (i, t) in enumerate(series['time']):
            h = int(t / 3600)
            t -= h * 3600
            m = int(t / 60)
            t -= m * 60
            s = t
            series['timestamp'][i] = '%02d%02d%02d' % (h, m, s)
        #sys.exit(1)
    else:
        dt = float(dt[0])
    #print 'dt =', dt

    (b, a) = spsig.iirfilter(5, dt / T_TEMP, btype = 'lowpass', analog = False, ftype = 'butter')
    t_ext = spsig.filtfilt(b, a, series['t_ext_raw'])

    zip_pos = zip(series['lat'][:-1], series['lat'][1:], series['lng'][:-1], series['lng'][1:])
    h_spd = np.array([distance_ground(lat1, lng1, lat2, lng2) for (lat1, lat2, lng1, lng2) in zip_pos]) / 5
    h_spd = np.concatenate(([0], h_spd))
    v_spd = np.concatenate(([0], np.diff(series['alt']))) / dt

    (b, a) = spsig.iirfilter(3, dt / T_SPEED, btype = 'lowpass', analog = False, ftype = 'butter')
    v_spd = spsig.filtfilt(b, a, v_spd)
    (b, a) = spsig.iirfilter(3, dt / T_SPEED_H, btype = 'lowpass', analog = False, ftype = 'butter')
    h_spd = spsig.filtfilt(b, a, h_spd)

    zip_pos3 = zip(series['lat'], series['lng'], series['alt'])
    pos_launch = zip_pos3[0]
    dist = np.array([distance_straight(pos_launch, pos) for pos in zip_pos3])
    
    series['t_ext'] = t_ext
    series['h_spd'] = h_spd
    series['v_spd'] = v_spd
    series['dist'] = dist
    
    #sys.stderr.write('len(dist) = %d\n' % len(dist))
    return series

pos_launch = None
loralog_path = sys.argv[1]
if len(sys.argv) > 2:
    file_out = open(sys.argv[2], 'w')
else:
    file_out = sys.stdout
series = read_lora_log(loralog_path)

process(series, pos_launch)

order = ['callsign', 'msg_id', 'timestamp', 'lat', 'lng', 'alt', 'n_sat', 't_ext', 'h_spd', 'v_spd', 'dist']
format = ['%s', '%s', '%s', '%.5f', '%.5f', '%.0f', '%d', '%.1f', '%.1f', '%.1f', '%.0f']

zip_series = zip(*[series[key] for key in order])

for entry in zip_series:
    file_out.write( ','.join([fmt % x for (x, fmt) in zip(entry, format)]) )
    file_out.write('\n')
