#include "FoxFourCameraControl.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>
#include <QtQml/QQmlEngine>
#include <QtXml/QDomDocument>
#include <QtXml/QDomNodeList>

#include "FTPManager.h"
#include "FoxFourSettings.h"
#include "FoxFourAutoPilotPlugin.h"
#include "FoxFourPlugin.h"
#include "MissionCommandTree.h"
#include "ParameterManager.h"
#include "ParameterSetter.h"
#include "QGCCameraIO.h"
#include "QGCCorePlugin.h"
#include "SettingsManager.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(FoxFourCameraControlLog, "FoxFour.CameraControl")


//-----------------------------------------------------------------------------
FoxFourCameraControl::FoxFourCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID,
                                           QObject* parent)
    : VehicleCameraControl(info, vehicle, compID, parent) {
    _cameraMode = CAM_MODE_VIDEO;
    connect(VideoManager::instance(), &VideoManager::recordingChanged, this,
            &FoxFourCameraControl::_processRecordingChanged);
    qCDebug(FoxFourCameraControlLog) << "FoxFour camera control initialized";
    _mavlinkCameraInfo.flags |= CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM;

    // _requestZoomBoundriesTimer.setInterval(1000);
    // connect(&_requestZoomBoundriesTimer,&QTimer::timeout,this,&FoxFourCameraControl::_requestFacts);
    // _requestZoomBoundriesTimer.start();
    connect(vehicle->parameterManager(), &ParameterManager::factAdded, this, &FoxFourCameraControl::_connectFact);
}

//-----------------------------------------------------------------------------
FoxFourCameraControl::~FoxFourCameraControl() {
    // Stop all timers to prevent them from firing during or after destruction
    _captureStatusTimer.stop();
    _videoRecordTimeUpdateTimer.stop();
    _streamInfoTimer.stop();
    _streamStatusTimer.stop();
    _cameraSettingsTimer.stop();
    _storageInfoTimer.stop();

    delete _netManager;
    _netManager = nullptr;
}

