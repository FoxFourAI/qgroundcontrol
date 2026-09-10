#pragma once

#include <QGeoCoordinate>

#include "VehicleComponent.h"

class VGMDialect : public VehicleComponent
{
    Q_OBJECT
    Q_PROPERTY(QVariantList detections READ detections NOTIFY detectionsListChanged FINAL)
public:
    VGMDialect(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

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

    QVariantList detections() const;

    void clearDetections();
signals:
    void companionVersionReceived(QVariantMap);
    void detectionsListChanged();

private slots:
    void _handleMavMessage(const mavlink_message_t& msg);
    void _handleCompanionVersion(const mavlink_companion_version_t& msg);
    void _handleF4Detector(const mavlink_f4_detector_t& msg);

private:
    const QString _name = "VGM Information", _description = "Infromation from VGM received by dialect.",
                  _icon = QStringLiteral("custom/img/vgm.svg");
    QVariantList _detections;
    bool _setupCompleted = false;
};
