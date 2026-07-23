import QtQuick

import QGroundControl
import QGroundControl.Controls

import Custom.Widgets 1.0
Label{

    StatusColors{
        id: colors
    }

    property int statusIndex : 0
    color: enabled ? colors.foreground_palette[statusIndex] : colors.foreground_palette[4]
}
