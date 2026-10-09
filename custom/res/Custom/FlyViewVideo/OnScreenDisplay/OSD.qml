pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
Item {
    id: _root

    // style
    property OSDStyle _style: OSDStyle{
        color:_settings.hudColor.value

    }

    FactPanelController { id: controller
        onMissingParametersAvailable: {
            if(parameterExists(-1, "AIRSPEED_MIN")) {
                const param = getParameterFact(-1, "AIRSPEED_MIN")
                spdTape.sectors.push({from: 0, to: param.value})
            }

        }
    }
    visible: active && _settings.hudVisible.value
    // settings
    property var _settingsManager:  QGroundControl.settingsManager
    property var _settings:         _settingsManager.foxFourSettings

    // status providers
    property var _activeVehicle:    globals.activeVehicle
    property var _videoManager:     QGroundControl.videoManager
    property var activeGroup:       _activeVehicle ? _activeVehicle.vehicle : null
    // status grid for telemetry
    property bool active: true
    property var statusGrid:        null
    property bool debug: true
    property real dummy: 0
    property real incr: 100

    layer.enabled: _settings.hudShadow.value
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: _style.shadowColor
        shadowHorizontalOffset: _style.shadowOffset
        shadowVerticalOffset: _style.shadowOffset
        shadowBlur: _style.shadowBlur
    }

    OSDCompass {
        id: compass
        visible:_settings.hudCompass.value
        style: _style
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: _style.majorFontSize * 2
        width: parent.width / 7
        height: width / 2
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        value: activeGroup.heading.value
    }

    OSDRoll{
        id: roll
        visible:_settings.hudRoll.value
        anchors.top: parent.top
        style: _style
        anchors.topMargin: _style.fontPadding
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width / 3
        height: width / 4
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        value: activeGroup.roll.value

        Text {
            anchors.centerIn: parent
            font.pixelSize: parent.style.titleFontSize
            color: parent.style.color
            text: qsTr("H %1").arg(activeGroup.heading.value)
        }
    }

    OSDLadder {
        id: spdTape
        visible:_settings.hudSpeed.value
        style: _style
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: parent.width / 10
        value: activeGroup.airSpeed.value
        targetValue: activeGroup.airSpeedSetpoint.value
        height: parent.height * 0.66
        from: 0

        header: qsTr("AS")
        footer: qsTr("GS %1").arg(activeGroup.groundSpeed.value.toFixed(0))
    }

    OSDLadder {
        id: altTape
        visible:_settings.hudAltitude.value
        style: _style
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

    OSDHorizon {
        id:horizon
        visible:_settings.hudHorizon.value
        playerSize: Qt.size(_root.width,_root.height)
        style: _style
        anchors.left: spdTape.right
        anchors.right: altTape.left
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        heading: activeGroup.heading.value
        roll: activeGroup.roll.value
        pitch: activeGroup.pitch.value
    }
}
