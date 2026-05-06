#include "imaging/mri/fastsurfer/conform_policy.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "imaging/mri/fastsurfer/constants.h"

namespace OpenHC::imaging::mri::fastsurfer {
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

ScalePolicy computeScalePolicy(const std::vector<float> &data, const float dstMin, const float dstMax)
{
    const auto [minIt, maxIt] = std::minmax_element(data.begin(), data.end());
    const float dataMin = *minIt;
    const float dataMax = *maxIt;

    if (dataMin == dataMax) {
        return {dataMin, 1.0F};
    }

    constexpr int bins = 1000;
    const double binWidth = static_cast<double>(dataMax - dataMin) / static_cast<double>(bins);
    if (binWidth <= std::numeric_limits<double>::epsilon()) {
        return {dataMin, 1.0F};
    }

    std::vector<int> histogram(bins, 0);
    std::size_t nonZeroVoxels = 0;
    for (const float value : data) {
        if (std::fabs(value) >= 1.0e-15F) {
            ++nonZeroVoxels;
        }

        int index = static_cast<int>(std::floor((static_cast<double>(value) - dataMin) / binWidth));
        index = std::clamp(index, 0, bins - 1);
        ++histogram[index];
    }

    std::vector<int> cumulative(bins + 1, 0);
    for (int index = 0; index < bins; ++index) {
        cumulative[index + 1] = cumulative[index] + histogram[index];
    }

    std::vector<float> binEdges(bins + 1, dataMin);
    for (int index = 0; index <= bins; ++index) {
        binEdges[index] = static_cast<float>(static_cast<double>(dataMin) + binWidth * index);
    }

    const int totalVoxels = static_cast<int>(data.size());
    const int lowerCutoff = 0;
    int lowerEdgeIndex = 0;
    for (int index = 0; index < static_cast<int>(cumulative.size()); ++index) {
        if (cumulative[index] < lowerCutoff) {
            lowerEdgeIndex = index + 1;
        }
    }

    const int upperCutoff = totalVoxels - static_cast<int>((1.0 - 0.999) * static_cast<double>(nonZeroVoxels));
    int upperEdgeIndex = bins - 1;
    bool foundUpper = false;
    for (int index = 0; index < static_cast<int>(cumulative.size()); ++index) {
        if (cumulative[index] >= upperCutoff) {
            upperEdgeIndex = std::max(0, index - 2);
            foundUpper = true;
            break;
        }
    }
    if (!foundUpper) {
        upperEdgeIndex = bins - 1;
    }

    const float srcMin = binEdges[lowerEdgeIndex];
    const float srcMax = binEdges[upperEdgeIndex];
    if (srcMin == srcMax) {
        return {srcMin, 1.0F};
    }

    return {srcMin, (dstMax - dstMin) / (srcMax - srcMin)};
}

float applyScalePolicyValue(
    const float mappedValue,
    const float dstMin,
    const float dstMax,
    const ScalePolicy &policy)
{
    if (std::fabs(mappedValue) <= 1.0e-8F) {
        return 0.0F;
    }

    return std::clamp(dstMin + policy.scale * (mappedValue - policy.srcMin), dstMin, dstMax);
}

std::vector<float> applyScalePolicy(
    const std::vector<float> &mappedData,
    const float dstMin,
    const float dstMax,
    const ScalePolicy &policy)
{
    std::vector<float> output(mappedData.size(), 0.0F);
    for (std::size_t index = 0; index < mappedData.size(); ++index) {
        output[index] = applyScalePolicyValue(mappedData[index], dstMin, dstMax, policy);
    }
    return output;
}

} // namespace OpenHC::imaging::mri::fastsurfer