#pragma once

#include <QtCore/QObject>


class CopterType {
    CopterType();
private:
    QString name,description;
};

class CopterConfigurator : public QObject {
    Q_OBJECT
public:
    CopterConfigurator();
};
