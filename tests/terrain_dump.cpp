// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen

// Same driver for the original and current core: compare active fields only,
// never struct padding or unused loops. Output is a host-native binary stream.
#include TERRAIN_HEADER
#include <cassert>
#include <cstdio>
#include <memory>
using namespace contour;
template <class T> void emit(const T &v) {
  assert(std::fwrite(&v, sizeof(v), 1, stdout) == 1);
}
int main() {
  std::unique_ptr<Terrain> terrain(new Terrain);
  std::unique_ptr<Motion> motion(new Motion);
  for (uint32_t seed : {1u, 42u, 1234u, 0xffffffffu})
    for (int levels = 1; levels <= 4; ++levels)
      for (int frame = 0; frame < 48; ++frame) {
        const auto &g =
            terrain->build(seed, frame * .37f, frame / 47.f, levels);
        emit(g.count);
        for (int i = 0; i < g.count; ++i) {
          const auto &l = g.loops[i];
          emit(l.id);
          emit(l.level);
          emit(l.perimeter);
          emit(l.age);
          emit(l.area);
          emit(l.center.x);
          emit(l.center.y);
          for (const auto &p : l.p) {
            emit(p.x);
            emit(p.y);
          }
          for (float d : l.distance)
            emit(d);
        }
        motion->adopt(g);
        for (int b = 1; b <= 4; ++b) {
          for (int step = 0; step < 64; ++step)
            motion->step(1.f / 48000, b, {{.1f, .2f, .3f, .4f}}, .25f);
          for (const auto &v : motion->balls) {
            emit(v.position.x);
            emit(v.position.y);
            emit(v.offset.x);
            emit(v.offset.y);
            emit(v.id);
            emit(v.phase);
            emit(v.cooldown);
            emit(v.blend);
            emit(v.state);
            emit(v.active);
            emit(v.hasJoined);
            const float x = 10 * v.position.x - 5, y = 10 * v.position.y - 5;
            emit(x);
            emit(y);
          }
        }
      }
}
