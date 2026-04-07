import QtQuick
import FastSurferDesktop

Rectangle {
    id: root

    property color panelColor: Theme.surfaceContainer
    property color outlineColor: Theme.ghostBorder
    property real panelRadius: Theme.radiusLg
    default property alias contentData: contentItem.data

    radius: panelRadius
    color: panelColor
    border.width: 1
    border.color: outlineColor

    layer.enabled: true
    layer.samples: 4

    Item {
        id: contentItem
        anchors.fill: parent
    }
}