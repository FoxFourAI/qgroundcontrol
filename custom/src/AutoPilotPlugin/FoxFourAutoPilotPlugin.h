#pragma once

#include "APM/APMAutoPilotPlugin.h"
#include "OnboardComputersManager.h"
#include "HeadingAlignment/HeadingAlignmentSetter.h"
class Vehicle;

class FoxFourAutoPilotPlugin : public APMAutoPilotPlugin
{
    Q_OBJECT
    Q_PROPERTY(OnboardComputersManager* onboardComputersManager MEMBER _onboardComputersMngr)
    Q_PROPERTY(HeadingAlignmentSetter* headingSetter MEMBER _headingSetter)
public:
    explicit FoxFourAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);
    ~FoxFourAutoPilotPlugin();
    /// This allows us to hide Vehicle Setup pages if needed
    const QVariantList &vehicleComponents() final;

    /// Reboot all onboard computers
    Q_INVOKABLE void rebootOnboardComputers();

private:
    QVariantList _components;
    HeadingAlignmentSetter *_headingSetter = nullptr;
    OnboardComputersManager *_onboardComputersMngr = nullptr;

};
