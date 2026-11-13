#include "../../src/orbitals/OrbitalModule.hpp"
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
int main() {
    void *handle = dlopen("./plugin.so", RTLD_NOW | RTLD_LOCAL);
    if (!handle)
        throw std::runtime_error(dlerror());
    auto init = reinterpret_cast<void (*)(rack::Plugin *)>(dlsym(handle, "init"));
    if (!init)
        throw std::runtime_error("init symbol missing");
    rack::Plugin plugin;
    init(&plugin);
    if (plugin.models.size() != 2)
        throw std::runtime_error("model registration");
    auto model = plugin.getModel("Orbitals");
    if (!model || model->slug != "Orbitals")
        throw std::runtime_error("module slug");
    auto m = model->createModule();
    if (m->params.size() != 10 || m->inputs.size() != 6 || m->outputs.size() != 2)
        throw std::runtime_error("ports");
    for (auto &o : m->outputs)
        o.setChannels(1);
    int snapshots = 0;
    for (double sr : {44100., 48000., 96000.}) {
        rack::engine::Module::SampleRateChangeEvent event{};
        event.sampleRate = sr;
        event.sampleTime = 1 / sr;
        m->onSampleRateChange(event);
        rack::engine::Module::ProcessArgs args{};
        args.sampleRate = sr;
        args.sampleTime = 1 / sr;
        for (int i = 0; i < sr; i++) {
            m->process(args);
            orbital::VisualizationState snapshot;
            if (static_cast<Orbital *>(m)->visualization.latest(snapshot)) {
                if (snapshot.frequency < 260 || snapshot.frequency > 263 ||
                    snapshot.coefficients[4] < .99)
                    throw std::runtime_error("invalid DSP visualization snapshot");
                ++snapshots;
            }
            for (auto &o : m->outputs)
                if (!std::isfinite(o.getVoltage()))
                    throw std::runtime_error("output");
        }
    }
    std::cout << "PASS plugin.so dlopen, model factory, 10 params / 6 inputs / 2 outputs, "
                 "44.1/48/96k process\n";
    if (snapshots < 85 || snapshots > 95)
        throw std::runtime_error("snapshot rate");
    std::cout << "PASS actual Rack module publishes " << snapshots << " snapshots in 3 seconds\n";
    // Exercise the actual binary against independent, unchanged DSP engines.
    // Port channels are set directly to emulate Rack's cable bookkeeping.
    for (double rate : {44100., 48000., 96000.}) {
        auto *subject = model->createModule();
        orbital::Engine mono, left, right;
        mono.setSampleRate(rate); left.setSampleRate(rate); right.setSampleRate(rate);
        rack::engine::Module::SampleRateChangeEvent event{};
        event.sampleRate = rate; subject->onSampleRateChange(event);
        rack::engine::Module::ProcessArgs args{};
        args.sampleRate = rate; args.sampleTime = 1. / rate;
        orbital::Controls c;
        double difference = 0., energyA = 0., energyB = 0.;
        for (int stage = 0; stage < 12; ++stage) {
            // Mono, stereo, R-only, CV endpoints, interior balance, unplug/replug.
            const bool stereo = stage != 0 && stage != 9 && stage != 11;
            subject->outputs[Orbital::L].channels = stage == 5 ? 0 : 1;
            subject->outputs[Orbital::R].channels = stereo ? 1 : 0;
            const double mixes[] = {.3,.5,0,1,.25,.5,.75,.5,.5,.7,.5,.2};
            c.mix = float(mixes[stage]);
            c.mixCV = stage == 7 ? -10 : stage == 8 ? 10 : 0;
            subject->params[Orbital::MIX].setValue(c.mix);
            subject->inputs[Orbital::MIX_CV].channels = 1;
            subject->inputs[Orbital::MIX_CV].setVoltage(c.mixCV);
            for (int i = 0; i < int(rate * .15); ++i) {
                // Modulation and repeated selector changes also cover mono equivalence.
                if (!stereo && i % 113 == 0) {
                    c.a = (i / 113) % 14; c.b = (i / 317) % 14;
                    subject->params[Orbital::A].setValue(c.a);
                    subject->params[Orbital::B].setValue(c.b);
                }
                if (!stereo) {
                    c.mixCV = float(8. * std::sin(i * .013));
                    subject->inputs[Orbital::MIX_CV].setVoltage(c.mixCV);
                }
                auto expectedMono = mono.process(c);
                auto single = c; single.mix = single.mixCV = 0;
                auto expectedA = left.process(single);
                single.a = c.b;
                auto expectedB = right.process(single);
                subject->process(args);
                const double l = subject->outputs[Orbital::L].getVoltage();
                const double r = subject->outputs[Orbital::R].getVoltage();
                auto close = [](double a, double b) {
                    if (std::abs(a-b) > 2e-5) throw std::runtime_error("routing mismatch");
                };
                if (!stereo) {
                    close(l, expectedMono.audio); close(r, 0.);
                } else if (i > rate * .1) {
                    double ga=1, gb=1;
                    if (stage==2 || stage==7) gb=0;
                    if (stage==3 || stage==8) ga=0;
                    if (stage==4) gb=.5;
                    if (stage==6) ga=.5;
                    close(l, expectedA.audio*ga); close(r, expectedB.audio*gb);
                    difference += std::abs(expectedA.audio-expectedB.audio);
                    energyA += expectedA.audio*expectedA.audio;
                    energyB += expectedB.audio*expectedB.audio;
                }
                if (!std::isfinite(l) || !std::isfinite(r) || std::abs(l)>5.00001 || std::abs(r)>5.00001)
                    throw std::runtime_error("routing output bounds");
            }
        }
        if (difference < 1 || energyA < 1 || energyB < 1)
            throw std::runtime_error("routing test needs distinct audible sources");
        delete subject;
    }
    std::cout << "PASS actual module mono equivalence, stereo separation, unity center, endpoints/interior balance, CV clamp, R-only and live cable changes at 44.1/48/96kHz\n";
    delete m;
    // Plugin owns models; its destructor runs before process-wide library cleanup.
}
