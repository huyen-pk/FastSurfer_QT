#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <Eigen/Dense>

#include <itkImage.h>
#include <itkImportImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkOrientImageFilter.h>
#include <itkSpatialOrientation.h>

#include "fastsurfer/core/conform_step_request.h"
#include "fastsurfer/core/conform_step_result.h"
#include "fastsurfer/core/conform_step_service.h"
#include "fastsurfer/core/mgh_image.h"
#include "fastsurfer/core/nifti_converter.h"

namespace {

using FloatImage3D = itk::Image<float, 3>;
using ImportFilter = itk::ImportImageFilter<float, 3>;
using InterpolatorType = itk::LinearInterpolateImageFunction<FloatImage3D, double>;
using OrientFilter = itk::OrientImageFilter<FloatImage3D, FloatImage3D>;
using OrientationEnum = itk::SpatialOrientation::ValidCoordinateOrientationFlags;

struct SourceOracle {
    std::vector<float> data;
    FloatImage3D::Pointer image;
    InterpolatorType::Pointer interpolator;
};

struct ScalePolicy {
    float srcMin {0.0F};
    float scale {1.0F};
};

struct ProbeStats {
    double maxPhysicalError {0.0};
    double maxIntensityError {0.0};
    double percentile95IntensityError {0.0};
};

std::filesystem::path makeFreshDirectory(const std::string &name)
{
    const auto root = std::filesystem::current_path() / ".tmp" / name;
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    return root;
}

void require(const bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireNear(const float left, const float right, const float tolerance, const std::string &message)
{
    if (std::fabs(left - right) > tolerance) {
        throw std::runtime_error(message + " Expected=" + std::to_string(right) + " actual=" + std::to_string(left));
    }
}

void requireNear(const double left, const double right, const double tolerance, const std::string &message)
{
    if (std::fabs(left - right) > tolerance) {
        throw std::runtime_error(message + " Expected=" + std::to_string(right) + " actual=" + std::to_string(left));
    }
}

Eigen::Matrix4d toEigenMatrix(const fastsurfer::core::Matrix4 &matrix)
{
    Eigen::Matrix4d result = Eigen::Matrix4d::Zero();
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            result(row, column) = matrix[row][column];
        }
    }
    return result;
}

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

float roundToEpsilonPrecision(const float value)
{
    constexpr float scale = 10000.0F;
    return std::round(value * scale) / scale;
}

int conformLikeCeil(const float value)
{
    constexpr double scale = 10000.0;
    return static_cast<int>(std::ceil(std::floor(static_cast<double>(value) * scale) / scale));
}

