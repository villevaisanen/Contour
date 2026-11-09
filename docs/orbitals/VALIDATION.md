> Historical standalone validation imported unchanged from the 2.1.2 worktree.
> These are not claims about the combined build. Current results: [migration record](../migrations/001-orbitals.md).

# Orbitals 2.1.2 validation

- SDK 2.6.6 / Linux x64: `make`, `make test`, `make test-rack`,
  `make test-visualization`, `make test-audio-regression`, and `make dist` pass.
- Plugin/module identifiers: `FilthAssembly/Orbitals`. Ten parameters, six CV
  inputs, four outputs. FM and Sync are removed from the Rack interface; the
  engine sources remain unchanged.
- Analytical suite: 273,466 checks. Audio regression: 150,000 AUDIO/XYZ
  comparisons against v2.1.0, maximum error 4.40e-14 V (tolerance 1e-10 V).
- Visualization: 128 x 128 matrix, allocation-free updates, concurrent snapshot
  transport, and complete trajectory framing at radii 0/6/24 and offsets
  -24/0/24. Updates remain separate from audio synthesis.
- Rack native module screenshot mode successfully renders the widget, but always
  forces light panels. A separate OpenGL harness loaded the built plugin and
  rendered the same widget through light/dark/light, verifying its selected SVG
  after each change. Both rendered themes were visually inspected: controls,
  label paths, custom jacks, screen placement and matrix. Images are
  `Orbitals.png` (dark) and `Orbitals-light.png` (light).
- Packaged SVGs have path outlines for text. Editable supplied source artwork is
  preserved in `panel-sources/`. The light panel's output group was moved up
  0.4715 mm in the runtime asset to match the dark panel's common jack centers.
- Tests used isolated Rack profiles; the installed Contour plugin was untouched.
  Both standalone projects now use the technical slug `FilthAssembly`, so their
  packages cannot coexist in one Rack plugin directory without combining modules
  into one plugin or choosing distinct technical slugs.

The following are historical validation notes for earlier Orbitals versions;
old identifiers, controls and screenshots describe those versions only.

# Validation — 2.1.0

Local environment: Linux x86-64, GCC, Rack SDK and Rack Free 2.6.6, 2026-10-31. These results describe this machine, not guarantees for every host.

## Passed

- `make -j4`: plugin.so builds with the SDK's C++11 and optimization flags.
- `make test`: **273,466 checks**. Independent Cartesian reference values for every state; origin/large coordinates; parity and angular/radial nodes; MIX endpoints and identical-state normalization; full 14×14 volume Gram matrix; trajectory periodicity; 1 V/oct and exponential FM; 44.1/48/96 kHz phase accumulation; sample-rate continuity; sync edge/hold/rearm; output limits and scaling; extreme and nonfinite controls/CV; all 14 states produce changing audio on the default path; no heap allocations over 10,000 samples with changing selectors.
- Integration uses midpoint r in [0,80] with step 0.1, 96 cos(theta) and 32 azimuth samples. Maximum Gram-matrix error **0.000542473**, accepted below 0.001. This tests physical spatial normalization independently of audio gain.
- AddressSanitizer and UndefinedBehaviorSanitizer run of the same suite: passed without diagnostics. Example command: `g++ -std=c++11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer test/test.cpp -o build/test/sanitized`.
- `make test-rack`: loads the actual compiled plugin.so with dlopen, resolves init, registers ORBITAL, creates the module via its model factory, verifies 10/8/4 parameters/inputs/outputs and finite Rack output processing at 44.1/48/96 kHz.
- Rack Free itself loads plugin **FilthAssemblyOrbital 2.1.0**. Its native `-t 2` screenshot mode successfully creates the widget and renders all labels/stock controls. `docs/ORBITAL.png` is the resulting image. A separate isolated Rack user folder is used, preserving the user's patch. A patch containing ORBITAL also ran headless for three seconds and exited cleanly on newline.

## Performance and spectrum

Standalone optimized (-O2) engine benchmark, fixed two-state mixture, 48 kHz: approximately **0.039 s** wall time per second of audio for one engine, **0.64 s** for 16 engines processed serially. Includes 4× analytical evaluation and decimation. This is an indicative local benchmark, not the Rack CPU-meter reading or a worst-case bound. During rapid selector changes several fading states can be evaluated simultaneously and CPU cost increases.

