#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

#include "fastsurfer/core/constants.h"

namespace fastsurfer::core {

enum class VoxelSizeMode {
    Min,
};

enum class ImageSizeMode {
    Auto,
    Fov,
};

enum class OrientationMode {
    Lia,
    Native,
};

inline constexpr std::string_view to_string_view(const VoxelSizeMode mode)
{
    switch (mode) {
    case VoxelSizeMode::Min:
        return "min";
    }

    return "unknown";
}

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

inline std::string to_string(const VoxelSizeMode mode)
{
    return std::string(to_string_view(mode));
}

inline std::string to_string(const ImageSizeMode mode)
{
    return std::string(to_string_view(mode));
}

inline std::string to_string(const OrientationMode mode)
{
    return std::string(to_string_view(mode));
}

inline VoxelSizeMode parseVoxelSizeMode(const std::string_view mode)
{
    if (mode == to_string_view(VoxelSizeMode::Min)) {
        return VoxelSizeMode::Min;
    }

    throw std::invalid_argument("Unsupported vox_size mode: " + std::string(mode));
}

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

struct ConformStepRequest {
    std::filesystem::path inputPath;
    std::filesystem::path copyOrigPath;
    std::filesystem::path conformedPath;
    VoxelSizeMode voxSizeMode {VoxelSizeMode::Min};
    ImageSizeMode imageSizeMode {ImageSizeMode::Auto};
    OrientationMode orientation {OrientationMode::Lia};
    float threshold1mm {constants::conform::DefaultThreshold1mm};
    bool forceConform {false};
};

} // namespace fastsurfer::core