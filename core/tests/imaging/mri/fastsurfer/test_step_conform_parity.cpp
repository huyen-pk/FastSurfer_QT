#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestConstants.h"
#include "TestHelpers.h"
#include "imaging/mri/fastsurfer/step_conform_request.h"
#include "imaging/mri/fastsurfer/step_conform.h"
#include "imaging/common/mgh_image.h"
#include <format>

namespace {

// Creates a clean temporary directory for one parity-test invocation.
// Parameters:
// - name: Directory suffix used to isolate this test run.
// Returns the recreated directory path.
std::filesystem::path makeFreshDirectory(const std::string &name)
{
    const auto root = [&]() {
        if (const char *envTmp = std::getenv("FASTSURFER_TEST_TMPDIR"); envTmp != nullptr && envTmp[0] != '\0') {
            return std::filesystem::path(envTmp) / name;
        }

        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
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

// use shared TestHelpers.h for assertion helpers

// Escapes a filesystem path for safe single-quoted shell invocation.
// Parameters:
// - path: Filesystem path to escape.
// Returns a shell-safe quoted string.
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

// Resolves the Python interpreter used for the parity reference script.
// Parameters:
// - repoRoot: Workspace root used to probe local virtual environments.
// Returns the chosen Python executable path or command.
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

namespace ohc = OpenHC::imaging::mri::fastsurfer;


// Builds a deliberately non-conformed synthetic input from the Subject140 fixture.
// Parameters:
// - fixturePath: Source MGZ fixture used to seed voxel values.
// Returns a cropped float image with non-conforming geometry.
ohc::MghImage createSyntheticNonConformedInput(const std::filesystem::path &fixturePath)
{
    const auto sourceImage = OpenHC::imaging::mri::fastsurfer::MghImage::load(fixturePath);
    const auto sourceData = sourceImage.voxelDataAsFloat();
    require(!sourceData.empty(), "The fixture MGZ appears empty or unreadable: " + fixturePath.string());

    OpenHC::imaging::mri::fastsurfer::MghImage::Header header = sourceImage.header();
    header.dimensions = {64, 72, 80};
    header.frames = 1;
    header.type = static_cast<std::int32_t>(OpenHC::imaging::mri::fastsurfer::MghDataType::Float32);
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

    return OpenHC::imaging::mri::fastsurfer::MghImage::fromVoxelData(header, cropped, header.type);
}

// Verifies that two affines agree within the configured tolerances.
// Parameters:
// - left: Affine produced by the native path.
// - right: Affine produced by the Python reference.
// - translationToleranceMm: Maximum allowed translation error in millimeters.
void assertAffineClose(
    const OpenHC::imaging::mri::fastsurfer::Matrix4 &left,
    const OpenHC::imaging::mri::fastsurfer::Matrix4 &right,
    const double translationToleranceMm = test_constants::AFFINE_TRANSLATION_TOLERANCE_MM)
{
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
                require(std::fabs(left[row][column] - right[row][column]) <= test_constants::AFFINE_LINEAR_TOLERANCE,
                    "Affine linear terms differ between native and Python outputs.");
        }

        require(std::fabs(left[row][3] - right[row][3]) <= translationToleranceMm,
                std::format("Affine translation differs from Python by more than {} millimeters.", translationToleranceMm));
    }

    require(std::fabs(left[3][0] - right[3][0]) <= test_constants::HOMOGENEOUS_ROW_TOLERANCE &&
                std::fabs(left[3][1] - right[3][1]) <= test_constants::HOMOGENEOUS_ROW_TOLERANCE &&
                std::fabs(left[3][2] - right[3][2]) <= test_constants::HOMOGENEOUS_ROW_TOLERANCE &&
                std::fabs(left[3][3] - right[3][3]) <= test_constants::HOMOGENEOUS_ROW_TOLERANCE,
            "Affine homogeneous row differs between native and Python outputs.");
}

// Verifies exact image equality between two MGZ outputs.
// Parameters:
// - leftPath: Path to the first output image.
// - rightPath: Path to the second output image.
void assertExactImageMatch(const std::filesystem::path &leftPath, const std::filesystem::path &rightPath)
{
    const auto leftImage = OpenHC::imaging::mri::fastsurfer::MghImage::load(leftPath);
    const auto rightImage = OpenHC::imaging::mri::fastsurfer::MghImage::load(rightPath);

    require(leftImage.header().dimensions == rightImage.header().dimensions,
            "Image dimensions differ between native and Python outputs.");
    require(leftImage.header().frames == rightImage.header().frames,
            "Frame counts differ between native and Python outputs.");
    require(leftImage.header().type == rightImage.header().type,
            "Data types differ between native and Python outputs.");
    require(leftImage.orientationCode() == rightImage.orientationCode(),
            "Output orientations differ between native and Python outputs.");
    for (int axis = 0; axis < 3; ++axis) {
        require(std::fabs(leftImage.header().spacing[axis] - rightImage.header().spacing[axis]) <= test_constants::PARITY_SPACING_TOLERANCE,
                "Output spacing differs between native and Python outputs.");
    }
    assertAffineClose(leftImage.affine(), rightImage.affine());

    const auto leftData = leftImage.voxelDataAsFloat();
    const auto rightData = rightImage.voxelDataAsFloat();
    require(leftData == rightData, "Voxel payload differs between native and Python outputs.");
}

// Verifies parity under the relaxed tolerances used for reconformed outputs.
// Parameters:
// - leftPath: Path to the native output image.
// - rightPath: Path to the Python reference image.
// - translationToleranceMm: Maximum allowed affine-translation error in millimeters.
void assertComparableConformedImages(
    const std::filesystem::path &leftPath,
    const std::filesystem::path &rightPath,
    const double translationToleranceMm = test_constants::AFFINE_TRANSLATION_TOLERANCE_MM)
{
        const auto leftImage = OpenHC::imaging::mri::fastsurfer::MghImage::load(leftPath);
        const auto rightImage = OpenHC::imaging::mri::fastsurfer::MghImage::load(rightPath);

        require(leftImage.header().dimensions == rightImage.header().dimensions,
                "Image dimensions differ between native and Python outputs.");
        require(leftImage.header().frames == rightImage.header().frames,
                "Frame counts differ between native and Python outputs.");
        require(leftImage.header().type == rightImage.header().type,
                "Data types differ between native and Python outputs.");
        require(leftImage.orientationCode() == rightImage.orientationCode(),
                "Output orientations differ between native and Python outputs.");
        for (int axis = 0; axis < 3; ++axis) {
                require(std::fabs(leftImage.header().spacing[axis] - rightImage.header().spacing[axis]) <= test_constants::PARITY_SPACING_TOLERANCE,
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
        require(foregroundMAD <= test_constants::PYTHON_FOREGROUND_MEAN_ABS_TOLERANCE, "Brain tissue intensity diverges too much.");
        require(maxDiff <= test_constants::PYTHON_LOCALIZED_MAX_DIFF_TOLERANCE, "Localized intensity error is too high.");

    }

// Escapes a raw string for safe single-quoted shell invocation.
// Parameters:
// - value: Raw command-line token to escape.
// Returns a shell-safe quoted string.
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

// Executes a shell command and returns the exit code.
// Parameters:
// - command: Full shell command to execute.
// Returns the process exit code reported by std::system.
int runCommand(const std::string &command)
{
        return std::system(command.c_str());
}

// Verifies native parity against the Python reference on a synthetic non-conformed MGZ input.
void test_step_conform_standardized_image_parity()
{
        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "data/Subject140/140_orig.mgz";
        const auto pythonExecutable = resolvePythonExecutable(repoRoot);

        const auto nativeDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_native_subject140");
        const auto pythonDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_python_subject140");

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
                << std::format("kwargs = {{'threshold_1mm': {}, 'vox_size': 'min', 'orientation': 'lia', 'img_size': 'fov'}}\n",
                               test_constants::CONFORM_THRESHOLD_1MM)
                << "if not is_conform(orig, verbose=False, **kwargs):\n"
                << "    orig = conform(orig, **kwargs)\n"
                << "    orig_data = np.asanyarray(orig.dataobj)\n"
                << "else:\n"
                << "    orig_data = np.asanyarray(orig.dataobj)\n"
                << "conformed_image = nib.MGHImage(orig_data, orig.affine, orig.header.copy())\n"
                << "conformed_image.set_data_dtype(np.uint8)\n"
                << "nib.save(conformed_image, str(conformed_path))\n";
        }

        OpenHC::imaging::mri::fastsurfer::ConformStepRequest request;
        request.inputPath = syntheticInputPath;
        request.copyOrigPath = nativeCopy;
        request.conformedPath = nativeConformed;
        request.imageSizeMode = OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Fov;

        OpenHC::imaging::mri::fastsurfer::ConformStepService service;
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

// Verifies native parity against generated Python artifacts for the oblique NIfTI fixture.
void test_step_conform_oblique_image_parity()
{
        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
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

        const auto nativeDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_native_oblique_parity");
        const auto nativeCopy = nativeDir / "copy_orig.mgz";
        const auto nativeConformed = nativeDir / "conformed_orig.mgz";

        OpenHC::imaging::mri::fastsurfer::ConformStepRequest request;
        request.inputPath = fixturePath;
        request.copyOrigPath = nativeCopy;
        request.conformedPath = nativeConformed;
        request.imageSizeMode = OpenHC::imaging::mri::fastsurfer::ImageSizeMode::Fov;
        request.orientation = OpenHC::imaging::mri::fastsurfer::OrientationMode::Lia;

        OpenHC::imaging::mri::fastsurfer::ConformStepService service;
        const auto nativeResult = service.run(request);
        require(nativeResult.success, "The native conform step did not succeed for the oblique parity test.");
        require(!nativeResult.alreadyConformed,
                "The oblique fixture should exercise native reconforming, not the already-conformed shortcut.");
        require(std::filesystem::exists(nativeCopy), "The native conform step did not write the oblique copy_orig output.");
        require(std::filesystem::exists(nativeConformed), "The native conform step did not write the oblique conformed output.");

        const auto nativeCopyImage = OpenHC::imaging::mri::fastsurfer::MghImage::load(nativeCopy);
        const auto pythonCopyImage = OpenHC::imaging::mri::fastsurfer::MghImage::load(pythonCopy);
        require(nativeCopyImage.header().dimensions == pythonCopyImage.header().dimensions,
                "The oblique copy_orig dimensions differ between native and Python artifacts.");
        require(nativeCopyImage.voxelDataAsFloat() == pythonCopyImage.voxelDataAsFloat(),
            "The oblique copy_orig voxel payload differs between native and Python artifacts.");
        assertExactImageMatch(nativeCopy, pythonCopy);

        assertComparableConformedImages(nativeConformed, pythonConformed);
}

// Dispatches one named parity test case.
// Parameters:
// - caseName: CLI token identifying the parity case to run.
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

// Runs one or more parity test scenarios from the command line.
// Parameters:
// - argc: Argument count. Optional second argument selects one named case.
// - argv: Argument vector containing the optional case selector.
// Returns 0 on success and 1 on failure.
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