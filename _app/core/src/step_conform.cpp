#include "fastsurfer/core/step_conform.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>

#include <itkImage.h>
#include <itkImportImageFilter.h>

#include "fastsurfer/core/constants.h"
#include "fastsurfer/core/conform_policy.h"
#include "fastsurfer/core/mgh_image.h"
#include "fastsurfer/core/nifti_converter.h"

namespace fastsurfer::core {
namespace {

using Matrix3 = std::array<std::array<double, 3>, 3>;
using Vector3 = std::array<double, 3>;
using Vector4 = std::array<double, 4>;
using FloatImage3D = itk::Image<float, 3>;

struct OrientationTransform {
    std::array<int, 3> axes {0, 1, 2};
    std::array<int, 3> flips {1, 1, 1};
};

std::string normalizeUpper(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

std::string resolveOrientationCode(const OrientationMode requestedOrientation, const std::string &sourceOrientation)
{
    if (requestedOrientation == OrientationMode::Native) {
        return sourceOrientation;
    }

    return normalizeUpper(to_string(requestedOrientation));
}

int axisGroup(const char axisCode)
{
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(axisCode)))) {
    case 'L':
    case 'R':
        return 0;
    case 'P':
    case 'A':
        return 1;
    case 'I':
    case 'S':
        return 2;
    default:
        throw std::runtime_error(std::string("Unsupported orientation axis code: '") + axisCode + "'.");
    }
}

Vector3 axisVector(const char axisCode)
{
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(axisCode)))) {
    case 'R':
        return {1.0, 0.0, 0.0};
    case 'L':
        return {-1.0, 0.0, 0.0};
    case 'A':
        return {0.0, 1.0, 0.0};
    case 'P':
        return {0.0, -1.0, 0.0};
    case 'S':
        return {0.0, 0.0, 1.0};
    case 'I':
        return {0.0, 0.0, -1.0};
    default:
        throw std::runtime_error(std::string("Unsupported orientation axis code: '") + axisCode + "'.");
    }
}

OrientationTransform computeOrientationTransform(const std::string &sourceOrientation, const std::string &targetOrientation)
{
    OrientationTransform transform;
    for (int targetAxis = 0; targetAxis < 3; ++targetAxis) {
        const int targetGroup = axisGroup(targetOrientation[targetAxis]);
        bool found = false;
        for (int sourceAxis = 0; sourceAxis < 3; ++sourceAxis) {
            if (axisGroup(sourceOrientation[sourceAxis]) != targetGroup) {
                continue;
            }

            transform.axes[targetAxis] = sourceAxis;
            transform.flips[targetAxis] =
                std::toupper(static_cast<unsigned char>(sourceOrientation[sourceAxis])) ==
                        std::toupper(static_cast<unsigned char>(targetOrientation[targetAxis]))
                    ? 1
                    : -1;
            found = true;
            break;
        }

        if (!found) {
            throw std::runtime_error(
                "Could not derive orientation transform for MGZ conform step. Source='" +
                sourceOrientation + "' target='" + targetOrientation + "'.");
        }
    }
    return transform;
}

Matrix3 inverse3x3(const Matrix3 &matrix)
{
    const double determinant =
        matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
        matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
        matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);

    if (std::fabs(determinant) <= std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("Affine matrix is singular and cannot be inverted.");
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

Matrix3 multiply3x3(const Matrix3 &left, const Matrix3 &right)
{
    Matrix3 result {{
        {{0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0}},
    }};

    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            for (int k = 0; k < 3; ++k) {
                result[row][column] += left[row][k] * right[k][column];
            }
        }
    }

    return result;
}

Matrix4 multiply(const Matrix4 &left, const Matrix4 &right)
{
    Matrix4 result {{
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 0.0}},
    }};

    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            for (int k = 0; k < 4; ++k) {
                result[row][column] += left[row][k] * right[k][column];
            }
        }
    }
    return result;
}

Vector4 multiply(const Matrix4 &matrix, const Vector4 &vector)
{
    Vector4 result {};
    for (int row = 0; row < 4; ++row) {
        result[row] = matrix[row][0] * vector[0] +
                      matrix[row][1] * vector[1] +
                      matrix[row][2] * vector[2] +
                      matrix[row][3] * vector[3];
    }
    return result;
}

