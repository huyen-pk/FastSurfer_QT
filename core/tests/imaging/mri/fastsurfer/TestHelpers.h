#ifndef OPENHC_IMAGING_MRI_FASTSURFER_TESTS_TESTHELPERS_H
#define OPENHC_IMAGING_MRI_FASTSURFER_TESTS_TESTHELPERS_H

#include <cmath>
#include <stdexcept>
#include <string>
#include "TestConstants.h"

// Throws when a test precondition or assertion is false.
// Parameters:
// - condition: Assertion result that must evaluate to true.
// - message: Failure message to surface when the assertion fails.
inline void require(const bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename L, typename R, typename T>
// Throws when the absolute difference between left and right exceeds tolerance.
// Parameters:
// - left: Actual value.
// - right: Expected value.
// - tolerance: Maximum allowed absolute error.
// - message: Failure message to surface when the assertion fails.
inline void requireNear(const L left, const R right, const T tolerance, const std::string &message)
{
    if (std::fabs(static_cast<double>(left - right)) > static_cast<double>(tolerance)) {
        throw std::runtime_error(message + " Expected=" + std::to_string(static_cast<double>(right)) + " actual=" + std::to_string(static_cast<double>(left)));
    }
}

// Uses the shared voxel tolerance for float comparisons.
// Parameters:
// - left: Actual value.
// - right: Expected value.
// - message: Failure message to surface when the assertion fails.
inline void requireNear(const float left, const float right, const std::string &message)
{
    requireNear(left, right, test_constants::CONFORM_POLICY_VOXEL_TOLERANCE, message);
}

// Uses the shared voxel tolerance for double comparisons.
// Parameters:
// - left: Actual value.
// - right: Expected value.
// - message: Failure message to surface when the assertion fails.
inline void requireNear(const double left, const double right, const std::string &message)
{
    requireNear(left, right, static_cast<double>(test_constants::CONFORM_POLICY_VOXEL_TOLERANCE), message);
}

#endif // OPENHC_IMAGING_MRI_FASTSURFER_TESTS_TESTHELPERS_H
