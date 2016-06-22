#!/usr/bin/env python

import matplotlib.pyplot as plt
import numpy as np
#import scipy.io.wavfile as spwav
import scipy.fftpack as spfft
import wave
import sys
import struct

class ViterbiSolver:
  def __init__(self, states):
    pass
  

def readBlock(stream, count = 1024):
  waveData = stream.readframes(count)
  numSamples = len(waveData) / 2
  data = struct.unpack("<" + "h"*numSamples, waveData)
  return np.array(data) / 32768.0

sigFile = wave.open(sys.argv[1])
Fs = sigFile.getframerate()
print "Channels: %d" % sigFile.getnchannels()
print "Sample rate: %d" % Fs

blockSize = 4096
shiftSize = 256

g1 = (550, 100)               # gaussian mean and std-dev
g2 = (1320, 100)              # gaussian mean and std-dev

p_ij = np.array((0.5, 0.5))                 # initial state probabilities
a_ij = np.array((0.2, 0.8), (0.8, 0.2))     # transition probabilities

while True:
  block = readBlock(sigFile, blockSize)
  if len(block) == 0:
    break
  N = len(block)
  if (N < blockSize): break
  H = spfft.fft(block)
  A = 20 * np.log10(np.abs(H))
  fgrid = np.linspace(0, Fs, N)
  M = 200
  plt.plot(fgrid[:M], A[:M])
  #print "Read %d frames" % len(block)

plt.show()
