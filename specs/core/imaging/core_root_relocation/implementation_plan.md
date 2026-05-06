# Core Root Relocation Implementation Plan

## Feature

`core_root_relocation`

## Project Name

`Open Healthcare`

## Goal

Move the native processing module to `core/` at the repository root while keeping the existing module identity unchanged:

- C++ namespace remains `OpenHC::imaging::mri::fastsurfer`
- public header hierarchy remains `include/imaging/mri/fastsurfer`
- source hierarchy remains `src/imaging/mri/fastsurfer`
- test hierarchy remains `tests/imaging/mri/fastsurfer`
- primary library target remains `openhc_imaging_mri_fastsurfer`

At the same time, normalize the repository `specs/` tree so spec artifacts use repository-relative paths and stop referring to the obsolete `_app/specs` location. All spec path references should be coherent with the relocated `core/` folder.

## Current Repository Facts

- The active native module now lives at `core/` and contains `CMakeLists.txt`, `build_core.sh`, `include/`, `src/`, `tests/`, and a local `build/` tree.
- The module already exposes the approved namespace-aligned internal layout under `core/include/imaging/mri/fastsurfer`, `core/src/imaging/mri/fastsurfer`, and `core/tests/imaging/mri/fastsurfer`.
- The active native library target is already `openhc_imaging_mri_fastsurfer`.
- `_app/desktop/CMakeLists.txt` currently consumes the native module with `add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core ...)`, so moving `core/` out of `_app/` will break the desktop integration unless that wiring is updated.
- Some docs and spec artifacts still describe the module as `_app/core` even though the live module now resides in `core/`.
- The live spec tree is already rooted at `specs/`, not `_app/specs`, but several historical specs still mention `_app/specs` or stale predecessor paths for the relocated module.
- The repository root now contains the relocated tracked source folder `core/`.

## Scope

In scope:

- Keep the tracked native module directory at `core/`.
- Preserve the internal API, namespace, include hierarchy, source hierarchy, test hierarchy, and target name.
- Update in-repo consumers and docs that refer to the old `_app/core` source location.
- Update `specs/` artifacts so absolute filesystem paths become repository-relative paths.
- Update `specs/` artifacts that still refer to `_app/specs` so they point to `specs/`.
- Verify that spec narratives remain coherent with the current module naming and folder hierarchy after the move.

Out of scope:

- Renaming namespaces, public headers, targets, or test executable names.
- Reworking MRI processing logic or test behavior.
- Cleaning every generated build artifact in the repository unless it blocks validation.
- Rewriting historical prose beyond what is required to keep paths and structure coherent.

## Architecture Decision

### Chosen Approach

Perform the relocation as a source-root move, not as a module redesign:

- the native module root is `core/`
- the internal `include/`, `src/`, `tests/`, and `build_core.sh` surfaces move with it unchanged
- all consumers change only the path they use to reach the module
- all spec artifacts use repository-relative paths such as `core/CMakeLists.txt`, `core/include/imaging/mri/fastsurfer/...`, or `specs/core/imaging/...`

This preserves the existing module identity and avoids reopening the namespace or target rename work that was already completed.

### Why This Approach

- The user explicitly asked to keep naming and module hierarchy unchanged.
- The active breakage is path-based rather than API-based.
- Treating the move as a root relocation minimizes behavioral risk and keeps validation focused on build-path integrity.
- Converting specs to repository-relative references removes machine-specific path leakage and aligns with the repo policy that plans and tests must remain inside the workspace.

### Alternative Rejected

Leave `_app/core` in place and add a forwarding `core/` wrapper.

Why rejected:

- It creates two physical homes for one module.
- It increases long-term maintenance and ambiguity.
- It does not satisfy the requested structural simplification.

### Alternative Rejected

Move only source files and recreate `core/` from scratch.

Why rejected:

- It adds unnecessary churn.
- It increases the chance of losing local build scripts, tests, or supporting files during the transition.
- The module already has a coherent internal structure that should remain intact.

## Proposed Target Structure

```text
core/
  CMakeLists.txt
  build_core.sh
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
    CMakeLists.txt
    imaging/
      mri/
        fastsurfer/
          TestConstants.h
          TestHelpers.h
          test_conform_policy.cpp
          test_native_reconform_non_conformed.cpp
          test_nifti_converter.cpp
          test_nifti_converter_roundtrip.cpp
          test_python_parity_conform_step.cpp
          test_step_conform.cpp
```

