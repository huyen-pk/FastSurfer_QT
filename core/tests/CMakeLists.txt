set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(FASTSURFER_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)


add_executable(test_nifti_converter
    imaging/mri/fastsurfer/test_nifti_converter.cpp
)

target_link_libraries(test_nifti_converter
    PRIVATE
        openhc_imaging_mri_fastsurfer
)

target_compile_definitions(test_nifti_converter
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME convert_nifti_to_mgz_should_preserve_expected_geometry_and_voxels COMMAND test_nifti_converter)

add_executable(test_nifti_converter_roundtrip
    imaging/mri/fastsurfer/test_nifti_converter_roundtrip.cpp
)

target_link_libraries(test_nifti_converter_roundtrip
    PRIVATE
        openhc_imaging_mri_fastsurfer
)

target_compile_definitions(test_nifti_converter_roundtrip
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME convert_nifti_to_mgz_and_back_should_produce_original_image COMMAND test_nifti_converter_roundtrip)


add_executable(test_conform_policy
    imaging/mri/fastsurfer/test_conform_policy.cpp
)

target_link_libraries(test_conform_policy
    PRIVATE
        openhc_imaging_mri_fastsurfer
)

add_test(NAME compute_target_voxel_size_should_respect_threshold_cases COMMAND test_conform_policy threshold)

add_test(NAME compute_native_target_dimensions_should_distinguish_auto_from_fov COMMAND test_conform_policy auto-vs-fov)

add_test(NAME compute_native_target_dimensions_should_cube_anisotropic_auto_size COMMAND test_conform_policy anisotropic-auto)

add_test(NAME compute_native_target_dimensions_should_floor_small_auto_size_to_256_cube COMMAND test_conform_policy small-auto-floor-256)

add_test(NAME parse_conform_modes_should_round_trip_supported_values_and_reject_invalid_input COMMAND test_conform_policy mode-parsing)

add_test(NAME compute_scale_policy_should_handle_constant_inputs_and_apply_scale_consistently COMMAND test_conform_policy scale-policy)

add_test(NAME apply_scale_policy_should_respect_zero_epsilon_boundary COMMAND test_conform_policy scale-zero-epsilon)

add_test(NAME compute_scale_policy_should_fallback_to_unit_scale_when_histogram_width_collapses COMMAND test_conform_policy scale-histogram-collapse)

add_test(NAME compute_scale_policy_should_clip_upper_percentile_outliers COMMAND test_conform_policy scale-upper-outliers)

add_test(NAME compute_scale_policy_should_scale_mri_like_distribution_without_artifact_dominance COMMAND test_conform_policy scale-mri-like)

add_executable(test_step_conform
    imaging/mri/fastsurfer/test_step_conform.cpp
)

target_link_libraries(test_step_conform
    PRIVATE
        openhc_imaging_mri_fastsurfer
    Eigen3::Eigen
)

target_compile_definitions(test_step_conform
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conformed_step_on_already_conformed_scan_produce_conformed_output COMMAND test_step_conform conformed)

add_test(NAME run_conform_step_on_already_conformed_scan_should_not_introduce_sampling_errors COMMAND test_step_conform force-conformed)

add_test(NAME run_conform_step_on_already_conformed_float_scan_should_only_introduce_histogram_scaling_errors COMMAND test_step_conform force-conformed-scaled)

add_test(NAME run_conform_step_on_standard_scan_should_produce_conformed_output COMMAND test_step_conform standard)

add_test(NAME run_conform_step_on_oblique_scan_should_produce_conformed_output COMMAND test_step_conform oblique)

add_executable(test_native_reconform_non_conformed
    imaging/mri/fastsurfer/test_native_reconform_non_conformed.cpp
)

target_link_libraries(test_native_reconform_non_conformed
    PRIVATE
        openhc_imaging_mri_fastsurfer
)

target_compile_definitions(test_native_reconform_non_conformed
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conform_step_on_non_conformed_mgz_should_produce_conformed_output COMMAND test_native_reconform_non_conformed)

add_executable(test_python_parity_conform_step
    imaging/mri/fastsurfer/test_python_parity_conform_step.cpp
)

target_link_libraries(test_python_parity_conform_step
    PRIVATE
        openhc_imaging_mri_fastsurfer
)

target_compile_definitions(test_python_parity_conform_step
    PRIVATE
        FASTSURFER_REPO_ROOT="${FASTSURFER_REPO_ROOT}"
)

add_test(NAME run_conform_step_should_match_python_reference_on_conformed_image COMMAND test_python_parity_conform_step conformed)

add_test(NAME run_conform_step_should_match_python_reference_on_oblique_image COMMAND test_python_parity_conform_step oblique)
