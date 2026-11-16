#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QMetaType>
#include <QtCore/QString>

#include <optional>

namespace F4Sei {


struct Telemetry {
    uint64_t encoderPts = 0;
    double   posX = 0;
    double   posY = 0;
    double   posZ = 0;
    double   qw = 0;
    double   qx = 0;
    double   qy = 0;
    double   qz = 0;
    double   nedZ = 0;
    bool     valid = false;
};

std::optional<QString> parseFrame(QByteArrayView data);

std::optional<Telemetry> parseTelemetry(QByteArrayView data);

} // namespace F4Sei

Q_DECLARE_METATYPE(F4Sei::Telemetry)