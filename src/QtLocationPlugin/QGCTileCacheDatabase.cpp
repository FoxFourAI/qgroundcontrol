#include "QGCTileCacheDatabase.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

//FoxFour part
#include <QBuffer>
#include <QImage>
#include <QImageWriter>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

#include <atomic>

#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCSqlHelper.h"
#include "QGCTile.h"
#include "QGCTileSet.h"

QGC_LOGGING_CATEGORY(QGCTileCacheDatabaseLog, "QtLocationPlugin.QGCTileCacheDatabase")

static std::atomic<quint64> s_connectionCounter{0};

QGCTileCacheDatabase::QGCTileCacheDatabase(const QString &databasePath)
    : _databasePath(databasePath)
    , _connectionName(QStringLiteral("QGCTileCache_%1").arg(s_connectionCounter.fetch_add(1)))
{
}

QGCTileCacheDatabase::~QGCTileCacheDatabase()
{
    disconnectDB();
}

QSqlDatabase QGCTileCacheDatabase::_database() const
{
    return QSqlDatabase::database(_connectionName);
}

QSqlDatabase QGCTileCacheDatabase::database() const
{
    return _database();
}

bool QGCTileCacheDatabase::_ensureConnected() const
{
    if (!_connected || !_valid) {
        qCWarning(QGCTileCacheDatabaseLog) << "Database not connected";
        return false;
    }
    return true;
}

bool QGCTileCacheDatabase::_checkSchemaVersion()
{
    QSqlDatabase db = _database();
    const auto current = QGCSqlHelper::userVersion(db);
    if (!current) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to read schema version";
        return false;
    }

    const int version = *current;
    if (version == kSchemaVersion) {
        return true;
    }

    QSqlQuery query(db);

    if (version == 0) {
        // Either a fresh database or a legacy database created before versioning.
        // Check for existing data — if Tiles table exists with rows, it's legacy.
        // Legacy DBs stored map type as text; migration is not supported so the cache is rebuilt.
        if (query.exec("SELECT COUNT(*) FROM Tiles") && query.next() && query.value(0).toInt() > 0) {
            qCWarning(QGCTileCacheDatabaseLog) << "Legacy database detected (no schema version). Discarding cached tiles and rebuilding.";
            _defaultSet = kInvalidTileSet;
            query.exec("DROP TABLE IF EXISTS TilesDownload");
            query.exec("DROP TABLE IF EXISTS SetTiles");
            query.exec("DROP TABLE IF EXISTS Tiles");
            query.exec("DROP TABLE IF EXISTS TileSets");
        }
        return true;
    }

    // Future: handle incremental migrations here (version < kSchemaVersion).
    qCWarning(QGCTileCacheDatabaseLog) << "Unknown schema version" << version << "(expected" << kSchemaVersion << "). Resetting cache.";
    _defaultSet = kInvalidTileSet;
    query.exec("DROP TABLE IF EXISTS TilesDownload");
    query.exec("DROP TABLE IF EXISTS SetTiles");
    query.exec("DROP TABLE IF EXISTS Tiles");
    query.exec("DROP TABLE IF EXISTS TileSets");
    return true;
}

bool QGCTileCacheDatabase::init()
{
    _failed = false;
    if (!_databasePath.isEmpty()) {
        qCDebug(QGCTileCacheDatabaseLog) << "Mapping cache directory:" << _databasePath;
        if (connectDB()) {
            if (!_checkSchemaVersion()) {
                _failed = true;
                disconnectDB();
                return false;
            }
            _valid = _createDB(_database());
            if (!_valid) {
                _failed = true;
                (void) QFile::remove(_databasePath);
            }
        } else {
            _failed = true;
        }
        disconnectDB();
    } else {
        qCCritical(QGCTileCacheDatabaseLog) << "Could not find suitable cache directory.";
        _failed = true;
    }

    return !_failed;
}

bool QGCTileCacheDatabase::connectDB()
{
    if (_connected) {
        disconnectDB();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", _connectionName);
    db.setDatabaseName(_databasePath);
    _valid = db.open();
    if (_valid) {
        QGCSqlHelper::applySqlitePragmas(db);
        _connected = true;
    } else {
        qCCritical(QGCTileCacheDatabaseLog) << "Map Cache SQL error (open db):" << db.lastError();
        QSqlDatabase::removeDatabase(_connectionName);
    }
    return _valid;
}

void QGCTileCacheDatabase::disconnectDB()
{
    if (!_connected) {
        return;
    }
    _connected = false;

    if (!QCoreApplication::instance()) {
        return;
    }

    {
        QSqlDatabase db = QSqlDatabase::database(_connectionName, false);
        if (db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(_connectionName);
}

bool QGCTileCacheDatabase::saveTile(const QString &hash, const QString &format, const QByteArray &img, const QString &type, quint64 tileSet)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for saveTile";
        return false;
    }

    QSqlQuery query(_database());
    if (!query.prepare("INSERT OR IGNORE INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare saveTile):" << query.lastError().text();
        return false;
    }
    query.addBindValue(hash);
    query.addBindValue(format);
    query.addBindValue(img);
    query.addBindValue(img.size());
    query.addBindValue(UrlFactory::getQtMapIdFromProviderType(type));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (saveTile INSERT):" << query.lastError().text();
        return false;
    }

    if (!query.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare tile lookup):" << query.lastError().text();
        return false;
    }
    query.addBindValue(hash);
    if (!query.exec() || !query.next()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (tile lookup):" << query.lastError().text();
        return false;
    }
    const quint64 tileID = query.value(0).toULongLong();

    const quint64 setID = (tileSet == kInvalidTileSet) ? _getDefaultTileSet() : tileSet;
    if (setID == kInvalidTileSet) {
        qCWarning(QGCTileCacheDatabaseLog) << "Cannot save tile: no valid tile set";
        return false;
    }
    if (!query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare SetTiles):" << query.lastError().text();
        return false;
    }
    query.addBindValue(tileID);
    query.addBindValue(setID);
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
        return false;
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit saveTile transaction";
        return false;
    }

    qCDebug(QGCTileCacheDatabaseLog) << "HASH:" << hash;
    return true;
}

std::unique_ptr<QGCCacheTile> QGCTileCacheDatabase::getTile(const QString &hash)
{
    if (!_ensureConnected()) {
        return nullptr;
    }

    QSqlQuery query(_database());
    if (!query.prepare("SELECT tile, format, type FROM Tiles WHERE hash = ?")) {
        return nullptr;
    }
    query.addBindValue(hash);
    if (query.exec() && query.next()) {
        const QByteArray tileData = query.value(0).toByteArray();
        const QString format = query.value(1).toString();
        const QString type = UrlFactory::getProviderTypeFromQtMapId(query.value(2).toInt());
        qCDebug(QGCTileCacheDatabaseLog) << "(Found in DB) HASH:" << hash;
        return std::make_unique<QGCCacheTile>(hash, tileData, format, type);
    }

    qCDebug(QGCTileCacheDatabaseLog) << "(NOT in DB) HASH:" << hash;
    return nullptr;
}

std::optional<quint64> QGCTileCacheDatabase::findTile(const QString &hash)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (!query.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
        return std::nullopt;
    }
    query.addBindValue(hash);
    if (query.exec() && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
}

QList<TileSetRecord> QGCTileCacheDatabase::getTileSets()
{
    QList<TileSetRecord> records;
    if (!_ensureConnected()) {
        return records;
    }

    QSqlQuery query(_database());
    query.setForwardOnly(true);
    if (!query.exec("SELECT setID, name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                     "minZoom, maxZoom, type, numTiles, defaultSet, date "
                     "FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
        return records;
    }

    while (query.next()) {
        TileSetRecord rec;
        rec.setID = query.value(0).toULongLong();
        rec.name = query.value(1).toString();
        rec.mapTypeStr = query.value(2).toString();
        rec.topleftLat = query.value(3).toDouble();
        rec.topleftLon = query.value(4).toDouble();
        rec.bottomRightLat = query.value(5).toDouble();
        rec.bottomRightLon = query.value(6).toDouble();
        rec.minZoom = query.value(7).toInt();
        rec.maxZoom = query.value(8).toInt();
        rec.type = query.value(9).toInt();
        rec.numTiles = query.value(10).toUInt();
        rec.defaultSet = (query.value(11).toInt() != 0);
        rec.date = query.value(12).toULongLong();
        records.append(rec);
    }

    return records;
}

std::optional<quint64> QGCTileCacheDatabase::createTileSet(const QString &name, const QString &mapTypeStr,
                                                            double topleftLat, double topleftLon,
                                                            double bottomRightLat, double bottomRightLon,
                                                            int minZoom, int maxZoom, const QString &type, quint32 numTiles)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for createTileSet";
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (!query.prepare("INSERT INTO TileSets("
        "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, date"
        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare createTileSet):" << query.lastError().text();
        return std::nullopt;
    }
    query.addBindValue(name);
    query.addBindValue(mapTypeStr);
    query.addBindValue(topleftLat);
    query.addBindValue(topleftLon);
    query.addBindValue(bottomRightLat);
    query.addBindValue(bottomRightLon);
    query.addBindValue(minZoom);
    query.addBindValue(maxZoom);
    query.addBindValue(UrlFactory::getQtMapIdFromProviderType(type));
    query.addBindValue(numTiles);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (add tileSet into TileSets):" << query.lastError().text();
        return std::nullopt;
    }

    const quint64 setID = query.lastInsertId().toULongLong();

    // Process tiles in streaming batches to avoid holding all coordinates in memory
    constexpr int kHashBatchSize = 500;
    const int mapTypeId = UrlFactory::getQtMapIdFromProviderType(type);

    struct TileCoord { int x, y; QString hash; };

    auto processBatch = [&](const QList<TileCoord> &tiles, int z) -> bool {
        QHash<QString, quint64> existingTiles;
        QSqlQuery lookup(_database());
        lookup.setForwardOnly(true);
        if (lookup.prepare(QStringLiteral("SELECT hash, tileID FROM Tiles WHERE hash IN (%1)").arg(QGCSqlHelper::placeholders(tiles.size())))) {
            for (const auto &tc : tiles) {
                lookup.addBindValue(tc.hash);
            }
            if (lookup.exec()) {
                while (lookup.next()) {
                    existingTiles.insert(lookup.value(0).toString(), lookup.value(1).toULongLong());
                }
            }
        }

        for (const auto &tc : tiles) {
            auto it = existingTiles.find(tc.hash);
            if (it != existingTiles.end()) {
                if (!query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
                    return false;
                }
                query.addBindValue(it.value());
                query.addBindValue(setID);
                if (!query.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
                    return false;
                }
            } else {
                if (!query.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ?, ?, ?)")) {
                    return false;
                }
                query.addBindValue(setID);
                query.addBindValue(tc.hash);
                query.addBindValue(mapTypeId);
                query.addBindValue(tc.x);
                query.addBindValue(tc.y);
                query.addBindValue(z);
                query.addBindValue(static_cast<int>(QGCTile::StatePending));
                if (!query.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (add tile into TilesDownload):" << query.lastError().text();
                    return false;
                }
            }
        }
        return true;
    };

    for (int z = minZoom; z <= maxZoom; z++) {
        const QGCTileSet set = UrlFactory::getTileCount(z, topleftLon, topleftLat, bottomRightLon, bottomRightLat, type);

        QList<TileCoord> batch;
        batch.reserve(kHashBatchSize);

        for (int x = set.tileX0; x <= set.tileX1; x++) {
            for (int y = set.tileY0; y <= set.tileY1; y++) {
                batch.append({x, y, UrlFactory::getTileHash(type, x, y, z)});

                if (batch.size() >= kHashBatchSize) {
                    if (!processBatch(batch, z)) return std::nullopt;
                    batch.clear();
                }
            }
        }

        if (!batch.isEmpty()) {
            if (!processBatch(batch, z)) return std::nullopt;
        }
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit createTileSet transaction";
        return std::nullopt;
    }

    return setID;
}

