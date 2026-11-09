#pragma once
#include "VisualizationState.hpp"
namespace orbital {
struct Controls {
    int a = 4, b = 9;
    double mix = 0., pitch = 0., radius = 6., offset = 1., shape = .35, tilt = .2, spin = 0.,
           level = .7;
    double voct = 0., fm = 0., radiusCV = 0., offsetCV = 0., mixCV = 0., tiltCV = 0., spinCV = 0.,
           sync = 0.;
};
struct Result {
    double audio;
    Vec3 xyz;
};
class Engine {
    static constexpr int taps = 95, os = 4;
    std::array<double, taps> fir{}, history{};
    std::array<double, count> state{};
    Controls smooth;
    Path visualizationPath;
    double visualizationFrequency = 0.;
    bool initialized = false, gate = false;
    int head = 0;
    double rate = 48000., slew = 0., dcPole = 0., prev = 0., dc = 0., spinPhase = 0.;

  public:
    double phase = 0.;
    Engine() {
        double sum = 0.;
        for (int i = 0; i < taps; i++) {
            double x = i - (taps - 1) * .5;
            double sinc = x == 0 ? .21 : std::sin(2 * pi * .105 * x) / (pi * x);
            fir[i] = sinc * (.42 - .5 * std::cos(2 * pi * i / (taps - 1)) +
                             .08 * std::cos(4 * pi * i / (taps - 1)));
            sum += fir[i];
        }
        for (double &f : fir)
            f /= sum;
        setSampleRate(48000.);
    }
    void setSampleRate(double sr) noexcept {
        rate = bounded(sr, 8000, 768000);
        if (rate == 0)
            rate = 48000;
        slew = 1. - std::exp(-1. / (.005 * rate));
        dcPole = std::exp(-2 * pi * 5 / rate);
    }
    static double frequency(double pitch, double voct, double fm, double sr) noexcept {
        double oct = bounded(pitch, -5, 5) + bounded(voct, -10, 10) + bounded(fm, -10, 10);
        return std::min(std::min(20000., .45 * sr),
                        261.6255653005986 * std::exp2(bounded(oct, -16, 16)));
    }
    VisualizationState visualizationState() const noexcept {
        VisualizationState s;
        s.path = visualizationPath;
        s.coefficients = state;
        s.phase = phase;
        s.spin = spinPhase;
        s.frequency = visualizationFrequency;
        return s;
    }
    Result process(const Controls &c) noexcept {
        Controls target = c;
        target.pitch = bounded(c.pitch, -5, 5);
        target.radius = bounded(c.radius, 0, 24);
        target.offset = bounded(c.offset, -24, 24);
        target.shape = bounded(c.shape, 0, 1);
        target.tilt = bounded(c.tilt, -1, 1);
        target.spin = bounded(c.spin, -1, 1);
        target.level = bounded(c.level, 0, 1);
        target.mix = bounded(c.mix, 0, 1);
        auto w = weights(c.a, c.b, bounded(target.mix + bounded(c.mixCV, -10, 10) / 10, 0, 1));
        if (!initialized) {
            smooth = target;
            state = w;
            initialized = true;
        }
        auto chase = [this](double &x, double t) { x += slew * (t - x); };
        chase(smooth.pitch, target.pitch);
        chase(smooth.radius, target.radius);
        chase(smooth.offset, target.offset);
        chase(smooth.shape, target.shape);
        chase(smooth.tilt, target.tilt);
        chase(smooth.spin, target.spin);
        chase(smooth.level, target.level);
        // Smooth the whole state vector, so repeated state changes never restart a fade.
        for (int i = 0; i < count; i++)
            chase(state[i], w[i]);
        Path path;
        path.radius = bounded(smooth.radius + bounded(c.radiusCV, -10, 10) * 2.4, 0, 24);
        path.offset = bounded(smooth.offset + bounded(c.offsetCV, -10, 10) * 2.4, -24, 24);
        path.shape = smooth.shape;
        path.tilt = bounded(smooth.tilt + bounded(c.tiltCV, -10, 10) / 5, -1, 1);
        double sync = bounded(c.sync, -100, 100);
        if (!gate && sync >= 1.) {
            phase = 0.;
            spinPhase = 0.;
            gate = true;
        }
        if (sync <= .1)
            gate = false;
        double hz = frequency(smooth.pitch, c.voct, c.fm, rate);
        double spinHz = bounded(smooth.spin + bounded(c.spinCV, -10, 10) / 10, -1, 1) * .5;
        visualizationPath = path;
        visualizationFrequency = hz;
        PreparedPath prepared(path);
        Vec3 position{};
        for (int k = 0; k < os; k++) {
            position = prepared.evaluate(phase);
            Vec3 q = fieldCoordinates(position, spinPhase);
            double raw = 0.;
            for (int i = 0; i < count; i++)
                if (state[i] > 1e-10)
                    raw += state[i] * evaluate(i, q);
            // One common gain preserves the superposition; no per-state reweighting or AGC.
            history[head] = raw * 40.;
            head = (head + 1) % taps;
            phase += hz / (os * rate);
            phase -= std::floor(phase);
            spinPhase += spinHz / (os * rate);
            spinPhase -= std::floor(spinPhase);
        }
        double filtered = 0.;
        for (int i = 0; i < taps; i++)
            filtered += fir[i] * history[(head + taps - 1 - i) % taps];
        dc = filtered - prev + dcPole * dc;
        prev = filtered;
        // Bounded linear output; only extremes reach the safety limiter.
        double audio = bounded(dc * smooth.level, -5, 5);
        return {audio,
                {5 * bounded(position.x / 48, -1, 1), 5 * bounded(position.y / 24, -1, 1),
                 5 * bounded(position.z / 34, -1, 1)}};
    }
};
} // namespace orbital