std::string normalizeUpper(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

OrientationEnum itkOrientationEnum(const std::string &orientation)
{
    const std::string normalized = normalizeUpper(orientation);
    if (normalized == "LIA") {
        return itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA;
    }

    throw std::runtime_error(
        "Unsupported ITK target orientation for the oblique conform oracle: '" + normalized + "'.");
}

float computeTargetVoxelSize(const fastsurfer::core::MghImage &image, const fastsurfer::core::ConformStepRequest &request)
{
    if (request.voxSizeMode != "min") {
        throw std::runtime_error("Unsupported vox_size mode in the oblique conform oracle.");
    }

    float targetVoxelSize = std::min(roundToEpsilonPrecision(image.minVoxelSize()), 1.0F);
    if (request.threshold1mm > 0.0F && targetVoxelSize > request.threshold1mm) {
        targetVoxelSize = 1.0F;
    }
    return targetVoxelSize;
}

std::array<int, 3> computeNativeTargetDimensions(
    const fastsurfer::core::MghImage &image,
    const float targetVoxelSize,
    const std::string &imageSizeMode)
{
    if (imageSizeMode == "auto") {
        if (std::fabs(targetVoxelSize - 1.0F) <= std::fabs(1.0F - 0.95F)) {
            return {256, 256, 256};
        }

        std::array<int, 3> target = image.header().dimensions;
        int maxDimension = 256;
        for (std::size_t index = 0; index < target.size(); ++index) {
            const float fov = image.header().spacing[index] * static_cast<float>(image.header().dimensions[index]);
            target[index] = conformLikeCeil(fov / targetVoxelSize);
            maxDimension = std::max(maxDimension, target[index]);
        }
        return {maxDimension, maxDimension, maxDimension};
    }

    if (imageSizeMode == "fov") {
        std::array<int, 3> target = image.header().dimensions;
        for (std::size_t index = 0; index < target.size(); ++index) {
            const float fov = image.header().spacing[index] * static_cast<float>(image.header().dimensions[index]);
            target[index] = conformLikeCeil(fov / targetVoxelSize);
        }
        return target;
    }

    throw std::runtime_error("Unsupported image_size mode in the oblique conform oracle.");
}

Eigen::Vector4d voxelCenter(const std::array<int, 3> &dimensions)
{
    return {
        static_cast<double>(dimensions[0]) / 2.0,
        static_cast<double>(dimensions[1]) / 2.0,
        static_cast<double>(dimensions[2]) / 2.0,
        1.0,
    };
}

FloatImage3D::DirectionType itkDirectionFromHeader(const fastsurfer::core::MghImage::Header &header)
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

FloatImage3D::SpacingType itkSpacingFromHeader(const fastsurfer::core::MghImage::Header &header)
{
    FloatImage3D::SpacingType spacing;
    for (unsigned int axis = 0; axis < 3; ++axis) {
        spacing[axis] = static_cast<double>(header.spacing[axis]);
    }
    return spacing;
}

FloatImage3D::PointType itkOriginFromAffine(const Eigen::Matrix4d &affine)
{
    FloatImage3D::PointType origin;
    for (unsigned int row = 0; row < 3; ++row) {
        origin[row] = affine(static_cast<int>(row), 3);
    }
    return origin;
}

Eigen::Vector3d toEigenVector(const FloatImage3D::PointType &point)
{
    return {point[0], point[1], point[2]};
}

Eigen::Vector3d toEigenVector(const FloatImage3D::SpacingType &spacing)
{
    return {spacing[0], spacing[1], spacing[2]};
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

std::array<int, 3> dimensionsFromGeometry(const FloatImage3D::Pointer &geometry)
{
    const auto size = geometry->GetLargestPossibleRegion().GetSize();
    return {
        static_cast<int>(size[0]),
        static_cast<int>(size[1]),
        static_cast<int>(size[2]),
    };
}

itk::ContinuousIndex<double, 3> continuousCenterIndex(const std::array<int, 3> &dimensions)
{
    itk::ContinuousIndex<double, 3> centerIndex;
    for (unsigned int axis = 0; axis < 3; ++axis) {
        centerIndex[axis] = static_cast<double>(dimensions[axis]) / 2.0;
    }
    return centerIndex;
}

FloatImage3D::PointType physicalCenter(
    const FloatImage3D::Pointer &geometry,
    const std::array<int, 3> &dimensions)
{
    FloatImage3D::PointType center;
    geometry->TransformContinuousIndexToPhysicalPoint(continuousCenterIndex(dimensions), center);
    return center;
}

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

FloatImage3D::Pointer buildExpectedGeometry(
    const fastsurfer::core::MghImage &sourceImage,
    const SourceOracle &sourceOracle,
    const fastsurfer::core::ConformStepRequest &request,
    const float targetVoxelSize,
    const std::array<int, 3> &nativeTargetDimensions)
{
    auto expectedGeometry = FloatImage3D::New();
    expectedGeometry->SetRegions(itkRegionFromDimensions(nativeTargetDimensions));

    FloatImage3D::SpacingType spacing;
    spacing.Fill(static_cast<double>(targetVoxelSize));
    expectedGeometry->SetSpacing(spacing);
    expectedGeometry->SetDirection(itkDirectionFromHeader(sourceImage.header()));
    expectedGeometry->SetOrigin(computeOriginFromCenter(
        expectedGeometry->GetDirection(),
        spacing,
        nativeTargetDimensions,
        physicalCenter(sourceOracle.image, sourceImage.header().dimensions)));
    expectedGeometry->Allocate();

    if (request.orientation == "native") {
        return expectedGeometry;
    }

    auto orientFilter = OrientFilter::New();
    orientFilter->UseImageDirectionOn();
    orientFilter->SetDesiredCoordinateOrientation(itkOrientationEnum(request.orientation));
    orientFilter->SetInput(expectedGeometry);
    orientFilter->Update();

    auto orientedGeometry = orientFilter->GetOutput();
    orientedGeometry->DisconnectPipeline();
    return orientedGeometry;
}

SourceOracle buildSourceOracle(const fastsurfer::core::MghImage &image)
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

FloatImage3D::Pointer buildGeometryImage(const fastsurfer::core::MghImage &image)
{
    auto geometry = FloatImage3D::New();
    geometry->SetRegions(itkRegionFromDimensions(image.header().dimensions));
    geometry->SetOrigin(itkOriginFromAffine(toEigenMatrix(image.affine())));
    geometry->SetSpacing(itkSpacingFromHeader(image.header()));
    geometry->SetDirection(itkDirectionFromHeader(image.header()));
    geometry->Allocate();
    return geometry;
}

ScalePolicy computeScalePolicy(const std::vector<float> &sourceData)
{
    const auto [minIt, maxIt] = std::minmax_element(sourceData.begin(), sourceData.end());
    const float dataMin = *minIt;
    const float dataMax = *maxIt;
    if (dataMin == dataMax) {
        return {dataMin, 1.0F};
    }

    constexpr int bins = 1000;
    const double binWidth = static_cast<double>(dataMax - dataMin) / static_cast<double>(bins);
    if (binWidth <= std::numeric_limits<double>::epsilon()) {
        return {dataMin, 1.0F};
    }

    std::vector<int> histogram(bins, 0);
    std::size_t nonZeroVoxels = 0;
    for (const float value : sourceData) {
        if (std::fabs(value) >= 1.0e-15F) {
            ++nonZeroVoxels;
        }

        int index = static_cast<int>(std::floor((static_cast<double>(value) - dataMin) / binWidth));
        index = std::clamp(index, 0, bins - 1);
        ++histogram[index];
    }

    std::vector<int> cumulative(bins + 1, 0);
    for (int index = 0; index < bins; ++index) {
        cumulative[index + 1] = cumulative[index] + histogram[index];
    }

    std::vector<float> binEdges(bins + 1, dataMin);
    for (int index = 0; index <= bins; ++index) {
        binEdges[index] = static_cast<float>(static_cast<double>(dataMin) + binWidth * index);
    }

    const int totalVoxels = static_cast<int>(sourceData.size());
    const int upperCutoff = totalVoxels - static_cast<int>((1.0 - 0.999) * static_cast<double>(nonZeroVoxels));

    int upperEdgeIndex = bins - 1;
    for (int index = 0; index < static_cast<int>(cumulative.size()); ++index) {
        if (cumulative[index] >= upperCutoff) {
            upperEdgeIndex = std::max(0, index - 2);
            break;
        }
    }

    const float srcMin = binEdges[0];
    const float srcMax = binEdges[upperEdgeIndex];
    if (srcMin == srcMax) {
        return {srcMin, 1.0F};
    }

    return {srcMin, 255.0F / (srcMax - srcMin)};
}

float scaleExpectedValue(const float mappedValue, const ScalePolicy &policy)
{
    if (std::fabs(mappedValue) <= 1.0e-8F) {
        return 0.0F;
    }

    const float scaled = std::clamp(policy.scale * (mappedValue - policy.srcMin), 0.0F, 255.0F);
    return std::round(scaled);
}

std::size_t flattenIndex(const std::array<int, 3> &dimensions, const std::array<int, 3> &index)
{
    return (static_cast<std::size_t>(index[2]) * static_cast<std::size_t>(dimensions[1]) +
            static_cast<std::size_t>(index[1])) *
               static_cast<std::size_t>(dimensions[0]) +
           static_cast<std::size_t>(index[0]);
}

std::vector<int> axisProbeSamples(const int dimension)
{
    std::vector<int> samples {
        1,
        std::max(1, dimension / 4),
        std::max(1, dimension / 2),
        std::max(1, (3 * dimension) / 4),
        std::max(1, dimension - 2),
    };
    std::sort(samples.begin(), samples.end());
    samples.erase(std::unique(samples.begin(), samples.end()), samples.end());
    return samples;
}

std::vector<std::array<int, 3>> generateProbeIndices(const std::array<int, 3> &dimensions)
{
    const auto xs = axisProbeSamples(dimensions[0]);
    const auto ys = axisProbeSamples(dimensions[1]);
    const auto zs = axisProbeSamples(dimensions[2]);

    std::vector<std::array<int, 3>> probes;
    probes.reserve(xs.size() * ys.size() * zs.size());
    for (const int x : xs) {
        for (const int y : ys) {
            for (const int z : zs) {
                probes.push_back({x, y, z});
            }
        }
    }
    return probes;
}

double percentile95(std::vector<double> values)
{
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());
    const std::size_t index = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(values.size()))) - 1U;
    return values[std::min(index, values.size() - 1U)];
}

