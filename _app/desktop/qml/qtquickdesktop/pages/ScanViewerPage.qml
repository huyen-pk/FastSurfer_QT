import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FastSurferDesktop

Item {
    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacingLg

        SurfacePanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelColor: Theme.viewerSurface

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLg
                spacing: Theme.spacingMd

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "Scan viewer"
                            color: Theme.textPrimary
                            font.family: Theme.displayFont
                            font.pixelSize: 30
                            font.weight: Font.ExtraBold
                        }

                        Text {
                            text: "Patient JX-9802, multi-view review with segmentation-ready placeholders and study metadata rail."
                            color: Theme.textSecondary
                            font.family: Theme.bodyFont
                            font.pixelSize: 14
                        }
                    }

                    RowLayout {
                        spacing: Theme.spacingSm

                        PillChip {
                            text: "PATIENT JX-9802"
                            accent: "cyan"
                        }

                        PillChip {
                            text: "Multi-view"
                            accent: "sky"
                            solid: true
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 2
                    rowSpacing: Theme.spacingSm
                    columnSpacing: Theme.spacingSm

                    Repeater {
                        model: controller.viewerPanels

                        delegate: ViewerPane {
                            required property var modelData

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: modelData.title
                            coordinate: modelData.coordinate
                            note: modelData.note
                            accent: modelData.accent
                            highlighted: modelData.highlighted
                        }
                    }
                }
            }
        }

        SurfacePanel {
            Layout.preferredWidth: Theme.rightRailWidth
            Layout.fillHeight: true
            panelColor: Theme.surfaceContainer

            ScrollView {
                anchors.fill: parent
                clip: true

                Column {
                    width: parent.width
                    spacing: Theme.spacingLg
                    padding: Theme.spacingLg

                    Text {
                        text: "Scan information"
                        color: Theme.textPrimary
                        font.family: Theme.displayFont
                        font.pixelSize: 22
                        font.weight: Font.Bold
                    }

                    SurfacePanel {
                        width: parent.width - Theme.spacingLg * 2
                        implicitHeight: 260
                        panelColor: Theme.surfaceContainerHigh

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingLg
                            columns: 2
                            rowSpacing: Theme.spacingMd
                            columnSpacing: Theme.spacingMd

                            Repeater {
                                model: controller.scanMetadata

                                delegate: ColumnLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Text {
                                        text: modelData.label.toUpperCase()
                                        color: Theme.textMuted
                                        font.family: Theme.bodyFont
                                        font.pixelSize: 10
                                        font.weight: Font.DemiBold
                                    }

                                    Text {
                                        text: modelData.value
                                        color: Theme.textPrimary
                                        font.family: Theme.bodyFont
                                        font.pixelSize: 13
                                        font.weight: Font.Medium
                                    }
                                }
                            }
                        }
                    }

                    Repeater {
                        model: controller.studyNotes

                        delegate: SurfacePanel {
                            required property var modelData

                            width: parent.width - Theme.spacingLg * 2
                            implicitHeight: 78
                            panelColor: Theme.surfaceContainerHigh

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.spacingMd
                                spacing: 4

                                Text {
                                    text: modelData.title.toUpperCase()
                                    color: Theme.textMuted
                                    font.family: Theme.bodyFont
                                    font.pixelSize: 10
                                    font.weight: Font.DemiBold
                                }

                                Text {
                                    text: modelData.value
                                    color: Theme.textPrimary
                                    font.family: Theme.bodyFont
                                    font.pixelSize: 13
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}