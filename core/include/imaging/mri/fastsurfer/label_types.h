#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace OpenHC::imaging::mri::fastsurfer {

template <typename Tag, typename Storage>
class TypedLabel {
public:
    using StorageType = Storage;

    constexpr TypedLabel() = default;
    explicit constexpr TypedLabel(const Storage value) : m_value(value) {}

    constexpr Storage value() const { return m_value; }

    friend constexpr bool operator==(const TypedLabel &left, const TypedLabel &right)
    {
        return left.m_value == right.m_value;
    }

    friend constexpr bool operator!=(const TypedLabel &left, const TypedLabel &right)
    {
        return !(left == right);
    }
    friend constexpr bool operator<(const TypedLabel &left, const TypedLabel &right)
    {
        return left.m_value < right.m_value;
    }

private:
    Storage m_value {};
};

struct FastSurferLabelTag;
struct MindBoggleDkt31LabelTag;
struct MindBoggleManualAsegLabelTag;
struct BratsTumorLabelTag;

using FastSurferLabel = TypedLabel<FastSurferLabelTag, std::uint16_t>;
using MindBoggleDkt31Label = TypedLabel<MindBoggleDkt31LabelTag, std::uint16_t>;
using MindBoggleManualAsegLabel = TypedLabel<MindBoggleManualAsegLabelTag, std::uint16_t>;
using BratsTumorLabel = TypedLabel<BratsTumorLabelTag, std::uint8_t>;

enum class FastSurferLabels : std::uint16_t {
    Background = 0,
    LeftCerebralWhiteMatter = 2,
    LeftLateralVentricle = 4,
    LeftInferiorLateralVentricle = 5,
    LeftCerebellumWhiteMatter = 7,
    LeftCerebellumCortex = 8,
    LeftThalamus = 10,
    LeftCaudate = 11,
    LeftPutamen = 12,
    LeftPallidum = 13,
    ThirdVentricle = 14,
    FourthVentricle = 15,
    BrainStem = 16,
    LeftHippocampus = 17,
    LeftAmygdala = 18,
    CSF = 24,
    LeftAccumbensArea = 26,
    LeftVentralDC = 28,
    LeftChoroidPlexus = 31,
    RightCerebralWhiteMatter = 41,
    RightLateralVentricle = 43,
    RightInferiorLateralVentricle = 44,
    RightCerebellumWhiteMatter = 46,
    RightCerebellumCortex = 47,
    RightThalamus = 49,
    RightCaudate = 50,
    RightPutamen = 51,
    RightPallidum = 52,
    RightHippocampus = 53,
    RightAmygdala = 54,
    RightAccumbensArea = 58,
    RightVentralDC = 60,
    RightChoroidPlexus = 63,
    WMHypointensities = 77,

    LeftCaudalAnteriorCingulate = 1002,
    LeftCaudalMiddleFrontal = 1003,
    LeftCuneus = 1005,
    LeftEntorhinal = 1006,
    LeftFusiform = 1007,
    LeftInferiorParietal = 1008,
    LeftInferiorTemporal = 1009,
    LeftIsthmusCingulate = 1010,
    LeftLateralOccipital = 1011,
    LeftLateralOrbitofrontal = 1012,
    LeftLingual = 1013,
    LeftMedialOrbitofrontal = 1014,
    LeftMiddleTemporal = 1015,
    LeftParahippocampal = 1016,
    LeftParacentral = 1017,
    LeftParsOpercularis = 1018,
    LeftParsOrbitalis = 1019,
    LeftParsTriangularis = 1020,
    LeftPericalcarine = 1021,
    LeftPostcentral = 1022,
    LeftPosteriorCingulate = 1023,
    LeftPrecentral = 1024,
    LeftPrecuneus = 1025,
    LeftRostralAnteriorCingulate = 1026,
    LeftRostralMiddleFrontal = 1027,
    LeftSuperiorFrontal = 1028,
    LeftSuperiorParietal = 1029,
    LeftSuperiorTemporal = 1030,
    LeftSupramarginal = 1031,
    LeftTransverseTemporal = 1034,
    LeftInsula = 1035,

