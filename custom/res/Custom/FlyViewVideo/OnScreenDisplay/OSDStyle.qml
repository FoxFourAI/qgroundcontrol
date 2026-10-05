import QtQuick

import QGroundControl
import QGroundControl.Controls

QtObject {
    //general
    property real   _subRatio:          0.8
    property color  color:              Qt.rgba(100 / 255, 1,0,1)
    property color  boxColor:           Qt.rgba(0,0,0,0.8)
    //shadow
    property color  shadowColor:        Qt.rgba(0,0,0,0.45)
    property real   shadowOffset:       2
    property real   shadowBlur:         4
    //font
    property real   majorFontSize:      ScreenTools.largeFontPointSize * 1.5
    property real   defaultFontSize:    majorFontSize * _subRatio
    property real   minorFontSize:      defaultFontSize * _subRatio
    property real   fontPadding:        minorFontSize
    //lines and ticks
    property real   majorLineWidth:     3
    property real   minorLineWidth:     majorLineWidth * _subRatio
}