ProbeStats evaluateProbes(
    const fastsurfer::core::MghImage &outputImage,
    const FloatImage3D::Pointer &outputGeometry,
    const SourceOracle &sourceOracle,
    const FloatImage3D::Pointer &expectedGeometry,
    const ScalePolicy &scalePolicy)
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

        FloatImage3D::PointType physicalPoint;
        outputGeometry->TransformIndexToPhysicalPoint(itkIndex, physicalPoint);

        FloatImage3D::PointType expectedPoint;
        expectedGeometry->TransformIndexToPhysicalPoint(itkIndex, expectedPoint);
        const double physicalError = std::sqrt(
            std::pow(physicalPoint[0] - expectedPoint[0], 2.0) +
            std::pow(physicalPoint[1] - expectedPoint[1], 2.0) +
            std::pow(physicalPoint[2] - expectedPoint[2], 2.0));
        stats.maxPhysicalError = std::max(stats.maxPhysicalError, physicalError);

        itk::ContinuousIndex<double, 3> sourceContinuousIndex;
        const bool mapped = sourceOracle.image->TransformPhysicalPointToContinuousIndex(physicalPoint, sourceContinuousIndex);

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

void assertCopyOrigMatches(
    const fastsurfer::core::MghImage &copyImage,
    const fastsurfer::core::MghImage &expectedCopy)
{
    require(copyImage.rawData() == expectedCopy.rawData(),
            "The copy-orig output differs from the direct NIfTI-to-MGZ conversion payload.");
    assertMatrixClose(
        toEigenMatrix(copyImage.affine()),
        toEigenMatrix(expectedCopy.affine()),
        1.0e-4,
        "The copy-orig MGZ affine differs from the direct NIfTI conversion affine.");
}

