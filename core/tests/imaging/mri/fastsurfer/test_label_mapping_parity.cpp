#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestConstants.h"
#include "TestHelpers.h"
#include "TestParitySupport.h"
#include "imaging/common/mgh_image.h"
#include "imaging/common/nifti_converter.h"
#include "imaging/mri/fastsurfer/label_mapping.h"
#include "imaging/mri/fastsurfer/label_types.h"

namespace ohc = OpenHC::imaging::mri::fastsurfer;
namespace oht = OpenHC::tests::fastsurfer::support;

namespace {

struct LoadedLabelVolume {
    std::vector<std::uint16_t> labels;
    ohc::FastSurferLabelMapper::Dimensions dimensions {0, 0, 0};
};

struct PerLabelDice {
    std::uint16_t label {0};
    std::size_t intersection {0};
    std::size_t candidateCount {0};
    std::size_t referenceCount {0};
    double dice {0.0};
};

struct SpatialParityReport {
    std::size_t unknownVoxelCountLeft {0};
    std::size_t unknownVoxelCountRight {0};
    std::size_t unknownVoxelsFilled_exactMatchCount {0};
    double unknownVoxelsFilled_exactMatchFraction {0.0};
    std::map<std::string, std::size_t> confusionOnUnknownMask;
    std::vector<PerLabelDice> perLabelDiceOnUnknownMask;
};

struct CoverageReport {
    std::string sourceName;
    std::size_t rawUniqueLabelCount {0};
    std::size_t canonicalizedUniqueLabelCount {0};
    std::vector<std::uint16_t> rawMissingFromTargetUniqueLabels;
    std::vector<std::uint16_t> canonicalizedMissingFromTargetUniqueLabels;
    std::vector<std::uint16_t> unsupportedUniqueLabels;
    std::size_t unsupportedUniqueLabelCount {0};
    std::size_t unsupportedVoxelCount {0};
    double unsupportedVoxelFraction {0.0};
    std::size_t droppedVoxelCount {0};
    std::size_t spatiallyResolvedVoxelCount {0};
    std::size_t reducedVoxelCount {0};
    std::map<std::string, std::size_t> dispositionHistogram;
};

struct OverlapReport {
    std::vector<std::uint16_t> evaluatedLabelSet;
    std::vector<std::uint16_t> excludedUnsupportedLabels;
    std::vector<PerLabelDice> perLabelDice;
    double macroDice {0.0};
    double microDice {0.0};
    double coverageLossFraction {0.0};
    std::size_t matchedVoxelCount {0};
    std::size_t mismatchedVoxelCount {0};
};

LoadedLabelVolume loadLabelVolume(const std::filesystem::path &path)
{
    const ohc::MghImage image = ohc::NiftiConverter::isSupportedPath(path)
        ? ohc::NiftiConverter::loadAsMgh(path)
        : ohc::MghImage::load(path);

    return {
        ohc::FastSurferLabelMapper::convertFloatVoxelsToLabels(image.voxelDataAsFloat()),
        image.header().dimensions,
    };
}

bool hasAlignedVoxelGrid(const LoadedLabelVolume &left, const LoadedLabelVolume &right)
{
    return left.dimensions == right.dimensions && left.labels.size() == right.labels.size();
}

std::size_t countMismatchedLabels(
    const std::vector<std::uint16_t> &left,
    const std::vector<std::uint16_t> &right)
{
    require(left.size() == right.size(), "Label mismatch counting requires equal-sized volumes.");

    std::size_t mismatches = 0U;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index] != right[index]) {
            ++mismatches;
        }
    }
    return mismatches;
}

std::vector<std::uint16_t> uniqueSorted(const std::vector<std::uint16_t> &labels)
{
    return ohc::FastSurferLabelMapper::uniqueSortedLabels(labels);
}

std::vector<std::uint16_t> ontologyLabels()
{
    std::vector<std::uint16_t> values;
    values.reserve(ohc::kAllFastSurferLabels.size());
    for (const ohc::FastSurferLabels label : ohc::kAllFastSurferLabels) {
        values.push_back(ohc::toUnderlying(label));
    }
    return values;
}

template <typename T>
std::vector<T> subtractSorted(const std::vector<T> &left, const std::vector<T> &right)
{
    std::vector<T> difference;
    std::set_difference(left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(difference));
    return difference;
}

std::string dispositionName(const ohc::MappingDisposition disposition)
{
    switch (disposition) {
    case ohc::MappingDisposition::Exact:
        return "Exact";
    case ohc::MappingDisposition::Reduced:
        return "Reduced";
    case ohc::MappingDisposition::Dropped:
        return "Dropped";
    case ohc::MappingDisposition::SpatiallyResolved:
        return "SpatiallyResolved";
    case ohc::MappingDisposition::Unsupported:
        return "Unsupported";
    case ohc::MappingDisposition::AmbiguousInverse:
        return "AmbiguousInverse";
    }

    return "Unknown";
}

