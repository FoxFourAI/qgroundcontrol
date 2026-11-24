#pragma once

#include "QObject"
#include "Vehicle.h"
#include <QVariantList>
#include "f4_autonomy/f4_autonomy.h"

class FoxFourCameraCalibration: public QObject {
    Q_OBJECT

    Q_PROPERTY(int          width      MEMBER _width      NOTIFY calibrationChanged)
    Q_PROPERTY(int          height     MEMBER _height     NOTIFY calibrationChanged)
    Q_PROPERTY(QString      model      MEMBER _model      NOTIFY calibrationChanged)
    Q_PROPERTY(QVariantList intrinsics MEMBER _intrinsics NOTIFY calibrationChanged)   // [fx, cx, fy, cy]
    Q_PROPERTY(QVariantList distortion MEMBER _distortion NOTIFY calibrationChanged)   // D[6]
public:

    FoxFourCameraCalibration(Vehicle* vehicle, QObject *parent): QObject(parent) {
        connect(vehicle,&Vehicle::mavlinkMessageReceived, this, &FoxFourCameraCalibration::handleMavlinkMessage);
    }

    void handleMavlinkMessage(const mavlink_message_t &raw) {
        if (raw.msgid != MAVLINK_MSG_ID_F4_CAM_CALIBR) {
            return;
        }
        mavlink_f4_cam_calibr_t msg;
        mavlink_msg_f4_cam_calibr_decode(&raw, &msg);
        const bool valid = msg.width > 0 && msg.height > 0 && msg.K[0] > 0 && msg.K[2] > 0;
        if (!valid) {
            return;
        }
        _model = QString::fromLatin1(QByteArray::fromRawData(reinterpret_cast<char*>(&msg.distortion_model),sizeof(msg.distortion_model))).trimmed();
        qDebug() << _model <<" :Received from the vehicle";
        _intrinsics.clear();
        _distortion.clear();

        for (double val: msg.K) {
            _intrinsics.append(val);
        }

        for (double val: msg.D) {
            _distortion .append(val);
        }

    }
signals:
    void calibrationChanged();
private:
    int _width = 0;
    int _height = 0;
    QString _model = "";
    QVariantList _intrinsics;
    QVariantList _distortion;
};
