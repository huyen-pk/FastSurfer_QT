#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <Eigen/Dense>

#include <itkImage.h>
#include <itkImportImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>

#include "TestConstants.h"
#include "TestHelpers.h"
#include "imaging/mri/fastsurfer/step_conform_request.h"
#include "imaging/mri/fastsurfer/step_conform_result.h"
#include "imaging/mri/fastsurfer/step_conform.h"
#include "imaging/mri/fastsurfer/conform_policy.h"
#include "imaging/common/mgh_image.h"
#include "imaging/common/nifti_converter.h"

namespace ohc = OpenHC::imaging::mri::fastsurfer;

namespace {

std::string resolveRequestedOrientationCode(
    const ohc::OrientationMode requestedOrientation,
    const std::string &sourceOrientation)
{
    if (requestedOrientation == ohc::OrientationMode::Native) {
        return sourceOrientation;
    }

    std::string orientation = ohc::to_string(requestedOrientation);
    std::transform(orientation.begin(), orientation.end(), orientation.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return orientation;
}

using FloatImage3D = itk::Image<float, 3>;
using ImportFilter = itk::ImportImageFilter<float, 3>;
using InterpolatorType = itk::LinearInterpolateImageFunction<FloatImage3D, double>;

struct SourceOracle {
    // Source voxel buffer exposed as float samples.
    std::vector<float> data;
    // ITK image carrying the source geometry.
    FloatImage3D::Pointer image;
    // Interpolator used by the probe-based geometry oracle.
    InterpolatorType::Pointer interpolator;
};

struct ProbeStats {
    // Largest physical-point deviation in millimeters across probes.
    double maxPhysicalError {0.0};
    // Largest absolute uint8 intensity error across probes.
    double maxIntensityError {0.0};
    // p95 of absolute uint8 intensity error across sampled probes; units are grayscale levels.
    double percentile95IntensityError {0.0};
};

// Creates a clean temporary directory for one conform-test invocation.
// Parameters:
// - name: Directory suffix used to isolate this test run.
// Returns the recreated directory path.
std::filesystem::path makeFreshDirectory(const std::string &name)
{
    if (const char *envTmp = std::getenv("FASTSURFER_TEST_TMPDIR"); envTmp != nullptr && envTmp[0] != '\0') {
        const auto root = std::filesystem::path(envTmp) / name;
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);
        return root;
    }

    const auto root = std::filesystem::temp_directory_path() / std::string("fastsurfer_tests") / name;
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    return root;
}

// assertion helpers are provided by TestHelpers.h

// Converts the native affine type into Eigen for matrix-based comparisons.
// Parameters:
// - matrix: Native 4x4 affine matrix.
// Returns the same matrix as Eigen::Matrix4d.
Eigen::Matrix4d toEigenMatrix(const ohc::Matrix4 &matrix)
{
    Eigen::Matrix4d result = Eigen::Matrix4d::Zero();
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            result(row, column) = matrix[row][column];
        }
    }
    return result;
}

// Converts an ITK direction matrix into Eigen form.
// Parameters:
// - direction: ITK direction matrix to convert.
// Returns the same matrix as Eigen::Matrix3d.
Eigen::Matrix3d toDirectionMatrix(const FloatImage3D::DirectionType &direction)
{
    Eigen::Matrix3d result = Eigen::Matrix3d::Zero();
    for (unsigned int column = 0; column < 3; ++column) {
        for (unsigned int row = 0; row < 3; ++row) {
            result(row, column) = direction[row][column];
        }
    }
    return result;
}

template <typename DerivedActual, typename DerivedExpected>
// Throws when two Eigen matrices differ by more than tolerance.
// Parameters:
// - actual: Observed matrix.
// - expected: Expected matrix.
// - tolerance: Maximum allowed absolute coefficient error.
// - message: Failure message to surface when the assertion fails.
void assertMatrixClose(
    const Eigen::MatrixBase<DerivedActual> &actual,
    const Eigen::MatrixBase<DerivedExpected> &expected,
    const double tolerance,
    const std::string &message)
{
    const double maxCoefficientError = (actual - expected).cwiseAbs().maxCoeff();
    if (maxCoefficientError > tolerance) {
        throw std::runtime_error(message + " Max coefficient error=" + std::to_string(maxCoefficientError));
    }
}

// Uppercases orientation codes for case-insensitive comparisons.
// Parameters:
// - value: Orientation string to normalize.
// Returns the uppercased orientation string.
std::string normalizeUpper(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

// Maps an orientation letter to its anatomical axis family.
// Parameters:
// - axisCode: Orientation letter such as L, R, A, P, I, or S.
// Returns 0 for left-right, 1 for anterior-posterior, and 2 for inferior-superior.
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
        throw std::runtime_error(std::string("Unsupported orientation axis code in the merged conform oracle: '") + axisCode + "'.");
    }
}

// Builds an ITK direction matrix from a three-letter orientation code.
// Parameters:
// - orientation: Orientation code such as LIA or RAS.
// Returns the matching direction matrix.
FloatImage3D::DirectionType directionFromOrientation(const std::string &orientation)
{
    FloatImage3D::DirectionType direction;
    direction.SetIdentity();

    const auto normalized = normalizeUpper(orientation);
    for (unsigned int column = 0; column < 3; ++column) {
        direction[0][column] = 0.0;
        direction[1][column] = 0.0;
        direction[2][column] = 0.0;

        switch (normalized[column]) {
        case 'R':
            direction[0][column] = 1.0;
            break;
        case 'L':
            direction[0][column] = -1.0;
            break;
        case 'A':
            direction[1][column] = 1.0;
            break;
        case 'P':
            direction[1][column] = -1.0;
            break;
        case 'S':
            direction[2][column] = 1.0;
            break;
        case 'I':
            direction[2][column] = -1.0;
            break;
        default:
            throw std::runtime_error(
                "Unsupported target orientation for the merged conform oracle: '" + normalized + "'.");
        }
    }

    return direction;
}

// Reorders native target dimensions to match a requested output orientation.
// Parameters:
// - nativeTargetDimensions: Dimensions before orientation remapping.
// - sourceOrientation: Source orientation code.
// - requestedOrientation: Requested output orientation code.
// Returns the target dimensions in requested orientation order.
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
        bool found = false;
        for (int sourceAxis = 0; sourceAxis < 3; ++sourceAxis) {
            if (axisGroup(sourceOrientation[sourceAxis]) != targetGroup) {
                continue;
            }

            targetDimensions[targetAxis] = nativeTargetDimensions[sourceAxis];
            found = true;
            break;
        }

        if (!found) {
            throw std::runtime_error(
                "Could not derive target dimensions for the merged conform oracle. Source='" +
                sourceOrientation + "' target='" + normalizedTarget + "'.");
        }
    }

    return targetDimensions;
}

