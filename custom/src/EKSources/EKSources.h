#pragma once

#include <QtCore/QObject>

#include "QGCLoggingCategory.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(EKSourcesLog)

class Vehicle;

class EKSources : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(int currentSource READ currentSource NOTIFY currentSourceChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
public:
    EKSources(Vehicle* vehicle, QObject* parent);
    QStringList sources() const;
    int currentSource();
    bool visible();
    bool dirty() {return _dirty;}
    Q_INVOKABLE void setSource(int index);
signals:
    void sourcesChanged();
    void visibleChanged();
    void currentSourceChanged();
    void dirtyChanged();
private slots:
    void _fetchSources(bool ready);
    void _setVisible(bool visible);
    void _setCurrentSource(int indx);

private:
    static void _changeSrcHandler(void* responceData,[[maybe_unused]] int compid, const mavlink_command_ack_t& ack,
                                  Vehicle::MavCmdResultFailureCode_t failureCode);
    void _setDirty(bool newState);

private:
    Vehicle* _vehicle = nullptr;
    const QString _ekfParamName = "SCR_EKF_SRC";
    QStringList _sources;
    bool _canSwitchSources = false;
    bool _visible = false;
    bool _dirty = false;
    QMetaObject::Connection _paramConnection;
    int _refreshFailedCount = 0;
    int _currentSource = -1;
    const int _maximumFaildeCount = 3;
};
