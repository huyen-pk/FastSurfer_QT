#include "screens/AnalyticsPage.h"

#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "app/Theme.h"
#include "widgets/AnalyticsChart.h"

namespace {

QLabel* makeTitle(const QString& text, int size, int weight, const QColor& color = desktop::Theme::text()) {
    auto* label = new QLabel(text);
    label->setFont(desktop::Theme::headlineFont(size, weight));
    label->setStyleSheet(QString("color: %1;").arg(desktop::Theme::toHex(color)));
    label->setWordWrap(true);
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

QWidget* makeKeyValue(const QString& label, const QString& value) {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(makeBody(label, 8, QFont::Bold, QColor("#738999")));
    layout->addWidget(makeBody(value, 12, QFont::Bold, desktop::Theme::text()));
    return widget;
}

}  // namespace

namespace desktop {

AnalyticsPage::AnalyticsPage(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(32, 28, 32, 24);
    outer->setSpacing(24);

    auto* header_row = new QHBoxLayout();
    auto* heading_col = new QVBoxLayout();
    heading_col->setContentsMargins(0, 0, 0, 0);
    heading_col->setSpacing(6);
    heading_col->addWidget(makeBody("Analytics  /  Comparative Results", 9, QFont::Bold, Theme::primary()));
    heading_col->addWidget(makeTitle("Comparative Study Analysis", 28, QFont::ExtraBold));
    heading_col->addWidget(makeBody("Visualizing volumetric neural data across Project Alpha Cohort B. Quantitative metrics verified against the standardized atlas.", 10, QFont::Medium));
    header_row->addLayout(heading_col, 1);

    auto* pdf_button = new QPushButton("Generate PDF Report", this);
    pdf_button->setStyleSheet(Theme::primaryButtonStyle());
    header_row->addWidget(pdf_button);
    outer->addLayout(header_row);

    auto* top_row = new QHBoxLayout();
    top_row->setSpacing(24);

    auto* chart_card = makeCard();
    auto* chart_layout = new QVBoxLayout(chart_card);
    chart_layout->setContentsMargins(22, 20, 22, 18);
    chart_layout->setSpacing(16);
    chart_layout->addWidget(makeBody("Volumetric Delta (cm3)", 9, QFont::Bold, Theme::primary()));
    chart_layout->addWidget(new AnalyticsChart(chart_card), 1);
    top_row->addWidget(chart_card, 3);

    auto* side_metrics = new QVBoxLayout();
    side_metrics->setSpacing(18);

    auto* confidence_card = new QFrame(this);
    confidence_card->setStyleSheet(Theme::cardStyle(Theme::surfaceHigh(), 20, Theme::outline()));
    auto* confidence_layout = new QVBoxLayout(confidence_card);
    confidence_layout->setContentsMargins(22, 20, 22, 20);
    confidence_layout->addWidget(makeBody("Processing Confidence", 8, QFont::Bold));
    confidence_layout->addWidget(makeTitle("98.4%", 26, QFont::ExtraBold, Theme::tertiary()));
    confidence_layout->addWidget(makeBody("Iterative refinement active. 0.2ms latency in volumetric mapping.", 9, QFont::Medium));
    side_metrics->addWidget(confidence_card);

    auto* total_card = new QFrame(this);
    total_card->setStyleSheet(Theme::cardStyle(Theme::surfaceHigh(), 20, Theme::outline()));
    auto* total_layout = new QVBoxLayout(total_card);
    total_layout->setContentsMargins(22, 20, 22, 20);
    total_layout->addWidget(makeBody("Total Studies Analyzed", 8, QFont::Bold));
    total_layout->addWidget(makeTitle("1,248", 26, QFont::ExtraBold, Theme::primary()));
    total_layout->addWidget(makeBody("Trend: +12.3% versus previous month.", 9, QFont::Medium));
    side_metrics->addWidget(total_card);
    side_metrics->addStretch(1);

    auto* side_widget = new QWidget(this);
    side_widget->setLayout(side_metrics);
    side_widget->setMaximumWidth(340);
    top_row->addWidget(side_widget, 1);

    outer->addLayout(top_row);

    auto* table_card = makeCard();
    auto* table_layout = new QVBoxLayout(table_card);
    table_layout->setContentsMargins(0, 0, 0, 0);
    table_layout->setSpacing(0);

    auto* table_header = new QWidget(table_card);
    auto* table_header_layout = new QHBoxLayout(table_header);
    table_header_layout->setContentsMargins(22, 18, 22, 18);
    table_header_layout->addWidget(makeTitle("Study Comparison Matrix", 12, QFont::Bold));
    table_header_layout->addStretch(1);
    auto* csv_button = new QPushButton("Export CSV", table_header);
    csv_button->setStyleSheet(Theme::secondaryButtonStyle());
    auto* filter_button = new QPushButton("Filter", table_header);
    filter_button->setStyleSheet(Theme::secondaryButtonStyle());
    table_header_layout->addWidget(csv_button);
    table_header_layout->addWidget(filter_button);
    table_layout->addWidget(table_header);

    auto* table = new QTableWidget(4, 6, table_card);
    table->setHorizontalHeaderLabels({"Study ID", "Metric", "Baseline Value", "Processed Value", "Variance", "Status"});
    table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setStyleSheet(Theme::tableStyle());
    table->setMinimumHeight(250);

    const QList<QStringList> rows = {
        {"MRI-2024-X01", "Whole Brain Volume", "1,240 cm3", "1,215 cm3", "-2.01%", "Critical"},
        {"MRI-2024-X04", "Ventricular Width", "12.4 mm", "12.6 mm", "+1.61%", "Stable"},
        {"MRI-2024-X09", "Cortical Thickness", "2.54 mm", "2.51 mm", "-1.18%", "Review"},
        {"MRI-2024-Y12", "White Matter Integrity", "0.84 FA", "0.88 FA", "+4.76%", "Improved"},
    };

    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < rows[row].size(); ++column) {
            auto* item = new QTableWidgetItem(rows[row][column]);
            if (column == 4) {
                item->setForeground(rows[row][column].startsWith("-") ? Theme::error() : Theme::tertiary());
            } else if (column == 0) {
                item->setForeground(Theme::primary());
            } else {
                item->setForeground(Theme::text());
            }
            table->setItem(row, column, item);
        }
    }
    table_layout->addWidget(table);
    outer->addWidget(table_card);

