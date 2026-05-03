#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

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

} // namespace

int main()
{
    try {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const auto pythonExecutable = resolvePythonExecutable(repoRoot);
        const auto outputDir = makeFreshDirectory("fastsurfer_core_nifti_roundtrip_test");

        const auto inputPath = repoRoot / "data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii";
        const auto mgzPath = outputDir / "roundtrip_input.mgz";
        const auto roundtripNiftiPath = outputDir / "roundtrip_output.nii.gz";
        const auto pythonScript = outputDir / "verify_roundtrip.py";

        fastsurfer::core::NiftiConverter::convertToMgh(inputPath, mgzPath);
        require(std::filesystem::exists(mgzPath), "The native NIfTI converter did not write the MGZ intermediate file.");

        {
            std::ofstream script(pythonScript);
            script
                << "import sys\n"
                << "from pathlib import Path\n"
                << "import nibabel as nib\n"
                << "import numpy as np\n"
                << "input_path = Path(sys.argv[1])\n"
                << "mgz_path = Path(sys.argv[2])\n"
                << "roundtrip_path = Path(sys.argv[3])\n"
                << "original = nib.load(str(input_path))\n"
                << "converted = nib.load(str(mgz_path))\n"
                << "converted_data = np.asarray(converted.dataobj)\n"
                << "mgz_to_nifti = np.diag([-1.0, -1.0, 1.0, 1.0])\n"
                << "roundtrip_affine = mgz_to_nifti @ converted.affine\n"
                << "roundtrip = nib.Nifti1Image(converted_data, roundtrip_affine)\n"
                << "roundtrip.set_data_dtype(converted_data.dtype)\n"
                << "nib.save(roundtrip, str(roundtrip_path))\n"
                << "reloaded = nib.load(str(roundtrip_path))\n"
                << "original_data = np.asarray(original.dataobj)\n"
                << "roundtrip_data = np.asarray(reloaded.dataobj)\n"
                << "if original.shape != reloaded.shape:\n"
                << "    raise SystemExit(f'Shape mismatch after NIfTI round-trip: {original.shape} vs {reloaded.shape}')\n"
                << "if not np.allclose(original.affine, reloaded.affine, atol=1e-4):\n"
                << "    raise SystemExit('Affine mismatch after NIfTI round-trip.')\n"
                << "if not np.allclose(original_data, roundtrip_data, atol=1e-4):\n"
                << "    diff = float(np.max(np.abs(original_data - roundtrip_data)))\n"
                << "    raise SystemExit(f'Voxel mismatch after NIfTI round-trip. max_diff={diff}')\n";
        }

        const std::string command =
            shellEscape(pythonExecutable) + " " + shellEscape(pythonScript) +
            " " + shellEscape(inputPath) +
            " " + shellEscape(mgzPath) +
            " " + shellEscape(roundtripNiftiPath);

        const int exitCode = std::system(command.c_str());
        require(exitCode == 0,
                "The NIfTI round-trip verification script failed. Set FASTSURFER_PYTHON_EXECUTABLE or create .venv/.venv-parity with numpy and nibabel.");
        require(std::filesystem::exists(roundtripNiftiPath),
                "The MGZ-to-NIfTI round-trip step did not create the NIfTI output file.");

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}