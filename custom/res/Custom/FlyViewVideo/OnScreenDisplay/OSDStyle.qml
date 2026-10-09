import QtQuick

import QGroundControl
import QGroundControl.Controls

QtObject {
    //general
    property real   _subRatio:          0.8
    property color  color:              Qt.rgba(100 / 255, 1,0,1)
    property color  boxColor:           Qt.rgba(0,0,0,0.6)
    //shadow
    property color  shadowColor:        Qt.rgba(0,0,0,0.9)
    property real   shadowOffset:       2
    property real   shadowBlur:         0.5
    //font
    property real   majorFontSize:      ScreenTools.largeFontPointSize * 1.5
    property real   defaultFontSize:    majorFontSize * _subRatio
    property real   minorFontSize:      defaultFontSize * _subRatio
    property real   fontPadding:        minorFontSize
    property real   titleFontSize:      majorFontSize * 1.3
    property real   currentValueFontSize: majorFontSize * 1.3
    //lines
    property real   majorLineWidth:     3
    property real   minorLineWidth:     majorLineWidth * _subRatio
    property real   majorTickLength:    majorFontSize * 0.5
    property real   minorTickLength:    majorTickLength * _subRatio
}
