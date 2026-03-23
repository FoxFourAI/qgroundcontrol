#include "EKSources.h"

#include "ParameterManager.h"

QGC_LOGGING_CATEGORY(EKSourcesLog, "FoxFour.EKSources")

EKSources::EKSources(Vehicle* vehicle, QObject* parent) : QObject(parent), _vehicle(vehicle) {
    connect(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged, this, &EKSources::_fetchSources);
    connect(_vehicle->parameterManager(), &ParameterManager::factAdded, this, [this](int srcId, Fact* fact) {
        if (srcId != _vehicle->defaultComponentId() || fact->name() != "SCR_EKF_SRC") {
            return;
        }
        _setVisible(true);
    });
}

QStringList EKSources::sources() const { return _sources; }

int EKSources::currentSource() { return _currentSource; }

bool EKSources::visible() { return _visible; }

void EKSources::setSource(int index) {
    // sending command to set new index for ekf source
    if (index == _currentSource) {
        return;
    }
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_SET_EKF_SOURCE_SET, false, index);
}

void EKSources::_fetchSources(bool ready) {
    if (!ready) {
        return;
    }
    qCDebug(EKSourcesLog) << "fetching sources";
    auto parameterManager = _vehicle->parameterManager();
    QMap<int, QString> names = {{-1, "----"},        {0, "NONE"}, {3, "GPS"},          {4, "Beacon"},
                                {5, "Optical Flow"}, {6, "VIO"},  {7, "Wheel Encoder"}};
    QString sourceTemplate = "EK3_SRC%1_VELXY";

    // Fetching all sources
    for (int i = 1; i <= 3; ++i) {
        qCDebug(EKSourcesLog) << "getting: " << sourceTemplate.arg(i);
        if (!parameterManager->parameterExists(_vehicle->defaultComponentId(), sourceTemplate.arg(i))) {
            _sources.append("-----");
            qCDebug(EKSourcesLog) << sourceTemplate.arg(i) << " does not exist";
            continue;
        }
        qCDebug(EKSourcesLog) << "getting parameter...";
        int source =
                parameterManager->getParameter(_vehicle->defaultComponentId(), sourceTemplate.arg(i))->rawValue().toInt();
        if (!names.contains(source)) {
            qCDebug(EKSourcesLog) << "parameter set to unknow state!";
            _sources.append("UNKNOWN");
            continue;
        }
        qCDebug(EKSourcesLog) << "parameter set to: " << names[source];
        _sources.append(names[source]);
    }
    emit sourcesChanged();

    // getting current source
    QString currentSourceParamName = "SCR_EKF_SRC";
    if (!parameterManager->parameterExists(_vehicle->defaultComponentId(), currentSourceParamName)) {
        return;
    }
    qCDebug(EKSourcesLog) << "SCR_EKF_SRC exist";
    auto fact = parameterManager->getParameter(_vehicle->defaultComponentId(), currentSourceParamName);
    connect(fact, &Fact::rawValueChanged, this, [=](const QVariant& value) { _setCurrentSource(value.toInt()); });
    _setCurrentSource(fact->rawValue().toInt());
}

void EKSources::_setVisible(bool visible) {
    if (_visible == visible) {
        return;
    }
    _visible = visible;
    qCDebug(EKSourcesLog) << "Visible changed";
    emit visibleChanged();
}

void EKSources::_setCurrentSource(int indx) {
    if (_currentSource == indx) {
        return;
    }
    _currentSource = indx;
    emit currentSourceChanged();
}

void EKSources::_changeSrcHandler(void* responceData, int, const mavlink_command_ack_t& ack,
                                  Vehicle::MavCmdResultFailureCode_t failureCode) {
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
