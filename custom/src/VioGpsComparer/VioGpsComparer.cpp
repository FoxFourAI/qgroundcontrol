#include "VioGpsComparer.h"
#include "Vehicle.h"
#include "ParameterManager.h"
#include "OnboardComputersManager.h"
#include "FoxFourAutoPilotPlugin.h"
#include "FoxFourPlugin.h"


VioGpsComparer::VioGpsComparer(Vehicle *vehicle, QObject *parent): QObject(parent){

    _vehicle = vehicle;
    connect(_vehicle,&Vehicle::updateTrajectory,this,&VioGpsComparer::_handleTrajectory);

    _refreshTimer.setInterval(_refreshInterval);
    connect(&_refreshTimer,&QTimer::timeout,this,&VioGpsComparer::_handleTimeout);

    //connecting to new parameters added. if it is "SCR_EKF_SRC",then connecting it
    connect(_vehicle->parameterManager(), &ParameterManager::factAdded, this,[this](int srcId,Fact* fact){
        if(srcId != _vehicle->defaultComponentId() || fact->name() != "SCR_EKF_SRC"){
            return;
        }
        _vioStatus = fact->rawValue().toInt();
        emit vioStatusChanged();
        connect(fact,&Fact::rawValueChanged, this, [this](const QVariant& value){
            _vioStatus = value.toInt();
            emit vioStatusChanged();
            if( _vioStatus == 1 ){
                updateATE();
                clearRMSE();
            }
        });
    });
    //connect updateTimer to parameterReady
    connect(_vehicle->parameterManager(),&ParameterManager::parametersReadyChanged,this,[this](bool ready){
        ready ? _refreshTimer.start() : _refreshTimer.stop();
    });
}

VioGpsComparer::~VioGpsComparer(){
    _refreshTimer.stop();
}

void VioGpsComparer::_handleTrajectory(QGeoCoordinate coordinate, uint8_t src){
    switch (src){
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        if(_vioStatus == 1){
            _gpsCoordinate = coordinate;
            calculateRMSE(coordinate);
        }
        break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        _vioCoordinate = coordinate;
        break;
    default:
        return;
    }

    //calculating current error
    double error = 0;
    error = _vioCoordinate.distanceTo(_gpsCoordinate);
    if(error == _currentError){
        return;
    }
    _currentError = error;
    emit currentErrorChanged();


}

void VioGpsComparer::_handleTimeout()
{
    if( !_vehicle){
        return ;
    }

    auto mngr = _vehicle->parameterManager();
    bool commLost = _vehicle->vehicleLinkManager()->communicationLost();
    bool paramExist = _vehicle->parameterManager()->parameterExists(_vehicle->defaultComponentId(), "SCR_EKF_SRC");
    if(paramExist && !commLost){
        mngr->refreshParameter(_vehicle->defaultComponentId(), "SCR_EKF_SRC");
    }
}

void VioGpsComparer::updateATE(){
    _ATESumm += RMSEError();
    _ATECount++;
    emit ATEErrorChanged();
}

void VioGpsComparer::calculateRMSE(const QGeoCoordinate &coordinate){
    if (!_vioCoordinate.isValid() || !coordinate.isValid()){
        return;
    }

    double distance = _vioCoordinate.distanceTo(coordinate); // meters
    double sqError = distance * distance;

    _RMSECount++;

    // Incremental mean of squared errors
    _RMSEAvr += (sqError - _RMSEAvr) / _RMSECount;

    emit RMSEErrorChanged();

}

void VioGpsComparer::clearRMSE()
{
    _RMSEAvr = 0;
    _RMSECount = 0;
    emit RMSEErrorChanged();
}

void VioGpsComparer::clearCurrentError(){
    _currentError = 0;
    emit currentErrorChanged();
}
