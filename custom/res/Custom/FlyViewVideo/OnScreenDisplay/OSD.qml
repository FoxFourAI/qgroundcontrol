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

    OSDLadder {
        x: 200
        y: 200
        width: 300
        height: 300
        orientation: Qt.Horizontal
    }

}
