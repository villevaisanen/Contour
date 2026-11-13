# Filth Assembly 2.2.0-dev.3 validation

Date: 2026-11-13. Branch: `dev`. Platform: Linux x64. Rack SDK and full Rack Free runtime: 2.6.6.

## Scope and preservation

The starting worktree already contained dev.2 stereo routing, tests and panel
updates. Those changes are retained. Compared with the starting source snapshot,
only Contour's two panel paths and Orbitals's LEVEL/L/R widget positions changed
in C++; DSP, parameter/input/output/light IDs and serialization are untouched.
Contour's light/dark panel bytes are unchanged after relocation to `res/contour/`.
Orbitals's dark artwork is unchanged except whitespace; its light artwork has
small output-group and LEVEL-group translations to match the dark layout.
The old root light/dark files are absent, so they cannot mask broken paths.
All panel hrefs are internal SVG fragment references; no external images broke
when moving the files. Historical provenance records retain their original paths.

## Automated checks

- Clean SDK build (`make clean`, `make -j4`): passed, no compiler warnings.
- `make test`: passed. Contour core, 512 KiB stack, worker/ports/CV/seed/concurrency;
  Orbitals math, visualization transport, actual-binary parameter/routing tests;
  combined plugin factories, simultaneous processing at 44.1/48/96 kHz,
  parameter round trips and nonzero audio; provenance/resource/symbol checks.
- Historical Orbitals regression: 150,000 comparisons, maximum error
  4.39648e-14 V. Existing mono/stereo balance, R-only, CV clamp and live cable
  changes passed with the actual binary at all three sample rates.
- `make test-equivalence`: passed, bit-exact Contour comparison against 2.1.2
  across 768 terrains and 5,364,128 output bytes.
- `make test-gui`: passed. Actual plugin widgets rendered through Rack's OpenGL
  API with real module processing; light/dark/light switches work in place for
  both modules. No missing runtime resources. Controls, ports, displays and full
  panel bounds were visually checked. Neither module defines lights.
- Panel regression checks compare L/R and LEVEL widget coordinates to both SVGs.
- `make test-package`: passed; exact manifest, both module slugs, shared binary,
  every runtime asset and licensing files are present in the archive. A fresh
  isolated Rack profile also extracted the final `.vcvplugin`, loaded dev.3
  without warnings and opened the combined patch; its installed binary matched
  the packaged binary byte for byte.
- Markdown relative links and PNG dimensions checked; `git diff --check` clean.

## Full Rack application checks

An isolated profile under `build/tests/runtime-profile` was used; the user's
installed plugin and patches were not replaced. The loaded plugin binary was
byte-identical to the newly built `plugin.so`, and the log reported
`Loaded plugin FilthAssembly 2.2.0-dev.3`.

Both Contour and Orbitals appeared in the full application module browser. A
separate `build/tests/runtime-test.vcv` contained both modules and a Contour X
output connected to Orbitals RADIUS CV. Saving with Rack and opening the saved
compressed patch in a second Rack process preserved all parameters, module
identities, the Contour seed and cable endpoints (compared against the autosave
JSON). Both modules processed in the same patch. The historical `tests/Scope.vcv`
with Contour 2.1.4 identifiers and Fundamental Scope also loaded, preserving both
modules and its two cables. No related loading or missing-resource warnings
appeared in either application log. The example itself was not saved over.

## Screenshots

- `docs/images/contour-dark.png`: 1200 × 1520.
- `docs/images/orbitals-dark.png`: 1080 × 1520.

Both show complete module widgets at 4× Rack scale, with actual knobs and ports.
`tests/theme_test.cpp` runs DSP and UI frames together, drains display snapshots
through rendering and captures Rack's framebuffer. Dark `*-theme-1.ppm` outputs
were converted losslessly to PNG using Pillow, without compositing, retouching
or generated imagery. Final PNGs were opened and visually inspected before the
README links were finalized. Contour shows moving travellers on contour paths;
Orbitals shows its actual orbital cloud and trajectory display.

Rack's built-in `-t/--screenshot` option is documented in the
[Rack manual](https://vcvrack.com/manual/Installing#Command-line-usage).
The existing live-module GUI harness was used to include populated displays
rather than relying on browser-only module screenshots.

## Limits and release follow-up

Automated DSP/control and GUI checks are not subjective listening or exhaustive
mouse-drag testing of every control. Audio-interface playback, production-patch
CPU profiling and other operating systems were not tested. Before a stable
release, perform those checks and review the pre-existing dev.2 output-port
compatibility limitations documented in README and CHANGELOG. The dev.3 change
does not introduce another port/patch migration.

Notify the existing VCV Library issue that the source repository is now
`https://github.com/villevaisanen/Filth-Assembly-Modules`. No issue comment,
Library update, GitHub Release, tag, push or mainline merge was made by this task.
