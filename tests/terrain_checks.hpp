// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen
#pragma once
#include "../src/core.hpp"
#include <cassert>

inline void checkTerrain(const contour::Geometry &g, int levels) {
  using namespace contour;
  assert(g.count > 0 && g.count <= MaxLoops);
  for (int i = 0; i < g.count; ++i) {
    const auto &l = g.loops[i];
    assert(l.id > 0 && l.level >= 0 && l.level < levels);
    assert(std::isfinite(l.age) && l.age >= 0);
    assert(std::isfinite(l.area) && l.area > 0);
    assert(std::isfinite(l.perimeter) && l.perimeter > .08f);
    assert(std::isfinite(l.center.x) && std::isfinite(l.center.y));
    assert(l.center.x > 0 && l.center.x < 1);
    assert(l.center.y > 0 && l.center.y < 1);
    assert(l.distance[0] == 0 && l.distance[Points] == l.perimeter);
    for (int j = 0; j < Points; ++j) {
      const auto &p = l.p[j];
      assert(std::isfinite(p.x) && std::isfinite(p.y));
      assert(p.x > 0 && p.x < 1 && p.y > 0 && p.y < 1);
      assert(std::isfinite(l.distance[j + 1]));
      assert(l.distance[j + 1] >= l.distance[j]);
    }
  }
}
