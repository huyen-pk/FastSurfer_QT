#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestConstants.h"
#include "TestHelpers.h"
#include "imaging/mri/fastsurfer/mgh_image.h"
#include "imaging/mri/fastsurfer/nifti_converter.h"

namespace {

struct VoxelExpectation {
    std::array<int, 3> index;
    float value;
};

struct FixtureExpectation {
    std::string relativePath;
    std::array<int, 3> dimensions;
    std::array<float, 3> spacing;
    std::string orientation;
    OpenHC::imaging::mri::fastsurfer::Matrix4 affine;
    std::vector<VoxelExpectation> voxels;
};

std::filesystem::path makeFreshDirectory(const std::string &name)
{
    if (const char *envTmp = std::getenv("FASTSURFER_TEST_TMPDIR"); envTmp != nullptr && envTmp[0] != '\0') {
        const auto root = std::filesystem::path(envTmp) / name;
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);
        return root;
    }

    const auto root = std::filesystem::temp_directory_path() / std::string("fastsurfer_tests") / name;
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    return root;
}

// assertion helpers are provided by TestHelpers.h

void assertAffineClose(const OpenHC::imaging::mri::fastsurfer::Matrix4 &actual, const OpenHC::imaging::mri::fastsurfer::Matrix4 &expected)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            requireNear(actual[row][column], expected[row][column], test_constants::AFFINE_LINEAR_TOLERANCE,
                        "Converted MGZ affine does not match the expected RAS-space affine.");
        }
    }
}

float voxelAt(const OpenHC::imaging::mri::fastsurfer::MghImage &image, const std::array<int, 3> &index)
{
    const auto dims = image.header().dimensions;
    const std::size_t flatIndex =
        (static_cast<std::size_t>(index[2]) * static_cast<std::size_t>(dims[1]) + static_cast<std::size_t>(index[1])) *
            static_cast<std::size_t>(dims[0]) +
        static_cast<std::size_t>(index[0]);
    return image.voxelDataAsFloat().at(flatIndex);
}

std::vector<FixtureExpectation> fixtureExpectations()
{
    return {
        FixtureExpectation {
            "data/parrec_oblique/NIFTI/T1W_trans_0_0_0_SENSE_4_1.nii",
            {560, 560, 1},
            {0.42857143F, 0.42857143F, 5.0F},
            "LPS",
            OpenHC::imaging::mri::fastsurfer::Matrix4 {{
                {{-0.428571433, 0.0, 0.0, 122.404823303}},
                {{0.0, -0.428571433, 0.0, 130.220428467}},
                {{0.0, 0.0, 5.0, 0.0}},
                {{0.0, 0.0, 0.0, 1.0}},
            }},
            {{
                {{0, 0, 0}, 0.0F},
                {{280, 280, 0}, 1371.08984F},
                {{559, 559, 0}, 0.0F},
            }},
        },
        FixtureExpectation {
            "data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii",
            {320, 320, 180},
            {0.8F, 0.8F, 1.0F},
            "LPS",
            OpenHC::imaging::mri::fastsurfer::Matrix4 {{
                {{-0.632992056, 0.169609815, -0.573576472, 126.919281006}},
                {{-0.374971205, -0.650149859, 0.346188561, 133.138259888}},
                {{-0.314193685, 0.434209270, 0.742403873, -74.587806702}},
                {{0.0, 0.0, 0.0, 1.0}},
            }},
            {{
                {{0, 0, 0}, 0.0F},
                {{160, 160, 90}, 1475.80127F},
                {{319, 319, 179}, 0.0F},
            }},
        },
    };
}

} // namespace

int main()
{
    try {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const std::filesystem::path outputDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_nifti_converter_test");

        for (const FixtureExpectation &fixture : fixtureExpectations()) {
            const auto inputPath = repoRoot / fixture.relativePath;
            const auto outputPath = outputDir / (std::filesystem::path(fixture.relativePath).stem().string() + ".mgz");

            require(OpenHC::imaging::mri::fastsurfer::NiftiConverter::isSupportedPath(inputPath),
                    "Expected the fixture path to be recognized as NIfTI input.");

            const auto convertedInMemory = OpenHC::imaging::mri::fastsurfer::NiftiConverter::loadAsMgh(inputPath);
            OpenHC::imaging::mri::fastsurfer::NiftiConverter::convertToMgh(inputPath, outputPath);
            require(std::filesystem::exists(outputPath), "The NIfTI converter did not write an MGZ output file.");

            const auto reloaded = OpenHC::imaging::mri::fastsurfer::MghImage::load(outputPath);

            require(reloaded.header().dimensions == fixture.dimensions,
                    "Converted MGZ dimensions differ from the expected fixture geometry.");
                require(reloaded.header().type == convertedInMemory.header().type,
                    "In-memory and saved NIfTI conversions produced different storage types.");
            for (int axis = 0; axis < 3; ++axis) {
                requireNear(reloaded.header().spacing[axis], fixture.spacing[axis], test_constants::METADATA_SPACING_TOLERANCE,
                            "Converted MGZ spacing differs from the expected fixture spacing.");
            }
            require(reloaded.orientationCode() == fixture.orientation,
                    "Converted MGZ orientation code differs from the expected RAS orientation.");
            assertAffineClose(reloaded.affine(), fixture.affine);

                require(convertedInMemory.header().dimensions == reloaded.header().dimensions,
                    "In-memory and saved NIfTI conversions produced different dimensions.");
            require(convertedInMemory.rawData() == reloaded.rawData(),
                    "In-memory and saved NIfTI conversions produced different voxel payloads.");

            for (const VoxelExpectation &voxel : fixture.voxels) {
                requireNear(voxelAt(reloaded, voxel.index), voxel.value, test_constants::VOXEL_EXPECTATION_TOLERANCE,
                            "Converted MGZ voxel value differs from the selected fixture sample.");
            }
        }

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}