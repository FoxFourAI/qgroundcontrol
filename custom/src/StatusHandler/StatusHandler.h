#ifndef STATUSHANDLER_H
#define STATUSHANDLER_H

#include <QObject>
class Vehicle;

class StatusHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap cpuInfo MEMBER _cpuInfo NOTIFY statusReceived)
    Q_PROPERTY(QVariantMap ramInfo MEMBER _ramInfo NOTIFY statusReceived)
    Q_PROPERTY(QVariantList storageInfo MEMBER _storageInfo NOTIFY statusReceived)
    // Implement other fields when they will be available
public:
    explicit StatusHandler(Vehicle* vehicle, QObject* parent = nullptr);

signals:
    void statusReceived();
private slots:
    void handleMavlinkMessage(const mavlink_message_t& msg);

private:
    const QStringList _storageNames{"HDD", "SSD", "EMMC", "SD", "SD+"};

    Vehicle* _vehicle = nullptr;
    /** cpuInfo
     *  {
     *   "totalUsage": 0-100%,
     *   "trace"[0-10]: {0-100%}
     *   "cores"[1-8] : {0-100%}
     *  }
     */
    QVariantMap _cpuInfo;
    /** ram info
     *  {
     *   "total": GB,
     *   "usage": GB
     *  }
     */
    QVariantMap _ramInfo;
    /** storage info
     * [1-4]{
     *          {"type": "HDD/SSD/EMMC/SD(Not removable)/SD",
     *           "usage": GB,
     *           "total": GB
     *          }
     *      }
     */
    QVariantList _storageInfo;
};

#endif  // STATUSHANDLER_H
