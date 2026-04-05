#pragma once

#include <QColor>
#include <QWidget>

namespace desktop {

class ViewerPane : public QWidget {
public:
    ViewerPane(const QString& title_text,
               const QString& subtitle_text,
               const QColor& accent_color,
               bool highlighted = false,
               QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString title_text_;
    QString subtitle_text_;
    QColor accent_color_;
    bool highlighted_;
};

}  // namespace desktop
