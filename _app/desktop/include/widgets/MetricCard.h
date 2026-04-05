#pragma once

#include <QColor>
#include <QFrame>

namespace desktop {

class MetricCard : public QFrame {
public:
    MetricCard(const QString& badge_text,
               const QString& label_text,
               const QString& value_text,
               const QString& detail_text,
               const QColor& accent_color,
               QWidget* parent = nullptr);
};

}  // namespace desktop
