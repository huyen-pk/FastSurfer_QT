#include "app/Theme.h"

#include <QPalette>

namespace {

const QColor kBackground("#0a151a");
const QColor kSurfaceLowest("#051015");
const QColor kSurfaceLow("#121d23");
const QColor kSurface("#162127");
const QColor kSurfaceHigh("#202b32");
const QColor kSurfaceHighest("#2b363d");
const QColor kPrimary("#a1cced");
const QColor kTertiary("#00daf3");
const QColor kSecondary("#afc9ea");
const QColor kError("#ff8d84");
const QColor kText("#d8e4ec");
const QColor kMutedText("#93a8b8");
const QColor kOutline("#324149");

QFont makeFont(const QStringList& families, int point_size, int weight) {
    QFont font;
    font.setFamilies(families);
    font.setPointSize(point_size);
    font.setWeight(static_cast<QFont::Weight>(weight));
    return font;
}

}  // namespace

namespace desktop::Theme {

void apply(QApplication& app) {
    app.setStyle("Fusion");

    QPalette palette;
    palette.setColor(QPalette::Window, background());
    palette.setColor(QPalette::Base, surfaceLowest());
    palette.setColor(QPalette::AlternateBase, surfaceLow());
    palette.setColor(QPalette::Button, surface());
    palette.setColor(QPalette::ButtonText, text());
    palette.setColor(QPalette::Text, text());
    palette.setColor(QPalette::WindowText, text());
    palette.setColor(QPalette::BrightText, tertiary());
    palette.setColor(QPalette::Highlight, tertiary());
    palette.setColor(QPalette::HighlightedText, QColor("#00363d"));
    app.setPalette(palette);
    app.setFont(bodyFont(10));

    app.setStyleSheet(QString(
        "QToolTip {"
        " color: %1;"
        " background-color: %2;"
        " border: 1px solid %3;"
        " padding: 6px;"
        "}"
        "QMainWindow, QWidget#AppRoot {"
        " background-color: %4;"
        " color: %1;"
        "}"
        "QScrollArea, QAbstractScrollArea {"
        " background: transparent;"
        " border: none;"
        "}"
        "QHeaderView::section {"
        " background-color: %5;"
        " color: %6;"
        " border: none;"
        " padding: 12px 14px;"
        " font-size: 10px;"
        " font-weight: 700;"
        "}"
        "QTableCornerButton::section {"
        " background-color: %5;"
        " border: none;"
        "}"
        "QSplitter::handle {"
        " background-color: %3;"
        "}"
        "QScrollBar:vertical {"
        " background: %4;"
        " width: 10px;"
        " margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        " background: %3;"
        " min-height: 30px;"
        " border-radius: 5px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        " background: none;"
        " height: 0px;"
        "}"
        "QProgressBar {"
        " background-color: %5;"
        " border: none;"
        " border-radius: 5px;"
        " text-align: center;"
        " color: %1;"
        "}"
        "QProgressBar::chunk {"
        " background-color: %7;"
        " border-radius: 5px;"
        "}"
        )
            .arg(toHex(text()))
            .arg(toHex(surfaceHigh()))
            .arg(toHex(outline()))
            .arg(toHex(background()))
            .arg(toHex(surfaceLow()))
            .arg(toHex(mutedText()))
            .arg(toHex(tertiary())));
}

QColor background() { return kBackground; }
QColor surfaceLowest() { return kSurfaceLowest; }
QColor surfaceLow() { return kSurfaceLow; }
QColor surface() { return kSurface; }
QColor surfaceHigh() { return kSurfaceHigh; }
QColor surfaceHighest() { return kSurfaceHighest; }
QColor primary() { return kPrimary; }
QColor tertiary() { return kTertiary; }
QColor secondary() { return kSecondary; }
QColor error() { return kError; }
QColor text() { return kText; }
QColor mutedText() { return kMutedText; }
QColor outline() { return kOutline; }

QString toHex(const QColor& color) {
    return color.name(QColor::HexRgb);
}

QString rgba(const QColor& color, int alpha) {
    return QString("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha);
}

QFont headlineFont(int point_size, int weight) {
    return makeFont({"Manrope", "Inter", "Segoe UI", "Arial"}, point_size, weight);
}

QFont bodyFont(int point_size, int weight) {
    return makeFont({"Inter", "Segoe UI", "Arial"}, point_size, weight);
}

QString cardStyle(const QColor& background_color, int radius, const QColor& border_color) {
    const QColor effective_border = border_color.isValid() ? border_color : QColor(0, 0, 0, 0);
    return QString(
        "background-color: %1;"
        "border: 1px solid %2;"
        "border-radius: %3px;"
    )
        .arg(toHex(background_color))
        .arg(border_color.isValid() ? rgba(effective_border, 120) : QString("transparent"))
        .arg(radius);
}

QString primaryButtonStyle() {
    return QString(
        "QPushButton {"
        " background-color: %1;"
        " color: #00363d;"
        " border: none;"
        " border-radius: 14px;"
        " padding: 12px 18px;"
        " font-size: 12px;"
        " font-weight: 700;"
        "}"
        "QPushButton:hover { background-color: %2; }"
        "QPushButton:pressed { background-color: %3; }"
    )
        .arg(QString("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2)")
                 .arg(toHex(tertiary()))
                 .arg(toHex(primary())))
        .arg(toHex(QColor("#38e4f7")))
        .arg(toHex(QColor("#87bedf")));
}

QString secondaryButtonStyle() {
    return QString(
        "QPushButton {"
        " background-color: transparent;"
        " color: %1;"
        " border: 1px solid %2;"
        " border-radius: 12px;"
        " padding: 10px 16px;"
        " font-size: 11px;"
        " font-weight: 600;"
        "}"
        "QPushButton:hover { background-color: %3; color: %4; }"
    )
        .arg(toHex(primary()))
        .arg(rgba(outline(), 120))
        .arg(toHex(surfaceHigh()))
        .arg(toHex(text()));
}

QString iconButtonStyle() {
    return QString(
        "QToolButton {"
        " background-color: transparent;"
        " color: %1;"
        " border: none;"
        " border-radius: 12px;"
        " padding: 8px;"
        "}"
        "QToolButton:hover { background-color: %2; }"
    )
        .arg(toHex(primary()))
        .arg(toHex(surfaceHigh()));
}

QString navigationButtonStyle(bool active) {
    const QString background_color = active ? toHex(surfaceHigh()) : QString("transparent");
    const QString text_color = active ? toHex(tertiary()) : toHex(primary());
    const QString border_color = active ? toHex(tertiary()) : QString("transparent");
    return QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " text-align: left;"
        " padding: 12px 14px;"
        " border: none;"
        " border-left: 3px solid %3;"
        " border-radius: 14px;"
        " font-size: 12px;"
        " font-weight: 600;"
        "}"
        "QPushButton:hover { background-color: %4; color: %5; }"
    )
        .arg(background_color)
        .arg(text_color)
        .arg(border_color)
        .arg(toHex(surfaceHigh()))
        .arg(toHex(text()));
}

QString searchFieldStyle() {
    return QString(
        "QLineEdit {"
        " background-color: transparent;"
        " color: %1;"
        " border: none;"
        " padding: 4px;"
        " font-size: 11px;"
        "}"
        "QLineEdit::placeholder { color: %2; }"
    )
        .arg(toHex(text()))
        .arg(toHex(mutedText()));
}

QString tableStyle() {
    return QString(
        "QTableWidget {"
        " background-color: transparent;"
        " color: %1;"
        " border: none;"
        " alternate-background-color: %2;"
        " selection-background-color: %3;"
        "}"
        "QTableWidget::item { padding: 12px; }"
    )
        .arg(toHex(text()))
        .arg(toHex(surfaceHigh()))
        .arg(rgba(tertiary(), 80));
}

QString chipStyle(const QColor& background_color, const QColor& text_color) {
    return QString(
        "background-color: %1;"
        "color: %2;"
        "border-radius: 10px;"
        "padding: 4px 8px;"
        "font-size: 10px;"
        "font-weight: 700;"
    )
        .arg(toHex(background_color))
        .arg(toHex(text_color));
}

}  // namespace desktop::Theme
