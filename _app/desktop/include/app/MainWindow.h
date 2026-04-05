#pragma once

#include <QHash>
#include <QMainWindow>

#include "app/PageId.h"

class QStackedWidget;

namespace desktop {

class AppStatusBar;
class NavigationRail;
class TopBar;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setCurrentPage(PageId page_id);

    TopBar* top_bar_;
    NavigationRail* navigation_;
    QStackedWidget* page_stack_;
    AppStatusBar* status_bar_;
    QHash<PageId, int> page_indexes_;
};

}  // namespace desktop
