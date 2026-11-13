# Orbitals 2.2.0-dev.4 visualization validation

Date: 2026-11-13. Branch: `dev`. Linux x64; Rack SDK and Rack Free 2.6.6.
No AGENTS.md was present in the project or its ancestor directories.

## Scope

The baseline is the existing, uncommitted dev.3 working tree, including all prior
panel and stereo changes. A byte-for-byte source/resource/test snapshot and
SHA-256 inventory were saved before editing. Comparisons against that snapshot
confirm that **all Contour files and its screenshot, all panel/control SVGs,
Orbitals DSP, module processing/routing/serialization, widget layout and plugin
registration are unchanged**. The only production C++ changes are the two
Orbitals display headers. Shared metadata/documentation and visualization tests
were updated as required. No push, merge, tag or publication was performed.

## Changed files

- Visual code: `src/orbitals/ui/OrbitalDisplay.hpp`, `src/orbitals/ui/OrbitalDisplayModel.hpp`.
- Existing tests/harness: `tests/orbitals/visualization.cpp`, `tests/theme_test.cpp`, `tests/check_migration.py`.
- Shared metadata: `plugin.json`, `README.md`, `CHANGELOG.md`.
- Orbitals documentation: `docs/orbitals/README.md`, `docs/images/orbitals-dark.png`, this validation record.

## Transparent rendering

The opaque rounded-rectangle fill and green stroke were both in
`OrbitalDisplay::draw()`. Removing that override leaves the widget's base layer
empty. Cloud and trajectory cells still draw on layer 1 over the original SVG.
The panel's own dark screen rectangle remains; its coordinate grid is now visible.
No panel SVG, framebuffer clear color or alpha format was changed.

The existing Rack framebuffer clears/recomposites each dirty frame and preserves
the overlay's alpha. `nvgSave` / `nvgRestore` bracket an intersected scissor, so
both the display bounds and any parent clip remain effective. Cloud matrices are
cleared every update; only the original 180 ms path trail persists.

The GUI harness compares the actual rendered widget with its display hidden:
417,764 / 487,204 screen-region pixels in light mode and 417,815 / 487,204 in dark
mode remain byte-identical to the panel alone (about 86%). Zero pixels change
outside the display. Rendering an identical frame again gives identical RGBA
pixels, verifying that alpha/stale frames do not accumulate. Nested panel caches
are allowed to settle after theme/scale switches before capture.

## Density and distribution

The old model used 3,072 spatial samples and a 128 × 128 grid. Samples below
brightness .018 were skipped, projected samples could collide, and the draw
quantizer omitted cell brightness below .1 (`int(value * 5 + .5) == 0`). Merely
doubling samples therefore would not guarantee twice as many visible dots.

The new budget is **24,576 distinct spatial samples**. The stratified radial law
`r = extent * sqrt((i + .5) / N)`, uniform angular sampling, analytical signed
wavefunction evaluation, display exposure/gamma, .018 cutoff, grain multiplier,
cell size, draw quantizer and alpha are unchanged. The deterministic RNG seed is
`0x4f524249` in both versions. No repeated positions are overdrawn as a substitute
for density. Natural nodal surfaces and sparse tails still come from the same
wavefunctions. All work remains in the UI-only display model; audio's snapshot
queue and calculations are untouched. Display updates allocate no memory.

Counts below are **unique cloud screen cells surviving the actual renderer's
quantizer**, counting positive/negative overlap once and excluding the trajectory
from the primary count. The last column additionally excludes cloud cells
covered by the path. Both measures exceed 2× in every case. This counts visible
output, not sample attempts. Parameters: 128 × 128 grid in the same 59 mm screen,
radius as listed, offset 1, shape .35, tilt .2, phase .17, spin .125, frequency
261.625565; pure orbital states except the listed mixture. Initial state and seed
are identical in the paired executables.

| State | Radius | dev.3 visible | dev.4 visible | Ratio | Outside path: old → new |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1s | 0 | 117 | 1104 | 9.44× | 116 → 1077 |
| 1s | 6 | 152 | 1147 | 7.55× | 145 → 1088 |
| 1s | 24 | 123 | 257 | 2.09× | 116 → 247 |
| 2p_z | 0 | 300 | 2562 | 8.54× | 298 → 2501 |
| 2p_z | 6 | 295 | 2572 | 8.72× | 276 → 2462 |
| 2p_z | 24 | 313 | 1296 | 4.14× | 298 → 1241 |
| 3p_x | 0 | 64 | 1059 | 16.55× | 62 → 1035 |
| 3p_x | 6 | 64 | 1059 | 16.55× | 58 → 984 |
| 3p_x | 24 | 92 | 1570 | 17.07× | 87 → 1505 |
| 3d_xy | 0 | 354 | 2626 | 7.42× | 340 → 2554 |
| 3d_xy | 6 | 354 | 2626 | 7.42× | 325 → 2444 |
| 3d_xy | 24 | 362 | 2256 | 6.23× | 346 → 2143 |
| 3d_z² | 0 | 264 | 2530 | 9.58× | 259 → 2473 |
| 3d_z² | 6 | 264 | 2530 | 9.58× | 253 → 2367 |
| 3d_z² | 24 | 269 | 2279 | 8.47× | 259 → 2174 |
| 2p_z / 3d_xy, mix .3 | 0 | 84 | 751 | 8.94× | 82 → 732 |
| 2p_z / 3d_xy, mix .3 | 6 | 84 | 751 | 8.94× | 82 → 713 |
| 2p_z / 3d_xy, mix .3 | 24 | 95 | 729 | 7.67× | 90 → 699 |

