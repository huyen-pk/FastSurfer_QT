import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FastSurferDesktop

Item {
    ScrollView {
        id: scrollView

        anchors.fill: parent
        clip: true

        Column {
            width: scrollView.availableWidth
            spacing: Theme.spacingXl

            SectionHeader {
                width: parent.width
                eyebrow: "Dashboard"
                title: "Clinical overview"
                subtitle: "Real-time monitoring of neuroimaging studies, operational throughput, and review readiness across the current research cohort."
            }

            Row {
                id: metricRow

                width: parent.width
                spacing: Theme.spacingMd

                Repeater {
                    model: controller.dashboardMetrics

                    delegate: MetricCard {
                        required property var modelData

                        width: (metricRow.width - metricRow.spacing * 2) / 3
                        title: modelData.title
                        value: modelData.value
                        note: modelData.note
                        accent: modelData.accent
                        glyph: modelData.glyph
                    }
                }
            }

            RowLayout {
                width: parent.width
                spacing: Theme.spacingLg

                SurfacePanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 470
                    panelColor: Theme.surfaceContainerLow

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingLg
                        spacing: Theme.spacingMd

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "Recent MRI studies"
                                color: Theme.textPrimary
                                font.family: Theme.displayFont
                                font.pixelSize: 22
                                font.weight: Font.Bold
                            }

                            Item { Layout.fillWidth: true }

                            PillChip {
                                text: "View all"
                                accent: "sky"
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Qt.rgba(Theme.textMuted.r, Theme.textMuted.g, Theme.textMuted.b, 0.16)
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Repeater {
                                model: ["Patient", "Protocol", "Status", "Time"]

                                delegate: Text {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    text: modelData.toUpperCase()
                                    color: Theme.textMuted
                                    font.family: Theme.bodyFont
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }
                            }
                        }

                        Repeater {
                            model: controller.studies

                            delegate: Rectangle {
                                required property var modelData
                                required property int index

                                Layout.fillWidth: true
                                implicitHeight: 72
                                radius: Theme.radiusMd
                                color: index % 2 === 0 ? Theme.surfaceContainer : Theme.surfaceContainerHigh

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.spacingMd
                                    anchors.rightMargin: Theme.spacingMd

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Text {
                                            text: modelData.patientId
                                            color: Theme.textPrimary
                                            font.family: Theme.bodyFont
                                            font.pixelSize: 14
                                            font.weight: Font.DemiBold
                                        }

                                        Text {
                                            text: modelData.series
                                            color: Theme.textSecondary
                                            font.family: Theme.bodyFont
                                            font.pixelSize: 12
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.protocol
                                        color: Theme.textSecondary
                                        font.family: Theme.bodyFont
                                        font.pixelSize: 13
                                        font.weight: Font.Medium
                                    }

                                    PillChip {
                                        Layout.fillWidth: true
                                        text: modelData.status
                                        accent: modelData.accent
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                        text: modelData.scheduledAt
                                        color: Theme.textSecondary
                                        font.family: Theme.bodyFont
                                        font.pixelSize: 13
                                    }
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: Theme.rightRailWidth
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.spacingLg

                    Repeater {
                        model: controller.operationalPanels

                        delegate: SurfacePanel {
                            required property var modelData

                            Layout.fillWidth: true
                            implicitHeight: 148
                            panelColor: Theme.surfaceContainer

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.spacingLg
                                spacing: Theme.spacingSm

                                PillChip {
                                    text: modelData.title
                                    accent: modelData.accent
                                }

                                Text {
                                    text: modelData.value
                                    color: Theme.textPrimary
                                    font.family: Theme.displayFont
                                    font.pixelSize: 28
                                    font.weight: Font.Bold
                                }

                                Text {
                                    text: modelData.detail
                                    color: Theme.textSecondary
                                    font.family: Theme.bodyFont
                                    font.pixelSize: 13
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}