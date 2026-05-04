set(FASTSURFER_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

add_executable(test_conform_step_service
    test_conform_step_service.cpp
)

target_link_libraries(test_conform_step_service
    PRIVATE
        fastsurfer_core
)

target_compile_definitions(test_conform_step_service
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conform_step_should_write_copy_and_conformed_outputs COMMAND test_conform_step_service)

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

add_test(NAME run_conform_step_should_match_python_reference_on_mgz_fixture COMMAND test_python_parity_conform_step)

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

add_executable(test_nifti_roundtrip
    test_nifti_roundtrip.cpp
)

target_link_libraries(test_nifti_roundtrip
    PRIVATE
        fastsurfer_core
)

target_compile_definitions(test_nifti_roundtrip
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME convert_nifti_to_mgz_and_back_to_nifti_should_roundtrip_fixture_geometry_and_voxels COMMAND test_nifti_roundtrip)

add_executable(test_conform_step_service_nifti
    test_conform_step_service_nifti.cpp
)

target_link_libraries(test_conform_step_service_nifti
    PRIVATE
        fastsurfer_core
    Eigen3::Eigen
)

target_compile_definitions(test_conform_step_service_nifti
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conform_step_on_nifti_input_should_write_copy_and_conformed_outputs COMMAND test_conform_step_service_nifti)