The minimum is 123 → 257 (2.09×), for 1s at radius 24. The existing visualization
suite now protects these baseline counts and checks that an unchanged cloud
state rebuilds identically without accumulation. These are finite tested cases,
not a claim that every possible parameter/rotation has the same density ratio.

## Performance

Paired CPU measurements compile the same existing `tests/orbitals/visualization.cpp`
driver against the frozen dev.3 header and the new header with GCC, C++11, `-O2`,
assertions enabled, on the same machine. `--measure` reports density plus the
median of five passes of 120 updates. One and eight models run serially on the UI
side; setup/allocation and the audio engine are outside the timed region.

| Models | Work per frame | dev.3 ms | dev.4 ms | Ratio |
| ---: | --- | ---: | ---: | ---: |
| 1 | cached field, rotating cloud | 0.0514 | 0.3401 | 6.62× |
| 1 | changing coefficients | 0.4531 | 3.5006 | 7.73× |
| 8 | cached field, rotating cloud | 0.3944 | 2.9140 | 7.39× |
| 8 | changing coefficients | 3.6141 | 28.8335 | 7.98× |

Full GUI measurements use the **same** extended `tests/theme_test.cpp` executable
with the dev.3 and dev.4 plugin binaries, at 2× rendering scale, one widget or
4 × 2 widgets. Each pass runs 90 animated frames, discards 30 warmup frames and
reports median UI-step + framebuffer-render + `glFinish` time. DSP preparation
and a 17 ms pacing sleep occur outside the timer. Three alternating old/new
passes were run without concurrent build/tests; the table gives the median of
the three per-pass medians. All eight modules have independent IDs and displays.

| Visible widgets | dev.3 ms/frame | dev.4 ms/frame | Increase |
| ---: | ---: | ---: | ---: |
| 1 | 4.619 | 5.361 | +16.1% |
| 8 | 43.489 | 48.236 | +10.9% |

The larger cloud has a real UI cost, especially when coefficients invalidate
all field values every frame. Eight continuously changing fields take about
28.8 ms for the model calculations alone; the synchronized eight-widget GPU test
is already above a 33.3 ms frame budget in both versions. This is a stress test,
not an audio deadline or a guarantee of 30 fps with many visible modules. Audio
processing code is unchanged. No production-patch CPU profiling was performed.

## Build, regression and GUI results

- SDK build passed without compiler warnings.
- `make test` passed: Contour core/stack/worker checks, mathematical Orbitals
  tests, queue/concurrency/zero-allocation visualization tests, density checks,
  actual combined-binary factories/processing/persistence and routing checks.
- Historical audio regression passed all 150,000 comparisons, maximum error
  4.39648e-14 V; actual-binary mono/stereo/R-only/CV checks passed at 44.1/48/96 kHz.
- `make test-gui` passed with both modules in the combined plugin. Orbitals
  light/dark/light switching, transparent compositing, clipping and stable redraw
  checks passed; one and eight real module widgets were animated and rendered.
- Additional real widget/DSP captures cover 1s, 3p_x, 3d_xy and 3d_z² at radii
  0, 6 and 24, in both themes. The PNG contact sheets were visually inspected:
  cloud structure/empty regions, panel grid and dotted path remain recognizable.
- `make test-package` passed: exact manifest, both modules and all assets in one
  installable package. A fresh isolated headless Rack 2.6.6 process extracted the
  actual package and loaded dev.4 plus both modules from the saved combined patch
  without warnings/errors. The extracted binary matched the packaged binary
  byte for byte. Relative documentation links and `git diff --check` passed.

The new [dark screenshot](images/orbitals-dark.png) is a lossless conversion of
the actual dev.4 harness framebuffer, 1080 × 1520 at the same 4× scale and crop as
dev.3. It was opened and checked at full resolution. The Contour image was not
rewritten. Temporary captures, profiles and measurements stay under ignored
`build/tests/` or `/tmp`, outside the source deliverables.

## Reproduction and limits

`make test-orbitals-visualization` runs regressions. The generated executable
accepts `--density` or `--measure`. To compare a prior implementation, compile
the same driver with `DISPLAY_MODEL_HEADER` naming that frozen header. The GUI
harness accepts an optional `Orbitals` model filter; a fifth `--benchmark`
argument skips screenshots and runs the one/eight-widget benchmark only.

The local artifact is `dist/FilthAssembly-2.2.0-dev.4-lin-x64.vcvplugin`.
Other platforms, subjective audio listening and exhaustive manual mouse-drag
checks were not performed. The visual validation uses actual Rack OpenGL module
widgets and DSP, not SVG-only previews or generated imagery.