`make spectrum` renders a coherent 4211 Hz path at 48 kHz, with a 768 kHz reference and a direct 48 kHz analytical baseline, discarding one second of settling. NumPy sums energy outside the legal harmonic bins (±2 bins around harmonics below Nyquist):

| Render | Non-harmonic energy / total |
|---|---|
| Direct, no oversampling | −7.77 dB |
| Implemented 4× FIR engine | −66.44 dB |
| 768 kHz reference | −151.83 dB |

This diagnostic validates a substantial improvement on one representative path/mixture. It is not a comprehensive aliasing measurement over the entire parameter space. The reference also uses the engine's oversampling. The baseline is an explicit analytical direct renderer, not an optional plugin mode.

## Known limits

- Fixed 4× oversampling reduces aliasing but does not eliminate it. Steep fields, high pitch, rapid geometry CV and the final hard safety clamp can introduce high harmonics. The FIR intentionally attenuates the upper audio band; pitch is capped at min(20 kHz, 0.45 × sample rate).
- One common fixed gain preserves the requested superposition but cannot equalize all states and paths. There is no adaptive gain; quiet/null paths stay quiet. Extreme settings can hit the ±5 V safety clamp.
- Knob/state transitions are smoothed. Geometry CV, pitch CV and hard sync may create transients; subjective listening quality and all transition combinations are not exhaustively validated.
- Mono, input channel 1 only; no polyphony, phase evolution by atomic energy, through-zero FM, 3D visualization or VST build.
- Only Linux x86-64 tested. No physical audio-interface listening or musical performance test is claimed. Automated renders verify generated audio, not the user's monitoring setup.

# Visualization update — 2.1.1

The analytical orbitals, trajectory equations, audio filters and output mapping are retained. The engine now exposes a value snapshot of effective path/coefficients and oscillator/spin state. Rack publishes it at 30 Hz into an eight-slot SPSC ring (seven usable slots); a full queue drops data instead of blocking audio. Release/acquire ownership prevents concurrent access to a non-atomic slot. The UI consumes the latest available snapshot, never reading mutable engine data directly.

Validation on the same Linux x86-64 / Rack Free 2.6.6 environment:

- Existing **273,466** mathematics/DSP checks pass.
- `make test-audio-regression`: **150,000 AUDIO/XYZ sample comparisons** against the engine extracted from tag `v2.1.0`, compiled with production optimization flags. Includes state changes, CV/FM, sync and sample-rate changes. Maximum difference **4.40 × 10⁻¹⁴ V**, below the 10⁻¹⁰ V tolerance (floating-point optimizer rounding).
- `make test-visualization`: queue empty/full/wrap cases; concurrent producer/consumer with 200,000 publication attempts checks monotonic untorn payloads; effective geometry/MIX CV; all nine requested visual control reactions; finite bounded cells; zero heap allocations during 100 field rebuilds/updates.
- The visualization suite passes **AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer**, without diagnostics. Example: `g++ -std=c++11 -O1 -g -pthread -fsanitize=thread test/visualization.cpp -o build/test/visualization-tsan`.
- Actual plugin.so factory/process test passes and receives **89 snapshots over three seconds** of 44.1/48/96 kHz processing, consistent with the sample-quantized 30 Hz publisher.
- Rack's native screenshot mode loads **FilthAssemblyOrbital 2.1.1** and renders the widget; `docs/ORBITAL.png` is the updated native screenshot. A separate live Rack patch with a 3d_xy/3d_z² mixture and nonzero spin was opened, the display inspected, and captures 300 ms apart differed, confirming animation through the real audio-to-UI path.
- Cached field projection/matrix calculation measured about **0.045 ms/update** in the standalone -O2 benchmark (CPU calculation only, excluding NanoVG/GPU drawing). The cloud uses 3,072 points; brightness caches rebuild on coefficient changes, not on each spin frame. Matrix updates are capped at 30 Hz; drawing uses 15 quantized brightness batches with fixed-size arrays. Rack/NanoVG owns its own rendering buffers; zero-allocation testing covers our CPU display model, not the graphics driver's internals.

Known visual limitations: fixed camera; qualitative density exposure; finite spatial extent; adaptive framing can zoom out on large paths; faint states can be visually overshadowed by stronger states. The actual audio-rate head is strobed at UI rate, so apparent head motion can alias, reverse or stand still. Persistence is a UI effect, not physical state evolution. The browser preview is deterministic. No audio waveforms, spatial tables or graphics results feed back into synthesis.
