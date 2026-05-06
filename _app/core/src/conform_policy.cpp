#include "fastsurfer/core/conform_policy.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "fastsurfer/core/constants.h"

namespace fastsurfer::core {
namespace {

float roundToEpsilonPrecision(const float value)
{
    constexpr float scale = 10000.0F;
    return std::round(value * scale) / scale;
}

int conformLikeCeil(const float value)
{
    constexpr double scale = 10000.0;
    return static_cast<int>(std::ceil(std::floor(static_cast<double>(value) * scale) / scale));
}

} // namespace

float computeTargetVoxelSize(const MghImage &image, const ConformStepRequest &request)
{
    if (request.voxSizeMode != VoxelSizeMode::Min) {
        throw std::runtime_error("Unsupported vox_size mode in native conform service: " + to_string(request.voxSizeMode));
    }

    float targetVoxelSize = std::min(roundToEpsilonPrecision(image.minVoxelSize()), constants::conform::UNIT_VOXEL_SIZE_MM);
    if (request.threshold1mm > 0.0F && targetVoxelSize > request.threshold1mm) {
        targetVoxelSize = constants::conform::UNIT_VOXEL_SIZE_MM;
    }
    return targetVoxelSize;
}

std::array<int, 3> computeNativeTargetDimensions(
    const MghImage &image,
    const float targetVoxelSize,
    const ImageSizeMode imageSizeMode)
{
    if (imageSizeMode == ImageSizeMode::Auto) {
        if (std::fabs(targetVoxelSize - constants::conform::UNIT_VOXEL_SIZE_MM) <=
            std::fabs(constants::conform::UNIT_VOXEL_SIZE_MM - constants::conform::DEFAULT_THRESHOLD_1MM)) {
            return {
                constants::conform::DEFAULT_AUTO_IMAGE_DIMENSION,
                constants::conform::DEFAULT_AUTO_IMAGE_DIMENSION,
                constants::conform::DEFAULT_AUTO_IMAGE_DIMENSION,
            };
        }

        std::array<int, 3> target = image.header().dimensions;
        int maxDimension = constants::conform::DEFAULT_AUTO_IMAGE_DIMENSION;
        for (std::size_t index = 0; index < target.size(); ++index) {
            const float fov = image.header().spacing[index] * static_cast<float>(image.header().dimensions[index]);
            target[index] = conformLikeCeil(fov / targetVoxelSize);
            maxDimension = std::max(maxDimension, target[index]);
        }
        return {maxDimension, maxDimension, maxDimension};
    }

    if (imageSizeMode == ImageSizeMode::Fov) {
        // recompute the dimensions based on the target voxel size while keeping the field of view
        std::array<int, 3> target = image.header().dimensions;
        for (std::size_t index = 0; index < target.size(); ++index) {
            const float fov = image.header().spacing[index] * static_cast<float>(image.header().dimensions[index]);
            target[index] = conformLikeCeil(fov / targetVoxelSize);
        }
        return target;
    }

    throw std::runtime_error("Unsupported image_size mode in native conform service: " + to_string(imageSizeMode));
}

} // namespace fastsurfer::core