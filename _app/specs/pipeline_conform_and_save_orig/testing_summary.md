# Testing Summary: Pipeline Conform And Save Orig

## Scope

Verification for the native `_app/core` MGZ conform step and its desktop integration in `_app/desktop`.

## What Was Verified

### 1. Source Diagnostics

- VS Code diagnostics for the touched `_app/core` and `_app/desktop` paths reported no editor-detected errors after implementation.

### 2. Native Core Configure And Build

Commands run:

```bash
cd _app/core
cmake -S . -B build
cmake --build build -j4
```

Result:

- configure completed successfully,
- the static library `libfastsurfer_core.a` was produced,
- the test executables were produced in `_app/core/build/tests`.

### 3. Native Core Test Execution

Commands run:

```bash
_app/core/build/tests/test_step_conform
_app/core/build/tests/test_python_parity_conform_step
```

Result:

- the native MGZ test executable completed without error,
- the Python parity executable completed without error,
- the native and Python outputs matched for the approved fixture `data/Subject140/140_orig.mgz`.

### 4. Desktop Configure And Build

Commands run:

```bash
cd _app/desktop
cmake -S . -B build
cmake --build build -j4
```

Result:

- the desktop target rebuilt successfully,
- `_app/desktop` linked against the new `_app/core` library,
- the output binary `build/fastsurfer_qt_desktop` was produced.

### 5. Desktop Headless Startup Check

Command run:

```bash
cd _app/desktop/build
./fastsurfer_qt_desktop -platform offscreen
```

Result:

- the application launched in offscreen mode without surfacing QML or startup errors in the captured log,
- the new pipelines-page component did not break application startup.

## Notes

- The parity test invokes `python3` directly as the reference runner because the environment-discovery tool did not resolve a repository Python environment in this workspace.
- The first `_app/core` implementation is intentionally limited to the already-conformed MGZ path required by the approved Subject140 fixture.

## Status

- Implementation: complete
- Core build verification: pass
- Core native test: pass
- Core Python parity test: pass
- Desktop build verification: pass
- Desktop headless startup verification: pass