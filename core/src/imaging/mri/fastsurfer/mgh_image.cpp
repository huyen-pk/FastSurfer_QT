#include "imaging/mri/fastsurfer/mgh_image.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

#include <zlib.h>

#include "imaging/mri/fastsurfer/constants.h"

namespace OpenHC::imaging::mri::fastsurfer {
namespace {

using Matrix3 = std::array<std::array<double, 3>, 3>;
using Vector3 = std::array<double, 3>;
using Vector4 = std::array<double, 4>;

std::int16_t readInt16BigEndian(const std::uint8_t *data)
{
    return static_cast<std::int16_t>((static_cast<std::uint16_t>(data[0]) << 8U) |
                                     static_cast<std::uint16_t>(data[1]));
}

std::int32_t readInt32BigEndian(const std::uint8_t *data)
{
    return static_cast<std::int32_t>((static_cast<std::uint32_t>(data[0]) << 24U) |
                                     (static_cast<std::uint32_t>(data[1]) << 16U) |
                                     (static_cast<std::uint32_t>(data[2]) << 8U) |
                                     static_cast<std::uint32_t>(data[3]));
}

float readFloatBigEndian(const std::uint8_t *data)
{
    const auto bits = static_cast<std::uint32_t>((static_cast<std::uint32_t>(data[0]) << 24U) |
                                                 (static_cast<std::uint32_t>(data[1]) << 16U) |
                                                 (static_cast<std::uint32_t>(data[2]) << 8U) |
                                                 static_cast<std::uint32_t>(data[3]));
    float value = 0.0F;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void writeInt16BigEndian(std::uint8_t *data, std::int16_t value)
{
    data[0] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(value) >> 8U) & 0xFFU);
    data[1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(value) & 0xFFU);
}

void writeInt32BigEndian(std::uint8_t *data, std::int32_t value)
{
    const auto unsignedValue = static_cast<std::uint32_t>(value);
    data[0] = static_cast<std::uint8_t>((unsignedValue >> 24U) & 0xFFU);
    data[1] = static_cast<std::uint8_t>((unsignedValue >> 16U) & 0xFFU);
    data[2] = static_cast<std::uint8_t>((unsignedValue >> 8U) & 0xFFU);
    data[3] = static_cast<std::uint8_t>(unsignedValue & 0xFFU);
}

void writeFloatBigEndian(std::uint8_t *data, float value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    data[0] = static_cast<std::uint8_t>((bits >> 24U) & 0xFFU);
    data[1] = static_cast<std::uint8_t>((bits >> 16U) & 0xFFU);
    data[2] = static_cast<std::uint8_t>((bits >> 8U) & 0xFFU);
    data[3] = static_cast<std::uint8_t>(bits & 0xFFU);
}

std::int32_t readInt32Value(const std::uint8_t *data, const std::int32_t typeCode)
{
    switch (static_cast<MghDataType>(typeCode)) {
    case MghDataType::UChar:
        return data[0];
    case MghDataType::Int16:
        return readInt16BigEndian(data);
    case MghDataType::Int32:
        return readInt32BigEndian(data);
    case MghDataType::Float32:
        return static_cast<std::int32_t>(std::lround(readFloatBigEndian(data)));
    }

    throw std::runtime_error("Unsupported MGH data type code: " + std::to_string(typeCode));
}

float readVoxelAsFloat(const std::uint8_t *data, const std::int32_t typeCode)
{
    switch (static_cast<MghDataType>(typeCode)) {
    case MghDataType::UChar:
        return static_cast<float>(data[0]);
    case MghDataType::Int16:
        return static_cast<float>(readInt16BigEndian(data));
    case MghDataType::Int32:
        return static_cast<float>(readInt32BigEndian(data));
    case MghDataType::Float32:
        return readFloatBigEndian(data);
    }

    throw std::runtime_error("Unsupported MGH data type code: " + std::to_string(typeCode));
}

void writeVoxelFromFloat(std::uint8_t *data, const float value, const std::int32_t typeCode)
{
    switch (static_cast<MghDataType>(typeCode)) {
    case MghDataType::UChar: {
        const auto rounded = std::clamp<std::int32_t>(static_cast<std::int32_t>(std::lround(value)), 0, 255);
        data[0] = static_cast<std::uint8_t>(rounded);
        return;
    }
    case MghDataType::Int16: {
        const auto rounded = std::clamp<std::int32_t>(static_cast<std::int32_t>(std::lround(value)),
                                                      std::numeric_limits<std::int16_t>::min(),
                                                      std::numeric_limits<std::int16_t>::max());
        writeInt16BigEndian(data, static_cast<std::int16_t>(rounded));
        return;
    }
    case MghDataType::Int32:
        writeInt32BigEndian(data, static_cast<std::int32_t>(std::lround(value)));
        return;
    case MghDataType::Float32:
        writeFloatBigEndian(data, value);
        return;
    }

    throw std::runtime_error("Unsupported MGH data type code: " + std::to_string(typeCode));
}

std::string axisCodeForVector(const Vector3 &vector)
{
    constexpr std::array<char, 3> positive {'R', 'A', 'S'};
    constexpr std::array<char, 3> negative {'L', 'P', 'I'};

    std::size_t dominantAxis = 0;
    float dominantMagnitude = 0.0F;
    for (std::size_t axis = 0; axis < vector.size(); ++axis) {
        const float magnitude = std::fabs(vector[axis]);
        if (magnitude > dominantMagnitude) {
            dominantMagnitude = magnitude;
            dominantAxis = axis;
        }
    }

    if (dominantMagnitude <= std::numeric_limits<float>::epsilon()) {
        return "?";
    }

    return std::string(1, vector[dominantAxis] >= 0.0F ? positive[dominantAxis] : negative[dominantAxis]);
}

std::string orientationFromAffine(const Matrix4 &affine)
{
    std::string result;
    result.reserve(3);
    for (int column = 0; column < 3; ++column) {
        result += axisCodeForVector({
            affine[0][column],
            affine[1][column],
            affine[2][column],
        });
    }
    return result;
}

Matrix3 inverse3x3(const Matrix3 &matrix)
{
    const double determinant =
        matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
        matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
        matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);

