#pragma once

#include "MavlinkCameraControl.h"
#include "VehicleCameraControl.h"

#include <QtCore/QElapsedTimer>

class FoxFourCameraControl : public VehicleCameraControl
{
    Q_OBJECT
    Q_PROPERTY(bool zoomEnabled READ zoomEnabled NOTIFY zoomEnabledChanged)
    Q_PROPERTY(int minZoomLevel READ minZoomLevel NOTIFY minZoomLevelChanged)
    Q_PROPERTY(int maxZoomLevel READ maxZoomLevel NOTIFY maxZoomLevelChanged)
public:
    FoxFourCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);
    virtual ~FoxFourCameraControl();

    Q_INVOKABLE virtual bool startVideoRecording    ();
    Q_INVOKABLE virtual bool stopVideoRecording     ();
    Q_INVOKABLE virtual void startTracking          (QRectF rec, QString timestamp, bool zoom);
    Q_INVOKABLE virtual void stopTracking           (uint64_t timestamp=0);
    // Q_INVOKABLE virtual void zoom                   (QRectF rec);
    void setZoomLevel(qreal level);

    virtual void handleSettings (const mavlink_camera_settings_t& settings);
    void handleStorageInfo(const mavlink_storage_information_t &st);

    int maxZoomLevel(){
        if( _maxZoomFact ){
            return _maxZoomFact->rawValue().toDouble();
        } else {
            return _defaultMaxZoom;
        }
    }

    int minZoomLevel(){
        if( _minZoomFact ){
            return _minZoomFact->rawValue().toDouble();
        } else {
            return _defaultMinZoom;
        }
    }

signals:
    void zoomEnabledChanged();
    void minZoomLevelChanged();
    void maxZoomLevelChanged();
    void storageCapacityChanged(uint32_t total, uint32_t free);
public slots:
    bool zoomEnabled(){return _zoomEnabled;}

protected slots:
    virtual void _processRecordingChanged();
    void         _requestZoomBoundries();
protected:
    const double _defaultMaxZoom = 16;
    const double _defaultMinZoom = 1;
    Fact         *_maxZoomFact = nullptr,
                 *_minZoomFact = nullptr;
    short         _requestZoomBoundriesMaxCount = 5;
    QTimer        _videoRecordTimeUpdateTimer;
    QTimer        _requestZoomBoundriesTimer;
    QElapsedTimer _videoRecordTimeElapsedTimer;
    bool _zoomEnabled = false;


};
