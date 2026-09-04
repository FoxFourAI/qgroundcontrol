#include "MrfExport.h"

#include <cmath>
#include <cstring>

bool MrfGridWriter::SQLTileToRGB(const QByteArray& blob, QByteArray& rgbOut, int sx, int sy, int sw, int sh)
{
    static constexpr int kTile = 256;
    static QAtomicInt counter(0);

    const QByteArray vpath = QStringLiteral("/vsimem/qgc_mrf_in_%1").arg(counter.fetchAndAddRelaxed(1)).toUtf8();

    VSILFILE* mf =
        VSIFileFromMemBuffer(vpath.constData(), reinterpret_cast<GByte*>(const_cast<char*>(blob.constData())),
                             static_cast<vsi_l_offset>(blob.size()), FALSE);  // FALSE: GDAL does not own it
    if (mf == nullptr) {
        return false;
    }
    VSIFCloseL(mf);

    GDALDataset* src = static_cast<GDALDataset*>(GDALOpen(vpath.constData(), GA_ReadOnly));
    if (src == nullptr) {
        VSIUnlink(vpath.constData());
        return false;
    }

    if (sw <= 0 || sh <= 0) {
        sx = 0;
        sy = 0;
        sw = src->GetRasterXSize();
        sh = src->GetRasterYSize();
    }

    rgbOut.resize(kTile * kTile * 3);
    GByte* dst = reinterpret_cast<GByte*>(rgbOut.data());
    memset(dst, 0, static_cast<size_t>(rgbOut.size()));

    bool ok = true;
    GDALColorTable* ctab = src->GetRasterBand(1)->GetColorTable();
    if (src->GetRasterCount() == 1 && ctab) {
        QByteArray idx(kTile * kTile, 0);
        ok = src->GetRasterBand(1)->RasterIO(GF_Read, sx, sy, sw, sh, idx.data(), kTile, kTile, GDT_Byte, 0, 0,
                                             nullptr) == CE_None;
        if (ok) {
            for (int i = 0; i < kTile * kTile; ++i) {
                const GDALColorEntry* e = ctab->GetColorEntry(static_cast<unsigned char>(idx.at(i)));
                if (!e) {
                    continue;
                }
                dst[3 * i] = static_cast<GByte>(e->c1);
                dst[(3 * i) + 1] = static_cast<GByte>(e->c2);
                dst[(3 * i) + 2] = static_cast<GByte>(e->c3);
            }
        }
    } else {
        const int nb = src->GetRasterCount();
        int bandMap[3];
        if (nb >= 3) {
            bandMap[0] = 1;
            bandMap[1] = 2;
            bandMap[2] = 3;
        } else {
            bandMap[0] = bandMap[1] = bandMap[2] = 1;
        }
        ok = src->RasterIO(GF_Read, sx, sy, sw, sh, dst, kTile, kTile, GDT_Byte, 3, bandMap, 3, 3 * kTile, 1,
                           nullptr) == CE_None;
    }

    GDALClose(src);
    VSIUnlink(vpath.constData());
    return ok;
}

bool MrfGridWriter::SQLParseTileHash(const QString& hash, int& x, int& y, int& z)
{
    const int xSize = 8;
    const int ySize = 8;
    const int zSize = 3;

    const int hashSuffixLength = xSize + ySize + zSize;

    if (hash.length() < hashSuffixLength) {
        return false;
    }
    const int base = hash.length() - hashSuffixLength;

    bool okX = false, okY = false, okZ = false;
    x = hash.mid(base, xSize).toInt(&okX);
    y = hash.mid(base + xSize, ySize).toInt(&okY);
    z = hash.mid(base + xSize + ySize, zSize).toInt(&okZ);

    if (!okX || !okY || !okZ) {
        return false;
    }
    if (z < 0 || z > 30 || x < 0 || y < 0) {
        return false;
    }
    // A tile index must fit inside the 2^z grid at its own zoom.
    const qint64 span = 1LL << z;
    return x < span && y < span;
}

bool MrfGridWriter::begin(const QString& basePath, int zoom, const QRect &area)
{
    _x0 = area.topLeft().x();
    _y0 = area.topLeft().y();
    _nx = area.width();
    _ny = area.height();

    GDALDriver* drv = GetGDALDriverManager()->GetDriverByName("MRF");
    if (drv == nullptr) {
        return false;
    }

    char** co = nullptr;
    co = CSLSetNameValue(co, "COMPRESS", "JPEG");  // -> .pjg
    co = CSLSetNameValue(co, "BLOCKSIZE", "256");  // one cache tile == one MRF page
    co = CSLSetNameValue(co, "QUALITY", "90");
    co = CSLSetNameValue(co, "INTERLEAVE", "PIXEL");
    co = CSLSetNameValue(co, "PHOTOMETRIC", "YCC");

    const QByteArray path = (basePath + QStringLiteral(".mrf")).toUtf8();
    _ds = drv->Create(path.constData(), _nx * kTileSize, _ny * kTileSize, 3, GDT_Byte, co);
    CSLDestroy(co);
    if (_ds == nullptr) {
        return false;
    }

    OGRSpatialReference srs;
    srs.importFromEPSG(3857);
    srs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    _ds->SetSpatialRef(&srs);

    // North-up, square pixels: the only shape get_tile()'s gt[0],[1],[3],[5] math handles.
    const double res = 2.0 * kHalf / (kTileSize * std::pow(2.0, zoom));
    double gt[6] = {-kHalf + _x0 * res * kTileSize, res, 0.0, kHalf - _y0 * res * kTileSize, 0.0, -res};
    return _ds->SetGeoTransform(gt) == CE_None;
}

bool MrfGridWriter::writeTile(int x, int y, const QByteArray& rgb)
{
    const int col = x - _x0, row = y - _y0;
    if (!_ds || col < 0 || row < 0 || col >= _nx || row >= _ny || rgb.size() != kTileSize * kTileSize * 3) {
        return false;
    }

    return _ds->RasterIO(GF_Write, col * kTileSize, row * kTileSize, kTileSize, kTileSize,
                         const_cast<char*>(rgb.constData()), kTileSize, kTileSize, GDT_Byte, 3, nullptr, 3,
                         3 * kTileSize, 1, nullptr) == CE_None;
}

bool MrfGridWriter::finish()
{
    if (_ds == nullptr) {
        return false;
    }
    GDALClose(_ds);
    _ds = nullptr;
    return true;
}

MrfGridWriter::~MrfGridWriter()
{
    if (_ds != nullptr)
        GDALClose(_ds);
}

int MrfGridWriter::long2tileX(double lon, int z)
{
    return static_cast<int>(std::floor((lon + 180.0) / 360.0 * std::pow(2.0, z)));
}

int MrfGridWriter::lat2tileY(double lat, int z)
{
    const double r = lat * M_PI / 180.0;
    return static_cast<int>(
        std::floor((1.0 - std::log(std::tan(r) + 1.0 / std::cos(r)) / M_PI) / 2.0 * std::pow(2.0, z)));
}
