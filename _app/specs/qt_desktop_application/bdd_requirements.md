# BDD Requirements: Qt Desktop Application

## Feature Intent
Provide a native Qt desktop application in `_app/desktop` that reproduces the visual structure and interaction flow of the UX assets in `_app/ux`, with a polished shell and four primary screens:
- Dashboard
- Scan Viewer
- Pipelines
- Results Analytics

## Milestone Scope
This BDD specification covers milestone one only.

Milestone-one constraints:
- the application is a native C++ Qt desktop executable
- the application must not require FastSurfer inference, ONNX execution, DICOM parsing, or MRI processing to launch
- screen content may use local static content and local resource images
- all behaviors must be implemented with real Qt components and real local resources, not mocked runtime integrations

## UX Sources
- `_app/ux/aetheris_medical/DESIGN.md`
- `_app/ux/dashboard/code.html`
- `_app/ux/simplified_pipeline_config/code.html`
- `_app/ux/simplified_scan_viewer/code.html`
- `_app/ux/results_analytics/code.html`

## Acceptance Criteria
- The desktop app builds from `_app/desktop` using CMake and Qt 6
- The app launches into a consistent shell with top bar, left navigation, and a routed central workspace
- Each of the four target screens is reachable from the left navigation
- Each screen preserves the intended content hierarchy, tonal layering, and primary action emphasis defined by the UX assets
- The app remains functional and visually coherent after window resizing across typical desktop dimensions
- No backend service is required for the first milestone experience

## Behavioral Scenarios

### Scenario 1: Application startup shows the native shell
Given the Qt desktop application has been built successfully
When the user launches the executable
Then the main window should open without requiring external backend processes
And the top application bar should be visible
And the left navigation rail should be visible
And the central content area should display the default landing page

### Scenario 2: Default route opens the dashboard experience
Given the application has just started
When the main window is first rendered
Then the dashboard page should be selected by default
And the dashboard navigation item should show an active visual state
And the dashboard page should present an overview header and summary content matching the dashboard UX hierarchy

### Scenario 3: Navigation switches between the four primary screens
Given the application shell is open
When the user selects each navigation item in the left rail
Then the corresponding page should become visible in the central workspace
And only one primary page should be active at a time
And the active navigation item should update to reflect the visible page

### Scenario 4: Dashboard presents clinical overview content
Given the user is on the dashboard page
When the page is rendered
Then the page should show a prominent overview title
And it should show summary metric cards near the top of the page
And it should show a recent studies table or equivalent structured study list
And it should show secondary summary content aligned with the dashboard mockup

### Scenario 5: Pipelines page presents the workflow sequence view
Given the user navigates to the pipelines page
When the page is rendered
Then the page should show a sequence header with action controls
And it should show vertically ordered pipeline nodes representing the workflow
And the node presentation should communicate process order visually
And the page should include a clear action area for executing or extending the workflow

### Scenario 6: Scan viewer presents a 2x2 diagnostic workspace
Given the user navigates to the scan viewer page
When the page is rendered
Then the page should show a viewer-specific toolbar
And it should show a 2x2 viewing grid in the main workspace
And it should show a metadata sidebar adjacent to the viewer area
And it should show local placeholder visual content suitable for the viewer layout

### Scenario 7: Analytics page presents comparative results content
Given the user navigates to the analytics page
When the page is rendered
Then the page should show a prominent analysis header
And it should show a primary chart or chart-like visualization area
And it should show supporting summary cards or metric panels
And it should include a visible report-oriented primary action

### Scenario 8: Shared theme reflects the approved design system
Given the application shell and pages are rendered
When the user navigates through the application
Then the UI should use a dark tonal layered theme derived from the Luminescent Lab design brief
And primary cyan accents should be reserved for active or high-priority actions
And the interface should rely on surface contrast rather than heavy divider lines for structure

### Scenario 9: Resize behavior preserves usability
Given the application is open on any of the four pages
When the user resizes the main window within normal desktop ranges
Then the top bar and left navigation should remain usable
And the active page layout should remain readable and non-overlapping
And critical actions should remain visible or reachable without layout corruption

### Scenario 10: The first milestone runs without inference integration
Given the application is launched on a machine without FastSurfer runtime services started
When the user opens and navigates the desktop app
Then the shell and all four primary screens should remain available
And the app should not block on model execution, scan processing, or remote assets

## Out of Scope for These Behaviors
- launching FastSurfer processing jobs
- loading real MRI data files into the viewer
- rendering real analytics derived from pipeline outputs
- saving pipeline definitions to a persistent backend
- exporting a real PDF report

## Test Intent for Later Implementation
The eventual implementation should be verifiable with real application execution and real Qt UI tests.

Expected verification characteristics:
- use real Qt widgets and real application startup
- avoid mocks, fake services, and simulated backend contracts presented as production dependencies
- keep test names behavior-focused using the `action_should_expected_behavior` pattern where applicable

## Approval Gate
This file is the Step 3 Product Owner artifact for the feature.
The workflow must pause here until the user confirms these BDD requirements are acceptable.