std::string jsonArray(const std::vector<std::uint16_t> &values)
{
    std::ostringstream stream;
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            stream << ',';
        }
        stream << values[index];
    }
    stream << ']';
    return stream.str();
}

void writePerLabelDiceJson(std::ostream &stream, const std::vector<PerLabelDice> &values, const int indent)
{
    const std::string padding(static_cast<std::size_t>(indent), ' ');
    stream << "[\n";
    for (std::size_t index = 0; index < values.size(); ++index) {
        const auto &entry = values[index];
        stream << padding << "  {\"label\":" << entry.label
               << ",\"intersection\":" << entry.intersection
               << ",\"candidate_count\":" << entry.candidateCount
               << ",\"reference_count\":" << entry.referenceCount
               << ",\"dice\":" << std::fixed << std::setprecision(8) << entry.dice << '}';
        if (index + 1 != values.size()) {
            stream << ',';
        }
        stream << '\n';
    }
    stream << padding << ']';
}

void writeHistogramJson(std::ostream &stream, const std::map<std::string, std::size_t> &histogram)
{
    stream << '{';
    bool first = true;
    for (const auto &[key, value] : histogram) {
        if (!first) {
            stream << ',';
        }
        first = false;
        stream << '\"' << key << "\":" << value;
    }
    stream << '}';
}

void writeSpatiallyResolvedParityReport(const std::filesystem::path &path, const SpatialParityReport &report)
{
    std::ofstream stream(path);
    stream << "{\n"
           << "  \"unknown_voxel_count_left\":" << report.unknownVoxelCountLeft << ",\n"
           << "  \"unknown_voxel_count_right\":" << report.unknownVoxelCountRight << ",\n"
           << "  \"unknown_voxels_filled_exact_match_count\":" << report.unknownVoxelsFilled_exactMatchCount << ",\n"
           << "  \"unknown_voxels_filled_exact_match_fraction\":" << std::fixed << std::setprecision(8) << report.unknownVoxelsFilled_exactMatchFraction << ",\n"
           << "  \"native_vs_python_confusion_on_unknown_mask\":";
    writeHistogramJson(stream, report.confusionOnUnknownMask);
    stream << ",\n  \"per_label_dice_on_unknown_mask\":";
    writePerLabelDiceJson(stream, report.perLabelDiceOnUnknownMask, 2);
    stream << "\n}\n";
}

void writeCoverageReport(const std::filesystem::path &path, const CoverageReport &report)
{
    std::ofstream stream(path);
    stream << "{\n"
           << "  \"source_name\":\"" << report.sourceName << "\",\n"
           << "  \"raw_unique_label_count\":" << report.rawUniqueLabelCount << ",\n"
           << "  \"canonicalized_unique_label_count\":" << report.canonicalizedUniqueLabelCount << ",\n"
           << "  \"raw_missing_from_target_unique_labels\":" << jsonArray(report.rawMissingFromTargetUniqueLabels) << ",\n"
           << "  \"canonicalized_missing_from_target_unique_labels\":" << jsonArray(report.canonicalizedMissingFromTargetUniqueLabels) << ",\n"
           << "  \"unsupported_unique_labels\":" << jsonArray(report.unsupportedUniqueLabels) << ",\n"
           << "  \"unsupported_unique_label_count\":" << report.unsupportedUniqueLabelCount << ",\n"
           << "  \"unsupported_voxel_count\":" << report.unsupportedVoxelCount << ",\n"
           << "  \"unsupported_voxel_fraction\":" << std::fixed << std::setprecision(8) << report.unsupportedVoxelFraction << ",\n"
           << "  \"dropped_voxel_count\":" << report.droppedVoxelCount << ",\n"
           << "  \"spatially_resolved_voxel_count\":" << report.spatiallyResolvedVoxelCount << ",\n"
           << "  \"reduced_voxel_count\":" << report.reducedVoxelCount << ",\n"
           << "  \"disposition_histogram\":";
    writeHistogramJson(stream, report.dispositionHistogram);
    stream << "\n}\n";
}

