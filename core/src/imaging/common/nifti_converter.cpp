#include "imaging/common/nifti_converter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <itkContinuousIndex.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageIOBase.h>
#include <itkImageIOFactory.h>

#include "imaging/mri/fastsurfer/constants.h"

namespace OpenHC::imaging::mri::fastsurfer {
namespace {

using RasTransform3 = std::array<std::array<double, 3>, 3>;

// Lowercases file-name comparisons for extension checks.
std::string normalizeLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

// Returns true when value ends with the requested suffix.
bool endsWith(const std::string &value, const std::string &suffix)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Maps ITK component types onto the narrower set of MGH storage encodings.
std::int32_t mapStorageType(const itk::ImageIOBase::IOComponentEnum componentType)
{
    using IOComponent = itk::ImageIOBase;

    // Collapse the wider set of NIfTI component types into the subset of MGH
    // storage formats implemented by the native core.
    switch (componentType) {
    case IOComponent::IOComponentEnum::UCHAR:
        return static_cast<std::int32_t>(MghDataType::UChar);
    case IOComponent::IOComponentEnum::CHAR:
    case IOComponent::IOComponentEnum::SHORT:
        return static_cast<std::int32_t>(MghDataType::Int16);
    case IOComponent::IOComponentEnum::USHORT:
    case IOComponent::IOComponentEnum::INT:
        return static_cast<std::int32_t>(MghDataType::Int32);
    case IOComponent::IOComponentEnum::UINT:
    case IOComponent::IOComponentEnum::LONG:
    case IOComponent::IOComponentEnum::ULONG:
    case IOComponent::IOComponentEnum::LONGLONG:
    case IOComponent::IOComponentEnum::ULONGLONG:
    case IOComponent::IOComponentEnum::FLOAT:
    case IOComponent::IOComponentEnum::DOUBLE:
        return static_cast<std::int32_t>(MghDataType::Float32);
    default:
        throw std::runtime_error("Unsupported NIfTI component type for MGH conversion.");
    }
}

// Converts ITK image geometry into the MGH header fields used by the native core.
template <typename TImage>
MghImage::Header buildHeaderFromImage(const typename TImage::Pointer &image, const std::int32_t storageType)
{
    static_assert(TImage::ImageDimension == 3, "NIfTI conversion expects 3D images.");

    MghImage::Header header;
    const auto size = image->GetLargestPossibleRegion().GetSize();
    const auto spacing = image->GetSpacing();
    const auto direction = image->GetDirection();

    header.dimensions = {
        static_cast<int>(size[0]),
        static_cast<int>(size[1]),
        static_cast<int>(size[2]),
    };
    header.frames = 1;
    header.type = storageType;
    header.rasGoodFlag = 1;
    header.spacing = {
        static_cast<float>(spacing[0]),
        static_cast<float>(spacing[1]),
        static_cast<float>(spacing[2]),
    };

    RasTransform3 lpsDirection {{
        {{direction[0][0], direction[0][1], direction[0][2]}},
        {{direction[1][0], direction[1][1], direction[1][2]}},
        {{direction[2][0], direction[2][1], direction[2][2]}},
    }};
    // ITK exposes physical geometry in LPS coordinates, while MGH headers store
    // direction cosines and centers in RAS coordinates.
    for (int column = 0; column < 3; ++column) {
        for (int row = 0; row < 3; ++row) {
            header.directionCosines[column * 3 + row] =
                static_cast<float>(constants::nifti::LPS_TO_RAS_SIGN[row] * lpsDirection[row][column]);
        }
    }

    itk::ContinuousIndex<double, 3> centerIndex;
    centerIndex[0] = static_cast<double>(size[0]) / 2.0;
    centerIndex[1] = static_cast<double>(size[1]) / 2.0;
    centerIndex[2] = static_cast<double>(size[2]) / 2.0;

    // Match the MGH center convention by sampling the physical point at the
    // continuous half-dimension of the ITK image.
    typename TImage::PointType centerPointLps;
    image->TransformContinuousIndexToPhysicalPoint(centerIndex, centerPointLps);
    header.center = {
        static_cast<float>(constants::nifti::LPS_TO_RAS_SIGN[0] * centerPointLps[0]),
        static_cast<float>(constants::nifti::LPS_TO_RAS_SIGN[1] * centerPointLps[1]),
        static_cast<float>(constants::nifti::LPS_TO_RAS_SIGN[2] * centerPointLps[2]),
    };
    return header;
}

// Reads a typed ITK image and converts its voxel buffer into the MghImage model.
template <typename PixelType>
MghImage readAsMghImage(const std::filesystem::path &inputPath, const std::int32_t storageType)
{
    using ImageType = itk::Image<PixelType, 3>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(inputPath.string());
    reader->Update();

    const typename ImageType::Pointer image = reader->GetOutput();
    const auto region = image->GetLargestPossibleRegion();
    const auto voxelCount = static_cast<std::size_t>(region.GetNumberOfPixels());

    std::vector<float> voxels(voxelCount);
    const PixelType *buffer = image->GetBufferPointer();
    // Normalize the import path around float samples so the downstream MGH
    // encoder owns the final storage-type conversion in one place.
    for (std::size_t index = 0; index < voxelCount; ++index) {
        voxels[index] = static_cast<float>(buffer[index]);
    }

    return MghImage::fromVoxelData(buildHeaderFromImage<ImageType>(image, storageType), voxels, storageType);
}

// Dispatches to the matching ITK pixel type after image-IO probing.
MghImage loadTypedImage(const std::filesystem::path &inputPath, const itk::ImageIOBase::IOComponentEnum componentType)
{
    const std::int32_t storageType = mapStorageType(componentType);
    using IOComponent = itk::ImageIOBase;

    switch (componentType) {
    case IOComponent::IOComponentEnum::UCHAR:
        return readAsMghImage<std::uint8_t>(inputPath, storageType);
    case IOComponent::IOComponentEnum::CHAR:
        return readAsMghImage<std::int8_t>(inputPath, storageType);
    case IOComponent::IOComponentEnum::USHORT:
        return readAsMghImage<std::uint16_t>(inputPath, storageType);
    case IOComponent::IOComponentEnum::SHORT:
        return readAsMghImage<std::int16_t>(inputPath, storageType);
    case IOComponent::IOComponentEnum::UINT:
        return readAsMghImage<std::uint32_t>(inputPath, storageType);
    case IOComponent::IOComponentEnum::INT:
        return readAsMghImage<std::int32_t>(inputPath, storageType);
    case IOComponent::IOComponentEnum::LONG:
        return readAsMghImage<long>(inputPath, storageType);
    case IOComponent::IOComponentEnum::ULONG:
        return readAsMghImage<unsigned long>(inputPath, storageType);
    case IOComponent::IOComponentEnum::LONGLONG:
        return readAsMghImage<long long>(inputPath, storageType);
    case IOComponent::IOComponentEnum::ULONGLONG:
        return readAsMghImage<unsigned long long>(inputPath, storageType);
    case IOComponent::IOComponentEnum::FLOAT:
        return readAsMghImage<float>(inputPath, storageType);
    case IOComponent::IOComponentEnum::DOUBLE:
        return readAsMghImage<double>(inputPath, storageType);
    default:
        throw std::runtime_error("Unsupported NIfTI component type for MGZ conversion.");
    }
}

} // namespace

bool NiftiConverter::isSupportedPath(const std::filesystem::path &path)
{
    const std::string normalized = normalizeLower(path.filename().string());
    return endsWith(normalized, ".nii") || endsWith(normalized, ".nii.gz");
}

MghImage NiftiConverter::loadAsMgh(const std::filesystem::path &inputPath)
{
    if (!isSupportedPath(inputPath)) {
        throw std::runtime_error("NIfTI conversion requires a .nii or .nii.gz input path.");
    }

    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(
        inputPath.string().c_str(), itk::ImageIOFactory::IOFileModeEnum::ReadMode);
    if (!imageIO) {
        throw std::runtime_error("Failed to create an ITK image reader for input: " + inputPath.string());
    }

    imageIO->SetFileName(inputPath.string());
    imageIO->ReadImageInformation();

    // The native conform service currently mirrors only the scalar 3D branch of
    // the Python pipeline, so reject vector, tensor, and 4D inputs early.
    if (imageIO->GetNumberOfDimensions() != 3) {
        throw std::runtime_error("Native NIfTI conversion currently supports only 3D scalar inputs.");
    }
    if (imageIO->GetNumberOfComponents() != 1 || imageIO->GetPixelType() != itk::IOPixelEnum::SCALAR) {
        throw std::runtime_error("Native NIfTI conversion currently supports only scalar inputs.");
    }

    return loadTypedImage(inputPath, imageIO->GetComponentType());
}

void NiftiConverter::convertToMgh(const std::filesystem::path &inputPath, const std::filesystem::path &outputPath)
{
    loadAsMgh(inputPath).save(outputPath);
}

} // namespace OpenHC::imaging::mri::fastsurfer