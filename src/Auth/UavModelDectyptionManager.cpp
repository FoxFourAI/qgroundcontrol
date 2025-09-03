#include <QGCLoggingCategory.h>

#include "QGCApplication.h"
#include "SettingsManager.h"
#include "UavModelDecryptionManager.h"

QGC_LOGGING_CATEGORY(UavModelDecryptionManagerLog, "UavModelDecryptionManagerLog")

UavModelDecryptionManager* UavModelDecryptionManager::instance() {
    static UavModelDecryptionManager* _instance = nullptr;
    if (!_instance) {
        _instance = new UavModelDecryptionManager(qApp);
    }
    return _instance;
}

UavModelDecryptionManager::UavModelDecryptionManager(QObject* parent)
    : QObject(parent),
      _vehicle(nullptr),
      _decryptState(DECRYPT_IDLE),
      _isDecrypting(false),
      _isDecrypted(false),
      _timeoutTimer(new QTimer(this)) {
    // Setup timeout timer
    _timeoutTimer->setSingleShot(true);
    _timeoutTimer->setInterval(DECRYPT_TIMEOUT_MS);
    connect(_timeoutTimer, &QTimer::timeout, this, &UavModelDecryptionManager::_decryptionTimeout);

    qCDebug(UavModelDecryptionManagerLog) << "UavModelDecryptionManager created";
}

UavModelDecryptionManager::~UavModelDecryptionManager() { _cleanup(); }

void UavModelDecryptionManager::setVehicle(Vehicle* vehicle) {
    if (_vehicle == vehicle) {
        return;
    }

    // Disconnect from old vehicle
    if (_vehicle && _mavlinkConnection) {
        disconnect(_mavlinkConnection);
    }

    _vehicle = vehicle;

    // Connect to new vehicle's MAVLink messages
    if (_vehicle) {
        _mavlinkConnection = connect(_vehicle, &Vehicle::mavlinkMessageReceived, this,
                                     &UavModelDecryptionManager::_handleMavlinkMessage);
        qCDebug(UavModelDecryptionManagerLog) << "Connected to vehicle MAVLink messages";
    }

    // Reset decryption state when vehicle changes
    resetDecryption();

    emit vehicleChanged();
}

void UavModelDecryptionManager::startModelDecryption(const QByteArray& signedToken) {
    if (!_vehicle) {
        qCWarning(UavModelDecryptionManagerLog) << "No vehicle connected";
        emit decryptionFailed("No vehicle connected");
        return;
    }

    if (signedToken.size() != SIGNED_TOKEN_SIZE) {
        qCWarning(UavModelDecryptionManagerLog)
            << "Invalid key data size:" << signedToken.size() << "expected:" << SIGNED_TOKEN_SIZE;
        emit decryptionFailed(
            QString("Invalid key data size: %1, expected: %2").arg(signedToken.size()).arg(SIGNED_TOKEN_SIZE));
        return;
    }

    if (_isDecrypting) {
        qCWarning(UavModelDecryptionManagerLog) << "Model decryption already in progress";
        return;
    }

    qCDebug(UavModelDecryptionManagerLog) << "Starting model decryption with key data size:" << signedToken.size();

    _decryptState = DECRYPT_WAITING_BLINDED_TOKEN;
    _setDecrypting(true);
    _setDecrypted(false);

    // Start timeout timer
    _timeoutTimer->start();

    // Send initial key data via TUNNEL message
    _sendTunnelMessage(signedToken, static_cast<int>(PayloadType::SignedToken));

    qCDebug(UavModelDecryptionManagerLog) << "Key data sent, waiting for confirmation...";
}

void UavModelDecryptionManager::resetDecryption() {
    qCDebug(UavModelDecryptionManagerLog) << "Resetting decryption state";

    _cleanup();
    _decryptState = DECRYPT_IDLE;
    _setDecrypting(false);
    _setDecrypted(false);
}

void UavModelDecryptionManager::_handleMavlinkMessage(const mavlink_message_t& message) {
    // Only process TUNNEL messages
    if (message.msgid != MAVLINK_MSG_ID_TUNNEL) {
        return;
    }

    mavlink_tunnel_t tunnel;
    mavlink_msg_tunnel_decode(&message, &tunnel);

    // Check if message is from onboard computer and is our decryption payload type
    if (tunnel.payload_type != static_cast<int>(PayloadType::BlindedToken) &&
        tunnel.payload_type != static_cast<int>(PayloadType::ModelDecryptionAck)) {
        return;
    }

    QByteArray payload(reinterpret_cast<const char*>(tunnel.payload), tunnel.payload_length);

    qCInfo(UavModelDecryptionManagerLog) << "Received TUNNEL message, state:" << _decryptState
                                         << "payload size:" << payload.size();
    switch (_decryptState) {
        case DECRYPT_WAITING_BLINDED_TOKEN:
            if (payload.size() == BLINDED_TOKEN_SIZE &&
                tunnel.payload_type == static_cast<int>(PayloadType::BlindedToken)) {
                _processBlindedToken(payload);
            } else {
                qCWarning(UavModelDecryptionManagerLog) << "Invalid key confirmation received";
                emit decryptionFailed("Invalid key confirmation");
                resetDecryption();
            }
            break;

        case DECRYPT_WAITING_FINAL_ACK:
            // Process final confirmation from UAV that model is decrypted and ready
            qCInfo(UavModelDecryptionManagerLog) << "Received decryption completion confirmation";
            _timeoutTimer->stop();
            _decryptState = DECRYPT_COMPLETED;
            _setDecrypting(false);
            _setDecrypted(true);
            emit decryptionCompleted();
            break;

        default:
            qCDebug(UavModelDecryptionManagerLog) << "Received tunnel message in unexpected state:" << _decryptState;
            break;
    }
}

