# Pipeline Conform And Save Orig Implementation Plan

## Feature

`pipeline_conform_and_save_orig`

## User-Directed Constraints

- Implement directly on the current branch. The workflow's normal worktree step is intentionally waived for this task by explicit user instruction.
- Use `_app/core` for image-processing code and parity tests.
- Use `_app/desktop` for QML UI and desktop-facing orchestration only.

## Goal

Add a desktop-visible pipeline step that matches the behavior of FastSurfer's `RunModelOnData.pipeline_conform_and_save_orig()` and its delegated `conform_and_save_orig()` stage from `FastSurferCNN/run_prediction.py`.

The new feature must:

- expose the step in the Qt Quick desktop app,
- execute the step through native C++ code in `_app/core`, and
- include a parity test that compares the C++ result against the original Python implementation.

## Current Repository Facts

- The Python source of truth is `FastSurferCNN/run_prediction.py`.
- `pipeline_conform_and_save_orig()` is a thin iterator/pipeline wrapper around `conform_and_save_orig()`.
- `conform_and_save_orig()` performs four behaviors that matter for parity:
  - load the input image,
  - optionally copy the original image to `copy_orig_name`,
  - check conformance and conform the image if needed,
  - save the conformed image to `conf_name`.
- The detailed conformance logic is currently implemented in Python in `FastSurferCNN/data_loader/conform.py`.
- `_app/core` is currently empty.
- `_app/desktop` is currently a static Qt Quick prototype with hardcoded `QVariantMap` data exposed by `ApplicationController`.
- `_app/desktop` currently has no backend action layer and no native test harness.
- The repository already contains a real MGZ fixture at `test/data/Subject140/140_orig.mgz`, which can be used for parity verification without introducing mocks.

## Scope For This Feature

This feature delivers a first working native implementation centered on the repository's available real test fixture and desktop prototype.

In scope:

- a reusable C++ core service for the conform-and-save-original step,
- desktop UI to configure and run that step,
- parity verification against the existing Python implementation,
- build and test wiring for the new core code.

Out of scope for this feature:

- full FastSurfer segmentation or downstream inference,
- replacing all Python conform functionality across the repo,
- asynchronous multi-subject batching equivalent to Python's full `pipeline()` helper,
- production-grade DICOM import or volume rendering.

## Format Support Decision

The first implementation should target **MGZ input and output parity** using the repository fixture `test/data/Subject140/140_orig.mgz`.

Reasoning:

- the user explicitly selected the MGZ fixture as the parity target,
- `conform_and_save_orig()` commonly writes `orig.mgz` in the FastSurfer flow, so MGZ is directly aligned with the original behavior,
- the runtime requirement remains native C++ in `_app/core`, with Python used only as the parity oracle.

This means MGZ/FreeSurfer-style image I/O is part of the first implementation scope rather than a later extension.

## Proposed Architecture

### 1. `_app/core` Native Processing Layer

Create a small C++ processing library under `_app/core` that owns the step behavior.

Proposed structure:

```text
_app/core/
  CMakeLists.txt
  include/
    fastsurfer/core/conform_step_request.h
    fastsurfer/core/conform_step_result.h
    fastsurfer/core/conform_step_service.h
    fastsurfer/core/image_metadata.h
  src/
    conform_step_service.cpp
    image_metadata.cpp
  tests/
    CMakeLists.txt
    test_conform_step_service.cpp
    test_python_parity_conform_step.cpp
```

Primary types:

- `ConformStepRequest`: input path, optional copy-orig path, conformed-output path, conformance options.
- `ConformStepResult`: resolved output paths, whether input was already conformed, image metadata, success/error state.
- `ConformStepService`: synchronous native implementation of the step.

### 2. Native Conform Behavior

`ConformStepService` should reproduce the same observable behavior as the Python step for the targeted MGZ flow:

1. Load the source image.
2. If `copyOrigPath` is set, save an unchanged copy of the input image.
3. Determine whether the image is already conformed according to the configured criteria.
4. If not conformed, create the conformed image using native C++ image processing.
5. Save the final conformed image.
6. Return metadata needed by the desktop UI and the parity tests.