bool QGCTileCacheDatabase::deleteTileSet(quint64 id)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for deleteTileSet";
        return false;
    }

    QSqlQuery query(_database());

    // Delete download queue entries first
    if (!query.prepare("DELETE FROM TilesDownload WHERE setID = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare download delete:" << query.lastError().text();
        return false;
    }
    query.addBindValue(id);
    if (!query.exec()) {
        return false;
    }

    // Find tiles unique to this set (not shared with other sets)
    // Must collect IDs before deleting SetTiles links
    QList<quint64> uniqueTileIDs;
    if (query.prepare(QStringLiteral("SELECT tileID FROM SetTiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery))) {
        query.addBindValue(id);
        if (query.exec()) {
            while (query.next()) {
                uniqueTileIDs.append(query.value(0).toULongLong());
            }
        }
    }

    // Remove set-tile links
    if (!query.prepare("DELETE FROM SetTiles WHERE setID = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare SetTiles delete:" << query.lastError().text();
        return false;
    }
    query.addBindValue(id);
    if (!query.exec()) {
        return false;
    }

    // Delete unique tiles (no longer referenced by any set)
    if (!uniqueTileIDs.isEmpty()) {
        if (query.prepare(QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(QGCSqlHelper::placeholders(uniqueTileIDs.size())))) {
            for (const quint64 tileID : uniqueTileIDs) {
                query.addBindValue(tileID);
            }
            if (!query.exec()) {
                qCWarning(QGCTileCacheDatabaseLog) << "Failed to delete unique tiles:" << query.lastError().text();
                return false;
            }
        }
    }

    // Delete the tile set itself
    if (!query.prepare("DELETE FROM TileSets WHERE setID = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare TileSets delete:" << query.lastError().text();
        return false;
    }
    query.addBindValue(id);
    if (!query.exec()) {
        return false;
    }

    if (id == _defaultSet) {
        _defaultSet = kInvalidTileSet;
    }

    return txn.commit();
}

bool QGCTileCacheDatabase::renameTileSet(quint64 setID, const QString &newName)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (!query.prepare("UPDATE TileSets SET name = ? WHERE setID = ?")) {
        return false;
    }
    query.addBindValue(newName);
    query.addBindValue(setID);
    return query.exec();
}

std::optional<quint64> QGCTileCacheDatabase::findTileSetID(const QString &name)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (!query.prepare("SELECT setID FROM TileSets WHERE name = ?")) {
        return std::nullopt;
    }
    query.addBindValue(name);
    if (query.exec() && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
}

bool QGCTileCacheDatabase::resetDatabase()
{
    if (!_ensureConnected()) {
        return false;
    }

    _defaultSet = kInvalidTileSet;

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for resetDatabase";
        return false;
    }
    QSqlQuery query(_database());
    if (!query.exec("DROP TABLE IF EXISTS TilesDownload") ||
        !query.exec("DROP TABLE IF EXISTS SetTiles") ||
        !query.exec("DROP TABLE IF EXISTS Tiles") ||
        !query.exec("DROP TABLE IF EXISTS TileSets")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to drop tables:" << query.lastError().text();
        return false;
    }
    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit table drops in resetDatabase";
        return false;
    }
    _valid = _createDB(_database());
    return _valid;
}

QList<QGCTile> QGCTileCacheDatabase::getTileDownloadList(quint64 setID, int count)
{
    QList<QGCTile> tiles;
    if (!_ensureConnected()) {
        return tiles;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for getTileDownloadList";
        return tiles;
    }

    QSqlQuery query(_database());
    if (!query.prepare("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = ? AND state = ? LIMIT ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare tile download list query:" << query.lastError().text();
        return tiles;
    }
    query.addBindValue(setID);
    query.addBindValue(static_cast<int>(QGCTile::StatePending));
    query.addBindValue(count);
    if (!query.exec()) {
        return tiles;
    }

    while (query.next()) {
        QGCTile tile;
        tile.hash = query.value(0).toString();
        tile.type = query.value(1).toInt();
        tile.x = query.value(2).toInt();
        tile.y = query.value(3).toInt();
        tile.z = query.value(4).toInt();
        tiles.append(std::move(tile));
    }

    if (!tiles.isEmpty()) {
        if (query.prepare(QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash IN (%1)").arg(QGCSqlHelper::placeholders(tiles.size())))) {
            query.addBindValue(static_cast<int>(QGCTile::StateDownloading));
            query.addBindValue(setID);
            for (qsizetype i = 0; i < tiles.size(); i++) {
                query.addBindValue(tiles[i].hash);
            }
            if (!query.exec()) {
                qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (batch set TilesDownload state):" << query.lastError().text();
                tiles.clear();
                return tiles;
            }
        }
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit getTileDownloadList transaction";
        tiles.clear();
    }

    return tiles;
}

bool QGCTileCacheDatabase::updateTileDownloadState(quint64 setID, int state, const QString &hash)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (state == QGCTile::StateComplete) {
        if (!query.prepare("DELETE FROM TilesDownload WHERE setID = ? AND hash = ?")) {
            return false;
        }
        query.addBindValue(setID);
        query.addBindValue(hash);
    } else {
        if (!query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash = ?")) {
            return false;
        }
        query.addBindValue(state);
        query.addBindValue(setID);
        query.addBindValue(hash);
    }

    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Error:" << query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::updateAllTileDownloadStates(quint64 setID, int state)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (!query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ?")) {
        return false;
    }
    query.addBindValue(state);
    query.addBindValue(setID);

    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Error:" << query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::pruneCache(quint64 amount)
{
    if (!_ensureConnected()) {
        return false;
    }

    quint64 remaining = amount;
    while (remaining > 0) {
        QSqlQuery query(_database());
        query.setForwardOnly(true);
        if (!query.prepare(QStringLiteral("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (%1) ORDER BY date ASC LIMIT ?").arg(kUniqueTilesSubquery))) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare prune query:" << query.lastError().text();
            return false;
        }
        query.addBindValue(_getDefaultTileSet());
        query.addBindValue(kPruneBatchSize);
        if (!query.exec()) {
            return false;
        }

        QList<quint64> tileIDs;
        while (query.next() && (remaining > 0)) {
            tileIDs << query.value(0).toULongLong();
            const quint64 sz = query.value(1).toULongLong();
            remaining = (sz >= remaining) ? 0 : remaining - sz;
            qCDebug(QGCTileCacheDatabaseLog) << "HASH:" << query.value(2).toString();
        }

        if (tileIDs.isEmpty()) {
            break;
        }

        QGCSqlHelper::Transaction txn(_database());
        if (!txn.ok()) {
            return false;
        }

        if (!_deleteTilesByIDs(tileIDs)) {
            return false;
        }

        if (!txn.commit()) {
            return false;
        }
    }

    return true;
}

void QGCTileCacheDatabase::deleteBingNoTileTiles()
{
    if (!_ensureConnected()) {
        return;
    }

    QSettings settings;
    if (settings.value(QLatin1String(kBingNoTileDoneKey), false).toBool()) {
        return;
    }

    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    if (!file.open(QFile::ReadOnly)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to Open File" << file.fileName() << ":" << file.errorString();
        return;
    }

    const QByteArray noTileBytes = file.readAll();
    file.close();

    QSqlQuery query(_database());
    query.setForwardOnly(true);
    if (!query.prepare("SELECT tileID, hash FROM Tiles WHERE LENGTH(tile) = ? AND tile = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare Bing no-tile query";
        return;
    }
    query.addBindValue(noTileBytes.length());
    query.addBindValue(noTileBytes);
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "query failed";
        return;
    }

    QList<quint64> idsToDelete;
    while (query.next()) {
        idsToDelete.append(query.value(0).toULongLong());
        qCDebug(QGCTileCacheDatabaseLog) << "HASH:" << query.value(1).toString();
    }

    if (idsToDelete.isEmpty()) {
        settings.setValue(QLatin1String(kBingNoTileDoneKey), true);
        return;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        return;
    }

    bool allSucceeded = true;
    for (qsizetype offset = 0; offset < idsToDelete.size(); offset += kPruneBatchSize) {
        const qsizetype batchEnd = qMin(offset + static_cast<qsizetype>(kPruneBatchSize), idsToDelete.size());
        const QList<quint64> batch = idsToDelete.mid(offset, batchEnd - offset);
        if (!_deleteTilesByIDs(batch)) {
            allSucceeded = false;
            break;
        }
    }

    if (allSucceeded && txn.commit()) {
        settings.setValue(QLatin1String(kBingNoTileDoneKey), true);
    }
}

