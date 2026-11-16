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
    Q_INVOKABLE void download(const QString &from, const QString &to);
    Q_INVOKABLE void remove(const QString &file);
    Q_INVOKABLE void updateList(const QString &path);

public slots:
    void setTargetComponent(int componentId);

private slots:
    void uploadFinishHandle(const QString& file, const QString& errorMsg);
signals:
    void canTransferChanged();

private:
    Vehicle *_vehicle = nullptr;
    bool _ftpAvailable;
    int _targetComtonent = MAV_COMP_ID_ALL;
};
