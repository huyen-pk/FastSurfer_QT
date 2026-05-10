#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "imaging/mri/fastsurfer/label_mapping_result.h"

namespace OpenHC::imaging::mri::fastsurfer {

class FastSurferLabelMapper {
public:
    using Dimensions = std::array<int, 3>;

    static VolumeLabelMappingResult mapMindBoggleDkt31ManualToFastSurfer(
        const std::vector<std::uint16_t> &labels);

    static VolumeLabelMappingResult mapMindBoggleManualAsegToFastSurfer(
        const std::vector<std::uint16_t> &labels,
        const Dimensions &dimensions,
        const LabelMappingOptions &options = {},
        const std::vector<std::uint16_t> *noCcReference = nullptr);

    static VolumeLabelMappingResult mapFreeSurferAparcAsegToFastSurfer(
        const std::vector<std::uint16_t> &labels,
        const Dimensions &dimensions,
        const LabelMappingOptions &options = {},
        const std::vector<std::uint16_t> *noCcReference = nullptr);

    static std::vector<std::uint8_t> mapBratsToTumorForeground(const std::vector<std::uint8_t> &labels);

    static std::vector<std::uint8_t> mapFastSurferToBrainForeground(const std::vector<std::uint16_t> &labels);

    static std::vector<std::uint16_t> splitCortexLabels(
        const std::vector<std::uint16_t> &labels,
        const Dimensions &dimensions);

    static std::vector<std::uint16_t> convertFloatVoxelsToLabels(const std::vector<float> &voxels);

    static std::vector<std::uint16_t> uniqueSortedLabels(const std::vector<std::uint16_t> &labels);
};

} // namespace OpenHC::imaging::mri::fastsurfer