Matrix4 inverseAffine(const Matrix4 &matrix)
{
    const Matrix3 linear {{
        {{matrix[0][0], matrix[0][1], matrix[0][2]}},
        {{matrix[1][0], matrix[1][1], matrix[1][2]}},
        {{matrix[2][0], matrix[2][1], matrix[2][2]}},
    }};

    const Matrix3 inverseLinear = inverse3x3(linear);
    Matrix4 inverse {{
        {{inverseLinear[0][0], inverseLinear[0][1], inverseLinear[0][2], 0.0}},
        {{inverseLinear[1][0], inverseLinear[1][1], inverseLinear[1][2], 0.0}},
        {{inverseLinear[2][0], inverseLinear[2][1], inverseLinear[2][2], 0.0}},
        {{0.0, 0.0, 0.0, 1.0}},
    }};

    for (int row = 0; row < 3; ++row) {
        inverse[row][3] = -(inverseLinear[row][0] * matrix[0][3] +
                            inverseLinear[row][1] * matrix[1][3] +
                            inverseLinear[row][2] * matrix[2][3]);
    }
    return inverse;
}

bool isClose(const double left, const double right, const double epsilon)
{
    return std::fabs(left - right) <= epsilon;
}

bool hasIntegerTranslation(const Matrix4 &vox2vox, const double epsilon)
{
    return isClose(vox2vox[0][3], std::round(vox2vox[0][3]), epsilon) &&
           isClose(vox2vox[1][3], std::round(vox2vox[1][3]), epsilon) &&
           isClose(vox2vox[2][3], std::round(vox2vox[2][3]), epsilon);
}

bool allCloseIdentity(const Matrix4 &matrix, const double epsilon)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const double expected = row == column ? 1.0 : 0.0;
            if (!isClose(matrix[row][column], expected, epsilon)) {
                return false;
            }
        }
    }
    return true;
}

bool rotationRequiresInterpolation(const Matrix4 &vox2vox)
{
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            const double absoluteValue = std::fabs(vox2vox[row][column]);
            if (!isClose(absoluteValue, 1.0, constants::conform::VOXEL_EPSILON) &&
                !isClose(absoluteValue, 0.0, constants::conform::ROTATION_EPSILON)) {
                return true;
            }
        }
    }
    return false;
}

Matrix4 affineFromHeader(const MghImage::Header &header)
{
    Matrix4 affine {{
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0, 1.0}},
    }};

    for (int column = 0; column < 3; ++column) {
        for (int row = 0; row < 3; ++row) {
            affine[row][column] = static_cast<double>(header.directionCosines[column * 3 + row]) *
                                  static_cast<double>(header.spacing[column]);
        }
    }

    for (int row = 0; row < 3; ++row) {
        affine[row][3] = static_cast<double>(header.center[row]);
        for (int column = 0; column < 3; ++column) {
            affine[row][3] -= affine[row][column] * (static_cast<double>(header.dimensions[column]) / 2.0);
        }
    }

    return affine;
}

std::array<float, 9> directionCosinesForOrientation(const std::string &orientation)
{
    std::array<float, 9> directionCosines {};
    for (int axis = 0; axis < 3; ++axis) {
        const Vector3 vector = axisVector(orientation[axis]);
        for (int component = 0; component < 3; ++component) {
            directionCosines[axis * 3 + component] = static_cast<float>(vector[component]);
        }
    }
    return directionCosines;
}

std::array<int, 3> targetDimensionsForOrientation(
    const std::array<int, 3> &nativeTargetDimensions,
    const std::string &sourceOrientation,
    const std::string &requestedOrientation)
{
    const auto normalizedTarget = normalizeUpper(requestedOrientation);
    if (normalizedTarget == normalizeUpper(sourceOrientation)) {
        return nativeTargetDimensions;
    }

    std::array<int, 3> targetDimensions {};
    for (int targetAxis = 0; targetAxis < 3; ++targetAxis) {
        const int targetGroup = axisGroup(normalizedTarget[targetAxis]);
        for (int sourceAxis = 0; sourceAxis < 3; ++sourceAxis) {
            if (axisGroup(sourceOrientation[sourceAxis]) != targetGroup) {
                continue;
            }

            targetDimensions[targetAxis] = nativeTargetDimensions[sourceAxis];
            break;
        }
    }

    return targetDimensions;
}

