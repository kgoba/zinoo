#!/usr/bin/env python

import scipy.signal as spsig
import scipy.io.wavfile as spwav
import numpy as np

import matplotlib.pyplot as plt

import sys

def h_hilbert(N):
    M = (N - 1) / 2
    h = np.zeros(N)
    for i in range(0, M, 2):
        j = i + 1
        k = 2 / (np.pi * j)
        h[M + j] = k
        h[M - j] = -k
    return h

path = sys.argv[1]

(fs, sig) = spwav.read(path)

gain = 1.0 / np.max(np.abs(sig))
sig = np.array(sig * gain)

sig2 = spsig.resample_poly(sig, 1, 16)
fs2 = fs / 16

print "fs2", fs2, "N", len(sig2)

#sig2 = spsig.hilbert(sig, fs / 100)
b1 = np.zeros(601)
b1[0] = 1.0
b2 = h_hilbert(601)
sig2 = spsig.lfilter(b1, 1, sig2) + 1j * spsig.lfilter(b2, 1, sig2)

t = np.arange(len(sig2)) / float(fs2)
sig2 *= np.exp(-1j * t * 2 * np.pi * 328) # 525 275 328

phase = 0
freq_i = 0.0 * (2*np.pi / fs2)
bw = 0.8 * (2*np.pi / fs2)
beta = np.sqrt(bw)

freq = []
for i in range(len(sig2)):
    y = sig2[i]
    vco = np.exp(-1j * phase)
    ph_diff = np.angle(vco * y)
    freq_i += bw * ph_diff
    phase += beta * ph_diff + freq_i
    freq.append(freq_i * fs2 / (2*np.pi))

freq = np.array(freq)

#phase = np.unwrap(np.angle(sig2))
#freq = np.diff(phase) / (2*np.pi) * fs2

freq = spsig.lfilter(spsig.firwin(1000, 5.0, nyq = fs2/2), 1, freq)

freq = spsig.resample_poly(freq, 1, 10)
freq = spsig.resample_poly(freq, 1, 10)

dt = 1.0 / (fs2 / 10 / 10)

t = dt * np.arange(len(freq))
v = freq / 434.25 * 300 # compensation - t * 1.0
h = np.cumsum(v) * dt

plt.plot(t, v)
plt.grid()
plt.show()

for (x_i, t_i) in zip(v, t):
    sys.stdout.write("%.3f,%.2f\n" % (t_i,x_i))
    #sys.stdout.write("%s\n" % str(f))
