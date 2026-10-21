import QtQuick
import QtQuick.Layouts

import QGroundControl.FactControls
import QGroundControl
import QGroundControl.Controls

//this page is not visible unless vioStatus is 0 or 1
ToolIndicatorPage {
    id: root
    showExpand: false
    property var activeVehicle: globals.activeVehicle
    property var comparer: activeVehicle.autopilotPlugin.vioGpsComparer
    property var parameterSetter: QGroundControl.corePlugin.parameterSetter
    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                heading: qsTr("VIO helper")

                FactCheckBoxSlider {
                    Layout.fillWidth: true
                    text: qsTr("Eanble")
                    fact: comparer.user2Fact
                }

                LabelledLabel {
                    Layout.fillWidth: true
                    label: qsTr("Current Error:")
                    labelText: root.comparer.currentError.toFixed(2) + " m"
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: ToolTip.show(
                                       qsTr("Current distance between gps and vio positions"))
                        onExited: ToolTip.hide()
                    }
                }
                LabelledLabel {
                    Layout.fillWidth: true
                    label: qsTr("RMSE:")
                    labelText: root.comparer.RMSEError.toFixed(2) + " m"
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: ToolTip.show(qsTr("Root Mean Squared Error"))
                        onExited: ToolTip.hide()
                    }
                }
                LabelledLabel {
                    Layout.fillWidth: true
                    label: qsTr("ATE:")
                    labelText: root.comparer.ATEError.toFixed(2) + " m"
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: ToolTip.show(
                                       qsTr("Absolute Trajectory Error"))
                        onExited: ToolTip.hide()
                    }
                }
            }
        }
    }
}
