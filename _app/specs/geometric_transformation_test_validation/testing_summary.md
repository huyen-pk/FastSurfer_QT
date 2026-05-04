# Testing Summary: Oblique Geometry Conform Validation

## Scope

- Added an ITK-backed expected-geometry oracle with Eigen-based matrix/vector comparisons for end-to-end oblique conform verification in `orientation="native"` and `orientation="lia"`.
- Preserved direct NIfTI-to-MGZ copy-orig validation as the ingress check.

## Commands Run

- `cmake -S /home/huyenpk/Projects/FastSurfer_QT/_app/core -B /home/huyenpk/Projects/FastSurfer_QT/_app/core/build`
- `cmake --build /home/huyenpk/Projects/FastSurfer_QT/_app/core/build --target test_conform_step_service_nifti`
- `cd /home/huyenpk/Projects/FastSurfer_QT/_app/core/build/tests && ./test_conform_step_service_nifti`
- `cd /home/huyenpk/Projects/FastSurfer_QT/_app/core/build/tests && ./test_nifti_converter`

## Results

- `run_conform_step_on_nifti_input_should_write_copy_and_conformed_outputs`: passed
- `convert_nifti_to_mgz_should_preserve_expected_geometry_and_voxels`: passed
- `run_conform_step_on_non_conformed_mgz_should_produce_conformed_output`: not rerun in this change set; still known to fail for a pre-existing fixture-path issue

## Observations

- The new oblique end-to-end test now verifies affine-equivalent ITK geometry, center preservation, orthonormality, handedness, and deterministic probe-based interpolation error for both `native` and `lia` output policies.
- The handwritten orientation-permutation and expected-affine construction logic was removed from the test and replaced with an ITK expected-geometry object plus ITK physical-point comparisons.
- The native conform path required preserving the source oblique direction cosines to satisfy the new geometry oracle.
- The workspace-level CTest tool resolved the wrong build tree in this environment, so the passing validation was executed directly from `/home/huyenpk/Projects/FastSurfer_QT/_app/core/build/tests`.
- The failing MGZ-native regression source still points at `data/Subject140/140_orig.mgz`, which is absent in this repository layout.