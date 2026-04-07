import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FastSurferDesktop

ApplicationWindow {
    id: window

    width: 1600
    height: 980
    minimumWidth: 1360
    minimumHeight: 820
    visible: true
    title: controller.applicationName
    color: Theme.background

    background: Rectangle {
        color: Theme.background

        Rectangle {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.8
            height: 240
            radius: height / 2
            color: Qt.rgba(0 / 255, 218 / 255, 243 / 255, 0.06)
            y: -140
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 420
            height: 420
            radius: width / 2
            color: Qt.rgba(161 / 255, 204 / 255, 237 / 255, 0.05)
            x: parent.width - width * 0.7
            y: parent.height - height * 0.55
        }
    }

    AppShell {
        anchors.fill: parent
    }
}