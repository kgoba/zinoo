#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import scipy.integrate as integrate

def func(y, t, M, I):
    # y[0] - wx, y[1] - wy, y[2] - wz
    # y[3] - q0, y[4] - q1, y[5] - q2, y[6] - q3
    if t < 1.5:
        mx, my, mz = (M[0], M[1], M[2])
    else:
        mx, my, mz = (0, 0, 0)
    dwx = mx/I[0] - (I[2] - I[1])/I[0] * y[1] * y[2]
    dwy = my/I[1] - (I[0] - I[2])/I[1] * y[2] * y[0]
    dwz = mz/I[2] - (I[1] - I[0])/I[2] * y[0] * y[1]
    dq0 = (            - y[4] * y[0] - y[5] * y[1] - y[6] * y[2]) / 2
    dq1 = (y[3] * y[0]               - y[5] * y[2] + y[6] * y[1]) / 2
    dq2 = (y[3] * y[1] + y[4] * y[2]               - y[6] * y[0]) / 2
    dq3 = (y[3] * y[2] - y[4] * y[1] + y[5] * y[0]              ) / 2

    return [dwx, dwy, dwz, dq0, dq1, dq2, dq3]

def toEulerAngle(qw, qx, qy, qz):
    # roll (x-axis rotation)
    sinr = +2.0 * (qw * qx + qy * qz)
    cosr = +1.0 - 2.0 * (qx * qx + qy * qy)
    roll = np.arctan2(sinr, cosr)

    # pitch (y-axis rotation)
    sinp = +2.0 * (qw * qy - qz * qx)
    if np.fabs(sinp) >= 1:
        pitch = np.copysign(np.pi / 2, sinp) # use 90 degrees if out of range
    else:
        pitch = np.arcsin(sinp)

    # yaw (z-axis rotation)
    siny = +2.0 * (qw * qz + qx * qy)
    cosy = +1.0 - 2.0 * (qy * qy + qz * qz)
    yaw = np.arctan2(siny, cosy)
    return (roll, pitch, yaw)

def inertiaCylinder(mass, radius, height):
    Il = mass/2.0 * (radius*radius)
    It = mass/12.0 * (3*radius*radius + height*height)
    return (Il, It, It)

def model(w0, M, I):
    y0 = (w0[0], w0[1], w0[2], 1, 0, 0, 0)

    t = np.linspace(0, 3, 3000)
    y = integrate.odeint(lambda y, t: func(y, t, M, I), y0, t)

    (wx, wy) = (y[:,0], y[:,1])
    (q0, q1, q2, q3) = (y[:,3], y[:,4], y[:,5], y[:,6])
    pn = np.sqrt(q1*q1 + q2*q2 + q3*q3)

    euler = np.array([toEulerAngle(yi[3], yi[4], yi[5], yi[6]) for yi in y])
    roll = euler[:,0] * 180 / np.pi
    pitch = euler[:,1] * 180 / np.pi
    yaw = euler[:,2] * 180 / np.pi

    dev = np.arccos(q0*q0 + q1*q1 - q2*q2 - q3*q3) * 180 / np.pi
    return dev

spin_rps = 12       # 1/sec
radius = 0.042/2    # meters
height = 0.800      # meters
mass   = 1.0        # kilograms
M = (-2E-3, 1 * 2.4, 0)

w_l = 2 * np.pi * spin_rps
y0 = (w_l, 0, 0, 1, 0, 0, 0)
#I = inertiaCylinder(mass, radius, height)   # (0.00022, 0.0534, 0.0534)
I = (0.00016, 0.012, 0.012) # from OpenRocket model
#I = (0.00023, 0.017, 0.017)

t = np.linspace(0, 3, 3000)
y = integrate.odeint(lambda y, t: func(y, t, M, I), y0, t)

(wx, wy) = (y[:,0], y[:,1])
(q0, q1, q2, q3) = (y[:,3], y[:,4], y[:,5], y[:,6])
pn = np.sqrt(q1*q1 + q2*q2 + q3*q3)

euler = np.array([toEulerAngle(yi[3], yi[4], yi[5], yi[6]) for yi in y])
roll = euler[:,0] * 180 / np.pi
pitch = euler[:,1] * 180 / np.pi
yaw = euler[:,2] * 180 / np.pi

dev = np.arccos(q0*q0 + q1*q1 - q2*q2 - q3*q3) * 180 / np.pi
#p1 = q1 / pn
#p2 = q2 / pn
#p3 = q3 / pn
#plt.plot(t, wx, t, wy)
#plt.plot(t, q1, t, q2, t, q0)
plt.plot(t, dev)
plt.grid()

plt.show()
