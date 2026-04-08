cmake_minimum_required(VERSION 3.21)

project(FastSurferCore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ITK REQUIRED COMPONENTS
    ITKCommon
    ITKImageFunction
    ITKImageGrid
    ITKTransform
)
find_package(ZLIB REQUIRED)

include(${ITK_USE_FILE})

add_library(fastsurfer_core STATIC
    src/conform_step_service.cpp
    src/mgh_image.cpp
)

target_include_directories(fastsurfer_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fastsurfer_core
    PUBLIC
    ${ITK_LIBRARIES}
        ZLIB::ZLIB
)

target_compile_features(fastsurfer_core PUBLIC cxx_std_17)

option(FASTSURFER_CORE_BUILD_TESTS "Build _app/core tests" ${PROJECT_IS_TOP_LEVEL})

if(FASTSURFER_CORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()