Matrix3 linearPart(const Matrix4 &affine)
{
    Matrix3 linear {{
        {{0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0}},
    }};

    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            linear[row][column] = affine[row][column];
        }
    }

    return linear;
}

Matrix3 scaledDirectionMatrix(const std::array<float, 9> &directionCosines, const std::array<float, 3> &spacing)
{
    Matrix3 linear {{
        {{0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0}},
        {{0.0, 0.0, 0.0}},
    }};

    for (int column = 0; column < 3; ++column) {
        for (int row = 0; row < 3; ++row) {
            linear[row][column] = static_cast<double>(directionCosines[column * 3 + row]) *
                                  static_cast<double>(spacing[column]);
        }
    }

    return linear;
}

Vector3 translationToFixCenter(
    const Matrix3 &vox2voxLinear,
    const std::array<int, 3> &sourceDimensions,
    const std::array<int, 3> &targetDimensions)
{
    const Vector3 sourceCenter {
        (static_cast<double>(sourceDimensions[0]) - 1.0) / 2.0,
        (static_cast<double>(sourceDimensions[1]) - 1.0) / 2.0,
        (static_cast<double>(sourceDimensions[2]) - 1.0) / 2.0,
    };
    const Vector3 targetCenter {
        (static_cast<double>(targetDimensions[0]) - 1.0) / 2.0,
        (static_cast<double>(targetDimensions[1]) - 1.0) / 2.0,
        (static_cast<double>(targetDimensions[2]) - 1.0) / 2.0,
    };

    Vector3 translation {};
    for (int row = 0; row < 3; ++row) {
        translation[row] = sourceCenter[row];
        for (int column = 0; column < 3; ++column) {
            translation[row] -= vox2voxLinear[row][column] * targetCenter[column];
        }
    }

    return translation;
}

std::array<float, 3> headerCenterFromAffine(const Matrix4 &affine, const std::array<int, 3> &dimensions)
{
    std::array<float, 3> center {};
    for (int row = 0; row < 3; ++row) {
        double value = affine[row][3];
        for (int column = 0; column < 3; ++column) {
            value += affine[row][column] * (static_cast<double>(dimensions[column]) / 2.0);
        }
        center[row] = static_cast<float>(value);
    }

    return center;
}

MghImage::Header buildTargetHeader(
    const MghImage &image,
    const float targetVoxelSize,
    const std::array<int, 3> &nativeTargetDimensions,
    const OrientationMode requestedOrientation)
{
    const std::string sourceOrientation = image.orientationCode();
    const std::string orientation = resolveOrientationCode(requestedOrientation, sourceOrientation);
    const std::array<int, 3> targetDimensions = requestedOrientation == OrientationMode::Native
        ? nativeTargetDimensions
        : targetDimensionsForOrientation(nativeTargetDimensions, sourceOrientation, orientation);

    MghImage::Header header = image.header();
    header.frames = 1;
    header.type = static_cast<std::int32_t>(MghDataType::UChar);
    header.rasGoodFlag = 1;
    header.spacing = {targetVoxelSize, targetVoxelSize, targetVoxelSize};
    header.dimensions = targetDimensions;
    header.directionCosines = requestedOrientation == OrientationMode::Native
        ? image.header().directionCosines
        : directionCosinesForOrientation(orientation);

    const Matrix4 sourceAffine = image.affine();
    const Matrix3 sourceLinear = linearPart(sourceAffine);
    const Matrix3 targetLinear = scaledDirectionMatrix(header.directionCosines, header.spacing);
    const Matrix3 vox2voxLinear = multiply3x3(inverse3x3(sourceLinear), targetLinear);
    const Vector3 vox2voxTranslation =
        translationToFixCenter(vox2voxLinear, image.header().dimensions, header.dimensions);

    Matrix4 vox2vox {{
        {{vox2voxLinear[0][0], vox2voxLinear[0][1], vox2voxLinear[0][2], vox2voxTranslation[0]}},
        {{vox2voxLinear[1][0], vox2voxLinear[1][1], vox2voxLinear[1][2], vox2voxTranslation[1]}},
        {{vox2voxLinear[2][0], vox2voxLinear[2][1], vox2voxLinear[2][2], vox2voxTranslation[2]}},
        {{0.0, 0.0, 0.0, 1.0}},
    }};

    const Matrix4 targetAffine = multiply(sourceAffine, vox2vox);
    header.center = headerCenterFromAffine(targetAffine, header.dimensions);

    return header;
}

