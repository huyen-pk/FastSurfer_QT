#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestConstants.h"
#include "fastsurfer/core/step_conform_request.h"
#include "fastsurfer/core/step_conform.h"
#include "fastsurfer/core/mgh_image.h"

namespace {

std::filesystem::path makeUniqueDirectory(const std::string &name)
{
    const auto root = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    return root;
}

void require(const bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

fastsurfer::core::MghImage createSyntheticNonConformedInput(const std::filesystem::path &fixturePath)
{
    const auto sourceImage = fastsurfer::core::MghImage::load(fixturePath);
    const auto sourceData = sourceImage.voxelDataAsFloat();
    require(!sourceData.empty(), "The fixture MGZ appears empty or unreadable: " + fixturePath.string());

    fastsurfer::core::MghImage::Header header = sourceImage.header();
    header.dimensions = {64, 72, 80};
    header.frames = 1;
    header.type = static_cast<std::int32_t>(fastsurfer::core::MghDataType::Float32);
    header.spacing = {1.1F, 1.2F, 0.9F};
    header.directionCosines = {
        1.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 1.0F,
    };
    header.center = {14.0F, -9.5F, 22.25F};

    std::vector<float> cropped(static_cast<std::size_t>(header.dimensions[0]) *
                               static_cast<std::size_t>(header.dimensions[1]) *
                               static_cast<std::size_t>(header.dimensions[2]));

    const auto sourceDimensions = sourceImage.header().dimensions;
    const int offsetX = (sourceDimensions[0] - header.dimensions[0]) / 2;
    const int offsetY = (sourceDimensions[1] - header.dimensions[1]) / 2;
    const int offsetZ = (sourceDimensions[2] - header.dimensions[2]) / 2;

    for (int z = 0; z < header.dimensions[2]; ++z) {
        for (int y = 0; y < header.dimensions[1]; ++y) {
            for (int x = 0; x < header.dimensions[0]; ++x) {
                const std::size_t sourceIndex =
                    (static_cast<std::size_t>(z + offsetZ) * static_cast<std::size_t>(sourceDimensions[1]) +
                     static_cast<std::size_t>(y + offsetY)) *
                        static_cast<std::size_t>(sourceDimensions[0]) +
                    static_cast<std::size_t>(x + offsetX);
                const std::size_t targetIndex =
                    (static_cast<std::size_t>(z) * static_cast<std::size_t>(header.dimensions[1]) + static_cast<std::size_t>(y)) *
                        static_cast<std::size_t>(header.dimensions[0]) +
                    static_cast<std::size_t>(x);
                cropped[targetIndex] = sourceData[sourceIndex];
            }
        }
    }

    return fastsurfer::core::MghImage::fromVoxelData(header, cropped, header.type);
}

bool allSpacingClose(const std::array<float, 3> &spacing, const float target)
{
    return std::all_of(spacing.begin(), spacing.end(), [target](const float value) {
        return std::fabs(value - target) <= test_constants::PARITY_SPACING_TOLERANCE;
    });
}

} // namespace

int main()
{
    try {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "data/Subject140/140_orig.mgz";
        const std::filesystem::path outputDir = makeUniqueDirectory("fastsurfer_core_native_reconform_test");

        const auto inputPath = outputDir / "synthetic_input.mgz";
        const auto copyPath = outputDir / "copy_orig.mgz";
        const auto conformedPath = outputDir / "conformed_orig.mgz";

        const auto syntheticInput = createSyntheticNonConformedInput(fixturePath);
        syntheticInput.save(inputPath);

        fastsurfer::core::ConformStepRequest request;
        request.inputPath = inputPath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;
        request.imageSizeMode = "fov";

        fastsurfer::core::ConformStepService service;
        const auto result = service.run(request);

        require(result.success, "The native reconform step did not succeed for the synthetic MGZ input.");
        require(!result.alreadyConformed, "The synthetic MGZ input should require native reconforming.");
        require(std::filesystem::exists(copyPath), "The native reconform step did not write the original copy output.");
        require(std::filesystem::exists(conformedPath), "The native reconform step did not write the conformed MGZ output.");

        const auto copiedImage = fastsurfer::core::MghImage::load(copyPath);
        const auto outputImage = fastsurfer::core::MghImage::load(conformedPath);

        require(copiedImage.rawData() == syntheticInput.rawData(), "The original-copy output does not match the synthetic input payload.");
        require(outputImage.header().type == static_cast<std::int32_t>(fastsurfer::core::MghDataType::UChar),
                "The reconformed output is not stored as uint8.");
        require(outputImage.orientationCode() == "LIA", "The reconformed output is not in LIA orientation.");
        require(allSpacingClose(outputImage.header().spacing, 0.9F), "The reconformed output spacing is not isotropic 0.9 mm.");
        require(outputImage.header().dimensions == std::array<int, 3> {79, 80, 96},
                "The reconformed output dimensions do not preserve the source field of view in fov mode.");

        const auto outputData = outputImage.voxelDataAsFloat();
        require(std::any_of(outputData.begin(), outputData.end(), [](const float value) { return value > 0.0F; }),
                "The reconformed output is unexpectedly empty.");

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}