    if (std::fabs(determinant) <= std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("MGH affine is singular and cannot be inverted.");
    }

    const double invDet = 1.0 / determinant;
    return Matrix3 {{
        {{(matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) * invDet,
          (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) * invDet,
          (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) * invDet}},
        {{(matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) * invDet,
          (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) * invDet,
          (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) * invDet}},
        {{(matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) * invDet,
          (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) * invDet,
          (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) * invDet}},
    }};
}

std::size_t checkedVoxelCount(const MghImage::Header &header)
{
    if (header.dimensions[0] < 0 || header.dimensions[1] < 0 || header.dimensions[2] < 0 || header.frames < 0) {
        throw std::runtime_error("MGH header contains negative dimensions.");
    }

    const auto width = static_cast<std::size_t>(header.dimensions[0]);
    const auto height = static_cast<std::size_t>(header.dimensions[1]);
    const auto depth = static_cast<std::size_t>(header.dimensions[2]);
    const auto frames = static_cast<std::size_t>(header.frames);
    return width * height * depth * frames;
}

Vector4 multiply(const Matrix4 &matrix, const Vector4 &vector)
{
    Vector4 result {};
    for (std::size_t row = 0; row < 4; ++row) {
        result[row] = matrix[row][0] * vector[0] +
                      matrix[row][1] * vector[1] +
                      matrix[row][2] * vector[2] +
                      matrix[row][3] * vector[3];
    }
    return result;
}

} // namespace

MghImage::MghImage(Header header, std::vector<std::uint8_t> rawData)
    : m_header(std::move(header))
    , m_rawData(std::move(rawData))
{
    const auto expectedSize = checkedVoxelCount(m_header) * bytesPerVoxel(m_header.type);
    if (m_rawData.size() != expectedSize) {
        throw std::runtime_error("MGH voxel payload size does not match header dimensions.");
    }
}

std::string toDataTypeName(const std::int32_t typeCode)
{
    switch (static_cast<MghDataType>(typeCode)) {
    case MghDataType::UChar:
        return "uint8";
    case MghDataType::Int32:
        return "int32";
    case MghDataType::Float32:
        return "float32";
    case MghDataType::Int16:
        return "int16";
    }

    throw std::runtime_error("Unsupported MGH data type code: " + std::to_string(typeCode));
}

