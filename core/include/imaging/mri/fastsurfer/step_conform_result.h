#pragma once

#include <filesystem>
#include <string>

#include "imaging/mri/fastsurfer/image_metadata.h"

namespace OpenHC::imaging::mri::fastsurfer {

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

} // namespace OpenHC::imaging::mri::fastsurfer