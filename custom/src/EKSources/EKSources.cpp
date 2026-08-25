#include "EKSources.h"

#include "ParameterManager.h"

QGC_LOGGING_CATEGORY(EKSourcesLog, "FoxFour.EKSources")

EKSources::EKSources(Vehicle* vehicle, QObject* parent) : AutoPilotWidget(vehicle, parent)
{
    _sources.resize(3);
    for (QString& src : _sources) {
        src = "---";
    }

    _sourcesFact.resize(3);
    _setRequiredParameters({{"SCR_EKF_SRC", _ekfSrcFact},
                            {"EK3_SRC1_VELXY", _sourcesFact[0]},
                            {"EK3_SRC2_VELXY", _sourcesFact[1]},
                            {"EK3_SRC3_VELXY", _sourcesFact[2]}});

    connect(this, &AutoPilotWidget::_requiredFactChanged, this, &EKSources::_handleFact);
    connect(_pm, &ParameterManager::_paramRequestReadFailure, this,
            [this]([[maybe_unused]] const int componentId, const QString& paramName,
                   [[maybe_unused]] const int parameterIndex) {
                if (_ekfSrcFact != nullptr && paramName == _ekfSrcFact->name()) {
                    _refreshFailedCount++;
                    _setDirty(_refreshFailedCount > _maximumFaildeCount);
                }
            });
    connect(_pm, &ParameterManager::_paramRequestReadSuccess, this,
            [this]([[maybe_unused]] const int componentId, const QString& paramName,
                   [[maybe_unused]] const int parameterIndex) {
                if (_ekfSrcFact && paramName == _ekfSrcFact->name()) {
                    _refreshFailedCount = 0;
                    _setDirty(_refreshFailedCount > _maximumFaildeCount);
                }
            });
}

QStringList EKSources::sources() const
{
    return _sources;
}

void EKSources::setSource(int index)
{
    // sending command to set new index for ekf source
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_SET_EKF_SOURCE_SET, false, index);
}

void EKSources::_handleFact(Fact* fact)
{
    if (fact == _ekfSrcFact) {
        emit ekfSrcFactChanged();
        return;
    }

    for (int i = 0; i < _sourcesFact.size(); i++) {
        if (_sourcesFact[i] == fact) {
            connect(fact, &Fact::rawValueChanged, this, [this,i] (const QVariant &value){
                static const QMap<int, QString> names = {{-1, "----"},        {0, "NONE"},         {3, "GPS"},
                                                         {4, "Beacon"},       {5, "Optical Flow"}, {6, "VIO"},
                                                         {7, "Wheel Encoder"}};
                _sources[i] = names.contains(value.toInt()) ? names[value.toInt()] : "UNKNOWN";
                emit sourcesChanged();
            });
        }
    }
}

void EKSources::_changeSrcHandler([[maybe_unused]] void* responceData, int, const mavlink_command_ack_t& ack,
                                  Vehicle::MavCmdResultFailureCode_t failureCode)
{
    if (ack.result != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
            case Vehicle::MavCmdResultCommandResultOnly:
                qCDebug(EKSourcesLog) << QStringLiteral("MAV_CMD_SET_EKF_SOURCE_SET error(%1)").arg(ack.result);
                break;
            case Vehicle::MavCmdResultFailureNoResponseToCommand:
                qCDebug(EKSourcesLog) << "MAV_CMD_SET_EKF_SOURCE_SET failed: no response from vehicle";
                break;
            case Vehicle::MavCmdResultFailureDuplicateCommand:
                qCDebug(EKSourcesLog) << "MAV_CMD_SET_EKF_SOURCE_SET failed: duplicate command";
                break;
        }
    }
}

void EKSources::_setDirty(bool newState)
{
    if (_dirty != newState) {
        _dirty = newState;
        emit dirtyChanged();
    }
}
