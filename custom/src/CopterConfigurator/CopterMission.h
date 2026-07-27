#pragma once

#include "CopterState.h"
#include "Fact.h"

class CopterMission : public CopterState{
    Q_OBJECT
    Q_PROPERTY(QList<Fact*> tunableParameters READ tunableParameters NOTIFY tunableParametersChanged FINAL)
    Q_PROPERTY(bool parametersReady READ parametersReady NOTIFY parametersReadyChanged FINAL)
public:
    enum Type{
        Disable = -1,
        Hover,
        TerminalAttack,
        Tuning,
        Cruise,
        SupplyDelivery,
        VisualPosHold,
        TerminalBombing
    };

    CopterMission(Type type, QStringList tunableParameters, Vehicle* vehicle, QObject* parent = nullptr);
    QList<Fact*> tunableParameters();
    Q_INVOKABLE void setActive() override;
    Q_INVOKABLE void checkParameters();
    bool parametersReady() const;

signals:
    void tunableParametersChanged();

    void parametersReadyChanged();

private slots:
    void _handleFacts(int componentId, Fact* fact);
    void _update() override;

private:
    static const QMap<Type,int>     type2index;
    static const QMap<Type,QString> type2name;
    Type _type;
    int _missionIndx = -1;
    Fact* _missionChangeFact = nullptr;
    QStringList _requiredParameters;
    QList<Fact*> _tunableParameters;
    bool _parametersReady = false;

};