// Returns the voxel-center coordinates for a lattice of the given dimensions.
// Parameters:
// - dimensions: Lattice dimensions along x, y, and z.
// Returns the homogeneous center coordinate.
Eigen::Vector4d voxelCenter(const std::array<int, 3> &dimensions)
{
    return {
        (static_cast<double>(dimensions[0]) - 1.0) / 2.0,
        (static_cast<double>(dimensions[1]) - 1.0) / 2.0,
        (static_cast<double>(dimensions[2]) - 1.0) / 2.0,
        1.0,
    };
}

// Converts MGH header direction cosines into ITK direction form.
// Parameters:
// - header: MGH header carrying direction cosines.
// Returns the matching ITK direction matrix.
FloatImage3D::DirectionType itkDirectionFromHeader(const ohc::MghImage::Header &header)
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

// Converts MGH spacing metadata into ITK spacing form.
// Parameters:
// - header: MGH header carrying voxel spacing.
// Returns the matching ITK spacing value.
FloatImage3D::SpacingType itkSpacingFromHeader(const ohc::MghImage::Header &header)
{
    FloatImage3D::SpacingType spacing;
    for (unsigned int axis = 0; axis < 3; ++axis) {
        spacing[axis] = static_cast<double>(header.spacing[axis]);
    }
    return spacing;
}

// Extracts an ITK origin from an Eigen affine matrix.
// Parameters:
// - affine: Homogeneous affine matrix whose translation defines the origin.
// Returns the corresponding ITK point.
FloatImage3D::PointType itkOriginFromAffine(const Eigen::Matrix4d &affine)
{
    FloatImage3D::PointType origin;
    for (unsigned int row = 0; row < 3; ++row) {
        origin[row] = affine(static_cast<int>(row), 3);
    }
    return origin;
}

// Converts an ITK point into Eigen vector form.
// Parameters:
// - point: ITK point to convert.
// Returns the same point as Eigen::Vector3d.
Eigen::Vector3d toEigenVector(const FloatImage3D::PointType &point)
{
    return {point[0], point[1], point[2]};
}

// Converts ITK spacing into Eigen vector form.
// Parameters:
// - spacing: ITK spacing to convert.
// Returns the same spacing as Eigen::Vector3d.
Eigen::Vector3d toEigenVector(const FloatImage3D::SpacingType &spacing)
{
    return {spacing[0], spacing[1], spacing[2]};
}

// Builds an ITK image region from integer dimensions.
// Parameters:
// - dimensions: Region size along x, y, and z.
// Returns the matching ITK region.
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

// Reads the spatial dimensions from an ITK geometry image.
// Parameters:
// - geometry: ITK image whose region defines the dimensions.
// Returns the x, y, and z dimensions.
std::array<int, 3> dimensionsFromGeometry(const FloatImage3D::Pointer &geometry)
{
    const auto size = geometry->GetLargestPossibleRegion().GetSize();
    return {
        static_cast<int>(size[0]),
        static_cast<int>(size[1]),
        static_cast<int>(size[2]),
    };
}

// Computes the continuous center index used by the physical-center oracle.
// Parameters:
// - dimensions: Lattice dimensions along x, y, and z.
// Returns the half-dimension index in ITK continuous-index form.
itk::ContinuousIndex<double, 3> continuousCenterIndex(const std::array<int, 3> &dimensions)
{
    itk::ContinuousIndex<double, 3> centerIndex;
    for (unsigned int axis = 0; axis < 3; ++axis) {
        centerIndex[axis] = static_cast<double>(dimensions[axis]) / 2.0;
    }
    return centerIndex;
}

// Computes the physical center of an ITK geometry image.
// Parameters:
// - geometry: Geometry image used for the transform.
// - dimensions: Dimensions whose center should be evaluated.
// Returns the physical center point.
FloatImage3D::PointType physicalCenter(
    const FloatImage3D::Pointer &geometry,
    const std::array<int, 3> &dimensions)
{
    FloatImage3D::PointType center;
    geometry->TransformContinuousIndexToPhysicalPoint(continuousCenterIndex(dimensions), center);
    return center;
}

// Reconstructs an ITK origin from a target center, direction, and spacing.
// Parameters:
// - direction: Target direction matrix.
// - spacing: Target voxel spacing.
// - dimensions: Target lattice dimensions.
// - center: Desired physical center point.
// Returns the origin that yields the supplied center.
FloatImage3D::PointType computeOriginFromCenter(
    const FloatImage3D::DirectionType &direction,
    const FloatImage3D::SpacingType &spacing,
    const std::array<int, 3> &dimensions,
    const FloatImage3D::PointType &center)
{
    const Eigen::Vector3d offset = toDirectionMatrix(direction) *
                                   toEigenVector(spacing).asDiagonal() *
                                   voxelCenter(dimensions).head<3>();
    const Eigen::Vector3d originVector = toEigenVector(center) - offset;

    FloatImage3D::PointType origin;
    for (unsigned int axis = 0; axis < 3; ++axis) {
        origin[axis] = originVector[axis];
    }
    return origin;
}

// Returns true when two floating-point values are within epsilon.
// Parameters:
// - left: First value to compare.
// - right: Second value to compare.
// - epsilon: Maximum allowed absolute difference.
bool isClose(const double left, const double right, const double epsilon)
{
    return std::fabs(left - right) <= epsilon;
}

// Builds an affine matrix from ITK direction, spacing, and origin values.
// Parameters:
// - direction: Direction matrix of the geometry.
// - spacing: Spacing vector of the geometry.
// - origin: Physical origin of the geometry.
// Returns the corresponding homogeneous affine matrix.
Eigen::Matrix4d affineFromGeometry(
    const FloatImage3D::DirectionType &direction,
    const FloatImage3D::SpacingType &spacing,
    const FloatImage3D::PointType &origin)
{
    Eigen::Matrix4d affine = Eigen::Matrix4d::Identity();
    const Eigen::Matrix3d directionMatrix = toDirectionMatrix(direction);

    for (int column = 0; column < 3; ++column) {
        affine.block<3, 1>(0, column) = directionMatrix.col(column) * spacing[column];
    }

    affine(0, 3) = origin[0];
    affine(1, 3) = origin[1];
    affine(2, 3) = origin[2];
    return affine;
}

// Returns true when the transform includes a non-axis-aligned rotation component.
// Parameters:
// - vox2vox: Voxel-to-voxel affine under test.
bool rotationRequiresInterpolation(const Eigen::Matrix4d &vox2vox)
{
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            const double absoluteValue = std::fabs(vox2vox(row, column));
            if (!isClose(absoluteValue, 1.0, test_constants::ROTATION_UNIT_TOLERANCE) &&
                !isClose(absoluteValue, 0.0, test_constants::ROTATION_ZERO_TOLERANCE)) {
                return true;
            }
        }
    }
    return false;
}

