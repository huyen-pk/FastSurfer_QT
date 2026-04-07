#include "app/application_controller.h"

#include <QVariantMap>

namespace {

QVariantMap metricCard(
    const QString &title,
    const QString &value,
    const QString &note,
    const QString &accent,
    const QString &glyph)
{
    return QVariantMap {
        {QStringLiteral("title"), title},
        {QStringLiteral("value"), value},
        {QStringLiteral("note"), note},
        {QStringLiteral("accent"), accent},
        {QStringLiteral("glyph"), glyph},
    };
}

QVariantMap labeledValue(
    const QString &label,
    const QString &value,
    const QString &accent = QString())
{
    return QVariantMap {
        {QStringLiteral("label"), label},
        {QStringLiteral("value"), value},
        {QStringLiteral("accent"), accent},
    };
}

} // namespace

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
    , m_navigation(this)
{
    m_dashboardMetrics = {
        metricCard(QStringLiteral("Monthly studies"), QStringLiteral("1,284"), QStringLiteral("12.5% above trailing month"), QStringLiteral("cyan"), QStringLiteral("MR")),
        metricCard(QStringLiteral("Processing confidence"), QStringLiteral("99.4%"), QStringLiteral("14 active compute nodes synchronized"), QStringLiteral("sky"), QStringLiteral("OK")),
        metricCard(QStringLiteral("Queued reviews"), QStringLiteral("18"), QStringLiteral("6 require neuroradiology sign-off"), QStringLiteral("slate"), QStringLiteral("Q"))
    };

    m_studies = {
        QVariantMap {
            {QStringLiteral("patientId"), QStringLiteral("PX-8821")},
            {QStringLiteral("series"), QStringLiteral("T1-weighted brain")},
            {QStringLiteral("protocol"), QStringLiteral("NEURO_3D")},
            {QStringLiteral("status"), QStringLiteral("Processing")},
            {QStringLiteral("scheduledAt"), QStringLiteral("09:42")},
            {QStringLiteral("accent"), QStringLiteral("cyan")},
        },
        QVariantMap {
            {QStringLiteral("patientId"), QStringLiteral("PX-8819")},
            {QStringLiteral("series"), QStringLiteral("FLAIR follow-up")},
            {QStringLiteral("protocol"), QStringLiteral("LONG_AXIS")},
            {QStringLiteral("status"), QStringLiteral("Ready for review")},
            {QStringLiteral("scheduledAt"), QStringLiteral("09:17")},
            {QStringLiteral("accent"), QStringLiteral("sky")},
        },
        QVariantMap {
            {QStringLiteral("patientId"), QStringLiteral("PX-8814")},
            {QStringLiteral("series"), QStringLiteral("Contrast reconstruction")},
            {QStringLiteral("protocol"), QStringLiteral("SURF_QC")},
            {QStringLiteral("status"), QStringLiteral("Escalated")},
            {QStringLiteral("scheduledAt"), QStringLiteral("08:54")},
            {QStringLiteral("accent"), QStringLiteral("amber")},
        },
        QVariantMap {
            {QStringLiteral("patientId"), QStringLiteral("PX-8808")},
            {QStringLiteral("series"), QStringLiteral("Longitudinal template")},
            {QStringLiteral("protocol"), QStringLiteral("VINN_LONG")},
            {QStringLiteral("status"), QStringLiteral("Completed")},
            {QStringLiteral("scheduledAt"), QStringLiteral("08:31")},
            {QStringLiteral("accent"), QStringLiteral("slate")},
        }
    };

    m_operationalPanels = {
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Cluster load")},
            {QStringLiteral("value"), QStringLiteral("42% GPU")},
            {QStringLiteral("detail"), QStringLiteral("VINN axial, coronal, and sagittal jobs balanced across CPU review nodes.")},
            {QStringLiteral("accent"), QStringLiteral("cyan")},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Urgent cases")},
            {QStringLiteral("value"), QStringLiteral("3 flagged")},
            {QStringLiteral("detail"), QStringLiteral("Two post-operative analytics packs and one quality-control rerun remain open.")},
            {QStringLiteral("accent"), QStringLiteral("amber")},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Research cohort")},
            {QStringLiteral("value"), QStringLiteral("Project Alpha")},
            {QStringLiteral("detail"), QStringLiteral("Comparative volumetry snapshots remain aligned to the current atlas definition.")},
            {QStringLiteral("accent"), QStringLiteral("sky")},
        }
    };

    m_viewerPanels = {
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Axial")},
            {QStringLiteral("coordinate"), QStringLiteral("Z 142 mm")},
            {QStringLiteral("note"), QStringLiteral("Cortical boundary emphasis")},
            {QStringLiteral("accent"), QStringLiteral("cyan")},
            {QStringLiteral("highlighted"), false},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Sagittal")},
            {QStringLiteral("coordinate"), QStringLiteral("X 12.5 mm")},
            {QStringLiteral("note"), QStringLiteral("Midline alignment")},
            {QStringLiteral("accent"), QStringLiteral("sky")},
            {QStringLiteral("highlighted"), false},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Coronal")},
            {QStringLiteral("coordinate"), QStringLiteral("Y -84.2 mm")},
            {QStringLiteral("note"), QStringLiteral("Signal uniformity check")},
            {QStringLiteral("accent"), QStringLiteral("cyan")},
            {QStringLiteral("highlighted"), false},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("3D volume")},
            {QStringLiteral("coordinate"), QStringLiteral("0.5 mm isotropic")},
            {QStringLiteral("note"), QStringLiteral("Segmentation-ready placeholder render")},
            {QStringLiteral("accent"), QStringLiteral("cyan")},
            {QStringLiteral("highlighted"), true},
        }
    };

    m_scanMetadata = {
        labeledValue(QStringLiteral("Field of view"), QStringLiteral("240 x 240")),
        labeledValue(QStringLiteral("TR / TE"), QStringLiteral("11 / 4.6 ms")),
        labeledValue(QStringLiteral("Slice thickness"), QStringLiteral("1.0 mm")),
        labeledValue(QStringLiteral("Flip angle"), QStringLiteral("15 deg")),
        labeledValue(QStringLiteral("Sequence"), QStringLiteral("GRE-3D")),
        labeledValue(QStringLiteral("Magnet"), QStringLiteral("3.0 Tesla"))
    };

    m_studyNotes = {
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Study date")},
            {QStringLiteral("value"), QStringLiteral("Oct 24, 2023 14:32")},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Referring physician")},
            {QStringLiteral("value"), QStringLiteral("Dr. Sarah Chen")},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Viewer mode")},
            {QStringLiteral("value"), QStringLiteral("Multi-view review")},
        }
    };

    m_pipelineSteps = {
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("DICOM input")},
            {QStringLiteral("subtitle"), QStringLiteral("Research_Repository_A1" )},
            {QStringLiteral("badge"), QStringLiteral("Input data")},
            {QStringLiteral("state"), QStringLiteral("Ready")},
            {QStringLiteral("accent"), QStringLiteral("sky")},
            {QStringLiteral("active"), false},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Motion correction")},
            {QStringLiteral("subtitle"), QStringLiteral("6 DOF standard alignment")},
            {QStringLiteral("badge"), QStringLiteral("Optimized")},
            {QStringLiteral("state"), QStringLiteral("Primary path")},
            {QStringLiteral("accent"), QStringLiteral("cyan")},
            {QStringLiteral("active"), true},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Skull stripping")},
            {QStringLiteral("subtitle"), QStringLiteral("Brain extraction and mask refinement")},
            {QStringLiteral("badge"), QStringLiteral("ML assist")},
            {QStringLiteral("state"), QStringLiteral("Queued")},
            {QStringLiteral("accent"), QStringLiteral("slate")},
            {QStringLiteral("active"), false},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Volume quantification")},
            {QStringLiteral("subtitle"), QStringLiteral("Segmentation-derived cortical metrics")},
            {QStringLiteral("badge"), QStringLiteral("Analytics")},
            {QStringLiteral("state"), QStringLiteral("Waiting on masks")},
            {QStringLiteral("accent"), QStringLiteral("sky")},
            {QStringLiteral("active"), false},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Output generation")},
            {QStringLiteral("subtitle"), QStringLiteral("Clinical packet and structured report")},
            {QStringLiteral("badge"), QStringLiteral("Report")},
            {QStringLiteral("state"), QStringLiteral("Standby")},
            {QStringLiteral("accent"), QStringLiteral("cyan")},
            {QStringLiteral("active"), false},
        }
    };

    m_analyticsBars = {
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Hippocampus")}, {QStringLiteral("baseline"), 75}, {QStringLiteral("current"), 82}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Amygdala")}, {QStringLiteral("baseline"), 60}, {QStringLiteral("current"), 58}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Thalamus")}, {QStringLiteral("baseline"), 90}, {QStringLiteral("current"), 94}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Caudate")}, {QStringLiteral("baseline"), 45}, {QStringLiteral("current"), 42}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Putamen")}, {QStringLiteral("baseline"), 68}, {QStringLiteral("current"), 72}}
    };

    m_analyticsCards = {
        metricCard(QStringLiteral("Processing confidence"), QStringLiteral("98.4%"), QStringLiteral("Cross-view agreement remains within tolerance."), QStringLiteral("cyan"), QStringLiteral("CF")),
        metricCard(QStringLiteral("Atlas drift"), QStringLiteral("0.8%"), QStringLiteral("Normalized against cohort baseline."), QStringLiteral("sky"), QStringLiteral("AT")),
        metricCard(QStringLiteral("Review turnaround"), QStringLiteral("6.2 h"), QStringLiteral("Median from pipeline completion to sign-off."), QStringLiteral("slate"), QStringLiteral("RT"))
    };

    m_analyticsInsights = {
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Comparative cohort note")},
            {QStringLiteral("detail"), QStringLiteral("Current post-operative scans show mild volumetric recovery in the hippocampal segment group and stable thalamic consistency.")},
        },
        QVariantMap {
            {QStringLiteral("title"), QStringLiteral("Radiology handoff")},
            {QStringLiteral("detail"), QStringLiteral("Three analytics packets remain prioritized for same-day neuroradiology review with export-ready summaries.")},
        }
    };
}

