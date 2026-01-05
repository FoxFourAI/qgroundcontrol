#include "HeadingAlignmentSetter.h"

HeadingAlignmentSetter::HeadingAlignmentSetter(Vehicle *vehicle, QObject *parent):
    _vehicle(vehicle)
  , QObject(parent)
{

}

void HeadingAlignmentSetter::setMapCoordinate(QGeoCoordinate coordinates)
{
    if(coordinates != _mapCoords){
        _mapCoords = coordinates;
        emit mapCoordinateChanged();
        canApply();
    }
}

void HeadingAlignmentSetter::setCameraCoordinate(QPointF coordinates)
{
    if(coordinates != _cameraCoords){
        _cameraCoords = coordinates;
        emit cameraCoordinateChanged();
        canApply();
    }
}

QGeoCoordinate HeadingAlignmentSetter::mapCoordinate()
{
    return _mapCoords;
}

QPointF HeadingAlignmentSetter::cameraCoordinate()
{
    return _cameraCoords;
}

bool HeadingAlignmentSetter::isActive()
{
    return _active;
}

void HeadingAlignmentSetter::start()
{
    setActive(true);
}

void HeadingAlignmentSetter::stop()
{
    setCameraCoordinate(QPointF(-1,-1));
    setMapCoordinate(QGeoCoordinate(-1,-1));

    setActive(false);
}

bool HeadingAlignmentSetter::canApply()
{
    if((_mapCoords.latitude() >=0 && _cameraCoords.x() >=0) != _appliable){
        _appliable = (_mapCoords.longitude() >=0 && _cameraCoords.x() >=0);
        emit canApplyChanged();
    }
    return _appliable;
}

void HeadingAlignmentSetter::apply()
{
//TODO: send msg to VGM

    stop();
}

void HeadingAlignmentSetter::setActive(bool active)
{
    if(active != _active){
        _active = active;
        emit activityChanged();
    }
}