void writeOverlapReport(const std::filesystem::path &path, const OverlapReport &report)
{
    std::ofstream stream(path);
    stream << "{\n"
           << "  \"evaluated_label_set\":" << jsonArray(report.evaluatedLabelSet) << ",\n"
           << "  \"excluded_unsupported_labels\":" << jsonArray(report.excludedUnsupportedLabels) << ",\n"
           << "  \"per_label_dice\":";
    writePerLabelDiceJson(stream, report.perLabelDice, 2);
    stream << ",\n  \"macro_dice\":" << std::fixed << std::setprecision(8) << report.macroDice << ",\n"
           << "  \"micro_dice\":" << std::fixed << std::setprecision(8) << report.microDice << ",\n"
           << "  \"coverage_loss_fraction\":" << std::fixed << std::setprecision(8) << report.coverageLossFraction << ",\n"
           << "  \"voxel_confusion_summary\":{\"matched\":" << report.matchedVoxelCount
           << ",\"mismatched\":" << report.mismatchedVoxelCount << "}\n"
           << "}\n";
}

SpatialParityReport computeSpatiallyResolvedParityReport(
    const std::vector<std::uint16_t> &rawLabels,
    const std::vector<std::uint16_t> &nativeLabels,
    const std::vector<std::uint16_t> &oracleLabels)
{
    require(rawLabels.size() == nativeLabels.size() && nativeLabels.size() == oracleLabels.size(),
            "Spatial parity inputs must all have the same size.");

    SpatialParityReport report;
    std::map<std::uint16_t, std::size_t> nativeCounts;
    std::map<std::uint16_t, std::size_t> oracleCounts;
    std::map<std::uint16_t, std::size_t> intersections;

    for (std::size_t index = 0; index < rawLabels.size(); ++index) {
        const std::uint16_t raw = rawLabels[index];
        if (raw != ohc::toUnderlying(ohc::FastSurferAuxiliaryLabel::UnknownLeftCortex) &&
            raw != ohc::toUnderlying(ohc::FastSurferAuxiliaryLabel::UnknownRightCortex)) {
            continue;
        }

        if (raw == ohc::toUnderlying(ohc::FastSurferAuxiliaryLabel::UnknownLeftCortex)) {
            ++report.unknownVoxelCountLeft;
            require(nativeLabels[index] > 1000U && nativeLabels[index] < 2000U,
                "Left unknown cortex must resolve to a left-hemisphere cortical label.");
        } else {
            ++report.unknownVoxelCountRight;
            require(nativeLabels[index] > 2000U && nativeLabels[index] < 3000U,
                "Right unknown cortex must resolve to a right-hemisphere cortical label.");
        }

        if (nativeLabels[index] == oracleLabels[index]) {
            ++report.unknownVoxelsFilled_exactMatchCount;
        }
        ++nativeCounts[nativeLabels[index]];
        ++oracleCounts[oracleLabels[index]];
        if (nativeLabels[index] == oracleLabels[index]) {
            ++intersections[nativeLabels[index]];
        }

        const std::string confusionKey = std::to_string(oracleLabels[index]) + "->" + std::to_string(nativeLabels[index]);
        ++report.confusionOnUnknownMask[confusionKey];
    }

    const std::size_t totalUnknownVoxels = report.unknownVoxelCountLeft + report.unknownVoxelCountRight;
    require(totalUnknownVoxels > 0U, "The Colin27 manual+aseg fixture should contain unknown cortex voxels.");
    report.unknownVoxelsFilled_exactMatchFraction = static_cast<double>(report.unknownVoxelsFilled_exactMatchCount) / static_cast<double>(totalUnknownVoxels);

    std::set<std::uint16_t> diceLabels;
    for (const auto &[label, _] : nativeCounts) {
        diceLabels.insert(label);
    }
    for (const auto &[label, _] : oracleCounts) {
        diceLabels.insert(label);
    }

    for (const std::uint16_t label : diceLabels) {
        const std::size_t nativeCount = nativeCounts[label];
        const std::size_t oracleCount = oracleCounts[label];
        const std::size_t intersection = intersections[label];
        const double dice = nativeCount + oracleCount == 0U
            ? 1.0
            : (2.0 * static_cast<double>(intersection)) /
                  static_cast<double>(nativeCount + oracleCount);
        report.perLabelDiceOnUnknownMask.push_back({label, intersection, nativeCount, oracleCount, dice});
    }

    return report;
}

