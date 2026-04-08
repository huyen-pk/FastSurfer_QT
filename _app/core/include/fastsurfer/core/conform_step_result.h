#pragma once

#include <filesystem>
#include <string>

#include "fastsurfer/core/image_metadata.h"

namespace fastsurfer::core {

struct ConformStepResult {
    bool success {false};
    bool alreadyConformed {false};
    std::filesystem::path inputPath;
    std::filesystem::path copyOrigPath;
    std::filesystem::path conformedPath;
    std::string message;
    ImageMetadata inputMetadata;
    ImageMetadata outputMetadata;
};

} // namespace fastsurfer::core