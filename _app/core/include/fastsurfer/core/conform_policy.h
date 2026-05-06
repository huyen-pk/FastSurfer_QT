#pragma once

#include <array>
#include <string>

#include "fastsurfer/core/mgh_image.h"
#include "fastsurfer/core/step_conform_request.h"

namespace fastsurfer::core {

float computeTargetVoxelSize(const MghImage &image, const ConformStepRequest &request);

std::array<int, 3> computeNativeTargetDimensions(
    const MghImage &image,
    float targetVoxelSize,
    ImageSizeMode imageSizeMode);

} // namespace fastsurfer::core