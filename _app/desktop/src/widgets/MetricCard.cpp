#include "widgets/MetricCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "app/Theme.h"

namespace desktop {

MetricCard::MetricCard(const QString& badge_text,
                       const QString& label_text,
                       const QString& value_text,
                       const QString& detail_text,
                       const QColor& accent_color,
                       QWidget* parent)
    : QFrame(parent) {
    setStyleSheet(Theme::cardStyle(Theme::surface(), 18, Theme::outline()));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(12);

    auto* top_row = new QHBoxLayout();
    top_row->setContentsMargins(0, 0, 0, 0);

    auto* badge = new QLabel(badge_text, this);
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(42, 42);
    badge->setFont(Theme::headlineFont(10, QFont::Bold));
    badge->setStyleSheet(QString(
        "background-color: %1; color: %2; border-radius: 12px;"
    )
                             .arg(Theme::rgba(accent_color, 50))
                             .arg(Theme::toHex(accent_color)));

    auto* overline = new QLabel(label_text, this);
    overline->setFont(Theme::bodyFont(8, QFont::Bold));
    overline->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::mutedText())));

    top_row->addWidget(badge, 0, Qt::AlignLeft);
    top_row->addStretch(1);
    top_row->addWidget(overline, 0, Qt::AlignTop);

    auto* value = new QLabel(value_text, this);
    value->setFont(Theme::headlineFont(23, QFont::ExtraBold));

    auto* detail = new QLabel(detail_text, this);
    detail->setFont(Theme::bodyFont(9, QFont::Medium));
    detail->setStyleSheet(QString("color: %1;").arg(Theme::toHex(accent_color)));

    layout->addLayout(top_row);
    layout->addWidget(value);
    layout->addWidget(detail);
    layout->addStretch(1);
}

}  // namespace desktop
