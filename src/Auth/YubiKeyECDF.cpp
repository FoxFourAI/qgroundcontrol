#include "YubiKeyECDF.h"

// TODO: on windows we may need to include just <winscard.h>
#include <PCSC/winscard.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace yubi {

void cout_packet(const std::vector<uint8_t> &packet, const size_t packet_len) {
    std::stringstream ss;
    for (size_t i = 0; i < std::min(packet.size(), packet_len); ++i) {
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(packet[i]) << " ";
    }
    // qDebug() << ss.str() << std::endl;
}

class YubiKeyHandler {
   private:
    SCARDCONTEXT context{};
    SCARDHANDLE card{};
    DWORD protocol{};

   public:
    YubiKeyHandler() {
        // Establish context
        LONG rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &context);
        if (rv != SCARD_S_SUCCESS) {
            throw YubiKeyError("Failed to establish PC/SC context");
        }

        // Find YubiKey reader
        auto readersLen = SCARD_AUTOALLOCATE;
        LPSTR readers = nullptr;
        rv = SCardListReaders(context, nullptr, (LPSTR)&readers, &readersLen);
        if (rv != SCARD_S_SUCCESS) {
            SCardReleaseContext(context);
            throw YubiKeyError("Failed to list readers");
        }

        // Find YubiKey (simplified - assumes first reader)
        std::string readerName(readers);
        if (readerName.find("Yubico") == std::string::npos) {
            SCardFreeMemory(context, readers);
            SCardReleaseContext(context);
            throw YubiKeyError("YubiKey not found");
        }

        // Connect to card
        rv =
            SCardConnect(context, readers, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1 | SCARD_PROTOCOL_T0, &card, &protocol);
        SCardFreeMemory(context, readers);

        if (rv != SCARD_S_SUCCESS) {
            SCardReleaseContext(context);
            throw YubiKeyError("Failed to connect to YubiKey");
        }

        selectOpenPGP();
    }

    ~YubiKeyHandler() {
        SCardDisconnect(card, SCARD_LEAVE_CARD);
        SCardReleaseContext(context);
    }

    std::vector<uint8_t> sendAPDU(const std::vector<uint8_t> &apdu, const std::string &operation) {
        SCARD_IO_REQUEST pioSendPci = (protocol == SCARD_PROTOCOL_T1) ? *SCARD_PCI_T1 : *SCARD_PCI_T0;

        qDebug() << operation << " APDU: ";
        cout_packet(apdu, apdu.size());

        std::vector<uint8_t> response(258);  // Max APDU response
        DWORD responseLen = response.size();

        LONG rv = SCardTransmit(card, &pioSendPci, apdu.data(), apdu.size(), nullptr, response.data(), &responseLen);

        if (rv != SCARD_S_SUCCESS) {
            throw YubiKeyError(operation + " transmission failed");
        }

        if (responseLen < 2) {
            throw YubiKeyError(operation + " invalid response length");
        }

        uint8_t sw1 = response[responseLen - 2];
        uint8_t sw2 = response[responseLen - 1];

        std::stringstream ss;
        ss << operation << " response len: " << responseLen << " SW1: " << std::hex << std::uppercase
                 << std::setw(2) << std::setfill('0') << static_cast<int>(sw1) << ", SW2: " << static_cast<int>(sw2);
        qDebug() << ss.str();
        cout_packet(response, responseLen);

        if (sw1 != 0x90 || sw2 != 0x00) {
            throw YubiKeyError(operation + " failed: " + std::to_string(sw1) + std::to_string(sw2));
        }

        response.resize(responseLen - 2);  // Remove status bytes
        return response;
    }

    void selectOpenPGP() {
        std::vector<uint8_t> apdu = {0x00, 0xA4, 0x04, 0x00, 0x06, 0xD2, 0x76, 0x00, 0x01, 0x24, 0x01};
        sendAPDU(apdu, "Select OpenPGP");
    }

    void getPWStatus() {
        std::vector<uint8_t> apdu = {0x00, 0xCA, 0x00, 0xC4, 0x00};
        sendAPDU(apdu, "Get PW Status");
    }

    void authenticatePIN(const std::string &pin) {
        if (pin.empty()) return;

        std::vector<uint8_t> pinBytes(pin.begin(), pin.end());

        for (uint8_t p2 : {0x82, 0x81}) {
            // PW2 then PW1
            std::vector<uint8_t> apdu = {0x00, 0x20, 0x00, p2, static_cast<uint8_t>(pinBytes.size())};
            apdu.insert(apdu.end(), pinBytes.begin(), pinBytes.end());
            sendAPDU(apdu, "PIN Auth");
        }
    }

    std::vector<uint8_t> performECDH(const std::vector<uint8_t> &pointBytes, int coordSize) {
        if (pointBytes.size() != 1 + 2 * coordSize) {
            throw std::invalid_argument("Invalid point bytes length");
        }

        int pointSize = 2 * coordSize;
        std::vector<uint8_t> tlvPrefix = generateTLVPrefix(pointSize);

        std::vector<uint8_t> pointData = tlvPrefix;
        pointData.insert(pointData.end(), pointBytes.begin(), pointBytes.end());
        pointData.push_back(0x00);

        std::vector<uint8_t> apdu = {0x00, 0x2A, 0x80, 0x86, static_cast<uint8_t>(pointData.size())};
        apdu.insert(apdu.end(), pointData.begin(), pointData.end());

        std::vector<uint8_t> response = sendAPDU(apdu, "Decipher");

        if (response.size() != coordSize) {
            throw YubiKeyError("Invalid result format");
        }

        return response;
    }

   private:
    static std::vector<uint8_t> generateTLVPrefix(int pointSize) {
        uint8_t pointTagLength = pointSize + 1;
        uint8_t pubkeyDOLength = 2 + pointTagLength;
        uint8_t outerTagLength = 3 + pubkeyDOLength;

        return {0xA6, outerTagLength, 0x7F, 0x49, pubkeyDOLength, 0x86, pointTagLength};
    }
};

YubiKeyECDH::YubiKeyECDH(const std::string &pin) : m_yubikey{new YubiKeyHandler()} {
    m_yubikey->selectOpenPGP();
    m_yubikey->getPWStatus();
    m_yubikey->authenticatePIN(pin);
}

QByteArray YubiKeyECDH::perform(const QByteArray &pubPoint, int coordSize) {
    // Make a conversion betwen QT-style QByteArray and common C++-style
    std::vector<uint8_t> vectorPubPoint(pubPoint.begin(), pubPoint.end());
    auto res = m_yubikey->performECDH(vectorPubPoint, coordSize);
    return QByteArray(reinterpret_cast<const char*>(res.data()), res.size());
}

}  // namespace yubi