TotalsResult QGCTileCacheDatabase::computeTotals()
{
    TotalsResult result;
    if (!_ensureConnected()) {
        return result;
    }

    QSqlQuery query(_database());

    if (query.exec("SELECT COUNT(size), SUM(size) FROM Tiles") && query.next()) {
        result.totalCount = query.value(0).toUInt();
        result.totalSize = query.value(1).toULongLong();
    }

    if (!query.prepare(QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery))) {
        return result;
    }
    query.addBindValue(_getDefaultTileSet());
    if (query.exec() && query.next()) {
        result.defaultCount = query.value(0).toUInt();
        result.defaultSize = query.value(1).toULongLong();
    }

    return result;
}

SetTotalsResult QGCTileCacheDatabase::computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount, const QString &type)
{
    SetTotalsResult result;

    if (isDefault) {
        TotalsResult totals = computeTotals();
        result.savedTileCount = totals.totalCount;
        result.savedTileSize = totals.totalSize;
        result.totalTileSize = totals.totalSize;
        result.uniqueTileCount = totals.defaultCount;
        result.uniqueTileSize = totals.defaultSize;
        return result;
    }

    if (!_ensureConnected()) {
        return result;
    }

    QSqlQuery subquery(_database());
    if (!subquery.prepare("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = ?")) {
        return result;
    }
    subquery.addBindValue(setID);
    if (!subquery.exec() || !subquery.next()) {
        return result;
    }

    result.savedTileCount = subquery.value(0).toUInt();
    result.savedTileSize = subquery.value(1).toULongLong();

    quint64 avg = UrlFactory::averageSizeForType(type);
    if (avg == 0) {
        avg = 4096;
    }
    if (totalTileCount <= result.savedTileCount) {
        result.totalTileSize = result.savedTileSize;
    } else {
        if ((result.savedTileCount > 10) && result.savedTileSize) {
            avg = result.savedTileSize / result.savedTileCount;
        }
        result.totalTileSize = avg * totalTileCount;
    }

    quint32 dbUniqueCount = 0;
    quint64 dbUniqueSize = 0;
    if (subquery.prepare(QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery))) {
        subquery.addBindValue(setID);
        if (subquery.exec() && subquery.next()) {
            dbUniqueCount = subquery.value(0).toUInt();
            dbUniqueSize = subquery.value(1).toULongLong();
        }
    } else {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare unique tiles query:" << subquery.lastError().text();
    }

    if (dbUniqueCount > 0) {
        result.uniqueTileCount = dbUniqueCount;
        result.uniqueTileSize = dbUniqueSize;
    } else {
        const quint32 estimatedCount = (totalTileCount > result.savedTileCount) ? (totalTileCount - result.savedTileCount) : 0;
        result.uniqueTileCount = estimatedCount;
        result.uniqueTileSize = estimatedCount * avg;
    }

    return result;
}

DatabaseResult QGCTileCacheDatabase::importSetsReplace(const QString &path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Import path must differ from the active database";
        return result;
    }
    _defaultSet = kInvalidTileSet;
    disconnectDB();
    const QString backupPath = _databasePath + QStringLiteral(".bak");
    (void) QFile::remove(backupPath);
    const bool hasBackup = QFile::rename(_databasePath, backupPath);
    if (!hasBackup) {
        (void) QFile::remove(_databasePath);
    }
    if (!QFile::copy(path, _databasePath)) {
        if (hasBackup) {
            (void) QFile::rename(backupPath, _databasePath);
        }
        result.errorString = "Failed to copy import database";
        _valid = false;
        _failed = true;
        return result;
    }
    (void) QFile::remove(backupPath);
    if (progressCb) progressCb(25);
    init();
    if (!_valid) {
        result.errorString = QStringLiteral("Failed to initialize tile cache database after import");
    } else {
        if (progressCb) progressCb(50);
        connectDB();
        if (!_valid) {
            result.errorString = QStringLiteral("Failed to connect to tile cache database after import");
        }
    }
    if (progressCb) progressCb(100);
    result.success = _valid;
    return result;
}

DatabaseResult QGCTileCacheDatabase::importSetsMerge(const QString &path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Import path must differ from the active database";
        return result;
    }
    if (!_ensureConnected()) {
        result.errorString = "Database not connected";
        return result;
    }

    QGCSqlHelper::ScopedConnection importDB(path, /*readOnly=*/true,
                                            QStringLiteral("QGeoTileImportSession"));
    if (!importDB.isValid()) {
        result.errorString = "Error opening import database";
        return result;
    }

    QSqlQuery query(importDB.database());
    quint64 tileCount = 0;
    int lastProgress = -1;
    if (query.exec("SELECT COUNT(tileID) FROM Tiles") && query.next()) {
        tileCount = query.value(0).toULongLong();
    }

    bool tilesImported = false;

    if (tileCount > 0) {
        if (query.exec("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
            quint64 currentCount = 0;
            while (query.next()) {
                QString name = query.value("name").toString();
                const quint64 setID = query.value("setID").toULongLong();
                const QString mapType = query.value("typeStr").toString();
                const double topleftLat = query.value("topleftLat").toDouble();
                const double topleftLon = query.value("topleftLon").toDouble();
                const double bottomRightLat = query.value("bottomRightLat").toDouble();
                const double bottomRightLon = query.value("bottomRightLon").toDouble();
                const int minZoom = query.value("minZoom").toInt();
                const int maxZoom = query.value("maxZoom").toInt();
                const int type = query.value("type").toInt();
                const quint32 numTiles = query.value("numTiles").toUInt();
                const int defaultSet = query.value("defaultSet").toInt();
                quint64 insertSetID = _getDefaultTileSet();

                // Wrap each set creation + tile copy in a single transaction
                QGCSqlHelper::Transaction txn(_database());
                if (!txn.ok()) {
                    result.errorString = "Failed to start transaction for import set";
                    break;
                }

                if (defaultSet == 0) {
                    name = _deduplicateSetName(name);
                    QSqlQuery cQuery(_database());
                    if (!cQuery.prepare("INSERT INTO TileSets("
                        "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, defaultSet, date"
                        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
                        result.errorString = "Error preparing tile set insert";
                        break;
                    }
                    cQuery.addBindValue(name);
                    cQuery.addBindValue(mapType);
                    cQuery.addBindValue(topleftLat);
                    cQuery.addBindValue(topleftLon);
                    cQuery.addBindValue(bottomRightLat);
                    cQuery.addBindValue(bottomRightLon);
                    cQuery.addBindValue(minZoom);
                    cQuery.addBindValue(maxZoom);
                    cQuery.addBindValue(type);
                    cQuery.addBindValue(numTiles);
                    cQuery.addBindValue(defaultSet);
                    cQuery.addBindValue(QDateTime::currentSecsSinceEpoch());
                    if (!cQuery.exec()) {
                        result.errorString = "Error adding imported tile set to database";
                        break;
                    }
                    insertSetID = cQuery.lastInsertId().toULongLong();
                }

                quint64 tilesIterated = 0;
                const quint64 tilesSaved = _copyTilesForSet(importDB.database(), setID, insertSetID,
                                                             currentCount, tileCount,
                                                             lastProgress, progressCb,
                                                             &tilesIterated, false);
                if (tilesSaved > 0) {
                    tilesImported = true;
                    QSqlQuery cQuery(_database());
                    if (cQuery.prepare("SELECT COUNT(size) FROM Tiles A INNER JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = ?")) {
                        cQuery.addBindValue(insertSetID);
                        if (cQuery.exec() && cQuery.next()) {
                            const quint64 count = cQuery.value(0).toULongLong();
                            if (cQuery.prepare("UPDATE TileSets SET numTiles = ? WHERE setID = ?")) {
                                cQuery.addBindValue(count);
                                cQuery.addBindValue(insertSetID);
                                (void) cQuery.exec();
                            }
                        }
                    }
                }

                if (!txn.commit()) {
                    qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit import transaction for set:" << name;
                    continue;
                }

                if (tilesIterated > tilesSaved) {
                    const quint64 alreadyExisting = tilesIterated - tilesSaved;
                    tileCount = (alreadyExisting < tileCount) ? tileCount - alreadyExisting : 0;
                }

                if ((tilesSaved == 0) && (defaultSet == 0)) {
                    qCDebug(QGCTileCacheDatabaseLog) << "No unique tiles in" << name << "Removing it.";
                    deleteTileSet(insertSetID);
                }
            }
        } else {
            result.errorString = "No tile set in database";
        }
    }

    if (!tilesImported && result.errorString.isEmpty()) {
        result.errorString = "No unique tiles in imported database";
    }
    result.success = result.errorString.isEmpty();
    return result;
}

DatabaseResult QGCTileCacheDatabase::exportSets(const QList<TileSetRecord> &sets, const QString &path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (!_ensureConnected()) {
        result.errorString = "Database not connected";
        return result;
    }
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Export path must differ from the active database";
        return result;
    }

    (void) QFile::remove(path);
    QGCSqlHelper::ScopedConnection exportDB(path, /*readOnly=*/false,
                                            QStringLiteral("QGeoTileExportSession"));
    if (!exportDB.isValid()) {
        qCCritical(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create export database):" << exportDB.database().lastError();
        result.errorString = "Error opening export database";
        return result;
    }

    if (!_createDB(exportDB.database(), false)) {
        result.errorString = "Error creating export database";
        return result;
    }

    quint64 tileCount = 0;
    quint64 currentCount = 0;
    int lastProgress = -1;
    for (const auto &set : sets) {
        QSqlQuery countQuery(_database());
        quint64 actualCount = 0;
        if (countQuery.prepare("SELECT COUNT(*) FROM Tiles T INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?")) {
            countQuery.addBindValue(set.setID);
            if (countQuery.exec() && countQuery.next()) {
                actualCount = countQuery.value(0).toULongLong();
            }
        }
        tileCount += (actualCount > 0) ? actualCount : set.numTiles;
    }

    if (tileCount == 0) {
        tileCount = 1;
    }

    for (const auto &set : sets) {
        QSqlQuery query(_database());
        query.setForwardOnly(true);
        if (!query.prepare("SELECT T.hash, T.format, T.tile, T.type, T.date FROM Tiles T "
                           "INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?")) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare tile query for export set" << set.name;
            continue;
        }
        query.addBindValue(set.setID);
        if (!query.exec()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to query tiles for export set" << set.name;
            continue;
        }

        QGCSqlHelper::Transaction txn(exportDB.database());
        if (!txn.ok()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for export set" << set.name;
            result.errorString = "Failed to start export transaction";
            break;
        }

        QSqlQuery exportQuery(exportDB.database());
        if (!exportQuery.prepare("INSERT INTO TileSets("
            "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, defaultSet, date"
            ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
            result.errorString = "Error preparing tile set insert for export";
            break;
        }
        exportQuery.addBindValue(set.name);
        exportQuery.addBindValue(set.mapTypeStr);
        exportQuery.addBindValue(set.topleftLat);
        exportQuery.addBindValue(set.topleftLon);
        exportQuery.addBindValue(set.bottomRightLat);
        exportQuery.addBindValue(set.bottomRightLon);
        exportQuery.addBindValue(set.minZoom);
        exportQuery.addBindValue(set.maxZoom);
        exportQuery.addBindValue(set.type);
        exportQuery.addBindValue(set.numTiles);
        exportQuery.addBindValue(set.defaultSet);
        exportQuery.addBindValue(set.date);
        if (!exportQuery.exec()) {
            result.errorString = "Error adding tile set to exported database";
            break;
        }

        const quint64 exportSetID = exportQuery.lastInsertId().toULongLong();

        quint64 skippedTiles = 0;
        while (query.next()) {
            const QString hash = query.value(0).toString();
            const QString format = query.value(1).toString();
            const QByteArray img = query.value(2).toByteArray();
            const int tileType = query.value(3).toInt();
            const quint64 tileDate = query.value(4).toULongLong();

            quint64 exportTileID = 0;
            if (!exportQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)")) {
                qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare tile INSERT for export:" << exportQuery.lastError().text();
                skippedTiles++;
                continue;
            }
            exportQuery.addBindValue(hash);
            exportQuery.addBindValue(format);
            exportQuery.addBindValue(img);
            exportQuery.addBindValue(img.size());
            exportQuery.addBindValue(tileType);
            exportQuery.addBindValue(tileDate);
            if (exportQuery.exec()) {
                exportTileID = exportQuery.lastInsertId().toULongLong();
            } else {
                QSqlQuery lookup(exportDB.database());
                if (lookup.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
                    lookup.addBindValue(hash);
                    if (lookup.exec() && lookup.next()) {
                        exportTileID = lookup.value(0).toULongLong();
                    }
                }
            }

            if (exportTileID > 0) {
                if (exportQuery.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
                    exportQuery.addBindValue(exportTileID);
                    exportQuery.addBindValue(exportSetID);
                    if (!exportQuery.exec()) {
                        qCWarning(QGCTileCacheDatabaseLog) << "Failed to link tile to set in export:" << exportQuery.lastError().text();
                    }
                }
            } else {
                skippedTiles++;
            }
            currentCount++;
            if (progressCb) {
                const int progress = qMin(100, static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0));
                if (lastProgress != progress) {
                    lastProgress = progress;
                    progressCb(progress);
                }
            }
        }
        if (skippedTiles > 0) {
            qCWarning(QGCTileCacheDatabaseLog) << "Skipped" << skippedTiles << "tiles during export of" << set.name;
        }
        if (!txn.commit()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit export transaction for" << set.name;
        }
    }

    result.success = result.errorString.isEmpty();
    return result;
}

