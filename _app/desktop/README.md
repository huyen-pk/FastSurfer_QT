# FastSurfer Qt Desktop

This directory contains a standalone Qt Quick/QML desktop prototype implemented in C++.

## Scope

The application implements the UX shell and four primary screens described in `_app/ux`:

- Dashboard
- Scan Viewer
- Pipelines
- Analytics

The current build is intentionally presentational. It uses representative C++ data exposed to QML and does not execute FastSurfer pipelines or load MRI volumes from disk.

## Prerequisites

- CMake 3.21 or newer
- Qt 6 with `Core`, `Gui`, `Qml`, `Quick`, and `QuickControls2`
- A C++17 compiler

Optional:

- `Inter` and `Manrope` installed system-wide for closer visual alignment with the UX references. The UI falls back to the platform default sans-serif fonts if these are not available.

## Build

```bash
cd _app/desktop
cmake -S . -B build
cmake --build build
```

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

## Structure

- `src/main.cpp`: C++ entry point and QML engine bootstrap
- `src/app/application_controller.*`: representative application data exposed to QML
- `src/app/navigation_state.*`: current screen state and navigation API
- `qml/qtquickdesktop`: reusable shell, theme, and page composition