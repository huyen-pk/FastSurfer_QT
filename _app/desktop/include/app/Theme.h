#pragma once

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QString>

namespace desktop::Theme {

void apply(QApplication& app);

QColor background();
QColor surfaceLowest();
QColor surfaceLow();
QColor surface();
QColor surfaceHigh();
QColor surfaceHighest();
QColor primary();
QColor tertiary();
QColor secondary();
QColor error();
QColor text();
QColor mutedText();
QColor outline();

QString toHex(const QColor& color);
QString rgba(const QColor& color, int alpha);

QFont headlineFont(int pointSize, int weight = QFont::Bold);
QFont bodyFont(int pointSize, int weight = QFont::Normal);

QString cardStyle(const QColor& backgroundColor, int radius = 18, const QColor& borderColor = QColor());
QString primaryButtonStyle();
QString secondaryButtonStyle();
QString iconButtonStyle();
QString navigationButtonStyle(bool active);
QString searchFieldStyle();
QString tableStyle();
QString chipStyle(const QColor& backgroundColor, const QColor& textColor);

}  // namespace desktop::Theme
