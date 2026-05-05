cmake_minimum_required(VERSION 3.21)

project(FastSurferCore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands to aid IDEs (compile_commands.json)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ITK REQUIRED COMPONENTS
    ITKCommon
    ITKImageFunction
    ITKImageGrid
    ITKIONIFTI
    ITKTransform
)
find_package(Eigen3 REQUIRED)
find_package(ZLIB REQUIRED)

include(${ITK_USE_FILE})

add_library(fastsurfer_core STATIC
    src/step_conform.cpp
    src/mgh_image.cpp
    src/nifti_converter.cpp
)

target_include_directories(fastsurfer_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Ensure ITK include directories are available to the target and its consumers.
if(DEFINED ITK_INCLUDE_DIRS)
    target_include_directories(fastsurfer_core PUBLIC ${ITK_INCLUDE_DIRS})
endif()

# Link libraries. Prefer modern imported ITK targets when available,
# otherwise fall back to the legacy ${ITK_LIBRARIES} variable.
target_link_libraries(fastsurfer_core
    PUBLIC
        ZLIB::ZLIB
)

if(TARGET ITK::Common)
    target_link_libraries(fastsurfer_core PUBLIC
        ITK::Common
        ITK::ImageFunction
        ITK::ImageGrid
        ITK::IONIFTI
        ITK::Transform
    )
else()
    target_link_libraries(fastsurfer_core PUBLIC ${ITK_LIBRARIES})
endif()

# Link Eigen so its include directories are exported to consumers (if available).
if(TARGET Eigen3::Eigen)
    target_link_libraries(fastsurfer_core PUBLIC Eigen3::Eigen)
endif()

target_compile_features(fastsurfer_core PUBLIC cxx_std_17)

option(FASTSURFER_CORE_BUILD_TESTS "Build _app/core tests" ${PROJECT_IS_TOP_LEVEL})

if(FASTSURFER_CORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()