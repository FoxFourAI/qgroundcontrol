#include "VioTrajectoryPoints.h"
#include "Vehicle.h"

VioTrajectoryPoints::VioTrajectoryPoints(Vehicle *vehicle, QObject *parent):
    QObject(parent),
    _vehicle(vehicle)
{
    connect(_vehicle,&Vehicle::armedChanged,this,[this](bool armed){
        armed ? start() : stop();
    });
}

void VioTrajectoryPoints::start()
{
    clear();
    connect(_vehicle,&Vehicle::mavlinkMessageReceived,this,&VioTrajectoryPoints::_processMavMSG);
}

void VioTrajectoryPoints::stop()
{
    disconnect(_vehicle,&Vehicle::mavlinkMessageReceived,this,&VioTrajectoryPoints::_processMavMSG);
}

void VioTrajectoryPoints::clear()
{
    _points.clear();
    _lastPoint = QGeoCoordinate();
    emit pointsCleared();
}

void VioTrajectoryPoints::_processMavMSG(const mavlink_message_t &msg)
{
    if(msg.msgid != MAVLINK_MSG_ID_GPS_RAW_INT){
        return;
    }
    mavlink_gps_raw_int_t gpsRawInt{};
    mavlink_msg_gps_raw_int_decode(&msg, &gpsRawInt);
    QGeoCoordinate coord(gpsRawInt.lat * 1e-7, gpsRawInt.lon * 1e-7);
    if(!_lastPoint.isValid()) {
        _lastPoint = coord;
        _points.append(QVariant::fromValue(coord));
        emit pointAdded(coord);
        return;
    }
    if (_lastPoint.distanceTo(coord) > _distanceTolerance){
        _lastPoint = coord;
        _points.append(QVariant::fromValue(coord));
        emit pointAdded(coord);
    } else {
        _points.last() = QVariant::fromValue(coord);
        emit updateLastPoint(coord);
    }
}
