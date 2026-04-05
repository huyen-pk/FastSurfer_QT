#pragma once

#include <QFrame>
#include <QHash>

#include "app/PageId.h"

class QPushButton;

namespace desktop {

class NavigationRail : public QFrame {
    Q_OBJECT

public:
    explicit NavigationRail(QWidget* parent = nullptr);

    void setCurrentPage(PageId page_id);

signals:
    void pageSelected(PageId page_id);

private:
    QPushButton* createButton(const QString& label, PageId page_id);
    void refreshButtonStyles();

    QHash<PageId, QPushButton*> buttons_;
    PageId current_page_;
};

}  // namespace desktop
