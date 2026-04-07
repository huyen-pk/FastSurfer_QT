# Qt Quick Desktop App Implementation Plan

## Feature

`qt_quick_desktop_app`

## Goal

Create a new Qt Quick/QML desktop application in C++ under `_app/desktop` that implements the UX direction defined in `_app/ux`, with a reusable application shell and four primary screens:

- Dashboard
- Scan Viewer
- Processing Pipelines
- Results Analytics

## Current Repository Facts

- `_app/desktop` is empty.
- `_app/cmake`, `_app/core`, `_app/rendering`, and `_app/scripts` are currently empty placeholders.
- `_app/specs` is empty, so this feature establishes the first spec artifact there.
- `_app/ux` contains the target visual references as HTML mockups and screenshots.
- `_app/ux/aetheris_medical/DESIGN.md` defines the visual system: dark tonal layering, Manrope + Inter typography, cyan accent usage, and a no-divider surface hierarchy.
- The repository does not currently contain a C++ or CMake desktop build for `_app`.

## Scope

This feature delivers a presentational desktop shell and screen composition layer, not pipeline execution or MRI processing integration. The initial implementation should prioritize:

- A compilable Qt 6 C++ application.
- A reusable QML design system based on the UX references.
- Navigation between the four target screens.
- Representative data models exposed from C++ to QML so the screens render meaningful layout states without introducing non-existent backend integrations.

Out of scope for this feature:

- FastSurfer job execution.
- Real MRI volume loading from repository datasets.
- Python/C++ process orchestration.
- Persistence, authentication, or network services.

## Proposed Architecture

### 1. Build and Project Bootstrap

Create a standalone CMake-based Qt 6 application rooted in `_app/desktop`.

Proposed structure:

```text
_app/desktop/
  CMakeLists.txt
  src/
    main.cpp
    app/
      application_controller.h
      application_controller.cpp
      navigation_state.h
      navigation_state.cpp
    models/
      dashboard_metrics_model.h
      dashboard_metrics_model.cpp
      studies_model.h
      studies_model.cpp
      pipeline_steps_model.h
      pipeline_steps_model.cpp
      analytics_metrics_model.h
      analytics_metrics_model.cpp
  qml/
    qtquickdesktop/
      Main.qml
      Theme.qml
      components/
      pages/
  resources/
    qtquickdesktop.qrc
```

Build requirements:

- `find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)`
- C++20 or C++17, depending on the Qt version available locally.
- `qt_add_executable` and `qt_add_qml_module` for packaging QML cleanly.

### 2. Application Composition

Use a small C++ composition root and keep visual logic in QML.

Responsibilities:

- `main.cpp`: initialize `QGuiApplication`, configure `QQmlApplicationEngine`, and register C++ types.
- `ApplicationController`: expose top-level app metadata and wire view models/models.
- `NavigationState`: own current section selection and provide a typed bridge between C++ and QML.

### 3. QML Design System

Create a local QML theme layer derived from `_app/ux/aetheris_medical/DESIGN.md`.

Theme responsibilities:

- Centralize surface colors and accent palette.
- Centralize spacing and corner radius tokens.
- Centralize typography roles that map to Manrope-like display styling and Inter-like utility styling.
- Enforce the “no-line” rule by favoring tonal surface changes over visible borders.

The QML layer should avoid external CDN assets. Any imagery in the mockups should be replaced with local placeholders, gradients, or neutral graphics so the app remains self-contained.

### 4. Shared UI Shell

Implement a reusable desktop shell that is persistent across screens.

Shared shell components:

- `TopBar`: product title, search field, action icons, profile affordance.
- `SideNav`: navigation entries for Dashboard, Scan Viewer, Pipelines, Analytics.
- `PrimaryActionButton`: “New Processing Task” CTA.
- `SurfacePanel`: reusable tonal card/panel primitive.
- `SectionHeader`: headline + supporting text layout.

Navigation behavior:

- Single-window application.
- Left navigation switches the main content area.
- Initial route should open Dashboard.

### 5. Screen Mapping From `_app/ux`

#### Dashboard

Map `_app/ux/dashboard` into:

- A hero header.
- Three key metric cards.
- Recent studies table.
- Right-hand operational/status panels.

Representative C++ models:

- `DashboardMetricsModel`
- `StudiesModel`

#### Scan Viewer

Map `_app/ux/simplified_scan_viewer` into:

- Viewer toolbar.
- Four-panel imaging layout.
- Right metadata sidebar.

Initial implementation guidance:

- Use neutral imaging placeholders rendered with gradients, overlays, and coordinate labels.
- Keep the layout ready for future replacement with real image providers or scene graph items.

Representative C++ models:

- `StudyMetadataModel`
- `ViewerLayoutState`

#### Processing Pipelines

Map `_app/ux/simplified_pipeline_config` into:

- Pipeline header and action bar.
- Vertical flow of processing nodes.
- Status tags and execution affordances.

Representative C++ models:

- `PipelineStepsModel`

#### Results Analytics

Map `_app/ux/results_analytics` into:

- Comparative analysis header.
- Primary chart card.
- Supporting metric panels.
- Secondary comparative summaries.

Initial implementation guidance:

- Render charts with Qt Quick primitives such as `Canvas`, `Repeater`, and rectangles.
- Avoid introducing chart dependencies unless necessary.

Representative C++ models:

- `AnalyticsMetricsModel`
- `ComparisonSeriesModel`

## Implementation Sequence

1. Bootstrap `_app/desktop` CMake project and compile a minimal Qt Quick window.
2. Add theme tokens and shared shell layout.
3. Implement navigation state and page routing.
4. Build Dashboard page and models.
5. Build Scan Viewer page and placeholder rendering panels.
6. Build Processing Pipelines page and models.
7. Build Results Analytics page and chart primitives.
8. Refine spacing, typography, and color hierarchy to better match `_app/ux`.
9. Document build/run steps in `_app/desktop/README.md`.

## Risks and Mitigations

### Risk: No Existing Native Desktop Build Infrastructure

Mitigation:

- Keep `_app/desktop` fully self-contained.
- Avoid introducing cross-directory dependencies in the first iteration.

### Risk: UX References Depend on Web Assets and Remote Images

Mitigation:

- Translate the visual hierarchy, not the HTML literally.
- Replace remote imagery with local or programmatic placeholders.

### Risk: User Requested C++/Qt While Repository Already Uses Python Qt Bindings Elsewhere

Mitigation:

- Keep this app independent from PySide usage.
- Use standard Qt 6 CMake conventions and avoid coupling to Python packaging.

## Acceptance Direction For Step 2 Approval

The implementation phase should begin only if the user approves the following:

- `_app/desktop` will be a new standalone Qt 6 CMake project.
- The first iteration is a polished UX shell with representative C++ data models, not a full FastSurfer execution client.
- The four UX references in `_app/ux` will be implemented as separate navigable screens within one application shell.
- Remote web assets in the mockups will be translated into self-contained native Qt visuals.

## Expected Outputs In Later Workflow Steps

- Step 3: `_app/specs/qt_quick_desktop_app/bdd_requirements.md`
- Step 5: Qt Quick/QML C++ implementation under `_app/desktop`
- Step 6: verification artifacts for build and runtime checks