std::size_t bytesPerVoxel(const std::int32_t typeCode)
{
    switch (static_cast<MghDataType>(typeCode)) {
    case MghDataType::UChar:
        return 1;
    case MghDataType::Int16:
        return 2;
    case MghDataType::Int32:
    case MghDataType::Float32:
        return 4;
    }

    throw std::runtime_error("Unsupported MGH data type code: " + std::to_string(typeCode));
}

MghImage MghImage::fromVoxelData(Header header, const std::vector<float> &values, const std::int32_t storageType)
{
    header.type = storageType;
    const auto voxelCount = checkedVoxelCount(header);
    if (values.size() != voxelCount) {
        throw std::runtime_error("MGH voxel value count does not match header dimensions.");
    }

    std::vector<std::uint8_t> rawData(voxelCount * bytesPerVoxel(storageType));
    const auto stride = bytesPerVoxel(storageType);
    for (std::size_t index = 0; index < voxelCount; ++index) {
        writeVoxelFromFloat(rawData.data() + index * stride, values[index], storageType);
    }

    return MghImage(std::move(header), std::move(rawData));
}

MghImage MghImage::load(const std::filesystem::path &path)
{
    gzFile file = gzopen(path.string().c_str(), "rb");
    if (file == nullptr) {
        throw std::runtime_error("Failed to open MGZ file: " + path.string());
    }

    std::array<std::uint8_t, constants::mgh::HEADER_SIZE> headerBytes {};
    const int readCount = gzread(file, headerBytes.data(), static_cast<unsigned int>(headerBytes.size()));
    if (readCount != static_cast<int>(headerBytes.size())) {
        gzclose(file);
        throw std::runtime_error("Failed to read full MGZ header: " + path.string());
    }

    MghImage image;
    image.m_header.version = readInt32BigEndian(&headerBytes[0]);
    image.m_header.dimensions = {
        readInt32BigEndian(&headerBytes[4]),
        readInt32BigEndian(&headerBytes[8]),
        readInt32BigEndian(&headerBytes[12]),
    };
    image.m_header.frames = readInt32BigEndian(&headerBytes[16]);
    image.m_header.type = readInt32BigEndian(&headerBytes[20]);
    image.m_header.degreesOfFreedom = readInt32BigEndian(&headerBytes[24]);
    image.m_header.rasGoodFlag = readInt16BigEndian(&headerBytes[constants::mgh::RAS_GOOD_FLAG_OFFSET]);

    std::size_t offset = constants::mgh::SPACING_OFFSET;
    for (float &spacing : image.m_header.spacing) {
        spacing = readFloatBigEndian(&headerBytes[offset]);
        offset += 4;
    }
    for (float &value : image.m_header.directionCosines) {
        value = readFloatBigEndian(&headerBytes[offset]);
        offset += 4;
    }
    for (float &centerValue : image.m_header.center) {
        centerValue = readFloatBigEndian(&headerBytes[offset]);
        offset += 4;
    }

    const auto payloadSize = checkedVoxelCount(image.m_header) * bytesPerVoxel(image.m_header.type);
    image.m_rawData.resize(payloadSize);
    if (payloadSize > 0) {
        const int payloadRead = gzread(file, image.m_rawData.data(), static_cast<unsigned int>(payloadSize));
        if (payloadRead != static_cast<int>(payloadSize)) {
            gzclose(file);
            throw std::runtime_error("Failed to read MGZ payload: " + path.string());
        }
    }

    gzclose(file);
    return image;
}

