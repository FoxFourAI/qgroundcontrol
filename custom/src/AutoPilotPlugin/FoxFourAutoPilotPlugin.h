#pragma once

#include "APM/APMAutoPilotPlugin.h"
#include "OnboardComputersManager.h"
#include "VioGpsComparer/VioGpsComparer.h"


class Vehicle;
class FoxFourCameraControl;
class FoxFourAutoPilotPlugin : public APMAutoPilotPlugin
{
    Q_OBJECT
    Q_PROPERTY(OnboardComputersManager* onboardComputersManager READ onboardComputersManager MEMBER _onboardComputersMngr)
    Q_PROPERTY(VioGpsComparer *vioGpsComparer MEMBER _vioGpsComparer)
    Q_PROPERTY(QString storageCapacity READ storageCapacity NOTIFY storageCapacityChanged)
    Q_PROPERTY(bool isDropper READ isDropper NOTIFY isDropperChanged)
public:
    explicit FoxFourAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);
    ~FoxFourAutoPilotPlugin();
    /// This allows us to hide Vehicle Setup pages if needed
    const QVariantList &vehicleComponents() final;
    QString storageCapacity();
    /// Reboot all onboard computers
    Q_INVOKABLE void rebootOnboardComputers();
    Q_INVOKABLE void setEK3Source(int index);
    Q_INVOKABLE void setServo(int servo, int value,int duration = -1);
    bool isDropper(){return _isDropper;}
    OnboardComputersManager* onboardComputersManager();
signals:
    void storageCapacityChanged();
    void isDropperChanged();
private slots:
    void setIsDropper(int type);
    void handleStorageCapacityChanged(uint32_t total, uint32_t free);
private:
    bool _isDropper = false;
    QVariantList _components;
    OnboardComputersManager *_onboardComputersMngr = nullptr;
    VioGpsComparer *_vioGpsComparer = nullptr;
    QString _storageCapacityStr = "0 / 0 MB";
    QMetaObject::Connection _cameraConnection;
};
