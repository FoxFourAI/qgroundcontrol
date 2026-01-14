#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

class Vehicle;

class VioGpsComparer: public QObject
{
    Q_OBJECT
    Q_PROPERTY(double avrError READ avrError NOTIFY avrErrorChanged)
    Q_PROPERTY(double minError READ minError NOTIFY minErrorChanged)
    Q_PROPERTY(double maxError READ maxError NOTIFY maxErrorChanged)
public:
    VioGpsComparer(Vehicle* vehicle,QObject* parent = nullptr);
	~VioGpsComparer();

    double avrError(){return _avrError;}
    double minError(){return _minError;}
    double maxError(){return _maxError;}
signals:
    void avrErrorChanged();
    void minErrorChanged();
    void maxErrorChanged();
private slots:
    void _handleTrajectory(QGeoCoordinate coordinate,uint8_t src);
private:
    void calculateError(QGeoCoordinate& coordinate);
    QGeoCoordinate  _vioCoordinate;
    const int _accumulateCount = 10;
    double  _minError =INT_MAX,
            _maxError = 0.,
            _avrError = 0.;
    QList<double> _window;
};
