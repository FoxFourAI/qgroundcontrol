#pragma once
#include <QObject>

class Vehicle;
class FtpComponentManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canTransfer READ canTransfer NOTIFY canTransferChanged)
public:
    FtpComponentManager(Vehicle *vehicle, QObject *parent);

    [[nodiscard]] bool canTransfer() const;
    Q_INVOKABLE void upload(const QString &file);
    Q_INVOKABLE void download(const QString &file);

public slots:
    void setTargetComponent(int componentId);
signals:
    void canTransferChanged();

private:
    Vehicle *_vehicle = nullptr;
    bool _ftpAvailable;
    int _targetComtonent = MAV_COMP_ID_ALL;
};
