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
    if (m->params.size() != 10 || m->inputs.size() != 6 || m->outputs.size() != 4)
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
    std::cout << "PASS plugin.so dlopen, model factory, 10 params / 6 inputs / 4 outputs, "
                 "44.1/48/96k process\n";
    if (snapshots < 85 || snapshots > 95)
        throw std::runtime_error("snapshot rate");
    std::cout << "PASS actual Rack module publishes " << snapshots << " snapshots in 3 seconds\n";
    delete m;
    // Plugin owns models; its destructor runs before process-wide library cleanup.
}
