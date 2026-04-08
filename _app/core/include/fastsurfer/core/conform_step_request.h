#pragma once

#include <filesystem>
#include <string>

namespace fastsurfer::core {

struct ConformStepRequest {
    std::filesystem::path inputPath;
    std::filesystem::path copyOrigPath;
    std::filesystem::path conformedPath;
    std::string voxSizeMode {"min"};
    std::string imageSizeMode {"auto"};
    std::string orientation {"lia"};
    float threshold1mm {0.95F};
};

} // namespace fastsurfer::core