CoverageReport computeCoverageReport(
    const std::string &sourceName,
    const std::vector<std::uint16_t> &rawLabels,
    const ohc::VolumeLabelMappingResult &mapped)
{
    require(rawLabels.size() == mapped.labels.size() && rawLabels.size() == mapped.dispositions.size(),
        "Coverage reporting requires source, mapped labels, and dispositions to have the same size.");

    CoverageReport report;
    report.sourceName = sourceName;
    report.rawUniqueLabelCount = uniqueSorted(rawLabels).size();
    report.canonicalizedUniqueLabelCount = uniqueSorted(mapped.labels).size();

    const std::vector<std::uint16_t> ontology = ontologyLabels();
    report.rawMissingFromTargetUniqueLabels = subtractSorted(uniqueSorted(rawLabels), ontology);
    report.canonicalizedMissingFromTargetUniqueLabels = subtractSorted(uniqueSorted(mapped.labels), ontology);

    std::vector<std::uint16_t> unsupportedRawLabels;
    for (std::size_t index = 0; index < mapped.dispositions.size(); ++index) {
        const ohc::MappingDisposition disposition = mapped.dispositions[index];
        ++report.dispositionHistogram[dispositionName(disposition)];

        if (disposition == ohc::MappingDisposition::Unsupported) {
            ++report.unsupportedVoxelCount;
            unsupportedRawLabels.push_back(rawLabels[index]);
        } else if (disposition == ohc::MappingDisposition::Dropped) {
            ++report.droppedVoxelCount;
        } else if (disposition == ohc::MappingDisposition::SpatiallyResolved) {
            ++report.spatiallyResolvedVoxelCount;
        } else if (disposition == ohc::MappingDisposition::Reduced) {
            ++report.reducedVoxelCount;
        }
    }

    report.unsupportedUniqueLabels = uniqueSorted(unsupportedRawLabels);
    report.unsupportedUniqueLabelCount = report.unsupportedUniqueLabels.size();
    report.unsupportedVoxelFraction = rawLabels.empty()
        ? 0.0
        : static_cast<double>(report.unsupportedVoxelCount) / static_cast<double>(rawLabels.size());
    return report;
}

OverlapReport computeOverlapReport(
    const std::vector<std::uint16_t> &rawLabels,
    const ohc::VolumeLabelMappingResult &mapped,
    const std::vector<std::uint16_t> &targetLabels)
{
    require(mapped.labels.size() == targetLabels.size() && mapped.labels.size() == rawLabels.size(),
        "Overlap reporting requires aligned source and target volumes.");

    OverlapReport report;
    std::vector<std::uint16_t> unsupportedRawLabels;
    std::map<std::uint16_t, std::size_t> mappedCounts;
    std::map<std::uint16_t, std::size_t> targetCounts;
    std::map<std::uint16_t, std::size_t> intersections;
    std::size_t totalIntersection = 0U;
    std::size_t totalMapped = 0U;
    std::size_t totalTarget = 0U;

    for (std::size_t index = 0; index < mapped.labels.size(); ++index) {
        if (mapped.dispositions[index] == ohc::MappingDisposition::Unsupported) {
            unsupportedRawLabels.push_back(rawLabels[index]);
            continue;
        }

        const std::uint16_t mappedLabel = mapped.labels[index];
        const std::uint16_t targetLabel = targetLabels[index];
        if (mappedLabel == targetLabel) {
            ++report.matchedVoxelCount;
        } else {
            ++report.mismatchedVoxelCount;
        }

        if (mappedLabel != ohc::toUnderlying(ohc::FastSurferLabels::Background) && ohc::isFastSurferSegmentationLabel(mappedLabel)) {
            ++mappedCounts[mappedLabel];
            ++totalMapped;
        }
        if (targetLabel != ohc::toUnderlying(ohc::FastSurferLabels::Background) && ohc::isFastSurferSegmentationLabel(targetLabel)) {
            ++targetCounts[targetLabel];
            ++totalTarget;
        }
        if (mappedLabel == targetLabel && mappedLabel != ohc::toUnderlying(ohc::FastSurferLabels::Background) &&
            ohc::isFastSurferSegmentationLabel(mappedLabel)) {
            ++intersections[mappedLabel];
            ++totalIntersection;
        }
    }

    report.excludedUnsupportedLabels = uniqueSorted(unsupportedRawLabels);
    report.coverageLossFraction = mapped.labels.empty()
        ? 0.0
        : static_cast<double>(unsupportedRawLabels.size()) / static_cast<double>(mapped.labels.size());

    std::set<std::uint16_t> labelsToEvaluate;
    for (const auto &[label, _] : mappedCounts) {
        labelsToEvaluate.insert(label);
    }
    for (const auto &[label, _] : targetCounts) {
        labelsToEvaluate.insert(label);
    }
    report.evaluatedLabelSet.assign(labelsToEvaluate.begin(), labelsToEvaluate.end());

    double macroSum = 0.0;
    for (const std::uint16_t label : report.evaluatedLabelSet) {
        const std::size_t mappedCount = mappedCounts[label];
        const std::size_t targetCount = targetCounts[label];
        const std::size_t intersection = intersections[label];
        const double dice = mappedCount + targetCount == 0U
            ? 1.0
            : (2.0 * static_cast<double>(intersection)) / static_cast<double>(mappedCount + targetCount);
        macroSum += dice;
        report.perLabelDice.push_back({label, intersection, mappedCount, targetCount, dice});
    }

    report.macroDice = report.perLabelDice.empty() ? 1.0 : macroSum / static_cast<double>(report.perLabelDice.size());
    report.microDice = totalMapped + totalTarget == 0U
        ? 1.0
        : (2.0 * static_cast<double>(totalIntersection)) / static_cast<double>(totalMapped + totalTarget);
    return report;
}

