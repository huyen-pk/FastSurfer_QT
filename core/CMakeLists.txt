cmake_minimum_required(VERSION 3.21)

project(OpenHealthcare LANGUAGES C CXX)

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

add_library(openhc_imaging_mri_fastsurfer STATIC
    src/imaging/mri/fastsurfer/conform_policy.cpp
    src/imaging/mri/fastsurfer/step_conform.cpp
    src/imaging/mri/fastsurfer/mgh_image.cpp
    src/imaging/mri/fastsurfer/nifti_converter.cpp
)

target_include_directories(openhc_imaging_mri_fastsurfer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Ensure ITK include directories are available to the target and its consumers.
if(DEFINED ITK_INCLUDE_DIRS)
    target_include_directories(openhc_imaging_mri_fastsurfer PUBLIC ${ITK_INCLUDE_DIRS})
endif()

# Link libraries. Prefer modern imported ITK targets when available,
# otherwise fall back to the legacy ${ITK_LIBRARIES} variable.
target_link_libraries(openhc_imaging_mri_fastsurfer
    PUBLIC
        ZLIB::ZLIB
)

if(TARGET ITK::Common)
    target_link_libraries(openhc_imaging_mri_fastsurfer PUBLIC
        ITK::Common
        ITK::ImageFunction
        ITK::ImageGrid
        ITK::IONIFTI
        ITK::Transform
    )
else()
    target_link_libraries(openhc_imaging_mri_fastsurfer PUBLIC ${ITK_LIBRARIES})
endif()

# Link Eigen so its include directories are exported to consumers (if available).
if(TARGET Eigen3::Eigen)
    target_link_libraries(openhc_imaging_mri_fastsurfer PUBLIC Eigen3::Eigen)
endif()

target_compile_features(openhc_imaging_mri_fastsurfer PUBLIC cxx_std_17)

option(OPENHC_IMAGING_MRI_FASTSURFER_BUILD_TESTS "Build _app/core tests" ${PROJECT_IS_TOP_LEVEL})

if(OPENHC_IMAGING_MRI_FASTSURFER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()