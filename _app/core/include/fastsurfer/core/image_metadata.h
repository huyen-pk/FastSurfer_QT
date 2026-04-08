#pragma once

#include <array>
#include <string>

namespace fastsurfer::core {

struct ImageMetadata {
    std::array<int, 3> dimensions {0, 0, 0};
    int frames {0};
    std::array<float, 3> spacing {0.0F, 0.0F, 0.0F};
    std::string orientationCode;
    std::string dataTypeName;
    bool rasGood {false};
};

} // namespace fastsurfer::core