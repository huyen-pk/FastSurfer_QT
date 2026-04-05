#include "screens/DashboardPage.h"

#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "app/Theme.h"
#include "widgets/MetricCard.h"

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

QFrame* makeCard() {
    auto* card = new QFrame();
    card->setStyleSheet(desktop::Theme::cardStyle(desktop::Theme::surfaceLow(), 20, desktop::Theme::outline()));
    return card;
}

QWidget* makeEvent(const QString& title, const QString& detail, const QString& time, const QColor& accent) {
    auto* wrapper = new QWidget();
    auto* layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(12, 0, 0, 0);
    layout->setSpacing(4);

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);

    auto* dot = new QLabel(wrapper);
    dot->setFixedSize(10, 10);
    dot->setStyleSheet(QString("background-color: %1; border-radius: 5px;").arg(desktop::Theme::toHex(accent)));

    row->addWidget(dot, 0, Qt::AlignTop);
    row->addWidget(makeBody(title, 10, QFont::Bold, desktop::Theme::text()), 1);

    layout->addLayout(row);
    layout->addWidget(makeBody(detail, 9, QFont::Medium));
    layout->addWidget(makeBody(time, 8, QFont::Medium, QColor("#6e8595")));
    return wrapper;
}

}  // namespace

namespace desktop {

DashboardPage::DashboardPage(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(32, 28, 32, 24);
    outer->setSpacing(24);

    outer->addWidget(makeTitle("Clinical Overview", 24, QFont::ExtraBold));
    outer->addWidget(makeBody("Real-time monitoring of neuroimaging computational pipelines.", 10, QFont::Medium));

    auto* metrics_row = new QHBoxLayout();
    metrics_row->setSpacing(18);
    metrics_row->addWidget(new MetricCard("SC", "MTD Scans", "1,284", "+12.5% from last month", Theme::primary(), this));
    metrics_row->addWidget(new MetricCard("OK", "Avg Efficiency", "99.4%", "Across 14 active compute nodes", Theme::tertiary(), this));
    metrics_row->addWidget(new MetricCard("Q", "Queue Status", "18", "Tasks awaiting prioritization", Theme::secondary(), this));
    outer->addLayout(metrics_row);

    auto* content_row = new QHBoxLayout();
    content_row->setSpacing(24);

    auto* studies_card = makeCard();
    auto* studies_layout = new QVBoxLayout(studies_card);
    studies_layout->setContentsMargins(0, 0, 0, 0);
    studies_layout->setSpacing(0);

    auto* studies_header = new QWidget(studies_card);
    studies_header->setStyleSheet(QString("background-color: %1; border-top-left-radius: 20px; border-top-right-radius: 20px;")
                                      .arg(Theme::toHex(Theme::surface())));
    auto* studies_header_layout = new QHBoxLayout(studies_header);
    studies_header_layout->setContentsMargins(22, 18, 22, 18);
    studies_header_layout->addWidget(makeTitle("Recent MRI Studies", 12, QFont::Bold));
    studies_header_layout->addStretch(1);
    studies_header_layout->addWidget(makeBody("View All", 9, QFont::Bold, Theme::primary()));
    studies_layout->addWidget(studies_header);

    auto* table = new QTableWidget(3, 4, studies_card);
    table->setHorizontalHeaderLabels({"Patient ID", "Protocol", "Status", "Time"});
    table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(Theme::tableStyle());
    table->setMinimumHeight(320);

    const QList<QStringList> rows = {
        {"PX-8821", "NEURO_3D", "Processing", "09:42 AM"},
        {"PX-8819", "STROKE_FAST", "Completed", "08:15 AM"},
        {"PX-8755", "CARD_V1", "Error", "Yesterday"},
    };
    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < rows[row].size(); ++column) {
            auto* item = new QTableWidgetItem(rows[row][column]);
            if (column == 2) {
                QColor color = Theme::mutedText();
                if (rows[row][column] == "Processing") {
                    color = Theme::tertiary();
                } else if (rows[row][column] == "Completed") {
                    color = QColor("#82d7b2");
                } else if (rows[row][column] == "Error") {
                    color = Theme::error();
                }
                item->setForeground(color);
            } else if (column == 0) {
                item->setForeground(Theme::text());
            } else {
                item->setForeground(Theme::mutedText());
            }
            table->setItem(row, column, item);
        }
    }

    studies_layout->addWidget(table);
    content_row->addWidget(studies_card, 3);

    auto* side_column = new QVBoxLayout();
    side_column->setSpacing(20);

    auto* events_card = new QFrame(this);
    events_card->setStyleSheet(Theme::cardStyle(Theme::surfaceHigh(), 20, Theme::outline()));
    auto* events_layout = new QVBoxLayout(events_card);
    events_layout->setContentsMargins(20, 20, 20, 20);
    events_layout->setSpacing(16);
    events_layout->addWidget(makeTitle("Pipeline Events", 12, QFont::Bold));
    events_layout->addWidget(makeEvent("Report Generated", "Diagnostic report for study PX-8819 has been finalized and signed.", "24m ago", Theme::tertiary()));
    events_layout->addWidget(makeEvent("Model Update", "Segmentation engine v2.4.1 deployed to Node Cluster Alpha.", "1h ago", Theme::primary()));
    events_layout->addWidget(makeEvent("Node Failure", "Compute Node 04 unresponsive. Tasks rerouted to standby servers.", "3h ago", Theme::error()));
    side_column->addWidget(events_card);

    auto* preview_card = new QFrame(this);
    preview_card->setMinimumHeight(250);
    preview_card->setStyleSheet(QString(
        "background-color: %1;"
        "border-radius: 20px;"
        "border: 1px solid %2;"
        "background-image: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %3, stop:1 %4);"
    )
                                    .arg(Theme::toHex(Theme::surfaceLowest()))
                                    .arg(Theme::rgba(Theme::outline(), 120))
                                    .arg(Theme::rgba(Theme::tertiary(), 35))
                                    .arg(Theme::rgba(Theme::background(), 255)));
    auto* preview_layout = new QVBoxLayout(preview_card);
    preview_layout->setContentsMargins(22, 22, 22, 22);
    preview_layout->addStretch(1);
    auto* live_chip = makeBody("LIVE MONITOR", 8, QFont::Bold, QColor("#00363d"));
    live_chip->setStyleSheet(QString("background-color: %1; color: #00363d; border-radius: 10px; padding: 4px 8px;").arg(Theme::toHex(Theme::tertiary())));
    preview_layout->addWidget(live_chip, 0, Qt::AlignLeft);
    preview_layout->addWidget(makeTitle("Segmental Analysis In Progress", 13, QFont::Bold));
    preview_layout->addWidget(makeBody("Study PX-8821  |  Slice 128/256", 9, QFont::Medium));
    side_column->addWidget(preview_card);

    auto* side_wrapper = new QWidget(this);
    side_wrapper->setLayout(side_column);
    side_wrapper->setMaximumWidth(410);
    content_row->addWidget(side_wrapper, 2);

    outer->addLayout(content_row, 1);
}

}  // namespace desktop