std::size_t flattenIndex(const std::array<int, 3> &dimensions, const int x, const int y, const int z)
{
    return (static_cast<std::size_t>(z) * static_cast<std::size_t>(dimensions[1]) + static_cast<std::size_t>(y)) *
               static_cast<std::size_t>(dimensions[0]) +
           static_cast<std::size_t>(x);
}

FloatImage3D::DirectionType itkDirectionFromHeader(const MghImage::Header &header)
{
    FloatImage3D::DirectionType direction;
    direction.SetIdentity();
    for (unsigned int column = 0; column < 3; ++column) {
        for (unsigned int row = 0; row < 3; ++row) {
            direction[row][column] = static_cast<double>(header.directionCosines[column * 3 + row]);
        }
    }
    return direction;
}

FloatImage3D::PointType itkOriginFromAffine(const Matrix4 &affine)
{
    FloatImage3D::PointType origin;
    for (unsigned int row = 0; row < 3; ++row) {
        origin[row] = affine[row][3];
    }
    return origin;
}

FloatImage3D::SpacingType itkSpacingFromHeader(const MghImage::Header &header)
{
    FloatImage3D::SpacingType spacing;
    for (unsigned int axis = 0; axis < 3; ++axis) {
        spacing[axis] = static_cast<double>(header.spacing[axis]);
    }
    return spacing;
}

FloatImage3D::RegionType itkRegionFromDimensions(const std::array<int, 3> &dimensions)
{
    FloatImage3D::RegionType region;
    FloatImage3D::IndexType start;
    start.Fill(0);
    FloatImage3D::SizeType size;
    for (unsigned int axis = 0; axis < 3; ++axis) {
        size[axis] = static_cast<FloatImage3D::SizeType::SizeValueType>(dimensions[axis]);
    }
    region.SetIndex(start);
    region.SetSize(size);
    return region;
}

FloatImage3D::Pointer importAsItkImage(const MghImage &image, std::vector<float> &sourceData)
{
    using ImportFilter = itk::ImportImageFilter<float, 3>;
    auto importFilter = ImportFilter::New();
    importFilter->SetRegion(itkRegionFromDimensions(image.header().dimensions));
    importFilter->SetOrigin(itkOriginFromAffine(image.affine()));
    importFilter->SetSpacing(itkSpacingFromHeader(image.header()));
    importFilter->SetDirection(itkDirectionFromHeader(image.header()));
    importFilter->SetImportPointer(sourceData.data(), sourceData.size(), false);
    importFilter->Update();
    return importFilter->GetOutput();
}

FloatImage3D::Pointer createGeometryImage(const MghImage::Header &header)
{
    auto geometryImage = FloatImage3D::New();
    geometryImage->SetRegions(itkRegionFromDimensions(header.dimensions));
    geometryImage->SetOrigin(itkOriginFromAffine(affineFromHeader(header)));
    geometryImage->SetSpacing(itkSpacingFromHeader(header));
    geometryImage->SetDirection(itkDirectionFromHeader(header));
    return geometryImage;
}

float sampleLinearLikeScipy(
    const std::vector<float> &sourceData,
    const std::array<int, 3> &dimensions,
    const itk::ContinuousIndex<double, 3> &continuousIndex)
{
    constexpr double boundaryEpsilon = 1.0e-5;
    std::array<int, 3> lower {};
    std::array<int, 3> upper {};
    std::array<double, 3> weightUpper {};
    std::array<double, 3> weightLower {};

    for (int axis = 0; axis < 3; ++axis) {
        const double maxIndex = static_cast<double>(dimensions[axis] - 1);
        double clampedIndex = continuousIndex[axis];
        if (clampedIndex < -boundaryEpsilon || clampedIndex > maxIndex + boundaryEpsilon) {
            return 0.0F;
        }
        clampedIndex = std::clamp(clampedIndex, 0.0, maxIndex);

        lower[axis] = static_cast<int>(std::floor(clampedIndex));
        upper[axis] = lower[axis] + 1;
        weightUpper[axis] = clampedIndex - static_cast<double>(lower[axis]);
        weightLower[axis] = 1.0 - weightUpper[axis];

        if (upper[axis] >= dimensions[axis]) {
            upper[axis] = lower[axis];
            weightUpper[axis] = 0.0;
            weightLower[axis] = 1.0;
        }
    }

    double value = 0.0;
    for (int corner = 0; corner < 8; ++corner) {
        const int x = (corner & 1) == 0 ? lower[0] : upper[0];
        const int y = (corner & 2) == 0 ? lower[1] : upper[1];
        const int z = (corner & 4) == 0 ? lower[2] : upper[2];

        const double weightX = (corner & 1) == 0 ? weightLower[0] : weightUpper[0];
        const double weightY = (corner & 2) == 0 ? weightLower[1] : weightUpper[1];
        const double weightZ = (corner & 4) == 0 ? weightLower[2] : weightUpper[2];

        value += static_cast<double>(sourceData[flattenIndex(dimensions, x, y, z)]) * weightX * weightY * weightZ;
    }

    return static_cast<float>(value);
}

