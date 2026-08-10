#include "CopterMission.h"
#include "FoxFourAutoPilotPlugin.h"
#include "ParameterManager.h"
#include "Vehicle.h"

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
            //trying to refresh parameter if exist, do not throw failure, if parameter does not exist.
            pm->refreshParameter(compId, _requiredParameters[i], false);
        }
    }
    _parametersReady = _requiredParameters.isEmpty();
    if(_parametersReady) {
        emit parametersReadyChanged();
    }
    _update();
}

void CopterMission::_handleFacts(int componentId, Fact* fact)
{
    // ignoring FCU parameters for now
    if (componentId == _vehicle->defaultComponentId()) {
        return;
    }

    if (fact->name() == "MISSN_GUID_TYPE") {
        connect(fact, &Fact::rawValueChanged, this, [this]([[maybe_unused]] const QVariant& value) { _update(); });
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
    _update();
}

void CopterMission::_update()
{
    if (_status == Unavailable || _missionChangeFact == nullptr) {
        return;
    }
    setStatus(_missionIndx == _missionChangeFact->rawValue().toInt() ? _parametersReady ? CopterState::Status::Enable : CopterState::Status::Warning : CopterState::Status::Disable);
}
