#include "OrbitalModule.hpp"
Orbital::Orbital() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
    std::vector<std::string> labels;
    for (auto n : orbital::names)
        labels.emplace_back(n);
    configSwitch(A, 0, 13, 4, "Orbital A", labels);
    configSwitch(B, 0, 13, 9, "Orbital B", labels);
    configParam(MIX, 0, 1, 0, "Mix / Balance (mono A/B mix; stereo L/R balance)", "%", 0, 100);
    configParam(PITCH, -5, 5, 0, "Pitch", " Hz", 2, dsp::FREQ_C4);
    configParam(RADIUS, 0, 24, 6, "Radius", " a0");
    configParam(OFFSET, -24, 24, 1, "Offset", " a0");
    configParam(SHAPE, 0, 1, .35, "Shape");
    configParam(TILT, -1, 1, .2, "Tilt", " degrees", 0, 180);
    configParam(SPIN, -1, 1, 0, "Spin", " Hz", 0, .5);
    configParam(LEVEL, 0, 1, .7, "Level", "%", 0, 100);
    const char *ins[] = {"V/oct (1 V/oct)", "Radius (2.4 a0/V)", "Offset (2.4 a0/V)",
                         "Mix / Balance (10 V full range)", "Tilt (36 degrees/V)", "Spin (0.05 Hz/V)"};
    for (int i = 0; i < NUM_INPUTS; i++)
        configInput(i, ins[i]);
    configOutput(L, "L (MONO): A/B mix; stereo oscillator A");
    configOutput(R, "R: oscillator B; cable activates stereo balance");
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
    // Keep all three histories running so cable changes never restart phase or filters.
    auto separate = c;
    separate.mix = separate.mixCV = 0.;
    auto a = engineA.process(separate);
    separate.a = c.b;
    auto b = engineB.process(separate);
    const double target = orbital::bounded(orbital::bounded(c.mix, 0, 1) +
                                           orbital::bounded(c.mixCV, -10, 10) / 10, 0, 1);
    if (!balanceInitialized) {
        balance = target;
        balanceInitialized = true;
    }
    balance += (1. - std::exp(-1. / (.005 * args.sampleRate))) * (target - balance);
    const bool stereo = outputs[R].isConnected();
    // Piecewise linear balance: unity at center, attenuation only, no crossfeed.
    outputs[L].setVoltage(stereo ? a.audio * std::min(1., 2. * (1. - balance)) : r.audio);
    outputs[R].setVoltage(stereo ? b.audio * std::min(1., 2. * balance) : 0.);
    visualizationTime += args.sampleTime;
    if (visualizationTime >= 1. / 30.) {
        visualizationTime -= 1. / 30.;
        visualization.publish(engine.visualizationState());
    }
}
