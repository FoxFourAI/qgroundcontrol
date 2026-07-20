#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class FoxFourSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    FoxFourSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(minimalMode)
    DEFINE_SETTINGFACT(trackingRate)
    DEFINE_SETTINGFACT(cacheVehicleParameters)
    DEFINE_SETTINGFACT(autoConfigureStream)
    DEFINE_SETTINGFACT(showGPSTrajectory)
    DEFINE_SETTINGFACT(mapMatchingPointsCnt)
    DEFINE_SETTINGFACT(enableVGMDialect)
    DEFINE_SETTINGFACT(videoToolBarOverlap)
};

