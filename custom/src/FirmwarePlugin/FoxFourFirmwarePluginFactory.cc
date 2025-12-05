/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FoxFourFirmwarePluginFactory.h"
#include "FoxFourFirmwarePlugin.h"

FoxFourFirmwarePluginFactory FoxFourFirmwarePluginFactoryImp;

FoxFourFirmwarePluginFactory::FoxFourFirmwarePluginFactory()
    : _pluginInstance(nullptr)
{

}

QList<QGCMAVLink::FirmwareClass_t> FoxFourFirmwarePluginFactory::supportedFirmwareClasses() const
{
    QList<QGCMAVLink::FirmwareClass_t> firmwareClasses;
    // firmwareClasses.append(QGCMAVLink::FirmwareClassPX4);
    firmwareClasses.append(QGCMAVLink::FirmwareClassArduPilot);
    return firmwareClasses;
}

QList<QGCMAVLink::VehicleClass_t> FoxFourFirmwarePluginFactory::supportedVehicleClasses() const
{
    QList<QGCMAVLink::VehicleClass_t> vehicleClasses;
    // vehicleClasses.append(QGCMAVLink::VehicleClassMultiRotor);
    vehicleClasses=FirmwarePluginFactory::supportedVehicleClasses();
    return vehicleClasses;
}

FirmwarePlugin *FoxFourFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{

    if (autopilotType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        if (!_pluginInstance) {
            _pluginInstance = new FoxFourFirmwarePlugin;
        }
        return _pluginInstance;
    }

    return nullptr;
}
