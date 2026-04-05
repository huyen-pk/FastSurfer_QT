#include "app/NavigationRail.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "app/Theme.h"

namespace desktop {

NavigationRail::NavigationRail(QWidget* parent) : QFrame(parent), current_page_(PageId::Dashboard) {
    setObjectName("NavigationRail");
    setFixedWidth(264);
    setStyleSheet(QString(
        "QFrame#NavigationRail {"
        " background-color: %1;"
        " border-right: 1px solid %2;"
        "}"
    )
                      .arg(Theme::toHex(Theme::surface()))
                      .arg(Theme::rgba(Theme::outline(), 120)));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 24, 20, 20);
    layout->setSpacing(14);

    auto* project_label = new QLabel("Project Alpha", this);
    project_label->setFont(Theme::headlineFont(13, QFont::Bold));

    auto* project_detail = new QLabel("Clinical Research", this);
    project_detail->setFont(Theme::bodyFont(9, QFont::Medium));
    project_detail->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::mutedText())));

    layout->addWidget(project_label);
    layout->addWidget(project_detail);
    layout->addSpacing(10);

    layout->addWidget(createButton("Dashboard", PageId::Dashboard));
    layout->addWidget(createButton("Scan Viewer", PageId::ScanViewer));
    layout->addWidget(createButton("Pipelines", PageId::Pipelines));
    layout->addWidget(createButton("Analytics", PageId::Analytics));

    layout->addSpacing(18);

    auto* primary_action = new QPushButton("New Processing Task", this);
    primary_action->setStyleSheet(Theme::primaryButtonStyle());
    layout->addWidget(primary_action);
    layout->addStretch(1);

    auto* footer_title = new QLabel("Clinical Utilities", this);
    footer_title->setFont(Theme::bodyFont(9, QFont::Bold));
    footer_title->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::mutedText())));
    layout->addWidget(footer_title);

    for (const QString& item_text : {QString("System Status"), QString("Support")}) {
        auto* item_button = new QPushButton(item_text, this);
        item_button->setStyleSheet(Theme::navigationButtonStyle(false));
        layout->addWidget(item_button);
    }

    refreshButtonStyles();
}

void NavigationRail::setCurrentPage(PageId page_id) {
    current_page_ = page_id;
    refreshButtonStyles();
}

QPushButton* NavigationRail::createButton(const QString& label, PageId page_id) {
    auto* button = new QPushButton(label, this);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(46);

    QStyle::StandardPixmap icon_type = QStyle::SP_DesktopIcon;
    switch (page_id) {
    case PageId::Dashboard:
        icon_type = QStyle::SP_ComputerIcon;
        break;
    case PageId::ScanViewer:
        icon_type = QStyle::SP_FileDialogContentsView;
        break;
    case PageId::Pipelines:
        icon_type = QStyle::SP_BrowserReload;
        break;
    case PageId::Analytics:
        icon_type = QStyle::SP_DriveHDIcon;
        break;
    }

    button->setIcon(style()->standardIcon(icon_type));
    button->setIconSize(QSize(18, 18));

    connect(button, &QPushButton::clicked, this, [this, page_id]() {
        current_page_ = page_id;
        refreshButtonStyles();
        emit pageSelected(page_id);
    });

    buttons_.insert(page_id, button);
    return button;
}

void NavigationRail::refreshButtonStyles() {
    for (auto iterator = buttons_.begin(); iterator != buttons_.end(); ++iterator) {
        iterator.value()->setStyleSheet(Theme::navigationButtonStyle(iterator.key() == current_page_));
    }
}

}  // namespace desktop