// Combines direction and spacing into the affine linear component.
// Parameters:
// - direction: Direction matrix.
// - spacing: Voxel spacing.
// Returns the scaled 3x3 linear component.
Eigen::Matrix3d scaledDirectionMatrix(
    const FloatImage3D::DirectionType &direction,
    const FloatImage3D::SpacingType &spacing)
{
    Eigen::Matrix3d linear = Eigen::Matrix3d::Zero();
    const Eigen::Matrix3d directionMatrix = toDirectionMatrix(direction);
    for (int column = 0; column < 3; ++column) {
        linear.col(column) = directionMatrix.col(column) * spacing[column];
    }
    return linear;
}

// Computes the voxel-space translation that aligns source and target centers.
// Parameters:
// - vox2voxLinear: Linear voxel-to-voxel transform.
// - sourceDimensions: Source lattice dimensions.
// - targetDimensions: Target lattice dimensions.
// Returns the translation vector that aligns the centers.
Eigen::Vector3d translationToFixCenter(
    const Eigen::Matrix3d &vox2voxLinear,
    const std::array<int, 3> &sourceDimensions,
    const std::array<int, 3> &targetDimensions)
{
    const Eigen::Vector3d sourceCenter {
        (static_cast<double>(sourceDimensions[0]) - 1.0) / 2.0,
        (static_cast<double>(sourceDimensions[1]) - 1.0) / 2.0,
        (static_cast<double>(sourceDimensions[2]) - 1.0) / 2.0,
    };
    const Eigen::Vector3d targetCenter {
        (static_cast<double>(targetDimensions[0]) - 1.0) / 2.0,
        (static_cast<double>(targetDimensions[1]) - 1.0) / 2.0,
        (static_cast<double>(targetDimensions[2]) - 1.0) / 2.0,
    };
    return sourceCenter - vox2voxLinear * targetCenter;
}

// Builds the expected output geometry used as the merged conform oracle.
// Parameters:
// - sourceImage: Source image being conformed.
// - sourceOracle: Source ITK representation and interpolator.
// - request: Conform request under test.
// - targetVoxelSize: Chosen output voxel size.
// - nativeTargetDimensions: Native target dimensions before orientation remapping.
// Returns an allocated ITK image carrying the expected output geometry.
FloatImage3D::Pointer buildExpectedGeometry(
    const ohc::MghImage &sourceImage,
    const SourceOracle &sourceOracle,
    const ohc::ConformStepRequest &request,
    const float targetVoxelSize,
    const std::array<int, 3> &nativeTargetDimensions)
{
    const bool useNativeOrientation = request.orientation == ohc::OrientationMode::Native;
    const std::string targetOrientation = resolveRequestedOrientationCode(request.orientation, sourceImage.orientationCode());
    const auto targetDimensions = useNativeOrientation
        ? nativeTargetDimensions
        : targetDimensionsForOrientation(
              nativeTargetDimensions,
              sourceImage.orientationCode(),
              targetOrientation);

    auto expectedGeometry = FloatImage3D::New();
    expectedGeometry->SetRegions(itkRegionFromDimensions(targetDimensions));

    FloatImage3D::SpacingType spacing;
    spacing.Fill(static_cast<double>(targetVoxelSize));
    expectedGeometry->SetSpacing(spacing);
    expectedGeometry->SetDirection(useNativeOrientation
        ? itkDirectionFromHeader(sourceImage.header())
        : directionFromOrientation(targetOrientation));

    const Eigen::Matrix4d sourceAffine = toEigenMatrix(sourceImage.affine());
    const Eigen::Matrix3d targetLinear = scaledDirectionMatrix(expectedGeometry->GetDirection(), spacing);
    const Eigen::Matrix3d vox2voxLinear = sourceAffine.topLeftCorner<3, 3>().inverse() * targetLinear;
    const Eigen::Vector3d vox2voxTranslation =
        translationToFixCenter(vox2voxLinear, sourceImage.header().dimensions, targetDimensions);

    Eigen::Matrix4d targetAffine = Eigen::Matrix4d::Identity();
    targetAffine.topLeftCorner<3, 3>() = targetLinear;
    targetAffine.block<3, 1>(0, 3) =
        (sourceAffine * Eigen::Vector4d(vox2voxTranslation[0], vox2voxTranslation[1], vox2voxTranslation[2], 1.0)).head<3>();

    FloatImage3D::PointType origin;
    origin[0] = targetAffine(0, 3);
    origin[1] = targetAffine(1, 3);
    origin[2] = targetAffine(2, 3);
    expectedGeometry->SetOrigin(origin);

    expectedGeometry->Allocate();
    return expectedGeometry;
}

// Builds the ITK-backed source oracle used by geometry and interpolation checks.
// Parameters:
// - image: Source image to expose through ITK.
// Returns the source voxel buffer, ITK image, and interpolator.
SourceOracle buildSourceOracle(const ohc::MghImage &image)
{
    SourceOracle oracle;
    oracle.data = image.voxelDataAsFloat();

    auto importFilter = ImportFilter::New();
    importFilter->SetRegion(itkRegionFromDimensions(image.header().dimensions));
    importFilter->SetOrigin(itkOriginFromAffine(toEigenMatrix(image.affine())));
    importFilter->SetSpacing(itkSpacingFromHeader(image.header()));
    importFilter->SetDirection(itkDirectionFromHeader(image.header()));
    importFilter->SetImportPointer(oracle.data.data(), oracle.data.size(), false);
    importFilter->Update();

    oracle.image = importFilter->GetOutput();
    oracle.interpolator = InterpolatorType::New();
    oracle.interpolator->SetInputImage(oracle.image);
    return oracle;
}

// Builds a geometry-only ITK image from an MGH image.
// Parameters:
// - image: Image whose geometry should be mirrored.
// Returns an allocated ITK image carrying the same geometry.
FloatImage3D::Pointer buildGeometryImage(const ohc::MghImage &image)
{
    auto geometry = FloatImage3D::New();
    geometry->SetRegions(itkRegionFromDimensions(image.header().dimensions));
    geometry->SetOrigin(itkOriginFromAffine(toEigenMatrix(image.affine())));
    geometry->SetSpacing(itkSpacingFromHeader(image.header()));
    geometry->SetDirection(itkDirectionFromHeader(image.header()));
    geometry->Allocate();
    return geometry;
}

// Applies the shared scale policy and uint8 rounding used by the conform output.
// Parameters:
// - sourceData: Input data to scale and round.
// Returns the rounded uint8-domain values as floats.
std::vector<float> computeRoundedScaledValues(const std::vector<float> &sourceData)
{
    const ohc::ScalePolicy scalePolicy = ohc::computeScalePolicy(sourceData, 0.0F, 255.0F);
    std::vector<float> scaledData = ohc::applyScalePolicy(sourceData, 0.0F, 255.0F, scalePolicy);
    for (float &value : scaledData) {
        value = std::round(std::clamp(value, 0.0F, 255.0F));
    }
    return scaledData;
}

