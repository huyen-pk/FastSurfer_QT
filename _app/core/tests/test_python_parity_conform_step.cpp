#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fastsurfer/core/conform_step_request.h"
#include "fastsurfer/core/conform_step_service.h"
#include "fastsurfer/core/mgh_image.h"

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

void assertAffineClose(const fastsurfer::core::Matrix4 &left, const fastsurfer::core::Matrix4 &right)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            require(std::fabs(left[row][column] - right[row][column]) <= 1.0e-4,
                    "Affine differs between native and Python outputs.");
        }
    }
}

void assertMatchingImages(const std::filesystem::path &leftPath, const std::filesystem::path &rightPath)
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
        require(leftData.size() == rightData.size(), "Voxel counts differ between native and Python outputs.");

        float maxDifference = 0.0F;
        for (std::size_t index = 0; index < leftData.size(); ++index) {
        maxDifference = std::max(maxDifference, std::fabs(leftData[index] - rightData[index]));
        }
        require(maxDifference <= 1.0F, "Voxel payload differs from Python by more than one intensity level.");
}

} // namespace

int main()
{
    try {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "test/data/Subject140/140_orig.mgz";
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
                << "sys.path.insert(0, str(repo_root))\n"
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

        const int exitCode = std::system(command.c_str());
        require(exitCode == 0,
                "The Python reference execution failed during the parity test. Set FASTSURFER_PYTHON_EXECUTABLE or create .venv-parity with numpy and nibabel.");

        require(std::filesystem::exists(pythonCopy), "The Python reference did not create the copy_orig output.");
        require(std::filesystem::exists(pythonConformed), "The Python reference did not create the conformed output.");

        assertMatchingImages(nativeCopy, pythonCopy);
        assertMatchingImages(nativeConformed, pythonConformed);

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}