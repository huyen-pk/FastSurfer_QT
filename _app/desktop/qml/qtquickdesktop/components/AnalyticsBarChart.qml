import QtQuick
import QtQuick.Layouts
import FastSurferDesktop

Item {
    id: root

    property var bars: []

    implicitHeight: 320

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacingLg

        Repeater {
            model: root.bars

            delegate: Item {
                required property var modelData

                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.spacingSm

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Row {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Rectangle {
                                width: (parent.width - 8) / 2
                                height: Math.max(12, parent.height * (modelData.baseline / 100.0))
                                radius: 8
                                anchors.bottom: parent.bottom
                                color: Qt.rgba(0 / 255, 218 / 255, 243 / 255, 0.25)
                            }

                            Rectangle {
                                width: (parent.width - 8) / 2
                                height: Math.max(12, parent.height * (modelData.current / 100.0))
                                radius: 8
                                anchors.bottom: parent.bottom
                                color: Theme.cyan
                            }
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.label
                        color: Theme.textSecondary
                        font.family: Theme.bodyFont
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }
            }
        }
    }
}