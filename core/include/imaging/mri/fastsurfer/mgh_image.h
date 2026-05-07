#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "imaging/mri/fastsurfer/constants.h"
#include "imaging/mri/fastsurfer/image_metadata.h"

namespace OpenHC::imaging::mri::fastsurfer {

// Homogeneous 4x4 affine transform expressed in RAS world coordinates.
using Matrix4 = std::array<std::array<double, 4>, 4>;

enum class MghDataType : std::int32_t {
    // Unsigned 8-bit voxel storage.
    UChar = 0,
    // Signed 32-bit integer voxel storage.
    Int32 = 1,
    // IEEE-754 single-precision voxel storage.
    Float32 = 3,
    // Signed 16-bit integer voxel storage.
    Int16 = 4,
};

// In-memory representation of a FreeSurfer MGH/MGZ volume and its header.
class MghImage {
public:
    // Serializable subset of the MGH header used by the native FastSurfer
    // conform pipeline.
    struct Header {
        // Format version written into the MGH header.
        std::int32_t version {constants::mgh::DEFAULT_VERSION};
        // Voxel dimensions along the storage x, y, and z axes.
        std::array<int, 3> dimensions {0, 0, 0};
        // Number of frames stored in the payload. The native conform path
        // currently expects this to be 1.
        std::int32_t frames {constants::mgh::DEFAULT_FRAME_COUNT};
        // MGH storage type code, typically one of MghDataType.
        std::int32_t type {static_cast<std::int32_t>(MghDataType::UChar)};
        // Legacy FreeSurfer degrees-of-freedom field preserved on round-trip.
        std::int32_t degreesOfFreedom {constants::mgh::DEFAULT_DEGREES_OF_FREEDOM};
        // Non-zero when spacing, direction cosines, and center are valid RAS
        // geometry metadata.
        std::int16_t rasGoodFlag {constants::mgh::DEFAULT_RAS_GOOD_FLAG};
        // Voxel spacing in millimeters for each storage axis.
        std::array<float, 3> spacing {constants::mgh::DEFAULT_SPACING};
        // Column-major direction cosines describing the RAS orientation of each
        // storage axis.
        std::array<float, 9> directionCosines {constants::mgh::DEFAULT_DIRECTION_COSINES};
        // RAS-space center point encoded in the MGH header.
        std::array<float, 3> center {constants::mgh::DEFAULT_CENTER};
    };

    MghImage() = default;
    // Builds an image from a parsed header and raw voxel payload. The payload
    // size must match the dimensions and storage type declared in the header.
    // Parameters:
    // - header: Parsed MGH header describing geometry and storage type.
    // - rawData: Serialized voxel payload encoded according to header.type.
    MghImage(Header header, std::vector<std::uint8_t> rawData);

    // Loads an MGH or MGZ volume from disk and parses its header plus payload.
    // Parameters:
    // - path: Input MGZ or MGH file to read.
    // Returns the decoded image and header.
    static MghImage load(const std::filesystem::path &path);

    // Encodes floating-point voxel values into the requested MGH storage type
    // and returns a fully constructed image.
    // Parameters:
    // - header: Target MGH header that supplies geometry and default metadata.
    // - values: Floating-point voxel values in storage order.
    // - storageType: MGH type code used to encode the output payload.
    // Returns the encoded image.
    static MghImage fromVoxelData(Header header, const std::vector<float> &values, std::int32_t storageType);

    // Serializes the current image as a gzip-compressed MGZ file.
    // Parameters:
    // - path: Output MGZ path to create or overwrite.
    void save(const std::filesystem::path &path) const;

    // Returns the parsed MGH header.
    const Header &header() const;

    // Returns the raw voxel payload encoded exactly as stored in the file.
    const std::vector<std::uint8_t> &rawData() const;

    // Returns the total number of voxels across all frames.
    std::size_t voxelCount() const;

    // Returns true when the image contains a single 3D frame.
    bool hasSingleFrame() const;

    // Returns true when the image payload is stored as unsigned 8-bit values.
    bool isUint8() const;

    // Returns true when the image dimensions match the supplied triplet.
    // Parameters:
    // - dimensions: Expected x, y, and z dimensions.
    bool hasDimensions(const std::array<int, 3> &dimensions) const;

    // Returns true when all voxel spacings are within epsilon of the requested
    // isotropic spacing.
    // Parameters:
    // - targetSpacing: Expected isotropic spacing in millimeters.
    // - epsilon: Allowed absolute difference for each spacing component.
    bool hasIsotropicSpacing(float targetSpacing, float epsilon = constants::conform::VOXEL_EPSILON) const;

    // Returns true when the image orientation code matches the requested
    // three-letter orientation string.
    // Parameters:
    // - targetOrientation: Expected orientation code such as LIA or RAS.
    bool matchesOrientation(const std::string &targetOrientation) const;

    // Builds the 4x4 vox2ras affine implied by the MGH header geometry.
    Matrix4 affine() const;

    // Returns the smallest voxel spacing across the three spatial axes.
    float minVoxelSize() const;

    // Derives the three-letter orientation code from the image affine.
    std::string orientationCode() const;

    // Decodes the stored voxel payload into floating-point samples.
    std::vector<float> voxelDataAsFloat() const;

    // Returns a compact metadata summary used by the conform-step API.
    ImageMetadata metadata() const;

private:
    // Parsed MGH header for the current volume.
    Header m_header;
    // Raw voxel bytes stored using the type declared in m_header.type.
    std::vector<std::uint8_t> m_rawData;
};

// Returns a readable storage-type name for diagnostics and metadata reports.
// Parameters:
// - typeCode: MGH storage type code.
// Returns a stable human-readable name for the type code.
std::string toDataTypeName(std::int32_t typeCode);

// Returns the number of bytes consumed by one voxel of the given MGH type.
// Parameters:
// - typeCode: MGH storage type code.
// Returns the serialized byte width of one voxel value.
std::size_t bytesPerVoxel(std::int32_t typeCode);

} // namespace OpenHC::imaging::mri::fastsurfer