void runPythonOracle(
    const std::filesystem::path &repoRoot,
    const std::filesystem::path &inputPath,
    const std::filesystem::path &noCcPath,
    const std::filesystem::path &oracleOutputPath,
    const std::filesystem::path &oracleReportPath)
{
    const std::filesystem::path pythonExecutable = oht::resolvePythonExecutable(repoRoot);
    const std::filesystem::path pythonScript =
        repoRoot / "core/tests/imaging/mri/fastsurfer/python_oracles/label_mapping_oracle.py";

    require(std::filesystem::exists(pythonScript), "The checked-in label-mapping Python oracle script is missing.");

    std::string command =
        oht::shellEscape(pythonExecutable) + " " + oht::shellEscape(pythonScript) +
        " " + oht::shellEscape(repoRoot) +
        " " + oht::shellEscape(inputPath) +
        " " + oht::shellEscape(oracleOutputPath) +
        " " + oht::shellEscape(oracleReportPath);
    if (!noCcPath.empty()) {
        command += " " + oht::shellEscape(noCcPath);
    }

    const int exitCode = oht::runCommand(command);
    require(exitCode == 0,
        "The Python label-mapping oracle failed. Set FASTSURFER_PYTHON_EXECUTABLE or create .venv/.venv-parity with numpy and nibabel.");
    require(std::filesystem::exists(oracleOutputPath), "The Python oracle did not create the output label volume.");
    require(std::filesystem::exists(oracleReportPath), "The Python oracle did not create the spatial report JSON.");
}

void fill_unknown_cortex_should_match_python_reference_on_colin27_manual_aseg_fixture()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const std::filesystem::path fixtureRoot =
        repoRoot / "data/MindBoggle101_20260501/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri";

    const LoadedLabelVolume manualAseg = loadLabelVolume(fixtureRoot / "labels.DKT31.manual+aseg.nii.gz");
    const ohc::LabelMappingOptions dropOptions {
        ohc::CorpusCallosumResolutionMode::Drop,
        ohc::LabelTargetSpace::FastSurferSegmentation,
    };
    const ohc::VolumeLabelMappingResult nativeMapped =
        ohc::FastSurferLabelMapper::mapMindBoggleManualAsegToFastSurfer(
            manualAseg.labels,
            manualAseg.dimensions,
            dropOptions,
            nullptr);

    const auto tempDir = oht::makeFreshDirectory("openhc_imaging_mri_fastsurfer_label_mapping_spatial_colin27");
    const auto oracleOutputPath = tempDir / "oracle_manual_aseg_filled.nii.gz";
    const auto oracleReportPath = tempDir / "oracle_manual_aseg_filled_report.json";
    const auto nativeReportPath = tempDir / "label_mapping_spatial_parity_colin27.json";

    runPythonOracle(
        repoRoot,
        fixtureRoot / "labels.DKT31.manual+aseg.nii.gz",
        std::filesystem::path(),
        oracleOutputPath,
        oracleReportPath);

    const LoadedLabelVolume oracleFilled = loadLabelVolume(oracleOutputPath);
    require(oracleFilled.dimensions == manualAseg.dimensions,
        "The Python oracle output dimensions must match the Colin27 manual+aseg fixture.");
    require(oracleFilled.labels.size() == nativeMapped.labels.size(),
        "The Python oracle output size must match the native mapping output.");
    require(std::count(nativeMapped.labels.begin(), nativeMapped.labels.end(),
                ohc::toUnderlying(ohc::FastSurferAuxiliaryLabel::UnknownLeftCortex)) == 0,
        "Native canonicalization should eliminate all left unknown cortex voxels.");
    require(std::count(nativeMapped.labels.begin(), nativeMapped.labels.end(),
                ohc::toUnderlying(ohc::FastSurferAuxiliaryLabel::UnknownRightCortex)) == 0,
        "Native canonicalization should eliminate all right unknown cortex voxels.");
    require(std::count(oracleFilled.labels.begin(), oracleFilled.labels.end(),
                ohc::toUnderlying(ohc::FastSurferAuxiliaryLabel::UnknownLeftCortex)) == 0,
        "The Python oracle should eliminate all left unknown cortex voxels.");
    require(std::count(oracleFilled.labels.begin(), oracleFilled.labels.end(),
                ohc::toUnderlying(ohc::FastSurferAuxiliaryLabel::UnknownRightCortex)) == 0,
        "The Python oracle should eliminate all right unknown cortex voxels.");

    const SpatialParityReport report =
        computeSpatiallyResolvedParityReport(manualAseg.labels, nativeMapped.labels, oracleFilled.labels);

    require(report.unknownVoxelsFilled_exactMatchCount == report.unknownVoxelCountLeft + report.unknownVoxelCountRight,
        "Native unknown-cortex fill should match the Python reference exactly on the original unknown mask.");

    writeSpatiallyResolvedParityReport(nativeReportPath, report);
    require(std::filesystem::exists(nativeReportPath), "The spatially resolved parity report JSON was not written.");
}

