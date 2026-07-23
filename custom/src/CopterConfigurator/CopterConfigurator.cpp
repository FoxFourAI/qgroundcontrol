#include "CopterConfigurator.h"

#include <QTimer>

#include "FoxFourAutoPilotPlugin.h"
#include "ParameterManager.h"
//-----------------------CopterState-----------------------
CopterState::Status CopterState::status() const
{
    return _status;
}

QString CopterState::name() const
{
    return _name;
}

void CopterState::setStatus(Status newStatus)
{
    if (_status == newStatus) {
        return;
    }
    _status = newStatus;
    emit statusChanged();
}

void CopterState::setName(const QString& newName)
{
    if (_name == newName)
        return;
    _name = newName;
    emit nameChanged();
}

//-----------------------CopterMission-----------------------
const QMap<CopterMission::Type, int> CopterMission::type2index{
    {Disable, -1}, {Hover, 0},   {TerminalAttack, 1},
    {Tuning, 2},   {Cruise, 3}, {SupplyDelivery, 4},
    {VisualPosHold, 1}, {TerminalBombing, 4}
};
const QMap<CopterMission::Type, QString> CopterMission::type2name{
    {Disable, "Disable"}, {Hover, "Hover"},   {TerminalAttack, "Terminal Attack"},
    {Tuning, "Tuning"},   {Cruise, "Cruise"}, {SupplyDelivery, "Supply Delivery"},
    {VisualPosHold, "Visual position hold"}, {TerminalBombing,"Terminal Bombing"}};

CopterMission::CopterMission(CopterMission::Type type, QStringList tunableParametersNames, Vehicle* vehicle,
                             QObject* parent)
    : CopterState(vehicle, parent), _requiredParameters(tunableParametersNames)
{
    _requiredParameters.removeDuplicates();
    _type = type;
    setName(type2name[_type]);
    // all missions is cast 1:1 except of bomber
    _missionIndx = type2index[type];
    connect(vehicle->parameterManager(), &ParameterManager::factAdded, this, &CopterMission::_handleFacts);
}

QList<Fact*> CopterMission::tunableParameters()
{
    return _tunableParameters;
}

void CopterMission::setActive()
{
    if (_missionChangeFact != nullptr) {
        _missionChangeFact->setRawValue(_missionIndx);
    }
    checkParameters();
}

void CopterMission::checkParameters()
{
   //if we do not have all parameters that we need, try to pull them
    if (_parametersReady) {
        return;
    }
    ParameterManager* pm = _vehicle->parameterManager();
    int compId = reinterpret_cast<FoxFourAutoPilotPlugin*>(_vehicle->autopilotPlugin())
                     ->onboardComputersManager()
                     ->currentComputerComponent();
    if (compId == 0) {
        return;
    }
    for (int i = 0; i < _requiredParameters.length(); i++) {
        if (pm->parameterExists(compId, _requiredParameters[i])) {
            _tunableParameters.append(pm->getParameter(compId, _requiredParameters[i]));
            _requiredParameters.removeAt(i);
            i--;
        } else {
            pm->refreshParameter(compId, _requiredParameters[i]);
        }
    }
    _parametersReady = _requiredParameters.isEmpty();
    if(_parametersReady) {
        emit parametersReadyChanged();
    }
}

void CopterMission::_handleFacts(int componentId, Fact* fact)
{
    // ignoring FCU parameters for now
    if (componentId == _vehicle->defaultComponentId()) {
        return;
    }

    if (fact->name() == "MISSN_GUID_TYPE") {
        connect(fact, &Fact::rawValueChanged, this, [this](const QVariant& /*value*/) { _update(); });
        _missionChangeFact = fact;
        _update();
        return;
    }

    // if we get all needed parameters, returning.
    if (_requiredParameters.isEmpty()) {
        return;
    }

    if (_requiredParameters.contains(fact->name())) {
        _requiredParameters.removeAt(_requiredParameters.indexOf(fact->name()));
        _tunableParameters.append(fact);
        emit tunableParametersChanged();
    }

    if(_requiredParameters.empty()){
        _parametersReady = true;
        emit parametersReadyChanged();
    }
}

void CopterMission::_update()
{
    if (_status == Unavailable || _missionChangeFact == nullptr) {
        return;
    }
    setStatus(_missionIndx == _missionChangeFact->rawValue().toInt() ? Status::Enable : Status::Disable);
}

//-----------------------CopterType-----------------------
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

//-----------------------CopterConfigurator-----------------------

CopterConfigurator::CopterConfigurator(Vehicle* vehicle, QObject* parent) : QObject(parent), _vehicle(vehicle)
{
    _init();
}

void CopterConfigurator::_init()
{
    _types.push_back(new CopterType(CopterType::Kamikaze, _vehicle, this));
    _types.push_back(new CopterType(CopterType::Plane, _vehicle, this));
    _types.push_back(new CopterType(CopterType::Bomber, _vehicle, this));
    _types.push_back(new CopterType(CopterType::Photolit, _vehicle, this));
    _currentType = _types.first();
    for (CopterType* type : _types) {
        connect(type, &CopterType::statusChanged, this, &CopterConfigurator::_currentTypeChangedCallback);
    }
    emit copterTypesChanged();
}

QList<CopterType*> CopterConfigurator::copterTypes() const
{
    return _types;
}

CopterType* CopterConfigurator::currentType() const
{
    return _currentType;
}

void CopterConfigurator::_currentTypeChangedCallback()
{
    CopterType* newType = reinterpret_cast<CopterType*>(sender());
    if (newType->status() == CopterType::Status::Unavailable || newType->status() == CopterType::Status::Disable) {
        return;
    }

    if (newType == _currentType) {
        return;
    }
    _currentType = newType;
    qDebug() << "current Type changes";
    emit currentTypeChanged();
}

void CopterConfigurator::handleVGMReboot()
{
    for (auto* type : _types) {
        type->resetVgmBootType();
    }
}

bool CopterMission::parametersReady() const
{
    return _parametersReady;
}
