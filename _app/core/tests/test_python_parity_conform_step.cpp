#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fastsurfer/core/step_conform_request.h"
#include "fastsurfer/core/step_conform.h"
#include "fastsurfer/core/mgh_image.h"
#include <format>

namespace {

std::filesystem::path makeFreshDirectory(const std::string &name)
{
    const auto root = [&]() {
        if (const char *envTmp = std::getenv("FASTSURFER_TEST_TMPDIR"); envTmp != nullptr && envTmp[0] != '\0') {
            return std::filesystem::path(envTmp) / name;
        }

        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        return repoRoot / ".tmp" / name;
    }();

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    ec.clear();
    std::filesystem::create_directories(root, ec);
    if (ec || !std::filesystem::exists(root)) {
        throw std::runtime_error("Failed to create temporary test directory: " + root.string());
    }

    return root;
}

void require(const bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string shellEscape(const std::filesystem::path &path)
{
    std::string value = path.string();
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('\'');
    for (const char character : value) {
        if (character == '\'') {
            escaped += "'\\''";
        } else {
            escaped.push_back(character);
        }
    }
    escaped.push_back('\'');
    return escaped;
}

std::filesystem::path resolvePythonExecutable(const std::filesystem::path &repoRoot)
{
    if (const char *configured = std::getenv("FASTSURFER_PYTHON_EXECUTABLE"); configured != nullptr && configured[0] != '\0') {
        return configured;
    }

    const auto workspaceVenv = repoRoot / ".venv-parity" / "bin" / "python";
    if (std::filesystem::exists(workspaceVenv)) {
        return workspaceVenv;
    }

    const auto repoVenv = repoRoot / ".venv" / "bin" / "python";
    if (std::filesystem::exists(repoVenv)) {
        return repoVenv;
    }

    return "python3";
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

void assertAffineClose(
    const fastsurfer::core::Matrix4 &left,
    const fastsurfer::core::Matrix4 &right,
    const double translationToleranceMm = 1.0e-4)
{
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            require(std::fabs(left[row][column] - right[row][column]) <= 1.0e-4,
                    "Affine linear terms differ between native and Python outputs.");
        }

        require(std::fabs(left[row][3] - right[row][3]) <= translationToleranceMm,
                std::format("Affine translation differs from Python by more than {} millimeters.", translationToleranceMm));
    }

    require(std::fabs(left[3][0] - right[3][0]) <= 1.0e-6 &&
                std::fabs(left[3][1] - right[3][1]) <= 1.0e-6 &&
                std::fabs(left[3][2] - right[3][2]) <= 1.0e-6 &&
                std::fabs(left[3][3] - right[3][3]) <= 1.0e-6,
            "Affine homogeneous row differs between native and Python outputs.");
}

void assertExactImageMatch(const std::filesystem::path &leftPath, const std::filesystem::path &rightPath)
{
    const auto leftImage = fastsurfer::core::MghImage::load(leftPath);
    const auto rightImage = fastsurfer::core::MghImage::load(rightPath);

    require(leftImage.header().dimensions == rightImage.header().dimensions,
            "Image dimensions differ between native and Python outputs.");
    require(leftImage.header().frames == rightImage.header().frames,
            "Frame counts differ between native and Python outputs.");
    require(leftImage.header().type == rightImage.header().type,
            "Data types differ between native and Python outputs.");
        require(leftImage.orientationCode() == rightImage.orientationCode(),
            "Output orientations differ between native and Python outputs.");
        for (int axis = 0; axis < 3; ++axis) {
        require(std::fabs(leftImage.header().spacing[axis] - rightImage.header().spacing[axis]) <= 1.0e-4F,
            "Output spacing differs between native and Python outputs.");
        }
        assertAffineClose(leftImage.affine(), rightImage.affine());

        const auto leftData = leftImage.voxelDataAsFloat();
        const auto rightData = rightImage.voxelDataAsFloat();
        require(leftData == rightData, "Voxel payload differs between native and Python outputs.");
    }

