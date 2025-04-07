/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

ColumnLayout {
    QGCSlider {
        id:                 "customSliderAttempt"
        orientation:        Qt.Horizontal
        from:               0
        to:                 100
        value:              50
        height:             ScreenTools.defaultFontPixelHeight
        width:              ScreenTools.defaultFontPixelWidth * 100
        leftPadding:        ScreenTools.defaultFontPixelWidth

    }   

    spacing: ScreenTools.defaultFontPixelHeight / 2

    FactPanelController { id: controller }

    SettingsGroupLayout {
        heading:            qsTr("GCS Failsafe")
        Layout.fillWidth:   true

        LabelledFactComboBox {
            label:      qsTr("Action")
            fact:       controller.getParameterFact(-1, "FS_GCS_ENABLE")
            indexModel: false
        }

        LabelledFactSlider {
            Layout.fillWidth:       true
            label:                  qsTr("Timeout")
            fact:                   controller.getParameterFact(-1, "FS_GCS_TIMEOUT")
            sliderPreferredWidth:   ScreenTools.defaultFontPixelWidth * 20
        }
    }

    SettingsGroupLayout {
        heading:            qsTr("Failsafe Options")
        Layout.fillWidth:   true

        Repeater {
            id:     repeater
            model:  fact ? fact.bitmaskStrings : []

            property Fact fact: controller.getParameterFact(-1, "FS_OPTIONS")

            QGCCheckBoxSlider {
                Layout.fillWidth: true
                text:               modelData
                checked:            fact.value & fact.bitmaskValues[index]

                property Fact fact: repeater.fact

                onClicked: {
                    var i
                    var otherCheckbox
                    if (checked) {
                        fact.value |= fact.bitmaskValues[index]
                    } else {
                        fact.value &= ~fact.bitmaskValues[index]
                    }
                }
            }
        }
    }
}
