#include "VGMDialect.h"

#include "Vehicle.h"
#include "f4_autonomy/f4_autonomy.h"

Q_LOGGING_CATEGORY(VGMDialectLog, "VGMDialectLog")

VGMDialect::VGMDialect(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownVehicleComponent::UnknownVehicleComponent, parent)
{}

QString VGMDialect::name() const
{
    return _name;
}

QString VGMDialect::description() const
{
    return _description;
}

QString VGMDialect::iconResource() const
{
    return _icon;
}

bool VGMDialect::requiresSetup() const
{
    return true;
}

bool VGMDialect::setupComplete() const
{
    return _setupCompleted;
}

QUrl VGMDialect::setupSource() const
{
    return QUrl();
}

QUrl VGMDialect::summaryQmlSource() const
{
    return QUrl();
}

QStringList VGMDialect::setupCompleteChangedTriggerList() const
{
    return QStringList();
}

void VGMDialect::setupTriggerSignals()
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &VGMDialect::_handleMavMessage);
}

void VGMDialect::_handleMavMessage(const mavlink_message_t& msg)
{
    switch (msg.msgid) {
        case MAVLINK_MSG_ID_F4_DETECTOR: {
            mavlink_f4_detector_t decoded;
            mavlink_msg_f4_detector_decode(&msg, &decoded);
            _handleF4Detector(decoded);
        } break;
        case MAVLINK_MSG_ID_COMPANION_VERSION: {
            mavlink_companion_version_t decoded;
            mavlink_msg_companion_version_decode(&msg, &decoded);
            _handleCompanionVersion(decoded);
        } break;
        default:
            return;
    }
}

void VGMDialect::_handleCompanionVersion(const mavlink_companion_version_t& msg)
{
    QVariantMap info;
    info["capabilities"] = QVariant(static_cast<qlonglong>(msg.capabilities));
    info["uid"] = QVariant(static_cast<qlonglong>(msg.uid));
    info["flight_sw_version"] = QVariant(msg.flight_sw_version);
    info["middleware_sw_version"] = QVariant(msg.middleware_sw_version);
    info["os_sw_version"] = QVariant(msg.os_sw_version);
    info["board_version"] = QVariant(msg.board_version);
    info["vendor_id"] = QVariant(msg.vendor_id);
    info["product_id"] = QVariant(msg.product_id);
    QString flightVersion(reinterpret_cast<const char*>(msg.flight_custom_version));
    QString middlewareVersion(reinterpret_cast<const char*>(msg.middleware_custom_version));
    QString osVersion(reinterpret_cast<const char*>(msg.os_custom_version));
    info["flight_custom_version"] = QVariant(flightVersion);
    info["middleware_custom_version"] = QVariant(middlewareVersion);
    info["os_custom_version"] = QVariant(osVersion);
    emit companionVersionReceived(info);
}

void VGMDialect::_handleF4Detector(const mavlink_f4_detector_t& msg)
{
    if (msg.longitude == INT32_MAX || msg.latitude == INT32_MAX) {
        qCDebug(VGMDialectLog) << "F4_DETECTOR message has bad coordinates. Ignoring";
        return;
    }
    F4_AUTONOMY_DETECTION_CLASS classType = msg.class_type >= F4_AUTONOMY_DETECTION_CLASS_ENUM_END
                                                ? F4_AUTONOMY_DETECTION_CLASS_CAR
                                                : F4_AUTONOMY_DETECTION_CLASS(msg.class_type);
    if (!_type2string.contains(classType)) {
        qCDebug(VGMDialectLog) << "Unknown class_type fallback to Unknow Civialian";
        classType = F4_AUTONOMY_DETECTION_CLASS_CAR;
    }
    QVariantMap detection;
    detection["coord"] = QVariant::fromValue(QGeoCoordinate(msg.latitude * 1e-7, msg.longitude * 1e-7));
    detection["type"] = _type2string[classType];
    _detections.append(detection);
    emit detectionsListChanged();
}

QVariantList VGMDialect::detections() const
{
    return _detections;
}

void VGMDialect::clearDetections()
{
    _detections.clear();
    emit detectionsListChanged();
}
