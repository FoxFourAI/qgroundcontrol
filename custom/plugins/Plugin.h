#pragma once
#include <QObject>
#include "QQmlApplicationEngine"

class Plugin: public QObject {
    Q_OBJECT
public:
    Plugin(QObject *parent = nullptr): QObject(parent) { }
    virtual void init() = 0;
    virtual QString name() = 0;
    virtual void cleanup() = 0;
    virtual void attachToQmlEngine([[maybe_unused]] QQmlApplicationEngine* engine) {return;}
    virtual QUrl resourceIntercept(const QUrl& url,[[maybe_unused]] QQmlAbstractUrlInterceptor::DataType type) {return url;}

};
