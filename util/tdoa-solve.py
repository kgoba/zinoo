#!/usr/bin/env python

import numpy as np

def tdoa_solve(P, D):
    N = P.shape[0]
    dim = P.shape[1]
    
    A = np.zeros((N - 1, dim))
    b = np.zeros((N - 1,))
    
    for m in range(1, N):
        i1 = m
        i2 = m - 1
        A[m - 1] = 2*P[i1]/D[i1] - 2*P[i2]/D[i2]
        b[m - 1] = -(D[i1] - np.dot(P[i1], P[i1])/D[i1]) + (D[i2] - np.dot(P[i2], P[i2])/D[i2])

    (x, _, _, _) = np.linalg.lstsq(A, b)
    return x

def tdoa_solve2(P, D):
    N = P.shape[0]
    dim = P.shape[1]
    
    A = np.zeros((N - 1, dim + 1))
    b = np.zeros((N - 1,))
    
    for m in range(1, N):
        P12 = P[m] - P[0]
        D12 = D[m] - D[0]
        A[m - 1] = np.concatenate( (P12, [D12] ) )
        b[m - 1] = (np.dot(P12, P12) - D12*D12) / 2

    (x, _, _, _) = np.linalg.lstsq(A, b)
    return P[0] + x[:-1]

def tdoa_solve3(P, D, x0 = None, eps = 1E-5):
    # https://srbuenaf.webs.ull.es/potencia/hyperbolic%20location/HyperbolicLocation.pdf
    N = P.shape[0]
    dim = P.shape[1]
    
    G = np.zeros((N - 1, dim))
    h = np.zeros((N - 1,))
    R = np.zeros((N,))
    
    #if not x0:
    #x0 = np.mean(P, axis = 0)
    x = x0
    
    nIter = 1
    while True:
        #if x[2] < 0:
        #    x = -x
        for m in range(N):
            R[m] = np.linalg.norm(P[m] - x)
            if m > 0:
                h[m - 1] = (D[m] - D[0]) - (R[m] - R[0])
                G[m - 1] = (P[0] - x) / R[0] - (P[m] - x) / R[m]
                
        #dx = np.linalg.inv(G.transpose().dot(Qinv).dot(G)).dot(G.transpose()).dot(Qinv).dot(h)
        #dx = np.linalg.inv(G.transpose().dot(G)).dot(G.transpose()).dot(h)       
        (dx, _, _, _) = np.linalg.lstsq(G, h)
        
        x = x + dx * 0.5

        if np.linalg.norm(dx) < eps:
            break
        
        nIter += 1

    print nIter, "iterations"
    return x

def tdoa_simulate(P, E):
    N = P.shape[0]
    dim = P.shape[1]

    D = np.zeros( (N,) )
    for m in range(N):
        D[m] = np.sqrt(np.dot(P[m] - E, P[m] - E))
    return D

P = np.array( ((-5, -4, 0.1), (-60, 7, 0), (8, 60, 0), (6, -25, -0.1)) )
E = np.array( (2, -50, 24) )

D = tdoa_simulate(P, E)
E_est = tdoa_solve3(P, D, np.array((0, 0, 100)) )

print "True:", E
print "Est.:", E_est
print "Err.:", E - E_est
