#include "CopterState.h"

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