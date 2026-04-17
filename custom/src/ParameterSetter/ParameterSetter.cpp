#include "ParameterSetter.h"

#include "MultiVehicleManager.h"
#include "Vehicle.h"

bool ParameterSetter::parameterExits(int compId, QString paramName) {
    return getFact(compId, paramName, false) != nullptr;
}

QString ParameterSetter::getParameter(int compId, QString paramName, bool report) {
    auto parameter = getFact(compId, paramName, report);
    if (parameter == nullptr) {
        return QString();
    }
    return parameter->rawValueString();
}

Fact* ParameterSetter::getFact(int compId, QString paramName, bool report) {
    auto vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (vehicle == nullptr) {
        return nullptr;
    }
    auto parameterManager = vehicle->parameterManager();
    if (!parameterManager->parametersReady()) {
        return nullptr;
    }
    Fact* fact = nullptr;
    if (report) {
        fact = parameterManager->getParameter(compId, paramName);
    } else {
        bool parameterExist = parameterManager->parameterExists(compId, paramName);
        if (parameterExist) {
            fact = parameterManager->getParameter(compId, paramName);
        }
    }
    return fact;
}

bool ParameterSetter::setParameter(int compId, QString paramName, float value) {
    auto vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (vehicle == nullptr) {
        return false;
    }
    auto parameterManager = vehicle->parameterManager();
    if (!parameterManager->parametersReady()) {
        return false;
    }
    auto parameter = parameterManager->getParameter(compId, paramName);
    if (parameter == nullptr) {
        return false;
    }
    parameter->setRawValue(value);
    return true;
}
