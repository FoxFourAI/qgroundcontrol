#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtCore/QPointF>
#include "Vehicle.h"

class HeadingAlignmentSetter: public QObject{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate mapCoordinate READ mapCoordinate WRITE setMapCoordinate NOTIFY mapCoordinateChanged)
    Q_PROPERTY(QPointF cameraCoordinate READ cameraCoordinate WRITE setCameraCoordinate NOTIFY cameraCoordinateChanged)
    Q_PROPERTY(bool isActive READ isActive NOTIFY activityChanged)
    Q_PROPERTY(bool canApply READ canApply NOTIFY canApplyChanged)
public:
    HeadingAlignmentSetter(Vehicle *vehicle,QObject* parent=nullptr);

    void setMapCoordinate(QGeoCoordinate coordinate);
    void setCameraCoordinate(QPointF coordinate);
    QGeoCoordinate mapCoordinate();
    QPointF cameraCoordinate();
    bool isActive();
    bool canApply();

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

    Q_INVOKABLE void apply();
signals:
    void mapCoordinateChanged();
    void cameraCoordinateChanged();
    void activityChanged();
    void canApplyChanged();
private:
    void setActive(bool active);
private:
    bool _appliable = false;
    bool _active = false;
    QGeoCoordinate _mapCoords = QGeoCoordinate(-1,-1);
    QPointF _cameraCoords = QPointF(-1,-1);
    Vehicle *_vehicle = nullptr;
};