void UavModelDecryptionManager::_sendTunnelMessage(const QByteArray& payload, uint16_t payloadType) {
    if (!_vehicle) {
        qCWarning(UavModelDecryptionManagerLog) << "Cannot send tunnel message: no vehicle";
        return;
    }

    mavlink_message_t msg;
    mavlink_tunnel_t tunnel = {};

    tunnel.target_system = 1;
    // TODO: find out the target_component dynamically or send the tunnel to each of the possible ONBOARD_COMPUTER instances
    tunnel.target_component = MAV_COMP_ID_ONBOARD_COMPUTER2;
    tunnel.payload_type = payloadType;
    tunnel.payload_length = qMin(payload.size(), static_cast<int>(sizeof(tunnel.payload)));

    memcpy(tunnel.payload, payload.data(), tunnel.payload_length);

    // get strong pointer to link
    SharedLinkInterfacePtr link = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (link) {
        mavlink_msg_tunnel_encode_chan(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::getComponentId(),
                                       link->mavlinkChannel(), &msg, &tunnel);

        _vehicle->sendMessageOnLinkThreadSafe(link.get(), msg);
    } else {
        qCWarning(UavModelDecryptionManagerLog) << "No active primary link to send tunnel message";
        return;
    }

    qCDebug(UavModelDecryptionManagerLog) << "Sent TUNNEL message, payload size:" << tunnel.payload_length;
}

void UavModelDecryptionManager::_processBlindedToken(const QByteArray& blindedToken) {
    qCDebug(UavModelDecryptionManagerLog) << "Processing blinded token, size:" << blindedToken.size();

    // Generate final decryption payload based on the key confirmation
    QByteArray unlockKey = _generateUnlockKey(blindedToken);

    if (unlockKey.size() != BLINDED_DEV_UNLOCK_KEY_SIZE) {
        qCWarning(UavModelDecryptionManagerLog) << "Generated invalid decryption payload size:" << unlockKey.size();
        emit decryptionFailed("Failed to generate decryption payload");
        resetDecryption();
        return;
    }

    _decryptState = DECRYPT_WAITING_FINAL_ACK;

    // Send final decryption payload
    _sendTunnelMessage(unlockKey, static_cast<int>(PayloadType::BlindedDevUnlockKey));

    qCDebug(UavModelDecryptionManagerLog) << "Final decryption payload sent, waiting for completion confirmation...";
}

QByteArray UavModelDecryptionManager::_generateUnlockKey(const QByteArray& blindedToken) {
    try {
        if (!_yubiKeyECDH) {
            // TODO: make the user prompt to enter the PIN code
            const std::string userPin{"123456"};

            // If we haven't created the yubikey object, then try to create it now
            _yubiKeyECDH = std::make_unique<yubi::YubiKeyECDH>(userPin);
        }
        // TODO: take the curve information (like the coordinate size) from some config
        const int coordSize = 32; // For now, using 32 bytes, as for NIST P-256 curve
        const auto unlockKey = _yubiKeyECDH->perform(blindedToken, coordSize);
        return unlockKey;
    } catch (const yubi::YubiKeyError &e) {
        qWarning() << "YUBI KEY ECDH ERROR: " << e.what();
    }

    return {};
}

void UavModelDecryptionManager::_decryptionTimeout() {
    qCWarning(UavModelDecryptionManagerLog) << "Model decryption timeout in state:" << _decryptState;

    emit decryptionFailed("Model decryption timeout");
    resetDecryption();
}

void UavModelDecryptionManager::_setDecrypting(bool decrypting) {
    if (_isDecrypting != decrypting) {
        _isDecrypting = decrypting;
        emit isDecryptingChanged();
    }
}

void UavModelDecryptionManager::_setDecrypted(bool decrypted) {
    if (_isDecrypted != decrypted) {
        _isDecrypted = decrypted;
        emit isDecryptedChanged();
    }
}

void UavModelDecryptionManager::_cleanup() { _timeoutTimer->stop(); }
