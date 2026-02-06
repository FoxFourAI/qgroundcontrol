#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QTimer>
class Vehicle;

class VioGpsComparer: public QObject
{
    Q_OBJECT
    Q_PROPERTY(double ATEError READ ATEError NOTIFY ATEErrorChanged)
    Q_PROPERTY(double RMSEError READ RMSEError NOTIFY RMSEErrorChanged)
    Q_PROPERTY(int vioStatus READ vioStatus NOTIFY vioStatusChanged)
    Q_PROPERTY(double currentError READ currentError NOTIFY currentErrorChanged)
public:
    VioGpsComparer(Vehicle* vehicle,QObject* parent = nullptr);
	~VioGpsComparer();
    int vioStatus(){
        return _vioStatus;
    }
    double RMSEError(){return sqrt(_RMSEAvr);}
    double ATEError(){
        if(_ATECount == 0){
            return 0;
        } else {
            return _ATESumm/_ATECount;
        }
    }
    double currentError(){
        return _currentError;
    }
signals:
    void ATEErrorChanged();
    void RMSEErrorChanged();
    void vioStatusChanged();
    void currentErrorChanged();
private slots:
    void _handleTrajectory(QGeoCoordinate coordinate,uint8_t src);
    void _handleTimeout();
private:
    void updateATE();
    void calculateRMSE(const QGeoCoordinate& coordinate);
    void clearRMSE();
    void clearCurrentError();
private:
    double _RMSEAvr = 0;
    double _ATESumm = 0;
    double _currentError = 0;
    int _RMSECount = 0;
    int _ATECount = 0;
    int _refreshInterval = 2000;
    int _vioStatus = -1;
    QGeoCoordinate  _vioCoordinate;
    QGeoCoordinate  _gpsCoordinate;
    QTimer _refreshTimer;
    Vehicle *_vehicle = nullptr;
};