//FoxFour part
namespace {

constexpr int kMRFTileSize = 256;
// EPSG:3857 (Web Mercator) constants: half the projected world width/height in meters.
constexpr double kMRFOriginShift = 20037508.342789244;
// Latitude beyond which web mercator is undefined.
constexpr double kMRFMaxLatitude = 85.05112878;
// How many tiles to sample when deciding the pyramid's single page format.
constexpr int kMRFFormatSampleLimit = 64;
// Written into <Quality> and used when encoding the overview pages we generate.
constexpr int kMRFJpegQuality = 90;
// The nominal grid is probed one tile wider on each side so a differing rounding convention
// between the downloader and this exporter cannot silently clip an edge row or column.
constexpr int kMRFProbeMargin = 1;
// Sanity bound on a sniffed page size, so a corrupt header cannot ask for a huge canvas.
constexpr int kMRFMaxPageSize = 8192;

// GDAL/PROJ's canonical WKT for EPSG:3857, embedded verbatim so exported MRFs are
// georeferenced without depending on a PROJ database being available at export time.
// The AUTHORITY nodes and the PROJ4 EXTENSION (in particular +nadgrids=@null) are what
// let GDAL resolve this back to EPSG:3857 rather than an anonymous Mercator_1SP.
constexpr const char *kMRFWebMercatorWkt =
    "PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\","
    "SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],"
    "AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],"
    "UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],"
    "AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],"
    "PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],"
    "PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],"
    "UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],"
    "AXIS[\"Northing\",NORTH],EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 "
    "+lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs\"],"
    "AUTHORITY[\"EPSG\",\"3857\"]]";

// --- Web mercator tile arithmetic ---------------------------------------------------------
// Done here rather than through UrlFactory::getTileCount so the geometry is guaranteed to be
// the standard XYZ mercator scheme the MRF header declares, and so the exporter keeps working
// for sets whose provider type cannot be resolved. Any disagreement with the downloader's own
// rounding is absorbed by the probe margin above.

QPoint coordToMrf(double lon, double lat, int zoom) {
    QPoint result;
    const double maxIndex = static_cast<double>(quint64(1) << zoom);
    result.setX(qBound(0.0,std::floor(((qBound(-180.0, lon, 180.0) + 180.0) / 360.0) * maxIndex), maxIndex - 1.0));
    const double s = std::sin(qDegreesToRadians(qBound(-kMRFMaxLatitude, lat, kMRFMaxLatitude)));
    result.setY(qBound(0.0,std::floor((0.5 - (std::log((1.0 + s) / (1.0 - s)) / (4.0 * M_PI))) * maxIndex), maxIndex - 1.0));
    return result;
}

QGeoCoordinate mrfToCoord(const QPoint& mrfPos) {
    QGeoCoordinate result;
    result.setLongitude((mrfPos.x() / kMRFOriginShift) * 180.0);
    const double phi = (mrfPos.y() / kMRFOriginShift) * 180.0;
    result.setLatitude(qRadiansToDegrees((2.0 * std::atan(std::exp(qDegreesToRadians(phi)))) - (M_PI / 2.0)));
    return result;
}

struct MRFPageFormat
{
    QString compression;
    int channels = 0;
    // Encoded pixel size of the page. MRF declares one <PageSize> for the entire file and GDAL
    // trusts it without consulting the pages, so a tile that decodes to any other size has to
    // be rescaled -- otherwise the raster is laid out on the wrong pixel grid and comes out
    // stretched. Providers do ship 512px tiles, and a set can mix sizes.
    int width = 0;
    int height = 0;
    // Palette PNGs decode to one band plus a color table, so they sniff identically to
    // greyscale but cannot be regenerated as one when we build overviews. Kept separate so the
    // two never end up in the same vote bucket.
    bool palette = false;

