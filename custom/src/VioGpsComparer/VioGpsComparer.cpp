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

    //connecting to the parameter manager. When parameters are pulled,check for "SCR_EKF_SRC"
    //if parameter exists, sign on changes and emitting a signal, if not, setting the status to -1,
    //stopping the timer, and unsigning from the parameter if were signed before
    connect(_vehicle->parameterManager(), &ParameterManager::missingParametersChanged, this,[this](bool missing){
        if(missing){
            return;
        }
        bool paramExist = _vehicle->parameterManager()->parameterExists(_vehicle->defaultComponentId(), "SCR_EKF_SRC");
        if(paramExist){
            Fact* ekf_src = _vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), "SCR_EKF_SRC");
            _vioStatus = ekf_src->rawValue().toInt();
            emit vioStatusChanged();

            _ekfSwitchConnection = connect(ekf_src,&Fact::rawValueChanged,this,[this](const QVariant& value){
                _vioStatus = value.toInt();
                emit vioStatusChanged();
                if( _vioStatus ){
                    _RMSEAvr = 0;
                    _RMSECount = 0;
                }
            });

            _refreshTimer.start();
        }  else {
            _vioStatus = -1;
            emit vioStatusChanged();
            if (_ekfSwitchConnection){
                disconnect(_ekfSwitchConnection);
                _refreshTimer.stop();
            }
        }
    });
}

VioGpsComparer::~VioGpsComparer(){
    _refreshTimer.stop();
}

void VioGpsComparer::_handleTrajectory(QGeoCoordinate coordinate, uint8_t src){
    switch (src){
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        if(_vioStatus == 1){
            calculateATE(coordinate);
        }
        break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        _vioCoordinate = coordinate;
        break;
    default:
        return;
    }
}

void VioGpsComparer::_handleArmedChanged(bool armed)
{
    //if the vehicle disarmed, then we clear the RMSE error
    if(!armed){
        _RMSEAvr = 0;
        _RMSECount = 0;
    }
}

void VioGpsComparer::_handleTimeout()
{
    if( !_vehicle){
        return ;
    }

    auto mngr = _vehicle->parameterManager();
    bool paramExist = _vehicle->parameterManager()->parameterExists(_vehicle->defaultComponentId(), "SCR_EKF_SRC");
    if(paramExist){
        mngr->refreshParameter(_vehicle->defaultComponentId(), "SCR_EKF_SRC");
    }
}

void VioGpsComparer::calculateATE(const QGeoCoordinate &coordinate){
    _ATESumm += calculateRMSE(coordinate);
    _ATECount++;
    emit ATEErrorChanged();
}

double VioGpsComparer::calculateRMSE(const QGeoCoordinate &coordinate){
    if (!_vioCoordinate.isValid() || !coordinate.isValid()){
        return 0.0;
    }

    double distance = _vioCoordinate.distanceTo(coordinate); // meters
    double sqError = distance * distance;

    _RMSECount++;

    // Incremental mean of squared errors
    _RMSEAvr += (sqError - _RMSEAvr) / _RMSECount;

    // RMSE = sqrt(mean squared error)
    double rmseError = std::sqrt(_RMSEAvr);
    emit RMSEErrorChanged();
    return rmseError;

}
