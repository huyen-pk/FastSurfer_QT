# BDD Requirements: Pipeline Conform And Save Orig

## Feature

Desktop-accessible native implementation of FastSurfer's `pipeline_conform_and_save_orig` step, split between `_app/core` for processing and `_app/desktop` for UI.

## Behavioral Intent

The desktop app should expose a dedicated pipeline step that lets a user run the same high-level behavior as FastSurfer's Python `conform_and_save_orig()` stage on a real MRI volume.

The implementation must be native C++ for the runtime path and must be verified against the existing Python implementation through parity tests.

## Acceptance Criteria

### Core Processing

- `_app/core` contains the native image-processing implementation for this step.
- The core implementation accepts an input image path, an optional copy-orig path, and a conformed-output path.
- When a copy-orig path is provided, the original input image is written there unchanged.
- When the input is already conformed for the configured criteria, the step still writes the expected conformed output path without applying unnecessary reconformation.
- When the input is not conformed, the step writes a conformed output image.
- The step returns structured status information that the desktop layer can display.

### Desktop UI

- `_app/desktop` contains a new QML component representing the `pipeline_conform_and_save_orig` stage.
- The component is visible from the desktop pipelines flow.
- The user can provide the input path and output path information needed to run the step.
- The user can trigger execution from the desktop app.
- The desktop app displays whether the run succeeded or failed.
- The desktop app displays relevant result details, such as output paths and whether conforming was required.

### Parity Verification

- `_app/core` contains parity tests for the native implementation.
- The parity tests use real repository data and the real repository Python implementation.
- The parity tests compare the native output with the Python output for the same input fixture.
- The parity comparison covers output file creation, shape, spacing, and voxel-content equivalence within a justified tolerance.

### Repository Constraints

- The runtime implementation uses native C++ code and libraries for processing.
- The parity tests do not use mocks or fake imaging services.
- The first implementation focuses on MGZ parity using the repository fixture `data/Subject140/140_orig.mgz`.

## Scenarios

### Scenario 1: User sees the conform step in the desktop pipeline view

**Given** the desktop application is running

**When** the user opens the pipelines page

**Then** the user should see a dedicated component for the `pipeline_conform_and_save_orig` step

**And** the component should describe the step as the stage that loads, optionally copies, conforms, and saves the original MRI input

### Scenario 2: User runs the step with valid paths

**Given** the user is viewing the conform step component

**And** the user has provided a valid input MRI path

**And** the user has provided a valid conformed output path

**When** the user starts the step

**Then** the desktop app should invoke the native `_app/core` processing service

**And** the desktop app should display a successful completion state when the step finishes

### Scenario 3: Step writes the original copy when requested

**Given** a valid input MRI path and a valid copy-orig output path

**When** the native conform step runs

**Then** the original image should be copied to the requested copy-orig output path

**And** the copied image should preserve the original image content and metadata relevant to file identity

### Scenario 4: Step writes a conformed image for non-conformed input

**Given** an input MRI that is not already conformed for the configured criteria

**When** the native conform step runs

**Then** a conformed output image should be written

**And** the step result should indicate that conforming was performed

### Scenario 5: Step preserves the already-conformed case correctly

**Given** an input MRI that is already conformed for the configured criteria

**When** the native conform step runs

**Then** the conformed output image should still be written to the requested destination

**And** the step result should indicate that additional conforming was not required

### Scenario 6: Native output matches the Python reference on the repository MGZ fixture

**Given** the repository fixture `data/Subject140/140_orig.mgz`

**When** the native `_app/core` conform step runs on that fixture

**And** the original Python `conform_and_save_orig()` behavior is run on the same fixture

**Then** both implementations should create the expected output files

**And** the conformed outputs should match in shape and spacing

**And** the conformed voxel data should match within the accepted tolerance

### Scenario 7: Desktop shows failures clearly

**Given** the user provides an invalid path or processing fails

**When** the desktop app runs the step

**Then** the desktop app should show a failure state

**And** the user should see an actionable error message rather than a silent failure

## Notes For Step 4 Approval

The implementation step should proceed only if the user agrees that:

- MGZ parity on the repository fixture is the acceptance target for this first iteration.
- `_app/core` owns the real processing and test logic.
- `_app/desktop` owns only the UI and controller bridge.