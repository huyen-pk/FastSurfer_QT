# Probe Generation Improvements Implementation Plan

## Feature

`probe_generation_improvements`

## User-Directed Constraints

- Document the approach under `_app/specs` on the current branch.
- Scope the work to `_app/core/tests/test_step_conform.cpp` probe-generation and probe-evaluation helpers.
- Do not change the native conform implementation in `_app/core/src`; improve the regression test oracle only.

## Goal

Strengthen the conform-step regression tests so that voxel probe sampling is more effective at catching geometry and interpolation regressions.

The updated test helper should:

- cover boundary and near-boundary voxels that were previously untested,
- retain deterministic sampling so failures remain reproducible,
- increase spatial coverage without turning the test into a full-volume comparison,
- add a few fixed-seed jittered samples so small localized islands are less likely to fall between Cartesian probe planes,
- reduce circularity in the probe intensity oracle by sampling against the expected geometry rather than the saved output geometry.

## Current Repository Facts

- The touched helper lives in `_app/core/tests/test_step_conform.cpp`.
- `verifyOutputGeometry()` already performs direct checks on output dimensions, spacing, direction, origin, handedness, and physical center.
- `evaluateProbes()` is therefore responsible mainly for value-oriented regression detection rather than primary geometry validation.
- The production conform path resamples every output voxel through either:
  - `mapImageWithoutInterpolation(...)` for integer-transform cases, or
  - `mapImageWithItk(...)` and `sampleLinearLikeScipy(...)` for interpolation cases.
- The production interpolation path has explicit boundary handling in `_app/core/src/step_conform.cpp`, so boundary coverage in the tests is important.

## Problem Statement

The previous probe generator used a sparse interior lattice only. That left two gaps:

1. Boundary shell blind spot:
   - probes excluded index `0` and `dimension - 1`, which meant faces, edges, and corners were not sampled,
   - this is exactly where clamping, out-of-bounds handling, and upper-neighbor collapse are most likely to regress.

2. Oracle circularity:
   - probe intensities were sampled from source space using physical points derived from `outputGeometry`,
   - that verified consistency with the saved geometry, but it did not independently pressure-test whether the saved geometry matched the expected geometry.

## Scope For This Spec

In scope:

- deterministic probe-generation improvements in `_app/core/tests/test_step_conform.cpp`,
- documentation of the testing strategy and rationale,
- focused validation using the rebuilt `test_step_conform` executable.

Out of scope:

- production conform-step code changes in `_app/core/src/step_conform.cpp`,
- broader refactors of the testing framework,
- full-volume parity comparisons,
- adding new fixtures or changing tolerances in unrelated tests.

## Implemented Approach

### 1. Boundary-Inclusive Axis Sampling

Add `clampProbeIndex(...)` and update `axisProbeSamples(...)` so each axis includes:

- `0`,
- `1`,
- quarter, midpoint, and three-quarter positions,
- `dimension - 2`,
- `dimension - 1`.

Why:

- catches face and corner regressions,
- directly exercises interpolation and bounds behavior near the image shell,
- preserves deterministic sample placement.

### 2. Deterministic Stratified Interior Sampling

Add `stratifiedAxisProbeSamples(...)` with a fixed bin count of six per axis.

Why:

- expands interior coverage beyond the previous coarse lattice,
- distributes probes more evenly across the full field of view,
- avoids randomization so test failures stay reproducible.

### 3. Merge Boundary And Stratified Cartesian Probe Sets

Use `appendCartesianProbes(...)` to build:

- a boundary-heavy Cartesian set, and
- a stratified interior Cartesian set.

Then sort and deduplicate the combined probes in `generateProbeIndices(...)`.

Why:

- increases coverage without introducing a full dense grid,
- keeps the implementation simple and inspectable,
- preserves a deterministic probe list for debugging.

### 4. Add Deterministic Jittered Samples

Add `generateJitteredProbeIndices(...)` with a fixed RNG seed and a coarse `3 x 3 x 3` binning strategy.

Each coarse volume bin contributes one integer voxel sample chosen uniformly within that bin, but because the seed is fixed the resulting samples are stable across runs.

Why:

- helps catch small isolated artifact islands that sit between the boundary and stratified Cartesian planes,
- improves spatial diversity without introducing test flakiness,
- keeps the added sample count modest.

### 5. Use Expected Geometry For Intensity Sampling

Keep the physical-error calculation based on:

- actual point from `outputGeometry`, and
- expected point from `expectedGeometry`.

Change the intensity oracle so source-space sampling uses `expectedPoint` rather than `actualPoint`.

Why:

- keeps `maxPhysicalError` as a direct mismatch signal,
- makes the intensity oracle less circular,
- gives the probe-value check a more independent reference against the expected conform geometry.

## Architecture Decision

This change stays entirely in the test oracle layer.

Reasoning:

- the production code path already has focused geometry and interpolation logic,
- the identified weakness was in test discriminative power, not in the conform algorithm itself,
- improving the probe helper increases QA coverage while minimizing risk to runtime behavior.

What not to build for this task:

- no generalized sampling framework,
- no randomized Monte Carlo harness,
- no new test binary or parallel spec tree,
- no production-side instrumentation.

## File-Level Plan

Primary file:

- `_app/core/tests/test_step_conform.cpp`

New documentation artifact:

- `_app/specs/probe_generation_improvements/implementation_plan.md`

## Validation Plan

Focused validation should rebuild the `_app/core` test target and run the conform-step cases that exercise the helper:

```bash
cd /home/huyenpk/Projects/FastSurfer_QT/_app/core/build
cmake ..
cmake --build . --target test_step_conform
cd tests
./test_step_conform standard
./test_step_conform oblique
./test_step_conform force-conformed
```

Expected result:

- build succeeds,
- all three focused runs exit successfully with no output.

## Observed Validation Result

The focused validation completed successfully after regenerating the stale `_app/core` build tree so it picked up the current `test_step_conform` target.

Validated cases:

- `standard`
- `oblique`
- `force-conformed`

Outcome:

- all completed with exit code `0`,
- no additional runtime output was produced,
- the stronger probe set did not expose a failing mismatch in the current implementation.

## Risks And Tradeoffs

### Risk: Test Runtime Growth

Increasing the probe set raises per-test work.

Mitigation:

- keep the probe set deterministic and modest in size,
- avoid full-volume comparisons,
- restrict validation to the existing focused executable.

### Risk: More Sensitive Oracle Could Expose Existing Numerical Drift

A stronger probe set may surface borderline tolerances in the future.

Mitigation:

- keep the current max and 95th-percentile error thresholds unchanged for now,
- use the deterministic probe set to inspect any future regression before changing tolerances.

### Risk: Duplicate Coverage With Geometry Assertions

Some overlap remains between `verifyOutputGeometry()` and `evaluateProbes()`.

Mitigation:

- geometry checks remain the primary affine oracle,
- probe checks now focus more cleanly on value correctness at expected spatial locations.

## Natural Follow-Up Work

Possible next steps if tighter diagnostics are needed later:

- add mean absolute intensity error alongside max and 95th-percentile error,
- add a small explicit corner-and-edge-only report for easier failure triage,
- promote selected probe statistics into a dedicated testing summary artifact if the conform test surface grows further.