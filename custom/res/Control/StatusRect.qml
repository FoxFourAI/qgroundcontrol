import QtQuick

import QGroundControl
import QGroundControl.Controls

Rectangle{

    StatusColors{
        id: colors
    }

    radius: ScreenTools.defaultBorderRadius
    property int statusIndex: 0
    color: enabled ? colors.background_palette[statusIndex] : colors.background_palette[4]
    border.color: enabled ? colors.foreground_palette[statusIndex] : colors.foreground_palette[4]
    border.width: 1
}
