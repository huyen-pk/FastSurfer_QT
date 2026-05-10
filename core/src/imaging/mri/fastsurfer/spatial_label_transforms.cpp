#include "imaging/mri/fastsurfer/spatial_label_transforms.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "imaging/mri/fastsurfer/constants.h"
#include "imaging/mri/fastsurfer/label_types.h"

namespace OpenHC::imaging::mri::fastsurfer {
namespace {

struct Coordinate {
    int x {0};
    int y {0};
    int z {0};
};

std::size_t flatIndex(const Coordinate coordinate, const UnknownCortexFillTransform::Dimensions &dimensions)
{
    return (static_cast<std::size_t>(coordinate.z) * static_cast<std::size_t>(dimensions[1]) +
            static_cast<std::size_t>(coordinate.y)) *
               static_cast<std::size_t>(dimensions[0]) +
           static_cast<std::size_t>(coordinate.x);
}

bool inBounds(const Coordinate coordinate, const UnknownCortexFillTransform::Dimensions &dimensions)
{
    return coordinate.x >= 0 && coordinate.y >= 0 && coordinate.z >= 0 &&
           coordinate.x < dimensions[0] && coordinate.y < dimensions[1] && coordinate.z < dimensions[2];
}

Coordinate coordinateFromFlatIndex(const std::size_t index, const UnknownCortexFillTransform::Dimensions &dimensions)
{
    const std::size_t planeSize = static_cast<std::size_t>(dimensions[0]) * static_cast<std::size_t>(dimensions[1]);
    const std::size_t z = index / planeSize;
    const std::size_t remainder = index % planeSize;
    const std::size_t y = remainder / static_cast<std::size_t>(dimensions[0]);
    const std::size_t x = remainder % static_cast<std::size_t>(dimensions[0]);
    return {
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(z),
    };
}

std::vector<Coordinate> shellOffsets()
{
    std::vector<Coordinate> offsets;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }

                const int squaredDistance = dx * dx + dy * dy + dz * dz;
                if (squaredDistance <= constants::label_mapping::LOCAL_NEIGHBOR_MAX_SQUARED_DISTANCE) {
                    offsets.push_back({dx, dy, dz});
                }
            }
        }
    }
    return offsets;
}

std::vector<double> gaussianWeights()
{
    std::vector<double> weights(
        static_cast<std::size_t>(constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS) + 1U,
        0.0);
    for (int distance = 0; distance <= constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS; ++distance) {
        const double normalized = static_cast<double>(distance) /
                                  constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_SIGMA;
        weights[static_cast<std::size_t>(distance)] = std::exp(-0.5 * normalized * normalized);
    }
    return weights;
}

std::vector<Coordinate> connectivityOffsets26()
{
    std::vector<Coordinate> offsets;
    offsets.reserve(26U);
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }

                offsets.push_back({dx, dy, dz});
            }
        }
    }
    return offsets;
}

int reflectIndex(int index, const int size)
{
    if (size <= 1) {
        return 0;
    }

    while (index < 0 || index >= size) {
        if (index < 0) {
            index = -index - 1;
        } else {
            index = (2 * size) - index - 1;
        }
    }

    return index;
}

std::vector<float> buildGaussianKernel(const double sigma, const double truncate)
{
    const int radius = std::max(1, static_cast<int>(std::llround(truncate * sigma)));
    std::vector<float> kernel(static_cast<std::size_t>(2 * radius + 1), 0.0F);
    double sum = 0.0;
    for (int offset = -radius; offset <= radius; ++offset) {
        const double normalized = static_cast<double>(offset) / sigma;
        const double value = std::exp(-0.5 * normalized * normalized);
        kernel[static_cast<std::size_t>(offset + radius)] = static_cast<float>(value);
        sum += value;
    }

    for (float &value : kernel) {
        value = static_cast<float>(static_cast<double>(value) / sum);
    }
    return kernel;
}

