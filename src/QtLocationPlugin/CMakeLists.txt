# ============================================================================
# Qt Location Plugin Module
# Custom map tile providers and caching engine
# ============================================================================

qt_add_plugin(QGCLocation
    STATIC
    CLASS_NAME QGeoServiceProviderFactoryQGC
    OUTPUT_TARGETS QGCLocation_targets
    Providers/BingMapProvider.cpp
    Providers/BingMapProvider.h
    Providers/ElevationMapProvider.cpp
    Providers/ElevationMapProvider.h
    Providers/EsriMapProvider.cpp
    Providers/EsriMapProvider.h
    Providers/GenericMapProvider.cpp
    Providers/GenericMapProvider.h
    Providers/GoogleMapProvider.cpp
    Providers/GoogleMapProvider.h
    Providers/MapboxMapProvider.cpp
    Providers/MapboxMapProvider.h
    Providers/MapProvider.cpp
    Providers/MapProvider.h
    Providers/TianDiTuProvider.cpp
    Providers/TianDiTuProvider.h
    QGCCachedTileSet.cpp
    QGCCachedTileSet.h
    QGCCacheTile.h
    QGCMapEngine.cpp
    QGCMapEngine.h
    QGCMapEngineManager.cc
    QGCMapEngineManager.h
    QGCMapTaskBase.h
    QGCMapTasks.cpp
    QGCMapTasks.h
    QGCMapUrlEngine.cpp
    QGCMapUrlEngine.h
    QGCTile.h
    QGCTileCacheDatabase.cpp
    QGCTileCacheDatabase.h
    QGCTileCacheTypes.h
    QGCTileCacheWorker.cpp
    QGCTileCacheWorker.h
    QGCTileSet.h
    QGeoFileTileCacheQGC.cpp
    QGeoFileTileCacheQGC.h
    QGeoMapReplyQGC.cpp
    QGeoMapReplyQGC.h
    QGeoServiceProviderPluginQGC.cpp
    QGeoServiceProviderPluginQGC.h
    QGeoTiledMappingManagerEngineQGC.cpp
    QGeoTiledMappingManagerEngineQGC.h
    QGeoTiledMapQGC.cpp
    QGeoTiledMapQGC.h
    QGeoTileFetcherQGC.cpp
    QGeoTileFetcherQGC.h
    #FoxFour part
    MrfExport/MrfExport.h
    MrfExport/MrfExport.cpp
)

if(IOS)
    target_compile_definitions(QGCLocation PRIVATE QGC_NO_GOOGLE_MAPS)
endif()

#FoxFour part --- Find GDAL cross-platform fallback logic ---
# First, look for the modern Config mode provided by up-to-date GDAL installations
find_package(GDAL CONFIG QUIET)

if(NOT GDAL_FOUND)
    # Fallback to the classic built-in CMake find module (standard on older Linux setups)
    find_package(GDAL REQUIRED)
endif()

# Create a unified target wrapper if the modern target 'GDAL::GDAL' isn't explicitly defined
if(NOT TARGET GDAL::GDAL)
    add_library(GDAL::GDAL INTERFACE IMPORTED)
    target_include_directories(GDAL::GDAL INTERFACE ${GDAL_INCLUDE_DIR})
    target_link_libraries(GDAL::GDAL INTERFACE ${GDAL_LIBRARY})
endif()


target_link_libraries(QGCLocation
    PRIVATE
        Qt6::Positioning
        QGCCompression
        QGCDatabase
        QGCFileSystem
        QGCFormat
        QGCLogging
        QGCNetwork
    PUBLIC
        Qt6::Core
        Qt6::Location
        Qt6::LocationPrivate
        Qt6::Network
        #FoxFour part
        GDAL::GDAL
)

target_include_directories(QGCLocation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Providers
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../FactSystem
        ${CMAKE_CURRENT_SOURCE_DIR}/../QmlControls
        ${CMAKE_CURRENT_SOURCE_DIR}/../Terrain
        ${CMAKE_CURRENT_SOURCE_DIR}/../Terrain/Providers
        ${CMAKE_CURRENT_SOURCE_DIR}/../Settings
        ${CMAKE_CURRENT_SOURCE_DIR}/../Utilities
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCLocation)

target_precompile_headers(QGCLocation PRIVATE
    <QtCore/QDir>
    <QtCore/QLoggingCategory>
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QStringList>
    <QtCore/QVariant>
    <QtNetwork/QNetworkRequest>
    <QtPositioning/QGeoCoordinate>
    <QtQml/QQmlEngine>
)

qt_add_resources(${CMAKE_PROJECT_NAME} qtlocationplugin_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/notile.png"
)