### 3. Desktop Integration In `_app/desktop`

Add desktop orchestration without moving image-processing logic into the UI layer.

Proposed additions:

- `ApplicationController` gets a new pipeline-facing property and `Q_INVOKABLE` action(s) to trigger the core service.
- Add a dedicated reusable QML component, for example `PipelineConformStepCard.qml`, that visually represents the `pipeline_conform_and_save_orig` stage.
- Add a focused page section on the existing pipelines screen to:
  - show the step title and Python-equivalent purpose,
  - collect input and output paths,
  - display execution state and result metadata,
  - surface success and failure states.

Desktop responsibilities:

- gather user input,
- invoke the core service,
- render results.

Core responsibilities:

- load, inspect, conform, and save images.

### 4. Controller Boundary

Keep the controller boundary simple and explicit.

Recommended addition in `_app/desktop/src/app`:

- `PipelineConformViewModel`-style data represented either by a small QObject or by controller-owned `QVariantMap`/`QVariantList` properties.

The desktop app does not need a fully generic task system for this feature. A narrowly scoped action and state object is enough.

## Dependency Strategy

The implementation should use a native C++ imaging library from `_app/core` rather than wrapping the Python implementation.

Because `_app/core` is currently empty, the coding step should introduce the required native dependency and CMake wiring there.

The approval decision for implementation is therefore:

- proceed with an **MGZ-first native C++ conform implementation** in `_app/core`,
- keep Python only as the **parity oracle** in tests.

## Parity Test Strategy

Parity tests belong in `_app/core/tests`.

They should use real files and the real Python implementation from this repository.

### Test Inputs

- Input fixture: `test/data/Subject140/140_orig.mgz`
- Temporary output directories created by the test runner

### Test Oracles

- Native C++ result from `_app/core`
- Python result produced by calling the repository implementation from `FastSurferCNN/run_prediction.py`

### Assertions

The parity test should compare:

- existence of copied original output when requested,
- existence of conformed output,
- output image shape,
- voxel spacing,
- affine or equivalent orientation metadata,
- voxel data equality or tolerance-based equivalence where interpolation requires numeric tolerance,
- whether the step reports "already conformed" versus "conformed during execution" consistently for the tested fixture.

### Test Naming

All new tests should follow the repository convention requested by the tester role, for example:

- `run_conform_step_should_write_copy_and_conformed_outputs`
- `run_conform_step_should_match_python_reference_on_subject140_mgz_fixture`

## Verification Plan

Implementation will be considered complete only after:

- the desktop target still configures and builds,
- `_app/core` tests build and run successfully,
- the parity test passes against the Python reference implementation,
- relevant diagnostics for touched files are clean.

Repository-wide reviewer gates remain separate from this plan and will be run after implementation where applicable.

## Risks And Mitigations

### Risk: Native conform logic may diverge from Python behavior

Mitigation:

- keep the acceptance target narrow and test directly against the Python implementation,
- compare both metadata and voxel arrays,
- start with a single real fixture before broadening coverage.

### Risk: Native MGZ imaging dependency may require additional system packages or extra format handling

Mitigation:

- isolate the dependency in `_app/core`,
- document the requirement in `_app/core` or `_app/desktop` build documentation,
- keep desktop code independent from the imaging implementation details,
- verify parity directly on the repository MGZ fixture before broadening coverage.

### Risk: `_app/desktop` has no action/test infrastructure today

Mitigation:

- add only the minimal controller/QML bridge required for this step,
- keep most behavioral verification in `_app/core` parity tests.

## Step 2 Approval Request

Implementation should proceed only if the following are approved:

- `_app/core` becomes the native home for the conform-and-save-original processing step.
- `_app/desktop` only provides the QML component, interaction flow, and result display.
- The first implementation targets **real MGZ parity** using `test/data/Subject140/140_orig.mgz`.
- Python remains the parity oracle in tests, not the runtime implementation.
- The no-worktree exception is accepted for this task because it was explicitly requested by the user.