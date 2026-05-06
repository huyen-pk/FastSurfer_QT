# Core Namespace Refactor Implementation Plan

## Feature

`core_namespace_refactor`

## Project Name

`Open Healthcare`

## Goal

Refactor `core/` so the public C++ API uses `OpenHC` as the root namespace, with the MRI module aligned on `OpenHC::imaging::mri::fastsurfer`, while the `core/` directory layout reflects only the domain hierarchy directly as `imaging/mri/fastsurfer` without adding an `OpenHC/` folder layer.

## Current Repository Facts

- `core/include` already exposes a single public include root containing `fastsurfer/core/*.h` headers.
- `core/src` currently stores implementation files in a flat layout even though they implement one cohesive MRI imaging domain.
- `core/tests` currently stores test sources in a flat layout and wires them explicitly from `core/tests/CMakeLists.txt`.
- The public namespace across `core/` is currently `fastsurfer::core`, with constants under `fastsurfer::core::constants`.
- The current library target is `fastsurfer_core`, so the build surface still exposes legacy naming even when the code is moved.
- `_app/desktop/src/app/application_controller.cpp` is an external consumer of `core/` headers and symbols, so this refactor cannot stop at `core/` alone.
- README text also mentions `fastsurfer::core::MghImage`, so documentation will need a narrow terminology update.

## Scope

This feature performs a structural and namespace refactor of the native imaging core.

In scope:

- Move `core/` headers into a namespace-aligned public include tree.
- Move `core/` source files into a matching implementation tree.
- Move `core/` tests and test helper headers into a matching test tree.
- Rename the C++ namespace from `fastsurfer::core` to `OpenHC::imaging::mri::fastsurfer`.
- Rename the constants namespace from `fastsurfer::core::constants` to `OpenHC::imaging::mri::fastsurfer::constants`.
- Keep the directory tree rooted at `imaging/...` and explicitly avoid creating an `OpenHC/` folder.
- Update `core/` CMake wiring to reference the new paths.
- Rename the build target so it reflects the `OpenHC` project identity instead of the legacy `fastsurfer` name.
- Update direct in-repo consumers such as `_app/desktop` and narrow README references.

Out of scope:

- Reworking algorithms, image-processing behavior, or test expectations.
- Adding a permanent compatibility layer that preserves both namespace trees indefinitely.
- Refactoring unrelated `_app/desktop` architecture beyond the import and type updates required by this move.

## Architecture Decision

### Chosen Approach

Adopt a clean namespace and directory realignment in one coherent migration:

- Public headers move from `core/include/fastsurfer/core/*.h` to `core/include/imaging/mri/fastsurfer/*.h`.
- Sources move from `core/src/*.cpp` to `core/src/imaging/mri/fastsurfer/*.cpp`.
- Tests move from `core/tests/*.cpp` to `core/tests/imaging/mri/fastsurfer/*.cpp`.
- Test support headers move into `core/tests/imaging/mri/fastsurfer/` as well.
- All declarations and definitions migrate to `OpenHC::imaging::mri::fastsurfer`.
- The filesystem intentionally starts at `imaging/` rather than `OpenHC/` so the repository models domain structure without duplicating the global namespace root as a directory prefix.
- The primary library target is renamed from `fastsurfer_core` to `openhc_imaging_mri_fastsurfer` so the build graph reflects the new public identity.

This keeps the physical package structure aligned with the semantic domain boundary while treating `OpenHC` as the organization-level namespace root rather than a filesystem package prefix. It removes the current mismatch where the code is MRI imaging infrastructure but the directory tree still exposes a legacy `fastsurfer/core` layout.

### Why This Approach

- It produces one authoritative API path instead of accumulating transitional aliases.
- It keeps the public include root stable at `core/include`, so consumers still depend on a single include directory while the nested path becomes domain-accurate.
- It keeps the directory tree concise by avoiding a redundant `OpenHC/` path segment that adds no domain information.
- It aligns the build target with the same root identity as the C++ namespace, so code, build metadata, and project naming do not diverge.
- It respects the user’s requirement that directories reflect the namespace hierarchy, not just the symbols.

