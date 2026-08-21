#include "AutoPilotWidget.h"

#include "FoxFourAutoPilotPlugin.h"
#include "ParameterManager.h"
#include "Vehicle.h"

AutoPilotWidget::AutoPilotWidget(Vehicle* vehicle, QObject* parent) : QObject(parent), _vehicle(vehicle)
{
    _pm = _vehicle->parameterManager();
    connect(_pm, &ParameterManager::factAdded, this, &AutoPilotWidget::_handleParameter);
    _apm = reinterpret_cast<FoxFourAutoPilotPlugin*>(parent);
    _ocm = _apm->onboardComputersManager();
    connect(_ocm, &OnboardComputersManager::currentComputerComponentChanged, this,
            &AutoPilotWidget::_handleVGMIdChanged);
    connect(_pm, &ParameterManager::parametersReadyChanged, this, [this](bool ready) {
        if (ready && !parametersReady()) {
            refreshParameters();
        }
    });
}

void AutoPilotWidget::refreshParameters()
{
    for (ParameterInfo& param : _requiredParametrs) {
        int componentId = param.fromVGM ? _ocm->currentComputerComponent() : MAV_COMP_ID_AUTOPILOT1;
        _pm->refreshParameter(componentId, param.name);
    }
}

void AutoPilotWidget::_setRequiredParameters(QVector<ParameterInfo> parameters)
{
    _requiredParametrs = parameters;
}

void AutoPilotWidget::_handleParameter(int componentId, Fact* parameter)
{
    bool isFromVGM = _ocm->currentComputerComponent() != 0 && componentId == _ocm->currentComputerComponent();
    bool isFromFCU = componentId == MAV_COMP_ID_AUTOPILOT1;
    for (ParameterInfo& param : _requiredParametrs) {
        if (param.fact != nullptr || param.name != parameter->name()) {
            continue;
        }
        if (param.fromVGM ? isFromVGM : isFromFCU) {
            param.fact = parameter;
            emit _requiredFactChanged(param.fact);
            _checkParametersReady();
            return;
        }
    }
}

void AutoPilotWidget::_handleVGMIdChanged(uint8_t newCompId)
{
    bool parametersChanged = false;

    // we clear all parameters from the VGM to update them.
    for (ParameterInfo& param : _requiredParametrs) {
        if (!param.fromVGM) {
            continue;
        }
        param.fact = nullptr;
        emit _requiredFactChanged(param.fact);
        parametersChanged = true;
    }

    // we do not have a valid vgm for now, so return.
    if (newCompId == 0) {
        return;
    }

    if (parametersChanged) {
        refreshParameters();
    }
}

void AutoPilotWidget::_checkParametersReady()
{
    for (const ParameterInfo& param : _requiredParametrs) {
        if (param.fact == nullptr) {
            _setParametersReady(false);
            return;
        }
    }
    _setParametersReady(true);
}

void AutoPilotWidget::_setParametersReady(bool ready)
{
    if (_parametersReady == ready) {
        return;
    }
    _parametersReady = ready;
    emit parametersReadyChanged();
}
