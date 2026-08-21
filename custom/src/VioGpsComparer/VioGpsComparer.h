#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QTimer>
#include "AutoPilotWidget/AutoPilotWidget.h"

class VioGpsComparer: public AutoPilotWidget
{
    Q_OBJECT
    Q_PROPERTY(double ATEError READ ATEError NOTIFY ATEErrorChanged)
    Q_PROPERTY(double RMSEError READ RMSEError NOTIFY RMSEErrorChanged)
    Q_PROPERTY(Fact* ekfSrcFact MEMBER _ekfSrcFact NOTIFY ekfSrcFactChanged)
    Q_PROPERTY(Fact* user2Fact  MEMBER _user2Fact  NOTIFY user2FactChanged)
    Q_PROPERTY(double currentError READ currentError NOTIFY currentErrorChanged)
public:
    VioGpsComparer(Vehicle* vehicle,QObject* parent = nullptr);
	~VioGpsComparer();

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
    void ekfSrcFactChanged();
    void user2FactChanged();
    void currentErrorChanged();
private slots:
    void _handleTrajectory(QGeoCoordinate coordinate,uint8_t src);
private:
    void updateATE();
    void calculateRMSE(const QGeoCoordinate& coordinate);
    void clearRMSE();
    void clearCurrentError();
private:
    Fact *_ekfSrcFact = nullptr,
         *_user2Fact = nullptr;
    double _RMSEAvr = 0;
    double _ATESumm = 0;
    double _currentError = 0;
    int _RMSECount = 0;
    int _ATECount = 0;
    int _refreshInterval = 2000;
    QGeoCoordinate  _vioCoordinate;
    QGeoCoordinate  _gpsCoordinate;
    QTimer _refreshTimer;
};
