cmake_minimum_required(VERSION 3.21)

project(FastSurferDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(fastsurfer_desktop
    include/app/AppStatusBar.h
    include/app/MainWindow.h
    include/app/NavigationRail.h
    include/app/PageId.h
    include/app/Theme.h
    include/app/TopBar.h
    include/screens/AnalyticsPage.h
    include/screens/DashboardPage.h
    include/screens/PipelinesPage.h
    include/screens/ScanViewerPage.h
    include/widgets/AnalyticsChart.h
    include/widgets/MetricCard.h
    include/widgets/PipelineNode.h
    include/widgets/ViewerPane.h
    src/main.cpp
    src/app/AppStatusBar.cpp
    src/app/MainWindow.cpp
    src/app/NavigationRail.cpp
    src/app/Theme.cpp
    src/app/TopBar.cpp
    src/screens/AnalyticsPage.cpp
    src/screens/DashboardPage.cpp
    src/screens/PipelinesPage.cpp
    src/screens/ScanViewerPage.cpp
    src/widgets/AnalyticsChart.cpp
    src/widgets/MetricCard.cpp
    src/widgets/PipelineNode.cpp
    src/widgets/ViewerPane.cpp
)

target_include_directories(fastsurfer_desktop PRIVATE include)

target_link_libraries(fastsurfer_desktop PRIVATE Qt6::Widgets)
