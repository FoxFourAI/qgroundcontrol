#pragma once

#include <QtPositioning/QGeoCoordinate>
#include <QtCore/QObject>

class Vehicle;

class VioTrajectoryPoints: public QObject {
    Q_OBJECT
public:
    VioTrajectoryPoints(Vehicle *vehicle, QObject *parent = nullptr);

    Q_INVOKABLE QVariantList list() const { return _points; }

    void start  ();
    void stop   ();

public slots:
    void clear  ();

signals:
    void pointAdded     (QGeoCoordinate coordinate);
    void updateLastPoint(QGeoCoordinate coordinate);
    void pointsCleared  ();
private:
    void _processMavMSG(const mavlink_message_t &msg);
    Vehicle*     _vehicle = nullptr;
    QVariantList _points;
    QGeoCoordinate _lastPoint = QGeoCoordinate();

    static constexpr double _distanceTolerance = 2.0;
};
