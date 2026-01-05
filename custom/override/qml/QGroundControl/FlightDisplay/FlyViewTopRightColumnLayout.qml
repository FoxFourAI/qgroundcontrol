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
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap



ColumnLayout {
    width: _rightPanelWidth

    TerrainProgress {
        Layout.alignment:       Qt.AlignTop
        Layout.preferredWidth:  _rightPanelWidth
    }

    ColumnLayout{
        Layout.alignment:   Qt.AlignTop | Qt.AlignRight
        visible: globals.activeVehicle && globals.activeVehicle.autopilotPlugin.headingSetter.isActive
        property var _headingSetter : globals.activeVehicle.autopilotPlugin.headingSetter

        QGCButton{
            text: qsTr("Apply")
            Layout.fillWidth: true
            enabled: parent._headingSetter.canApply
            onClicked:{
                parent._headingSetter.apply()
            }
        }
        QGCButton{
            text: qsTr("Cancel")
            Layout.fillWidth: true
            onClicked:{
                parent._headingSetter.stop()
            }
        }
    }
    // We use a Loader to load the photoVideoControlComponent only when we have an active vehicle and a camera manager.
    // This make it easier to implement PhotoVideoControl without having to check for the mavlink camera
    // to be null all over the place
    Loader {
        id:                 photoVideoControlLoader
        Layout.alignment:   Qt.AlignTop | Qt.AlignRight
        sourceComponent:    globals.activeVehicle && globals.activeVehicle.cameraManager ? photoVideoControlComponent : undefined

        property real rightEdgeCenterInset: visible ? parent.width - x : 0

        Component {
            id: photoVideoControlComponent

            PhotoVideoControl {
            }
        }
    }
}
