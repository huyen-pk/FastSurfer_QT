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
// Shared scaling factor used to match FastSurfer's 4-decimal rounding behavior in float space.
inline constexpr float FLOAT_ROUNDING_SCALE = 10000.0F;
// Shared scaling factor used to match FastSurfer's 4-decimal rounding behavior in double space.
inline constexpr double DOUBLE_ROUNDING_SCALE = 10000.0;
// Histogram bin count used by FastSurfer-style uint8 conform scaling.
inline constexpr int HISTOGRAM_BIN_COUNT = 1000;
// Epsilon used to treat values as non-zero during histogram-window estimation.
inline constexpr float HISTOGRAM_NON_ZERO_EPSILON = 1.0e-15F;
// Upper percentile retained by the FastSurfer-style histogram clipping rule.
inline constexpr double HISTOGRAM_UPPER_PERCENTILE = 0.999;
// Epsilon below which scaled values remain explicit background zeros.
inline constexpr float SCALED_ZERO_EPSILON = 1.0e-8F;
// Boundary tolerance for scipy-compatible interpolation index clamping.
inline constexpr double INTERPOLATION_BOUNDARY_EPSILON = 1.0e-5;

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

namespace label_mapping {

// Gaussian sigma used by unknown-cortex fill to reproduce FastSurfer's spatial weighting.
inline constexpr double UNKNOWN_FILL_GAUSSIAN_SIGMA = 5.0;
// Neighborhood radius used by unknown-cortex fill Gaussian support accumulation.
inline constexpr int UNKNOWN_FILL_GAUSSIAN_RADIUS = 20;
// Gaussian sigma used to relateralize overlapping cortical parcels during split-cortex post-processing.
inline constexpr double SPLIT_OVERLAP_GAUSSIAN_SIGMA = 3.0;
// Gaussian truncate factor used to build the overlap-relateralization kernel.
inline constexpr double SPLIT_OVERLAP_GAUSSIAN_TRUNCATE = 4.0;
// 18-connected neighborhood threshold expressed as squared distance in a 3x3x3 stencil.
inline constexpr int LOCAL_NEIGHBOR_MAX_SQUARED_DISTANCE = 2;
// Left-hemisphere cortical labels that may relateralize into right-hemisphere labels in FastSurfer's split stage.
inline constexpr std::array<std::uint16_t, 19> SPLIT_ELIGIBLE_LEFT_LABELS {
	1003, 1006, 1007, 1008, 1009, 1011, 1015, 1018, 1019, 1020,
	1025, 1026, 1027, 1028, 1029, 1030, 1031, 1034, 1035,
};
// Cortical labels requiring the additional Gaussian WM-support overlap split.
inline constexpr std::array<std::uint16_t, 4> SPLIT_OVERLAP_LEFT_LABELS {
	1011, 1019, 1026, 1029,
};

} // namespace label_mapping

} // namespace OpenHC::imaging::mri::fastsurfer::constants