    bool isValid() const
    {
        return (channels > 0) && (width > 0) && (height > 0) &&
               (width <= kMRFMaxPageSize) && (height <= kMRFMaxPageSize) && !compression.isEmpty();
    }
    bool operator==(const MRFPageFormat &other) const
    {
        return (channels == other.channels) && (compression == other.compression) &&
               (palette == other.palette) && (width == other.width) && (height == other.height);
    }
};

int mrfReadBigEndian32(const QByteArray &b, qsizetype at)
{
    return (static_cast<quint8>(b.at(at)) << 24) | (static_cast<quint8>(b.at(at + 1)) << 16) |
           (static_cast<quint8>(b.at(at + 2)) << 8) | static_cast<quint8>(b.at(at + 3));
}

/// Reads the band count out of the encoded tile header rather than trusting the Tiles.format
/// column or a QImage round-trip. MRF stores the original PNG/JPEG bytes verbatim and GDAL
/// decodes them with libpng/libjpeg, so <Size c=".."> has to match the codec's own channel
/// count -- a palette PNG decodes to one band plus a color table, not to three.
MRFPageFormat sniffMRFPageFormat(const QByteArray &img)
{
    static const QByteArray pngSignature = QByteArrayLiteral("\x89PNG\r\n\x1a\n");

    MRFPageFormat fmt;

            // IHDR is the mandatory first chunk, so its color type byte sits at a fixed offset.
    if (img.startsWith(pngSignature) && (img.size() > 25)) {
        switch (static_cast<quint8>(img.at(25))) {
            case 0: fmt.channels = 1; break; // greyscale
            case 2: fmt.channels = 3; break; // RGB
            case 3: fmt.channels = 1; fmt.palette = true; break; // palette
            case 4: fmt.channels = 2; break; // greyscale + alpha
            case 6: fmt.channels = 4; break; // RGBA
            default: return MRFPageFormat();
        }
        // IHDR payload: width(4) height(4) then the depth/color bytes read above.
        fmt.width = mrfReadBigEndian32(img, 16);
        fmt.height = mrfReadBigEndian32(img, 20);
        fmt.compression = QStringLiteral("PNG");
        return fmt.isValid() ? fmt : MRFPageFormat();
    }

    if ((img.size() < 4) || (static_cast<quint8>(img.at(0)) != 0xFF) || (static_cast<quint8>(img.at(1)) != 0xD8)) {
        return MRFPageFormat();
    }

            // Walk the JPEG marker segments to the Start-Of-Frame, whose payload carries the component
            // count (1 = greyscale, 3 = YCbCr, 4 = MRF's alpha-carrying mode).
    qsizetype pos = 2;
    while ((pos + 4) <= img.size()) {
        if (static_cast<quint8>(img.at(pos)) != 0xFF) {
            return MRFPageFormat();
        }
        const quint8 marker = static_cast<quint8>(img.at(pos + 1));
        if (marker == 0xFF) { // fill byte
            pos++;
            continue;
        }
        if ((marker >= 0xD0) && (marker <= 0xD9)) { // RSTn / SOI / EOI carry no length
            pos += 2;
            continue;
        }
        if (marker == 0xDA) { // start of scan; no frame header found
            return MRFPageFormat();
        }

        const int segLen = (static_cast<quint8>(img.at(pos + 2)) << 8) | static_cast<quint8>(img.at(pos + 3));
        if (segLen < 2) {
            return MRFPageFormat();
        }

        static const QSet<quint8> sofMarkers = {0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                                                0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF};
        if (sofMarkers.contains(marker)) {
            // payload: precision(1) height(2) width(2) componentCount(1)
            if ((pos + 9) >= img.size()) {
                return MRFPageFormat();
            }
            fmt.channels = static_cast<quint8>(img.at(pos + 9));
            if ((fmt.channels != 1) && (fmt.channels != 3) && (fmt.channels != 4)) {
                return MRFPageFormat();
            }
            fmt.height = (static_cast<quint8>(img.at(pos + 5)) << 8) | static_cast<quint8>(img.at(pos + 6));
            fmt.width = (static_cast<quint8>(img.at(pos + 7)) << 8) | static_cast<quint8>(img.at(pos + 8));
            fmt.compression = QStringLiteral("JPEG");
            return fmt.isValid() ? fmt : MRFPageFormat();
        }

        pos += 2 + segLen;
    }

    return MRFPageFormat();
}

/// One 16-byte index record: page offset into the data file and encoded page length.
struct MRFIndexEntry
{
    quint64 offset = 0;
    quint64 size = 0;
};

/// One resolution level of the pyramid, in pages.
struct MRFLevel
{
    quint64 pagesX = 0;
    quint64 pagesY = 0;
    QVector<MRFIndexEntry> entries;
};

/// GDAL derives the data file name from the .mrf basename when <DataFile> is absent, using an
/// extension that encodes the compression. Match that so the triplet is self-describing.
QString mrfDataExtension(const QString &compression)
{
    if (compression == QLatin1String("PNG")) {
        return QStringLiteral(".ppg");
    }
    if (compression == QLatin1String("JPEG")) {
        return QStringLiteral(".pjg");
    }
    return QStringLiteral(".til");
}

/// The QImage format whose PNG/JPEG encoding round-trips to exactly `channels` bands. Anything
/// else and the pages we generate would disagree with the header's <Size c="..">.
QImage::Format mrfImageFormat(int channels)
{
    switch (channels) {
        case 1: return QImage::Format_Grayscale8;  // PNG color type 0 / 1-component JPEG
        case 4: return QImage::Format_ARGB32;      // PNG color type 6
        default: return QImage::Format_RGB32;      // PNG color type 2 / 3-component JPEG
    }
}

QByteArray encodeMRFPage(const QImage &page, const MRFPageFormat &fmt)
{
    const QImage converted = (page.format() == mrfImageFormat(fmt.channels))
    ? page
    : page.convertToFormat(mrfImageFormat(fmt.channels));
    if (converted.isNull()) {
        return QByteArray();
    }

    QByteArray out;
    QBuffer buffer(&out);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return QByteArray();
    }

    const bool jpeg = (fmt.compression == QLatin1String("JPEG"));
    QImageWriter writer(&buffer, jpeg ? QByteArrayLiteral("jpeg") : QByteArrayLiteral("png"));
    if (jpeg) {
        writer.setQuality(kMRFJpegQuality);
    }
    if (!writer.write(converted)) {
        return QByteArray();
    }
    buffer.close();

    return out;
}

/// bounds.json stores coordinates at 7 decimals, which is well under a millimetre on the ground
/// and keeps the file diffable.
double mrfRound7(double v)
{
    return std::round(v * 1e7) / 1e7;
}

QJsonObject mrfBoundsObject(const QGeoCoordinate &minCoord, const QGeoCoordinate &maxCoord)
{
    QJsonObject o;
    o[QStringLiteral("min_lat")] = mrfRound7(minCoord.latitude());
    o[QStringLiteral("min_lon")] = mrfRound7(minCoord.longitude());
    o[QStringLiteral("max_lat")] = mrfRound7(maxCoord.latitude());
    o[QStringLiteral("max_lon")] = mrfRound7(maxCoord.longitude());
    return o;
}

/// Which provider name, zoom level and tile rectangle the set's cached tiles actually sit at.
struct MRFTileGrid
{
    QString providerType;
    int zoom = -1;
    int tileX0 = 0;
    int tileY0 = 0;
    int tileX1 = -1;
    int tileY1 = -1;
    quint64 tilesFound = 0;

    bool isValid() const { return (zoom >= 0) && (tileX1 >= tileX0) && (tileY1 >= tileY0); }
};

/// Locates the set's tiles by hash instead of trusting a provider lookup.
///
/// `Tiles` has no x/y/z columns -- coordinates live only inside the hash -- so the grid has to
/// be found by generating candidate hashes and seeing which ones the set actually contains.
/// That makes the provider name self-validating: whichever candidate matches real rows is the
/// right one, and a set whose type int no longer resolves still exports fine as long as its
/// stored `typeStr` does. Zooms are tried finest first, so a set whose maxZoom was never fully
/// downloaded falls back to the finest zoom that has data rather than exporting an empty
/// raster.
MRFTileGrid findMRFTileGrid(const TileSetRecord &set, const QSet<QString> &setHashes,
                            const QStringList &candidateTypes)
{
    const double minLat = qMin(set.topleftLat, set.bottomRightLat);
    const double maxLat = qMax(set.topleftLat, set.bottomRightLat);
    const double minLon = qMin(set.topleftLon, set.bottomRightLon);
    const double maxLon = qMax(set.topleftLon, set.bottomRightLon);

    const int maxZoom = qBound(0, set.maxZoom, 22);
    const int minZoom = qBound(0, qMin(set.minZoom, maxZoom), maxZoom);

    for (int zoom = maxZoom; zoom >= minZoom; zoom--) {
        QPoint minCoord = coordToMrf(minLon, minLat, zoom);
        QPoint maxCoord = coordToMrf(maxLon, maxLat, zoom);

        const int worldMax = static_cast<int>((quint64(1) << zoom) - 1);
        const int probeX0 = qMax(0, minCoord.x() - kMRFProbeMargin);
        const int probeX1 = qMin(worldMax, maxCoord.x() + kMRFProbeMargin);
        const int probeY0 = qMax(0, maxCoord.y() - kMRFProbeMargin);
        const int probeY1 = qMin(worldMax, minCoord.y() + kMRFProbeMargin);

        MRFTileGrid best;
        for (const QString &type : candidateTypes) {
            MRFTileGrid found;
            found.providerType = type;
            found.zoom = zoom;
            found.tileX0 = probeX1;
            found.tileY0 = probeY1;
            found.tileX1 = probeX0;
            found.tileY1 = probeY0;

            for (int y = probeY0; y <= probeY1; y++) {
                for (int x = probeX0; x <= probeX1; x++) {
                    if (!setHashes.contains(UrlFactory::getTileHash(type, x, y, zoom))) {
                        continue;
                    }
                    found.tilesFound++;
                    found.tileX0 = qMin(found.tileX0, x);
                    found.tileX1 = qMax(found.tileX1, x);
                    found.tileY0 = qMin(found.tileY0, y);
                    found.tileY1 = qMax(found.tileY1, y);
                }
            }

            if (found.tilesFound > best.tilesFound) {
                best = found;
            }
        }

                // Finest zoom with any coverage wins; the MRF's own overviews supply the rest.
        if (best.tilesFound > 0) {
            return best;
        }
    }

    return MRFTileGrid();
}

} // namespace

