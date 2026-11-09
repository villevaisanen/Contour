"""Compare current audio against the committed v2.1.0 engine, not a copied test oracle."""
from pathlib import Path
import os
import subprocess
root = Path(__file__).resolve().parents[2]
out = root / 'build' / 'tests' / 'orbitals'
out.mkdir(parents=True, exist_ok=True)
old = (root / 'tests/orbitals/baseline/OrbitalEngine-v2.1.0.hpp').read_text()
old = old[old.index('class Engine {'):].replace('Engine', 'BaselineEngine')
(out / 'baseline.hpp').write_text('#include "../../../src/orbitals/dsp/OrbitalEngine.hpp"\nnamespace orbital {\n' + old)
(out / 'audio-regression.cpp').write_text(r'''
#include "baseline.hpp"
#include <iostream>
int main() {
    int samples=0; double worst=0.;
    for (double rate : {44100., 48000., 96000.}) {
        orbital::Engine current;
        orbital::BaselineEngine baseline;
        current.setSampleRate(rate); baseline.setSampleRate(rate);
        orbital::Controls c;
        for (int i=0;i<50000;i++) {
            if (i%113==0) {
                c.a=(i/113)%14; c.b=(i/317)%14; c.mix=(i%101)/100.;
                c.radius=(i%240)/10.; c.offset=(i%480)/10.-24;
                c.shape=(i%100)/100.; c.tilt=(i%200)/100.-1;
                c.spin=(i%100)/50.-1; c.pitch=(i%100)/10.-5;
                c.radiusCV=std::sin(i*.01); c.mixCV=std::cos(i*.03);
                c.offsetCV=c.tiltCV=c.spinCV=std::sin(i*.02);
            }
            c.voct=std::sin(i*.001); c.fm=.2*std::sin(i*.7);
            c.sync=i%401==0?5:0;
            if(i==25000){current.setSampleRate(48000);baseline.setSampleRate(48000);}
            auto a=current.process(c), b=baseline.process(c);
            double delta=std::max(std::max(std::abs(a.audio-b.audio),std::abs(a.xyz.x-b.xyz.x)),std::max(std::abs(a.xyz.y-b.xyz.y),std::abs(a.xyz.z-b.xyz.z)));
            worst=std::max(worst,delta);
            if(delta>1e-10) {
                std::cerr<<"audio regression at sample "<<i<<"\n";return 1;
            }
            // Reading display state cannot mutate synthesis state.
            if(i%1600==0) current.visualizationState();
            ++samples;
        }
    }
    std::cout<<"PASS "<<samples<<" AUDIO/XYZ comparisons vs v2.1.0 (tolerance 1e-10 V), including CV, state transitions, sync and rate changes; maximum error "<<worst<<" V\n";
}
''')
subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++11', '-O3', '-funsafe-math-optimizations', str(out / 'audio-regression.cpp'), '-o', str(out / 'audio-regression')], check=True)
subprocess.run([str(out / 'audio-regression')], check=True)