    RightCaudalAnteriorCingulate = 2002,
    RightCaudalMiddleFrontal = 2003,
    RightCuneus = 2005,
    RightEntorhinal = 2006,
    RightFusiform = 2007,
    RightInferiorParietal = 2008,
    RightInferiorTemporal = 2009,
    RightIsthmusCingulate = 2010,
    RightLateralOccipital = 2011,
    RightLateralOrbitofrontal = 2012,
    RightLingual = 2013,
    RightMedialOrbitofrontal = 2014,
    RightMiddleTemporal = 2015,
    RightParahippocampal = 2016,
    RightParacentral = 2017,
    RightParsOpercularis = 2018,
    RightParsOrbitalis = 2019,
    RightParsTriangularis = 2020,
    RightPericalcarine = 2021,
    RightPostcentral = 2022,
    RightPosteriorCingulate = 2023,
    RightPrecentral = 2024,
    RightPrecuneus = 2025,
    RightRostralAnteriorCingulate = 2026,
    RightRostralMiddleFrontal = 2027,
    RightSuperiorFrontal = 2028,
    RightSuperiorParietal = 2029,
    RightSuperiorTemporal = 2030,
    RightSupramarginal = 2031,
    RightTransverseTemporal = 2034,
    RightInsula = 2035,
};

enum class FastSurferAuxiliaryLabel : std::uint16_t {
    LeftVessel = 30,
    RightVessel = 62,
    FifthVentricle = 72,
    NonWmHypointensities = 80,
    OpticChiasm = 85,
    CorpusCallosumPosterior = 251,
    CorpusCallosumMidPosterior = 252,
    CorpusCallosumCentral = 253,
    CorpusCallosumMidAnterior = 254,
    CorpusCallosumAnterior = 255,
    UnknownLeftCortex = 1000,
    LeftBankssts = 1001,
    LeftFrontalPole = 1032,
    LeftTemporalPole = 1033,
    UnknownRightCortex = 2000,
    RightBankssts = 2001,
    RightFrontalPole = 2032,
    RightTemporalPole = 2033,
};

enum class KnownBratsTumorLabel : std::uint8_t {
    Background = 0,
    Class1 = 1,
    Class2 = 2,
    Class3 = 3,
};

template <typename Enum>
constexpr auto toUnderlying(const Enum value) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(value);
}

