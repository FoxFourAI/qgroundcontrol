#pragma once

#include <QtCore/QObject>
#include "FactGroup.h"
#include "Vehicle.h"
#include <QVariantList>

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

class CopterMission : public CopterState{
    Q_OBJECT
    Q_PROPERTY(QList<Fact*> tunableParameters READ tunableParameters NOTIFY tunableParametersChanged FINAL)
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
signals:
    void tunableParametersChanged();

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

};

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

class CopterConfigurator: public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<CopterType*> copterTypes READ copterTypes NOTIFY copterTypesChanged FINAL)
    Q_PROPERTY(CopterType* currentType READ currentType NOTIFY currentTypeChanged FINAL)
public:


    CopterConfigurator(Vehicle* vehicle, QObject* parent=nullptr);
    QList<CopterType*> copterTypes() const;
    CopterType *currentType() const;
    void handleVGMReboot();
signals:
    void copterTypesChanged();
    void currentTypeChanged();
private:
    void _currentTypeChangedCallback();
    void _init();

private:
    Vehicle *_vehicle =nullptr;
    QList<CopterType *> _types;
    CopterType* _currentType = nullptr;

};
