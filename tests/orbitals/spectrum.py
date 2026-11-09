"""Optional NumPy spectral diagnostic, not an alias-free guarantee."""
import numpy as np
for name, sr in [('native48',48000),('os48',48000),('ref768',768000)]:
    x=np.fromfile('build/tests/orbitals/'+name+'.raw',dtype=np.float32)
    power=np.abs(np.fft.rfft(x))**2
    valid=np.zeros(len(power),dtype=bool)
    valid[:3]=True
    for freq in range(4211,sr//2,4211):valid[max(0,freq-2):freq+3]=True
    # Non-harmonic energy measures folded harmonics for this coherent steady-state tone.
    print(name, 'non-harmonic energy dB',10*np.log10(power[~valid].sum()/power.sum()))
