# Core Native Documentation Pass Implementation Plan

## Feature

`core_native_documentation_pass`

## Goal

Add comprehensive developer-facing documentation to the native C++ core module so public APIs, internal helper functions, and important state-carrying structures are understandable without reverse-engineering the implementation.

For this pass, "core native module" means the current C++ code under `core/include/` and `core/src/`.

## User-Approved Scope For This Plan

The user selected **Core native module only** rather than the entire workspace or vendored baseline tree.

## Current Repository Facts

- The current native implementation surface is concentrated in the FastSurfer MRI module under `core/include/imaging/mri/fastsurfer/` and `core/src/imaging/mri/fastsurfer/`.
- The current file inventory in scope is:
  - `core/include/imaging/mri/fastsurfer/constants.h`
  - `core/include/imaging/mri/fastsurfer/conform_policy.h`
  - `core/include/imaging/mri/fastsurfer/image_metadata.h`
  - `core/include/imaging/mri/fastsurfer/mgh_image.h`
  - `core/include/imaging/mri/fastsurfer/nifti_converter.h`
  - `core/include/imaging/mri/fastsurfer/step_conform.h`
  - `core/include/imaging/mri/fastsurfer/step_conform_request.h`
  - `core/include/imaging/mri/fastsurfer/step_conform_result.h`
  - `core/src/imaging/mri/fastsurfer/conform_policy.cpp`
  - `core/src/imaging/mri/fastsurfer/mgh_image.cpp`
  - `core/src/imaging/mri/fastsurfer/nifti_converter.cpp`
  - `core/src/imaging/mri/fastsurfer/step_conform.cpp`
- The module currently has little to no API-level documentation or explanatory comments.
- The repository workflow requires an approved implementation plan before the coding step proceeds.

## Documentation Objectives

This pass should document three layers consistently:

1. Public API intent
   - document exported types, enums, structs, classes, and functions in public headers,
   - explain parameters, return values, and invariants where the signature alone is insufficient.

2. Internal implementation intent
   - document non-obvious helper functions in `.cpp` files,
   - explain algorithm choices, data-format assumptions, and parity-sensitive math.

3. Important state and constants
   - document fields and variables when they carry domain-specific meaning that is not obvious from the identifier alone,
   - avoid line-by-line commentary on trivial variables or self-evident assignments.

## Non-Goals

Out of scope for this documentation pass:

- changing runtime behavior,
- renaming APIs solely for readability,
- reformatting unrelated code,
- documenting the vendored Python baseline under `baseline/`,
- forcing comments onto every local variable where the name is already clear.

## Documentation Standard For This Pass

The pass should use concise, maintainable comments rather than verbose prose.

Rules:

- Public headers receive API comments for exported declarations.
- Struct fields receive comments only when the field meaning is domain-specific or encoded by file-format semantics.
- Implementation files receive comments only for logic that is hard to infer from the code itself.
- Avoid repeating the function name in the comment without adding meaning.
- Prefer documenting invariants, units, ownership, orientation conventions, file-format assumptions, and parity-sensitive rounding behavior.

## Proposed Execution Order

### 1. Public Header Pass

Document the public surface first so downstream readers understand the exposed contract:

- `mgh_image.h`
- `image_metadata.h`
- `step_conform_request.h`
- `step_conform_result.h`
- `step_conform.h`
- `nifti_converter.h`
- `constants.h`
- `conform_policy.h`

### 2. Source Commentary Pass

Add focused explanatory comments in the implementation files where algorithms or binary-format behavior are not self-evident:

- `mgh_image.cpp`
- `nifti_converter.cpp`
- `step_conform.cpp`
- `conform_policy.cpp`

### 3. Verification Pass

- run editor diagnostics on all touched files,
- if a narrow native-core build or test is needed to catch comment-related breakage, run the smallest relevant validation step,
- keep the pass documentation-only with no semantic code drift.

## Risks And Mitigations

### Risk: Over-documenting trivial code makes the module harder to read

Mitigation:

- comment only exported contracts and non-obvious implementation logic,
- skip trivial local variables and obvious getters/setters.

### Risk: Comments become incorrect if they overstate behavior

Mitigation:

- derive comments directly from current code behavior,
- keep wording factual and local to the implementation.

### Risk: "Comprehensive" is interpreted as documenting every local variable

Mitigation:

- treat comprehensive documentation as full API coverage plus focused explanation of domain-specific internal logic,
- avoid mechanical comments that do not add information.

## Acceptance Criteria

This pass is complete when:

- every public type and exported function in the native core FastSurfer module has developer-facing documentation,
- domain-specific struct fields and enum semantics are documented where needed,
- each implementation file contains comments for the non-obvious algorithms and file-format logic it owns,
- touched files remain free of editor-reported errors.

## Step 2 Approval Request

Implementation should proceed only if the following are approved:

- the pass is limited to the native C++ core module under `core/include` and `core/src`,
- "comprehensive documentation" means full public API coverage plus targeted explanation of non-obvious internal logic, not comments on every trivial local variable,
- the initial file set is the current FastSurfer MRI native module inventory listed above.