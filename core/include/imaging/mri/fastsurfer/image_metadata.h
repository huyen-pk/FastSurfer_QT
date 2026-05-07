#pragma once

#include <array>
#include <string>

namespace OpenHC::imaging::mri::fastsurfer {

// Lightweight metadata summary returned by native load and conform operations.
struct ImageMetadata {
    // Spatial voxel dimensions along x, y, and z.
    std::array<int, 3> dimensions {0, 0, 0};
    // Number of frames present in the source volume.
    int frames {0};
    // Voxel spacing in millimeters along each spatial axis.
    std::array<float, 3> spacing {0.0F, 0.0F, 0.0F};
    // Derived three-letter orientation code such as LIA or RAS.
    std::string orientationCode;
    // Human-readable storage type name such as uint8 or float32.
    std::string dataTypeName;
    // True when the source header reports valid RAS geometry fields.
    bool rasGood {false};
};

} // namespace OpenHC::imaging::mri::fastsurfer