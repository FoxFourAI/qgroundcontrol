pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects

import QGroundControl
import QGroundControl.Controls

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
    property bool debug: false
    property real dummy: 0
    property real incr: 1

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: _style.shadowColor
        shadowHorizontalOffset: _style.shadowOffset
        shadowVerticalOffset: _style.shadowOffset
        shadowBlur: _style.shadowBlur
    }

    Rectangle {
        anchors.centerIn: parent
        width: 20
        height: 20
    }

    OSDCompass {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: _style.majorFontSize * 2
        width: parent.width / 7
        height: width / 2
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        value: activeGroup.heading.value
    }

    OSDLadder {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: parent.width / 10
        value: activeGroup.airSpeed.value
        targetValue: activeGroup.airSpeedSetpoint.value
        height: parent.height * 0.66
        from: 0
        sectors: [{from:0, to: 3, color:"red"},{from:3, to:5, color:"yellow"}]
        header: qsTr("AS")
        footer: qsTr("GS %1").arg(activeGroup.groundSpeed.value.toFixed(0))
    }

    OSDLadder {
        mirrored: true
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: parent.width / 10
        tickStep: 10
        majorEvery: 20
        tickSpacing: height / 10
        height: parent.height * 0.66
        value: activeGroup.altitudeAMSL.value
        targetValue: activeGroup.altitudeTuningSetpoint.value
        header: qsTr("ASL")
        footer: qsTr("AGL %1").arg(activeGroup.altitudeRelative.value.toFixed(1))
    }



}
