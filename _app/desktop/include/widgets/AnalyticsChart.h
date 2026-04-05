#pragma once

#include <QWidget>

namespace desktop {

class AnalyticsChart : public QWidget {
public:
    explicit AnalyticsChart(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

}  // namespace desktop