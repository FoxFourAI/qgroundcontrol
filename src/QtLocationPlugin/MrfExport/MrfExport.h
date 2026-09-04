#pragma once
#include <QAtomicInt>
#include <QByteArray>
#include <QString>
#include <cpl_vsi.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <QRect>

class MrfGridWriter
{
public:
    MrfGridWriter() = default;
    static constexpr int kTileSize = 256;

    // basePath without extension -> basePath.mrf / .idx / .pjg
    bool begin(const QString& basePath, int zoom, const QRect &area);

    // rgb must be 256*256*3 interleaved, as produced by SQLTileToRGB.
    bool writeTile(int x, int y, const QByteArray& rgb);

    bool finish();

    ~MrfGridWriter();

    // --- XYZ helpers (Google/OSM scheme, y = 0 at north)
    static int long2tileX(double lon, int z);
    static int lat2tileY(double lat, int z);

    // Decode a cached tile blob to 256x256 interleaved RGB.
    static bool SQLTileToRGB(const QByteArray& blob, QByteArray& rgbOut, int sx = 0, int sy = 0, int sw = 0,
                             int sh = 0);

    // QGCHash check
    static bool SQLParseTileHash(const QString& hash, int& x, int& y, int& z);

private:
    static constexpr double kHalf = 20037508.342789244;
    GDALDataset* _ds = nullptr;
    int _x0 = 0, _y0 = 0, _nx = 0, _ny = 0;
};
