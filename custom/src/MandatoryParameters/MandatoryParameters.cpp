#include "MandatoryParameters.h"

#include <QtCore/QSettings>

#include "FoxFourAutoPilotPlugin.h"
#include "MultiVehicleManager.h"
#include "OnboardComputersManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"

MandatoryParameters::MandatoryParameters(QObject* parent) : QObject(parent) { _loadParameters(); }

MandatoryParameters::~MandatoryParameters() { _saveParameters(); }

const QVariantMap MandatoryParameters::parameters() {
    QVariantMap result;
    for (auto it = _parameters.begin(); it != _parameters.end(); ++it) {
        result[componentNames[it.key()]] = it.value();
    }
    return result;
}

const QMap<MandatoryParameters::ComponentType, QStringList>& MandatoryParameters::rawParameters() {
    return _parameters;
}

void MandatoryParameters::removeParameter(const QString& parameter,const QString component) {
    if (!componentNames.contains(component)) {
        return;
    }

    ComponentType index = ComponentType(componentNames.indexOf(component));

    if (!_parameters[index].contains(parameter)) {
        return;
    }
    _parameters[index].removeAt(_parameters[index].indexOf(parameter));
    emit parametersChanged();
}

void MandatoryParameters::addParameter(const QString& parameter, const int componentId) {
    // user can add parameters only when there is an active vehicle

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();

    if (vehicle == nullptr) {
        return;
    }

    OnboardComputersManager* ocm =
            reinterpret_cast<FoxFourAutoPilotPlugin*>(vehicle->autopilotPlugin())->onboardComputersManager();

    if (ocm->currentComputerComponent() != componentId && componentId != MAV_COMP_ID_AUTOPILOT1) {
        // we do not handle any onboard computers info except of VGM and FCU.
        return;
    }

    ComponentType type = componentId == 1 ? ComponentType::FCU : ComponentType::VGM;

    // if we already have the parameter in the list, ignore it
    if (_parameters[type].contains(parameter)) {
        return;
    }
    _parameters[type].append(parameter);
    emit parametersChanged();
}

void MandatoryParameters::_loadParameters() {
    QSettings settings;
    settings.beginGroup(_groupKey);
    for (int compType = ComponentType::FCU; compType != ComponentType::Unknown; ++compType) {
        _parameters[ComponentType(compType)] = settings.value(componentNames[compType] + "List").toStringList();
    }
    settings.endGroup();
}

void MandatoryParameters::_saveParameters() {
    QSettings settings;
    settings.beginGroup(_groupKey);
    for (int compType = ComponentType::FCU; compType != ComponentType::Unknown; ++compType) {
        settings.setValue(componentNames[compType] + "List", _parameters[ComponentType(compType)]);
    }
    settings.endGroup();
}

void MandatoryParameters::loadDefaultParameters() {
    // TODO: add default parameters that are essential for show all UI elements in QGC.
    _parameters[ComponentType::FCU] = {
        "FRAME_CLASS",     "COMPASS_AUTODEC", "COMPASS_AUTO_ROT", "COMPASS_CAL_FIT", "COMPASS_DEC",    "COMPASS_DEV_ID",
        "COMPASS_DEV_ID2", "COMPASS_DEV_ID3", "COMPASS_USE",      "COMPASS_USE2",    "COMPASS_USE3",   "COMPASS_OFS_X",
        "COMPASS_OFS_Y",   "COMPASS_OFS_Z",   "COMPASS_OFS2_X",   "COMPASS_OFS2_Y",  "COMPASS_OFS2_Z", "COMPASS_OFS3_X",
        "COMPASS_OFS3_Y",  "COMPASS_OFS3_Z",  "INS_ACCOFFS_X",    "INS_ACCOFFS_Y",   "INS_ACCOFFS_Z",  "SCR_USER1",
        "SCR_USER2",       "SCR_USER3",       "SCR_USER4",        "SCR_USER5",       "SCR_USER6",      "SCR_EKF_SRC",
        "EK3_SRC1_POSXY",  "EK3_SRC1_POSZ",   "EK3_SRC1_VELXY",   "EK3_SRC1_VELZ",   "EK3_SRC1_YAW",   "EK3_SRC2_POSXY",
        "EK3_SRC2_POSZ",   "EK3_SRC2_VELXY",  "EK3_SRC2_VELZ",    "EK3_SRC2_YAW",    "EK3_SRC3_POSXY", "EK3_SRC3_POSZ",
        "EK3_SRC3_VELXY",  "EK3_SRC3_VELZ",   "EK3_SRC3_YAW",     "EK3_SRC_OPTIONS", "EK3_OPTIONS"};

    _parameters[ComponentType::VGM] = {"CAM_EXPOSURE", "VID_ZOOM_MAX", "VID_ZOOM_MIN"};
    emit parametersChanged();
}

