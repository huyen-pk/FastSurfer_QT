import QtQuick
import QtQuick.Controls
import FastSurferDesktop

Rectangle {
    id: root

    property string text: ""
    property string accent: "sky"
    property bool solid: false

    implicitHeight: 28
    implicitWidth: label.implicitWidth + 22
    radius: Theme.radiusPill
    color: solid ? Theme.accentColor(accent) : Theme.accentSurface(accent)
    border.width: solid ? 0 : 1
    border.color: Qt.rgba(Theme.accentColor(accent).r, Theme.accentColor(accent).g, Theme.accentColor(accent).b, 0.35)

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: solid ? Theme.background : Theme.accentColor(accent)
        font.family: Theme.bodyFont
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }
}