std::vector<float> gaussianBlurMask(
    const std::vector<std::uint16_t> &labels,
    const UnknownCortexFillTransform::Dimensions &dimensions,
    const std::uint16_t targetLabel,
    const double sigma)
{
    const std::vector<float> kernel = buildGaussianKernel(
        sigma,
        constants::label_mapping::SPLIT_OVERLAP_GAUSSIAN_TRUNCATE);
    const int radius = static_cast<int>((kernel.size() - 1U) / 2U);
    const std::size_t voxelCount = labels.size();
    std::vector<float> input(voxelCount, 0.0F);
    std::vector<float> temp(voxelCount, 0.0F);
    std::vector<float> output(voxelCount, 0.0F);

    for (std::size_t index = 0; index < voxelCount; ++index) {
        input[index] = labels[index] == targetLabel ? 1000.0F : 0.0F;
    }

    for (int z = 0; z < dimensions[2]; ++z) {
        for (int y = 0; y < dimensions[1]; ++y) {
            for (int x = 0; x < dimensions[0]; ++x) {
                float value = 0.0F;
                for (int offset = -radius; offset <= radius; ++offset) {
                    const int sampleX = reflectIndex(x + offset, dimensions[0]);
                    value += kernel[static_cast<std::size_t>(offset + radius)] *
                             input[flatIndex({sampleX, y, z}, dimensions)];
                }
                temp[flatIndex({x, y, z}, dimensions)] = value;
            }
        }
    }

    for (int z = 0; z < dimensions[2]; ++z) {
        for (int y = 0; y < dimensions[1]; ++y) {
            for (int x = 0; x < dimensions[0]; ++x) {
                float value = 0.0F;
                for (int offset = -radius; offset <= radius; ++offset) {
                    const int sampleY = reflectIndex(y + offset, dimensions[1]);
                    value += kernel[static_cast<std::size_t>(offset + radius)] *
                             temp[flatIndex({x, sampleY, z}, dimensions)];
                }
                output[flatIndex({x, y, z}, dimensions)] = value;
            }
        }
    }

    for (int z = 0; z < dimensions[2]; ++z) {
        for (int y = 0; y < dimensions[1]; ++y) {
            for (int x = 0; x < dimensions[0]; ++x) {
                float value = 0.0F;
                for (int offset = -radius; offset <= radius; ++offset) {
                    const int sampleZ = reflectIndex(z + offset, dimensions[2]);
                    value += kernel[static_cast<std::size_t>(offset + radius)] *
                             output[flatIndex({x, y, sampleZ}, dimensions)];
                }
                temp[flatIndex({x, y, z}, dimensions)] = value;
            }
        }
    }

    return temp;
}

void validateDimensions(const std::vector<std::uint16_t> &labels, const UnknownCortexFillTransform::Dimensions &dimensions)
{
    if (dimensions[0] <= 0 || dimensions[1] <= 0 || dimensions[2] <= 0) {
        throw std::invalid_argument("Unknown-cortex fill requires strictly positive dimensions.");
    }

    const std::size_t expectedSize = static_cast<std::size_t>(dimensions[0]) *
                                     static_cast<std::size_t>(dimensions[1]) *
                                     static_cast<std::size_t>(dimensions[2]);
    if (expectedSize != labels.size()) {
        throw std::invalid_argument("Unknown-cortex fill dimensions do not match the label buffer size.");
    }
}

