#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "imaging/common/mgh_image.h"

namespace OpenHC::tests::fastsurfer::support {

namespace ohc = OpenHC::imaging::mri::fastsurfer;

struct SyntheticNonConformedInputSpec {
    std::array<int, 3> dimensions;
    std::array<float, 3> spacing;
    std::array<float, 9> directionCosines;
    std::array<float, 3> center;
    std::int32_t type;
};

inline SyntheticNonConformedInputSpec makeSubject140NonConformedInputSpec()
{
    return {
        {64, 72, 80},
        {1.1F, 1.2F, 0.9F},
        {
            1.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F,
            0.0F, 0.0F, 1.0F,
        },
        {14.0F, -9.5F, 22.25F},
        static_cast<std::int32_t>(ohc::MghDataType::Float32),
    };
}

inline ohc::MghImage createSyntheticNonConformedInput(
    const ohc::MghImage &sourceImage,
    const SyntheticNonConformedInputSpec &spec)
{
    const auto sourceData = sourceImage.voxelDataAsFloat();
    if (sourceData.empty()) {
        throw std::runtime_error("The source image for synthetic fixture generation appears empty or unreadable.");
    }

    const auto sourceDimensions = sourceImage.header().dimensions;
    for (int axis = 0; axis < 3; ++axis) {
        if (spec.dimensions[axis] <= 0) {
            throw std::runtime_error("Synthetic fixture dimensions must be positive on every axis.");
        }
        if (spec.dimensions[axis] > sourceDimensions[axis]) {
            throw std::runtime_error("Synthetic fixture crop dimensions must fit within the source image.");
        }
    }

    ohc::MghImage::Header header = sourceImage.header();
    header.dimensions = spec.dimensions;
    header.frames = 1;
    header.type = spec.type;
    header.spacing = spec.spacing;
    header.directionCosines = spec.directionCosines;
    header.center = spec.center;

    std::vector<float> cropped(static_cast<std::size_t>(header.dimensions[0]) *
                               static_cast<std::size_t>(header.dimensions[1]) *
                               static_cast<std::size_t>(header.dimensions[2]));

    const int offsetX = (sourceDimensions[0] - header.dimensions[0]) / 2;
    const int offsetY = (sourceDimensions[1] - header.dimensions[1]) / 2;
    const int offsetZ = (sourceDimensions[2] - header.dimensions[2]) / 2;

    for (int z = 0; z < header.dimensions[2]; ++z) {
        for (int y = 0; y < header.dimensions[1]; ++y) {
            for (int x = 0; x < header.dimensions[0]; ++x) {
                const std::size_t sourceIndex =
                    (static_cast<std::size_t>(z + offsetZ) * static_cast<std::size_t>(sourceDimensions[1]) +
                     static_cast<std::size_t>(y + offsetY)) *
                        static_cast<std::size_t>(sourceDimensions[0]) +
                    static_cast<std::size_t>(x + offsetX);
                const std::size_t targetIndex =
                    (static_cast<std::size_t>(z) * static_cast<std::size_t>(header.dimensions[1]) +
                     static_cast<std::size_t>(y)) *
                        static_cast<std::size_t>(header.dimensions[0]) +
                    static_cast<std::size_t>(x);
                cropped[targetIndex] = sourceData[sourceIndex];
            }
        }
    }

    return ohc::MghImage::fromVoxelData(header, cropped, header.type);
}

inline ohc::MghImage createSyntheticNonConformedInput(
    const std::filesystem::path &fixturePath,
    const SyntheticNonConformedInputSpec &spec)
{
    return createSyntheticNonConformedInput(ohc::MghImage::load(fixturePath), spec);
}

} // namespace OpenHC::tests::fastsurfer::support