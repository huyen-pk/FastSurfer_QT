#include "screens/ScanViewerPage.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/Theme.h"
#include "widgets/ViewerPane.h"

namespace {

QLabel* makeTitle(const QString& text, int size, int weight, const QColor& color = desktop::Theme::text()) {
    auto* label = new QLabel(text);
    label->setFont(desktop::Theme::headlineFont(size, weight));
    label->setStyleSheet(QString("color: %1;").arg(desktop::Theme::toHex(color)));
    return label;
}

QLabel* makeBody(const QString& text, int size, int weight, const QColor& color = desktop::Theme::mutedText()) {
    auto* label = new QLabel(text);
    label->setFont(desktop::Theme::bodyFont(size, weight));
    label->setStyleSheet(QString("color: %1;").arg(desktop::Theme::toHex(color)));
    label->setWordWrap(true);
    return label;
}

QFrame* makeSidebarCard() {
    auto* card = new QFrame();
    card->setStyleSheet(desktop::Theme::cardStyle(desktop::Theme::surfaceHigh(), 18, desktop::Theme::outline()));
    return card;
}

QWidget* makeStat(const QString& label, const QString& value) {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(makeBody(label, 8, QFont::Bold, QColor("#738999")));
    layout->addWidget(makeBody(value, 10, QFont::Medium, desktop::Theme::text()));
    return widget;
}

}  // namespace