void MghImage::save(const std::filesystem::path &path) const
{
    std::filesystem::create_directories(path.parent_path());

    std::array<std::uint8_t, constants::mgh::HEADER_SIZE> headerBytes {};
    writeInt32BigEndian(&headerBytes[0], m_header.version);
    writeInt32BigEndian(&headerBytes[4], m_header.dimensions[0]);
    writeInt32BigEndian(&headerBytes[8], m_header.dimensions[1]);
    writeInt32BigEndian(&headerBytes[12], m_header.dimensions[2]);
    writeInt32BigEndian(&headerBytes[16], m_header.frames);
    writeInt32BigEndian(&headerBytes[20], m_header.type);
    writeInt32BigEndian(&headerBytes[24], m_header.degreesOfFreedom);
    writeInt16BigEndian(&headerBytes[constants::mgh::RAS_GOOD_FLAG_OFFSET], m_header.rasGoodFlag);

    std::size_t offset = constants::mgh::SPACING_OFFSET;
    for (const float spacing : m_header.spacing) {
        writeFloatBigEndian(&headerBytes[offset], spacing);
        offset += 4;
    }
    for (const float value : m_header.directionCosines) {
        writeFloatBigEndian(&headerBytes[offset], value);
        offset += 4;
    }
    for (const float centerValue : m_header.center) {
        writeFloatBigEndian(&headerBytes[offset], centerValue);
        offset += 4;
    }

    gzFile file = gzopen(path.string().c_str(), "wb");
    if (file == nullptr) {
        throw std::runtime_error("Failed to create MGZ file: " + path.string());
    }

    const int headerWrite = gzwrite(file, headerBytes.data(), static_cast<unsigned int>(headerBytes.size()));
    if (headerWrite != static_cast<int>(headerBytes.size())) {
        gzclose(file);
        throw std::runtime_error("Failed to write MGZ header: " + path.string());
    }

    if (!m_rawData.empty()) {
        const int payloadWrite = gzwrite(file, m_rawData.data(), static_cast<unsigned int>(m_rawData.size()));
        if (payloadWrite != static_cast<int>(m_rawData.size())) {
            gzclose(file);
            throw std::runtime_error("Failed to write MGZ payload: " + path.string());
        }
    }

    gzclose(file);
}

const MghImage::Header &MghImage::header() const
{
    return m_header;
}

const std::vector<std::uint8_t> &MghImage::rawData() const
{
    return m_rawData;
}

std::size_t MghImage::voxelCount() const
{
    return checkedVoxelCount(m_header);
}

bool MghImage::hasSingleFrame() const
{
    return m_header.frames == 1;
}

bool MghImage::isUint8() const
{
    return m_header.type == static_cast<std::int32_t>(MghDataType::UChar);
}

bool MghImage::hasDimensions(const std::array<int, 3> &dimensions) const
{
    return m_header.dimensions == dimensions;
}

bool MghImage::hasIsotropicSpacing(const float targetSpacing, const float epsilon) const
{
    return std::all_of(m_header.spacing.begin(), m_header.spacing.end(), [targetSpacing, epsilon](const float value) {
        return std::fabs(value - targetSpacing) <= epsilon;
    });
}

bool MghImage::matchesOrientation(const std::string &targetOrientation) const
{
    std::string normalizedTarget = targetOrientation;
    std::transform(normalizedTarget.begin(), normalizedTarget.end(), normalizedTarget.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });

    return orientationCode() == normalizedTarget;
}

Matrix4 MghImage::affine() const
{
    Matrix4 affine {{
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 1.0}},
    }};

    for (int column = 0; column < 3; ++column) {
        for (int row = 0; row < 3; ++row) {
            affine[row][column] = static_cast<double>(m_header.directionCosines[column * 3 + row]) *
                                  static_cast<double>(m_header.spacing[column]);
        }
    }

    const Vector3 halfDimensions {
        static_cast<double>(m_header.dimensions[0]) / 2.0,
        static_cast<double>(m_header.dimensions[1]) / 2.0,
        static_cast<double>(m_header.dimensions[2]) / 2.0,
    };

    for (int row = 0; row < 3; ++row) {
        double offset = static_cast<double>(m_header.center[row]);
        for (int column = 0; column < 3; ++column) {
            offset -= affine[row][column] * halfDimensions[column];
        }
        affine[row][3] = offset;
    }

    return affine;
}

float MghImage::minVoxelSize() const
{
    return *std::min_element(m_header.spacing.begin(), m_header.spacing.end());
}

std::string MghImage::orientationCode() const
{
    return orientationFromAffine(affine());
}

std::vector<float> MghImage::voxelDataAsFloat() const
{
    std::vector<float> values(voxelCount());
    const auto stride = bytesPerVoxel(m_header.type);
    for (std::size_t index = 0; index < values.size(); ++index) {
        values[index] = readVoxelAsFloat(m_rawData.data() + index * stride, m_header.type);
    }
    return values;
}

ImageMetadata MghImage::metadata() const
{
    ImageMetadata metadata;
    metadata.dimensions = m_header.dimensions;
    metadata.frames = m_header.frames;
    metadata.spacing = m_header.spacing;
    metadata.orientationCode = orientationCode();
    metadata.dataTypeName = toDataTypeName(m_header.type);
    metadata.rasGood = m_header.rasGoodFlag != 0;
    return metadata;
}

} // namespace OpenHC::imaging::mri::fastsurfer