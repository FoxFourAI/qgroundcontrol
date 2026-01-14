#pragma once

#include "APM/APMAutoPilotPlugin.h"
#include "OnboardComputersManager.h"
#include "VioGpsComparer/VioGpsComparer.h"
class Vehicle;

class FoxFourAutoPilotPlugin : public APMAutoPilotPlugin
{
    Q_OBJECT
    Q_PROPERTY(OnboardComputersManager* onboardComputersManager READ onboardComputersManager MEMBER _onboardComputersMngr)
    Q_PROPERTY(VioGpsComparer *vioGpsComparer MEMBER _vioGpsComparer)
public:
    explicit FoxFourAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);
    ~FoxFourAutoPilotPlugin();
    /// This allows us to hide Vehicle Setup pages if needed
    const QVariantList &vehicleComponents() final;

    /// Reboot all onboard computers
    Q_INVOKABLE void rebootOnboardComputers();
    OnboardComputersManager* onboardComputersManager();
private:
    QVariantList _components;
    OnboardComputersManager *_onboardComputersMngr = nullptr;
    VioGpsComparer *_vioGpsComparer = nullptr;
};
