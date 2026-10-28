#include "../src/core.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
using namespace contour;
Loop circle(int id, V center, float radius) {
  Loop l;
  l.id = id;
  l.center = center;
  l.age = 1;
  for (int i = 0; i < Points; i++) {
    float a = 2 * M_PI * i / Points;
    l.p[i] = center + V(std::cos(a), std::sin(a)) * radius;
  }
  l.measure();
  return l;
}
int main() {
  assert(cv(.5, 5) == 1 && cv(.5, -5) == 0 && cv(.2, 0) == .2f);
  assert(cv(.5, std::numeric_limits<float>::infinity()) == .5f);
  assert(rate(0, .005, 2) == 0);
  assert(std::abs(rate(1, .005, 2) - 2) < 1e-5);
  assert(10 * 0.f - 5 == -5 && 10 * 1.f - 5 == 5);
  V p{0, 0};
  V q = limitStep(p, {1, 1}, .1f);
  assert(std::abs(length(q - p) - .1) < 1e-6);
  assert(length(limitStep(p, p, .1) - p) == 0);
  Geometry g;
  g.count = 2;
  g.loops[0] = circle(1, {.3, .3}, .1);
  g.loops[1] = circle(2, {.7, .7}, .15);
  std::array<float, 4> zero{{0, 0, 0, 0}}, speed{{.3, .3, .3, .3}};
  Motion m;
  m.adopt(g);
  for (int i = 0; i < 100; i++) {
    auto before = m.balls;
    m.step(.001, 2, zero, .02);
    for (int b = 0; b < 2; b++)
      assert(length(m.balls[b].position - before[b].position) <= .0000202f);
  }
  assert(m.balls[0].id != m.balls[1].id); // reservation includes transitions
  int target = m.balls[0].id;
  g.loops[target - 1].id = 99;
  m.adopt(g);
  V old = m.balls[0].position;
  m.step(.001, 2, zero, .02);
  assert(length(m.balls[0].position - old) <= .0000202f);
  Geometry empty;
  m.adopt(empty);
  old = m.balls[0].position;
  for (int i = 0; i < 100; i++)
    m.step(.01, 4, speed, .25);
  assert(length(m.balls[0].position - old) == 0);
  Geometry bad;
  bad.count = 1;
  bad.loops[0].perimeter = 0;
  m.adopt(bad);
  assert(m.geometry.count == 0);
  // Arc distance and sample-rate invariance, independent of loop size.
  for (float sr : {44100.f, 48000.f, 96000.f}) {
    Motion a;
    a.adopt(g);
    a.step(.01, 1, zero, 4);
    auto &b = a.balls[0];
    b.id = g.loops[0].id;
    b.state = Following;
    b.phase = 0;
    b.position = g.loops[0].at(0);
    b.cooldown = 1;
    for (int i = 0; i < int(sr); i++)
      a.step(1 / sr, 1, speed, 4);
    assert(std::abs(b.phase - .3f / g.loops[0].perimeter) < .001f);
  }
  // Shared paths redistribute only when a persistent free path appears.
  Geometry one = g;
  one.count = 1;
  Motion shared;
  shared.adopt(one);
  for (int i = 0; i < 1000; i++)
    shared.step(.01, 3, zero, 4);
  assert(shared.balls[0].id == shared.balls[1].id);
  assert(length(shared.balls[0].position - shared.balls[1].position) > .01);
  g.loops[1].age = 0;
  shared.adopt(g);
  for (int i = 0; i < 10; i++)
    shared.step(.01, 3, zero, 4);
  assert(shared.balls[0].id != g.loops[1].id);
  g.loops[1].age = .3;
  shared.adopt(g);
  for (int i = 0; i < 100; i++)
    shared.step(.01, 3, zero, 4);
  int users = 0;
  for (int b = 0; b < 3; b++)
    users += shared.balls[b].id == g.loops[1].id;
  assert(users == 1);
  auto start = std::chrono::steady_clock::now();
  int builds = 0, minLoops = 99, maxLoops = 0;
  for (int balls = 1; balls <= 4; balls++)
    for (int levels = 1; levels <= 4; levels++) {
      Terrain terrain;
      Motion motion;
      for (int frame = 0; frame < 24; frame++) {
        Geometry a =
            terrain.build(1234, frame * .03f, float(frame) / 23, levels);
        assert(a.count > 0 && a.count <= MaxLoops);
        minLoops = std::min(minLoops, a.count);
        maxLoops = std::max(maxLoops, a.count);
        for (int i = 0; i < a.count; i++) {
          assert(a.loops[i].level < levels && a.loops[i].perimeter > .08);
          for (V p : a.loops[i].p)
            assert(p.x > 0 && p.x < 1 && p.y > 0 && p.y < 1);
        }
        motion.adopt(a);
        for (int step = 0; step < 1600; step++) {
          auto before = motion.balls;
          motion.step(1.f / 48000, balls, speed, .25);
          for (int b = 0; b < balls; b++) {
            auto &v = motion.balls[b];
            assert(std::isfinite(v.position.x) && std::isfinite(v.position.y));
            if (before[b].state == Transition)
              assert(length(v.position - before[b].position) <
                     .25f / 48000 + 1e-6f);
          }
        }
        builds++;
      }
    }
  Terrain t;
  auto a = t.build(42, 0, .5, 1), b = t.build(42, 0, .5, 4);
  for (int i = 0; i < a.count; i++) {
    bool found = false;
    for (int j = 0; j < b.count; j++)
      if (b.loops[j].level == 0 &&
          length(a.loops[i].center - b.loops[j].center) < 1e-7)
        found = true;
    assert(found);
  }
  std::cout << "PASS: 16 combinations, reservations, hysteresis, transitions, "
               "missing/degenerate geometry, arc speed, 44.1/48/96kHz, CV. "
            << builds << " terrains; loops " << minLoops << ".." << maxLoops
            << "; elapsed "
            << std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                             start)
                   .count()
            << "s\n";
}