// Evaluates the expected scaled value for one interpolated source sample.
// Parameters:
// - mappedValue: Interpolated source intensity.
// - policy: Scale policy used by the conform output.
// Returns the rounded uint8-domain expected value.
float scaleExpectedValue(const float mappedValue, const ohc::ScalePolicy &policy)
{
    return std::round(ohc::applyScalePolicyValue(mappedValue, 0.0F, 255.0F, policy));
}

// Flattens 3D voxel coordinates into a row-major linear index.
// Parameters:
// - dimensions: Lattice dimensions.
// - index: x, y, z voxel coordinates.
// Returns the linear buffer index.
std::size_t flattenIndex(const std::array<int, 3> &dimensions, const std::array<int, 3> &index)
{
    return (static_cast<std::size_t>(index[2]) * static_cast<std::size_t>(dimensions[1]) +
            static_cast<std::size_t>(index[1])) *
               static_cast<std::size_t>(dimensions[0]) +
           static_cast<std::size_t>(index[0]);
}

// Clamps a probe coordinate into the valid index range for one axis.
// Parameters:
// - dimension: Axis length.
// - index: Candidate probe index.
// Returns the clamped index.
int clampProbeIndex(const int dimension, const int index)
{
    return std::clamp(index, 0, std::max(0, dimension - 1));
}

// Generates boundary-focused probe samples for one axis.
// Parameters:
// - dimension: Axis length.
// Returns sorted unique probe indices along that axis.
std::vector<int> axisProbeSamples(const int dimension)
{
    if (dimension <= 0) {
        return {};
    }

    std::vector<int> samples {
        0,
        clampProbeIndex(dimension, 1),
        clampProbeIndex(dimension, dimension / 4),
        clampProbeIndex(dimension, dimension / 2),
        clampProbeIndex(dimension, (3 * dimension) / 4),
        clampProbeIndex(dimension, dimension - 2),
        clampProbeIndex(dimension, dimension - 1),
    };
    std::sort(samples.begin(), samples.end());
    samples.erase(std::unique(samples.begin(), samples.end()), samples.end());
    return samples;
}

// Generates evenly stratified probe samples for one axis.
// Parameters:
// - dimension: Axis length.
// - binCount: Requested number of sampling bins.
// Returns sorted unique probe indices along that axis.
std::vector<int> stratifiedAxisProbeSamples(
    const int dimension,
    const int binCount = test_constants::STRATIFIED_PROBE_BIN_COUNT)
{
    if (dimension <= 0) {
        return {};
    }

    const int effectiveBinCount = std::min(std::max(2, binCount), dimension);
    std::vector<int> samples;
    samples.reserve(static_cast<std::size_t>(effectiveBinCount));
    for (int bin = 0; bin < effectiveBinCount; ++bin) {
        const double center = ((static_cast<double>(bin) + 0.5) * static_cast<double>(dimension)) /
                              static_cast<double>(effectiveBinCount);
        samples.push_back(clampProbeIndex(dimension, static_cast<int>(std::floor(center))));
    }

    std::sort(samples.begin(), samples.end());
    samples.erase(std::unique(samples.begin(), samples.end()), samples.end());
    return samples;
}

// Returns the inclusive bounds of one jittered-probe bin.
// Parameters:
// - dimension: Axis length.
// - bin: Bin index to evaluate.
// - binCount: Total number of bins.
// Returns the inclusive start and end indices for that bin.
std::array<int, 2> jitteredBinBounds(const int dimension, const int bin, const int binCount)
{
    const int start = (bin * dimension) / binCount;
    const int nextStart = ((bin + 1) * dimension) / binCount;
    const int end = std::max(start, nextStart - 1);
    return {start, end};
}

// Generates one jittered probe per bin across the 3D lattice.
// Parameters:
// - dimensions: Lattice dimensions.
// - binCount: Requested bin count per axis.
// Returns sorted unique jittered probe indices.
std::vector<std::array<int, 3>> generateJitteredProbeIndices(
    const std::array<int, 3> &dimensions,
    const int binCount = test_constants::JITTERED_PROBE_BIN_COUNT)
{
    if (dimensions[0] <= 0 || dimensions[1] <= 0 || dimensions[2] <= 0) {
        return {};
    }

    const int effectiveBinsX = std::min(std::max(1, binCount), dimensions[0]);
    const int effectiveBinsY = std::min(std::max(1, binCount), dimensions[1]);
    const int effectiveBinsZ = std::min(std::max(1, binCount), dimensions[2]);

    std::mt19937 randomEngine(test_constants::JITTERED_PROBE_SEED);
    std::vector<std::array<int, 3>> probes;
    probes.reserve(static_cast<std::size_t>(effectiveBinsX * effectiveBinsY * effectiveBinsZ));

    for (int binX = 0; binX < effectiveBinsX; ++binX) {
        const auto [startX, endX] = jitteredBinBounds(dimensions[0], binX, effectiveBinsX);
        std::uniform_int_distribution<int> distributionX(startX, endX);

        for (int binY = 0; binY < effectiveBinsY; ++binY) {
            const auto [startY, endY] = jitteredBinBounds(dimensions[1], binY, effectiveBinsY);
            std::uniform_int_distribution<int> distributionY(startY, endY);

            for (int binZ = 0; binZ < effectiveBinsZ; ++binZ) {
                const auto [startZ, endZ] = jitteredBinBounds(dimensions[2], binZ, effectiveBinsZ);
                std::uniform_int_distribution<int> distributionZ(startZ, endZ);
                probes.push_back({
                    distributionX(randomEngine),
                    distributionY(randomEngine),
                    distributionZ(randomEngine),
                });
            }
        }
    }

    std::sort(probes.begin(), probes.end());
    probes.erase(std::unique(probes.begin(), probes.end()), probes.end());
    return probes;
}

// Appends the Cartesian product of axis probes to the destination list.
// Parameters:
// - xs: Probe indices along x.
// - ys: Probe indices along y.
// - zs: Probe indices along z.
// - probes: Destination probe list to extend.
void appendCartesianProbes(
    const std::vector<int> &xs,
    const std::vector<int> &ys,
    const std::vector<int> &zs,
    std::vector<std::array<int, 3>> &probes)
{
    for (const int x : xs) {
        for (const int y : ys) {
            for (const int z : zs) {
                probes.push_back({x, y, z});
            }
        }
    }
}

