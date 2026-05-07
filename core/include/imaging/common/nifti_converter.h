#pragma once

#include <filesystem>

#include "imaging/common/mgh_image.h"

namespace OpenHC::imaging::mri::fastsurfer {

// Bridges supported scalar 3D NIfTI inputs into the native MGH image model.
class NiftiConverter {
public:
    // Returns true when the path looks like a supported .nii or .nii.gz input.
    // Parameters:
    // - path: Candidate input path to inspect.
    // Returns true when the extension is one of the supported NIfTI forms.
    static bool isSupportedPath(const std::filesystem::path &path);

    // Loads a supported NIfTI volume and maps its geometry plus voxel payload
    // into an MghImage.
    // Parameters:
    // - inputPath: Supported NIfTI file to import.
    // Returns the imported image represented as an MghImage.
    static MghImage loadAsMgh(const std::filesystem::path &inputPath);

    // Converts a supported NIfTI input directly into an MGZ output file.
    // Parameters:
    // - inputPath: Supported NIfTI file to import.
    // - outputPath: MGZ path to create from the imported image.
    static void convertToMgh(const std::filesystem::path &inputPath, const std::filesystem::path &outputPath);
};

} // namespace OpenHC::imaging::mri::fastsurfer