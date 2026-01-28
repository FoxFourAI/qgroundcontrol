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

import Custom.Widgets 1.0
Item{
    id: root
    //to prevent empty spaces, resize widget when its needed
    width: comparer.vioStatus !== -1 ? mainRow.width : 0
    anchors.top: parent.top
    visible: comparer.vioStatus !== -1
    anchors.bottom: parent.bottom
    property var parameterSetter: QGroundControl.corePlugin.parameterSetter
    property var comparer : globals.activeVehicle.autopilotPlugin.vioGpsComparer
    property var parameterManager: globals.activeVehicle.parameterManager
    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Row{
        id: mainRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage{
            id: vioIndicator
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: height
            fillMode: Image.PreserveAspectFit
            color: qgcPal.text
            source: "qrc:/custom/img/eye-"+comparer.vioStatus+".svg"
            Component.onCompleted: {

            }
        }
        Connections{
            target :comparer
            onVioStatusChanged:{
                console.log("vio status is: "+ comparer.vioStatus)
            }
        }

        Column{
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Math.max(ateText.width,rmseText.width)
            spacing: ScreenTools.defaultFontPixelWidth / 2
            QGCLabel{
                id:     rmseText
                text: qsTr(root.comparer.RMSEError.toFixed(2) + "m")
            }
            QGCLabel{
                id:     ateText
                text: qsTr(root.comparer.ATEError.toFixed(2) + "m")

            }
        }
    }
    MouseArea{
        anchors.fill: parent
        onClicked: {
            mainWindow.showIndicatorDrawer(vioIndicatorPage,root)
        }
    }
    Component{
        id: vioIndicatorPage
        VioIndicatorPage { }
    }
}
