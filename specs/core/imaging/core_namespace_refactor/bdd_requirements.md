# BDD Requirements: Core Namespace Refactor

## Feature

Refactor `core/` so the native MRI imaging module uses `OpenHC` as the root C++ namespace, keeps the filesystem rooted at `imaging/...` without an `OpenHC/` folder, and updates the build target and in-repo consumers to the new public identity.

## Behavioral Intent

The native core should present one coherent public identity across code, include paths, directory structure, and build metadata.

The code should no longer expose `fastsurfer::core` as the active namespace, should no longer expose `fastsurfer/core/...` as the public header layout, and should no longer expose `fastsurfer_core` as the active library target.

At the same time, the repository structure should avoid introducing an unnecessary `OpenHC/` directory prefix. `OpenHC` is the logical namespace root, while `imaging/mri/fastsurfer` remains the physical module path.

## Acceptance Criteria

### Namespace And Include Surface

- `core/` public headers live under `core/include/imaging/mri/fastsurfer/`.
- Public C++ declarations in the migrated module use `OpenHC::imaging::mri::fastsurfer`.
- Constants previously under `fastsurfer::core::constants` move to `OpenHC::imaging::mri::fastsurfer::constants`.
- In-repo consumers include headers from `imaging/mri/fastsurfer/...` rather than `fastsurfer/core/...`.
- Active in-repo code no longer refers to `fastsurfer::core`.

### Directory Structure

- `core/src` implementation files for this module live under `core/src/imaging/mri/fastsurfer/`.
- `core/tests` sources and test helpers for this module live under `core/tests/imaging/mri/fastsurfer/`.
- No `core/` include, source, or test directory introduces an `OpenHC/` path segment.

### Build Identity

- `core/CMakeLists.txt` uses the project identity `OpenHealthcare`.
- The native core library target is renamed from `fastsurfer_core` to `openhc_imaging_mri_fastsurfer`.
- `core/tests/CMakeLists.txt` links all native core tests against `openhc_imaging_mri_fastsurfer`.
- Active in-repo build files no longer link against `fastsurfer_core` for this module.

### Consumer And Documentation Updates

- Direct in-repo consumers, including `_app/desktop`, are updated to the new include paths and namespace.
- Narrow documentation references describing the active native-core API are updated so they no longer present `fastsurfer::core` or `fastsurfer_core` as current names.

### Behavioral Safety

- The refactor does not intentionally change MRI processing algorithms or expected output semantics.
- `core/` still builds successfully after the move.
- The focused `core/` test suite runs with no new failures attributable to namespace, path, or target migration.

## Scenarios

### Scenario 1: Public headers reflect the new API identity

**Given** the `core/` public include tree

**When** a developer inspects the active header layout after the refactor

**Then** the module headers should live under `imaging/mri/fastsurfer/`

**And** there should be no active public header tree under `fastsurfer/core/`

**And** there should be no added `OpenHC/` directory layer in the include path

### Scenario 2: Source and test directories mirror the module hierarchy

**Given** the `core/` implementation and test directories

**When** a developer inspects the migrated files

**Then** implementation sources should live under `core/src/imaging/mri/fastsurfer/`

**And** test sources and helpers should live under `core/tests/imaging/mri/fastsurfer/`

**And** the physical directory structure should mirror the module hierarchy without adding `OpenHC/`

### Scenario 3: The native core exposes the new namespace root

**Given** the migrated `core/` headers and source files

**When** the active declarations and definitions are inspected

**Then** the public namespace should be `OpenHC::imaging::mri::fastsurfer`

**And** the constants namespace should be `OpenHC::imaging::mri::fastsurfer::constants`

**And** active code should no longer use `fastsurfer::core`

### Scenario 4: Build metadata matches the namespace root

**Given** the `core/` CMake configuration

**When** the native core target is configured after the refactor

**Then** the project name should be `OpenHealthcare`

**And** the library target should be `openhc_imaging_mri_fastsurfer`

**And** the `core/` tests should link against `openhc_imaging_mri_fastsurfer`

**And** active in-repo build files should no longer reference `fastsurfer_core` for this module

### Scenario 5: Desktop and other in-repo consumers compile against the migrated API

**Given** an in-repo consumer such as `_app/desktop/src/app/application_controller.cpp`

**When** the repository is updated to the new public API

**Then** the consumer should include headers from `imaging/mri/fastsurfer/...`

**And** the consumer should use `OpenHC::imaging::mri::fastsurfer` symbols

**And** the consumer should build against the renamed native core target where applicable

### Scenario 6: The refactor remains behavior-preserving

**Given** the existing `core/` native processing and tests

**When** the namespace, paths, and target names are migrated

**Then** the implementation should preserve the existing processing behavior

**And** the focused `core/` build should succeed

**And** the focused `core/` test suite should not introduce new failures caused by the structural refactor itself

## Notes For Step 4 Approval

Implementation should proceed only if the user agrees that:

- `OpenHC` is the canonical root namespace.
- The physical directory tree starts at `imaging/...` and must not add `OpenHC/`.
- Renaming the library target from `fastsurfer_core` to `openhc_imaging_mri_fastsurfer` is part of the accepted scope.
- This change is intended to be structural and naming-focused, not algorithmic.