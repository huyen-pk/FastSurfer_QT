#include "imaging/mri/fastsurfer/label_mapping.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

#include "imaging/mri/fastsurfer/label_types.h"
#include "imaging/mri/fastsurfer/spatial_label_transforms.h"

namespace OpenHC::imaging::mri::fastsurfer {
namespace {

bool isCorpusCallosumLabel(const std::uint16_t label)
{
    return label >= toUnderlying(FastSurferAuxiliaryLabel::CorpusCallosumPosterior) &&
           label <= toUnderlying(FastSurferAuxiliaryLabel::CorpusCallosumAnterior);
}

bool isUnsupportedSurfaceOnlyLabel(const std::uint16_t label)
{
    switch (label) {
    case toUnderlying(FastSurferAuxiliaryLabel::LeftBankssts):
    case toUnderlying(FastSurferAuxiliaryLabel::RightBankssts):
    case toUnderlying(FastSurferAuxiliaryLabel::LeftFrontalPole):
    case toUnderlying(FastSurferAuxiliaryLabel::RightFrontalPole):
    case toUnderlying(FastSurferAuxiliaryLabel::LeftTemporalPole):
    case toUnderlying(FastSurferAuxiliaryLabel::RightTemporalPole):
        return true;
    default:
        return false;
    }
}

void validateDimensions(const std::vector<std::uint16_t> &labels, const FastSurferLabelMapper::Dimensions &dimensions)
{
    if (dimensions[0] <= 0 || dimensions[1] <= 0 || dimensions[2] <= 0) {
        throw std::invalid_argument("Label mapping requires strictly positive dimensions.");
    }

    const std::size_t expectedSize = static_cast<std::size_t>(dimensions[0]) *
                                     static_cast<std::size_t>(dimensions[1]) *
                                     static_cast<std::size_t>(dimensions[2]);
    if (labels.size() != expectedSize) {
        throw std::invalid_argument("Label mapping dimensions do not match the label buffer size.");
    }
}

void applyCorpusCallosumResolution(
    VolumeLabelMappingResult &result,
    const LabelMappingOptions &options,
    const std::vector<std::uint16_t> *noCcReference)
{
    if (options.targetSpace == LabelTargetSpace::FastSurferWithCc) {
        return;
    }

    if (options.corpusCallosumMode == CorpusCallosumResolutionMode::RequireWithCcTarget) {
        for (std::size_t index = 0; index < result.labels.size(); ++index) {
            if (isCorpusCallosumLabel(result.labels[index])) {
                throw std::invalid_argument(
                    "Corpus callosum labels require a with-CC target space or an explicit reduced-space policy.");
            }
        }
        return;
    }

    if (options.corpusCallosumMode == CorpusCallosumResolutionMode::UseReferenceNoCc) {
        if (noCcReference == nullptr) {
            throw std::invalid_argument(
                "UseReferenceNoCc requires a companion no-CC reference volume for corpus callosum replacement.");
        }
        if (noCcReference->size() != result.labels.size()) {
            throw std::invalid_argument("Corpus callosum reference volume does not match the source label size.");
        }
    }

    for (std::size_t index = 0; index < result.labels.size(); ++index) {
        if (!isCorpusCallosumLabel(result.labels[index])) {
            continue;
        }

        if (options.corpusCallosumMode == CorpusCallosumResolutionMode::UseReferenceNoCc) {
            result.labels[index] = noCcReference->at(index);
            result.dispositions[index] = MappingDisposition::Reduced;
            continue;
        }

        result.labels[index] = toUnderlying(FastSurferLabels::Background);
        result.dispositions[index] = MappingDisposition::Dropped;
    }
}

void applyStaticReductions(VolumeLabelMappingResult &result)
{
    for (std::size_t index = 0; index < result.labels.size(); ++index) {
        std::uint16_t &label = result.labels[index];
        switch (label) {
        case 29:
        case 61:
            label = toUnderlying(FastSurferLabels::Background);
            result.dispositions[index] = MappingDisposition::Dropped;
            break;
        case 30:
            label = toUnderlying(FastSurferLabels::LeftCerebralWhiteMatter);
            result.dispositions[index] = MappingDisposition::Reduced;
            break;
        case 62:
            label = toUnderlying(FastSurferLabels::RightCerebralWhiteMatter);
            result.dispositions[index] = MappingDisposition::Reduced;
            break;
        case 72:
            label = toUnderlying(FastSurferLabels::CSF);
            result.dispositions[index] = MappingDisposition::Reduced;
            break;
        case 80:
            label = toUnderlying(FastSurferLabels::WMHypointensities);
            result.dispositions[index] = MappingDisposition::Reduced;
            break;
        case 85:
            label = toUnderlying(FastSurferLabels::Background);
            result.dispositions[index] = MappingDisposition::Dropped;
            break;
        default:
            break;
        }
    }
}

void applyUnsupportedSurfaceDrops(VolumeLabelMappingResult &result)
{
    for (std::size_t index = 0; index < result.labels.size(); ++index) {
        if (!isUnsupportedSurfaceOnlyLabel(result.labels[index])) {
            continue;
        }

        result.labels[index] = toUnderlying(FastSurferLabels::Background);
        result.dispositions[index] = MappingDisposition::Unsupported;
    }
}

void applyUnknownFill(VolumeLabelMappingResult &result, const FastSurferLabelMapper::Dimensions &dimensions)
{
    const std::vector<std::uint16_t> beforeFill = result.labels;

    UnknownCortexFillTransform transform;
    transform.applyInPlace(result.labels, dimensions);

    for (std::size_t index = 0; index < result.labels.size(); ++index) {
           if ((beforeFill[index] == toUnderlying(FastSurferAuxiliaryLabel::UnknownLeftCortex) ||
               beforeFill[index] == toUnderlying(FastSurferAuxiliaryLabel::UnknownRightCortex)) &&
            result.labels[index] != beforeFill[index]) {
            result.dispositions[index] = MappingDisposition::SpatiallyResolved;
        }
    }
}

VolumeLabelMappingResult mapRichVolumeToFastSurfer(
    const std::vector<std::uint16_t> &labels,
    const FastSurferLabelMapper::Dimensions &dimensions,
    const LabelMappingOptions &options,
    const std::vector<std::uint16_t> *noCcReference,
    const bool dropUnsupportedSurfaceLabels)
{
    validateDimensions(labels, dimensions);

    VolumeLabelMappingResult result;
    result.labels = labels;
    result.dispositions.assign(labels.size(), MappingDisposition::Exact);

    applyCorpusCallosumResolution(result, options, noCcReference);
    applyStaticReductions(result);
    applyUnknownFill(result, dimensions);
    if (dropUnsupportedSurfaceLabels) {
        applyUnsupportedSurfaceDrops(result);
    }
    return result;
}

} // namespace

std::size_t VolumeLabelMappingResult::countDisposition(const MappingDisposition disposition) const
{
    return static_cast<std::size_t>(std::count(dispositions.begin(), dispositions.end(), disposition));
}

VolumeLabelMappingResult FastSurferLabelMapper::mapMindBoggleDkt31ManualToFastSurfer(
    const std::vector<std::uint16_t> &labels)
{
    VolumeLabelMappingResult result;
    result.labels = labels;
    result.dispositions.assign(labels.size(), MappingDisposition::Exact);
    return result;
}

VolumeLabelMappingResult FastSurferLabelMapper::mapMindBoggleManualAsegToFastSurfer(
    const std::vector<std::uint16_t> &labels,
    const FastSurferLabelMapper::Dimensions &dimensions,
    const LabelMappingOptions &options,
    const std::vector<std::uint16_t> *noCcReference)
{
    return mapRichVolumeToFastSurfer(labels, dimensions, options, noCcReference, false);
}

VolumeLabelMappingResult FastSurferLabelMapper::mapFreeSurferAparcAsegToFastSurfer(
    const std::vector<std::uint16_t> &labels,
    const FastSurferLabelMapper::Dimensions &dimensions,
    const LabelMappingOptions &options,
    const std::vector<std::uint16_t> *noCcReference)
{
    return mapRichVolumeToFastSurfer(labels, dimensions, options, noCcReference, true);
}

std::vector<std::uint8_t> FastSurferLabelMapper::mapBratsToTumorForeground(const std::vector<std::uint8_t> &labels)
{
    std::vector<std::uint8_t> mask(labels.size(), 0U);
    for (std::size_t index = 0; index < labels.size(); ++index) {
        const std::uint8_t label = labels[index];
        if (label > toUnderlying(KnownBratsTumorLabel::Class3)) {
            throw std::invalid_argument("BRATS label mapping currently supports only labels 0 through 3.");
        }

        mask[index] = label == toUnderlying(KnownBratsTumorLabel::Background) ? 0U : 1U;
    }
    return mask;
}

std::vector<std::uint8_t> FastSurferLabelMapper::mapFastSurferToBrainForeground(const std::vector<std::uint16_t> &labels)
{
    std::vector<std::uint8_t> mask(labels.size(), 0U);
    for (std::size_t index = 0; index < labels.size(); ++index) {
        mask[index] = labels[index] == toUnderlying(FastSurferLabels::Background) ? 0U : 1U;
    }
    return mask;
}

std::vector<std::uint16_t> FastSurferLabelMapper::splitCortexLabels(
    const std::vector<std::uint16_t> &labels,
    const FastSurferLabelMapper::Dimensions &dimensions)
{
    validateDimensions(labels, dimensions);

    std::vector<std::uint16_t> split = labels;
    SplitCortexLabelsTransform transform;
    transform.applyInPlace(split, dimensions);
    return split;
}

std::vector<std::uint16_t> FastSurferLabelMapper::convertFloatVoxelsToLabels(const std::vector<float> &voxels)
{
    std::vector<std::uint16_t> labels(voxels.size(), 0U);
    for (std::size_t index = 0; index < voxels.size(); ++index) {
        const float value = voxels[index];
        if (value < 0.0F || value > static_cast<float>(std::numeric_limits<std::uint16_t>::max())) {
            throw std::invalid_argument("Voxel label value is outside the uint16 label range.");
        }

        const float rounded = std::round(value);
        if (std::fabs(rounded - value) > 1.0e-4F) {
            throw std::invalid_argument("Voxel label value is not sufficiently close to an integer label.");
        }

        labels[index] = static_cast<std::uint16_t>(rounded);
    }
    return labels;
}

std::vector<std::uint16_t> FastSurferLabelMapper::uniqueSortedLabels(const std::vector<std::uint16_t> &labels)
{
    std::set<std::uint16_t> unique(labels.begin(), labels.end());
    return std::vector<std::uint16_t>(unique.begin(), unique.end());
}

} // namespace OpenHC::imaging::mri::fastsurfer