DatabaseResult QGCTileCacheDatabase::exportSetAsMRF(const TileSetRecord &set, const QString &basePath, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (!_ensureConnected()) {
        result.errorString = QStringLiteral("Database not connected");
        return result;
    }

            // Elevation providers use their own flat degree grid (and count y northwards), so the
            // web-mercator geometry below does not describe them at all.
    if (UrlFactory::isElevation(set.type)) {
        result.errorString = QStringLiteral("Elevation tile sets cannot be exported as MRF");
        return result;
    }

            // --- Inventory the set's tiles --------------------------------------------------------
            // Scoped through SetTiles on setID, the same way exportSets() does, so the raster contains
            // this set's tiles and nothing else. Only the hashes are held; blobs are fetched per page
            // below so a multi-gigabyte set never has to fit in memory.
    QSet<QString> setHashes;
    {
        QSqlQuery hashQuery(_database());
        hashQuery.setForwardOnly(true);
        if (!hashQuery.prepare("SELECT T.hash FROM Tiles T "
                               "INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?")) {
            result.errorString = QStringLiteral("Failed to prepare tile query for MRF export");
            return result;
        }
        hashQuery.addBindValue(set.setID);
        if (!hashQuery.exec()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to query tiles for MRF export of" << set.name
                                               << hashQuery.lastError().text();
            result.errorString = QStringLiteral("Failed to query tiles for MRF export");
            return result;
        }
        while (hashQuery.next()) {
            setHashes.insert(hashQuery.value(0).toString());
        }
    }

    if (setHashes.isEmpty()) {
        result.errorString = QStringLiteral("Tile set '%1' contains no tiles").arg(set.name);
        return result;
    }

            // `typeStr` is the provider name the set was stored with, so it is tried first;
            // getProviderTypeFromQtMapId() is a fallback because `type` holds a provider type id rather
            // than a Qt map id for most sets and so often resolves to nothing.
    QStringList candidateTypes;
    for (const QString &candidate : {set.mapTypeStr, UrlFactory::getProviderTypeFromQtMapId(set.type)}) {
        if (!candidate.isEmpty() && !candidateTypes.contains(candidate)) {
            candidateTypes.append(candidate);
        }
    }
    if (candidateTypes.isEmpty()) {
        result.errorString = QStringLiteral("Tile set '%1' has no usable map provider name").arg(set.name);
        return result;
    }

    const MRFTileGrid grid = findMRFTileGrid(set, setHashes, candidateTypes);
    if (!grid.isValid()) {
        result.errorString = QStringLiteral("Could not locate the %1 cached tiles of '%2' on the "
                                 "web mercator grid (tried provider %3, zoom %4-%5)")
                                 .arg(setHashes.size())
                                 .arg(set.name, candidateTypes.join(QStringLiteral(", ")))
                                 .arg(set.minZoom).arg(set.maxZoom);
        return result;
    }

    const int zoom = grid.zoom;
    const QString type = grid.providerType;
    const quint64 gridWidth = static_cast<quint64>(grid.tileX1 - grid.tileX0 + 1);
    const quint64 gridHeight = static_cast<quint64>(grid.tileY1 - grid.tileY0 + 1);
    const quint64 totalPages = gridWidth * gridHeight;

    qCDebug(QGCTileCacheDatabaseLog) << "MRF export: set" << set.name << "resolved to provider" << type
                                     << "zoom" << zoom << "grid" << gridWidth << "x" << gridHeight
                                     << "-" << grid.tilesFound << "of" << setHashes.size()
                                     << "set tiles at this zoom";

            // Only this set's rows are visible to the page fetch, matching the hash inventory above.
    QSqlQuery tileQuery(_database());
    if (!tileQuery.prepare("SELECT T.tile FROM Tiles T "
                           "INNER JOIN SetTiles S ON T.tileID = S.tileID "
                           "WHERE S.setID = ? AND T.hash = ?")) {
        result.errorString = QStringLiteral("Failed to prepare tile lookup for MRF export");
        return result;
    }

    auto fetchTile = [&](int x, int y) -> QByteArray {
        const QString hash = UrlFactory::getTileHash(type, x, y, zoom);
        if (!setHashes.contains(hash)) { // absent page; no need to hit the database
            return QByteArray();
        }
        tileQuery.addBindValue(set.setID);
        tileQuery.addBindValue(hash);
        if (!tileQuery.exec() || !tileQuery.next()) {
            return QByteArray();
        }
        return tileQuery.value(0).toByteArray();
    };

            // --- Pick the one page format the whole file will declare ------------------------------
            // A single Compression and channel count covers the whole file, so sample the grid to pick
            // the format the majority of cached pages actually use before committing to a header.
    MRFPageFormat pageFormat;
    {
        // Only a handful of (compression, band count) combinations exist, so tally them in a
        // list rather than hashing a composite key.
        QList<QPair<MRFPageFormat, int>> votes;
        const quint64 step = qMax<quint64>(1, totalPages / kMRFFormatSampleLimit);
        int sampled = 0;

        for (quint64 page = 0; (page < totalPages) && (sampled < kMRFFormatSampleLimit); page += step) {
            const int x = grid.tileX0 + static_cast<int>(page % gridWidth);
            const int y = grid.tileY0 + static_cast<int>(page / gridWidth);

            const MRFPageFormat fmt = sniffMRFPageFormat(fetchTile(x, y));
            if (!fmt.isValid()) {
                continue;
            }
            sampled++;

            auto it = std::find_if(votes.begin(), votes.end(),
                                   [&fmt](const QPair<MRFPageFormat, int> &v) { return v.first == fmt; });
            if (it != votes.end()) {
                it->second++;
            } else {
                votes.append({fmt, 1});
            }
        }

        // Color beats a headcount. Sampling 64 pages out of a large grid can easily let a few
        // greyscale or paletted tiles outvote the imagery, and declaring 1 band would flatten
        // the whole export to greyscale -- a much worse outcome than rescaling or transcoding a
        // minority of pages. So once any sampled page has three or more bands, only those
        // compete.
        bool colorAvailable = false;
        for (const auto &vote : votes) {
            colorAvailable = colorAvailable || (vote.first.channels >= 3);
        }

        int best = 0;
        for (const auto &vote : votes) {
            if (colorAvailable && (vote.first.channels < 3)) {
                continue;
            }
            if (vote.second > best) {
                best = vote.second;
                pageFormat = vote.first;
            }
        }
    }

    if (!pageFormat.isValid()) {
        result.errorString = QStringLiteral("No decodable PNG or JPEG tiles cached for this set");
        return result;
    }

            // Base pages are normally stored byte-for-byte, which is both lossless and fast. Three
            // formats break that -- palette PNG, greyscale+alpha PNG, and the 4-component JPEG GDAL
            // writes with an APP3 "Zen" marker -- because QImageWriter cannot reproduce them, so the
            // overviews we generate could not match the header. Promote those sets to RGBA PNG and
            // re-encode every page instead: still lossless, just slower.
    bool recodeBasePages = false;
    const bool unwritableShape = pageFormat.palette ||
                                 ((pageFormat.compression == QLatin1String("PNG")) && (pageFormat.channels == 2)) ||
                                 ((pageFormat.compression == QLatin1String("JPEG")) && (pageFormat.channels == 4));
    if (unwritableShape) {
        qCDebug(QGCTileCacheDatabaseLog) << "MRF export: re-encoding as 4-band PNG;" << pageFormat.compression
                                         << pageFormat.channels << "band (palette:" << pageFormat.palette
                                         << ") pages cannot be regenerated for the overview levels";
        recodeBasePages = true;
        pageFormat = MRFPageFormat{QStringLiteral("PNG"), 4, pageFormat.width, pageFormat.height, false};
    }

    const QString mrfPath = basePath + QStringLiteral(".mrf");
    const QString idxPath = basePath + QStringLiteral(".idx");
    // <DataFile>/<IndexFile> are omitted from the header, so the names must be exactly the ones
    // GDAL derives from the basename.
    const QString datPath = basePath + mrfDataExtension(pageFormat.compression);
    const QString boundsPath = QFileInfo(basePath).dir().filePath(QStringLiteral("bounds.json"));

            // Any of the files left behind from a previous run would be read alongside a new one, so
            // clear them all up front and again on every failure path below. Both possible data
            // extensions are removed because the format vote may have picked a different one this time.
    auto discardOutputs = [&]() {
        (void) QFile::remove(mrfPath);
        (void) QFile::remove(idxPath);
        (void) QFile::remove(datPath);
        (void) QFile::remove(basePath + QStringLiteral(".ppg"));
        (void) QFile::remove(basePath + QStringLiteral(".pjg"));
        (void) QFile::remove(boundsPath);
    };
    discardOutputs();

            // Levels, finest first. Each level halves the page counts, rounding up, and the pyramid
            // stops once a level fits in a single page -- the same shape gdaladdo produces.
    QList<MRFLevel> levels;
    {
        MRFLevel base;
        base.pagesX = gridWidth;
        base.pagesY = gridHeight;
        levels.append(base);
        while ((levels.last().pagesX > 1) || (levels.last().pagesY > 1)) {
            MRFLevel next;
            next.pagesX = qMax<quint64>(1, (levels.last().pagesX + 1) / 2);
            next.pagesY = qMax<quint64>(1, (levels.last().pagesY + 1) / 2);
            levels.append(next);
        }
    }

    quint64 totalWork = 0;
    for (const MRFLevel &level : levels) {
        totalWork += level.pagesX * level.pagesY;
    }

            // Opened ReadWrite because building level N means reading level N-1's pages back out.
    QFile datFile(datPath);
    if (!datFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        result.errorString = QStringLiteral("Failed to create MRF data file");
        discardOutputs();
        return result;
    }

    quint64 dataOffset = 0;
    quint64 pagesWritten = 0;
    quint64 rescaledPages = 0;
    quint64 transcodedPages = 0;
    quint64 undecodablePages = 0;
    quint64 workDone = 0;
    int lastProgress = -1;

    // Brings a tile onto the file's declared page size and codec. Only called when the tile
    // actually differs, so the common case stays a byte-for-byte copy with no recompression.
    auto normalizePage = [&](const QByteArray &src) -> QByteArray {
        QImage decoded = QImage::fromData(src);
        if (decoded.isNull()) {
            return QByteArray();
        }
        if ((decoded.width() != pageFormat.width) || (decoded.height() != pageFormat.height)) {
            decoded = decoded.scaled(pageFormat.width, pageFormat.height,
                                     Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        return encodeMRFPage(decoded, pageFormat);
    };

    auto reportProgress = [&]() {
        workDone++;
        if (!progressCb) {
            return;
        }
        const int progress = qMin(100, static_cast<int>((static_cast<double>(workDone) / static_cast<double>(totalWork)) * 100.0));
        if (lastProgress != progress) {
            lastProgress = progress;
            progressCb(progress);
        }
    };

    auto appendPage = [&](const QByteArray &img, MRFIndexEntry &entry) -> bool {
        if (img.isEmpty()) { // absent page: (0, 0)
            entry = MRFIndexEntry();
            return true;
        }
        if (datFile.write(img) != img.size()) {
            return false;
        }
        entry.offset = dataOffset;
        entry.size = static_cast<quint64>(img.size());
        dataOffset += entry.size;
        pagesWritten++;
        return true;
    };

    auto fail = [&](const QString &what) {
        result.errorString = what;
        discardOutputs();
    };

            // --- Level 0: the cached tiles themselves, in row-major page order. -------------------
    {
        MRFLevel &base = levels[0];
        base.entries.resize(static_cast<qsizetype>(totalPages));

        for (quint64 page = 0; page < totalPages; page++) {
            const int x = grid.tileX0 + static_cast<int>(page % gridWidth);
            const int y = grid.tileY0 + static_cast<int>(page / gridWidth);

            QByteArray img = fetchTile(x, y);
            if (!img.isEmpty()) {
                const MRFPageFormat srcFmt = sniffMRFPageFormat(img);
                if (recodeBasePages || !(srcFmt == pageFormat)) {
                    const QByteArray normalized = normalizePage(img);
                    if (normalized.isEmpty()) {
                        undecodablePages++;
                        img.clear();
                    } else {
                        if (srcFmt.isValid() &&
                            ((srcFmt.width != pageFormat.width) || (srcFmt.height != pageFormat.height))) {
                            rescaledPages++;
                        } else {
                            transcodedPages++;
                        }
                        img = normalized;
                    }
                }
            }

            if (!appendPage(img, base.entries[static_cast<qsizetype>(page)])) {
                fail(QStringLiteral("Failed to write MRF data file: %1").arg(datFile.errorString()));
                return result;
            }
            reportProgress();
        }

        if (pagesWritten == 0) {
            fail(QStringLiteral("No usable tiles found at zoom %1 for this set").arg(zoom));
            return result;
        }
    }

            // --- Overview levels: each page is the 2x2 block above it, downsampled. ---------------
            // Composited in ARGB32 regardless of the target format so QPainter always has a format it
            // can paint on; absent parents stay transparent, and collapse to black for formats without
            // an alpha channel.
    for (qsizetype levelIdx = 1; levelIdx < levels.size(); levelIdx++) {
        const MRFLevel &parent = levels.at(levelIdx - 1);
        MRFLevel &level = levels[levelIdx];
        level.entries.resize(static_cast<qsizetype>(level.pagesX * level.pagesY));

        for (quint64 py = 0; py < level.pagesY; py++) {
            for (quint64 px = 0; px < level.pagesX; px++) {
                QImage canvas(pageFormat.width * 2, pageFormat.height * 2, QImage::Format_ARGB32);
                canvas.fill(Qt::transparent);
                bool anyParent = false;

                {
                    QPainter painter(&canvas);
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            const quint64 sx = (px * 2) + static_cast<quint64>(dx);
                            const quint64 sy = (py * 2) + static_cast<quint64>(dy);
                            if ((sx >= parent.pagesX) || (sy >= parent.pagesY)) {
                                continue;
                            }
                            const MRFIndexEntry &src = parent.entries.at(static_cast<qsizetype>((sy * parent.pagesX) + sx));
                            if (src.size == 0) {
                                continue;
                            }
                            if (!datFile.seek(static_cast<qint64>(src.offset))) {
                                continue;
                            }
                            const QImage decoded = QImage::fromData(datFile.read(static_cast<qint64>(src.size)));
                            if (decoded.isNull()) {
                                continue;
                            }
                            painter.drawImage(QPoint(dx * pageFormat.width, dy * pageFormat.height), decoded);
                            anyParent = true;
                        }
                    }
                }

                QByteArray img;
                if (anyParent) {
                    img = encodeMRFPage(canvas.scaled(pageFormat.width, pageFormat.height,
                                                      Qt::IgnoreAspectRatio, Qt::SmoothTransformation),
                                        pageFormat);
                    if (img.isEmpty()) {
                        fail(QStringLiteral("Failed to encode MRF overview page as %1").arg(pageFormat.compression));
                        return result;
                    }
                }

                        // Appends land at the end of the file, which seek() above moved away from.
                if (!datFile.seek(static_cast<qint64>(dataOffset))) {
                    fail(QStringLiteral("Failed to seek MRF data file: %1").arg(datFile.errorString()));
                    return result;
                }
                if (!appendPage(img, level.entries[static_cast<qsizetype>((py * level.pagesX) + px)])) {
                    fail(QStringLiteral("Failed to write MRF data file: %1").arg(datFile.errorString()));
                    return result;
                }
                reportProgress();
            }
        }
    }

            // close() swallows flush errors, so force the buffers out while they can still be reported.
    if (!datFile.flush()) {
        fail(QStringLiteral("Failed to flush MRF data file to disk"));
        return result;
    }
    datFile.close();

    if ((rescaledPages > 0) || (transcodedPages > 0) || (undecodablePages > 0)) {
        qCWarning(QGCTileCacheDatabaseLog) << "MRF export: normalized pages to" << pageFormat.width << "x"
                  << pageFormat.height << pageFormat.compression
                  << pageFormat.channels << "band -" << rescaledPages
                  << "rescaled," << transcodedPages << "transcoded,"
                  << undecodablePages << "undecodable";
    }

            // --- Index: every level's records back to back, finest first. ------------------------
            // 16 bytes per page -- big-endian offset then big-endian length, row-major within a level,
            // (0, 0) marking an absent page. No header and no padding between levels.
    {
        QFile idxFile(idxPath);
        if (!idxFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            fail(QStringLiteral("Failed to create MRF index file"));
            return result;
        }

        QByteArray records;
        records.reserve(static_cast<qsizetype>(totalWork) * 16);
        for (const MRFLevel &level : levels) {
            for (const MRFIndexEntry &entry : level.entries) {
                const quint64 beOffset = qToBigEndian(entry.offset);
                const quint64 beSize = qToBigEndian(entry.size);
                records.append(reinterpret_cast<const char*>(&beOffset), sizeof(beOffset));
                records.append(reinterpret_cast<const char*>(&beSize), sizeof(beSize));
            }
        }

        const bool idxFailed = (idxFile.write(records) != records.size()) || !idxFile.flush();
        idxFile.close();
        if (idxFailed) {
            fail(QStringLiteral("Failed to write MRF index file"));
            return result;
        }
    }

            // Geographic footprint of the tile grid in EPSG:3857 meters. Web-mercator tile y counts
            // southwards from the top of the world, hence the inverted y terms.
    const double tileSizeMeters = (2.0 * kMRFOriginShift) / static_cast<double>(quint64(1) << zoom);
    const double minX = -kMRFOriginShift + (grid.tileX0 * tileSizeMeters);
    const double maxX = -kMRFOriginShift + ((grid.tileX1 + 1) * tileSizeMeters);
    const double maxY = kMRFOriginShift - (grid.tileY0 * tileSizeMeters);
    const double minY = kMRFOriginShift - ((grid.tileY1 + 1) * tileSizeMeters);

    // --- Header --------------------------------------------------------------------------
    {
        QString header;
        header += QStringLiteral("<MRF_META>\n");
        header += QStringLiteral("  <Raster>\n");
        header += QStringLiteral("    <Size x=\"%1\" y=\"%2\" c=\"%3\" />\n")
                      .arg(gridWidth * static_cast<quint64>(pageFormat.width))
                      .arg(gridHeight * static_cast<quint64>(pageFormat.height))
                      .arg(pageFormat.channels);
        header += QStringLiteral("    <PageSize x=\"%1\" y=\"%2\" c=\"%3\" />\n")
                      .arg(pageFormat.width)
                      .arg(pageFormat.height)
                      .arg(pageFormat.channels);
        header += QStringLiteral("    <Compression>%1</Compression>\n").arg(pageFormat.compression);
        if (pageFormat.compression == QLatin1String("JPEG")) {
            header += QStringLiteral("    <Quality>%1</Quality>\n").arg(kMRFJpegQuality);
        }
        header += QStringLiteral("  </Raster>\n");

        header += QStringLiteral("  <GeoTags>\n");
        header += QStringLiteral("    <BoundingBox minx=\"%1\" miny=\"%2\" maxx=\"%3\" maxy=\"%4\" />\n")
                      .arg(minX, 0, 'f', 8)
                      .arg(minY, 0, 'f', 8)
                      .arg(maxX, 0, 'f', 8)
                      .arg(maxY, 0, 'f', 8);
        header += QStringLiteral("    <Projection>%1</Projection>\n").arg(QLatin1String(kMRFWebMercatorWkt));
        header += QStringLiteral("  </GeoTags>\n");

                // Declares the overview levels written above. Omitted when the raster is a single page.
        if (levels.size() > 1) {
            header += QStringLiteral("  <Rsets model=\"uniform\" scale=\"2\" />\n");
        }

                // <DataFile>/<IndexFile> are intentionally absent: GDAL derives both from the basename.

        header += QStringLiteral("</MRF_META>\n");

        QFile mrfFile(mrfPath);
        if (!mrfFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            fail(QStringLiteral("Failed to create MRF metadata file"));
            return result;
        }
        const QByteArray headerBytes = header.toUtf8();
        const bool headerFailed = (mrfFile.write(headerBytes) != headerBytes.size()) || !mrfFile.flush();
        mrfFile.close();
        if (headerFailed) {
            fail(QStringLiteral("Failed to write MRF metadata file"));
            return result;
        }
    }

            // --- bounds.json ---------------------------------------------------------------------
            // mrf_bounds is the tile-aligned raster footprint reprojected to WGS84; requested_bounds is
            // what the tile set was defined with. coverage_pct counts only the finest level, since the
            // overviews are derived from it.
    {
        QJsonObject root;
        root[QStringLiteral("coverage_pct")] = mrfRound7((static_cast<double>(qMin(pagesWritten, totalPages)) /
                                                          static_cast<double>(totalPages)) * 100.0);

        QJsonObject expansion;
        for (const char *key : {"east_m", "north_m", "south_m", "west_m"}) {
            expansion[QLatin1String(key)] = 0.0;
        }
        root[QStringLiteral("expansion_m")] = expansion;

        QGeoCoordinate requestMinCoord(qMin(set.topleftLat, set.bottomRightLat),
                                       qMin(set.topleftLon, set.bottomRightLon));
        QGeoCoordinate requestMaxCoord(qMax(set.topleftLat, set.bottomRightLat),
                                       qMax(set.topleftLon, set.bottomRightLon));

        const QJsonObject requested = mrfBoundsObject(requestMinCoord,
                                                      requestMaxCoord);
        root[QStringLiteral("mcap_bounds")] = requested;
        root[QStringLiteral("requested_bounds")] = requested;
        QGeoCoordinate minCoord = mrfToCoord({qFloor(minX),qFloor(minY)}),
                       maxCoord = mrfToCoord({qCeil(maxX),qCeil(maxY)});
        root[QStringLiteral("mrf_bounds")] = mrfBoundsObject(minCoord,
                                                             maxCoord);

        QFile boundsFile(boundsPath);
        if (!boundsFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            fail(QStringLiteral("Failed to create bounds.json"));
            return result;
        }
        const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
        const bool jsonFailed = (boundsFile.write(json) != json.size()) || !boundsFile.flush();
        boundsFile.close();
        if (jsonFailed) {
            fail(QStringLiteral("Failed to write bounds.json"));
            return result;
        }
    }

    qCDebug(QGCTileCacheDatabaseLog) << "MRF export:" << mrfPath << gridWidth << "x" << gridHeight
                                     << "pages at zoom" << zoom << "-" << levels.size() << "levels,"
                                     << totalWork << "index records," << pagesWritten << "pages written";
    result.success = true;
    return result;
}

