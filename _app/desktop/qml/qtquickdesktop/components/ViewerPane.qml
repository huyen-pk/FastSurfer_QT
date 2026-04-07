import QtQuick
import QtQuick.Layouts
import FastSurferDesktop

Rectangle {
    id: root

    property string title: ""
    property string coordinate: ""
    property string note: ""
    property string accent: "cyan"
    property bool highlighted: false

    radius: Theme.radiusMd
    color: Theme.viewerSurface
    border.width: highlighted ? 1 : 0
    border.color: highlighted ? Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.35) : "transparent"
    clip: true

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.04) }
            GradientStop { position: 0.5; color: Qt.rgba(0 / 255, 218 / 255, 243 / 255, highlighted ? 0.08 : 0.03) }
            GradientStop { position: 1.0; color: Qt.rgba(161 / 255, 204 / 255, 237 / 255, 0.02) }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.62
        height: parent.height * 0.62
        radius: width / 2
        color: Qt.rgba(1, 1, 1, 0.025)
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Qt.rgba(Theme.textMuted.r, Theme.textMuted.g, Theme.textMuted.b, 0.18)
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: 1
        color: Qt.rgba(Theme.textMuted.r, Theme.textMuted.g, Theme.textMuted.b, 0.18)
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Theme.spacingMd
        spacing: 4

        Text {
            text: root.title.toUpperCase()
            color: Theme.accentColor(root.accent)
            font.family: Theme.bodyFont
            font.pixelSize: 11
            font.weight: Font.Bold
        }

        Text {
            text: root.coordinate
            color: Theme.textMuted
            font.family: Theme.bodyFont
            font.pixelSize: 11
        }
    }

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacingMd
        text: root.note
        color: Qt.rgba(Theme.textPrimary.r, Theme.textPrimary.g, Theme.textPrimary.b, 0.78)
        font.family: Theme.bodyFont
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
}