### Alternative Rejected

Keep the directory tree as-is and rename only the namespace.

Why rejected:

- It would leave the repository with a misleading physical layout.
- It would force future contributors to remember two unrelated naming systems.
- It would satisfy only half of the requested refactor.

### Alternative Rejected

Introduce forwarding headers and namespace aliases from `fastsurfer::core` to `OpenHC::imaging::mri::fastsurfer`.

Why rejected:

- It prolongs ambiguity about the canonical API.
- It increases maintenance cost for every future header addition.
- The current blast radius is still small enough to complete a clean in-repo migration directly.

### Alternative Rejected

Create an `OpenHC/imaging/mri/fastsurfer` directory tree to mirror the namespace literally.

Why rejected:

- `OpenHC` is a logical root namespace and project identity, not a meaningful storage boundary inside `core/`.
- Adding an `OpenHC/` folder would make include paths longer without improving modular separation.
- The user explicitly requested that no `OpenHC` folder be introduced.

## Proposed Target Structure

```text
core/
  include/
    imaging/
      mri/
        fastsurfer/
          conform_policy.h
          constants.h
          image_metadata.h
          mgh_image.h
          nifti_converter.h
          step_conform.h
          step_conform_request.h
          step_conform_result.h
  src/
    imaging/
      mri/
        fastsurfer/
          conform_policy.cpp
          mgh_image.cpp
          nifti_converter.cpp
          step_conform.cpp
  tests/
    imaging/
      mri/
        fastsurfer/
          TestConstants.h
          TestHelpers.h
          test_conform_policy.cpp
          test_native_reconform_non_conformed.cpp
          test_nifti_converter.cpp
          test_nifti_converter_roundtrip.cpp
          test_step_conform_parity.cpp
          test_step_conform.cpp
    CMakeLists.txt
```

## Public API Mapping

| Current include | Target include |
| --- | --- |
| `fastsurfer/core/conform_policy.h` | `imaging/mri/fastsurfer/conform_policy.h` |
| `fastsurfer/core/constants.h` | `imaging/mri/fastsurfer/constants.h` |
| `fastsurfer/core/image_metadata.h` | `imaging/mri/fastsurfer/image_metadata.h` |
| `fastsurfer/core/mgh_image.h` | `imaging/mri/fastsurfer/mgh_image.h` |
| `fastsurfer/core/nifti_converter.h` | `imaging/mri/fastsurfer/nifti_converter.h` |
| `fastsurfer/core/step_conform.h` | `imaging/mri/fastsurfer/step_conform.h` |
| `fastsurfer/core/step_conform_request.h` | `imaging/mri/fastsurfer/step_conform_request.h` |
| `fastsurfer/core/step_conform_result.h` | `imaging/mri/fastsurfer/step_conform_result.h` |

Namespace mapping:

- `fastsurfer::core` -> `OpenHC::imaging::mri::fastsurfer`
- `fastsurfer::core::constants` -> `OpenHC::imaging::mri::fastsurfer::constants`

Build identity mapping:

- CMake project name: `FastSurferCore` -> `OpenHealthcare`
- Library target: `fastsurfer_core` -> `openhc_imaging_mri_fastsurfer`

## Implementation Sequence

