# 001 — Orbitals migration: preflight findings

- Canonical destination: `https://github.com/villevaisanen/Filth-Assembly-Modules`, branch `dev` (repository renamed after the migration).
- Destination baseline: `ee059f160f27d8f9e4aff38980930f1da35da24e` (`master`, clean). No local/remote dev existed. Mainline remains untouched.
- Source: `https://github.com/villevaisanen/orbitals`, branch `main`, HEAD `67fca58ef97b6d91c24d8ed68a743c13f2ca17c4`.
- Source is dirty: current 2.1.2 worktree includes selector/port/visualization/theme updates absent from HEAD. Import this current implementation, not the older HEAD behavior. Exact source hashes, status and file mapping are in [source manifest](001-orbitals-source.json); tracked differences are preserved in [worktree patch](001-orbitals-worktree.patch). No source commits or edits are made.
- Current source identity: `FilthAssembly/Orbitals`; model C++ symbol `modelOrbital`. HEAD historical identity: `FilthAssemblyOrbital/ORBITAL`.
- Destination identity stays `FilthAssembly/Contour`; add `FilthAssembly/Orbitals` to the same instance.
- Shared symbol collision: both sources define `pluginInstance` and `init`; keep only destination definitions. Add only `modelOrbital` declaration/registration.
- Destination compiles `src/*.cpp`; source also compiles `src/ui/*.cpp`. Imported module goes under `src/orbitals/`, UI under `src/orbitals/ui/`, DSP unchanged under `src/orbitals/dsp/`.
- Both themes already exist. Orbitals uses five runtime SVGs (two panels, three controls). Control assets currently hash-identical to Contour; namespace Orbitals resources anyway to prevent future accidental collisions. No bundled fonts or runtime raster images required; Contour uses Rack's system font.
- Build: Rack SDK / Rack Free 2.6.6, Linux x64, C++11. No active Rack process found. Separate temporary runtime profile required for any Rack launch.
- Tests: keep Contour core, small-stack, worker/persistence and equivalence suites; port Orbitals math, visualization, binary factory and audio regression tests. Audio regression depends on source tag v2.1.0: vendor that exact historical engine as a test fixture with its SHA, avoiding a dependency on the source repository or unrelated history merge.
- Version: 2.1.4 → 2.2.0-dev.1, minor addition, development only; no release tag.
- Licensing: Contour is GPL-3.0-only, source Orbitals proprietary. User explicitly authorized GPL-3.0-only for imported Orbitals code and artwork on 2026-11-09. Preserve attribution and record the source's original notice; do not change source licensing files.

## Imported files and integration changes

The complete source-to-destination inventory and source SHA-256 values are in
`001-orbitals-source.json`. In summary:

- `src/orbitals/`: OrbitalModule implementation, four analytical DSP headers,
  three UI files. `modelOrbital` retains its original name and `Orbitals` slug.
- `res/orbitals/`: five original runtime SVGs, both themes and all controls.
- `docs/orbitals/`: original manual, mathematics, validation history, design,
  reference images and editable artwork. Formula delimiters/commands were fixed
  for Obsidian without changing normalization constants or equations.
- `tests/orbitals/`: math, snapshot/display, actual model factory, spectral and
  audio-regression suites; exact historical engine fixture from source tag
  `v2.1.0` (resolved commit recorded in the JSON inventory).
- `src/plugin.*`: only one pluginInstance/init; both model registrations.
- Build globs include Orbitals module/UI directories. Package contains the
  entire `res/` and `docs/`, both licenses and one manifest. SDK-free tests are
  retained. No build/test path requires the original Orbitals directory.
- Original Contour core, module, IDs, worker, SVGs and existing tests are untouched.
  Preservation checks compare source/artwork SHA-256 values. Orbitals DSP and
  display mathematics likewise remain byte-for-byte identical to the imported
  worktree. Only module-header include and widget resource paths changed.

## Licensing

The source manifest and provenance patch retain their historical proprietary
notice. The owner explicitly answered “Kyllä, GPL-3.0-only” on 2026-11-09 when
asked to publish Orbitals code and artwork in Contour under GPL-3.0-only. The
imported snapshot and historical test fixture therefore use the same GPL-3.0-only
license as Contour. See `LICENSES.md`; source repository files remain unchanged.

## Compatibility

- `FilthAssembly/Contour` is unchanged, including nine parameter, six input,
  eight output IDs, seed JSON and theme behavior.
- Current dirty source `FilthAssembly/Orbitals` preserves ten parameter, six
  input, four output IDs, selector defaults/ranges, audio/XYZ mappings, 30 Hz
  snapshot transport and Rack parameter serialization. Internal DSP/filter/phase
  state is not saved, exactly as before.
