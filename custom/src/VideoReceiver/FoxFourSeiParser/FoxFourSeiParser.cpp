#include "FoxFourSeiParser.h"

#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QtCore/QtEndian>

#include <algorithm>
#include <iterator>

namespace F4Sei {
namespace {

// _UUID
constexpr char kUuidBytes[16] = {
    '\xf4', '\xf0', '\x4a', '\x1b', '\xc8', '\xe5', '\x11', '\xed',
    '\xaa', '\x4b', '\x02', '\x42', '\xac', '\x12', '\x00', '\x02',
};
const QByteArrayView kUuid(kUuidBytes, sizeof(kUuidBytes));

const QByteArrayView kStartCode3("\x00\x00\x01", 3);
const QByteArrayView kStartCode4("\x00\x00\x00\x01", 4);
const QByteArrayView kEmulation("\x00\x00\x03", 3);
const QByteArrayView kTwoZeros("\x00\x00", 2);

inline quint8 byteAt(QByteArrayView data, qsizetype i)
{
    return static_cast<quint8>(data.at(i));
}

// _decode_sei_size()
qsizetype decodeSeiSize(QByteArrayView rbsp, qsizetype &k)
{
    qsizetype size = 0;
    while (k < rbsp.size() && byteAt(rbsp, k) == 0xFF) {
        size += 255;
        ++k;
    }
    if (k < rbsp.size()) {
        size += byteAt(rbsp, k);
        ++k;
    }
    return size;
}

// End of the NAL that starts at 'from': position of the next 3- or 4-byte start code
qsizetype findNalEnd(QByteArrayView data, qsizetype from)
{
    const qsizetype sc = data.indexOf(kStartCode3, from);
    if (sc < 0) {
        return data.size();
    }
    // 00 00 00 01: the NAL ends at the leading zero of the 4-byte start code
    return (sc - 1 >= from && data.at(sc - 1) == '\0') ? sc - 1 : sc;
}

// Body of parse_frame_sei(), on Annex-B data
std::optional<QString> parseAnnexB(QByteArrayView data)
{
    qsizetype i = 0;
    while (true) {
        // Locate next start code (00 00 01, also matches the tail of 00 00 00 01)
        const qsizetype sc = data.indexOf(kStartCode3, i);
        if (sc < 0) {
            break;
        }
        i = sc + 3;
        if (i >= data.size()) {
            break;
        }

        const quint8 first = byteAt(data, i);
        const quint8 h264Type = first & 0x1f;
        const quint8 h265Type = (first >> 1) & 0x3f;

        const bool isSei = (h264Type == 6) || (h265Type == 39);
        i += (h265Type == 39 && h264Type != 6) ? 2 : 1;

        if (!isSei || i > data.size()) {
            continue;
        }

        const qsizetype j = findNalEnd(data, i);
        QByteArray rbsp = data.sliced(i, j - i).toByteArray();
        rbsp.replace(kEmulation, kTwoZeros);   // _dep3()

        qsizetype k = 0;
        while (k < rbsp.size()) {
            if (byteAt(rbsp, k) == 0x80) {   // RBSP trailing bits
                break;
            }
            const quint8 seiType = byteAt(rbsp, k);
            ++k;
            if (k >= rbsp.size()) {
                break;
            }
            const qsizetype seiSize = decodeSeiSize(rbsp, k);
            if (k + seiSize > rbsp.size()) {
                break;
            }
            if (seiType == 5 && seiSize > 16 && QByteArrayView(rbsp).sliced(k, 16) == kUuid) {
                return QString::fromUtf8(rbsp.sliced(k + 16, seiSize - 16));   // decode(errors='replace')
            }
            k += seiSize;
        }
    }
    return std::nullopt;
}

// avc / hvc1 (4-byte big-endian NAL lengths) -> Annex-B.
// Returns nullopt if the lengths don't add up, i.e. the buffer isn't length-prefixed.
std::optional<QByteArray> lengthPrefixedToAnnexB(QByteArrayView data)
{
    QByteArray out;
    out.reserve(data.size() + 16);
    qsizetype pos = 0;
    while (pos < data.size()) {
        if (data.size() - pos < 4) {
            return std::nullopt;
        }
        const qsizetype len = qFromBigEndian<quint32>(data.constData() + pos);
        pos += 4;
        if (len == 0 || len > data.size() - pos) {
            return std::nullopt;
        }
        out.append(kStartCode4);
        out.append(data.sliced(pos, len));
        pos += len;
    }
    return out;
}

} // namespace

std::optional<QString> parseFrame(QByteArrayView data)
{
    if (data.isEmpty()) {
        return std::nullopt;
    }
    if (data.startsWith(kStartCode3) || data.startsWith(kStartCode4)) {
        return parseAnnexB(data);
    }
    if (const auto annexB = lengthPrefixedToAnnexB(data)) {
        return parseAnnexB(*annexB);
    }
    return parseAnnexB(data);
}

std::optional<Telemetry> parseTelemetry(QByteArrayView data)
{
    const std::optional<QString> text = parseFrame(data);
    if (!text) {
        return std::nullopt;
    }

            // fields = text.rstrip(",").split(",")
    QString csv = *text;
    while (csv.endsWith(u',')) {
        csv.chop(1);
    }
    const QStringList fields = csv.split(u',');
    if (fields.size() < 9) {
        return std::nullopt;
    }

            // int() / float(): C locale, surrounding whitespace allowed
    bool ok = false;
    Telemetry r;
    r.encoderPts = fields[0].trimmed().toULongLong(&ok);
    if (!ok) {
        return std::nullopt;
    }

    double v[8];
    for (int idx = 0; idx < 8; ++idx) {
        v[idx] = fields[idx + 1].trimmed().toDouble(&ok);
        if (!ok) {
            return std::nullopt;
        }
    }
    r.posX = v[0]; r.posY = v[1]; r.posZ = v[2];
    r.qw = v[3];   r.qx = v[4];   r.qy = v[5]; r.qz = v[6];
    r.nedZ = v[7];

    r.valid = !std::all_of(std::begin(v), std::end(v), [](double d) { return d == -1.0; });
    return r;
}

} // namespace F4Sei