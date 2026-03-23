/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FoxFourAutoPilotPlugin.h"

#include "Camera/FoxFourCameraControl.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "QGCCorePlugin.h"
#include "Vehicle.h"

//define boundries
#define SERVO_MAX 1900
#define SERVO_MIN 1100

FoxFourAutoPilotPlugin::FoxFourAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : APMAutoPilotPlugin(vehicle, parent) {
    _onboardComputersMngr = new OnboardComputersManager(vehicle, this);
    _vioGpsComparer = new VioGpsComparer(vehicle, this);
    auto cameraMgr = vehicle->cameraManager();
    connect(cameraMgr, &QGCCameraManager::currentCameraChanged, this, [this, cameraMgr]() {
        if (_cameraConnection) {
            disconnect(_cameraConnection);
        }
        auto camera = reinterpret_cast<FoxFourCameraControl*>(cameraMgr->currentCameraInstance());
        _cameraConnection = connect(camera, &FoxFourCameraControl::storageCapacityChanged, this,
                                    &FoxFourAutoPilotPlugin::handleStorageCapacityChanged);
        connect(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged, this, [=](bool ready) {
            if (!ready) {
                return;
            }
            auto pm = _vehicle->parameterManager();
            const int compId = _onboardComputersMngr->currentComputerComponent();
            if (!pm->parameterExists(compId, "GUID_FRAME_TYPE")) {
                return;
            }
            auto fact = pm->getParameter(compId, "GUID_FRAME_TYPE");
            connect(fact, &Fact::rawValueChanged,this,[=](QVariant value){
               setIsDropper(value.toInt());
            });
        });
    });
}

FoxFourAutoPilotPlugin::~FoxFourAutoPilotPlugin() { delete _onboardComputersMngr; }

const QVariantList& FoxFourAutoPilotPlugin::vehicleComponents() {
    if (_components.isEmpty()) {
        _components = APMAutoPilotPlugin::vehicleComponents();
    }
    return _components;
}

void FoxFourAutoPilotPlugin::rebootOnboardComputers() {
    if (!_onboardComputersMngr) {
        qCWarning(VehicleLog) << "Cannot reboot onboard computers: no manager is present";
        return;
    }
    qWarning() << "Rebooting onboard computers";
    _onboardComputersMngr->rebootAllOnboardComputers();
}

void FoxFourAutoPilotPlugin::setEK3Source(int index) {
    // sending command to set new index for ekf source
    _vehicle->sendCommand(_vehicle->defaultComponentId(), MAV_CMD_SET_EKF_SOURCE_SET, false, index);
}

void FoxFourAutoPilotPlugin::setServo(int servo, int value, int duration) {
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_DO_SET_SERVO, false, servo, qMin(SERVO_MAX,value));
    if (duration == -1) {
        return;
    }
    QTimer::singleShot(duration,[=](){setServo(servo,SERVO_MIN);});
}

OnboardComputersManager* FoxFourAutoPilotPlugin::onboardComputersManager() { return _onboardComputersMngr; }

void FoxFourAutoPilotPlugin::setIsDropper(int type)
{
    bool dropperFlag = type == 2;
    if (dropperFlag != _isDropper) {
        _isDropper = dropperFlag;
        emit isDropperChanged();
    }
}

QString FoxFourAutoPilotPlugin::storageCapacity() { return _storageCapacityStr; }

void FoxFourAutoPilotPlugin::handleStorageCapacityChanged(uint32_t total, uint32_t free) {
    _storageCapacityStr =
            qgcApp()->bigSizeMBToString(free).split(' ').first() + " / " + qgcApp()->bigSizeMBToString(total);
    emit storageCapacityChanged();
}
