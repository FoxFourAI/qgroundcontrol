#pragma once

#include "APM/APMAutoPilotPlugin.h"
#include "EKSources/EKSources.h"
#include "OnboardComputersManager.h"
#include "VioGpsComparer/VioGpsComparer.h"
#include "MapMatching/MapMatching.h"

class Vehicle;
class FoxFourCameraControl;
class FoxFourAutoPilotPlugin : public APMAutoPilotPlugin {
    Q_OBJECT
    Q_PROPERTY(OnboardComputersManager* onboardComputersManager READ onboardComputersManager MEMBER _onboardComputersMngr)
    Q_PROPERTY(VioGpsComparer* vioGpsComparer MEMBER _vioGpsComparer)
    Q_PROPERTY(EKSources* ekSources MEMBER _ekSources)
    Q_PROPERTY(MapMatching* mapMatching READ mapMatching NOTIFY mapMatchingCreated)
    Q_PROPERTY(QString storageCapacity READ storageCapacity NOTIFY storageCapacityChanged)
    Q_PROPERTY(bool isDropper READ isDropper NOTIFY isDropperChanged)
public:
    explicit FoxFourAutoPilotPlugin(Vehicle* vehicle, QObject* parent = nullptr);
    ~FoxFourAutoPilotPlugin();
    /// This allows us to hide Vehicle Setup pages if needed
    const QVariantList& vehicleComponents() final;
    QString storageCapacity();
    /// Reboot all onboard computers
    Q_INVOKABLE void rebootOnboardComputers();
    Q_INVOKABLE void setEK3Source(int index);
    Q_INVOKABLE void flipServo(int servo);
    MapMatching* mapMatching() {return _mapMatching;}
    bool isDropper() { return _isDropper; }
    OnboardComputersManager* onboardComputersManager();
signals:
    void storageCapacityChanged();
    void isDropperChanged();
    void mapMatchingCreated();
private slots:
    void setIsDropper(int type);
    void handleStorageCapacityChanged(uint32_t total, uint32_t free);

private:
    bool _isDropper = false;
    EKSources* _ekSources = nullptr;
    MapMatching* _mapMatching = nullptr;
    QVariantList _components;
    OnboardComputersManager* _onboardComputersMngr = nullptr;
    VioGpsComparer* _vioGpsComparer = nullptr;
    QString _storageCapacityStr = "0 / 0 MB";
    QMetaObject::Connection _cameraConnection;

    static const int _servoCount = 9;
    bool _servoActive[_servoCount];
    // define boundries
    static inline constexpr int SERVO_MIN = 1100;
    static inline constexpr int SERVO_MAX = 1900;
};
