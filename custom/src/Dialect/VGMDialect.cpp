#include "VGMDialect.h"

VGMDialect::VGMDialect(Vehicle *vehicle,
                       AutoPilotPlugin *autopilot,
                       QObject *parent):
    VehicleComponent(vehicle,autopilot,AutoPilotPlugin::KnownVehicleComponent::UnknownVehicleComponent, parent)
{

}

QString VGMDialect::name() const
{
    return _name;
}

QString VGMDialect::description() const
{
    return _description;
}

QString VGMDialect::iconResource() const
{
    return _icon;
}

bool VGMDialect::requiresSetup() const
{
    return true;
}

bool VGMDialect::setupComplete() const
{
    return _setupCompleted;
}

QUrl VGMDialect::setupSource() const
{
    return QUrl();
}

QUrl VGMDialect::summaryQmlSource() const
{
    return QUrl();
}

QStringList  VGMDialect::setupCompleteChangedTriggerList() const
{
    return QStringList();
}

void VGMDialect::setupTriggerSignals()
{

}