void split_cortex_stage_should_expose_expected_79_to_96_label_transition_on_generated_colin27_artifacts()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const std::filesystem::path generatedRoot = repoRoot / "generated/FastSurferCNN/Colin27-1";

    const LoadedLabelVolume step6 = loadLabelVolume(
        generatedRoot / "step_06_map_internal_class_indices_to_freesurfer_label_ids/pred_classes_freesurfer_ids.mgz");
    const LoadedLabelVolume step7 = loadLabelVolume(
        generatedRoot / "step_07_split_cortical_labels_into_left_and_right_dkt_cortical_labels/pred_classes_split_cortex.mgz");
    require(hasAlignedVoxelGrid(step6, step7),
        "The generated Colin27 split-cortex fixtures must share the same voxel grid.");

    const std::vector<std::uint16_t> nativeSplit =
        ohc::FastSurferLabelMapper::splitCortexLabels(step6.labels, step6.dimensions);

    const std::vector<std::uint16_t> step6Unique = uniqueSorted(step6.labels);
    const std::vector<std::uint16_t> step7Unique = uniqueSorted(step7.labels);
    const std::vector<std::uint16_t> nativeSplitUnique = uniqueSorted(nativeSplit);
    const std::vector<std::uint16_t> introduced = subtractSorted(nativeSplitUnique, step6Unique);
    const std::vector<std::uint16_t> expectedIntroduced(
        test_constants::COLIN27_SPLIT_STAGE_EXPECTED_INTRODUCED.begin(),
        test_constants::COLIN27_SPLIT_STAGE_EXPECTED_INTRODUCED.end());
    const std::size_t changedVoxelCount = countMismatchedLabels(step6.labels, nativeSplit);
    const std::size_t mismatchCount = countMismatchedLabels(nativeSplit, step7.labels);

    require(step6Unique.size() == test_constants::COLIN27_STEP6_UNIQUE_LABEL_COUNT,
        "The generated Colin27 pre-split artifact should remain in the 79-label inference ID space.");
    require(step7Unique.size() == test_constants::COLIN27_STEP7_UNIQUE_LABEL_COUNT,
        "The generated Colin27 post-split artifact should remain in the 96-label segmentation output space.");
    require(changedVoxelCount > 0U,
        "Native split-cortex relabeling should modify at least one voxel from the pre-split Colin27 artifact.");
    require(nativeSplitUnique == step7Unique,
        "Native split-cortex relabeling should reproduce the post-split Colin27 label set exactly.");
    require(introduced == expectedIntroduced,
        "Native split-cortex relabeling should introduce only the expected right-hemisphere cortical labels.");
    require(mismatchCount == 0U,
        "Native split-cortex relabeling should reproduce the generated Colin27 post-split artifact voxel-for-voxel.");
}

