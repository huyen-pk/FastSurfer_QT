#include "widgets/AnalyticsChart.h"

#include <QPaintEvent>
#include <QPainter>

#include "app/Theme.h"

namespace {

struct VolumeMetric {
    const char* label;
    qreal baseline;
    qreal current;
};

const VolumeMetric kMetrics[] = {
    {"Hippocampus", 0.75, 0.82},
    {"Amygdala", 0.60, 0.58},
    {"Thalamus", 0.90, 0.94},
    {"Caudate", 0.45, 0.42},
    {"Putamen", 0.68, 0.72},
};

}  // namespace

namespace desktop {

AnalyticsChart::AnalyticsChart(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(280);
}

void AnalyticsChart::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), Qt::transparent);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::tertiary());
    painter.drawEllipse(QRectF(10, 12, 10, 10));
    painter.setBrush(QColor(Theme::primary().red(), Theme::primary().green(), Theme::primary().blue(), 90));
    painter.drawEllipse(QRectF(150, 12, 10, 10));

    painter.setPen(Theme::text());
    painter.setFont(Theme::bodyFont(9, QFont::Medium));
    painter.drawText(QPointF(28, 22), "Current Post-OP");
    painter.drawText(QPointF(168, 22), "Baseline Study");

    const QRect chart_rect = rect().adjusted(18, 44, -18, -28);
    painter.setPen(QPen(QColor(Theme::outline().red(), Theme::outline().green(), Theme::outline().blue(), 150), 1));
    painter.drawLine(chart_rect.bottomLeft(), chart_rect.bottomRight());

    const int group_count = static_cast<int>(std::size(kMetrics));
    const qreal group_width = static_cast<qreal>(chart_rect.width()) / group_count;
    const qreal bar_width = group_width * 0.24;
    const int label_height = 34;

    for (int index = 0; index < group_count; ++index) {
        const auto& metric = kMetrics[index];
        const qreal base_x = chart_rect.left() + index * group_width;
        const qreal baseline_height = metric.baseline * (chart_rect.height() - label_height);
        const qreal current_height = metric.current * (chart_rect.height() - label_height);
        const QRectF baseline_bar(base_x + group_width * 0.26,
                                  chart_rect.bottom() - label_height - baseline_height,
                                  bar_width,
                                  baseline_height);
        const QRectF current_bar(base_x + group_width * 0.52,
                                 chart_rect.bottom() - label_height - current_height,
                                 bar_width,
                                 current_height);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(Theme::primary().red(), Theme::primary().green(), Theme::primary().blue(), 90));
        painter.drawRoundedRect(baseline_bar, 4, 4);

        painter.setBrush(Theme::tertiary());
        painter.drawRoundedRect(current_bar, 4, 4);

        painter.setPen(QColor(Theme::mutedText().red(), Theme::mutedText().green(), Theme::mutedText().blue(), 190));
        painter.setFont(Theme::bodyFont(8, QFont::Medium));
        painter.drawText(QRectF(base_x, chart_rect.bottom() - label_height + 10, group_width, 20), Qt::AlignCenter, metric.label);
    }
}

}  // namespace desktop
