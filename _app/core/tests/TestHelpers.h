#ifndef FASTSURFER_CORE_TESTS_TESTHELPERS_H
#define FASTSURFER_CORE_TESTS_TESTHELPERS_H

#include <cmath>
#include <stdexcept>
#include <string>
#include "TestConstants.h"

inline void require(const bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename L, typename R, typename T>
inline void requireNear(const L left, const R right, const T tolerance, const std::string &message)
{
    if (std::fabs(static_cast<double>(left - right)) > static_cast<double>(tolerance)) {
        throw std::runtime_error(message + " Expected=" + std::to_string(static_cast<double>(right)) + " actual=" + std::to_string(static_cast<double>(left)));
    }
}

inline void requireNear(const float left, const float right, const std::string &message)
{
    requireNear(left, right, test_constants::CONFORM_POLICY_VOXEL_TOLERANCE, message);
}

inline void requireNear(const double left, const double right, const std::string &message)
{
    requireNear(left, right, static_cast<double>(test_constants::CONFORM_POLICY_VOXEL_TOLERANCE), message);
}

#endif // FASTSURFER_CORE_TESTS_TESTHELPERS_H
