import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager

SettingsPage {
    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _onboardCompManager: _activeVehicle ? _activeVehicle.onboardComputersManager : 0
    SettingsGroupLayout{
        Layout.fillWidth:   true
        heading:            qsTr("VGM Information")

        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Capabilities ")
            labelText:          formatValue(_onboardCompManager.currCompCapabilities)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("UID:")
            labelText:          formatValue(_onboardCompManager.currCompCapabilities)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Firmware version:")
            labelText:          formatValue(_onboardCompManager.currCompFirmwareVersion)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Middleware version:")
            labelText:          formatValue(_onboardCompManager.currCompMiddlewareVersion)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Operating system version:")
            labelText:          formatValue(_onboardCompManager.currCompOSVersion)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("HW / board version:")
            labelText:          formatValue(_onboardCompManager.currCompHWVersion)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Board vendor ID:")
            labelText:          formatValue(_onboardCompManager.currCompVendorId)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Product ID:")
            labelText:          formatValue(_onboardCompManager.currCompProductId)
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Flight custom version:")
            labelText:          _onboardCompManager.currCompFlightVersionHash
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Middle custom version:")
            labelText:          _onboardCompManager.currCompMiddlewareVersionHash
        }
        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("OS custom version:")
            labelText:          _onboardCompManager.currCompOSVersionHash
        }
    }

    function formatValue(value){
        return qsTr("%1(%2)").arg("0x"+value.toString(16).toUpperCase().padStart(4,"0")).arg(value)
    }
}


