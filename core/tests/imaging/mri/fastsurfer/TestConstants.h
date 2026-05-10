#pragma once

#include <array>
#include <cstdint>

#include "imaging/mri/fastsurfer/constants.h"

namespace test_constants {

// Hard validation contract for _app/core tests.
// Do not widen these thresholds without deliberate parity review.

// Shared threshold for the 1 mm snapping behavior under test.
inline constexpr float CONFORM_THRESHOLD_1MM = OpenHC::imaging::mri::fastsurfer::constants::conform::DEFAULT_THRESHOLD_1MM;
// Canonical target voxel size used by conform outputs.
inline constexpr float UNIT_VOXEL_SIZE_MM = OpenHC::imaging::mri::fastsurfer::constants::conform::UNIT_VOXEL_SIZE_MM;
// Tolerance used when comparing computed target voxel sizes in unit tests.
inline constexpr float CONFORM_POLICY_VOXEL_TOLERANCE = 1.0e-5F;
// Shared decimal precision used by the conform rounding checks.
inline constexpr float FLOAT_ROUNDING_SCALE = OpenHC::imaging::mri::fastsurfer::constants::conform::FLOAT_ROUNDING_SCALE;
inline constexpr double DOUBLE_ROUNDING_SCALE = OpenHC::imaging::mri::fastsurfer::constants::conform::DOUBLE_ROUNDING_SCALE;

// Tolerance for affine linear-term comparisons.
inline constexpr double AFFINE_LINEAR_TOLERANCE = 1.0e-4;
// Tolerance for affine translation comparisons in millimeters.
inline constexpr double AFFINE_TRANSLATION_TOLERANCE_MM = 1.0e-4;
// Tolerance for the affine homogeneous row checks.
inline constexpr double HOMOGENEOUS_ROW_TOLERANCE = 1.0e-6;

// Tolerance for metadata spacing values reported by the native API.
inline constexpr float METADATA_SPACING_TOLERANCE = 1.0e-5F;
// Tolerance for spacing comparisons against Python parity artifacts.
inline constexpr float PARITY_SPACING_TOLERANCE = 1.0e-4F;

inline constexpr double DIRECTION_MATRIX_TOLERANCE = 1.0e-5;
inline constexpr double ORTHONORMALITY_TOLERANCE = 1.0e-5;
inline constexpr double DETERMINANT_TOLERANCE = 1.0e-5;
inline constexpr double PHYSICAL_POINT_TOLERANCE_MM = 1.0e-4;

inline constexpr double ROTATION_UNIT_TOLERANCE = 1.0e-4;
inline constexpr double ROTATION_ZERO_TOLERANCE = 1.0e-6;

inline constexpr float HISTOGRAM_NON_ZERO_EPSILON = OpenHC::imaging::mri::fastsurfer::constants::conform::HISTOGRAM_NON_ZERO_EPSILON;
inline constexpr float SCALED_ZERO_EPSILON = OpenHC::imaging::mri::fastsurfer::constants::conform::SCALED_ZERO_EPSILON;

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

inline constexpr std::size_t COLIN27_MANUAL_DKT31_RAW_UNIQUE_LABEL_COUNT = 63U;
inline constexpr std::size_t COLIN27_STEP6_UNIQUE_LABEL_COUNT = 79U;
inline constexpr std::size_t COLIN27_STEP7_UNIQUE_LABEL_COUNT = 96U;
inline constexpr std::size_t COLIN27_MANUAL_ASEG_RAW_UNIQUE_LABEL_COUNT = 107U;
inline constexpr std::size_t COLIN27_APARC_ASEG_RAW_UNIQUE_LABEL_COUNT = 113U;

inline constexpr std::array<std::uint16_t, 17> COLIN27_SPLIT_STAGE_EXPECTED_INTRODUCED {
	2003, 2006, 2007, 2008, 2009, 2011, 2015, 2018, 2019, 2020, 2026, 2027, 2029, 2030, 2031, 2034, 2035,
};

inline constexpr std::array<std::uint16_t, 11> COLIN27_MANUAL_ASEG_EXPECTED_RAW_MISSING {
	30, 62, 80, 85, 251, 252, 253, 254, 255, 1000, 2000,
};

inline constexpr std::array<std::uint16_t, 17> COLIN27_APARC_ASEG_EXPECTED_RAW_MISSING {
	30, 62, 80, 85, 251, 252, 253, 254, 255, 1000, 1001, 1032, 1033, 2000, 2001, 2032, 2033,
};

inline constexpr std::array<std::uint16_t, 6> COLIN27_APARC_ASEG_EXPECTED_UNSUPPORTED {
	1001, 1032, 1033, 2001, 2032, 2033,
};

inline constexpr std::array<std::uint16_t, 15> SYNTHETIC_MANUAL_ASEG_LABELS {
	30, 62, 80, 85, 251, 252, 253, 254, 255, 1002, 1000, 1003, 2002, 2000, 2003,
};

inline constexpr std::array<std::uint16_t, 15> SYNTHETIC_MANUAL_ASEG_NO_CC_REFERENCE {
	30, 62, 80, 85, 2, 2, 41, 41, 0, 1002, 1000, 1003, 2002, 2000, 2003,
};

inline constexpr std::array<std::uint16_t, 6> SYNTHETIC_APARC_UNSUPPORTED_LABELS {
	1001, 1032, 1033, 2001, 2032, 2033,
};

inline constexpr std::array<std::uint8_t, 4> BRATS_BRIDGE_INPUT_LABELS {0, 1, 2, 3};
inline constexpr std::array<std::uint8_t, 4> BRATS_BRIDGE_EXPECTED_FOREGROUND {0, 1, 1, 1};

inline constexpr std::array<std::uint16_t, 4> FASTSURFER_BRAIN_FOREGROUND_INPUT_LABELS {0, 2, 41, 1002};
inline constexpr std::array<std::uint8_t, 4> FASTSURFER_BRAIN_FOREGROUND_EXPECTED_MASK {0, 1, 1, 1};

} // namespace test_constants