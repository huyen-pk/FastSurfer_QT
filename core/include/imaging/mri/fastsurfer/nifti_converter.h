#pragma once

#include <filesystem>

#include "imaging/mri/fastsurfer/mgh_image.h"

namespace OpenHC::imaging::mri::fastsurfer {

class NiftiConverter {
public:
    static bool isSupportedPath(const std::filesystem::path &path);
    static MghImage loadAsMgh(const std::filesystem::path &inputPath);
    static void convertToMgh(const std::filesystem::path &inputPath, const std::filesystem::path &outputPath);
};

} // namespace OpenHC::imaging::mri::fastsurfer