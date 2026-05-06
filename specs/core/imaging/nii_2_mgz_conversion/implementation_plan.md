## Plan: NIfTI To MGZ Conversion

Add a standalone native NIfTI-to-MGZ conversion boundary in `core/` by reusing ITK for `.nii` and `.nii.gz` reading and continuing to use `MghImage` for MGZ serialization. The converter should be explicit rather than hidden inside `MghImage::load(path)`, so format conversion stays separate from the existing MGZ loader and can be reused by tests, future CLI entrypoints, and the conform pipeline without overloading the meaning of `load()`.

Status: approved by user on 2026-05-03.

**Steps**
1. Phase 1: dependency and boundary setup. Extend `core/CMakeLists.txt` to request the ITK NIfTI IO component needed for reading `.nii` and `.nii.gz`. Keep ZLIB and existing ITK modules unchanged otherwise.
2. Phase 2: standalone converter API. Add a new public utility surface under `core/include/imaging/mri/fastsurfer/` and `core/src/imaging/mri/fastsurfer/` for explicit NIfTI conversion, such as `NiftiConverter::loadAsMgh(path)` or `NiftiConverter::convertToMgh(inputPath, outputPath)`. The converter should read a 3D scalar volume with ITK, map spacing/origin/direction into `MghImage::Header`, convert voxel payload into the existing in-memory representation, and reject unsupported cases such as multi-frame or non-3D inputs. *Depends on 1.*
3. Phase 3: preserve MGZ loader semantics. Keep `core/include/imaging/mri/fastsurfer/mgh_image.h` and `core/src/imaging/mri/fastsurfer/mgh_image.cpp` focused on MGZ/MGH loading and writing, apart from any small helpers needed to build an `MghImage` from already-normalized metadata.
4. Phase 4: integrate with conform path deliberately. Update `core/src/imaging/mri/fastsurfer/step_conform.cpp` to call the new converter only when a NIfTI input is intended by the approved scope, rather than silently broadening `MghImage::load()`. This can be done either with extension-based routing at the service layer or with an explicit preparatory conversion step before `run()` uses the image. *Depends on 2 and 3.*
5. Phase 5: real-data verification. Add a new `core/` test executable that loads real NIfTI fixtures from `data/parrec_oblique/NIFTI`, converts them to temporary MGZ outputs, reloads the MGZ, and checks dimensions, spacing, affine-derived orientation, and selected voxel values. Use one non-oblique and one oblique fixture. *Depends on 2.*
6. Phase 6: integration test coverage. Add a conform-step test that feeds a NIfTI input through the explicit conversion path and verifies output writing succeeds and preserves expected geometry contracts. Keep Python parity additive and separate from the core conversion oracle. *Depends on 2 and 4.*
7. Phase 7: docs/spec alignment. Update the relevant `specs/core/imaging/pipeline_conform_and_save_orig` notes and any desktop/core README text that currently claims the native path is MGZ-only, but keep scope explicitly limited to NIfTI input conversion into the existing MGZ-centered pipeline. *Depends on 4 and 6.*

**Relevant files**
- `core/CMakeLists.txt` — add the ITK NIfTI IO component and register any new converter source file.
- `core/include/imaging/mri/fastsurfer/mgh_image.h` — keep MGZ-focused public semantics; only expose helper construction points if the converter needs them.
- `core/src/imaging/mri/fastsurfer/mgh_image.cpp` — keep MGZ/MGH parsing and serialization centered here.
- `core/include/imaging/mri/fastsurfer/` new converter header — define the explicit NIfTI conversion API.
- `core/src/imaging/mri/fastsurfer/` new converter source — implement the ITK-backed NIfTI reader and `MghImage` construction.
- `core/src/imaging/mri/fastsurfer/step_conform.cpp` — route NIfTI inputs through the explicit converter instead of overloading `MghImage::load()`.
- `core/tests/CMakeLists.txt` — register new NIfTI conversion and integration tests.
- `core/tests/imaging/mri/fastsurfer/test_step_conform.cpp` — reference pattern for direct service invocation.
- `core/tests/imaging/mri/fastsurfer/test_python_parity_conform_step.cpp` — reference pattern for real-file comparison and temp outputs; do not make it the only oracle for conversion correctness.
- `data/parrec_oblique/README.md` — source description for real oblique fixtures.
- `specs/core/imaging/pipeline_conform_and_save_orig/implementation_plan.md` and `specs/core/imaging/pipeline_conform_and_save_orig/bdd_requirements.md` — update scope notes after implementation if the repo continues to use this feature folder.

**Verification**
1. Configure and build `core/` after adding the ITK NIfTI module.
2. Run the new standalone conversion-focused test on at least one axial and one oblique fixture from `data/parrec_oblique/NIFTI`.
3. Run the integration test that exercises the converter plus `ConformStepService`.
4. Run the existing `core/` MGZ tests to ensure MGZ behavior is not regressed.
5. Inspect one converted MGZ output by reloading it through `MghImage::load()` and verifying dimensions, spacing, and orientation are stable across the round-trip.
6. If the Python environment is available, optionally compare one converted oblique case against nibabel metadata as a secondary confidence check, not the primary conversion oracle.

**Decisions**
- Recommended library stack: ITK for NIfTI reading, existing `MghImage` code for MGZ writing.
- Selected API shape: use a standalone converter API instead of folding NIfTI support into `MghImage::load(path)`.
- Included scope: `.nii` and `.nii.gz` scalar 3D inputs into the existing conform pipeline through an explicit conversion step.
- Excluded scope: PAR/REC parsing, 4D NIfTI support, DICOM ingestion, or replacing the current MGZ serializer.
- Key risk: affine/orientation translation from ITK NIfTI metadata into FreeSurfer-style direction cosines must be verified on the oblique fixtures, not only on axial data.

**Further Considerations**
1. Recommended converter shape: start with a small reusable API that can return `MghImage` in memory and optionally write MGZ to disk, so the service and future tools share one implementation.
2. If you later want desktop users to convert files without running the full conform step, expose that same converter as a separate application action rather than adding a second conversion path.
