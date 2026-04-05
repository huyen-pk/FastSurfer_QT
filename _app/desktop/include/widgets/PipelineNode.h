#pragma once

#include <QColor>
#include <QWidget>

namespace desktop {

class PipelineNode : public QWidget {
public:
    PipelineNode(const QString& badge_text,
                 const QString& title_text,
                 const QString& detail_text,
                 const QColor& accent_color,
                 const QString& chip_text = QString(),
                 bool draw_connector = true,
                 QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor accent_color_;
    bool draw_connector_;
};

}  // namespace desktop