bool QGCTileCacheDatabase::_createDB(QSqlDatabase db, bool createDefault)
{
    // applySqlitePragmas (in connectDB / ScopedConnection ctor) already
    // enabled foreign_keys; nothing to redo here.
    QSqlQuery query(db);

    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS Tiles ("
        "tileID INTEGER PRIMARY KEY NOT NULL, "
        "hash TEXT NOT NULL UNIQUE, "
        "format TEXT NOT NULL, "
        "tile BLOB NULL, "
        "size INTEGER, "
        "type INTEGER, "
        "date INTEGER DEFAULT 0)"))
    {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create Tiles db):" << query.lastError().text();
        return false;
    }

    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS TileSets ("
        "setID INTEGER PRIMARY KEY NOT NULL, "
        "name TEXT NOT NULL UNIQUE, "
        "typeStr TEXT, "
        "topleftLat REAL DEFAULT 0.0, "
        "topleftLon REAL DEFAULT 0.0, "
        "bottomRightLat REAL DEFAULT 0.0, "
        "bottomRightLon REAL DEFAULT 0.0, "
        "minZoom INTEGER DEFAULT 3, "
        "maxZoom INTEGER DEFAULT 3, "
        "type INTEGER DEFAULT -1, "
        "numTiles INTEGER DEFAULT 0, "
        "defaultSet INTEGER DEFAULT 0, "
        "date INTEGER DEFAULT 0)"))
    {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create TileSets db):" << query.lastError().text();
        return false;
    }

    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS SetTiles ("
        "setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
        "tileID INTEGER NOT NULL REFERENCES Tiles(tileID) ON DELETE CASCADE)"))
    {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create SetTiles db):" << query.lastError().text();
        return false;
    }

    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS TilesDownload ("
        "setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
        "hash TEXT NOT NULL, "
        "type INTEGER, "
        "x INTEGER, "
        "y INTEGER, "
        "z INTEGER, "
        "state INTEGER DEFAULT 0)"))
    {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create TilesDownload db):" << query.lastError().text();
        return false;
    }

    static const char *indexStatements[] = {
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_settiles_unique ON SetTiles(tileID, setID)",
        "CREATE INDEX IF NOT EXISTS idx_settiles_setid ON SetTiles(setID)",
        "CREATE INDEX IF NOT EXISTS idx_settiles_tileid ON SetTiles(tileID)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_tilesdownload_setid_hash ON TilesDownload(setID, hash)",
        "CREATE INDEX IF NOT EXISTS idx_tilesdownload_setid_state ON TilesDownload(setID, state)",
        "CREATE INDEX IF NOT EXISTS idx_tiles_date ON Tiles(date)",
    };
    for (const char *sql : indexStatements) {
        if (!query.exec(QLatin1String(sql))) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to create index:" << sql << query.lastError().text();
        }
    }

    if (!QGCSqlHelper::setUserVersion(db, kSchemaVersion)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to set schema version";
    }

    if (!createDefault) {
        return true;
    }

    if (!query.prepare("SELECT name FROM TileSets WHERE name = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare default set check):" << db.lastError();
        return false;
    }
    query.addBindValue(QStringLiteral("Default Tile Set"));
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (Looking for default tile set):" << db.lastError();
        return true;
    }
    if (query.next()) {
        return true;
    }

    if (!query.prepare("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare default tile set):" << db.lastError();
        return false;
    }
    query.addBindValue(QStringLiteral("Default Tile Set"));
    query.addBindValue(1);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (Creating default tile set):" << db.lastError();
        return false;
    }

    return true;
}