std::vector<float> mapImageWithItk(
    const MghImage &image,
    std::vector<float> &sourceData,
    const MghImage::Header &targetHeader)
{
    const auto inputImage = importAsItkImage(image, sourceData);
    const auto outputGeometry = createGeometryImage(targetHeader);
    const std::size_t voxelCount = static_cast<std::size_t>(targetHeader.dimensions[0]) *
                                   static_cast<std::size_t>(targetHeader.dimensions[1]) *
                                   static_cast<std::size_t>(targetHeader.dimensions[2]);
    std::vector<float> mappedData(voxelCount, 0.0F);

    for (int z = 0; z < targetHeader.dimensions[2]; ++z) {
        for (int y = 0; y < targetHeader.dimensions[1]; ++y) {
            for (int x = 0; x < targetHeader.dimensions[0]; ++x) {
                FloatImage3D::IndexType outputIndex;
                outputIndex[0] = x;
                outputIndex[1] = y;
                outputIndex[2] = z;

                FloatImage3D::PointType physicalPoint;
                outputGeometry->TransformIndexToPhysicalPoint(outputIndex, physicalPoint);

                itk::ContinuousIndex<double, 3> inputContinuousIndex;
                inputImage->TransformPhysicalPointToContinuousIndex(physicalPoint, inputContinuousIndex);

                mappedData[flattenIndex(targetHeader.dimensions, x, y, z)] =
                    sampleLinearLikeScipy(sourceData, image.header().dimensions, inputContinuousIndex);
            }
        }
    }

    return mappedData;
}

std::vector<float> mapImageWithoutInterpolation(
    const std::vector<float> &sourceData,
    const std::array<int, 3> &sourceDimensions,
    const std::array<int, 3> &targetDimensions,
    const Matrix4 &targetToSource)
{
    const std::size_t voxelCount = static_cast<std::size_t>(targetDimensions[0]) *
                                   static_cast<std::size_t>(targetDimensions[1]) *
                                   static_cast<std::size_t>(targetDimensions[2]);
    std::vector<float> mappedData(voxelCount, 0.0F);

    for (int z = 0; z < targetDimensions[2]; ++z) {
        for (int y = 0; y < targetDimensions[1]; ++y) {
            for (int x = 0; x < targetDimensions[0]; ++x) {
                const int sourceX = static_cast<int>(std::lround(
                    targetToSource[0][0] * static_cast<double>(x) +
                    targetToSource[0][1] * static_cast<double>(y) +
                    targetToSource[0][2] * static_cast<double>(z) +
                    targetToSource[0][3]));
                const int sourceY = static_cast<int>(std::lround(
                    targetToSource[1][0] * static_cast<double>(x) +
                    targetToSource[1][1] * static_cast<double>(y) +
                    targetToSource[1][2] * static_cast<double>(z) +
                    targetToSource[1][3]));
                const int sourceZ = static_cast<int>(std::lround(
                    targetToSource[2][0] * static_cast<double>(x) +
                    targetToSource[2][1] * static_cast<double>(y) +
                    targetToSource[2][2] * static_cast<double>(z) +
                    targetToSource[2][3]));

                if (sourceX < 0 || sourceX >= sourceDimensions[0] ||
                    sourceY < 0 || sourceY >= sourceDimensions[1] ||
                    sourceZ < 0 || sourceZ >= sourceDimensions[2]) {
                    continue;
                }

                mappedData[flattenIndex(targetDimensions, x, y, z)] =
                    sourceData[flattenIndex(sourceDimensions, sourceX, sourceY, sourceZ)];
            }
        }
    }

    return mappedData;
}

} // namespace

