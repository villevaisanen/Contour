#include "../../src/orbitals/dsp/OrbitalEngine.hpp"
#ifndef DISPLAY_MODEL_HEADER
#define DISPLAY_MODEL_HEADER "../../src/orbitals/ui/OrbitalDisplayModel.hpp"
#endif
#include DISPLAY_MODEL_HEADER
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <memory>
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
// Keep the malloc-backed replacement out of line for GCC allocation diagnostics.
#if defined(__GNUC__)
__attribute__((noinline))
#endif
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
// dev.3 counts for --density's deterministic states/radii, after the actual
// 5-level draw quantizer. Count unique screen cells, excluding the path layer.
void checkDensity() {
    const int oldCloud[6][3]={{117,152,123},{300,295,313},{64,64,92},
                            {354,354,362},{264,264,269},{84,84,95}};
    const int oldUnobscured[6][3]={{116,145,116},{298,276,298},{62,58,87},
                                 {340,325,346},{259,253,259},{82,82,90}};
    const int states[]={0,4,6,9,13,-1};const double radii[]={0.,6.,24.};
    for(int row=0;row<6;++row)for(int col=0;col<3;++col) {
        VisualizationState s;
        int state=states[row];
        s.coefficients=state<0?weights(4,9,.3):weights(state,state,0.);
        s.path.radius=radii[col];s.path.offset=1.;s.path.shape=.35;s.path.tilt=.2;
        s.phase=.17;s.spin=.125;s.frequency=261.625565;
        DisplayModel model;model.update(s,1./30.);
        int cells=0,unobscured=0;
        for(int i=0;i<DisplayModel::cells;++i) {
            bool cloud=int(model.positive[i]*5+.5f)>0||int(model.negative[i]*5+.5f)>0;
            cells+=cloud;unobscured+=cloud&&int(model.trail[i]*5+.5f)==0;
        }
        require(cells>=2*oldCloud[row][col], "at least 2x distinct visible cloud cells vs dev.3");
        require(unobscured>=2*oldUnobscured[row][col], "at least 2x cloud cells outside trajectory vs dev.3");
        // A new cloud frame replaces the old one, with no cloud persistence.
        auto pos=model.positive,neg=model.negative;
        model.update(s,1./30.);
        require(model.positive==pos&&model.negative==neg,"cloud clears and rebuilds without accumulation");
    }
}
// Optional paired dev.3/dev.4 measurement using the exact renderer quantizer.
int measure(bool timing = true) {
    auto visible = [](float value) { return std::min(5, int(value * 5 + .5f)) > 0; };
    for (int state : {0, 4, 6, 9, 13, -1})
        for (double radius : {0., 6., 24.}) {
            VisualizationState s;
            s.coefficients = state < 0 ? weights(4,9,.3) : weights(state,state,0.);
            s.path.radius = radius;s.path.offset=1.;s.path.shape=.35;s.path.tilt=.2;
            s.phase=.17;s.spin=.125;s.frequency=261.625565;
            DisplayModel model;model.update(s,1./30.);
            int cells=0, unobscured=0;
            for(int i=0;i<DisplayModel::cells;++i) {
                bool cloud=visible(model.positive[i])||visible(model.negative[i]);
                cells+=cloud;unobscured+=cloud&&!visible(model.trail[i]);
            }
            std::cout<<"DENSITY "<<state<<' '<<radius<<' '<<cells<<' '<<unobscured<<'\n';
        }
    if(!timing)return 0;
    // Median of five passes, same states, deterministic seed, compiler and hardware.
    for(int instances : {1,8}) for(bool changing : {false,true}) {
        std::array<double,5> times{};
        double checksum=0.;
        for(auto& time:times) {
            std::unique_ptr<std::array<DisplayModel,8>> models(new std::array<DisplayModel,8>);
            VisualizationState s;s.coefficients=weights(4,9,.3);s.path.radius=6.;
            auto start=std::chrono::steady_clock::now();
            for(int frame=0;frame<120;++frame) {
                s.spin=frame*.001;s.phase=frame*.013;
                if(changing)s.coefficients=weights(4,9,.2+(frame%100)*.006);
                for(int i=0;i<instances;++i)(*models)[i].update(s,1./30.);
            }
            time=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/120;
            for(int i=0;i<instances;++i)checksum+=(*models)[i].positive[8192];
        }
        std::sort(times.begin(),times.end());
        std::cout<<"PERF "<<instances<<' '<<changing<<' '<<times[2]<<' '<<checksum<<'\n';
    }
    return 0;
}
int main(int argc, char** argv) {
    if(argc==2 && std::string(argv[1])=="--measure")return measure();
    if(argc==2 && std::string(argv[1])=="--density")return measure(false);
    checkDensity();
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
