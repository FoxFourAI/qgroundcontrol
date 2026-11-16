#include "FoxFourSeiParser.h"

#include <cstring>
#include <cctype>
#include <limits>
#include <locale>
#include <sstream>
#include <vector>

namespace F4Sei {
namespace {

// _UUID
constexpr uint8_t kUuid[16] = {
    0xf4, 0xf0, 0x4a, 0x1b, 0xc8, 0xe5, 0x11, 0xed, 0xaa, 0x4b, 0x02, 0x42, 0xac, 0x12, 0x00, 0x02,
};

// _dep3(): strip emulation prevention bytes (00 00 03 -> 00 00)
std::vector<uint8_t> dep3(const uint8_t *data, size_t len)
{
    std::vector<uint8_t> out;
    out.reserve(len);
    size_t i = 0;
    while (i < len) {
        if (i + 2 < len && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 3) {
            out.push_back(0);
            out.push_back(0);
            i += 3;
        } else {
            out.push_back(data[i]);
            i += 1;
        }
    }
    return out;
}

// _decode_sei_size()
uint32_t decodeSeiSize(const std::vector<uint8_t> &rbsp, size_t &k)
{
    uint32_t size = 0;
    while (k < rbsp.size() && rbsp[k] == 0xFF) {
        size += 255;
        k += 1;
    }
    if (k < rbsp.size()) {
        size += rbsp[k];
        k += 1;
    }
    return size;
}

bool isStartCode4(const uint8_t *d, size_t n, size_t i)
{
    return i + 3 < n && d[i] == 0 && d[i + 1] == 0 && d[i + 2] == 0 && d[i + 3] == 1;
}

bool isStartCode3(const uint8_t *d, size_t n, size_t i)
{
    return i + 2 < n && d[i] == 0 && d[i + 1] == 0 && d[i + 2] == 1;
}

// Body of parse_frame_sei(), on Annex-B data
std::optional<std::string> parseAnnexB(const uint8_t *data, size_t n)
{
    size_t i = 0;
    while (n >= 2 && i < n - 2) {
        // Locate next start code
        if (isStartCode4(data, n, i)) {
            i += 4;
        } else if (isStartCode3(data, n, i)) {
            i += 3;
        } else {
            i += 1;
            continue;
        }
        if (i >= n) {
            break;
        }

        const uint8_t first = data[i];
        const uint8_t h264Type = first & 0x1f;
        const uint8_t h265Type = (first >> 1) & 0x3f;

        const bool isSei = (h264Type == 6) || (h265Type == 39);
        i += (h265Type == 39 && h264Type != 6) ? 2 : 1;

        if (!isSei) {
            continue;
        }

                // Find end of this NAL unit (next start code or buffer end)
        size_t j = i;
        while (j < n) {
            if (isStartCode4(data, n, j)) break;
            if (isStartCode3(data, n, j)) break;
            j += 1;
        }
        if (i > j) {   // H.265 header ran past the end of the buffer
            continue;
        }

        const std::vector<uint8_t> rbsp = dep3(data + i, j - i);
        size_t k = 0;
        while (k < rbsp.size()) {
            if (rbsp[k] == 0x80) {   // RBSP trailing bits
                break;
            }
            const uint8_t seiType = rbsp[k];
            k += 1;
            if (k >= rbsp.size()) {
                break;
            }
            const uint32_t seiSize = decodeSeiSize(rbsp, k);
            if (k + seiSize > rbsp.size()) {
                break;
            }
            if (seiType == 5 && seiSize > 16 && std::memcmp(&rbsp[k], kUuid, 16) == 0) {
                return std::string(reinterpret_cast<const char *>(&rbsp[k + 16]), seiSize - 16);
            }
            k += seiSize;
        }
    }
    return std::nullopt;
}

bool startsWithStartCode(const uint8_t *d, size_t n)
{
    return isStartCode3(d, n, 0) || isStartCode4(d, n, 0);
}

// avc / hvc1 (4-byte big-endian NAL lengths) -> Annex-B. Returns false if the
// lengths don't add up exactly, i.e. the buffer isn't length-prefixed.
bool lengthPrefixedToAnnexB(const uint8_t *d, size_t n, std::vector<uint8_t> &out)
{
    out.clear();
    out.reserve(n + 16);
    size_t pos = 0;
    while (pos < n) {
        if (n - pos < 4) return false;
        const size_t len = (size_t(d[pos]) << 24) | (size_t(d[pos + 1]) << 16) |
                           (size_t(d[pos + 2]) << 8) | size_t(d[pos + 3]);
        pos += 4;
        if (len == 0 || len > n - pos) return false;
        out.insert(out.end(), {0x00, 0x00, 0x00, 0x01});
        out.insert(out.end(), d + pos, d + pos + len);
        pos += len;
    }
    return true;
}

// Python str.strip() for the whitespace that int()/float() tolerate
std::string strip(const std::string &s)
{
    const char *ws = " \t\n\r\f\v";
    const size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(ws) - b + 1);
}

// Python float(): locale-independent, whole string must be consumed
bool pyFloat(const std::string &field, double &out)
{
    const std::string s = strip(field);
    if (s.empty()) return false;

    std::string lower;
    for (char c : s) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const bool neg = !lower.empty() && lower[0] == '-';
    const std::string body = (lower[0] == '-' || lower[0] == '+') ? lower.substr(1) : lower;
    if (body == "nan") { out = std::numeric_limits<double>::quiet_NaN(); return true; }
    if (body == "inf" || body == "infinity") {
        out = neg ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
        return true;
    }

    std::istringstream in(s);
    in.imbue(std::locale::classic());   // '.' decimal point even under cs_CZ etc.
    in >> out;
    return !in.fail() && in.peek() == std::char_traits<char>::eof();
}

// Python int() for the pts field
bool pyInt(const std::string &field, uint64_t &out)
{
    const std::string s = strip(field);
    if (s.empty()) return false;
    size_t p = (s[0] == '+') ? 1 : 0;
    if (p >= s.size()) return false;
    uint64_t v = 0;
    for (; p < s.size(); ++p) {
        if (s[p] < '0' || s[p] > '9') return false;
        v = v * 10 + uint64_t(s[p] - '0');
    }
    out = v;
    return true;
}

} // namespace

