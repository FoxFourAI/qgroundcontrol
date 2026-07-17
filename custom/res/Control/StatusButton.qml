import QtQuick

import QGroundControl
import QGroundControl.Controls

Button {
    id: statusButton

    state: "ok"
    property int statusIndex: 0
    property alias textField: field
    property alias backgroundRect: bg
    MouseArea{
        anchors.fill:parent
        acceptedButtons: Qt.NoButton
        cursorShape: statusButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    //                                              ok         warning    fail       disabled
    readonly property var background_colors_dark:  ['#132613', '#2a2410', '#2a1215', '#1d2024']
    readonly property var foreground_colors_dark:  ['#5DCAA5', '#e8b84a', '#f07070', '#6d7379']
    readonly property var background_colors_light: ['#e4f3e4', '#f7f0d9', '#fbe3e5', '#ececec']
    readonly property var foreground_colors_light: ['#1e8a68', '#a37a10', '#c94a4a', '#9aa0a5']

    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    property var backgorund_palette: qgcPal.globalTheme == QGCPalette.Dark ? background_colors_dark : background_colors_light
    property var foreground_palette: qgcPal.globalTheme == QGCPalette.Dark ? foreground_colors_dark : foreground_colors_light

    contentItem: Label{
        id: field
        text: parent.text
        color: parent.enabled ? foreground_palette[statusIndex] : foreground_palette[3]
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle{
        id: bg
        radius: ScreenTools.defaultBorderRadius
        color: parent.enabled ? backgorund_palette[statusIndex] : backgorund_palette[3]
        border.color: parent.enabled ? foreground_palette[statusIndex] : foreground_palette[3]
        border.width: 1
    }
}
