#include "imaging/mri/fastsurfer/conform_policy.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "imaging/mri/fastsurfer/constants.h"

namespace OpenHC::imaging::mri::fastsurfer {
namespace {

// Rounds to the precision used by FastSurfer's conform heuristics.
float roundToEpsilonPrecision(const float value)
{
    constexpr float scale = 10000.0F;
    // Mirror the Python conform path's practical precision before comparing
    // source voxel sizes against the 1 mm policy threshold.
    return std::round(value * scale) / scale;
}

// Applies FastSurfer's ceil-after-truncate dimension rule for FOV-derived sizes.
int conformLikeCeil(const float value)
{
    constexpr double scale = 10000.0;
    // Match the Python reference by truncating to 4 decimal places before the
    // final ceil to avoid precision-driven off-by-one dimension changes.
    return static_cast<int>(std::ceil(std::floor(static_cast<double>(value) * scale) / scale));
}

} // namespace

float computeTargetVoxelSize(const MghImage &image, const ConformStepRequest &request)
{
    if (request.voxSizeMode != VoxelSizeMode::Min) {
        throw std::runtime_error("Unsupported vox_size mode in native conform service: " + to_string(request.voxSizeMode));
    }

    float targetVoxelSize = std::min(roundToEpsilonPrecision(image.minVoxelSize()), constants::conform::UNIT_VOXEL_SIZE_MM);
    // FastSurfer treats near-1 mm inputs as exactly 1 mm so the native policy
    // does not create slightly larger target grids for numerically noisy inputs.
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
            // Preserve FastSurfer's canonical 256^3 output whenever the target
            // voxel size resolves to the standard 1 mm conform case.
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

// Builds the histogram-based source intensity window used for uint8 conform output.
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
    // Use the same high-percentile clipping strategy as FastSurfer's conform
    // path so rare bright voxels do not dominate the output scaling.
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

// Applies the scale policy to one sample while preserving explicit background zeros.
float applyScalePolicyValue(
    const float source,
    const float dstMin,
    const float dstMax,
    const ScalePolicy &policy)
{
    // Keep exact background zeros at zero so interpolation does not introduce a
    // faint non-zero halo after histogram scaling.
    if (std::fabs(source) <= 1.0e-8F) {
        return 0.0F;
    }

    return std::clamp(dstMin + policy.scale * (source - policy.srcMin), dstMin, dstMax);
}

// Applies one scale policy to the full source voxel buffer.
std::vector<float> applyScalePolicy(
    const std::vector<float> &source,
    const float dstMin,
    const float dstMax,
    const ScalePolicy &policy)
{
    std::vector<float> output(source.size(), 0.0F);
    for (std::size_t index = 0; index < source.size(); ++index) {
        output[index] = applyScalePolicyValue(source[index], dstMin, dstMax, policy);
    }
    return output;
}

} // namespace OpenHC::imaging::mri::fastsurfer