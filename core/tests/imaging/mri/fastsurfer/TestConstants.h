#pragma once

namespace test_constants {

// Hard validation contract for _app/core tests.
// Do not widen these thresholds without deliberate parity review.

inline constexpr float CONFORM_THRESHOLD_1MM = 0.95F;
inline constexpr float UNIT_VOXEL_SIZE_MM = 1.0F;
// Tolerance used when comparing computed target voxel sizes in unit tests.
inline constexpr float CONFORM_POLICY_VOXEL_TOLERANCE = 1.0e-5F;
inline constexpr float FLOAT_ROUNDING_SCALE = 10000.0F;
inline constexpr double DOUBLE_ROUNDING_SCALE = 10000.0;

inline constexpr double AFFINE_LINEAR_TOLERANCE = 1.0e-4;
inline constexpr double AFFINE_TRANSLATION_TOLERANCE_MM = 1.0e-4;
inline constexpr double HOMOGENEOUS_ROW_TOLERANCE = 1.0e-6;

inline constexpr float METADATA_SPACING_TOLERANCE = 1.0e-5F;
inline constexpr float PARITY_SPACING_TOLERANCE = 1.0e-4F;

inline constexpr double DIRECTION_MATRIX_TOLERANCE = 1.0e-5;
inline constexpr double ORTHONORMALITY_TOLERANCE = 1.0e-5;
inline constexpr double DETERMINANT_TOLERANCE = 1.0e-5;
inline constexpr double PHYSICAL_POINT_TOLERANCE_MM = 1.0e-4;

inline constexpr double ROTATION_UNIT_TOLERANCE = 1.0e-4;
inline constexpr double ROTATION_ZERO_TOLERANCE = 1.0e-6;

inline constexpr float HISTOGRAM_NON_ZERO_EPSILON = 1.0e-15F;
inline constexpr float SCALED_ZERO_EPSILON = 1.0e-8F;

inline constexpr double VOXEL_EXPECTATION_TOLERANCE = 0.5;
inline constexpr double PROBE_MAX_INTENSITY_ERROR_TOLERANCE = 2.0;
// This gate is measured in uint8 intensity levels, not percent. A value of 1.0 means
// at least 95% of sampled probes must be exact or off by at most one grayscale level.
inline constexpr double PROBE_PERCENTILE95_INTENSITY_ERROR_TOLERANCE = 1.0;
inline constexpr double PROBE_PERCENTILE_FRACTION = 0.95;

inline constexpr double PYTHON_FOREGROUND_MEAN_ABS_TOLERANCE = 0.5;
inline constexpr double PYTHON_LOCALIZED_MAX_DIFF_TOLERANCE = 2.0;

inline constexpr int STRATIFIED_PROBE_BIN_COUNT = 6;
inline constexpr int JITTERED_PROBE_BIN_COUNT = 3;
inline constexpr unsigned int JITTERED_PROBE_SEED = 1337U;

} // namespace test_constants