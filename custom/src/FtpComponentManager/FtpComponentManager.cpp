#include "FtpComponentManager.h"
#include "Vehicle.h"
#include "FTPManager.h"
#include "FTPController.h"
#include "AppMessages.h"

FtpComponentManager::FtpComponentManager(Vehicle *vehicle, QObject *parent) :
                                    QObject(parent),
                                    _vehicle(vehicle)
{
    FTPManager *mgr = _vehicle->ftpManager();
    connect(mgr, &FTPManager::downloadComplete, this, [=](const QString& file, const QString& errorMsg){qDebug() << "downloaded: " << file << " error:" << errorMsg;});
    connect(mgr, &FTPManager::uploadComplete, this, &FtpComponentManager::uploadFinishHandle);
    connect(mgr, &FTPManager::deleteComplete, this, [=](const QString& file, const QString& errorMsg){qDebug() << "deleted: " << file << " error:" << errorMsg;});
    connect(mgr, &FTPManager::listDirectoryComplete, this, [=](const QStringList& dirList, const QString& errorMsg){qDebug() << "list available: " << dirList << "and error is: " << errorMsg;});
    connect(mgr, &FTPManager::commandProgress, this, [=](float progress){qDebug() << "ftp progress " << progress * 100;});
}

bool FtpComponentManager::canTransfer() const
{
    return _ftpAvailable;
}

void FtpComponentManager::upload(const QString &file)
{
    QFileInfo fileInfo(file);
    _vehicle->ftpManager()->upload(_targetComtonent, fileInfo.fileName(), file);
}

void FtpComponentManager::download(const QString &from, const QString &to)
{
    QFileInfo fileInfo(to);
    _vehicle->ftpManager()->download(_targetComtonent, from, fileInfo.path(), fileInfo.fileName());
}

void FtpComponentManager::remove(const QString &/*file*/)
{

}

void FtpComponentManager::updateList(const QString &path)
{
    _vehicle->ftpManager()->listDirectory(_targetComtonent, path);
}


void FtpComponentManager::setTargetComponent(int componentId)
{
    if (componentId == _targetComtonent) {
        return;
    }
    _targetComtonent = componentId;
    bool compIdCorrenct = _targetComtonent != MAV_COMP_ID_ALL;
    if (compIdCorrenct != _ftpAvailable)  {
        _ftpAvailable = compIdCorrenct;
        emit canTransferChanged();
    }

}

void FtpComponentManager::uploadFinishHandle(const QString &file, const QString &errorMsg)
{

    if (!errorMsg.isEmpty()) {
        QGC::showCriticalVehicleMessage(errorMsg);
    } else {
        QGC::showAppMessage("Upload succeed. File " + file + " was uploaded to vehicle.", "Ftp Upload");
    }
}
