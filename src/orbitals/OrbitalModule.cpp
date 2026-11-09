#include "OrbitalModule.hpp"
Orbital::Orbital() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
    std::vector<std::string> labels;
    for (auto n : orbital::names)
        labels.emplace_back(n);
    configSwitch(A, 0, 13, 4, "Orbital A", labels);
    configSwitch(B, 0, 13, 9, "Orbital B", labels);
    configParam(MIX, 0, 1, 0, "Mix", "%", 0, 100);
    configParam(PITCH, -5, 5, 0, "Pitch", " Hz", 2, dsp::FREQ_C4);
    configParam(RADIUS, 0, 24, 6, "Radius", " a0");
    configParam(OFFSET, -24, 24, 1, "Offset", " a0");
    configParam(SHAPE, 0, 1, .35, "Shape");
    configParam(TILT, -1, 1, .2, "Tilt", " degrees", 0, 180);
    configParam(SPIN, -1, 1, 0, "Spin", " Hz", 0, .5);
    configParam(LEVEL, 0, 1, .7, "Level", "%", 0, 100);
    const char *ins[] = {"V/oct (1 V/oct)", "Radius (2.4 a0/V)", "Offset (2.4 a0/V)",
                         "Mix (10 V full range)", "Tilt (36 degrees/V)", "Spin (0.05 Hz/V)"};
    for (int i = 0; i < NUM_INPUTS; i++)
        configInput(i, ins[i]);
    configOutput(AUDIO, "Audio");
    configOutput(X, "X (5 V / 48 a0)");
    configOutput(Y, "Y (5 V / 24 a0)");
    configOutput(Z, "Z (5 V / 34 a0)");
}
void Orbital::process(const ProcessArgs &args) {
    orbital::Controls c;
    c.a = std::lround(params[A].getValue());
    c.b = std::lround(params[B].getValue());
    c.mix = params[MIX].getValue();
    c.pitch = params[PITCH].getValue();
    c.radius = params[RADIUS].getValue();
    c.offset = params[OFFSET].getValue();
    c.shape = params[SHAPE].getValue();
    c.tilt = params[TILT].getValue();
    c.spin = params[SPIN].getValue();
    c.level = params[LEVEL].getValue();
    c.voct = inputs[VOCT].getVoltage();
    c.radiusCV = inputs[RADIUS_CV].getVoltage();
    c.offsetCV = inputs[OFFSET_CV].getVoltage();
    c.mixCV = inputs[MIX_CV].getVoltage();
    c.tiltCV = inputs[TILT_CV].getVoltage();
    c.spinCV = inputs[SPIN_CV].getVoltage();
    auto r = engine.process(c);
    outputs[AUDIO].setVoltage(r.audio);
    outputs[X].setVoltage(r.xyz.x);
    outputs[Y].setVoltage(r.xyz.y);
    outputs[Z].setVoltage(r.xyz.z);
    visualizationTime += args.sampleTime;
    if (visualizationTime >= 1. / 30.) {
        visualizationTime -= 1. / 30.;
        visualization.publish(engine.visualizationState());
    }
}