// Builds the full probe set used by the merged conform oracle.
// Parameters:
// - dimensions: Output lattice dimensions.
// Returns sorted unique probe coordinates across boundary, stratified, and jittered samples.
std::vector<std::array<int, 3>> generateProbeIndices(const std::array<int, 3> &dimensions)
{
    const auto boundaryXs = axisProbeSamples(dimensions[0]);
    const auto boundaryYs = axisProbeSamples(dimensions[1]);
    const auto boundaryZs = axisProbeSamples(dimensions[2]);
    const auto stratifiedXs = stratifiedAxisProbeSamples(dimensions[0], test_constants::STRATIFIED_PROBE_BIN_COUNT);
    const auto stratifiedYs = stratifiedAxisProbeSamples(dimensions[1], test_constants::STRATIFIED_PROBE_BIN_COUNT);
    const auto stratifiedZs = stratifiedAxisProbeSamples(dimensions[2], test_constants::STRATIFIED_PROBE_BIN_COUNT);
    const auto jitteredProbes = generateJitteredProbeIndices(dimensions);

    std::vector<std::array<int, 3>> probes;
    probes.reserve(
        boundaryXs.size() * boundaryYs.size() * boundaryZs.size() +
        stratifiedXs.size() * stratifiedYs.size() * stratifiedZs.size() +
        jitteredProbes.size());

    appendCartesianProbes(boundaryXs, boundaryYs, boundaryZs, probes);
    appendCartesianProbes(stratifiedXs, stratifiedYs, stratifiedZs, probes);
    probes.insert(probes.end(), jitteredProbes.begin(), jitteredProbes.end());

    std::sort(probes.begin(), probes.end());
    probes.erase(std::unique(probes.begin(), probes.end()), probes.end());
    return probes;
}

// Computes the 95th percentile of a vector of absolute errors.
// Parameters:
// - values: Error samples to summarize.
// Returns the 95th-percentile sample value.
double percentile95(std::vector<double> values)
{
    if (values.empty()) {
        return 0.0;
    }

    // Returns the 95th-percentile absolute intensity error in the same units as the
    // sampled probe errors, i.e. uint8 grayscale levels rather than a normalized ratio.
    std::sort(values.begin(), values.end());
    const std::size_t index = static_cast<std::size_t>(
        std::ceil(test_constants::PROBE_PERCENTILE_FRACTION * static_cast<double>(values.size()))) - 1U;
    return values[std::min(index, values.size() - 1U)];
}

// Evaluates physical and intensity probe errors against the merged conform oracle.
// Parameters:
// - outputImage: Written conform output under test.
// - outputGeometry: ITK geometry derived from the output image.
// - sourceOracle: ITK-backed source image and interpolator.
// - expectedGeometry: Expected output geometry from the oracle.
// - scalePolicy: Scale policy used to compute expected intensities.
// Returns aggregated probe error statistics.
ProbeStats evaluateProbes(
    const ohc::MghImage &outputImage,
    const FloatImage3D::Pointer &outputGeometry,
    const SourceOracle &sourceOracle,
    const FloatImage3D::Pointer &expectedGeometry,
    const ohc::ScalePolicy &scalePolicy)
{
    const auto probes = generateProbeIndices(outputImage.header().dimensions);
    const auto outputData = outputImage.voxelDataAsFloat();
    std::vector<double> intensityErrors;
    intensityErrors.reserve(probes.size());

    ProbeStats stats;
    for (const auto &probe : probes) {
        FloatImage3D::IndexType itkIndex;
        itkIndex[0] = probe[0];
        itkIndex[1] = probe[1];
        itkIndex[2] = probe[2];

        FloatImage3D::PointType actualPoint;
        outputGeometry->TransformIndexToPhysicalPoint(itkIndex, actualPoint);

        FloatImage3D::PointType expectedPoint;
        expectedGeometry->TransformIndexToPhysicalPoint(itkIndex, expectedPoint);
        const double physicalError = std::sqrt(
            std::pow(actualPoint[0] - expectedPoint[0], 2.0) +
            std::pow(actualPoint[1] - expectedPoint[1], 2.0) +
            std::pow(actualPoint[2] - expectedPoint[2], 2.0));
        stats.maxPhysicalError = std::max(stats.maxPhysicalError, physicalError);

        itk::ContinuousIndex<double, 3> sourceContinuousIndex;
        const bool mapped =
            sourceOracle.image->TransformPhysicalPointToContinuousIndex(expectedPoint, sourceContinuousIndex);

        float sampledValue = 0.0F;
        if (mapped && sourceOracle.interpolator->IsInsideBuffer(sourceContinuousIndex)) {
            sampledValue = sourceOracle.interpolator->EvaluateAtContinuousIndex(sourceContinuousIndex);
        }

        const float expectedValue = scaleExpectedValue(sampledValue, scalePolicy);
        const float actualValue = outputData[flattenIndex(outputImage.header().dimensions, probe)];
        const double intensityError = std::fabs(static_cast<double>(actualValue) - static_cast<double>(expectedValue));
        stats.maxIntensityError = std::max(stats.maxIntensityError, intensityError);
        intensityErrors.push_back(intensityError);
    }

    stats.percentile95IntensityError = percentile95(std::move(intensityErrors));
    return stats;
}

// Verifies that the copy-orig output matches the expected input image exactly.
// Parameters:
// - copyImage: Copy-orig image produced by the service.
// - expectedCopy: Image that should have been copied.
void assertCopyOrigMatches(
    const ohc::MghImage &copyImage,
    const ohc::MghImage &expectedCopy)
{
    require(copyImage.rawData() == expectedCopy.rawData(),
            "The copy-orig output differs from the direct NIfTI-to-MGZ conversion payload.");
    assertMatrixClose(
        toEigenMatrix(copyImage.affine()),
        toEigenMatrix(expectedCopy.affine()),
        test_constants::AFFINE_LINEAR_TOLERANCE,
        "The copy-orig MGZ affine differs from the direct NIfTI conversion affine.");
}

