// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen

// Runs the real Rack module, worker, ports and persistence without a window.
#include "../src/Contour.cpp"
#include <cassert>
#include <iostream>
Plugin *pluginInstance = nullptr;
int main() {
  auto *m = new Contour;
  for (auto &port : m->outputs)
    port.channels = 1; // Emulate Rack cable connections.
  Module::ProcessArgs args{};
  args.sampleRate = 48000;
  args.sampleTime = 1.f / 48000;
  m->params[Contour::EVOLVE].setValue(0);
  for (int i = 0; i < 1000; i++)
    m->process(args);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  bool moved = false;
  for (int i = 0; i < 48000; i++) {
    m->process(args);
    for (int b = 0; b < 4; b++)
      for (int a = 0; a < 2; a++) {
        float v = m->outputs[b * 2 + a].getVoltage();
        assert(std::isfinite(v) && v >= -5.001 && v <= 5.001);
        assert(m->outputs[b * 2 + a].getChannels() == 1);
        if (b < 2) {
          float p =
              a ? m->motion.balls[b].position.y : m->motion.balls[b].position.x;
          assert(std::abs(v - (10 * p - 5)) < 1e-6);
          moved |= std::abs(v) > .01;
        }
      }
  }
  assert(moved);
  m->inputs[Contour::EVOLVE_CV].channels = 2;
  m->inputs[Contour::EVOLVE_CV].setVoltage(5, 0);
  m->inputs[Contour::EVOLVE_CV].setVoltage(-5, 1);
  for (int i = 0; i < 4800; i++)
    m->process(args);
  assert(m->smooth[0] > .99);
  m->params[Contour::BALLS].setValue(1);
  for (int i = 0; i < 500; i++)
    m->process(args);
  assert(m->outputs[Contour::X2].getVoltage() == 0 &&
         m->outputs[Contour::Y2].getVoltage() == 0);
  m->seed.store(1234567890);
  json_t *saved = m->dataToJson();
  m->seed.store(0);
  m->dataFromJson(saved);
  assert(m->seed.load() == 1234567890);
  json_decref(saved);
  // The display snapshot contains the exact same coordinates as the output.
  m->displayMail.read([](const Frame &) {});
  m->displayClock = 1;
  m->process(args);
  bool snapshot = m->displayMail.read([&](const Frame &f) {
    assert(std::abs(m->outputs[0].getVoltage() -
                    (10 * f.balls[0].position.x - 5)) < 1e-6);
    assert(std::abs(m->outputs[1].getVoltage() -
                    (10 * f.balls[0].position.y - 5)) < 1e-6);
  });
  assert(snapshot);
  // Concurrent producer and consumer, including full-mailbox frame drops.
  Mailbox<uint64_t> mailbox;
  std::atomic<bool> done{false};
  std::thread producer([&] {
    for (uint64_t i = 1; i <= 100000; i++)
      mailbox.write([&](uint64_t &v) { v = i; });
    done.store(true);
  });
  uint64_t latest = 0;
  auto consume = [&](uint64_t value) {
    assert(value > latest);
    latest = value;
  };
  while (!done.load())
    mailbox.read(consume);
  producer.join();
  mailbox.read(consume);
  assert(latest > 0);
  delete m;
  for (int i = 0; i < 16; i++) {
    auto *module = new Contour;
    delete module;
  }
  // Several live workers must keep their storage and saved seeds independent.
  std::array<std::unique_ptr<Contour>, 4> modules;
  for (int i = 0; i < 4; ++i) {
    modules[i].reset(new Contour);
    json_t *j = json_object();
    json_object_set_new(j, "seed", json_integer(1000 + i));
    modules[i]->dataFromJson(j);
    json_decref(j);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(75));
  for (int step = 0; step < 4800; ++step)
    for (auto &module : modules)
      module->process(args);
  for (int i = 0; i < 4; ++i) {
    assert(modules[i]->motion.geometry.count > 0);
    json_t *j = modules[i]->dataToJson();
    assert(json_integer_value(json_object_get(j, "seed")) == 1000 + i);
    json_decref(j);
  }
  std::cout
      << "PASS: real Rack module/worker lifecycle, eight mono ports, exact "
         "displayed XY/CV mapping, poly CV channel 0, output fade, seed JSON, "
         "four concurrent instances\n";
}
