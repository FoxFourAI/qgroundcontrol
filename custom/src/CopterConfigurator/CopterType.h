#pragma once

#include "CopterMission.h"

class CopterType : public CopterState{
    Q_OBJECT
    Q_PROPERTY(QList<CopterMission*> missions READ missions NOTIFY missionsChanged)
    Q_PROPERTY(CopterMission* currentMission READ currentMission NOTIFY currentMissionChanged)
public:
    enum Type{
        Unknown = -1,
        Kamikaze,
        Plane,
        Bomber,
        Photolit
    };
    void resetVgmBootType();
    static const QMap<Type,QString> type2name;
    CopterType(Type type, Vehicle* vehicle, QObject* parent = nullptr);
    QList<CopterMission *> missions() const;
    Q_INVOKABLE void setActive() override;
    CopterMission* currentMission();
signals:
    void currentMissionChanged();
    void missionsChanged();

private:
    void _currentMissionChangedCallback();
    void _update() override;
    void _handleFacts(int componentId, Fact* fact);
private:
    CopterMission * _currentMission = nullptr;
    Fact* _typeChangeFact = nullptr;
    Type _vgmBootType = Unknown;
    Type _type;
    QList<CopterMission*> _missions;

};
