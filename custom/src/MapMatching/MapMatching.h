#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
class Vehicle;

class MapMatching : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList anchors READ anchors NOTIFY listChanged);
public:
    MapMatching(Vehicle* vehicle, QObject* parent = nullptr);
    QVariantList anchors(){return _anchors;}
signals:
    void listChanged();
private slots:
    void _handleMavCmd(const mavlink_message_t& msg);
    void _clear();
private:
    Vehicle *_vehicle = nullptr;
    QVariantList _anchors;
};