constexpr std::array<FastSurferLabels, 96> kAllFastSurferLabels {
    FastSurferLabels::Background,
    FastSurferLabels::LeftCerebralWhiteMatter,
    FastSurferLabels::LeftLateralVentricle,
    FastSurferLabels::LeftInferiorLateralVentricle,
    FastSurferLabels::LeftCerebellumWhiteMatter,
    FastSurferLabels::LeftCerebellumCortex,
    FastSurferLabels::LeftThalamus,
    FastSurferLabels::LeftCaudate,
    FastSurferLabels::LeftPutamen,
    FastSurferLabels::LeftPallidum,
    FastSurferLabels::ThirdVentricle,
    FastSurferLabels::FourthVentricle,
    FastSurferLabels::BrainStem,
    FastSurferLabels::LeftHippocampus,
    FastSurferLabels::LeftAmygdala,
    FastSurferLabels::CSF,
    FastSurferLabels::LeftAccumbensArea,
    FastSurferLabels::LeftVentralDC,
    FastSurferLabels::LeftChoroidPlexus,
    FastSurferLabels::RightCerebralWhiteMatter,
    FastSurferLabels::RightLateralVentricle,
    FastSurferLabels::RightInferiorLateralVentricle,
    FastSurferLabels::RightCerebellumWhiteMatter,
    FastSurferLabels::RightCerebellumCortex,
    FastSurferLabels::RightThalamus,
    FastSurferLabels::RightCaudate,
    FastSurferLabels::RightPutamen,
    FastSurferLabels::RightPallidum,
    FastSurferLabels::RightHippocampus,
    FastSurferLabels::RightAmygdala,
    FastSurferLabels::RightAccumbensArea,
    FastSurferLabels::RightVentralDC,
    FastSurferLabels::RightChoroidPlexus,
    FastSurferLabels::WMHypointensities,
    FastSurferLabels::LeftCaudalAnteriorCingulate,
    FastSurferLabels::LeftCaudalMiddleFrontal,
    FastSurferLabels::LeftCuneus,
    FastSurferLabels::LeftEntorhinal,
    FastSurferLabels::LeftFusiform,
    FastSurferLabels::LeftInferiorParietal,
    FastSurferLabels::LeftInferiorTemporal,
    FastSurferLabels::LeftIsthmusCingulate,
    FastSurferLabels::LeftLateralOccipital,
    FastSurferLabels::LeftLateralOrbitofrontal,
    FastSurferLabels::LeftLingual,
    FastSurferLabels::LeftMedialOrbitofrontal,
    FastSurferLabels::LeftMiddleTemporal,
    FastSurferLabels::LeftParahippocampal,
    FastSurferLabels::LeftParacentral,
    FastSurferLabels::LeftParsOpercularis,
    FastSurferLabels::LeftParsOrbitalis,
    FastSurferLabels::LeftParsTriangularis,
    FastSurferLabels::LeftPericalcarine,
    FastSurferLabels::LeftPostcentral,
    FastSurferLabels::LeftPosteriorCingulate,
    FastSurferLabels::LeftPrecentral,
    FastSurferLabels::LeftPrecuneus,
    FastSurferLabels::LeftRostralAnteriorCingulate,
    FastSurferLabels::LeftRostralMiddleFrontal,
    FastSurferLabels::LeftSuperiorFrontal,
    FastSurferLabels::LeftSuperiorParietal,
    FastSurferLabels::LeftSuperiorTemporal,
    FastSurferLabels::LeftSupramarginal,
    FastSurferLabels::LeftTransverseTemporal,
    FastSurferLabels::LeftInsula,
    FastSurferLabels::RightCaudalAnteriorCingulate,
    FastSurferLabels::RightCaudalMiddleFrontal,
    FastSurferLabels::RightCuneus,
    FastSurferLabels::RightEntorhinal,
    FastSurferLabels::RightFusiform,
    FastSurferLabels::RightInferiorParietal,
    FastSurferLabels::RightInferiorTemporal,
    FastSurferLabels::RightIsthmusCingulate,
    FastSurferLabels::RightLateralOccipital,
    FastSurferLabels::RightLateralOrbitofrontal,
    FastSurferLabels::RightLingual,
    FastSurferLabels::RightMedialOrbitofrontal,
    FastSurferLabels::RightMiddleTemporal,
    FastSurferLabels::RightParahippocampal,
    FastSurferLabels::RightParacentral,
    FastSurferLabels::RightParsOpercularis,
    FastSurferLabels::RightParsOrbitalis,
    FastSurferLabels::RightParsTriangularis,
    FastSurferLabels::RightPericalcarine,
    FastSurferLabels::RightPostcentral,
    FastSurferLabels::RightPosteriorCingulate,
    FastSurferLabels::RightPrecentral,
    FastSurferLabels::RightPrecuneus,
    FastSurferLabels::RightRostralAnteriorCingulate,
    FastSurferLabels::RightRostralMiddleFrontal,
    FastSurferLabels::RightSuperiorFrontal,
    FastSurferLabels::RightSuperiorParietal,
    FastSurferLabels::RightSuperiorTemporal,
    FastSurferLabels::RightSupramarginal,
    FastSurferLabels::RightTransverseTemporal,
    FastSurferLabels::RightInsula,
};

constexpr bool isFastSurferSegmentationLabel(const std::uint16_t value) noexcept
{
    for (const FastSurferLabels label : kAllFastSurferLabels) {
        if (toUnderlying(label) == value) {
            return true;
        }
    }

    return false;
}

} // namespace OpenHC::imaging::mri::fastsurfer