// Verifies geometry-related invariants for one conform output.
// Parameters:
// - sourceImage: Source image provided to the service.
// - sourceOracle: ITK-backed oracle built from the source image.
// - request: Conform request under test.
// - result: Service result metadata.
// - outputImage: Written conformed output image.
void verifyOutputGeometry(
    const ohc::MghImage &sourceImage,
    const SourceOracle &sourceOracle,
    const ohc::ConformStepRequest &request,
    const ohc::ConformStepResult &result,
    const ohc::MghImage &outputImage)
{
    const float targetVoxelSize = ohc::computeTargetVoxelSize(sourceImage, request);
    const auto nativeTargetDimensions =
        ohc::computeNativeTargetDimensions(sourceImage, targetVoxelSize, request.imageSizeMode);
    const std::string expectedOrientation = resolveRequestedOrientationCode(request.orientation, sourceImage.orientationCode());
    const auto expectedGeometry =
        buildExpectedGeometry(sourceImage, sourceOracle, request, targetVoxelSize, nativeTargetDimensions);
    const auto expectedDimensions = dimensionsFromGeometry(expectedGeometry);
    const auto outputGeometry = buildGeometryImage(outputImage);

    require(outputImage.hasSingleFrame(), "The conformed NIfTI output should remain single-frame.");
    require(outputImage.isUint8(), "The conformed NIfTI output should be saved as uint8 MGZ.");
    require(outputImage.orientationCode() == expectedOrientation,
            "The conformed NIfTI output orientation code does not match the request policy.");
    require(outputImage.header().dimensions == expectedDimensions,
            "The conformed NIfTI output dimensions do not match the expected conform target dimensions.");
    require(outputImage.hasIsotropicSpacing(targetVoxelSize, test_constants::METADATA_SPACING_TOLERANCE),
            "The conformed NIfTI output spacing does not match the expected isotropic target voxel size.");

    require(result.outputMetadata.dimensions == expectedDimensions,
            "The conform step reported unexpected output dimensions for the NIfTI fixture.");
    require(result.outputMetadata.orientationCode == expectedOrientation,
            "The conform step reported an unexpected output orientation for the NIfTI fixture.");
    require(result.outputMetadata.dataTypeName == "uint8",
            "The conform step reported an unexpected output data type for the NIfTI fixture.");

    requireNear(
        (toEigenVector(outputGeometry->GetOrigin()) - toEigenVector(expectedGeometry->GetOrigin())).norm(),
        0.0,
        test_constants::PHYSICAL_POINT_TOLERANCE_MM,
        "The conformed MGZ origin differs from the merged conform geometry oracle.");

    assertMatrixClose(
        toEigenVector(outputGeometry->GetSpacing()),
        toEigenVector(expectedGeometry->GetSpacing()),
        test_constants::METADATA_SPACING_TOLERANCE,
        "The conformed MGZ spacing differs from the merged conform geometry oracle.");

    const Eigen::Matrix3d actualDirection = toDirectionMatrix(outputGeometry->GetDirection());
    const Eigen::Matrix3d expectedDirection = toDirectionMatrix(expectedGeometry->GetDirection());
    assertMatrixClose(
        actualDirection,
        expectedDirection,
        test_constants::DIRECTION_MATRIX_TOLERANCE,
        "The conformed direction matrix differs from the expected orientation policy.");
    assertMatrixClose(
        actualDirection.transpose() * actualDirection,
        Eigen::Matrix3d::Identity(),
        test_constants::ORTHONORMALITY_TOLERANCE,
        "The conformed direction matrix is not orthonormal.");
    requireNear(actualDirection.determinant(), expectedDirection.determinant(), test_constants::DETERMINANT_TOLERANCE,
                "The conformed direction handedness differs from the expected orientation policy.");

    requireNear(
        (toEigenVector(physicalCenter(expectedGeometry, expectedDimensions)) -
         toEigenVector(physicalCenter(outputGeometry, outputImage.header().dimensions)))
            .norm(),
        0.0,
        test_constants::PHYSICAL_POINT_TOLERANCE_MM,
        "The conformed output center differs from the expected conform geometry.");
}

// Verifies both geometry and probe-sampled intensities for one conform output.
// Parameters:
// - sourceImage: Source image provided to the service.
// - sourceOracle: ITK-backed oracle built from the source image.
// - request: Conform request under test.
// - result: Service result metadata.
// - outputImage: Written conformed output image.
void verifyOutputCase(
    const ohc::MghImage &sourceImage,
    const SourceOracle &sourceOracle,
    const ohc::ConformStepRequest &request,
    const ohc::ConformStepResult &result,
    const ohc::MghImage &outputImage)
{
    verifyOutputGeometry(sourceImage, sourceOracle, request, result, outputImage);

    const float targetVoxelSize = ohc::computeTargetVoxelSize(sourceImage, request);
    const auto nativeTargetDimensions =
        ohc::computeNativeTargetDimensions(sourceImage, targetVoxelSize, request.imageSizeMode);
    const auto expectedGeometry =
        buildExpectedGeometry(sourceImage, sourceOracle, request, targetVoxelSize, nativeTargetDimensions);
    const auto outputGeometry = buildGeometryImage(outputImage);

    const auto probeStats = evaluateProbes(
        outputImage,
        outputGeometry,
        sourceOracle,
        expectedGeometry,
        ohc::computeScalePolicy(sourceOracle.data, 0.0F, 255.0F));

            requireNear(probeStats.maxPhysicalError, 0.0, test_constants::PHYSICAL_POINT_TOLERANCE_MM,
            "The conformed output physical landmarks differ from the affine oracle.");
            require(probeStats.maxIntensityError <= test_constants::PROBE_MAX_INTENSITY_ERROR_TOLERANCE,
            "The conformed output exceeded the maximum ITK-interpolated probe intensity error tolerance.");
            require(probeStats.percentile95IntensityError <= test_constants::PROBE_PERCENTILE95_INTENSITY_ERROR_TOLERANCE,
            "The conformed output exceeded the 95th-percentile ITK-interpolated probe intensity error tolerance.");
}

// Verifies the already-conformed fast path on the Subject140 MGZ fixture.
void test_subject140_already_conformed()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const std::filesystem::path fixturePath = repoRoot / "data/Subject140/140_orig.mgz";
    const std::filesystem::path outputDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_native_conform_step_test");

    const auto copyPath = outputDir / "copy_orig.mgz";
    const auto conformedPath = outputDir / "conformed_orig.mgz";

    const auto inputImage = ohc::MghImage::load(fixturePath);
    require(!inputImage.rawData().empty(), "The fixture MGZ appears empty or unreadable: " + fixturePath.string());
    require(inputImage.orientationCode().size() == 3 && inputImage.orientationCode().find('?') == std::string::npos,
        "The fixture orientation code could not be derived from the MGZ header: '" + inputImage.orientationCode() + "'.");

    ohc::ConformStepRequest request;
    request.inputPath = fixturePath;
    request.copyOrigPath = copyPath;
    request.conformedPath = conformedPath;

    ohc::ConformStepService service;
    const auto result = service.run(request);

    require(result.success, "The native conform step did not succeed.");
    require(result.alreadyConformed, "The Subject140 fixture should be treated as already conformed.");
    require(std::filesystem::exists(copyPath), "The native conform step did not write the copy_orig output.");
    require(std::filesystem::exists(conformedPath), "The native conform step did not write the conformed output.");

        const SourceOracle sourceOracle = buildSourceOracle(inputImage);
        const auto copyImage = ohc::MghImage::load(copyPath);
    const auto outputImage = ohc::MghImage::load(conformedPath);

        assertCopyOrigMatches(copyImage, inputImage);
            verifyOutputGeometry(inputImage, sourceOracle, request, result, outputImage);

    require(inputImage.header().dimensions == outputImage.header().dimensions,
            "The conformed output dimensions differ from the input fixture.");
    require(inputImage.rawData() == outputImage.rawData(),
            "The conformed output voxel payload differs from the input fixture.");
}

