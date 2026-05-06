# Revised Plan: Library-Backed Oblique Conform Verification

**Investigator:** GitHub Copilot
**Date:** 2026-05-03
**Scope:** revise the oblique conform verification plan to use common C++ medical-imaging and linear algebra libraries wherever possible, and avoid manual test math except for FastSurfer-specific policy rules.

## Executive Summary

Strengthen the end-to-end oblique conform verification in `core/tests/imaging/mri/fastsurfer/test_step_conform.cpp` instead of creating a separate geometry-only harness. Keep `core/tests/imaging/mri/fastsurfer/test_nifti_converter.cpp` as an ingress and MGZ-serialization test only. For the conform test, use ITK as the spatial oracle for image geometry, physical-point transforms, and interpolation, and use Eigen for independent affine and rigid-geometry checks. Limit custom test math to FastSurfer-specific policy rules such as target voxel-size selection, FOV sizing rules, and optional final intensity scaling checks.

## 1. Test Target And File Placement

### Primary End-to-End Verification File
Path: `core/tests/imaging/mri/fastsurfer/test_step_conform.cpp` using the `oblique` case.

Rationale:
- This file already exercises `ConformStepService::run()` on a real oblique NIfTI fixture.
- The missing coverage is not loader support; it is output affine correctness, physical-center preservation, target-header correctness, and oblique resampling geometry.
- Keeping the test in the end-to-end conform harness aligns with QA separation of responsibilities: converter verification stays in `core/tests/imaging/mri/fastsurfer/test_nifti_converter.cpp`, while release-gate oblique conform correctness stays in `core/tests/imaging/mri/fastsurfer/test_step_conform.cpp` under the `oblique` case.

### Supporting Ingress Test
Path: `core/tests/imaging/mri/fastsurfer/test_nifti_converter.cpp`

Role after revision:
- verify `.nii` support, direct NIfTI-to-MGZ conversion, saved-vs-in-memory consistency, spacing/orientation/affine preservation at conversion time
- do not treat it as evidence that conform geometry or oblique resampling is correct

## 2. Fixture Strategy

### Primary Conform Fixture
Use `data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii` as the main end-to-end conform oracle.

Why this fixture:
- it is a real 3D oblique volume rather than a 2D slice stack surrogate
- it exercises through-plane geometry as well as in-plane rotation
- it is already the fixture used by the current `test_step_conform.cpp` oblique case

### Optional Secondary Fixture
Use `data/parrec_oblique/NIFTI/T1W_trans_5_15_30_SENSE_10_1.nii` later as a lighter-weight oblique regression, but do not make it the main conform geometry oracle.

### Baseline Control Fixture
Use `data/parrec_oblique/NIFTI/T1W_trans_0_0_0_SENSE_4_1.nii` only as a converter or low-complexity sanity control, not as the primary end-to-end conform test.

## 3. Library-Backed Verification Stack

### Medical-Imaging Library: ITK
Use ITK in the test for:
- loading the source NIfTI fixture as the reference image
- reading spacing, direction, origin, and region information
- transforming voxel indices to physical points and physical points back to continuous source indices
- trilinear interpolation of the source float image at sampled physical points
- verifying output geometry through ITK image metadata when the output MGZ is re-imported into an ITK image if that path is added

ITK-backed verification should replace custom implementations for:
- index-to-physical coordinate transforms
- physical-to-continuous-index transforms
- interpolation on sampled probe points
- most image-space geometry handling

### Linear Algebra Library: Eigen
Use Eigen in the test for:
- independent 4x4 affine assembly and comparison
- determinant, handedness, and orthonormality checks on output directions
- world-center preservation checks
- landmark world-coordinate calculations
- max-element and norm-based error reporting on affine differences

Because Eigen is already discoverable in the configured build through ITK-related CMake state, it is the preferred common C++ matrix library for test-side affine oracles.

### Keep Custom Test Math Only For
- `vox_size=min` and `threshold1mm` policy
- `auto` versus `fov` target-size rules
- explicit orientation-policy expectations such as `native` versus `lia`
- optional replication of the final FastSurfer-specific intensity scaling contract if final uint8 voxel checks are added

## 4. Revised Test Structure

### Subcase 1: Native Orientation Geometry Preservation
Run the oblique 3D fixture through `ConformStepService::run()` with `orientation="native"`.

Purpose:
- isolate voxel-size selection, target-dimension sizing, center preservation, and spatial resampling geometry without introducing an orientation-code change

Use libraries for verification:
- ITK: confirm physical point consistency between output voxels and source-space sample positions
- Eigen: compute the expected affine from preserved world center, expected spacing, expected dimensions, and source orientation matrix

### Subcase 2: LIA Orientation Geometry Transform
Run the same oblique 3D fixture through `ConformStepService::run()` with `orientation="lia"`.

Purpose:
- exercise the full oblique-to-target orientation transform produced by `buildTargetHeader()` in `core/src/imaging/mri/fastsurfer/step_conform.cpp`

Use libraries for verification:
- ITK: spatial probe sampling across the resampled output volume
- Eigen: construct the expected LIA target affine independently and compare against the saved conformed MGZ affine

## 5. Assertions To Keep, Replace, Or Add

### Keep
- output existence checks
- success and `alreadyConformed` state checks
- output type, frame count, spacing, dimensions, and orientation metadata checks

### Replace Manual Math With Libraries
Replace manual or ad hoc affine reasoning in the tests with:
- ITK image geometry for source physical-space transforms
- Eigen matrices and vectors for affine assembly, inversion, determinant, and error metrics

