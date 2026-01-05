#include "HeadingAligmentSetter.h"

HeadingAligmentSetter::HeadingAligmentSetter(Vehicle *vehicle, QObject *parent):
    _vehicle(vehicle)
  , QObject(parent)
{

}

void HeadingAligmentSetter::setMapCoordinate(QGeoCoordinate coordinates)
{
    if(coordinates != _mapCoords){
        _mapCoords = coordinates;
        emit mapCoordinateChanged();
        canApply();
    }
}

void HeadingAligmentSetter::setCameraCoordinate(QPointF coordinates)
{
    if(coordinates != _cameraCoords){
        _cameraCoords = coordinates;
        emit cameraCoordinateChanged();
        canApply();
    }
}

QGeoCoordinate HeadingAligmentSetter::mapCoordinate()
{
    return _mapCoords;
}

QPointF HeadingAligmentSetter::cameraCoordinate()
{
    return _cameraCoords;
}

bool HeadingAligmentSetter::isActive()
{
    return _active;
}

void HeadingAligmentSetter::start()
{
    setActive(true);
}

void HeadingAligmentSetter::stop()
{
    setCameraCoordinate(QPointF(-1,-1));
    setMapCoordinate(QGeoCoordinate(-1,-1));

    setActive(false);
}

bool HeadingAligmentSetter::canApply()
{
    if((_mapCoords.latitude() >=0 && _cameraCoords.x() >=0) != _appliable){
        _appliable = (_mapCoords.longitude() >=0 && _cameraCoords.x() >=0);
        emit canApplyChanged();
    }
    return _appliable;
}

void HeadingAligmentSetter::apply()
{
//TODO: send msg to VGM

    stop();
}

void HeadingAligmentSetter::setActive(bool active)
{
    if(active != _active){
        _active = active;
        emit activityChanged();
    }
}
