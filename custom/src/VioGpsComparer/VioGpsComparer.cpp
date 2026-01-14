#include "VioGpsComparer.h"
#include "Vehicle.h"

VioGpsComparer::VioGpsComparer(Vehicle *vehicle, QObject *parent){
    connect(vehicle,&Vehicle::updateTrajectory,this,&VioGpsComparer::_handleTrajectory);
    _window.reserve(_accumulateCount);
}

VioGpsComparer::~VioGpsComparer(){

}

void VioGpsComparer::_handleTrajectory(QGeoCoordinate coordinate, uint8_t src){
    switch (src){
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        calculateError(coordinate);
        break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        _vioCoordinate = coordinate;
        break;
    default:
        return;
    }
}

void VioGpsComparer::calculateError(QGeoCoordinate &coordinate){
    if(!_vioCoordinate.isValid()){
        return;
    }
    double error = _vioCoordinate.distanceTo(coordinate);
    if(error > _maxError){
        _maxError =error;
        emit maxErrorChanged();
    }
    if(error < _minError){
        _minError = error;
        emit minErrorChanged();
    }

    _window.push_back(error);
    while(_window.size() > _accumulateCount){
        _window.pop_front();
    }
    double newAvrError = std::accumulate(_window.begin(),_window.end(),0.0,[=](double result,double val){return result+val;})/_window.size();
    if(newAvrError != _avrError){
        _avrError = newAvrError;
        emit avrErrorChanged();
    }
}
