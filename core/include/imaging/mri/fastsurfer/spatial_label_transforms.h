#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace OpenHC::imaging::mri::fastsurfer {

class UnknownCortexFillTransform {
public:
    using Dimensions = std::array<int, 3>;

    void applyInPlace(std::vector<std::uint16_t> &labels, const Dimensions &dimensions) const;
};

class SplitCortexLabelsTransform {
public:
    using Dimensions = std::array<int, 3>;

    void applyInPlace(std::vector<std::uint16_t> &labels, const Dimensions &dimensions) const;
};

} // namespace OpenHC::imaging::mri::fastsurfer