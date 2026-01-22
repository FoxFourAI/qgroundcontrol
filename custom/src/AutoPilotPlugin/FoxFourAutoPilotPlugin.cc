/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FoxFourAutoPilotPlugin.h"
#include "ParameterManager.h"
#include "QGCCorePlugin.h"
#include "Vehicle.h"
#include "Camera/FoxFourCameraControl.h"
#include "QGCCameraManager.h"
#include "QGCApplication.h"
FoxFourAutoPilotPlugin::FoxFourAutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : APMAutoPilotPlugin(vehicle, parent)
{
    _onboardComputersMngr = new OnboardComputersManager(vehicle, this);
    auto cameraMgr =vehicle->cameraManager();
    connect(cameraMgr, &QGCCameraManager::currentCameraChanged,this,[this,cameraMgr](){
        if(_cameraConnnection){
            disconnect(_cameraConnnection);
        }
        auto camera =reinterpret_cast<FoxFourCameraControl*>(cameraMgr->currentCameraInstance());
        _cameraConnnection = connect(camera,&FoxFourCameraControl::storageCapacityChanged,this,&FoxFourAutoPilotPlugin::handleStorageCapacityChanged);
    });
}

FoxFourAutoPilotPlugin::~FoxFourAutoPilotPlugin(){
    delete _onboardComputersMngr;
}

const QVariantList &FoxFourAutoPilotPlugin::vehicleComponents(){
    if (_components.isEmpty()) {
        _components = APMAutoPilotPlugin::vehicleComponents();
    }
    return _components;
}

void FoxFourAutoPilotPlugin::rebootOnboardComputers(){
    if (!_onboardComputersMngr) {
            qCWarning(VehicleLog) << "Cannot reboot onboard computers: no manager is present";
            return;
        }
        qWarning() << "Rebooting onboard computers";
        _onboardComputersMngr->rebootAllOnboardComputers();
}

OnboardComputersManager *FoxFourAutoPilotPlugin::onboardComputersManager(){
    return _onboardComputersMngr;
}
QString FoxFourAutoPilotPlugin::storageCapacity(){
    return _storageCapacityStr;
}
void FoxFourAutoPilotPlugin::handleStorageCapacityChanged(uint32_t total, uint32_t free)
{
    _storageCapacityStr = qgcApp()->bigSizeMBToString(free).split(' ').first() + " / "+ qgcApp()->bigSizeMBToString(total);
    emit storageCapacityChanged();
}
