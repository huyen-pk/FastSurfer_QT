set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(FASTSURFER_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

add_executable(test_step_conform
    test_step_conform.cpp
)

target_link_libraries(test_step_conform
    PRIVATE
        fastsurfer_core
    Eigen3::Eigen
)

target_compile_definitions(test_step_conform
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conformed_step_on_already_conformed_scan_produce_conformed_output COMMAND test_step_conform conformed)

add_test(NAME run_conform_step_on_already_conformed_scan_should_not_introduce_sampling_errors COMMAND test_step_conform force-conformed)

add_test(NAME run_conform_step_on_standard_scan_should_produce_conformed_output COMMAND test_step_conform standard)

add_test(NAME run_conform_step_on_oblique_scan_should_produce_conformed_output COMMAND test_step_conform oblique)

add_executable(test_native_reconform_non_conformed
    test_native_reconform_non_conformed.cpp
)

target_link_libraries(test_native_reconform_non_conformed
    PRIVATE
        fastsurfer_core
)

target_compile_definitions(test_native_reconform_non_conformed
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conform_step_on_non_conformed_mgz_should_produce_conformed_output COMMAND test_native_reconform_non_conformed)

add_executable(test_python_parity_conform_step
    test_python_parity_conform_step.cpp
)

target_link_libraries(test_python_parity_conform_step
    PRIVATE
        fastsurfer_core
)

target_compile_definitions(test_python_parity_conform_step
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conform_step_should_match_python_reference_on_conformed_image COMMAND test_python_parity_conform_step conformed)

add_test(NAME run_conform_step_should_match_python_reference_on_oblique_image COMMAND test_python_parity_conform_step oblique)

add_executable(test_nifti_converter
    test_nifti_converter.cpp
)

target_link_libraries(test_nifti_converter
    PRIVATE
        fastsurfer_core
)

target_compile_definitions(test_nifti_converter
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME convert_nifti_to_mgz_should_preserve_expected_geometry_and_voxels COMMAND test_nifti_converter)

add_executable(test_nifti_converter_roundtrip
    test_nifti_converter_roundtrip.cpp
)

target_link_libraries(test_nifti_converter_roundtrip
    PRIVATE
        fastsurfer_core
)

target_compile_definitions(test_nifti_converter_roundtrip
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME convert_nifti_to_mgz_and_back_should_produce_original_image COMMAND test_nifti_converter_roundtrip)

