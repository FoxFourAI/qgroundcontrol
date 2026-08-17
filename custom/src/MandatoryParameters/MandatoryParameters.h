#pragma once

#include "QObject"




class Vehicle;
class MandatoryParameters : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY parametersChanged FINAL)
    Q_PROPERTY(bool showList MEMBER _showList NOTIFY showListChanged)
public:

    enum ComponentType {
        FCU = 0,
        VGM,
        Unknown
    };
    QStringList componentNames{
        "FCU",
        "VGM"
    };

    MandatoryParameters(QObject* parent);
    ~MandatoryParameters();
    const QVariantMap parameters();
    const QMap<ComponentType,QStringList>& rawParameters();
    Q_INVOKABLE ComponentType componentType(const QString componentName);
    Q_INVOKABLE void toggleParameter(const QString& parameter, const int componentId);
    Q_INVOKABLE void removeParameter(const QString& parameter);
    Q_INVOKABLE void loadDefaultParameters();
    Q_INVOKABLE bool isMandatory(const QString parameterName);
signals:
    void parametersChanged();
    void showListChanged();
private:
    void _saveParameters();
    void _loadParameters();

private:
    QMap<ComponentType,QStringList> _parameters;
    bool _parametersReady = false;
    static const QString _groupKey;
    bool _showList = false;
};
