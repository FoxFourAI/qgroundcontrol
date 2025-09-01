#pragma once

#include <QByteArray>
#include <QLoggingCategory>
#include <QObject>
#include <QTimer>

#include "MAVLinkProtocol.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(UavModelDecryptionManagerLog)

/**
 * @brief UavModelDecryptionManager handles ML model decryption key exchange with UAV
 *
 * This class manages the key exchange protocol for model decryption that:
 * 1. Sends initial 128-bit decryption key/parameters to onboard computer
 * 2. Receives 32-byte key confirmation/derivation data from UAV
 * 3. Processes response and sends final 128-byte decryption payload
 */
class UavModelDecryptionManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isDecrypting READ isDecrypting NOTIFY isDecryptingChanged)
    Q_PROPERTY(bool isDecrypted READ isDecrypted NOTIFY isDecryptedChanged)
    Q_PROPERTY(Vehicle* vehicle READ vehicle WRITE setVehicle NOTIFY vehicleChanged)

   public:
    static UavModelDecryptionManager* instance();

    explicit UavModelDecryptionManager(QObject* parent = nullptr);
    ~UavModelDecryptionManager();

    // Property getters
    bool isDecrypting() const { return _isDecrypting; }
    bool isDecrypted() const { return _isDecrypted; }
    Vehicle* vehicle() const { return _vehicle; }

    // Property setters
    void setVehicle(Vehicle* vehicle);

   public slots:
    /**
     * @brief Stars model decryption key exchange with 128-bit key/parameters
     * @param signedToken 128-byte signed token that starts the decryption negotiation process
     */
    Q_INVOKABLE void startModelDecryption(const QByteArray& signedToken);

    /**
     * @brief Reset decryption state
     */
    Q_INVOKABLE void resetDecryption();

   signals:
    void isDecryptingChanged();
    void isDecryptedChanged();
    void vehicleChanged();
    void decryptionCompleted();
    void decryptionFailed(const QString& error);

   private slots:
    void _handleMavlinkMessage(const mavlink_message_t& message);
    void _decryptionTimeout();

   private:
    void _sendTunnelMessage(const QByteArray& payload, uint16_t payloadType = 0);
    void _processBlindedToken(const QByteArray& blindedToken);
    QByteArray _generateUnlockKey(const QByteArray& keyConfirmData);
    void _setDecrypting(bool decrypting);
    void _setDecrypted(bool decrypted);
    void _cleanup();

    // State management
    enum DecryptionState { DECRYPT_IDLE, DECRYPT_WAITING_BLINDED_TOKEN, DECRYPT_WAITING_FINAL_ACK, DECRYPT_COMPLETED };

    Vehicle* _vehicle;
    DecryptionState _decryptState;
    bool _isDecrypting;
    bool _isDecrypted;
    QTimer* _timeoutTimer;

    // MAVLink connection
    QMetaObject::Connection _mavlinkConnection;

    // Constants
    enum class PayloadType {
        SignedToken = 43,
        BlindedToken = 44,
        BlindedDevUnlockKey = 45,
        ModelDecryptionAck = 46
        // ACK is not specified in the protocol by TSIROT, but is a good thing to let the GCS know
        // that the last tunnel message was received
    };

    static constexpr int DECRYPT_TIMEOUT_MS = 10000;  // 10 second timeout
    static constexpr int SIGNED_TOKEN_SIZE = 128;
    static constexpr int BLINDED_TOKEN_SIZE = 32;
    static constexpr int BLINDED_DEV_UNLOCK_KEY_SIZE = 128;
    static constexpr int TUNNEL_MESSAGE_PAYLOAD_SIZE = 128;
};
