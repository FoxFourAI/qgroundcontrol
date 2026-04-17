#include "MapMatching.h"

#include "FlyViewSettings.h"
#include "FoxFourAutoPilotPlugin.h"
#include "SettingsManager.h"
#include "Vehicle.h"

MapMatching::MapMatching(Vehicle* vehicle, QObject* parent) : QObject(parent), _vehicle(vehicle) {
    connect(vehicle, &Vehicle::mavlinkMessageReceived, this, &MapMatching::_handleMavCmd);
    connect(vehicle, &Vehicle::flyingChanged, this, [this](bool flying) {
        if (!flying) {
            _clear();
        }
    });
    auto settings = SettingsManager::instance()->flyViewSettings();
    connect(settings->mapMatchingPointsCnt(), &Fact::rawValueChanged, this,
            [this](const QVariant& value) { _maxWaypoints = value.toInt(); });
    _maxWaypoints = settings->mapMatchingPointsCnt()->rawValue().toInt();
}

void MapMatching::_handleMavCmd(const mavlink_message_t& msg) {
    auto plugin = reinterpret_cast<FoxFourAutoPilotPlugin*>(_vehicle->autopilotPlugin());

    if (msg.sysid != _vehicle->id() ||
        msg.compid != plugin->onboardComputersManager()->currentComputerComponent() ||
        msg.msgid != MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
        return;
    }
    mavlink_global_position_int_t globalPosInt;
    mavlink_msg_global_position_int_decode(&msg, &globalPosInt);
    if (globalPosInt.lat == 0 && globalPosInt.lon == 0) {
        return;
    }
    QGeoCoordinate newAnchor(globalPosInt.lat / (double)1E7, globalPosInt.lon / (double)1E7, globalPosInt.alt / 1000.0);
    _anchors.append(QVariant::fromValue(newAnchor));
    if (_anchors.length() > _maxWaypoints) {
        _anchors.remove(_anchors.length() - _maxWaypoints, 0);
    }
    emit listChanged();
}

void MapMatching::_clear() {
    _anchors.clear();
    emit listChanged();
}
