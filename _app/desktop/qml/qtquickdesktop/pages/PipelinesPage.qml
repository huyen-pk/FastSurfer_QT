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
                    eyebrow: "Pipelines"
                    title: "Automated pipeline overview"
                    subtitle: "Sequence 4429 remains tuned for research throughput, with motion correction prioritized before segmentation and reporting steps."
                }

                RowLayout {
                    spacing: Theme.spacingSm

                    PillChip {
                        text: "Duplicate"
                        accent: "sky"
                    }

                    PillChip {
                        text: "Execute sequence"
                        accent: "cyan"
                        solid: true
                    }
                }
            }

            Column {
                width: parent.width * 0.78
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.spacingXl

                Column {
                    width: parent.width
                    spacing: 0

                    Repeater {
                        model: controller.pipelineSteps

                        delegate: Column {
                            required property var modelData

                            width: parent.width
                            spacing: 0

                            SurfacePanel {
                                width: parent.width
                                implicitHeight: 136
                                panelColor: modelData.active ? Theme.surfaceContainerHigh : Theme.surfaceContainer

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingLg
                                    spacing: Theme.spacingLg

                                    Rectangle {
                                        Layout.preferredWidth: 54
                                        Layout.preferredHeight: 54
                                        radius: Theme.radiusMd
                                        color: Theme.accentSurface(modelData.accent)

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.badge.substring(0, 2).toUpperCase()
                                            color: Theme.accentColor(modelData.accent)
                                            font.family: Theme.bodyFont
                                            font.pixelSize: 12
                                            font.weight: Font.Bold
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Text {
                                            text: modelData.title
                                            color: Theme.textPrimary
                                            font.family: Theme.displayFont
                                            font.pixelSize: 24
                                            font.weight: Font.Bold
                                        }

                                        Text {
                                            text: modelData.subtitle
                                            color: Theme.textSecondary
                                            font.family: Theme.bodyFont
                                            font.pixelSize: 14
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: Theme.spacingSm
                                        Layout.alignment: Qt.AlignTop

                                        PillChip {
                                            text: modelData.badge
                                            accent: modelData.accent
                                        }

                                        Text {
                                            text: modelData.state
                                            color: Theme.accentColor(modelData.accent)
                                            font.family: Theme.bodyFont
                                            font.pixelSize: 13
                                            font.weight: Font.DemiBold
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                visible: index < controller.pipelineSteps.length - 1
                                width: 2
                                height: 44
                                x: 81
                                color: Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.24)
                            }
                        }
                    }
                }

                SectionHeader {
                    width: parent.width
                    eyebrow: "Native step"
                    title: "Desktop parity for pipeline_conform_and_save_orig"
                    subtitle: "This card executes the approved _app/core MGZ step and exposes the same original-copy and conformed-output behavior targeted by the Python FastSurfer pipeline."
                }

                PipelineConformStepCard {
                    width: parent.width
                }
            }
        }
    }
}