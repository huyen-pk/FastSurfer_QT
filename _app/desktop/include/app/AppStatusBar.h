#pragma once

#include <QFrame>

namespace desktop {

class AppStatusBar : public QFrame {
public:
    explicit AppStatusBar(QWidget* parent = nullptr);
};

}  // namespace desktop
