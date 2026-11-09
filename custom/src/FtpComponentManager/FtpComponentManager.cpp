#include "FtpComponentManager.h"
#include "Vehicle.h"
#include "FTPManager.h"
#include "FTPController.h"

FtpComponentManager::FtpComponentManager(Vehicle *vehicle, QObject *parent) :
                                    QObject(parent),
                                    _vehicle(vehicle)
{
    FTPManager *mgr = _vehicle->ftpManager();
    connect(mgr, &FTPManager::uploadComplete,this,[=](){qDebug() << "uploaded!";});
}

bool FtpComponentManager::canTransfer() const
{
    return _ftpAvailable;
}

void FtpComponentManager::upload(const QString &file)
{
    QUrl fileUrl(file);
    QString uploadPath = QString("%1").arg(fileUrl.fileName());
    _vehicle->ftpManager()->upload(_targetComtonent, uploadPath, file);
}

void FtpComponentManager::download(const QString &/*file*/)
{

}


void FtpComponentManager::setTargetComponent(int componentId)
{
    qDebug() << "updating target cmponent----------------------------";
    if (componentId == _targetComtonent) {
        return;
    }
    _targetComtonent = componentId;

    if (_targetComtonent != MAV_COMP_ID_ALL) {
        emit canTransferChanged();
    }
}
