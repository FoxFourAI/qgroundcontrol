#include "ParameterSetter.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"


QString ParameterSetter::getParameter(int compId, QString paramName, bool report)
{
    auto parameter = getFact(compId, paramName, report);
    if( parameter == nullptr ){
        return QString();
    }
    return parameter->rawValueString();
}

Fact *ParameterSetter::getFact(int compId, QString paramName, bool report)
{
    auto vehicle=MultiVehicleManager::instance()->activeVehicle();
    if(vehicle == nullptr){
        return nullptr;
    }
    auto parameterManager = vehicle->parameterManager();
    Fact *fact = nullptr;
    if( report ){
        fact = parameterManager->getParameter(compId,paramName);
    } else {
        bool parameterExist = parameterManager->parameterExists(compId,paramName);
        if( parameterExist ){
            fact = parameterManager->getParameter(compId,paramName);
        }
    }
    return fact;
}

bool ParameterSetter::setParameter(int compId, QString paramName, float value)
{
    auto vehicle=MultiVehicleManager::instance()->activeVehicle();
    if(vehicle == nullptr){
        return false;
    }
    auto parameterManager = vehicle->parameterManager();
    auto parameter = parameterManager->getParameter(compId,paramName);
    if(parameter == nullptr){
        return false;
    }
   parameter->setRawValue(value);
   return true;
}
