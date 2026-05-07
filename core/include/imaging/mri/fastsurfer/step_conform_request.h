#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

#include "imaging/mri/fastsurfer/constants.h"

namespace OpenHC::imaging::mri::fastsurfer {

enum class VoxelSizeMode {
    // Use the minimum source voxel size, subject to FastSurfer's 1 mm threshold.
    Min,
};

enum class ImageSizeMode {
    // Use FastSurfer's auto-sizing policy, which may force a 256^3 cube.
    Auto,
    // Preserve field of view and recompute dimensions from target voxel size.
    Fov,
};

enum class OrientationMode {
    // Reorient the output into the canonical LIA orientation.
    Lia,
    // Preserve the source image orientation.
    Native,
};

// Returns the stable string spelling used by configuration and diagnostics.
// Parameters:
// - mode: Voxel-size mode to stringify.
// Returns the non-owning text form of the mode.
inline constexpr std::string_view to_string_view(const VoxelSizeMode mode)
{
    switch (mode) {
    case VoxelSizeMode::Min:
        return "min";
    }

    return "unknown";
}

// Returns the stable string spelling used by configuration and diagnostics.
// Parameters:
// - mode: Image-size mode to stringify.
// Returns the non-owning text form of the mode.
inline constexpr std::string_view to_string_view(const ImageSizeMode mode)
{
    switch (mode) {
    case ImageSizeMode::Auto:
        return "auto";
    case ImageSizeMode::Fov:
        return "fov";
    }

    return "unknown";
}

// Returns the stable string spelling used by configuration and diagnostics.
// Parameters:
// - mode: Orientation mode to stringify.
// Returns the non-owning text form of the mode.
inline constexpr std::string_view to_string_view(const OrientationMode mode)
{
    switch (mode) {
    case OrientationMode::Lia:
        return "lia";
    case OrientationMode::Native:
        return "native";
    }

    return "unknown";
}

// Returns an owning string version of the voxel-size mode name.
// Parameters:
// - mode: Voxel-size mode to stringify.
// Returns the owning string form of the mode.
inline std::string to_string(const VoxelSizeMode mode)
{
    return std::string(to_string_view(mode));
}

// Returns an owning string version of the image-size mode name.
// Parameters:
// - mode: Image-size mode to stringify.
// Returns the owning string form of the mode.
inline std::string to_string(const ImageSizeMode mode)
{
    return std::string(to_string_view(mode));
}

// Returns an owning string version of the orientation mode name.
// Parameters:
// - mode: Orientation mode to stringify.
// Returns the owning string form of the mode.
inline std::string to_string(const OrientationMode mode)
{
    return std::string(to_string_view(mode));
}

// Parses the user-facing voxel-size mode string into the native enum.
// Parameters:
// - mode: User-facing text value such as "min".
// Returns the parsed voxel-size mode.
inline VoxelSizeMode parseVoxelSizeMode(const std::string_view mode)
{
    if (mode == to_string_view(VoxelSizeMode::Min)) {
        return VoxelSizeMode::Min;
    }

    throw std::invalid_argument("Unsupported vox_size mode: " + std::string(mode));
}

// Parses the user-facing image-size mode string into the native enum.
// Parameters:
// - mode: User-facing text value such as "auto" or "fov".
// Returns the parsed image-size mode.
inline ImageSizeMode parseImageSizeMode(const std::string_view mode)
{
    if (mode == to_string_view(ImageSizeMode::Auto)) {
        return ImageSizeMode::Auto;
    }
    if (mode == to_string_view(ImageSizeMode::Fov)) {
        return ImageSizeMode::Fov;
    }

    throw std::invalid_argument("Unsupported image_size mode: " + std::string(mode));
}

// Parses the user-facing orientation mode string into the native enum.
// Parameters:
// - mode: User-facing text value such as "lia" or "native".
// Returns the parsed orientation mode.
inline OrientationMode parseOrientationMode(const std::string_view mode)
{
    if (mode == to_string_view(OrientationMode::Lia)) {
        return OrientationMode::Lia;
    }
    if (mode == to_string_view(OrientationMode::Native)) {
        return OrientationMode::Native;
    }

    throw std::invalid_argument("Unsupported orientation mode: " + std::string(mode));
}

// Input contract for one native conform-step execution.
struct ConformStepRequest {
    // Source image path. Native MGZ and supported NIfTI inputs are accepted.
    std::filesystem::path inputPath;
    // Optional copy of the original input written before any reconforming.
    std::filesystem::path copyOrigPath;
    // Final conformed MGZ output path.
    std::filesystem::path conformedPath;
    // Policy used to choose the output voxel size.
    VoxelSizeMode voxSizeMode {VoxelSizeMode::Min};
    // Policy used to choose the output image dimensions.
    ImageSizeMode imageSizeMode {ImageSizeMode::Auto};
    // Requested output orientation, or Native to preserve the source orientation.
    OrientationMode orientation {OrientationMode::Lia};
    // Threshold used by the 1 mm snapping rule in Min voxel-size mode.
    float threshold1mm {constants::conform::DEFAULT_THRESHOLD_1MM};
    // Forces a reconform pass even when the input already satisfies the target policy.
    bool forceConform {false};
};

} // namespace OpenHC::imaging::mri::fastsurfer