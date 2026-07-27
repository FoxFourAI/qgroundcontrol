#include "CopterConfigurator.h"

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
