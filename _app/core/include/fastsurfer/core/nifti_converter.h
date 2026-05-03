#pragma once

#include <filesystem>

#include "fastsurfer/core/mgh_image.h"

namespace fastsurfer::core {

class NiftiConverter {
public:
    static bool isSupportedPath(const std::filesystem::path &path);
    static MghImage loadAsMgh(const std::filesystem::path &inputPath);
    static void convertToMgh(const std::filesystem::path &inputPath, const std::filesystem::path &outputPath);
};

} // namespace fastsurfer::core