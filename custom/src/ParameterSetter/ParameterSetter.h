#pragma once 

#include <QtCore/QObject>
#include "ParameterManager.h"
class ParameterSetter: public QObject{
    Q_OBJECT
public:
    ParameterSetter(QObject* parent =nullptr):QObject(parent){};
    Q_INVOKABLE QString getParameter(int compId,QString paramName, bool report = true);
    Q_INVOKABLE Fact* getFact(int compId, QString paramName, bool report = true);
    Q_INVOKABLE void setParameter(int compId,QString paramName,float value);
};
