#pragma once

#include <QtCore/QObject>

#include "AutoPilotWidget/AutoPilotWidget.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(EKSourcesLog)

class Vehicle;
class Fact;

class EKSources : public AutoPilotWidget
{
    Q_OBJECT
    Q_PROPERTY(QStringList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(Fact* ekfSrcFact MEMBER _ekfSrcFact NOTIFY ekfSrcFactChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)

public:
    EKSources(Vehicle* vehicle, QObject* parent);
    QStringList sources() const;
    Fact* ekfSrcFact();
    bool visible();

    bool dirty() { return _dirty; }

    Q_INVOKABLE void setSource(int index);

signals:
    void sourcesChanged();
    void visibleChanged();
    void ekfSrcFactChanged();
    void dirtyChanged();

private slots:
    void _handleFact(Fact* fact);

private:
    static void _changeSrcHandler(void* responceData, [[maybe_unused]] int compid, const mavlink_command_ack_t& ack,
                                  Vehicle::MavCmdResultFailureCode_t failureCode);
    void _setDirty(bool newState);

private:
    Vehicle* _vehicle = nullptr;
    QStringList _sources;
    Fact* _ekfSrcFact = nullptr;
    QVector<Fact*> _sourcesFact;
    bool _dirty = false;
    int _refreshFailedCount = 0;
    const int _maximumFaildeCount = 3;
};