void verifyOutputCase(
    const fastsurfer::core::MghImage &sourceImage,
    const SourceOracle &sourceOracle,
    const fastsurfer::core::ConformStepRequest &request,
    const fastsurfer::core::ConformStepResult &result,
    const fastsurfer::core::MghImage &outputImage)
{
    const float targetVoxelSize = computeTargetVoxelSize(sourceImage, request);
    const auto nativeTargetDimensions =
        computeNativeTargetDimensions(sourceImage, targetVoxelSize, request.imageSizeMode);
    const std::string expectedOrientation = request.orientation == "native"
        ? sourceImage.orientationCode()
        : normalizeUpper(request.orientation);
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
    require(outputImage.hasIsotropicSpacing(targetVoxelSize, 1.0e-5F),
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
        1.0e-4,
        "The conformed MGZ origin differs from the independent oblique geometry oracle.");

    assertMatrixClose(
        toEigenVector(outputGeometry->GetSpacing()),
        toEigenVector(expectedGeometry->GetSpacing()),
        1.0e-5,
        "The conformed MGZ spacing differs from the independent oblique geometry oracle.");

    const Eigen::Matrix3d actualDirection = toDirectionMatrix(outputGeometry->GetDirection());
    const Eigen::Matrix3d expectedDirection = toDirectionMatrix(expectedGeometry->GetDirection());
    assertMatrixClose(
        actualDirection,
        expectedDirection,
        1.0e-5,
        "The conformed direction matrix differs from the expected orientation policy.");
    assertMatrixClose(
        actualDirection.transpose() * actualDirection,
        Eigen::Matrix3d::Identity(),
        1.0e-5,
        "The conformed direction matrix is not orthonormal.");
    requireNear(actualDirection.determinant(), expectedDirection.determinant(), 1.0e-5,
                "The conformed direction handedness differs from the expected orientation policy.");

    requireNear(
        (toEigenVector(physicalCenter(sourceOracle.image, sourceImage.header().dimensions)) -
         toEigenVector(physicalCenter(outputGeometry, outputImage.header().dimensions)))
            .norm(),
        0.0,
        1.0e-4,
                "The conformed output does not preserve the source world-space center.");

    const auto probeStats = evaluateProbes(
        outputImage,
        outputGeometry,
        sourceOracle,
        expectedGeometry,
        computeScalePolicy(sourceOracle.data));

    requireNear(probeStats.maxPhysicalError, 0.0, 1.0e-4,
                "The conformed output physical landmarks differ from the affine oracle.");
    require(probeStats.maxIntensityError <= 2.0,
            "The conformed output exceeded the maximum ITK-interpolated probe intensity error tolerance.");
    require(probeStats.percentile95IntensityError <= 1.0,
            "The conformed output exceeded the 95th-percentile ITK-interpolated probe intensity error tolerance.");
}

} // namespace