std::array<double, 3> computeLargestComponentCentroid(
    const std::vector<std::uint16_t> &labels,
    const UnknownCortexFillTransform::Dimensions &dimensions,
    const std::uint16_t targetLabel)
{
    const auto offsets = connectivityOffsets26();
    std::vector<std::uint8_t> visited(labels.size(), 0U);
    std::size_t largestCount = 0U;
    std::array<double, 3> largestCentroid {0.0, 0.0, 0.0};

    for (std::size_t seedIndex = 0; seedIndex < labels.size(); ++seedIndex) {
        if (visited[seedIndex] != 0U || labels[seedIndex] != targetLabel) {
            continue;
        }

        std::deque<std::size_t> pending;
        pending.push_back(seedIndex);
        visited[seedIndex] = 1U;

        std::size_t componentCount = 0U;
        double sumX = 0.0;
        double sumY = 0.0;
        double sumZ = 0.0;

        while (!pending.empty()) {
            const std::size_t currentIndex = pending.front();
            pending.pop_front();
            const Coordinate current = coordinateFromFlatIndex(currentIndex, dimensions);
            ++componentCount;
            sumX += static_cast<double>(current.x);
            sumY += static_cast<double>(current.y);
            sumZ += static_cast<double>(current.z);

            for (const Coordinate offset : offsets) {
                const Coordinate neighbor {
                    current.x + offset.x,
                    current.y + offset.y,
                    current.z + offset.z,
                };
                if (!inBounds(neighbor, dimensions)) {
                    continue;
                }

                const std::size_t neighborIndex = flatIndex(neighbor, dimensions);
                if (visited[neighborIndex] != 0U || labels[neighborIndex] != targetLabel) {
                    continue;
                }

                visited[neighborIndex] = 1U;
                pending.push_back(neighborIndex);
            }
        }

        if (componentCount > largestCount) {
            largestCount = componentCount;
            largestCentroid = {
                sumX / static_cast<double>(componentCount),
                sumY / static_cast<double>(componentCount),
                sumZ / static_cast<double>(componentCount),
            };
        }
    }

    if (largestCount == 0U) {
        throw std::invalid_argument("Split-cortex relabeling requires white-matter support in both hemispheres.");
    }

    return largestCentroid;
}

double squaredDistance(const Coordinate coordinate, const std::array<double, 3> &centroid)
{
    const double dx = static_cast<double>(coordinate.x) - centroid[0];
    const double dy = static_cast<double>(coordinate.y) - centroid[1];
    const double dz = static_cast<double>(coordinate.z) - centroid[2];
    return dx * dx + dy * dy + dz * dz;
}

void relabelComponentsCloserToRightHemisphere(
    std::vector<std::uint16_t> &labels,
    const UnknownCortexFillTransform::Dimensions &dimensions,
    const std::uint16_t targetLabel,
    const std::array<double, 3> &leftCentroid,
    const std::array<double, 3> &rightCentroid)
{
    const auto offsets = connectivityOffsets26();
    std::vector<std::uint8_t> visited(labels.size(), 0U);

    for (std::size_t seedIndex = 0; seedIndex < labels.size(); ++seedIndex) {
        if (visited[seedIndex] != 0U || labels[seedIndex] != targetLabel) {
            continue;
        }

        std::deque<std::size_t> pending;
        std::vector<std::size_t> componentIndices;
        pending.push_back(seedIndex);
        visited[seedIndex] = 1U;

        double sumX = 0.0;
        double sumY = 0.0;
        double sumZ = 0.0;

        while (!pending.empty()) {
            const std::size_t currentIndex = pending.front();
            pending.pop_front();
            componentIndices.push_back(currentIndex);

            const Coordinate current = coordinateFromFlatIndex(currentIndex, dimensions);
            sumX += static_cast<double>(current.x);
            sumY += static_cast<double>(current.y);
            sumZ += static_cast<double>(current.z);

            for (const Coordinate offset : offsets) {
                const Coordinate neighbor {
                    current.x + offset.x,
                    current.y + offset.y,
                    current.z + offset.z,
                };
                if (!inBounds(neighbor, dimensions)) {
                    continue;
                }

                const std::size_t neighborIndex = flatIndex(neighbor, dimensions);
                if (visited[neighborIndex] != 0U || labels[neighborIndex] != targetLabel) {
                    continue;
                }

                visited[neighborIndex] = 1U;
                pending.push_back(neighborIndex);
            }
        }

        const Coordinate componentCentroid {
            static_cast<int>(std::lround(sumX / static_cast<double>(componentIndices.size()))),
            static_cast<int>(std::lround(sumY / static_cast<double>(componentIndices.size()))),
            static_cast<int>(std::lround(sumZ / static_cast<double>(componentIndices.size()))),
        };

        if (squaredDistance(componentCentroid, rightCentroid) < squaredDistance(componentCentroid, leftCentroid)) {
            for (const std::size_t componentIndex : componentIndices) {
                labels[componentIndex] = static_cast<std::uint16_t>(targetLabel + 1000U);
            }
        }
    }
}

