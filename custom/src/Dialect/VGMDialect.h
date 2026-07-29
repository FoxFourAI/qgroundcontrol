#pragma once

#include "VehicleComponent.h"

class VGMDialect : public VehicleComponent
{
    Q_OBJECT

public:
    VGMDialect(Vehicle *vehicle, AutoPilotPlugin *autopilot,QObject *parent=nullptr);


    // VehicleComponent interface
public:
    QString name() const override;
    QString description() const override;
    QString iconResource() const override;
    bool requiresSetup() const override;
    bool setupComplete() const override;
    QUrl setupSource() const override;
    QUrl summaryQmlSource() const override;
    QStringList setupCompleteChangedTriggerList() const override;
    void setupTriggerSignals() override;

private:
    const QString _name = "VGM Information",
                  _description = "Infromation from VGM received by dialect.",
                  _icon = QStringLiteral("custom/img/vgm.svg");
    bool _setupCompleted = false;



};
