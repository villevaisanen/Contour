#include "../../src/orbitals/dsp/OrbitalEngine.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
static bool track = false;
static int allocations = 0;
void *operator new(std::size_t n) {
    if (track)
        ++allocations;
    if (void *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
using namespace orbital;
int checks = 0;
void check(bool b, const char *s) {
    ++checks;
    if (!b) {
        std::cerr << "FAIL: " << s << "\n";
        std::exit(1);
    }
}
void near(double a, double b, double eps, const char *s) { check(std::abs(a - b) < eps, s); }
int main() {
    Vec3 p{1, 2, 3};
    double r = std::sqrt(14.);
    near(evaluate(0, p), std::exp(-r) / std::sqrt(pi), 1e-14, "1s reference");
    near(evaluate(1, p), (2 - r) * std::exp(-r / 2) / (4 * std::sqrt(2 * pi)), 1e-14,
         "2s reference");
    for (int i = 0; i < 3; i++) {
        double v = i + 1;
        near(evaluate(2 + i, p), v * std::exp(-r / 2) / (4 * std::sqrt(2 * pi)), 1e-14,
             "2p reference");
        near(evaluate(6 + i, p),
             std::sqrt(2.) * v * (6 - r) * std::exp(-r / 3) / (81 * std::sqrt(pi)), 1e-14,
             "3p reference");
    }
    near(evaluate(5, p), (27 - 18 * r + 2 * r * r) * std::exp(-r / 3) / (81 * std::sqrt(3 * pi)),
         1e-14, "3s reference");
    double d = 2 * std::exp(-r / 3) / (81 * std::sqrt(2 * pi));
    near(evaluate(9, p), 2 * d, 1e-14, "dxy reference");
    near(evaluate(10, p), 3 * d, 1e-14, "dxz reference");
    near(evaluate(11, p), 6 * d, 1e-14, "dyz reference");
    near(evaluate(12, p), -1.5 * d, 1e-14, "dx2-y2 reference");
    near(evaluate(13, p), 13 * d / (2 * std::sqrt(3.)), 1e-14, "dz2 reference");
    for (int i = 0; i < count; i++) {
        check(std::isfinite(evaluate(i, {0, 0, 0})), "origin finite");
        check(std::isfinite(evaluate(i, {1e300, 0, 0})), "large finite");
        int parity = (i >= 2 && i <= 4) || (i >= 6 && i <= 8) ? -1 : 1;
        near(evaluate(i, {-1, -2, -3}), parity * evaluate(i, p), 1e-14, "parity");
        for (double mix : {.1, .5, .9}) {
            auto w = weights(i, (i + 1) % count, mix);
            double norm = 0.;
            for (double v : w)
                norm += v * v;
            near(norm, 1., 1e-14, "distinct superposition normalization");
        }
        auto a = weights(i, (i + 1) % count, 0);
        auto b = weights((i + 1) % count, i, 1);
        near(a[i], 1, 1e-14, "mix A");
        near(b[i], 1, 1e-14, "mix B");
        for (double m : {0., .1, .5, .9, 1.})
            near(weights(i, i, m)[i], 1, 1e-14, "same state normalization");
    }
    near(evaluate(1, {2, 0, 0}), 0, 1e-14, "2s radial node");
    near(evaluate(6, {6, 0, 0}), 0, 1e-14, "3p radial node");
    near(evaluate(2, {0, 2, 3}), 0, 1e-14, "px node");
    near(evaluate(4, {1, 2, 0}), 0, 1e-14, "pz node");
    near(evaluate(9, {0, 2, 3}), 0, 1e-14, "dxy node");
    near(evaluate(10, {1, 2, 0}), 0, 1e-14, "dxz node");
    near(evaluate(11, {1, 0, 3}), 0, 1e-14, "dyz node");
    near(evaluate(12, {2, 2, 3}), 0, 1e-14, "dx2-y2 node");
    near(evaluate(13, {1, 1, 1}), 0, 1e-14, "dz2 node");
    // Independent volume quadrature: midpoint r, cos(theta), phi; full Gram matrix.
    double gram[count][count]{};
    for (int ir = 0; ir < 800; ir++) {
        double rr = (ir + .5) * .1;
        for (int iz = 0; iz < 96; iz++) {
            double z = -1 + (iz + .5) * 2 / 96.;
            double xy = std::sqrt(1 - z * z);
            for (int ip = 0; ip < 32; ip++) {
                double phi = 2 * pi * (ip + .5) / 32.;
                Vec3 q{rr * xy * std::cos(phi), rr * xy * std::sin(phi), rr * z};
                double val[count];
                for (int i = 0; i < count; i++)
                    val[i] = evaluate(i, q);
                double volume = rr * rr * .1 * 2 / 96. * 2 * pi / 32.;
                for (int i = 0; i < count; i++)
                    for (int j = 0; j <= i; j++)
                        gram[i][j] += volume * val[i] * val[j];
            }
        }
    }
    double worst = 0.;
    for (int i = 0; i < count; i++)
        for (int j = 0; j <= i; j++) {
            double err = std::abs(gram[i][j] - (i == j ? 1. : 0.));
            worst = std::max(worst, err);
            check(err < .001, "Gram normalization / orthogonality");
        }
    std::cout << "Gram maximum error " << worst << "\n";
    for (double sh : {0., .3, 1.}) {
        Path path;
        path.shape = sh;
        auto a = trajectory(.123, path), b = trajectory(1.123, path);
        near(a.x, b.x, 1e-12, "path x periodic");
        near(a.y, b.y, 1e-12, "path y periodic");
        near(a.z, b.z, 1e-12, "path z periodic");
    }
    near(Engine::frequency(0, 1, 0, 48000), 2 * Engine::frequency(0, 0, 0, 48000), 1e-10, "1V/oct");
    near(Engine::frequency(0, 0, 1, 48000), 2 * Engine::frequency(0, 0, 0, 48000), 1e-10,
         "FM 1V/oct");
    for (double sr : {44100., 48000., 96000.}) {
        Engine e;
        e.setSampleRate(sr);
        Controls c;
        double energy = 0;
        for (int i = 0; i < sr; i++) {
            auto o = e.process(c);
            energy += o.audio * o.audio;
            check(std::isfinite(o.audio) && std::abs(o.audio) <= 5, "audio finite bounded");
        }
        check(energy / sr > .001, "default audible RMS");
        near(e.phase, std::fmod(Engine::frequency(0, 0, 0, sr), 1.), 1e-8, "phase frequency");
        c.sync = 5;
        e.process(c);
        near(e.phase, Engine::frequency(0, 0, 0, sr) / sr, 1e-12, "sync rising edge");
        e.process(c);
        near(e.phase, 2 * Engine::frequency(0, 0, 0, sr) / sr, 1e-12, "sync held");
        c.sync = 0;
        e.process(c);
        c.sync = 5;
        e.process(c);
        near(e.phase, Engine::frequency(0, 0, 0, sr) / sr, 1e-12, "sync rearm");
        double before = e.phase;
        e.setSampleRate(96000);
        near(e.phase, before, 1e-14, "rate change phase continuity");
        for (int i = 0; i < 14000; i++) {
            double v = (i % 2 ? 1 : -1) * 1e20;
            c.a = i % 14;
            c.b = (i / 14) % 14;
            c.voct = v;
            c.fm = -v;
            c.radius = c.radiusCV = c.offset = c.offsetCV = c.mix = c.mixCV = c.tilt = c.tiltCV =
                c.spin = c.spinCV = c.shape = c.level = v;
            auto o = e.process(c);
            check(std::isfinite(o.audio) && std::abs(o.audio) <= 5, "extreme audio");
            check(std::abs(o.xyz.x) <= 5 && std::abs(o.xyz.y) <= 5 && std::abs(o.xyz.z) <= 5,
                  "XYZ bounds");
        }
        c = Controls();
        c.radius = 0;
        c.offset = 0;
        Engine zero;
        zero.setSampleRate(sr);
        auto o = zero.process(c);
        near(o.xyz.x, 0, 1e-14, "zero X");
        near(o.xyz.y, 0, 1e-14, "zero Y");
        near(o.xyz.z, 0, 1e-14, "zero Z");
    }
    {
        Engine e;
        Controls c;
        Path p;
        auto expected = trajectory(3 * Engine::frequency(0, 0, 0, 48000) / (4 * 48000), p);
        auto o = e.process(c);
        near(o.xyz.x, 5 * expected.x / 48, 1e-12, "X scaling");
        near(o.xyz.y, 5 * expected.y / 24, 1e-12, "Y scaling");
        near(o.xyz.z, 5 * expected.z / 34, 1e-12, "Z scaling");
    }
    for (int id = 0; id < count; id++) {
        Engine e;
        Controls c;
        c.a = c.b = id;
        double energy = 0.;
        for (int k = 0; k < 48000; k++) {
            auto o = e.process(c);
            if (k > 24000)
                energy += o.audio * o.audio;
        }
        check(energy / 24000 > 1e-7, "each orbital produces changing audio on default path");
    }
    {
        Engine e;
        Controls c;
        track = true;
        for (int i = 0; i < 10000; i++) {
            c.a = i % 14;
            c.b = (i / 14) % 14;
            e.process(c);
        }
        track = false;
        check(allocations == 0, "audio processing allocates no memory");
    }
    {
        Engine e;
        Controls c;
        double bad = std::numeric_limits<double>::quiet_NaN();
        c.pitch = c.radius = c.offset = c.shape = c.tilt = c.spin = c.level = c.mix = c.voct =
            c.fm = c.radiusCV = c.offsetCV = c.mixCV = c.tiltCV = c.spinCV = c.sync = bad;
        for (int i = 0; i < 1000; i++) {
            auto o = e.process(c);
            check(std::isfinite(o.audio) && std::isfinite(o.xyz.x) && std::isfinite(o.xyz.y) &&
                      std::isfinite(o.xyz.z),
                  "nonfinite input protection");
        }
    }
    auto start = std::chrono::steady_clock::now();
    for (int voices : {1, 16}) {
        std::array<Engine, 16> engines;
        Controls c;
        c.mix = .5;
        double sum = 0.;
        start = std::chrono::steady_clock::now();
        for (int i = 0; i < 48000; i++)
            for (int v = 0; v < voices; v++)
                sum += engines[v].process(c).audio;
        std::cout << voices << " voices, 1s audio: "
                  << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
                  << " s CPU wall, checksum " << sum << "\n";
    }
    std::cout << "PASS " << checks << " checks\n";
}
