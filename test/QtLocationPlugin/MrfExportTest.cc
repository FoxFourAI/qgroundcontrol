#include "MrfExportTest.h"

#include <QtCore/QTemporaryDir>

#include "MrfExport/MrfExport.h"

void MrfExportTest::_testProjectionAndOverviews()
{
    GDALAllRegister();

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString basePath = tempDir.filePath(QStringLiteral("map"));
    const QRect tileArea(10, 12, 4, 4);
    MrfGridWriter writer;
    QVERIFY(writer.begin(basePath, 5, tileArea));

    QByteArray rgb(MrfGridWriter::kTileSize * MrfGridWriter::kTileSize * 3, '\0');
    for (int y = tileArea.top(); y <= tileArea.bottom(); ++y) {
        for (int x = tileArea.left(); x <= tileArea.right(); ++x) {
            QVERIFY(writer.writeTile(x, y, rgb));
        }
    }
    QVERIFY(writer.finish());

    const QByteArray path = (basePath + QStringLiteral(".mrf")).toUtf8();
    GDALDataset* dataset = static_cast<GDALDataset*>(GDALOpen(path.constData(), GA_ReadOnly));
    QVERIFY(dataset != nullptr);

    OGRSpatialReference expected;
    QCOMPARE(expected.importFromEPSG(3857), OGRERR_NONE);
    expected.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    const OGRSpatialReference* actual = dataset->GetSpatialRef();
    QVERIFY(actual != nullptr);
    QVERIFY(actual->IsSame(&expected));

    double geoTransform[6] = {};
    QCOMPARE(dataset->GetGeoTransform(geoTransform), CE_None);
    QVERIFY(geoTransform[1] > 0.0);
    QVERIFY(geoTransform[5] < 0.0);

    GDALRasterBand* band = dataset->GetRasterBand(1);
    QVERIFY(band != nullptr);
    QCOMPARE(band->GetOverviewCount(), 2);
    QCOMPARE(band->GetOverview(0)->GetXSize(), dataset->GetRasterXSize() / 2);
    QCOMPARE(band->GetOverview(1)->GetXSize(), dataset->GetRasterXSize() / 4);

    GDALClose(dataset);
}

UT_REGISTER_TEST(MrfExportTest, TestLabel::Unit)