void applyOverlapSplits(
    std::vector<std::uint16_t> &labels,
    const UnknownCortexFillTransform::Dimensions &dimensions)
{
    const std::vector<float> leftSupport = gaussianBlurMask(
        labels,
        dimensions,
        toUnderlying(FastSurferLabels::LeftCerebralWhiteMatter),
        constants::label_mapping::SPLIT_OVERLAP_GAUSSIAN_SIGMA);
    const std::vector<float> rightSupport = gaussianBlurMask(
        labels,
        dimensions,
        toUnderlying(FastSurferLabels::RightCerebralWhiteMatter),
        constants::label_mapping::SPLIT_OVERLAP_GAUSSIAN_SIGMA);

    for (const std::uint16_t leftLabel : constants::label_mapping::SPLIT_OVERLAP_LEFT_LABELS) {
        const std::uint16_t rightLabel = static_cast<std::uint16_t>(leftLabel + 1000U);
        for (std::size_t index = 0; index < labels.size(); ++index) {
            if (labels[index] != leftLabel && labels[index] != rightLabel) {
                continue;
            }

            labels[index] = leftSupport[index] >= rightSupport[index] ? leftLabel : rightLabel;
        }
    }
}

void fillUnknownPerHemisphere(
    std::vector<std::uint16_t> &labels,
    const UnknownCortexFillTransform::Dimensions &dimensions,
    const std::uint16_t unknownLabel,
    const std::uint16_t cortexStop)
{
    const auto offsets = shellOffsets();
    const auto weights = gaussianWeights();

    std::vector<Coordinate> unknownCoordinates;
    Coordinate minCoordinate {dimensions[0], dimensions[1], dimensions[2]};
    Coordinate maxCoordinate {0, 0, 0};

    for (int z = 0; z < dimensions[2]; ++z) {
        for (int y = 0; y < dimensions[1]; ++y) {
            for (int x = 0; x < dimensions[0]; ++x) {
                const Coordinate coordinate {x, y, z};
                if (labels[flatIndex(coordinate, dimensions)] != unknownLabel) {
                    continue;
                }

                unknownCoordinates.push_back(coordinate);
                minCoordinate.x = std::min(minCoordinate.x, coordinate.x);
                minCoordinate.y = std::min(minCoordinate.y, coordinate.y);
                minCoordinate.z = std::min(minCoordinate.z, coordinate.z);
                maxCoordinate.x = std::max(maxCoordinate.x, coordinate.x);
                maxCoordinate.y = std::max(maxCoordinate.y, coordinate.y);
                maxCoordinate.z = std::max(maxCoordinate.z, coordinate.z);
            }
        }
    }

    if (unknownCoordinates.empty()) {
        return;
    }

    std::set<std::uint16_t> candidateLabels;
    for (const Coordinate coordinate : unknownCoordinates) {
        for (const Coordinate offset : offsets) {
            const Coordinate neighbor {coordinate.x + offset.x, coordinate.y + offset.y, coordinate.z + offset.z};
            if (!inBounds(neighbor, dimensions)) {
                continue;
            }

            const std::uint16_t neighborLabel = labels[flatIndex(neighbor, dimensions)];
            if (neighborLabel > unknownLabel && neighborLabel < cortexStop) {
                candidateLabels.insert(neighborLabel);
            }
        }
    }

    if (candidateLabels.empty()) {
        return;
    }

    const Coordinate roiMin {
        std::max(0, minCoordinate.x - constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS),
        std::max(0, minCoordinate.y - constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS),
        std::max(0, minCoordinate.z - constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS),
    };
    const Coordinate roiMax {
        std::min(dimensions[0] - 1, maxCoordinate.x + constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS),
        std::min(dimensions[1] - 1, maxCoordinate.y + constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS),
        std::min(dimensions[2] - 1, maxCoordinate.z + constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS),
    };

    std::map<std::uint16_t, std::vector<Coordinate>> parcelCoordinates;
    for (const std::uint16_t candidateLabel : candidateLabels) {
        parcelCoordinates[candidateLabel] = {};
    }

    for (int z = roiMin.z; z <= roiMax.z; ++z) {
        for (int y = roiMin.y; y <= roiMax.y; ++y) {
            for (int x = roiMin.x; x <= roiMax.x; ++x) {
                const Coordinate coordinate {x, y, z};
                const std::uint16_t label = labels[flatIndex(coordinate, dimensions)];
                auto iterator = parcelCoordinates.find(label);
                if (iterator != parcelCoordinates.end()) {
                    iterator->second.push_back(coordinate);
                }
            }
        }
    }

    for (const Coordinate unknownCoordinate : unknownCoordinates) {
        double bestScore = -1.0;
        std::uint16_t bestLabel = unknownLabel;

        for (const auto &[candidateLabel, coordinates] : parcelCoordinates) {
            double score = 0.0;
            for (const Coordinate parcelCoordinate : coordinates) {
                const int dx = std::abs(unknownCoordinate.x - parcelCoordinate.x);
                const int dy = std::abs(unknownCoordinate.y - parcelCoordinate.y);
                const int dz = std::abs(unknownCoordinate.z - parcelCoordinate.z);
                if (dx > constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS ||
                    dy > constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS ||
                    dz > constants::label_mapping::UNKNOWN_FILL_GAUSSIAN_RADIUS) {
                    continue;
                }

                score += weights[static_cast<std::size_t>(dx)] *
                         weights[static_cast<std::size_t>(dy)] *
                         weights[static_cast<std::size_t>(dz)];
            }

            if (score > bestScore) {
                bestScore = score;
                bestLabel = candidateLabel;
            }
        }

        if (bestLabel != unknownLabel) {
            labels[flatIndex(unknownCoordinate, dimensions)] = bestLabel;
        }
    }
}

} // namespace

