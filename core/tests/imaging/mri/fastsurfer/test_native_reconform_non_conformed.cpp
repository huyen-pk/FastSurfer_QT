#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestConstants.h"
#include "TestFixtureBuilders.h"
#include "TestHelpers.h"
#include "TestParitySupport.h"
#include "imaging/mri/fastsurfer/step_conform_request.h"
#include "imaging/mri/fastsurfer/step_conform.h"
#include "imaging/common/mgh_image.h"

namespace ohc = OpenHC::imaging::mri::fastsurfer;
namespace oht = OpenHC::tests::fastsurfer::support;

namespace {

// assertion helpers are provided by TestHelpers.h

// Returns true when all spacing components are within parity tolerance of target.
// Parameters:
// - spacing: Spacing triplet to validate.
// - target: Expected isotropic spacing.
bool allSpacingClose(const std::array<float, 3> &spacing, const float target)
{
    return std::all_of(spacing.begin(), spacing.end(), [target](const float value) {
        return std::fabs(value - target) <= test_constants::PARITY_SPACING_TOLERANCE;
    });
}

} // namespace

// Verifies reconforming behavior on a synthetic non-conformed MGZ input.
// Returns 0 on success and 1 on failure.
int main()
{
    try {
        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "data/Subject140/140_orig.mgz";
        const std::filesystem::path outputDir = oht::makeFreshDirectory("openhc_imaging_mri_fastsurfer_native_reconform_test");

        const auto inputPath = outputDir / "synthetic_input.mgz";
        const auto copyPath = outputDir / "copy_orig.mgz";
        const auto conformedPath = outputDir / "conformed_orig.mgz";

        const auto syntheticInput = oht::createSyntheticNonConformedInput(
            fixturePath,
            oht::makeSubject140NonConformedInputSpec());
        syntheticInput.save(inputPath);

        ohc::ConformStepRequest request;
        request.inputPath = inputPath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;
        request.imageSizeMode = OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Fov;

        request.imageSizeMode = ohc::ImageSizeMode::Fov;

        ohc::ConformStepService service;
        const auto result = service.run(request);

        require(result.success, "The native reconform step did not succeed for the synthetic MGZ input.");
        require(!result.alreadyConformed, "The synthetic MGZ input should require native reconforming.");
        require(std::filesystem::exists(copyPath), "The native reconform step did not write the original copy output.");
        require(std::filesystem::exists(conformedPath), "The native reconform step did not write the conformed MGZ output.");

        const auto copiedImage = ohc::MghImage::load(copyPath);
        const auto outputImage = ohc::MghImage::load(conformedPath);

        require(copiedImage.rawData() == syntheticInput.rawData(), "The original-copy output does not match the synthetic input payload.");
        require(outputImage.header().type == static_cast<std::int32_t>(ohc::MghDataType::UChar),
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