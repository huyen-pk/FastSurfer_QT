pragma Singleton

import QtQuick

QtObject {
    readonly property color background: "#0a151a"
    readonly property color backgroundGlow: "#13232b"
    readonly property color surfaceLow: "#0d181d"
    readonly property color surfaceContainerLow: "#121d23"
    readonly property color surfaceContainer: "#162127"
    readonly property color surfaceContainerHigh: "#202b32"
    readonly property color surfaceContainerHighest: "#2b363d"
    readonly property color viewerSurface: "#051015"
    readonly property color textPrimary: "#d8e4ec"
    readonly property color textSecondary: "#a7bcc8"
    readonly property color textMuted: "#6f8792"
    readonly property color cyan: "#00daf3"
    readonly property color sky: "#a1cced"
    readonly property color amber: "#f3c56d"
    readonly property color success: "#8ad39a"
    readonly property color ghostBorder: "#26404d"
    readonly property color shadowColor: "#000000"

    readonly property string displayFont: "Manrope"
    readonly property string bodyFont: "Inter"

    readonly property int spacingXs: 8
    readonly property int spacingSm: 12
    readonly property int spacingMd: 16
    readonly property int spacingLg: 24
    readonly property int spacingXl: 32
    readonly property int spacingXxl: 40

    readonly property real radiusSm: 10
    readonly property real radiusMd: 16
    readonly property real radiusLg: 22
    readonly property real radiusPill: 999

    readonly property int shellPadding: 28
    readonly property int sideNavWidth: 272
    readonly property int rightRailWidth: 320

    function accentColor(name) {
        if (name === "cyan") {
            return cyan;
        }

        if (name === "amber") {
            return amber;
        }

        if (name === "success") {
            return success;
        }

        return sky;
    }

    function accentSurface(name) {
        if (name === "cyan") {
            return Qt.rgba(0 / 255, 218 / 255, 243 / 255, 0.12);
        }

        if (name === "amber") {
            return Qt.rgba(243 / 255, 197 / 255, 109 / 255, 0.14);
        }

        if (name === "success") {
            return Qt.rgba(138 / 255, 211 / 255, 154 / 255, 0.14);
        }

        return Qt.rgba(161 / 255, 204 / 255, 237 / 255, 0.12);
    }
}