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

            RowLayout {
                width: parent.width

                SectionHeader {
                    Layout.fillWidth: true
                    eyebrow: "Analytics"
                    title: "Comparative study analysis"
                    subtitle: "Volumetric deltas remain aligned to the cohort baseline with export-ready summaries for post-operative review."
                }

                PillChip {
                    text: "Generate PDF report"
                    accent: "cyan"
                    solid: true
                }
            }

            RowLayout {
                width: parent.width
                spacing: Theme.spacingLg

                SurfacePanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 430
                    panelColor: Theme.surfaceContainer

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingLg
                        spacing: Theme.spacingLg

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "Volumetric delta"
                                color: Theme.textPrimary
                                font.family: Theme.displayFont
                                font.pixelSize: 22
                                font.weight: Font.Bold
                            }

                            Item { Layout.fillWidth: true }

                            PillChip { text: "Baseline"; accent: "sky" }
                            PillChip { text: "Current"; accent: "cyan" }
                        }

                        AnalyticsBarChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            bars: controller.analyticsBars
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: Theme.rightRailWidth
                    spacing: Theme.spacingLg

                    Repeater {
                        model: controller.analyticsCards

                        delegate: MetricCard {
                            required property var modelData

                            Layout.fillWidth: true
                            title: modelData.title
                            value: modelData.value
                            note: modelData.note
                            accent: modelData.accent
                            glyph: modelData.glyph
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: Theme.spacingLg

                Repeater {
                    model: controller.analyticsInsights

                    delegate: SurfacePanel {
                        required property var modelData

                        width: (parent.width - Theme.spacingLg) / 2
                        implicitHeight: 170
                        panelColor: Theme.surfaceContainerHigh

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingLg
                            spacing: Theme.spacingSm

                            Text {
                                text: modelData.title
                                color: Theme.textPrimary
                                font.family: Theme.displayFont
                                font.pixelSize: 20
                                font.weight: Font.Bold
                            }

                            Text {
                                text: modelData.detail
                                color: Theme.textSecondary
                                font.family: Theme.bodyFont
                                font.pixelSize: 14
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