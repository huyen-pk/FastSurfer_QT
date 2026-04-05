#include "screens/PipelinesPage.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "app/Theme.h"
#include "widgets/PipelineNode.h"

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

}  // namespace

namespace desktop {

PipelinesPage::PipelinesPage(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(32, 28, 32, 24);
    outer->setSpacing(24);

    auto* header_row = new QHBoxLayout();
    auto* heading_col = new QVBoxLayout();
    heading_col->setContentsMargins(0, 0, 0, 0);
    heading_col->setSpacing(6);
    heading_col->addWidget(makeTitle("Automated Pipeline Overview", 24, QFont::ExtraBold));
    heading_col->addWidget(makeBody("Sequence #4429  |  Optimized Research Template", 10, QFont::Medium, Theme::primary()));
    header_row->addLayout(heading_col, 1);

    auto* duplicate_button = new QPushButton("Duplicate Template", this);
    duplicate_button->setStyleSheet(Theme::secondaryButtonStyle());
    auto* execute_button = new QPushButton("Execute Sequence", this);
    execute_button->setStyleSheet(Theme::primaryButtonStyle());
    header_row->addWidget(duplicate_button);
    header_row->addWidget(execute_button);

    outer->addLayout(header_row);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget(scroll);
    auto* content = new QVBoxLayout(container);
    content->setContentsMargins(120, 10, 120, 10);
    content->setSpacing(0);

    content->addWidget(new PipelineNode("IN", "DICOM Input", "Source: Research_Repository_A1", Theme::primary(), "Input Data", true, container));
    content->addWidget(new PipelineNode("MC", "Motion Correction", "FSL McFlirt Algorithm  |  6 DOF (Standard)", Theme::tertiary(), "Optimized", true, container));
    content->addWidget(new PipelineNode("SS", "Skull Stripping", "BET brain extraction with robust estimation", Theme::primary(), QString(), true, container));
    content->addWidget(new PipelineNode("VQ", "Volume Quantification", "Segmentation-based T1 measurement", Theme::primary(), QString(), true, container));
    content->addWidget(new PipelineNode("RG", "Report Generation", "Clinical PDF and raw CSV output", Theme::secondary(), "Endpoint", false, container));

    auto* add_step = new QPushButton("Add Step to Sequence\nChoose from pre-configured clinical modules", container);
    add_step->setMinimumHeight(110);
    add_step->setStyleSheet(QString(
        "QPushButton {"
        " background-color: transparent;"
        " color: %1;"
        " border: 2px dashed %2;"
        " border-radius: 18px;"
        " padding: 18px;"
        " font-size: 12px;"
        " font-weight: 700;"
        "}"
        "QPushButton:hover { background-color: %3; }"
    )
                               .arg(Theme::toHex(Theme::primary()))
                               .arg(Theme::rgba(Theme::outline(), 160))
                               .arg(Theme::toHex(Theme::surfaceLow())));
    content->addSpacing(18);
    content->addWidget(add_step);
    content->addStretch(1);

    scroll->setWidget(container);
    outer->addWidget(scroll, 1);
}

}  // namespace desktop
