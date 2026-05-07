#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace OpenHC::imaging::mri::fastsurfer::constants {

namespace conform {

// Canonical 1 mm isotropic voxel size used by conform outputs.
inline constexpr float UNIT_VOXEL_SIZE_MM = 1.0F;
// Threshold above which the min-voxel rule snaps to the canonical 1 mm size.
inline constexpr float DEFAULT_THRESHOLD_1MM = 0.95F;
// Default cubic output size used by FastSurfer's auto conform mode.
inline constexpr int DEFAULT_AUTO_IMAGE_DIMENSION = 256;
// Tolerance for voxel-size and integer-translation comparisons.
inline constexpr float VOXEL_EPSILON = 1.0e-4F;
// Tolerance for deciding whether a rotation matrix is effectively axis-aligned.
inline constexpr double ROTATION_EPSILON = 1.0e-6;

} // namespace conform

namespace mgh {

// Serialized byte size of the MGH header block before voxel payload data.
inline constexpr std::size_t HEADER_SIZE = 284;
// Byte offset of the ras_good_flag field inside the MGH header.
inline constexpr std::size_t RAS_GOOD_FLAG_OFFSET = 28;
// Byte offset where spacing values begin inside the MGH header.
inline constexpr std::size_t SPACING_OFFSET = 30;
// Default MGH format version used for new files.
inline constexpr std::int32_t DEFAULT_VERSION = 1;
// Default frame count for scalar 3D volumes.
inline constexpr std::int32_t DEFAULT_FRAME_COUNT = 1;
// Default legacy degrees-of-freedom field for new files.
inline constexpr std::int32_t DEFAULT_DEGREES_OF_FREEDOM = 0;
// Default value indicating that geometry metadata is present.
inline constexpr std::int16_t DEFAULT_RAS_GOOD_FLAG = 1;
// Default isotropic spacing used by synthesized headers.
inline constexpr std::array<float, 3> DEFAULT_SPACING {1.0F, 1.0F, 1.0F};
// Default LIA-style orientation used by synthesized headers.
inline constexpr std::array<float, 9> DEFAULT_DIRECTION_COSINES {
	-1.0F, 0.0F, 0.0F,
	0.0F, 0.0F, -1.0F,
	0.0F, 1.0F, 0.0F,
};
// Default center point used before image-specific geometry is computed.
inline constexpr std::array<float, 3> DEFAULT_CENTER {0.0F, 0.0F, 0.0F};

} // namespace mgh

namespace nifti {

// Per-axis sign change for converting ITK's LPS coordinates into MGH RAS coordinates.
inline constexpr std::array<double, 3> LPS_TO_RAS_SIGN {-1.0, -1.0, 1.0};

} // namespace nifti

} // namespace OpenHC::imaging::mri::fastsurfer::constants