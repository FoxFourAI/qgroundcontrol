#pragma once

#include <QtCore/QElapsedTimer>
#include "VehicleCameraControl.h"
#include "Vehicle.h"
class FoxFourCameraControl : public VehicleCameraControl {
    Q_OBJECT
    Q_PROPERTY(bool zoomEnabled READ zoomEnabled NOTIFY zoomEnabledChanged)
    Q_PROPERTY(Fact* minZoomLevel READ minZoomLevel NOTIFY minZoomLevelChanged)
    Q_PROPERTY(Fact* maxZoomLevel READ maxZoomLevel NOTIFY maxZoomLevelChanged)
    Q_PROPERTY(int cameraIndex READ cameraIndex NOTIFY cameraSwitched)
public:
    FoxFourCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID,
                         QObject* parent = nullptr);
    ~FoxFourCameraControl() override;

    Q_INVOKABLE bool startVideoRecording() override;
    Q_INVOKABLE bool stopVideoRecording() override;
    Q_INVOKABLE void startTracking(QRectF rec, bool zoom);
    Q_INVOKABLE void stopTracking() override;
    Q_INVOKABLE void setCameraIndex(int index);
    // Q_INVOKABLE virtual void zoom                   (QRectF rec);
    Q_INVOKABLE void zoomToRegion(QRectF rec);
    void setZoomLevel(qreal level) override;
    void handleCameraCaptureStatus(const mavlink_camera_capture_status_t& cameraCaptureStatus) override;

    virtual void handleSettings(const mavlink_camera_settings_t& settings);
    void handleStorageInfo(const mavlink_storage_information_t& st);

    Fact* maxZoomLevel() {
        return _maxZoomFact;
    }

    Fact* minZoomLevel() {
        return _minZoomFact;
    }

    [[nodiscard]] int cameraIndex() const {
        return _cameraIndex;
    }

    bool toggleVideoRecording() override;
signals:
    void zoomEnabledChanged();
    void minZoomLevelChanged();
    void maxZoomLevelChanged();
    void storageCapacityChanged(uint32_t total, uint32_t free);
    void cameraSwitched();
public slots:
    [[nodiscard]] bool zoomEnabled() const { return _zoomEnabled; }

protected slots:
    virtual void _processRecordingChanged();
    void _connectFact(int componentId, Fact *fact);

protected:
    void _fetchFacts();
    void _requestTrackingStatus() override;
    void _unsubscribeFromCameraFact();
    static void _zoomResponse(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    void _updateZoomEnable();
protected:
    friend void _cameraSwitchHandler(void *resultHandlerData, int compId, const mavlink_command_ack_t &ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    int         _cameraIndex = 1;
    double    _pendingZoom = -1;
    const int _defaultMaxZoom = 16;
    const int _defaultMinZoom = 1;
    Fact    *_maxZoomFact = nullptr,
            *_minZoomFact = nullptr;
    bool     _commandSwitch = false;
    QMetaObject::Connection _cameraSwitchConnection;
    QTimer _videoRecordTimeUpdateTimer;
    QElapsedTimer _videoRecordTimeElapsedTimer;
    bool _zoomEnabled = false;
};
