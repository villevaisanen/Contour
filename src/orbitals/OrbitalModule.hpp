#pragma once
#include "dsp/OrbitalEngine.hpp"
#include "../plugin.hpp"
struct Orbital : Module {
    enum ParamIds { A, B, MIX, PITCH, RADIUS, OFFSET, SHAPE, TILT, SPIN, LEVEL, NUM_PARAMS };
    enum InputIds { VOCT, RADIUS_CV, OFFSET_CV, MIX_CV, TILT_CV, SPIN_CV, NUM_INPUTS };
    enum OutputIds { AUDIO, X, Y, Z, NUM_OUTPUTS };
    orbital::Engine engine;
    orbital::VisualizationQueue visualization;
    double visualizationTime = 0.;
    Orbital();
    void process(const ProcessArgs &args) override;
    void onSampleRateChange(const SampleRateChangeEvent &e) override {
        engine.setSampleRate(e.sampleRate);
    }
    void onReset(const ResetEvent &e) override {
        Module::onReset(e);
        engine = orbital::Engine();
        engine.setSampleRate(APP->engine->getSampleRate());
    }
};
