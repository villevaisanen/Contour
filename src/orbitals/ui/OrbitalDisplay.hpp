#pragma once
#include "../OrbitalModule.hpp"
#include "OrbitalDisplayModel.hpp"

struct OrbitalDisplay : widget::Widget {
    Orbital *module = nullptr;
    orbital::DisplayModel display;
    orbital::VisualizationState state;
    double nextUpdate = 0., lastUpdate = 0.;
    explicit OrbitalDisplay(Orbital *m) : module(m) {
        state.coefficients = orbital::weights(4, 9, 0.);
        state.phase = .17;
        box.pos = mm2px(Vec(4.33987f, 10.94057f));
        box.size = mm2px(Vec(59, 59));
    }
    void step() override {
        Widget::step();
        double now = system::getTime();
        if (now < nextUpdate)
            return;
        const double dt = lastUpdate == 0. ? 1. / 30. : now - lastUpdate;
        lastUpdate = now;
        nextUpdate = now + 1. / 30.;
        if (module)
            module->visualization.latest(state);
        // Browser preview stays deterministic and does not access an audio module.
        display.update(state, dt);
    }
    // Draw only the live phosphor layer over the panel's own screen and grid.
    // No opaque backing or decorative stroke; the Rack framebuffer keeps alpha.
    void drawLayer(const DrawArgs &args, int layer) override {
        if (layer == 1) {
            nvgSave(args.vg);
            nvgIntersectScissor(args.vg, 2, 2, box.size.x - 4, box.size.y - 4);
            const float dx = box.size.x / orbital::DisplayModel::width;
            const float dy = box.size.y / orbital::DisplayModel::height;
            // Quantized brightness batches: bounded path count, no per-dot allocations.
            // Rectangular phosphor cells leave dark gaps for the instrument's matrix texture.
            for (int channel = 0; channel < 3; ++channel) {
                const auto &values = channel == 0   ? display.positive
                                     : channel == 1 ? display.negative
                                                    : display.trail;
                for (int level = 1; level <= 5; ++level) {
                    nvgBeginPath(args.vg);
                    for (int i = 0; i < orbital::DisplayModel::cells; ++i) {
                        int brightness = std::min(5, int(values[i] * 5 + .5f));
                        if (brightness != level)
                            continue;
                        nvgRect(args.vg, (i % orbital::DisplayModel::width) * dx + dx * .135f,
                                (i / orbital::DisplayModel::width) * dy + dy * .135f, dx * .73f,
                                dy * .73f);
                    }
                    NVGcolor color = channel == 1
                                         ? nvgRGBA(217, 185, 83, 35 + 40 * level)
                                         : nvgRGBA(channel == 2 ? 206 : 137, 236,
                                                   channel == 2 ? 139 : 93, 35 + 40 * level);
                    nvgFillColor(args.vg, color);
                    nvgFill(args.vg);
                }
            }
            if (display.head >= 0) {
                float x = (display.head % orbital::DisplayModel::width) * dx + dx * .5f;
                float y = (display.head / orbital::DisplayModel::width) * dy + dy * .5f;
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, x, y, 3.8f);
                nvgFillColor(args.vg, nvgRGBA(194, 250, 130, 35));
                nvgFill(args.vg);
                nvgBeginPath(args.vg);
                nvgRect(args.vg, x - 1.3f, y - 1.3f, 2.6f, 2.6f);
                nvgFillColor(args.vg, nvgRGB(230, 255, 195));
                nvgFill(args.vg);
            }
            nvgRestore(args.vg);
        }
        Widget::drawLayer(args, layer);
    }
};
