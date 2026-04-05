# FastSurfer Qt Desktop Prototype

This directory contains a standalone Qt 6 Widgets application that recreates the approved UX direction from `_app/ux` as a native C++ desktop shell.

## Scope
- Native C++ Qt Widgets application
- Four primary views: Dashboard, Scan Viewer, Pipelines, and Analytics
- Local placeholder content only for milestone one
- No FastSurfer execution or MRI processing integration yet

## Build

```bash
cmake -S _app/desktop -B _app/desktop/build
cmake --build _app/desktop/build
```

## Run

```bash
./_app/desktop/build/fastsurfer_desktop
```
