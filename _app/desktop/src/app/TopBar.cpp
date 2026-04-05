#include "app/TopBar.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>

#include "app/Theme.h"

namespace desktop {

TopBar::TopBar(QWidget* parent)
    : QFrame(parent), search_field_(new QLineEdit(this)), primary_action_(new QPushButton("New Processing Task", this)) {
    setObjectName("TopBar");
    setFixedHeight(78);
    setStyleSheet(QString(
        "QFrame#TopBar {"
        " background-color: %1;"
        " border-bottom: 1px solid %2;"
        "}"
    )
                      .arg(Theme::toHex(Theme::background()))
                      .arg(Theme::rgba(Theme::outline(), 120)));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(28, 14, 28, 14);
    layout->setSpacing(16);

    auto* brand = new QLabel("Luminescent MRI", this);
    brand->setFont(Theme::headlineFont(16, QFont::ExtraBold));
    brand->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::tertiary())));

    auto* search_shell = new QFrame(this);
    search_shell->setStyleSheet(Theme::cardStyle(Theme::surfaceHighest(), 14, Theme::outline()));
    auto* search_layout = new QHBoxLayout(search_shell);
    search_layout->setContentsMargins(12, 4, 12, 4);
    search_layout->setSpacing(8);

    auto* search_icon = new QLabel(search_shell);
    search_icon->setPixmap(style()->standardIcon(QStyle::SP_FileDialogContentsView).pixmap(16, 16));
    search_icon->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::primary())));

    search_field_->setPlaceholderText("Search clinical records...");
    search_field_->setMinimumWidth(280);
    search_field_->setStyleSheet(Theme::searchFieldStyle());

    search_layout->addWidget(search_icon);
    search_layout->addWidget(search_field_);

    layout->addWidget(brand);
    layout->addWidget(search_shell, 0, Qt::AlignLeft);
    layout->addStretch(1);

    primary_action_->setStyleSheet(Theme::primaryButtonStyle());
    layout->addWidget(primary_action_);

    const QList<QStyle::StandardPixmap> icons = {
        QStyle::SP_MessageBoxInformation,
        QStyle::SP_FileDialogDetailedView,
        QStyle::SP_DialogHelpButton,
    };

    for (const auto icon_type : icons) {
        auto* button = new QToolButton(this);
        button->setIcon(style()->standardIcon(icon_type));
        button->setIconSize(QSize(18, 18));
        button->setStyleSheet(Theme::iconButtonStyle());
        layout->addWidget(button);
    }

    auto* profile = new QLabel("DR", this);
    profile->setAlignment(Qt::AlignCenter);
    profile->setFixedSize(36, 36);
    profile->setFont(Theme::headlineFont(10, QFont::Bold));
    profile->setStyleSheet(QString(
        "background-color: %1; color: %2; border-radius: 18px; border: 1px solid %3;"
    )
                               .arg(Theme::toHex(Theme::surfaceHighest()))
                               .arg(Theme::toHex(Theme::text()))
                               .arg(Theme::rgba(Theme::tertiary(), 100)));
    layout->addWidget(profile);
}

}  // namespace desktop
