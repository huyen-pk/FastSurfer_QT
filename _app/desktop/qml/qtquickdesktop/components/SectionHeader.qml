import QtQuick
import QtQuick.Layouts
import FastSurferDesktop

ColumnLayout {
    id: root

    property string eyebrow: ""
    property string title: ""
    property string subtitle: ""

    spacing: 6

    Text {
        visible: root.eyebrow.length > 0
        text: root.eyebrow.toUpperCase()
        color: Theme.textMuted
        font.family: Theme.bodyFont
        font.pixelSize: 11
        font.weight: Font.DemiBold
    }

    Text {
        text: root.title
        color: Theme.textPrimary
        font.family: Theme.displayFont
        font.pixelSize: 34
        font.weight: Font.ExtraBold
        wrapMode: Text.WordWrap
    }

    Text {
        text: root.subtitle
        color: Theme.textSecondary
        font.family: Theme.bodyFont
        font.pixelSize: 15
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}