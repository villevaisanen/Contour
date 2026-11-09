#include "OrbitalDisplay.hpp"

// The base stays fixed; only the top rotates about the supplied SVG's circular center.
struct OrbitalsKnob : Knob {
  std::shared_ptr<Svg> base, top;
  OrbitalsKnob() {
    minAngle = -0.75f * M_PI;
    maxAngle = 0.75f * M_PI;
    box.size = mm2px(Vec(14.382292f, 14.382292f));
    base = Svg::load(asset::plugin(pluginInstance, "res/orbitals/knob_static.svg"));
    top = Svg::load(asset::plugin(pluginInstance, "res/orbitals/knob_top.svg"));
  }
  void draw(const DrawArgs &args) override {
    auto *quantity = getParamQuantity();
    float value = quantity ? quantity->getScaledValue() : 0.5f;
    float angle = minAngle + value * (maxAngle - minAngle);
    float scale = box.size.x / mm2px(14.382292f);
    nvgSave(args.vg);
    nvgTranslate(args.vg, box.size.x / 2, box.size.y / 2);
    nvgScale(args.vg, scale, scale);
    nvgSave(args.vg);
    nvgTranslate(args.vg, -mm2px(7.191145f), -mm2px(7.191148f));
    window::svgDraw(args.vg, base->handle);
    nvgRestore(args.vg);
    nvgRotate(args.vg, angle);
    nvgTranslate(args.vg, -mm2px(4.768337f), -mm2px(5.066264f));
    window::svgDraw(args.vg, top->handle);
    nvgRestore(args.vg);
  }
};
struct MixKnob : OrbitalsKnob {
  MixKnob() { box.size = mm2px(Vec(10.4f, 10.4f)); }
};
struct OrbitalsPort : SvgPort {
  OrbitalsPort() {
    setSvg(Svg::load(asset::plugin(pluginInstance, "res/orbitals/jack.svg")));
    shadow->opacity = 0;
  }
};
struct OrbitalWidget : ModuleWidget {
    OrbitalWidget(Orbital *module) {
        setModule(module);
        setPanel(createPanel(
            asset::plugin(pluginInstance, "res/orbitals/orbitals_panel_light.svg"),
            asset::plugin(pluginInstance, "res/orbitals/orbitals_panel_dark.svg")));
        addChild(new OrbitalDisplay(module));
        // Centers from the supplied panel's labeled placeholders, in millimeters.
        auto knob = [&](float x, float y, int id) {
            addParam(createParamCentered<OrbitalsKnob>(mm2px(Vec(x, y)), module, id));
        };
        knob(76.97527f, 17.89754f, Orbital::A);
        knob(76.97527f, 53.27151f, Orbital::B);
        addParam(createParamCentered<MixKnob>(mm2px(Vec(71.19703f, 35.63756f)), module, Orbital::MIX));
        knob(76.97527f, 102.26203f, Orbital::PITCH);
        knob(26.08793f, 85.76049f, Orbital::RADIUS);
        knob(41.42929f, 85.76049f, Orbital::OFFSET);
        knob(10.74659f, 85.76049f, Orbital::SHAPE);
        knob(56.77063f, 85.76049f, Orbital::TILT);
        knob(76.97527f, 72.97239f, Orbital::SPIN);
        knob(13.17073f, 116.63393f, Orbital::LEVEL);
        auto input = [&](float x, float y, int id) {
            addInput(createInputCentered<OrbitalsPort>(mm2px(Vec(x, y)), module, id));
        };
        input(76.97527f, 116.06269f, Orbital::VOCT);
        input(26.07166f, 99.51354f, Orbital::RADIUS_CV);
        input(41.41844f, 99.70060f, Orbital::OFFSET_CV);
        input(82.53787f, 35.25445f, Orbital::MIX_CV);
        input(56.77618f, 99.76674f, Orbital::TILT_CV);
        input(76.97500f, 86.63918f, Orbital::SPIN_CV);
        const float outputs[] = {30.92189f, 40.79958f, 50.67727f, 60.55523f};
        for (int i = 0; i < 4; ++i)
            addOutput(createOutputCentered<OrbitalsPort>(mm2px(Vec(outputs[i], 116.27410f)), module, i));
    }
};
Model *modelOrbital = createModel<Orbital, OrbitalWidget>("Orbitals");
