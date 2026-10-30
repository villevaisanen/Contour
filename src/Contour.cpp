// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen

#include "core.hpp"
#include "plugin.hpp"
#include <atomic>
#include <chrono>
#include <thread>
using namespace contour;
namespace contourRack {
const NVGcolor lime = nvgRGBA(0xb9, 0xea, 0x09, 0xff);
struct Frame {
  Geometry geometry;
  std::array<Ball, 4> balls;
  int count = 2;
};
// Single producer / single consumer. Acq-rel atomics transfer exclusive
// ownership. Neither audio callback spins nor waits; serials discard stale
// pending frames.
template <class T> struct Mailbox {
  struct Slot {
    T value;
    std::atomic<int> state{0};
    uint64_t serial = 0;
  };
  std::array<Slot, 3> slots;
  uint64_t serial = 0;         // producer only
  uint64_t consumedSerial = 0; // consumer only
  template <class F> bool write(F fill) {
    for (auto &s : slots) {
      int expected = 0;
      if (s.state.compare_exchange_strong(expected, 1)) {
        fill(s.value);
        s.serial = ++serial;
        s.state.store(2);
        return true;
      }
    }
    return false;
  }
  template <class F> bool read(F consume) {
    Slot *latest = nullptr;
    for (auto &s : slots) {
      int expected = 2;
      if (s.state.compare_exchange_strong(expected, 3)) {
        if (!latest || s.serial > latest->serial)
          latest = &s;
      }
    }
    bool fresh = latest && latest->serial > consumedSerial;
    if (fresh) {
      consumedSerial = latest->serial;
      consume(latest->value);
    }
    for (auto &s : slots)
      if (s.state.load() == 3)
        s.state.store(0);
    return fresh;
  }
};
} // namespace contourRack
using namespace contourRack;
struct Contour : Module {
  enum {
    BALLS,
    CONTOURS,
    EVOLVE,
    FRACTALITY,
    SLEW,
    VEL1,
    VEL2,
    VEL3,
    VEL4,
    NUM_PARAMS
  };
  enum {
    EVOLVE_CV,
    FRACTALITY_CV,
    VEL1_CV,
    VEL2_CV,
    VEL3_CV,
    VEL4_CV,
    NUM_INPUTS
  };
  enum { X1, Y1, X2, Y2, X3, Y3, X4, Y4, NUM_OUTPUTS };
  Motion motion;
  Mailbox<Geometry> terrainMail;
  Mailbox<Frame> displayMail;
  std::atomic<bool> stopping{false};
  std::atomic<float> time{0}, fractal{.4f};
  std::atomic<int> levels{3};
  std::atomic<uint32_t> seed{1};
  std::thread worker;
  std::array<float, 7> smooth{{.35f, .4f, .477f, .4f, .45f, .5f, .55f}};
  std::array<float, 8> voltage{};
  std::array<float, 4> fadeRemaining{};
  float displayClock = 0;
  double noiseTime = 0;
  Contour() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
    configParam(BALLS, 1, 4, 2, "Balls");
    getParamQuantity(BALLS)->snapEnabled = true;
    configParam(CONTOURS, 1, 4, 3, "Contours (brightness levels)");
    getParamQuantity(CONTOURS)->snapEnabled = true;
    configParam(EVOLVE, 0, 1, .35f, "Evolve", " %", 0, 100);
    configParam(FRACTALITY, 0, 1, .4f, "Fractality", " %", 0, 100);
    configParam(SLEW, 0, 1, std::log(.25f / .02f) / std::log(200.f),
                "Transition speed limit", " widths/s", 200, .02f);
    for (int i = 0; i < 4; i++) {
      configParam(VEL1 + i, 0, 1, .4f + .05f * i,
                  "Velocity " + std::to_string(i + 1), " %", 0, 100);
      configInput(VEL1_CV + i, "Velocity " + std::to_string(i + 1));
      configOutput(2 * i, std::to_string(i + 1) + " X");
      configOutput(2 * i + 1, std::to_string(i + 1) + " Y");
    }
    configInput(EVOLVE_CV, "Evolve");
    configInput(FRACTALITY_CV, "Fractality");
    seed.store(random::u32());
    worker = std::thread([this] {
      Terrain terrain; // Small owner; large storage is allocated once on heap.
      while (!stopping.load()) {
        auto start = std::chrono::steady_clock::now();
        const auto &g = terrain.build(seed.load(), time.load(), fractal.load(),
                                      levels.load());
        // Copy into the exclusively reserved mailbox slot before reusing g.
        terrainMail.write([&](Geometry &dest) { dest = g; });
        auto deadline = start + std::chrono::microseconds(33333);
        while (!stopping.load() && std::chrono::steady_clock::now() < deadline)
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    });
  }
  ~Contour() override {
    stopping.store(true);
    worker.join();
  }
  json_t *dataToJson() override {
    json_t *j = json_object();
    json_object_set_new(j, "seed", json_integer(seed.load()));
    return j;
  }
  void dataFromJson(json_t *j) override {
    json_t *s = json_object_get(j, "seed");
    if (json_is_integer(s))
      seed.store(uint32_t(json_integer_value(s)));
  }
  void onReset(const ResetEvent &e) override {
    Module::onReset(e);
    seed.store(random::u32());
  }
  void process(const ProcessArgs &args) override {
    float dt = args.sampleTime, a = 1 - std::exp(-dt / .012f);
    int ps[7] = {EVOLVE, FRACTALITY, SLEW, VEL1, VEL2, VEL3, VEL4};
    int ins[7] = {EVOLVE_CV, FRACTALITY_CV, -1,     VEL1_CV,
                  VEL2_CV,   VEL3_CV,       VEL4_CV};
    for (int i = 0; i < 7; i++) {
      float target = cv(params[ps[i]].getValue(),
                        ins[i] >= 0 ? inputs[ins[i]].getVoltage(0) : 0);
      smooth[i] += (target - smooth[i]) * a;
    }
    noiseTime += rate(smooth[0], .002f, .5f) * dt;
    time.store(float(noiseTime));
    fractal.store(smooth[1]);
    levels.store(int(std::round(params[CONTOURS].getValue())));
    terrainMail.read([&](const Geometry &g) { motion.adopt(g); });
    std::array<float, 4> speeds;
    for (int i = 0; i < 4; i++)
      speeds[i] = rate(smooth[i + 3], .005f, 2.f);
    int count = int(std::round(params[BALLS].getValue()));
    motion.step(dt, count, speeds, .02f * std::pow(200.f, smooth[2]));
    for (int b = 0; b < 4; b++) {
      float fade =
          fadeRemaining[b] > 0 ? std::max(0.f, 1 - dt / fadeRemaining[b]) : 0;
      for (int axis = 0; axis < 2; axis++) {
        int o = b * 2 + axis;
        float p =
            axis ? motion.balls[b].position.y : motion.balls[b].position.x;
        float target = b < count ? 10 * p - 5 : 0;
        if (b < count)
          voltage[o] = target;
        else
          voltage[o] *= fade;
        if (!std::isfinite(voltage[o]))
          voltage[o] = 0;
        outputs[o].setChannels(1);
        outputs[o].setVoltage(voltage[o]);
      }
      fadeRemaining[b] =
          b < count ? .01f : std::max(0.f, fadeRemaining[b] - dt);
    }
    displayClock += dt;
    if (displayClock >= 1.f / 30) {
      displayClock = 0;
      displayMail.write([&](Frame &f) {
        f.geometry = motion.geometry;
        f.balls = motion.balls;
        f.count = count;
      });
    }
  }
};
struct Display : Widget {
  Contour *module = nullptr;
  Frame frame;
  void drawLayer(const DrawArgs &args, int layer) override {
    if (layer != 1)
      return;
    if (module)
      module->displayMail.read([&](const Frame &f) { frame = f; });
    auto point = [&](V p) {
      return Vec(5 + p.x * (box.size.x - 10),
                 5 + (1 - p.y) * (box.size.y - 10));
    };
    nvgSave(args.vg);
    nvgIntersectScissor(args.vg, 0, 0, box.size.x, box.size.y);
    for (int i = 0; i < frame.geometry.count; i++) {
      const auto &l = frame.geometry.loops[i];
      nvgBeginPath(args.vg);
      for (int j = 0; j < Points; j++) {
        Vec p = point(l.p[j]);
        if (j == 0)
          nvgMoveTo(args.vg, p.x, p.y);
        else
          nvgLineTo(args.vg, p.x, p.y);
      }
      nvgClosePath(args.vg);
      nvgStrokeWidth(args.vg, 1);
      nvgStrokeColor(args.vg, lime);
      nvgStroke(args.vg);
    }
    auto font =
        APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
    if (font)
      nvgFontFaceId(args.vg, font->handle);
    for (int i = 0; i < frame.count; i++) {
      Vec p = point(frame.balls[i].position);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg, p.x, p.y, 6);
      nvgFillColor(args.vg, lime);
      nvgFill(args.vg);
      nvgFillColor(args.vg, nvgRGB(15, 20, 25));
      nvgFontSize(args.vg, 9);
      nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
      nvgText(args.vg, p.x, p.y, std::to_string(i + 1).c_str(), nullptr);
    }
    nvgRestore(args.vg);
  }
};
// Draw both supplied SVGs about their actual circular centers. Only the top
// rotates; the static base (including its lighting and shadow) stays fixed.
struct ContourKnob : Knob {
  std::shared_ptr<Svg> base, top;
  ContourKnob() {
    minAngle = -0.75f * M_PI;
    maxAngle = 0.75f * M_PI;
    box.size = mm2px(Vec(14.382292f, 14.382292f));
    base = Svg::load(asset::plugin(pluginInstance, "res/knob_static.svg"));
    top = Svg::load(asset::plugin(pluginInstance, "res/knob_top.svg"));
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
struct VelocityKnob : ContourKnob {
  VelocityKnob() { box.size = mm2px(Vec(8, 8)); }
};
struct ContourPort : SvgPort {
  ContourPort() {
    setSvg(Svg::load(asset::plugin(pluginInstance, "res/jack.svg")));
    shadow->opacity = 0;
  }
};
struct ContourWidget : ModuleWidget {
  ContourWidget(Contour *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Contour_panel.svg")));
    box.size = Vec(20 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    auto *display = new Display;
    display->module = module;
    display->box.pos = mm2px(Vec(21.4f, 11.5f));
    display->box.size = mm2px(Vec(59, 59));
    addChild(display);
    auto knob = [&](float x, float y, int id) {
      addParam(createParamCentered<ContourKnob>(mm2px(Vec(x, y)), module, id));
    };
    auto in = [&](float x, float y, int id) {
      addInput(createInputCentered<ContourPort>(mm2px(Vec(x, y)), module, id));
    };
    // Coordinates from the panel's components layer, in millimeters.
    knob(10.158777f, 20, Contour::BALLS);
    knob(10.158777f, 40, Contour::CONTOURS);
    knob(10.158777f, 60, Contour::SLEW);
    knob(91.486679f, 19.996662f, Contour::EVOLVE);
    in(91.486679f, 32, Contour::EVOLVE_CV);
    knob(91.486679f, 51.162865f, Contour::FRACTALITY);
    in(91.486679f, 63, Contour::FRACTALITY_CV);
    const float velocityX[] = {18.151552f, 40.234154f, 62.288799f, 84.301758f};
    const Vec velocityCV[] = {{18.151552f, 92.04052f}, {40.192844f, 92.04052f},
                              {62.079422f, 91.984695f}, {84.294342f, 91.984695f}};
    const float outputX[] = {18.160337f, 32.711632f, 47.262932f, 61.86422f};
    for (int i = 0; i < 4; i++) {
      addParam(createParamCentered<VelocityKnob>(
          mm2px(Vec(velocityX[i], 82.670471f)), module, Contour::VEL1 + i));
      in(velocityCV[i].x, velocityCV[i].y, Contour::VEL1_CV + i);
      addOutput(createOutputCentered<ContourPort>(
          mm2px(Vec(outputX[i], 110.0976f)), module, 2 * i));
      addOutput(createOutputCentered<ContourPort>(
          mm2px(Vec(outputX[i], 120.01434f)), module, 2 * i + 1));
    }
  }
};
Model *modelContour = createModel<Contour, ContourWidget>("Contour");
