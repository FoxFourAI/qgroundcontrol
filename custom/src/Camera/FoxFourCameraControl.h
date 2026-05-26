#pragma once

#include <QtCore/QElapsedTimer>

#include "MavlinkCameraControl.h"
#include "VehicleCameraControl.h"
#include "Vehicle.h"
class FoxFourCameraControl : public VehicleCameraControl {
    Q_OBJECT
    Q_PROPERTY(bool zoomEnabled READ zoomEnabled NOTIFY zoomEnabledChanged)
    Q_PROPERTY(int minZoomLevel READ minZoomLevel NOTIFY minZoomLevelChanged)
    Q_PROPERTY(int maxZoomLevel READ maxZoomLevel NOTIFY maxZoomLevelChanged)
    Q_PROPERTY(int cameraIndex READ cameraIndex NOTIFY cameraSwitched)
public:
    FoxFourCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID,
                         QObject* parent = nullptr);
    virtual ~FoxFourCameraControl();

    Q_INVOKABLE virtual bool startVideoRecording() override;
    Q_INVOKABLE virtual bool stopVideoRecording() override;
    Q_INVOKABLE virtual void startTracking(QRectF rec, QString timestamp);
    Q_INVOKABLE virtual void stopTracking(uint64_t timestamp = 0);
    Q_INVOKABLE void setCameraIndex(int index);
    // Q_INVOKABLE virtual void zoom                   (QRectF rec);
    Q_INVOKABLE void zoomToRegion(QRectF rec,QString timestamp);
    void setZoomLevel(qreal level) override;

    virtual void handleSettings(const mavlink_camera_settings_t& settings);
    void handleStorageInfo(const mavlink_storage_information_t& st);

    float maxZoomLevel() {
        if (_maxZoomFact) {
            return _maxZoomFact->rawValue().toDouble();
        } else {
            return _defaultMaxZoom;
        }
    }

    float minZoomLevel() {
        if (_minZoomFact) {
            return _minZoomFact->rawValue().toDouble();
        } else {
            return _defaultMinZoom;
        }
    }

    int cameraIndex() {
        return _cameraIndex;
    }

signals:
    void zoomEnabledChanged();
    void minZoomLevelChanged();
    void maxZoomLevelChanged();
    void storageCapacityChanged(uint32_t total, uint32_t free);
    void cameraSwitched();
public slots:
    bool zoomEnabled() { return _zoomEnabled; }

protected slots:
    virtual void _processRecordingChanged();
    void _connectFact(int componentId, Fact *fact);

protected:
    void _requestTrackingStatus() override;
    void _unsubscribeFromCameraFact();
    static void _zoomResponse(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);
protected:
    friend void _cameraSwitchHandler(void *resultHandlerData, int compId, const mavlink_command_ack_t &ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    int         _cameraIndex = 1;
    const double _defaultMaxZoom = 16;
    const double _defaultMinZoom = 1;
    Fact    *_maxZoomFact = nullptr,
    *_minZoomFact = nullptr,
    *_cameraSwitchFact = nullptr;
    bool     _commandSwitch = false;
    QMetaObject::Connection _cameraSwitchConnection;
    QTimer _videoRecordTimeUpdateTimer;
    QElapsedTimer _videoRecordTimeElapsedTimer;
    bool _zoomEnabled = false;
};
