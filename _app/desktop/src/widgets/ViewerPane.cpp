#include "widgets/ViewerPane.h"

#include <QPaintEvent>
#include <QPainter>

#include "app/Theme.h"

namespace desktop {

ViewerPane::ViewerPane(const QString& title_text,
                       const QString& subtitle_text,
                       const QColor& accent_color,
                       bool highlighted,
                       QWidget* parent)
    : QWidget(parent),
      title_text_(title_text),
      subtitle_text_(subtitle_text),
      accent_color_(accent_color),
      highlighted_(highlighted) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(260, 220);
}

void ViewerPane::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
    gradient.setColorAt(0.0, Theme::surfaceLowest());
    gradient.setColorAt(1.0, QColor("#08181d"));
    painter.fillRect(rect(), gradient);

    painter.setPen(QPen(highlighted_ ? accent_color_ : QColor(Theme::outline().red(), Theme::outline().green(), Theme::outline().blue(), 180), highlighted_ ? 2 : 1));
    painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 10, 10);

    painter.setClipRect(rect().adjusted(8, 8, -8, -8));

    painter.setPen(QPen(QColor(Theme::primary().red(), Theme::primary().green(), Theme::primary().blue(), 28), 1));
    for (int i = 1; i <= 4; ++i) {
        painter.drawEllipse(rect().center(), width() / (5 - i), height() / (6 - i));
    }

    painter.setPen(QPen(QColor(Theme::mutedText().red(), Theme::mutedText().green(), Theme::mutedText().blue(), 55), 1));
    painter.drawLine(QPoint(width() / 2, 18), QPoint(width() / 2, height() - 18));
    painter.drawLine(QPoint(18, height() / 2), QPoint(width() - 18, height() / 2));

    painter.setPen(QPen(QColor(accent_color_.red(), accent_color_.green(), accent_color_.blue(), 110), 2));
    painter.drawEllipse(rect().center(), width() / 7, height() / 8);

    painter.setClipping(false);
    painter.setPen(accent_color_);
    painter.setFont(Theme::bodyFont(8, QFont::Bold));
    painter.drawText(QRect(14, 12, width() - 28, 18), Qt::AlignLeft | Qt::AlignVCenter, title_text_);

    painter.setPen(QColor(Theme::text().red(), Theme::text().green(), Theme::text().blue(), 110));
    painter.setFont(Theme::bodyFont(8, QFont::Medium));
    painter.drawText(QRect(14, 28, width() - 28, 18), Qt::AlignLeft | Qt::AlignVCenter, subtitle_text_);

    painter.drawText(QRect(14, height() - 28, width() - 28, 16), Qt::AlignRight | Qt::AlignVCenter, "R:45.2  L:22.1");
}

}  // namespace desktop