void evaluate_colin27_manual_dkt31_should_report_exact_shared_label_coverage()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const std::filesystem::path fixtureRoot =
        repoRoot / "data/MindBoggle101_20260501/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri";
    const LoadedLabelVolume manualDkt31 = loadLabelVolume(fixtureRoot / "labels.DKT31.manual.nii.gz");
    const LoadedLabelVolume fastSurferDeep =
        loadLabelVolume(repoRoot / "generated/FastSurferCNN/Colin27-1/mri/aparc.DKTatlas+aseg.deep.mgz");

    const ohc::VolumeLabelMappingResult mapped =
        ohc::FastSurferLabelMapper::mapMindBoggleDkt31ManualToFastSurfer(manualDkt31.labels);
    const CoverageReport coverage = computeCoverageReport("manual_dkt31", manualDkt31.labels, mapped);

    const auto tempDir = oht::makeFreshDirectory("openhc_imaging_mri_fastsurfer_label_mapping_coverage_manual_dkt31");
    writeCoverageReport(tempDir / "label_mapping_missing_label_report_colin27_manual_dkt31.json", coverage);
    if (hasAlignedVoxelGrid(manualDkt31, fastSurferDeep)) {
        const OverlapReport overlap = computeOverlapReport(manualDkt31.labels, mapped, fastSurferDeep.labels);
        writeOverlapReport(tempDir / "label_mapping_overlap_report_colin27_manual_dkt31.json", overlap);
    }

    require(coverage.rawUniqueLabelCount == test_constants::COLIN27_MANUAL_DKT31_RAW_UNIQUE_LABEL_COUNT,
        "The Colin27 manual DKT31 fixture should expose 63 unique labels including background.");
    require(coverage.rawMissingFromTargetUniqueLabels.empty(),
        "The Colin27 manual DKT31 fixture should be fully contained in the FastSurfer segmentation ontology.");
    require(coverage.canonicalizedMissingFromTargetUniqueLabels.empty(),
        "Manual DKT31 identity mapping should not create labels outside the FastSurfer ontology.");
    require(coverage.unsupportedUniqueLabelCount == 0U,
        "Manual DKT31 identity mapping should not mark any labels unsupported.");
    require(mapped.countDisposition(ohc::MappingDisposition::Exact) == mapped.labels.size(),
        "Manual DKT31 evaluation should remain an exact forward mapping.");
}

void evaluate_colin27_manual_aseg_should_report_expected_raw_missing_labels_and_zero_post_canonicalization_unsupported_labels()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const std::filesystem::path fixtureRoot =
        repoRoot / "data/MindBoggle101_20260501/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri";
    const LoadedLabelVolume manualAseg = loadLabelVolume(fixtureRoot / "labels.DKT31.manual+aseg.nii.gz");
    const LoadedLabelVolume fastSurferDeep =
        loadLabelVolume(repoRoot / "generated/FastSurferCNN/Colin27-1/mri/aparc.DKTatlas+aseg.deep.mgz");

    const ohc::LabelMappingOptions dropOptions {
        ohc::CorpusCallosumResolutionMode::Drop,
        ohc::LabelTargetSpace::FastSurferSegmentation,
    };
    const ohc::VolumeLabelMappingResult mapped =
        ohc::FastSurferLabelMapper::mapMindBoggleManualAsegToFastSurfer(
            manualAseg.labels,
            manualAseg.dimensions,
            dropOptions,
            nullptr);
    const CoverageReport coverage = computeCoverageReport("manual_aseg", manualAseg.labels, mapped);

    const auto tempDir = oht::makeFreshDirectory("openhc_imaging_mri_fastsurfer_label_mapping_coverage_manual_aseg");
    writeCoverageReport(tempDir / "label_mapping_missing_label_report_colin27_manual_aseg.json", coverage);
    if (hasAlignedVoxelGrid(manualAseg, fastSurferDeep)) {
        const OverlapReport overlap = computeOverlapReport(manualAseg.labels, mapped, fastSurferDeep.labels);
        writeOverlapReport(tempDir / "label_mapping_overlap_report_colin27_manual_aseg.json", overlap);
    }

    const std::vector<std::uint16_t> expectedRawMissing(
        test_constants::COLIN27_MANUAL_ASEG_EXPECTED_RAW_MISSING.begin(),
        test_constants::COLIN27_MANUAL_ASEG_EXPECTED_RAW_MISSING.end());

    require(coverage.rawUniqueLabelCount == test_constants::COLIN27_MANUAL_ASEG_RAW_UNIQUE_LABEL_COUNT,
        "The Colin27 manual+aseg fixture should expose 107 unique labels.");
    require(coverage.rawMissingFromTargetUniqueLabels == expectedRawMissing,
        "The Colin27 manual+aseg raw missing-label set should match the known FastSurfer reductions and unknown placeholders.");
    require(coverage.unsupportedUniqueLabelCount == 0U,
        "Manual+aseg canonicalization should not leave unsupported labels once no-CC substitution and unknown fill are applied.");
    require(coverage.canonicalizedMissingFromTargetUniqueLabels.empty(),
        "Manual+aseg canonicalization should end inside the FastSurfer segmentation ontology.");
    require(std::all_of(mapped.labels.begin(), mapped.labels.end(), [](const std::uint16_t value) {
        return ohc::isFastSurferSegmentationLabel(value);
    }),
        "Manual+aseg canonicalization should not leave auxiliary labels in the output volume.");
}