bool ConformStepService::isAlreadyConformed(const MghImage &image, const ConformStepRequest &request)
{
    const float targetVoxelSize = computeTargetVoxelSize(image, request);
    const auto targetDimensions = computeNativeTargetDimensions(image, targetVoxelSize, request.imageSizeMode);
    const std::string requestedOrientation = resolveOrientationCode(request.orientation, image.orientationCode());

    return image.hasSingleFrame() &&
           image.isUint8() &&
               image.hasIsotropicSpacing(targetVoxelSize, constants::conform::VOXEL_EPSILON) &&
           image.hasDimensions(targetDimensions) &&
           image.matchesOrientation(requestedOrientation);
}

ConformStepResult ConformStepService::run(const ConformStepRequest &request) const
{
    if (request.inputPath.empty()) {
        throw std::runtime_error("The conform step requires a non-empty input path.");
    }
    if (request.conformedPath.empty()) {
        throw std::runtime_error("The conform step requires a non-empty conformed output path.");
    }

    const MghImage image = NiftiConverter::isSupportedPath(request.inputPath)
        ? NiftiConverter::loadAsMgh(request.inputPath)
        : MghImage::load(request.inputPath);
    if (!image.hasSingleFrame()) {
        throw std::runtime_error("Native MGZ reconforming currently supports only single-frame inputs.");
    }

    ConformStepResult result;
    result.inputPath = request.inputPath;
    result.copyOrigPath = request.copyOrigPath;
    result.conformedPath = request.conformedPath;
    result.inputMetadata = image.metadata();

    if (!request.copyOrigPath.empty()) {
        image.save(request.copyOrigPath);
    }

    const bool alreadyConformed = isAlreadyConformed(image, request);
    if (alreadyConformed && !request.forceConform) {
        image.save(request.conformedPath);
        result.success = true;
        result.alreadyConformed = true;
        result.outputMetadata = image.metadata();
        result.message = "Input image is already conformed. Original copy and conformed output were written successfully.";
        return result;
    }

    const float targetVoxelSize = computeTargetVoxelSize(image, request);
    const auto nativeTargetDimensions = computeNativeTargetDimensions(image, targetVoxelSize, request.imageSizeMode);
    const MghImage::Header targetHeader = buildTargetHeader(image, targetVoxelSize, nativeTargetDimensions, request.orientation);
    const Matrix4 targetAffine = affineFromHeader(targetHeader);
    const Matrix4 targetToSource = multiply(inverseAffine(image.affine()), targetAffine);
    std::vector<float> sourceData = image.voxelDataAsFloat();
    std::vector<float> mappedData =
        !rotationRequiresInterpolation(targetToSource) && hasIntegerTranslation(targetToSource, 1.0e-4)
            ? mapImageWithoutInterpolation(sourceData, image.header().dimensions, targetHeader.dimensions, targetToSource)
            : mapImageWithItk(image, sourceData, targetHeader);

    if (alreadyConformed && request.forceConform && image.isUint8()) {
        // Skip the histogram scaling to measure resampling errors
        // when running conform on an already conformed image (round trip test).
        for (float &value : mappedData) {
            value = std::clamp(value, 0.0F, 255.0F);
        }
    } else {
        const ScalePolicy scalePolicy = computeScalePolicy(sourceData, 0.0F, 255.0F);
        mappedData = applyScalePolicy(mappedData, 0.0F, 255.0F, scalePolicy);
    }

    for (float &value : mappedData) {
        value = std::round(std::clamp(value, 0.0F, 255.0F));
    }

    const MghImage outputImage =
        MghImage::fromVoxelData(targetHeader, mappedData, static_cast<std::int32_t>(MghDataType::UChar));
    outputImage.save(request.conformedPath);

    result.success = true;
    result.alreadyConformed = false;
    result.outputMetadata = outputImage.metadata();
    result.message = "Input image was reconformed natively and the conformed MGZ output was written successfully.";
    return result;
}

} // namespace fastsurfer::core