int main()
{
    try {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const auto fixturePath = repoRoot / "data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii";
        const auto outputDir = makeFreshDirectory("fastsurfer_core_conform_step_nifti_test");
        const auto expectedCopy = fastsurfer::core::NiftiConverter::loadAsMgh(fixturePath);

        require(expectedCopy.header().dimensions == std::array<int, 3> {320, 320, 180},
                "The direct NIfTI conversion produced unexpected input dimensions for the oblique fixture.");
        require(expectedCopy.orientationCode() == "RAS",
                "The direct NIfTI conversion produced an unexpected input orientation for the oblique fixture.");
        requireNear(expectedCopy.header().spacing[0], 0.8F, 1.0e-5F,
                    "The direct NIfTI conversion produced unexpected X spacing for the oblique fixture.");
        requireNear(expectedCopy.header().spacing[1], 0.8F, 1.0e-5F,
                    "The direct NIfTI conversion produced unexpected Y spacing for the oblique fixture.");
        requireNear(expectedCopy.header().spacing[2], 1.0F, 1.0e-5F,
                    "The direct NIfTI conversion produced unexpected Z spacing for the oblique fixture.");

        const SourceOracle sourceOracle = buildSourceOracle(expectedCopy);
        fastsurfer::core::ConformStepService service;

        for (const std::string &orientation : {std::string("native"), std::string("lia")}) {
            const auto caseDir = outputDir / orientation;
            const auto copyPath = caseDir / "copy_orig.mgz";
            const auto conformedPath = caseDir / "conformed_orig.mgz";

            fastsurfer::core::ConformStepRequest request;
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
            require(result.inputMetadata.orientationCode == "RAS",
                    "The conform step reported an unexpected input orientation for the NIfTI fixture.");
            require(result.inputMetadata.dataTypeName == expectedCopy.metadata().dataTypeName,
                    "The conform step reported an unexpected input data type for the NIfTI fixture.");
            requireNear(result.inputMetadata.spacing[0], 0.8F, 1.0e-5F,
                        "The conform step reported unexpected X spacing for the NIfTI fixture.");
            requireNear(result.inputMetadata.spacing[1], 0.8F, 1.0e-5F,
                        "The conform step reported unexpected Y spacing for the NIfTI fixture.");
            requireNear(result.inputMetadata.spacing[2], 1.0F, 1.0e-5F,
                        "The conform step reported unexpected Z spacing for the NIfTI fixture.");

            const auto copyImage = fastsurfer::core::MghImage::load(copyPath);
            const auto conformedImage = fastsurfer::core::MghImage::load(conformedPath);

            assertCopyOrigMatches(copyImage, expectedCopy);
            verifyOutputCase(expectedCopy, sourceOracle, request, result, conformedImage);
        }

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}