// Verifies that forcing reconform on an already standardized uint8 image does not introduce sampling changes.
void test_measure_sampling_errors_by_force_conformed_already_standardized_image()
{
        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "data/Subject140/140_orig.mgz";
        const std::filesystem::path outputDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_native_force_conform_subject140_test");

        const auto copyPath = outputDir / "copy_orig.mgz";
        const auto conformedPath = outputDir / "conformed_orig.mgz";

        const auto inputImage = ohc::MghImage::load(fixturePath);
        require(!inputImage.rawData().empty(), "The fixture MGZ appears empty or unreadable: " + fixturePath.string());

        ohc::ConformStepRequest request;
        request.inputPath = fixturePath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;
        request.forceConform = true;

        ohc::ConformStepService service;
        const auto result = service.run(request);

        require(result.success, "The forced conform step did not succeed on the already conformed Subject140 fixture.");
        require(!result.alreadyConformed,
            "Forced conform should bypass the already-conformed shortcut for the Subject140 fixture.");
        require(std::filesystem::exists(copyPath), "The forced conform step did not write the copy_orig output.");
        require(std::filesystem::exists(conformedPath), "The forced conform step did not write the conformed output.");

        const SourceOracle sourceOracle = buildSourceOracle(inputImage);
        const auto copyImage = ohc::MghImage::load(copyPath);
        const auto outputImage = ohc::MghImage::load(conformedPath);

        assertCopyOrigMatches(copyImage, inputImage);
        verifyOutputGeometry(inputImage, sourceOracle, request, result, outputImage);

        require(inputImage.header().dimensions == outputImage.header().dimensions,
            "Forced conform changed the Subject140 output dimensions.");
        require(inputImage.rawData() == outputImage.rawData(),
            "Forced conform changed the Subject140 voxel payload even though the input was already conformed.");
}

// Verifies the histogram-scaling path on an already standardized float image.
void test_measure_scaling_errors_by_force_conforming_histogram_scaled_already_standardized_image()
{
        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "data/Subject140/140_orig.mgz";
        const std::filesystem::path outputDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_native_force_histogram_scaled_subject140_test");

        const auto floatInputPath = outputDir / "already_conformed_float_input.mgz";
        const auto copyPath = outputDir / "copy_orig.mgz";
        const auto conformedPath = outputDir / "conformed_orig.mgz";

        const auto inputImage = ohc::MghImage::load(fixturePath);
        require(!inputImage.rawData().empty(), "The fixture MGZ appears empty or unreadable: " + fixturePath.string());
        require(inputImage.isUint8(), "The Subject140 fixture must start as uint8 so the test can isolate histogram scaling from the existing resampling-only shortcut.");

        const std::vector<float> floatSourceData = inputImage.voxelDataAsFloat();
        auto floatHeader = inputImage.header();
        floatHeader.type = static_cast<std::int32_t>(ohc::MghDataType::Float32);
        const auto floatInputImage = ohc::MghImage::fromVoxelData(
            floatHeader,
            floatSourceData,
            static_cast<std::int32_t>(ohc::MghDataType::Float32));
        floatInputImage.save(floatInputPath);

        ohc::ConformStepRequest request;
        request.inputPath = floatInputPath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;
        request.forceConform = true;

        ohc::ConformStepService service;
        const auto result = service.run(request);

        require(result.success,
            "The forced conform step did not succeed on the already standardized float Subject140 fixture.");
        require(!result.alreadyConformed,
            "The float Subject140 input should bypass the already-conformed shortcut and exercise histogram scaling.");
        require(std::filesystem::exists(copyPath), "The forced histogram-scaling test did not write the copy_orig output.");
        require(std::filesystem::exists(conformedPath), "The forced histogram-scaling test did not write the conformed output.");

        const SourceOracle sourceOracle = buildSourceOracle(floatInputImage);
        const auto copyImage = ohc::MghImage::load(copyPath);
        const auto outputImage = ohc::MghImage::load(conformedPath);

        assertCopyOrigMatches(copyImage, floatInputImage);
        verifyOutputCase(floatInputImage, sourceOracle, request, result, outputImage);

        require(floatInputImage.header().dimensions == outputImage.header().dimensions,
            "Forced histogram scaling changed the Subject140 output dimensions.");

        const std::vector<float> expectedScaledData = computeRoundedScaledValues(floatSourceData);
        require(outputImage.voxelDataAsFloat() == expectedScaledData,
            "Forced histogram scaling on the already standardized Subject140 fixture diverged from the shared scale policy.");
        require(outputImage.rawData() != inputImage.rawData(),
            "Forced histogram scaling should change the uint8 payload relative to the original already conformed fixture.");
}

// Verifies conform behavior on the Colin27 MGZ fixture for native and canonical orientations.
void test_colin27_mgz_cases()
{
        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
        const auto fixturePath = repoRoot /
                     "data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/orig/001.mgz";
        const auto outputDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_conform_step_colin27_test");
        const auto expectedCopy = ohc::MghImage::load(fixturePath);
        require(!expectedCopy.rawData().empty(), "The Colin27-1 MGZ fixture appears empty or could not be loaded: " + fixturePath.string());
        require(expectedCopy.orientationCode().size() == 3 && expectedCopy.orientationCode().find('?') == std::string::npos,
            "The Colin27-1 orientation code could not be derived from the MGZ header: '" + expectedCopy.orientationCode() + "'.");

        const SourceOracle sourceOracle = buildSourceOracle(expectedCopy);
        ohc::ConformStepService service;

        for (const ohc::OrientationMode orientation : {ohc::OrientationMode::Native, ohc::OrientationMode::Lia}) {
        const auto caseDir = outputDir / ohc::to_string(orientation);
        const auto copyPath = caseDir / "copy_orig.mgz";
        const auto conformedPath = caseDir / "conformed_orig.mgz";

        ohc::ConformStepRequest request;
        request.inputPath = fixturePath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;
        request.orientation = orientation;

        const auto result = service.run(request);

        require(result.success, "The conform step failed for the Colin27-1 MGZ fixture.");
        require(std::filesystem::exists(copyPath),
            "The conform step did not write the copy-orig MGZ output for the Colin27-1 input.");
        require(std::filesystem::exists(conformedPath),
            "The conform step did not write the conformed MGZ output for the Colin27-1 input.");

        require(result.inputMetadata.dimensions == expectedCopy.header().dimensions,
            "The conform step reported unexpected input dimensions for the Colin27-1 fixture.");
        require(result.inputMetadata.orientationCode == expectedCopy.orientationCode(),
            "The conform step reported an unexpected input orientation for the Colin27-1 fixture.");
        require(result.inputMetadata.dataTypeName == expectedCopy.metadata().dataTypeName,
            "The conform step reported an unexpected input data type for the Colin27-1 fixture.");
        requireNear(result.inputMetadata.spacing[0], expectedCopy.header().spacing[0], test_constants::METADATA_SPACING_TOLERANCE,
                "The conform step reported unexpected X spacing for the Colin27-1 fixture.");
        requireNear(result.inputMetadata.spacing[1], expectedCopy.header().spacing[1], test_constants::METADATA_SPACING_TOLERANCE,
                "The conform step reported unexpected Y spacing for the Colin27-1 fixture.");
        requireNear(result.inputMetadata.spacing[2], expectedCopy.header().spacing[2], test_constants::METADATA_SPACING_TOLERANCE,
                "The conform step reported unexpected Z spacing for the Colin27-1 fixture.");

        const auto copyImage = ohc::MghImage::load(copyPath);
        const auto conformedImage = ohc::MghImage::load(conformedPath);

        assertCopyOrigMatches(copyImage, expectedCopy);
        verifyOutputCase(expectedCopy, sourceOracle, request, result, conformedImage);
        }
}

