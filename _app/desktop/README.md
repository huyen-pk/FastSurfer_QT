# FastSurfer Qt Desktop

This directory contains a standalone Qt Quick/QML desktop application implemented in C++.

## Scope

The application implements the UX shell and four primary screens described in `_app/ux`:

- Dashboard
- Scan Viewer
- Pipelines
- Analytics

The current build remains mostly presentational, but it now includes one real native pipeline step backed by `_app/core`:

- `pipeline_conform_and_save_orig`

That step exposes a desktop UI on the Pipelines page and executes a native MGZ processing path for the approved Subject140 parity fixture.

## Prerequisites

- CMake 3.21 or newer
- Qt 6 with `Core`, `Gui`, `Qml`, `Quick`, and `QuickControls2`
- A C++17 compiler

Optional:

- `Inter` and `Manrope` installed system-wide for closer visual alignment with the UX references. The UI falls back to the platform default sans-serif fonts if these are not available.

Additional runtime/build dependency for the native conform step:

- zlib development libraries, which are typically available by default on Linux distributions used for Qt/C++ development

## Build

```bash
cd _app/desktop
cmake -S . -B build
cmake --build build
```

The desktop build pulls `_app/core` in as a native library dependency.

## Run

```bash
cd _app/desktop
./build/fastsurfer_qt_desktop
```

For headless verification:

```bash
cd _app/desktop
./build/fastsurfer_qt_desktop -platform offscreen
```

To build and run the native parity tests directly:

```bash
cd _app/core
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Structure

- `src/main.cpp`: C++ entry point and QML engine bootstrap
- `src/app/application_controller.*`: representative application data and native conform-step bridge exposed to QML
- `src/app/navigation_state.*`: current screen state and navigation API
- `qml/qtquickdesktop/components/PipelineConformStepCard.qml`: desktop UI for the native `pipeline_conform_and_save_orig` step
- `qml/qtquickdesktop`: reusable shell, theme, and page composition

## Current Native Step Scope

The first native implementation in `_app/core` is intentionally narrow:

- it targets MGZ inputs,
- it is verified against `data/Subject140/140_orig.mgz`,
- it supports the already-conformed MGZ path required for the approved parity fixture,
- Python remains the parity oracle in tests only.