    void assertComparableConformedImages(
        const std::filesystem::path &leftPath,
        const std::filesystem::path &rightPath,
        const double translationToleranceMm = 1.0e-4,
        const double meanAbsoluteDifferenceTolerance = 1.0)
    {
        const auto leftImage = fastsurfer::core::MghImage::load(leftPath);
        const auto rightImage = fastsurfer::core::MghImage::load(rightPath);

        require(leftImage.header().dimensions == rightImage.header().dimensions,
                "Image dimensions differ between native and Python outputs.");
        require(leftImage.header().frames == rightImage.header().frames,
                "Frame counts differ between native and Python outputs.");
        require(leftImage.header().type == rightImage.header().type,
                "Data types differ between native and Python outputs.");
        require(leftImage.orientationCode() == rightImage.orientationCode(),
                "Output orientations differ between native and Python outputs.");
        for (int axis = 0; axis < 3; ++axis) {
            require(std::fabs(leftImage.header().spacing[axis] - rightImage.header().spacing[axis]) <= 1.0e-4F,
                    "Output spacing differs between native and Python outputs.");
        }
        assertAffineClose(leftImage.affine(), rightImage.affine(), translationToleranceMm);

        const auto leftData = leftImage.voxelDataAsFloat();
        const auto rightData = rightImage.voxelDataAsFloat();
        require(leftData.size() == rightData.size(), "Voxel counts differ between native and Python outputs.");
        double totalDifference = 0.0;
        std::size_t foregroundCount = 0;
        double maxDiff = 0.0;

        for (std::size_t i = 0; i < leftData.size(); ++i) {
            double diff = std::fabs(static_cast<double>(leftData[i]) - static_cast<double>(rightData[i]));
            
            // Update Max Diff
            if (diff > maxDiff) maxDiff = diff;

            // Only count differences where there is actual signal (brain tissue)
            if (leftData[i] > 0 || rightData[i] > 0) {
                totalDifference += diff;
                foregroundCount++;
            }
        }

        const double foregroundMAD = totalDifference / static_cast<double>(foregroundCount);
        require(foregroundMAD <= 0.5, "Brain tissue intensity diverges too much.");
        require(maxDiff <= 2.0, "Localized intensity error is too high.");

        const double meanAbsoluteDifference = totalDifference / static_cast<double>(leftData.size());
        require(meanAbsoluteDifference <= meanAbsoluteDifferenceTolerance,
                "Voxel payload diverges from Python beyond the allowed mean absolute difference.");
    }

    std::string shellEscapeString(const std::string &value)
    {
        std::string escaped;
        escaped.reserve(value.size() + 2);
        escaped.push_back('\'');
        for (const char character : value) {
            if (character == '\'') {
                escaped += "'\\''";
            } else {
                escaped.push_back(character);
            }
        }
        escaped.push_back('\'');
        return escaped;
    }

    int runCommand(const std::string &command)
    {
        return std::system(command.c_str());
    }

    void test_step_conform_standardized_image_parity()
    {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "data/Subject140/140_orig.mgz";
        const auto pythonExecutable = resolvePythonExecutable(repoRoot);

        const auto nativeDir = makeFreshDirectory("fastsurfer_core_native_subject140");
        const auto pythonDir = makeFreshDirectory("fastsurfer_core_python_subject140");

        const auto syntheticInputPath = nativeDir / "synthetic_non_conformed_input.mgz";
        const auto nativeCopy = nativeDir / "copy_orig.mgz";
        const auto nativeConformed = nativeDir / "conformed_orig.mgz";
        const auto pythonCopy = pythonDir / "copy_orig.mgz";
        const auto pythonConformed = pythonDir / "conformed_orig.mgz";
        const auto pythonScript = pythonDir / "run_python_reference.py";

        const auto syntheticInput = createSyntheticNonConformedInput(fixturePath);
        syntheticInput.save(syntheticInputPath);

        {
            std::ofstream script(pythonScript);
            script
                << "import sys\n"
                << "from pathlib import Path\n"
                << "import nibabel as nib\n"
                << "import numpy as np\n"
                << "repo_root = Path(sys.argv[1])\n"
                << "input_path = Path(sys.argv[2])\n"
                << "copy_path = Path(sys.argv[3])\n"
                << "conformed_path = Path(sys.argv[4])\n"
                << "sys.path.insert(0, str(repo_root / 'baseline' / 'FastSurfer'))\n"
                << "from FastSurferCNN.data_loader.conform import conform, is_conform\n"
                << "orig = nib.load(str(input_path))\n"
                << "orig_data = np.asarray(orig.dataobj)\n"
                << "copy_image = nib.MGHImage(orig_data, orig.affine, orig.header.copy())\n"
                << "copy_image.set_data_dtype(orig_data.dtype)\n"
                << "nib.save(copy_image, str(copy_path))\n"
                << "kwargs = {'threshold_1mm': 0.95, 'vox_size': 'min', 'orientation': 'lia', 'img_size': 'fov'}\n"
                << "if not is_conform(orig, verbose=False, **kwargs):\n"
                << "    orig = conform(orig, **kwargs)\n"
                << "    orig_data = np.asanyarray(orig.dataobj)\n"
                << "else:\n"
                << "    orig_data = np.asanyarray(orig.dataobj)\n"
                << "conformed_image = nib.MGHImage(orig_data, orig.affine, orig.header.copy())\n"
                << "conformed_image.set_data_dtype(np.uint8)\n"
                << "nib.save(conformed_image, str(conformed_path))\n";
        }

        fastsurfer::core::ConformStepRequest request;
        request.inputPath = syntheticInputPath;
        request.copyOrigPath = nativeCopy;
        request.conformedPath = nativeConformed;
        request.imageSizeMode = "fov";

        fastsurfer::core::ConformStepService service;
        const auto nativeResult = service.run(request);
        require(nativeResult.success, "The native conform step did not succeed for the parity test.");
        require(!nativeResult.alreadyConformed, "The parity input should exercise native reconforming, not the already-conformed shortcut.");

        const std::string command =
            shellEscape(pythonExecutable) + " " + shellEscape(pythonScript) +
            " " + shellEscape(repoRoot) +
            " " + shellEscape(syntheticInputPath) +
            " " + shellEscape(pythonCopy) +
            " " + shellEscape(pythonConformed);

        const int exitCode = runCommand(command);
        require(exitCode == 0,
                "The Python reference execution failed during the parity test. Set FASTSURFER_PYTHON_EXECUTABLE or create .venv-parity with numpy and nibabel.");

        require(std::filesystem::exists(pythonCopy), "The Python reference did not create the copy_orig output.");
        require(std::filesystem::exists(pythonConformed), "The Python reference did not create the conformed output.");

        assertExactImageMatch(nativeCopy, pythonCopy);
        assertComparableConformedImages(nativeConformed, pythonConformed);
    }

