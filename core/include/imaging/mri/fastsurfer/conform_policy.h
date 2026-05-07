#pragma once

#include <array>
#include <string>
#include <vector>

#include "imaging/mri/fastsurfer/mgh_image.h"
#include "imaging/mri/fastsurfer/step_conform_request.h"

namespace OpenHC::imaging::mri::fastsurfer {

// Describes the linear remapping used to project source intensities into the
// conformed destination range.
struct ScalePolicy {
    // Source intensity treated as the lower bound of the remapped range.
    float srcMin {0.0F};
    // Multiplicative factor applied after subtracting srcMin.
    float scale {1.0F};
};

// Returns the voxel size that the native conform step should target for the
// requested image and conform configuration.
// Parameters:
// - image: Source image whose spacing drives the conform voxel-size policy.
// - request: Conform settings that choose the voxel-size mode and 1 mm threshold.
// Returns the requested isotropic voxel size in millimeters.
float computeTargetVoxelSize(const MghImage &image, const ConformStepRequest &request);

// Computes the output volume dimensions used by the native conform step for the
// requested image-size mode and target voxel size.
// Parameters:
// - image: Source image whose field of view defines the target dimensions.
// - targetVoxelSize: Isotropic voxel size chosen for the conform output.
// - imageSizeMode: Policy that chooses between cubic auto sizing and FOV sizing.
// Returns the target x, y, and z dimensions.
std::array<int, 3> computeNativeTargetDimensions(
    const MghImage &image,
    float targetVoxelSize,
    ImageSizeMode imageSizeMode);

// Derives the intensity scaling policy used to map conformed voxel values into
// the requested output range.
// Parameters:
// - data: Source voxel intensities used to estimate the histogram scaling window.
// - dstMin: Lower bound of the destination intensity range.
// - dstMax: Upper bound of the destination intensity range.
// Returns the source minimum and multiplicative scale for the remapping.
ScalePolicy computeScalePolicy(const std::vector<float> &data, float dstMin, float dstMax);

// Applies a previously computed scale policy to one voxel value while clamping
// the result to the destination range.
// Parameters:
// - source: Source intensity to transform.
// - dstMin: Lower bound of the destination intensity range.
// - dstMax: Upper bound of the destination intensity range.
// - policy: Scaling policy previously derived from the source histogram.
// Returns the scaled and clamped output intensity.
float applyScalePolicyValue(float source, float dstMin, float dstMax, const ScalePolicy &policy);

// Applies a previously computed scale policy to a full voxel buffer.
// Parameters:
// - source: Source intensities to transform.
// - dstMin: Lower bound of the destination intensity range.
// - dstMax: Upper bound of the destination intensity range.
// - policy: Scaling policy previously derived from the source histogram.
// Returns a new buffer containing the scaled output intensities.
std::vector<float> applyScalePolicy(
    const std::vector<float> &source,
    float dstMin,
    float dstMax,
    const ScalePolicy &policy);

} // namespace OpenHC::imaging::mri::fastsurfer