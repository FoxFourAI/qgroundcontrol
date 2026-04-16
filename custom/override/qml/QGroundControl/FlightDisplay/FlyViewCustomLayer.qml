/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models

import QGroundControl

import QGroundControl.Controls

import QGroundControl.FlightDisplay
import QGroundControl.FlightMap




// To implement a custom overlay copy this code to your own control in your custom code source. Then override the
// FlyViewCustomLayer.qml resource with your own qml. See the custom example and documentation for details.
Item {
    id: _root

    property var parentToolInsets               // These insets tell you what screen real estate is available for positioning the controls in your overlay
    property var totalToolInsets:   _toolInsets // These are the insets for your custom overlay additions
    property var mapControl

    // since this file is a placeholder for the custom layer in a standard build, we will just pass through the parent insets
    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }
    Rectangle{
        property real   _margins:      ScreenTools.defaultFontPixelHeight / 2
        property real   _smallMargins: ScreenTools.defaultFontPixelWidth / 2
        property real   _spacing:      ScreenTools.defaultFontPixelWidth * 5
        id:shortcutIndicator
        anchors.right: parent.right
        anchors.rightMargin: 200
        anchors.top: parent.top
        width: parent.width * 0.1
        height: ScreenTools.defaultFontPixelHeight + 4
        color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
        radius:     _margins
        opacity: 0
        visible: infoLabel.label != ""

        LabelledLabel{
            anchors.leftMargin: _margins * 2
            anchors.rightMargin: _margins * 2
            id: infoLabel
            anchors.fill: parent
        }

        Connections{
            target:  mainWindow
            onShortcutPressed: function(description,value){
                infoLabel.label = description
                infoLabel.labelText = value
                console.log(description.length * ScreenTools.defaultFontPixelWidth)
                shortcutIndicator.width = description.length * ScreenTools.defaultFontPixelWidth + value.length * ScreenTools.defaultFontPixelWidth + shortcutIndicator._spacing
                shortcutIndicator.opacity = 1
                fadeOutTimer.restart()
            }
        }

        Timer{
            id: fadeOutTimer
            interval: 9000
            repeat: false
            onTriggered: fadeAnimation.start()
        }

        NumberAnimation on opacity{
            id: fadeAnimation
            from: 1
            to: 0
            duration: 1000
        }
    }
    Rectangle {
        id: sliderBox
        visible: false
        radius: 4
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
        width: expText.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
        height: parent.height * 0.4
        anchors.left: parent.left
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: parent.verticalCenter
        ColumnLayout{
            anchors.fill: parent
            QGCLabel{
                id: expText
                text:qsTr("Exp.")
                Layout.alignment: Qt.AlignHCenter
            }
            QGCSlider{
                id:expSlider
                orientation: Qt.Vertical
                Layout.alignment: Qt.AlignHCenter
                from: 1
                to: 15
                Layout.fillHeight: true
                Layout.fillWidth: true
                snapMode: Slider.SnapAlways
                stepSize: 1
                onValueChanged: {
                    let goldenRatio = 1.61803398875
                    let compId = globalShortcuts.currentComputerId
                    let paramSetter = globalShortcuts.parameterSetter
                    let newExposure = Math.ceil(2 * Math.pow(goldenRatio,value))
                    paramSetter.setParameter(compId, "CAM_EXPOSURE", newExposure)
                }
                Connections{
                    target:globals.activeVehicle.parameterManager
                    onParametersReadyChanged: (ready)=>{
                        if (!ready) {
                            return;
                        }
                        let compId = globalShortcuts.currentComputerId
                        let paramSetter = globalShortcuts.parameterSetter
                        let exposure = Math.floor(paramSetter.getParameter(compId, "CAM_EXPOSURE"))
                        let goldenRatio = 1.61803398875
                        expSlider.value = Math.round(Math.log(exposure / 2) / Math.log(goldenRatio))
                        sliderBox.visible = ture
                    }
                }
            }
        }
    }
}