quint64 QGCTileCacheDatabase::_getDefaultTileSet()
{
    if (_defaultSet != kInvalidTileSet) {
        return _defaultSet;
    }

    if (!_ensureConnected()) {
        return kInvalidTileSet;
    }

    QSqlQuery query(_database());
    if (query.exec("SELECT setID FROM TileSets WHERE defaultSet = 1") && query.next()) {
        _defaultSet = query.value(0).toULongLong();
        return _defaultSet;
    }

    qCWarning(QGCTileCacheDatabaseLog) << "Default tile set not found in database";
    return kInvalidTileSet;
}

bool QGCTileCacheDatabase::_deleteTilesByIDs(const QList<quint64> &ids)
{
    if (ids.isEmpty()) {
        return true;
    }

    QSqlQuery query(_database());
    if (!query.prepare(QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(QGCSqlHelper::placeholders(ids.size())))) {
        return false;
    }
    for (const quint64 id : ids) {
        query.addBindValue(id);
    }
    return query.exec();
}

QString QGCTileCacheDatabase::_deduplicateSetName(const QString &name)
{
    if (!findTileSetID(name).has_value()) {
        return name;
    }

    QSet<QString> existing;
    existing.insert(name);
    QSqlQuery query(_database());
    QString escaped = name;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('%'), QStringLiteral("\\%"));
    escaped.replace(QLatin1Char('_'), QStringLiteral("\\_"));
    if (query.prepare(QStringLiteral("SELECT name FROM TileSets WHERE name LIKE ? || ' %' ESCAPE '\\'"))) {
        query.addBindValue(escaped);
        if (query.exec()) {
            while (query.next()) {
                existing.insert(query.value(0).toString());
            }
        }
    }

    for (int i = 1; i <= 9999; i++) {
        const QString candidate = QStringLiteral("%1 %2").arg(name).arg(i, 4, 10, QChar('0'));
        if (!existing.contains(candidate)) {
            return candidate;
        }
    }

    return QStringLiteral("%1 %2").arg(name, QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

quint64 QGCTileCacheDatabase::_copyTilesForSet(QSqlDatabase srcDB, quint64 srcSetID, quint64 dstSetID,
                                                 quint64 &currentCount, quint64 tileCount,
                                                 int &lastProgress, ProgressCallback progressCb,
                                                 quint64 *tilesIteratedOut, bool useTransaction)
{
    QSqlQuery subQuery(srcDB);
    subQuery.setForwardOnly(true);
    if (!subQuery.prepare("SELECT T.hash, T.format, T.tile, T.type, T.date FROM Tiles T "
                          "INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?")) {
        if (tilesIteratedOut) *tilesIteratedOut = 0;
        return 0;
    }
    subQuery.addBindValue(srcSetID);
    if (!subQuery.exec()) {
        if (tilesIteratedOut) *tilesIteratedOut = 0;
        return 0;
    }

    quint64 tilesFound = 0;
    quint64 tilesLinked = 0;

    std::unique_ptr<QGCSqlHelper::Transaction> txn;
    if (useTransaction) {
        txn = std::make_unique<QGCSqlHelper::Transaction>(_database());
        if (!txn->ok()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for merge import";
            if (tilesIteratedOut) *tilesIteratedOut = 0;
            return 0;
        }
    }

    QSqlQuery cQuery(_database());
    while (subQuery.next()) {
        tilesFound++;
        const QString hash = subQuery.value(0).toString();
        const QString format = subQuery.value(1).toString();
        const QByteArray img = subQuery.value(2).toByteArray();
        const int tileType = subQuery.value(3).toInt();
        const quint64 tileDate = subQuery.value(4).toULongLong();

        quint64 importTileID = 0;
        if (cQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)")) {
            cQuery.addBindValue(hash);
            cQuery.addBindValue(format);
            cQuery.addBindValue(img);
            cQuery.addBindValue(img.size());
            cQuery.addBindValue(tileType);
            cQuery.addBindValue(tileDate);
            if (cQuery.exec()) {
                importTileID = cQuery.lastInsertId().toULongLong();
            } else {
                if (cQuery.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
                    cQuery.addBindValue(hash);
                    if (cQuery.exec() && cQuery.next()) {
                        importTileID = cQuery.value(0).toULongLong();
                    }
                }
            }
        }

        if (importTileID > 0) {
            if (cQuery.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
                cQuery.addBindValue(importTileID);
                cQuery.addBindValue(dstSetID);
                if (cQuery.exec() && cQuery.numRowsAffected() > 0) {
                    tilesLinked++;
                }
            }
        }

        currentCount++;
        if (tileCount > 0 && progressCb) {
            const int progress = qMin(100, static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0));
            if (lastProgress != progress) {
                lastProgress = progress;
                progressCb(progress);
            }
        }
    }

    if (txn && !txn->commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit merge import transaction";
        if (tilesIteratedOut) *tilesIteratedOut = tilesFound;
        return 0;
    }

    if (tilesIteratedOut) *tilesIteratedOut = tilesFound;
    return tilesLinked;
}
