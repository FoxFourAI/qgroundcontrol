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
#include <QtCore/QSet>
#include <QtCore/QtEndian>
#include <QtCore/QXmlStreamWriter>
#include <algorithm>

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
// How many tiles to sample when deciding the pyramid's single page format.
constexpr int kMRFFormatSampleLimit = 64;

// GDAL/PROJ's canonical WKT for EPSG:3857, embedded verbatim so exported MRFs are
// georeferenced without depending on a PROJ database being available at export time.
constexpr const char *kMRFWebMercatorWkt =
    "PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\","
    "SPHEROID[\"WGS 84\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],"
    "UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Mercator_1SP\"],"
    "PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],"
    "PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],"
    "UNIT[\"metre\",1],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH]]";

struct MRFPageFormat
{
    QString compression;
    int channels = 0;

    bool isValid() const { return (channels > 0) && !compression.isEmpty(); }
    bool operator==(const MRFPageFormat &other) const
    {
        return (channels == other.channels) && (compression == other.compression);
    }
};

/// Reads the band count out of the encoded tile header rather than trusting the Tiles.format
/// column or a QImage round-trip. MRF stores the original PNG/JPEG bytes verbatim and GDAL
/// decodes them with libpng/libjpeg, so <Size c=".."> has to match the codec's own channel
/// count -- a palette PNG decodes to one band plus a colour table, not to three.
MRFPageFormat sniffMRFPageFormat(const QByteArray &img)
{
    static const QByteArray pngSignature = QByteArrayLiteral("\x89PNG\r\n\x1a\n");

    MRFPageFormat fmt;

    // IHDR is the mandatory first chunk, so its colour type byte sits at a fixed offset.
    if (img.startsWith(pngSignature) && (img.size() > 25)) {
        switch (static_cast<quint8>(img.at(25))) {
        case 0: fmt.channels = 1; break; // greyscale
        case 2: fmt.channels = 3; break; // RGB
        case 3: fmt.channels = 1; break; // palette (colour table travels with the page)
        case 4: fmt.channels = 2; break; // greyscale + alpha
        case 6: fmt.channels = 4; break; // RGBA
        default: return MRFPageFormat();
        }
        fmt.compression = QStringLiteral("PNG");
        return fmt;
    }

    if ((img.size() < 4) || (static_cast<quint8>(img.at(0)) != 0xFF) || (static_cast<quint8>(img.at(1)) != 0xD8)) {
        return MRFPageFormat();
    }

    // Walk the JPEG marker segments to the Start-Of-Frame, whose payload carries the
    // component count (1 = greyscale, 3 = YCbCr).
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
            if ((fmt.channels != 1) && (fmt.channels != 3)) {
                return MRFPageFormat();
            }
            fmt.compression = QStringLiteral("JPEG");
            return fmt;
        }

        pos += 2 + segLen;
    }

    return MRFPageFormat();
}

bool writeMRFIndexEntry(QFile &idxFile, quint64 offset, quint64 size)
{
    const quint64 beOffset = qToBigEndian(offset);
    const quint64 beSize = qToBigEndian(size);
    return (idxFile.write(reinterpret_cast<const char*>(&beOffset), sizeof(beOffset)) == sizeof(beOffset)) &&
           (idxFile.write(reinterpret_cast<const char*>(&beSize), sizeof(beSize)) == sizeof(beSize));
}

} // namespace

