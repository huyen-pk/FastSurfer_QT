import QtQuick
import QtQuick.Layouts
import FastSurferDesktop

SurfacePanel {
    id: root

    property string title: ""
    property string value: ""
    property string note: ""
    property string accent: "sky"
    property string glyph: ""

    implicitHeight: 180
    panelColor: Theme.surfaceContainer

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingLg
        spacing: Theme.spacingMd

        RowLayout {
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: 46
                Layout.preferredHeight: 46
                radius: Theme.radiusMd
                color: Theme.accentSurface(root.accent)

                Text {
                    anchors.centerIn: parent
                    text: root.glyph
                    color: Theme.accentColor(root.accent)
                    font.family: Theme.bodyFont
                    font.pixelSize: 14
                    font.weight: Font.Bold
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.title.toUpperCase()
                color: Theme.textMuted
                font.family: Theme.bodyFont
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }

        Text {
            text: root.value
            color: Theme.textPrimary
            font.family: Theme.displayFont
            font.pixelSize: 40
            font.weight: Font.ExtraBold
        }

        Text {
            text: root.note
            color: Theme.accentColor(root.accent)
            font.family: Theme.bodyFont
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }
    }
}