namespace desktop {

ScanViewerPage::ScanViewerPage(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 20, 24, 20);
    outer->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);

    auto* viewer_shell = new QWidget(splitter);
    auto* viewer_layout = new QVBoxLayout(viewer_shell);
    viewer_layout->setContentsMargins(0, 0, 0, 0);
    viewer_layout->setSpacing(12);

    auto* viewer_toolbar = new QFrame(viewer_shell);
    viewer_toolbar->setFixedHeight(48);
    viewer_toolbar->setStyleSheet(Theme::cardStyle(Theme::surfaceLow(), 16, Theme::outline()));
    auto* toolbar_layout = new QHBoxLayout(viewer_toolbar);
    toolbar_layout->setContentsMargins(16, 10, 16, 10);

    auto* patient_chip = new QLabel("PATIENT: JX-9802", viewer_toolbar);
    patient_chip->setStyleSheet(Theme::chipStyle(Theme::tertiary(), QColor("#00363d")));
    toolbar_layout->addWidget(patient_chip, 0, Qt::AlignLeft);
    toolbar_layout->addWidget(makeBody("Series: T1 Weighted Contrast Enhancement", 9, QFont::Medium));
    toolbar_layout->addStretch(1);

    for (const QString& mode : {QString("Multi-View"), QString("Cinema"), QString("Comparison")}) {
        auto* mode_button = new QPushButton(mode, viewer_toolbar);
        mode_button->setStyleSheet(mode == "Multi-View" ? Theme::primaryButtonStyle() : Theme::secondaryButtonStyle());
        toolbar_layout->addWidget(mode_button);
    }

    auto* fullscreen_button = new QToolButton(viewer_toolbar);
    fullscreen_button->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    fullscreen_button->setStyleSheet(Theme::iconButtonStyle());
    toolbar_layout->addWidget(fullscreen_button);
    viewer_layout->addWidget(viewer_toolbar);

    auto* viewer_grid = new QWidget(viewer_shell);
    viewer_grid->setStyleSheet(QString("background-color: %1; border-radius: 16px;").arg(Theme::toHex(Theme::outline())));
    auto* grid = new QGridLayout(viewer_grid);
    grid->setContentsMargins(1, 1, 1, 1);
    grid->setSpacing(1);

    grid->addWidget(new ViewerPane("AXIAL", "Z: 142mm", Theme::tertiary(), false, viewer_grid), 0, 0);
    grid->addWidget(new ViewerPane("SAGITTAL", "X: 12.5mm", Theme::tertiary(), false, viewer_grid), 0, 1);
    grid->addWidget(new ViewerPane("CORONAL", "Y: -84.2mm", Theme::tertiary(), false, viewer_grid), 1, 0);
    grid->addWidget(new ViewerPane("3D VOLUME", "Voxel: 0.5mm ISO", Theme::tertiary(), true, viewer_grid), 1, 1);

    viewer_layout->addWidget(viewer_grid, 1);

    auto* controls = new QFrame(viewer_shell);
    controls->setStyleSheet(Theme::cardStyle(Theme::surfaceLow(), 16, Theme::outline()));
    auto* controls_layout = new QHBoxLayout(controls);
    controls_layout->setContentsMargins(16, 10, 16, 10);
    controls_layout->setSpacing(10);
    for (const QString& action : {QString("Rotate"), QString("Zoom"), QString("Layers")}) {
        auto* button = new QPushButton(action, controls);
        button->setStyleSheet(Theme::secondaryButtonStyle());
        controls_layout->addWidget(button);
    }
    controls_layout->addStretch(1);
    viewer_layout->addWidget(controls);

    auto* sidebar = new QFrame(splitter);
    sidebar->setMinimumWidth(320);
    sidebar->setMaximumWidth(360);
    sidebar->setStyleSheet(Theme::cardStyle(Theme::surface(), 18, Theme::outline()));
    auto* sidebar_layout = new QVBoxLayout(sidebar);
    sidebar_layout->setContentsMargins(18, 18, 18, 18);
    sidebar_layout->setSpacing(16);

    auto* sidebar_header = new QHBoxLayout();
    sidebar_header->addWidget(makeTitle("Scan Information", 12, QFont::Bold));
    sidebar_header->addStretch(1);
    sidebar_header->addWidget(makeBody("Metadata", 9, QFont::Bold, Theme::primary()));
    sidebar_layout->addLayout(sidebar_header);

    auto* metadata = makeSidebarCard();
    auto* metadata_layout = new QGridLayout(metadata);
    metadata_layout->setContentsMargins(18, 18, 18, 18);
    metadata_layout->setHorizontalSpacing(16);
    metadata_layout->setVerticalSpacing(12);
    metadata_layout->addWidget(makeStat("Field of View", "240 x 240"), 0, 0);
    metadata_layout->addWidget(makeStat("TR / TE", "11 / 4.6ms"), 0, 1);
    metadata_layout->addWidget(makeStat("Slice Thickness", "1.0mm"), 1, 0);
    metadata_layout->addWidget(makeStat("Flip Angle", "15 deg"), 1, 1);
    metadata_layout->addWidget(makeStat("Sequence", "GRE-3D"), 2, 0);
    metadata_layout->addWidget(makeStat("Magnet", "3.0 Tesla"), 2, 1);
    sidebar_layout->addWidget(metadata);

    auto* study_details = makeSidebarCard();
    auto* details_layout = new QVBoxLayout(study_details);
    details_layout->setContentsMargins(18, 18, 18, 18);
    details_layout->setSpacing(12);
    details_layout->addWidget(makeBody("Study Date", 8, QFont::Bold, QColor("#738999")));
    details_layout->addWidget(makeBody("Oct 24, 2023  |  14:32", 10, QFont::Medium, Theme::text()));
    details_layout->addWidget(makeBody("Referring Physician", 8, QFont::Bold, QColor("#738999")));
    details_layout->addWidget(makeBody("Dr. Sarah Chen", 10, QFont::Medium, Theme::text()));
    details_layout->addWidget(makeBody("Observations", 8, QFont::Bold, QColor("#738999")));
    details_layout->addWidget(makeBody("Viewer containers are prepared for future MRI rendering and metadata synchronization.", 9, QFont::Medium));
    sidebar_layout->addWidget(study_details);
    sidebar_layout->addStretch(1);

    auto* export_button = new QPushButton("Export DICOM", sidebar);
    export_button->setStyleSheet(Theme::secondaryButtonStyle());
    auto* report_button = new QPushButton("Finalize Report", sidebar);
    report_button->setStyleSheet(Theme::primaryButtonStyle());
    sidebar_layout->addWidget(export_button);
    sidebar_layout->addWidget(report_button);

    splitter->addWidget(viewer_shell);
    splitter->addWidget(sidebar);
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 1);

    outer->addWidget(splitter, 1);
}

}  // namespace desktop
