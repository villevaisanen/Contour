#pragma once
#include "../dsp/VisualizationState.hpp"
#include <cstdint>

namespace orbital {
// Rack-independent display calculation. All storage is fixed at construction; the draw
// method only reads these cells. Spatial samples are UI-only and never feed synthesis.
class DisplayModel {
    // Eight times the spatial budget compensates for projection collisions and
    // quantization, including compact clouds at large scan radii. UI thread only.
    static constexpr int samples = 24576;
    struct Sample {
        Vec3 p;
        float brightness = 0.;
        bool positive = true;
    };
    std::array<Sample, samples> cloud{};
    std::array<double, count> cachedWeights{};
    double fieldExtent = 0.;
    double viewExtent = 12.;
    double viewCenterX = 0., viewCenterY = 0.;
    bool cached = false;
    std::uint32_t rng = 0x4f524249;
    float random() noexcept {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return (rng >> 8) * (1.f / 16777216.f);
    }
    void prepareField(const VisualizationState &s) noexcept {
        int n = 1;
        bool changed = !cached;
        for (int i = 0; i < count; ++i) {
            if (s.coefficients[i] > .005)
                n = std::max(n, i == 0 ? 1 : i < 5 ? 2 : 3);
            changed |= std::abs(s.coefficients[i] - cachedWeights[i]) > .001;
        }
        const double extent = n == 1 ? 4.5 : n == 2 ? 11. : 21.;
        if (extent != fieldExtent) {
            fieldExtent = extent;
            rng = 0x4f524249;
            // Stratified radial samples with deterministic angular grain. This is a
            // qualitative signed-density projection, not a Monte Carlo electron simulation.
            for (int i = 0; i < samples; ++i) {
                const double r = fieldExtent * std::sqrt((i + .5) / samples);
                const double z = 2. * random() - 1., a = 2 * pi * random();
                const double xy = std::sqrt(1. - z * z);
                cloud[i].p = {r * xy * std::cos(a), r * xy * std::sin(a), r * z};
            }
            changed = true;
        }
        if (!changed)
            return;
        double peak = 1e-30;
        for (auto &point : cloud) {
            double psi = 0.;
            for (int i = 0; i < count; ++i)
                if (s.coefficients[i] > 1e-8)
                    psi += s.coefficients[i] * evaluate(i, point.p);
            point.brightness = psi * psi;
            point.positive = psi >= 0.;
            peak = std::max(peak, double(point.brightness));
        }
        // Display-only exposure and gamma reveal weak lobes; signed analytical audio is untouched.
        for (auto &point : cloud)
            point.brightness = std::sqrt(point.brightness / peak);
        cachedWeights = s.coefficients;
        cached = true;
    }

  public:
    static constexpr int width = 128, height = 128, cells = width * height;
    std::array<float, cells> positive{}, negative{}, trail{};
    int head = -1;
    DisplayModel() {
        VisualizationState s;
        s.coefficients = weights(4, 9, 0.);
        s.phase = .17;
        update(s, 1. / 30.);
    }
    // Fixed orthographic camera: horizontal = .8x-.6y; vertical is a perpendicular
    // elevated axis. Equal coordinate scaling preserves geometric proportions.
    int project(Vec3 p) const noexcept {
        const double scale = (height - 6) / (2. * viewExtent);
        int x = std::lround(width * .5 + scale * (.8 * p.x - .6 * p.y - viewCenterX));
        int y = std::lround(height * .5 + scale * (.3 * p.x + .4 * p.y - .8660254 * p.z - viewCenterY));
        if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1)
            return -1;
        return y * width + x;
    }
    void update(const VisualizationState &s, double dt) noexcept {
        prepareField(s);

        positive.fill(0);
        negative.fill(0);
        const float decay = std::exp(-bounded(dt, 0, 1) / .18);
        for (auto &v : trail)
            v *= decay;
        const double cs = std::cos(2 * pi * s.spin), ss = std::sin(2 * pi * s.spin);
        // Fit the main field lobes and the full path to the screen with an 8% margin.
        // Very faint density tails may fall outside the camera, rather than shrinking the lobes.
        // This camera is UI-only; field and trajectory share one undistorted projection.
        PreparedPath path(s.path);
        double minX = 1e30, maxX = -1e30, minY = 1e30, maxY = -1e30;
        auto include = [&](Vec3 p) {
            const double x = .8 * p.x - .6 * p.y;
            const double y = .3 * p.x + .4 * p.y - .8660254 * p.z;
            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minY = std::min(minY, y); maxY = std::max(maxY, y);
        };
        for (const auto &point : cloud)
            if (point.brightness >= .15)
                include({cs * point.p.x - ss * point.p.y,
                         ss * point.p.x + cs * point.p.y, point.p.z});
        for (int i = 0; i < 192; ++i)
            include(path.evaluate(i / 192.));
        viewCenterX = (minX + maxX) * .5;
        viewCenterY = (minY + maxY) * .5;
        viewExtent = std::max(.5, std::max(maxX - minX, maxY - minY) * .54);
        for (int i = 0; i < samples; ++i) {
            const auto &point = cloud[i];
            if (point.brightness < .018)
                continue;
            // Field coordinates -> world is the inverse of the audio sampling transform.
            Vec3 world{cs * point.p.x - ss * point.p.y, ss * point.p.x + cs * point.p.y, point.p.z};
            int cell = project(world);
            if (cell < 0)
                continue;
            auto &v = point.positive ? positive[cell] : negative[cell];
            const float grain = .72f + .28f * ((i * 37 % 101) / 100.f);
            v = std::min(1.f, v + .26f * point.brightness * grain);
        }
        for (int i = 0; i < 192; ++i) {
            int cell = project(path.evaluate(i / 192.));
            if (cell >= 0)
                trail[cell] = std::max(trail[cell], .26f);
        }
        // Sampled DSP phase is strobed at display rate, not slowed to pretend audio
        // frequencies are visible. Phase aliasing at audio rates is expected and documented.
        for (int i = 0; i < 32; ++i) {
            double phase = s.phase - (31 - i) * std::min(.25, s.frequency * .002) / 32.;
            int cell = project(path.evaluate(phase));
            if (cell >= 0)
                trail[cell] = std::max(trail[cell], .3f + .6f * i / 31);
        }
        head = project(path.evaluate(s.phase));
    }
};
} // namespace orbital
