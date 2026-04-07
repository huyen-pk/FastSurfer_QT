# BDD Requirements: Qt Quick Desktop App

## Feature

Qt Quick desktop application in C++ under `_app/desktop`, implementing the UX direction from `_app/ux`.

## Behavioral Intent

The application should provide a polished native desktop shell that presents four major work areas derived from the approved UX references:

- Dashboard
- Scan Viewer
- Pipelines
- Analytics

The first iteration is a presentational application shell with representative application state and native Qt visuals. It is not required to execute FastSurfer jobs or load real MRI datasets.

## Acceptance Criteria

### Application Startup

- The application builds successfully as a Qt 6 C++ project from `_app/desktop`.
- Launching the executable opens a single desktop window without a QML load failure.
- The initial window displays the shared application shell with top bar, side navigation, and a primary content region.
- The default selected screen is Dashboard.

### Shared Shell And Visual System

- The shell uses the dark tonal design system described in `_app/ux/aetheris_medical/DESIGN.md`.
- The visual hierarchy relies primarily on layered surfaces rather than visible divider lines.
- The navigation and primary action styling remain visually consistent across all screens.
- The application does not depend on remote CDN assets or remote images to render the core interface.

### Navigation

- The side navigation contains entries for Dashboard, Scan Viewer, Pipelines, and Analytics.
- Selecting a navigation item replaces the main content area with the corresponding screen.
- The active navigation item is visually distinct from inactive items.
- Navigation between screens does not require restarting the application.

### Dashboard Screen

- The Dashboard screen presents a top-level heading and supporting summary text.
- The screen includes three summary metric cards.
- The screen includes a recent studies table or table-like layout with multiple rows.
- The screen includes at least one secondary operational panel that complements the recent studies area.
- Dashboard data is supplied from application-owned C++ data structures or models exposed to QML.

### Scan Viewer Screen

- The Scan Viewer screen presents a viewer-focused layout with a dedicated toolbar/header area.
- The main viewer area contains four visible image panes or pane-like placeholders.
- The screen includes a metadata sidebar with study or scan details.
- The screen visually distinguishes the imaging workspace from the metadata panel using tonal surfaces.
- The screen uses native Qt rendering or local placeholder visuals rather than remote imagery.

### Pipelines Screen

- The Pipelines screen presents a sequence or flow of processing stages.
- Each pipeline stage displays a label and a visible state or descriptor.
- The screen includes a prominent action area consistent with the approved UX direction.
- The pipeline content is driven by application-owned C++ data structures or models exposed to QML.

### Analytics Screen

- The Analytics screen presents a high-level analysis heading and supporting context text.
- The screen includes a primary chart-like visualization area.
- The screen includes secondary metric panels or comparison summaries.
- The chart or chart-like visuals render using native Qt primitives or local QML components.
- Analytics data is supplied from application-owned C++ data structures or models exposed to QML.

### Native App Structure

- The application entry point is implemented in C++.
- QML is packaged as part of the application resources or QML module configuration.
- Screen-level state is organized so that navigation and representative data can be maintained without hardcoding all UI logic into a single QML file.

### Documentation

- `_app/desktop/README.md` documents how to configure, build, and run the application.
- The README identifies any Qt version or system prerequisites needed to launch the app.

## BDD Scenarios

### Scenario 1: Launching the application shows the approved desktop shell

**Given** the Qt desktop application has been built successfully from `_app/desktop`

**When** the user launches the executable

**Then** a single desktop window should open

**And** the window should show the shared top bar

**And** the window should show the left navigation rail

**And** the main content area should display the Dashboard screen by default

### Scenario 2: The user can navigate between all four screens

**Given** the application is running on the Dashboard screen

**When** the user selects Scan Viewer from the navigation

**Then** the main content area should display the Scan Viewer layout

**When** the user selects Pipelines from the navigation

**Then** the main content area should display the Pipelines layout

**When** the user selects Analytics from the navigation

**Then** the main content area should display the Analytics layout

**And** the currently selected navigation item should be visibly active

### Scenario 3: The Dashboard screen presents operational overview content

**Given** the application is displaying the Dashboard screen

**Then** the user should see a prominent page heading

**And** the user should see three summary metric cards

**And** the user should see a recent studies table or table-like list with multiple entries

**And** the user should see at least one secondary information panel

### Scenario 4: The Scan Viewer screen presents a four-pane viewing workspace

**Given** the application is displaying the Scan Viewer screen

**Then** the user should see a viewer toolbar or secondary header

**And** the user should see four viewer panes in the main workspace

**And** the user should see scan metadata in a dedicated sidebar

**And** the imaging area should remain visually distinct from the metadata area

### Scenario 5: The Pipelines screen presents a processing flow

**Given** the application is displaying the Pipelines screen

**Then** the user should see a page heading and contextual summary

**And** the user should see multiple ordered pipeline stages

**And** each stage should present a visible name and status or descriptor

**And** the user should see a prominent action affordance associated with pipeline execution or management

### Scenario 6: The Analytics screen presents comparative results

**Given** the application is displaying the Analytics screen

**Then** the user should see a high-level analysis heading

**And** the user should see a primary chart-like visualization area

**And** the user should see supporting metric panels or summaries

**And** the screen should preserve the shared shell styling used throughout the application

### Scenario 7: The application renders without external web dependencies

**Given** the application has been built and launched in an environment without network access

**When** the main screens are opened through the side navigation

**Then** the shell and all four screens should render their core layout successfully

**And** the application should not require remote fonts, remote images, or CDN-hosted scripts for the interface structure to appear

### Scenario 8: The application structure supports maintainable screen composition

**Given** the source tree under `_app/desktop` is inspected

**Then** the project should contain a C++ application entry point

**And** screen composition should be split into multiple QML files or reusable components

**And** representative screen data should be exposed from C++ through structured objects or models rather than embedded as one monolithic block of QML literals

## Notes For Implementation And Verification

- The verification phase should use the built application and real Qt runtime behavior.
- Placeholder content is acceptable only where the approved implementation plan explicitly scoped out real FastSurfer execution and real MRI image loading.
- The visual implementation should prioritize faithful translation of layout, spacing, tonal depth, and navigation behavior from `_app/ux`, rather than literal HTML reproduction.