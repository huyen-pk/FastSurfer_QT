#pragma once

#include <filesystem>
#include <string>

#include "imaging/mri/fastsurfer/image_metadata.h"

namespace OpenHC::imaging::mri::fastsurfer {

// Result object returned by the native conform-step service.
struct ConformStepResult {
    // True when the service completed and wrote the requested outputs.
    bool success {false};
    // True when the source image already met the conform policy and was copied through.
    bool alreadyConformed {false};
    // Original input path supplied by the caller.
    std::filesystem::path inputPath;
    // Optional copy-orig output path supplied by the caller.
    std::filesystem::path copyOrigPath;
    // Final conformed MGZ output path supplied by the caller.
    std::filesystem::path conformedPath;
    // Human-readable execution summary.
    std::string message;
    // Metadata captured from the input image before reconforming.
    ImageMetadata inputMetadata;
    // Metadata captured from the written output image.
    ImageMetadata outputMetadata;
};

} // namespace OpenHC::imaging::mri::fastsurfer