- Historical HEAD and old releases used `FilthAssemblyOrbital/ORBITAL`. These
  patches do not load automatically as `FilthAssembly/Orbitals`. Change identifiers
  on a backup and remap cables; FM/Sync were removed and input IDs changed in
  the source worktree before migration, not by this consolidation.
- Standalone Contour and current Orbitals packages already share a plugin slug;
  they cannot coexist as two packages. The combined package exposes both models.
- No new DSP features, audio normalization changes, presets or module migrations
  beyond Orbitals. Fugitive/Superposition remain separate projects.

## Automated validation (Linux x64, Rack SDK 2.6.6)

- `make -j2`: compiles and links one shared binary.
- `make test`: Contour core (384 terrains), 512 KiB worker-stack regression
  (192 terrains), module/worker lifecycle, seed JSON and concurrent instances;
  Orbitals 273,466 analytical/DSP checks (Gram error 0.000542473), visualization
  queue saturation/concurrency and allocation-free updates; actual binary model
  factories, parameter ranges, both parameter persistence round trips and
  simultaneous 44.1/48/96 kHz processing with nonzero Orbitals audio.
- Audio regression: 150,000 AUDIO/XYZ comparisons against the vendored source
  `v2.1.0` engine; maximum error 4.39648e-14 V, tolerance 1e-10 V.
- `make test-equivalence`: 768 terrains and motion/CV outputs compare bit for bit
  against Contour 2.1.2, 5,364,128 compared bytes.
- `make test-gui`: live modules from the combined plugin load all controls and
  both panel SVGs, render via OpenGL and switch light/dark/light in place.
  Rendered images visually inspected; no artwork redesigned. Test output is
  ignored under `build/tests/`.
- `make test-migration`: unchanged Contour source/artwork, unchanged Orbitals
  DSP/display math/assets, exact baseline fixture, manifest and SVG parsing,
  one exported init/pluginInstance and both model symbols.

## Fresh checkout, package and Rack validation

- Exported the staged repository to `build/fresh-checkout/` **without `.git` or
  any source Orbitals checkout**. Both `make -j2` and `make test -j2` passed from
  that clean source tree. No dependency on the unrelated source Git history.
- `make test-package` built and inspected one
  `FilthAssembly-2.2.0-dev.1-lin-x64.vcvplugin`: both module manifest entries,
  every runtime SVG byte-identical to its repository copy, full GPL license and
  licensing/attribution record present.
- Rack Free 2.6.6 launched headlessly using an isolated user folder under
  `build/tests/rack-profile/`, extracted and loaded this development package,
  loaded a patch containing both `FilthAssembly/Contour` and
  `FilthAssembly/Orbitals` without missing-model warnings, loaded both sets of
  light/dark panels and controls, then exited cleanly after three seconds.
- The original user Rack profile and installed plugins were not touched. No
  pre-existing Rack session was terminated. No physical-interface listening is
  claimed; nonzero generated Orbitals audio is verified in the binary tests.
- Initial test-host setup issues (manifest-array argument, widget ownership) and
  a first Rack run using development mode were corrected in the harness/profile;
  the final combined binary, GUI and standalone Rack runs pass. No DSP fixes were
  needed.
- Source branch, HEAD, dirty status and every imported source file hash were
  rechecked unchanged after migration.

## Git workflow

Migration is committed and pushed only on destination `dev`, to the existing
`origin` repository. Mainline `master` stays at the inspected baseline. No stable
tag, GitHub release, visibility change, Library submission or history rewrite.
Commit IDs and push outcome are reported in the completion message so this record
does not need to embed its own commit hash.

## Remaining manual checks / next phase

1. Back up a `FilthAssembly/Contour` patch; open it with the development package
   in an isolated Rack profile and confirm controls, cables and seed behavior.
2. Load a current `FilthAssembly/Orbitals` patch; check selectors, geometry CV,
   LEVEL and live display. Confirm intentionally quiet/null paths stay quiet.
3. Listen through an attenuated VCA/mixer and verify level/timbre; automated
   generated audio is not physical audio-interface listening.
4. Toggle Rack themes and room brightness in a real mixed patch; measure Rack
   CPU with several instances. OpenGL renders check graphics but not performance
   on every GPU/platform. Other OS builds remain unvalidated.
5. Review `dev` before a future merge/release. No stable tag, Library submission
   or release is created here. Select the next module for a separate migration;
   do not import Fugitive or Superposition in this phase.