//-----------------------------------------------------------------------------
bool FoxFourCameraControl::startVideoRecording() {
    if (!_resetting) {
        qCDebug(FoxFourCameraControlLog) << "startVideoRecording()";
        //-- Check if camera can capture videos or if it can capture it while in Photo Mode
        if ((cameraMode() == CAM_MODE_PHOTO && !videoInPhotoMode())) {
            return false;
        } else if (!capturesVideo()) {
            // If camera can't record video, just do the normal ground station recording here
            VideoManager::instance()->startRecording();

            if (captureVideoState() == CaptureVideoStateCapturing) {
                qCWarning(FoxFourCameraControlLog) << "startVideoRecording: Camera already recording";
                return false;
            }

            _videoRecordTimeUpdateTimer.start();
            _videoRecordTimeElapsedTimer.start();
            VideoManager::instance()->startRecording();
            _setVideoCaptureStatus(VIDEO_CAPTURE_STATUS_RUNNING);
            return true;
        }

        if (_videoCaptureStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::handleSettings(const mavlink_camera_settings_t& settings) {
    qCDebug(FoxFourCameraControlLog) << "Received CAMERA_SETTINGS Mode:" << settings.mode_id
                              << "- stopping timer, resetting retries";
    _cameraSettingsTimer.stop();
    _cameraSettingsRetries = 0;
    // FIXME: for now just hardcoding to support older VGM firmware with mode_id hardcoded to photo in mavsdk
    int mode_id = 1;
    _setCameraMode(static_cast<CameraMode>(mode_id));

    // qreal z = static_cast<qreal>(settings.zoomLevel);
    qreal f = static_cast<qreal>(settings.focusLevel);
    // if(std::isfinite(z) && z != _zoomLevel) {
    //     _zoomLevel = z;
    //     emit zoomLevelChanged();
    // }
    if (std::isfinite(f) && f != _focusLevel) {
        _focusLevel = f;
        emit focusLevelChanged();
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::_processRecordingChanged() {
    bool isRecording = VideoManager::instance()->recording();
    if (!isRecording && !capturesVideo() && _videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _videoRecordTimeUpdateTimer.stop();
        _setVideoCaptureStatus(VIDEO_CAPTURE_STATUS_STOPPED);
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::_connectFact(int /*componentId*/, Fact* fact) {
    // signing to maximal zoom value
    if (fact->name() == "VID_ZOOM_MAX" && _maxZoomFact == nullptr) {
        _maxZoomFact = fact;
        connect(_maxZoomFact, &Fact::valueChanged, this, [this](const QVariant& /*value*/) { emit maxZoomLevelChanged(); });
        emit maxZoomLevelChanged();
    }

    // signing  to the camera source
    if (fact->name() == "SCR_USER3" && _cameraSwitchFact == nullptr) {
        _cameraSwitchFact = fact;
        _cameraIndex = _cameraSwitchFact->rawValue().toInt();
        _cameraSwitchConnection = connect(_cameraSwitchFact, &Fact::valueChanged, this, [this](const QVariant& value) {
            _cameraIndex = value.toInt();
            emit cameraSwitched();
        });
        emit cameraSwitched();
    }

    // signing to minimal zoom value
    if (fact->name() == "VID_ZOOM_MIN" && _minZoomFact == nullptr) {
        _minZoomFact = fact;
        connect(_minZoomFact, &Fact::valueChanged, this, [this](const QVariant& /*value*/) { emit minZoomLevelChanged(); });
        emit minZoomLevelChanged();
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::_requestTrackingStatus() {
    //FOXFOUR_TODO: add parameter
    uint64_t rate = 1'000'000 / SettingsManager::instance()->foxFourSettings()->trackingRate()->rawValue().toInt();
    _vehicle->sendMavCommand(_compID, MAV_CMD_SET_MESSAGE_INTERVAL, true, MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS,
                             rate);  // Interval (us)
}

void FoxFourCameraControl::_unsubscribeFromCameraFact() {
    _commandSwitch = true;
    disconnect(_cameraSwitchConnection);
    _cameraSwitchFact = nullptr;
}

//-----------------------------------------------------------------------------

// handler for camera switch responce
void _cameraSwitchHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack,
                          Vehicle::MavCmdResultFailureCode_t /*failureCode*/) {
    if (ack.result != MAV_RESULT_ACCEPTED) {
        qCDebug(FoxFourCameraControlLog) << "error occured while switching cameras!";
    }

    FoxFourCameraControl* ctrl = static_cast<FoxFourCameraControl*>(resultHandlerData);
    if (ctrl->_cameraSwitchFact) {
        ctrl->_unsubscribeFromCameraFact();
    }
    qCDebug(FoxFourCameraControlLog) << "camera swiched successfully";
    ctrl->_cameraIndex += 1;
    if (ctrl->_cameraIndex > 2 ) {
        ctrl->_cameraIndex = 1;
    }
    emit ctrl->cameraSwitched();
}

void FoxFourCameraControl::setCameraIndex(int index) {
    if (!_commandSwitch) {
        if (_cameraSwitchFact == nullptr && _vehicle->parameterManager()->parameterExists(_vehicle->defaultComponentId(), "SCR_USER3")) {
            _cameraSwitchFact = _vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), "SCR_USER3");
        }
        if (_cameraSwitchFact) {
            _cameraSwitchFact->setCookedValue(index);
            _cameraSwitchFact->valueChanged(index);
            emit cameraSwitched();
        }
    }

    Vehicle::MavCmdAckHandlerInfo_t handler;
    handler.resultHandler = _cameraSwitchHandler;
    handler.resultHandlerData = this;
    _vehicle->sendMavCommandWithHandler(&handler, _compID, MAV_CMD_VIDEO_START_STREAMING, index);
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::handleStorageInfo(const mavlink_storage_information_t& st) {
    VehicleCameraControl::handleStorageInformation(st);
    qDebug()<<"capacity changed";
    emit storageCapacityChanged(_storageTotal, _storageFree);
}

//-----------------------------------------------------------------------------
bool FoxFourCameraControl::stopVideoRecording() {
    if (!_resetting) {
        if (!capturesVideo()) {
            // Again, if camera doesn't have the recording, do one on the ground station
            if (_videoCaptureStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
                qCWarning(FoxFourCameraControlLog) << "stopVideoRecording: Camera not recording";
                return false;
            }

            _videoRecordTimeUpdateTimer.stop();
            VideoManager::instance()->stopRecording();
            _setVideoCaptureStatus(VIDEO_CAPTURE_STATUS_STOPPED);
            return true;
        }

        // Firstly, stop video recording on the UAV
        qCDebug(FoxFourCameraControlLog) << "stopVideoRecording()";
        if (_videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
            _vehicle->sendMavCommand(_compID,                     // Target component
                                     MAV_CMD_VIDEO_STOP_CAPTURE,  // Command id
                                     false,                       // Don't Show Error (handle locally)
                                     0);                          // Reserved (Set to 0)

            VideoManager::instance()->stopRecording();

            return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
void FoxFourCameraControl::startTracking(QRectF rec, bool zoom) {
    uint64_t time = 0;
    if (_trackingImageRect != rec) {
        _trackingImageRect = rec;

        qCDebug(FoxFourCameraControlLog) << "Start Tracking (Rectangle: [" << static_cast<float>(rec.x()) << ", "
                                  << static_cast<float>(rec.y()) << "] - [" << static_cast<float>(rec.x() + rec.width())
                                  << ", " << static_cast<float>(rec.y() + rec.height()) << "]"
                                  << ", Timestamp: " << time;
        // if we are zooming, calculating new zoom level and setting it.
        if (zoom) {
            // for now zoom is just a bit in timestamp
            time = time | (1ULL << 63);
            double newZoomLevel = qMin(1.0 / rec.width(), 1.0 / rec.height());
            if (_zoomLevel != newZoomLevel) {
                _zoomLevel = newZoomLevel;
                double minZoomLevel = _minZoomFact ? _minZoomFact->rawValue().toDouble() : _defaultMinZoom;
                if ((_zoomLevel > minZoomLevel) != _zoomEnabled) {
                    _zoomEnabled = !_zoomEnabled;
                    emit zoomEnabledChanged();
                }
                emit zoomLevelChanged();
            }
        }
        uint32_t timestampLow = static_cast<uint32_t>(time);
        uint32_t timestampHight = static_cast<uint32_t>(time >> 32);

        float param5, param6;

        std::memcpy(&param5, &timestampLow, sizeof(param5));
        std::memcpy(&param6, &timestampHight, sizeof(param6));

        _vehicle->sendMavCommand(_compID, MAV_CMD_CAMERA_TRACK_RECTANGLE, true, static_cast<float>(rec.x()),
                                 static_cast<float>(rec.y()), static_cast<float>(rec.x() + rec.width()),
                                 static_cast<float>(rec.y() + rec.height()), param5, param6);

        // Request tracking status
        _requestTrackingStatus();
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::stopTracking() {
    qCDebug(FoxFourCameraControlLog) << "Stop Tracking";
    uint64_t timestamp = QDateTime::currentMSecsSinceEpoch();
    uint32_t timestampLow = static_cast<uint32_t>(timestamp);
    uint32_t timestampHigh = static_cast<uint32_t>(timestamp >> 32);

    float param1, param2;

    std::memcpy(&param1, &timestampLow, sizeof(param1));
    std::memcpy(&param2, &timestampHigh, sizeof(param2));

    //-- Stop Tracking
    _vehicle->sendMavCommand(_compID, MAV_CMD_CAMERA_STOP_TRACKING, false);

    //-- Stop Sending Tracking Status
    _vehicle->sendMavCommand(_compID, MAV_CMD_SET_MESSAGE_INTERVAL, false, MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS,
                             -1);

    // reset tracking image rectangle
    _trackingImageRect = {};
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::setZoomLevel(qreal level) {
    VehicleCameraControl::setZoomLevel(level);
    _zoomLevel = level;
    double minZoom = _minZoomFact ? _minZoomFact->rawValue().toDouble() : _defaultMinZoom;
    if ((_zoomLevel > minZoom) != _zoomEnabled) {
        _zoomEnabled = !_zoomEnabled;
        emit zoomEnabledChanged();
    }
    emit zoomLevelChanged();
}
