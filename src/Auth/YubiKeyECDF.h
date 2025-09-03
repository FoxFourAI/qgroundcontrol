#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

namespace yubi {
class YubiKeyError : public std::runtime_error {
   public:
    explicit YubiKeyError(const std::string &msg) : std::runtime_error(msg) {}
};

// Hiding the class from the main application to make it as independent as possible
class YubiKeyHandler;

/*!
 * Class for performing the ECDH with a yubikey
 * @throws YubiKeyError in case some operation fails
 */
class YubiKeyECDH {
   private:
    std::shared_ptr<YubiKeyHandler> m_yubikey{nullptr};

   public:
    YubiKeyECDH() = delete;
    /*!
     *
     * @param pin user PIN for the yubikey connected to the computer
     */
    explicit YubiKeyECDH(const std::string &pin);

    /*!
     * Perform ECDH algorithm
     * @param pubPoint Public point on the eliptic curve. MUST be of size coordSize * 2 + 1 with
     * first byte being 0x04
     * @param coordSize coordinate size in bytes for the specific (configured in yubikey and from
     * which the pubPoint is taken) EC. e.g. 32 for NIST P-256 and 48 for NIST P-384
     * @return shared secret returned by the yubikey of coordSize bytes
     */
    QByteArray perform(const QByteArray &pubPoint, int coordSize);
};

}  // namespace yubi
