#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "fastsurfer/core/conform_step_request.h"
#include "fastsurfer/core/conform_step_service.h"
#include "fastsurfer/core/mgh_image.h"
#include "fastsurfer/core/nifti_converter.h"

namespace {

std::filesystem::path makeFreshDirectory(const std::string &name)
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

void requireNear(const float left, const float right, const float tolerance, const std::string &message)
{
    if (std::fabs(left - right) > tolerance) {
        throw std::runtime_error(message + " Expected=" + std::to_string(right) + " actual=" + std::to_string(left));
    }
}

void assertAffineClose(const fastsurfer::core::Matrix4 &left, const fastsurfer::core::Matrix4 &right)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            if (std::fabs(left[row][column] - right[row][column]) > 1.0e-4) {
                throw std::runtime_error("Copy-orig MGZ affine differs from the direct NIfTI conversion affine.");
            }
        }
    }
}

} // namespace

int main()
{
    try {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const auto fixturePath = repoRoot / "data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii";
        const auto outputDir = makeFreshDirectory("fastsurfer_core_conform_step_nifti_test");

        const auto copyPath = outputDir / "copy_orig.mgz";
        const auto conformedPath = outputDir / "conformed_orig.mgz";
        const auto expectedCopy = fastsurfer::core::NiftiConverter::loadAsMgh(fixturePath);

        fastsurfer::core::ConformStepRequest request;
        request.inputPath = fixturePath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;

        fastsurfer::core::ConformStepService service;
        const auto result = service.run(request);

        require(result.success, "The conform step failed for the NIfTI fixture.");
        require(!result.alreadyConformed, "The oblique NIfTI fixture should exercise reconforming, not the already-conformed shortcut.");
        require(std::filesystem::exists(copyPath), "The conform step did not write the copy-orig MGZ output for the NIfTI input.");
        require(std::filesystem::exists(conformedPath), "The conform step did not write the conformed MGZ output for the NIfTI input.");

        require(result.inputMetadata.dimensions == std::array<int, 3> {320, 320, 180},
                "The conform step reported unexpected input dimensions for the NIfTI fixture.");
        require(result.inputMetadata.orientationCode == "RAS",
                "The conform step reported an unexpected input orientation for the NIfTI fixture.");
        require(result.inputMetadata.dataTypeName == expectedCopy.metadata().dataTypeName,
            "The conform step reported an unexpected input data type for the NIfTI fixture.");
        requireNear(result.inputMetadata.spacing[0], 0.8F, 1.0e-5F,
                    "The conform step reported unexpected X spacing for the NIfTI fixture.");
        requireNear(result.inputMetadata.spacing[1], 0.8F, 1.0e-5F,
                    "The conform step reported unexpected Y spacing for the NIfTI fixture.");
        requireNear(result.inputMetadata.spacing[2], 1.0F, 1.0e-5F,
                    "The conform step reported unexpected Z spacing for the NIfTI fixture.");

        const auto copyImage = fastsurfer::core::MghImage::load(copyPath);
        const auto conformedImage = fastsurfer::core::MghImage::load(conformedPath);

        require(copyImage.rawData() == expectedCopy.rawData(),
                "The copy-orig output differs from the direct NIfTI-to-MGZ conversion payload.");
        assertAffineClose(copyImage.affine(), expectedCopy.affine());

        require(conformedImage.hasSingleFrame(), "The conformed NIfTI output should remain single-frame.");
        require(conformedImage.isUint8(), "The conformed NIfTI output should be saved as uint8 MGZ.");
        require(conformedImage.orientationCode() == "LIA",
                "The conformed NIfTI output should be written in LIA orientation.");
        require(conformedImage.hasDimensions({320, 320, 320}),
                "The conformed NIfTI output should be padded to the expected cube dimensions.");
        require(conformedImage.hasIsotropicSpacing(0.8F, 1.0e-5F),
                "The conformed NIfTI output should use isotropic 0.8 mm spacing.");

        require(result.outputMetadata.dimensions == std::array<int, 3> {320, 320, 320},
                "The conform step reported unexpected output dimensions for the NIfTI fixture.");
        require(result.outputMetadata.orientationCode == "LIA",
                "The conform step reported an unexpected output orientation for the NIfTI fixture.");
        require(result.outputMetadata.dataTypeName == "uint8",
                "The conform step reported an unexpected output data type for the NIfTI fixture.");

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}