QString ApplicationController::applicationName() const
{
    return QStringLiteral("Luminescent MRI");
}

QString ApplicationController::productName() const
{
    return QStringLiteral("FastSurfer Qt Desktop");
}

QString ApplicationController::projectName() const
{
    return QStringLiteral("Project Alpha");
}

QString ApplicationController::projectContext() const
{
    return QStringLiteral("Clinical research");
}

NavigationState *ApplicationController::navigation()
{
    return &m_navigation;
}

QVariantList ApplicationController::dashboardMetrics() const
{
    return m_dashboardMetrics;
}

QVariantList ApplicationController::studies() const
{
    return m_studies;
}

QVariantList ApplicationController::operationalPanels() const
{
    return m_operationalPanels;
}

QVariantList ApplicationController::viewerPanels() const
{
    return m_viewerPanels;
}

QVariantList ApplicationController::scanMetadata() const
{
    return m_scanMetadata;
}

QVariantList ApplicationController::studyNotes() const
{
    return m_studyNotes;
}

QVariantList ApplicationController::pipelineSteps() const
{
    return m_pipelineSteps;
}

QVariantList ApplicationController::analyticsBars() const
{
    return m_analyticsBars;
}

QVariantList ApplicationController::analyticsCards() const
{
    return m_analyticsCards;
}

QVariantList ApplicationController::analyticsInsights() const
{
    return m_analyticsInsights;
}