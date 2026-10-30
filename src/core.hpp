// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen

#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>
namespace contour {
constexpr int Grid = 96, MaxLoops = 48, Points = 192;
struct V {
  float x, y;
  V(float x = .5f, float y = .5f) : x(x), y(y) {}
  V operator+(V b) const { return {x + b.x, y + b.y}; }
  V operator-(V b) const { return {x - b.x, y - b.y}; }
  V operator*(float s) const { return {x * s, y * s}; }
};
inline float dot(V a, V b) { return a.x * b.x + a.y * b.y; }
inline float length(V a) { return std::sqrt(dot(a, a)); }
inline float clamp(float v, float lo = 0, float hi = 1) {
  return std::max(lo, std::min(hi, v));
}
inline V limitStep(V p, V t, float step) {
  V d = t - p;
  float n = length(d);
  return n > 0 ? p + d * std::min(1.f, step / n) : p;
}
inline float cv(float knob, float volts) {
  return clamp(knob + (std::isfinite(volts) ? volts : 0) / 5);
}
inline float rate(float n, float low, float high) {
  return n <= .001f ? 0 : low * std::pow(high / low, (n - .001f) / .999f);
}
struct Loop {
  int id = 0, level = 0;
  float perimeter = 0, age = 0, area = 0;
  V center;
  std::array<V, Points> p;
  std::array<float, Points + 1> distance{};
  void measure() {
    distance[0] = 0;
    for (int i = 0; i < Points; i++)
      distance[i + 1] = distance[i] + length(p[(i + 1) % Points] - p[i]);
    perimeter = distance[Points];
  }
  V at(float phase) const {
    if (perimeter <= 1e-8f)
      return center;
    float d = (phase - std::floor(phase)) * perimeter;
    auto it = std::upper_bound(distance.begin(), distance.end(), d);
    int i = std::max(0, std::min(Points - 1, int(it - distance.begin()) - 1));
    float t =
        (d - distance[i]) / std::max(distance[i + 1] - distance[i], 1e-12f);
    return p[i] * (1 - t) + p[(i + 1) % Points] * t;
  }
  float nearest(V q) const {
    float best = 1e9, phase = 0;
    for (int i = 0; i < Points; i++) {
      V a = p[i], d = p[(i + 1) % Points] - a;
      float t = clamp(dot(q - a, d) / std::max(dot(d, d), 1e-12f));
      float n = length(a + d * t - q);
      if (n < best) {
        best = n;
        phase = (distance[i] + t * (distance[i + 1] - distance[i])) /
                std::max(perimeter, 1e-12f);
      }
    }
    return phase;
  }
};
struct Geometry {
  int count = 0;
  std::array<Loop, MaxLoops> loops;
};
struct Noise {
  uint32_t seed;
  explicit Noise(uint32_t s) : seed(s) {}
  uint32_t hash(int x, int y, int z) const {
    uint32_t h = seed ^ uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u ^
                 uint32_t(z) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
  }
  float grad(int x, int y, int z, float a, float b, float c) const {
    unsigned h = hash(x, y, z) & 15;
    float u = h < 8 ? a : b, v = h < 4 ? b : (h == 12 || h == 14 ? a : c);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
  }
  float operator()(float x, float y, float z) const {
    int ix = int(std::floor(x)), iy = int(std::floor(y)),
        iz = int(std::floor(z));
    x -= ix;
    y -= iy;
    z -= iz;
    auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
    float u = fade(x), v = fade(y), w = fade(z), sum = 0;
    for (int k = 0; k < 2; k++)
      for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++)
          sum += grad(ix + i, iy + j, iz + k, x - i, y - j, z - k) *
                 (i ? u : 1 - u) * (j ? v : 1 - v) * (k ? w : 1 - w);
    return sum;
  }
};
// Extraction runs only on the worker. Grid-edge IDs make stitching exact.
class Terrain {
  int nextId = 1;
  Geometry previous;

public:
  Geometry build(uint32_t seed, float time, float fract, int levels) {
    levels = std::max(0, std::min(4, levels));
    fract = clamp(fract);
    std::array<float, Grid * Grid> f;
    Noise noise(seed);
    for (int y = 0; y < Grid; y++)
      for (int x = 0; x < Grid; x++) {
        float u = float(x) / (Grid - 1), v = float(y) / (Grid - 1), n = 0,
              amp = 1, norm = 0;
        for (int o = 0; o < 4; o++) {
          float freq = 2.6f * (1 << o);
          n += amp * noise(u * freq + 13, v * freq + 7, time);
          norm += amp;
          amp *= .08f + .62f * fract;
        }
        float window = std::sin(float(M_PI) * u) * std::sin(float(M_PI) * v);
        f[y * Grid + x] = window * (.58f + .55f * n / norm);
      }
    Geometry out;
    constexpr float heights[] = {.20f, .32f, .44f, .56f};
    constexpr int Edges = 2 * Grid * Grid;
    for (int lev = 0; lev < levels; lev++) {
      std::array<std::array<int, 2>, Edges> links;
      for (auto &a : links)
        a = {{-1, -1}};
      std::array<V, Edges> pos;
      std::array<bool, Edges> seen{};
      auto connect = [&](int a, int b) {
        auto put = [&](int p, int q) {
          if (links[p][0] < 0)
            links[p][0] = q;
          else
            links[p][1] = q;
        };
        put(a, b);
        put(b, a);
      };
      for (int y = 0; y < Grid - 1; y++)
        for (int x = 0; x < Grid - 1; x++) {
          float a[4] = {f[y * Grid + x], f[y * Grid + x + 1],
                        f[(y + 1) * Grid + x + 1], f[(y + 1) * Grid + x]};
          V corners[4] = {
              {float(x) / (Grid - 1), float(y) / (Grid - 1)},
              {float(x + 1) / (Grid - 1), float(y) / (Grid - 1)},
              {float(x + 1) / (Grid - 1), float(y + 1) / (Grid - 1)},
              {float(x) / (Grid - 1), float(y + 1) / (Grid - 1)}};
          int ids[4] = {y * Grid + x, Grid * Grid + y * Grid + x + 1,
                        (y + 1) * Grid + x, Grid * Grid + y * Grid + x};
          int hit[4], n = 0;
          for (int e = 0; e < 4; e++) {
            int b = (e + 1) % 4;
            if ((a[e] > heights[lev]) != (a[b] > heights[lev])) {
              float t = (heights[lev] - a[e]) / (a[b] - a[e]);
              pos[ids[e]] = corners[e] + (corners[b] - corners[e]) * t;
              hit[n++] = e;
            }
          }
          if (n == 2)
            connect(ids[hit[0]], ids[hit[1]]);
          if (n == 4) { // asymptotic decider, fixed tie rule
            float q = (a[0] - heights[lev]) * (a[2] - heights[lev]) -
                      (a[1] - heights[lev]) * (a[3] - heights[lev]);
            if (q >= 0) {
              connect(ids[0], ids[1]);
              connect(ids[2], ids[3]);
            } else {
              connect(ids[0], ids[3]);
              connect(ids[1], ids[2]);
            }
          }
        }
      for (int start = 0; start < Edges; start++) {
        if (seen[start] || links[start][0] < 0)
          continue;
        std::vector<V> path;
        int cur = start, prev = -1;
        bool closed = false;
        for (int k = 0; k < Edges; k++) {
          if (cur < 0)
            break;
          if (seen[cur]) {
            closed = cur == start;
            break;
          }
          seen[cur] = true;
          path.push_back(pos[cur]);
          int next = links[cur][0] == prev ? links[cur][1] : links[cur][0];
          prev = cur;
          cur = next;
        }
        if (!closed || path.size() < 8)
          continue;
        std::vector<float> dist(path.size() + 1, 0);
        float area = 0;
        V center{0, 0};
        for (size_t i = 0; i < path.size(); i++) {
          dist[i + 1] = dist[i] + length(path[(i + 1) % path.size()] - path[i]);
          area += path[i].x * path[(i + 1) % path.size()].y -
                  path[(i + 1) % path.size()].x * path[i].y;
          center = center + path[i];
        }
        if (dist.back() < .09f || std::abs(area) < .0004f ||
            out.count == MaxLoops)
          continue;
        Loop l;
        l.level = lev;
        l.perimeter = dist.back();
        l.area = std::abs(area) * .5f;
        l.center = center * (1.f / path.size());
        size_t j = 0;
        for (int i = 0; i < Points; i++) {
          float d = l.perimeter * i / Points;
          while (j + 1 < path.size() && dist[j + 1] < d)
            j++;
          l.p[i] = path[j] +
                   (path[(j + 1) % path.size()] - path[j]) *
                       ((d - dist[j]) / std::max(dist[j + 1] - dist[j], 1e-9f));
        }
        if (area < 0)
          std::reverse(l.p.begin(), l.p.end());
        l.measure();
        out.loops[out.count++] = l;
      }
    }
    std::array<bool, MaxLoops> used{};
    for (int i = 0; i < out.count; i++) {
      Loop &l = out.loops[i];
      float best = .16f;
      int match = -1;
      for (int j = 0; j < previous.count; j++) {
        const Loop &p = previous.loops[j];
        float score = length(l.center - p.center) +
                      .3f * std::abs(std::sqrt(l.area) - std::sqrt(p.area));
        if (!used[j] && p.level == l.level && score < best) {
          best = score;
          match = j;
        }
      }
      if (match >= 0) {
        used[match] = true;
        l.id = previous.loops[match].id;
        l.age = previous.loops[match].age + 1.f / 30;
      } else
        l.id = nextId++;
    }
    previous = out;
    return out;
  }
};
enum State { Waiting, Following, Transition };
struct Ball {
  V position, offset{0, 0};
  int id = 0;
  float phase = 0, cooldown = 0, blend = 0;
  State state = Waiting;
  bool active = false, hasJoined = false;
};
class Motion {
public:
  Geometry geometry;
  std::array<Ball, 4> balls;
  int active = 2;
  float decisionClock = 0;
  int index(int id) const {
    for (int i = 0; i < geometry.count; i++)
      if (geometry.loops[i].id == id)
        return i;
    return -1;
  }
  void adopt(const Geometry &g) {
    std::array<V, 4> targets;
    for (int b = 0; b < 4; b++) {
      int i = index(balls[b].id);
      targets[b] =
          i >= 0 ? geometry.loops[i].at(balls[b].phase) : balls[b].position;
    }
    geometry.count = 0;
    for (int i = 0; i < std::min(g.count, MaxLoops); i++) {
      const Loop &l = g.loops[i];
      bool valid = l.perimeter > .00001f && std::isfinite(l.perimeter);
      for (const V &p : l.p)
        valid = valid && std::isfinite(p.x) && std::isfinite(p.y);
      if (valid)
        geometry.loops[geometry.count++] = l;
    }
    for (int b = 0; b < 4; b++) {
      Ball &v = balls[b];
      int i = index(v.id);
      if (i < 0) {
        v.id = 0;
        v.state = Waiting;
        continue;
      }
      auto &l = geometry.loops[i];
      v.phase = l.nearest(targets[b]);
      V target = l.at(v.phase);
      if (v.state == Following) {
        if (length(target - v.position) > .075f) {
          v.state = Transition;
          v.offset = {0, 0};
        } else {
          v.offset = v.position - target;
          v.blend = 1.f / 30;
        }
      }
    }
  }
  void step(float dt, int count, const std::array<float, 4> &speeds,
            float slew) {
    active = count;
    decisionClock -= dt;
    bool decide = decisionClock <= 0;
    if (decide)
      decisionClock = .01f;
    std::array<int, MaxLoops> occupancy{};
    for (int b = 0; b < 4; b++) {
      Ball &v = balls[b];
      if (b >= count) {
        v.active = false;
        v.id = 0;
        v.state = Waiting;
        continue;
      }
      if (!v.active) {
        v = Ball{};
        v.active = true;
      }
      int i = index(v.id);
      if (i >= 0)
        occupancy[i]++;
      else {
        v.id = 0;
        v.state = Waiting;
      }
    }
    for (int b = 0; b < count; b++) {
      Ball &v = balls[b];
      v.cooldown = std::max(0.f, v.cooldown - dt);
      int old = index(v.id);
      bool required = old < 0;
      bool voluntary = old >= 0 && v.state == Following && occupancy[old] > 1 &&
                       v.cooldown == 0;
      int choice = -1;
      float best = 1e9;
      if (decide && (required || voluntary)) {
        for (int i = 0; i < geometry.count; i++) {
          auto &l = geometry.loops[i];
          if (voluntary && (occupancy[i] || l.age < .2f))
            continue;
          float d = length(l.at(l.nearest(v.position)) - v.position) +
                    (occupancy[i] ? 10.f : 0.f);
          if (d < best) {
            best = d;
            choice = i;
          }
        }
        if (choice >= 0 && choice != old) {
          if (old >= 0)
            occupancy[old]--;
          occupancy[choice]++;
          v.id = geometry.loops[choice].id;
          v.phase = geometry.loops[choice].nearest(v.position);
          if (required && !v.hasJoined && occupancy[choice] > 1)
            v.phase =
                std::fmod(v.phase + float(occupancy[choice] - 1) / count, 1.f);
          v.hasJoined = true;
          v.state = Transition;
          v.cooldown = .5f;
          v.offset = {0, 0};
        }
      }
      int i = index(v.id);
      if (i < 0)
        continue;
      const Loop &l = geometry.loops[i];
      if (v.state == Transition) {
        V target = l.at(v.phase);
        bool reached = length(target - v.position) <= slew * dt;
        v.position = limitStep(v.position, target, slew * dt);
        if (reached) {
          v.position =
              target; // The remaining distance was within this sample's budget.
          v.state = Following;
        }
      } else {
        v.phase += speeds[b] * dt / l.perimeter;
        v.phase -= std::floor(v.phase);
        float ratio = v.blend > 0 ? std::max(0.f, 1 - dt / v.blend) : 0;
        v.offset = v.offset * ratio;
        v.blend = std::max(0.f, v.blend - dt);
        v.position = l.at(v.phase) + v.offset;
      }
      v.position.x = clamp(v.position.x);
      v.position.y = clamp(v.position.y);
    }
  }
};
} // namespace contour
