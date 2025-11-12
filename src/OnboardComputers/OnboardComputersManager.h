/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>

#include "MAVLinkLib.h"
#include "QmlObjectListModel.h"
#include "f4_autonomy/f4_autonomy.h"
Q_DECLARE_LOGGING_CATEGORY(OnboardComputersManagerLog)

class Vehicle;

//-----------------------------------------------------------------------------
/// Onboard Computers Manager
class OnboardComputersManager : public QObject {
    Q_OBJECT

   public:
    OnboardComputersManager(Vehicle* vehicle);
    virtual ~OnboardComputersManager() = default;

    static void registerQmlTypes();

    Q_PROPERTY(int currentComputer                     READ currentComputer                WRITE setCurrentComputer             NOTIFY currentComputerChanged)
    Q_PROPERTY(bool currCompIsVGM                      READ currCompIsVGM                  NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint64_t currCompCapabilities           READ currCompCapabilities           NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint64_t currCompUID                    READ currCompUID                    NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint32_t currCompFirmwareVersion        READ currCompFirmwareVersion        NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint32_t currCompMiddlewareVersion      READ currCompMiddlewareVersion      NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint32_t currCompOSVersion              READ currCompOSVersion              NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint32_t currCompHWVersion              READ currCompHWVersion              NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint16_t currCompVendorId               READ currCompVendorId               NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(uint16_t currCompProductId              READ currCompProductId              NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(QString  currCompFlightVersionHash      READ currCompFlightVersionHash      NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(QString  currCompMiddlewareVersionHash  READ currCompMiddlewareVersionHash  NOTIFY currentComputerInfoUpdated)
    Q_PROPERTY(QString  currCompOSVersionHash          READ currCompOSVersionHash          NOTIFY currentComputerInfoUpdated)

    int currentComputer()                   { return _currentComputerIndex; }  ///< Current selected computer index
    bool     currCompIsVGM();
    uint64_t currCompCapabilities()         {return _onboardComputers[_currentComputerIndex].info.capabilities;}
    uint64_t currCompUID()                  {return _onboardComputers[_currentComputerIndex].info.uid;}
    uint32_t currCompFirmwareVersion()      {return _onboardComputers[_currentComputerIndex].info.flight_sw_version;}
    uint32_t currCompMiddlewareVersion()    {return _onboardComputers[_currentComputerIndex].info.middleware_sw_version;}
    uint32_t currCompOSVersion()            {return _onboardComputers[_currentComputerIndex].info.os_sw_version;}
    uint32_t currCompHWVersion()            {return _onboardComputers[_currentComputerIndex].info.board_version;}
    uint16_t currCompVendorId()             {return _onboardComputers[_currentComputerIndex].info.vendor_id;}
    uint16_t currCompProductId()            {return _onboardComputers[_currentComputerIndex].info.product_id;}
    QString  currCompFlightVersionHash()    {return QString((char*)_onboardComputers[_currentComputerIndex].info.flight_custom_version);}
    QString  currCompMiddlewareVersionHash(){return QString((char*)_onboardComputers[_currentComputerIndex].info.middleware_custom_version);}
    QString  currCompOSVersionHash()        {return QString((char*)_onboardComputers[_currentComputerIndex].info.os_custom_version);}

    // virtual
    virtual void setCurrentComputer(int sel);


    // This is public to avoid some circular include problems caused by statics
    class OnboardComputerStruct {
       public:
        OnboardComputerStruct(uint8_t compID_, Vehicle* vehicle_);
        OnboardComputerStruct() = default;
        QElapsedTimer lastHeartbeat{};
        uint8_t compID{0};
        uint8_t infoRequestCnt{0};
        mavlink_companion_version_t info{0};
        Vehicle* vehicle{nullptr};
    };

   signals:
    void onboardComputersChanged();
    void currentComputerChanged(uint8_t compID);
    void streamChanged();
    void onboardComputerTimeout(uint8_t compID);
    void onboardComputerInfoUpdated(uint8_t compID);
    void onboardComputerInfoRecievedError( uint8_t compID);
    void currentComputerInfoUpdated();

   protected slots:
    virtual void _vehicleReady(bool ready);
    virtual void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _checkTimeouts();

   public:
    virtual void rebootAllOnboardComputers();

    // TODO: this function should be moved to some node handling VIO communication
    /*!
     * \brief send an external position estimate command to the current onboard computer running a navigation algorithm.
     */
    Q_INVOKABLE virtual void sendExternalPositionEstimate(const QGeoCoordinate& coord);

   protected:
    virtual void _handleHeartbeat(const mavlink_message_t& message);
    virtual void _handleCompanionVersion(const mavlink_message_t& message);
    // TODO: we could extend this with handling of ONBOARD_COMPUTER_STATUS mavlink message, but it is still WIP
    Vehicle* _vehicle = nullptr;
    bool _vehicleReadyState = false;
    int _currentComputerIndex = 0;
    const int _companionVersionMaxRetryCount=4;
    const int _timeoutCheckInterval=2000;
    QTimer    _timeoutCheckTimer;
    QMap<uint8_t, OnboardComputerStruct> _onboardComputers;
};
