import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FastSurferDesktop

Rectangle {
    id: root

    readonly property var entries: [
        { pageId: "dashboard", label: "Dashboard", glyph: "DB" },
        { pageId: "scan_viewer", label: "Scan Viewer", glyph: "SV" },
        { pageId: "pipelines", label: "Pipelines", glyph: "PL" },
        { pageId: "analytics", label: "Analytics", glyph: "AN" }
    ]

    implicitWidth: Theme.sideNavWidth
    color: Theme.surfaceContainerLow

    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: Qt.rgba(Theme.textMuted.r, Theme.textMuted.g, Theme.textMuted.b, 0.16)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingLg
        spacing: Theme.spacingLg

        ColumnLayout {
            spacing: 4

            Text {
                text: controller.projectName
                color: Theme.textPrimary
                font.family: Theme.displayFont
                font.pixelSize: 24
                font.weight: Font.Bold
            }

            Text {
                text: controller.projectContext
                color: Theme.textMuted
                font.family: Theme.bodyFont
                font.pixelSize: 12
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            Repeater {
                model: root.entries

                delegate: Rectangle {
                    required property var modelData

                    readonly property bool active: controller.navigation.currentPage === modelData.pageId

                    Layout.fillWidth: true
                    implicitHeight: 56
                    radius: Theme.radiusMd
                    color: active ? Theme.surfaceContainerHigh : "transparent"
                    border.width: active ? 1 : 0
                    border.color: Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.22)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMd
                        anchors.rightMargin: Theme.spacingMd
                        spacing: Theme.spacingMd

                        Rectangle {
                            Layout.preferredWidth: 34
                            Layout.preferredHeight: 34
                            radius: 17
                            color: active ? Theme.accentSurface("cyan") : Theme.surfaceContainer

                            Text {
                                anchors.centerIn: parent
                                text: modelData.glyph
                                color: active ? Theme.cyan : Theme.textSecondary
                                font.family: Theme.bodyFont
                                font.pixelSize: 11
                                font.weight: Font.Bold
                            }
                        }

                        Text {
                            text: modelData.label
                            color: active ? Theme.cyan : Theme.textSecondary
                            font.family: Theme.bodyFont
                            font.pixelSize: 14
                            font.weight: active ? Font.DemiBold : Font.Medium
                        }

                        Item { Layout.fillWidth: true }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: controller.navigation.setCurrentPage(modelData.pageId)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 54
            radius: Theme.radiusLg
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.cyan }
                GradientStop { position: 1.0; color: Theme.sky }
            }

            Text {
                anchors.centerIn: parent
                text: "New Processing Task"
                color: Theme.background
                font.family: Theme.bodyFont
                font.pixelSize: 14
                font.weight: Font.Bold
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
            }
        }

        SurfacePanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            panelColor: Theme.surfaceContainer

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLg
                spacing: Theme.spacingSm

                Text {
                    text: "System status"
                    color: Theme.textPrimary
                    font.family: Theme.displayFont
                    font.pixelSize: 18
                    font.weight: Font.Bold
                }

                Text {
                    text: "Inference nodes are synchronized and review capacity remains available for urgent cohorts."
                    color: Theme.textSecondary
                    font.family: Theme.bodyFont
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                PillChip {
                    text: "Stable"
                    accent: "success"
                }
            }
        }

        Item { Layout.fillHeight: true }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            Text {
                text: "Support"
                color: Theme.textMuted
                font.family: Theme.bodyFont
                font.pixelSize: 12
            }

            Text {
                text: "System Status"
                color: Theme.textSecondary
                font.family: Theme.bodyFont
                font.pixelSize: 13
            }

            Text {
                text: "Clinical Operations"
                color: Theme.textSecondary
                font.family: Theme.bodyFont
                font.pixelSize: 13
            }
        }
    }
}