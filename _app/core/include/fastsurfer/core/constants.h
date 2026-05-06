#pragma once

#include <array>
#include <cstddef>

namespace fastsurfer::core::constants {

namespace conform {

inline constexpr float UnitVoxelSizeMm = 1.0F;
inline constexpr float DefaultThreshold1mm = 0.95F;
inline constexpr int DefaultAutoImageDimension = 256;
inline constexpr float VoxelEpsilon = 1.0e-4F;
inline constexpr double RotationEpsilon = 1.0e-6;

} // namespace conform

namespace mgh {

inline constexpr std::size_t HeaderSize = 284;
inline constexpr std::size_t RasGoodFlagOffset = 28;
inline constexpr std::size_t SpacingOffset = 30;

} // namespace mgh

namespace nifti {

inline constexpr std::array<double, 3> LpsToRasSign {-1.0, -1.0, 1.0};

} // namespace nifti

} // namespace fastsurfer::core::constants