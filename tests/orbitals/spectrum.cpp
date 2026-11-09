#include "../../src/orbitals/dsp/OrbitalEngine.hpp"
#include <cstdio>
using namespace orbital;
int main() {
    for (double rate : {48000., 768000.}) {
        Engine e;
        e.setSampleRate(rate);
        Controls c;
        c.pitch = std::log2(4211. / 261.6255653005986);
        c.mix = .4;
        FILE *f = std::fopen(rate == 48000 ? "build/tests/orbitals/os48.raw" : "build/tests/orbitals/ref768.raw", "wb");
        for (int i = 0; i < 2 * rate; i++) {
            float v = e.process(c).audio;
            if (i >= rate)
                std::fwrite(&v, 4, 1, f);
        }
        std::fclose(f);
    }
    Path p;
    double phase = 0, prev = 0, dc = 0, pole = std::exp(-2 * pi * 5 / 48000.);
    auto w = weights(4, 9, .4);
    FILE *f = std::fopen("build/tests/orbitals/native48.raw", "wb");
    for (int i = 0; i < 96000; i++) {
        auto q = trajectory(phase, p);
        double raw = 40 * (w[4] * evaluate(4, q) + w[9] * evaluate(9, q));
        dc = raw - prev + pole * dc;
        prev = raw;
        float out = bounded(dc * .7, -5, 5);
        if (i >= 48000)
            std::fwrite(&out, 4, 1, f);
        phase += 4211. / 48000;
        phase -= std::floor(phase);
    }
    std::fclose(f);
}
