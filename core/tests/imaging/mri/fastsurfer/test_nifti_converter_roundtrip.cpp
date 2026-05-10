#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "TestConstants.h"
#include "TestHelpers.h"
#include "TestParitySupport.h"
#include "imaging/common/nifti_converter.h"

namespace ohc = OpenHC::imaging::mri::fastsurfer;
namespace oht = OpenHC::tests::fastsurfer::support;

namespace {
} // namespace

// Converts one NIfTI fixture to MGZ and verifies the round-trip back to NIfTI.
// Returns 0 on success and 1 on failure.
int main()
{
    try {
        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
        const auto pythonExecutable = oht::resolvePythonExecutable(repoRoot);
        const auto outputDir = oht::makeFreshDirectory("openhc_imaging_mri_fastsurfer_nifti_roundtrip_test");

        const auto inputPath = repoRoot / "data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii";
        const auto mgzPath = outputDir / "roundtrip_input.mgz";
        const auto roundtripNiftiPath = outputDir / "roundtrip_output.nii.gz";
        const auto pythonScript = outputDir / "verify_roundtrip.py";

        ohc::NiftiConverter::convertToMgh(inputPath, mgzPath);
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
                << "original_data = np.asarray(original.dataobj, dtype=np.float32)\n"
                << "roundtrip_affine = converted.affine\n"
                << "roundtrip = nib.Nifti1Image(converted_data, roundtrip_affine)\n"
                << "roundtrip.set_data_dtype(converted_data.dtype)\n"
                << "nib.save(roundtrip, str(roundtrip_path))\n"
                << "reloaded = nib.load(str(roundtrip_path))\n"
                << "roundtrip_data = np.asarray(reloaded.dataobj)\n"
                << "if original.shape != reloaded.shape:\n"
                << "    raise SystemExit(f'Shape mismatch after NIfTI round-trip: {original.shape} vs {reloaded.shape}')\n"
                << "if not np.allclose(original.affine, reloaded.affine, atol=" << test_constants::AFFINE_LINEAR_TOLERANCE << "):\n"
                << "    raise SystemExit('Affine mismatch after NIfTI round-trip.')\n"
                << "if not np.array_equal(original_data, roundtrip_data):\n"
                << "    diff = float(np.max(np.abs(original_data - roundtrip_data)))\n"
                << "    raise SystemExit(f'Voxel mismatch after NIfTI round-trip. max_diff={diff}')\n";
        }

        const std::string command =
            oht::shellEscape(pythonExecutable) + " " + oht::shellEscape(pythonScript) +
            " " + oht::shellEscape(inputPath) +
            " " + oht::shellEscape(mgzPath) +
            " " + oht::shellEscape(roundtripNiftiPath);

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