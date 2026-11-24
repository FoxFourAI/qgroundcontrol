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
#include "VideoManager.h"
#include "VideoReceiver/FoxFourGstVideoReceiver.h"
#include "MissionCommandTree.h"
#include "ParameterManager.h"
#include "ParameterSetter.h"
#include "QGCCameraIO.h"
#include "QGCCorePlugin.h"
#include "SettingsManager.h"
#include "VideoManager.h"
#include "QGCCameraManager.h"
#include "f4_autonomy/f4_autonomy.h"
QGC_LOGGING_CATEGORY(FoxFourCameraControlLog, "FoxFour.CameraControl")


//-----------------------------------------------------------------------------
FoxFourCameraControl::FoxFourCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID,
                                           QObject* parent)
    : VehicleCameraControl(info, vehicle, compID, parent) {
    setCameraMode(CameraMode::CAM_MODE_VIDEO);
    connect(VideoManager::instance(), &VideoManager::recordingChanged, this,
            &FoxFourCameraControl::_processRecordingChanged);
    qCDebug(FoxFourCameraControlLog) << "FoxFour camera control initialized";
    _mavlinkCameraInfo.flags |= CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM;
    connect(vehicle->parameterManager(), &ParameterManager::factAdded, this, &FoxFourCameraControl::_connectFact);
    _fetchFacts();

    //Requesting camera calibration message from vehicle.
    _vehicle->requestMessage([](void*, MAV_RESULT, Vehicle::RequestMessageResultHandlerFailureCode_t, const mavlink_message_t&){},
                             nullptr,
                             _compID,
                             MAVLINK_MSG_ID_F4_CAM_CALIBR);
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
        VideoManager::instance()->startRecording();
        _setVideoCaptureStatus(VIDEO_CAPTURE_STATUS_RUNNING);
        return true;
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

    qreal f = static_cast<qreal>(settings.focusLevel);
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
void FoxFourCameraControl::_connectFact([[maybe_unused]] int componentId, Fact* fact) {
    // signing to maximal zoom value
    if (fact->name() == "VID_ZOOM_MAX" && _maxZoomFact == nullptr) {
        _maxZoomFact = fact;
        emit maxZoomLevelChanged();
    }

    // signing to minimal zoom value
    if (fact->name() == "VID_ZOOM_MIN" && _minZoomFact == nullptr) {
        _minZoomFact = fact;
        emit minZoomLevelChanged();
    }
}

//Fetching facts on initialize complete, because simetimes camera can be created after all parameters are loaded.
void FoxFourCameraControl::_fetchFacts()
{
    auto pm=_vehicle->parameterManager();
    auto vgmId = reinterpret_cast<FoxFourAutoPilotPlugin*>(_vehicle->autopilotPlugin())->onboardComputersManager()->currentComputerComponent();

    if(pm->parameterExists(vgmId, "VID_ZOOM_MAX")) {
        _connectFact(vgmId,pm->getParameter(vgmId,"VID_ZOOM_MAX"));
    }
    if(pm->parameterExists(vgmId, "VID_ZOOM_MIN")) {
        _connectFact(vgmId, pm->getParameter(vgmId, "VID_ZOOM_MIN"));
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::_requestTrackingStatus() {
    uint64_t rate = 1'000'000 / SettingsManager::instance()->foxFourSettings()->trackingRate()->rawValue().toInt();
    _vehicle->sendMavCommand(_compID, MAV_CMD_SET_MESSAGE_INTERVAL, true, MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS,
                             rate);  // Interval (us)
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::_zoomResponse(void *resultHandlerData, int /*compId*/, const mavlink_command_ack_t &ack, Vehicle::MavCmdResultFailureCode_t /*failureCode*/)
{
    auto camControl = reinterpret_cast<FoxFourCameraControl*>(resultHandlerData);
    qCDebug(FoxFourCameraControlLog) << "new factor is " << ack.result_param2 / 100.;
    camControl->_zoomLevel = ack.result_param2 / 100.;
    camControl->_updateZoomEnable();
    emit camControl->zoomLevelChanged();

}

void FoxFourCameraControl::_updateZoomEnable()
{
    const double minZoom = _minZoomFact ? _minZoomFact->rawValue().toDouble() : _defaultMinZoom;
    if ((_zoomLevel > minZoom) != _zoomEnabled) {
        _zoomEnabled = !_zoomEnabled;
        emit zoomEnabledChanged();
    }
}

//-----------------------------------------------------------------------------
// handler for camera switch responce
void _cameraSwitchHandler(void* resultHandlerData,[[maybe_unused]] int compId, const mavlink_command_ack_t& ack,
                          [[maybe_unused]] Vehicle::MavCmdResultFailureCode_t failureCode) {
    if (ack.result != MAV_RESULT_ACCEPTED) {
        qCDebug(FoxFourCameraControlLog) << "error occured while switching cameras!";
    }
    auto ctrl = reinterpret_cast<FoxFourCameraControl*>(resultHandlerData);
    qCDebug(FoxFourCameraControlLog) << "camera swiched successfully";
    ctrl->_requestStreamInfo(0);
    ctrl->_cameraIndex += 1;
    if (ctrl->_cameraIndex > 2 ) {
        ctrl->_cameraIndex = 1;
    }
    emit ctrl->cameraSwitched();
}

//-----------------------------------------------------------------------------
bool FoxFourCameraControl::toggleVideoRecording() {
    if (captureVideoState() == CaptureVideoState::CaptureVideoStateCapturing) {
        qCDebug(FoxFourCameraControlLog) << "stopping capturing";
        return stopVideoRecording();
    } else {
        qCDebug(FoxFourCameraControlLog) << "starting capturing";
        return startVideoRecording();
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::setCameraIndex(int index) {
    Vehicle::MavCmdAckHandlerInfo_t handler;
    handler.resultHandler = _cameraSwitchHandler;
    handler.resultHandlerData = this;
    _vehicle->sendMavCommandWithHandler(&handler, _compID, MAV_CMD_VIDEO_START_STREAMING, index);
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::handleStorageInfo(const mavlink_storage_information_t& st) {
    VehicleCameraControl::handleStorageInformation(st);
    qCDebug(FoxFourCameraControlLog) << "capacity changed";
    emit storageCapacityChanged(_storageTotal, _storageFree);
    // }
}

//-----------------------------------------------------------------------------
bool FoxFourCameraControl::stopVideoRecording() {
    if (!_resetting) {
        // Again, if camera doesn't have the recording, do one on the ground station
        if (captureVideoState() != CaptureVideoState::CaptureVideoStateCapturing) {
            qCWarning(FoxFourCameraControlLog) << "stopVideoRecording: Camera not recording";
            return false;
        }

        _videoRecordTimeUpdateTimer.stop();
        VideoManager::instance()->stopRecording();
        _setVideoCaptureStatus(VIDEO_CAPTURE_STATUS_STOPPED);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::startTracking(QRectF rec, bool zoom) {

    uint64_t time = reinterpret_cast<FoxFourPlugin*>(FoxFourPlugin::instance())->latestAvailableTimestamp();

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
        if (!zoom) {
        _requestTrackingStatus();
        }
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::stopTracking() {
    qCDebug(FoxFourCameraControlLog) << "Stop Tracking";
    uint64_t timestamp = reinterpret_cast<FoxFourPlugin*>(FoxFourPlugin::instance())->latestAvailableTimestamp();
    uint32_t timestampLow = static_cast<uint32_t>(timestamp);
    uint32_t timestampHigh = static_cast<uint32_t>(timestamp >> 32);
    float param1, param2;

    std::memcpy(&param1, &timestampLow, sizeof(param1));
    std::memcpy(&param2, &timestampHigh, sizeof(param2));

    //-- Stop Tracking
    //MavSDK handle first and second parameters as a timestamp, which is not described in the documentation.
    _vehicle->sendMavCommand(_compID, MAV_CMD_CAMERA_STOP_TRACKING, false, param1, param2);

    //-- Stop Sending Tracking Status
    _vehicle->sendMavCommand(_compID, MAV_CMD_SET_MESSAGE_INTERVAL, false, MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS,
                             -1);

    // reset tracking image rectangle
    _trackingImageRect = {};
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::setZoomLevel(qreal level) {

    if(!hasZoom()) {
        return;
    }
    //-- Limit
    level = std::min(std::max(level, 0.0), 100.0);
    Vehicle::MavCmdAckHandlerInfo_t *hndl = new Vehicle::MavCmdAckHandlerInfo_t;
    hndl->resultHandlerData = this;
    hndl->resultHandler = [](void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& /*ack*/, Vehicle::MavCmdResultFailureCode_t /*failureCode*/) {
        auto camControl = reinterpret_cast<FoxFourCameraControl*>(resultHandlerData);
        if(camControl->_pendingZoom < 0) {
            return;
        }
        camControl->_zoomLevel = camControl->_pendingZoom;
        camControl->_pendingZoom = -1;
        camControl->_updateZoomEnable();
        emit camControl->zoomLevelChanged();
    };
    if(_vehicle) {
        _pendingZoom = level;
        _vehicle->sendMavCommandWithHandler(
            hndl,
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_ZOOM,                // Command id
            ZOOM_TYPE_RANGE,                        // Zoom type
            static_cast<float>(level));             // Level
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::zoomToRegion(QRectF rec)
{
    int vgmCompID = reinterpret_cast<FoxFourAutoPilotPlugin*>(_vehicle->autopilotPlugin())->onboardComputersManager()->currentComputerComponent();
    if(vgmCompID == 0){
        return;
    }
    uint64_t time = reinterpret_cast<FoxFourPlugin*>(FoxFourPlugin::instance())->latestKlvTimestamp();
    uint32_t timestampLow = static_cast<uint32_t>(time);
    uint32_t timestampHigh = static_cast<uint32_t>(time >> 32);
    float param5;
    memset(&param5,timestampLow,sizeof(uint32_t));
    float param6;
    memset(&param6,timestampHigh,sizeof(uint32_t));
    auto handler = new Vehicle::MavCmdAckHandlerInfo_t();
    handler->resultHandlerData = this;
    handler->resultHandler = _zoomResponse;
    _vehicle->sendMavCommandWithHandler(handler,vgmCompID, MAV_CMD_DO_REGION_ZOOM,
                             rec.topLeft().x(),
                             rec.topLeft().y(),
                             rec.width(),
                             rec.height(),
                             param5,
                             param6);
}

void FoxFourCameraControl::handleCameraCaptureStatus([[maybe_unused]] const mavlink_camera_capture_status_t& cameraCaptureStatus) {
    //For now camera capture status sends zeros, so do not handle them.
    // VehicleCameraControl::handleCameraCaptureStatus(cameraCaptureStatus);
}
