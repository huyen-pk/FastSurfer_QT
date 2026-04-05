# Qt Desktop Application Implementation Plan

## Feature
Create a C++ Qt desktop application inside `_app/desktop` that implements the UX direction and screen flows defined in `_app/ux`.

## Workflow Status
- Current stage: Step 1, Architect planning
- Required next step: user approval of this plan before BDD requirements or code implementation
- Git isolation note: `.worktrees/` is already ignored in the repository root `.gitignore`, so the repository is prepared for workflow-compliant worktree usage if a dedicated feature worktree is created

## Goals
- Establish a native Qt Widgets application in `_app/desktop`
- Recreate the UX system from `_app/ux/aetheris_medical/DESIGN.md`
- Implement the four primary screens defined by the HTML mockups:
  - Dashboard
  - Scan Viewer
  - Pipelines
  - Results Analytics
- Build the application so the UX can be previewed and iterated locally without requiring the Python inference backend on day one
- Leave clean extension points for later FastSurfer integration

## Non-Goals
- Do not integrate FastSurfer execution, ONNX inference, or MRI loading in the first implementation pass
- Do not add mock service layers or fake backends presented as production integrations
- Do not introduce Qt Quick/QML unless the user explicitly changes direction

## Source UX Inputs
- `_app/ux/aetheris_medical/DESIGN.md`
- `_app/ux/dashboard/code.html`
- `_app/ux/simplified_pipeline_config/code.html`
- `_app/ux/simplified_scan_viewer/code.html`
- `_app/ux/results_analytics/code.html`

## Recommended Technical Direction

### Framework Choice
Use Qt 6 with C++ and Qt Widgets.

Reasoning:
- The repository has no existing Qt or C++ stack, so Widgets is the most direct path to a desktop executable without introducing QML runtime structure
- The supplied UX is panel-heavy and dashboard-oriented, which maps cleanly to custom-styled QWidget layouts, QStackedWidget views, QTableView, and QSplitter-based shells
- The initial milestone is a faithful desktop shell, not animated scene composition

### Build System
Create a new CMake-based Qt application under `_app/desktop`.

Recommended root files:
- `_app/desktop/CMakeLists.txt`
- `_app/desktop/src/...`
- `_app/desktop/include/...`
- `_app/desktop/resources/...`

### Application Shell
Create a `QMainWindow`-based shell with:
- top application bar
- persistent left navigation rail
- central `QStackedWidget` containing the four primary views
- shared theme and reusable styled components

## Proposed Architecture

### Directory Layout
Recommended initial structure:

```text
_app/desktop/
  CMakeLists.txt
  README.md
  resources/
    icons/
    fonts/
    app.qrc
  include/
    app/
      MainWindow.h
      Theme.h
      NavigationRail.h
      TopBar.h
    screens/
      DashboardPage.h
      ScanViewerPage.h
      PipelinesPage.h
      AnalyticsPage.h
    widgets/
      MetricCard.h
      PanelCard.h
      SectionHeader.h
      TagChip.h
      ViewerPane.h
      DataTable.h
  src/
    main.cpp
    app/
      MainWindow.cpp
      Theme.cpp
      NavigationRail.cpp
      TopBar.cpp
    screens/
      DashboardPage.cpp
      ScanViewerPage.cpp
      PipelinesPage.cpp
      AnalyticsPage.cpp
    widgets/
      MetricCard.cpp
      PanelCard.cpp
      SectionHeader.cpp
      TagChip.cpp
      ViewerPane.cpp
      DataTable.cpp
```

### Core Layers
- App shell layer: main window, navigation, route switching, global theme
- Screen layer: one QWidget per UX screen
- Reusable widget layer: cards, chips, toolbars, viewer panes, metrics, table containers
- Resource layer: colors, icons, fonts, style sheets, placeholder imagery

### Theme Strategy
Implement a centralized theme module derived from the UX design system.

Theme responsibilities:
- define the “Luminescent Lab” color palette
- expose spacing, corner radius, and typography constants
- apply application-wide stylesheet rules
- provide helper APIs for semantic colors such as `surfaceContainerHigh`, `tertiary`, `onSurface`, and `outlineVariant`

### Navigation Model
Use a simple enum-based page router:
- Dashboard
- Scan Viewer
- Pipelines
- Analytics

