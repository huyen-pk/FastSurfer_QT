#include "imaging/mri/fastsurfer/conform_policy.h"

#include <cmath>
#include <cstdio>
#include "TestConstants.h"
#include "TestHelpers.h"
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "imaging/mri/fastsurfer/mgh_image.h"
#include "imaging/mri/fastsurfer/step_conform_request.h"

namespace {

// Assertion helpers are provided by TestHelpers.h

void appendRepeatedValues(std::vector<float> &values, const std::size_t count, const float value)
{
    values.insert(values.end(), count, value);
}

OpenHC::imaging::mri::fastsurfer::MghImage makeImage(
    const std::array<int, 3> &dimensions,
    const std::array<float, 3> &spacing)
{
    OpenHC::imaging::mri::fastsurfer::MghImage::Header header;
    header.dimensions = dimensions;
    header.spacing = spacing;

    const std::size_t voxelCount = static_cast<std::size_t>(dimensions[0]) *
                                   static_cast<std::size_t>(dimensions[1]) *
                                   static_cast<std::size_t>(dimensions[2]);
    return OpenHC::imaging::mri::fastsurfer::MghImage::fromVoxelData(
        header,
        std::vector<float>(voxelCount, 0.0F),
        static_cast<std::int32_t>(OpenHC::imaging::mri::fastsurfer::MghDataType::Float32));
}

void compute_target_voxel_size_should_respect_threshold_cases()
{
    OpenHC::imaging::mri::fastsurfer::ConformStepRequest request;

    const auto belowThresholdImage = makeImage({32, 32, 32}, {0.8F, 1.1F, 1.2F});
    requireNear(
        OpenHC::imaging::mri::fastsurfer::computeTargetVoxelSize(belowThresholdImage, request),
        0.8F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Voxel-size policy should keep the minimum voxel size when it is below the threshold.");

    const auto aboveThresholdImage = makeImage({32, 32, 32}, {0.96F, 1.1F, 1.2F});
    requireNear(
        OpenHC::imaging::mri::fastsurfer::computeTargetVoxelSize(aboveThresholdImage, request),
        1.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Voxel-size policy should promote the target voxel size to 1mm when it is above the threshold.");

    const auto boundaryImage = makeImage({32, 32, 32}, {0.95F, 1.1F, 1.2F});
    requireNear(
        OpenHC::imaging::mri::fastsurfer::computeTargetVoxelSize(boundaryImage, request),
        0.95F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Voxel-size policy should preserve the exact threshold boundary because the comparison is strict-greater-than.");
}

void compute_native_target_dimensions_should_distinguish_auto_from_fov()
{
    const auto image = makeImage({300, 240, 180}, {0.8F, 0.8F, 1.0F});

    const auto autoDimensions = OpenHC::imaging::mri::fastsurfer::computeNativeTargetDimensions(image, 0.8F, OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Auto);
    require(
        autoDimensions == std::array<int, 3> {300, 300, 300},
        "Auto image-size mode should cube the target to the largest derived dimension.");

    const auto fovDimensions = OpenHC::imaging::mri::fastsurfer::computeNativeTargetDimensions(image, 0.8F, OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Fov);
    require(
        fovDimensions == std::array<int, 3> {300, 240, 225},
        "FOV image-size mode should preserve per-axis target dimensions.");
}

void compute_native_target_dimensions_should_cube_anisotropic_auto_size()
{
    const auto image = makeImage({320, 320, 180}, {0.8F, 0.8F, 1.0F});

    const auto autoDimensions = OpenHC::imaging::mri::fastsurfer::computeNativeTargetDimensions(image, 0.8F, OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Auto);
    require(
        autoDimensions == std::array<int, 3> {320, 320, 320},
        "Auto image-size mode should expand anisotropic FOV-derived dimensions into a cube using the maximum axis.");
}

void compute_native_target_dimensions_should_floor_small_auto_size_to_256_cube()
{
    const auto image = makeImage({120, 110, 90}, {0.8F, 0.8F, 0.8F});

    const auto autoDimensions = OpenHC::imaging::mri::fastsurfer::computeNativeTargetDimensions(image, 0.8F, OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Auto);
    require(
        autoDimensions == std::array<int, 3> {256, 256, 256},
        "Auto image-size mode should retain the minimum 256^3 cube for small fields of view.");
}

void parse_conform_modes_should_round_trip_supported_values_and_reject_invalid_input()
{
    require(
        OpenHC::imaging::mri::fastsurfer::parseVoxelSizeMode("min") == OpenHC::imaging::mri::fastsurfer::VoxelSizeMode::Min,
        "Voxel-size mode parser should accept the supported 'min' value.");
    require(
        OpenHC::imaging::mri::fastsurfer::parseImageSizeMode("auto") == OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Auto,
        "Image-size mode parser should accept the supported 'auto' value.");
    require(
        OpenHC::imaging::mri::fastsurfer::parseImageSizeMode("fov") == OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Fov,
        "Image-size mode parser should accept the supported 'fov' value.");
    require(
        OpenHC::imaging::mri::fastsurfer::to_string(OpenHC::imaging::mri::fastsurfer::VoxelSizeMode::Min) == "min",
        "Voxel-size mode string conversion should preserve the public text form.");
    require(
        OpenHC::imaging::mri::fastsurfer::to_string(OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Auto) == "auto" &&
            OpenHC::imaging::mri::fastsurfer::to_string(OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Fov) == "fov",
        "Image-size mode string conversion should preserve the public text forms.");
    require(
        OpenHC::imaging::mri::fastsurfer::parseOrientationMode("lia") == OpenHC::imaging::mri::fastsurfer::OrientationMode::Lia &&
            OpenHC::imaging::mri::fastsurfer::parseOrientationMode("native") == OpenHC::imaging::mri::fastsurfer::OrientationMode::Native,
        "Orientation mode parser should accept the supported request values.");
    require(
        OpenHC::imaging::mri::fastsurfer::to_string(OpenHC::imaging::mri::fastsurfer::OrientationMode::Lia) == "lia" &&
            OpenHC::imaging::mri::fastsurfer::to_string(OpenHC::imaging::mri::fastsurfer::OrientationMode::Native) == "native",
        "Orientation mode string conversion should preserve the public text forms.");

    bool invalidVoxelModeRejected = false;
    try {
        static_cast<void>(OpenHC::imaging::mri::fastsurfer::parseVoxelSizeMode("bogus"));
    } catch (const std::invalid_argument &) {
        invalidVoxelModeRejected = true;
    }
    require(invalidVoxelModeRejected, "Voxel-size mode parser should reject unsupported input.");

    bool invalidImageModeRejected = false;
    try {
        static_cast<void>(OpenHC::imaging::mri::fastsurfer::parseImageSizeMode("bogus"));
    } catch (const std::invalid_argument &) {
        invalidImageModeRejected = true;
    }
    require(invalidImageModeRejected, "Image-size mode parser should reject unsupported input.");

    bool invalidOrientationModeRejected = false;
    try {
        static_cast<void>(OpenHC::imaging::mri::fastsurfer::parseOrientationMode("bogus"));
    } catch (const std::invalid_argument &) {
        invalidOrientationModeRejected = true;
    }
    require(invalidOrientationModeRejected, "Orientation mode parser should reject unsupported input.");
}

void compute_scale_policy_should_handle_constant_inputs_and_apply_scale_consistently()
{
    const OpenHC::imaging::mri::fastsurfer::ScalePolicy constantPolicy =
        OpenHC::imaging::mri::fastsurfer::computeScalePolicy(std::vector<float> {42.0F, 42.0F, 42.0F}, 0.0F, 255.0F);
    requireNear(constantPolicy.srcMin, 42.0F, test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Scale policy should preserve the constant input minimum.");
    requireNear(constantPolicy.scale, 1.0F, test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Scale policy should default to a unit scale for constant inputs.");

    const OpenHC::imaging::mri::fastsurfer::ScalePolicy policy {10.0F, 2.0F};
    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(0.0F, 0.0F, 255.0F, policy),
        0.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Scale policy should preserve exact zero values.");
    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(20.0F, 0.0F, 255.0F, policy),
        20.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Scale policy should shift and scale mapped values.");
    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(200.0F, 0.0F, 255.0F, policy),
        255.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Scale policy should clamp scaled values to the destination range.");

    const std::vector<float> mappedData {0.0F, 20.0F, 200.0F};
    const std::vector<float> scaled = OpenHC::imaging::mri::fastsurfer::applyScalePolicy(mappedData, 0.0F, 255.0F, policy);
    require(scaled == std::vector<float>({0.0F, 20.0F, 255.0F}),
        "Vector scale application should match the per-value scale policy behavior.");
}

void apply_scale_policy_should_respect_zero_epsilon_boundary()
{
    const OpenHC::imaging::mri::fastsurfer::ScalePolicy policy {0.0F, 1.0e9F};

    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(test_constants::SCALED_ZERO_EPSILON * 0.5F, 0.0F, 255.0F, policy),
        0.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Values below the scaled-zero epsilon should be forced to exact zero.");
    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(test_constants::SCALED_ZERO_EPSILON, 0.0F, 255.0F, policy),
        0.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Values exactly on the scaled-zero epsilon boundary should still be treated as zero.");
    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(test_constants::SCALED_ZERO_EPSILON * 2.0F, 0.0F, 255.0F, policy),
        20.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Values above the scaled-zero epsilon should participate in normal scaling.");
}

void compute_scale_policy_should_fallback_to_unit_scale_when_histogram_width_collapses()
{
    const std::vector<float> nearlyIdenticalValues {1.0e-20F, 1.5e-20F, 2.0e-20F};
    const OpenHC::imaging::mri::fastsurfer::ScalePolicy policy =
        OpenHC::imaging::mri::fastsurfer::computeScalePolicy(nearlyIdenticalValues, 0.0F, 255.0F);

    require(
        policy.srcMin == nearlyIdenticalValues.front(),
        "When histogram bins collapse numerically, the scale policy should keep the minimum input value.");
    requireNear(
        policy.scale,
        1.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "When histogram bins collapse numerically, the scale policy should fall back to unit scaling.");
}

void compute_scale_policy_should_clip_upper_percentile_outliers()
{
    std::vector<float> rampValues;
    rampValues.reserve(2000);
    for (int value = 1; value <= 2000; ++value) {
        rampValues.push_back(static_cast<float>(value));
    }

    const OpenHC::imaging::mri::fastsurfer::ScalePolicy policy = OpenHC::imaging::mri::fastsurfer::computeScalePolicy(rampValues, 0.0F, 255.0F);
    const float unclippedScale = 255.0F / 1999.0F;

    requireNear(
        policy.srcMin,
        1.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "The scale policy should preserve the lower bound when the lower cutoff is zero.");
    require(
        policy.scale > unclippedScale,
        "Percentile clipping should increase the foreground scale compared with a full-range min-max mapping.");
    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(1999.0F, 0.0F, 255.0F, policy),
        255.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "A value just below the raw maximum should already saturate once upper-percentile clipping is applied.");
    require(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(1500.0F, 0.0F, 255.0F, policy) >
            (1500.0F - 1.0F) * unclippedScale,
        "Upper-percentile clipping should stretch mid-high intensities above the plain min-max baseline.");
}

void compute_scale_policy_should_scale_mri_like_distribution_without_artifact_dominance()
{
    std::vector<float> sourceData;
    sourceData.reserve(6602);
    appendRepeatedValues(sourceData, 3000, 0.0F);
    appendRepeatedValues(sourceData, 1200, 35.0F);
    appendRepeatedValues(sourceData, 1200, 85.0F);
    appendRepeatedValues(sourceData, 1200, 125.0F);
    appendRepeatedValues(sourceData, 2, 4000.0F);

    const OpenHC::imaging::mri::fastsurfer::ScalePolicy policy = OpenHC::imaging::mri::fastsurfer::computeScalePolicy(sourceData, 0.0F, 255.0F);
    const float csfLikeValue = OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(35.0F, 0.0F, 255.0F, policy);
    const float gmLikeValue = OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(85.0F, 0.0F, 255.0F, policy);
    const float wmLikeValue = OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(125.0F, 0.0F, 255.0F, policy);
    const float artifactValue = OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(4000.0F, 0.0F, 255.0F, policy);

    requireNear(
        policy.srcMin,
        0.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Background zeros should remain the scale-policy minimum for MRI-like intensity distributions.");
    requireNear(
        OpenHC::imaging::mri::fastsurfer::applyScalePolicyValue(0.0F, 0.0F, 255.0F, policy),
        0.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Background voxels should stay at exact zero after scaling.");
    require(
        csfLikeValue > 0.0F && csfLikeValue < gmLikeValue && gmLikeValue < wmLikeValue,
        "Foreground tissue clusters should remain ordered after scaling.");
    require(
        gmLikeValue > 100.0F,
        "Sparse bright artifacts should not collapse normal MRI foreground contrast into the bottom of the uint8 range.");
    requireNear(
        wmLikeValue,
        255.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Upper foreground tissue should saturate once the policy clips sparse bright artifacts.");
    requireNear(
        artifactValue,
        255.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Bright artifacts should clamp to the destination maximum.");
}

void runNamedCase(const std::string &caseName)
{
    if (caseName == "threshold") {
        compute_target_voxel_size_should_respect_threshold_cases();
        return;
    }

    if (caseName == "auto-vs-fov") {
        compute_native_target_dimensions_should_distinguish_auto_from_fov();
        return;
    }

    if (caseName == "anisotropic-auto") {
        compute_native_target_dimensions_should_cube_anisotropic_auto_size();
        return;
    }

    if (caseName == "small-auto-floor-256") {
        compute_native_target_dimensions_should_floor_small_auto_size_to_256_cube();
        return;
    }

    if (caseName == "mode-parsing") {
        parse_conform_modes_should_round_trip_supported_values_and_reject_invalid_input();
        return;
    }

    if (caseName == "scale-policy") {
        compute_scale_policy_should_handle_constant_inputs_and_apply_scale_consistently();
        return;
    }

    if (caseName == "scale-zero-epsilon") {
        apply_scale_policy_should_respect_zero_epsilon_boundary();
        return;
    }

    if (caseName == "scale-histogram-collapse") {
        compute_scale_policy_should_fallback_to_unit_scale_when_histogram_width_collapses();
        return;
    }

    if (caseName == "scale-upper-outliers") {
        compute_scale_policy_should_clip_upper_percentile_outliers();
        return;
    }

    if (caseName == "scale-mri-like") {
        compute_scale_policy_should_scale_mri_like_distribution_without_artifact_dominance();
        return;
    }

    throw std::runtime_error("Unknown conform policy test case: " + caseName);
}

} // namespace

int main(int argc, char **argv)
{
    try {
        if (argc != 2) {
            throw std::runtime_error("Expected exactly one test case argument.");
        }

        runNamedCase(argv[1]);
        return 0;
    } catch (const std::exception &error) {
        return (void)std::fprintf(stderr, "%s\n", error.what()), 1;
    }
}