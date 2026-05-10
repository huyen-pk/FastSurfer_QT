#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestConstants.h"
#include "TestHelpers.h"
#include "imaging/common/mgh_image.h"
#include "imaging/common/nifti_converter.h"
#include "imaging/mri/fastsurfer/label_mapping.h"
#include "imaging/mri/fastsurfer/label_types.h"

namespace ohc = OpenHC::imaging::mri::fastsurfer;

namespace {

std::vector<std::uint16_t> loadMghLabels(const std::filesystem::path &path)
{
    return ohc::FastSurferLabelMapper::convertFloatVoxelsToLabels(ohc::MghImage::load(path).voxelDataAsFloat());
}

std::vector<std::uint16_t> loadNiftiLabels(const std::filesystem::path &path)
{
    return ohc::FastSurferLabelMapper::convertFloatVoxelsToLabels(ohc::NiftiConverter::loadAsMgh(path).voxelDataAsFloat());
}

bool containsLabel(const std::vector<std::uint16_t> &labels, const std::uint16_t needle)
{
    return std::binary_search(labels.begin(), labels.end(), needle);
}

void map_manual_aseg_should_apply_expected_fastsurfer_rules()
{
    const std::vector<std::uint16_t> labels(
        test_constants::SYNTHETIC_MANUAL_ASEG_LABELS.begin(),
        test_constants::SYNTHETIC_MANUAL_ASEG_LABELS.end());
    const std::vector<std::uint16_t> noCcReference(
        test_constants::SYNTHETIC_MANUAL_ASEG_NO_CC_REFERENCE.begin(),
        test_constants::SYNTHETIC_MANUAL_ASEG_NO_CC_REFERENCE.end());

    const ohc::FastSurferLabelMapper::Dimensions dimensions {15, 1, 1};
    const ohc::VolumeLabelMappingResult mapped =
        ohc::FastSurferLabelMapper::mapMindBoggleManualAsegToFastSurfer(labels, dimensions, {}, &noCcReference);

    require(mapped.labels[0] == ohc::toUnderlying(ohc::FastSurferLabels::LeftCerebralWhiteMatter),
        "Left vessel should reduce to left white matter.");
    require(mapped.labels[1] == ohc::toUnderlying(ohc::FastSurferLabels::RightCerebralWhiteMatter),
        "Right vessel should reduce to right white matter.");
    require(mapped.labels[2] == ohc::toUnderlying(ohc::FastSurferLabels::WMHypointensities),
        "Non-WM hypointensities should reduce to WM hypointensities.");
    require(mapped.labels[3] == ohc::toUnderlying(ohc::FastSurferLabels::Background),
        "Optic chiasm should drop to background.");
    require(mapped.labels[4] == 2 && mapped.labels[5] == 2 && mapped.labels[6] == 41 && mapped.labels[7] == 41,
        "Corpus callosum voxels should be replaced from the supplied no-CC reference.");
    require(mapped.labels[8] == 0,
        "Corpus callosum replacement should preserve a background voxel from the no-CC reference.");
    require(mapped.labels[10] == 1002,
        "Left unknown cortex should resolve to the nearest-parcel candidate using the FastSurfer-style fill rule.");
    require(mapped.labels[13] == 2002,
        "Right unknown cortex should resolve to the nearest-parcel candidate using the FastSurfer-style fill rule.");

    require(mapped.dispositions[4] == ohc::MappingDisposition::Reduced,
        "Corpus callosum replacement should be tracked as a reduced mapping.");
    require(mapped.dispositions[10] == ohc::MappingDisposition::SpatiallyResolved,
        "Left unknown cortex fill should be tracked as a spatially resolved mapping.");
    require(mapped.dispositions[13] == ohc::MappingDisposition::SpatiallyResolved,
        "Right unknown cortex fill should be tracked as a spatially resolved mapping.");

    const ohc::LabelMappingOptions dropOptions {ohc::CorpusCallosumResolutionMode::Drop, ohc::LabelTargetSpace::FastSurferSegmentation};
    const ohc::VolumeLabelMappingResult dropped =
        ohc::FastSurferLabelMapper::mapMindBoggleManualAsegToFastSurfer(labels, dimensions, dropOptions, nullptr);
    require(dropped.labels[4] == 0 && dropped.labels[8] == 0,
        "Drop mode should remove corpus callosum labels from the reduced-space output.");
    require(dropped.dispositions[4] == ohc::MappingDisposition::Dropped,
        "Drop mode should record dropped corpus callosum voxels explicitly.");

    const std::vector<std::uint16_t> aparcExtraLabels(
        test_constants::SYNTHETIC_APARC_UNSUPPORTED_LABELS.begin(),
        test_constants::SYNTHETIC_APARC_UNSUPPORTED_LABELS.end());
    const ohc::VolumeLabelMappingResult aparcMapped =
        ohc::FastSurferLabelMapper::mapFreeSurferAparcAsegToFastSurfer(aparcExtraLabels, {6, 1, 1}, dropOptions, nullptr);
    require(std::all_of(aparcMapped.labels.begin(), aparcMapped.labels.end(), [](const std::uint16_t value) { return value == 0; }),
        "Surface-only aparc labels unsupported by FastSurfer volumetric outputs should drop out of the reduced comparison space.");
    require(aparcMapped.countDisposition(ohc::MappingDisposition::Unsupported) == aparcExtraLabels.size(),
        "Surface-only aparc labels should be tracked as unsupported.");
}

void validate_colin27_manual_dkt31_labels_should_be_supported_by_generated_fastsurfer_output()
{
    const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
    const std::filesystem::path manualPath =
        repoRoot / "data/MindBoggle101_20260501/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/labels.DKT31.manual.nii.gz";
    const std::filesystem::path fastsurferPath =
        repoRoot / "generated/FastSurferCNN/Colin27-1/mri/aparc.DKTatlas+aseg.deep.mgz";

    require(std::filesystem::exists(manualPath), "Colin27 manual DKT31 validation fixture is missing.");
    require(std::filesystem::exists(fastsurferPath), "Generated FastSurfer Colin27 validation fixture is missing.");

    const std::vector<std::uint16_t> manualLabels = loadNiftiLabels(manualPath);
    const std::vector<std::uint16_t> fastsurferLabels = loadMghLabels(fastsurferPath);

    const ohc::VolumeLabelMappingResult mapped =
        ohc::FastSurferLabelMapper::mapMindBoggleDkt31ManualToFastSurfer(manualLabels);
    const std::vector<std::uint16_t> manualUnique = ohc::FastSurferLabelMapper::uniqueSortedLabels(mapped.labels);
    const std::vector<std::uint16_t> fastsurferUnique = ohc::FastSurferLabelMapper::uniqueSortedLabels(fastsurferLabels);
    std::vector<std::uint16_t> expectedFastSurferUnique;
    expectedFastSurferUnique.reserve(ohc::kAllFastSurferLabels.size());
    for (const ohc::FastSurferLabels label : ohc::kAllFastSurferLabels) {
        expectedFastSurferUnique.push_back(ohc::toUnderlying(label));
    }

    require(manualUnique.size() == 63U,
        "The Colin27 manual cortical volume should expose the expected 63 unique DKT31 labels including background.");
    require(mapped.countDisposition(ohc::MappingDisposition::Exact) == mapped.labels.size(),
        "The manual DKT31 mapping should remain an exact identity mapping.");
    require(fastsurferUnique == expectedFastSurferUnique,
        "FastSurferLabels should remain the single source of truth for the 96-label segmentation-stage output ontology.");

    for (const std::uint16_t label : manualUnique) {
        require(containsLabel(fastsurferUnique, label),
            "Every Colin27 manual DKT31 label should be present in the generated FastSurfer segmentation label set.");
    }
}

void map_brats_labels_should_produce_simple_foreground_bridge()
{
    const std::vector<std::uint8_t> bratsLabels(
        test_constants::BRATS_BRIDGE_INPUT_LABELS.begin(),
        test_constants::BRATS_BRIDGE_INPUT_LABELS.end());
    const std::vector<std::uint8_t> tumorMask = ohc::FastSurferLabelMapper::mapBratsToTumorForeground(bratsLabels);
    require(tumorMask == std::vector<std::uint8_t>(
        test_constants::BRATS_BRIDGE_EXPECTED_FOREGROUND.begin(),
        test_constants::BRATS_BRIDGE_EXPECTED_FOREGROUND.end()),
        "The v1 BRATS bridge should expose a simple binary tumor foreground mask.");

    const std::vector<std::uint16_t> fastsurferLabels(
        test_constants::FASTSURFER_BRAIN_FOREGROUND_INPUT_LABELS.begin(),
        test_constants::FASTSURFER_BRAIN_FOREGROUND_INPUT_LABELS.end());
    const std::vector<std::uint8_t> brainMask = ohc::FastSurferLabelMapper::mapFastSurferToBrainForeground(fastsurferLabels);
    require(brainMask == std::vector<std::uint8_t>(
        test_constants::FASTSURFER_BRAIN_FOREGROUND_EXPECTED_MASK.begin(),
        test_constants::FASTSURFER_BRAIN_FOREGROUND_EXPECTED_MASK.end()),
        "FastSurfer brain foreground should remain a simple non-zero mask in the first BRATS bridge implementation.");
}

} // namespace

int main(const int argc, char **argv)
{
    try {
        if (argc != 2) {
            throw std::runtime_error("Expected exactly one test case argument.");
        }

        const std::string testCase = argv[1];
        if (testCase == "synthetic") {
            map_manual_aseg_should_apply_expected_fastsurfer_rules();
            return 0;
        }
        if (testCase == "colin27") {
            validate_colin27_manual_dkt31_labels_should_be_supported_by_generated_fastsurfer_output();
            return 0;
        }
        if (testCase == "brats") {
            map_brats_labels_should_produce_simple_foreground_bridge();
            return 0;
        }

        throw std::runtime_error("Unknown label-mapping test case requested.");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}