DatabaseResult QGCTileCacheDatabase::exportSetAsMRF(const TileSetRecord &set, const QString &basePath, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (!_ensureConnected()) {
        result.errorString = QStringLiteral("Database not connected");
        return result;
    }

    const QString type = UrlFactory::getProviderTypeFromQtMapId(set.type);
    if (type.isEmpty()) {
        result.errorString = QStringLiteral("Unknown map provider for this tile set");
        return result;
    }

    // Elevation providers use their own flat degree grid (and count y northwards), so the
    // web-mercator geometry below does not describe them at all.
    if (UrlFactory::isElevation(set.type)) {
        result.errorString = QStringLiteral("Elevation tile sets cannot be exported as MRF");
        return result;
    }

    // MRF describes one raster of a fixed extent, and only the finest cached zoom maps onto
    // such a raster: the set's lower zooms are crops of world tiles covering far more ground
    // than this set, not downsampled versions of it. So a single level is written here and
    // overviews are left to `gdaladdo`, which resamples them properly.
    const int zoom = set.maxZoom;
    const QGCTileSet grid = UrlFactory::getTileCount(zoom, set.topleftLon, set.topleftLat,
                                                      set.bottomRightLon, set.bottomRightLat, type);
    if ((grid.tileX1 < grid.tileX0) || (grid.tileY1 < grid.tileY0)) {
        result.errorString = QStringLiteral("Tile set covers no tiles at zoom %1").arg(zoom);
        return result;
    }

    const quint64 gridWidth = static_cast<quint64>(grid.tileX1 - grid.tileX0 + 1);
    const quint64 gridHeight = static_cast<quint64>(grid.tileY1 - grid.tileY0 + 1);
    const quint64 totalPages = gridWidth * gridHeight;

    QSqlQuery tileQuery(_database());
    if (!tileQuery.prepare("SELECT tile FROM Tiles WHERE hash = ?")) {
        result.errorString = QStringLiteral("Failed to prepare tile lookup for MRF export");
        return result;
    }

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

            tileQuery.addBindValue(UrlFactory::getTileHash(type, x, y, zoom));
            if (!tileQuery.exec() || !tileQuery.next()) {
                continue;
            }
            const MRFPageFormat fmt = sniffMRFPageFormat(tileQuery.value(0).toByteArray());
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

        int best = 0;
        for (const auto &vote : votes) {
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

    const QString mrfPath = basePath + QStringLiteral(".mrf");
    const QString idxPath = basePath + QStringLiteral(".idx");
    const QString datPath = basePath + QStringLiteral(".dat");

    // Any of the three files left behind from a previous run would be read alongside a new
    // one, so clear all three up front and again on every failure path below.
    auto discardOutputs = [&]() {
        (void) QFile::remove(mrfPath);
        (void) QFile::remove(idxPath);
        (void) QFile::remove(datPath);
    };
    discardOutputs();

    QFile idxFile(idxPath);
    QFile datFile(datPath);
    if (!idxFile.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        !datFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.errorString = QStringLiteral("Failed to create MRF data files");
        discardOutputs();
        return result;
    }

    quint64 dataOffset = 0;
    quint64 pagesWritten = 0;
    quint64 skippedMismatched = 0;
    int lastProgress = -1;

    // Index records run in row-major page order for the single level, 16 bytes each:
    // big-endian offset then big-endian length, with (0, 0) marking an absent page.
    for (quint64 page = 0; page < totalPages; page++) {
        const int x = grid.tileX0 + static_cast<int>(page % gridWidth);
        const int y = grid.tileY0 + static_cast<int>(page / gridWidth);

        QByteArray img;
        tileQuery.addBindValue(UrlFactory::getTileHash(type, x, y, zoom));
        if (tileQuery.exec() && tileQuery.next()) {
            img = tileQuery.value(0).toByteArray();
            if (!(sniffMRFPageFormat(img) == pageFormat)) {
                skippedMismatched++;
                img.clear();
            }
        }

        bool ok = true;
        if (img.isEmpty()) {
            ok = writeMRFIndexEntry(idxFile, 0, 0);
        } else if (datFile.write(img) != img.size()) {
            ok = false;
        } else {
            ok = writeMRFIndexEntry(idxFile, dataOffset, static_cast<quint64>(img.size()));
            dataOffset += static_cast<quint64>(img.size());
            pagesWritten++;
        }

        if (!ok) {
            result.errorString = QStringLiteral("Failed to write MRF output: %1")
                                     .arg(idxFile.error() != QFileDevice::NoError ? idxFile.errorString() : datFile.errorString());
            discardOutputs();
            return result;
        }

        if (progressCb) {
            const int progress = qMin(100, static_cast<int>((static_cast<double>(page + 1) / static_cast<double>(totalPages)) * 100.0));
            if (lastProgress != progress) {
                lastProgress = progress;
                progressCb(progress);
            }
        }
    }

    // close() swallows flush errors, so force the buffers out while they can still be reported.
    if (!idxFile.flush() || !datFile.flush()) {
        result.errorString = QStringLiteral("Failed to flush MRF output to disk");
        discardOutputs();
        return result;
    }
    idxFile.close();
    datFile.close();

    if (pagesWritten == 0) {
        result.errorString = QStringLiteral("No cached tiles found at zoom %1 for this set").arg(zoom);
        discardOutputs();
        return result;
    }
    if (skippedMismatched > 0) {
        qCWarning(QGCTileCacheDatabaseLog) << "MRF export: skipped" << skippedMismatched
                                            << "tiles that did not match the file's" << pageFormat.compression
                                            << pageFormat.channels << "band format";
    }

    // Geographic footprint of the tile grid in EPSG:3857 meters. Web-mercator tile y counts
    // southwards from the top of the world, hence the inverted y terms.
    const double tileSizeMeters = (2.0 * kMRFOriginShift) / static_cast<double>(quint64(1) << zoom);
    const double minX = -kMRFOriginShift + (grid.tileX0 * tileSizeMeters);
    const double maxX = -kMRFOriginShift + ((grid.tileX1 + 1) * tileSizeMeters);
    const double maxY = kMRFOriginShift - (grid.tileY0 * tileSizeMeters);
    const double minY = kMRFOriginShift - ((grid.tileY1 + 1) * tileSizeMeters);

    QFile mrfFile(mrfPath);
    if (!mrfFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.errorString = QStringLiteral("Failed to create MRF metadata file");
        discardOutputs();
        return result;
    }
    qDebug() << "writing to " << mrfFile.fileName();
    QXmlStreamWriter xml(&mrfFile);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("MRF_META"));

    xml.writeStartElement(QStringLiteral("Raster"));
    xml.writeStartElement(QStringLiteral("Size"));
    xml.writeAttribute(QStringLiteral("x"), QString::number(gridWidth * kMRFTileSize));
    xml.writeAttribute(QStringLiteral("y"), QString::number(gridHeight * kMRFTileSize));
    xml.writeAttribute(QStringLiteral("c"), QString::number(pageFormat.channels));
    xml.writeEndElement(); // Size
    xml.writeStartElement(QStringLiteral("PageSize"));
    xml.writeAttribute(QStringLiteral("x"), QString::number(kMRFTileSize));
    xml.writeAttribute(QStringLiteral("y"), QString::number(kMRFTileSize));
    xml.writeAttribute(QStringLiteral("c"), QString::number(pageFormat.channels));
    xml.writeEndElement(); // PageSize
    xml.writeTextElement(QStringLiteral("Compression"), pageFormat.compression);
    xml.writeTextElement(QStringLiteral("DataType"), QStringLiteral("Byte"));
    xml.writeEndElement(); // Raster

    // No <Rsets>: this file has a single level. Overviews come from `gdaladdo <file>.mrf`.

    xml.writeStartElement(QStringLiteral("GeoTags"));
    xml.writeStartElement(QStringLiteral("BoundingBox"));
    xml.writeAttribute(QStringLiteral("minx"), QString::number(minX, 'f', 6));
    xml.writeAttribute(QStringLiteral("miny"), QString::number(minY, 'f', 6));
    xml.writeAttribute(QStringLiteral("maxx"), QString::number(maxX, 'f', 6));
    xml.writeAttribute(QStringLiteral("maxy"), QString::number(maxY, 'f', 6));
    xml.writeEndElement(); // BoundingBox
    xml.writeTextElement(QStringLiteral("Projection"), QLatin1String(kMRFWebMercatorWkt));
    xml.writeEndElement(); // GeoTags

    xml.writeTextElement(QStringLiteral("DataFile"), QFileInfo(datPath).fileName());
    xml.writeTextElement(QStringLiteral("IndexFile"), QFileInfo(idxPath).fileName());

    xml.writeEndElement(); // MRF_META
    xml.writeEndDocument();

    const bool xmlFailed = xml.hasError() || !mrfFile.flush();
    mrfFile.close();
    if (xmlFailed) {
        result.errorString = QStringLiteral("Failed to write MRF metadata file");
        discardOutputs();
        return result;
    }

    qCDebug(QGCTileCacheDatabaseLog) << "MRF export:" << mrfPath << gridWidth << "x" << gridHeight
                                      << "pages at zoom" << zoom << "-" << pagesWritten << "written";
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
