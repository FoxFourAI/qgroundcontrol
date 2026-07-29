#include "CopterType.h"
#include <QTimer>
#include "Vehicle.h"
#include "ParameterManager.h"

const QMap<CopterType::Type, QString> CopterType::type2name = {
    {Kamikaze, "Kamikaze"}, {Plane, "Plane"}, {Bomber, "Bomber"}, {Photolit, "Photolit"}};

QList<CopterMission*> CopterType::missions() const
{
    return _missions;
}

void CopterType::setActive()
{
    if (_typeChangeFact != nullptr) {
        _typeChangeFact->setRawValue(_type);
    }
}

CopterMission* CopterType::currentMission()
{
    return _currentMission;
}

void CopterType::_currentMissionChangedCallback()
{
    CopterMission* newMission = reinterpret_cast<CopterMission*>(sender());
    if (newMission->status() == Status::Unavailable || newMission->status() == Status::Disable) {
        return;
    }
    if (newMission == _currentMission) {
        return;
    }
    _currentMission = newMission;
    emit currentMissionChanged();
}

void CopterType::_update()
{
    if (_status == Unavailable || _vgmBootType == Unknown) {
        return;
    }

    if (Type(_typeChangeFact->rawValue().toInt()) == _type) {
        setStatus(_type == _vgmBootType ? Status::Enable : Status::Warning);
    } else {
        setStatus(Status::Disable);
    }
}

void CopterType::_handleFacts(int componentId, Fact* fact)
{
    if (componentId == _vehicle->defaultComponentId()) {
        return;
    }
    if (fact->name() == "GUID_FRAME_TYPE") {
        connect(fact, &Fact::rawValueChanged, this, [this](const QVariant& /*value*/) { _update(); });
        // dont know why, but the first value that appears when Fact received, is 0, so
        // trigger the timer to update it after a second of delay....
        _typeChangeFact = fact;
        QTimer::singleShot(1000, [this]() {
            _vgmBootType = Type(_typeChangeFact->rawValue().toInt());
            _update();
        });
    }
}

void CopterType::resetVgmBootType()
{
    _vgmBootType = Type(_typeChangeFact->rawValue().toInt());
    _update();
}

CopterType::CopterType(Type type, Vehicle* vehicle, QObject* parent) : CopterState(vehicle, parent)
{
    _type = type;
    setName(type2name[type]);
    connect(vehicle->parameterManager(), &ParameterManager::factAdded, this, &CopterType::_handleFacts);
    switch (type) {
        case Kamikaze:
            _missions.append(new CopterMission(CopterMission::Disable, {"MISSN_AUTONOMY", "MISSN_ONE_WAY"}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::Hover, {"MISSN_AUTONOMY"}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::TerminalAttack, {"MISSN_AUTONOMY", "MISSN_TERM_VEL", "MISSN_ONE_WAY"}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::Tuning, {"MISSN_AUTONOMY"}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::Cruise, {"MISSN_AUTONOMY", "MISSN_CRUISE_VEL", "MISSN_ONE_WAY"}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::TerminalBombing, {"MISSN_AUTONOMY", "MISSN_BOMB_BIAS", "MISSN_ASCEND_ALT", "MISSN_BOMB_ALT"}, _vehicle, this));
            break;
        case Plane:
            _missions.append(new CopterMission(CopterMission::Disable, {}, _vehicle, this));
            break;

        case Bomber:
            _missions.append(new CopterMission(CopterMission::Disable, {}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::Hover, {}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::VisualPosHold,{}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::Tuning, {}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::Cruise, {"MISSN_CRUISE_VEL"}, _vehicle, this));
            _missions.append(new CopterMission(CopterMission::SupplyDelivery,{"MISSN_DRP_ALT", "MISSN_DRP_SPD_DN", "MISSN_DRP_SPD_UP", "MISSN_DRP_SRV_N", "MISSN_DRP_SRV_PW"},
                                               _vehicle, this));
            break;
        case Photolit:
            _missions.append(new CopterMission(CopterMission::Disable, {}, _vehicle, this));
            break;

        default:
            break;
    }
    _currentMission = _missions.first();
    for (auto* mission : _missions) {
        connect(mission, &CopterMission::statusChanged, this, &CopterType::_currentMissionChangedCallback);
    }
}