std::optional<std::string> parseFrame(const uint8_t *data, size_t size)
{
    if (!data || size == 0) {
        return std::nullopt;
    }
    if (startsWithStartCode(data, size)) {
        return parseAnnexB(data, size);
    }
    std::vector<uint8_t> annexB;
    if (lengthPrefixedToAnnexB(data, size, annexB)) {
        return parseAnnexB(annexB.data(), annexB.size());
    }
    return parseAnnexB(data, size);
}

std::optional<Telemetry> parseTelemetry(const uint8_t *data, size_t size)
{
    const std::optional<std::string> text = parseFrame(data, size);
    if (!text) {
        return std::nullopt;
    }

            // fields = text.rstrip(",").split(",")
    std::string t = *text;
    while (!t.empty() && t.back() == ',') t.pop_back();
    std::vector<std::string> fields;
    size_t start = 0;
    while (true) {
        const size_t comma = t.find(',', start);
        fields.push_back(t.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    if (fields.size() < 9) {
        return std::nullopt;
    }

    Telemetry r;
    double v[8];
    if (!pyInt(fields[0], r.encoderPts)) return std::nullopt;
    for (int idx = 0; idx < 8; ++idx) {
        if (!pyFloat(fields[idx + 1], v[idx])) return std::nullopt;
    }
    r.posX = v[0]; r.posY = v[1]; r.posZ = v[2];
    r.qw = v[3];   r.qx = v[4];   r.qy = v[5]; r.qz = v[6];
    r.nedZ = v[7];

    bool allMinusOne = true;
    for (double d : v) allMinusOne = allMinusOne && (d == -1.0);
    r.valid = !allMinusOne;
    return r;
}

} // namespace F4Sei