Initial navigation behavior:
- left rail click changes the active page in `QStackedWidget`
- top bar remains fixed across all views
- action buttons are page-specific but visually consistent

## Screen Mapping

### Dashboard
Implement:
- overview header
- three metric cards
- recent studies table
- right-side status panels or summary cards

Qt mapping:
- `QGridLayout` for the metric and content grid
- `QTableView` or `QTableWidget` for recent studies
- reusable metric card widgets with accent icon zones

### Pipelines
Implement:
- sequence header with actions
- vertically stacked pipeline nodes
- connector lines between nodes
- call-to-action area for adding or executing pipeline steps

Qt mapping:
- `QScrollArea` containing a vertical layout of custom node widgets
- node widgets draw accent borders and connector lines using `paintEvent`

### Scan Viewer
Implement:
- secondary toolbar
- 2x2 viewer grid
- metadata sidebar
- viewer control strip

Qt mapping:
- `QSplitter` for viewer canvas and metadata sidebar
- `QGridLayout` for the 2x2 panel arrangement
- placeholder MRI images loaded from local resources for the first milestone
- custom viewer pane widget to support future image/volume integration

### Results Analytics
Implement:
- header and export action
- analytics cards
- comparative chart area
- cohort summary panels

Qt mapping:
- custom painted chart widgets for static bar and line visualizations in milestone one
- layout-driven statistic panels for supporting summaries

## Reuse and Styling Rules from UX
- Preserve the dark tonal layering from the design brief
- Avoid visible divider-heavy layouts; use nested surface colors instead
- Use cyan accents sparingly for primary action and active state
- Use rounded medium-to-large corners
- Prefer subtle elevation and tonal contrast over hard borders
- Bundle local font assets if licensing permits; otherwise use a Qt-safe fallback while keeping the typography hierarchy intact

## Implementation Phases

### Phase 1: Project Bootstrap
- add Qt/CMake project files
- add executable target
- add resources and theme system
- create main window shell with top bar and navigation rail

### Phase 2: Screen Scaffolding
- add the four page classes
- wire page switching
- create shared card, chip, and section widgets

### Phase 3: UX Parity Pass
- match layout spacing, colors, and typography to the supplied UX
- add static content and representative placeholder visuals
- tune responsive window resizing behavior

### Phase 4: Interaction Pass
- add navigation state
- add button interactions and lightweight local state
- add screen-specific toolbar behavior

### Phase 5: Readiness for Backend Integration
- define interfaces for future data providers without implementing inference integration yet
- isolate placeholder content generation so real data sources can replace it cleanly

## Acceptance Criteria for Implementation
- A Qt desktop executable builds from `_app/desktop`
- Launching the executable shows a polished main shell with persistent top bar and left navigation
- Each nav destination opens a dedicated page matching the corresponding UX mockup at a structural level
- The application theme reflects the palette and tonal-depth rules in `_app/ux/aetheris_medical/DESIGN.md`
- The app remains usable at desktop sizes commonly used for clinical workstations
- Code structure supports later integration of real FastSurfer data and job execution

## Risks and Mitigations
- Risk: exact HTML-to-Qt visual parity is expensive
  - Mitigation: match hierarchy, palette, spacing, and component intent first; reserve micro-polish for a later pass
- Risk: remote web fonts and icons in the HTML mockups are not suitable for an offline desktop app
  - Mitigation: bundle local assets or use Qt-native iconography and documented fallbacks
- Risk: scan viewer expectations may imply real DICOM/NIfTI functionality
  - Mitigation: clearly scope milestone one to a native shell with placeholder imagery and extensible viewer containers

## Validation Plan
- Configure and build the Qt target with CMake
- Launch the executable locally to verify navigation and rendering
- Verify all four pages load without runtime errors
- Resize-test the main window to validate layout stability

## Open Decisions for Approval
- Confirm Qt Widgets is acceptable for the first implementation milestone
- Confirm the first milestone should focus on native UX parity and structure, not backend execution
- Confirm the feature spec path should use `_app/specs/qt_desktop_application` as the repository-specific equivalent of `app/specs/{feature_name}`

## Next Workflow Step After Approval
After user approval of this plan, create `_app/specs/qt_desktop_application/bdd_requirements.md` and only then proceed to code implementation in `_app/desktop`.