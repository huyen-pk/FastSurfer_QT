import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FastSurferDesktop

Rectangle {
    id: root

    implicitHeight: 78
    color: Theme.background
    border.width: 0

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Qt.rgba(Theme.textMuted.r, Theme.textMuted.g, Theme.textMuted.b, 0.18)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.shellPadding
        anchors.rightMargin: Theme.shellPadding
        spacing: Theme.spacingLg

        ColumnLayout {
            spacing: 2

            Text {
                text: controller.applicationName
                color: Theme.cyan
                font.family: Theme.displayFont
                font.pixelSize: 24
                font.weight: Font.ExtraBold
            }

            Text {
                text: controller.productName
                color: Theme.textMuted
                font.family: Theme.bodyFont
                font.pixelSize: 12
            }
        }

        Rectangle {
            Layout.preferredWidth: 320
            Layout.preferredHeight: 42
            radius: Theme.radiusPill
            color: Theme.surfaceContainerHigh
            border.width: 1
            border.color: Qt.rgba(Theme.textMuted.r, Theme.textMuted.g, Theme.textMuted.b, 0.14)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingMd
                anchors.rightMargin: Theme.spacingMd

                Text {
                    text: "Search studies"
                    color: Theme.textMuted
                    font.family: Theme.bodyFont
                    font.pixelSize: 13
                }

                Item { Layout.fillWidth: true }

                PillChip {
                    text: "CTRL K"
                    accent: "sky"
                }
            }
        }

        Item { Layout.fillWidth: true }

        Repeater {
            model: [
                { label: "Alerts", accent: "cyan" },
                { label: "Nodes", accent: "sky" },
                { label: "Support", accent: "sky" }
            ]

            delegate: PillChip {
                required property var modelData
                text: modelData.label
                accent: modelData.accent
            }
        }

        Rectangle {
            Layout.preferredWidth: 42
            Layout.preferredHeight: 42
            radius: 21
            color: Theme.surfaceContainerHigh
            border.width: 1
            border.color: Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.22)

            Text {
                anchors.centerIn: parent
                text: "DR"
                color: Theme.textPrimary
                font.family: Theme.bodyFont
                font.pixelSize: 12
                font.weight: Font.Bold
            }
        }
    }
}