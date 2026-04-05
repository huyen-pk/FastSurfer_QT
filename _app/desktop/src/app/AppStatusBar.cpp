#include "app/AppStatusBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

#include "app/Theme.h"

namespace desktop {

AppStatusBar::AppStatusBar(QWidget* parent) : QFrame(parent) {
    setObjectName("AppStatusBar");
    setFixedHeight(40);
    setStyleSheet(QString(
        "QFrame#AppStatusBar {"
        " background-color: %1;"
        " border-top: 1px solid %2;"
        "}"
    )
                      .arg(Theme::toHex(Theme::surfaceLowest()))
                      .arg(Theme::rgba(Theme::outline(), 120)));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 6, 24, 6);
    layout->setSpacing(20);

    auto make_status_label = [](const QString& text, const QColor& color) {
        auto* label = new QLabel(text);
        label->setFont(Theme::bodyFont(9, QFont::Medium));
        label->setStyleSheet(QString("color: %1;").arg(Theme::toHex(color)));
        return label;
    };

    layout->addWidget(make_status_label("Queue: 18", Theme::primary()));
    layout->addWidget(make_status_label("Active: 4", Theme::tertiary()));
    layout->addWidget(make_status_label("Logs", Theme::mutedText()));
    layout->addStretch(1);

    auto* load_label = new QLabel("GPU Load", this);
    load_label->setFont(Theme::bodyFont(8, QFont::Bold));
    load_label->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::mutedText())));

    auto* load_bar = new QProgressBar(this);
    load_bar->setTextVisible(false);
    load_bar->setFixedWidth(130);
    load_bar->setFixedHeight(10);
    load_bar->setValue(72);

    auto* load_value = new QLabel("72%", this);
    load_value->setFont(Theme::bodyFont(8, QFont::Bold));
    load_value->setStyleSheet(QString("color: %1;").arg(Theme::toHex(Theme::tertiary())));

    layout->addWidget(load_label);
    layout->addWidget(load_bar);
    layout->addWidget(load_value);
}

}  // namespace desktop
