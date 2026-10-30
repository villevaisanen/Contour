// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen
#include "terrain_checks.hpp"
#include <iostream>
#if defined(__unix__) || defined(__APPLE__)
#include <cstdlib>
#include <cstring>
#include <pthread.h>

static void checkPthread(int result, const char *operation) {
  if (result != 0) {
    std::cerr << "FAIL: " << operation << ": " << std::strerror(result) << '\n';
    std::exit(EXIT_FAILURE);
  }
}
static void *run(void *) {
  // Exactly the production worker's ownership and call pattern. Terrain is a
  // small owner; its constructor allocates all large storage directly on heap.
  contour::Terrain terrain;
  int builds = 0;
  for (uint32_t seed : {1u, 42u, 1234u, 0xffffffffu})
    for (int levels = 1; levels <= 4; ++levels)
      for (int frame = 0; frame < 12; ++frame) {
        const auto &g = terrain.build(seed, frame * .37f, frame / 11.f, levels);
        checkTerrain(g, levels);
        ++builds;
      }
  assert(builds == 192);
  return nullptr;
}
int main() {
  pthread_attr_t attr;
  checkPthread(pthread_attr_init(&attr), "pthread_attr_init");
  const size_t stackBytes = 512 * 1024;
  checkPthread(pthread_attr_setstacksize(&attr, stackBytes),
               "pthread_attr_setstacksize(512 KiB)");
  size_t configured = 0;
  checkPthread(pthread_attr_getstacksize(&attr, &configured),
               "pthread_attr_getstacksize");
  assert(configured == stackBytes);
  pthread_t thread;
  checkPthread(pthread_create(&thread, &attr, run, nullptr), "pthread_create");
  checkPthread(pthread_attr_destroy(&attr), "pthread_attr_destroy");
  checkPthread(pthread_join(thread, nullptr), "pthread_join");
  std::cout << "PASS: 512 KiB pthread stack, production Terrain ownership, "
               "192 terrains, four seeds, all 1-4 contour levels\n";
}
#else
int main() {
  std::cout << "SKIP: 512 KiB stack regression requires POSIX pthread stack "
               "attributes; "
               "this platform is not supported\n";
}
#endif