// Verifies conform behavior on the oblique NIfTI fixture for native and canonical orientations.
void test_oblique_nifti_cases()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const auto fixturePath = repoRoot / "data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii";
    const auto outputDir = makeFreshDirectory("openhc_imaging_mri_fastsurfer_conform_step_nifti_test");
    const auto expectedCopy = ohc::NiftiConverter::loadAsMgh(fixturePath);
    require(!expectedCopy.rawData().empty(), "The NIfTI fixture appears empty or could not be loaded: " + fixturePath.string());

    require(expectedCopy.header().dimensions == std::array<int, 3> {320, 320, 180},
            "The direct NIfTI conversion produced unexpected input dimensions for the oblique fixture.");
    require(expectedCopy.orientationCode() == "LPS",
            "The direct NIfTI conversion produced an unexpected input orientation for the oblique fixture.");
    requireNear(expectedCopy.header().spacing[0], 0.8F, test_constants::METADATA_SPACING_TOLERANCE,
                "The direct NIfTI conversion produced unexpected X spacing for the oblique fixture.");
    requireNear(expectedCopy.header().spacing[1], 0.8F, test_constants::METADATA_SPACING_TOLERANCE,
                "The direct NIfTI conversion produced unexpected Y spacing for the oblique fixture.");
    requireNear(expectedCopy.header().spacing[2], 1.0F, test_constants::METADATA_SPACING_TOLERANCE,
                "The direct NIfTI conversion produced unexpected Z spacing for the oblique fixture.");

    const SourceOracle sourceOracle = buildSourceOracle(expectedCopy);
    ohc::ConformStepService service;

    for (const ohc::OrientationMode orientation : {ohc::OrientationMode::Native, ohc::OrientationMode::Lia}) {
        const auto caseDir = outputDir / ohc::to_string(orientation);
        const auto copyPath = caseDir / "copy_orig.mgz";
        const auto conformedPath = caseDir / "conformed_orig.mgz";

        ohc::ConformStepRequest request;
        request.inputPath = fixturePath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;
        request.orientation = orientation;

        const auto result = service.run(request);

        require(result.success, "The conform step failed for the NIfTI fixture.");
        require(!result.alreadyConformed,
                "The oblique NIfTI fixture should exercise reconforming, not the already-conformed shortcut.");
        require(std::filesystem::exists(copyPath),
                "The conform step did not write the copy-orig MGZ output for the NIfTI input.");
        require(std::filesystem::exists(conformedPath),
                "The conform step did not write the conformed MGZ output for the NIfTI input.");

        require(result.inputMetadata.dimensions == std::array<int, 3> {320, 320, 180},
                "The conform step reported unexpected input dimensions for the NIfTI fixture.");
        require(result.inputMetadata.orientationCode == expectedCopy.orientationCode(),
                "The conform step reported an unexpected input orientation for the NIfTI fixture.");
        require(result.inputMetadata.dataTypeName == expectedCopy.metadata().dataTypeName,
                "The conform step reported an unexpected input data type for the NIfTI fixture.");
        requireNear(result.inputMetadata.spacing[0], 0.8F, test_constants::METADATA_SPACING_TOLERANCE,
                    "The conform step reported unexpected X spacing for the NIfTI fixture.");
        requireNear(result.inputMetadata.spacing[1], 0.8F, test_constants::METADATA_SPACING_TOLERANCE,
                    "The conform step reported unexpected Y spacing for the NIfTI fixture.");
        requireNear(result.inputMetadata.spacing[2], 1.0F, test_constants::METADATA_SPACING_TOLERANCE,
                    "The conform step reported unexpected Z spacing for the NIfTI fixture.");

        const auto copyImage = ohc::MghImage::load(copyPath);
        const auto conformedImage = ohc::MghImage::load(conformedPath);

        assertCopyOrigMatches(copyImage, expectedCopy);
        if (result.alreadyConformed) {
            verifyOutputGeometry(expectedCopy, sourceOracle, request, result, conformedImage);
            require(expectedCopy.rawData() == conformedImage.rawData(),
                    "The already-conformed Colin27-1 output voxel payload differs from the input fixture.");
        } else {
            verifyOutputCase(expectedCopy, sourceOracle, request, result, conformedImage);
        }
    }
}

// Dispatches one named merged conform test case.
// Parameters:
// - caseName: CLI token identifying the test case to run.
void runNamedCase(const std::string &caseName)
{
    if (caseName == "conformed") {
        test_subject140_already_conformed();
        return;
    }

    if (caseName == "force-conformed") {
        test_measure_sampling_errors_by_force_conformed_already_standardized_image();
        return;
    }

    if (caseName == "force-conformed-scaled") {
        test_measure_scaling_errors_by_force_conforming_histogram_scaled_already_standardized_image();
        return;
    }

    if (caseName == "oblique") {
        test_oblique_nifti_cases();
        return;
    }

    if (caseName == "standard") {
        test_colin27_mgz_cases();
        return;
    }

    throw std::runtime_error("Unknown merged conform test case: " + caseName);
}

} // namespace

// Runs one or more conform-step test scenarios from the command line.
// Parameters:
// - argc: Argument count. Optional second argument selects one named case.
// - argv: Argument vector containing the optional case selector.
// Returns 0 on success and 1 on failure.
int main(int argc, char **argv)
{
    try {
        if (argc > 1) {
            runNamedCase(argv[1]);
        } else {
            test_subject140_already_conformed();
            test_colin27_mgz_cases();
            test_oblique_nifti_cases();
        }

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}