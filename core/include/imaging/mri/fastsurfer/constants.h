#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace OpenHC::imaging::mri::fastsurfer::constants {

namespace conform {

inline constexpr float UNIT_VOXEL_SIZE_MM = 1.0F;
inline constexpr float DEFAULT_THRESHOLD_1MM = 0.95F;
inline constexpr int DEFAULT_AUTO_IMAGE_DIMENSION = 256;
inline constexpr float VOXEL_EPSILON = 1.0e-4F;
inline constexpr double ROTATION_EPSILON = 1.0e-6;

} // namespace conform

namespace mgh {

inline constexpr std::size_t HEADER_SIZE = 284;
inline constexpr std::size_t RAS_GOOD_FLAG_OFFSET = 28;
inline constexpr std::size_t SPACING_OFFSET = 30;
inline constexpr std::int32_t DEFAULT_VERSION = 1;
inline constexpr std::int32_t DEFAULT_FRAME_COUNT = 1;
inline constexpr std::int32_t DEFAULT_DEGREES_OF_FREEDOM = 0;
inline constexpr std::int16_t DEFAULT_RAS_GOOD_FLAG = 1;
inline constexpr std::array<float, 3> DEFAULT_SPACING {1.0F, 1.0F, 1.0F};
inline constexpr std::array<float, 9> DEFAULT_DIRECTION_COSINES {
	-1.0F, 0.0F, 0.0F,
	0.0F, 0.0F, -1.0F,
	0.0F, 1.0F, 0.0F,
};
inline constexpr std::array<float, 3> DEFAULT_CENTER {0.0F, 0.0F, 0.0F};

} // namespace mgh

namespace nifti {

inline constexpr std::array<double, 3> LPS_TO_RAS_SIGN {-1.0, -1.0, 1.0};

} // namespace nifti

} // namespace OpenHC::imaging::mri::fastsurfer::constants