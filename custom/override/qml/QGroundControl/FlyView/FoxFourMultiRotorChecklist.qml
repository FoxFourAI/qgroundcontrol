import QtQuick
import QtQuick.Controls
import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Item {
    property var model: listModel
    PreFlightCheckModel {
        id:     listModel
        PreFlightCheckGroup {
            name: qsTr("Initial Checks")

            PreFlightCheckButton {
                name:           qsTr("Hardware")
                manualText:     qsTr("Are props mounted, secured, and not colliding? Are all antennas connected?")
            }

            PreFlightCheckButton {
                name:           qsTr("Video")
                manualText:     qsTr("Do you have the video from the vehicle?")
            }

            PreFlightCheckButton {
                name:           qsTr("Batteries")
                manualText:     qsTr("Check if the batteries have at least 4.0V per cell.")
            }

            PreFlightSensorsHealthCheck {
            }
        }

        PreFlightCheckGroup {
            name : qsTr("RC check")

            PreFlightCheckButton {
                name:           qsTr("Connection")
                manualText:     qsTr("Is connection quality visible on the RC screen?")
            }

            PreFlightCheckButton {
                name:           qsTr("Flight modes")
                manualText:     qsTr("Can you change flight modes?")
            }

            PreFlightCheckButton {
                name:           qsTr("VIO")
                manualText:     qsTr("Can you toggle VIO?")
            }

            PreFlightCheckButton {
                name:           qsTr("Cameras")
                manualText:     qsTr("Can you switch cameras?")
            }

            PreFlightCheckButton {
                name:           qsTr("Payload")
                manualText:     qsTr("Can you control payload drop? Does the fixation mechanism work properly?")
            }
        }
    }
}