### Add New Assertions
1. Output affine equals an independently constructed Eigen affine oracle within tolerance.
2. Source and output image centers in world coordinates match within tolerance.
3. Output direction matrix is orthonormal within tolerance and has the expected handedness sign.
4. For a fixed landmark set of output voxels, the ITK-derived physical points match the physical points predicted by the Eigen affine oracle.
5. For a deterministic stratified probe set, the conformed output intensities are consistent with ITK trilinear interpolation from the source image at the corresponding physical locations.
6. Report max and 95th-percentile probe error rather than a single anecdotal point comparison.

## 6. Independent Target Geometry Oracle

Construct the target geometry in the test without calling production helpers such as `computeTargetVoxelSize()`, `computeTargetImageSize()`, or `buildTargetHeader()`.

### What Remains Custom
- derive `target_voxel_size` from request policy
- derive `target_dimensions` from request policy and source FOV
- choose target direction explicitly: source direction for `native`, fixed LIA basis for `lia`

### What Uses Eigen
- construct the 3x3 direction matrix
- build the 4x4 target affine from direction, spacing, dimensions, and preserved source world center
- compare the predicted affine with the actual conformed MGZ affine

This preserves oracle independence while removing most manual matrix code from the test body.

## 7. Resampling Geometry Verification Plan

Use ITK to validate the oblique resampling path instead of custom interpolation code wherever possible.

### Probe Selection
Choose a deterministic probe set covering:
- image center
- near-corner interior points
- mid-slab points along each axis
- a coarse stratified grid such as `4 x 4 x 4` or `5 x 5 x 5`
- a small number of near-boundary points

### Probe Workflow
For each sampled output voxel:
1. Convert the output voxel index to a physical point using the output affine oracle and confirm consistency with ITK geometry.
2. Map the physical point back into the source image using ITK continuous-index transforms.
3. Interpolate the source float image with ITK linear interpolation.
4. Compare the interpolated value to the conformed output voxel, optionally after applying the expected scaling policy if that stage is under test.

### Statistical Reporting
Use deterministic numeric thresholds rather than inferential statistics:
- max absolute geometric error
- max and 95th-percentile interpolation error on the fixed probe set
- hard pass/fail thresholds per metric

## 8. Minimal Custom Helpers Still Needed

Keep helper code small and library-oriented:
- path and temp-directory setup helpers
- thin wrapper to load the source NIfTI fixture with ITK
- thin wrapper to convert `MghImage::affine()` into an Eigen 4x4 matrix
- thin wrapper to extract output-center and direction data from the reloaded MGZ
- deterministic probe-generation helper
- concise assertion helpers for Eigen and ITK numeric comparisons

Avoid introducing custom helpers for:
- matrix inverse
- determinant
- orthonormality checks
- trilinear interpolation
- physical-point transforms

Those should all come from Eigen and ITK.

## 9. CMake And Dependency Implications

### ITK
Current `core/CMakeLists.txt` already links components that support this direction:
- `ITKImageFunction`
- `ITKIONIFTI`
- `ITKTransform`

If test-side interpolation helpers require a more specific ITK interpolation header or module, add only the minimal needed ITK component rather than a broad imaging bundle.

### Eigen
Add an explicit `find_package(Eigen3 REQUIRED)` in `core/CMakeLists.txt` only if the test target needs direct include and link exposure rather than relying on transitive availability through ITK. Keep Eigen usage scoped to tests and verification utilities unless production code later benefits from it.

## 10. Risks And Mitigations

### Risk 1: Coordinate-System Convention Confusion
Mitigation:
- use ITK as the reference for source image physical-space interpretation
- explicitly document any RAS/LPS sign conventions in the test comments
- validate the baseline control fixture first to pin down convention assumptions before trusting the oblique case

### Risk 2: Overfitting To Production Implementation
Mitigation:
- do not call production helper functions in the oracle
- use ITK and Eigen as independent libraries for geometry and matrix reasoning
- keep FastSurfer-specific policy calculations confined to the few rules that libraries cannot supply

### Risk 3: Too Much Manual Reimplementation In Tests
Mitigation:
- every new helper should be justified as policy-specific rather than math-specific
- default to ITK or Eigen whenever a helper would otherwise duplicate standard library functionality

### Risk 4: Brittle Full-Volume Voxel Comparisons
Mitigation:
- use deterministic stratified probe sets and percentile/max error summaries
- reserve exhaustive voxel comparisons for converter-only tests or secondary parity checks

## 11. Verification Criteria

1. `test_nifti_converter.cpp` continues to pass and remains scoped to ingress and MGZ serialization.
2. `test_step_conform.cpp` passes on the 3D oblique fixture for `orientation="native"` and `orientation="lia"`.
3. Output affine, center preservation, spacing, dimensions, and handedness all satisfy hard thresholds.
4. ITK-interpolated probe checks satisfy max and 95th-percentile numeric thresholds.
5. No part of the primary oblique conform verification depends on Python parity.

## 12. Summary

- Keep `core/tests/imaging/mri/fastsurfer/test_nifti_converter.cpp` for conversion ingress only.
- Strengthen `core/tests/imaging/mri/fastsurfer/test_step_conform.cpp` into the primary end-to-end oblique conform verification through its `oblique` case.
- Use ITK for physical-space geometry and interpolation checks.
- Use Eigen for independent affine, determinant, orthonormality, and center-preservation checks.
- Keep custom test computation only for FastSurfer-specific conform policy rules.
- Prefer deterministic probe-based numeric validation over manual matrix or interpolation code in the tests.
