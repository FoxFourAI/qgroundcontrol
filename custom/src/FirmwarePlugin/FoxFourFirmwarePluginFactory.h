/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FirmwarePluginFactory.h"
#include "QGCMAVLink.h"

class APMFirmwarePlugin;
class PX4FirmwarePlugin;
class FirmwarePlugin;

class FoxFourFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    FoxFourFirmwarePluginFactory();
    QList<QGCMAVLinkTypes::FirmwareClass_t> supportedFirmwareClasses() const;
    FirmwarePlugin* firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;

};

extern FoxFourFirmwarePluginFactory CustomFirmwarePluginFactoryImp;