1. Create the new nested include, source, and test directories under `core/`.
2. Move the header files to `core/include/imaging/mri/fastsurfer/` and update internal includes to the new public paths.
3. Move the source files to `core/src/imaging/mri/fastsurfer/` and update include statements accordingly.
4. Rename namespaces in headers and sources from `fastsurfer::core` to `OpenHC::imaging::mri::fastsurfer`, including the constants namespace.
5. Move test sources and helper headers to `core/tests/imaging/mri/fastsurfer/` and update include paths plus symbol references.
6. Update `core/CMakeLists.txt` to reference the nested source paths, rename the CMake project to `OpenHealthcare`, rename the library target to `openhc_imaging_mri_fastsurfer`, and keep `${CMAKE_CURRENT_SOURCE_DIR}/include` as the public include root.
7. Update `core/tests/CMakeLists.txt` to compile test sources from the nested test paths and link against `openhc_imaging_mri_fastsurfer`.
8. Update `_app/desktop/src/app/application_controller.cpp` and any other in-repo consumers to the new include paths, namespace, and target name where applicable.
9. Update narrow documentation references that still name `fastsurfer::core` or `fastsurfer_core` when they refer to the native core API.
10. Rebuild `core/` and run the focused native core test suite.

## CMake Strategy

- Rename the CMake project declaration from `FastSurferCore` to `OpenHealthcare` so the generated project metadata reflects the product name `Open Healthcare`.
- Rename the library target from `fastsurfer_core` to `openhc_imaging_mri_fastsurfer` so the build graph reflects the new `OpenHC` root namespace while preserving the module path beneath it.
- Continue exporting `core/include` as the public include root.
- Use explicit file lists in CMake rather than broad globbing so the build remains reviewable and stable.
- Keep current test executable names intact so existing CTest names and downstream scripts do not churn unnecessarily.

## Consumer Strategy

Direct consumers inside this repository should be updated in the same change set.

Known direct consumers today:

- `core/` sources and tests
- `_app/desktop/src/app/application_controller.cpp`
- README references to the public native-core types

What not to build now:

- No umbrella compatibility header tree under both `fastsurfer/core` and `imaging/mri/fastsurfer`.
- No namespace alias bridge kept in public headers after the move.
- No extra `OpenHC/` directory layer under `include`, `src`, or `tests`.
- No package-export redesign beyond the target rename required by this refactor.

## Risks and Mitigations

### Risk: Build breaks due to partial path migration

Mitigation:

- Move headers first, then sources, then tests, then consumers, and update CMake only after the new paths exist.
- Keep the include root unchanged so only include suffixes change.

### Risk: Downstream code outside `core/` still references `fastsurfer::core`

Mitigation:

- Search the workspace for both include paths and symbol names before and after the move.
- Update `_app/desktop` in the same patch series.

### Risk: Build scripts and link directives still reference `fastsurfer_core`

Mitigation:

- Search the workspace for `fastsurfer_core` before and after the rename.
- Update every in-repo `target_link_libraries` consumer in the same change set.

### Risk: Test failures get conflated with existing fixture-path issues

Mitigation:

- Treat this refactor as a no-behavior-change migration.
- Use compilation plus the existing focused test suite to validate structural correctness, while distinguishing any pre-existing fixture failures from migration regressions.

## Validation Plan

Primary validation:

- Reconfigure or rebuild `core/` after the path move.
- Run the focused `core/` test suite through the existing build tree.

Secondary validation:

- Verify `_app/desktop` still compiles against the updated include paths and namespace references if it is part of the active local build.
- Search the repository to confirm no remaining in-repo includes reference `fastsurfer/core/`, no active code references `fastsurfer::core`, and no build files reference `fastsurfer_core`.

## Success Criteria

- `core/` headers, sources, and tests live under directories that mirror `imaging/mri/fastsurfer`.
- The public C++ namespace is consistently `OpenHC::imaging::mri::fastsurfer`.
- No `core/` directory adds an `OpenHC/` path segment.
- The `core/` CMake project and library target reflect the `Open Healthcare` and `OpenHC` identity instead of `FastSurfer` naming.
- `core/` builds successfully with the updated CMake paths.
- Focused `core/` tests run with no new failures attributable to the refactor.
- In-repo consumers no longer include `fastsurfer/core/*`, refer to `fastsurfer::core`, or link against `fastsurfer_core`.