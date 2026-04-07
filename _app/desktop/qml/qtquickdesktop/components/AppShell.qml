import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FastSurferDesktop

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TopBar {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            SideNav {
                Layout.fillHeight: true
                Layout.preferredWidth: Theme.sideNavWidth
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Loader {
                    id: pageLoader

                    anchors.fill: parent
                    anchors.margins: Theme.shellPadding
                    sourceComponent: {
                        switch (controller.navigation.currentPage) {
                        case "scan_viewer":
                            return scanViewerPage;
                        case "pipelines":
                            return pipelinesPage;
                        case "analytics":
                            return analyticsPage;
                        default:
                            return dashboardPage;
                        }
                    }
                }

                Component { id: dashboardPage; DashboardPage {} }
                Component { id: scanViewerPage; ScanViewerPage {} }
                Component { id: pipelinesPage; PipelinesPage {} }
                Component { id: analyticsPage; AnalyticsPage {} }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 38
            color: Theme.viewerSurface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.shellPadding
                anchors.rightMargin: Theme.shellPadding
                spacing: Theme.spacingLg

                Text {
                    text: "Queue 2"
                    color: Theme.textMuted
                    font.family: Theme.bodyFont
                    font.pixelSize: 12
                }

                Text {
                    text: "Active"
                    color: Theme.cyan
                    font.family: Theme.bodyFont
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "GPU load 42%"
                    color: Theme.textMuted
                    font.family: Theme.bodyFont
                    font.pixelSize: 12
                }
            }
        }
    }
}