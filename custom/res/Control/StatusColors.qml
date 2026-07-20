import QtQuick

import QGroundControl
import QGroundControl.Controls

Item{
    //                                       Normal      Ok       Warning     Error     Disabled
    property var background_colors_dark:  ['#1d2024', '#132613', '#2a2410', '#2a1215', '#1c1e21']
    property var foreground_colors_dark:  ['#8e969d', '#5DCAA5', '#e8b84a', '#f07070', '#4a4d52']
    property var background_colors_light: ['#ececec', '#e4f3e4', '#f7f0d9', '#fbe3e5', '#f0f0f0']
    property var foreground_colors_light: ['#767b7f', '#1e8a68', '#a37a10', '#c94a4a', '#c4c8cb']

    QGCPalette{
        id:qgcPal
        colorGroupEnabled: true
    }
    property var background_palette: qgcPal.globalTheme == QGCPalette.Dark ? background_colors_dark : background_colors_light
    property var foreground_palette: qgcPal.globalTheme == QGCPalette.Dark ? foreground_colors_dark : foreground_colors_light

}
