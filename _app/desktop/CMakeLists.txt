cmake_minimum_required(VERSION 3.21)

project(FastSurferQtDesktop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup()

set_source_files_properties(
    qml/qtquickdesktop/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_executable(fastsurfer_qt_desktop
    src/main.cpp
    src/app/application_controller.cpp
    src/app/application_controller.h
    src/app/navigation_state.cpp
    src/app/navigation_state.h
)

target_include_directories(fastsurfer_qt_desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

qt_add_qml_module(fastsurfer_qt_desktop
    URI FastSurferDesktop
    VERSION 1.0
    QML_FILES
        qml/qtquickdesktop/Main.qml
        qml/qtquickdesktop/Theme.qml
        qml/qtquickdesktop/components/AnalyticsBarChart.qml
        qml/qtquickdesktop/components/AppShell.qml
        qml/qtquickdesktop/components/MetricCard.qml
        qml/qtquickdesktop/components/PillChip.qml
        qml/qtquickdesktop/components/SectionHeader.qml
        qml/qtquickdesktop/components/SideNav.qml
        qml/qtquickdesktop/components/SurfacePanel.qml
        qml/qtquickdesktop/components/TopBar.qml
        qml/qtquickdesktop/components/ViewerPane.qml
        qml/qtquickdesktop/pages/AnalyticsPage.qml
        qml/qtquickdesktop/pages/DashboardPage.qml
        qml/qtquickdesktop/pages/PipelinesPage.qml
        qml/qtquickdesktop/pages/ScanViewerPage.qml
)

target_link_libraries(fastsurfer_qt_desktop PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

install(TARGETS fastsurfer_qt_desktop
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)