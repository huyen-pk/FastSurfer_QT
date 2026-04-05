#include "app/MainWindow.h"

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "app/AppStatusBar.h"
#include "app/NavigationRail.h"
#include "app/TopBar.h"
#include "screens/AnalyticsPage.h"
#include "screens/DashboardPage.h"
#include "screens/PipelinesPage.h"
#include "screens/ScanViewerPage.h"

namespace desktop {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      top_bar_(new TopBar(this)),
      navigation_(new NavigationRail(this)),
      page_stack_(new QStackedWidget(this)),
      status_bar_(new AppStatusBar(this)) {
    setWindowTitle("FastSurfer Desktop");
    resize(1580, 980);
    setMinimumSize(1280, 820);

    auto* root = new QWidget(this);
    root->setObjectName("AppRoot");
    auto* root_layout = new QVBoxLayout(root);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);

    auto* content_row = new QWidget(root);
    auto* content_layout = new QHBoxLayout(content_row);
    content_layout->setContentsMargins(0, 0, 0, 0);
    content_layout->setSpacing(0);

    page_stack_->addWidget(new DashboardPage(page_stack_));
    page_indexes_.insert(PageId::Dashboard, page_stack_->count() - 1);
    page_stack_->addWidget(new ScanViewerPage(page_stack_));
    page_indexes_.insert(PageId::ScanViewer, page_stack_->count() - 1);
    page_stack_->addWidget(new PipelinesPage(page_stack_));
    page_indexes_.insert(PageId::Pipelines, page_stack_->count() - 1);
    page_stack_->addWidget(new AnalyticsPage(page_stack_));
    page_indexes_.insert(PageId::Analytics, page_stack_->count() - 1);

    content_layout->addWidget(navigation_);
    content_layout->addWidget(page_stack_, 1);

    root_layout->addWidget(top_bar_);
    root_layout->addWidget(content_row, 1);
    root_layout->addWidget(status_bar_);

    connect(navigation_, &NavigationRail::pageSelected, this, [this](PageId page_id) { setCurrentPage(page_id); });

    setCentralWidget(root);
    setCurrentPage(PageId::Dashboard);
}

void MainWindow::setCurrentPage(PageId page_id) {
    page_stack_->setCurrentIndex(page_indexes_.value(page_id, 0));
    navigation_->setCurrentPage(page_id);
}

}  // namespace desktop
