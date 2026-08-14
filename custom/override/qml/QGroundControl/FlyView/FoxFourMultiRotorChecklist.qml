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
                manualText:     qsTr("Props mounted, secured and do not collide? All antennas are connected?")
            }

            PreFlightCheckButton {
                name:           qsTr("Video")
                manualText:     qsTr("Do you have the video from the vehicle?")
            }

            PreFlightCheckButton {
                name:           qsTr("Batteries")
                manualText:     qsTr("Check if the batteries has at least 4.0V per cell.")
            }

            // PreFlightBatteryCheck {
            //     failurePercent:                 40
            //     allowFailurePercentOverride:    false
            // }

            PreFlightSensorsHealthCheck {
            }
        }

        PreFlightCheckGroup {
            name : qsTr("RC check")

            PreFlightCheckButton {
                name:           qsTr("Connection")
                manualText:     qsTr("Is connection quality visible on RC screen?")
            }

            PreFlightCheckButton {
                name:           qsTr("Flight modes")
                manualText:     qsTr("Can you change flight modes?")
            }

            PreFlightCheckButton {
                name:           qsTr("VIO")
                manualText:     qsTr("Can you switch VIO to \"Feeding Pose\"?")
            }

            PreFlightCheckButton {
                name:           qsTr("Cameras")
                manualText:     qsTr("Can you switch cameras? Change zoom on the front one?")
            }

            PreFlightCheckButton {
                name:           qsTr("Payload")
                manualText:     qsTr("Can you control payload drop? Does the fixation mechamism works properlly?")
            }
        }
    }
}
