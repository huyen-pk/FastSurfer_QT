#include "widgets/PipelineNode.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

#include "app/Theme.h"

namespace desktop {

PipelineNode::PipelineNode(const QString& badge_text,
                           const QString& title_text,
                           const QString& detail_text,
                           const QColor& accent_color,
                           const QString& chip_text,
                           bool draw_connector,
                           QWidget* parent)
    : QWidget(parent), accent_color_(accent_color), draw_connector_(draw_connector) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(draw_connector_ ? 118 : 96);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, draw_connector_ ? 28 : 18);
    layout->setSpacing(18);

    auto* badge = new QLabel(badge_text, this);
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(46, 46);
    badge->setFont(Theme::headlineFont(10, QFont::Bold));
    badge->setStyleSheet(QString(
        "background-color: %1; color: %2; border-radius: 12px;"
    )
                             .arg(Theme::rgba(accent_color_, 45))
                             .arg(Theme::toHex(accent_color_)));

    auto* text_column = new QVBoxLayout();
    text_column->setContentsMargins(0, 0, 0, 0);
    text_column->setSpacing(4);

    auto* title = new QLabel(title_text, this);
    title->setFont(Theme::headlineFont(12, QFont::Bold));

    auto* detail = new QLabel(detail_text, this);
    detail->setFont(Theme::bodyFont(9, QFont::Medium));
    detail->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::mutedText())));

    text_column->addWidget(title);
    text_column->addWidget(detail);

    layout->addWidget(badge, 0, Qt::AlignTop);
    layout->addLayout(text_column, 1);

    if (!chip_text.isEmpty()) {
        auto* chip = new QLabel(chip_text, this);
        chip->setStyleSheet(Theme::chipStyle(QColor("#2f4865"), Theme::secondary()));
        layout->addWidget(chip, 0, Qt::AlignTop);
    }
}

void PipelineNode::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF card_rect = rect().adjusted(0.5, 0.5, -0.5, draw_connector_ ? -18.0 : -0.5);
    QPainterPath path;
    path.addRoundedRect(card_rect, 18.0, 18.0);

    painter.fillPath(path, Theme::surface());
    painter.setPen(QPen(QColor(Theme::outline().red(), Theme::outline().green(), Theme::outline().blue(), 150), 1));
    painter.drawPath(path);

    painter.setPen(QPen(accent_color_, 4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(card_rect.left() + 2.0, card_rect.top() + 16.0), QPointF(card_rect.left() + 2.0, card_rect.bottom() - 16.0));

    if (draw_connector_) {
        painter.setPen(QPen(QColor(accent_color_.red(), accent_color_.green(), accent_color_.blue(), 120), 2, Qt::SolidLine, Qt::RoundCap));
        const qreal connector_x = 47.0;
        painter.drawLine(QPointF(connector_x, card_rect.bottom() + 2.0), QPointF(connector_x, rect().bottom() - 3.0));
    }
}

}  // namespace desktop
