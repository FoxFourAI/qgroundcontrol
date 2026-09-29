import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    //general
    property real   _minorRatio:        0.7
    property color  color:              "lightgreen"
    property color  boxColor:           Qt.rgba(0,0,0,0.8)
    //shadow
    property color  shadowColor:        Qt.rgba(0,0,0,0.45)
    property real   shadowOffset:       2
    property real   shadowBlur:         4
    //font
    property real   majorFontSize:      ScreenTools.largeFontPointSize * 1.5
    property real   minorFontSize:      majorFontSize * _minorRatio
    property real   fontPadding:        minorFontSize
    //lines
    property real   majorLineWidth:     3
    property real   majorTickLength:    majorFontSize
    property real   minorTickLength:    minorFontSize
    property real   minorLineWidth:     majorLineWidth * _minorRatio
}
