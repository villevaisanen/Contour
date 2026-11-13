# Changelog

## 2.2.0-dev.4 — 2026-11-13 (development only)

- Remove the Orbitals display's decorative green border and opaque backing so the panel grid remains visible.
- Densify the UI-only orbital cloud while retaining analytical sampling, the dot-matrix style and the existing trajectory rendering.
- Update the Orbitals dark screenshot and add paired visibility/performance measurements; no DSP, routing, controls, panel SVG or Contour changes.

## 2.2.0-dev.3 — 2026-11-13 (development only)

- Preserve the updated Contour and Orbitals light/dark panel artwork.
- Move Contour runtime panels to `res/contour/` and update resource loading and preservation checks.
- Align Orbitals L/R ports and LEVEL knob with the edited panels; synchronize their light/dark positions.
- Refresh module documentation and dark screenshots rendered with the new build.
- Update repository URLs for `Filth-Assembly-Modules`; retain plugin/module slugs, DSP, port IDs and patch format from dev.2.

## 2.2.0-dev.2 — 2026-11-10 (development only)

- Replace Orbitals AUDIO/X/Y/Z ports with L (MONO) and R audio outputs.
- Automatically select original mono A/B mixing without an R cable, or separate A/L and B/R stereo with an R cable (including R-only use).
- Preserve mono DSP, Mix curve/CV and saved parameter IDs/values; stereo Mix/Balance attenuates only, with unity center and 5 ms smoothing.
- Use temporary light/dark test panels labeled L (MONO), R and MIX / BAL.
- Keep output ID 0 as L; repurpose old X ID 1 as R, removing Y/Z IDs 2/3. Old coordinate cables require manual removal/repatching; an old X cable activates stereo. Full backward compatibility is not promised.
- Keep Contour in the combined FilthAssembly package.

## 2.2.0-dev.1 — 2026-11-09 (development only)

- Integrate Orbitals with Contour in the canonical `FilthAssembly` plugin.
- Register `Contour` and `Orbitals` through one plugin instance and initialization entry point.
- Preserve Contour source/artwork and Orbitals analytical DSP, parameter/port IDs and serialization.
- Namespace Orbitals code and both light/dark panel resources without artwork redesign.
- Include source-snapshot provenance, owner-authorized GPL-3.0-only licensing and historical patch compatibility notes.
- Build/package both modules together; import mathematical, visual and self-contained audio regression tests and add combined-binary tests.
- No stable release, release tag, Library submission or mainline merge.

## 2.1.4 (historical Contour release)

- Native Rack light/dark panels and Filth Assembly branding.
- Subsequent correction establishes `FilthAssembly/Contour` (commit `ee059f1`).
- Earlier history and details are preserved in the [Contour manual](docs/Contour.md).
