#pragma once

#include "QObject"

class Vehicle;
class MandatoryParameters : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList parameters READ parameters NOTIFY parametersChanged FINAL)
public:
    MandatoryParameters(QObject* parent);
    ~MandatoryParameters();
    const QStringList& parameters();
    Q_INVOKABLE void removeParameter(const QString& parameter);
    Q_INVOKABLE void addParameter(const QString& parameter);
    Q_INVOKABLE void loadDefaultParameters();
    void setParametersReady(bool ready);
signals:
    void parametersChanged();
    void parametersReadyChanged(bool ready);

private:
    void _saveParameters();
    void _loadParameters();

private:
    QStringList _parameters;
    bool _parametersReady = false;
    static constexpr std::string _groupKey = "mandatoryParams";
    static constexpr std::string _paramListName = "list";
};