    void test_step_conform_oblique_image_parity()
    {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii";
        const std::filesystem::path artifactRoot =
            repoRoot / "generated/FastSurferCNN/parrec_oblique_3d_t1w_trans_35_25_15_sense_26_1_cpp_parity" /
            "step_02_conform_input_image_and_save_conformed_orig";
        const std::filesystem::path pythonCopy = artifactRoot / "input_copy.mgz";
        const std::filesystem::path pythonConformed = artifactRoot / "conformed_orig.mgz";

        require(std::filesystem::exists(pythonCopy),
                "Missing generated Python oblique copy artifact. Regenerate with tools/generate_fastsurfercnn_intermediate_outputs.py --no-image-size --stop-after-step 3.");
        require(std::filesystem::exists(pythonConformed),
                "Missing generated Python oblique conformed artifact. Regenerate with tools/generate_fastsurfercnn_intermediate_outputs.py --no-image-size --stop-after-step 3.");

        const auto nativeDir = makeFreshDirectory("fastsurfer_core_native_oblique_parity");
        const auto nativeCopy = nativeDir / "copy_orig.mgz";
        const auto nativeConformed = nativeDir / "conformed_orig.mgz";

        fastsurfer::core::ConformStepRequest request;
        request.inputPath = fixturePath;
        request.copyOrigPath = nativeCopy;
        request.conformedPath = nativeConformed;
        request.imageSizeMode = "fov";
        request.orientation = "lia";

        fastsurfer::core::ConformStepService service;
        const auto nativeResult = service.run(request);
        require(nativeResult.success, "The native conform step did not succeed for the oblique parity test.");
        require(!nativeResult.alreadyConformed,
                "The oblique fixture should exercise native reconforming, not the already-conformed shortcut.");
        require(std::filesystem::exists(nativeCopy), "The native conform step did not write the oblique copy_orig output.");
        require(std::filesystem::exists(nativeConformed), "The native conform step did not write the oblique conformed output.");

        const auto nativeCopyImage = fastsurfer::core::MghImage::load(nativeCopy);
        const auto pythonCopyImage = fastsurfer::core::MghImage::load(pythonCopy);
        require(nativeCopyImage.header().dimensions == pythonCopyImage.header().dimensions,
                "The oblique copy_orig dimensions differ between native and Python artifacts.");
        require(nativeCopyImage.voxelDataAsFloat() == pythonCopyImage.voxelDataAsFloat(),
            "The oblique copy_orig voxel payload differs between native and Python artifacts.");
        assertExactImageMatch(nativeCopy, pythonCopy);

        assertComparableConformedImages(nativeConformed, pythonConformed);
    }

    void runNamedCase(const std::string &caseName)
    {
        if (caseName == "conformed") {
            test_step_conform_standardized_image_parity();
            return;
        }

        if (caseName == "oblique") {
            test_step_conform_oblique_image_parity();
            return;
        }

        throw std::runtime_error("Unknown parity test case: " + caseName);
}

} // namespace

    int main(int argc, char **argv)
{
    try {
            if (argc > 1) {
                runNamedCase(argv[1]);
            } else {
                test_step_conform_standardized_image_parity();
                test_step_conform_oblique_image_parity();
        }

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}