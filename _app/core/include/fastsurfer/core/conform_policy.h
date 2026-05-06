#pragma once

#include <array>
#include <string>
#include <vector>

#include "fastsurfer/core/mgh_image.h"
#include "fastsurfer/core/step_conform_request.h"

namespace fastsurfer::core {

struct ScalePolicy {
    float srcMin {0.0F};
    float scale {1.0F};
};

float computeTargetVoxelSize(const MghImage &image, const ConformStepRequest &request);

std::array<int, 3> computeNativeTargetDimensions(
    const MghImage &image,
    float targetVoxelSize,
    ImageSizeMode imageSizeMode);

ScalePolicy computeScalePolicy(const std::vector<float> &data, float dstMin, float dstMax);

float applyScalePolicyValue(float mappedValue, float dstMin, float dstMax, const ScalePolicy &policy);

std::vector<float> applyScalePolicy(
    const std::vector<float> &mappedData,
    float dstMin,
    float dstMax,
    const ScalePolicy &policy);

} // namespace fastsurfer::core