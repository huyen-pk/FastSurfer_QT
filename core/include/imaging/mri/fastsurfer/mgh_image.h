#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "imaging/mri/fastsurfer/constants.h"
#include "imaging/mri/fastsurfer/image_metadata.h"

namespace OpenHC::imaging::mri::fastsurfer {

using Matrix4 = std::array<std::array<double, 4>, 4>;

enum class MghDataType : std::int32_t {
    UChar = 0,
    Int32 = 1,
    Float32 = 3,
    Int16 = 4,
};

class MghImage {
public:
    struct Header {
        std::int32_t version {constants::mgh::DEFAULT_VERSION};
        std::array<int, 3> dimensions {0, 0, 0};
        std::int32_t frames {constants::mgh::DEFAULT_FRAME_COUNT};
        std::int32_t type {static_cast<std::int32_t>(MghDataType::UChar)};
        std::int32_t degreesOfFreedom {constants::mgh::DEFAULT_DEGREES_OF_FREEDOM};
        std::int16_t rasGoodFlag {constants::mgh::DEFAULT_RAS_GOOD_FLAG};
        std::array<float, 3> spacing {constants::mgh::DEFAULT_SPACING};
        std::array<float, 9> directionCosines {constants::mgh::DEFAULT_DIRECTION_COSINES};
        std::array<float, 3> center {constants::mgh::DEFAULT_CENTER};
    };

    MghImage() = default;
    MghImage(Header header, std::vector<std::uint8_t> rawData);

    static MghImage load(const std::filesystem::path &path);
    static MghImage fromVoxelData(Header header, const std::vector<float> &values, std::int32_t storageType);

    void save(const std::filesystem::path &path) const;

    const Header &header() const;
    const std::vector<std::uint8_t> &rawData() const;
    std::size_t voxelCount() const;

    bool hasSingleFrame() const;
    bool isUint8() const;
    bool hasDimensions(const std::array<int, 3> &dimensions) const;
    bool hasIsotropicSpacing(float targetSpacing, float epsilon = constants::conform::VOXEL_EPSILON) const;
    bool matchesOrientation(const std::string &targetOrientation) const;

    Matrix4 affine() const;
    float minVoxelSize() const;
    std::string orientationCode() const;
    std::vector<float> voxelDataAsFloat() const;
    ImageMetadata metadata() const;

private:
    Header m_header;
    std::vector<std::uint8_t> m_rawData;
};

std::string toDataTypeName(std::int32_t typeCode);
std::size_t bytesPerVoxel(std::int32_t typeCode);

} // namespace OpenHC::imaging::mri::fastsurfer