#pragma once

#include "APM/APMAutoPilotPlugin.h"
#include "EKSources/EKSources.h"
#include "OnboardComputersManager.h"
#include "VioGpsComparer/VioGpsComparer.h"
#include "ButtonList/ButtonList.h"
#include "MapMatching/MapMatching.h"
#include "VioTrajectory/VioTrajectoryPoints.h"
#include "CopterConfigurator/CopterConfigurator.h"
#include "StatusHandler/StatusHandler.h"
class Vehicle;
class FoxFourCameraControl;
class FoxFourAutoPilotPlugin : public APMAutoPilotPlugin {
    Q_OBJECT
    Q_PROPERTY(OnboardComputersManager* onboardComputersManager READ onboardComputersManager MEMBER _onboardComputersMngr)
    Q_PROPERTY(VioGpsComparer* vioGpsComparer MEMBER _vioGpsComparer)
    Q_PROPERTY(EKSources* ekSources MEMBER _ekSources)
    Q_PROPERTY(ButtonList* buttonList MEMBER _buttonList)
    Q_PROPERTY(VioTrajectoryPoints* vioTrajectory MEMBER _vioTrajectory)
    Q_PROPERTY(MapMatching* mapMatching READ mapMatching NOTIFY childsCreated)
    Q_PROPERTY(QString storageCapacity READ storageCapacity NOTIFY storageCapacityChanged)
    Q_PROPERTY(bool isDropper READ isDropper NOTIFY isDropperChanged)
    Q_PROPERTY(bool exposureAvailable READ exposureAvailable NOTIFY exposureAvailableChanged)
    Q_PROPERTY(CopterConfigurator* configurator MEMBER _configurator)
    Q_PROPERTY(StatusHandler* computerStatus MEMBER _computerStatus NOTIFY childsCreated)
public:
    explicit FoxFourAutoPilotPlugin(Vehicle* vehicle, QObject* parent = nullptr);
    ~FoxFourAutoPilotPlugin();
    /// This allows us to hide Vehicle Setup pages if needed
    const QVariantList& vehicleComponents() final;
    QString storageCapacity();
    /// Reboot all onboard computers
    Q_INVOKABLE void rebootOnboardComputers();
    Q_INVOKABLE void setEK3Source(int index);
    Q_INVOKABLE void setServo(int servo, int value);
    MapMatching* mapMatching() {return _mapMatching;}
    bool isDropper() { return _isDropper; }
    OnboardComputersManager* onboardComputersManager();
    bool exposureAvailable() {return _exposureAvailable;}
signals:
    void exposureAvailableChanged();
    void storageCapacityChanged();
    void isDropperChanged();
    void buttonListChanged();
    void childsCreated();
private slots:
    void setIsDropper(int type);
    void handleStorageCapacityChanged(uint32_t total, uint32_t free);
    void handleFactAdded(int compinentId, Fact* fact);

private:
    bool _isDropper = false;
    bool _exposureAvailable = false;
    EKSources* _ekSources = nullptr;
    ButtonList* _buttonList = nullptr;
    MapMatching* _mapMatching = nullptr;
    VioTrajectoryPoints* _vioTrajectory = nullptr;
    StatusHandler* _computerStatus = nullptr;
    QVariantList _components;
    OnboardComputersManager* _onboardComputersMngr = nullptr;
    VioGpsComparer* _vioGpsComparer = nullptr;
    QString _storageCapacityStr = "0 / 0 MB";
    CopterConfigurator* _configurator = nullptr;
    QMetaObject::Connection _cameraConnection;
};
