#include "MandatoryParameters.h"

#include <QtCore/QSettings>

#include "ParameterManager.h"
#include "Vehicle.h"

MandatoryParameters::MandatoryParameters(QObject* parent) : QObject(parent) { _loadParameters(); }

MandatoryParameters::~MandatoryParameters() { _saveParameters(); }

const QStringList& MandatoryParameters::parameters() { return _parameters; }

void MandatoryParameters::removeParameter(const QString& parameter) {
    if (!_parameters.contains(parameter)) {
        return;
    }
    _parameters.removeAt(_parameters.indexOf(parameter));
    emit parametersChanged();
}

void MandatoryParameters::addParameter(const QString& parameter) {
    if (_parameters.contains(parameter)) {
        return;
    }
    _parameters.append(parameter);
    emit parametersChanged();
}

void MandatoryParameters::_loadParameters() {
    QSettings settings;
    settings.beginGroup(_groupKey);
    _parameters = settings.value(_paramListName).toStringList();
    settings.endGroup();
}

void MandatoryParameters::_saveParameters() {
    QSettings settings;
    settings.beginGroup(_groupKey);
    settings.setValue(_paramListName, _parameters);
    settings.endGroup();
}

void MandatoryParameters::loadDefaultParameters() {
    // TODO: add default parameters that are essential for show all UI elements in QGC.
    _parameters = {
        "FRAME_CLASS",     "COMPASS_AUTODEC", "COMPASS_AUTO_ROT", "COMPASS_CAL_FIT", "COMPASS_DEC",    "COMPASS_DEV_ID",
        "COMPASS_DEV_ID2", "COMPASS_DEV_ID3", "COMPASS_USE",      "COMPASS_USE2",    "COMPASS_USE3",   "COMPASS_OFS_X",
        "COMPASS_OFS_Y",   "COMPASS_OFS_Z",   "COMPASS_OFS2_X",   "COMPASS_OFS2_Y",  "COMPASS_OFS2_Z", "COMPASS_OFS3_X",
        "COMPASS_OFS3_Y",  "COMPASS_OFS3_Z",  "INS_ACCOFFS_X",    "INS_ACCOFFS_Y",   "INS_ACCOFFS_Z",  "SCR_USER1",
        "SCR_USER2",       "SCR_USER3",       "SCR_USER4",        "SCR_USER5",       "SCR_USER6",      "EK3_SRC1_POSXY",
        "EK3_SRC1_POSZ",   "EK3_SRC1_VELXY",  "EK3_SRC1_VELZ",    "EK3_SRC1_YAW",    "EK3_SRC2_POSXY", "EK3_SRC2_POSZ",
        "EK3_SRC2_VELXY",  "EK3_SRC2_VELZ",   "EK3_SRC2_YAW",     "EK3_SRC3_POSXY",  "EK3_SRC3_POSZ",  "EK3_SRC3_VELXY",
        "EK3_SRC3_VELZ",   "EK3_SRC3_YAW",    "EK3_SRC_OPTIONS",  "EK3_OPTIONS"};
}

void MandatoryParameters::setParametersReady(bool ready) {
    if (_parametersReady == ready) {
        return;
    }
    _parametersReady = ready;
    emit parametersReadyChanged(ready);
}
