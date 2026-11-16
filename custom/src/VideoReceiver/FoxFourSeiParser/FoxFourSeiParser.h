#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace F4Sei {


struct Telemetry {
    uint64_t encoderPts = 0;   // pts
    double   posX = 0;         // pos_x
    double   posY = 0;         // pos_y
    double   posZ = 0;         // pos_z
    double   qw = 0;           // orient_w
    double   qx = 0;           // orient_x
    double   qy = 0;           // orient_y
    double   qz = 0;           // orient_z
    double   nedZ = 0;         // local_pose_ned
    bool     valid = false;    // extra: false when the sender wrote -1 placeholders
};

std::optional<std::string> parseFrame(const uint8_t *data, size_t size);

std::optional<Telemetry> parseTelemetry(const uint8_t *data, size_t size);

} // namespace F4Sei