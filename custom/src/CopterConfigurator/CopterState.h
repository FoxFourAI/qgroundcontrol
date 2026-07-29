#pragma once

#include <QtCore/QObject>

class Vehicle;

class CopterState : public QObject{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
public:
    enum Status{
        Disable,
        Enable,
        Warning,
        Error,
        Unavailable
    };
    CopterState(Vehicle* vehicle, QObject* parent = nullptr): QObject(parent), _vehicle(vehicle){}
    [[nodiscard]] Status status() const;
    [[nodiscard]] QString name() const;

    virtual void setStatus(Status newStatus);
    virtual void setName(const QString &newName);
    Q_INVOKABLE virtual void setActive() = 0;

signals:
    void nameChanged();
    void statusChanged();

protected:
    Vehicle* _vehicle = nullptr;
    virtual void _update() = 0;
    QString _name;
    Status _status = Disable;

};
