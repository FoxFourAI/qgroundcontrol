#pragma once

#include <QtCore/QObject>
#include "Vehicle.h"

class Vehicle;

class EKSources : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(int currentSource READ currentSource NOTIFY currentSourceChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
public:
    EKSources(Vehicle* vehicle, QObject* parent);
    QStringList sources() const;
    int currentSource();
    bool visible();
    Q_INVOKABLE void setSource(int index);
signals:
    void sourcesChanged();
    void visibleChanged();
    void currentSourceChanged();
private slots:
    void _fetchSources(bool ready);
    void _setVisible(bool visible);
    void _setCurrentSource(int indx);

private:
   static  void _changeSrcHandler(void* responceData,int /*compid*/, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);
private:
    QMetaObject::Connection _paramConnection;
    QStringList _sources;
    Vehicle* _vehicle = nullptr;
    bool _visible = false;
    int _currentSource = -1;
};
