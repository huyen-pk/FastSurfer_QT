#include "fastsurfer/core/conform_policy.h"

#include <cmath>
#include <cstdio>
#include "TestConstants.h"
#include "TestHelpers.h"
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "fastsurfer/core/mgh_image.h"
#include "fastsurfer/core/step_conform_request.h"

namespace {

// Assertion helpers are provided by TestHelpers.h

fastsurfer::core::MghImage makeImage(
    const std::array<int, 3> &dimensions,
    const std::array<float, 3> &spacing)
{
    fastsurfer::core::MghImage::Header header;
    header.dimensions = dimensions;
    header.spacing = spacing;

    const std::size_t voxelCount = static_cast<std::size_t>(dimensions[0]) *
                                   static_cast<std::size_t>(dimensions[1]) *
                                   static_cast<std::size_t>(dimensions[2]);
    return fastsurfer::core::MghImage::fromVoxelData(
        header,
        std::vector<float>(voxelCount, 0.0F),
        static_cast<std::int32_t>(fastsurfer::core::MghDataType::Float32));
}

void compute_target_voxel_size_should_respect_threshold_cases()
{
    fastsurfer::core::ConformStepRequest request;

    const auto belowThresholdImage = makeImage({32, 32, 32}, {0.8F, 1.1F, 1.2F});
    requireNear(
        fastsurfer::core::computeTargetVoxelSize(belowThresholdImage, request),
        0.8F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Voxel-size policy should keep the minimum voxel size when it is below the threshold.");

    const auto aboveThresholdImage = makeImage({32, 32, 32}, {0.96F, 1.1F, 1.2F});
    requireNear(
        fastsurfer::core::computeTargetVoxelSize(aboveThresholdImage, request),
        1.0F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Voxel-size policy should promote the target voxel size to 1mm when it is above the threshold.");

    const auto boundaryImage = makeImage({32, 32, 32}, {0.95F, 1.1F, 1.2F});
    requireNear(
        fastsurfer::core::computeTargetVoxelSize(boundaryImage, request),
        0.95F,
        test_constants::CONFORM_POLICY_VOXEL_TOLERANCE,
        "Voxel-size policy should preserve the exact threshold boundary because the comparison is strict-greater-than.");
}

void compute_native_target_dimensions_should_distinguish_auto_from_fov()
{
    const auto image = makeImage({300, 240, 180}, {0.8F, 0.8F, 1.0F});

    const auto autoDimensions = fastsurfer::core::computeNativeTargetDimensions(image, 0.8F, fastsurfer::core::ImageSizeMode::Auto);
    require(
        autoDimensions == std::array<int, 3> {300, 300, 300},
        "Auto image-size mode should cube the target to the largest derived dimension.");

    const auto fovDimensions = fastsurfer::core::computeNativeTargetDimensions(image, 0.8F, fastsurfer::core::ImageSizeMode::Fov);
    require(
        fovDimensions == std::array<int, 3> {300, 240, 225},
        "FOV image-size mode should preserve per-axis target dimensions.");
}

void compute_native_target_dimensions_should_cube_anisotropic_auto_size()
{
    const auto image = makeImage({320, 320, 180}, {0.8F, 0.8F, 1.0F});

    const auto autoDimensions = fastsurfer::core::computeNativeTargetDimensions(image, 0.8F, fastsurfer::core::ImageSizeMode::Auto);
    require(
        autoDimensions == std::array<int, 3> {320, 320, 320},
        "Auto image-size mode should expand anisotropic FOV-derived dimensions into a cube using the maximum axis.");
}

void compute_native_target_dimensions_should_floor_small_auto_size_to_256_cube()
{
    const auto image = makeImage({120, 110, 90}, {0.8F, 0.8F, 0.8F});

    const auto autoDimensions = fastsurfer::core::computeNativeTargetDimensions(image, 0.8F, fastsurfer::core::ImageSizeMode::Auto);
    require(
        autoDimensions == std::array<int, 3> {256, 256, 256},
        "Auto image-size mode should retain the minimum 256^3 cube for small fields of view.");
}

void parse_conform_modes_should_round_trip_supported_values_and_reject_invalid_input()
{
    require(
        fastsurfer::core::parseVoxelSizeMode("min") == fastsurfer::core::VoxelSizeMode::Min,
        "Voxel-size mode parser should accept the supported 'min' value.");
    require(
        fastsurfer::core::parseImageSizeMode("auto") == fastsurfer::core::ImageSizeMode::Auto,
        "Image-size mode parser should accept the supported 'auto' value.");
    require(
        fastsurfer::core::parseImageSizeMode("fov") == fastsurfer::core::ImageSizeMode::Fov,
        "Image-size mode parser should accept the supported 'fov' value.");
    require(
        fastsurfer::core::to_string(fastsurfer::core::VoxelSizeMode::Min) == "min",
        "Voxel-size mode string conversion should preserve the public text form.");
    require(
        fastsurfer::core::to_string(fastsurfer::core::ImageSizeMode::Auto) == "auto" &&
            fastsurfer::core::to_string(fastsurfer::core::ImageSizeMode::Fov) == "fov",
        "Image-size mode string conversion should preserve the public text forms.");
    require(
        fastsurfer::core::parseOrientationMode("lia") == fastsurfer::core::OrientationMode::Lia &&
            fastsurfer::core::parseOrientationMode("native") == fastsurfer::core::OrientationMode::Native,
        "Orientation mode parser should accept the supported request values.");
    require(
        fastsurfer::core::to_string(fastsurfer::core::OrientationMode::Lia) == "lia" &&
            fastsurfer::core::to_string(fastsurfer::core::OrientationMode::Native) == "native",
        "Orientation mode string conversion should preserve the public text forms.");

    bool invalidVoxelModeRejected = false;
    try {
        static_cast<void>(fastsurfer::core::parseVoxelSizeMode("bogus"));
    } catch (const std::invalid_argument &) {
        invalidVoxelModeRejected = true;
    }
    require(invalidVoxelModeRejected, "Voxel-size mode parser should reject unsupported input.");

    bool invalidImageModeRejected = false;
    try {
        static_cast<void>(fastsurfer::core::parseImageSizeMode("bogus"));
    } catch (const std::invalid_argument &) {
        invalidImageModeRejected = true;
    }
    require(invalidImageModeRejected, "Image-size mode parser should reject unsupported input.");

    bool invalidOrientationModeRejected = false;
    try {
        static_cast<void>(fastsurfer::core::parseOrientationMode("bogus"));
    } catch (const std::invalid_argument &) {
        invalidOrientationModeRejected = true;
    }
    require(invalidOrientationModeRejected, "Orientation mode parser should reject unsupported input.");
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