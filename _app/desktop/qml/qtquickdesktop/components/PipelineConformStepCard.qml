import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FastSurferDesktop

SurfacePanel {
    id: root

    panelColor: Theme.surfaceContainerHigh
    implicitHeight: contentColumn.implicitHeight + Theme.spacingLg * 2

    ColumnLayout {
        id: contentColumn

        anchors.fill: parent
        anchors.margins: Theme.spacingLg
        spacing: Theme.spacingLg

        property var result: controller.conformStepResult
        property bool success: Boolean(result.success)
        property string accentName: result.status === "success" ? "success" : (result.status === "failed" ? "amber" : "sky")

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLg

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "pipeline_conform_and_save_orig"
                    color: Theme.textPrimary
                    font.family: Theme.displayFont
                    font.pixelSize: 24
                    font.weight: Font.Bold
                }

                Text {
                    text: "Native MGZ step in _app/core that mirrors FastSurfer's original-image load, optional copy, conformance check, and conformed-output write for the approved Subject140 fixture."
                    color: Theme.textSecondary
                    font.family: Theme.bodyFont
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            PillChip {
                text: contentColumn.result.statusLabel || "Idle"
                accent: contentColumn.accentName
                solid: contentColumn.result.status === "success"
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMd

            Text {
                text: "Input MGZ"
                color: Theme.textMuted
                font.family: Theme.bodyFont
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }

            TextField {
                Layout.fillWidth: true
                text: controller.conformStepInputPath
                color: Theme.textPrimary
                font.family: Theme.bodyFont
                selectByMouse: true
                onTextEdited: controller.conformStepInputPath = text

                background: Rectangle {
                    radius: Theme.radiusMd
                    color: Theme.surfaceContainer
                    border.width: 1
                    border.color: Theme.ghostBorder
                }
            }

            Text {
                text: "Copy Original Output"
                color: Theme.textMuted
                font.family: Theme.bodyFont
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }

            TextField {
                Layout.fillWidth: true
                text: controller.conformStepCopyOrigPath
                color: Theme.textPrimary
                font.family: Theme.bodyFont
                selectByMouse: true
                onTextEdited: controller.conformStepCopyOrigPath = text

                background: Rectangle {
                    radius: Theme.radiusMd
                    color: Theme.surfaceContainer
                    border.width: 1
                    border.color: Theme.ghostBorder
                }
            }

            Text {
                text: "Conformed Output"
                color: Theme.textMuted
                font.family: Theme.bodyFont
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }

            TextField {
                Layout.fillWidth: true
                text: controller.conformStepConformedPath
                color: Theme.textPrimary
                font.family: Theme.bodyFont
                selectByMouse: true
                onTextEdited: controller.conformStepConformedPath = text

                background: Rectangle {
                    radius: Theme.radiusMd
                    color: Theme.surfaceContainer
                    border.width: 1
                    border.color: Theme.ghostBorder
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            Button {
                id: runButton

                text: "Run Native Step"
                onClicked: controller.runConformStep()

                background: Rectangle {
                    radius: Theme.radiusMd
                    color: Theme.cyan
                }

                contentItem: Text {
                    text: runButton.text
                    color: Theme.background
                    font.family: Theme.bodyFont
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                id: restoreButton

                text: "Restore Defaults"
                onClicked: controller.restoreConformStepDefaults()

                background: Rectangle {
                    radius: Theme.radiusMd
                    color: Theme.surfaceContainer
                    border.width: 1
                    border.color: Theme.ghostBorder
                }

                contentItem: Text {
                    text: restoreButton.text
                    color: Theme.textPrimary
                    font.family: Theme.bodyFont
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: contentColumn.result.alreadyConformed ? "Already conformed" : "Needs verification"
                color: Theme.accentColor(contentColumn.accentName)
                font.family: Theme.bodyFont
                font.pixelSize: 13
                font.weight: Font.DemiBold
                visible: contentColumn.result.status === "success" || contentColumn.result.status === "failed"
            }
        }

        SurfacePanel {
            Layout.fillWidth: true
            implicitHeight: metadataColumn.implicitHeight + Theme.spacingLg * 2
            panelColor: Theme.surfaceContainer

            ColumnLayout {
                id: metadataColumn

                anchors.fill: parent
                anchors.margins: Theme.spacingLg
                spacing: Theme.spacingMd

                Text {
                    text: "Execution Summary"
                    color: Theme.textPrimary
                    font.family: Theme.displayFont
                    font.pixelSize: 20
                    font.weight: Font.Bold
                }

                Text {
                    text: contentColumn.result.message || "No execution has been recorded yet."
                    color: Theme.textSecondary
                    font.family: Theme.bodyFont
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: Theme.spacingLg
                    rowSpacing: Theme.spacingMd

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "Input Metadata"
                            color: Theme.textMuted
                            font.family: Theme.bodyFont
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "Dims: " + (contentColumn.result.inputMetadata.dimensions || "-")
                            color: Theme.textPrimary
                            font.family: Theme.bodyFont
                            font.pixelSize: 13
                        }

                        Text {
                            text: "Spacing: " + (contentColumn.result.inputMetadata.spacing || "-")
                            color: Theme.textPrimary
                            font.family: Theme.bodyFont
                            font.pixelSize: 13
                        }

                        Text {
                            text: "Orientation: " + (contentColumn.result.inputMetadata.orientation || "-")
                            color: Theme.textPrimary
                            font.family: Theme.bodyFont
                            font.pixelSize: 13
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "Output Metadata"
                            color: Theme.textMuted
                            font.family: Theme.bodyFont
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "Dims: " + (contentColumn.result.outputMetadata.dimensions || "-")
                            color: Theme.textPrimary
                            font.family: Theme.bodyFont
                            font.pixelSize: 13
                        }

                        Text {
                            text: "Spacing: " + (contentColumn.result.outputMetadata.spacing || "-")
                            color: Theme.textPrimary
                            font.family: Theme.bodyFont
                            font.pixelSize: 13
                        }

                        Text {
                            text: "Orientation: " + (contentColumn.result.outputMetadata.orientation || "-")
                            color: Theme.textPrimary
                            font.family: Theme.bodyFont
                            font.pixelSize: 13
                        }
                    }
                }

                Text {
                    text: "Copy orig: " + (contentColumn.result.copyOrigPath || controller.conformStepCopyOrigPath)
                    color: Theme.textMuted
                    font.family: Theme.bodyFont
                    font.pixelSize: 12
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }

                Text {
                    text: "Conformed: " + (contentColumn.result.conformedPath || controller.conformStepConformedPath)
                    color: Theme.textMuted
                    font.family: Theme.bodyFont
                    font.pixelSize: 12
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
            }
        }
    }
}