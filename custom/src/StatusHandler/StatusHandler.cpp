#include "StatusHandler.h"

#include "Vehicle.h"

StatusHandler::StatusHandler(Vehicle* vehicle, QObject* parent) : QObject(parent), _vehicle(vehicle)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &StatusHandler::handleMavlinkMessage);
}

void StatusHandler::handleMavlinkMessage(const mavlink_message_t& msg)
{
    if (msg.msgid != MAVLINK_MSG_ID_ONBOARD_COMPUTER_STATUS) {
        return;
    }
    mavlink_onboard_computer_status_t decoded;
    mavlink_msg_onboard_computer_status_decode(&msg, &decoded);
    QVariantList list;

    if (decoded.cpu_combined[0] < 100) {
        _cpuInfo["totalUsage"] = decoded.cpu_combined[0];

        for (int i = 0; i < 10 && decoded.cpu_combined[i] < 100; i++) {
            list.append(decoded.cpu_combined[i]);
        }
        _cpuInfo["trace"] = list;
        list.clear();
    }

    for (int i = 0; i < 8 && decoded.cpu_cores[i] < 100; i++) {
        list.append(decoded.cpu_cores[i]);
    }
    _cpuInfo["cores"] = list;
    list.clear();

    _ramInfo["total"] = static_cast<double>(static_cast<uint64_t>(decoded.ram_total) << 20) / 1e9;
    _ramInfo["usage"] = static_cast<double>(static_cast<uint64_t>(decoded.ram_usage) << 20) / 1e9;

    for (int i = 0; i < 4 && decoded.storage_type[i] < _storageNames.count(); i++) {
        QVariantMap storageInfo;
        storageInfo["type"] = _storageNames[decoded.storage_type[i]];
        storageInfo["usage"] = static_cast<double>(static_cast<uint64_t>(decoded.storage_usage[i]) << 20) / 1e9;
        storageInfo["total"] = static_cast<double>(static_cast<uint64_t>(decoded.storage_total[i]) << 20) / 1e9;
        list.append(storageInfo);
    }
    _storageInfo = list;
    list.clear();

    emit statusReceived();
}