void UnknownCortexFillTransform::applyInPlace(
    std::vector<std::uint16_t> &labels,
    const UnknownCortexFillTransform::Dimensions &dimensions) const
{
    validateDimensions(labels, dimensions);

    fillUnknownPerHemisphere(
        labels,
        dimensions,
        toUnderlying(FastSurferAuxiliaryLabel::UnknownLeftCortex),
        toUnderlying(FastSurferAuxiliaryLabel::UnknownRightCortex));
    fillUnknownPerHemisphere(labels, dimensions, toUnderlying(FastSurferAuxiliaryLabel::UnknownRightCortex), 3000U);
}

void SplitCortexLabelsTransform::applyInPlace(
    std::vector<std::uint16_t> &labels,
    const SplitCortexLabelsTransform::Dimensions &dimensions) const
{
    validateDimensions(labels, dimensions);

    const std::array<double, 3> rightCentroid = computeLargestComponentCentroid(
        labels,
        dimensions,
        toUnderlying(FastSurferLabels::RightCerebralWhiteMatter));
    const std::array<double, 3> leftCentroid = computeLargestComponentCentroid(
        labels,
        dimensions,
        toUnderlying(FastSurferLabels::LeftCerebralWhiteMatter));

    for (const std::uint16_t leftLabel : constants::label_mapping::SPLIT_ELIGIBLE_LEFT_LABELS) {
        relabelComponentsCloserToRightHemisphere(labels, dimensions, leftLabel, leftCentroid, rightCentroid);
    }

    applyOverlapSplits(labels, dimensions);
}

} // namespace OpenHC::imaging::mri::fastsurfer