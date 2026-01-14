/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item{
    id: root
    width: mainRow.width
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    Row{
        id: mainRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2
        QGCColoredImage{
            id: indicator
            anchors.bottom:     parent.bottom
            anchors.top:        parent.top
            Layout.fillHeight:  true
            sourceSize.height:  height
            width:              height
            source:             "/custom/img/wifi-0.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.windowTransparentText
        }
        Timer{
            interval:   300
            repeat:     true
            running:    true
            property int indx: 0
            onTriggered: {
                indicator.source = "/custom/img/wifi-"+indx+".svg"
                indx = (indx+1)%4
            }
        }

        QGCLabel{
            property var comparer : globals.activeVehicle.autopilotPlugin.vioGpsComparer
            id:     featureText
            anchors.verticalCenter: parent.verticalCenter
            text: comparer.avrError.toFixed(2)+"m"
        }
    }
    MouseArea{
        anchors.fill: parent
        onClicked: {
            console.log("clicked")
        }
    }



}
