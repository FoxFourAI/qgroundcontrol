#pragma once

#include "QObject"




class Vehicle;
class MandatoryParameters : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY parametersChanged FINAL)
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
    Q_INVOKABLE void removeParameter(const QString& parameter, const QString component);
    Q_INVOKABLE void addParameter(const QString& parameter, const int componentId);
    Q_INVOKABLE void loadDefaultParameters();
signals:
    void parametersChanged();
private:
    void _saveParameters();
    void _loadParameters();

private:
    QMap<ComponentType,QStringList> _parameters;
    bool _parametersReady = false;
    static constexpr std::string_view _groupKey = "mandatoryParams";
};
