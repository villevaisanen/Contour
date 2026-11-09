#include "../../src/orbitals/dsp/OrbitalEngine.hpp"
#include "../../src/orbitals/ui/OrbitalDisplayModel.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>

static bool tracking = false;
static int allocations = 0;
void *operator new(std::size_t n) {
    if (tracking)
        ++allocations;
    if (void *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void require(bool b, const char *message) {
    if (!b) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
}
using namespace orbital;
double difference(const DisplayModel &a, const DisplayModel &b) {
    double d = 0.;
    for (int i = 0; i < DisplayModel::cells; ++i)
        d += std::abs(a.positive[i] - b.positive[i]) + std::abs(a.negative[i] - b.negative[i]) +
             std::abs(a.trail[i] - b.trail[i]);
    return d;
}
int main() {
    static_assert(DisplayModel::width == 128 && DisplayModel::height == 128, "128x128 display");
    VisualizationQueue q;
    VisualizationState s, out;
    require(!q.latest(out), "empty queue");
    for (int i = 0; i < 7; ++i) {
        s.phase = i;
        require(q.publish(s), "available slot");
    }
    require(!q.publish(s), "full queue drops without blocking");
    require(q.latest(out) && out.phase == 6, "latest snapshot, not oldest");
    require(!q.latest(out), "consumed queue");
    for (int i = 0; i < 100; ++i) {
        s.phase = i;
        require(q.publish(s) && q.latest(out) && out.phase == i, "ring wraparound");
    }
    std::atomic<bool> done{false};
    std::thread producer([&] {
        VisualizationState v;
        for (int i = 1; i <= 200000; ++i) {
            v.phase = i;
            v.spin = -i;
            v.frequency = 3. * i;
            v.path.radius = i;
            v.coefficients.fill(i);
            q.publish(v);
        }
        done.store(true, std::memory_order_release);
    });
    double last = 0.;
    int received = 0;
    do {
        if (q.latest(out)) {
            require(out.phase > last, "monotonic snapshots");
            last = out.phase;
            require(out.spin == -out.phase && out.frequency == 3 * out.phase &&
                        out.path.radius == out.phase,
                    "coherent payload");
            for (double w : out.coefficients)
                require(w == out.phase, "untorn coefficient vector");
            ++received;
        }
    } while (!done.load(std::memory_order_acquire));
    producer.join();
    require(received > 0, "concurrent consumer receives snapshots");

    Engine e;
    Controls c;
    c.a = 2;
    c.b = 9;
    c.mix = .3;
    c.radiusCV = 2;
    c.offsetCV = -1;
    c.mixCV = 2;
    c.tiltCV = 1;
    c.spinCV = 1;
    for (int i = 0; i < 1000; i++)
        e.process(c);
    s = e.visualizationState();
    require(std::abs(s.path.radius - 10.8) < 1e-12 && std::abs(s.path.offset + 1.4) < 1e-12,
            "effective CV geometry");
    require(std::abs(s.path.tilt - .4) < 1e-12, "effective tilt CV");
    require(std::abs(s.coefficients[2] - std::sqrt(.5)) < 1e-12 &&
                std::abs(s.coefficients[9] - std::sqrt(.5)) < 1e-12,
            "effective mix CV");
    require(s.spin > 0 && s.frequency > 260 && s.phase > 0, "real phase and spin");
    DisplayModel base;
    base.update(s, 1. / 30);
    for (int change = 0; change < 9; ++change) {
        auto altered = s;
        switch (change) {
        case 0:
            altered.coefficients = weights(12, 9, .5);
            break;
        case 1:
            altered.coefficients = weights(2, 13, .5);
            break;
        case 2:
            altered.coefficients = weights(2, 9, .9);
            break;
        case 3:
            altered.path.radius = 12;
            break;
        case 4:
            altered.path.offset = 5;
            break;
        case 5:
            altered.path.shape = .9;
            break;
        case 6:
            altered.path.tilt = -.5;
            break;
        case 7:
            altered.spin = .25;
            break;
        case 8:
            altered.phase += .3;
            altered.frequency *= 2;
            break;
        }
        DisplayModel other;
        other.update(altered, 1. / 30);
        require(difference(base, other) > .1, "every requested control changes display");
    }
    // Adaptive framing keeps the entire trajectory visible, including extreme offsets.
    for (double radius : {0., 6., 24.})
        for (double offset : {-24., 0., 24.}) {
            auto framed = s;
            framed.path.radius = radius;
            framed.path.offset = offset;
            DisplayModel camera;
            camera.update(framed, 1. / 30.);
            PreparedPath path(framed.path);
            for (int i = 0; i < 192; ++i)
                require(camera.project(path.evaluate(i / 192.)) >= 0, "full trajectory fits camera");
            require(camera.head >= 0, "head fits camera");
        }
    tracking = true;
    for (int i = 0; i < 100; i++) {
        s.spin += .01;
        s.coefficients = weights(i % 14, (i + 1) % 14, .4);
        base.update(s, 1. / 30);
    }
    tracking = false;
    require(allocations == 0, "no allocations during display updates");
    for (int i = 0; i < DisplayModel::cells; i++)
        require(std::isfinite(base.positive[i]) && base.positive[i] >= 0 && base.positive[i] <= 1 &&
                    std::isfinite(base.negative[i]) && std::isfinite(base.trail[i]),
                "finite bounded matrix");
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 300; i++) {
        s.spin += .001;
        base.update(s, 1. / 30);
    }
    std::cout << "PASS queue saturation/wrap, " << received
              << " concurrent snapshots, CV state, 9 parameter reactions, allocation-free matrix; "
                 "cached update "
              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                         .count() /
                     300
              << " ms/frame\n";
}
