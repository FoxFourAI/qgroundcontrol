#pragma once

#include <QtCore/QObject>
#include "CopterType.h"

class CopterConfigurator: public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<CopterType*> copterTypes READ copterTypes NOTIFY copterTypesChanged FINAL)
    Q_PROPERTY(CopterType* currentType READ currentType NOTIFY currentTypeChanged FINAL)
public:

    Q_INVOKABLE void refresh();
    CopterConfigurator(Vehicle* vehicle, QObject* parent=nullptr);
    QList<CopterType*> copterTypes() const;
    CopterType *currentType() const;
    void handleVGMReboot();
signals:
    void copterTypesChanged();
    void currentTypeChanged();
private:
    void _currentTypeChangedCallback();
    void _init();

private:
    static const QString _frameTypeFact;
    static const QString _missnFact;
    int _componentId = 0;
    Vehicle *_vehicle =nullptr;
    QList<CopterType *> _types;
    CopterType* _currentType = nullptr;
};
