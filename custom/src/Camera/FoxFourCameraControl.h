#pragma once

#include <QtCore/QElapsedTimer>
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
    ~FoxFourCameraControl() override;

    Q_INVOKABLE bool startVideoRecording() override;
    Q_INVOKABLE bool stopVideoRecording() override;
    Q_INVOKABLE void startTracking(QRectF rec, bool zoom);
    Q_INVOKABLE void stopTracking() override;
    Q_INVOKABLE void setCameraIndex(int index);
    // Q_INVOKABLE virtual void zoom                   (QRectF rec);
    void setZoomLevel(qreal level) override;

    virtual void handleSettings(const mavlink_camera_settings_t& settings);
    void handleStorageInfo(const mavlink_storage_information_t& st);

    int maxZoomLevel() {
        if (_maxZoomFact != nullptr) {
            return _maxZoomFact->rawValue().toInt();
        }
        return _defaultMaxZoom;
    }

    int minZoomLevel() {
        if (_minZoomFact != nullptr) {
            return _minZoomFact->rawValue().toInt();
        }
        return _defaultMinZoom;
    }

    [[nodiscard]] int cameraIndex() const {
        return _cameraIndex;
    }

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
    void _requestTrackingStatus() override;
    void _unsubscribeFromCameraFact();

protected:
    friend void _cameraSwitchHandler(void *resultHandlerData, int compId, const mavlink_command_ack_t &ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    int         _cameraIndex = 1;
    const int _defaultMaxZoom = 16;
    const int _defaultMinZoom = 1;
    Fact    *_maxZoomFact = nullptr,
            *_minZoomFact = nullptr,
            *_cameraSwitchFact = nullptr;
    bool     _commandSwitch = false;
    QMetaObject::Connection _cameraSwitchConnection;
    QTimer _videoRecordTimeUpdateTimer;
    QElapsedTimer _videoRecordTimeElapsedTimer;
    bool _zoomEnabled = false;
};