## Primary Blast Radius

### Source And Build Files

- `_app/desktop/CMakeLists.txt` must stop referencing `../core` and point to the relocated module.
- `core/build_core.sh` must use path assumptions that match the relocated `core/` root.
- Root or desktop documentation that tells users to `cd _app/core` must be updated.
- Any validation commands or scripts that configure `_app/core` must be updated to configure `core/` instead.

### Specification Files

Two classes of spec drift need correction:

1. Stale repository-location references such as `_app/specs/...` or `_app/core/...`
2. Stale command examples that still assume the old module root under `_app/`

Spec normalization rules for this feature:

- Use repository-relative paths such as `core/CMakeLists.txt`, `core/tests/CMakeLists.txt`, `specs/core/imaging/core_namespace_refactor/implementation_plan.md`, or `data/parrec_oblique/NIFTI/...`
- Do not keep machine-specific absolute paths under `specs/`
- Update path prose only as far as needed to match the current module placement and naming
- Preserve historical intent and accepted scope where possible

## Implementation Sequence

1. Update source/build consumers that directly reference `_app/core`, starting with `_app/desktop/CMakeLists.txt` and `core/build_core.sh`.
2. Update repo docs and UI strings that describe the module location as `_app/core`.
3. Sweep `specs/` for stale `_app/specs` references and convert them to `specs/...`.
4. Sweep `specs/` for stale `_app/core` references and convert them to `core/...` where the reference is structural rather than historical context.
5. Re-read the touched spec artifacts to verify that the new wording remains coherent with the current target name `openhc_imaging_mri_fastsurfer`, namespace `OpenHC::imaging::mri::fastsurfer`, and the relocated folder hierarchy.
6. Reconfigure and rebuild the relocated `core/` module.
7. Run the focused native core test suite from the relocated build tree.

## Path Rewrite Policy For Specs

Apply these replacements where they preserve meaning:

- `_app/specs/...` -> `specs/...`
- `_app/core/...` -> `core/...` when the text refers to the current live module location rather than a quoted historical step
- command examples should prefer repository-relative paths such as `core/...`, `specs/...`, and `data/...`

Do not blindly replace:

- generated build outputs outside `specs/`
- code snippets whose semantics depend on the old path unless the snippet is being updated as an active command example
- historical narrative that explicitly contrasts an older state from a newer one

## Risks And Mitigations

### Risk: Desktop build wiring breaks after the folder move

Mitigation:

- Update `_app/desktop/CMakeLists.txt` in the same change set as the directory move.
- Prefer root-anchored or repo-relative subdirectory wiring over assumptions that `core/` is a sibling of `desktop/` under `_app/`.

### Risk: Validation commands in specs become inconsistent

Mitigation:

- Normalize all command examples in `specs/` to repository-relative paths during the same sweep.
- Recheck focused testing summaries and implementation plans that currently embed absolute build commands.

### Risk: Generated build artifacts produce noisy stale `_app/core` matches

Mitigation:

- Treat tracked source, docs, and `specs/` as the authoritative surfaces.
- Only clean or regenerate build outputs if they block the relocated `core/` validation.

### Risk: Some spec references are intentionally historical rather than normative

Mitigation:

- Preserve historical statements unless they are presenting the old path as the current truth.
- Update acceptance criteria, commands, file inventories, and implementation guidance first.

## Validation Plan

Primary validation:

- Configure the relocated module with `cmake -S core -B core/build -DOPENHC_IMAGING_MRI_FASTSURFER_BUILD_TESTS=ON`
- Build with `cmake --build core/build`
- Run `ctest --output-on-failure` from `core/build`

Secondary validation:

- Search the repository for active `_app/core` references in tracked source/docs/spec files after the relocation.
- Verify no touched spec still points to `_app/specs` as the live artifact root.

## Success Criteria

- The tracked native module now lives at `core/`.
- The internal module hierarchy and naming remain unchanged under `core/`.
- The desktop consumer and other active tracked files no longer depend on the `_app/core` location.
- `specs/` artifacts no longer contain machine-specific absolute workspace paths.
- `specs/` artifacts no longer describe `_app/specs` as the live spec root.
- Touched spec artifacts are coherent with the current namespace, target name, and relocated `core/` folder.
- The relocated `core/` module builds successfully and its focused test suite passes without new failures attributable to the move.