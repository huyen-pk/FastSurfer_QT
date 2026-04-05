#pragma once

#include <QFrame>

class QLineEdit;
class QPushButton;

namespace desktop {

class TopBar : public QFrame {
public:
    explicit TopBar(QWidget* parent = nullptr);

private:
    QLineEdit* search_field_;
    QPushButton* primary_action_;
};

}  // namespace desktop
