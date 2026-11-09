# Filth Assembly — VCV Rack modules

Development version **2.2.0-dev.1**, canonical plugin slug `FilthAssembly`,
brand **Filth Assembly**. This repository now builds **one plugin containing
Contour and Orbitals**. The migration is developed on `dev`; it is not a stable
release or a VCV Library submission.

| Module slug | Module | Manual |
| --- | --- | --- |
| `Contour` | 20 HP animated contour paths with four XY modulation sources | [Contour manual](docs/Contour.md) |
| `Orbitals` | 18 HP direct analytical hydrogen-orbital oscillator with live matrix visualization and XYZ outputs | [Orbitals manual](docs/orbitals/README.md) |

Both modules retain their existing light/dark panels, controls, IDs, DSP and
state behavior. Orbitals uses signed wavefunctions, not wavetables; quiet or
silent paths are intentional. Module behavior is unchanged by packaging.

## Build and test

Requires a C++11 compiler, GNU Make, Rack 2 SDK, jq, tar and zstd. The tested SDK
and runtime are Rack 2.6.6 on Linux x64. Override the default `~/Rack-SDK` as needed.
Python 3 is required for migration/audio checks; NumPy only for optional spectrum.

```sh
make -j4 RACK_DIR="$HOME/Rack-SDK"
make test RACK_DIR="$HOME/Rack-SDK"
make test-equivalence       # optional bit-exact Contour regression vs 2.1.2
make spectrum               # optional Orbitals spectral diagnostic (NumPy)
make dist                   # one FilthAssembly package with both modules
make test-package           # inspect packaged manifest/assets/license
make test-gui               # optional: installed Rack, display and OpenGL required
```

`make test` runs Contour core/small-stack and Rack worker tests, Orbitals
mathematical and concurrent visualization suites, actual combined-binary factory
and persistence checks, the historical audio regression, and preservation/asset
checks. `make test-core`, `make test-orbitals`, `make test-orbitals-visualization`,
`make test-audio-regression` and `make test-migration` can run without Rack SDK.
`make test-rack` needs the SDK and builds the combined plugin automatically.

All source, resources and historical audio-test fixtures are in this repository;
the standalone Orbitals repository is not a build or test dependency. Build outputs
are under `build/` and `dist/`. Assertions remain enabled in tests.

## Install a development build

Back up patches and close Rack before changing its plugin installation. Copy
`dist/FilthAssembly-2.2.0-dev.1-lin-x64.vcvplugin` into Rack's user-folder
`plugins-lin-x64` directory, or use `make install`. Restart Rack and search for
**Filth Assembly**: both **Contour** and **Orbitals** should appear.

Use **Help → Open user folder** in Rack to locate the folder. A combined package
replaces the standalone `FilthAssembly` package; two packages with the same plugin
slug cannot coexist. For development, prefer an isolated Rack user folder rather
than replacing your working installation. This task does not install globally.

## Development workflow and compatibility

Work on `dev`, keep commits reviewable, build and run tests before pushing.
Do not merge to the mainline (`master` in this repository), tag a stable release,
or submit to the VCV Library during this phase. Future modules are separate tasks.

Existing `FilthAssembly/Contour` patches retain their parameter, input, output,
seed and model identity. Current standalone `FilthAssembly/Orbitals` patches retain
the same ten parameter, six input and four output IDs. Loading restarts its DSP
state as before; only Rack parameters are serialized.

Older `FilthAssemblyOrbital/ORBITAL` patches **are not automatically compatible**.
Back up and migrate their module identifiers to `FilthAssembly/Orbitals` and
remap input cables: FM/Sync were removed in the source's 2.1.2 worktree, and
remaining input IDs changed there before this migration. Earlier `Contour/Contour`
patches likewise need the plugin-slug correction documented in the Contour manual.

The [migration record](docs/migrations/001-orbitals.md) documents the source
commit, uncommitted source snapshot, files, licensing, changes and validation.
See [changelog](CHANGELOG.md), [Orbitals mathematics](docs/orbitals/MATHEMATICS.md)
and [historical Orbitals validation](docs/orbitals/VALIDATION.md).

## Validation limits

Only Linux x64 is validated. Native builds are required for other platforms.
Compilation and generated audio tests are not subjective listening tests. The
migration record lists completed runtime checks and the remaining manual checklist.
Existing panel graphics are preserved. No Fugitive or Superposition is included.

## License

Copyright © 2026 Ville Väisänen / Filth Assembly.

**GPL-3.0-only** for the combined source, documentation and artwork. The owner
explicitly authorized relicensing the imported Orbitals snapshot; historical
source licensing and the grant are recorded in [LICENSES.md](LICENSES.md).
The full license is [licence.txt](licence.txt).
