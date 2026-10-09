/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FoxFourFirmwarePluginFactory.h"

#include "FoxFourCopterFirmwarePlugin.h"
#include "FoxFourPlaneFirmwarePlugin.h"
FoxFourFirmwarePluginFactory FoxFourFirmwarePluginFactoryImp;

FoxFourFirmwarePluginFactory::FoxFourFirmwarePluginFactory() {}

QList<QGCMAVLinkTypes::FirmwareClass_t> FoxFourFirmwarePluginFactory::supportedFirmwareClasses() const {
    return {QGCMAVLink::FirmwareClassArduPilot};
}

FirmwarePlugin* FoxFourFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType,
                                                                         [[maybe_unused]] MAV_TYPE vehicleType)
{
    // For now F4 only supported ArduPilot
    if (autopilotType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            switch (vehicleType) {
                case MAV_TYPE_FIXED_WING:
                    return new FoxFourPlaneFirmwarePlugin;
                    break;
                case MAV_TYPE_QUADROTOR:
                    return new FoxFourCopterFirmwarePlugin;
                    break;
                default:
                    return nullptr;
            }
    }
    return nullptr;
}