void evaluate_colin27_aparc_aseg_should_report_expected_surface_only_unsupported_labels_after_canonicalization()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const std::filesystem::path fixtureRoot =
        repoRoot / "data/MindBoggle101_20260501/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri";
    const LoadedLabelVolume aparcAseg = loadLabelVolume(fixtureRoot / "aparc+aseg.nii.gz");
    const LoadedLabelVolume fastSurferDeep =
        loadLabelVolume(repoRoot / "generated/FastSurferCNN/Colin27-1/mri/aparc.DKTatlas+aseg.deep.mgz");

    const ohc::LabelMappingOptions dropOptions {
        ohc::CorpusCallosumResolutionMode::Drop,
        ohc::LabelTargetSpace::FastSurferSegmentation,
    };
    const ohc::VolumeLabelMappingResult mapped =
        ohc::FastSurferLabelMapper::mapFreeSurferAparcAsegToFastSurfer(
            aparcAseg.labels,
            aparcAseg.dimensions,
            dropOptions,
            nullptr);
    const CoverageReport coverage = computeCoverageReport("aparc_aseg", aparcAseg.labels, mapped);

    const auto tempDir = oht::makeFreshDirectory("openhc_imaging_mri_fastsurfer_label_mapping_coverage_aparc_aseg");
    writeCoverageReport(tempDir / "label_mapping_missing_label_report_colin27_aparc_aseg.json", coverage);
    OverlapReport overlap;
    if (hasAlignedVoxelGrid(aparcAseg, fastSurferDeep)) {
        overlap = computeOverlapReport(aparcAseg.labels, mapped, fastSurferDeep.labels);
        writeOverlapReport(tempDir / "label_mapping_overlap_report_colin27_aparc_aseg.json", overlap);
    }

    const std::vector<std::uint16_t> expectedRawMissing(
        test_constants::COLIN27_APARC_ASEG_EXPECTED_RAW_MISSING.begin(),
        test_constants::COLIN27_APARC_ASEG_EXPECTED_RAW_MISSING.end());
    const std::vector<std::uint16_t> expectedUnsupported(
        test_constants::COLIN27_APARC_ASEG_EXPECTED_UNSUPPORTED.begin(),
        test_constants::COLIN27_APARC_ASEG_EXPECTED_UNSUPPORTED.end());

    require(coverage.rawUniqueLabelCount == test_constants::COLIN27_APARC_ASEG_RAW_UNIQUE_LABEL_COUNT,
        "The Colin27 aparc+aseg fixture should expose 113 unique labels.");
    require(coverage.rawMissingFromTargetUniqueLabels == expectedRawMissing,
        "The Colin27 aparc+aseg raw missing-label set should match the known FastSurfer reductions plus the six surface-only parcels.");
    require(coverage.unsupportedUniqueLabels == expectedUnsupported,
        "Aparc+aseg canonicalization should retain only the six known surface-only labels as unsupported for FastSurfer volumetric comparison.");
    require(coverage.canonicalizedMissingFromTargetUniqueLabels.empty(),
        "Aparc+aseg canonicalization should drop unsupported surface-only labels out of the reduced FastSurfer comparison space.");
    if (hasAlignedVoxelGrid(aparcAseg, fastSurferDeep)) {
        require(overlap.excludedUnsupportedLabels == expectedUnsupported,
            "The overlap report should exclude exactly the six surface-only unsupported labels from Dice denominators.");
    }
}

void runNamedCase(const std::string &caseName)
{
    if (caseName == "spatial-colin27") {
        fill_unknown_cortex_should_match_python_reference_on_colin27_manual_aseg_fixture();
        return;
    }
    if (caseName == "split-stage-colin27") {
        split_cortex_stage_should_expose_expected_79_to_96_label_transition_on_generated_colin27_artifacts();
        return;
    }
    if (caseName == "coverage-manual-dkt31") {
        evaluate_colin27_manual_dkt31_should_report_exact_shared_label_coverage();
        return;
    }
    if (caseName == "coverage-manual-aseg") {
        evaluate_colin27_manual_aseg_should_report_expected_raw_missing_labels_and_zero_post_canonicalization_unsupported_labels();
        return;
    }
    if (caseName == "coverage-aparc-aseg") {
        evaluate_colin27_aparc_aseg_should_report_expected_surface_only_unsupported_labels_after_canonicalization();
        return;
    }

    throw std::runtime_error("Unknown label-mapping parity test case requested.");
}

} // namespace

int main(const int argc, char **argv)
{
    try {
        if (argc != 2) {
            throw std::runtime_error("Expected exactly one test case argument.");
        }

        runNamedCase(argv[1]);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}