    auto* bottom_row = new QHBoxLayout();
    bottom_row->setSpacing(24);

    auto* heatmap_card = new QFrame(this);
    heatmap_card->setMinimumHeight(240);
    heatmap_card->setStyleSheet(QString(
        "background-color: %1;"
        "border-radius: 20px;"
        "border: 1px solid %2;"
        "background-image: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %3, stop:1 %4);"
    )
                                    .arg(Theme::toHex(Theme::surfaceLowest()))
                                    .arg(Theme::rgba(Theme::outline(), 120))
                                    .arg(Theme::rgba(Theme::tertiary(), 50))
                                    .arg(Theme::rgba(Theme::primary(), 28)));
    auto* heatmap_layout = new QVBoxLayout(heatmap_card);
    heatmap_layout->setContentsMargins(24, 24, 24, 24);
    heatmap_layout->addStretch(1);
    heatmap_layout->addWidget(makeTitle("Neural Density Mapping", 16, QFont::Bold));
    heatmap_layout->addWidget(makeBody("Global cohort average baseline versus Subject Alpha-01", 9, QFont::Medium));
    bottom_row->addWidget(heatmap_card, 1);

    auto* stats_card = makeCard();
    auto* stats_layout = new QGridLayout(stats_card);
    stats_layout->setContentsMargins(24, 24, 24, 24);
    stats_layout->setHorizontalSpacing(22);
    stats_layout->setVerticalSpacing(18);
    stats_layout->addWidget(makeKeyValue("Standard Deviation", "sigma = 0.042"), 0, 0);
    stats_layout->addWidget(makeKeyValue("P-Value Significance", "p < 0.001"), 0, 1);
    stats_layout->addWidget(makeKeyValue("Model Version", "v4.2.8-stable"), 1, 0);
    stats_layout->addWidget(makeKeyValue("Verification Hash", "8f2g-9k1L-pZ92"), 1, 1);
    bottom_row->addWidget(stats_card, 1);

    outer->addLayout(bottom_row);
}

}  // namespace desktop