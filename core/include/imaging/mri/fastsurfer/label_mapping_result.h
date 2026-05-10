#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace OpenHC::imaging::mri::fastsurfer {

enum class MappingDisposition : std::uint8_t {
    Exact,
    Reduced,
    Dropped,
    SpatiallyResolved,
    Unsupported,
    AmbiguousInverse,
};

enum class CorpusCallosumResolutionMode : std::uint8_t {
    UseReferenceNoCc,
    Drop,
    RequireWithCcTarget,
};

enum class LabelTargetSpace : std::uint8_t {
    FastSurferSegmentation,
    FastSurferWithCc,
    BratsSimple,
};

struct LabelMappingOptions {
    CorpusCallosumResolutionMode corpusCallosumMode {CorpusCallosumResolutionMode::UseReferenceNoCc};
    LabelTargetSpace targetSpace {LabelTargetSpace::FastSurferSegmentation};
};

template <typename FromLabel, typename ToLabel>
struct LabelMappingRule {
    FromLabel from;
    ToLabel to;
    MappingDisposition disposition {MappingDisposition::Exact};
};

struct VolumeLabelMappingResult {
    std::vector<std::uint16_t> labels;
    std::vector<MappingDisposition> dispositions;

    std::size_t countDisposition(const MappingDisposition disposition) const;
};

} // namespace OpenHC::imaging::mri::fastsurfer