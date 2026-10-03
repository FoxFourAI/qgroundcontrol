pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects

import QGroundControl

Item {
    id: _root

    // style
    OSDStyle {
        id: _style
    }

    // settings
    property var _settingsManager:  QGroundControl.settingsManager
    property var _settings:         _settingsManager.foxFourSettings

    // status providers
    property var _activeVehicle:    globals.activeVehicle
    property var _videoManager:     QGroundControl.videoManager
    property var activeGroup:       _activeVehicle ? _activeVehicle.vehicle : null
    // status grid for telemetry
    property var statusGrid:        null

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled:          true
        shadowColor:            _style.shadowColor
        shadowHorizontalOffset: _style.shadowOffset
        shadowVerticalOffset:   _style.shadowOffset
        shadowBlur:             _style.shadowBlur
    }

    Grid {
        id: rec
        x: 200
        y: 200
        width: 800
        height: 800
        columns: 2
        rows: 2
        spacing: 4
        property real dummy: 0
        Timer {
            repeat: true
            running: true
            interval: 100
            onTriggered: rec.dummy += 0.5
        }
        Rectangle{
            width: 300
            height: 300
            color: "black"
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Vertical
                mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
                currentValuePos: 0.1
            }
            Label {
                anchors.centerIn: parent
                text: qsTr("Vert+M")
                color: "white"
            }
        }
        Rectangle{
            width: 300
            height: 300
            color: "black"
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Horizontal
                // mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
            }
            Label {
                anchors.centerIn: parent
                text: qsTr("Hor")
                color: "white"
            }
        }
        Rectangle{
            width: 300
            height: 300
            color: "black"
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Horizontal
                mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
            }
            Label {
                anchors.centerIn: parent
                text: qsTr("hor")
                color: "white"
            }
        }
        Rectangle{
            width: 300
            height: 300
            color: "black"
            OSDLadder {
                anchors.fill: parent
                orientation: Qt.Vertical
                // mirrored: true
                value: rec.dummy
                header: "AVS"
                footer: "M/S"
            }
            Label {
                anchors.centerIn: parent
                text: qsTr("Vert")
                color: "white"
            }
        }
    }



}
