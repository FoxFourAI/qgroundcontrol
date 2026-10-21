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
    DEFINE_SETTINGFACT(showDetections)
    DEFINE_SETTINGFACT(enableVGMDialect)
    DEFINE_SETTINGFACT(videoToolBarOverlap)
    DEFINE_SETTINGFACT(directVGM)
    DEFINE_SETTINGFACT(disableVehicleTracking)
    //OSD part
    DEFINE_SETTINGFACT(hudVisible)
    DEFINE_SETTINGFACT(hudOpacity)
    DEFINE_SETTINGFACT(hudColor)
    DEFINE_SETTINGFACT(hudShadow)
    DEFINE_SETTINGFACT(hudCompass)
    DEFINE_SETTINGFACT(hudRoll)
    DEFINE_SETTINGFACT(hudSpeed)
    DEFINE_SETTINGFACT(hudAltitude)
    DEFINE_SETTINGFACT(hudHorizon)
};

