#include "VioGpsComparer.h"

#include "FoxFourAutoPilotPlugin.h"
#include "FoxFourPlugin.h"
#include "OnboardComputersManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

VioGpsComparer::VioGpsComparer(Vehicle* vehicle, QObject* parent) : AutoPilotWidget(vehicle, parent)
{
    // we need 2 parameters to fetch data for VIO
    _setRequiredParameters({{"SCR_EKF_SRC", _ekfSrcFact}, {"SCR_USER2", _user2Fact}});
    _notifyOnRefreshFail = false;
    connect(this, &AutoPilotWidget::_requiredFactChanged, this, [this](Fact* fact) {
        if (fact == _ekfSrcFact) {
            emit ekfSrcFactChanged();
            connect(fact, &Fact::rawValueChanged, this, [this](const QVariant& value) {
                if (value.toInt() > 0) {
                    updateATE();
                    clearRMSE();
                }
            });
        }
        if (fact == _user2Fact) {
            emit user2FactChanged();
        }
    });
    _refreshTimer.setInterval(_refreshInterval);
    connect(&_refreshTimer, &QTimer::timeout, this, &VioGpsComparer::refreshParameters);
    connect(_pm, &ParameterManager::parametersReadyChanged, this,
            [this](bool ready) { ready ? _refreshTimer.start() : _refreshTimer.stop(); });
}

VioGpsComparer::~VioGpsComparer()
{
    _refreshTimer.stop();
}

void VioGpsComparer::_handleTrajectory(QGeoCoordinate coordinate, uint8_t src)
{
    if (_ekfSrcFact == nullptr) {
        return;
    }
    switch (src) {
        case MAVLINK_MSG_ID_GPS_RAW_INT:
            if (_ekfSrcFact->rawValue().toInt() == 1) {
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

    // calculating current error
    double error = 0;
    error = _vioCoordinate.distanceTo(_gpsCoordinate);
    if (error == _currentError) {
        return;
    }
    _currentError = error;
    emit currentErrorChanged();
}

void VioGpsComparer::updateATE()
{
    _ATESumm += RMSEError();
    _ATECount++;
    emit ATEErrorChanged();
}

void VioGpsComparer::calculateRMSE(const QGeoCoordinate& coordinate)
{
    if (!_vioCoordinate.isValid() || !coordinate.isValid()) {
        return;
    }

    double distance = _vioCoordinate.distanceTo(coordinate);  // meters
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

void VioGpsComparer::clearCurrentError()
{
    _currentError = 0;
    emit currentErrorChanged();
}
