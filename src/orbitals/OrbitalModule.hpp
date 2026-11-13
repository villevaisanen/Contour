#pragma once
#include "dsp/OrbitalEngine.hpp"
#include "../plugin.hpp"
struct Orbital : Module {
    enum ParamIds { A, B, MIX, PITCH, RADIUS, OFFSET, SHAPE, TILT, SPIN, LEVEL, NUM_PARAMS };
    enum InputIds { VOCT, RADIUS_CV, OFFSET_CV, MIX_CV, TILT_CV, SPIN_CV, NUM_INPUTS };
    // Keep legacy AUDIO=0 as L and X=1 as R; former Y/Z ports are removed.
    enum OutputIds { L, R, NUM_OUTPUTS };
    orbital::Engine engine, engineA, engineB;
    double balance = 0.;
    bool balanceInitialized = false;
    orbital::VisualizationQueue visualization;
    double visualizationTime = 0.;
    Orbital();
    void process(const ProcessArgs &args) override;
    void onSampleRateChange(const SampleRateChangeEvent &e) override {
        engine.setSampleRate(e.sampleRate);
        engineA.setSampleRate(e.sampleRate);
        engineB.setSampleRate(e.sampleRate);
    }
    void onReset(const ResetEvent &e) override {
        Module::onReset(e);
        engine = engineA = engineB = orbital::Engine();
        balanceInitialized = false;
        engine.setSampleRate(APP->engine->getSampleRate());
        engineA.setSampleRate(APP->engine->getSampleRate());
        engineB.setSampleRate(APP->engine->getSampleRate());
    }
};
